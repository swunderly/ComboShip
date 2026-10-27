"""The live check tracker follows Full Wallmaster routes into the other game and back, as the generator's oracles do.

The generator's oracles exchange each game's Wallmaster departures as the other game's arrivals until nothing changes
(CrossWorldRando.h QuerySongWorlds, OracleFns::GetEntranceSourcesForAge and SetEntranceAgeArrivals). The live trackers
did not: OoT's tracker search rooted no MM arrival (fill.cpp gated it on the oracle), MM's neither (Logic.cpp), and
neither recorded a departure, so a check reachable only through a cross-game Wallmaster showed as out of reach.

1. The exchange itself (combo/rando/LiveWallmasterRoutes.h) runs against a model of both games: an OoT check behind an
   OoT -> MM -> OoT chain of Wallmasters, the ages MM is entered at, MM entered not at all, and the MM-in-front side.
   With Warp Songs and Soaring Spots Shuffle a Full Wallmaster may also land at the other game's warp pads and owl
   statues (the arrival bits after the dungeons', CrossWallmasters.h): a route through the last owl and the last pad,
   and a chain through every arrival bit of both games, settle within the exchange's passes.
2. The production wiring: both engines' tracker searches root the exchange's arrivals and record their departures
   over every arrival bit (fill.cpp, Logic.cpp, ComboEntrancesMM.cpp; run in song_live_arrival_tests.py,
   mm_unified_logic_tests.py and oot_wallmaster_overlay_tests.py, the warp pads and owls included), each tracker search
   runs through the exchange while its game is in front, with either Full layout (with or without the warps), and each
   game exports the other's search.
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
#include <vector>
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
// A chain of Full Wallmasters, by arrival bits: OoT's start reaches the Wallmaster to MM bit mm[0], an MM arrival at mm[k]
// reaches the one to OoT bit oot[k], and an OoT arrival at oot[k] the one to mm[k + 1]. Child Link enters MM by the
// Mask Shop; an OoT arrival at adultAt (if any) also lets adult Link reach MM.
struct Chain {
    std::vector<int> mm, oot;
    int adultAt = -1;
    int ootSearches = 0, mmSearches = 0;
    uint32_t reached = 0; // the OoT arrivals the last OoT search had, at either age
    CE::LiveOotSearch Oot(const CE::WallmasterAgeMasks& arrivals) {
        ++ootSearches;
        reached = arrivals.masks[0] | arrivals.masks[1];
        CE::LiveOotSearch found;
        found.departures = 1u << mm[0];
        for (size_t k = 0; k + 1 < mm.size() && k < oot.size(); ++k)
            if ((reached >> oot[k]) & 1)
                found.departures |= 1u << mm[k + 1];
        found.mmAges = uint8_t(2 | (adultAt >= 0 && ((reached >> adultAt) & 1) ? 1 : 0));
        return found;
    }
    uint32_t Mm(uint32_t arrivals) {
        ++mmSearches;
        uint32_t departures = 0;
        for (size_t k = 0; k < mm.size() && k < oot.size(); ++k)
            if ((arrivals >> mm[k]) & 1)
                departures |= 1u << oot[k];
        return departures;
    }
};
int main() {
    // The passes cover every arrival bit of both games, warp pads and owls included, and the two ages, and one pass
    // more to confirm.
    static_assert(CE::kOotCrossArrivalCount == 17 && CE::kMmCrossArrivalCount == 17);
    static_assert(CE::kLiveWallmasterPasses == int(CE::kOotCrossArrivalCount + CE::kMmCrossArrivalCount) + 2 + 1);
    static_assert(CE::kOotWallmasterArrivalBits == 0x1ffffu && CE::kMmWallmasterArrivalBits == 0x1ffffu);
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
    {   // The warps, Full: an OoT Wallmaster to MM's Stone Tower owl, and from there an MM one to OoT's Temple of Time
        // pad, the highest bit of each game.
        const int owl = CE::MmWallmasterArrivalIndex("MM_WARP_OWL_STONE_TOWER");
        const int pad = CE::OotWallmasterArrivalIndex("OOT_WARP_SONG_TEMPLE");
        assert(owl == 16 && pad == 16 && CE::MmWallmasterArrivalRow(owl) == "MM_WARP_OWL_STONE_TOWER" &&
               CE::OotWallmasterArrivalRow(pad) == "OOT_WARP_SONG_TEMPLE");
        Chain c{ { owl }, { pad } };
        const auto arrivals = CE::LiveWallmastersOotInFront([&](const auto& a) { return c.Oot(a); },
                                                            [&](uint32_t d) { return c.Mm(d); });
        assert(arrivals.masks[1] == 1u << pad && arrivals.masks[0] == 0 && c.reached == 1u << pad);
        assert(c.ootSearches == 2);
        // MM in front, at the owl: OoT's search with MM's departure to the pad, and its departure to the owl.
        Chain m{ { owl }, { pad } };
        uint32_t lastArrivals = 0;
        const auto fromOot = CE::LiveWallmastersMmInFront(
            [&](uint32_t a) { lastArrivals = a; return m.Mm(a); },
            [&](uint32_t fromMm) {
                return CE::LiveOotDeparturesForMm(fromMm, 2, [&](const auto& a) { return m.Oot(a); });
            });
        assert(fromOot == 1u << owl && lastArrivals == 1u << owl && m.reached == 1u << pad);
    }
    {   // The longest chain: through every MM arrival bit and every OoT one in turn (dungeons, then owls and pads), with
        // adult Link reaching MM only at its end. Each pass adds a bit; the exchange settles with all of them.
        Chain c;
        for (int bit = 0; bit < int(CE::kMmCrossArrivalCount); ++bit)
            c.mm.push_back(bit);
        for (int bit = 0; bit < int(CE::kOotCrossArrivalCount); ++bit)
            c.oot.push_back(bit);
        c.adultAt = int(CE::kOotCrossArrivalCount) - 1;
        const auto arrivals = CE::LiveWallmastersOotInFront([&](const auto& a) { return c.Oot(a); },
                                                            [&](uint32_t d) { return c.Mm(d); });
        assert(arrivals.masks[0] == CE::kOotWallmasterArrivalBits && arrivals.masks[1] == CE::kOotWallmasterArrivalBits);
        assert(c.reached == CE::kOotWallmasterArrivalBits);
        assert(c.ootSearches == int(CE::kOotCrossArrivalCount) + 2 && c.ootSearches <= CE::kLiveWallmasterPasses);
        Chain m = c;
        m.ootSearches = m.mmSearches = 0;
        const auto fromOot = CE::LiveWallmastersMmInFront(
            [&](uint32_t a) { return m.Mm(a); },
            [&](uint32_t fromMm) {
                return CE::LiveOotDeparturesForMm(fromMm, 2, [&](const auto& a) { return m.Oot(a); });
            });
        assert(fromOot == CE::kMmWallmasterArrivalBits);
        assert(m.mmSearches == int(CE::kMmCrossArrivalCount) + 1 && m.mmSearches <= CE::kLiveWallmasterPasses);
    }
    std::puts("PASS: the tracker's live exchange follows Wallmaster routes into the other game and back, warp pads and "
              "owl statues included");
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
# Every arrival bit is rooted, the warp pads' (OoT bits 11-16) and the owls' (MM bits 7-16) too.
assert hostcxx.contains(fill, 'for (int bit = 0; bit < int(ComboRando::Entrances::kOotCrossArrivalCount); ++bit) {')
assert hostcxx.contains(fill, 'if (!ComboRando::OotSongOracle() && !logic->CalculatingAvailableChecks) return; '
                              'auto& departures = ComboRando::Entrances::OotWallmasterDepartures().masks;')
logic = read('mm/2s2h/Rando/Logic/Logic.cpp')
assert hostcxx.contains(logic, 'if (currentRegion == RR_MAX && (ComboRando::MmSongOracle() || evaluatingLiveTracker) '
                               '&& gSaveContext.save.linkAge <= 1) { const uint32_t arrivals = '
                               'ComboRando::Entrances::MmWallmasterArrivals().masks[gSaveContext.save.linkAge];')
assert hostcxx.contains(logic, 'for (size_t bit = 0; bit < ComboRando::Entrances::kMmCrossArrivalCount; ++bit) {')
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
# Either Full layout runs the exchange: wallmasters-full-v1, and wallmasters-full-warps-v1 with the warps. MM reads the
# layout's Wallmaster value, which is Full for both.
oot_gate = function(oot_inc, 'static bool ComboCrossGameWallmasters() {')
assert hostcxx.contains(oot_gate, 'return layout.active() && LiveCE::RequiresFullWallmasters('
                                  'layout.json().value("required", std::vector<std::string>{}));')
full_ids = function(read('combo/rando/entrances/EntranceLayout.h'),
                    'inline bool RequiresFullWallmasters(const std::vector<std::string>& required) {')
assert 'kFullWallmasterCapability' in full_ids and 'kFullWarpsWallmasterCapability' in full_ids
assert hostcxx.contains(function(mm_inc, 'static bool ComboMmCrossGameWallmasters() {'),
                        'return settings && settings->erWallmasters == LiveCE::Mode::Full;')
# The departures each game hands the other keep every arrival bit (CrossWallmasters.h's masks, the warps' included).
assert hostcxx.contains(oot_inc, '(departures[0] | departures[1]) & LiveCE::kMmWallmasterArrivalBits')
assert hostcxx.contains(mm_inc, '(departures[0] | departures[1]) & LiveCE::kOotWallmasterArrivalBits')
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
print('PASS: both trackers run their searches through the live exchange, with either Full layout, and root its '
      'arrivals, warp pads and owl statues included')
