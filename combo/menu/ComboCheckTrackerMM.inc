// Included by the native MM tracker. Check flags continue to live in its normal save.
#include "ComboCheckTrackerABI.h"
#include "../ComboExport.h"

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
