"""The Temple probe's GLOBAL node against OoTMM's own validateAgeTemple (Wallmaster limits design, Stage 2).

tools/ootmm/temple-song-facts.json records the pinned validateAgeTemple run (run_pinned_entrance_pools.mts --temple)
on every case of tools/ootmm/temple-song-fixtures.json: a starting age, a world (dekuTree open, dekuTree closed as the
Beginner preset sets it, or a crafted Closed Forest), an instrument (no ocarina, the Fairy Ocarina, shuffled buttons,
a missing C-Left, shared buttons without C-Down) and no starting warp song or one of the six OoT ones. Each songless
case also records whether the opposite age walks to the Temple of Time from its spawn area and from each warp-song
landing.

This runs the production probe core (TempleProbeCore.h RunQuery) on the same cases, with the production GLOBAL edges
(TempleProbe.cpp GlobalEdges, WarpLanding, BoundExit and the generated combo_oot_edge_bindings.inc), Root Exits and
its warp regions as root.cpp has them, and the production warp-song predicate (logic.cpp CanUse: the song, an ocarina
and its notes, read from the source), on the real region and entrance enums. Between the spawn regions, the landings
and the Temple only OoTMM's recorded walks are linked, so what is compared is GLOBAL itself: where the opposite age
enters it, which songs play with which instrument, and where they land. Every case must give OoTMM's answer and its
ages. With the GLOBAL edges left out, the cases where the song decides must disagree.

The rest of Ship's world, which the probe searches natively, is not run here: the device checks (DEVICE_TESTS.md)
compare the shipped probe with seeds.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess
import hostcxx
import temple_probe_stubs as stubs

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
randomizer = source/stubs.RANDOMIZER
probe = (randomizer/'TempleProbe.cpp').read_text(encoding='utf-8')
root = (randomizer/'location_access/root.cpp').read_text(encoding='utf-8')
logic_cpp = (randomizer/'logic.cpp').read_text(encoding='utf-8')
fixture = json.loads((source/'tools/ootmm/temple-song-fixtures.json').read_text(encoding='utf-8'))
facts = json.loads((source/'tools/ootmm/temple-song-facts.json').read_text(encoding='utf-8'))
audit = json.loads((source/'tools/ootmm/oot-edge-audit.json').read_text(encoding='utf-8'))
registry = {e['name']: e for e in json.loads((source/'tools/ootmm/registry-source.json').read_text(
    encoding='utf-8'))['entrances']}

SONGS = {'FOREST': 'RG_MINUET_OF_FOREST', 'FIRE': 'RG_BOLERO_OF_FIRE', 'WATER': 'RG_SERENADE_OF_WATER',
         'SHADOW': 'RG_NOCTURNE_OF_SHADOW', 'SPIRIT': 'RG_REQUIEM_OF_SPIRIT', 'LIGHT': 'RG_PRELUDE_OF_LIGHT'}
NOTES = {'A': 'RG_OCARINA_A_BUTTON', 'C_RIGHT': 'RG_OCARINA_C_RIGHT_BUTTON', 'C_LEFT': 'RG_OCARINA_C_LEFT_BUTTON',
         'C_UP': 'RG_OCARINA_C_UP_BUTTON', 'C_DOWN': 'RG_OCARINA_C_DOWN_BUTTON'}

# Root Exits and the warp regions as root.cpp defines them; the harness builds the same edges.
root_exits = root[root.index('areaTable[RR_ROOT_EXITS] = Region("Root Exits", SCENE_ID_MAX,'):]
root_exits = root_exits[:root_exits.index('});')]
warps = re.findall(r'ENTRANCE\((RR_\w+_WARP),\s*logic->CanUse\((RG_\w+)\)\),', root_exits)
assert sorted(rg for _, rg in warps) == sorted(SONGS.values()) and len(warps) == 6, warps
pads = {}
for warp, _ in warps:
    block = root[root.index(f'areaTable[{warp}] = Region('):]
    exits = re.findall(r'ENTRANCE\((RR_\w+),\s*true\),', block[:block.index('});')])
    assert len(exits) == 1, (warp, exits)
    pads[warp] = exits[0]
# The audit's GLOBAL rows name the same edges and pads, and map OoTMM's landing (the row's area) to Ship's.
landing_region = {}
for row in audit['rows']:
    if row['role'] == 'global':
        assert (row['target'], row['song']) in warps and pads[row['target']] == row['landing'], row['name']
        landing_region[registry[row['name']]['to']] = row['landing']
assert len(landing_region) == 6
landing_region["OOT Link's House"] = 'RR_KF_LINKS_HOUSE'  # the spawn regions' only exits (root.cpp:55-63)
assert 'areaTable[RR_CHILD_SPAWN] = Region("Child Spawn"' in root and 'ENTRANCE(RR_KF_LINKS_HOUSE, true),' in root
assert 'areaTable[RR_ADULT_SPAWN] = Region("Adult Spawn"' in root and 'ENTRANCE(RR_TEMPLE_OF_TIME, true),' in root
# The production warp-song predicate: CanUse's guard and its song cases (pinned against OoTMM's notes by
# entrance_temple_global_lemma_tests.py); an ocarina and the songs are allowed in Root Exits' SCENE_ID_MAX region.
can_use = stubs.function(logic_cpp, 'bool Logic::CanUse(RandomizerGet itemName)')
assert hostcxx.contains(can_use, 'if (!HasItem(itemName)) return false;')
song_cases = can_use[can_use.index('// Songs'):can_use.index('// Misc. Items')]
assert hostcxx.contains(can_use, 'case RG_FAIRY_OCARINA: case RG_OCARINA_OF_TIME: return true;')
resolve = stubs.function(probe, 'Overlay<Entrance> Resolve(const QueryRequest& q) const')
for needle in ['const auto& spawn =', 'Binding(sPrepared->startingChild ? "OOT_SPAWN_ADULT" : "OOT_SPAWN_CHILD", '
               'TempleBindingRole::RootPolicy);', 'overlay.rootSpawn = &BoundExit(RR_ROOT_EXITS, spawn.parent);',
               'overlay.rootSpawnTarget = spawn.parent;', 'overlay.global = GlobalEdges();']:
    assert hostcxx.contains(resolve, needle), needle
roles = probe[probe.index('enum class TempleBindingRole'):probe.index('constexpr TempleBinding kBindings[] = {')]


def items(instrument, song):
    """Ship's starting items for an OoTMM instrument: the shared ocarina and notes are the OoT ones in the probe's
    private Logic (TempleProbe.cpp ApplyProbeStartingItems), and Ship owns every note when its buttons are not
    shuffled (logic.cpp Logic::Reset)."""
    result = set()
    for item in instrument['items']:
        if item in ('OOT_OCARINA', 'SHARED_OCARINA'):
            result.add('RG_FAIRY_OCARINA')
        elif m := re.fullmatch(r'(?:OOT|SHARED)_BUTTON_(\w+)', item):
            result.add(NOTES[m.group(1)])
        else:
            raise AssertionError(item)
    if not instrument['settings'].get('ocarinaButtonsShuffleOot'):
        result |= set(NOTES.values())
    if song:
        result.add(SONGS[song])
    return sorted(result)


cases = []
for group in facts['groups']:
    instrument = fixture['instruments'][group['instrument']]
    walks = sorted({landing_region[area] for area, walks in group['walks'].items() if walks})
    for case in group['cases']:
        opposite = case['ages'][group['opposite']]
        assert opposite['temple'] == case['temple']
        cases.append({'id': case['id'], 'startingChild': group['startingAge'] == 'child',
                      'items': items(instrument, case['song']), 'walks': walks, 'temple': case['temple'],
                      'songDecides': case['temple'] != group['walks'][group['spawnArea']]})
assert len(cases) == 63 and sum(c['songDecides'] for c in cases) > 0
(work/'cases.json').write_text(json.dumps(cases, indent=1), encoding='utf-8')

pinned = stubs.pinned(source)
cpp = r'''
#define NOMINMAX
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define RANDO_ENUM_END(name) };
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerRegion.h"
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
#define RANDO_ENUM_BEGIN(name) const std::map<std::string, name> k##name##Names = {
#define RANDO_ENUM_ITEM(name, ...) { #name, name },
#define RANDO_ENUM_END(name) };
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerRegion.h"
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
enum NativeEntrance {
#define DEFINE_ENTRANCE(name, ...) name,
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
enum LogicVal { LOGIC_NONE, LOGIC_MAX };
namespace Rando {
struct Logic {
    bool IsChild = false, IsAdult = false, AtDay = false, AtNight = false;
    RandomizerRegion CurrentRegionKey = RR_NONE;
    bool inLogic[LOGIC_MAX] = {};
    std::set<RandomizerGet> items;
    bool Get(LogicVal v) const { return inLogic[v]; }
    void Set(LogicVal v, bool value) { inLogic[v] = value; }
    bool HasItem(RandomizerGet item) const { return items.count(item) != 0; }
    // logic.cpp Logic::CanUse for an ocarina and the songs: its guard, ItemUseAllowed (songs always, an ocarina in
    // Root Exits' SCENE_ID_MAX region) and its song cases, read from the source.
    bool CanUse(RandomizerGet itemName) {
        if (!HasItem(itemName))
            return false;
        if (CurrentRegionKey != RR_ROOT_EXITS)
            throw std::logic_error("a warp song evaluated outside Root Exits");
        switch (itemName) {
''' + song_cases + r'''
            case RG_FAIRY_OCARINA:
            case RG_OCARINA_OF_TIME:
                return true;
            default:
                throw std::logic_error("unexpected CanUse");
        }
    }
};
}
std::shared_ptr<Rando::Logic> logic;
typedef bool (*ConditionFn)();
class Region;
Region* RegionTable(RandomizerRegion key);
class EventAccess {
  public:
    bool ConditionsMet() const { return false; }
    bool CheckConditionAtAgeTime(bool& age, bool& time);
    void EventOccurred() {}
    bool GetEvent() const { return true; }
};
namespace Rando {
class Entrance {
  public:
    Entrance(RandomizerRegion connected, ConditionFn condition, RandomizerRegion parent)
        : connectedRegion(connected), condition_function(condition), parentRegion(parent) {}
    bool GetConditionsMet() const { return condition_function(); } // glitchless
    bool CheckConditionAtAgeTime(bool& age, bool& time, bool passAnyway = false) const;
    RandomizerRegion GetConnectedRegionKey() const { return connectedRegion; }
    RandomizerRegion GetOriginalConnectedRegionKey() const { return connectedRegion; }
    RandomizerRegion GetParentRegionKey() const { return parentRegion; }
    bool IsShuffled() const { return false; }
  private:
    RandomizerRegion connectedRegion;
    ConditionFn condition_function;
    RandomizerRegion parentRegion;
};
}
using Rando::Entrance;
class Region {
  public:
    std::vector<EventAccess> events;
    std::list<Entrance> exits;
    bool childDay = false, childNight = false, adultDay = false, adultNight = false, addedToPool = false;
    bool TimePass() { return false; }
    void ApplyTimePass();
    bool UpdateEvents();
    bool Child() const { return childDay || childNight; }
    bool Adult() const { return adultDay || adultNight; }
};
std::array<Region, RR_MAX> areaTable;
Region* RegionTable(RandomizerRegion key) { return &areaTable[key]; }
#define StartPerformanceTimer(timer) ((void)0)
#define StopPerformanceTimer(timer) ((void)0)
''' + pinned['EventAccess::CheckConditionAtAgeTime'] + '\n' + pinned['Region::ApplyTimePass'] + '\n' + \
    pinned['Region::UpdateEvents'] + '\nnamespace Rando {\n' + pinned['Entrance::CheckConditionAtAgeTime'] + \
    '\n}\n#include "TempleProbeCore.h"\nusing namespace Rando::TempleProbe;\n' + roles + r'''
constexpr TempleBinding kBindings[] = {
#include "combo_oot_edge_bindings.inc"
};
struct Prepared { bool startingChild = false; };
std::optional<Prepared> sPrepared = Prepared{};
''' + stubs.function(probe, 'const TempleBinding& Binding(') + '\n' + \
    stubs.function(probe, 'const Entrance& BoundExit(') + '\n' + \
    stubs.function(probe, 'const Entrance& WarpLanding(') + '\n' + \
    stubs.function(probe, 'std::vector<AddedEdge<Entrance>> GlobalEdges()') + '\n' + r'''
bool True() { return true; }
void Link(RandomizerRegion from, RandomizerRegion to, ConditionFn condition) {
    areaTable[from].exits.emplace_back(to, condition, from);
}
template <RandomizerGet song> bool Plays() { return logic->CanUse(song); }
bool gWalkChild = false;
bool Walks() { return gWalkChild ? logic->IsChild : logic->IsAdult; }
// root.cpp's Root, Root Exits, spawns and warp regions; then OoTMM's recorded walks to the Temple of Time.
void World(const nlohmann::json& c) {
    for (auto& r : areaTable)
        r = Region{};
    Link(RR_ROOT, RR_ROOT_EXITS, True);
    Link(RR_ROOT_EXITS, RR_CHILD_SPAWN, [] { return logic->IsChild; });
    Link(RR_ROOT_EXITS, RR_ADULT_SPAWN, [] { return logic->IsAdult; });
''' + ''.join(f'    Link(RR_ROOT_EXITS, {w}, Plays<{rg}>);\n    Link({w}, {pads[w]}, True);\n' for w, rg in warps) + r'''
    Link(RR_CHILD_SPAWN, RR_KF_LINKS_HOUSE, True);
    Link(RR_ADULT_SPAWN, RR_TEMPLE_OF_TIME, True);
    gWalkChild = !c.at("startingChild").get<bool>();
    for (const auto& name : c.at("walks"))
        if (name != "RR_TEMPLE_OF_TIME")
            Link(kRandomizerRegionNames.at(name.get<std::string>()), RR_TEMPLE_OF_TIME, Walks);
}
std::shared_ptr<Rando::Logic> gLive;
struct ParityPolicy {
    bool scope = false, global = true;
    std::set<RandomizerGet> starting;
    std::string_view Registry() const { return "parity"; }
    void Check() {}
    std::array<Region, RR_MAX>& Regions() { return areaTable; }
    std::shared_ptr<Rando::Logic>& Handle() { return logic; }
    bool& Scope() { return scope; }
    size_t Root() const { return RR_ROOT; }
    size_t Target() const { return RR_TEMPLE_OF_TIME; }
    bool StartingChild() const { return sPrepared->startingChild; }
    bool SameLogic() const { return logic == gLive; }
    std::shared_ptr<Rando::Logic> MakeLogic() const { return std::make_shared<Rando::Logic>(); }
    void ApplyStartingItems() { logic->items.insert(starting.begin(), starting.end()); }
    // TempleProbe.cpp NativePolicy::Resolve for a query with no candidate edge: the root policy and GLOBAL.
    Overlay<Entrance> Resolve(const QueryRequest&) const {
        Overlay<Entrance> overlay;
        const auto& spawn =
            Binding(sPrepared->startingChild ? "OOT_SPAWN_ADULT" : "OOT_SPAWN_CHILD", TempleBindingRole::RootPolicy);
        overlay.rootSpawn = &BoundExit(RR_ROOT_EXITS, spawn.parent);
        overlay.rootSpawnTarget = spawn.parent;
        if (global)
            overlay.global = GlobalEdges();
        return overlay;
    }
};
int main(int argc, char** argv) {
    assert(argc == 2);
    std::ifstream file(argv[1]);
    const auto cases = nlohmann::json::parse(file);
    gLive = std::make_shared<Rando::Logic>();
    logic = gLive;
    const std::string query = nlohmann::json{ { "schema", "oot-temple-probe-query-v1" }, { "registry", "parity" },
        { "token", 1 }, { "assumedInstalled", true }, { "assumed", nlohmann::json::array() },
        { "deleted", nlohmann::json::array() }, { "edges", nlohmann::json::array() },
        { "portalLinks", nlohmann::json::array() },
        { "mm", { { "deleted", nlohmann::json::array() }, { "edges", nlohmann::json::array() } } } }.dump();
    size_t agree = 0, decided = 0, withoutGlobal = 0;
    for (const auto& c : cases) {
        World(c);
        sPrepared->startingChild = c.at("startingChild").get<bool>();
        ParityPolicy policy;
        for (const auto& item : c.at("items"))
            policy.starting.insert(kRandomizerGetNames.at(item.get<std::string>()));
        uint8_t ages = 0xff;
        std::string error;
        const int code = Firewall(error, [&] { return RunQuery(policy, query.c_str(), nullptr, &ages); });
        const bool temple = c.at("temple").get<bool>();
        const uint8_t opposite = sPrepared->startingChild ? 1 : 2;
        if (code != (temple ? kReached : kNotReached) || ages != (temple ? opposite : 0)) {
            std::cerr << c.at("id").get<std::string>() << ": probe " << code << " ages " << int(ages) << " " << error
                      << ", OoTMM " << temple << '\n';
            return 1;
        }
        ++agree;
        // Without GLOBAL the probe walks only: it must miss exactly the cases where the song decides.
        policy.global = false;
        const int walked = Firewall(error, [&] { return RunQuery(policy, query.c_str(), nullptr, &ages); });
        const bool songDecides = c.at("songDecides").get<bool>();
        assert((walked == kReached) == (songDecides ? !temple : temple));
        decided += songDecides;
        withoutGlobal += (walked == kReached) != temple;
        assert(logic == gLive && gLive->items.empty());
        for (const auto& r : areaTable)
            assert(!r.childDay && !r.childNight && !r.adultDay && !r.adultNight);
    }
    assert(decided > 0 && withoutGlobal == decided);
    std::cout << "PASS the probe's GLOBAL node gives OoTMM's validateAgeTemple answer and ages in all " << agree
              << " recorded cases; without it the " << decided << " cases the song decides disagree\n";
}
'''
path = work/'temple_global_parity.cpp'
path.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(args.compiler, path, includes=[randomizer, source/'soh', source/'soh/include', args.json_include],
                    defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/bigobj', '/Zc:preprocessor'])
subprocess.run([str(exe), str(work/'cases.json')], check=True, timeout=hostcxx.RUN_TIMEOUT)
