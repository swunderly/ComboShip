// Native arbitrary arrival service, including the pinned33 typed grotto identities.
#include "rando/entrances/NativeArrival.h"
#include "rando/entrances/OotNativeArrival.h"
#include "soh/Enhancements/randomizer/randomizer_grotto.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetAlternateHeaders.h"
#include "soh/resource/type/scenecommand/SetEntranceList.h"
#include "soh/resource/type/scenecommand/SetRoomList.h"
#include "soh/resource/type/scenecommand/SetStartPositionList.h"
#include "rando/entrances/NativeSceneArrival.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include <ship/resource/CrossRMRegistry.h>
#include <spdlog/spdlog.h>
namespace CEArrival = ComboRando::Entrances;
namespace {
CEArrival::NativeEntranceFlight sOotEntranceFlight;
// A queued departure belongs to the play state that queued it (a Full Wallmaster capture's). A reset or a quit during
// its fade, or any other end of that play state, forgets it, so the next transition of a reloaded file is never sent
// to the other game. An arrival armed for OoT stays: the combo return fires OnExitGame before it consumes the arrival
// (title_setup.c).
void RegisterOotEntranceFlightHooks() {
    COND_HOOK(OnExitGame, true, [](int32_t) { sOotEntranceFlight.cancelDeparture(); });
    COND_HOOK(OnPlayDestroy, true, []() { sOotEntranceFlight.cancelDeparture(); });
}
RegisterShipInitFunc sOotEntranceFlightHooks(RegisterOotEntranceFlightHooks);
bool OotRawArrivalBound(uint32_t id, int age) {
    const auto* record = CEArrival::Find(CEArrival::Game::Oot, id);
    const auto arrival = CEArrival::ResolveOotNativeArrival(id);
    if (!record || !arrival || arrival->entrance >= ENTRANCE_TABLE_SIZE || age < 0 || age > 1)
        return false;
    if (arrival->grotto) {
        GrottoLoadInfo native{};
        if (!Grotto_GetComboNativeLoadInfo(arrival->grotto->nativeOffset, &native) ||
            static_cast<uint16_t>(native.entranceIndex) != arrival->grotto->load ||
            static_cast<uint8_t>(native.content) != arrival->grotto->content)
            return false;
    }
    const uint32_t raw = arrival->entrance;
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!owner)
        return false;
    Ship::OwnRMScope scope("oot");
    // Same normal-load layer selection as native Play_Init, including the two
    // overworld event-specific variants. Cross arrivals clear pending cutscenes.
    const auto time = gSaveContext.nextDayTime != 0xffff ? gSaveContext.nextDayTime : gSaveContext.dayTime;
    const bool night = time > 0xc000 || time < 0x4555;
    int layer = (age == 0 ? 2 : 0) + (night ? 1 : 0);
    const int baseScene = gEntranceTable[raw].scene;
    if (baseScene == SCENE_HYRULE_FIELD && age == 1)
        layer = CHECK_QUEST_ITEM(QUEST_KOKIRI_EMERALD) && CHECK_QUEST_ITEM(QUEST_GORON_RUBY) &&
                        CHECK_QUEST_ITEM(QUEST_ZORA_SAPPHIRE)
                    ? 1
                    : 0;
    else if (baseScene == SCENE_KOKIRI_FOREST && age == 0)
        layer = Flags_GetEventChkInf(EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP) ? 3 : 2;
    if (raw + layer >= ENTRANCE_TABLE_SIZE)
        return false;
    const auto& entrance = gEntranceTable[raw + layer];
    const int scene = entrance.scene;
    if (scene < 0 || scene >= SCENE_ID_MAX || !gSceneTable[scene].sceneFile.fileName)
        return false;
    const bool dungeon = (scene >= SCENE_DEKU_TREE && scene <= SCENE_ICE_CAVERN) ||
                         scene == SCENE_GERUDO_TRAINING_GROUND || scene == SCENE_INSIDE_GANONS_CASTLE;
    const std::string version = dungeon ? (ResourceMgr_SceneUsesMqAssets(scene) ? "mq" : "nonmq") : "shared";
    const std::string name = gSceneTable[scene].sceneFile.fileName;
    return CEArrival::ValidateOwnedNativeSpawn(owner, "scenes/" + version + "/" + name + "/" + name, layer,
                                               entrance.spawn, arrival->returning ? arrival->grotto->returnRoom : -1,
                                               true);
}
void OotReloadEntranceSword(PlayState* play) {
    static const u8 swords[] = { ITEM_NONE, ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER, ITEM_SWORD_BGS };
    u8 item = swords[gSaveContext.equips.equipment & 0xf];
    if (item == ITEM_NONE)
        Flags_SetInfTable(INFTABLE_SWORDLESS);
    else
        Flags_UnsetInfTable(INFTABLE_SWORDLESS);
    if (item == ITEM_SWORD_BGS && !gSaveContext.bgsFlag && !gSaveContext.swordHealth)
        item = ITEM_SWORD_BROKEN;
    gSaveContext.equips.buttonItems[0] = item;
    Interface_LoadItemIcon1(play, 0);
}
} // namespace
extern "C" COMBO_EXPORT void SOH_SetEntranceBridge(ComboEntrancePreflight preflight, ComboEntranceCommit commit) {
    Ship::CrossRMRegistry::RegisterTeardownListener([]() { sOotEntranceFlight.cancel(); });
    sOotEntranceFlight.bridge(preflight, commit);
}
extern "C" COMBO_EXPORT void SOH_CancelEntranceFlight() {
    sOotEntranceFlight.cancel();
}
extern "C" COMBO_EXPORT int SOH_CanEntranceArrival(int file, uint32_t id, int age, int crossAge) {
    if (file < 0 || file > 2 || file != gSaveContext.fileNum || age < 0 || age > 1 ||
        (crossAge != 0 && crossAge != 1) || !OotRawArrivalBound(id, crossAge ? age : gSaveContext.linkAge))
        return 0;
    return !crossAge || SOH_CanApplySavedHumanAge(file, age);
}
extern "C" COMBO_EXPORT int SOH_ArmEntranceArrival(int file, uint32_t id, int age, int crossAge) {
    if (!SOH_CanEntranceArrival(file, id, age, crossAge))
        return 0;
    return sOotEntranceFlight.arm({ file, 1, 0, age, crossAge, id }, 0);
}
extern "C" COMBO_EXPORT int SOH_RequestEntranceDeparture(int file, int destination, uint32_t id, int crossAge) {
    if (file != gSaveContext.fileNum || !gPlayState || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        SOH_FaroreDeparturePending())
        return 0;
    // Pinned ReloadSword indexes a four-entry native equipment field.
    if ((gSaveContext.equips.equipment & 0xf) > 3)
        return 0;
    return sOotEntranceFlight.queue({ file, 0, destination, gSaveContext.linkAge, crossAge, id }, gPlayState);
}
// A Full Wallmaster capture's departure to an MM dungeon (combo_entrance_runtime.cpp), under the seed's Cross-Games
// Age as the Farore rules hold it: the launcher's preflight refuses any other value (ComboShip.cpp
// Combo_PreflightEntranceDeparture).
extern "C" int Combo_OotRequestWallmasterDeparture(uint32_t mmId) {
    return SOH_RequestEntranceDeparture(gSaveContext.fileNum, 1, mmId, sCrossAge ? 1 : 0);
}
extern "C" COMBO_EXPORT int Combo_OotFinishEntranceDeparture(void* state) {
    auto* play = static_cast<PlayState*>(state);
    if (!play || play != gPlayState)
        return 0;
    const int horse = AREG(6);
    const u8 button = gSaveContext.equips.buttonItems[0];
    const bool swordless = Flags_GetInfTable(INFTABLE_SWORDLESS) != 0;
    const auto end = sOotEntranceFlight.finish(
        gSaveContext.fileNum, 0, play,
        [play](const auto&) {
            if ((gSaveContext.equips.equipment & 0xf) > 3)
                return false;
            // Exact pinned Horse_ForceUnmount, then Minigame_Fix, before the full save.
            if (AREG(6) != 0) {
                AREG(6) = 0;
                OotReloadEntranceSword(play);
            }
            if (play->sceneNum == SCENE_BOMBCHU_BOWLING_ALLEY && Flags_GetSwitch(play, 0x38))
                OotReloadEntranceSword(play);
            // Pinned Save_DoSave(SF_OWL) (oot/save.c:398-414, from comboGameSwitch, common/switch.c:75-85): the
            // scene's live chest, switch, clear and collect flags (PlayStoreFlags) and the scene number, as the pause
            // menu's save does. Play_Destroy stores them only after this save, so without it they would be lost.
            Play_SaveSceneFlags(play);
            gSaveContext.savedSceneNum = play->sceneNum;
            SaveManager::Instance->SaveFile(gSaveContext.fileNum);
            SaveManager::Instance->ThreadPoolWait();
            return true;
        },
        [play, horse, button, swordless](const auto&) {
            AREG(6) = horse;
            gSaveContext.equips.buttonItems[0] = button;
            if (swordless)
                Flags_SetInfTable(INFTABLE_SWORDLESS);
            else
                Flags_UnsetInfTable(INFTABLE_SWORDLESS);
            Interface_LoadItemIcon1(play, 0);
            SaveManager::Instance->SaveFile(gSaveContext.fileNum);
            SaveManager::Instance->ThreadPoolWait();
        });
    // A pending departure refused here keeps the native transition (D7); nothing queued is the ordinary case.
    if (CEArrival::DepartureRefused(end))
        SPDLOG_WARN("[ComboShip] OoT entrance departure refused at the end of its transition: {}; using the native "
                    "transition",
                    CEArrival::DepartureEndText(end));
    return end == CEArrival::DepartureEnd::Departed;
}
extern "C" COMBO_EXPORT int SOH_ConsumeEntranceArrival() {
    const auto arrival = sOotEntranceFlight.takeArrival(gSaveContext.fileNum, 0);
    if (!arrival)
        return -1;
    if (!SOH_CanEntranceArrival(arrival->file, arrival->id, arrival->age, arrival->crossAge))
        return -1;
    if (arrival->crossAge && !SOH_ApplySavedHumanAge(arrival->file, arrival->age))
        return -1;
    const auto target = CEArrival::ResolveOotNativeArrival(arrival->id);
    if (!target)
        return -1;
    if (target->grotto) {
        if (!Grotto_SetComboNativeArrival(target->grotto->nativeOffset, target->returning ? 1 : 0))
            return -1;
        if (target->returning)
            CEArrival::ApplyOotNativeGrottoReturn(*target->grotto, gSaveContext.respawn, gSaveContext.respawnFlag,
                                                  gSaveContext.nextTransitionType);
        else
            CEArrival::PrepareOotNativeGrottoLoad(*target->grotto, gSaveContext.respawn[RESPAWN_MODE_RETURN]);
    }
    return static_cast<int>(target->entrance);
}
