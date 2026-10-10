#pragma once
// Ported from pinned OoTMM En_Horse.c, MIT; see 2s2h/ComboAdultUpstream.LICENSE.
// Native callbacks replace ROM-overlay patches only after exact imported resources exist.
extern int Combo_MM_AdultResourcesReady(void);
extern int Combo_MM_EponaVariantObjectId(int adult);
extern void Combo_MM_ApplyEponaVariantTables(int adult);
#define ENHORSE_REINIT_CLEAR_FLAGS (ENHORSE_JUMPING | ENHORSE_CALC_RIDER_POS | ENHORSE_OBSTACLE | ENHORSE_TURNING_TO_PLAYER | ENHORSE_DRAW | ENHORSE_FLAG_30)
typedef struct {
    s32 active;
    EnHorse* horse;
    ActorFunc update;
    ActorFunc draw;
    u32 stateFlags;
} EponaReloadContext;

static EponaReloadContext sEponaReload;
void EponaAge_ClearReload(void)
{
    memset(&sEponaReload, 0, sizeof(sEponaReload));
}

static s32 EponaAge_IsValidHorse(Actor* actor)
{
    return actor != NULL &&
           actor->id == ACTOR_EN_HORSE &&
           actor->init == NULL &&
           actor->update != NULL &&
           ((EnHorse*)actor)->type == HORSE_TYPE_2;
}

static void EponaAge_StopActor(Actor* actor)
{
    actor->speed = 0.0f;
    actor->velocity.x = 0.0f;
    actor->velocity.y = 0.0f;
    actor->velocity.z = 0.0f;
}

static void EponaAge_Reinit(
    Actor* thisx,
    PlayState* play
);

static EnHorse* EponaAge_FindHorse(PlayState* play)
{
    ActorListEntry* lists;
    Actor* actor;

    actor = GET_PLAYER(play)->rideActor;
    if (EponaAge_IsValidHorse(actor))
        return (EnHorse*)actor;

    lists = play->actorCtx.actorLists;

    for (actor = lists[ACTORCAT_BG].first; actor != NULL; actor = actor->next) {
        if (EponaAge_IsValidHorse(actor))
            return (EnHorse*)actor;
    }

    return NULL;
}

static void EponaAge_Restore(EnHorse* horse, PlayState* play)
{
    Actor* actor = &horse->actor;
    Player* player = GET_PLAYER(play);
    s32 mounted = player->rideActor == actor;
    horse->stateFlags = sEponaReload.stateFlags & ~ENHORSE_REINIT_CLEAR_FLAGS;
    horse->postDrawFunc = NULL;
    horse->animIndex = ENHORSE_ANIM_IDLE;
    horse->curFrame = horse->skin.skelAnime.curFrame;
    EponaAge_StopActor(actor);
    actor->gravity = -3.5f;
    horse->playerControlled = mounted;
    if (mounted) {
        horse->stateFlags &= ~ENHORSE_INACTIVE;
        horse->action = ENHORSE_ACTION_MOUNTED_IDLE;
        player->rideActor = actor;
    } else if (sEponaReload.stateFlags & ENHORSE_INACTIVE) {
        horse->stateFlags |= ENHORSE_INACTIVE;
        horse->action = ENHORSE_ACTION_INACTIVE;
    } else {
        horse->stateFlags &= ~ENHORSE_INACTIVE;
        horse->action = ENHORSE_ACTION_IDLE;
    }

    actor->update = sEponaReload.update;
    actor->draw = sEponaReload.draw;
}

static void EponaAge_Reinit(Actor* thisx, PlayState* play)
{
    EnHorse* horse = (EnHorse*)thisx;
    if (!sEponaReload.active || sEponaReload.horse != horse) {
        EponaAge_ClearReload();
        return;
    }
    func_8087D540(thisx, play);
    // Native actor initialization may be retried while its object is still loading.
    if (!Object_IsLoaded(&play->objectCtx, horse->objectSlot))return;
    EponaAge_Restore(horse, play);
    EponaAge_ClearReload();
}

void EponaAge_Request(PlayState* play, s32 targetAdult)
{
    EnHorse* horse;
    Actor* actor;
    s32 objectSlot;
    s16 objectId;
    s32 adult = targetAdult != 0;
    if (sEponaReload.active || !Combo_MM_AdultResourcesReady())
        return;
    horse = EponaAge_FindHorse(play);
    if (horse == NULL || (horse->unk_1EC & 1))
        return;
    actor = &horse->actor;
    objectSlot = actor->objectSlot;
    if (objectSlot <= OBJECT_SLOT_NONE)
        objectSlot = horse->objectSlot;
    if (objectSlot <= OBJECT_SLOT_NONE ||
        !Object_IsLoaded(&play->objectCtx, objectSlot)) {
        return;
        }
    objectId = Combo_MM_EponaVariantObjectId(adult);
    if(objectId <= OBJECT_UNSET_0 || objectId >= OBJECT_ID_MAX)return;
    sEponaReload.active = true;
    sEponaReload.horse = horse;
    sEponaReload.update = actor->update;
    sEponaReload.draw = actor->draw;
    sEponaReload.stateFlags = horse->stateFlags;
    EponaAge_StopActor(actor);
    actor->draw = NULL;
    actor->init = EponaAge_Reinit;
    horse->postDrawFunc = NULL;
    horse->stateFlags &= ~ENHORSE_DRAW;
    Skin_Free(&play->state, &horse->skin);
    Skin_Setup(&horse->skin);
    Combo_MM_ApplyEponaVariantTables(adult);
    func_8012F73C(&play->objectCtx, objectSlot, objectId);
    actor->objectSlot = objectSlot;
    horse->objectSlot = objectSlot;
    horse->animIndex = ENHORSE_ANIM_IDLE;
    horse->curFrame = 0.0f;
}

s32 EponaAge_IsBusy(void)
{
    return sEponaReload.active;
}


