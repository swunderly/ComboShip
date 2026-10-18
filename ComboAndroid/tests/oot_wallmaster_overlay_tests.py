"""Execute the production overlay and age/time traversal with engine stubs, not gameplay evidence.

Checks the generated predicates against an independent truth table; uses the real soul ownership
policy, registry, candidate split and age/time propagation. Native object compilation is separate.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
import hostcxx
from temple_probe_stubs import function

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_oot_wallmaster_overlay as tool

fixture = json.loads((source/tool.JSON).read_bytes())
assert len(fixture['edges']) == 11
assert fixture['auditSha256'] == hashlib.sha256((source/tool.AUDIT).read_bytes()).hexdigest()
for path, data in tool.outputs(fixture).items():
    assert (source/path).read_bytes() == data and b'\r' not in data, path
reference = source.parents[1]/'references/ootmm-stable-32.3'
if reference.exists():
    assert tool.build(reference) == fixture
    subprocess.run([sys.executable, '-B', str(source/'tools/ootmm/export_oot_wallmaster_overlay.py'),
                    '--reference', str(reference), '--output-root', str(source), '--check'], check=True, timeout=120)

randomizer = source/'soh/soh/Enhancements/randomizer'
overlay = (randomizer/'combo_entrance_overlay.cpp').read_text(encoding='utf-8')
# Engine headers are replaced by the explicit stubs below; generated predicate/arrival includes stay.
overlay = '\n'.join(line for line in overlay.splitlines()
                    if not line.startswith('#include') or '.inc"' in line)
fill = (randomizer/'3drando/fill.cpp').read_text(encoding='utf-8')
entrance = (randomizer/'entrance.cpp').read_text(encoding='utf-8')
process = function(fill, 'static void ProcessComboEntranceOverlay(')
assert hostcxx.contains(function(fill, 'void ProcessExits('),
                        'ProcessComboEntranceOverlay(region, gals, ignore, stopOnBeatable, addToPlaythrough);')
assert 'entranceSphere' not in process and 'region->exits' not in process

cpp = work/'overlay.cpp'
cpp.write_text(r'''
#define OOT_SOUL_POLICY_HOST_ENUMS
#define COMBO_BUILD
#define COMBO_ENTRANCE_DEV
#define SPDLOG_ERROR(...) ((void)0)
#include <cassert>
#include <array>
#include <list>
#include <memory>
#include <set>
#include <string>
#include "rando/entrances/OotUnifiedLayout.h"
#include "souls/OotSoulState.h"
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define RANDO_ENUM_END(name) };
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerRegion.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
enum NativeEntrance {
#define DEFINE_ENTRANCE(name, ...) name,
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
enum RandomizerGet { RG_NONE, RG_LENS_OF_TRUTH, RG_BOMBCHU_5, RG_MIRROR_SHIELD, RG_POWER_BRACELET };
enum DungeonKey { DEKU_TREE, DODONGOS_CAVERN, JABU_JABUS_BELLY, FOREST_TEMPLE, FIRE_TEMPLE,
    WATER_TEMPLE, SPIRIT_TEMPLE, SHADOW_TEMPLE, BOTTOM_OF_THE_WELL, ICE_CAVERN, GERUDO_TRAINING_GROUND, GANONS_CASTLE };
enum Trick { RT_LENS_BOTW, RT_LENS_GANON };
struct Dungeon { bool mq = false; bool IsMQ() { return mq; } };
struct Dungeons { Dungeon rows[12]; Dungeon* GetDungeon(DungeonKey key) { return &rows[key]; } };
namespace Rando {
struct Logic {
    bool IsChild = false, IsAdult = false, AtDay = false, AtNight = false;
    bool lens = false, bombchu = false, mirror = false, bracelet = false, sunlight = false;
    bool adultOnly = false;
    Souls::GeneralOotSoulsOwnership souls{};
    Souls::GeneralOotSoulOptions soulOptions{true, true, true, true, true};
    bool CanUse(RandomizerGet item) {
        const bool usable = !adultOnly || IsAdult;
        return usable && ((item == RG_LENS_OF_TRUTH && lens) || (item == RG_BOMBCHU_5 && bombchu) ||
                          (item == RG_MIRROR_SHIELD && mirror));
    }
    bool HasItem(RandomizerGet item) { return item == RG_POWER_BRACELET && bracelet; }
    bool SunlightArrows() { return sunlight && (!adultOnly || IsAdult); }
};
struct Context {
    std::shared_ptr<Logic> logic = std::make_shared<Logic>();
    Dungeons dungeons;
    bool trick = false;
    static std::shared_ptr<Context> GetInstance() { static auto p = std::make_shared<Context>(); return p; }
    std::shared_ptr<Logic> GetLogic() { return logic; }
    Dungeons* GetDungeons() { return &dungeons; }
    bool GetTrickOption(Trick) { return trick; }
};
namespace Souls {
bool Has(Logic* logic, OotSoul soul) {
    assert(soul == OotSoul::ENEMY_WALLMASTER);
    return HaveGeneralOotSoul(logic->souls, logic->soulOptions, soul);
}
}
}
auto logic = Rando::Context::GetInstance()->GetLogic();
namespace Rando { bool& ProbeLogicActive(); std::shared_ptr<Logic> ActiveLogic(); }
''' + function((randomizer/'location_access.cpp').read_text(encoding='utf-8'), 'bool& Rando::ProbeLogicActive()') + '\n' +
function((randomizer/'location_access.cpp').read_text(encoding='utf-8'), 'std::shared_ptr<Rando::Logic> Rando::ActiveLogic()') + r'''
struct Region;
Region* RegionTable(RandomizerRegion);
using ConditionFn = bool(*)();
class Entrance {
    RandomizerRegion connectedRegion, parent = RR_NONE;
    ConditionFn condition;
  public:
    Entrance(RandomizerRegion dest, ConditionFn fn, std::string, bool) : connectedRegion(dest), condition(fn) {}
    void SetParentRegion(RandomizerRegion key) { parent = key; }
    Region* GetParentRegion() { return RegionTable(parent); }
    Region* GetConnectedRegion() { return RegionTable(connectedRegion); }
    bool GetConditionsMet() const { return condition(); }
    bool CheckConditionAtAgeTime(bool& age, bool& time, bool passAnyway = false) const;
};
struct Region {
    RandomizerRegion randomizerRegionKey;
    bool childDay = false, childNight = false, adultDay = false, adultNight = false, addedToPool = false;
    std::list<Entrance> exits;
};
std::map<RandomizerRegion, Region> regions;
Region* RegionTable(RandomizerRegion key) {
    auto [it, inserted] = regions.try_emplace(key);
    it->second.randomizerRegionKey = key;
    return &it->second;
}
constexpr int PT_TOD_ACCESS = 0;
void StartPerformanceTimer(int) {}
void StopPerformanceTimer(int) {}
struct GetAccessibleLocationsStruct {
    bool logicUpdated = false, sphereZeroComplete = true, foundTempleOfTime = true, validatedStartingRegion = true;
    std::vector<RandomizerRegion> regionPool;
};
int processed = 0;
void ValidateOtherEntrance(GetAccessibleLocationsStruct&) {}
void ValidateSphereZero(GetAccessibleLocationsStruct&) {}
void ProcessRegion(Region*, GetAccessibleLocationsStruct&, RandomizerGet, bool, bool) { ++processed; }
''' + function(entrance, 'bool Entrance::CheckConditionAtAgeTime(') + '\n' + overlay + '\n' +
function(fill, 'static bool UpdateToDAccess(') + '\n' + process + r'''
int main() {
    namespace CE = ComboRando::Entrances;
    using namespace Rando::ComboEntrances;
    auto ctx = Rando::Context::GetInstance();
    // The same predicate table must read the probe's private inventory, even when live inventory
    // would permit the route. This compiles the real ActiveLogic implementation above.
    auto liveLogic = logic;
    liveLogic->soulOptions.enabled = false;
    auto probe = std::make_shared<Rando::Logic>();
    logic = probe;
    Rando::ProbeLogicActive() = true;
    CE::Settings settings; settings.erWallmasters = CE::Mode::OwnGame;
    const auto world = CE::BuildCandidateWorld({}, CE::LayoutV1::MakePools(settings, {}, false), settings);
    const auto candidate = CE::EncodeOotOverlay(world);
    assert(SetAssumptions(candidate.c_str()) && CE::ActiveOotUnifiedAssumptions());
    assert(!OverlayExits(RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM)[0].GetConditionsMet());
    struct Expected { RandomizerRegion anchor; int condition; };
    const Expected expected[] = {
        {RR_BOTW_MIDDLE, 1}, {RR_FOREST_TEMPLE_NW_HALLWAY_TWISTED, 0},
        {RR_FOREST_TEMPLE_NW_HALLWAY_STRAIGHTENED, 0}, {RR_FOREST_TEMPLE_NE_HALLWAY_STRAIGHTENED, 0},
        {RR_FOREST_TEMPLE_NE_HALLWAY_TWISTED, 0}, {RR_GANONS_CASTLE_LIGHT_TRIAL_FINAL_ROOM, 1},
        {RR_GANONS_CASTLE_SPIRIT_TRIAL_BEFORE_SWITCH, 2}, {RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM, 0},
        {RR_SHADOW_TEMPLE_UPPER_HUGE_PIT, 0}, {RR_SPIRIT_TEMPLE_RUPEE_BRIDGE_NORTH, 0},
        {RR_SPIRIT_TEMPLE_2F_MIRROR_ROOM, 3}
    };
    // Exhaustive independent truth table: missing/present soul, shuffle off, lens/trick,
    // Bombchu, Mirror Shield, sunlight and bracelet. No bow is needed for Ganon Spirit.
    for (const auto& row : expected) for (unsigned bits = 0; bits < 256; ++bits) {
        logic->souls = {};
        const bool owned = (bits & 1) != 0;
        logic->soulOptions.enabled = (bits & 2) != 0;
        Rando::Souls::ChangeGeneralOotSoul(logic->souls,
            Rando::Souls::GeneralOotSoulIndex(Rando::Souls::OotSoul::ENEMY_WALLMASTER), owned);
        logic->lens = (bits & 4) != 0; ctx->trick = (bits & 8) != 0;
        logic->bombchu = (bits & 16) != 0; logic->mirror = (bits & 32) != 0;
        logic->sunlight = (bits & 64) != 0; logic->bracelet = (bits & 128) != 0;
        const bool soul = owned || !logic->soulOptions.enabled;
        const bool gate = row.condition == 0 || (row.condition == 1 && (logic->lens || ctx->trick)) ||
            (row.condition == 2 && logic->bombchu && (logic->mirror || logic->sunlight)) ||
            (row.condition == 3 && (logic->bracelet || logic->sunlight));
        const auto exits = OverlayExits(row.anchor);
        assert(exits.size() == 11);
        std::set<RandomizerRegion> destinations;
        for (const auto& exit : exits) {
            assert(exit.GetConditionsMet() == (soul && gate));
            destinations.insert(const_cast<Entrance&>(exit).GetConnectedRegion()->randomizerRegionKey);
        }
        assert(destinations.size() == 11);
    }
    // Every age/time combination propagates only from a reached source with its own predicate.
    logic->soulOptions.enabled = false; logic->lens = true; logic->adultOnly = true; ctx->trick = false;
    for (unsigned mask = 0; mask < 16; ++mask) {
        regions.clear(); processed = 0;
        auto* src = RegionTable(RR_BOTW_MIDDLE);
        src->childDay = (mask & 1) != 0; src->childNight = (mask & 2) != 0;
        src->adultDay = (mask & 4) != 0; src->adultNight = (mask & 8) != 0;
        GetAccessibleLocationsStruct gals;
        ProcessComboEntranceOverlay(src, gals, RG_NONE, false, true);
        for (auto exit : OverlayExits(RR_BOTW_MIDDLE)) {
            const auto* dst = exit.GetConnectedRegion();
            assert(!dst->childDay && !dst->childNight);
            assert(dst->adultDay == src->adultDay && dst->adultNight == src->adultNight);
        }
        assert(src->exits.empty());
        assert(gals.regionPool.size() == ((mask & 12) ? 11u : 0u));
        assert(processed == ((mask & 12) ? 11 : 0));
    }
    ctx->dungeons.rows[0].mq = true;
    assert(!SetAssumptions(candidate.c_str()) && !CE::ActiveOotUnifiedAssumptions());
    assert(OverlayExits(RR_BOTW_MIDDLE).empty());
    ctx->dungeons.rows[0].mq = false;
    assert(SetAssumptions(candidate.c_str()));
    assert(!SetAssumptions("{}") && !CE::ActiveOotUnifiedAssumptions());
    assert(SetAssumptions(candidate.c_str()));
    ClearAssumptions();
    assert(OverlayExits(RR_BOTW_MIDDLE).empty());
    // Inactive traversal is the identity for existing native exits, flags and search state.
    regions.clear();
    auto* native = RegionTable(RR_BOTW_MIDDLE);
    native->childDay = true; native->adultNight = true; native->addedToPool = true;
    native->exits.emplace_back(RR_DEKU_TREE_ENTRYWAY, [] { return true; }, "native", false);
    const auto* identity = &native->exits.front();
    GetAccessibleLocationsStruct idle;
    ProcessComboEntranceOverlay(native, idle, RG_NONE, false, true);
    assert(native->childDay && !native->childNight && !native->adultDay && native->adultNight && native->addedToPool);
    assert(!idle.logicUpdated && idle.regionPool.empty() && native->exits.size() == 1 && &native->exits.front() == identity);
    assert(SetLayout("{}") && std::string(DumpLayout()) == "{}");
    const auto context = nlohmann::json::parse(AreaContext());
    assert(context.at("ootMq").size() == 12 && context.at("profile") == "ootmm-oot-area-context-v1");
    logic = liveLogic;
    Rando::ProbeLogicActive() = false;
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source/'combo', a.json_include, randomizer,
                    source/'soh', source/'soh/include'], defines=['_CRT_SECURE_NO_WARNINGS'],
                    msvc=['/bigobj', '/Zc:preprocessor'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('PASS production vanilla overlay: 2816 predicate cases, 16 age/time cases; native exits untouched; MQ refused')
