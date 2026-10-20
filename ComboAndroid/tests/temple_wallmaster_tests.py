"""Real native Resolve, shared audited predicates and opposite-age Search, with engine stubs.

The independent Temple isolation suite still executes RunQuery and restoration on every exit path.
"""
from pathlib import Path
import argparse
import subprocess
import hostcxx
from temple_probe_stubs import function
from oot_wallmaster_stubs import prelude

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
randomizer = source/'soh/soh/Enhancements/randomizer'
probe = (randomizer/'TempleProbe.cpp').read_text(encoding='utf-8')
overlay = (randomizer/'combo_entrance_overlay.cpp').read_text(encoding='utf-8')
factory = function(overlay, 'std::vector<Entrance> WallmasterCandidateExits(')
assert 'ActiveOotUnified' not in factory and 'OotUnifiedGraphRoutes' not in factory
assert 'edge.condition' in factory and 'edge.anchor' in factory
resolve = function(probe, 'Overlay<Entrance> Resolve(')
assert 'WallmasterCandidateExits' in resolve
assert 'ActiveOotUnified' not in resolve and 'OverlayExits' not in resolve

cpp = work/'temple_wallmaster.cpp'
cpp.write_text(prelude(source) + r'''
#include "rando/entrances/TempleAdapter.h"
#include "rando/entrances/MmUnifiedLayout.h"
#include "TempleProbeCore.h"
#include <iostream>
using namespace Rando::TempleProbe;
enum class TempleBindingRole : uint8_t { Candidate, RootPolicy };
struct TempleBinding {
    std::string_view name; uint32_t id; TempleBindingRole role;
    RandomizerRegion parent, target; int16_t index;
};
constexpr TempleBinding kBindings[] = {
#include "combo_oot_edge_bindings.inc"
};
struct Prepared { bool startingChild = true; };
std::optional<Prepared> sPrepared = Prepared{};
''' + function(probe, 'const TempleBinding& Binding(') + '\n' +
function(probe, 'const Entrance& BoundExit(') + '\n' +
'struct NativePolicy {\n' + resolve + '\n};\n' + r'''
namespace CE = ComboRando::Entrances;
template<class F> void Unsupported(F f) {
    bool rejected = false;
    try { f(); } catch (const Failure& e) { rejected = e.code == kUnsupported; }
    assert(rejected);
}
void Connect(RandomizerRegion from, RandomizerRegion to, ConditionFn condition) {
    regions[from].exits.emplace_back(to, condition, "native", true);
    regions[from].exits.back().SetParentRegion(from);
}
QueryRequest Query(std::string src, std::string dst) {
    QueryRequest q; q.deleted.push_back(src); q.edges.push_back({src, dst}); return q;
}
// Probe inertness under foreign-song logic (limits design, Stage 1; lemma S1). A world where both spawns reach every
// anchor of the context's candidate edges and every arrival leads to the Temple of Time, so each candidate's answer
// is its own predicate at the opposite age. Every candidate edge alone, and all of them together, at both starting
// ages and over several inventories, must give the same Search answer with the song state off and adversarially on:
// the foreign-song flags, and arrival and knowledge masks that would put a song landing in the Temple of Time.
std::vector<int> InertAnswers(NativePolicy& policy, const QueryRequest& full, Rando::Logic& probe) {
    const auto overlay = policy.Resolve(full);
    ClearRegions();
    Connect(RR_ROOT_EXITS, RR_ADULT_SPAWN, [] { return logic->IsAdult; });
    Connect(RR_ROOT_EXITS, RR_CHILD_SPAWN, [] { return logic->IsChild; });
    std::set<size_t> anchors, arrivals;
    for (const auto& edge : overlay.added) {
        anchors.insert(edge.parent);
        arrivals.insert(edge.target);
    }
    for (const auto anchor : anchors) {
        Connect(RR_ADULT_SPAWN, static_cast<RandomizerRegion>(anchor), [] { return true; });
        Connect(RR_CHILD_SPAWN, static_cast<RandomizerRegion>(anchor), [] { return true; });
    }
    for (const auto arrival : arrivals)
        Connect(static_cast<RandomizerRegion>(arrival), RR_TEMPLE_OF_TIME, [] { return true; });
    std::vector<int> answers;
    for (int inventory = 0; inventory < 4; ++inventory)
        for (bool startingChild : {false, true}) {
            sPrepared->startingChild = startingChild;
            const bool all = inventory >= 2;
            probe.lens = probe.bombchu = probe.mirror = probe.bracelet = probe.sunlight = all;
            probe.fireArrows = probe.endingSuns = all;
            probe.forestKeys = all ? 5 : 0;
            probe.adultOnly = inventory == 3;
            probe.souls = {};
            Rando::Souls::ChangeGeneralOotSoul(probe.souls,
                Rando::Souls::GeneralOotSoulIndex(Rando::Souls::OotSoul::ENEMY_WALLMASTER), inventory != 1);
            const auto run = [&](const QueryRequest& q) {
                auto candidate = policy.Resolve(q);
                uint8_t ages = 0;
                RegionFlagGuard guard(regions);
                const int result = Search(regions, probe, candidate, RR_ROOT, RR_TEMPLE_OF_TIME, startingChild,
                                          [] { return false; }, ages);
                assert(result == kReached || result == kNotReached);
                answers.push_back(result);
                answers.push_back(ages);
            };
            for (const auto& edge : full.edges)
                run(Query(edge.src, edge.dst));
            run(full);
        }
    return answers;
}
void SongState(bool on) {
    auto& songs = ComboRando::ActiveSongTravel();
    songs = {};
    songs.foreignSongs = songs.logicForeignSongs = on;
    ComboRando::OotSongArrivals() = on ? 0xffffu : 0u;
    ComboRando::OotSongAgeArrivals() = on ? ComboRando::SongAgeArrivals{true, {0xffffu, 0xffffu}}
                                          : ComboRando::SongAgeArrivals{};
    ComboRando::OotForeignKnowledge() = ComboRando::MmForeignKnowledge() = on ? 0x7fu : 0u;
}
void Graph() {
    ClearRegions();
    Connect(RR_ROOT_EXITS, RR_ADULT_SPAWN, [] { return logic->IsAdult; });
    Connect(RR_ROOT_EXITS, RR_CHILD_SPAWN, [] { return logic->IsChild; });
    Connect(RR_ADULT_SPAWN, RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM, [] { return true; });
    Connect(RR_CHILD_SPAWN, RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM, [] { return true; });
    Connect(RR_ICE_CAVERN_ENTRYWAY, RR_TEMPLE_OF_TIME, [] { return true; });
}
int main() {
    using namespace Rando::ComboEntrances;
    NativePolicy policy;
    Graph();
    const auto q = Query("OOT_WALLMASTER_GTG", "OOT_ICE_CAVERN");
    auto resolved = policy.Resolve(q);
    assert(resolved.deleted.empty() && resolved.added.size() == 1 && resolved.owned.size() == 1);
    assert(resolved.added[0].parent == RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM);
    assert(resolved.added[0].target == RR_ICE_CAVERN_ENTRYWAY);
    // Query-owned pointers remain valid when the overlay is copied or moved.
    auto copy = resolved;
    resolved = {};
    auto moved = std::move(copy);
    assert(moved.owned.front().get() == moved.added.front().entrance);
    auto live = logic;
    live->soulOptions.enabled = false; // adversarial live inventory permits every soul
    auto privateLogic = std::make_shared<Rando::Logic>();
    logic = privateLogic;
    Rando::ProbeLogicActive() = true;
    for (bool startingChild : {false, true}) for (bool hasSoul : {false, true}) {
        Graph();
        sPrepared->startingChild = startingChild;
        auto candidate = policy.Resolve(q);
        privateLogic->souls = {};
        Rando::Souls::ChangeGeneralOotSoul(privateLogic->souls,
            Rando::Souls::GeneralOotSoulIndex(Rando::Souls::OotSoul::ENEMY_WALLMASTER), hasSoul);
        const auto root = RegionTable(RR_ROOT);
        root->childNight = true; // snapshot must survive this test's scoped search
        uint8_t ages = 0;
        {
            RegionFlagGuard guard(regions);
            const int result = Search(regions, *privateLogic, candidate, RR_ROOT, RR_TEMPLE_OF_TIME,
                                      startingChild, [] { return false; }, ages);
            assert(result == (hasSoul ? kReached : kNotReached));
            assert(ages == (hasSoul ? (startingChild ? 1 : 2) : 0));
        }
        assert(root->childNight && !root->adultDay);
        assert(regions[RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM].exits.empty());
    }
    // Both Forest anchors share one source identity and the same audited table as regular logic.
    Graph();
    const auto forest = policy.Resolve(Query("OOT_WALLMASTER_FOREST_CORRIDOR_WEST", "OOT_ICE_CAVERN"));
    assert(forest.added.size() == 2 && forest.owned.size() == 2);
    assert(forest.added[0].parent != forest.added[1].parent);
    CE::Settings settings; settings.erWallmasters = CE::Mode::OwnGame;
    const auto world = CE::BuildCandidateWorld({}, CE::LayoutV1::MakePools(settings, {}, false), settings);
    const auto fullQuery = CE::EncodeTempleQuery(CE::BuildTempleCandidate({}, CE::LayoutV1::MakePools(settings, {}, false), settings));
    const auto full = policy.Resolve(ParseQuery(fullQuery.c_str(), CE::kRegistrySourceSha256));
    assert(full.deleted.empty() && full.owned.size() == 121 && full.added.size() == 121);
    // Poison the installed and assumption tables with different destinations. Resolve must not read either.
    CE::EntranceLayout committed;
    committed.required = {std::string(CE::kVanillaWallmasterCapability)};
    committed.settings = settings;
    CE::Overrides rows;
    for (size_t i = 0; i < CE::kOotVanillaWallmasterSources.size(); ++i)
        rows[CE::kOotVanillaWallmasterSources[i]] = CE::kOotWallmasterArrivalRows[i];
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i)
        rows[CE::kMmWallmasterSources[i]] = CE::kMmBaseWallmasterDestinations[i];
    std::swap(rows["OOT_WALLMASTER_GTG"], rows["OOT_WALLMASTER_FOREST_CORRIDOR_EAST"]);
    committed.overrides = rows;
    CE::ActiveOotUnified().install({{"entranceLayout", CE::EncodeEntranceLayout(committed)}},
                                  CE::CapabilitySet::AtLeast(CE::Stage::Hidden));
    CE::OotUnifiedAssumptions poisoned;
    poisoned.routes[CE::Find("OOT_WALLMASTER_GTG")->id] = {{CE::Find("OOT_TEMPLE_FIRE")->id}};
    CE::ActiveOotUnifiedAssumptions() = poisoned;
    const auto isolated = policy.Resolve(q);
    assert(isolated.added[0].target == RR_ICE_CAVERN_ENTRYWAY);
    CE::ActiveOotUnifiedAssumptions().reset(); CE::ActiveOotUnified().clear();
    // An MQ-only source does not exist in the vanilla Spirit Temple.
    Unsupported([&] { policy.Resolve(Query("OOT_WALLMASTER_SPIRIT_STATUE", "OOT_ICE_CAVERN")); });
    Unsupported([&] { policy.Resolve(Query("OOT_WALLMASTER_GTG", "OOT_FIELD_OWL")); });
    Unsupported([&] { auto bad = q; bad.deleted.clear(); policy.Resolve(bad); });
    // A Master Quest dungeon: admitted with the quests id (this dev build has it), where the source exists.
    auto& dungeons = Rando::Context::GetInstance()->dungeons.rows;
    assert(CE::ActiveCapabilities().Active(CE::kQuestsWallmasterCapability));
    dungeons[0].mq = true; // MQ Deku Tree: GTG is unchanged
    assert(policy.Resolve(q).added.size() == 1);
    dungeons[0].mq = false;
    dungeons[7].mq = true; // MQ Shadow Temple has no Wallmaster
    Unsupported([&] { policy.Resolve(Query("OOT_WALLMASTER_SHADOW", "OOT_ICE_CAVERN")); });
    for (auto& row : dungeons) row.mq = true;
    // Every dungeon MQ: the factory hands out the loaded quest's edges only, each from its MQ anchor.
    const auto statue = policy.Resolve(Query("OOT_WALLMASTER_SPIRIT_STATUE", "OOT_ICE_CAVERN"));
    assert(statue.added.size() == 1 && statue.added[0].parent == RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM);
    const auto east = policy.Resolve(Query("OOT_WALLMASTER_FOREST_CORRIDOR_EAST", "OOT_ICE_CAVERN"));
    assert(east.added.size() == 2 && east.added[0].parent == RR_FOREST_TEMPLE_MQ_BETH_ROOM &&
           east.added[1].parent == RR_FOREST_TEMPLE_MQ_BETH_ROOM);
    const auto pit = policy.Resolve(Query("OOT_WALLMASTER_BOTW_PIT", "OOT_DEKU_TREE"));
    assert(pit.added.size() == 1 && pit.added[0].parent == RR_BOTW_MQ_FLOORMASTER_ROOM &&
           pit.added[0].target == RR_DEKU_TREE_ENTRYWAY);
    Unsupported([&] { policy.Resolve(Query("OOT_WALLMASTER_GANON_LIGHT", "OOT_ICE_CAVERN")); });
    for (auto& row : dungeons) row.mq = false;
    CE::TempleFacts facts;
    for (unsigned mask = 0; mask < 4096; ++mask) {
        CE::OotQuests quests{};
        for (size_t i = 0; i < quests.size(); ++i) quests[i] = (mask >> i) & 1u;
        // Every OoT Wallmaster source is bound (U11); the pools hold the context's own.
        CE::CheckTempleCapability(settings, facts, [&](std::string_view area) {
            return CE::LayoutV1::AreaExists(area, quests, false);
        });
    }
    bool rejected = false;
    auto withOneWays = settings;
    withOneWays.erOneWays = CE::Mode::OwnGame;
    withOneWays.erOneWaysMajor = true;
    try {
        CE::CheckTempleCapability(withOneWays, facts, [](std::string_view area) {
            return CE::LayoutV1::AreaExists(area, {}, false);
        });
    } catch (const CE::TempleProbeUnsupported&) { rejected = true; }
    assert(rejected); // Wallmaster arrivals at one-way rows remain unbound before a draw.
    // Probe inertness under foreign-song logic, over the candidate edges of the vanilla context (121 resolved
    // edges), the all-Master Quest one and Forest and Spirit Master Quest.
    size_t inert = 0, reached = 0, unreached = 0;
    for (const unsigned mask : {0u, 0xfffu, (1u << CE::OotMqIndex("Forest")) | (1u << CE::OotMqIndex("Spirit"))}) {
        CE::OotQuests quests{};
        for (size_t i = 0; i < quests.size(); ++i)
            dungeons[i].mq = quests[i] = ((mask >> i) & 1u) != 0;
        const auto candidate = CE::BuildTempleCandidate({}, CE::LayoutV1::MakePools(settings, quests, false), settings);
        const auto all = ParseQuery(CE::EncodeTempleQuery(candidate).c_str(), CE::kRegistrySourceSha256);
        assert(mask != 0 || (policy.Resolve(all).added.size() == 121 && all.edges.size() == 99));
        SongState(false);
        const auto off = InertAnswers(policy, all, *privateLogic);
        SongState(true);
        const auto on = InertAnswers(policy, all, *privateLogic);
        SongState(false);
        assert(off == on && off.size() == 4 * 2 * 2 * (all.edges.size() + 1));
        for (size_t i = 0; i < off.size(); i += 2)
            (off[i] == kReached ? reached : unreached) += 1;
        inert += off.size() / 2;
    }
    for (auto& row : dungeons) row.mq = false;
    assert(reached > 0 && unreached > 0); // the predicates decide, so the comparison is not vacuous
    logic = live; Rando::ProbeLogicActive() = false;
    std::cout << "PASS isolated native Temple Resolve, 121 shared vanilla predicate edges, the MQ ones of the "
                 "loaded quests, both ages and soul negatives; " << inert << " Searches over three quest contexts "
                 "answer the same with foreign-song logic on and off\n";
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source/'combo', a.json_include, randomizer,
                    source/'soh', source/'soh/include'], defines=['_CRT_SECURE_NO_WARNINGS'],
                   msvc=['/bigobj', '/Zc:preprocessor'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
