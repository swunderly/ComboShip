"""The live check tracker follows Full Wallmaster routes into the other game and back, as the generator's oracles do.

The generator's oracles exchange each game's Wallmaster departures as the other game's arrivals until nothing changes
(CrossWorldRando.h QuerySongWorlds, OracleFns::GetEntranceSourcesForAge and SetEntranceAgeArrivals). The live trackers
did not: OoT's tracker search rooted no MM arrival (fill.cpp gated it on the oracle), MM's neither (Logic.cpp), and
neither recorded a departure, so a check reachable only through a cross-game Wallmaster showed as out of reach.

1. The exchange itself (combo/rando/LiveWallmasterRoutes.h) runs against a model of both games: an OoT check behind an
   OoT -> MM -> OoT chain of Wallmasters, the ages MM is entered at, MM entered not at all, and the MM-in-front side.
2. The production wiring: both engines' tracker searches root the exchange's arrivals and record their departures
   (fill.cpp, Logic.cpp, ComboEntrancesMM.cpp; run in song_live_arrival_tests.py and mm_unified_logic_tests.py), each
   tracker search runs through the exchange while its game is in front, and each game exports the other's search.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)


def read(rel):
    return (source / rel).read_text(encoding='utf-8')


def function(text, signature):
    start = hostcxx.find(text, signature)
    depth, end = 1, text.index('{', start) + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


cpp = r'''
#include "rando/LiveWallmasterRoutes.h"
#include <cassert>
#include <cstdio>
namespace CE = ComboRando::Entrances;
// The model. OoT: child Link reaches the Mask Shop portal (MM entered as child) and the Kokiri Forest Wallmaster, which
// routes into MM's Woodfall Temple (MM bit 0). MM: from Woodfall Temple a Wallmaster routes into OoT's Forest Temple
// (OoT bit 3), whose check only that arrival reaches. MM's own start reaches no Wallmaster source.
struct Model {
    bool portal = true, forestSource = true, woodfallSource = true;
    int ootSearches = 0, mmSearches = 0;
    bool forestCheck = false; // the last OoT search's result
    CE::LiveOotSearch Oot(const CE::WallmasterAgeMasks& arrivals) {
        ++ootSearches;
        forestCheck = (arrivals.masks[1] >> 3) & 1; // child arrival at the Forest Temple
        CE::LiveOotSearch found;
        if (forestSource)
            found.departures |= 1u << 0;
        found.mmAges = uint8_t((portal ? 2 : 0) | (forestSource ? 2 : 0));
        return found;
    }
    uint32_t Mm(uint32_t arrivals) {
        ++mmSearches;
        return woodfallSource && (arrivals & 1u) ? 1u << 3 : 0u;
    }
};
int main() {
    {   // OoT in front: the check behind OoT -> MM -> OoT is reached, by the age MM was entered at only.
        Model m;
        const auto arrivals = CE::LiveWallmastersOotInFront([&](const auto& a) { return m.Oot(a); },
                                                            [&](uint32_t d) { return m.Mm(d); });
        assert(m.forestCheck && arrivals.masks[1] == 1u << 3 && arrivals.masks[0] == 0);
        assert(m.ootSearches == 2 && m.mmSearches <= 2);
    }
    {   // MM's Wallmaster unreachable: one OoT search more is not needed, and nothing arrives.
        Model m;
        m.woodfallSource = false;
        const auto arrivals = CE::LiveWallmastersOotInFront([&](const auto& a) { return m.Oot(a); },
                                                            [&](uint32_t d) { return m.Mm(d); });
        assert(!m.forestCheck && !arrivals.any() && m.ootSearches == 1 && m.mmSearches == 1);
    }
    {   // MM never entered (no portal, no song, no Wallmaster into it): MM is not searched.
        Model m;
        m.portal = m.forestSource = false;
        CE::LiveWallmastersOotInFront([&](const auto& a) { return m.Oot(a); }, [&](uint32_t d) { return m.Mm(d); });
        assert(m.ootSearches == 1 && m.mmSearches == 0 && !m.forestCheck);
    }
    {   // MM in front: MM's search gets OoT's departures (OoT searched with MM's, at the age Link has in MM) and roots
        // them; its last search ran with the final arrivals.
        Model m;
        uint32_t lastArrivals = 99;
        const auto fromOot = CE::LiveWallmastersMmInFront(
            [&](uint32_t arrivals) { lastArrivals = arrivals; return m.Mm(arrivals); },
            [&](uint32_t fromMm) {
                return CE::LiveOotDeparturesForMm(fromMm, 2, [&](const auto& a) { return m.Oot(a); });
            });
        assert(fromOot == 1u && lastArrivals == 1u && m.mmSearches == 2);
        // OoT's searches for MM root MM's departures at the age Link has in MM from the first one.
        Model n;
        CE::LiveOotDeparturesForMm(1u << 3, 1, [&](const auto& a) {
            assert(a.masks[0] == 1u << 3);
            return n.Oot(a);
        });
        assert(n.ootSearches == 2); // child too, once the portal is known
    }
    std::puts("PASS: the tracker's live exchange follows Wallmaster routes into the other game and back");
    return 0;
}
'''
path = out / 'tracker_wallmaster_routes.cpp'
path.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(a.compiler, path, includes=[source / 'combo'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)

# ---- 2. Production wiring.
fill = read('soh/soh/Enhancements/randomizer/3drando/fill.cpp')
assert hostcxx.contains(fill, 'if (region->randomizerRegionKey == RR_ROOT && (ComboRando::OotSongOracle() || '
                              'logic->CalculatingAvailableChecks) && ComboRando::Entrances::OotWallmasterArrivals().any())')
assert hostcxx.contains(fill, 'if (!ComboRando::OotSongOracle() && !logic->CalculatingAvailableChecks) return; '
                              'auto& departures = ComboRando::Entrances::OotWallmasterDepartures().masks;')
logic = read('mm/2s2h/Rando/Logic/Logic.cpp')
assert hostcxx.contains(logic, 'if (currentRegion == RR_MAX && (ComboRando::MmSongOracle() || evaluatingLiveTracker) '
                               '&& gSaveContext.save.linkAge <= 1) { const uint32_t arrivals = '
                               'ComboRando::Entrances::MmWallmasterArrivals().masks[gSaveContext.save.linkAge];')
entrances = read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp')
assert hostcxx.contains(entrances, 'if (capturedTimes && (ComboRando::MmSongOracle() || evaluatingLiveTracker) && '
                                   'age <= 1) CE::MmWallmasterDepartures().masks[age] |= 1u << bit;')
oot_tracker = read('soh/soh/Enhancements/randomizer/randomizer_check_tracker.cpp')
recalc = function(oot_tracker, 'bool InternalRecalculateAvailableChecks(RandomizerRegion startingRegion, RandoAgeTime '
                               'startingAgeTime) {')
assert hostcxx.contains(recalc, 'ComboTrackerSearch([&] { availableChecks = ReachabilitySearch(targetLocations, '
                                'RG_NONE, true, startingRegion, startingAgeTime); });')
mm_tracker = read('mm/2s2h/Rando/CheckTracker/CheckTracker.cpp')
assert hostcxx.contains(function(mm_tracker, 'void Rando::CheckTracker::ComputeChecksInLogic('),
                        'ComboMmTrackerSearch([&] { ComputeChecksInLogicOnce(inLogic); });')
oot_inc, mm_inc = read('combo/menu/ComboCheckTrackerOOT.inc'), read('combo/menu/ComboCheckTrackerMM.inc')
oot_search = function(oot_inc, 'static void ComboTrackerSearch(const std::function<void()>& search) {')
assert hostcxx.contains(oot_search, 'if (sComboLiveExchange || !Combo_OotIsForeground() || !ComboCrossGameWallmasters())')
assert 'LiveCE::LiveWallmastersOotInFront(' in oot_search and '"MM_LiveWallmasterDepartures"' in oot_search
mm_search = function(mm_inc, 'static void ComboMmTrackerSearch(const std::function<void()>& search) {')
assert hostcxx.contains(mm_search, 'if (sComboMmLiveExchange || !Combo_MmIsForeground() || !ComboMmCrossGameWallmasters())')
assert 'LiveCE::LiveWallmastersMmInFront(' in mm_search and '"SOH_LiveWallmasterDepartures"' in mm_search
oot_export = function(oot_inc, 'extern "C" COMBO_EXPORT uint32_t SOH_LiveWallmasterDepartures(int fileNum, uint32_t fromMm, '
                                'uint64_t mmState) {')
assert 'LiveCE::LiveOotDeparturesForMm(fromMm, LINK_IS_CHILD ? 2 : 1,' in oot_export
mm_export = function(mm_inc, 'extern "C" COMBO_EXPORT uint32_t MM_LiveWallmasterDepartures(int fileNum, uint32_t fromOot, '
                              'uint64_t ootState) {')
assert 'Rando::CheckTracker::ComputeChecksInLogicOnce(inLogic)' in mm_export
# The exchange leaves no arrivals behind for an oracle's or the playthrough's search.
for text, arrivals in ((oot_inc, 'LiveCE::OotWallmasterArrivals() = {};'), (mm_inc, 'LiveCE::MmWallmasterArrivals() = {};')):
    scope = text[text.index('ExchangeScope() {'):]
    assert arrivals in scope[:scope.index(chr(10) + '};')]
print('PASS: both trackers run their searches through the live exchange and root its arrivals')
