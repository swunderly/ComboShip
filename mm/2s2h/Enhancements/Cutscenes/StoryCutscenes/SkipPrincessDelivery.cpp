#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "overlays/actors/ovl_En_Dnp/z_en_dnp.h"
}

#define CVAR_NAME "gEnhancements.Cutscenes.SkipStoryCutscenes"
#define CVAR CVarGetInteger(CVAR_NAME, 0)

// The released princess asks for her cutscene every frame until it starts, while queued events wait for
// Link to leave the bottle release. Queue the transition once per scene, or each queued copy reloads the
// chamber again in turn.
static bool sTransitionQueued = false;

void RegisterSkipPrincessDelivery() {
    COND_VB_SHOULD(VB_START_CUTSCENE, CVAR, {
        s16* csId = va_arg(args, s16*);
        if (gPlayState->sceneId == SCENE_DEKU_KING && *csId == 16 &&
            gPlayState->transitionTrigger == TRANS_TRIGGER_OFF) {
            if (!sTransitionQueued) {
                sTransitionQueued = true;

                // Set the flag that would normally be set after the dialogue
                SET_WEEKEVENTREG(WEEKEVENTREG_23_20);

                // Set up transition to Deku Palace throne room
                GameInteractor::Instance->events.emplace_back(
                    GIEventTransition{ .entrance = ENTRANCE(DEKU_KINGS_CHAMBER, 3),
                                       .cutsceneIndex = 0,
                                       .transitionTrigger = TRANS_TRIGGER_START,
                                       .transitionType = TRANS_TYPE_FADE_BLACK });
            }
            *should = false;
        }
    });
    // Unconditionally registered, so turning the skip off and on again mid-delivery cannot leave the flag set.
    COND_HOOK(OnSceneInit, true, [](s8 sceneId, s8 spawnNum) { sTransitionQueued = false; });
}

static RegisterShipInitFunc initFunc(RegisterSkipPrincessDelivery, { CVAR_NAME });