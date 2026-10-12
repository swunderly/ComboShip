"""Host stubs shared by the Temple probe harnesses.

The prelude compiles the production TempleProbeCore.h together with the production age/time code it
relies on (Region::ApplyTimePass, Region::UpdateEvents, EventAccess and Entrance
CheckConditionAtAgeTime, fill.cpp UpdateToDAccess), extracted from the sources, against a tiny engine:
a stub Logic, glitchless condition functions and an areaTable of stub regions.
"""
from pathlib import Path
import hostcxx

RANDOMIZER = 'soh/soh/Enhancements/randomizer'


def function(text, signature):
    """The whole function whose header matches signature (whitespace-insensitive)."""
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise ValueError('unterminated ' + signature)


def pinned(source):
    """Production functions the probe reuses or mirrors, by name."""
    randomizer = Path(source)/RANDOMIZER
    access = (randomizer/'location_access.cpp').read_text(encoding='utf-8')
    entrance = (randomizer/'entrance.cpp').read_text(encoding='utf-8')
    fill = (randomizer/'3drando/fill.cpp').read_text(encoding='utf-8')
    return {
        'UpdateToDAccess': function(fill, 'static bool UpdateToDAccess(Entrance* entrance, Region* connection)'),
        'Region::ApplyTimePass': function(access, 'void Region::ApplyTimePass()'),
        'Region::UpdateEvents': function(access, 'bool Region::UpdateEvents()'),
        'EventAccess::CheckConditionAtAgeTime': function(access, 'bool EventAccess::CheckConditionAtAgeTime(bool& age, bool& time)'),
        'Entrance::CheckConditionAtAgeTime': function(
            entrance, 'bool Entrance::CheckConditionAtAgeTime(bool& age, bool& time, bool passAnyway) const'),
    }


def prelude(source):
    p = pinned(source)
    return r'''
#define NOMINMAX
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#define StartPerformanceTimer(timer) ((void)0)
#define StopPerformanceTimer(timer) ((void)0)
enum RandomizerRegion {
    RR_NONE, RR_ROOT, RR_ROOT_EXITS, RR_CHILD_SPAWN, RR_ADULT_SPAWN, RR_KF_LINKS_HOUSE, RR_TOT_ENTRANCE,
    RR_TEMPLE_OF_TIME, RR_TOT_BEYOND_DOOR_OF_TIME, RR_A, RR_B, RR_C, RR_D, RR_MAX
};
enum LogicVal { LOGIC_NONE, LOGIC_EVENT_A, LOGIC_EVENT_B, LOGIC_SETTING_EVENT, LOGIC_MAX };
enum StubItem { PRELUDE = 1, KEY, SWORD, BOOTS, SONG_OF_TIME };
struct SaveContext { uint8_t bytes[64]; };
namespace Rando {
struct Logic {
    bool IsChild = false, IsAdult = false, AtDay = false, AtNight = false;
    RandomizerRegion CurrentRegionKey = RR_NONE;
    uint8_t BaseHearts = 0;
    bool inLogic[LOGIC_MAX] = {};
    std::set<int> items;
    SaveContext* mSaveContext = nullptr;
    bool Get(LogicVal v) const { return inLogic[v]; }
    void Set(LogicVal v, bool value) { inLogic[v] = value; }
    bool HasItem(int item) const { return items.count(item) != 0; }
};
}
std::shared_ptr<Rando::Logic> logic;
typedef bool (*ConditionFn)();
class Region;
Region* RegionTable(RandomizerRegion key);
class EventAccess {
  public:
    EventAccess(LogicVal event_, ConditionFn condition) : event(event_), condition_function(condition) {}
    bool ConditionsMet() const { return condition_function(); } // glitchless
    bool CheckConditionAtAgeTime(bool& age, bool& time);
    void EventOccurred() { logic->Set(event, true); }
    bool GetEvent() const { return logic->Get(event); }
  private:
    LogicVal event;
    ConditionFn condition_function;
};
namespace Rando {
class Entrance {
  public:
    Entrance(RandomizerRegion connected, ConditionFn condition, RandomizerRegion parent)
        : connectedRegion(connected), condition_function(condition), parentRegion(parent) {}
    bool GetConditionsMet() const { return condition_function(); } // glitchless
    bool CheckConditionAtAgeTime(bool& age, bool& time, bool passAnyway = false) const;
    RandomizerRegion GetConnectedRegionKey() const { return connectedRegion; }
    Region* GetParentRegion() const { return RegionTable(parentRegion); }
  private:
    RandomizerRegion connectedRegion;
    ConditionFn condition_function;
    RandomizerRegion parentRegion;
};
}
using Rando::Entrance;
class Region {
  public:
    bool timePass = false;
    std::vector<EventAccess> events;
    std::list<Entrance> exits;
    std::vector<int> locations; // items a native search would collect; the probe must never read them
    bool childDay = false, childNight = false, adultDay = false, adultNight = false, addedToPool = false;
    bool TimePass() { return timePass; }
    void ApplyTimePass();
    bool UpdateEvents();
    bool Child() const { return childDay || childNight; }
    bool Adult() const { return adultDay || adultNight; }
};
std::array<Region, RR_MAX> areaTable;
Region* RegionTable(RandomizerRegion key) { return &areaTable[key]; }
''' + p['EventAccess::CheckConditionAtAgeTime'] + '\n' + p['Region::ApplyTimePass'] + '\n' + \
        p['Region::UpdateEvents'] + '\nnamespace Rando {\n' + p['Entrance::CheckConditionAtAgeTime'] + \
        '\n}\n' + p['UpdateToDAccess'] + '\n#include "TempleProbeCore.h"\n' + r'''
using namespace Rando::TempleProbe;

// A small native-shaped world: Root -> Root Exits -> both spawns and a Prelude warp, the spawns'
// vanilla targets, a child route Link's House -> A -> (KEY) -> ToT Entrance -> ToT, and the door.
bool True() { return true; }
void Link(RandomizerRegion from, RandomizerRegion to, ConditionFn condition) {
    areaTable[from].exits.emplace_back(to, condition, from);
}
void World() {
    for (auto& r : areaTable)
        r = Region{};
    Link(RR_ROOT, RR_ROOT_EXITS, True);
    Link(RR_ROOT_EXITS, RR_CHILD_SPAWN, [] { return logic->IsChild; });
    Link(RR_ROOT_EXITS, RR_ADULT_SPAWN, [] { return logic->IsAdult; });
    Link(RR_ROOT_EXITS, RR_TEMPLE_OF_TIME, [] { return logic->HasItem(PRELUDE); }); // never followed
    Link(RR_CHILD_SPAWN, RR_KF_LINKS_HOUSE, True);
    Link(RR_ADULT_SPAWN, RR_TEMPLE_OF_TIME, True);
    Link(RR_KF_LINKS_HOUSE, RR_A, True);
    Link(RR_A, RR_TOT_ENTRANCE, [] { return logic->HasItem(KEY); });
    Link(RR_TOT_ENTRANCE, RR_TEMPLE_OF_TIME, True);
    Link(RR_TEMPLE_OF_TIME, RR_TOT_ENTRANCE, True);
    Link(RR_TEMPLE_OF_TIME, RR_TOT_BEYOND_DOOR_OF_TIME, [] { return logic->HasItem(SWORD); });
    Link(RR_TOT_BEYOND_DOOR_OF_TIME, RR_TEMPLE_OF_TIME, True);
    areaTable[RR_KF_LINKS_HOUSE].timePass = areaTable[RR_A].timePass = true;
}
const Entrance* Exit(RandomizerRegion parent, RandomizerRegion target) {
    const Entrance* found = nullptr;
    for (const auto& e : areaTable[parent].exits)
        if (e.GetConnectedRegionKey() == target) {
            if (found)
                throw Failure(kUnsupported, "several exits");
            found = &e;
        }
    if (!found)
        throw Failure(kUnsupported, "no exit");
    return found;
}

// Row name -> bound native exit, as TempleProbe.cpp's generated table.
std::map<std::string, std::pair<RandomizerRegion, RandomizerRegion>> gBindings = {
    { "ROW_KEY_DOOR", { RR_A, RR_TOT_ENTRANCE } },
};
std::shared_ptr<Rando::Logic> gLive; // the Context's Logic
struct StubPolicy {
    bool startingChild = false;
    std::set<int> starting;
    bool scope = false;
    const Entrance* rootSpawnOverride = nullptr;
    std::function<void()> check = [] {};
    std::function<void()> onApply = [] {};
    int applied = 0;
    std::string_view Registry() const { return "stub-registry"; }
    void Check() { check(); }
    std::array<Region, RR_MAX>& Regions() { return areaTable; }
    std::shared_ptr<Rando::Logic>& Handle() { return logic; }
    bool& Scope() { return scope; }
    size_t Root() const { return RR_ROOT; }
    size_t Target() const { return RR_TEMPLE_OF_TIME; }
    bool StartingChild() const { return startingChild; }
    bool SameLogic() const { return logic == gLive; }
    std::shared_ptr<Rando::Logic> MakeLogic() const { return std::make_shared<Rando::Logic>(); }
    void ApplyStartingItems() {
        assert(scope && logic != gLive);
        ++applied;
        logic->items.insert(starting.begin(), starting.end());
        onApply();
    }
    Overlay<Entrance> Resolve(const QueryRequest& q) const {
        auto bound = [](const std::string& row) {
            const auto it = gBindings.find(row);
            if (it == gBindings.end())
                throw Failure(kUnsupported, "no native binding for " + row);
            return it->second;
        };
        Overlay<Entrance> overlay;
        for (const auto& row : q.deleted)
            overlay.deleted.push_back(Exit(bound(row).first, bound(row).second));
        std::sort(overlay.deleted.begin(), overlay.deleted.end(), std::less<const Entrance*>());
        for (const auto& e : q.edges)
            overlay.added.push_back({ static_cast<size_t>(bound(e.src).first), Exit(bound(e.src).first, bound(e.src).second),
                                      static_cast<size_t>(bound(e.dst).second) });
        const auto spawn = startingChild ? RR_ADULT_SPAWN : RR_CHILD_SPAWN;
        overlay.rootSpawn = rootSpawnOverride ? rootSpawnOverride : Exit(RR_ROOT_EXITS, spawn);
        overlay.rootSpawnTarget = spawn;
        return overlay;
    }
};
std::string Query(std::vector<std::string> deleted = {}, std::vector<std::pair<std::string, std::string>> edges = {}) {
    nlohmann::json q = { { "schema", "oot-temple-probe-query-v1" }, { "registry", "stub-registry" },
                         { "token", 7 }, { "assumedInstalled", true }, { "assumed", nlohmann::json::array() },
                         { "deleted", deleted }, { "edges", nlohmann::json::array() },
                         { "portalLinks", nlohmann::json::array() },
                         { "mm", { { "deleted", nlohmann::json::array() }, { "edges", nlohmann::json::array() } } } };
    for (const auto& [src, dst] : edges)
        q["edges"].push_back({ { "src", src }, { "dst", dst } });
    return q.dump();
}
struct Outcome {
    int code;
    uint8_t ages;
    std::string error;
    bool operator==(const Outcome& o) const { return code == o.code && ages == o.ages; }
};
Outcome Run(StubPolicy& policy, const std::string& json, int (*poll)(void) = nullptr) {
    Outcome o{ 0, 0xff, {} };
    o.code = Firewall(o.error, [&] { return RunQuery(policy, json.c_str(), poll, &o.ages); });
    return o;
}
'''
