"""Check a Master Quest walk's developer room log against OoTMM's bundled MQ bins.

    python tools/mq_walk_compare.py native.log [more logs] [--require-all] [--data combo/mq/data]

With the developer CVar gCombo.Debug.MqRoomLog set to 1, every room load of the 12 MQ-capable dungeons logs one
"[ComboShip MQ room]" line (combo/mq/MqRoomLog.h) with the lists the game holds once the room's commands have run.
This reads those lines from any log (native.log, the logs/ folder, logcat), ignoring the sink's prefix and every
other line, and checks each against the bins (combo/mq/data, read by ComboAndroid/tests/mq_bins.py):

- quest=mq: the room's actors (id and params, the six broken actors removed as id ffff) and objects equal its
  entry in mq_rooms.bin, or stay vanilla for a room without one. The ten rooms the bins give no actors log
  actors=mq:0 too: their vanilla header has no actor list to rewrite, and they spawn none. The scene's transition
  actors and paths are the MQ ones with the bins' counts (paths only where the dungeon has MQ paths); Fire Temple's
  and Ganon's Castle's collision is the patched copy, changing exactly as many polygons and surface types as the
  patches name, and every other dungeon's is vanilla. In Jabu-Jabu and Ice Cavern a cutscene layer (4 and up) runs the
  vanilla alternate header, as in OoTMM, so vanilla lists there are noted, not failed.
- quest=vanilla (a dungeon the seed left vanilla, or a vanilla save): nothing is MQ.

It prints each failure, which rooms of each dungeon were seen as child (layers 0-1) and adult (2-3), and exits 1 on
any failure, on a line it cannot read, or with --require-all on a bins room never seen as MQ.
"""
from pathlib import Path
import argparse
import re
import sys

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / 'ComboAndroid/tests'))
import mq_bins  # noqa: E402  (the reader the host tests check against combo/mq/ComboMqData.cpp)

TAG = '[ComboShip MQ room]'
KEYS = ('scene', 'room', 'dungeon', 'quest', 'layer', 'actors', 'objects', 'transitions', 'paths', 'collision',
        'actorList', 'objectList')
JABU, FIRE, ICE, GANON = 2, 4, 9, 11
# src/oot/mq.c kMqBrokenActors: (dungeon, room, actor index), removed as id 0xffff.
BROKEN_ACTORS = {(1, 0x9, 0x0), (1, 0xa, 0x6), (2, 0x7, 0x1), (6, 0x1, 0x2), (6, 0x1, 0x3), (6, 0x17, 0x0)}
# src/oot/mq.c:157-174: the Ganon's Castle polygons patched on every MQ load.
GANON_POLYS = set(range(2403, 2411)) | set(range(2473, 2481))
ANSI = re.compile(r'\x1b\[[0-9;]*m')


def dungeon_from_scene(scene):
    """mq.c mqDungeonId, as combo/mq/ComboMqData.h DungeonFromScene."""
    if 0x00 <= scene <= 0x09:
        return scene
    return {0x0B: 10, 0x0D: 11}.get(scene, -1)


def expected(data):
    rooms = {(r['dungeon'], r['room']): r for r in data['rooms']}
    scenes = {s['dungeon']: s for s in data['scenes']}
    return rooms, scenes


def expected_actors(room):
    return [(0xffff if (room['dungeon'], room['room'], i) in BROKEN_ACTORS else actor[0] & 0xffff, actor[7] & 0xffff)
            for i, actor in enumerate(room['actors'])]


def expected_collision(dungeon, scene):
    """(polygons, surface types) the patched copy changes, or None for a dungeon whose collision stays vanilla."""
    polys = {patch[0] for patch in scene['polyPatches']} | (GANON_POLYS if dungeon == GANON else set())
    surfaces = {patch[0] for patch in scene['surfacePatches']}
    return (len(polys), len(surfaces)) if polys or surfaces else None


def parse_source(value, what):
    source, _, count = value.partition(':')
    if source not in ('mq', 'vanilla'):
        raise ValueError(f'{what}={value}')
    return source, int(count) if count else None


def parse_line(text):
    fields = dict(token.partition('=')[::2] for token in text.split())
    missing = [key for key in KEYS if key not in fields]
    if missing:
        raise ValueError('missing ' + ', '.join(missing))
    line = dict(scene=int(fields['scene'], 16), room=int(fields['room']), dungeon=int(fields['dungeon']),
                quest=fields['quest'], layer=int(fields['layer']))
    if line['quest'] not in ('mq', 'vanilla'):
        raise ValueError(f'quest={line["quest"]}')
    for key in ('actors', 'objects', 'transitions', 'paths'):
        line[key] = parse_source(fields[key], key)
    collision = fields['collision'].split(':')
    if collision[0] == 'patched' and len(collision) == 3:
        line['collision'] = ('patched', (int(collision[1]), int(collision[2])))
    elif collision[0] in ('vanilla', 'failed') and len(collision) == 1:
        line['collision'] = (collision[0], None)
    else:
        raise ValueError(f'collision={fields["collision"]}')
    actors = [] if fields['actorList'] == '-' else fields['actorList'].split(',')
    line['actorList'] = [tuple(int(part, 16) for part in actor.split('/')) for actor in actors]
    if any(len(actor) != 2 for actor in line['actorList']):
        raise ValueError(f'actorList={fields["actorList"]}')
    line['objectList'] = [] if fields['objectList'] == '-' else [int(o, 16) for o in fields['objectList'].split(',')]
    if line['actors'][1] != len(line['actorList']) or line['objects'][1] != len(line['objectList']):
        raise ValueError('a count differs from its list')
    return line


def first_difference(got, want):
    for i, (a, b) in enumerate(zip(got, want)):
        if a != b:
            return i
    return min(len(got), len(want))


def check_line(line, rooms, scenes):
    """(failures, notes) for one parsed line."""
    failures, notes = [], []
    dungeon = line['dungeon']
    if dungeon_from_scene(line['scene']) != dungeon or dungeon < 0:
        return [f'scene 0x{line["scene"]:02x} is not dungeon {dungeon}'], notes
    if line['quest'] == 'vanilla':
        for key in ('actors', 'objects', 'transitions', 'paths'):
            if line[key][0] != 'vanilla':
                failures.append(f'{key} are MQ in a vanilla dungeon')
        if line['collision'][0] != 'vanilla':
            failures.append(f'collision is {line["collision"][0]} in a vanilla dungeon')
        return failures, notes

    room, scene = rooms.get((dungeon, line['room'])), scenes[dungeon]
    # Jabu-Jabu and Ice Cavern list a layer-4 alternate header; it runs with its vanilla lists (mq.c rewrites only the
    # top-level header), and Jabu-Jabu's scene header stops after it.
    alternate = dungeon in (JABU, ICE) and line['layer'] >= 4

    def expect_mq(key, matches, what):
        source, _ = line[key]
        if source == 'vanilla':
            if alternate:
                notes.append(f'{key} vanilla on cutscene layer {line["layer"]}')
            else:
                failures.append(f'{key} are vanilla, expected the MQ {what}')
        elif not matches():
            failures.append(f'{key} differ from the MQ {what}')

    def same(got, want, what):
        if got == want:
            return True
        i = first_difference(got, want)
        failures.append(f'{what}: {len(got)} entries, the bins have {len(want)}; first difference at index {i}')
        return False

    if room is None:
        for key in ('actors', 'objects'):
            if line[key][0] != 'vanilla':
                failures.append(f'{key} are MQ, but the bins have no room {line["room"]}')
    else:
        expect_mq('actors', lambda: same(line['actorList'], expected_actors(room), 'actorList'), 'room actors')
        expect_mq('objects', lambda: same(line['objectList'], [o & 0xffff for o in room['objects']], 'objectList'),
                  'room objects')
    expect_mq('transitions', lambda: line['transitions'][1] == len(scene['transitions']),
              f'transition actors ({len(scene["transitions"])})')
    if scene['hasPaths']:
        expect_mq('paths', lambda: line['paths'][1] == len(scene['paths']), f'paths ({len(scene["paths"])})')
    elif line['paths'][0] != 'vanilla':
        failures.append('paths are MQ, but the dungeon has no MQ paths')
    want = expected_collision(dungeon, scene)
    kind, counts = line['collision']
    if want is None and kind != 'vanilla':
        failures.append(f'collision is {kind}, but the dungeon has no MQ collision patches')
    elif want is not None and kind != 'patched':
        failures.append(f'collision is {kind}, expected the copy patched at {want[0]} polygons, {want[1]} surface types')
    elif want is not None and counts != want:
        failures.append(f'collision changes {counts[0]} polygons and {counts[1]} surface types, '
                        f'the patches name {want[0]} and {want[1]}')
    return failures, notes


def read_lines(paths):
    for path in paths:
        with open(path, encoding='utf-8', errors='replace') as log:
            for number, raw in enumerate(log, 1):
                at = raw.find(TAG)
                if at >= 0:
                    yield f'{path}:{number}', ANSI.sub('', raw[at + len(TAG):]).strip()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('logs', nargs='+', type=Path)
    parser.add_argument('--data', type=Path, default=REPO / 'combo/mq/data', help='the folder of the three bins')
    parser.add_argument('--require-all', action='store_true', help='fail unless every bins room was seen as MQ')
    args = parser.parse_args(argv)
    rooms, scenes = expected(mq_bins.decode(*mq_bins.load(args.data)))

    failures, notes, lines, vanilla = [], [], 0, 0
    seen = {}  # (dungeon, room) -> set of ages
    for where, text in read_lines(args.logs):
        lines += 1
        try:
            line = parse_line(text)
        except ValueError as error:
            failures.append(f'{where}: cannot read the line ({error})')
            continue
        bad, note = check_line(line, rooms, scenes)
        name = f'{where}: {mq_bins.DUNGEONS[line["dungeon"]] if 0 <= line["dungeon"] < 12 else "?"} room {line["room"]}'
        failures += [f'{name}: {message}' for message in bad]
        notes += [f'{name}: {message}' for message in note]
        if line['quest'] == 'vanilla':
            vanilla += 1
        elif not bad:
            age = 'child' if line['layer'] in (0, 1) else 'adult' if line['layer'] in (2, 3) else 'cutscene'
            seen.setdefault((line['dungeon'], line['room']), set()).add(age)

    print(f'{lines} room lines: {len(seen)} distinct MQ rooms matched the bins, {vanilla} vanilla lines')
    for dungeon, name in enumerate(mq_bins.DUNGEONS):
        total = sorted(room for (d, room) in rooms if d == dungeon)
        child = [room for room in total if 'child' in seen.get((dungeon, room), ())]
        adult = [room for room in total if 'adult' in seen.get((dungeon, room), ())]
        unseen = [room for room in total if (dungeon, room) not in seen]
        print(f'  {name}: {len(total) - len(unseen)}/{len(total)} rooms (child {len(child)}, adult {len(adult)})' +
              (f'; not seen: {", ".join(str(room) for room in unseen)}' if unseen else ''))
        if args.require_all and unseen:
            failures.append(f'{name}: rooms never seen as MQ: {", ".join(str(room) for room in unseen)}')
    for note in notes:
        print('NOTE:', note)
    for failure in failures:
        print('FAIL:', failure)
    if lines == 0:
        print(f'FAIL: no "{TAG}" lines; was gCombo.Debug.MqRoomLog set to 1?')
        return 1
    print('OK' if not failures else f'{len(failures)} failures')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
