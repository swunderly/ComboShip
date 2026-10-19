"""OoTMM's Master Quest scene and room overrides on the user's own extracted vanilla OoT archive (MQ port unit U4).

combo/mq/MqRuntime.h plans which commands of a vanilla scene or room header the bundled MQ data replaces, as OoTMM's
src/oot/mq.c rewrites them (mq.c:300-411), and patches copies of the Fire Temple and Ganon's Castle collision
(mq.c:142-218). This test reads the command lists of the 12 dungeon scenes and their 220 rooms, and the Fire and
Ganon collision polygons and surface types, from the user's oot.o2r with SoH's binary scene format
(tools/ootmm/export_oot_wallmaster_edges.py scene_commands, soh/soh/resource/importer/CollisionHeaderFactory.cpp).
It writes them to a fixture in the build directory, never the repository, deleted after the run, and runs
mq_runtime_plan_tests --archive-fixture on it, which checks that:
- each scene replaces exactly its 0x0E, the path dungeons' 0x0D, Ice Cavern's 0x18 and Fire's and Ganon's 0x03;
- each room replaces exactly its 0x01 and 0x0B, and the lists they then point at are the bins';
- a vanilla load replaces nothing;
- the patched Fire collision differs from vanilla in exactly 553 polygons and 33 surface types and Ganon's in its
  16 polygons, each as mq.c writes it, and the vanilla lists are untouched.
Local only: the archive comes from --archive, COMBO_OOT_ARCHIVE, or the first oot.o2r under a local-roms folder
beside the checkout; without one, or for an archive other than NTSC-U 1.0 (the only version the bundled data is
verified against), the test reports SKIPPED.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--exe', type=Path, required=True, help='mq_runtime_plan_tests built by CMake')
p.add_argument('--archive', default='')
a = p.parse_args()
root = a.source.resolve()
sys.path.insert(0, str(root / 'tools/ootmm'))
import export_oot_wallmaster_edges as scenes_tool  # noqa: E402

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

# The 12 MQ dungeons in OoTMM order (mq.c:93-124), as scene_table.h folders (ydan_scene -> ydan).
scene_table = (root / 'soh/include/tables/scene_table.h').read_text(encoding='utf-8')
scene_folders = {int(number, 16): args.split(',')[0].strip() for number, args in
                 re.findall(r'/\* (0x[0-9A-F]+) \*/ DEFINE_SCENE\(([^)]*)\)', scene_table)}
FOLDERS = [scene_folders[scene][:-len('_scene')] for scene in (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0B, 0x0D)]
FIRE, GANON = 4, 11
versions = {int(value, 16): name for name, value in
            re.findall(r'#define (OOT_\w+) (0x[0-9A-Fa-f]+)', (root / 'soh/soh/GameVersions.h').read_text())}


def command_ids(data):
    found = []
    scenes_tool.scene_commands(data, found)
    return [command for command, _ in found]


def collision(data):
    """Every polygon field and the surface types (data[0], data[1]), read as CollisionHeaderFactory.cpp does."""
    reader = scenes_tool.ResourceReader(data)
    reader.take('6h')
    vertices = reader.take('i')
    reader.at += vertices * 6
    polys = [reader.take('HHHHhhhh') for _ in range(reader.take('I'))]
    surfaces = []
    for _ in range(reader.take('I')):
        data1 = reader.take('I')
        surfaces.append((reader.take('I'), data1))
    return polys, surfaces


with zipfile.ZipFile(archive) as z:
    version_entry = z.read('version')
    version = int.from_bytes(version_entry[1:5], 'big' if version_entry[0] == 1 else 'little')
    name = versions.get(version, f'0x{version:08X}')
    if name != 'OOT_NTSC_US_10':
        print(f'SKIPPED: {archive} is {name}; bundled MQ is verified only against OOT_NTSC_US_10')
        sys.exit(0)
    names = set(z.namelist())
    lines = []
    for dungeon, folder in enumerate(FOLDERS):
        base = f'scenes/nonmq/{folder}_scene/'
        scene = z.read(base + folder + '_scene')
        ids = command_ids(scene)
        lines.append(f'scene {dungeon} {len(ids)} ' + ' '.join(map(str, ids)))
        rooms = sorted(int(m[1]) for entry in names for m in
                       [re.fullmatch(re.escape(base + folder) + r'_room_(\d+)', entry)] if m)
        for room in rooms:
            ids = command_ids(z.read(f'{base}{folder}_room_{room}'))
            lines.append(f'room {dungeon} {room} {len(ids)} ' + ' '.join(map(str, ids)))
        if dungeon in (FIRE, GANON):
            found = []
            scenes_tool.scene_commands(scene, found)
            polys, surfaces = collision(z.read(next(detail for command, detail in found if command == 0x03)))
            lines.append(f'polys {dungeon} {len(polys)}')
            lines += [' '.join(map(str, poly)) for poly in polys]
            lines.append(f'surfaces {dungeon} {len(surfaces)}')
            lines += [f'{data0} {data1}' for data0, data1 in surfaces]

work = a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
fixture = work / 'vanilla_commands_and_collision.txt'
fixture.write_text('\n'.join(lines) + '\n', encoding='utf-8')
try:
    result = subprocess.run([str(a.exe), '--archive-fixture', str(fixture)], capture_output=True, text=True,
                            timeout=300)
finally:
    # The fixture holds data read from the user's ROM; it does not outlive the run.
    fixture.unlink()
print(result.stdout + result.stderr, end='')
sys.exit(result.returncode)
