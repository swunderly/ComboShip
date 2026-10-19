"""An independent reader for OoTMM v32.3's Master Quest bins (combo/mq/data).

Written from OoTMM's packages/generator/scripts/make_mq.ts (the writer) and src/oot/mq.c (the reader), not from
combo/mq/ComboMqData.cpp, so the host tests can compare the two. Every field is big-endian. decode() returns plain
lists in the shape ComboMqDataTests' --dump-json writes:

    rooms:  [{dungeon, room, actors: [[id, x, y, z, rx, ry, rz, params]], objects: [id]}], by dungeon then room
    scenes: [{dungeon, transitions: [[frontRoom, frontEffects, backRoom, backEffects, id, x, y, z, rotY, params]],
              hasPaths, paths: [[[x, y, z]]], minimap: [[[markType, count, [[chestFlag, x, y]] * 12]] * 3],
              polyPatches: [[id, type, flags]], surfacePatches: [[id, hi, lo]]}], by dungeon
    pause:  [[[markType, unk04, hasVtx, vtxCount, count, [[chestFlag, xBits, yBits]] * 12]] * 3] * 34

Signed fields are signed; floats are given as their IEEE-754 bit patterns so the comparison is exact.
"""
from pathlib import Path
import struct

DUNGEONS = ['DT', 'DC', 'JJ', 'Forest', 'Fire', 'Water', 'Spirit', 'Shadow', 'BotW', 'Ice', 'GTG', 'Ganon']
# make_mq.ts PATHS, by dungeon index.
PATH_COUNTS = [0, 0, 0, 0, 4, 2, 5, 0, 2, 5, 0, 6]
MINIMAP_ENTRY = 0x72
PAUSE_FLOORS, PAUSE_RECORDS, PAUSE_RECORD = 34, 3, 0xa4
FILES = ('mq_rooms.bin', 'mq_scenes.bin', 'mq_maps.bin')


def room_headers(rooms):
    count = struct.unpack_from('>I', rooms, 0)[0]
    out = []
    for i in range(count):
        size, offset, dungeon, room, actor_count, actor_offset, object_count, object_offset = struct.unpack_from(
            '>IIBBHHHH', rooms, 0x20 + i * 0x20)
        out.append(dict(size=size, offset=offset, dungeon=dungeon, room=room, actorCount=actor_count,
                        actorOffset=actor_offset, objectCount=object_count, objectOffset=object_offset))
    return out


def scene_headers(scenes):
    count = struct.unpack_from('>I', scenes, 0)[0]
    out = []
    for i in range(count):
        (size, offset, dungeon, pad, transition_count, transition_offset, path_offset, minimap_size, minimap_offset,
         poly_count, poly_offset, surface_count, surface_offset) = struct.unpack_from('>IIBBHHHIIIIII', scenes,
                                                                                    0x40 + i * 0x40)
        out.append(dict(size=size, offset=offset, dungeon=dungeon, transitionCount=transition_count,
                        transitionOffset=transition_offset, pathOffset=path_offset, minimapSize=minimap_size,
                        minimapOffset=minimap_offset, polyCount=poly_count, polyOffset=poly_offset,
                        surfaceCount=surface_count, surfaceOffset=surface_offset))
    return out


def decode_rooms(rooms):
    out = []
    for h in room_headers(rooms):
        base = h['offset']
        actors = [list(struct.unpack_from('>8h', rooms, base + h['actorOffset'] + j * 0x10))
                  for j in range(h['actorCount'])]
        objects = [struct.unpack_from('>h', rooms, base + h['objectOffset'] + j * 2)[0]
                   for j in range(h['objectCount'])]
        out.append(dict(dungeon=h['dungeon'], room=h['room'], actors=actors, objects=objects))
    return sorted(out, key=lambda r: (r['dungeon'], r['room']))


def decode_scenes(scenes):
    out = []
    for h in scene_headers(scenes):
        base = h['offset']
        transitions = [list(struct.unpack_from('>4b6h', scenes, base + h['transitionOffset'] + j * 0x10))
                       for j in range(h['transitionCount'])]
        paths = []
        if h['pathOffset'] != 0xffff:
            for k in range(PATH_COUNTS[h['dungeon']]):
                count, points = struct.unpack_from('>B3xI', scenes, base + h['pathOffset'] + k * 8)
                paths.append([list(struct.unpack_from('>3h', scenes, base + points + p * 6)) for p in range(count)])
        minimap = []
        for e in range(h['minimapSize'] // MINIMAP_ENTRY):
            entry = []
            for k in range(3):
                at = h['minimapOffset'] + e * MINIMAP_ENTRY + k * 0x26
                mark_type, count = struct.unpack_from('>bB', scenes, at)
                points = [list(struct.unpack_from('>bBB', scenes, at + 2 + p * 3)) for p in range(12)]
                entry.append([mark_type, count, points])
            minimap.append(entry)
        polys = [list(struct.unpack_from('>HBB', scenes, h['polyOffset'] + j * 4)) for j in range(h['polyCount'])]
        surfaces = [list(struct.unpack_from('>III', scenes, h['surfaceOffset'] + j * 0x10))
                    for j in range(h['surfaceCount'])]
        out.append(dict(dungeon=h['dungeon'], transitions=transitions, hasPaths=h['pathOffset'] != 0xffff,
                        paths=paths, minimap=minimap, polyPatches=polys, surfacePatches=surfaces))
    return sorted(out, key=lambda s: s['dungeon'])


def decode_maps(maps):
    floors = []
    for f in range(PAUSE_FLOORS):
        records = []
        for k in range(PAUSE_RECORDS):
            at = f * PAUSE_RECORDS * PAUSE_RECORD + k * PAUSE_RECORD
            mark_type, unk, vtx, vtx_count, count = struct.unpack_from('>h2xiIii', maps, at)
            points = [list(struct.unpack_from('>h2xII', maps, at + 0x14 + p * 0xc)) for p in range(12)]
            records.append([mark_type, unk, vtx != 0, vtx_count, count, points])
        floors.append(records)
    return floors


def decode(rooms, scenes, maps):
    return dict(rooms=decode_rooms(rooms), scenes=decode_scenes(scenes), pause=decode_maps(maps))


def load(data_dir):
    """The three files' bytes from a directory, as (rooms, scenes, maps)."""
    return tuple((Path(data_dir) / name).read_bytes() for name in FILES)


def float_bits(value):
    return struct.unpack('>I', struct.pack('>f', value))[0]
