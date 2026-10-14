"""M1-08: OoTMM's PG_ER hot-water patch in the MM runtime: hot spring water never cools under entrance shuffle.

OoTMM v32.3 packages/generator/src/mm/patches/hot_water.S makes the function at 0x8010eba0 return at
once (jr ra) whenever isEntranceShuffle turns PG_ER on (patch-build/index.ts:58). The receipt
(tools/ootmm/er-enabled-sites.json, runtime) names it Interface_StartBottleTimer from the MM decomp's
US 1.0 symbol map, and this checks that identification against OoTMM's own link addresses
(link_mm.in) and the source order of mm/src/code/z_parameter.c. It then checks the port:
  - Interface_StartBottleTimer returns before touching anything while Combo_MMEntranceShuffleActive(),
    only under COMBO_BUILD, and is otherwise byte-identical to 2Ship's (references/2ship-android when it
    is there); its three callers and the only BOTTLE_TIMER_STATE_COUNTING write are unchanged;
  - compiled with the real MM exports (ComboEntrancesMM.cpp): the timer starts as before with nothing,
    a private MM profile, a cleared slot, the generation's settings alone, outside a rando save, or a
    layout this build refuses; it does not start while a unified layout that shuffles entrances is
    installed; and without COMBO_BUILD it always starts.
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
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--reference', type=Path)
args = parser.parse_args()
root = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(ok, message):
    if not ok:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def function(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function ' + signature)


receipt = json.loads(read('tools/ootmm/er-enabled-sites.json'))
fixture = json.loads(read('tools/ootmm/m1-contract.json'))
(row,) = receipt['runtime']
pinned, symbol, fork = row['pinned'], row['symbol'], row['fork']

# ---- The receipt against the fixture's PG_ER patch and its enable condition.
hot = fixture['runtime']['hotWater']
check(row['flag'] == pinned['group'] == hot['group'] == 'PG_ER', 'PG_ER group')
check(int(pinned['address'], 16) == int(hot['address'], 16), 'patch address differs from the fixture')
check(pinned['instructions'] == hot['instructions'] == ['jr ra', 'nop'], 'patch body differs from the fixture')
enable = [c for c in fixture['runtime']['enableConditions'] if c['flag'] == 'PG_ER']
check(len(enable) == 1 and {k: enable[0][k] for k in ('file', 'line', 'text')} == pinned['enabledBy'],
      'PG_ER enable condition differs from the fixture')

# ---- The symbol: the map's neighbours, OoTMM's own link addresses, and the port's source order.
address = int(pinned['address'], 16)
entries = {name: int(value, 16) for name, value in symbol['map entries'].items()}
order = ['Interface_StartGoronRaceTimer', 'Interface_StartBottleTimer', 'Interface_GetCompressedTimerDigits',
         'Interface_NewDay']
check(list(entries) == order and sorted(entries.values()) == [entries[n] for n in order], 'map entries out of order')
check(symbol['name'] == fork['function'] == 'Interface_StartBottleTimer' and entries[symbol['name']] == address,
      'the patched address is not the named function')
anchors = {a['symbol']: a for a in symbol['pinned anchors']}
check(int(anchors['Interface_NewDay']['address'], 16) == entries['Interface_NewDay'],
      "OoTMM's Interface_NewDay link address differs from the map")
check(int(anchors['Gfx_TextureIA8']['address'], 16) < address < int(anchors['Interface_NewDay']['address'], 16),
      "OoTMM's link addresses do not bracket the patch")
z_parameter = read(fork['file'])
positions = []
for name in [anchors['Gfx_TextureIA8']['fork']] + order:
    m = re.search(r'^\w[\w\s\*]*\b' + name + r'\(', z_parameter, re.M)
    check(m is not None, f'{fork["file"]} has no {name}')
    positions.append(m.start() if m else -1)
check(positions == sorted(positions), f'{fork["file"]} source order differs from the symbol map')
between = z_parameter[positions[1]:positions[3]]
check(len(re.findall(r'^\w[^\n;]*\)\s*\{', between, re.M)) == 2, 'another function sits beside the bottle timer')

# ---- The port's function, its callers and the only counting write.
gate = '''#ifdef COMBO_BUILD
extern int Combo_MMEntranceShuffleActive(void);
#endif

'''
timer = function(z_parameter, 'void Interface_StartBottleTimer(s16 seconds, s16 timerId) {')
check(z_parameter[:z_parameter.index(timer)].endswith(gate), 'the gate declaration is not right before the function')
body = timer[timer.index('{') + 1:]
early = re.match(r'\s*#ifdef COMBO_BUILD\n(?:\s*//[^\n]*\n)*\s*if \(Combo_MMEntranceShuffleActive\(\)\) \{\s*return;\s*\}\s*'
                 r'#endif\n', body)
check(early is not None, 'Interface_StartBottleTimer does not return first under Combo_MMEntranceShuffleActive()')
native = 'void Interface_StartBottleTimer(s16 seconds, s16 timerId) {\n' + (body[early.end():] if early else body)
upstream_file = root.parent.parent / 'references/2ship-android/mm/src/code/z_parameter.c'
if not upstream_file.exists():
    upstream_file = root.parent / 'references/2ship-android/mm/src/code/z_parameter.c'
if upstream_file.exists():
    upstream = function(upstream_file.read_text(encoding='utf-8'), 'void Interface_StartBottleTimer(')
    check(native == upstream, 'Interface_StartBottleTimer differs from 2Ship outside the COMBO_BUILD gate')
    print(f'2Ship reference checked: {upstream_file}')
for caller in fork['callers']:
    definition = re.search(r'^\w[^\n;]*\b' + caller + r'\([^;{]*\)\s*\{', z_parameter, re.M)
    check(definition is not None, f'{fork["file"]} does not define {caller}')
    text = function(z_parameter[definition.start():], caller + '(') if definition else ''
    check(text.count('Interface_StartBottleTimer(60,') == 1 and 'ITEM_HOT_SPRING_WATER' in text,
          f'{caller} no longer starts the hot spring water timer')
COUNTING_WRITE = re.compile(r'[^=!<>]=\s*BOTTLE_TIMER_STATE_COUNTING\b')
callers = []
for path in [p for d in ('mm/src', 'mm/include', 'mm/2s2h') for p in sorted((root / d).rglob('*.[ch]*'))]:
    text = path.read_text(encoding='utf-8', errors='replace')
    callers += [path.name] * len(re.findall(r'\bInterface_StartBottleTimer\(\d', text))
    if path.name != 'z_parameter.c':
        check(not COUNTING_WRITE.search(text), f'{path}: another bottle timer start')
check(callers == ['z_parameter.c'] * 3, f'Interface_StartBottleTimer callers changed: {callers}')
check(len(COUNTING_WRITE.findall(z_parameter)) == 1 and COUNTING_WRITE.search(timer),
      'z_parameter.c starts a bottle timer elsewhere')
entrances_mm = read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp')
check(re.search(r'extern "C" int ' + fork['gate'] + r'\(void\) \{\s*return IS_RANDO && CE::MmUnifiedEntranceShuffle\(\);',
                entrances_mm) is not None, f'{fork["gate"]} reads more than the installed layout')

# ---- OoTMM itself, when the checkout is there.
reference = args.reference
if reference is None:
    for candidate in (root.parent / 'references/ootmm-stable-32.3', root.parent.parent / 'references/ootmm-stable-32.3'):
        if (candidate / pinned['file']).exists():
            reference = candidate
            break
if reference is not None:
    patch = (reference / pinned['file']).read_text(encoding='utf-8')
    check(re.search(r'PATCH_GROUP PG_ER\n\nPATCH_START ' + pinned['address'] + r'\n  jr ra\n   nop\nPATCH_END', patch)
          is not None, 'hot_water.S differs from the receipt')
    link = (reference / 'packages/generator/src/link_mm.in').read_text(encoding='utf-8').splitlines()
    for anchor in symbol['pinned anchors']:
        check(link[anchor['line'] - 1] == f'{anchor["symbol"]} = {anchor["address"]};', f'link_mm.in:{anchor["line"]}')
    index = (reference / pinned['enabledBy']['file']).read_text(encoding='utf-8').splitlines()
    check(index[pinned['enabledBy']['line'] - 1].strip() == pinned['enabledBy']['text'], 'PG_ER enable condition')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources against the fixture only')

if failures:
    sys.exit('\n'.join(failures))

# ---- Compiled: the gate with the real MM exports, with and without COMBO_BUILD.
includes = '\n'.join(re.findall(r'^#include "rando/entrances/[^"]+"$', entrances_mm, re.M))
file_locals = entrances_mm[hostcxx.find(entrances_mm, 'namespace CE = ComboRando::Entrances;'):
                           hostcxx.find(entrances_mm, 'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(')]
exports = '\n'.join(function(entrances_mm, s) for s in (
    'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(',
    'extern "C" COMBO_EXPORT int MM_SetEntranceGenerationSettingsForCombo(',
    'extern "C" int Combo_MMEntranceShuffleActive(',
))
assert exports.count('CE::ActiveCapabilities()') == 2
exports = exports.replace('CE::ActiveCapabilities()', 'HarnessCapabilities()')
program = r'''
#define NOMINMAX
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
''' + includes + r'''
#include "rando/entrances/MmRegionalFlags.h" // through 2Ship's Logic.h in the game
#define COMBO_EXPORT
static int loggedErrors = 0;
template <class... A> void LogError(const char*, const A&...) { ++loggedErrors; }
#define SPDLOG_ERROR(...) LogError(__VA_ARGS__)
static bool harnessRando = true;
#define IS_RANDO harnessRando
enum RandoRegionId {
    RR_BENEATH_THE_GRAVEYARD_DAMPE = 1,
    RR_BENEATH_THE_WELL_FREEZARD_ROOM,
    RR_BENEATH_THE_WELL_TWO_SPIKED_BARS,
    RR_BENEATH_THE_WELL_FOUR_SPIKED_BARS,
};
static ComboRando::Entrances::CapabilitySet harnessCaps = ComboRando::Entrances::ActiveCapabilities();
static ComboRando::Entrances::CapabilitySet HarnessCapabilities() { return harnessCaps; }
''' + file_locals + '\n' + exports + r'''
// z64save.h and z_parameter.c, as far as the bottle timer reads them.
using s16 = int16_t;
using u64 = uint64_t;
using OSTime = uint64_t;
enum { BOTTLE_TIMER_STATE_OFF, BOTTLE_TIMER_STATE_COUNTING };
#define SECONDS_TO_TIMER(seconds) ((seconds) * 100)
struct {
    uint8_t bottleTimerStates[6];
    u64 bottleTimerCurTimes[6], bottleTimerTimeLimits[6];
    OSTime bottleTimerStartOsTimes[6], bottleTimerPausedOsTimes[6];
} gSaveContext;
OSTime sBottleTimerPausedOsTime = 0;
int osTimeCalls = 0;
OSTime osGetTime() { ++osTimeCalls; return 1234; }
''' + gate + timer + r'''
using Json = nlohmann::json;
CE::CapabilitySet WholeCatalog() {
    auto caps = CE::CapabilitySet::AtLeast(CE::Stage::Hidden);
    caps.devBuild = true;
    return caps;
}
Json Seed() {
    CE::EntranceLayout layout;
    layout.required = {std::string(CE::kMmTrancheCapability)};
    layout.settings.erWallmasters = CE::Mode::OwnGame;
    CE::Overrides routes;
    const char* to[] = {"MM_TEMPLE_WOODFALL", "MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_GREAT_BAY", "MM_SPIDER_HOUSE_SWAMP"};
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i)
        routes[CE::LayoutV1::FindRow(CE::kMmWallmasterSources[i])->name] = CE::LayoutV1::FindRow(to[i])->name;
    layout.overrides = routes;
    return {{"fileType", ComboRando::kEntranceSeedFileType}, {"version", ComboRando::kEntranceSeedVersion},
            {"entranceLayout", CE::EncodeEntranceLayout(layout)}};
}
Json LegacySeed() {
    std::vector<CE::SavedOverride> rows;
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i) {
        const auto* src = CE::Find(CE::kMmWallmasterSources[i]);
        const auto* dst = CE::Find(CE::kMmBaseWallmasterDestinations[i]);
        rows.push_back({std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id});
    }
    return {{"entranceExpansion", CE::EncodeMmWallmasterExtension(rows)}};
}
// Picks up hot spring water in bottle slot 2 (Item_GiveImpl's call); true when its timer started.
bool Cools() {
    for (int i = 0; i < 6; ++i) {
        gSaveContext.bottleTimerStates[i] = BOTTLE_TIMER_STATE_OFF;
        gSaveContext.bottleTimerCurTimes[i] = gSaveContext.bottleTimerTimeLimits[i] = 77;
        gSaveContext.bottleTimerStartOsTimes[i] = gSaveContext.bottleTimerPausedOsTimes[i] = 77;
    }
    sBottleTimerPausedOsTime = 55;
    osTimeCalls = 0;
    Interface_StartBottleTimer(60, 2);
    for (int i = 0; i < 6; ++i)
        if (i != 2)
            assert(gSaveContext.bottleTimerStates[i] == BOTTLE_TIMER_STATE_OFF && gSaveContext.bottleTimerCurTimes[i] == 77);
    const auto& s = gSaveContext;
    if (s.bottleTimerStates[2] == BOTTLE_TIMER_STATE_OFF) {
        // Nothing at all was touched: no state, time or clock read.
        assert(s.bottleTimerCurTimes[2] == 77 && s.bottleTimerTimeLimits[2] == 77 && s.bottleTimerStartOsTimes[2] == 77 &&
               s.bottleTimerPausedOsTimes[2] == 77 && sBottleTimerPausedOsTime == 55 && osTimeCalls == 0);
        return false;
    }
    assert(s.bottleTimerStates[2] == BOTTLE_TIMER_STATE_COUNTING && s.bottleTimerCurTimes[2] == 6000 &&
           s.bottleTimerTimeLimits[2] == 6000 && s.bottleTimerStartOsTimes[2] == 1234 &&
           s.bottleTimerPausedOsTimes[2] == 0 && sBottleTimerPausedOsTime == 0 && osTimeCalls == 1);
    return true;
}

int main() {
    assert(SourceForRegion(RR_BENEATH_THE_GRAVEYARD_DAMPE) == "MM_WALLMASTER_DAMPE"); // file-local, unused here
#ifdef COMBO_BUILD
    const bool gated = true;
#else
    const bool gated = false;
#endif
    harnessCaps = WholeCatalog();
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1 && Cools());
    // A private MM profile keeps the native timer.
    assert(MM_ApplyEntranceExpansionForCombo(LegacySeed().dump().c_str()) == 1 && Cools());
    // An installed unified layout that shuffles entrances: the water never cools (PG_ER).
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 1);
    assert(Cools() == !gated);
    harnessRando = false;
    assert(Cools());
    harnessRando = true;
    assert(Cools() == !gated);
    // The next bind (a slot without a layout) clears it.
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1 && Cools());
    // The generation's settings never reach the runtime.
    CE::EntranceLayout document;
    document.required = {std::string(CE::kMmTrancheCapability)};
    document.devBuild = true;
    document.settings.erWallmasters = CE::Mode::OwnGame;
    assert(MM_SetEntranceGenerationSettingsForCombo(CE::EncodeEntranceLayout(document).dump().c_str()) == 1 && Cools());
    assert(MM_SetEntranceGenerationSettingsForCombo(nullptr) == 1);
    // A layout this build refuses installs nothing.
    harnessCaps = CE::CapabilitySet::AtLeast(CE::Stage::Public);
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 0 && loggedErrors == 1 && Cools());
    // This build: a release build refuses the tranche; a Phase A dev build installs it.
    harnessCaps = CE::ActiveCapabilities();
    const int installed = MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str());
#ifdef COMBO_ENTRANCE_DEV
    assert(installed == 1 && Cools() == !gated);
#else
    assert(installed == 0 && Cools());
#endif
    std::cout << (gated ? "COMBO_BUILD: hot spring water never cools under an installed entrance-shuffle layout, "
                          "and cools as before otherwise\n"
                        : "Without COMBO_BUILD the bottle timer always starts\n");
}
'''
for name, defines in (('hot_water_gate', ['COMBO_BUILD']), ('hot_water_gate_dev', ['COMBO_BUILD', 'COMBO_ENTRANCE_DEV=1']),
                      ('hot_water_native', [])):
    cpp = work / (name + '.cpp')
    cpp.write_text(program, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[root / 'combo', args.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'] + defines, werror=True, msvc=['/bigobj'],
                        gnu=['-Wno-missing-field-initializers'])
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('PG_ER hot water: identified at 0x8010eba0, gated in Interface_StartBottleTimer, never cools under an installed '
      'entrance-shuffle layout and cools as before otherwise.')
