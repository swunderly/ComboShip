#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include "variables.h"
#include "src/overlays/actors/ovl_En_Door/z_en_door.h"
extern PlayState* gPlayState;
}

static constexpr int32_t CVAR_MQ_WATER_LOCK_DEFAULT = 0;
#define CVAR_MQ_WATER_LOCK_FIX_NAME CVAR_ENHANCEMENT("MQWaterLockFix")
#define CVAR_MQ_WATER_LOCK_VALUE CVarGetInteger(CVAR_MQ_WATER_LOCK_FIX_NAME, CVAR_MQ_WATER_LOCK_DEFAULT)

static void OnInitEnDoor(void* refActor) {
    EnDoor* enDoor = reinterpret_cast<EnDoor*>(refActor);
    if (gPlayState->sceneNum == SCENE_WATER_TEMPLE && ResourceMgr_IsGameMasterQuest() &&
#ifdef COMBO_BUILD
        // ComboShip: only a real oot-mq.o2r's door. OoTMM's bundled MQ Water already moved it to switch 2
        // (scripts/make_mq.ts:226-229), which dungeon.cpp counts for a bundled seed.
        !ResourceMgr_GameUsesBundledMq() &&
#endif
        enDoor->actor.params == 22659) {
        enDoor->actor.params = 22660;
    }
}

static void RegisterMQWaterLockFix() {
    COND_ID_HOOK(OnActorInit, ACTOR_EN_DOOR, IS_RANDO || CVAR_MQ_WATER_LOCK_VALUE, OnInitEnDoor);
}

static RegisterShipInitFunc initFunc(RegisterMQWaterLockFix, { CVAR_MQ_WATER_LOCK_FIX_NAME, "IS_RANDO" });
