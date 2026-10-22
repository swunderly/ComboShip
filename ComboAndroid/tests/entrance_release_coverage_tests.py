"""C4 release-configuration coverage proof (M1 plan A-10, P1-04), compiled with and without COMBO_ENTRANCE_DEV.

The native tables the proof needs are read here from the sources the app builds and handed to
entrance_release_coverage_tests.cpp as release_coverage_native.inc: the OoT Wallmaster logic edges
and the scene of every OoT region, the OoT and MM runtime capture scenes, the MM logic source
regions and the MM region that receives each entrance. The production lines that make those tables
the ones the game uses are pinned below. No engine, archive, device or gameplay is exercised.
"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import importlib.util
import re
import subprocess
import sys
import time
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
RANDO = 'soh/soh/Enhancements/randomizer/'


def read(path):
    return (source/path).read_text(encoding='utf-8')


def body(text, signature):
    """The braced body of the production function or block that starts at signature."""
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production block ' + signature)


def combo_build(text):
    """The text a COMBO_BUILD compile sees; the region files use no other conditional."""
    kept, stack = [], []
    for line in text.splitlines():
        directive = line.strip()
        if directive.startswith(('#ifdef', '#ifndef', '#if ', '#elif')):
            assert directive in ('#ifdef COMBO_BUILD', '#ifndef COMBO_BUILD'), directive
            stack.append(directive == '#ifdef COMBO_BUILD')
        elif directive.startswith('#else'):
            stack[-1] = not stack[-1]
        elif directive.startswith('#endif'):
            stack.pop()
        elif all(stack):
            kept.append(line)
    assert not stack
    return '\n'.join(kept)


# OoT logic: the audited Wallmaster edges the overlay and the Temple probe build from.
overlay = read(RANDO + 'combo_entrance_overlay.cpp')
for needle in ('const std::array<SourceEdge, 22> edges = { {\n#include "combo_oot_wallmaster_edges.inc"',
               'const std::array<Arrival, 11> arrivals = { {\n#include "combo_oot_wallmaster_arrivals.inc"',
               'for (const auto& edge : edges) '
               'if (edge.source == source && CE::OotWallmasterEdgeInQuests(edge.auditId, quests)) {',
               'Entrance exit(arrival->region, edge.condition, std::string(edge.auditId), false);'):
    assert hostcxx.contains(overlay, needle), needle
edges_inc = read(RANDO + 'combo_oot_wallmaster_edges.inc')
edges = re.findall(r'^\{ "([^"]+)", "(OOT_WALLMASTER_\w+)", (RR_\w+), \[\] \{$', edges_inc, re.M)
assert len(edges) == edges_inc.count('\n{ "') == 22, edges
arrivals = re.findall(r'^\{ "OOT_\w+", 0x[0-9a-f]+u, ENTR_\w+, (RR_\w+) \},$', read(RANDO + 'combo_oot_wallmaster_arrivals.inc'),
                      re.M)
assert len(arrivals) == 11
# The Temple probe adds a source's candidate edges through the same overlay tables.
probe = read(RANDO + 'TempleProbe.cpp')
assert hostcxx.contains(probe, 'for (auto exit : Rando::ComboEntrances::WallmasterCandidateExits(edge.src, edge.dst)) {')

# Every native OoT region and its scene (location_access areaTable).
oot_scenes = {}
for path in sorted((source/RANDO/'location_access').rglob('*.cpp')):
    text = combo_build(path.read_text(encoding='utf-8'))
    for region, scene in re.findall(r'areaTable\[(RR_\w+)\]\s*=\s*Region\(\s*"(?:\\.|[^"\\])*"\s*,\s*(SCENE_\w+)', text):
        assert oot_scenes.setdefault(region, scene) == scene, region
used = sorted({anchor for _, _, anchor in edges} | set(arrivals))
assert all(region in oot_scenes for region in used), [r for r in used if r not in oot_scenes]

# OoT runtime: the transport classifies by native scene and actor room, then loads the resolved arrival.
runtime = read(RANDO + 'combo_entrance_runtime.cpp')
transport = body(runtime, 'extern "C" int Combo_OOTWallmasterTransport(PlayState* play, Actor* actor) {')
for needle in ('CE::OotWallmasterSource(NativeWallmasterScene(play->sceneNum), static_cast<uint8_t>(actor->room));',
               'CE::OotUnifiedDestinationBound(row->name) ? CE::ResolveOotNativeArrival(row->id) : std::nullopt;',
               'if (!arrival || arrival->grotto || arrival->entrance > 0xffffu) {',
               'play->nextEntranceIndex = static_cast<uint16_t>(arrival->entrance);'):
    assert hostcxx.contains(transport, needle), needle
oot_captures = re.findall(r'case (SCENE_\w+):\s*return CE::OotWallmasterScene::(\w+);',
                          body(runtime, 'CE::OotWallmasterScene NativeWallmasterScene(int scene) {'))
assert len(oot_captures) == 6, oot_captures

# MM logic: the regions the Wallmaster exploration starts from, and the region each entrance enters.
mm_logic = read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp')
mm_sources = re.findall(r'case (RR_\w+):\s*return "(MM_WALLMASTER_\w+)";',
                        body(mm_logic, 'std::string_view SourceForRegion(RandoRegionId region) {'))
assert len(mm_sources) == 4, mm_sources
explore = body(mm_logic, 'bool ExploreMmWallmasterEntrances(RandoRegionId region, uint64_t sourceTimes,')
assert hostcxx.contains(explore, 'const auto source = SourceForRegion(region);')
assert hostcxx.contains(explore, 'return ExploreMmUnifiedWallmasterEntrances(source, sourceTimes, reachable, times);')
assert hostcxx.contains(body(mm_logic, 'static bool ExploreMmUnifiedWallmasterEntrances('),
                        'const auto target = GetRegionIdFromEntrance(entrance);')
mm_transport = body(mm_logic, 'extern "C" int Combo_MMWallmasterTransport(PlayState* play, Actor* actor) {')
unified = mm_transport[:hostcxx.find(mm_transport, 'if (!play || !actor || !IS_RANDO || !CE::ActiveMmWallmasters().active())')]
mm_captures = re.findall(r'play->sceneId == (SCENE_\w+)\s*\?\s*CE::SceneContext::(\w+)', unified)
assert len(mm_captures) == 2, mm_captures
for needle in ('const auto* source = CE::WallmasterSource(scene, static_cast<uint8_t>(actor->room));',
               'if (age < 0 || !MM_CanEntranceArrival(gSaveContext.fileNum, destination, age, 0)) {',
               'if (native > 0xffffu) {'):
    assert hostcxx.contains(unified, needle), needle
# MM_CanEntranceArrival checks only the age's range and MmArrivalBound, whose tables the proof reads.
cross = read('mm/2s2h/Rando/CrossEntrance.inc.h')
bound = body(cross, 'bool MmArrivalBound(uint32_t id) {')
for table in ('kMmGrottoGraphBindings', 'kMmOrdinaryBindings'):
    assert hostcxx.contains(bound, f'for (const auto& row : CEMmArrival::{table}) if (row.logicalId == id) bound = true;')
assert hostcxx.contains(bound, 'if (!bound) return false;')
assert hostcxx.contains(body(cross, 'extern "C" COMBO_EXPORT int MM_CanEntranceArrival(int file, uint32_t id, int age, int crossAge) {'),
                        'age < 0 || age > 1 || (crossAge != 0 && crossAge != 1) || !MmArrivalBound(id)')

# GetRegionIdFromEntrance maps each exit's return entrance and each one-way entrance to its region,
# skipping the COMBO_BUILD graph-only regions.
logic = read('mm/2s2h/Rando/Logic/Logic.cpp')
lookup = body(logic, 'RandoRegionId GetRegionIdFromEntrance(s32 entrance) {')
for needle in ('if (randoRegionId >= RR_MM_GROTTO_WOODS_BANK || randoRegionId == RR_MM_STONE_TOWER_FRONT_OF_TEMPLE) continue;',
               'entranceToRegionId[regionExit.returnEntrance] = randoRegionId;',
               'for (auto& entrance : randoRegion.oneWayEntrances) { entranceToRegionId[entrance] = randoRegionId; }'):
    assert hostcxx.contains(lookup, needle), needle
types = read('mm/2s2h/Rando/Types.h')
enum_end = types.index('} RandoRegionId;')
enum_text = combo_build(types[types.rindex('typedef enum {', 0, enum_end):enum_end])
enum_text = re.sub(r'//[^\n]*', '', enum_text)
mm_regions = re.findall(r'\b(RR_\w+)\b', enum_text)
graph_only = set(mm_regions[mm_regions.index('RR_MM_GROTTO_WOODS_BANK'):]) | {'RR_MM_STONE_TOWER_FRONT_OF_TEMPLE'}
scene_values = {name: int(value, 16) for value, name in
                re.findall(r'/\* (0x[0-9A-Fa-f]+) \*/ (ENTR_SCENE_\w+),', read('mm/include/z64scene.h'))}
assert scene_values['ENTR_SCENE_WOODFALL_TEMPLE'] == 0x18
assert hostcxx.contains(read('mm/include/z64scene.h'),
                        '#define ENTRANCE(scene, spawn) ((((ENTR_SCENE_##scene) & 0x7F) << 9) | (((spawn) & 0x1F) << 4))')


def entrance(scene, spawn):
    return ((scene_values['ENTR_SCENE_' + scene] & 0x7f) << 9) | ((int(spawn) & 0x1f) << 4)


mm_region_scenes, mm_arrivals = {}, []
for path in sorted((source/'mm/2s2h/Rando/Logic/Regions').glob('*.cpp')):
    text = combo_build(path.read_text(encoding='utf-8'))
    starts = [m for m in re.finditer(r'Regions\[(RR_\w+)\]\s*=\s*RandoRegion\s*\{', text)]
    for i, match in enumerate(starts):
        region = match.group(1)
        block = text[match.start():starts[i + 1].start() if i + 1 < len(starts) else len(text)]
        assert region in mm_regions and region not in mm_region_scenes, region
        scene = re.search(r'\.sceneId\s*=\s*(SCENE_\w+)', block)
        mm_region_scenes[region] = scene.group(1) if scene else ''
        if region in graph_only:
            continue
        for _, _, scene_name, spawn in re.findall(r'EXIT\(\s*ENTRANCE\((\w+),\s*(\d+)\)\s*,\s*ENTRANCE\((\w+),\s*(\d+)\)',
                                                  block):
            mm_arrivals.append((entrance(scene_name, spawn), region))
        one_way = re.search(r'\.oneWayEntrances\s*=\s*\{([^}]*)\}', block)
        for scene_name, spawn in re.findall(r'ENTRANCE\((\w+),\s*(\d+)\)', one_way.group(1) if one_way else ''):
            mm_arrivals.append((entrance(scene_name, spawn), region))
assert all(region in mm_region_scenes and mm_region_scenes[region] for region, _ in mm_sources), mm_sources

lines = ['// Generated by entrance_release_coverage_tests.py from the native sources; do not edit.',
         '// OoT Wallmaster logic edges (combo_oot_wallmaster_edges.inc): audit id, source, anchor region.',
         'struct OotEdge { std::string_view audit, source; RandomizerRegion anchor; };',
         'static const OotEdge kOotEdges[] = {']
lines += [f'    {{ "{audit}", "{src}", {anchor} }},' for audit, src, anchor in edges]
lines += ['};', '// The native scene of each region the edges and arrivals use (location_access areaTable).',
          'struct OotRegionScene { RandomizerRegion region; std::string_view scene; };',
          'static const OotRegionScene kOotRegionScenes[] = {']
lines += [f'    {{ {region}, "{oot_scenes[region]}" }},' for region in used]
lines += ['};', '// combo_entrance_runtime.cpp NativeWallmasterScene: capture class and native scene.',
          'struct OotCapture { ComboRando::Entrances::OotWallmasterScene scene; std::string_view native; };',
          'static const OotCapture kOotCaptures[] = {']
lines += [f'    {{ ComboRando::Entrances::OotWallmasterScene::{cls}, "{scene}" }},' for scene, cls in oot_captures]
lines += ['};', '// ComboEntrancesMM.cpp SourceForRegion: source, logic region and its scene (Regions/*.cpp).',
          'struct MmSource { std::string_view source, region, scene; };', 'static const MmSource kMmSources[] = {']
lines += [f'    {{ "{src}", "{region}", "{mm_region_scenes[region]}" }},' for region, src in mm_sources]
lines += ['};', '// Combo_MMWallmasterTransport: capture class and native scene.',
          'struct MmCapture { ComboRando::Entrances::SceneContext scene; std::string_view native; };',
          'static const MmCapture kMmCaptures[] = {']
lines += [f'    {{ ComboRando::Entrances::SceneContext::{cls}, "{scene}" }},' for scene, cls in mm_captures]
lines += ['};', '// GetRegionIdFromEntrance: each entrance a COMBO_BUILD logic region receives.',
          'struct MmArrival { uint32_t entrance; std::string_view region; };', 'static const MmArrival kMmArrivals[] = {']
lines += [f'    {{ 0x{value:x}u, "{region}" }},' for value, region in sorted(mm_arrivals)]
lines += ['};', '']
(work/'release_coverage_native.inc').write_text('\n'.join(lines), encoding='utf-8', newline='\n')

# Native keeps every dungeon vanilla without MQ data (no oot-mq.o2r, and a seed that does not record OoTMM's bundled
# MQ data) or with MQ Dungeon Setting None; the menu admits only those.
settings = read(RANDO + 'settings.cpp')
for needle in ('const bool bundledMq = GetMqDataSource() == ComboMq::SeedMqSource::Bundled;',
               'if (OTRGlobals::Instance->HasOriginal() && !OTRGlobals::Instance->HasMasterQuest() && !bundledMq) { '
               'mOptions[RSK_MQ_DUNGEON_RANDOM].Set(RO_MQ_DUNGEONS_NONE); }',
               'if (mOptions[RSK_MQ_DUNGEON_RANDOM].Is(RO_MQ_DUNGEONS_NONE)) { mOptions[RSK_MQ_DUNGEON_COUNT].Set(0); '
               'mOptions[RSK_MQ_DUNGEON_SET].Set(RO_GENERIC_OFF); }'):
    assert hostcxx.contains(settings, needle), needle

# Each process must finish within hostcxx.RUN_TIMEOUT, the binding limit, even on a loaded machine (CTest's timeout
# only bounds the whole harness). Each proof over all 4096 quest masks takes tens of seconds, so the proof runs in three
# parts (the .cpp's argv[1]: Own Game without and with song facts, and Full), and the release and dev builds' six
# processes run at once. Each build compiles in its
# own directory, so the two compilations can run at once too.
PARTS = ('ids', 'songs', 'full')


def compile_build(dev):
    (work/f'build_{dev}').mkdir(exist_ok=True)
    return hostcxx.build(a.compiler, source/'ComboAndroid/tests/entrance_release_coverage_tests.cpp',
                         includes=[source/'combo', a.json_include, source/'soh', source/'soh/include', work],
                         defines=(['COMBO_ENTRANCE_DEV=1'] if dev else []) + ['_CRT_SECURE_NO_WARNINGS'],
                         msvc=['/Zc:preprocessor', '/bigobj', '/O2'], gnu=['-O2'],
                         exe=work/f'build_{dev}'/f'release_coverage_{dev}.exe')


def prove(job):
    exe, part = job
    started = time.monotonic()
    result = subprocess.run([str(exe), part], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    return exe, part, result, time.monotonic() - started


with ThreadPoolExecutor(max_workers=2) as pool:
    exes = list(pool.map(compile_build, (0, 1)))
with ThreadPoolExecutor(max_workers=len(exes) * len(PARTS)) as pool:
    runs = list(pool.map(prove, [(exe, part) for exe in exes for part in PARTS]))
for exe, part, result, seconds in runs:
    output = (result.stdout + result.stderr).strip()
    print(f'{exe.name} {part} ({seconds:.0f} s of {hostcxx.RUN_TIMEOUT}): {output}')
failed = [f'{exe.name} {part} exited with {result.returncode}' for exe, part, result, _ in runs if result.returncode]
if failed:
    sys.exit('FAIL: ' + '; '.join(failed))

# The release gate (verify_apk.py) still refuses only the dev build: a Public capability adds no dev marker.
sys.path.insert(0, str(source/'ComboAndroid/tools'))
spec = importlib.util.spec_from_file_location('verify_apk', source/'ComboAndroid/tools/verify_apk.py')
verify_apk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify_apk)
binaries = [(exe.name, exe.read_bytes()) for exe in exes]
assert verify_apk.entrance_dev_libraries(binaries) == ['release_coverage_1.exe']
