"""The MM unified entrance layout through the real MM exports and the real MM Wallmaster logic, on the host.

Compiled harness: the four entrance exports and the Rando::Logic Wallmaster exploration are taken
from mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp with the real combo/rando/entrances headers and stubs
for the 2Ship region search. It checks that this build's capabilities refuse every layout (Phase 0),
then, with every catalog value open (tests only), that a layout installs, dumps unchanged, installs
again from the dump and binds through the launcher's ApplyNativeSeed; that every bind clears the
unified table and a layout beside a private MM profile is refused; that the unified assumption
profile is decoded before the legacy fallback and gives the pinned 4x7 MM Wallmaster table
(tools/ootmm/m1-contract.json); and that the logic reads the unified routes with the
HaveEnemySoul(ACTOR_EN_WALLMAS) gate, the can_reset_time AND for a destination without no-sot,
FilterNativeMmIngressTimes and no ACCESS on arrival, while the private profile's logic is unchanged.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
text = (source/'mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp').read_text(encoding='utf-8')
fixture = json.loads((source/'tools/ootmm/m1-contract.json').read_text(encoding='utf-8'))


def function(signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function ' + signature)


# Production pieces: the entrance headers, the file-local state and source classifier, the four
# exports, and the Rando::Logic block with both Wallmaster explorations.
includes = '\n'.join(re.findall(r'^#include "rando/entrances/[^"]+"$', text, re.M))
assert '#include "rando/entrances/MmUnifiedLayout.h"' in includes
locals_start = hostcxx.find(text, 'namespace CE = ComboRando::Entrances;')
locals_end = hostcxx.find(text, 'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(')
file_locals = text[locals_start:locals_end]
apply_export = function('extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(')
exports = '\n'.join([
    apply_export,
    function('extern "C" COMBO_EXPORT int MM_SetEntranceExpansionAssumptionsForCombo('),
    function('extern "C" COMBO_EXPORT const char* MM_DumpEntranceExpansionForCombo('),
    function('extern "C" COMBO_EXPORT void MM_ClearEntranceExpansionAssumptionsForCombo('),
])
logic = function('namespace Rando::Logic {')
unified = function('static bool ExploreMmUnifiedWallmasterEntrances(')

# The installer reads this build's capabilities once; the harness swaps in a settable set.
assert apply_export.count('CE::ActiveCapabilities()') == 1 and exports.count('ActiveCapabilities') == 1
exports = exports.replace('CE::ActiveCapabilities()', 'HarnessCapabilities()')
# The unified profile is decoded before the legacy fallback, and every reset names the unified state.
assumptions = function('extern "C" COMBO_EXPORT int MM_SetEntranceExpansionAssumptionsForCombo(')
assert assumptions.index('CE::kCandidateAssumptionsProfile') < assumptions.index('DecodeMmWallmasterAssumptions')
for name in ('MM_ApplyEntranceExpansionForCombo(', 'MM_SetEntranceExpansionAssumptionsForCombo(',
             'MM_ClearEntranceExpansionAssumptionsForCombo('):
    assert 'CE::ActiveMmUnifiedAssumptions().reset();' in function('extern "C" COMBO_EXPORT ' +
                                                                   ('void ' if 'Clear' in name else 'int ') + name)
# No arrival grants ACCESS: neither exploration records it.
assert 'RecordMmAccess' not in logic and 'RecordMmAccess' not in unified

pinned = {}
for combination in fixture['combinations']:
    if combination['id'] == 'erWallmasters=ownGame;erOneWays=none':
        for pool in combination['pools']:
            for group in pool['candidates']:
                for src in group['sources']:
                    if src.startswith('MM_'):
                        pinned[src] = sorted(group['destinations'])
assert len(pinned) == 4 and all(len(d) == 7 for d in pinned.values())
pinned_cpp = '\n'.join('    {"%s", {%s}},' % (src, ', '.join('"%s"' % d for d in dsts)) for src, dsts in sorted(pinned.items()))
sample = [row for row in next(c for c in fixture['combinations']
                              if c['id'] == 'erWallmasters=ownGame;erOneWays=none')['sampleLayout']
          if row[0].startswith('MM_')]
sample_cpp = '\n'.join('    {"%s", "%s"},' % (src, dst) for src, dst in sample)

prelude = r'''
#define NOMINMAX
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
''' + includes + r'''
#include "rando/entrances/MmRegionalFlags.h" // through 2Ship's Logic.h in the game
#include "rando/entrances/NativeBridge.h"
#define COMBO_EXPORT
static int loggedErrors = 0;
template <class... A> void LogError(const char*, const A&...) { ++loggedErrors; }
#define SPDLOG_ERROR(...) LogError(__VA_ARGS__)
using s32 = int32_t;
enum RandoRegionId {
    RR_BENEATH_THE_GRAVEYARD_DAMPE = 1,
    RR_BENEATH_THE_WELL_FREEZARD_ROOM,
    RR_BENEATH_THE_WELL_TWO_SPIKED_BARS,
    RR_BENEATH_THE_WELL_FOUR_SPIKED_BARS,
    RR_OTHER_SOURCE,
    RR_FIRST_TARGET = 100,
    RR_MAX = 1000,
};
static ComboRando::Entrances::CapabilitySet harnessCaps = ComboRando::Entrances::ActiveCapabilities();
static ComboRando::Entrances::CapabilitySet HarnessCapabilities() { return harnessCaps; }
namespace Rando::Logic {
struct RandoRegion { bool canStayOverTime = false; };
struct RegionTimeState {
    uint64_t timeSlices;
    bool canStayOverTime;
    ComboRando::Entrances::MmFlagDomain regionalFlags;
};
std::map<RandoRegionId, RandoRegion> Regions;
uint64_t gCurrentRegionTime = 0;
enum { ACTOR_EN_WALLMAS = 0x8b };
bool soulOwned = true, canResetTime = false;
int soulChecks = 0, resetChecks = 0, accessWrites = 0;
std::map<s32, RandoRegionId> entranceRegions;
std::set<s32> filtered;
std::vector<s32> filterCalls;
std::vector<RandoRegionId> explored;
bool HaveEnemySoul(int actor) { assert(actor == ACTOR_EN_WALLMAS); ++soulChecks; return soulOwned; }
bool RecordMmAccess(uint64_t) { ++accessWrites; return true; }
namespace TimeLogic {
template <class F> uint64_t EvaluateConditionTimes(F predicate, uint64_t times) { return predicate() ? times : 0; }
}
RandoRegionId GetRegionIdFromEntrance(s32 entrance) {
    const auto it = entranceRegions.find(entrance);
    return it == entranceRegions.end() ? RR_MAX : it->second;
}
bool TimeStateCovers(const RegionTimeState& a, const RegionTimeState& b) {
    return (a.timeSlices & b.timeSlices) == b.timeSlices;
}
RegionTimeState MergeTimeStates(const RegionTimeState& a, const RegionTimeState& b) {
    return { a.timeSlices | b.timeSlices, a.canStayOverTime || b.canStayOverTime, b.regionalFlags };
}
void FindReachableRegions(RandoRegionId region, std::set<RandoRegionId>&, std::unordered_map<RandoRegionId, RegionTimeState>&) {
    explored.push_back(region);
}
} // namespace Rando::Logic
'''
after = r'''
namespace Rando::Logic {
bool NativeMmCanResetTime() { ++resetChecks; return canResetTime; }
uint64_t FilterNativeMmIngressTimes(s32 entrance, uint64_t arrivalTimes) {
    filterCalls.push_back(entrance);
    return filtered.contains(entrance) ? 0 : arrivalTimes;
}
} // namespace Rando::Logic
'''
tests = r'''
using namespace Rando::Logic;
using Json = nlohmann::json;
using Targets = std::map<std::string, std::set<std::string>>;
const Targets kPinned = {
''' + pinned_cpp + r'''
};
const std::vector<std::pair<std::string, std::string>> kSample = {
''' + sample_cpp + r'''
};
const RandoRegionId kSources[] = {RR_BENEATH_THE_GRAVEYARD_DAMPE, RR_BENEATH_THE_WELL_FREEZARD_ROOM,
                                  RR_BENEATH_THE_WELL_TWO_SPIKED_BARS, RR_BENEATH_THE_WELL_FOUR_SPIKED_BARS};
const char* kSourceNames[] = {"MM_WALLMASTER_DAMPE", "MM_WALLMASTER_BTW_ENTRANCE", "MM_WALLMASTER_BTW_FOUNTAIN",
                              "MM_WALLMASTER_BTW_EXIT"};

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
    for (const auto& [src, dst] : kSample)
        routes[CE::LayoutV1::FindRow(src)->name] = CE::LayoutV1::FindRow(dst)->name;
    layout.overrides = routes;
    return {{"fileType", ComboRando::kEntranceSeedFileType}, {"version", ComboRando::kEntranceSeedVersion},
            {"entranceLayout", CE::EncodeEntranceLayout(layout)}};
}
Json Dump() { return Json::parse(MM_DumpEntranceExpansionForCombo()); }
std::vector<CE::SavedOverride> LegacyRows() {
    std::vector<CE::SavedOverride> rows;
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i) {
        const auto* src = CE::Find(CE::kMmWallmasterSources[i]);
        const auto* dst = CE::Find(CE::kMmBaseWallmasterDestinations[6 - i]);
        rows.push_back({std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id});
    }
    return rows;
}
Json LegacySeed() { return {{"entranceExpansion", CE::EncodeMmWallmasterExtension(LegacyRows())}}; }
std::string TrancheOverlay() {
    CE::Settings s;
    s.erWallmasters = CE::Mode::OwnGame;
    const CE::Overrides links = {{"OOT_SHOP_MASKS", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER"},
                                 {"MM_CLOCK_TOWER_FROM_CLOCK_TOWN", "OOT_MARKET_FROM_MASK_SHOP"}};
    return CE::EncodeMmOverlay(CE::BuildCandidateWorld(links, CE::LayoutV1::MakePools(s, {}, true), s));
}
bool Cleared() {
    const auto dump = Dump();
    return dump == Json::object() && !CE::ActiveMmUnified().active() && !CE::ActiveMmWallmasters().active() &&
           !CE::ActiveMmUnifiedAssumptions() && !CE::ActiveMmWallmasterAssumptions();
}

// This build has no capability in Phase 0: the real export refuses every layout, dev build or not.
void PhaseZero() {
    harnessCaps = CE::ActiveCapabilities();
    assert(harnessCaps.Empty());
    loggedErrors = 0;
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 0 && loggedErrors == 1 && Cleared());
}

void InstallClearRoundTrip() {
    harnessCaps = WholeCatalog();
    const auto seed = Seed();
    assert(MM_ApplyEntranceExpansionForCombo(seed.dump().c_str()) == 1);
    const auto dump = Dump();
    assert(dump.size() == 1 && dump.at("entranceLayout") == seed.at("entranceLayout"));
    assert(CE::ActiveMmUnified().routes().size() == 4 && !CE::ActiveMmWallmasters().active());
    // The dump installs again and dumps the same bytes.
    const std::string bytes = MM_DumpEntranceExpansionForCombo();
    assert(MM_ApplyEntranceExpansionForCombo(bytes.c_str()) == 1 && MM_DumpEntranceExpansionForCombo() == bytes);
    // The launcher's bind with the real exports.
    const CE::NativeFns native{MM_ApplyEntranceExpansionForCombo, MM_SetEntranceExpansionAssumptionsForCombo,
                               MM_ClearEntranceExpansionAssumptionsForCombo, MM_DumpEntranceExpansionForCombo};
    CE::ApplyNativeSeed(native, seed, harnessCaps);
    assert(Dump().at("entranceLayout") == seed.at("entranceLayout"));
    // Every bind clears the unified table and candidate state: a clear, a private profile, a refusal.
    CE::ActiveMmUnifiedAssumptions() = CE::MmUnifiedAssumptions{};
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1 && Cleared());
    assert(MM_ApplyEntranceExpansionForCombo(seed.dump().c_str()) == 1);
    assert(MM_ApplyEntranceExpansionForCombo(LegacySeed().dump().c_str()) == 1);
    assert(!CE::ActiveMmUnified().active() && CE::ActiveMmWallmasters().active());
    assert(Dump().size() == 1 && Dump().contains("entranceExpansion"));
    // A layout beside any private profile is refused and leaves nothing installed.
    for (const char* key : ComboRando::kLegacyEntranceKeys) {
        assert(MM_ApplyEntranceExpansionForCombo(seed.dump().c_str()) == 1);
        auto both = seed;
        both[key] = LegacySeed().at("entranceExpansion");
        loggedErrors = 0;
        assert(MM_ApplyEntranceExpansionForCombo(both.dump().c_str()) == 0 && loggedErrors == 1 && Cleared());
    }
    assert(MM_ApplyEntranceExpansionForCombo(LegacySeed().dump().c_str()) == 1);
    auto both = LegacySeed();
    both["entranceLayout"] = seed.at("entranceLayout");
    assert(MM_ApplyEntranceExpansionForCombo(both.dump().c_str()) == 0 && Cleared());
    // A broken layout after a good one, and a release build reading a dev layout.
    assert(MM_ApplyEntranceExpansionForCombo(seed.dump().c_str()) == 1);
    auto broken = seed;
    broken["entranceLayout"]["overrides"].erase(0);
    assert(MM_ApplyEntranceExpansionForCombo(broken.dump().c_str()) == 0 && Cleared());
    auto dev = seed;
    dev["entranceLayout"]["devBuild"] = true;
    assert(MM_ApplyEntranceExpansionForCombo(dev.dump().c_str()) == 1);
    harnessCaps.devBuild = false;
    assert(MM_ApplyEntranceExpansionForCombo(dev.dump().c_str()) == 0 && Cleared());
    harnessCaps = WholeCatalog();
}

void Assumptions() {
    harnessCaps = WholeCatalog();
    assert(MM_SetEntranceExpansionAssumptionsForCombo(TrancheOverlay().c_str()) == 1);
    const auto& assumed = CE::ActiveMmUnifiedAssumptions();
    assert(assumed && !CE::ActiveMmWallmasterAssumptions());
    Targets targets;
    for (const auto& [source, routes] : assumed->routes)
        for (const auto& route : routes)
            targets[std::string(CE::Find(CE::Game::Mm, source)->name)].insert(
                std::string(CE::Find(route.destination.game, route.destination.id)->name));
    assert(targets == kPinned);
    MM_ClearEntranceExpansionAssumptionsForCombo();
    assert(!CE::ActiveMmUnifiedAssumptions());
    // Another assumption call replaces it; a refused one leaves nothing.
    assert(MM_SetEntranceExpansionAssumptionsForCombo(TrancheOverlay().c_str()) == 1);
    assert(MM_SetEntranceExpansionAssumptionsForCombo(
               CE::EncodeMmWallmasterAssumptions({}, {}).dump().c_str()) == 1);
    assert(!CE::ActiveMmUnifiedAssumptions() && CE::ActiveMmWallmasterAssumptions());
    auto refused = Json::parse(TrancheOverlay());
    refused["assumed"] = {"MM_TEMPLE_WOODFALL"};
    assert(MM_SetEntranceExpansionAssumptionsForCombo(refused.dump().c_str()) == 0);
    assert(!CE::ActiveMmUnifiedAssumptions() && !CE::ActiveMmWallmasterAssumptions());
    // An unknown profile still reaches the legacy decoder, which refuses it.
    auto unknown = Json::parse(TrancheOverlay());
    unknown["profile"] = "ootmm-entrance-assumptions-v2";
    assert(MM_SetEntranceExpansionAssumptionsForCombo(unknown.dump().c_str()) == 0);
    assert(!CE::ActiveMmUnifiedAssumptions() && !CE::ActiveMmWallmasterAssumptions());
    // A bind clears candidate state.
    assert(MM_SetEntranceExpansionAssumptionsForCombo(TrancheOverlay().c_str()) == 1);
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1 && Cleared());
}

void ResetSearch() {
    soulOwned = true;
    canResetTime = false;
    soulChecks = resetChecks = accessWrites = 0;
    filtered.clear();
    filterCalls.clear();
    explored.clear();
}
std::set<std::string> Reached(RandoRegionId source, uint64_t times = 0b1011) {
    std::set<RandoRegionId> reachable;
    std::unordered_map<RandoRegionId, RegionTimeState> states;
    explored.clear();
    ExploreMmWallmasterEntrances(source, times, reachable, states);
    std::set<std::string> result;
    for (const auto region : reachable) {
        assert(states.at(region).timeSlices == times);
        for (const auto& [entrance, target] : entranceRegions)
            if (target == region)
                result.insert(std::string(CE::Find(CE::Game::Mm, static_cast<uint32_t>(entrance))->name));
    }
    assert(explored.size() == reachable.size());
    return result;
}

void Logic() {
    // Every MM arrival the harness knows has a region.
    int next = RR_FIRST_TARGET;
    for (const auto& row : CE::kRegistry)
        if (row.game == CE::Game::Mm && row.id <= 0xffffu && !CE::IsVoidArea(row.to)) {
            entranceRegions[static_cast<s32>(row.id)] = static_cast<RandoRegionId>(next);
            Regions[static_cast<RandoRegionId>(next++)] = {};
        }
    harnessCaps = WholeCatalog();
    // Candidate assumptions: each source reaches its pinned seven, under the soul gate, through the
    // ingress filter, with no reset gate (all no-sot) and no ACCESS.
    assert(MM_SetEntranceExpansionAssumptionsForCombo(TrancheOverlay().c_str()) == 1);
    for (size_t i = 0; i < 4; ++i) {
        ResetSearch();
        assert(Reached(kSources[i]) == kPinned.at(kSourceNames[i]));
        assert(soulChecks == 1 && resetChecks == 0 && accessWrites == 0 && filterCalls.size() == 7);
        ResetSearch();
        soulOwned = false;
        assert(Reached(kSources[i]).empty() && soulChecks == 1);
        ResetSearch();
        assert(Reached(kSources[i], 0).empty() && soulChecks == 0);
    }
    ResetSearch();
    assert(Reached(RR_OTHER_SOURCE).empty() && soulChecks == 0);
    ResetSearch();
    filtered.insert(static_cast<s32>(CE::Find("MM_TEMPLE_WOODFALL")->id));
    auto expected = kPinned.at("MM_WALLMASTER_DAMPE");
    expected.erase("MM_TEMPLE_WOODFALL");
    assert(Reached(RR_BENEATH_THE_GRAVEYARD_DAMPE) == expected);
    // The installed layout: one arrival per source.
    MM_ClearEntranceExpansionAssumptionsForCombo();
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 1);
    for (size_t i = 0; i < 4; ++i) {
        ResetSearch();
        std::string dst;
        for (const auto& [src, to] : kSample)
            if (src == kSourceNames[i])
                dst = to;
        assert(Reached(kSources[i]) == std::set<std::string>{dst} && accessWrites == 0);
    }
    // Synthetic routes: a destination without no-sot needs can_reset_time, an arrival into VOID is a
    // dead end, a foreign or regionless arrival fails loudly.
    const auto* noSot = CE::Find("MM_SWAMP_FROM_IKANA_CANYON");
    const auto reset = CE::MakeMmUnifiedRoute(*noSot);
    assert(reset.resetTime && !reset.toVoid);
    const auto intoVoid = CE::MakeMmUnifiedRoute(*CE::Find("MM_VOID_GREAT_BAY"));
    assert(intoVoid.toVoid);
    CE::MmUnifiedAssumptions synthetic;
    synthetic.routes[0xff00u] = {reset, intoVoid};
    CE::ActiveMmUnifiedAssumptions() = synthetic;
    ResetSearch();
    assert(Reached(RR_BENEATH_THE_GRAVEYARD_DAMPE).empty() && resetChecks == 1 && accessWrites == 0);
    assert(filterCalls == std::vector<s32>{static_cast<s32>(noSot->id)}); // the VOID arrival is never filtered
    ResetSearch();
    canResetTime = true;
    assert(Reached(RR_BENEATH_THE_GRAVEYARD_DAMPE) == std::set<std::string>{"MM_SWAMP_FROM_IKANA_CANYON"});
    assert(resetChecks == 1 && accessWrites == 0);
    for (const auto destination : {CE::Key{CE::Game::Oot, 0x169u}, CE::Key{CE::Game::Mm, 0xfffeu}}) {
        synthetic.routes[0xff00u] = {{destination, false, false}};
        CE::ActiveMmUnifiedAssumptions() = synthetic;
        bool threw = false;
        try { Reached(RR_BENEATH_THE_GRAVEYARD_DAMPE); } catch (const std::runtime_error&) { threw = true; }
        assert(threw);
    }
    // The private profile's logic is unchanged and never mixed with the unified one: candidate
    // assumptions of either kind come before an installed layout of either kind.
    CE::ActiveMmUnifiedAssumptions().reset();
    assert(MM_ApplyEntranceExpansionForCombo(LegacySeed().dump().c_str()) == 1);
    for (size_t i = 0; i < 4; ++i) {
        ResetSearch();
        const auto legacy = LegacyRows()[i];
        assert(Reached(kSources[i]) == std::set<std::string>{legacy.destination});
    }
    assert(MM_SetEntranceExpansionAssumptionsForCombo(TrancheOverlay().c_str()) == 1);
    ResetSearch();
    assert(Reached(RR_BENEATH_THE_GRAVEYARD_DAMPE) == kPinned.at("MM_WALLMASTER_DAMPE"));
    assert(MM_ApplyEntranceExpansionForCombo(Seed().dump().c_str()) == 1);
    assert(MM_SetEntranceExpansionAssumptionsForCombo(
               CE::EncodeMmWallmasterAssumptions({{"MM_WALLMASTER_DAMPE", "MM_TEMPLE_SNOWHEAD"}}, {}).dump().c_str()) == 1);
    ResetSearch();
    assert(Reached(RR_BENEATH_THE_GRAVEYARD_DAMPE) == std::set<std::string>{"MM_TEMPLE_SNOWHEAD"});
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1);
    ResetSearch();
    assert(Reached(RR_BENEATH_THE_GRAVEYARD_DAMPE).empty() && soulChecks == 0);
}

int main() {
    PhaseZero();
    InstallClearRoundTrip();
    Assumptions();
    Logic();
    std::cout << "MM unified layout: Phase 0 refusal, install/clear/round trip and bind through the real exports, "
                 "pinned 4x7 assumptions, unified Wallmaster logic and the unchanged private profile passed\n";
}
'''
for dev in (False, True):
    cpp = work/('mm_unified_logic_dev.cpp' if dev else 'mm_unified_logic.cpp')
    cpp.write_text(prelude + file_locals + '\n' + exports + '\n' + logic + '\n' + after + tests, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'] + (['COMBO_ENTRANCE_DEV=1'] if dev else []),
                        werror=True, msvc=['/bigobj'], gnu=['-Wno-missing-field-initializers'])
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('MM unified layout exports and Wallmaster logic match the plan as shipped and with COMBO_ENTRANCE_DEV.')
