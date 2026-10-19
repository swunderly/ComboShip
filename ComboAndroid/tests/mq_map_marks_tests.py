"""OoTMM's Master Quest minimap and pause-map marks for a dungeon that is MQ from the bundled data (MQ port unit U5).

OoTMM writes over the overlay tables it has just loaded: mq_maps.bin over the whole pause-map table on every pause in
an MQ dungeon, with every set vertex pointer moved to the overlay's chest vertices (src/oot/mq.c:452-477), and the
dungeon's block of mq_scenes.bin over the start of its vanilla minimap marks on every map-mark load (mq.c:479-515).
The production z_map_mark_data.c and z_lmap_mark_data.c are compiled whole, with the glue (ComboMqMaps.cpp,
combo/mq/MqMaps.h), the decoder and the embedded bins, on stub headers that hold SoH's own map-mark structs and
macros; MapMark_Init and PauseMapMark_Init are extracted verbatim. Checks:
- the bundled tables hold the bins exactly, as read by mq_bins.py (an independent reader): every pause record's fields,
  each set vertex pointer at the chest vertices, and each dungeon's minimap block over a copy of its vanilla table, so
  Jabu-Jabu's entry 16 stays vanilla and the Ice Cavern ends on its own last entry;
- they draw what SoH's MQ tables draw except OoTMM's two Fire Temple chests (pause floor 17, minimap room 3), and no
  draw walk runs off a dungeon's entries or the table;
- the vanilla and SoH MQ tables stay unchanged byte for byte, the fill refuses a short table and never writes past
  one, and each bundled table is filled once;
- MapMark_Init and PauseMapMark_Init pick the bundled tables only for a bundled-MQ dungeon, SoH's MQ tables for an
  archive-MQ one, and the vanilla tables otherwise.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess
import sys
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--blobs', type=Path, required=True, help='CMake\'s generated combo_mq dir (ComboMqBlobs.h/.cpp)')
args = parser.parse_args()
source, work, blobs = args.source.resolve(), args.build_dir.resolve(), args.blobs.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import mq_bins  # noqa: E402

failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def without_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def function(text, signature):
    """The definition that starts with `signature`, up to its closing brace."""
    start = hostcxx.find(text, signature)
    brace = text.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def read(path):
    return (source / path).read_text(encoding='utf-8')


MAP_DATA = 'soh/src/overlays/misc/ovl_map_mark_data/z_map_mark_data.c'
PAUSE_DATA = 'soh/src/overlays/misc/ovl_kaleido_scope/z_lmap_mark_data.c'
map_data, pause_data = read(MAP_DATA), read(PAUSE_DATA)

# Stub headers: SoH's own map-mark structs and macros, copied out of its headers.
macros = read('soh/include/macros.h')
array_count = re.search(r'^#define ARRAY_COUNT\(arr\) .*$', macros, re.M).group(0)
vtx_macro = re.search(r'^#define VTX\(.*$', macros, re.M).group(0)
map_mark_h = re.sub(r'^#include <libultraship/libultra\.h>\n', '', read('soh/include/z64map_mark.h'), flags=re.M)
z64 = read('soh/include/z64.h')
pause_structs = z64[z64.index('#define PAUSE_MAP_MARK_NONE'):]
pause_structs = pause_structs[:pause_structs.index('PauseMapMarksData[3];') + len('PauseMapMarksData[3];')]
(work / 'z64.h').write_text(f'''#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;
// libultraship's Vtx (gbi.h); VTX initialises its first member.
typedef struct {{ short ob[3]; unsigned short flag; short tc[2]; unsigned char cn[4]; }} Vtx_t;
typedef union {{ Vtx_t v; long long int force_structure_alignment; }} Vtx;
{map_mark_h}
{pause_structs}
struct PlayState {{ s16 sceneNum; }};
''', encoding='utf-8')
(work / 'global.h').write_text(f'''#pragma once
#include "z64.h"
{array_count}
{vtx_macro}
extern "C" uint32_t ResourceMgr_IsGameMasterQuest(void);
extern "C" uint32_t ResourceMgr_GameUsesBundledMq(void);
extern PauseMapMarksData* gLoadedPauseMarkDataTable;
''', encoding='utf-8')
(work / 'z_kaleido_scope.h').write_text('#pragma once\n#include "global.h"\n', encoding='utf-8')
# The two data files, unchanged; beside the stubs, so their own includes find them.
(work / 'z_map_mark_data.c').write_text(map_data, encoding='utf-8')
(work / 'z_lmap_mark_data.c').write_text(pause_data, encoding='utf-8')

# MapMark_Init and PauseMapMark_Init, verbatim.
map_mark = without_comments(read('soh/src/code/z_map_mark.c'))
pause_mark = without_comments(read('soh/src/overlays/misc/ovl_kaleido_scope/z_lmap_mark.c'))
loaded = 'static MapMarkData** sLoadedMarkDataTable;'
check(loaded in map_mark, 'z_map_mark.c: sLoadedMarkDataTable is missing')
extracted = loaded + '\n' + function(map_mark, 'void MapMark_Init(PlayState* play)') + \
    function(pause_mark, 'void PauseMapMark_Init(PlayState* play)')


def table_names(text, table):
    body = text[text.index(table + '[] = {') + len(table) + 6:]
    return re.findall(r'\w+', body[:body.index('}')])


vanilla_names = table_names(map_data, 'gMapMarkDataTableVanilla')
mq_names = table_names(map_data, 'gMapMarkDataTableMq')
check(len(vanilla_names) == len(mq_names) == 10, f'minimap tables: {vanilla_names} {mq_names}')
pause_tables = ['gPauseMapMarkDataTable', 'gPauseMapMarkDataTableMasterQuest']
snapshot = vanilla_names + mq_names + ['gMapMarkDataTableVanilla', 'gMapMarkDataTableMq'] + pause_tables + [
    'sMarkChestVtx', 'sMarkBossVtx']

code = r'''
#include "global.h"
#include "z_map_mark_data.c"
#include "z_lmap_mark_data.c"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

PauseMapMarksData* gLoadedPauseMarkDataTable;
uint32_t gGameplayMq, gBundledMq;
extern "C" uint32_t ResourceMgr_IsGameMasterQuest(void) { return gGameplayMq; }
extern "C" uint32_t ResourceMgr_GameUsesBundledMq(void) { return gBundledMq; }
'''
code += extracted
code += '#define VANILLA_MINIMAPS(X) ' + ' '.join(f'X({n})' for n in vanilla_names) + '\n'
code += '#define MQ_MINIMAPS(X) ' + ' '.join(f'X({n})' for n in mq_names) + '\n'
code += '#define SNAPSHOT(X) ' + ' '.join(f'X({n})' for n in snapshot) + '\n'
code += r'''
void Expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        std::exit(1);
    }
}

template <class T, size_t N> std::vector<unsigned char> Bytes(const T (&array)[N]) {
    const unsigned char* at = reinterpret_cast<const unsigned char*>(array);
    return std::vector<unsigned char>(at, at + sizeof(array));
}
std::vector<std::vector<unsigned char>> Snapshot() {
    std::vector<std::vector<unsigned char>> out;
#define ADD(name) out.push_back(Bytes(name));
    SNAPSHOT(ADD)
#undef ADD
    return out;
}
template <class T> bool AllBytes(const T* at, size_t size, unsigned char value) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(at);
    for (size_t i = 0; i < size; i++)
        if (bytes[i] != value)
            return false;
    return true;
}

// The fill refuses what does not fit and never writes past the table it is given.
void CheckBounds() {
    const unsigned char canary = 0xa5;
    MapMarkData rooms[64];
    // Ice Cavern: 12 MQ entries.
    memset(rooms, canary, sizeof(rooms));
    Expect(ComboMq_OverlayMinimapMarks(9, rooms, 12), "Ice Cavern fills 12 entries");
    Expect(AllBytes(&rooms[12], sizeof(rooms) - 12 * sizeof(MapMarkData), canary), "nothing past entry 12 is written");
    Expect(!AllBytes(&rooms[11], sizeof(MapMarkData), canary), "entry 11 is written");
    memset(rooms, canary, sizeof(rooms));
    Expect(!ComboMq_OverlayMinimapMarks(9, rooms, 11), "Ice Cavern's 12 entries do not fit 11");
    Expect(AllBytes(rooms, sizeof(rooms), canary), "a refused fill writes nothing");
    // Jabu-Jabu: 16 MQ entries, so its 17th stays.
    Expect(ComboMq_OverlayMinimapMarks(2, rooms, 17), "Jabu-Jabu fills 17 entries");
    Expect(AllBytes(&rooms[16], sizeof(rooms) - 16 * sizeof(MapMarkData), canary), "Jabu-Jabu writes 16 entries");
    memset(rooms, canary, sizeof(rooms));
    for (int dungeon : { -1, 10, 11, 12 }) {
        Expect(!ComboMq_OverlayMinimapMarks(dungeon, rooms, 64), "no minimap block outside dungeons 0-9");
        Expect(AllBytes(rooms, sizeof(rooms), canary), "a dungeon without a block writes nothing");
    }
    Expect(!ComboMq_OverlayMinimapMarks(0, rooms, -1), "a negative room count is refused");
    Expect(!ComboMq_OverlayMinimapMarks(0, NULL, 64), "a missing table is refused");

    static PauseMapMarksData floors[35];
    memset(floors, canary, sizeof(floors));
    Expect(!ComboMq_FillPauseMapMarks(floors, 33, sMarkChestVtx), "33 floors are refused");
    Expect(!ComboMq_FillPauseMapMarks(floors, 35, sMarkChestVtx), "35 floors are refused");
    Expect(!ComboMq_FillPauseMapMarks(floors, 34, NULL), "a missing vertex list is refused");
    Expect(!ComboMq_FillPauseMapMarks(NULL, 34, sMarkChestVtx), "a missing table is refused");
    Expect(AllBytes(floors, sizeof(floors), canary), "a refused pause fill writes nothing");
    Expect(ComboMq_FillPauseMapMarks(floors, 34, sMarkChestVtx), "34 floors fill");
    Expect(AllBytes(&floors[34], sizeof(floors[34]), canary), "nothing past floor 33 is written");
}

void CheckSelection() {
    PlayState play = { 0 };
    // Vanilla and archive MQ: SoH's tables, and the bundled tables are never filled.
    gGameplayMq = 0;
    gBundledMq = 0;
    MapMark_Init(&play);
    PauseMapMark_Init(&play);
    Expect(sLoadedMarkDataTable == gMapMarkDataTableVanilla, "a vanilla dungeon keeps the vanilla minimap marks");
    Expect(gLoadedPauseMarkDataTable == gPauseMapMarkDataTable, "a vanilla dungeon keeps the vanilla pause marks");
    gGameplayMq = 1;
    MapMark_Init(&play);
    PauseMapMark_Init(&play);
    Expect(sLoadedMarkDataTable == gMapMarkDataTableMq, "an archive-MQ dungeon keeps SoH's MQ minimap marks");
    Expect(gLoadedPauseMarkDataTable == gPauseMapMarkDataTableMasterQuest,
           "an archive-MQ dungeon keeps SoH's MQ pause marks");
    for (int d = 0; d < ARRAY_COUNT(sMapMarkDataTableBundledMq); d++)
        Expect(AllBytes(sMapMarkDataTableBundledMq[d], sMapMarkDataTableBundledMqRooms[d] * sizeof(MapMarkData), 0),
               "the bundled minimap table is not filled outside bundled MQ");
    Expect(AllBytes(sPauseMapMarkDataTableBundledMq, sizeof(sPauseMapMarkDataTableBundledMq), 0),
           "the bundled pause table is not filled outside bundled MQ");

    // Bundled MQ: OoTMM's tables.
    gBundledMq = 1;
    MapMark_Init(&play);
    PauseMapMark_Init(&play);
    Expect(sLoadedMarkDataTable == sMapMarkDataTableBundledMq, "a bundled-MQ dungeon shows OoTMM's minimap marks");
    Expect(gLoadedPauseMarkDataTable == sPauseMapMarkDataTableBundledMq,
           "a bundled-MQ dungeon shows OoTMM's pause marks");

    // Filled once: a later Init finds the table as it was left.
    MapMarkIconData& probe = sMapMarkDataTableBundledMq[4][0][0];
    PauseMapMarkData& pauseProbe = sPauseMapMarkDataTableBundledMq[17][0];
    const MapMarkIconData savedIcon = probe;
    const PauseMapMarkData savedMark = pauseProbe;
    probe.count = 99;
    pauseProbe.count = 99;
    MapMark_Init(&play);
    PauseMapMark_Init(&play);
    Expect(sLoadedMarkDataTable == sMapMarkDataTableBundledMq && probe.count == 99, "the minimap table is filled once");
    Expect(gLoadedPauseMarkDataTable == sPauseMapMarkDataTableBundledMq && pauseProbe.count == 99,
           "the pause table is filled once");
    probe = savedIcon;
    pauseProbe = savedMark;
}

std::string Points(const MapMarkIconData& icon) {
    std::string out = "[";
    for (int p = 0; p < 12; p++)
        out += std::string(p ? "," : "") + "[" + std::to_string(icon.points[p].chestFlag) + "," +
               std::to_string(icon.points[p].x) + "," + std::to_string(icon.points[p].y) + "]";
    return out + "]";
}
std::string Minimap(const MapMarkData* table, size_t rooms) {
    std::string out = "[";
    for (size_t room = 0; room < rooms; room++) {
        out += room ? ",[" : "[";
        for (int k = 0; k < 3; k++)
            out += std::string(k ? "," : "") + "[" + std::to_string(table[room][k].markType) + "," +
                   std::to_string(table[room][k].count) + "," + Points(table[room][k]) + "]";
        out += "]";
    }
    return out + "]";
}
uint32_t Bits(f32 value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
int VtxCode(const Vtx* vtx) {
    return vtx == NULL ? 0 : vtx == sMarkChestVtx ? 1 : vtx == sMarkBossVtx ? 2 : -1;
}
std::string Pause(const PauseMapMarksData* table, size_t floors) {
    std::string out = "[";
    for (size_t floor = 0; floor < floors; floor++) {
        out += floor ? ",[" : "[";
        for (int k = 0; k < 3; k++) {
            const PauseMapMarkData& mark = table[floor][k];
            out += std::string(k ? "," : "") + "[" + std::to_string(mark.markType) + "," +
                   std::to_string(mark.unk_04) + "," + std::to_string(VtxCode(mark.vtx)) + "," +
                   std::to_string(mark.vtxCount) + "," + std::to_string(mark.count) + ",[";
            for (int p = 0; p < 12; p++)
                out += std::string(p ? "," : "") + "[" + std::to_string(mark.points[p].chestFlag) + "," +
                       std::to_string(Bits(mark.points[p].x)) + "," + std::to_string(Bits(mark.points[p].y)) + "]";
            out += "]]";
        }
        out += "]";
    }
    return out + "]";
}

int main() {
    const auto before = Snapshot();
    Expect(sizeof(sMarkBossVtx) == sizeof(sMarkChestVtx) && memcmp(sMarkBossVtx, sMarkChestVtx, sizeof(sMarkChestVtx)) == 0,
           "OoTMM draws boss marks with the chest vertices, which must equal the boss vertices");
    CheckBounds();
    CheckSelection();
    Expect(ComboMq_GetMapMarkDataTable() == sMapMarkDataTableBundledMq &&
           ComboMq_GetPauseMapMarkDataTable() == sPauseMapMarkDataTableBundledMq, "the getters return the tables");
    Expect(Snapshot() == before, "the vanilla and SoH MQ tables are unchanged");

    std::string out = "{\"bundledMinimap\":[";
    for (int d = 0; d < ARRAY_COUNT(sMapMarkDataTableBundledMq); d++)
        out += (d ? "," : "") + Minimap(sMapMarkDataTableBundledMq[d], sMapMarkDataTableBundledMqRooms[d]);
    out += "],\"vanillaMinimap\":[";
    bool first = true;
#define DUMP(name) out += (first ? "" : ",") + Minimap(name, ARRAY_COUNT(name)); first = false;
    VANILLA_MINIMAPS(DUMP)
    out += "],\"sohMqMinimap\":[";
    first = true;
    MQ_MINIMAPS(DUMP)
#undef DUMP
    out += "],\"bundledPause\":" + Pause(sPauseMapMarkDataTableBundledMq, ARRAY_COUNT(sPauseMapMarkDataTableBundledMq));
    out += ",\"sohMqPause\":" + Pause(gPauseMapMarkDataTableMasterQuest, ARRAY_COUNT(gPauseMapMarkDataTableMasterQuest));
    out += "}";
    std::puts(out.c_str());
    return 0;
}
'''
harness = work / 'mq_map_marks.cpp'
harness.write_text(code, encoding='utf-8')
exe = hostcxx.build(args.compiler, harness, std='c++17', warnings=False, defines=('COMBO_BUILD',),
                    includes=[work, source / 'soh', source / 'combo', blobs],
                    sources=[source / 'soh/soh/Enhancements/randomizer/ComboMqMaps.cpp',
                             source / 'combo/mq/ComboMqData.cpp', blobs / 'ComboMqBlobs.cpp'],
                    msvc=('/utf-8', '/bigobj'))
result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
if not check(result.returncode == 0, 'the map-mark harness failed:\n' + result.stdout[-2000:] + result.stderr):
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
dump = json.loads(result.stdout)

rooms_bin, scenes_bin, maps_bin = mq_bins.load(source / 'combo/mq/data')
decoded = mq_bins.decode(rooms_bin, scenes_bin, maps_bin)
NONE, CHEST = -1, 0
FIRE, FIRE_FLOOR, FIRE_ROOM, JABU, ICE = 4, 17, 3, 2, 9

# Pause marks: mq_maps.bin exactly, every set vertex pointer at the chest vertices (mq.c:467-476).
bundled_pause, soh_pause = dump['bundledPause'], dump['sohMqPause']
check(len(bundled_pause) == len(decoded['pause']) == 34, f'{len(bundled_pause)} bundled pause floors')
for floor, (ours, theirs) in enumerate(zip(bundled_pause, decoded['pause'])):
    for record, (mark, expected) in enumerate(zip(ours, theirs)):
        mark_type, unk, has_vtx, vtx_count, count, points = expected
        check(mark == [mark_type, unk, 1 if has_vtx else 0, vtx_count, count, points],
              f'pause floor {floor} record {record}: {mark} is not mq_maps.bin\'s {expected}')

# Minimap marks: each block's whole entries over a copy of the dungeon's vanilla table (mq.c:497-514).
bundled_minimap, vanilla_minimap = dump['bundledMinimap'], dump['vanillaMinimap']
blocks = {scene['dungeon']: scene['minimap'] for scene in decoded['scenes']}
check(len(bundled_minimap) == len(vanilla_minimap) == 10, 'the bundled minimap table covers dungeons 0-9')
for dungeon, (ours, vanilla) in enumerate(zip(bundled_minimap, vanilla_minimap)):
    name, block = mq_bins.DUNGEONS[dungeon], blocks[dungeon]
    check(len(ours) == len(vanilla) and 0 < len(block) <= len(vanilla),
          f'{name}: {len(ours)} bundled, {len(vanilla)} vanilla and {len(block)} MQ entries')
    check(ours[:len(block)] == block, f'{name}: the bundled entries are not the MQ block')
    check(ours[len(block):] == vanilla[len(block):], f'{name}: the entries past the MQ block are not vanilla')
check(len(blocks[JABU]) == 16 and len(bundled_minimap[JABU]) == 17 and
      bundled_minimap[JABU][16] == vanilla_minimap[JABU][16] and bundled_minimap[JABU][16][0][0] == NONE,
      'Jabu-Jabu minimap 16 must stay the vanilla NONE entry, as OoTMM leaves it')
check(len(bundled_minimap[ICE]) == len(blocks[ICE]) == 12 and bundled_minimap[ICE][11] == blocks[ICE][11],
      'the Ice Cavern must end on its own 12th MQ entry')


def draw(table, entry, pause):
    """What the draw loop draws for an entry: records until NONE, each with its first `count` points. The boss and
    chest vertex lists are equal (checked in the harness), so only whether a record has one is compared."""
    out = []
    for at in range(entry * 3, len(table) * 3):
        record = table[at // 3][at % 3]
        if record[0] == NONE:
            return out
        if pause:
            mark, _, vtx, vtx_count, count, points = record
            if count > 0:
                out.append((mark, vtx in (1, 2) and vtx_count == 4, tuple(tuple(p) for p in points[:count])))
        else:
            mark, count, points = record
            if count > 0:
                out.append((mark, tuple(tuple(p) for p in points[:count])))
    return out + ['runs off the table']


# Drawn like SoH's MQ tables, except OoTMM's two Fire Temple chests (make_mq.ts:285-294, 401-409).
added = (0, mq_bins.float_bits(46.5), mq_bins.float_bits(-17.5))
for floor in range(34):
    ours, theirs = draw(bundled_pause, floor, True), draw(soh_pause, floor, True)
    check('runs off the table' not in ours, f'pause floor {floor}: the bundled walk runs off the table')
    check(all(item[1] for item in ours if not isinstance(item, str)), f'pause floor {floor}: a drawn mark has no quad')
    if floor == FIRE_FLOOR:
        expected = [(theirs[0][0], theirs[0][1], theirs[0][2] + (added,))] + theirs[1:]
        check(theirs and theirs[0][0] == CHEST and len(theirs[0][2]) == 5 and ours == expected,
              f'Fire pause floor 17: bundled {ours} is not SoH\'s {theirs} plus chest {added}')
    else:
        check(ours == theirs, f'pause floor {floor}: bundled {ours} != SoH MQ {theirs}')
for dungeon, (ours_table, soh_table) in enumerate(zip(bundled_minimap, dump['sohMqMinimap'])):
    check(len(ours_table) == len(soh_table), f'{mq_bins.DUNGEONS[dungeon]}: {len(ours_table)} bundled minimaps, '
          f'{len(soh_table)} in SoH\'s MQ table')
    for room in range(len(ours_table)):
        ours, theirs = draw(ours_table, room, False), draw(soh_table, room, False)
        check('runs off the table' not in ours, f'{mq_bins.DUNGEONS[dungeon]} minimap {room}: the walk runs off')
        if (dungeon, room) == (FIRE, FIRE_ROOM):
            check(theirs == [] and ours == [(CHEST, ((0, 72, 60),))],
                  f'Fire minimap 3: bundled {ours}, SoH MQ {theirs}; OoTMM adds chest 0 at (72, 60)')
        else:
            check(ours == theirs, f'{mq_bins.DUNGEONS[dungeon]} minimap {room}: bundled {ours} != SoH MQ {theirs}')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: the bundled-MQ minimap and pause tables hold the bins exactly over untouched vanilla tables, draw like '
      'SoH\'s MQ tables except OoTMM\'s two Fire Temple chests, and are picked only for a bundled-MQ dungeon')
