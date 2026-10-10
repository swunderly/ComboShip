// Full logical MM arrivals use destination-native respawn and scene-setup adapters.
#include "rando/entrances/NativeArrival.h"
#include "rando/entrances/MmGrottoBindings.h"
#include "rando/entrances/MmOrdinaryBindings.h"
#include "rando/entrances/MmGrottoNative.h"
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/scenecommand/SetAlternateHeaders.h"
#include "2s2h/resource/type/scenecommand/SetEntranceList.h"
#include "2s2h/resource/type/scenecommand/SetRoomList.h"
#include "2s2h/resource/type/scenecommand/SetStartPositionList.h"
#include "rando/entrances/NativeSceneArrival.h"
#include <ship/resource/CrossRMRegistry.h>
extern "C" int MM_GetEffectiveHumanAge(int fileNum);
extern "C" int MM_CanFaroreTransitionAge(int fileNum, int age);
extern "C" int MM_ApplyFaroreTransitionAge(int fileNum, int age);
namespace CEMmArrival = ComboRando::Entrances;
namespace {
CEMmArrival::NativeEntranceFlight sMmEntranceFlight;
bool MmArrivalBound(uint32_t id) {
    bool bound = false;
    for (const auto& row : CEMmArrival::kMmGrottoGraphBindings)
        if (row.logicalId == id)
            bound = true;
    for (const auto& row : CEMmArrival::kMmOrdinaryBindings)
        if (row.logicalId == id)
            bound = true;
    if (!bound)
        return false;
    const auto owner = Ship::CrossRMRegistry::Get("mm");
    if (!owner)
        return false;
    Ship::OwnRMScope scope("mm");
    try {
        // Resolve custom IDs using a local copy. Preflight changes no respawn flags,
        // grotto data, previous scene, age, form or native save fields.
        auto context = CEMmArrival::ReadNativeMmGrotto(gSaveContext.respawn, CEMmArrival::MmPreviousScene::Other, 0,
                                                       CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE));
        auto entrance = CEMmArrival::ApplyMmCustomEntrance({ CEMmArrival::Game::Mm, id }, context).id;
        int cutscene = 0;
        entrance =
            CEMmArrival::FixMmNativeArrival(entrance, CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE),
                                            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE),
                                            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE),
                                            CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE), cutscene);
        const int layer = cutscene >= 0xfff0 ? (cutscene & 0xf) + 1 : 0;
        if (entrance + layer > 0xffff)
            return false;
        const int scene = Entrance_GetSceneIdAbsolute(static_cast<u16>(entrance + layer));
        const int spawn = Entrance_GetSpawnNum(static_cast<u16>(entrance + layer));
        if (scene < 0 || scene >= SCENE_MAX || !gSceneTable[scene].segment.fileName)
            return false;
        const std::string name = gSceneTable[scene].segment.fileName;
        const int room = context.respawnFlag == 4 ? context.respawn[3].roomIndex : -1;
        return CEMmArrival::ValidateOwnedNativeSpawn(owner, "scenes/nonmq/" + name + "/" + name, layer, spawn, room,
                                                     false);
    } catch (...) { return false; }
}
bool MmEntranceNeedsFdCleanup(PlayState* play) {
    if (!play || gSaveContext.save.playerForm != PLAYER_FORM_FIERCE_DEITY ||
        CVarGetInteger("gEnhancements.Masks.FierceDeitysAnywhere", 0))
        return false;
    switch (play->sceneId) {
        case SCENE_MITURIN_BS:
        case SCENE_HAKUGIN_BS:
        case SCENE_SEA_BS:
        case SCENE_INISIE_BS:
        case SCENE_LAST_BS:
            return true;
        default:
            return false;
    }
}
} // namespace
extern "C" COMBO_EXPORT void MM_SetEntranceBridge(ComboEntrancePreflight preflight, ComboEntranceCommit commit) {
    Ship::CrossRMRegistry::RegisterTeardownListener([]() { sMmEntranceFlight.cancel(); });
    sMmEntranceFlight.bridge(preflight, commit);
}
extern "C" COMBO_EXPORT void MM_CancelEntranceFlight() {
    sMmEntranceFlight.cancel();
}
extern "C" COMBO_EXPORT int MM_CanEntranceArrival(int file, uint32_t id, int age, int crossAge) {
    if (file < 0 || file > 2 || file != gSaveContext.fileNum || age < 0 || age > 1 ||
        (crossAge != 0 && crossAge != 1) || !MmArrivalBound(id) || MM_GetEffectiveHumanAge(file) < 0)
        return 0;
    return !crossAge || MM_CanFaroreTransitionAge(file, age);
}
extern "C" COMBO_EXPORT int MM_ArmEntranceArrival(int file, uint32_t id, int age, int crossAge) {
    if (!MM_CanEntranceArrival(file, id, age, crossAge))
        return 0;
    return sMmEntranceFlight.arm({ file, 0, 1, age, crossAge, id }, 1);
}
extern "C" COMBO_EXPORT int MM_RequestEntranceDeparture(int file, int destination, uint32_t id, int crossAge) {
    if (file != gSaveContext.fileNum || !gPlayState || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        MM_FaroreDeparturePending())
        return 0;
    // Pinned MM Play_GameSwitch snaps MM to the saved OoT age before saving.
    const int age = crossAge ? (sOotAge ? sOotAge(file) : -1) : MM_GetEffectiveHumanAge(file);
    if (crossAge && !MM_CanFaroreTransitionAge(file, age))
        return 0;
    return sMmEntranceFlight.queue({ file, 1, destination, age, crossAge, id });
}
extern "C" COMBO_EXPORT int Combo_MMFinishEntranceDeparture(void* state) {
    auto* play = static_cast<PlayState*>(state);
    if (!play || play != gPlayState)
        return 0;
    const int originalAge = MM_GetEffectiveHumanAge(gSaveContext.fileNum);
    const auto originalForm = gSaveContext.save.playerForm, originalMask = gSaveContext.save.equippedMask;
    return sMmEntranceFlight.finish(
        gSaveContext.fileNum, 1,
        [play, originalForm, originalMask](const auto& request) {
            // Exact pinned comboTransitionOverrideMM cleanup, before the native owl save.
            if (MmEntranceNeedsFdCleanup(play)) {
                gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
                gSaveContext.save.equippedMask = PLAYER_MASK_NONE;
            }
            if (request.crossAge && !MM_ApplyFaroreTransitionAge(request.file, request.age)) {
                gSaveContext.save.playerForm = originalForm;
                gSaveContext.save.equippedMask = originalMask;
                return false;
            }
            SaveManager_SaveCurrentForCombo();
            return true;
        },
        [originalAge, originalForm, originalMask](const auto& request) {
            if (request.crossAge)
                MM_ApplyFaroreTransitionAge(request.file, originalAge);
            gSaveContext.save.playerForm = originalForm;
            gSaveContext.save.equippedMask = originalMask;
            SaveManager_SaveCurrentForCombo();
        });
}
extern "C" COMBO_EXPORT int MM_ConsumeFinishedEntranceDeparture() {
    return sMmEntranceFlight.takeFinished();
}
extern "C" COMBO_EXPORT int MM_ConsumeEntranceArrival() {
    const auto arrival = sMmEntranceFlight.takeArrival(gSaveContext.fileNum, 1);
    if (!arrival)
        return -1;
    if (!MM_CanEntranceArrival(arrival->file, arrival->id, arrival->age, arrival->crossAge))
        return -1;
    if (arrival->crossAge && !MM_ApplyFaroreTransitionAge(arrival->file, arrival->age))
        return -1;
    auto previous = CEMmArrival::MmPreviousScene::Other;
    int32_t flag = gSaveContext.respawnFlag;
    uint32_t entrance =
        CEMmArrival::ApplyNativeMmGrotto({ CEMmArrival::Game::Mm, arrival->id }, gSaveContext.respawn, previous, flag,
                                         CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE));
    auto& ship = gSaveContext.save.shipSaveInfo;
    if (previous == CEMmArrival::MmPreviousScene::TerminaField ||
        previous == CEMmArrival::MmPreviousScene::GreatBayCoast) {
        ship.grottoPreviousScene =
            previous == CEMmArrival::MmPreviousScene::TerminaField ? SCENE_00KEIKOKU : SCENE_30GYOSON;
        ship.grottoPreviousSceneValid = 1;
    }
    int nextCutscene = 0; // pinned FastInit clears pending cutscene before scene fixup
    entrance =
        CEMmArrival::FixMmNativeArrival(entrance, CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE),
                                        CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE),
                                        CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE),
                                        CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE), nextCutscene);
    gSaveContext.nextCutsceneIndex = nextCutscene;
    gSaveContext.respawnFlag = flag;
    return static_cast<int>(entrance);
}
