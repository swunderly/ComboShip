#include "2s2h/GameInteractor/GameInteractor.h"
#ifdef COMBO_BUILD
#include "rando/entrances/MmOrdinaryGraph.h"
#include "rando/entrances/MmGrottoGraph.h"
#include "rando/entrances/MmMixedGraph.h"
#endif
#include "2s2h/ShipInit.hpp"

#include "Logic.h"
#ifdef COMBO_BUILD
#include "2s2h/Rando/ActorBehavior/RenewableStock.h"
#include "2s2h/Rando/ActorBehavior/BeanSources.h"
#endif
#ifdef COMBO_BUILD
#include "ComboEntrancesMM.h"
#endif
#ifdef COMBO_BUILD
#include "rando/SongTravelMM.h"
extern "C" uint32_t MM_LiveIncomingSongs();
#endif

namespace Rando {

namespace Logic {

std::map<RandoRegionId, RandoRegion> Regions = {};

uint64_t gCurrentRegionTime = 0;
#ifdef COMBO_BUILD
inline bool TypedMmGrottoSourceActive(uint32_t id);
inline bool TypedMmSwampActive();
bool NativeMmErContextsEnabled() {
    return ComboRando::Entrances::ActiveMmMixed().active() ||
           ComboRando::Entrances::ActiveMmMixedAssumptions().has_value() ||
           ComboRando::Entrances::ActiveMmGrottoAssumptions().has_value() ||
           ComboRando::Entrances::ActiveMmGrottos().active() || ComboRando::Entrances::ActiveMmOrdinary().active() ||
           ComboRando::Entrances::ActiveMmOrdinaryAssumptions().has_value();
}
#endif

#ifdef COMBO_BUILD
namespace {
uint32_t RefillResource(RandoItemId item) {
    using namespace ComboRando;
    switch (item) {
        case RI_ARROWS_10:
        case RI_ARROWS_30:
        case RI_ARROWS_50:
            return MmArrows;
        case RI_BOMBS_5:
        case RI_BOMBS_10:
            return MmBombs;
        case RI_BOMBCHU:
        case RI_BOMBCHU_5:
        case RI_BOMBCHU_10:
            return MmBombchus;
        case RI_DEKU_STICK:
        case RI_DEKU_STICKS_5:
            return MmSticks;
        case RI_DEKU_NUT:
        case RI_DEKU_NUTS_5:
        case RI_DEKU_NUTS_10:
            return MmNuts;
        case RI_MAGIC_JAR_SMALL:
        case RI_MAGIC_JAR_BIG:
            return MmMagic;
        case RI_GREEN_POTION_REFILL:
        case RI_BLUE_POTION_REFILL:
        case RI_CHATEAU_ROMANI_REFILL:
            return HAS_BOTTLE ? MmMagic : 0;
        default:
            return 0; // One-time equipment and foreign deliveries are not refills.
    }
}

uint32_t OutdoorResources(RandoRegionId region) {
    using namespace ComboRando;
    uint32_t result = 0;
    // Pinned OoTMM v32.3 world/mm/overworld. These sources dominate the other
    // MM refill events while ordinary MM entrances remain unshuffled.
    switch (region) {
        case RR_TERMINA_FIELD:
            result = MmRupees | MmArrows | MmBombs;
            if (HaveEnemySoul(ACTOR_EN_SLIME))
                result |= MmMagic;
            if (HaveEnemySoul(ACTOR_EN_DEKUBABA) && (CAN_USE_SWORD || CAN_BE_DEKU))
                result |= MmSticks;
            break;
        case RR_SOUTHERN_SWAMP_NORTH:
            result = MmRupees | MmArrows | MmBombs | MmMagic | MmSticks;
            break;
        case RR_MM_GROTTO_TWIN_RAMP:
            result = MmMagic;
            break;
        case RR_CLOCK_TOWN_NORTH:
            result = MmRupees;
            break;
        case RR_CLOCK_TOWN_LAUNDRY:
            result = MmMagic;
            break;
        case RR_WOODFALL_OWL_STATUE_PLATFORM:
            result = MmSticks | MmNuts;
            break;
        default:
            return 0;
    }
    if ((region == RR_TERMINA_FIELD || region == RR_SOUTHERN_SWAMP_NORTH) && HaveEnemySoul(ACTOR_EN_DEKUBABA) &&
        (CAN_USE_SWORD || CAN_BE_GORON || CAN_BE_ZORA || CAN_BE_DEKU || HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_EXPLOSIVE ||
         CAN_USE_BOW)) {
        result |= MmNuts;
    }
    return result;
}
} // namespace

bool UpdateResourceSources(const std::set<RandoRegionId>& reached,
                           std::unordered_map<RandoRegionId, RegionTimeState>& times) {
    bool changed = false;
    struct RestoreTime {
        uint64_t previous = gCurrentRegionTime;
        ComboRando::Entrances::MmFlagDomain previousFlags = ComboRando::Entrances::mmFlagSource;
        ~RestoreTime() {
            gCurrentRegionTime = previous;
            ComboRando::Entrances::mmFlagSource = previousFlags;
        }
    } restoreTime;
    for (auto id : reached) {
        const auto time = times.find(id);
        const auto region = Regions.find(id);
        if (time == times.end() || !time->second.timeSlices || region == Regions.end())
            continue;
        changed |= ComboRando::mmResourceAccess.Add(OutdoorResources(id));
        gCurrentRegionTime = time->second.timeSlices;
        ComboRando::Entrances::mmFlagSource = time->second.regionalFlags;
        // OoTMM distinguishes northern hot springs from water at the Well.
        // A distant bottle cools before reaching the mountains without Soaring.
        if ((id == RR_GORON_GRAVEYARD && CAN_BE_GORON) || id == RR_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO) {
            changed |= ComboRando::mmResourceAccess.Add(ComboRando::MmHotWaterNorth);
        }
        for (const auto& [check, predicate] : region->second.checks) {
            const auto definition = Rando::StaticData::Checks.find(check);
            if (definition == Rando::StaticData::Checks.end())
                continue;
            // locations.ts: these three MM Business Scrub shops are renewable;
            // Bomb Bag/All-Night one-time shelves are explicitly excluded.
            const bool scrub = check == RC_SOUTHERN_SWAMP_SCRUB_BEANS || check == RC_ZORA_HALL_SCRUB_POTION_REFILL ||
                               check == RC_IKANA_CANYON_SCRUB_POTION_REFILL;
            if (!Rando::BeanSourceForCheck(check, definition->second.randoCheckType).renewable)
                continue;
            const auto stock = Rando::RenewableStockItem(RANDO_SAVE_CHECKS[check].randoItemId);
            const auto refill = RefillResource(stock);
            // Renewable bottle contents are also progression resources. A
            // reachable Milk Bar shelf can supply the Well's milk trade.
            const bool milk = HAS_BOTTLE && stock == RI_MILK_REFILL;
            const bool chateau = HAS_BOTTLE && stock == RI_CHATEAU_ROMANI_REFILL;
            const bool red = HAS_BOTTLE && stock == RI_RED_POTION_REFILL;
            const bool green = HAS_BOTTLE && stock == RI_GREEN_POTION_REFILL;
            const bool blue = HAS_BOTTLE && stock == RI_BLUE_POTION_REFILL;
            const bool beans = INV_CONTENT(ITEM_MAGIC_BEANS) == ITEM_MAGIC_BEANS && stock == RI_MAGIC_BEAN;
            if ((!refill || ComboRando::mmResourceAccess.Has(static_cast<ComboRando::MmResource>(refill))) &&
                (!milk || RANDO_EVENTS[RE_ACCESS_MILK_REFILL]) && (!red || RANDO_EVENTS[RE_ACCESS_RED_POTION_REFILL]) &&
                (!green || RANDO_EVENTS[RE_ACCESS_GREEN_POTION_REFILL]) &&
                (!blue || RANDO_EVENTS[RE_ACCESS_BLUE_POTION_REFILL]) &&
                (!beans || RANDO_EVENTS[RE_ACCESS_BEANS_REFILL]) &&
                (!chateau || ComboRando::mmResourceAccess.Has(ComboRando::MmChateau)))
                continue;
            // Business Scrub runtime prices are fixed (10/40/100), independent
            // of the unrelated random price field on their NPC checks.
            if (scrub && !CAN_PAY(check == RC_SOUTHERN_SWAMP_SCRUB_BEANS      ? 10
                                  : check == RC_ZORA_HALL_SCRUB_POTION_REFILL ? 40
                                                                              : 100))
                continue;
            // Vanilla refill shelves are paid even though RandoSaveCheck.price
            // is zero for unshuffled stock. All use the base wallet except
            // Chateau (200). Shuffled shelves use their rolled price instead.
            if (definition->second.randoCheckType == RCTYPE_SHOP && !RANDO_SAVE_CHECKS[check].shuffled &&
                !CAN_PAY(check == RC_MILK_BAR_PURCHASE_CHATEAU ? 200 : 1))
                continue;
            // Old pool dumps also rolled unused prices for vanilla shelves.
            // Native purchases ignore those prices; evaluate the shelf with
            // its real wallet gate above, then restore the save unchanged.
            auto& checkSave = RANDO_SAVE_CHECKS[check];
            struct RestorePrice {
                decltype(checkSave.price)& destination;
                decltype(checkSave.price) previous;
                ~RestorePrice() {
                    destination = previous;
                }
            } restorePrice{ checkSave.price, checkSave.price };
            if (!RANDO_SAVE_CHECKS[check].shuffled)
                RANDO_SAVE_CHECKS[check].price = 0;
            const bool available = TimeLogic::EvaluateConditionTimes(predicate.first, gCurrentRegionTime);
            if (available) {
                changed |= ComboRando::mmResourceAccess.Add(refill);
                if (chateau)
                    changed |= ComboRando::mmResourceAccess.Add(ComboRando::MmChateau);
                if (red && !RANDO_EVENTS[RE_ACCESS_RED_POTION_REFILL]) {
                    RANDO_EVENTS[RE_ACCESS_RED_POTION_REFILL] = true;
                    changed = true;
                }
                if (green && !RANDO_EVENTS[RE_ACCESS_GREEN_POTION_REFILL]) {
                    RANDO_EVENTS[RE_ACCESS_GREEN_POTION_REFILL] = true;
                    changed = true;
                }
                if (blue && !RANDO_EVENTS[RE_ACCESS_BLUE_POTION_REFILL]) {
                    RANDO_EVENTS[RE_ACCESS_BLUE_POTION_REFILL] = true;
                    changed = true;
                }
                if (beans && !RANDO_EVENTS[RE_ACCESS_BEANS_REFILL]) {
                    RANDO_EVENTS[RE_ACCESS_BEANS_REFILL] = true;
                    changed = true;
                }
                if (milk && !RANDO_EVENTS[RE_ACCESS_MILK_REFILL]) {
                    RANDO_EVENTS[RE_ACCESS_MILK_REFILL] = true;
                    changed = true;
                }
            }
        }
    }
    return changed;
}
#endif

#ifdef COMBO_BUILD
namespace {
thread_local uint64_t mmAccessIngress = 0;
thread_local bool mmAccessEvent = false;
thread_local bool mmAccessChanged = false;

bool NativeMmCanResetTimeImpl() {
    // macros_mm.yml: moonCrash=cycle OR can_play_time. Native MoonCrashSave
    // guarantees SaveEndOfCycle for glitchless rando even with its CVar off.
    // Enhancement-only changes are usable by the live tracker, never assumed
    // by generation. MM_MAJORA_LOGIC is unsupported and is not invented here.
    const bool cycleSave = (IS_RANDO && RANDO_SAVE_OPTIONS[RO_LOGIC] == RO_LOGIC_GLITCHLESS) ||
                           (evaluatingLiveTracker && CVarGetInteger("gEnhancements.Cycle.SaveOnMoonCrash", 0));
    return cycleSave || CAN_PLAY_SONG(TIME);
}
} // namespace

void ResetMmAccess() {
    mmAccessIngress = 0;
    mmAccessEvent = false;
    mmAccessChanged = false;
}

bool RecordMmAccess(uint64_t timeMask) {
    mmAccessIngress |= timeMask & TIME_ALL_SLICES;
    if (!mmAccessEvent && mmAccessIngress && NativeMmCanResetTimeImpl()) {
        mmAccessEvent = true;
        mmAccessChanged = true;
        return true;
    }
    return false;
}

bool UpdateMmAccess() {
    RecordMmAccess(0); // A newly obtained song/instrument can satisfy old ingress.
    const bool changed = mmAccessChanged;
    mmAccessChanged = false;
    return changed;
}

bool NativeMmCanResetTime() {
    return mmAccessEvent;
}
bool NativeMmCanResetTimeDungeon() {
    // can_reset_time_dungeon reduces to ACCESS while erMoon is unsupported.
    // Do not assume upstream Farore, autoInvert, or Majora-in-logic options.
    return NativeMmCanResetTime();
}

MmAccessScope::MmAccessScope()
    : previousIngress(mmAccessIngress), previousAccess(mmAccessEvent), previousChanged(mmAccessChanged) {
    ResetMmAccess();
}
MmAccessScope::~MmAccessScope() {
    mmAccessIngress = previousIngress;
    mmAccessEvent = previousAccess;
    mmAccessChanged = previousChanged;
}

bool NativeMmIngressNeedsDungeonReset(s32 entrance) {
    // These native ingress nodes merge an empty OoTMM dungeon wrapper with
    // its interior. Wallet/Mask-of-Truth and Well/Ikana vestibules instead
    // keep the source gate on their interior connection in the region files.
    switch (entrance) {
        case ENTRANCE(WOODFALL_TEMPLE, 0):
        case ENTRANCE(SNOWHEAD_TEMPLE, 0):
        case ENTRANCE(GREAT_BAY_TEMPLE, 0):
        case ENTRANCE(STONE_TOWER_TEMPLE, 0):
        case ENTRANCE(STONE_TOWER_TEMPLE_INVERTED, 0):
        case ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0):
        case ENTRANCE(SECRET_SHRINE, 0):
            return true;
        default:
            return false;
    }
}

bool NativeMmIngressNeedsReset(s32 entrance) {
    // Source Owl areas gate the step away from their landing. Clock Town's
    // landing is split explicitly, allowing the platform/roof before ACCESS.
    switch (entrance) {
        case ENTRANCE(SOUTHERN_SWAMP_POISONED, 10):
        case ENTRANCE(WOODFALL, 4):
        case ENTRANCE(MILK_ROAD, 4):
        case ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8):
        case ENTRANCE(SNOWHEAD, 3):
        case ENTRANCE(GREAT_BAY_COAST, 11):
        case ENTRANCE(ZORA_CAPE, 6):
        case ENTRANCE(IKANA_CANYON, 4):
        case ENTRANCE(STONE_TOWER, 3):
            return true;
        default:
            return false;
    }
}

uint64_t FilterNativeMmIngressTimes(s32 entrance, uint64_t arrivalTimes) {
    if (!arrivalTimes)
        return 0;
    if (NativeMmIngressNeedsDungeonReset(entrance) && !NativeMmCanResetTimeDungeon())
        return 0;
    if (NativeMmIngressNeedsReset(entrance) && !NativeMmCanResetTime())
        return 0;
    return arrivalTimes;
}
#endif

// GROTTOS
// grottos are a bit weird, 15 of them share entrances with other grottos, so we have to check an extra field to
// determine which grotto we're in, and convert the entrance ID to an unused one that we store for logic purposes.
// ENTRANCE(GROTTOS, 17) = RR_TERMINA_FIELD_COW_GROTTO
// ENTRANCE(GROTTOS, 18) = RR_TERMINA_FIELD_PILLAR_GROTTO
// ENTRANCE(GROTTOS, 19) = RR_TERMINA_FIELD_TALL_GRASS_GROTTO
// ENTRANCE(GROTTOS, 20) = RR_ROAD_TO_SOUTHERN_SWAMP_GROTTO
// ENTRANCE(GROTTOS, 21) = RR_SOUTHERN_SWAMP_GROTTO
// ENTRANCE(GROTTOS, 22) = RR_WOODS_OF_MYSTERY_GROTTO
// ENTRANCE(GROTTOS, 23) = RR_MOUNTAIN_VILLAGE_TUNNEL_GROTTO
// ENTRANCE(GROTTOS, 24) = RR_PATH_TO_GORON_VILLAGE_RAMP_GROTTO
// ENTRANCE(GROTTOS, 25) = RR_PATH_TO_SNOWHEAD_GROTTO
// ENTRANCE(GROTTOS, 26) = RR_GREAT_BAY_COAST_FISHERMAN_GROTTO
// ENTRANCE(GROTTOS, 27) = RR_GREAT_BAY_COAST_COW_GROTTO
// ENTRANCE(GROTTOS, 28) = RR_ZORA_CAPE_GROTTO
// ENTRANCE(GROTTOS, 29) = RR_IKANA_CANYON_GROTTO
// ENTRANCE(GROTTOS, 30) = RR_IKANA_GRAVEYARD_GROTTO
// ENTRANCE(GROTTOS, 31) = RR_ROAD_TO_IKANA_GROTTO

#ifdef COMBO_BUILD
bool HasUsableBottle() {
    for (int slot = SLOT_BOTTLE_1; slot <= SLOT_BOTTLE_6; ++slot) {
        const auto item = gSaveContext.save.saveInfo.inventory.items[slot];
        if (item == ITEM_NONE)
            continue;
        if (item != ITEM_GOLD_DUST)
            return true;
        // Gold Dust is an exchange item and cannot simply be poured out.
        // OoTMM macros_common.yml: has_bottle_gold / GOLD_DUST_USED.
        // This engine also empties Gold Dust on a playable cycle reset
        // (z_sram_NES.c), unlike OoTMM, which deliberately preserves it.
        if (CAN_PLAY_SONG(TIME) || (CAN_ACCESS(SMITHY) && GET_CUR_UPG_VALUE(UPG_WALLET) >= 1))
            return true;
    }
    return false;
}
#endif

RandoRegionId GetRegionIdFromEntrance(s32 entrance) {
#ifdef COMBO_BUILD
    if (TypedMmGrottoSourceActive(0x14c0u) && entrance == ENTRANCE(DEKU_PALACE, 9))
        return RR_MM_GROTTO_BEANS_BANK;
    if (TypedMmGrottoSourceActive(65542u) && entrance == ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 2))
        return RR_MM_TWIN_NEAR_RACE;
    // Graph-only context nodes must not overwrite the immutable vanilla cache.
    // Ordinary saved arrivals have the same exact outside area as the pin.
    if (TypedMmSwampActive()) {
        if (entrance == ENTRANCE(SOUTHERN_SWAMP_POISONED, 8))
            return RR_MM_SWAMP_NEAR_SPIDER;
        if (entrance == ENTRANCE(SOUTHERN_SWAMP_POISONED, 4))
            return RR_MM_SWAMP_CANOPY_FRONT;
    }
    if (TypedMmGrottoSourceActive(65548u) && entrance == ENTRANCE(IKANA_CANYON, 12))
        return RR_MM_GROTTO_IKANA_SHRINE_BANK;
    // The pinned Stone Tower graph leaves the temple at its front; the legacy graph at its top.
    if (MM_LOGIC_STONE_TOWER_PINNED && entrance == ENTRANCE(STONE_TOWER, 2))
        return RR_MM_STONE_TOWER_FRONT_OF_TEMPLE;
#endif

    static std::map<s32, RandoRegionId> entranceToRegionId;
    if (entranceToRegionId.empty()) {
        for (auto& [randoRegionId, randoRegion] : Regions) {
#ifdef COMBO_BUILD
            if (randoRegionId >= RR_MM_GROTTO_WOODS_BANK || randoRegionId == RR_MM_STONE_TOWER_FRONT_OF_TEMPLE)
                continue;
#endif
            for (auto& [_, regionExit] : randoRegion.exits) {
                if (regionExit.returnEntrance == ONE_WAY_EXIT) {
                    continue;
                }
                entranceToRegionId[regionExit.returnEntrance] = randoRegionId;
            }
            for (auto& entrance : randoRegion.oneWayEntrances) {
                entranceToRegionId[entrance] = randoRegionId;
            }
        }
    }

    s32 modifiedEntrance = entrance;
    if (entrance == ENTRANCE(GROTTOS, 4)) {
        switch (gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance) {
            case ENTRANCE(TERMINA_FIELD, 0):
                modifiedEntrance = gSaveContext.respawn[RESPAWN_MODE_UNK_3].data == 0x3F ? ENTRANCE(GROTTOS, 19)
                                                                                         : ENTRANCE(GROTTOS, 18);
                break;
            case ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 20);
                break;
            case ENTRANCE(SOUTHERN_SWAMP_POISONED, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 21);
                break;
            case ENTRANCE(WOODS_OF_MYSTERY, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 22);
                break;
            case ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 23);
                break;
            case ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 24);
                break;
            case ENTRANCE(PATH_TO_SNOWHEAD, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 25);
                break;
            case ENTRANCE(GREAT_BAY_COAST, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 26);
                break;
            case ENTRANCE(ZORA_CAPE, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 28);
                break;
            case ENTRANCE(IKANA_CANYON, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 29);
                break;
            case ENTRANCE(IKANA_GRAVEYARD, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 30);
                break;
            case ENTRANCE(ROAD_TO_IKANA, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 31);
                break;
        }
    } else if (entrance == ENTRANCE(GROTTOS, 10)) {
        switch (gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance) {
            case ENTRANCE(TERMINA_FIELD, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 17);
                break;
            case ENTRANCE(GREAT_BAY_COAST, 0):
                modifiedEntrance = ENTRANCE(GROTTOS, 27);
                break;
        }
    }

    if (entranceToRegionId.contains(modifiedEntrance)) {
        return entranceToRegionId[modifiedEntrance];
    }

    return RR_MAX;
}

// Helper: Convert runtime game time to TimeSlice enum for dynamic time checking
TimeSlice TimeSliceFromGameTime(s32 day, u16 time) {
    // Handle edge cases: day 0 or invalid inputs
    if (day < 1 || day > 3) {
        return TIME_DAY1_AM_06_00; // Default fallback
    }

    // Convert to time slice based on day/time ranges
    // This is approximate - exact mapping would need game time constants
    bool isNight = (time >= GAME_TIME_NIGHT_START || time < GAME_TIME_DAY_START);
    int halfDayOffset = (day - 1) * 2 + (isNight ? 1 : 0);

    // Map to approximate time slice within the half-day
    if (halfDayOffset >= 6)
        return TIME_NIGHT3_AM_05_00;

    const auto& range = HALF_DAY_TIME_RANGES[halfDayOffset];
    return static_cast<TimeSlice>(range.startSlice);
}

// Helper: Returns the initial time state for logic solving (start at Day 1, 6:00 AM)
RegionTimeState InitialTimeState() {
    return { .timeSlices = (TIME_BIT_ONE << TIME_DAY1_AM_06_00), .canStayOverTime = false };
}

// Shared initialization function for region time states
std::unordered_map<RandoRegionId, RegionTimeState> InitializeRegionTimeStates(RandoRegionId startRegion) {
    std::unordered_map<RandoRegionId, RegionTimeState> states;

    // Start with appropriate time based on Clock Shuffle
    if (SettingClocks()) {
        // Clock Shuffle: start with owned time slices only
        states[startRegion] = { .timeSlices = TimeLogic::GetOwnedTimeSlices(), .canStayOverTime = false };
    } else {
        // No Clock Shuffle: start at Day 1 6am
        states[startRegion] = InitialTimeState();
    }

    return states;
}

// Helper to ensure region time state exists
void EnsureRegionTimeState(std::unordered_map<RandoRegionId, RegionTimeState>& regionTimeStates,
                           RandoRegionId regionId) {
    if (regionTimeStates.find(regionId) == regionTimeStates.end()) {
        auto& region = Regions[regionId];
        regionTimeStates[regionId] = { .timeSlices = TimeLogic::GetOwnedTimeSlices(),
                                       .canStayOverTime = region.canStayOverTime };
    }
}

// Time expansion during region traversal with stay restrictions
// Time expansion semantics: if canStayOverTime, sequentially test each future time slice
// Stop permanently if any timeStayRestrictions check fails
#ifdef COMBO_BUILD
#include "MmOrdinarySourcePredicates.inc.h"
#ifdef COMBO_BUILD
#include "MmGrottoRegionBindings.inc.h"
#include "MmGrottoContextRegions.inc.h"
#endif
#endif
void FindReachableRegions(RandoRegionId currentRegion, std::set<RandoRegionId>& reachableRegions,
                          std::unordered_map<RandoRegionId, RegionTimeState>& regionTimeStates) {
#ifdef COMBO_BUILD
    EnsureMmGrottoContextRegions();
#endif
    // Ensure current region has time state
    EnsureRegionTimeState(regionTimeStates, currentRegion);

    auto& sourceRegion = Regions[currentRegion];
    auto& sourceTimeState = regionTimeStates[currentRegion];
#ifdef COMBO_BUILD
    ComboRando::Entrances::MmFlagScope regionalScope(sourceTimeState.regionalFlags);
#endif

    // Expand time if player can wait in this region
    uint64_t currentTime = sourceTimeState.timeSlices;
    if (sourceTimeState.canStayOverTime) {
        currentTime = TimeLogic::ExpandTimeForward(currentTime, sourceRegion);
        sourceTimeState.timeSlices = currentTime;
    }

#ifdef COMBO_BUILD
    // _system.yml GLOBAL -> ACCESS is unconditional; impl still needs a
    // reachable logical slice. Never read the player's current day/time.
    if (currentRegion == RR_MAX)
        RecordMmAccess(currentTime);
    // The live tracker follows the active cross-game song options, as OoT's does; other searches use the seed's.
    const bool liveSongTravel = !ComboRando::MmSongOracle() && evaluatingLiveTracker &&
                                (ComboRando::ActiveSongTravel().enabled || ComboRando::ActiveSongTravel().foreignSongs);
    if (currentRegion == RR_MAX && (ComboRando::ActiveSongTravel().logicEnabled ||
                                    ComboRando::ActiveSongTravel().logicForeignSongs || liveSongTravel)) {
        const auto& ageArrivals = ComboRando::MmSongAgeArrivals();
        const uint32_t arrivals =
            ComboRando::MmSongOracle()
                ? (ageArrivals.active
                       ? (gSaveContext.save.linkAge <= 1 ? ageArrivals.masks[gSaveContext.save.linkAge] : 0)
                       : ComboRando::MmSongArrivals())
                : MM_LiveIncomingSongs();
        for (int i = 0; i < 10; i++)
            if (arrivals & (1u << (i + 6))) {
                const auto target = GetRegionIdFromEntrance(ComboRando::kMmSongEntrances[i]);
                if (target == RR_MAX)
                    continue;
                const uint64_t arrivalTimes = FilterNativeMmIngressTimes(ComboRando::kMmSongEntrances[i], currentTime);
                if (!arrivalTimes)
                    continue;
                RegionTimeState incoming{ .timeSlices = arrivalTimes,
                                          .canStayOverTime = Regions[target].canStayOverTime };
                if (reachableRegions.count(target) && TimeStateCovers(regionTimeStates[target], incoming))
                    continue;
                reachableRegions.insert(target);
                regionTimeStates[target] = MergeTimeStates(regionTimeStates[target], incoming);
                FindReachableRegions(target, reachableRegions, regionTimeStates);
            }
    }
#endif
#ifdef COMBO_BUILD
    if (currentRegion == RR_MAX)
        if (const auto& assumed = ComboRando::Entrances::ActiveMmOrdinaryAssumptions()) {
            for (const auto destination : assumed->syntheticDestinations) {
                const auto target = GetRegionIdFromEntrance(static_cast<s32>(destination));
                if (target == RR_MAX || !Regions.contains(target))
                    throw std::runtime_error("Unbound synthetic MM assumed arrival");
                // Source ASSUMED directly enters ee.to; ACCESS is not a source edge here.
                uint64_t incomingTimes = FilterNativeMmIngressTimes(static_cast<s32>(destination), TIME_ALL_SLICES);
                if (!incomingTimes)
                    continue;
                RegionTimeState incoming{ incomingTimes, Regions[target].canStayOverTime };
                if (reachableRegions.contains(target) && TimeStateCovers(regionTimeStates[target], incoming))
                    continue;
                reachableRegions.insert(target);
                regionTimeStates[target] = MergeTimeStates(regionTimeStates[target], incoming);
                FindReachableRegions(target, reachableRegions, regionTimeStates);
            }
        }
#endif
#ifdef COMBO_BUILD
    if (currentRegion == RR_MAX)
        if (const auto& assumed = ComboRando::Entrances::ActiveMmMixedAssumptions()) {
            for (const auto destination : assumed->syntheticDestinations) {
                const auto* record = ComboRando::Entrances::Find(ComboRando::Entrances::Game::Mm, destination);
                const bool grotto = record && ComboRando::Entrances::MmGrottoBound(record->name);
                const auto proxy = grotto ? ComboRando::Entrances::MmGrottoLogicArrival(destination) : destination;
                const auto target =
                    grotto ? MmTypedGrottoArrivalRegion(destination) : GetRegionIdFromEntrance(static_cast<s32>(proxy));
                if (target == RR_MAX || !Regions.contains(target))
                    throw std::runtime_error("Unbound synthetic MM mixed arrival");
                uint64_t incomingTimes = FilterNativeMmIngressTimes(static_cast<s32>(proxy), TIME_ALL_SLICES);
                if (!incomingTimes)
                    continue;
                RegionTimeState incoming{ incomingTimes, Regions[target].canStayOverTime };
                if (reachableRegions.contains(target) && TimeStateCovers(regionTimeStates[target], incoming))
                    continue;
                reachableRegions.insert(target);
                regionTimeStates[target] = MergeTimeStates(regionTimeStates[target], incoming);
                FindReachableRegions(target, reachableRegions, regionTimeStates);
            }
        }
#endif
    // Explore connections
    for (auto& [connectedRegionId, condition] : sourceRegion.connections) {
        // Set global time for check evaluation
        gCurrentRegionTime = currentTime;

        const uint64_t arrivalTimes = TimeLogic::EvaluateConditionTimes(condition.first, currentTime);
#ifdef COMBO_BUILD
        const auto arrivalFlags = ComboRando::Entrances::mmFlagResult;
#endif
        if (arrivalTimes) {
            auto& targetRegion = Regions[connectedRegionId];
            RegionTimeState incomingState = { .timeSlices = arrivalTimes,
                                              .canStayOverTime = targetRegion.canStayOverTime };
#ifdef COMBO_BUILD
            incomingState.regionalFlags = arrivalFlags;
#endif

            auto existingIt = regionTimeStates.find(connectedRegionId);
            if (existingIt != regionTimeStates.end()) {
                // Region already visited - check if we have new time to add
                if (TimeStateCovers(existingIt->second, incomingState)) {
                    continue; // Skip - existing state already covers incoming
                }
                // Merge time states for re-exploration
                existingIt->second = MergeTimeStates(existingIt->second, incomingState);
            } else {
                // First visit to this region
                reachableRegions.insert(connectedRegionId);
                regionTimeStates[connectedRegionId] = incomingState;
            }

            FindReachableRegions(connectedRegionId, reachableRegions, regionTimeStates);
        }
    }

    // Explore exits
    for (auto& [exitId, regionExit] : sourceRegion.exits) {
        // Set global time for check evaluation
        gCurrentRegionTime = currentTime;

        auto sourceCondition = regionExit.condition;
#ifdef COMBO_BUILD
        const auto grottoSource = MmNativeGrottoSource(currentRegion, exitId);
        const bool grottoOverride = grottoSource && (ComboRando::Entrances::MmGrottoGraphOverrides(*grottoSource) ||
                                                     NativeMmErContextsEnabled());
        const uint32_t mixedSource = grottoSource ? grottoSource->id : ComboRando::Entrances::MmMixedSourceKey(exitId);
        const bool mixedOverride = ComboRando::Entrances::MmMixedGraphOverrides(mixedSource);
        const bool ordinaryOverride = ComboRando::Entrances::MmOrdinaryGraphOverrides(exitId);
        const auto ordinaryDestinations = mixedOverride ? ComboRando::Entrances::MmMixedGraphDestinations(mixedSource)
                                          : grottoOverride
                                              ? ComboRando::Entrances::MmGrottoGraphDestinations(*grottoSource)
                                              : ComboRando::Entrances::MmOrdinaryGraphDestinations(exitId);
        if (ordinaryOverride || (mixedOverride && ComboRando::Entrances::MmMixedOrdinarySource(mixedSource)))
            sourceCondition = MmOrdinarySourcePredicate(currentRegion, exitId, regionExit.condition);
        if (grottoOverride && grottoSource->id == 0x1000eu)
            sourceCondition = [] { return true; }; // pinned NearCowGrotto -> cow, shovelMm off
        for (const auto destination : ordinaryDestinations) {
            const auto* destinationRecord = ComboRando::Entrances::Find(ComboRando::Entrances::Game::Mm, destination);
            const bool grottoDestination = (mixedOverride || grottoOverride) && destinationRecord &&
                                           ComboRando::Entrances::MmGrottoBound(destinationRecord->name);
            s32 destinationEntrance = static_cast<s32>(
                grottoDestination ? ComboRando::Entrances::MmGrottoLogicArrival(destination) : destination);
#else
        s32 destinationEntrance = exitId;
#endif
#ifdef COMBO_BUILD
            RandoRegionId connectedRegionId = grottoDestination ? MmTypedGrottoArrivalRegion(destination)
                                                                : GetRegionIdFromEntrance(destinationEntrance);
#else
        RandoRegionId connectedRegionId = GetRegionIdFromEntrance(destinationEntrance);
#endif
            uint64_t arrivalTimes = TimeLogic::EvaluateConditionTimes(sourceCondition, currentTime);
#ifdef COMBO_BUILD
            const auto arrivalFlags = ComboRando::Entrances::mmFlagResult;
#endif
            if (arrivalTimes) {
#ifdef COMBO_BUILD
                if (currentRegion == RR_MAX && ComboRando::ActiveSongTravel().logicEnabled) {
                    const int source = ComboRando::MmSongSource(exitId);
                    if (source >= 0) {
                        int destination = ComboRando::ActiveSongTravel().logicDestination(source);
                        if (destination < 6)
                            continue;
                        destinationEntrance = ComboRando::kMmSongEntrances[destination - 6];
                        connectedRegionId = GetRegionIdFromEntrance(destinationEntrance);
                    }
                }
                if (mixedOverride || ordinaryOverride || grottoOverride) {
                    const auto* record = ComboRando::Entrances::Find(ComboRando::Entrances::Game::Mm, destination);
                    if (!record)
                        throw std::runtime_error("Unregistered shuffled MM arrival");
                    if (!record->noSongOfTime) {
                        RecordMmAccess(arrivalTimes);
                        arrivalTimes =
                            TimeLogic::EvaluateConditionTimes([] { return NativeMmCanResetTime(); }, arrivalTimes);
                    }
                    if (connectedRegionId == RR_MAX)
                        throw std::runtime_error("Shuffled MM arrival has no native region");
                }
                arrivalTimes = FilterNativeMmIngressTimes(destinationEntrance, arrivalTimes);
                if (!arrivalTimes)
                    continue;
#endif
                auto& targetRegion = Regions[connectedRegionId];
                RegionTimeState incomingState = { .timeSlices = arrivalTimes,
                                                  .canStayOverTime = targetRegion.canStayOverTime };
#ifdef COMBO_BUILD
                incomingState.regionalFlags = arrivalFlags;
#endif

                auto existingIt = regionTimeStates.find(connectedRegionId);
                if (existingIt != regionTimeStates.end()) {
                    // Region already visited - check if we have new time to add
                    if (TimeStateCovers(existingIt->second, incomingState)) {
                        continue; // Skip - existing state already covers incoming
                    }
                    // Merge time states for re-exploration
                    existingIt->second = MergeTimeStates(existingIt->second, incomingState);
                } else {
                    // First visit to this region
                    reachableRegions.insert(connectedRegionId);
                    regionTimeStates[connectedRegionId] = incomingState;
                }

                FindReachableRegions(connectedRegionId, reachableRegions, regionTimeStates);
            }
#ifdef COMBO_BUILD
        }
#endif
    }
#ifdef COMBO_BUILD
    // Pinned MM Wallmaster buffer edges; these carry their source time/soul gate.
    ExploreMmWallmasterEntrances(currentRegion, currentTime, reachableRegions, regionTimeStates);
#endif
}

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_MAX] = RandoRegion{ .sceneId = SCENE_MAX,
        .checks = {
            CHECK(RC_STARTING_ITEM_DEKU_MASK, true),
            CHECK(RC_STARTING_ITEM_SONG_OF_HEALING, true),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(SOUTH_CLOCK_TOWN, 0),                      ONE_WAY_EXIT, true), // Save warp
            EXIT(ENTRANCE(SOUTH_CLOCK_TOWN, 9),                      ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_CLOCK_TOWN)),
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 10),              ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_SOUTHERN_SWAMP)),
            EXIT(ENTRANCE(WOODFALL, 4),                              ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_WOODFALL)),
            EXIT(ENTRANCE(MILK_ROAD, 4),                             ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_MILK_ROAD)),
            EXIT(ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8),               ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_MOUNTAIN_VILLAGE)),
            EXIT(ENTRANCE(SNOWHEAD, 3),                              ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_SNOWHEAD)),
            EXIT(ENTRANCE(GREAT_BAY_COAST, 11),                      ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_GREAT_BAY_COAST)),
            EXIT(ENTRANCE(ZORA_CAPE, 6),                             ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_ZORA_CAPE)),
            EXIT(ENTRANCE(IKANA_CANYON, 4),                          ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_IKANA_CANYON)),
            EXIT(ENTRANCE(STONE_TOWER, 3),                           ONE_WAY_EXIT, CAN_PLAY_SONG(SOARING) && CAN_OWL_WARP(OWL_WARP_STONE_TOWER)),
        },
    };
}, RANDO_LOGIC_INIT_PATHS);
// clang-format on

} // namespace Logic

} // namespace Rando
