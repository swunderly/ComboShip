#pragma once
// Port of pinned OoTMM adultmask.c; included after native player overlay definitions.
// Resource/age seams below intentionally remain unresolved until exact extractor resources exist.
#include "ComboAdultMMSeams.h"
int Combo_MM_AdultMaskAction(void) { return 0x5b; } // Exact pinned PLAYER_CUSTOM_IA_MASK_ADULT.
#define ADULT_MASK_SKIP_BUTTONS (BTN_A | BTN_B | BTN_CUP | BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT)
#define ADULT_MASK_ITEM_ICON_SIZE 0x1000
static PlayerAgeProperties sAdultMaskChildProperties;
static PlayerAnimationHeader* sAdultMaskChildDoorA[PLAYER_ANIMTYPE_MAX];
static PlayerAnimationHeader* sAdultMaskChildDoorB[PLAYER_ANIMTYPE_MAX];
static int sAdultMaskOverlayDefaultsValid;
static void AdultMask_ApplyNativeOverlayAge(int adult) {
    PlayerAnimationHeader** table=(PlayerAnimationHeader**)D_8085BE84;
    if(!sAdultMaskOverlayDefaultsValid) {
        sAdultMaskChildProperties=sPlayerAgeProperties[PLAYER_FORM_HUMAN];
        memcpy(sAdultMaskChildDoorA,table+0x36,sizeof(sAdultMaskChildDoorA));
        memcpy(sAdultMaskChildDoorB,table+0x42,sizeof(sAdultMaskChildDoorB));
        sAdultMaskOverlayDefaultsValid=1;
    }
    if(adult){Combo_MM_CopyAdultProperties(&sPlayerAgeProperties[PLAYER_FORM_HUMAN]);
        for(int i=0;i<PLAYER_ANIMTYPE_MAX;++i){table[0x36+i]=(PlayerAnimationHeader*)&gPlayerAnim_pz_doorA_open;table[0x42+i]=(PlayerAnimationHeader*)&gPlayerAnim_pz_doorB_open;}
    }else{sPlayerAgeProperties[PLAYER_FORM_HUMAN]=sAdultMaskChildProperties;
        memcpy(table+0x36,sAdultMaskChildDoorA,sizeof(sAdultMaskChildDoorA));memcpy(table+0x42,sAdultMaskChildDoorB,sizeof(sAdultMaskChildDoorB));}
}
void AdultMask_Action(Player* player, PlayState* play);

typedef enum {
    ADULT_MASK_CS_NONE,
    ADULT_MASK_CS_PUT_ON,
    ADULT_MASK_CS_TAKE_OFF_TRANSFORM,
    ADULT_MASK_CS_ENDING_ANIM,
} AdultMaskCutsceneMode;

typedef enum {
    ADULT_MASK_WHITE_FILL_NONE,
    ADULT_MASK_WHITE_FILL_IN,
    ADULT_MASK_WHITE_FILL_HOLD,
    ADULT_MASK_WHITE_FILL_OUT,
} AdultMaskWhiteFillMode;

static AdultMaskCutsceneMode sAdultMaskCutsceneMode;
static AdultMaskWhiteFillMode sAdultMaskWhiteFillMode;
static AdjLightSettings sAdultMaskSavedAdjLightSettings;
static s32 sAdultMaskSavedAdjLightSettingsValid;
static s32 sAdultMaskTargetAdult;
static s32 sAdultMaskCommitted;
static s32 sAdultMaskTimer;
static s32 sAdultMaskPlayedFlashSfx;
static s32 sAdultMaskDrawTransformFace;
static s32 sAdultMaskPlayerReloadStarted;
static s32 sAdultMaskPendingReloadAfterWhiteIn;
static s32 sAdultMaskInterrupted;
static s32 sAdultMaskReloadBusy;

static void AdultMask_ClearState(void);
static void AdultMask_StartEndingAnim(Player* player, PlayState* play);
static void AdultMask_PlayerReloadUpdateWrapper(Actor* thisx, PlayState* play);



void AdultMask_DrawMaskInHand(PlayState* play, Player* player) { Combo_MM_DrawAdultMask(play, player, 0); }
void AdultMask_DrawMaskOnFaceNativeLike(PlayState* play, Player* player) { Combo_MM_DrawAdultMask(play, player, 1); }
void AdultMask_DrawTransformationMaskOnFace(PlayState* play, Player* player) { Combo_MM_DrawAdultMask(play, player, 2); }

s32 AdultMask_IsActive(void) { return sAdultMaskCutsceneMode != ADULT_MASK_CS_NONE; }
s32 AdultMask_IsPuttingOn(void) { return sAdultMaskCutsceneMode == ADULT_MASK_CS_PUT_ON; }
s32 AdultMask_IsTakeOffTransform(void) { return sAdultMaskCutsceneMode == ADULT_MASK_CS_TAKE_OFF_TRANSFORM; }
s32 AdultMask_IsTakeOffChild(void) { return sAdultMaskCutsceneMode == ADULT_MASK_CS_ENDING_ANIM; }
s32 AdultMask_IsTakingOff(void) { return AdultMask_IsTakeOffTransform() || AdultMask_IsTakeOffChild(); }
s32 AdultMask_IsCsItem(Player* player) { return player->itemAction == Combo_MM_AdultMaskAction(); }
s32 AdultMask_ShouldDrawAdultModel(void) { return Combo_MM_IsAdult(); }
s32 AdultMask_GetTimer(void) { return sAdultMaskTimer; }
s32 AdultMask_ShouldDrawTransformFace(void) { return sAdultMaskDrawTransformFace; }

static void AdultMask_SetB10(Player* player, f32 value)
{
    player->unk_B10[2] = value;
    player->unk_B10[3] = value;
    player->unk_B10[4] = value;
    player->unk_B10[5] = value;
}

static void AdultMask_StopTransformLoopSfx(void)
{
    AudioSfx_StopById(NA_SE_PL_TRANSFORM_VOICE);
    AudioSfx_StopById(NA_SE_IT_TRANSFORM_MASK_BROKEN);
    AudioSfx_StopById(NA_SE_PL_FACE_CHANGE);
    Play_DisableMotionBlurPriority();
}

static void AdultMask_SaveEnvVisualState(PlayState* play)
{
    if (sAdultMaskSavedAdjLightSettingsValid)
        return;
    sAdultMaskSavedAdjLightSettings = play->envCtx.adjLightSettings;
    sAdultMaskSavedAdjLightSettingsValid = 1;
}

static void AdultMask_RestoreEnvVisualState(PlayState* play)
{
    if (sAdultMaskSavedAdjLightSettingsValid)
        play->envCtx.adjLightSettings = sAdultMaskSavedAdjLightSettings;
}

static void AdultMask_ApplyRuntimeAgeProperties(Player* player, PlayState* play)
{
    f32 height;
    player->ageProperties = &sPlayerAgeProperties[PLAYER_FORM_HUMAN];
    func_80123140(play, player);
    height = Player_GetHeight(player);

    player->actor.focus.pos.y = player->actor.world.pos.y + height;
    player->cylinder.dim.height = height;
    player->cylinder.dim.yShift = 0;
    player->cylinder.dim.radius = player->ageProperties->wallCheckRadius;
    player->actor.cullingVolumeDistance = 4000.0f;
    player->actor.cullingVolumeScale = height;
    player->actor.cullingVolumeDownward = height;
}

static void AdultMask_RefreshButtonIcons(PlayState* play)
{
    for(int i=0;i<4;++i)Interface_Dpad_LoadItemIconImpl(play,i);
    s32 button;

    for (button = EQUIP_SLOT_C_LEFT; button <= EQUIP_SLOT_C_RIGHT; button++) {
        u8 item = BUTTON_ITEM_EQUIP(0, button);

        if (item != ITEM_NONE && item != 0xff) {
            Interface_LoadItemIconImpl(play, button);
        } else {
            u8* iconBuffer = (u8*)play->interfaceCtx.iconItemSegment + button * ADULT_MASK_ITEM_ICON_SIZE;
            memset(iconBuffer, 0, ADULT_MASK_ITEM_ICON_SIZE);
        }
    }
}

void AdultMask_ResetDrawStateForNewPlay(void)
{
    sAdultMaskReloadBusy = 0;
    AdultMask_ClearState();
    EponaAge_ClearReload();
}

static s32 AdultMask_CanUse(Player* player, PlayState* play)
{
    return Combo_MM_AdultResourcesReady() && Combo_MM_AdultCanSwapAge() && !sAdultMaskReloadBusy &&
           player->transformation == PLAYER_FORM_HUMAN &&
           player->actor.draw != NULL &&
           !Player_InCsMode(play) &&
           !(player->stateFlags1 & 0x207c7080) &&
           !(player->stateFlags3 & 0x1000);
}

s32 AdultMask_TryUse(Player* player, PlayState* play, s32 itemAction)
{
    s32 fastMasks;
    if (itemAction != Combo_MM_AdultMaskAction())
        return 0;
    fastMasks = CVarGetInteger("gEnhancements.Masks.FastTransformation", 0);
    if (fastMasks && sAdultMaskReloadBusy)
        return 1;

    if (!AdultMask_CanUse(player, play)) {
        Lib_PlaySfx_2(0x4806);
        return 1;
    }
    if (fastMasks) {
        AdultMask_StartCsItem(player, play);
        return 1;
    }

    player->unk_AA5 = 5;
    player->itemAction = itemAction;
    Combo_MM_ClearCustomMask();
    gSaveContext.save.equippedMask = 0;
    player->currentMask = 0;
    return 1;
}

static void AdultMask_SetWhiteFillAlpha(PlayState* play, s32 alpha)
{
    if (alpha < 0)
        alpha = 0;
    else if (alpha > 255)
        alpha = 255;
    if (alpha > 0)
        Play_FillScreen(play, 45, 220, 220, 220, alpha);
    else {
        R_PLAY_FILL_SCREEN_ON = 0;
        R_PLAY_FILL_SCREEN_ALPHA = 0;
    }
}

static void AdultMask_StartWhiteFillIn(void)
{
    if (sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_IN || sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_HOLD)
        return;

    sAdultMaskWhiteFillMode = ADULT_MASK_WHITE_FILL_IN;
    R_PLAY_FILL_SCREEN_ON = 45;
    R_PLAY_FILL_SCREEN_R = R_PLAY_FILL_SCREEN_G = R_PLAY_FILL_SCREEN_B = 220;
    R_PLAY_FILL_SCREEN_ALPHA = 0;
}

static void AdultMask_StartWhiteFillOut(void)
{
    sAdultMaskWhiteFillMode = ADULT_MASK_WHITE_FILL_OUT;
    R_PLAY_FILL_SCREEN_ON = 45;
    R_PLAY_FILL_SCREEN_R = R_PLAY_FILL_SCREEN_G = R_PLAY_FILL_SCREEN_B = 220;
    R_PLAY_FILL_SCREEN_ALPHA = 255;
}

static s32 AdultMask_UpdateWhiteFill(PlayState* play)
{
    s32 alpha;

    if (sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_IN) {
        alpha = R_PLAY_FILL_SCREEN_ALPHA + 45;
        if (alpha >= 255) {
            sAdultMaskWhiteFillMode = ADULT_MASK_WHITE_FILL_HOLD;
            AdultMask_SetWhiteFillAlpha(play, 255);
            return 1;
        }
        AdultMask_SetWhiteFillAlpha(play, alpha);
    } else if (sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_OUT) {
        alpha = R_PLAY_FILL_SCREEN_ALPHA - 45;
        if (alpha <= 0) {
            sAdultMaskWhiteFillMode = ADULT_MASK_WHITE_FILL_NONE;
            AdultMask_SetWhiteFillAlpha(play, 0);
            return 1;
        }
        AdultMask_SetWhiteFillAlpha(play, alpha);
    }

    return 0;
}

static void AdultMask_ClearFill(void)
{
    sAdultMaskWhiteFillMode = ADULT_MASK_WHITE_FILL_NONE;
    R_PLAY_FILL_SCREEN_ON = 0;
    R_PLAY_FILL_SCREEN_ALPHA = 0;
}

static void AdultMask_ResetRunState(void)
{
    sAdultMaskTimer = 0;
    sAdultMaskPlayedFlashSfx = 0;
    sAdultMaskDrawTransformFace = 0;
    sAdultMaskPlayerReloadStarted = 0;
    sAdultMaskPendingReloadAfterWhiteIn = 0;
    sAdultMaskInterrupted = 0;
    AdultMask_ClearFill();
}

static void AdultMask_ResetPlayerVisualState(Player* player)
{
    player->av1.actionVar1 = 0;
    player->av2.actionVar2 = 0;
    AdultMask_SetB10(player, 0.0f);
}

static void AdultMask_ClearState(void)
{
    sAdultMaskCutsceneMode = ADULT_MASK_CS_NONE;
    sAdultMaskTargetAdult = 0;
    sAdultMaskCommitted = 0;
    sAdultMaskSavedAdjLightSettingsValid = 0;
    AdultMask_ResetRunState();
}

static void AdultMask_CommitAge(Player* player)
{
    if (sAdultMaskCommitted)
        return;

    if(!Combo_MM_AdultSwapAge(gPlayState))return;
    sAdultMaskCommitted = 1;

    /* Reset FW spawn */
    Combo_MM_AdultRestoreFaroreRespawn();

    /* Reset mask */
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    player->transformation = PLAYER_FORM_HUMAN;
    player->currentMask = 0;
    gSaveContext.save.equippedMask = 0;
    Combo_MM_ClearCustomMask();
}

static void AdultMask_EndCutsceneCamera(Player* player)
{
    Player_StopCutscene(player);
    func_8082DAD4(player);
    player->csAction = 0;
    player->prevCsAction = 0;
    player->csId = CS_ID_NONE;
    player->subCamId = SUB_CAM_ID_DONE;
}

static void AdultMask_ClearHeldMaskAndUnlock(Player* player)
{
    player->itemAction = PLAYER_IA_NONE;
    player->heldItemAction = PLAYER_IA_NONE;
    player->stateFlags1 &= ~0x30000000;
    player->stateFlags3 &= ~0x80;
}

static void AdultMask_Finish(Player* player, PlayState* play)
{
    Play_DisableMotionBlurPriority();
    AdultMask_EndCutsceneCamera(player);
    AdultMask_ClearHeldMaskAndUnlock(player);
    func_808369F4(player, play);
    AdultMask_RestoreEnvVisualState(play);
    AdultMask_ClearState();
}

static s32 AdultMask_HasHorseActor(PlayState* play)
{
    s32 category;
    Actor* actor;

    for (category = 0; category < ACTORCAT_MAX; category++) {
        for (actor = play->actorCtx.actorLists[category].first; actor != NULL; actor = actor->next) {
            if (actor->id == ACTOR_EN_HORSE)
                return 1;
        }
    }

    return 0;
}

static void AdultMask_CommitAgeApplyTablesAndReloadPlayer(Player* player, PlayState* play)
{
    s32 fastMasks;
    if (sAdultMaskPlayerReloadStarted)
        return;
    fastMasks = CVarGetInteger("gEnhancements.Masks.FastTransformation", 0);
    sAdultMaskReloadBusy = 1;
    sAdultMaskPlayerReloadStarted = 1;
    player->actor.draw = NULL;
    AdultMask_StopTransformLoopSfx();
    AdultMask_RestoreEnvVisualState(play);
    AdultMask_CommitAge(player);
    if(!sAdultMaskCommitted){
        // A bridge rejection must restore visible/native control without a false commit.
        sAdultMaskReloadBusy = 0;
        AdultMask_Finish(player,play);AdultMask_ClearFill();
        player->actor.draw=Player_Draw;player->actor.update=Player_Update;
        return;
    }
    Combo_MM_ApplyAdultGameplayKeep(play, sAdultMaskTargetAdult);
    if (!fastMasks || AdultMask_HasHorseActor(play))
        EponaAge_Request(play, sAdultMaskTargetAdult);
    else
        EponaAge_ClearReload();
    Combo_MM_ApplyAdultModelTables(sAdultMaskTargetAdult);
    AdultMask_ApplyNativeOverlayAge(sAdultMaskTargetAdult);
    if (sAdultMaskCutsceneMode != ADULT_MASK_CS_NONE)
        AdultMask_EndCutsceneCamera(player);
    AdultMask_ClearHeldMaskAndUnlock(player);
    AdultMask_ResetPlayerVisualState(player);
    player->actor.update = AdultMask_PlayerReloadUpdateWrapper;
    player->actor.draw = NULL;

    if (sAdultMaskCutsceneMode != ADULT_MASK_CS_TAKE_OFF_TRANSFORM && sAdultMaskCutsceneMode != ADULT_MASK_CS_PUT_ON)
        sAdultMaskCutsceneMode = ADULT_MASK_CS_NONE;
}

static void AdultMask_PlayerPostReloadUpdateWrapper(Actor* thisx, PlayState* play)
{
    Player* player = (Player*)thisx;
    s32 fastMasks = CVarGetInteger("gEnhancements.Masks.FastTransformation", 0);
    u8 sharedBItem = gSaveContext.save.saveInfo.equips.buttonItems[0][EQUIP_SLOT_B];
    u8 sharedBSlot = gSaveContext.save.saveInfo.equips.cButtonSlots[0][EQUIP_SLOT_B];
    u16 sharedEquipment = gSaveContext.save.saveInfo.equips.equipment;

    player->actor.draw = NULL;
    AdultMask_ApplyRuntimeAgeProperties(player, play);
    Player_Update(thisx, play);
    AdultMask_ApplyRuntimeAgeProperties(player, play);
    player->actor.draw = NULL;

    if (EponaAge_IsBusy() && (!fastMasks || AdultMask_HasHorseActor(play))) {
        if (fastMasks) {
            AdultMask_ClearFill();
        } else {
            AdultMask_SetWhiteFillAlpha(play, 255);
            sAdultMaskWhiteFillMode = ADULT_MASK_WHITE_FILL_HOLD;
        }
        return;
    }

    if (fastMasks)
        AdultMask_ClearFill();
    gSaveContext.save.saveInfo.equips.equipment = sharedEquipment;
    gSaveContext.save.saveInfo.equips.buttonItems[0][EQUIP_SLOT_B] = sharedBItem;
    gSaveContext.save.saveInfo.equips.cButtonSlots[0][EQUIP_SLOT_B] = sharedBSlot;

    func_80123140(play, player);
    Combo_MM_AdultCheckBoots(play);
    Player_SetEquipmentData(play, player);
    AdultMask_RefreshButtonIcons(play);

    player->actor.update = (ActorFunc)Player_Update;
    player->actor.draw = Player_Draw;
    sAdultMaskReloadBusy = 0;

    if (sAdultMaskCutsceneMode == ADULT_MASK_CS_TAKE_OFF_TRANSFORM || sAdultMaskCutsceneMode == ADULT_MASK_CS_PUT_ON)
        AdultMask_StartEndingAnim(player, play);
    else
        AdultMask_ClearState();
}

static void AdultMask_PlayerReloadUpdateWrapper(Actor* thisx, PlayState* play)
{
    Player* player = (Player*)thisx;

    func_8012301C(thisx, play);
    if (player->actor.update == AdultMask_PlayerReloadUpdateWrapper)
        return;
    player->actor.update = AdultMask_PlayerPostReloadUpdateWrapper;
    player->actor.draw = NULL;
    AdultMask_ApplyRuntimeAgeProperties(player, play);
}

static u8 AdultMask_GetFierceDeityRingAlpha(void)
{
    s32 ringFrame;

    if (sAdultMaskCutsceneMode == ADULT_MASK_CS_PUT_ON) {
        if (sAdultMaskTimer < 54)
            return 0;
        ringFrame = sAdultMaskTimer - 54;
        if (ringFrame < 12)
            return 255;
        return ringFrame < 36 ? (u8)((36 - ringFrame) * (255.0f / 24.0f)) : 0;
    }

    if (sAdultMaskCutsceneMode == ADULT_MASK_CS_TAKE_OFF_TRANSFORM) {
        if (sAdultMaskTimer < 0x10)
            return 0;
        ringFrame = sAdultMaskTimer - 0x10;
        if (ringFrame < 0x28)
            return 255;
        ringFrame -= 0x28;
        return ringFrame < 0x0c ? (u8)((0x0c - ringFrame) * (255.0f / 0x0c)) : 0;
    }

    return 0;
}

void AdultMask_DrawTransformRing(PlayState* play, Player* player)
{
    u8 alpha = AdultMask_GetFierceDeityRingAlpha();
    if (alpha == 0)
        return;

    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    AnimatedMat_DrawXlu(play, Lib_SegmentedToVirtual(gameplay_keep_Matanimheader_054F18));
    Matrix_Translate(-230.0f, -520.0f, 0.0f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gDPSetEnvColor(POLY_XLU_DISP++, 0, 0, 255, alpha);
    gSPDisplayList(POLY_XLU_DISP++, gameplay_keep_DL_054C90);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
    (void)player;
}

static void AdultMask_UpdateEffectSteps(Player* player, struct_8085D910* cfg, s32 playLightningSfx)
{
    s32 frame = player->av1.actionVar1;
    if (frame >= cfg->unk_0) {
        if (frame < cfg->unk_2) {
            Math_StepToF(&player->unk_B10[4], 1.0f, cfg->unk_1 / 100.0f);
        } else if (frame < cfg->unk_3) {
            if (playLightningSfx && frame == cfg->unk_2)
                Lib_PlaySfx_2(NA_SE_EV_LIGHTNING_HARD);
            Math_StepToF(&player->unk_B10[4], 2.0f, 0.5f);
        } else {
            Math_StepToF(&player->unk_B10[4], 3.0f, 0.2f);
        }
    }

    if (frame >= 0x10) {
        if (frame < 0x37)
            Math_StepToF(&player->unk_B10[5], 1.0f, 0.2f);
        else if (frame < 0x40)
            Math_StepToF(&player->unk_B10[5], 2.0f, 1.0f);
        else
            Math_StepToF(&player->unk_B10[5], 3.0f, 0.55f);
    }
}

static void AdultMask_UpdateTransformCameraAndEffects(Player* player, PlayState* play, s32 form, s32 camMode, s32 drawArg, s32 playLightningSfx)
{
    struct_8085D910* cfg = &D_8085D910[0];
    Camera* cam;
    s32 prevPlayerForm;
    s32 prevSaveForm;
    func_808323C0(player, player->csId);
    cam = Play_GetCamera(play, play->activeCamId);
    Camera_ChangeMode(cam, camMode);
    player->stateFlags2 |= PLAYER_STATE2_40;
    player->actor.shape.rot.y = Camera_GetCamDirYaw(cam) + 0x8000;
    prevPlayerForm = player->transformation;
    prevSaveForm = gSaveContext.save.playerForm;
    player->transformation = form;
    gSaveContext.save.playerForm = form;
    func_80855218(play, player, &cfg);
    player->transformation = prevPlayerForm;
    gSaveContext.save.playerForm = prevSaveForm;
    if (!sAdultMaskInterrupted && player->av1.actionVar1 == 0x14)
        Play_EnableMotionBlurPriority(100);
    sAdultMaskTimer = ++player->av1.actionVar1;
    AdultMask_UpdateEffectSteps(player, cfg, playLightningSfx);
    func_808550D0(play, player, player->unk_B10[4], player->unk_B10[5], drawArg);
}

static s32 AdultMask_ShouldSkipTransformLoop(PlayState* play)
{
    return CHECK_BTN_ANY(play->state.input[0].press.button, ADULT_MASK_SKIP_BUTTONS);
}

static void AdultMask_SkipTransformLoopToFlash(Player* player, PlayState* play, s32 flashFrame)
{
    s16 currentCsId;
    s16 subCamId;
    if (player->av1.actionVar1 >= flashFrame)
        return;
    sAdultMaskInterrupted = 1;
    AdultMask_StopTransformLoopSfx();
    currentCsId = CutsceneManager_GetCurrentCsId();
    if (currentCsId == player->csId) {
        subCamId = CutsceneManager_GetCurrentSubCamId(player->csId);
        if (subCamId != SUB_CAM_ID_DONE && subCamId != CS_ID_NONE)
            func_800E0348(Play_GetCamera(play, subCamId));
    }
    AdultMask_RestoreEnvVisualState(play);
    AdultMask_SetB10(player, 0.0f);
    player->av1.actionVar1 = flashFrame;
    sAdultMaskTimer = flashFrame;
}

static void AdultMask_StartReloadFlash(Player* player)
{
    if (!sAdultMaskPlayedFlashSfx) {
        sAdultMaskPlayedFlashSfx = 1;
        AdultMask_StopTransformLoopSfx();
        Player_PlaySfx(player, NA_SE_SY_TRANSFORM_MASK_FLASH);
    }

    AdultMask_StartWhiteFillIn();
    sAdultMaskPendingReloadAfterWhiteIn = 1;
}

static void AdultMask_UpdatePutOn(Player* player, PlayState* play)
{
    if (AdultMask_ShouldSkipTransformLoop(play))
        AdultMask_SkipTransformLoopToFlash(player, play, 0x54);

    if (BEN_ANIM_EQUAL(player->skelAnime.animation, &gPlayerAnim_cl_setmask) && player->skelAnime.curFrame >= 51)
        sAdultMaskDrawTransformFace = 1;

    if (sAdultMaskTimer < 0x54 && sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_NONE)
        AdultMask_UpdateTransformCameraAndEffects(player, play, PLAYER_FORM_FIERCE_DEITY, CAM_MODE_NORMAL, 0, 1);

    if (sAdultMaskTimer >= 0x54)
        AdultMask_StartReloadFlash(player);
}

static s32 AdultMask_UpdateTakeOffTransform(Player* player, PlayState* play)
{
    if (AdultMask_ShouldSkipTransformLoop(play))
        AdultMask_SkipTransformLoopToFlash(player, play, 0x45);
    if (sAdultMaskTimer < 0x45 && sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_NONE)
        AdultMask_UpdateTransformCameraAndEffects(player, play, PLAYER_FORM_ZORA, CAM_MODE_JUMP, 1, 0);
    if (sAdultMaskTimer < 0x45)
        return 0;
    AdultMask_StartReloadFlash(player);
    return 1;
}

static void AdultMask_UpdateEndingAnim(Player* player, PlayState* play)
{
    sAdultMaskTimer++;

    if (sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_HOLD)
        AdultMask_StartWhiteFillOut();
    if (sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_OUT)
        AdultMask_UpdateWhiteFill(play);
    if (!PlayerAnimation_Update(play, &player->skelAnime))
        return;
    PlayerAnimation_Change(play, &player->skelAnime, Player_GetIdleAnim(player), 1, 0.0f, 20.0f, ANIMMODE_ONCE, 20.0f);
    AdultMask_Finish(player, play);
}

static void AdultMask_StartEndingAnim(Player* player, PlayState* play)
{
    PlayerAnimationHeader* endingAnim = sAdultMaskTargetAdult ? &gPlayerAnim_cl_setmaskend : Combo_MM_AdultMaskOffAnimation();
    sAdultMaskCutsceneMode = ADULT_MASK_CS_ENDING_ANIM;
    sAdultMaskTimer = 0;
    sAdultMaskPlayerReloadStarted = 0;
    sAdultMaskWhiteFillMode = ADULT_MASK_WHITE_FILL_HOLD;
    AdultMask_SetWhiteFillAlpha(play, 255);
    AdultMask_ResetPlayerVisualState(player);
    player->itemAction = Combo_MM_AdultMaskAction();
    player->heldItemAction = Combo_MM_AdultMaskAction();
    func_8082DE50(play, player);
    Player_SetAction_PreserveItemAction(play, player, AdultMask_Action, 0);
    Player_Anim_PlayOnceAdjusted(play, player, endingAnim);
    player->stateFlags1 |= 0x30000000;
    player->stateFlags3 |= 0x80;
}

void AdultMask_Action(Player* player, PlayState* play)
{
    player->actor.gravity = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->stateFlags1 |= 0x30000000;
    player->stateFlags3 |= 0x80;
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    player->transformation = PLAYER_FORM_HUMAN;
    player->itemAction = Combo_MM_AdultMaskAction();

    if (sAdultMaskPlayerReloadStarted)
        return;

    if (sAdultMaskWhiteFillMode != ADULT_MASK_WHITE_FILL_NONE) {
        Play_DisableMotionBlurPriority();
        AdultMask_SetB10(player, 0.0f);
    }

    if ((sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_IN || sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_OUT) &&
        AdultMask_UpdateWhiteFill(play) &&
        sAdultMaskPendingReloadAfterWhiteIn &&
        sAdultMaskWhiteFillMode == ADULT_MASK_WHITE_FILL_HOLD) {
        sAdultMaskPendingReloadAfterWhiteIn = 0;
        AdultMask_CommitAgeApplyTablesAndReloadPlayer(player, play);
        return;
    }

    if (sAdultMaskCutsceneMode == ADULT_MASK_CS_PUT_ON) {
        AdultMask_UpdatePutOn(player, play);
        if (sAdultMaskPendingReloadAfterWhiteIn)
            return;
    } else if (sAdultMaskCutsceneMode == ADULT_MASK_CS_TAKE_OFF_TRANSFORM) {
        if (AdultMask_UpdateTakeOffTransform(player, play) || sAdultMaskPlayerReloadStarted)
            return;
    } else if (sAdultMaskCutsceneMode == ADULT_MASK_CS_ENDING_ANIM) {
        AdultMask_UpdateEndingAnim(player, play);
    } else {
        AdultMask_Finish(player, play);
        return;
    }

    Player_DecelerateToZero(player);
    if (sAdultMaskCutsceneMode != ADULT_MASK_CS_ENDING_ANIM)
        func_8083249C(player);
}

static void AdultMask_SetStartCamera(Player* player, PlayState* play, s32 camMode)
{
    Camera* cam;

    player->csId = play->playerCsIds[5];
    func_808323C0(player, player->csId);
    cam = Play_GetCamera(play, play->activeCamId);
    Camera_ChangeMode(cam, camMode);
    player->actor.shape.rot.y = Camera_GetCamDirYaw(cam) + 0x8000;
}

static void AdultMask_FastStartUpdateWrapper(Actor* thisx, PlayState* play)
{
    Player* player = (Player*)thisx;
    player->actor.draw = NULL;
    AdultMask_CommitAgeApplyTablesAndReloadPlayer(player, play);
}

void AdultMask_StartCsItem(Player* player, PlayState* play)
{
    if (!Combo_MM_AdultResourcesReady()) return;
    s32 isAdult = Combo_MM_IsAdult();
    s32 fastMasks = CVarGetInteger("gEnhancements.Masks.FastTransformation", 0);
    sAdultMaskTargetAdult = !isAdult;
    sAdultMaskCommitted = 0;
    AdultMask_ResetRunState();
    AdultMask_ResetPlayerVisualState(player);
    if (fastMasks) {
        sAdultMaskCutsceneMode = ADULT_MASK_CS_NONE;
        sAdultMaskSavedAdjLightSettingsValid = 0;
        sAdultMaskReloadBusy = 1;
        player->actor.gravity = 0.0f;
        player->actor.velocity.y = 0.0f;
        player->actor.draw = NULL;
        player->actor.update = AdultMask_FastStartUpdateWrapper;
        return;
    }

    sAdultMaskCutsceneMode = isAdult ? ADULT_MASK_CS_TAKE_OFF_TRANSFORM : ADULT_MASK_CS_PUT_ON;
    AdultMask_SaveEnvVisualState(play);
    player->actor.gravity = 0.0f;
    player->actor.velocity.y = 0.0f;
    func_8082DE50(play, player);
    Player_SetAction_PreserveItemAction(play, player, AdultMask_Action, 0);
    player->itemAction = Combo_MM_AdultMaskAction();

    if (sAdultMaskCutsceneMode == ADULT_MASK_CS_PUT_ON) {
        AdultMask_SetStartCamera(player, play, CAM_MODE_NORMAL);
        Player_Anim_PlayOnceMorphAdjusted(play, player, &gPlayerAnim_cl_setmask);
    } else {
        AdultMask_SetStartCamera(player, play, CAM_MODE_JUMP);
        Player_Anim_PlayOnceMorphAdjusted(play, player, &gPlayerAnim_pz_maskoffstart);
    }

    func_8083249C(player);
    player->stateFlags1 |= 0x30000000;
    player->stateFlags3 |= 0x80;
}

void AdultMask_AfterStart(Player* player)
{
    (void)player;
}

// Cross-engine pre-Player_Init table bridge; activation requires complete resources upstream.
void Combo_MM_ApplyAdultOverlayAge(int adult){AdultMask_ApplyNativeOverlayAge(adult);}

extern Gfx* Combo_MM_GetAdultMaskBinding(int index);
// Exact pinned hand/face transforms, using decoded native MM display lists and owners.
void Combo_MM_DrawAdultMask(PlayState* play, Player* player, int mode) {
    Gfx* dl;
    if(mode<0||mode>2||!Combo_MM_AdultResourcesReady())return;
    dl=Combo_MM_GetAdultMaskBinding(mode==2?2:3);if(dl==NULL)return;
    Matrix_Push();
    if(mode==0){
        Matrix_Translate(-323.67f,412.15f,-969.96f,MTXMODE_APPLY);
        Matrix_RotateZYX(-0x32BE,-0x50DE,-0x7717,MTXMODE_APPLY);
    }else if(mode==1){
        Matrix_Scale(1.0f,1.0f-player->unk_B10[3],1.0f-player->unk_B10[2],MTXMODE_APPLY);
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++,Matrix_Finalize(play->state.gfxCtx),G_MTX_NOPUSH|G_MTX_LOAD|G_MTX_MODELVIEW);
    // Decoded resources use owning-MM paths; no raw segmented custom object pointer remains.
    gSPDisplayList(POLY_OPA_DISP++,dl);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
}
