"""Engine stubs for executing the shared vanilla and MQ predicate table and traversal on the host.

Uses real region/entrance enums, soul ownership, ActiveLogic and age/time propagation. The event
and time-passing methods here are inert; their real behavior is covered by the Temple core suite.
dev=False builds the shipping capability set (no COMBO_ENTRANCE_DEV).
"""
from pathlib import Path
from temple_probe_stubs import function


def prelude(source, dev=True):
    randomizer = Path(source)/'soh/soh/Enhancements/randomizer'
    overlay = (randomizer/'combo_entrance_overlay.cpp').read_text(encoding='utf-8')
    overlay = '\n'.join(line for line in overlay.splitlines()
                        if not line.startswith('#include') or '.inc"' in line)
    fill = (randomizer/'3drando/fill.cpp').read_text(encoding='utf-8')
    entrance = (randomizer/'entrance.cpp').read_text(encoding='utf-8')
    process = function(fill, 'static void ProcessComboEntranceOverlay(')
    return (
r'''
#define OOT_SOUL_POLICY_HOST_ENUMS
#define COMBO_BUILD
''' + ('#define COMBO_ENTRANCE_DEV\n' if dev else '') + r'''#define SPDLOG_ERROR(...) ((void)0)
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
enum RandomizerGet { RG_NONE, RG_LENS_OF_TRUTH, RG_BOMBCHU_5, RG_MIRROR_SHIELD, RG_POWER_BRACELET, RG_FIRE_ARROWS,
    RG_MINUET_OF_FOREST, RG_BOLERO_OF_FIRE, RG_SERENADE_OF_WATER, RG_NOCTURNE_OF_SHADOW, RG_REQUIEM_OF_SPIRIT,
    RG_PRELUDE_OF_LIGHT };
enum DungeonKey { DEKU_TREE, DODONGOS_CAVERN, JABU_JABUS_BELLY, FOREST_TEMPLE, FIRE_TEMPLE,
    WATER_TEMPLE, SPIRIT_TEMPLE, SHADOW_TEMPLE, BOTTOM_OF_THE_WELL, ICE_CAVERN, GERUDO_TRAINING_GROUND, GANONS_CASTLE };
enum Trick { RT_LENS_BOTW, RT_LENS_GANON };
enum LogicVal { LOGIC_GANON_MQ_SPIRIT_ENDING_SUNS };
struct Dungeon { bool mq = false; bool IsMQ() { return mq; } };
struct Dungeons { Dungeon rows[12]; Dungeon* GetDungeon(DungeonKey key) { return &rows[key]; } };
namespace Rando {
struct Logic {
    bool IsChild = false, IsAdult = false, AtDay = false, AtNight = false;
    RandomizerRegion CurrentRegionKey = RR_NONE;
    bool lens = false, bombchu = false, mirror = false, bracelet = false, sunlight = false;
    bool fireArrows = false, endingSuns = false;
    int forestKeys = 0;
    bool adultOnly = false;
    Souls::GeneralOotSoulsOwnership souls{};
    Souls::GeneralOotSoulOptions soulOptions{true, true, true, true, true};
    std::set<RandomizerGet> warpSongs; // playable warp songs, at any age, with an ocarina and their notes
    bool CanUse(RandomizerGet item) {
        if (item >= RG_MINUET_OF_FOREST)
            return warpSongs.count(item) != 0;
        const bool usable = !adultOnly || IsAdult;
        return usable && ((item == RG_LENS_OF_TRUTH && lens) || (item == RG_BOMBCHU_5 && bombchu) ||
                          (item == RG_MIRROR_SHIELD && mirror) || (item == RG_FIRE_ARROWS && fireArrows));
    }
    bool HasItem(RandomizerGet item) { return item == RG_POWER_BRACELET && bracelet; }
    bool SunlightArrows() { return sunlight && (!adultOnly || IsAdult); }
    bool SmallKeys(int scene, int count) { return scene == SCENE_FOREST_TEMPLE && forestKeys >= count; }
    bool Get(LogicVal value) { return value == LOGIC_GANON_MQ_SPIRIT_ENDING_SUNS && endingSuns; }
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
    RandomizerRegion GetParentRegionKey() const { return parent; }
    RandomizerRegion GetConnectedRegionKey() const { return connectedRegion; }
    RandomizerRegion GetOriginalConnectedRegionKey() const { return connectedRegion; }
    bool IsShuffled() const { return false; }
    Region* GetParentRegion() { return RegionTable(parent); }
    Region* GetConnectedRegion() { return RegionTable(connectedRegion); }
    bool GetConditionsMet() const { return condition(); }
    bool CheckConditionAtAgeTime(bool& age, bool& time, bool passAnyway = false) const;
};
struct Region {
    RandomizerRegion randomizerRegionKey;
    bool childDay = false, childNight = false, adultDay = false, adultNight = false, addedToPool = false;
    std::list<Entrance> exits;
    bool Child() const { return childDay || childNight; }
    bool Adult() const { return adultDay || adultNight; }
    void ApplyTimePass() {}
    bool UpdateEvents() { return false; }
};
std::array<Region, RR_MAX> regions;
auto& areaTable = regions;
void ClearRegions() {
    regions = {};
    for (size_t i = 0; i < regions.size(); ++i)
        regions[i].randomizerRegionKey = static_cast<RandomizerRegion>(i);
}
Region* RegionTable(RandomizerRegion key) {
    regions[key].randomizerRegionKey = key;
    return &regions[key];
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
function(fill, 'static bool UpdateToDAccess(') + '\n' + process
    )
