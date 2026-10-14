#ifdef COMBO_BUILD
#include "rando/entrances/MmRegionalFlags.h"
#endif
#ifndef RANDO_LOGIC_H
#define RANDO_LOGIC_H

#include "Rando/ActorBehavior/MissingMmSouls.h"
#include "Rando/Rando.h"
#include "Rando/Ocarinas.h"
#include "ComboAdultMaskInventory.h"
#include "ComboAdultGearInventory.h"
#ifdef COMBO_BUILD
#include "rando/MmAgeLogic.h"
#endif
#include "Rando/ActorBehavior/Souls.h"
#include "Rando/ActorBehavior/ExpandedSoulsLogic.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipUtils.h"

#include <set>
#include <cassert>
#include <type_traits>
#ifdef COMBO_BUILD
#include "rando/GameplayRules.h"
#include "rando/MmAbilities.h"
#include "rando/MmResourceAccess.h"
#include "rando/MmTricks.h"
#include "rando/MmTrickRoutes.h"
#include "rando/MmEnhancementLogic.h"
#include "rando/MmGhostHutRules.h"
#include "rando/MmCombatRules.h"
#include "rando/FaroreOracleRoutes.h"
#include "rando/SpellRules.h"
#else
enum { COMBO_MM_GHOST_HUT_MIN_HEARTS = 4 };
#endif

extern "C" {
#include "functions.h"
#include "variables.h"
}

#ifdef COMBO_BUILD
// ComboShip: extra ShipInit path so MM_InitRandoHeadless can build ONLY the rando region graph
// (ShipInit::Init("RANDO_LOGIC")), skipping the full UI/audio init. Region registrations pass this as
// their updatePaths; standalone builds get {} (behavior unchanged).
#define RANDO_LOGIC_INIT_PATHS \
    { "RANDO_LOGIC" }
// Hidden MM tricks (ComboRando::IsHiddenMmTrick) never count in logic, whatever a seed, profile or CVar
// says. bool_constant decides it at compile time, so a public trick is the plain CVar read.
#define COMBO_MM_TRICK(name)                                                          \
    (std::bool_constant<ComboRando::IsHiddenMmTrick("gCombo.Tricks.MM." name)>::value \
         ? 0                                                                          \
         : CVarGetInteger("gCombo.Tricks.MM." name, 0))
#define CAN_RESET_TIME Rando::Logic::NativeMmCanResetTime()
#define CAN_RESET_TIME_DUNGEON Rando::Logic::NativeMmCanResetTimeDungeon()
#define CAN_ACTIVATE_OWL (CAN_USE_SWORD || CAN_USE_STICKS)
#define CAN_FIGHT_WITH_STICKS (CAN_USE_STICKS && COMBO_MM_TRICK("DekuStickFighting"))
#define CAN_NAVIGATE_HIDDEN \
    ComboRando::CanNavigateHidden(HAS_ITEM(ITEM_LENS_OF_TRUTH), HAS_MAGIC, COMBO_MM_TRICK("Lensless") != 0)
#define CAN_CROSS_PINNACLE_ROCK                                                                                   \
    ComboRando::MmTrickRoutes::PinnacleRock(CAN_BE_ZORA, CAN_USE_ABILITY(SWIM), RANDO_EVENTS[RE_ACCESS_SEAHORSE], \
                                            COMBO_MM_TRICK("PinnacleNoSeahorse") != 0)
#define CAN_WIN_POST_OFFICE                                                                                   \
    ComboRando::MmTrickRoutes::PostOffice(HAS_ITEM(ITEM_MASK_BUNNY), COMBO_MM_TRICK("PostOfficeTiming") != 0, \
                                          BETWEEN(TIME_DAY1_PM_03_00, TIME_NIGHT1_AM_12_00),                  \
                                          RANDO_EVENTS[RE_MAIL_LETTER_TO_KAFEI],                              \
                                          BETWEEN(TIME_NIGHT2_PM_06_00, TIME_NIGHT2_AM_12_00))
#define CAN_OPEN_OCEANSIDE_CODE_CHEST                                                  \
    ComboRando::MmTrickRoutes::OceansideCode(CAN_USE_BOW, HAS_ITEM(ITEM_MASK_CAPTAIN), \
                                             COMBO_MM_TRICK("OceansideCode") != 0)
#define CAN_REACH_CLOCK_TOWN_LOWER_CHEST                                                                          \
    ComboRando::MmTrickRoutes::ClockTownLowerChest(CAN_BE_DEKU, Flags_GetRandoInf(RANDO_INF_OBTAINED_MOONS_TEAR), \
                                                   HAS_ITEM(ITEM_HOOKSHOT), COMBO_MM_TRICK("ClockTownLowerJump") != 0)
#define CAN_REACH_DOGGY_CHEST                                                                           \
    ComboRando::MmTrickRoutes::DoggyChest(HAS_ITEM(ITEM_HOOKSHOT), CAN_USE_DAY2_RAIN_BEAN, CAN_BE_ZORA, \
                                          COMBO_MM_TRICK("DoggyFenceJump") != 0)
#define CAN_SKIP_PALACE_BEANS ComboRando::MmTrickRoutes::PalaceDoorwayClimb(COMBO_MM_TRICK("PalaceBeanSkip") != 0)
#define CAN_CLIMB_IKANA_RIVERSIDE                                                            \
    ComboRando::MmTrickRoutes::IkanaClimb(HAS_ITEM(ITEM_HOOKSHOT), CAN_USE_MAGIC_ARROW(ICE), \
                                          COMBO_MM_TRICK("IkanaIceSkip") != 0)
#define CAN_CLIMB_STONE_TOWER                                                                               \
    ComboRando::MmTrickRoutes::StoneTowerClimb(HAS_ITEM(ITEM_HOOKSHOT), CAN_PLAY_SONG(ELEGY), CAN_BE_GORON, \
                                               CAN_BE_ZORA, COMBO_MM_TRICK("StoneTowerOneMask") != 0)
#else
#define RANDO_LOGIC_INIT_PATHS \
    {}
#define COMBO_MM_TRICK(name) 0
#define CAN_RESET_TIME true
#define CAN_RESET_TIME_DUNGEON true
#define CAN_ACTIVATE_OWL CAN_USE_SWORD
#define CAN_FIGHT_WITH_STICKS CAN_USE_STICKS
#define CAN_NAVIGATE_HIDDEN (HAS_ITEM(ITEM_LENS_OF_TRUTH) && HAS_MAGIC)
#define CAN_CROSS_PINNACLE_ROCK (RANDO_EVENTS[RE_ACCESS_SEAHORSE] && CAN_BE_ZORA)
#define CAN_WIN_POST_OFFICE                                \
    (HAS_ITEM(ITEM_MASK_BUNNY) &&                          \
     (BETWEEN(TIME_DAY1_PM_03_00, TIME_NIGHT1_AM_12_00) || \
      (RANDO_EVENTS[RE_MAIL_LETTER_TO_KAFEI] && BETWEEN(TIME_NIGHT2_PM_06_00, TIME_NIGHT2_AM_12_00))))
#define CAN_OPEN_OCEANSIDE_CODE_CHEST (CAN_USE_BOW && HAS_ITEM(ITEM_MASK_CAPTAIN))
#define CAN_REACH_CLOCK_TOWN_LOWER_CHEST \
    ((CAN_BE_DEKU && Flags_GetRandoInf(RANDO_INF_OBTAINED_MOONS_TEAR)) || HAS_ITEM(ITEM_HOOKSHOT))
#define CAN_REACH_DOGGY_CHEST (HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_DAY2_RAIN_BEAN || CAN_BE_ZORA)
#define CAN_SKIP_PALACE_BEANS false
#define CAN_CLIMB_IKANA_RIVERSIDE (HAS_ITEM(ITEM_HOOKSHOT) && CAN_USE_MAGIC_ARROW(ICE))
#define CAN_CLIMB_STONE_TOWER (HAS_ITEM(ITEM_HOOKSHOT) && CAN_PLAY_SONG(ELEGY) && CAN_BE_GORON && CAN_BE_ZORA)
#endif

namespace Rando {

namespace Logic {

// Time slice enum - 46 pinned OoTMM time points throughout MM's 3-day cycle
enum TimeSlice {
    TIME_DAY1_AM_06_00 = 0,
    TIME_DAY1_AM_07_00,
    TIME_DAY1_AM_08_00,
    TIME_DAY1_AM_10_00,
    TIME_DAY1_PM_01_45,
    TIME_DAY1_PM_03_00,
    TIME_DAY1_PM_04_00,
    TIME_NIGHT1_PM_06_00,
    TIME_NIGHT1_PM_08_00,
    TIME_NIGHT1_PM_09_00,
    TIME_NIGHT1_PM_10_00,
    TIME_NIGHT1_PM_11_00,
    TIME_NIGHT1_AM_12_00,
    TIME_NIGHT1_AM_02_30,
    TIME_NIGHT1_AM_04_00,
    TIME_NIGHT1_AM_05_00,
    TIME_DAY2_AM_06_00,
    TIME_DAY2_AM_07_00,
    TIME_DAY2_AM_08_00,
    TIME_DAY2_AM_10_00,
    TIME_DAY2_AM_11_30,
    TIME_DAY2_PM_02_00,
    TIME_DAY2_PM_04_00,
    TIME_NIGHT2_PM_06_00,
    TIME_NIGHT2_PM_08_00,
    TIME_NIGHT2_PM_09_00,
    TIME_NIGHT2_PM_10_00,
    TIME_NIGHT2_PM_11_00,
    TIME_NIGHT2_AM_12_00,
    TIME_NIGHT2_AM_04_00,
    TIME_NIGHT2_AM_05_00,
    TIME_NIGHT2_AM_05_30,
    TIME_DAY3_AM_06_00,
    TIME_DAY3_AM_07_00,
    TIME_DAY3_AM_08_00,
    TIME_DAY3_AM_10_00,
    TIME_DAY3_AM_11_30,
    TIME_DAY3_PM_01_00,
    TIME_NIGHT3_PM_06_00,
    TIME_NIGHT3_PM_08_00,
    TIME_NIGHT3_PM_09_00,
    TIME_NIGHT3_PM_10_00,
    TIME_NIGHT3_PM_11_00,
    TIME_NIGHT3_AM_12_00,
    TIME_NIGHT3_AM_04_00,
    TIME_NIGHT3_AM_05_00 // = 45
};

// Time slice count and bitmask constants
// Derived from enum - update if last enum value changes
constexpr int TIME_SLICE_COUNT = TIME_NIGHT3_AM_05_00 + 1;
constexpr uint64_t TIME_BIT_ONE = 1ULL;              // Base value for bit shifting
constexpr uint64_t TIME_ALL_SLICES = (TIME_BIT_ONE << TIME_SLICE_COUNT) - 1; // All 46 time bits set

// Half-day period definitions for Clock Shuffle
struct HalfDayRange {
    int startSlice;
    int endSlice;
};

// Map half-day indices to their time slice ranges
// Index 0-5 correspond to HALF_DAY1_DAY through HALF_DAY3_NIGHT
constexpr HalfDayRange HALF_DAY_TIME_RANGES[6] = {
    { TIME_DAY1_AM_06_00, TIME_DAY1_PM_04_00 }, { TIME_NIGHT1_PM_06_00, TIME_NIGHT1_AM_05_00 },
    { TIME_DAY2_AM_06_00, TIME_DAY2_PM_04_00 }, { TIME_NIGHT2_PM_06_00, TIME_NIGHT2_AM_05_30 },
    { TIME_DAY3_AM_06_00, TIME_DAY3_PM_01_00 }, { TIME_NIGHT3_PM_06_00, TIME_NIGHT3_AM_05_00 },
};

// Game time constants for day/night transitions
constexpr u16 GAME_TIME_DAY_START = 0x4000;   // 6:00 AM
constexpr u16 GAME_TIME_NIGHT_START = 0xc000; // 6:00 PM

// Time state for tracking time accessibility during logic solving
struct RegionTimeState {
    uint64_t timeSlices;
    bool canStayOverTime;
#ifdef COMBO_BUILD
    ComboRando::Entrances::MmFlagDomain regionalFlags;
#endif
};

// Set by every read of gCurrentRegionTime, so time-independent predicates are evaluated once.
inline thread_local bool gCurrentRegionTimeRead = false;
extern uint64_t gCurrentRegionTime;
#ifdef COMBO_BUILD
bool NativeMmErContextsEnabled();
inline bool NativeMmNorthWinter() {
    return NativeMmErContextsEnabled() ? ComboRando::Entrances::MmRegionalCursed(1)
                                       : !RANDO_EVENTS[RE_CLEARED_SNOWHEAD_TEMPLE];
}
inline bool NativeMmNorthSpring() {
    return NativeMmErContextsEnabled()
               ? ComboRando::Entrances::MmRegionalCleared(1, RANDO_EVENTS[RE_CLEARED_SNOWHEAD_TEMPLE])
               : bool(RANDO_EVENTS[RE_CLEARED_SNOWHEAD_TEMPLE]);
}
inline bool NativeMmSwampPoisoned() {
    return NativeMmErContextsEnabled() ? ComboRando::Entrances::MmRegionalCursed(0)
                                       : !RANDO_EVENTS[RE_CLEARED_WOODFALL_TEMPLE];
}
inline bool NativeMmSwampCleared() {
    return NativeMmErContextsEnabled()
               ? ComboRando::Entrances::MmRegionalCleared(0, RANDO_EVENTS[RE_CLEARED_WOODFALL_TEMPLE])
               : bool(RANDO_EVENTS[RE_CLEARED_WOODFALL_TEMPLE]);
}
inline bool NativeMmOceanCleared() {
    return NativeMmErContextsEnabled()
               ? ComboRando::Entrances::MmRegionalCleared(2, RANDO_EVENTS[RE_CLEARED_GREAT_BAY_TEMPLE])
               : bool(RANDO_EVENTS[RE_CLEARED_GREAT_BAY_TEMPLE]);
}
#else
inline bool NativeMmErContextsEnabled() {
    return false;
}
inline bool NativeMmNorthWinter() {
    return !RANDO_EVENTS[RE_CLEARED_SNOWHEAD_TEMPLE];
}
inline bool NativeMmNorthSpring() {
    return RANDO_EVENTS[RE_CLEARED_SNOWHEAD_TEMPLE];
}
inline bool NativeMmSwampPoisoned() {
    return !RANDO_EVENTS[RE_CLEARED_WOODFALL_TEMPLE];
}
inline bool NativeMmSwampCleared() {
    return RANDO_EVENTS[RE_CLEARED_WOODFALL_TEMPLE];
}
inline bool NativeMmOceanCleared() {
    return RANDO_EVENTS[RE_CLEARED_GREAT_BAY_TEMPLE];
}
#endif

#ifdef COMBO_BUILD
// Scoped by the tracker only; native generation keeps the default false.
inline thread_local bool evaluatingLiveTracker = false;
class LiveTrackerRuleScope {
    bool previous;

  public:
    LiveTrackerRuleScope() : previous(evaluatingLiveTracker) {
        evaluatingLiveTracker = true;
    }
    ~LiveTrackerRuleScope() {
        evaluatingLiveTracker = previous;
    }
    LiveTrackerRuleScope(const LiveTrackerRuleScope&) = delete;
    LiveTrackerRuleScope& operator=(const LiveTrackerRuleScope&) = delete;
};
inline int GameplayRuleValue(const char* runtime) {
    const auto* rule = ComboRando::FindGameplayRule(runtime);
    return rule ? ComboRando::ReadGameplayRule(*rule, !evaluatingLiveTracker, CVarGetInteger) : 0;
}
#endif

#ifdef COMBO_BUILD
// OoTMM ACCESS is a query event, separate from its reset capability implementation.
// Routed entrances record the original reachable edge BEFORE filtering its destination.
bool NativeMmCanResetTime();
bool NativeMmCanResetTimeDungeon();
bool NativeMmIngressNeedsDungeonReset(s32 entrance);
bool NativeMmIngressNeedsReset(s32 entrance);
uint64_t FilterNativeMmIngressTimes(s32 entrance, uint64_t arrivalTimes);
bool RecordMmAccess(uint64_t timeMask);
void ResetMmAccess();
bool UpdateMmAccess(); // Consume ACCESS changes as part of every graph fixpoint.
// OoTMM macros_mm.yml:305 er_enabled over the unified entrance layout's settings (ComboEntrancesMM.cpp).
bool NativeMmErEnabled();
class MmAccessScope {
    uint64_t previousIngress;
    bool previousAccess;
    bool previousChanged;

  public:
    MmAccessScope();
    ~MmAccessScope();
    MmAccessScope(const MmAccessScope&) = delete;
    MmAccessScope& operator=(const MmAccessScope&) = delete;
};
#endif

// Helper: Convert runtime game time to TimeSlice enum
TimeSlice TimeSliceFromGameTime(s32 day, u16 time);

// Helper: Returns the initial time state for logic solving
RegionTimeState InitialTimeState();

// Shared initialization function for region time states
std::unordered_map<RandoRegionId, RegionTimeState> InitializeRegionTimeStates(RandoRegionId startRegion);

// Helper to ensure region time state exists
void EnsureRegionTimeState(std::unordered_map<RandoRegionId, RegionTimeState>& regionTimeStates,
                           RandoRegionId regionId);

// Helper to set current region time
inline void SetCurrentRegionTime(const std::unordered_map<RandoRegionId, RegionTimeState>& regionTimeStates,
                                 RandoRegionId regionId) {
    gCurrentRegionTime = regionTimeStates.at(regionId).timeSlices;
#ifdef COMBO_BUILD
    ComboRando::Entrances::mmFlagSource = regionTimeStates.at(regionId).regionalFlags;
#endif
}

void FindReachableRegions(RandoRegionId currentRegion, std::set<RandoRegionId>& reachableRegions,
                          std::unordered_map<RandoRegionId, RegionTimeState>& regionTimeStates);
RandoRegionId GetRegionIdFromEntrance(s32 entrance);
void GeneratePools(RandoSaveInfo& saveInfo, std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool);
void ApplyGlitchlessLogicToSaveContext(std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool);
void ApplyNearlyNoLogicToSaveContext(std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool);
void ApplyNoLogicToSaveContext(std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool);
bool IsItemAllowedAtCheck(RandoItemId itemId, RandoCheckId checkId);
size_t SelectItemForCheck(const std::vector<RandoItemId>& itemPool, const std::vector<RandoCheckId>& checkPool,
                          RandoCheckId checkId);
void PreplaceConfinedItems(std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool);

struct RandoRegionExit {
    s32 returnEntrance;
    std::function<bool()> condition;
    std::string conditionString;
};

struct RandoRegion {
    const char* name = "";
    SceneId sceneId;
    std::map<RandoCheckId, std::pair<std::function<bool()>, std::string>> checks;
    std::map<s32, RandoRegionExit> exits;
    std::map<RandoRegionId, std::pair<std::function<bool()>, std::string>> connections;
    std::vector<std::pair<RandoEvent, std::function<bool()>>> events;
    std::set<s32> oneWayEntrances;

    // Time logic fields for Clock Shuffle and time-based region access
    uint64_t timeSlices = 0;     // Bitfield: accessible time slices (bits 0-45) - unused in current implementation
    bool canStayOverTime = true; // Can player wait for time to pass? Set false for dungeons, shops with closing hours
    std::unordered_map<TimeSlice, std::function<bool()>>
        timeStayRestrictions; // Time slices where staying is restricted - use STAY() macro in region definitions
};

extern std::map<RandoRegionId, RandoRegion> Regions;

// ============================================================================
// TIME LOGIC NAMESPACE
// ============================================================================
#ifdef COMBO_BUILD
bool UpdateResourceSources(const std::set<RandoRegionId>& reached,
                           std::unordered_map<RandoRegionId, RegionTimeState>& times);
#endif

namespace TimeLogic {
// Preserve the time restrictions of a native boolean route predicate.
uint64_t EvaluateConditionTimes(const std::function<bool()>& condition, uint64_t timeSlices);

// Core expansion function - sequential time expansion with stay restrictions
uint64_t ExpandTimeForward(uint64_t timeSlices, const RandoRegion& region);

// Owned time calculation - aggregates all owned half-day time slices
uint64_t GetOwnedTimeSlices();

// The bit of the slice holding the current in-game time. Where the player stands is known
// reachable from then on (ExpandTimeForward applies the region's stay limits), not earlier.
uint64_t CurrentTimeSlice();

// Validation helper for clock ownership during logic generation
void ValidateRegionTimeOwnership(RandoRegionId regionId, RandoCheckId checkId, uint64_t regionTime,
                                 const char* context);
} // namespace TimeLogic

#ifdef COMBO_BUILD
bool HasUsableBottle();
#endif

// TODO: This may not stay here
#define IS_DEKU (GET_PLAYER_FORM == PLAYER_FORM_DEKU)
#define IS_ZORA (GET_PLAYER_FORM == PLAYER_FORM_ZORA)
#define IS_DEITY (GET_PLAYER_FORM == PLAYER_FORM_FIERCE_DEITY)
#define IS_GORON (GET_PLAYER_FORM == PLAYER_FORM_GORON)
#define IS_HUMAN (GET_PLAYER_FORM == PLAYER_FORM_HUMAN)
#define HAS_ITEM(item)                                                                                          \
    (ComboAdultGearInventory::Group(item)                                                                       \
         ? ComboAdultGearInventory::Has(ComboAdultGearInventory::Group(item) == 2                               \
                                            ? gSaveContext.save.shipSaveInfo.adultMm.gearTrade2Bits             \
                                            : gSaveContext.save.shipSaveInfo.adultMm.gearTrade3Bits,            \
                                        item)                                                                   \
         : ((item) == ITEM_MASK_FIERCE_DEITY                                                                    \
                ? ComboAdultMaskInventory::HasFierceDeity(gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits, \
                                                          INV_CONTENT(ITEM_MASK_FIERCE_DEITY))                  \
                : (INV_CONTENT(item) == item)))
#ifdef COMBO_BUILD
// The age the macros_mm.yml:59-62 age terms read. Native MM stores linkAge 0 for its child Link, so a save's linkAge
// is an age only while its adult state is enabled (ComboAdultMM::IsAdult); every other save is child.
#define MM_LOGIC_AGE (gSaveContext.save.shipSaveInfo.adultMm.enabled ? gSaveContext.save.linkAge : 1)
#define MM_LOGIC_IS_CHILD ComboRando::MmAgeLogic::IsChild(RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] != 0, MM_LOGIC_AGE)
#define MM_LOGIC_IS_ADULT ComboRando::MmAgeLogic::IsAdult(RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] != 0, MM_LOGIC_AGE)
#define MM_LOGIC_IS_TALL ComboRando::MmAgeLogic::IsTall(HAS_ITEM(ITEM_MASK_ZORA), MM_LOGIC_IS_ADULT)
#define MM_LOGIC_IS_SHORT ComboRando::MmAgeLogic::IsShort(HAS_ITEM(ITEM_MASK_DEKU), MM_LOGIC_IS_CHILD)
// Pinned stone_tower.yml graph (East.cpp), which splits the temple front from the Soaring top.
#define MM_LOGIC_STONE_TOWER_PINNED (RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] || RANDO_SAVE_OPTIONS[RO_MM_BOOTS_HOVER])
// Pinned OoTMM v32.3 Deku Palace, Southern Swamp and Woodfall graph (South.cpp and MmSwampContexts.inc.h): on
// whenever a setting is on whose terms only that graph can express (is_adult/is_tall, hover and iron boots,
// and Nayru's Love, e.g. deku_palace.yml:7-9, woodfall.yml:11-52, macros_mm.yml:260 cross_poison_swamp).
#define MM_LOGIC_SOUTH_PINNED                                                        \
    (RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] || RANDO_SAVE_OPTIONS[RO_MM_BOOTS_HOVER] || \
     RANDO_SAVE_OPTIONS[RO_MM_BOOTS_IRON] || RANDO_SAVE_OPTIONS[RO_MM_SPELL_LOVE])
// Pinned macros_mm.yml70..75: MM equipment ownership is effective only with its option.
#define MM_LOGIC_HAS_IRON_BOOTS (RANDO_SAVE_OPTIONS[RO_MM_BOOTS_IRON] && HAS_ITEM(ITEM_COMBO_BOOTS_IRON))
#define MM_LOGIC_HAS_HOVER_BOOTS (RANDO_SAVE_OPTIONS[RO_MM_BOOTS_HOVER] && HAS_ITEM(ITEM_COMBO_BOOTS_HOVER))
#define MM_LOGIC_HAS_GORON_TUNIC_STRICT (RANDO_SAVE_OPTIONS[RO_MM_TUNIC_GORON] && HAS_ITEM(ITEM_COMBO_TUNIC_GORON))
#define MM_LOGIC_HAS_ZORA_TUNIC_STRICT (RANDO_SAVE_OPTIONS[RO_MM_TUNIC_ZORA] && HAS_ITEM(ITEM_COMBO_TUNIC_ZORA))
// macros_mm.yml:76-77: trick(MM_TUNICS) ("Fewer Tunic Requirements (MM)") stands in for either tunic.
#define MM_LOGIC_HAS_GORON_TUNIC (MM_LOGIC_HAS_GORON_TUNIC_STRICT || COMBO_MM_TRICK("Tunics"))
#define MM_LOGIC_HAS_ZORA_TUNIC (MM_LOGIC_HAS_ZORA_TUNIC_STRICT || COMBO_MM_TRICK("Tunics"))
// macros_mm.yml:84 can_swim_or_sink = can_swim || has_iron_boots. The port has no MM scales, so can_swim is the
// native Swim ability, as on the water edges that use this macro.
#define MM_LOGIC_CAN_SWIM_OR_SINK (CAN_USE_ABILITY(SWIM) || MM_LOGIC_HAS_IRON_BOOTS)
// macros_mm.yml:258-259 underwater_walking and underwater_walking_strict.
#define MM_LOGIC_UNDERWATER_WALKING (HAS_ITEM(ITEM_MASK_ZORA) || (MM_LOGIC_HAS_ZORA_TUNIC && MM_LOGIC_HAS_IRON_BOOTS))
#define MM_LOGIC_UNDERWATER_WALKING_STRICT \
    (HAS_ITEM(ITEM_MASK_ZORA) || (MM_LOGIC_HAS_IRON_BOOTS && MM_LOGIC_HAS_ZORA_TUNIC_STRICT))

#endif

#define CAN_BE_DEKU (IS_DEKU || HAS_ITEM(ITEM_MASK_DEKU))
#define CAN_BE_ZORA (IS_ZORA || HAS_ITEM(ITEM_MASK_ZORA))
#define CAN_BE_DEITY (IS_DEITY || HAS_ITEM(ITEM_MASK_FIERCE_DEITY))
#define CAN_BE_GORON (IS_GORON || HAS_ITEM(ITEM_MASK_GORON))
#define CAN_BE_HUMAN                                                                                        \
    (IS_HUMAN || (IS_DEITY && HAS_ITEM(ITEM_MASK_FIERCE_DEITY)) || (IS_ZORA && HAS_ITEM(ITEM_MASK_ZORA)) || \
     (IS_DEKU && HAS_ITEM(ITEM_MASK_DEKU)) || (IS_GORON && HAS_ITEM(ITEM_MASK_GORON)))
#define CHECK_MAX_HP(TARGET_HP) ((TARGET_HP * 16) <= gSaveContext.save.saveInfo.playerData.healthCapacity)
#define HAS_MAGIC (gSaveContext.save.saveInfo.playerData.isMagicAcquired)
#ifdef COMBO_BUILD
#undef HAS_MAGIC
#define HAS_MAGIC \
    ComboRando::mmResourceAccess.CanUse(gSaveContext.save.saveInfo.playerData.isMagicAcquired, ComboRando::MmMagic)
#define CAN_USE_BOW ComboRando::mmResourceAccess.CanUse(HAS_ITEM(ITEM_BOW), ComboRando::MmArrows)
#define CAN_USE_BOMBS ComboRando::mmResourceAccess.CanUse(HAS_ITEM(ITEM_BOMB), ComboRando::MmBombs)
#define CAN_USE_BOMBCHUS                                                                                 \
    (ComboRando::mmResourceAccess.active                                                                 \
         ? ComboRando::mmResourceAccess.CanUse(CUR_UPG_VALUE(UPG_BOMB_BAG) != 0, ComboRando::MmBombchus) \
         : HAS_ITEM(ITEM_BOMBCHU))
#define CAN_USE_STICKS                                                                            \
    (ComboRando::mmResourceAccess.active ? ComboRando::mmResourceAccess.Has(ComboRando::MmSticks) \
                                         : HAS_ITEM(ITEM_DEKU_STICK))
#define CAN_USE_NUTS                                                                            \
    (ComboRando::mmResourceAccess.active ? ComboRando::mmResourceAccess.Has(ComboRando::MmNuts) \
                                         : HAS_ITEM(ITEM_DEKU_NUT))
#else
#define CAN_USE_BOW HAS_ITEM(ITEM_BOW)
#define CAN_USE_BOMBS HAS_ITEM(ITEM_BOMB)
#define CAN_USE_BOMBCHUS HAS_ITEM(ITEM_BOMBCHU)
#define CAN_USE_STICKS HAS_ITEM(ITEM_DEKU_STICK)
#define CAN_USE_NUTS HAS_ITEM(ITEM_DEKU_NUT)
#endif
#ifdef COMBO_BUILD
#define CAN_HOOK_SCARECROW                                                                           \
    (HAS_PLAYABLE_OCARINA && HAS_ITEM(ITEM_HOOKSHOT) && canPlaySong(OCARINA_SONG_SCARECROW_SPAWN) && \
     (RANDO_EVENTS[RE_LEARN_SCARECROW_SONG] || CHECK_WEEKEVENTREG(WEEKEVENTREG_79_08) ||             \
      COMBO_LOGIC_RULE("gEnhancements.Playback.SkipScarecrowSong")))
#else
#define CAN_HOOK_SCARECROW \
    (HAS_PLAYABLE_OCARINA && HAS_ITEM(ITEM_HOOKSHOT) && canPlaySong(OCARINA_SONG_SCARECROW_SPAWN))
#endif
#ifdef COMBO_BUILD
// OoTMM has_explosives: the Blast Mask works without a shield (it deals damage).
#define CAN_USE_EXPLOSIVE (CAN_USE_BOMBS || CAN_USE_BOMBCHUS || HAS_ITEM(ITEM_MASK_BLAST))
#else
#define CAN_USE_EXPLOSIVE                 \
    (CAN_USE_BOMBS || CAN_USE_BOMBCHUS || \
     (HAS_ITEM(ITEM_MASK_BLAST) && GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) > EQUIP_VALUE_SHIELD_NONE))
#endif
#define CAN_USE_HUMAN_SWORD (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) >= EQUIP_VALUE_SWORD_KOKIRI)
#ifdef COMBO_BUILD
#define COMBO_LOGIC_RULE(name) Rando::Logic::GameplayRuleValue(name)
#define CAN_USE_DEITY_OUTSIDE_BOSS                                       \
    ComboRando::CanUseDeityOutsideBoss(HAS_ITEM(ITEM_MASK_FIERCE_DEITY), \
                                       COMBO_LOGIC_RULE("gEnhancements.Masks.FierceDeitysAnywhere"))
#define CAN_WADE_SHALLOW_WATER                                                                       \
    ComboRando::CanTraverseWaistWater(CAN_USE_ABILITY(SWIM), CAN_BE_DEKU, CAN_BE_ZORA, CAN_BE_GORON, \
                                      CAN_USE_DEITY_OUTSIDE_BOSS)
#define CAN_GORON_SPIKE_ROLL \
    ComboRando::CanSpikeRoll(CAN_BE_GORON, HAS_MAGIC, COMBO_LOGIC_RULE("gEnhancements.Masks.GoronRollingIgnoresMagic"))
#define CAN_USE_POWDER_KEG                                                                                   \
    ComboRando::CanUsePowderKeg(                                                                             \
        CAN_BE_GORON,                                                                                        \
        (HAS_ITEM(ITEM_POWDER_KEG) &&                                                                        \
         (RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] != RO_GENERIC_YES || RANDO_EVENTS[RE_EXPANDED_BUY_KEG])), \
        HAS_ITEM(ITEM_MASK_BLAST), COMBO_LOGIC_RULE("gEnhancements.Masks.BlastMaskKeg"))
#define CAN_PASS_GIBDO(resource)                                                                         \
    ComboRando::CanPassGibdoDoor(COMBO_LOGIC_RULE("gEnhancements.DifficultyOptions.GibdoTradeSequence"), \
                                 HAS_ITEM(ITEM_MASK_GIBDO), (resource))
#define CAN_SUPPLY_GIBDO_POTION                                                                              \
    ComboRando::CanSupplyGibdoPotion(COMBO_LOGIC_RULE("gEnhancements.DifficultyOptions.GibdoTradeSequence"), \
                                     HAS_BOTTLE, CAN_ACCESS(BLUE_POTION_REFILL), CAN_ACCESS(RED_POTION_REFILL))
#define FOUND_ENOUGH_CHOIR_FROGS                                                                                    \
    ComboRando::HasEnoughChoirFrogs(COMBO_LOGIC_RULE("gEnhancements.DifficultyOptions.FrogChoirCount"),             \
                                    CHECK_WEEKEVENTREG(WEEKEVENTREG_32_40), CHECK_WEEKEVENTREG(WEEKEVENTREG_32_80), \
                                    CHECK_WEEKEVENTREG(WEEKEVENTREG_33_01), CHECK_WEEKEVENTREG(WEEKEVENTREG_33_02))
#define CAN_CROSS_OPEN_WATER_SURFACE                                         \
    ComboRando::CanCrossOpenWaterSurface(CAN_USE_ABILITY(SWIM), CAN_BE_DEKU, \
                                         COMBO_LOGIC_RULE("gEnhancements.Player.InfiniteDekuHopping") != 0)
#define CAN_WIN_DOGGY_RACE                                                                      \
    ComboRando::CanWinDoggyRace(COMBO_LOGIC_RULE("gEnhancements.Minigames.AlwaysWinDoggyRace"), \
                                HAS_ITEM(ITEM_MASK_TRUTH))
#else
#define COMBO_LOGIC_RULE(name) CVarGetInteger(name, 0)
#define CAN_USE_DEITY_OUTSIDE_BOSS (CAN_BE_DEITY && CVarGetInteger("gEnhancements.Masks.FierceDeitysAnywhere", 0))
#define CAN_WADE_SHALLOW_WATER (CAN_USE_ABILITY(SWIM) || CAN_BE_DEKU || CAN_BE_ZORA || CAN_BE_GORON)
#define CAN_GORON_SPIKE_ROLL (CAN_BE_GORON && HAS_MAGIC)
#define CAN_USE_POWDER_KEG                        \
    (CAN_BE_GORON && HAS_ITEM(ITEM_POWDER_KEG) && \
     (RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] != RO_GENERIC_YES || RANDO_EVENTS[RE_EXPANDED_BUY_KEG]))
#define CAN_PASS_GIBDO(resource) (HAS_ITEM(ITEM_MASK_GIBDO) && (resource))
#define CAN_SUPPLY_GIBDO_POTION (HAS_BOTTLE && CAN_ACCESS(BLUE_POTION_REFILL))
#define FOUND_ENOUGH_CHOIR_FROGS FOUND_ALL_FROGS
#define CAN_CROSS_OPEN_WATER_SURFACE CAN_USE_ABILITY(SWIM)
#define CAN_WIN_DOGGY_RACE HAS_ITEM(ITEM_MASK_TRUTH)
#endif
#define CAN_USE_SWORD (CAN_USE_HUMAN_SWORD || HAS_ITEM(ITEM_SWORD_GREAT_FAIRY) || CAN_USE_DEITY_OUTSIDE_BOSS)
#ifdef COMBO_BUILD
// OoTMM v32.3 keaton_grass_easy / keaton_grass_hard. The first
// patch is free; later patches must be cut before the ring retreats.
#define CAN_CUT_KEATON_GRASS_EASY (CAN_USE_SWORD && HAS_MAGIC)
#define CAN_CUT_KEATON_GRASS_HARD                                                                        \
    (CAN_CUT_KEATON_GRASS_EASY &&                                                                        \
     (CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK) || HAS_ITEM(ITEM_SWORD_GREAT_FAIRY) || \
      GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) >= EQUIP_VALUE_SWORD_GILDED))
#else
#define CAN_CUT_KEATON_GRASS_EASY true
#define CAN_CUT_KEATON_GRASS_HARD true
#endif
#define CAN_FULLY_CUT_KEATON_GRASS                                                      \
    (HAS_MAGIC && (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) >= EQUIP_VALUE_SWORD_RAZOR) && \
     CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK))
// Be careful here, as some checks require you to play the song as a specific form
// Exact pinned macros_mm.yml has_ocarina gate, using the persisted seed options.
#define HAS_PLAYABLE_OCARINA                                                                                           \
    Rando::Ocarinas::CanPlay(RANDO_SAVE_OPTIONS[RO_SHARED_OCARINA] != 0, RANDO_SAVE_OPTIONS[RO_FAIRY_OCARINA_MM] != 0, \
                             INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_OF_TIME                                 \
                                 ? 2                                                                                   \
                                 : (INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_FAIRY ? 1 : 0))
#define CAN_PLAY_SONG(song)                                         \
    (HAS_PLAYABLE_OCARINA && CHECK_QUEST_ITEM(QUEST_SONG_##song) && \
     Rando::Logic::canPlaySong((QUEST_SONG_##song - QUEST_SONG_SONATA) + OCARINA_SONG_SONATA))
#define CAN_RIDE_EPONA (CAN_PLAY_SONG(EPONA))
#define GBT_CAN_REVERSE_WATER_FLOW                                                         \
    (RANDO_EVENTS[RE_GREAT_BAY_RED_SWITCH_1] && RANDO_EVENTS[RE_GREAT_BAY_RED_SWITCH_2] && \
     HAS_ITEM(ITEM_HOOKSHOT)) // Keeping for the sake of check tracker clarity
#define GBT_GREEN_SWITCH_FLOW                                                                  \
    (RANDO_EVENTS[RE_GREAT_BAY_GREEN_SWITCH_1] && RANDO_EVENTS[RE_GREAT_BAY_GREEN_SWITCH_2] && \
     RANDO_EVENTS[RE_GREAT_BAY_GREEN_SWITCH_3])
#define ONE_WAY_EXIT -1
#define CAN_OWL_WARP(owlId) ((gSaveContext.save.saveInfo.playerData.owlActivationFlags >> owlId) & 1)
#define SET_OWL_WARP(owlId) (gSaveContext.save.saveInfo.playerData.owlActivationFlags |= (1 << owlId))
#define CLEAR_OWL_WARP(owlId) (gSaveContext.save.saveInfo.playerData.owlActivationFlags &= ~(1 << owlId))
#define HAS_BOTTLE_ITEM(item) (Inventory_HasItemInBottle(item))
#ifdef COMBO_BUILD
#define HAS_BOTTLE (Rando::Logic::HasUsableBottle())
#define CAN_SUPPLY_CHATEAU                           \
    (HAS_BOTTLE && (HAS_BOTTLE_ITEM(ITEM_CHATEAU) || \
                    (ComboRando::mmResourceAccess.active && ComboRando::mmResourceAccess.Has(ComboRando::MmChateau))))
#else
#define HAS_BOTTLE (INV_CONTENT(ITEM_BOTTLE) != ITEM_NONE)
#endif
#define CAN_USE_PROJECTILE (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || (CAN_BE_DEKU && HAS_MAGIC) || CAN_BE_ZORA)
#define CAN_ACCESS(access) (RANDO_EVENTS[RE_ACCESS_##access])
#ifdef COMBO_BUILD
// OoTMM macros_mm.yml has_hot_water_mtn / has_hot_water_distance. Ordinary
// Farore is available only when its persisted seed option, item and magic are present.
#define CAN_USE_DINS_FIRE                                                                          \
    ComboRando::SpellOracle::CanUse(                                                               \
        RANDO_SAVE_OPTIONS[RO_MM_SPELL_FIRE], HAS_MAGIC, RANDO_SAVE_OPTIONS[RO_SHARED_SPELL_FIRE], \
        gSaveContext.save.shipSaveInfo.spellFireOwned, gSaveContext.save.shipSaveInfo.spellFireOwned)
#define CAN_USE_NAYRUS_LOVE                                                                        \
    ComboRando::SpellOracle::CanUse(                                                               \
        RANDO_SAVE_OPTIONS[RO_MM_SPELL_LOVE], HAS_MAGIC, RANDO_SAVE_OPTIONS[RO_SHARED_SPELL_LOVE], \
        gSaveContext.save.shipSaveInfo.spellLoveOwned, gSaveContext.save.shipSaveInfo.spellLoveOwned)
#define CAN_USE_FARORE                                                                             \
    ComboRando::FaroreOracle::CanUse(                                                              \
        RANDO_SAVE_OPTIONS[RO_MM_SPELL_WIND], HAS_MAGIC, RANDO_SAVE_OPTIONS[RO_SHARED_SPELL_WIND], \
        gSaveContext.save.shipSaveInfo.faroresWind.owned, gSaveContext.save.shipSaveInfo.faroresWind.owned)
// OoTMM macros_mm.yml:182 can_use_fire_short_range = can_use_fire_arrows || can_use_din.
#define CAN_USE_FIRE_SHORT_RANGE (CAN_USE_MAGIC_ARROW(FIRE) || CAN_USE_DINS_FIRE)
// Shared and local Wind pickups converge into the same native owned byte through GiveItem.
#define CAN_USE_GBT_GEYSER                                                                             \
    ComboRando::FaroreOracle::GbtGeyser(COMBO_MM_TRICK("GbtCentralGeyser"), CAN_USE_MAGIC_ARROW(FIRE), \
                                        CAN_USE_MAGIC_ARROW(ICE), CAN_USE_DINS_FIRE, CAN_USE_FARORE,   \
                                        CAN_USE_NAYRUS_LOVE)
// OoTMM macros_mm.yml:305 er_enabled and :273 has_hot_water_er = er_enabled && has_hot_water_any.
// has_hot_water_any is the one hot spring water event with a bottle, as CAN_BRING_HOT_WATER_NORTH
// reads it; under entrance shuffle the water never cools (z_parameter.c). Sites:
// tools/ootmm/er-enabled-sites.json.
#define ER_ENABLED Rando::Logic::NativeMmErEnabled()
#define HAS_HOT_WATER_ER (ER_ENABLED && HAS_BOTTLE && CAN_ACCESS(HOT_SPRING_WATER))
// twin_island.yml:14, :19 list has_hot_water_er beside has_hot_water_mtn, so it is carried here: the
// MM Check Tracker shows the Goron Elder CHECK's condition text, which stays as it was.
#define CAN_BRING_HOT_WATER_NORTH                                                                 \
    ((CAN_ACCESS(HOT_SPRING_WATER) &&                                                             \
      (ComboRando::mmResourceAccess.Has(ComboRando::MmHotWaterNorth) ||                           \
       (CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_MOUNTAIN_VILLAGE)) || CAN_USE_FARORE)) || \
     HAS_HOT_WATER_ER)
#else
#define CAN_USE_FIRE_SHORT_RANGE CAN_USE_MAGIC_ARROW(FIRE)
#define CAN_BRING_HOT_WATER_NORTH CAN_ACCESS(HOT_SPRING_WATER)
#define ER_ENABLED false
#define HAS_HOT_WATER_ER false
#endif
#define CAN_GROW_BEAN_PLANT      \
    (CAN_ACCESS(BEANS_REFILL) && \
     (CAN_PLAY_SONG(STORMS) || (HAS_BOTTLE && (CAN_ACCESS(SPRING_WATER) || CAN_ACCESS(HOT_SPRING_WATER)))))
// Bean patches that auto-water on Day 2 (Doggy Racetrack, Deku Palace Upper)
// Rain only occurs Day 2 from 7:00 AM to 5:30 PM (not Night 2)
// Requires magic beans and (Day 2 OR manual watering)
#define CAN_USE_DAY2_RAIN_BEAN (CAN_GROW_BEAN_PLANT || (CAN_ACCESS(BEANS_REFILL) && CLOCK_DAY2()))
#define CAN_USE_MAGIC_ARROW(arrowType) (CAN_USE_BOW && HAS_ITEM(ITEM_ARROW_##arrowType) && HAS_MAGIC)
#define CAN_LIGHT_TORCH_NEAR_ANOTHER (CAN_USE_STICKS || CAN_USE_MAGIC_ARROW(FIRE) || CAN_USE_DINS_FIRE)
#define KEY_COUNT(dungeon) (gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[DUNGEON_SCENE_INDEX_##dungeon])
// Match the native TycoonWallet hook (5000), including headless searches where
// its display hook has not run. The ordinary fourth capacity is still 500.
#define MM_WALLET_CAPACITY                                                                                   \
    (CUR_UPG_VALUE(UPG_WALLET) >= 3 && RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] == RO_GENERIC_YES ? 5000 \
     : CUR_UPG_VALUE(UPG_WALLET) >= 2                                                                 ? 500  \
     : CUR_UPG_VALUE(UPG_WALLET) == 1                                                                 ? 200  \
                                                                                                      : 99)
#ifdef COMBO_BUILD
#define CAN_PAY(price) ComboRando::mmResourceAccess.CanPay((price), MM_WALLET_CAPACITY)
#else
#define CAN_PAY(price) ((price) <= MM_WALLET_CAPACITY)
#endif
#define CAN_AFFORD(rc) CAN_PAY(RANDO_SAVE_CHECKS[rc].price)
#define HAS_ENOUGH_STRAY_FAIRIES(dungeonIndex) \
    (gSaveContext.save.saveInfo.inventory.strayFairies[dungeonIndex] >= RANDO_SAVE_OPTIONS[RO_STRAY_FAIRIES_REQUIRED])
#define FOUND_ALL_FROGS                                                                  \
    (CHECK_WEEKEVENTREG(WEEKEVENTREG_33_01) && CHECK_WEEKEVENTREG(WEEKEVENTREG_32_40) && \
     CHECK_WEEKEVENTREG(WEEKEVENTREG_32_80) && CHECK_WEEKEVENTREG(WEEKEVENTREG_33_02))
#define CAN_USE_ABILITY(ability) Flags_GetRandoInf(RI_ABILITY_##ability - RI_ABILITY_SWIM + RANDO_INF_OBTAINED_SWIM)
#define HAS_ENOUGH_SKULLTULA_TOKENS(sceneId) \
    (Inventory_GetSkullTokenCount(sceneId) >= RANDO_SAVE_OPTIONS[RO_SKULLTULA_TOKENS_REQUIRED])

#define EVENT(randoEvent, condition)                                                               \
    {                                                                                              \
        randoEvent, [] { return (condition) && Rando::ExpandedSoulEventPrerequisite(randoEvent); } \
    }
#define EXIT(toEntrance, fromEntrance, condition)                           \
    {                                                                       \
        toEntrance, {                                                       \
            fromEntrance, [] { return condition; }, LogicString(#condition) \
        }                                                                   \
    }
#define CONNECTION(region, condition)                         \
    {                                                         \
        region, {                                             \
            [] { return condition; }, LogicString(#condition) \
        }                                                     \
    }
#define CHECK(check, condition)                                                                                \
    {                                                                                                          \
        check, {                                                                                               \
            [] { return (condition) && Rando::ExpandedSoulCheckPrerequisite(check); }, LogicString(#condition) \
        }                                                                                                      \
    }
// STAY macro for region time restrictions - defines when player can stay in a region over time
// Usage in region definitions: .timeStayRestrictions = { STAY(TIME_NIGHT1_PM_08_00, !HAS_ROOM_KEY) }
// If condition is false at the specified time, player is kicked out (expansion stops permanently)
// Examples:
//   STAY(TIME_NIGHT1_PM_08_00, !Flags_GetRandoInf(RANDO_INF_OBTAINED_ROOM_KEY)) // Kicked out without room key
//   STAY(TIME_NIGHT3_PM_10_00, false) // Always kicked out at this time (shop closes)
#define STAY(timeSlice, condition)          \
    {                                       \
        timeSlice, [] { return condition; } \
    }

inline std::string LogicString(std::string condition) {
    if (condition == "true")
        return "";

    return condition;
}

inline uint8_t FoundOcarinaButtons() {
    uint8_t foundButtons = 0;
    for (int i = RANDO_INF_OBTAINED_OCARINA_BUTTON_A; i <= RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP; i++) {
        if (Flags_GetRandoInf((RandoInf)i)) {
            foundButtons++;
        }
    }
    return foundButtons;
}

inline bool canPlaySong(u8 songId) {
    switch (songId) {
        case OCARINA_SONG_SONATA:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT));
        case OCARINA_SONG_GORON_LULLABY:
        case OCARINA_SONG_GORON_LULLABY_INTRO:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT));
        case OCARINA_SONG_NEW_WAVE:
        case OCARINA_SONG_ELEGY:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN));
        case OCARINA_SONG_OATH:
            // OoTMM can_play_order and the native song both use A, C-right,
            // C-down and C-up. C-left is only needed by the Zora sound check.
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP));
        case OCARINA_SONG_WIND_FISH_ZORA:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP));
        case OCARINA_SONG_TIME:
        case OCARINA_SONG_INVERTED_TIME:
        case OCARINA_SONG_DOUBLE_TIME:
        case OCARINA_SONG_WIND_FISH_GORON:
        case OCARINA_SONG_EVAN_PART1:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN));
        case OCARINA_SONG_HEALING:
        case OCARINA_SONG_SARIAS:
        case OCARINA_SONG_EVAN_PART2:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN));
        case OCARINA_SONG_EPONAS:
        case OCARINA_SONG_WIND_FISH_HUMAN:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT));
        case OCARINA_SONG_SOARING:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP));
        case OCARINA_SONG_STORMS:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP));
        case OCARINA_SONG_SUNS:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP));
        case OCARINA_SONG_WIND_FISH_DEKU:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT));
        case OCARINA_SONG_SCARECROW_SPAWN:
            return FoundOcarinaButtons() >= 2;
        case OCARINA_SONG_TERMINA_WALL:
            return (Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_A) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_DOWN) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_RIGHT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_LEFT) &&
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_OCARINA_BUTTON_C_UP));
        default:
            return true;
    }
}

inline bool CanAccessDungeon(DungeonSceneIndex dungeonIndex) {
    bool hasSongAccess = false;
    bool hasFormAccess = false;
    switch (dungeonIndex) {
        case DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE:
            hasSongAccess = CAN_PLAY_SONG(SONATA);
            hasFormAccess = CAN_BE_DEKU && HAS_PLAYABLE_OCARINA;
            break;
        case DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE:
            hasSongAccess = CAN_PLAY_SONG(LULLABY);
            hasFormAccess = CAN_BE_GORON && HAS_PLAYABLE_OCARINA;
            break;
        case DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE:
            hasSongAccess = CAN_PLAY_SONG(BOSSA_NOVA);
            hasFormAccess = CAN_BE_ZORA && HAS_PLAYABLE_OCARINA;
            break;
        default:
            break;
    }
    switch (RANDO_SAVE_OPTIONS[RO_ACCESS_DUNGEONS]) {
        case RO_ACCESS_DUNGEONS_FORM_OR_SONG:
            return hasSongAccess || hasFormAccess;
        case RO_ACCESS_DUNGEONS_FORM_ONLY:
            return hasFormAccess;
        case RO_ACCESS_DUNGEONS_SONG_ONLY:
            return hasSongAccess;
        case RO_ACCESS_DUNGEONS_OPEN:
            return true;
        case RO_ACCESS_DUNGEONS_FORM_AND_SONG:
        default:
            return hasSongAccess && hasFormAccess;
    }
}

inline uint32_t MoonMaskCount() {
    uint32_t count = 0;
    for (int i = ITEM_MASK_TRUTH; i <= ITEM_MASK_GIANT; i++) {
        if (INV_CONTENT(i) == i) {
            count++;
        }
    }
    return count;
}

inline uint32_t RemainsCount() {
    uint32_t count = 0;
    for (int i = QUEST_REMAINS_ODOLWA; i <= QUEST_REMAINS_TWINMOLD; i++) {
        if (CHECK_QUEST_ITEM(i)) {
            count++;
        }
    }
    return count;
}

inline bool MeetsMoonRequirements() {
    return RemainsCount() >= RANDO_SAVE_OPTIONS[RO_ACCESS_MOON_REMAINS_COUNT] &&
           MoonMaskCount() >= RANDO_SAVE_OPTIONS[RO_ACCESS_MOON_MASKS_COUNT];
}

// ============================================================================
// CLOCK OWNERSHIP HELPERS
// ============================================================================

inline uint32_t ClockCount() {
    uint32_t count = 0;
    for (int i = 0; i < 6; ++i) {
        if (Flags_GetRandoInf(static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + i))) {
            count++;
        }
    }
    return count;
}

inline bool SettingClocks() {
    return RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE] != 0;
}

// Centralized clock ownership check
inline bool OwnsClockHalfDay(int halfDayIndex) {
    if (halfDayIndex < 0 || halfDayIndex >= 6)
        return false;
    RandoInf clockFlag = static_cast<RandoInf>(RANDO_INF_OBTAINED_CLOCK_DAY_1 + halfDayIndex);
    return Flags_GetRandoInf(clockFlag);
}

// New consolidated helper that encapsulates ascending/descending/random logic
inline bool OwnsHalfDayForMode(int halfDayIndex) {
    if (!SettingClocks() || halfDayIndex < 0 || halfDayIndex >= 6) {
        return !SettingClocks(); // If not shuffling clocks, all time is available
    }

    int clockMode = RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE_PROGRESSIVE];
    uint32_t totalClocks = ClockCount();

    switch (clockMode) {
        case RO_CLOCK_SHUFFLE_RANDOM:
            // Random mode: check if this specific half-day is owned
            return OwnsClockHalfDay(halfDayIndex);

        case RO_CLOCK_SHUFFLE_ASCENDING:
            // Ascending: own first N half-days in sequence (0,1,2,3,4,5)
            return totalClocks > halfDayIndex;

        case RO_CLOCK_SHUFFLE_DESCENDING:
            // Descending: own last N half-days in reverse sequence (5,4,3,2,1,0)
            return totalClocks > (5 - halfDayIndex);

        default:
            return false;
    }
}

// ============================================================================
// TIME OPERATOR FUNCTIONS
// ============================================================================

inline bool RawAt(TimeSlice slice) {
    gCurrentRegionTimeRead = true;
    return (gCurrentRegionTime & (TIME_BIT_ONE << slice)) != 0;
}

inline bool RawBefore(TimeSlice slice) {
    gCurrentRegionTimeRead = true;
    if (slice == 0)
        return false;
    uint64_t mask = (TIME_BIT_ONE << slice) - 1;
    return (gCurrentRegionTime & mask) != 0;
}

inline bool RawAfter(TimeSlice slice) {
    gCurrentRegionTimeRead = true;
    uint64_t mask = ~((TIME_BIT_ONE << slice) - 1) & TIME_ALL_SLICES;
    return (gCurrentRegionTime & mask) != 0;
}

inline bool RawBetween(TimeSlice start, TimeSlice end) {
    gCurrentRegionTimeRead = true;
    uint64_t mask = ((TIME_BIT_ONE << end) - 1) & ~((TIME_BIT_ONE << start) - 1);
    return (gCurrentRegionTime & mask) != 0;
}

// Generate bitmask for a half-day period's time slices
inline constexpr uint64_t GetHalfDayTimeMask(int halfDayIndex) {
    if (halfDayIndex < 0 || halfDayIndex >= 6)
        return 0;

    uint64_t mask = 0;
    const auto& range = HALF_DAY_TIME_RANGES[halfDayIndex];
    for (int slice = range.startSlice; slice <= range.endSlice; ++slice) {
        mask |= (1ULL << slice);
    }
    return mask;
}

// Merge two time states (bitwise OR on time slices)
inline RegionTimeState MergeTimeStates(const RegionTimeState& a, const RegionTimeState& b) {
    if (!a.timeSlices)
        return b;
    if (!b.timeSlices)
        return a;
    RegionTimeState result{ .timeSlices = a.timeSlices | b.timeSlices,
                            .canStayOverTime = a.canStayOverTime || b.canStayOverTime };
#ifdef COMBO_BUILD
    result.regionalFlags = ComboRando::Entrances::Merge(a.regionalFlags, b.regionalFlags);
#endif
    return result;
}

// Check if 'a' covers 'b' (a is a superset - adding b gives nothing new)
inline bool TimeStateCovers(const RegionTimeState& a, const RegionTimeState& b) {
    return (a.timeSlices | b.timeSlices) == a.timeSlices
#ifdef COMBO_BUILD
           && ComboRando::Entrances::Covers(a.regionalFlags, b.regionalFlags)
#endif
        ;
}

// ============================================================================
// CLOCK ITEM MACROS
// ============================================================================

// Simplified clock macros using consolidated helper
#define CLOCK_DAY1() OwnsHalfDayForMode(0)
#define CLOCK_NIGHT1() OwnsHalfDayForMode(1)
#define CLOCK_DAY2() OwnsHalfDayForMode(2)
#define CLOCK_NIGHT2() OwnsHalfDayForMode(3)
#define CLOCK_DAY3() OwnsHalfDayForMode(4)
#define CLOCK_NIGHT3() OwnsHalfDayForMode(5)

// ============================================================================
// CLOCK SHUFFLE VALIDATION FUNCTIONS
// ============================================================================

// Validation: Check if a time slice is in an owned half-day period
inline bool IsTimeSliceOwned(TimeSlice slice) {
    if (!SettingClocks())
        return true;

    for (int i = 0; i < 6; ++i) {
        const auto& range = HALF_DAY_TIME_RANGES[i];
        if (slice >= range.startSlice && slice <= range.endSlice) {
            return OwnsClockHalfDay(i);
        }
    }
    return false;
}

// Validation: Check if any time in the timeslice mask is owned
inline bool HasAnyOwnedTime(uint64_t timeSlices) {
    if (!SettingClocks())
        return true;

    for (int i = 0; i < 6; ++i) {
        if (OwnsClockHalfDay(i)) {
            uint64_t halfDayMask = GetHalfDayTimeMask(i);
            if (timeSlices & halfDayMask) {
                return true;
            }
        }
    }
    return false;
}

// ============================================================================
// COMPOSITE TIME CHECKS
// ============================================================================

#define IS_DAY1() (RawBefore(TIME_NIGHT1_PM_06_00) && CLOCK_DAY1())
#define IS_NIGHT1() (RawBetween(TIME_NIGHT1_PM_06_00, TIME_DAY2_AM_06_00) && CLOCK_NIGHT1())
#define IS_DAY2() (RawBetween(TIME_DAY2_AM_06_00, TIME_NIGHT2_PM_06_00) && CLOCK_DAY2())
#define IS_NIGHT2() (RawBetween(TIME_NIGHT2_PM_06_00, TIME_DAY3_AM_06_00) && CLOCK_NIGHT2())
#define IS_DAY3() (RawBetween(TIME_DAY3_AM_06_00, TIME_NIGHT3_PM_06_00) && CLOCK_DAY3())
#define IS_NIGHT3() (RawAfter(TIME_NIGHT3_PM_06_00) && CLOCK_NIGHT3())

// Global clock filter for owned time periods
// Returns true if Clock Shuffle is disabled OR if player has access to any time period
inline bool ClockFilter() {
    if (!SettingClocks())
        return true;

    return IS_DAY1() || IS_NIGHT1() || IS_DAY2() || IS_NIGHT2() || IS_DAY3() || IS_NIGHT3();
}

#define IS_DAY() (IS_DAY1() || IS_DAY2() || IS_DAY3())
#define IS_NIGHT() (IS_NIGHT1() || IS_NIGHT2() || IS_NIGHT3())
#define FIRST_DAY() (IS_DAY1() || IS_NIGHT1())
#define SECOND_DAY() (IS_DAY2() || IS_NIGHT2())
#define FINAL_DAY() (IS_DAY3() || IS_NIGHT3())

// ============================================================================
// PUBLIC TIME API
// ============================================================================

#define AT(slice) (RawAt(slice) && ClockFilter())

#define BEFORE(slice) (RawBefore(slice) && ClockFilter())

#define AFTER(slice) (RawAfter(slice) && ClockFilter())

#define BETWEEN(s, e) (RawBetween(s, e) && ClockFilter())

#define MIDNIGHT()                                                                                             \
    (BETWEEN(TIME_NIGHT1_AM_12_00, TIME_DAY2_AM_06_00) || BETWEEN(TIME_NIGHT2_AM_12_00, TIME_DAY3_AM_06_00) || \
     AFTER(TIME_NIGHT3_AM_12_00))

#ifdef COMBO_BUILD
inline bool CanDefeatMajora() {
    return ComboRando::CanDefeatMajora(RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] == RO_GENERIC_YES,
                                       Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_MAJORA), CAN_USE_BOW,
                                       CAN_BE_ZORA, CAN_BE_DEITY, HAS_MAGIC, CAN_USE_HUMAN_SWORD,
                                       HAS_ITEM(ITEM_SWORD_GREAT_FAIRY));
}
#endif

inline bool CanKillEnemy(ActorId EnemyId, unsigned actorParams = 0) {
    if (RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS]) {
        const int index = Rando::MissingMmSoulForActor(EnemyId, actorParams);
        if (index >= 0 && !Rando::HaveMissingMmSoul(index))
            return false;
    }
    // If enemy souls are shuffled, and the relevant soul is not obtained, we cannot kill that enemy.
    if (RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] &&
        !(RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS] &&
          (Rando::MissingMmSoulForActor(EnemyId, actorParams) >= 0 || (EnemyId == ACTOR_EN_SW && (actorParams & 3)))) &&
        !HaveEnemySoul(EnemyId)) {
        return false;
    }

#ifdef COMBO_BUILD
    const ComboRando::MmCombatCapabilities combat{ CAN_USE_SWORD,
                                                   CAN_BE_DEKU,
                                                   CAN_BE_GORON,
                                                   CAN_BE_ZORA,
                                                   CAN_BE_DEITY,
                                                   HAS_ITEM(ITEM_MASK_GIANT),
                                                   HAS_MAGIC,
                                                   CAN_USE_BOW,
                                                   HAS_ITEM(ITEM_HOOKSHOT),
                                                   CAN_USE_BOMBS || CAN_USE_BOMBCHUS || HAS_ITEM(ITEM_MASK_BLAST),
                                                   CAN_USE_NUTS,
                                                   CAN_FIGHT_WITH_STICKS,
                                                   CAN_USE_MAGIC_ARROW(FIRE),
                                                   CAN_USE_MAGIC_ARROW(ICE),
                                                   CAN_USE_MAGIC_ARROW(LIGHT),
                                                   GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) >= EQUIP_VALUE_SHIELD_MIRROR,
                                                   CAN_USE_ABILITY(SWIM),
                                                   IS_DAY(),
                                                   COMBO_MM_TRICK("TwinmoldBow") != 0,
                                                   COMBO_MM_TRICK("TwinmoldFireIce") != 0,
                                                   CAN_USE_DINS_FIRE,
                                                   COMBO_MM_TRICK("IgosDins") != 0,
                                                   MM_LOGIC_HAS_IRON_BOOTS,
                                                   MM_LOGIC_HAS_ZORA_TUNIC_STRICT,
                                                   COMBO_MM_TRICK("GyorgIrons") != 0 };
    // Expressions are imported from OoTMM. Keep actor-specific soul gates here.
    switch (EnemyId) {
        case ACTOR_BOSS_01:
            return ComboRando::can_kill_odolwa(combat) && (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] ||
                                                           Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_ODOLWA));
        case ACTOR_BOSS_02:
            return ComboRando::can_kill_twinmold(combat) &&
                   (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] ||
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_TWINMOLD));
        case ACTOR_BOSS_03:
            return ComboRando::can_kill_gyorg(combat) && (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] ||
                                                          Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_GYORG));
        case ACTOR_BOSS_HAKUGIN:
            return ComboRando::can_kill_goht(combat) && (!RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] ||
                                                         Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT));
        // Remaining imported actor cases are generated below.
        case ACTOR_BOSS_04:
            return ComboRando::can_kill_warts(combat);
        case ACTOR_EN_KNIGHT:
            return ComboRando::can_kill_igos(combat);
        case ACTOR_EN_KAIZOKU:
            return ComboRando::can_fight_pirates(combat);
        case ACTOR_EN_PAMETFROG:
            return ComboRando::can_kill_gekko_wft(combat);
        case ACTOR_EN_BIGSLIME:
            return ComboRando::can_kill_gekko_gbt(combat);
        case ACTOR_EN_DINOFOS:
            return ComboRando::can_kill_lizalfos_dinolfos(combat);
        case ACTOR_EN_WIZ:
            return ComboRando::can_kill_wizzrobe_close(combat);
        case ACTOR_EN_WF:
            return ComboRando::can_kill_wolfos(combat);
        case ACTOR_EN_JSO:
            return ComboRando::can_kill_garo(combat);
        case ACTOR_EN_JSO2:
            return ComboRando::can_kill_garo_master(combat);
        case ACTOR_EN_GRASSHOPPER:
            return ComboRando::can_kill_dragonflies(combat);
        case ACTOR_EN_MKK:
            return ComboRando::can_kill_boes(combat);
        case ACTOR_EN_BIGPAMET:
            return ComboRando::can_kill_snappers_flowers(combat);
        case ACTOR_EN_ST:
            return ComboRando::can_kill_skulltulas(combat);
        case ACTOR_EN_BAT:
            return ComboRando::can_kill_bad_bats_single(combat);
        case ACTOR_EN_DEKUBABA:
            return ComboRando::can_kill_baba_nuts(combat);
        case ACTOR_EN_AM:
            return ComboRando::can_kill_armos(combat);
        case ACTOR_EN_VM:
            return ComboRando::can_kill_beamos(combat);
        case ACTOR_EN_BB:
            return ComboRando::can_kill_bubbles(combat);
        case ACTOR_EN_BBFALL:
            return ComboRando::can_kill_bubbles(combat);
        case ACTOR_EN_FAMOS:
            return ComboRando::can_kill_eyegore_distance(combat);
        case ACTOR_EN_DODONGO:
            return ComboRando::can_kill_dodongos(combat);
        case ACTOR_EN_FLOORMAS:
            return ComboRando::can_kill_floormasters(combat);
        case ACTOR_EN_WALLMAS:
            return ComboRando::can_kill_wallmasters(combat);
        case ACTOR_EN_FZ:
            return ComboRando::can_kill_freezard_short_range(combat);
        case ACTOR_EN_CROW:
            return ComboRando::can_kill_guays(combat);
        case ACTOR_EN_FIREFLY:
            return ComboRando::can_kill_keese_close(combat);
        case ACTOR_EN_RR:
            return ComboRando::can_kill_like_like(combat);
        case ACTOR_EN_DEKUNUTS:
            return ComboRando::can_kill_scrubs_anywhere(combat);
        case ACTOR_EN_PEEHAT:
            return ComboRando::can_kill_peahats(combat);
        case ACTOR_EN_BSB:
            return ComboRando::can_kill_keeta(combat);
        case ACTOR_EN_SKB:
            return ComboRando::can_kill_stalchildren(combat);
        case ACTOR_EN_TITE:
            return ComboRando::can_kill_tektites(combat);
        case ACTOR_EN_SLIME:
            return ComboRando::can_kill_chuchu(combat);
        case ACTOR_EN_WDHAND:
            return ComboRando::can_kill_dexihands(combat);
        case ACTOR_EN_KAME:
            return ComboRando::can_kill_snappers_any(combat);
        case ACTOR_EN_OKUTA:
            return ComboRando::can_kill_octoroks(combat);
        case ACTOR_EN_EGOL:
            return ComboRando::can_kill_eyegore(combat);
        case ACTOR_EN_NEO_REEBA:
            return ComboRando::can_kill_leevers(combat);
        case ACTOR_EN_SB:
            return ComboRando::can_kill_shell_blades_underwater(combat);
        case ACTOR_EN_PR2: {
            // can_kill_skullfish (macros_mm.yml:364) adds its Iron Boots route to the native rule below, which keeps
            // its magic and Swim requirements for the Zora route.
            auto ironBoots = combat;
            ironBoots.zora = false;
            if (ComboRando::can_kill_skullfish(ironBoots))
                return true;
            break;
        }
        case ACTOR_BOSS_05:
            return ComboRando::can_kill_bio_babas(combat);
        case ACTOR_EN_DRAGON:
            return ComboRando::can_kill_deep_python(combat);
        case ACTOR_EN_BIGPO:
            return ComboRando::can_kill_big_poe(combat);
        case ACTOR_EN_PO_SISTERS:
            return ComboRando::can_kill_poe_sisters(combat);
        case ACTOR_EN_INVADEPOH:
            return ComboRando::can_kill_aliens(combat);
        case ACTOR_EN_DEATH:
            return ComboRando::can_kill_gomess(combat);
        default:
            break;
    }
#endif

    switch (EnemyId) {
        case ACTOR_BOSS_01: // Odolwa
            return (CAN_USE_SWORD || CAN_BE_DEITY || CAN_BE_GORON || CAN_BE_ZORA || CAN_USE_EXPLOSIVE ||
                    CAN_USE_MAGIC_ARROW(FIRE) || CAN_USE_MAGIC_ARROW(LIGHT)) &&
                   (Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_ODOLWA) ||
                    RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] == RO_GENERIC_NO);
        case ACTOR_BOSS_02: // Twinmold
            return (CAN_USE_BOW || (HAS_ITEM(ITEM_MASK_GIANT) && HAS_MAGIC && CAN_USE_HUMAN_SWORD)) &&
                   (Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_TWINMOLD) ||
                    RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] == RO_GENERIC_NO);
        case ACTOR_BOSS_03: // Gyorg
            return ((CAN_BE_DEITY && HAS_MAGIC) || (CAN_BE_ZORA && HAS_MAGIC)) &&
                   (Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_GYORG) ||
                    RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] == RO_GENERIC_NO);
        case ACTOR_BOSS_04: // Wart
            return (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || CAN_BE_ZORA);
        case ACTOR_BOSS_HAKUGIN: // Goht
            return (CAN_USE_MAGIC_ARROW(FIRE)) && (Flags_GetRandoInf(RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT) ||
                                                   RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS] == RO_GENERIC_NO);
        case ACTOR_EN_KNIGHT: // Igos du Ikana/IdI Lackey
            return (CAN_USE_MAGIC_ARROW(FIRE) &&
                    (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) >= EQUIP_VALUE_SHIELD_MIRROR) &&
                    (CAN_USE_SWORD || CAN_BE_DEKU || CAN_BE_GORON || CAN_BE_ZORA));
        case ACTOR_EN_KAIZOKU: // Fighter Pirate
            return (CAN_USE_SWORD || CAN_BE_ZORA);
        case ACTOR_EN_PAMETFROG: // Woodfall Temple Gekko (and Snapper)
            return (CAN_USE_BOW && CanKillEnemy(ACTOR_EN_BIGPAMET));
        case ACTOR_EN_BIGSLIME: // Great Bay Temple Gekko
            return (CAN_USE_MAGIC_ARROW(ICE));
        case ACTOR_EN_SW: // Gold Skulltula & Skullwalltula
            return (CAN_USE_PROJECTILE || CAN_BE_DEKU || CAN_BE_GORON || CAN_USE_SWORD || CAN_USE_EXPLOSIVE);
        case ACTOR_EN_DINOFOS: // Dinolfos
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_USE_BOW || (CAN_BE_DEKU && HAS_MAGIC));
        case ACTOR_EN_WIZ: // Wizrobe
            return (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_SWORD || CAN_BE_GORON);
        case ACTOR_EN_WF: // Wolfos
            return (CAN_USE_SWORD || (CAN_BE_DEKU && HAS_MAGIC) || CAN_BE_GORON || CAN_BE_ZORA);
        case ACTOR_EN_JSO: // Garo
            return (HAS_ITEM(ITEM_MASK_GARO) && (CAN_USE_BOW || CAN_BE_GORON || CAN_USE_SWORD));
        case ACTOR_EN_JSO2: // Garo Master
            return (CAN_USE_BOW || CAN_BE_GORON || CAN_USE_SWORD);
        case ACTOR_EN_IK: // Iron Knuckle
            return (CAN_USE_SWORD || CAN_BE_GORON);
        case ACTOR_EN_GRASSHOPPER: // Dragonfly
            return ((CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_EXPLOSIVE || CAN_USE_NUTS || CAN_USE_SWORD || CAN_BE_ZORA);
        case ACTOR_EN_MKK: // Boe
            return ((CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_EXPLOSIVE || CAN_USE_NUTS || CAN_USE_SWORD || CAN_BE_ZORA ||
                    CAN_BE_GORON);
        case ACTOR_EN_BIGPAMET: // Snapper
            return (CAN_BE_DEKU || CAN_USE_EXPLOSIVE || CAN_BE_GORON);
        case ACTOR_EN_ST: // Large Skulltula
            return (CAN_USE_SWORD || CAN_USE_PROJECTILE || CAN_BE_GORON || CAN_USE_EXPLOSIVE);
        case ACTOR_EN_BAT: // Bad Bat
            return (CAN_USE_SWORD || HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_BOW || CAN_USE_EXPLOSIVE || CAN_BE_GORON ||
                    CAN_BE_ZORA);
        case ACTOR_EN_DEKUBABA: // Neck bending Deku Baba
            return (CAN_USE_SWORD || CAN_BE_DEKU || CAN_BE_GORON || CAN_BE_ZORA || CAN_USE_BOW || CAN_USE_EXPLOSIVE ||
                    CAN_USE_STICKS);
        case ACTOR_OBJ_SNOWBALL: // Large Snowball: OoTMM macros_mm.yml:183 can_break_snowballs
            return (CAN_USE_EXPLOSIVE || CAN_BE_GORON || CAN_USE_FIRE_SHORT_RANGE);
        case ACTOR_EN_AM: // Armos
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || CAN_USE_EXPLOSIVE);
        case ACTOR_EN_VM: // Beamos
            return (CAN_USE_EXPLOSIVE);
        case ACTOR_EN_BB:     // Blue Bubble
        case ACTOR_EN_BBFALL: // Red Bubble
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_USE_BOW || CAN_USE_STICKS);
        case ACTOR_EN_RAT:       // Real Bombchu
        case ACTOR_EN_TUBO_TRAP: // Flying Pot
            return true;
        case ACTOR_EN_FAMOS: // Death Armos
            return (CAN_USE_MAGIC_ARROW(LIGHT));
        case ACTOR_EN_DODONGO: // Dodongo
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC));
        case ACTOR_EN_FLOORMAS: // Floormaster
        case ACTOR_EN_WALLMAS:  // Wallmaster
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_BOW ||
                    CAN_USE_STICKS);
        case ACTOR_EN_FZ: // Freezard
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_USE_MAGIC_ARROW(FIRE) ||
                    HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_CROW: // Guay (Generic, excludes the one circling Clock Town En_Ruppecrow)
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || CAN_USE_BOW || CAN_USE_STICKS ||
                    HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_FIREFLY: // Keese
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || CAN_USE_BOW || CAN_USE_STICKS ||
                    HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_RR: // Like Like
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_BOW ||
                    CAN_USE_STICKS);
        case ACTOR_EN_DEKUNUTS: // Mad Scrub
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || CAN_USE_BOW || CAN_USE_STICKS ||
                    HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_KAREBABA: // Wilted/Mini Babas
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU);
        case ACTOR_EN_PEEHAT: // Peahat
            return (
                (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || CAN_USE_BOW || CAN_FIGHT_WITH_STICKS) &&
                IS_DAY());
        case ACTOR_EN_RD: // Redead & Gibdos
            return (CAN_USE_SWORD || CAN_BE_DEKU || CAN_BE_GORON || CAN_BE_ZORA || CAN_USE_STICKS);
        case ACTOR_EN_BSB: // Captain Keeta (May be possible without bow, but the window is tight. Requiring for now)
            return (CAN_USE_BOW &&
                    (CAN_USE_SWORD || CAN_USE_STICKS || CAN_USE_EXPLOSIVE || CAN_BE_GORON || CAN_BE_ZORA));
        case ACTOR_EN_SKB: // Stalchild
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_BOW ||
                    CAN_USE_STICKS || HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_TITE: // Tektite
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_BOW);
        case ACTOR_EN_SLIME: // Chuchus
            return (CAN_USE_SWORD || CAN_BE_ZORA || CAN_BE_DEKU || CAN_USE_BOW || CAN_USE_STICKS);
        case ACTOR_EN_SNOWMAN: // Eeno
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_BOW ||
                    CAN_USE_STICKS || HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_WDHAND: // Dexihand (Basic kill method, seems like a pain to require other things)
            return (CAN_BE_ZORA && HAS_MAGIC);
        case ACTOR_EN_KAME: // Snapper (non Gekko Miniboss)
            return (CAN_USE_EXPLOSIVE || CAN_BE_GORON);
        case ACTOR_EN_OKUTA: // Octorok
        case ACTOR_EN_EGOL:  // Eyegore
            return (CAN_USE_PROJECTILE);
        case ACTOR_EN_BAGUO: // Nejiron
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_NEO_REEBA: // Leever
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_BOW ||
                    CAN_USE_STICKS);
        case ACTOR_EN_PP: // Hiploop
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || (CAN_BE_DEKU && HAS_MAGIC) || CAN_USE_BOW ||
                    CAN_USE_STICKS || HAS_ITEM(ITEM_HOOKSHOT));
        case ACTOR_EN_PR:  // Desbreko
        case ACTOR_EN_PR2: // Skull fish
        case ACTOR_EN_SB:  // Shellblade
            return (CAN_BE_ZORA && HAS_MAGIC && CAN_USE_ABILITY(SWIM));
        case ACTOR_BOSS_05: // Bio Deku Baba
            return CAN_BE_ZORA && CAN_USE_ABILITY(SWIM);
        case ACTOR_EN_BEE: // Giant Bee
            return (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || CAN_USE_STICKS ||
                    CAN_USE_PROJECTILE || CAN_USE_EXPLOSIVE || CAN_USE_NUTS);
        case ACTOR_EN_DRAGON: // Deep Python
            return (CAN_BE_ZORA && HAS_MAGIC);
        case ACTOR_EN_BIGPO:
            return CAN_USE_BOW;
        case ACTOR_EN_PO_SISTERS:
            // The first three sisters can be damaged with almost anything, but Meg requires ranged attacks. Not using
            // CAN_USE_EXPLOSIVE here, as the Blast Mask cannot reach, and the Powder Keg can only be used once.
            return (CAN_USE_PROJECTILE || CAN_USE_BOMBS || CAN_USE_BOMBCHUS);
        case ACTOR_EN_INVADEPOH: // Them
            return CAN_USE_BOW;
        case ACTOR_EN_THIEFBIRD: // Takkuri
            return (CAN_USE_PROJECTILE || CAN_BE_GORON || CAN_BE_ZORA || CAN_USE_EXPLOSIVE || CAN_USE_SWORD);
        case ACTOR_EN_DEATH: // Gomess
            return (CAN_USE_SWORD || CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || CAN_BE_ZORA || CAN_BE_GORON ||
                    (CAN_BE_DEKU && HAS_MAGIC));
        default: // Incorrect actor ID inputed.
            assert(false);
            return false;
    }
}

} // namespace Logic

} // namespace Rando

#endif // RANDO_LOGIC_H
