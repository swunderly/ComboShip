// Included by the native OoT tracker so its hooks and save format remain authoritative.
#include "ComboCheckTrackerABI.h"
#include "../ComboExport.h"
#include "../ComboResolve.h"
#include "../rando/GameplayRules.h"
#include "../rando/LiveTrackerRules.h"
#include "../rando/LiveWallmasterRoutes.h"
#include "../rando/entrances/OotUnifiedLayout.h"
uint8_t SOH_LiveMmEntryAges(const std::array<uint32_t, 2>& wallmasterDepartures); // OTRGlobals.cpp
extern "C" COMBO_EXPORT uint32_t SOH_GetLiveSongKnowledge();
extern "C" COMBO_EXPORT uint32_t SOH_GetLiveSongSources();

namespace CheckTracker {
static void UpdateComboAvailabilitySettings(bool wantLogic) {
    static std::vector<int> lastValues;
    std::vector<int> values;
    for (const auto& rule : ComboRando::kGameplayRules)
        values.push_back(ComboRando::ReadGameplayRule(rule, false, CVarGetInteger));
    // Held supplies can change availability even when ownership does not.
    // Ammo and magic need a refresh when emptied/refilled. Reachable renewable
    // money sources and wallet capacity govern prices, not the current balance.
    if (wantLogic) {
        for (int slot : { SLOT_BOW, SLOT_SLINGSHOT, SLOT_BOMB, SLOT_BOMBCHU })
            values.push_back(gSaveContext.inventory.ammo[slot] > 0);
        values.push_back(gSaveContext.magic > 0);
    }
    if (wantLogic && (!enableAvailableChecks || values != lastValues)) {
        RecalculateAvailableChecks();
    }
    enableAvailableChecks = wantLogic;
    lastValues = std::move(values);
}

static const char* ComboCheckType(RandomizerCheckType type) {
    switch (type) {
        case RCTYPE_SKULL_TOKEN: return "Skulltula";
        case RCTYPE_SHOP: case RCTYPE_MERCHANT: case RCTYPE_SCRUB: return "Shop";
        case RCTYPE_COW: return "Cow";
        case RCTYPE_SONG_LOCATION: return "Song";
        case RCTYPE_POT: return "Pot";
        case RCTYPE_CRATE: case RCTYPE_NLCRATE: case RCTYPE_SMALL_CRATE: return "Crate";
        case RCTYPE_BEEHIVE: return "Beehive";
        case RCTYPE_GRASS: case RCTYPE_BUSH: return "Grass";
        case RCTYPE_FREESTANDING: return "Freestanding";
        case RCTYPE_DUNGEON_REWARD: case RCTYPE_BOSS_HEART_OR_OTHER_REWARD: return "Dungeon reward";
        case RCTYPE_FOUNTAIN_FAIRY: case RCTYPE_STONE_FAIRY: case RCTYPE_BEAN_FAIRY:
        case RCTYPE_SONG_FAIRY: case RCTYPE_BUTTERFLY_FAIRY: return "Fairy";
        case RCTYPE_TREE: case RCTYPE_NLTREE: return "Tree";
        case RCTYPE_FISH: return "Fish";
        case RCTYPE_WONDER_ITEM: return "Wonder item";
        default: return "Item";
    }
}
}

// Full Wallmaster Shuffle's routes between the games (rando/LiveWallmasterRoutes.h). Only a Full layout has them, with
// or without the warps: its Wallmaster setting is Full under either Full id, as MM's tracker reads it.
namespace CheckTracker {
namespace LiveCE = ComboRando::Entrances;
static bool ComboCrossGameWallmasters() {
    const auto* settings = LiveCE::ActiveOotUnified().settings();
    return settings && settings->erWallmasters == LiveCE::Mode::Full;
}
// One live search with these arrivals: fill.cpp roots them and records the departures, as in an oracle search.
static LiveCE::LiveOotSearch ComboLiveOotSearch(const std::function<void()>& search,
                                                const LiveCE::WallmasterAgeMasks& arrivals) {
    LiveCE::OotWallmasterArrivals() = arrivals;
    LiveCE::OotWallmasterDepartures() = {};
    search();
    const auto departures = LiveCE::OotWallmasterDepartures().masks;
    return { (departures[0] | departures[1]) & LiveCE::kMmWallmasterArrivalBits, SOH_LiveMmEntryAges(departures) };
}
// The exchange's searches run alone, and leave no arrivals for a later search (an oracle's, the playthrough's).
static bool sComboLiveExchange = false;
struct ComboLiveExchangeScope {
    ComboLiveExchangeScope() {
        sComboLiveExchange = true;
    }
    ~ComboLiveExchangeScope() {
        LiveCE::OotWallmasterArrivals() = {};
        LiveCE::OotWallmasterDepartures() = {};
        sComboLiveExchange = false;
    }
};
static void ComboTrackerSearch(const std::function<void()>& search) {
    if (sComboLiveExchange || !Combo_OotIsForeground() || !ComboCrossGameWallmasters()) {
        search();
        return;
    }
    static const auto mmDepartures =
        (uint32_t (*)(int, uint32_t, uint64_t))Combo_ResolveSym("2ship", "MM_LiveWallmasterDepartures");
    // What MM's search reads of OoT, which OoT's searches leave as they are; not OoT's save, which changes every frame.
    const uint64_t ootState = LiveCE::LiveSongExportsKey(SOH_GetLiveSongKnowledge(), SOH_GetLiveSongSources());
    ComboLiveExchangeScope exchange;
    try {
        LiveCE::LiveWallmastersOotInFront(
            [&](const LiveCE::WallmasterAgeMasks& arrivals) { return ComboLiveOotSearch(search, arrivals); },
            [&](uint32_t fromOot) {
                return mmDepartures
                           ? mmDepartures(gSaveContext.fileNum, fromOot, ootState) & LiveCE::kOotWallmasterArrivalBits
                           : 0u;
            });
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] Tracker Wallmaster routes into MM unavailable: {}", e.what());
        LiveCE::OotWallmasterArrivals() = {};
        search();
    }
}
} // namespace CheckTracker

// MM in front: OoT's live search from its save in memory with MM's departures (OoT bits), and its departures into MM
// (MM bits). Nothing it reads changes while MM runs unless something happens (an item, a song, an option), so its
// answers are kept for what it reads (rando/LiveWallmasterRoutes.h LiveDepartureMemo): OoT's save, what it reads of MM
// (mmState, LiveSongExportsKey) and the rules.
extern "C" COMBO_EXPORT uint32_t SOH_LiveWallmasterDepartures(int fileNum, uint32_t fromMm, uint64_t mmState) {
    using namespace CheckTracker;
    if (!IS_RANDO || fileNum != gSaveContext.fileNum || sComboLiveExchange || !ComboCrossGameWallmasters())
        return 0;
    static LiveCE::LiveDepartureMemo memo;
    Ship::ResourceManagerScope rmScope(Ship::CrossRMRegistry::Get("oot"));
    struct Peek {
        int previous = gComboTrackerPeekSaveLoaded;
        Peek() {
            gComboTrackerPeekSaveLoaded = !Combo_OotIsForeground();
        }
        ~Peek() {
            gComboTrackerPeekSaveLoaded = previous;
        }
    } peek;
    if (!GameInteractor::IsSaveLoaded())
        return 0;
    const uint64_t inputs =
        LiveCE::LiveStateKey(&gSaveContext, sizeof(gSaveContext),
                             LiveCE::LiveRulesKey(CVarGetInteger, LiveCE::ActiveOotUnified().revision(), mmState));
    try {
        return memo.Answer(inputs, fromMm, [&] {
            logic = OTRGlobals::Instance->gRandoContext->GetLogic();
            RandomizerRegion region = RR_ROOT;
            RandoAgeTime ageTime = RAT_NONE;
            LiveSearchStart(region, ageTime);
            const auto search = [&] { ReachabilitySearch({}, RG_NONE, true, region, ageTime); };
            ComboLiveExchangeScope exchange;
            return LiveCE::LiveOotDeparturesForMm(fromMm, LINK_IS_CHILD ? 2 : 1, [&](const auto& arrivals) {
                return ComboLiveOotSearch(search, arrivals);
            });
        });
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] Tracker Wallmaster routes from MM unavailable: {}", e.what());
        return 0;
    }
}

extern "C" COMBO_EXPORT void SOH_ComboCheckSnapshot(ComboCheckInfo* info, ComboCheckVisitor visitor, void* user) {
    using namespace CheckTracker;
    if (!info) return;
    *info = { 0, -1, -1 };
    if (!Combo_OotIsForeground() || !gPlayState || !GameInteractor::IsSaveLoaded() || !initialized) return;
    *info = { 1, static_cast<int32_t>(gSaveContext.fileNum), static_cast<int32_t>(currentArea) };
    if (!visitor) return;

    mystery = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("MysteriousShuffle"), 0);
    const bool wantLogic = IS_RANDO && CVarGetInteger("gCombo.CheckTracker.ShowLogic", 1);
    UpdateComboAvailabilitySettings(wantLogic);
    if (recalculateAvailable && InternalRecalculateAvailableChecks(availableChecksStartingRegion, availableChecksStartingAgeTime)) {
        recalculateAvailable = false;
        availableChecksStartingRegion = RR_ROOT;
        availableChecksStartingAgeTime = RAT_NONE;
    }
    for (const auto& [area, checks] : checksByArea) {
        if (area == RCAREA_INVALID) continue;
        const std::string areaName = RandomizerCheckObjects::GetRCAreaName(area);
        if (!IsAreaSpoiled(area) && !CVarGetInteger(CVAR_TRACKER_CHECK("MQSpoilers"), 0)) {
            ComboCheckRow row{ -1, area, COMBO_CHECK_AREA_HIDDEN, areaName.c_str(),
                               "Explore this area to reveal its checks", "", "", "" };
            visitor(&row, user);
            continue;
        }
        for (auto rc : checks) {
            if (!IsVisibleInCheckTracker(rc)) continue;
            const auto* loc = Rando::StaticData::GetLocation(rc);
            auto* item = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);
            const auto status = item->GetCheckStatus();
            uint32_t flags = item->HasObtained() ? COMBO_CHECK_COLLECTED : 0;
            if (item->GetIsSkipped()) flags |= COMBO_CHECK_SKIPPED;
            if (status == RCSHOW_SAVED) flags |= COMBO_CHECK_SAVED;
            if (status == RCSHOW_SCUMMED) flags |= COMBO_CHECK_SCUMMED;
            if (status == RCSHOW_SEEN_OR_HINTED || status == RCSHOW_IDENTIFIED) {
                const auto knowledge = GetKnowledge(rc);
                // Legacy price/seen flags do not prove that a reward was learned.
                if (!knowledge.displayName.empty()) {
                    if (ComboRando::TrackerKnowledgeSeen(knowledge.source)) flags |= COMBO_CHECK_SEEN;
                    else if (knowledge.source == 2) flags |= COMBO_CHECK_HINTED;
                }
            }
            if (wantLogic) flags |= COMBO_CHECK_LOGIC_KNOWN;
            if (wantLogic && item->IsAvailable()) flags |= COMBO_CHECK_AVAILABLE;
            std::string name = checkNameOverrides.contains(rc) ? checkNameOverrides[rc] : loc->GetShortName();
            std::string reward = ComboKnownReward(rc);
            if (reward == "Skipped") reward.clear(); // Native view's status fallback is not a reward.
            std::string condition;
            if (wantLogic) {
                for (auto& access : areaTable[item->GetParentRegionKey()].locations) {
                    if (access.GetLocation() == rc) {
                        condition = access.GetConditionStr();
                        if (condition == "true") condition.clear();
                        break;
                    }
                }
            }
            ComboCheckRow row{ rc, area, flags, areaName.c_str(), name.c_str(), ComboCheckType(loc->GetRCType()),
                               reward.c_str(), condition.c_str() };
            visitor(&row, user);
        }
    }
}

extern "C" COMBO_EXPORT int SOH_ComboCheckSetSkipped(int32_t slot, int32_t id, int32_t skipped) {
    using namespace CheckTracker;
    ComboCheckInfo info{};
    SOH_ComboCheckSnapshot(&info, nullptr, nullptr);
    if (!info.loaded || slot != info.slot || id < 0 || id >= RC_MAX) return 0;
    auto rc = static_cast<RandomizerCheck>(id);
    if (!IsVisibleInCheckTracker(rc)) return 0;
    auto* item = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);
    if (item->HasObtained()) return 0;
    item->SetIsSkipped(skipped != 0);
    UpdateOrdering(Rando::StaticData::GetLocation(rc)->GetArea());
    UpdateInventoryChecks();
    SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);
    return 1;
}
