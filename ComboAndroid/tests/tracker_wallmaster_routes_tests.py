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
   runs through the exchange while its game is in front, with either Full layout (with or without the warps; both
   trackers' gates are compiled and run on layouts installed from encoded seeds, Own Game ones and none too), and each
   game exports the other's search.
3. The game not in front keeps its answers (LiveDepartureMemo): a refresh with nothing changed runs the searches of the
   game in front only, none of the other game's (before, the other game's answer was keyed by the whole save of the game
   in front, which changes every frame, so a refresh with MM in front ran OoT's searches every time, every 20 frames).
   The answers are keyed by what the other game's search reads: its own save (MM's but its logic events, which each MM
   search clears and rewrites), the song knowledge and sources of the game in front, the options both games' live logic
   reads (every literal option read in their logic is checked to be one of them), the song map and the installed
   layout's revision, which both unified layouts change on every clear and install. A change to any of them searches
   again.
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
p.add_argument('--json-include', type=Path, required=True)
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
#include "rando/LiveTrackerRules.h"
#include "rando/LiveWallmasterRoutes.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
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
    {   // MM in front, refreshed again with nothing changed: OoT's answers are kept per arrivals for the same inputs
        // (SOH_LiveWallmasterDepartures' memo), so the repeat runs MM's own searches and none of OoT's.
        Model m;
        CE::LiveDepartureMemo memo;
        uint64_t inputs = 11;
        const auto refresh = [&] {
            return CE::LiveWallmastersMmInFront([&](uint32_t a) { return m.Mm(a); }, [&](uint32_t fromMm) {
                return memo.Answer(inputs, fromMm, [&] {
                    return CE::LiveOotDeparturesForMm(fromMm, 2, [&](const auto& a) { return m.Oot(a); });
                });
            });
        };
        assert(refresh() == 1u);
        const int ootFirst = m.ootSearches, mmFirst = m.mmSearches;
        assert(ootFirst >= 2 && mmFirst == 2);
        for (int again = 1; again <= 3; ++again) {
            assert(refresh() == 1u);
            assert(m.ootSearches == ootFirst && m.mmSearches == mmFirst * (again + 1));
        }
        std::printf("MM in front: the first refresh ran %d MM and %d OoT searches, each repeat %d MM and 0 OoT\n",
                    mmFirst, ootFirst, mmFirst);
        // Something OoT's search reads changed (its save, MM's song exports, an option): OoT is searched again, and the
        // answer follows it; the next repeat keeps that answer.
        m.forestSource = false;
        inputs = 12;
        assert(refresh() == 0u && m.ootSearches > ootFirst);
        const int ootChanged = m.ootSearches;
        assert(refresh() == 0u && m.ootSearches == ootChanged);
    }
    {   // OoT in front, the same with MM's answers (MM_LiveWallmasterDepartures' memo).
        Model m;
        CE::LiveDepartureMemo memo;
        const auto refresh = [&] {
            return CE::LiveWallmastersOotInFront([&](const auto& a) { return m.Oot(a); }, [&](uint32_t fromOot) {
                return memo.Answer(21, fromOot, [&] { return m.Mm(fromOot); });
            });
        };
        const auto first = refresh();
        const int mmFirst = m.mmSearches, ootFirst = m.ootSearches;
        assert(first.masks[1] == 1u << 3 && mmFirst >= 1);
        const auto again = refresh();
        assert(again.masks == first.masks && m.mmSearches == mmFirst && m.ootSearches == 2 * ootFirst);
    }
    {   // The longest chain, MM in front: every pass asks OoT with more arrivals, and a repeat asks none of them again.
        Chain m;
        for (int bit = 0; bit < int(CE::kMmCrossArrivalCount); ++bit)
            m.mm.push_back(bit);
        for (int bit = 0; bit < int(CE::kOotCrossArrivalCount); ++bit)
            m.oot.push_back(bit);
        m.adultAt = int(CE::kOotCrossArrivalCount) - 1;
        CE::LiveDepartureMemo memo;
        const auto refresh = [&] {
            return CE::LiveWallmastersMmInFront([&](uint32_t a) { return m.Mm(a); }, [&](uint32_t fromMm) {
                return memo.Answer(31, fromMm, [&] {
                    return CE::LiveOotDeparturesForMm(fromMm, 2, [&](const auto& a) { return m.Oot(a); });
                });
            });
        };
        assert(refresh() == CE::kMmWallmasterArrivalBits);
        const int ootFirst = m.ootSearches, mmFirst = m.mmSearches;
        assert(refresh() == CE::kMmWallmasterArrivalBits && m.ootSearches == ootFirst && m.mmSearches == 2 * mmFirst);
        std::printf("MM in front, the longest chain: the first refresh ran %d MM and %d OoT searches, a repeat %d MM "
                    "and 0 OoT\n",
                    mmFirst, ootFirst, mmFirst);
    }
    {   // A search that throws keeps nothing; more arrivals than one exchange asks for still get their own answers.
        CE::LiveDepartureMemo memo;
        int searches = 0;
        bool threw = false;
        try {
            memo.Answer(5, 1, [&]() -> uint32_t {
                ++searches;
                throw std::runtime_error("unavailable");
            });
        } catch (const std::runtime_error&) { threw = true; }
        assert(threw);
        assert(memo.Answer(5, 1, [&] { ++searches; return 7u; }) == 7u && searches == 2);
        assert(memo.Answer(5, 1, [&] { ++searches; return 9u; }) == 7u && searches == 2);
        const uint32_t many = 2u * uint32_t(CE::kLiveWallmasterPasses);
        for (uint32_t a = 0; a < many; ++a)
            assert(memo.Answer(6, a, [&] { ++searches; return a * 3; }) == a * 3);
        const int kept = searches;
        for (uint32_t a = many - uint32_t(CE::kLiveWallmasterPasses); a < many; ++a)
            assert(memo.Answer(6, a, [&] { ++searches; return 0u; }) == a * 3);
        assert(searches == kept);
        assert(memo.Answer(6, 0, [&] { ++searches; return 0u; }) == 0u && searches == kept + 1);
    }
    {   // The keys. MM's save is keyed but for its logic events, which each MM search clears and rewrites.
        uint8_t save[64];
        for (int i = 0; i < 64; ++i)
            save[i] = uint8_t(i * 5);
        const uint64_t key = CE::LiveStateKeySkipping(save, sizeof(save), 16, 8);
        save[16] ^= 0xff;
        save[23] ^= 0x0f;
        assert(CE::LiveStateKeySkipping(save, sizeof(save), 16, 8) == key);
        save[15] ^= 1;
        assert(CE::LiveStateKeySkipping(save, sizeof(save), 16, 8) != key);
        save[15] ^= 1;
        save[24] ^= 1;
        assert(CE::LiveStateKeySkipping(save, sizeof(save), 16, 8) != key);
        assert(CE::LiveStateKeySkipping(save, sizeof(save), 0, 0) == CE::LiveStateKey(save, sizeof(save)));
        assert(CE::LiveStateKeySkipping(save, sizeof(save), 64, 0) == CE::LiveStateKey(save, sizeof(save)));
        // Eight bytes a step, and the bytes after the last whole word count too.
        for (size_t at = 0; at < 13; ++at) {
            uint8_t odd[13] = {};
            const uint64_t oddKey = CE::LiveStateKey(odd, sizeof(odd));
            odd[at] = 1;
            assert(CE::LiveStateKey(odd, sizeof(odd)) != oddKey);
        }
        // What the other game reads of the game in front: its song knowledge and its song sources.
        assert(CE::LiveSongExportsKey(1, 2) == CE::LiveSongExportsKey(1, 2));
        assert(CE::LiveSongExportsKey(1, 2) != CE::LiveSongExportsKey(1, 3));
        assert(CE::LiveSongExportsKey(1, 2) != CE::LiveSongExportsKey(3, 2));
        assert(CE::LiveSongExportsKey(1, 2) != CE::LiveSongExportsKey(2, 1));
    }
    {   // The rules: every option both games' live logic reads, the song map and the layout's revision.
        std::map<std::string, int> cvars;
        const auto read = [&](const char* name, int fallback) {
            const auto it = cvars.find(name);
            return it == cvars.end() ? fallback : it->second;
        };
        const uint64_t base = CE::LiveRulesKey(read, 7, 42);
        assert(CE::LiveRulesKey(read, 7, 42) == base && CE::LiveRulesKey(read, 7, 43) != base); // seeded by the exports
        cvars["gCombo.CheckTracker.ShowLogic"] = 0; // not read by any search
        assert(CE::LiveRulesKey(read, 7, 42) == base);
        const auto changes = [&](const char* name, int value) {
            cvars[name] = value;
            const bool changed = CE::LiveRulesKey(read, 7, 42) != base;
            cvars.erase(name);
            return changed && CE::LiveRulesKey(read, 7, 42) == base;
        };
        for (const auto& rule : ComboRando::kGameplayRules)
            assert(changes(rule.name, rule.fallback == rule.maximum ? rule.minimum : rule.maximum));
        for (const auto& trick : ComboRando::kMmTricks)
            assert(changes(trick.key, 1));
        assert(changes(CE::kLiveMoonCrashRule, 1));
        auto& songs = ComboRando::ActiveSongTravel();
        const auto saved = songs;
        songs.foreignSongs = !songs.foreignSongs;
        assert(CE::LiveRulesKey(read, 7, 42) != base);
        songs = saved;
        std::swap(songs.destinations[0], songs.destinations[6]);
        assert(CE::LiveRulesKey(read, 7, 42) != base);
        songs = saved;
        assert(CE::LiveRulesKey(read, 7, 42) == base);
        assert(CE::LiveRulesKey(read, 8, 42) != base && CE::LiveRulesKey(read, 7ull << 32, 42) != base);
    }
    std::puts("PASS: the tracker's live exchange follows Wallmaster routes into the other game and back, warp pads and "
              "owl statues included");
    return 0;
}
'''
path = out / 'tracker_wallmaster_routes.cpp'
path.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(a.compiler, path, includes=[source / 'combo', a.json_include])
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
# Either Full layout runs the exchange: wallmasters-full-v1, and wallmasters-full-warps-v1 with the warps; an Own Game
# layout, with or without the warps, and no layout do not. Both trackers' gates are compiled as they are and run on the
# installed layouts of seeds EncodeEntranceLayout writes, so a gate that reads the layout by another name than the
# encoder's fails here (the OoT one read "required", where the encoder writes "requires", and never ran the exchange).
gates = out / 'tracker_wallmaster_gates.cpp'
gates.write_text(r'''
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <cstdio>
#include <set>
namespace LiveCE = ComboRando::Entrances;
''' + function(oot_inc, 'static bool ComboCrossGameWallmasters() {') + '\n'
                 + function(mm_inc, 'static bool ComboMmCrossGameWallmasters() {') + r'''
using namespace ComboRando::Entrances;
int main() {
    const auto caps = CapabilitySet::AtLeast(Stage::Hidden);
    struct Kind { Mode wallmasters; bool warps; std::string_view id; };
    const Kind kinds[] = {{Mode::Full, false, kFullWallmasterCapability},
                          {Mode::Full, true, kFullWarpsWallmasterCapability},
                          {Mode::OwnGame, false, kVanillaWallmasterCapability},
                          {Mode::OwnGame, true, kOwnGameWarpsWallmasterCapability}};
    assert(!ComboCrossGameWallmasters() && !ComboMmCrossGameWallmasters());
    for (const auto& kind : kinds) {
        EntranceLayout layout;
        layout.settings.erWallmasters = kind.wallmasters;
        layout.settings.erWarps = kind.warps ? Warps::Full : Warps::None;
        layout.required = {std::string(kind.id)};
        const auto pools = kind.warps ? LayoutV3::MakePools(layout.settings, layout.ootMq)
                         : kind.wallmasters == Mode::Full ? LayoutV2::MakePools(layout.settings, layout.ootMq)
                                                          : LayoutV1::MakePools(layout.settings, layout.ootMq, false);
        // Warp i lands at warp i + 5; each other source, in name order, takes its pool's first free candidate.
        Overrides routes;
        if (kind.warps)
            for (size_t i = 0; i < kWarpRows.size(); ++i)
                routes[kWarpRows[i]] = kWarpRows[(i + 5) % kWarpRows.size()];
        std::set<std::string_view> used;
        for (const auto& pool : pools)
            for (const auto src : pool.src)
                for (const auto dst : pool.dst) {
                    const auto* from = LayoutV3::FindRow(src);
                    const auto* to = LayoutV3::FindRow(dst);
                    if (!routes.contains(src) && !used.contains(dst) && LayoutV1::RowAssignable(*from, *to, pool.ownGame)) {
                        routes[src] = dst;
                        used.insert(dst);
                    }
                }
        layout.overrides = routes;
        Validate(layout, caps);
        // Installed as a seed is loaded (combo_entrance_overlay.cpp, ComboEntrancesMM.cpp): from the encoded layout.
        const nlohmann::json seed{{"entranceLayout", EncodeEntranceLayout(layout)}};
        ActiveOotUnified().install(seed, caps);
        ActiveMmUnified().install(seed, caps);
        const bool full = kind.wallmasters == Mode::Full;
        assert(ComboCrossGameWallmasters() == full && ComboMmCrossGameWallmasters() == full);
        ActiveOotUnified().clear();
        ActiveMmUnified().clear();
        assert(!ComboCrossGameWallmasters() && !ComboMmCrossGameWallmasters());
    }
    std::puts("PASS: both trackers' gates run the exchange for either installed Full layout, not for Own Game or none");
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, gates, includes=[source / 'combo', a.json_include],
                    defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/bigobj'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
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
# Each export answers only under its game's gate, the one run above.
assert '!ComboCrossGameWallmasters()' in oot_export and '!ComboMmCrossGameWallmasters()' in mm_export
# The exchange leaves no arrivals behind for an oracle's or the playthrough's search.
for text, arrivals in ((oot_inc, 'LiveCE::OotWallmasterArrivals() = {};'), (mm_inc, 'LiveCE::MmWallmasterArrivals() = {};')):
    scope = text[text.index('ExchangeScope() {'):]
    assert arrivals in scope[:scope.index(chr(10) + '};')]
print('PASS: both trackers run their searches through the live exchange, with either Full layout, and root its '
      'arrivals, warp pads and owl statues included')

# ---- 3. The kept answers, in production.
# The game in front hands the other what that game's search reads of it, its song exports, never its own save.
assert hostcxx.contains(oot_search, 'const uint64_t ootState = LiveCE::LiveSongExportsKey(SOH_GetLiveSongKnowledge(), '
                                    'SOH_GetLiveSongSources());')
assert hostcxx.contains(mm_search, 'const uint64_t mmState = LiveCE::LiveSongExportsKey(MM_GetLiveSongKnowledge(), '
                                   'MM_GetLiveSongSources());')
for search in (oot_search, mm_search):
    assert 'LiveStateKey(&gSaveContext' not in search, 'the game in front keys the answers by its own save again'
# The game not in front keeps its answers for what its search reads, and searches only when they are not kept.
assert hostcxx.contains(oot_export, 'static LiveCE::LiveDepartureMemo memo;')
assert hostcxx.contains(oot_export, 'const uint64_t inputs = LiveCE::LiveStateKey(&gSaveContext, sizeof(gSaveContext), '
                                    'LiveCE::LiveRulesKey(CVarGetInteger, LiveCE::ActiveOotUnified().revision(), '
                                    'mmState));')
assert hostcxx.find(oot_export, 'if (!GameInteractor::IsSaveLoaded()) return 0;') < \
    hostcxx.find(oot_export, 'return memo.Answer(inputs, fromMm, [&] {') < hostcxx.find(oot_export, 'ReachabilitySearch(')
assert hostcxx.contains(mm_export, 'static LiveCE::LiveDepartureMemo memo;')
assert hostcxx.contains(mm_export, 'const size_t events = static_cast<size_t>(reinterpret_cast<const char*>(&RANDO_EVENTS) '
                                   '- reinterpret_cast<const char*>(&gSaveContext));')
assert hostcxx.contains(mm_export, 'const uint64_t inputs = LiveCE::LiveStateKeySkipping(&gSaveContext, '
                                   'sizeof(gSaveContext), events, sizeof(RANDO_EVENTS), '
                                   'LiveCE::LiveRulesKey(CVarGetInteger, LiveCE::ActiveMmUnified().revision(), '
                                   'ootState));')
assert hostcxx.find(mm_export, 'return memo.Answer(inputs, fromOot, [&] {') < \
    hostcxx.find(mm_export, 'Rando::CheckTracker::ComputeChecksInLogicOnce(inLogic)')
for export in (oot_export, mm_export):
    assert 'cachedKey' not in export and 'static uint32_t cached' not in export
# MM's search clears the logic events it keeps in the save before it counts them, so they are no input of it.
once = function(mm_tracker, 'void Rando::CheckTracker::ComputeChecksInLogicOnce(std::unordered_map<RandoCheckId, bool>& '
                            'inLogic) {')
assert hostcxx.find(once, 'RANDO_EVENTS[i] = 0;') < hostcxx.find(once, 'RANDO_EVENTS[event.first]++;')
# What each search reads of the other game is those exports: OoT's search resolves only MM's two song exports...
otr = read('soh/soh/OTRGlobals.cpp')
for signature in ('ComboRando::SongAgeArrivals SOH_LiveIncomingSongAges() {',
                  'uint8_t SOH_LiveMmEntryAges(const std::array<uint32_t, 2>& wallmasterDepartures) {',
                  'static uint32_t OotLiveSoaringSources(bool logical = false) {'):
    reads = set(re.findall(r'Combo_ResolveSym\("2ship", "(\w+)"\)', function(otr, signature)))
    assert reads <= {'MM_GetLiveSongSources', 'MM_GetLiveSongKnowledge'}, (signature, reads)
assert 'Combo_ResolveSym' not in function(otr, 'static LiveOotSongDepartures LiveSongDepartures(uint32_t mmKnowledge) {')
for rel in ('soh/soh/Enhancements/randomizer/3drando/fill.cpp', 'soh/soh/Enhancements/randomizer/logic.cpp'):
    assert 'Combo_ResolveSym("2ship"' not in read(rel), rel
# ...and MM's only OoT's two (MM_LiveIncomingSongs for the tracker, and MmLiveSongSources).
benport = read('mm/2s2h/BenPort.cpp')
incoming = function(benport, 'extern "C" uint32_t MM_LiveIncomingSongs() {')
logic_branch = 'auto fn = (uint32_t(*)())Combo_ResolveSym("soh", "SOH_GetLiveSongLogicSources");'
tracker_branch = incoming[:incoming.index(logic_branch)]
assert set(re.findall(r'Combo_ResolveSym\("soh", "(\w+)"\)', tracker_branch)) == {'SOH_GetLiveSongSources'}
assert set(re.findall(r'Combo_ResolveSym\("soh", "(\w+)"\)', function(benport, 'static uint32_t MmLiveSongSources('
                                                                                   'bool logical) {'))) == \
    {'SOH_GetLiveSongKnowledge'}
mm_logic = sorted((source / 'mm/2s2h/Rando/Logic').rglob('*.[ch]*'))
assert mm_logic and not any('Combo_ResolveSym("soh"' in path.read_text(encoding="utf-8", errors="replace")
                             for path in mm_logic)
# Every option the live logic reads by name is in the rules' key: a gameplay rule, an MM trick or Save on Moon Crash.
rules = read('combo/rando/GameplayRules.h')
named = set()
for path in mm_logic + [source / 'soh/soh/Enhancements/randomizer/logic.cpp',
                        source / 'soh/soh/Enhancements/randomizer/3drando/fill.cpp'] + \
        sorted((source / 'soh/soh/Enhancements/randomizer/location_access').rglob('*.cpp')):
    named |= set(re.findall(r'CVarGetInteger\("([^"]+)"', path.read_text(encoding="utf-8", errors="replace")))
assert named, 'no option read by name'
for name in named:
    assert name == 'gCombo.Tricks.MM.' or name == 'gEnhancements.Cycle.SaveOnMoonCrash' or \
        f'{{ "{name}", "' in rules, f'the live logic reads {name}, which the kept answers do not key'
live_rules = read('combo/rando/LiveTrackerRules.h')
assert 'inline constexpr const char* kLiveMoonCrashRule = "gEnhancements.Cycle.SaveOnMoonCrash";' in live_rules
assert hostcxx.contains(read('mm/2s2h/Rando/Logic/Logic.h'), '#define COMBO_MM_TRICK(name)') and \
    hostcxx.contains(read('mm/2s2h/Rando/Logic/Logic.h'), ': CVarGetInteger("gCombo.Tricks.MM." name, 0))')
trick_keys = re.findall(r'\{ \d+, "([^"]+)",', read('combo/rando/MmTricks.h'))
assert trick_keys and all(key.startswith('gCombo.Tricks.MM.') for key in trick_keys)
# Each unified layout changes its revision, which the rules' key holds, on every clear and on every install that
# succeeds; both seed binders clear it first (MM_ApplyEntranceExpansionForCombo, ComboEntrances::SetLayout).
for rel, name in (('combo/rando/entrances/OotUnifiedLayout.h', 'class OotUnifiedLayout {'),
                  ('combo/rando/entrances/MmUnifiedLayout.h', 'class MmUnifiedLayout {')):
    layout = function(read(rel), name)
    assert hostcxx.contains(layout, 'uint64_t revision() const { return revision_; }')
    clear = function(layout, 'void clear() {')
    assert hostcxx.contains(clear, '++revision_;'), rel
    install = function(layout, 'void install(const nlohmann::json& seed, const CapabilitySet& caps) {')
    assert hostcxx.find(install, 'warpTable = std::move(nextWarps);') < hostcxx.find(install, '++revision_;'), rel
mm_bind = function(read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp'),
                   'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(const char* seedJson) {')
assert hostcxx.find(mm_bind, 'CE::ActiveMmUnified().clear();') < hostcxx.find(mm_bind, 'CE::ActiveMmUnified().install(')
oot_bind = function(read('soh/soh/Enhancements/randomizer/combo_entrance_overlay.cpp'),
                    'int SetLayout(const char* seedJson) {')
assert hostcxx.find(oot_bind, 'CE::ActiveOotUnified().clear();') < hostcxx.find(oot_bind, 'CE::ActiveOotUnified().install(')
print('PASS: the game not in front keeps its answers for what its search reads, and a refresh with nothing changed '
      'searches it not at all')
