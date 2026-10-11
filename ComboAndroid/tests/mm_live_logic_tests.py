"""Compile the MM tracker's live search (ComputeChecksInLogic) and Saria's Song hint that picks from it.

The Logic graph walk is a stub (an exit is open for a fixed set of slices, and a region explored
without a time state gets every owned slice, as EnsureRegionTimeState does); the search itself and
the hint are extracted unchanged from production.
"""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()


def function(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


tracker = (a.source/'mm/2s2h/Rando/CheckTracker/CheckTracker.cpp').read_text(encoding='utf-8')
hint = (a.source/'mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp').read_text(encoding='utf-8')
search = function(tracker, 'void Rando::CheckTracker::ComputeChecksInLogic(')
pick = function(hint, 'RandoCheckId GetProgressiveCheckInLogic(')

code = r'''
#define COMBO_BUILD 1
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
enum RandoCheckId { RC_UNKNOWN, RC_RANCH_EPONAS_SONG, RC_RANCH_BARN, RC_BOSSA_NOVA, RC_MAX };
enum RandoRegionId { RR_FIELD, RR_RANCH, RR_FORTRESS, RR_PINNACLE, RR_MAX };
enum RandoEvent { RE_ACCESS_ZORA_EGG, RE_MAX };
enum RandoItemId { RI_UNKNOWN, RI_BOW, RI_RUPEE_GREEN };
enum RandoItemType { RITYPE_JUNK, RITYPE_MAJOR, RITYPE_MASK };
struct RandoSaveCheck { RandoItemId randoItemId; bool shuffled; bool obtained; };
struct RandoSaveInfo { RandoSaveCheck randoSaveChecks[RC_MAX]; uint8_t randoEvents[RE_MAX]; };
struct { struct { int entrance; struct { RandoSaveInfo rando; } shipSaveInfo; } save; } gSaveContext;
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
#define RANDO_EVENTS gSaveContext.save.shipSaveInfo.rando.randoEvents
constexpr uint64_t DAY1 = 1, DAY2 = 2, DAY3 = 4, OWNED = DAY1 | DAY2 | DAY3;
uint64_t now = DAY1;
std::map<RandoRegionId, std::vector<std::pair<RandoRegionId, uint64_t>>> exits;
int liveScopes = 0; // checks are judged by the live rules, resources and ACCESS state
struct Scope {
    Scope() { ++liveScopes; }
    ~Scope() { --liveScopes; }
};
namespace ComboRando { using MmResourceScope = Scope; }
namespace Rando {
namespace Logic {
using LiveTrackerRuleScope = Scope;
using MmAccessScope = Scope;
struct RegionTimeState { uint64_t timeSlices; bool canStayOverTime; };
struct RandoRegion {
    std::map<RandoCheckId, std::pair<std::function<bool()>, std::string>> checks;
    std::vector<std::pair<RandoEvent, std::function<bool()>>> events;
    bool canStayOverTime = true;
};
std::map<RandoRegionId, RandoRegion> Regions;
uint64_t gCurrentRegionTime = 0;
RandoRegionId GetRegionIdFromEntrance(int entrance) { return RandoRegionId(entrance); }
std::unordered_map<RandoRegionId, RegionTimeState> InitializeRegionTimeStates(RandoRegionId start) {
    return { { start, { OWNED, true } } };
}
void FindReachableRegions(RandoRegionId from, std::set<RandoRegionId>& reachable,
                          std::unordered_map<RandoRegionId, RegionTimeState>& times) {
    const uint64_t fromTimes = times.try_emplace(from, RegionTimeState{ OWNED, true }).first->second.timeSlices;
    reachable.insert(from);
    for (auto [to, open] : exits[from])
        if (fromTimes & open) {
            reachable.insert(to);
            times[to].timeSlices |= fromTimes & open;
        }
}
bool UpdateMmAccess() { return false; }
bool UpdateResourceSources(const std::set<RandoRegionId>&, const std::unordered_map<RandoRegionId, RegionTimeState>&) {
    return false;
}
void SetCurrentRegionTime(const std::unordered_map<RandoRegionId, RegionTimeState>& times, RandoRegionId region) {
    gCurrentRegionTime = times.at(region).timeSlices;
}
namespace TimeLogic {
uint64_t EvaluateConditionTimes(const std::function<bool()>& condition, uint64_t timeSlices) {
    const uint64_t previous = gCurrentRegionTime;
    uint64_t allowed = 0;
    for (uint64_t bit = 1; bit <= timeSlices; bit <<= 1)
        if (timeSlices & bit) {
            gCurrentRegionTime = bit;
            if (condition())
                allowed |= bit;
        }
    gCurrentRegionTime = previous;
    return allowed;
}
uint64_t CurrentTimeSlice() { return now; }
uint64_t GetOwnedTimeSlices() { return OWNED; }
} // namespace TimeLogic
} // namespace Logic
namespace CheckTracker { void ComputeChecksInLogic(std::unordered_map<RandoCheckId, bool>& inLogic); }
namespace StaticData { struct Item { RandoItemType randoItemType; }; std::map<RandoItemId, Item> Items; }
std::vector<RandoItemId> GetSariaPriorityItemsFromSave(RandoSaveInfo&) { return { RI_BOW }; }
RandoItemId ConvertItem(RandoItemId item, RandoCheckId) { return item; }
} // namespace Rando
template <typename Low, typename High> Low Ship_Random(Low low, High) { return low; }
''' + search + '\n' + pick + r'''
static bool InLogic(RandoRegionId here, uint64_t time, RandoCheckId check) {
    gSaveContext.save.entrance = here;
    now = time;
    std::unordered_map<RandoCheckId, bool> inLogic;
    Rando::CheckTracker::ComputeChecksInLogic(inLogic);
    return inLogic.contains(check);
}
static RandoCheckId Hint(RandoRegionId here, uint64_t time) {
    gSaveContext.save.entrance = here;
    now = time;
    return GetProgressiveCheckInLogic();
}
int main() {
    using Rando::Logic::gCurrentRegionTime;
    Rando::StaticData::Items = { { RI_BOW, { RITYPE_MAJOR } }, { RI_RUPEE_GREEN, { RITYPE_JUNK } } };
    // Romani Ranch: the boulder on the road is gone only on Day 3; Epona's Song is a Day 1 check.
    exits[RR_MAX] = { { RR_FIELD, OWNED } };
    exits[RR_FIELD] = { { RR_RANCH, DAY3 } };
    auto& ranch = Rando::Logic::Regions[RR_RANCH];
    ranch.checks[RC_RANCH_EPONAS_SONG] = { [] { return (gCurrentRegionTime & DAY1) != 0; }, "" };
    ranch.checks[RC_RANCH_BARN] = { [] { return liveScopes == 3; }, "" };
    Rando::Logic::Regions[RR_FIELD];
    RANDO_SAVE_CHECKS[RC_RANCH_EPONAS_SONG] = { RI_BOW, true, false };
    RANDO_SAVE_CHECKS[RC_RANCH_BARN] = { RI_RUPEE_GREEN, true, false };

    // Standing in the ranch on Day 3 proves access from Day 3 on only.
    assert(InLogic(RR_RANCH, DAY3, RC_RANCH_BARN));
    assert(!InLogic(RR_RANCH, DAY3, RC_RANCH_EPONAS_SONG));
    // Saria's Song uses the same search, so it never spends the hint on the Day 1 check.
    assert(Hint(RR_RANCH, DAY3) == RC_UNKNOWN);
    assert(Hint(RR_FIELD, DAY2) == RC_UNKNOWN);
    // A leftover time from an earlier evaluation does not leak into a region's checks.
    gCurrentRegionTime = DAY1;
    assert(Hint(RR_FIELD, DAY1) == RC_UNKNOWN);
    // Standing there on Day 1 does make it obtainable, and the hint names it.
    assert(InLogic(RR_RANCH, DAY1, RC_RANCH_EPONAS_SONG));
    assert(Hint(RR_RANCH, DAY1) == RC_RANCH_EPONAS_SONG);
    RANDO_SAVE_CHECKS[RC_RANCH_EPONAS_SONG].obtained = true;
    assert(!InLogic(RR_RANCH, DAY1, RC_RANCH_EPONAS_SONG) && Hint(RR_RANCH, DAY1) == RC_UNKNOWN);

    // RE_ACCESS_ZORA_EGG counts one EVENT entry per egg: four in Pirates' Fortress, three at Pinnacle Rock.
    static bool lastEgg = true;
    exits[RR_FIELD].push_back({ RR_FORTRESS, OWNED });
    exits[RR_FIELD].push_back({ RR_PINNACLE, OWNED });
    for (int egg = 0; egg < 4; ++egg)
        Rando::Logic::Regions[RR_FORTRESS].events.push_back({ RE_ACCESS_ZORA_EGG, [] { return true; } });
    for (int egg = 0; egg < 3; ++egg)
        Rando::Logic::Regions[RR_PINNACLE].events.push_back(
            { RE_ACCESS_ZORA_EGG, [egg] { return egg < 2 || lastEgg; } });
    Rando::Logic::Regions[RR_FIELD].checks[RC_BOSSA_NOVA] = { [] { return RANDO_EVENTS[RE_ACCESS_ZORA_EGG] >= 7; },
                                                              "" };
    RANDO_SAVE_CHECKS[RC_BOSSA_NOVA] = { RI_RUPEE_GREEN, true, false };
    for (int refresh = 0; refresh < 2; ++refresh) {
        assert(InLogic(RR_FIELD, DAY1, RC_BOSSA_NOVA));
        assert(RANDO_EVENTS[RE_ACCESS_ZORA_EGG] == 7); // recounted each search, never accumulated
    }
    lastEgg = false;
    assert(!InLogic(RR_FIELD, DAY1, RC_BOSSA_NOVA) && RANDO_EVENTS[RE_ACCESS_ZORA_EGG] == 6);
    std::puts("MM live search: current-time seed, per-region times, Saria's Song hint and Zora egg count passed");
}
'''
a.build_dir.mkdir(parents=True, exist_ok=True)
source = a.build_dir/'mm_live_logic.cpp'
source.write_text(code, encoding='utf-8')
binary = hostcxx.build(a.compiler, source, werror=True)
subprocess.run([str(binary)], check=True, timeout=hostcxx.RUN_TIMEOUT)
