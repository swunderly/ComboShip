"""The bundled OoTMM v32.3 Master Quest bins against the user's own extracted vanilla OoT archive (MQ port unit U2).

OoTMM's src/oot/mq.c patches the vanilla scene and room headers in place, so the bundled data only works where the
vanilla archive has the commands it rewrites. Local only: reads the user's extracted oot.o2r (--archive,
COMBO_OOT_ARCHIVE, or the first oot.o2r under a local-roms folder beside the checkout), parses it with SoH's binary
scene format (tools/ootmm/export_oot_wallmaster_edges.py scene_commands) and is reported as skipped where there is
none. Nothing is copied from it. Checks:
- every MQ room is a vanilla room, and every MQ room with actors or objects lands on a vanilla 0x01 or 0x0B command
  (mq.c:386-393 only rewrites commands that exist); the ten MQ rooms without actors have no vanilla 0x01 either;
- every scene has 0x0E and 0x03; the dungeons with MQ paths have 0x0D, except Ice Cavern, whose 0x18 mq.c:328-339
  turns into the path list;
- Fire Temple collision has 4857 polygons and 68 surface types, all 553 polygon patches change a vanilla polygon and
  fit, and the surface patches are in SoH's data[0], data[1] word order; Ganon's Castle has 5096 polygons and the 16
  polygons mq.c:159-174 patches are type 57 with flags 0;
- alternate-header layers 1-3 are unset in all 12 dungeons, and Jabu-Jabu and Ice Cavern set only layer 4 on the scene
  and on every room, so the MQ lists apply in normal play at both ages.
Bundled MQ is verified only against OoT NTSC-U 1.0: another version's archive is fingerprinted and reported as skipped
with the result. On every host: no ROM or extracted archive is tracked in the repository.
"""
from pathlib import Path
import argparse
import os
import re
import struct
import subprocess
import sys
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--archive', default='')
a = p.parse_args()
root = a.source.resolve()
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(root / 'tools/ootmm'))
import mq_bins  # noqa: E402
import export_oot_wallmaster_edges as scenes_tool  # noqa: E402

# On every host: no ROM or extracted archive is tracked (the user's archive stays local).
listing = subprocess.run(['git', '-C', str(root), 'ls-files'], capture_output=True, text=True)
if listing.returncode == 0:
    tracked = [name for name in listing.stdout.splitlines()
               if name.lower().endswith(('.o2r', '.otr', '.z64', '.n64', '.v64'))]
    if tracked:
        sys.exit(f'FAIL: ROM or archive files are tracked: {tracked}')
else:
    print('Not a git checkout; the tracked-file check was not run')

archive = a.archive or os.environ.get('COMBO_OOT_ARCHIVE', '')
if not archive:
    for folder in (root.parent / 'local-roms', root.parent.parent / 'local-roms'):
        found = sorted(folder.rglob('oot.o2r')) if folder.is_dir() else []
        if found:
            archive = str(found[0])
            break
if not archive or not Path(archive).is_file():
    print('SKIPPED: no extracted OoT archive (pass --archive or set COMBO_OOT_ARCHIVE); this check is local only')
    sys.exit(0)

failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


# The 12 MQ dungeons in OoTMM order, as scene_table.h folders (ydan_scene -> ydan).
scene_table = (root / 'soh/include/tables/scene_table.h').read_text(encoding='utf-8')
scene_folders = {}
for number, args in re.findall(r'/\* (0x[0-9A-F]+) \*/ DEFINE_SCENE\(([^)]*)\)', scene_table):
    scene_folders[int(number, 16)] = args.split(',')[0].strip()
DUNGEON_SCENES = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0B, 0x0D]
FOLDERS = [scene_folders[scene][:-len('_scene')] for scene in DUNGEON_SCENES]
ICE, JABU, FIRE, GANON = 9, 2, 4, 11
# mq.c:159-174: Ganon's Castle polygons OoTMM sets to type 57, flags 4 on every MQ load.
GANON_POLYS = list(range(2403, 2411)) + list(range(2473, 2481))
# The MQ rooms mq_rooms.bin gives no actor list (dungeon, room).
ACTORLESS = [(0, 11), (1, 16), (2, 15), (3, 22), (4, 26), (5, 22), (6, 11), (6, 28), (7, 22), (9, 10)]

versions = {int(value, 16): name for name, value in
            re.findall(r'#define (OOT_\w+) (0x[0-9A-Fa-f]+)', (root / 'soh/soh/GameVersions.h').read_text())}


def collision(data):
    """Polygons (type, flags_vIA) and surface types (data[0], data[1]), read as CollisionHeaderFactory.cpp:17-63."""
    reader = scenes_tool.ResourceReader(data)
    reader.take('6h')
    vertices = reader.take('i')
    reader.at += vertices * 6
    polys = [reader.take('HHHHHHHH')[:2] for _ in range(reader.take('I'))]
    surfaces = []
    for _ in range(reader.take('I')):
        data1 = reader.take('I')
        surfaces.append((reader.take('I'), data1))
    return polys, surfaces


def commands(z, name):
    found = []
    scenes_tool.scene_commands(z.read(name), found)
    return found


rooms, scenes, _ = mq_bins.load(root / 'combo/mq/data')
mq_rooms = {(h['dungeon'], h['room']): h for h in mq_bins.room_headers(rooms)}
mq_scenes = {s['dungeon']: s for s in mq_bins.decode_scenes(scenes)}


def scan(z, names):
    """Every check that reads the archive; problems go to failures."""
    layer4 = {}
    actorless = []
    for dungeon, folder in enumerate(FOLDERS):
        base = f'scenes/nonmq/{folder}_scene/'
        room_ids = sorted(int(m[1]) for name in names for m in [re.fullmatch(re.escape(base + folder) + r'_room_(\d+)',
                                                                              name)] if m)
        check(room_ids == sorted(room for d, room in mq_rooms if d == dungeon),
              f'{folder}: vanilla rooms {room_ids} are not the MQ rooms')
        scene_cmds = commands(z, base + folder + '_scene')
        ids = [command for command, _ in scene_cmds]
        check(0x0E in ids and 0x03 in ids, f'{folder}: scene has no 0x0E or 0x03 for mq.c to rewrite')
        if mq_scenes[dungeon]['hasPaths']:
            if dungeon == ICE:
                check(0x18 in ids and 0x0D not in ids, 'Ice Cavern: MQ paths need its 0x18 and no 0x0D')
            else:
                check(0x0D in ids, f'{folder}: MQ paths but no vanilla 0x0D')
        headers = [(folder + ' scene', detail) for command, detail in scene_cmds if command == 0x18]
        for room in room_ids:
            room_cmds = commands(z, f'{base}{folder}_room_{room}')
            room_ids_present = [command for command, _ in room_cmds]
            header = mq_rooms[(dungeon, room)]
            if header['actorCount']:
                check(0x01 in room_ids_present, f'{folder} room {room}: MQ actors but no vanilla 0x01')
            else:
                actorless.append((dungeon, room, 0x01 in room_ids_present))
            if header['objectCount']:
                check(0x0B in room_ids_present, f'{folder} room {room}: MQ objects but no vanilla 0x0B')
            headers += [(f'{folder} room {room}', detail) for command, detail in room_cmds if command == 0x18]
        for where, layers in headers:
            check(not any(layers[:3]), f'{where}: alternate-header layers 1-3 are set: {layers[:3]}')
        layer4[dungeon] = [(where, len(layers) > 3 and bool(layers[3])) for where, layers in headers]
        collision_path = next(detail for command, detail in scene_cmds if command == 0x03)
        polys, surfaces = collision(z.read(collision_path))
        patches = mq_scenes[dungeon]
        if dungeon == FIRE:
            check((len(polys), len(surfaces)) == (4857, 68), f'Fire collision is {len(polys)}/{len(surfaces)}')
            check(len(patches['polyPatches']) == 553 and len(patches['surfacePatches']) == 33,
                  'Fire patch counts changed')
            check(all(pid < len(polys) and kind < len(surfaces) for pid, kind, _ in patches['polyPatches']) and
                  all(sid < len(surfaces) for sid, _, _ in patches['surfacePatches']), 'Fire patch out of range')
            unchanged = [pid for pid, kind, flags in patches['polyPatches'] if pid < len(polys) and
                         polys[pid][0] == kind and polys[pid][1] >> 13 == flags]
            check(not unchanged, f'Fire polygon patches that change nothing: {unchanged[:8]}')
            # mq.c PATCH_POLY_TYPE writes hi to the first word and lo to the second; SoH keeps the first word in
            # data[0]. MQ moves the vanilla surface types up one slot, so many patches equal a vanilla pair in this
            # order and none in the swapped one.
            vanilla_pairs = set(surfaces)
            straight = sum((hi, lo) in vanilla_pairs for _, hi, lo in patches['surfacePatches'])
            swapped = sum((lo, hi) in vanilla_pairs for _, hi, lo in patches['surfacePatches'])
            check(straight == 32 and swapped == 0, f'Fire surface word order: {straight} straight, {swapped} swapped')
        elif dungeon == GANON:
            check(len(polys) == 5096, f'Ganon collision has {len(polys)} polygons')
            wrong = [pid for pid in GANON_POLYS if pid >= len(polys) or polys[pid][0] != 57 or polys[pid][1] >> 13]
            check(not wrong, f'Ganon polygons not type 57 with flags 0: {wrong}')
        else:
            check(not patches['polyPatches'] and not patches['surfacePatches'], f'{folder}: unexpected patches')

    # The ten MQ rooms without actors have no vanilla 0x01 either, so there is no vanilla actor list left to clear.
    check(actorless == [(d, room, False) for d, room in ACTORLESS],
          f'MQ rooms without actors against vanilla: {actorless}')
    for dungeon, folder in enumerate(FOLDERS):
        flags = layer4[dungeon]
        if dungeon in (JABU, ICE):
            rooms_with_header = {where for where, _ in flags}
            expected = {folder + ' scene'} | {f'{folder} room {room}' for d, room in mq_rooms if d == dungeon}
            check(rooms_with_header == expected and all(set4 for _, set4 in flags),
                  f'{folder}: layer 4 is not set on the scene and every room: {flags}')
        else:
            check(not any(set4 for _, set4 in flags), f'{folder}: alternate headers set: {flags}')


with zipfile.ZipFile(archive) as z:
    version_entry = z.read('version')
    version = int.from_bytes(version_entry[1:5], 'big' if version_entry[0] == 1 else 'little')
    names = z.namelist()
    name = versions.get(version, f'0x{version:08X}')
    if name == 'OOT_NTSC_US_10':
        scan(z, names)
    else:
        try:
            scan(z, names)
        except (KeyError, ValueError, StopIteration, struct.error) as problem:
            failures.append(f'unreadable: {problem!r}')

if name != 'OOT_NTSC_US_10':
    print(f'SKIPPED: {archive} is {name}; bundled MQ is verified only against OOT_NTSC_US_10 '
          f'(fingerprint here: {"matches" if not failures else "; ".join(failures)})')
    sys.exit(0)
if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print(f'PASS: {archive} ({name}) has every vanilla command mq.c rewrites for the 220 MQ rooms and 12 scenes, the '
      'collision mq.c patches, and only layer-4 alternate headers (Jabu-Jabu and Ice Cavern)')
