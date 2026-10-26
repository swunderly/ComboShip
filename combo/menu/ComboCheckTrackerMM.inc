// Included by the native MM tracker. Check flags continue to live in its normal save.
#include "ComboCheckTrackerABI.h"
#include "../ComboExport.h"
#include "../ComboResolve.h"
#include "../rando/LiveWallmasterRoutes.h"
#include "../rando/entrances/MmUnifiedLayout.h"

// Full Wallmaster Shuffle's routes between the games (rando/LiveWallmasterRoutes.h). Only a Full layout has them.
namespace LiveCE = ComboRando::Entrances;
static bool ComboMmCrossGameWallmasters() {
    const auto* settings = LiveCE::ActiveMmUnified().settings();
    return settings && settings->erWallmasters == LiveCE::Mode::Full;
}
// One live search with these arrivals (both ages: MM searches with one): Logic.cpp roots them and
// ComboEntrancesMM.cpp records the departures, as in an oracle search. Returns the departures (OoT bits).
static uint32_t ComboLiveMmSearch(const std::function<void()>& search, uint32_t arrivals) {
    LiveCE::MmWallmasterArrivals() = { { arrivals, arrivals } };
    LiveCE::MmWallmasterDepartures() = {};
    search();
    const auto& departures = LiveCE::MmWallmasterDepartures().masks;
    return (departures[0] | departures[1]) & LiveCE::kOotWallmasterArrivalBits;
}
// The exchange's searches run alone, and leave no arrivals for a later search (an oracle's).
static bool sComboMmLiveExchange = false;
struct ComboMmLiveExchangeScope {
    ComboMmLiveExchangeScope() {
        sComboMmLiveExchange = true;
    }
    ~ComboMmLiveExchangeScope() {
        LiveCE::MmWallmasterArrivals() = {};
        LiveCE::MmWallmasterDepartures() = {};
        sComboMmLiveExchange = false;
    }
};
static void ComboMmTrackerSearch(const std::function<void()>& search) {
    if (sComboMmLiveExchange || !Combo_MmIsForeground() || !ComboMmCrossGameWallmasters()) {
        search();
        return;
    }
    static const auto ootDepartures =
        (uint32_t (*)(int, uint32_t, uint64_t))Combo_ResolveSym("soh", "SOH_LiveWallmasterDepartures");
    ComboMmLiveExchangeScope exchange;
    try {
        LiveCE::LiveWallmastersMmInFront(
            [&](uint32_t arrivals) { return ComboLiveMmSearch(search, arrivals); },
            [&](uint32_t fromMm) {
                // The MM search keeps its logic events in the save: key by the save it left.
                const uint64_t mmState = LiveCE::LiveStateKey(&gSaveContext, sizeof(gSaveContext));
                return ootDepartures
                           ? ootDepartures(gSaveContext.fileNum, fromMm, mmState) & LiveCE::kMmWallmasterArrivalBits
                           : 0u;
            });
    } catch (const std::exception& e) {
        SPDLOG_ERROR("[ComboShip] Tracker Wallmaster routes into OoT unavailable: {}", e.what());
        LiveCE::MmWallmasterArrivals() = {};
        search();
    }
}

// OoT in front: MM's live search from its save in memory with OoT's departures (MM bits), and its departures into OoT
// (OoT bits). Its state does not change while OoT runs, except by a grant, so the answer is kept until the save does.
extern "C" COMBO_EXPORT uint32_t MM_LiveWallmasterDepartures(int fileNum, uint32_t fromOot, uint64_t ootState) {
    if (!IS_RANDO || fileNum != gSaveContext.fileNum || sComboMmLiveExchange || !ComboMmCrossGameWallmasters())
        return 0;
    static uint64_t cachedKey = 0;
    static uint32_t cached = 0;
    const auto stateKey = [fromOot, ootState] {
        return LiveCE::LiveStateKey(&gSaveContext, sizeof(gSaveContext), ootState ^ fromOot);
    };
    if (stateKey() == cachedKey)
        return cached;
    uint32_t departures = 0;
    {
        ComboMmLiveExchangeScope exchange;
        try {
            std::unordered_map<RandoCheckId, bool> inLogic;
            departures = ComboLiveMmSearch([&] { Rando::CheckTracker::ComputeChecksInLogicOnce(inLogic); }, fromOot);
        } catch (const std::exception& e) {
            SPDLOG_ERROR("[ComboShip] Tracker Wallmaster routes from OoT unavailable: {}", e.what());
            departures = 0;
        }
    }
    // The search rewrites the logic events it keeps in the save; key the answer by the save it leaves.
    cachedKey = stateKey();
    cached = departures;
    return departures;
}

static const char* ComboMmCheckType(RandoCheckType type) {
    static constexpr const char* names[] = { "Item", "Barrel", "Butterfly", "Chest", "Cow", "Crate",
        "Enemy drop", "Freestanding", "Frog", "Grass", "Heart", "Beehive", "Minigame", "NPC", "Owl",
        "Pot", "Dungeon reward", "Shop", "Skulltula", "Snowball", "Song", "Fairy", "Shop", "Tree", "Wonder item" };
    return type >= 0 && type < RCTYPE_MAX ? names[type] : "Item";
}

extern "C" COMBO_EXPORT void MM_ComboCheckSnapshot(ComboCheckInfo* info, ComboCheckVisitor visitor, void* user) {
    if (!info) return;
    *info = { 0, -1, -1 };
    if (!Combo_MmIsForeground() || !gPlayState || !IS_RANDO || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2) return;
    *info = { 1, gSaveContext.fileNum, GetScrollTargetScene(gPlayState->sceneId) };
    if (!visitor) return;
    const bool wantLogic = CVarGetInteger("gCombo.CheckTracker.ShowLogic", 1);
    if (wantLogic) RefreshChecksInLogic();
    for (const auto scene : sortedSceneIds) {
        if (scene == SCENE_MAX) continue;
        const std::string areaName = GetCheckTrackerSceneName(scene);
        for (const auto id : sceneChecks[scene]) {
            const auto& check = RANDO_SAVE_CHECKS[id];
            if (!check.shuffled) continue;
            uint32_t flags = check.obtained ? COMBO_CHECK_COLLECTED : 0;
            if (check.skipped) flags |= COMBO_CHECK_SKIPPED;
            if (check.seenItemId != RI_UNKNOWN) flags |= check.knowledgeSource == 1 ? COMBO_CHECK_SEEN :
                check.knowledgeSource == 2 ? COMBO_CHECK_HINTED : COMBO_CHECK_KNOWN;
            if (wantLogic) flags |= COMBO_CHECK_LOGIC_KNOWN;
            if (wantLogic && checksInLogic.contains(id)) flags |= COMBO_CHECK_AVAILABLE;
            std::string reward;
            if (check.obtained || check.seenItemId != RI_UNKNOWN) {
                if (check.randoItemId == RI_COMBO_FOREIGN) {
                    const auto* item = Rando::MiscBehavior::MM_LookupForeign(id);
                    if (item) {
                        // A preview of a disguised trap never discloses its actual reward.
                        reward = !check.obtained && item->HasDisguise() ? item->fakeDisplayName : item->displayName;
                    }
                } else {
                    const auto item = Rando::StaticData::Items.find(check.obtained ? check.randoItemId : check.seenItemId);
                    if (item != Rando::StaticData::Items.end()) reward = item->second.name;
                }
            }
            const std::string condition = wantLogic ? accessLogicFuncs[id] : "";
            ComboCheckRow row{ id, scene, flags, areaName.c_str(), Rando::StaticData::CheckNames[id].c_str(),
                ComboMmCheckType(Rando::StaticData::Checks[id].randoCheckType), reward.c_str(), condition.c_str() };
            visitor(&row, user);
        }
    }
}

extern "C" COMBO_EXPORT int MM_ComboCheckSetSkipped(int32_t slot, int32_t id, int32_t skipped) {
    ComboCheckInfo info{};
    MM_ComboCheckSnapshot(&info, nullptr, nullptr);
    if (!info.loaded || info.slot != slot || id < 0 || id >= RC_MAX) return 0;
    auto& check = RANDO_SAVE_CHECKS[id];
    if (!check.shuffled || check.obtained) return 0;
    check.skipped = skipped != 0;
    // Matches the native tracker: persisted by the game's normal save, never forces a time-cycle save.
    return 1;
}
