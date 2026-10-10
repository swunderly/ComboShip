// Included by the native OoT tracker so its hooks and save format remain authoritative.
#include "ComboCheckTrackerABI.h"
#include "../ComboExport.h"
#include "../rando/GameplayRules.h"

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
                    if (knowledge.source == 1) flags |= COMBO_CHECK_SEEN;
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
