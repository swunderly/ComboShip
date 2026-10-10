#ifdef COMBO_BUILD
// Ported from MM ovl_En_Test7: native wing keyframes, feather physics and camera timing.
// Engine adapters are confined to camera, resources, environment and the final handoff.
#include "CrossOwl.h"
extern void SkinMatrix_SetRotateYXZ(MtxF*, s16, s16, s16);
extern float OTRGetAspectRatio(void);
extern u32 OTRGetGameRenderWidth(void);
extern u32 OTRGetGameRenderHeight(void);
extern void func_80082850(PlayState*, s16);
#include "CrossWarp.h"
#include "CrossWarpAudio.h"
#include "MMWarpAssets.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/framebuffer_effects.h"

// The framebuffer calls are the same ones used by MM's Play_DrawMotionBlur.
// Keep the history private to this departure and reset it after a resize.
static void CrossOwl_DrawMotionBlur(CrossOwl* this, PlayState* play) {
    if (this->timer <= 10 || this->timer > 100)
        return;
    const u32 width = OTRGetGameRenderWidth();
    const u32 height = OTRGetGameRenderHeight();
    OPEN_DISPS(play->state.gfxCtx);
    Gfx* head = POLY_OPA_DISP;
    Gfx* gfx = Graph_GfxPlusOne(head);
    gSPDisplayList(OVERLAY_DISP++, gfx);
    if (this->blurReady && this->blurWidth == width && this->blurHeight == height) {
        FB_DrawFromFramebuffer(&gfx, gBlurFrameBuffer, 120);
    }
    this->blurReady = true;
    this->blurWidth = width;
    this->blurHeight = height;
    this->blurCopied = false;
    FB_CopyToFramebuffer(&gfx, 0, gBlurFrameBuffer, true, &this->blurCopied);
    gSPEndDisplayList(gfx++);
    Graph_BranchDlist(head, gfx);
    POLY_OPA_DISP = gfx;
    CLOSE_DISPS(play->state.gfxCtx);
}

// The native MM Soaring screen wipe uses the same actor in effect-only mode.
static void DrawSoaringWipe(CrossOwl* this, PlayState* play) {
    Vtx* vertices = Combo_SOHWarpResource(gOceff6Vtx);
    const int count = Combo_SOHWarpVertexCount(gOceff6Vtx);
    if (!vertices || count <= 0)
        return;
    float distance = 1220.0f;
    const float aspect = OTRGetAspectRatio();
    if (aspect > 4.0f / 3.0f)
        distance /= aspect * .85f;
    const float z = this->timer < 32 ? Math_SinS(this->timer * 0x200) * distance : distance;
    const u8 alpha = this->timer >= 80 ? 12 * (100 - this->timer) : 255;
    for (int i = 1; i < count; i += 2)
        vertices[i].v.cn[3] = alpha;
    Camera* cam = Play_GetCamera(play, play->activeCamera);
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPush(POLY_XLU_DISP++, "mm-warp");
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    Matrix_Translate(cam->eye.x, cam->eye.y, cam->eye.z, MTXMODE_NEW);
    Matrix_Scale(.1f, .1f, .1f, MTXMODE_APPLY);
    Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
    Matrix_RotateX(0x708 * (M_PI / 0x8000), MTXMODE_APPLY);
    Matrix_Translate(0, 0, -z, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_DISPS(play->state.gfxCtx);
    Combo_SOHWarpMaterial(play, ovl_Oceff_Wipe6_Matanimheader_000338, this->timer);
    OPEN_DISPS(play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)Combo_SOHWarpResource(gOceff6DL));
    gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    Combo_SOHRestoreWarpMaterial(play, ovl_Oceff_Wipe6_Matanimheader_000338);
}
void Combo_SOHStartSoaringEffect(PlayState* play) {
    // Match the originating game's playback preference. A skipped performance
    // must not leave its 100-tick screen wipe over the map or departure.
    if (CVarGetInteger(CVAR_ENHANCEMENT("FastOcarinaPlayback"), 0))
        return;
    if (!Combo_SOHPrepareWarpAssets())
        return;
    Actor_Spawn(&play->actorCtx, play, ACTOR_COMBO_OWL_WARP, 0, 0, 0, 0, 0, 0, 0x100);
}
static void CWEnvironment_LerpAmbientColor(PlayState* play, Color_RGB8* color, f32 lerp) {
    u8* rgb = (u8*)color;
    for (int i = 0; i < 3; i++)
        play->envCtx.adjAmbientColor[i] = (rgb[i] - play->envCtx.lightSettings.ambientColor[i]) * lerp;
}
static void CWEnvironment_LerpFogColor(PlayState* play, Color_RGB8* color, f32 lerp) {
    u8* rgb = (u8*)color;
    for (int i = 0; i < 3; i++)
        play->envCtx.adjFogColor[i] = (rgb[i] - play->envCtx.lightSettings.fogColor[i]) * lerp;
}
static void CWEnvironment_LerpFog(PlayState* play, s16 nearTarget, s16 farTarget, f32 lerp) {
    play->envCtx.adjFogNear = (nearTarget - play->envCtx.lightSettings.fogNear) * lerp;
    play->envCtx.adjFogFar = (farTarget - play->envCtx.lightSettings.fogFar) * lerp;
}

void CrossOwl_SetupAction(CrossOwl* this, CrossOwlActionFunc actionFunc);
void CrossOwl_InitFeathers(OwlWarpFeather* feathers);
void CrossOwl_AddFeather(OwlWarpFeather* feather, Vec3f* pos, s32 isPosYRand);
void CrossOwl_AddAndChooseFeather(OwlWarpFeather* feathers, Vec3f* pos, s32 isPosYRand);
void CrossOwl_UpdateFeatherType1(PlayState* play, OwlWarpFeather* feather);
void CrossOwl_UpdateFeatherType2(PlayState* play, OwlWarpFeather* feather);
void CrossOwl_UpdateFeathers(PlayState* play, OwlWarpFeather* feathers, CrossOwl* this, s32 arg3, s32 arg4);
void CrossOwl_DrawFeathers(PlayState* play2, OwlWarpFeather* feathers);
void CrossOwl_InitWindCapsule(OwlWarpWindCapsule* windCapsule);
void CrossOwl_WarpCsPart1(CrossOwl* this, PlayState* play);
void CrossOwl_UpdateGrowingWindCapsule(CrossOwl* this, PlayState* play);
void CrossOwl_WarpCsPart2(CrossOwl* this, PlayState* play);
void CrossOwl_WarpCsPart3(CrossOwl* this, PlayState* play);
void CrossOwl_WarpCsPart4(CrossOwl* this, PlayState* play);
void CrossOwl_WarpCsPart5(CrossOwl* this, PlayState* play);
void CrossOwl_WarpCsPart6(CrossOwl* this, PlayState* play);
void CrossOwl_WarpCsWait(CrossOwl* this, PlayState* play);
void CrossOwl_UpdateSubCamWarpCs2(CrossOwl* this, PlayState* play, f32 lerp);
void CrossOwl_UpdateSubCamWarpCs1(CrossOwl* this, PlayState* play, f32 lerp);
void CrossOwl_SpinAndSquishPlayer(CrossOwl* this, PlayState* play, f32 lerp);
void CrossOwl_PlayerAndSubCamAction(CrossOwl* this, PlayState* play);
void CrossOwl_Update(Actor* thisx, PlayState* play);
s32 CrossOwl_OverrideLimbDraw(PlayState* play, CWKFSkelAnimeFlex* kfSkelAnime, s32 limbIndex, Gfx** dList, u8* flags,
                              void* thisx, Vec3f* scale, Vec3s* rot, Vec3f* pos);
void CrossOwl_Draw(Actor* thisx, PlayState* play);

void CrossOwl_WarpCsWarp(CrossOwl* this, PlayState* play) {
    Vec3f featherPos;

    Math_Vec3f_Copy(&featherPos, &this->actor.world.pos);

    if (Rand_ZeroOne() < 0.1f) {
        CrossOwl_AddAndChooseFeather(this->feathers, &featherPos, true);
    }

    this->windCapsule.yaw -= 0x2EE0;

    // MM keeps this actor, its feathers and capsule until the scene changes. Start
    // the warp once and, as MM does, let the destination start its own music.
    if (!this->departed) {
        this->departed = true;
        Combo_SOHCompleteWarp(play, this->actor.params);
        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
    }
}
int Combo_SOHSoaringCutsceneActive(PlayState* play) {
    // Native MM's soaringCsOrSoTCsPlaying covers departure ticks 0..99.
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head; actor; actor = actor->next) {
        if (actor->id == ACTOR_COMBO_OWL_WARP && actor->params != 0x100 && actor->update &&
            ((CrossOwl*)actor)->timer < 100)
            return 1;
    }
    return 0;
}
void CrossOwl_Init(Actor* thisx, PlayState* play) {
    if (thisx->params == 0x100)
        return;
    CrossOwl* this = (CrossOwl*)thisx;
    Player* player = GET_PLAYER(play);
    this->actor.world.rot.y = this->actor.shape.rot.y = player->actor.shape.rot.y;
    this->playerYaw = player->actor.shape.rot.y;
    this->playerScaleX = player->actor.scale.x;
    this->playerScaleZ = player->actor.scale.z;
    this->playerDrawFunc = player->actor.draw;
    this->savedHud = gSaveContext.hudVisibilityMode;
    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);
    Actor_SetScale(&this->actor, LINK_IS_ADULT ? 0.015f : 0.01f);
    CWKeyframe_InitFlex(&this->kfSkelAnime, (void*)gGameplayKeepKFSkel_85640, (void*)gGameplayKeepKFAnim_83534,
                        this->jointTable, this->morphTable, NULL);
    CWKeyframe_FlexPlayOnce(&this->kfSkelAnime, (void*)gGameplayKeepKFAnim_83534);
    CrossOwl_InitFeathers(this->feathers);
    CrossOwl_InitWindCapsule(&this->windCapsule);
    // End OoT's instrument camera before taking over, as MM's cutscene
    // manager does. Player cleanup must not reactivate it during departure.
    Camera* previous = GET_ACTIVE_CAM(play);
    const Vec3f initialEye = previous->eye, initialAt = previous->at;
    const f32 initialFov = previous->fov;
    if (player->subCamId != SUBCAM_NONE) {
        Play_ClearCamera(play, player->subCamId);
        player->subCamId = SUBCAM_NONE;
    }
    this->subCamId = Play_CreateSubCamera(play);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);
    this->subCamEye = initialEye;
    this->subCamAt = initialAt;
    Play_CameraSetAtEye(play, this->subCamId, &this->subCamAt, &this->subCamEye);
    Play_CameraSetFov(play, this->subCamId, initialFov);
    Player_SetCsActionWithHaltedActors(play, &this->actor, 8);
    player->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    this->actionFunc = CrossOwl_WarpCsPart1;
    this->playerCamFunc = CrossOwl_PlayerAndSubCamAction;
    Combo_SOHSoaringAudioBegin();
    Lights_PointNoGlowSetInfo(&this->lightInfo, player->actor.world.pos.x + Math_SinS(this->playerYaw) * 90,
                              player->actor.world.pos.y + 10,
                              player->actor.world.pos.z + Math_CosS(this->playerYaw) * 90, 255, 255, 255, 255);
    this->lightNode = LightContext_InsertLight(play, &play->lightCtx, &this->lightInfo);
}
void CrossOwl_Destroy(Actor* thisx, PlayState* play) {
    if (thisx->params == 0x100)
        return;
    CrossOwl* this = (CrossOwl*)thisx;
    Combo_SOHSoaringAudioEnd(this->departed);
    if (!this->departed) {
        Player* player = GET_PLAYER(play);
        // Game handoff destroys the player category before item-action actors.
        if (player) {
            player->actor.draw = this->playerDrawFunc;
            player->actor.scale.x = this->playerScaleX;
            player->actor.scale.z = this->playerScaleZ;
            player->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
            Player_SetCsAction(play, &this->actor, 7);
        }
        Play_ClearCamera(play, this->subCamId);
        Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_ACTIVE);
        Interface_ChangeHudVisibilityMode(this->savedHud);
    }
    LightContext_RemoveLight(play, &play->lightCtx, this->lightNode);
}
const ActorInit Combo_Owl_Warp_InitVars = {
    ACTOR_COMBO_OWL_WARP,
    ACTORCAT_ITEMACTION,
    ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED | ACTOR_FLAG_UPDATE_DURING_OCARINA,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(CrossOwl),
    CrossOwl_Init,
    CrossOwl_Destroy,
    CrossOwl_Update,
    CrossOwl_Draw,
    NULL,
};
int Combo_SOHStartSoaring(PlayState* play, int destination) {
    if (destination < 0 || destination >= 16)
        return 0;
    // This is MM's Soaring departure even when played in OoT. Keep the selected
    // route and native arrival when its dedicated cutscene skip is enabled.
    if (CVarGetInteger("gEnhancements.Songs.SkipSoaringCutscene", 0)) {
        Combo_SOHCompleteWarp(play, destination);
        return 1;
    }
    Player* player = GET_PLAYER(play);
    return Actor_Spawn(&play->actorCtx, play, ACTOR_COMBO_OWL_WARP, player->actor.world.pos.x,
                       player->actor.world.pos.y, player->actor.world.pos.z, 0, player->actor.shape.rot.y, 0,
                       destination) != NULL;
}
void CrossOwl_SetupAction(CrossOwl* this, CrossOwlActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void CrossOwl_InitFeathers(OwlWarpFeather* feathers) {
    OwlWarpFeather* feather = feathers;
    s32 i;

    for (i = 0, feather = feathers; i < OWL_WARP_NUM_FEATHERS; i++, feather++) {
        s32 pad;

        feather->type = OWL_WARP_FEATHER_TYPE_DISABLED;
        feather->unk_04 = 0;
        feather->pos.x = 0.0f;
        feather->pos.y = 0.0f;
        feather->pos.z = 0.0f;
        feather->velocity.x = 0.0f;
        feather->velocity.y = 0.0f;
        feather->velocity.z = 0.0f;
        feather->accel.x = 0.0f;
        feather->accel.y = 0.0f;
        feather->accel.z = 0.0f;
        feather->scale = 0.00001f;
        feather->rot.x = 0;
        feather->rot.y = 0;
        feather->rot.z = 0;
        feather->angularVelocity.x = 0;
        feather->angularVelocity.y = 0;
        feather->angularVelocity.z = 0;
    }
}

void CrossOwl_AddFeather(OwlWarpFeather* feather, Vec3f* pos, s32 isPosYRand) {
    s16 yaw = Rand_ZeroOne() * 0xFFFF;
    f32 velocityXZ;

    feather->pos = *pos;

    if (isPosYRand) {
        feather->pos.x += Math_CosS(yaw) * 0.0f;
        feather->pos.y += (Rand_ZeroOne() * 100.0f) - 20.0f;
        feather->pos.z += Math_SinS(yaw) * 0.0f;
    }

    velocityXZ = (Rand_ZeroOne() * 4.0f) + 2.0f;

    feather->velocity.x = Math_CosS(yaw) * velocityXZ;
    feather->velocity.y = Rand_ZeroOne();
    feather->velocity.z = Math_SinS(yaw) * velocityXZ;

    feather->accel.x = 0.0f;
    feather->accel.y = 0.0f;
    feather->accel.z = 0.0f;

    feather->scale = 0.25f;
    feather->rot.x = Rand_ZeroOne() * 0x10000;
    feather->rot.y = Rand_ZeroOne() * 0x10000;
    feather->rot.z = Rand_ZeroOne() * 0x10000;
    feather->unk_04 = 60;

    if (Rand_ZeroOne() < 0.9f) {
        feather->type = OWL_WARP_FEATHER_TYPE_1;
        feather->rot.x = Rand_ZeroOne() * 0x10000;
        feather->rot.y = Rand_ZeroOne() * 0x10000;
        feather->rot.z = Rand_ZeroOne() * 0x10000;
        feather->angularVelocity.x = 0;
        feather->angularVelocity.y = 0;
        feather->angularVelocity.z = 0;
    } else {
        feather->type = OWL_WARP_FEATHER_TYPE_2;
        feather->rot.x = 0;
        feather->rot.y = 0;
        feather->rot.z = Rand_ZeroOne() * 5000.0f;
        feather->angularVelocity.x = 0;
        feather->angularVelocity.y = (Rand_ZeroOne() * 8000.0f) + 2000.0f;
        if (Rand_ZeroOne() > 0.5f) {
            feather->angularVelocity.y = -feather->angularVelocity.y;
        }
        feather->angularVelocity.z = 0;
    }
}

void CrossOwl_AddAndChooseFeather(OwlWarpFeather* feathers, Vec3f* pos, s32 isPosYRand) {
    static s32 sFeatherIndex = 0;
    s32 i;
    s32 featherAdded = false;
    OwlWarpFeather* feather;
    s32 featherIndex;

    for (i = 0, feather = feathers; i < OWL_WARP_NUM_FEATHERS; i++, feather++) {
        if (feather->type == OWL_WARP_FEATHER_TYPE_DISABLED) {
            CrossOwl_AddFeather(feather, pos, isPosYRand);
            sFeatherIndex = i;
            featherAdded = true;
            break;
        }
    }

    if (!featherAdded) {
        featherIndex = sFeatherIndex + 1;
        if (featherIndex >= OWL_WARP_NUM_FEATHERS) {
            featherIndex = 0;
        }

        CrossOwl_AddFeather(&feathers[featherIndex], pos, isPosYRand);
    }
}

void CrossOwl_UpdateFeatherType1(PlayState* play, OwlWarpFeather* feather) {
    static Vec3f sUnitVecY = { 0.0f, 1.0f, 0.0f };
    static Vec3f sUnitVecZ = { 0.0f, 0.0f, 1.0f };
    static MtxF sFeatherRotMf;
    static Vec3f sFeatherAccel;
    static Vec3f sFeatherUnused;
    s32 swayAngleX = feather->unk_04 % 41;
    s32 swayAngleY = (feather->unk_04 + 7000) % 41;
    s32 sp24 = false;

    SkinMatrix_SetRotateZYX(&sFeatherRotMf, feather->rot.x, feather->rot.y, feather->rot.z);
    SkinMatrix_Vec3fMtxFMultXYZ(&sFeatherRotMf, &sUnitVecY, &sFeatherAccel);
    SkinMatrix_Vec3fMtxFMultXYZ(&sFeatherRotMf, &sUnitVecZ, &sFeatherUnused);

    if (feather->rot.x < 0x3448) {
        feather->rot.x += 0x384;
    } else if (feather->rot.x >= 0x4BB9) {
        feather->rot.x -= 0x384;
    } else {
        feather->rot.x = (Math_SinS(((f32)swayAngleX * 0xFFFF) / 41.0f) * 2000.0f) + 0x4000;
    }

    if (feather->rot.y < -0xBB8) {
        feather->rot.y += 0x384;
    } else if (feather->rot.y > 0xBB8) {
        feather->rot.y -= 0x384;
    } else {
        sp24 = true;
        feather->rot.y = Math_SinS(((f32)swayAngleY * 0xFFFF) / 41.0f) * 2000.0f;
    }

    if (sp24 == true) {
        if (sFeatherAccel.y < 0.0f) {
            feather->velocity.x += sFeatherAccel.x * 0.5f;
            feather->velocity.y += sFeatherAccel.y * 0.5f + 0.08f;
            feather->velocity.z += sFeatherAccel.z * 0.5f;
        } else {
            feather->velocity.x += -sFeatherAccel.x * 0.5f;
            feather->velocity.y += -sFeatherAccel.y * 0.5f + 0.08f;
            feather->velocity.z += -sFeatherAccel.z * 0.5f;
        }
    } else if (sFeatherAccel.y < 0.0f) {
        feather->velocity.x += sFeatherAccel.x * 0.2f;
        feather->velocity.y += sFeatherAccel.y * 0.2f + 0.08f;
        feather->velocity.z += sFeatherAccel.z * 0.2f;
    } else {
        feather->velocity.x += -sFeatherAccel.x * 0.2f;
        feather->velocity.y += -sFeatherAccel.y * 0.2f + 0.08f;
        feather->velocity.z += -sFeatherAccel.z * 0.2f;
    }

    feather->pos.x += feather->velocity.x;
    feather->pos.y += feather->velocity.y;
    feather->pos.z += feather->velocity.z;
}

void CrossOwl_UpdateFeatherType2(PlayState* play, OwlWarpFeather* feather) {
    feather->rot.y += feather->angularVelocity.y;

    feather->accel.x = Rand_Centered();
    feather->accel.y = Rand_Centered() + -0.01f;
    feather->accel.z = Rand_Centered();

    feather->velocity.x += feather->accel.x;
    feather->velocity.y += feather->accel.y;
    feather->velocity.z += feather->accel.z;

    feather->pos.x += feather->velocity.x;
    feather->pos.y += feather->velocity.y;
    feather->pos.z += feather->velocity.z;
}

void CrossOwl_UpdateFeathers(PlayState* play, OwlWarpFeather* feathers, CrossOwl* this, s32 arg3, s32 arg4) {
    s32 pad;
    s32 i;
    f32 temp_f28;
    f32 temp_f0;
    f32 temp_f22;
    f32 temp_f24;
    f32 temp_f26;
    f32 temp_f2;
    Vec3f sp8C;
    OwlWarpFeather* feather;
    s16 phi_s1;

    for (i = 0; i < OWL_WARP_NUM_FEATHERS; i++) {
        feather = &feathers[i];

        if (feather->type == OWL_WARP_FEATHER_TYPE_DISABLED) {
            continue;
        }

        if (feather->type == OWL_WARP_FEATHER_TYPE_1) {
            CrossOwl_UpdateFeatherType1(play, feather);
        } else {
            CrossOwl_UpdateFeatherType2(play, feather);
        }

        if (arg3) {
            temp_f22 = feather->pos.x - this->actor.world.pos.x;
            temp_f24 = feather->pos.z - this->actor.world.pos.z;
            temp_f0 = SQ(temp_f22) + SQ(temp_f24);

            phi_s1 = -0x2710;
            if (temp_f0 > SQ(20.0f)) {
                phi_s1 /= ((temp_f0 - SQ(20.0f)) * 0.00125f) + 1.0f;
            }

            temp_f26 = (temp_f22 * Math_CosS(phi_s1)) - (temp_f24 * Math_SinS(phi_s1));
            temp_f28 = (temp_f22 * Math_SinS(phi_s1)) + (temp_f24 * Math_CosS(phi_s1));

            feather->pos.x = this->actor.world.pos.x + temp_f26;
            feather->pos.z = this->actor.world.pos.z + temp_f28;

            temp_f22 = feather->velocity.x;
            temp_f24 = feather->velocity.z;

            feather->velocity.x = (temp_f22 * Math_CosS(phi_s1)) - (temp_f24 * Math_SinS(phi_s1));
            feather->velocity.z = (temp_f22 * Math_SinS(phi_s1)) + (temp_f24 * Math_CosS(phi_s1));

            temp_f22 = feather->accel.x;
            temp_f24 = feather->accel.z;

            feather->accel.x = (temp_f22 * Math_CosS(phi_s1)) - (temp_f24 * Math_SinS(phi_s1));
            feather->accel.z = (temp_f22 * Math_SinS(phi_s1)) + (temp_f24 * Math_CosS(phi_s1));
        }

        if (arg4) {
            sp8C.x = feather->pos.x - this->actor.world.pos.x;
            sp8C.y = feather->pos.y - (this->actor.world.pos.y + 40.0f);
            sp8C.z = feather->pos.z - this->actor.world.pos.z;

            temp_f2 = 1.0f - (0.5f / ((Math3D_Vec3fMagnitude(&sp8C) / 500.0f) + 1.0f));

            sp8C.x *= temp_f2;
            sp8C.y *= temp_f2;
            sp8C.z *= temp_f2;

            feather->pos.x = this->actor.world.pos.x + sp8C.x;
            feather->pos.y = this->actor.world.pos.y + sp8C.y + 40.0f;
            feather->pos.z = this->actor.world.pos.z + sp8C.z;
        }
    }
}

void CrossOwl_DrawFeathers(PlayState* play2, OwlWarpFeather* feathers) {
    PlayState* play = play2;
    s32 i;
    Gfx* featherDL = (Gfx*)Combo_SOHWarpResource(gSoaringWarpCsFeatherDL);

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_26Opa(play->state.gfxCtx);

    Matrix_Push();

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);

    for (i = 0; i < OWL_WARP_NUM_FEATHERS; i++) {
        Mtx* mtx;
        MtxF sp6C;
        OwlWarpFeather* feather = &feathers[i];

        if (feather->type == OWL_WARP_FEATHER_TYPE_DISABLED) {
            continue;
        }

        if ((feather->pos.x > 30000.0f) || (feather->pos.x < -30000.0f) || (feather->pos.y > 30000.0f) ||
            (feather->pos.y < -30000.0f) || (feather->pos.z > 30000.0f) || (feather->pos.z < -30000.0f)) {
            feather->type = OWL_WARP_FEATHER_TYPE_DISABLED;
            continue;
        }

        Matrix_Translate(feather->pos.x, feather->pos.y, feather->pos.z, MTXMODE_NEW);

        if (feather->type == OWL_WARP_FEATHER_TYPE_1) {
            Matrix_RotateZYX(feather->rot.x, feather->rot.y, feather->rot.z, MTXMODE_APPLY);
        } else {
            SkinMatrix_SetRotateYXZ(&sp6C, feather->rot.x, feather->rot.y, feather->rot.z);
            Matrix_Mult(&sp6C, MTXMODE_APPLY);
        }

        Matrix_Scale(feather->scale, feather->scale, feather->scale, MTXMODE_APPLY);
        if (feather->type == OWL_WARP_FEATHER_TYPE_2) {
            Matrix_Translate(0.0f, 30.0f, 0.0f, MTXMODE_APPLY);
        }

        mtx = MATRIX_NEWMTX(play->state.gfxCtx);
        if (mtx == NULL) {
            continue;
        }
        gSPMatrix(POLY_OPA_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, featherDL);
    }

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
}

void CrossOwl_InitWindCapsule(OwlWarpWindCapsule* windCapsule) {
    windCapsule->unk_00 = 0.0f;
    windCapsule->unk_04 = 0.0f;
    windCapsule->xzScale = 1.0f;
    windCapsule->yScale = 1.0f;
    windCapsule->yaw = 0;
}

void CrossOwl_WarpCsPart1(CrossOwl* this, PlayState* play) {
    Color_RGB8 fogColor = { 64, 0, 0 };
    Color_RGB8 ambientColor = { 220, 220, 255 };
    f32 envLerp = this->timer / (f32)(10 - 0);

    CWEnvironment_LerpAmbientColor(play, &ambientColor, envLerp);
    CWEnvironment_LerpFogColor(play, &fogColor, envLerp);
    CWEnvironment_LerpFog(play, 2000, 4000, envLerp);

    if (this->timer >= 10) {
        Camera* subCam = Play_GetCamera(play, this->subCamId);

        this->subCamEye = subCam->eye;
        this->subCamAt = subCam->at;
        this->subCamFov = subCam->fov;

        CrossOwl_SetupAction(this, CrossOwl_WarpCsPart2);
        this->flags |= OWL_WARP_FLAGS_20;
        Combo_SOHSoaringSound(0);
    }
}

void CrossOwl_UpdateGrowingWindCapsule(CrossOwl* this, PlayState* play) {
    this->flags |= OWL_WARP_FLAGS_DRAW_WIND_CAPSULE;

    if (this->windCapsule.unk_04 < 11.0f) {
        this->windCapsule.unk_04 += 0.3f;
        this->windCapsule.xzScale = ((this->windCapsule.unk_00 * -0.45f) / 11.0f) + 0.7f;
        this->windCapsule.yScale = ((this->windCapsule.unk_00 * -0.29999998f) / 11.0f) + 0.7f;
    }

    if (this->windCapsule.unk_00 < 11.0f) {
        this->windCapsule.unk_00 += 1.0f;
        if (this->windCapsule.unk_00 > 6.0f) {
            Player* player = GET_PLAYER(play);

            this->flags &= ~OWL_WARP_FLAGS_DRAW_WINGS;
            player->actor.draw = NULL;
        }
    } else if (this->timer > 86) {
        CrossOwl_SetupAction(this, CrossOwl_WarpCsPart4);
        this->flags &= ~OWL_WARP_FLAGS_8;
        this->windCapsule.yaw -= 0x2EE0;
    } else {
        this->windCapsule.yaw -= 0x2EE0;
    }
}

void CrossOwl_WarpCsPart2(CrossOwl* this, PlayState* play) {
    Vec3f featherPos;

    if (CWKeyframe_UpdateFlex(&this->kfSkelAnime)) {
        CrossOwl_SetupAction(this, CrossOwl_WarpCsPart3);
    }

    if (this->kfSkelAnime.frameCtrl.curTime > 60.0f) {
        CrossOwl_UpdateGrowingWindCapsule(this, play);
    }

    if ((this->kfSkelAnime.frameCtrl.curTime > 20.0f) && !(this->flags & OWL_WARP_FLAGS_40)) {
        this->flags |= OWL_WARP_FLAGS_40;
        Combo_SOHSoaringSound(1);
    }

    if (this->kfSkelAnime.frameCtrl.curTime > 42.0f) {
        if (!(this->flags & OWL_WARP_FLAGS_80)) {
            this->flags |= OWL_WARP_FLAGS_80;
            Combo_SOHSoaringSound(2);
        }

        if (Rand_ZeroOne() < 0.3f) {
            Camera* subCam = Play_GetCamera(play, this->subCamId);
            f32 randLerp = Rand_ZeroOne();

            featherPos.x = LERP(this->actor.world.pos.x, subCam->eye.x, randLerp);
            featherPos.y = LERP(this->actor.world.pos.y, subCam->eye.y, randLerp);
            featherPos.z = LERP(this->actor.world.pos.z, subCam->eye.z, randLerp);

            CrossOwl_AddAndChooseFeather(this->feathers, &featherPos, true);
            this->flags |= OWL_WARP_FLAGS_8;
        }
    } else {
        this->flags |= OWL_WARP_FLAGS_DRAW_WINGS;
    }
}

void CrossOwl_WarpCsPart3(CrossOwl* this, PlayState* play) {
    Vec3f featherPos;
    Camera* subCam;
    f32 randLerp;

    CrossOwl_UpdateGrowingWindCapsule(this, play);

    if (Rand_ZeroOne() < 0.3f) {
        subCam = Play_GetCamera(play, this->subCamId);
        randLerp = Rand_ZeroOne();

        featherPos.x = LERP(this->actor.world.pos.x, subCam->eye.x, randLerp);
        featherPos.y = LERP(this->actor.world.pos.y, subCam->eye.y, randLerp);
        featherPos.z = LERP(this->actor.world.pos.z, subCam->eye.z, randLerp);

        CrossOwl_AddAndChooseFeather(this->feathers, &featherPos, true);
    }

    Math_Vec3f_Copy(&featherPos, &this->actor.world.pos);
    CrossOwl_AddAndChooseFeather(this->feathers, &featherPos, true);
    this->jointTable[1].y += 0x2EE0;
}

void CrossOwl_WarpCsPart4(CrossOwl* this, PlayState* play) {
    s32 pad;
    s32 temp = this->timer - 86;
    f32 temp_f0 = temp / (f32)(96 - 86);
    Vec3f featherPos;

    this->flags |= OWL_WARP_FLAGS_10;

    this->windCapsule.xzScale = (-0.15f * temp_f0) + 0.25f;
    this->windCapsule.yScale = (-0.3f * temp_f0) + 0.4f;
    this->windCapsule.yaw -= 0x2EE0;

    this->flags |= OWL_WARP_FLAGS_DRAW_LENS_FLARE;

    if (this->timer >= 96) {
        CrossOwl_SetupAction(this, CrossOwl_WarpCsPart5);
        this->flags &= ~OWL_WARP_FLAGS_10;
    }
    Math_Vec3f_Copy(&featherPos, &this->actor.world.pos);
    CrossOwl_AddAndChooseFeather(this->feathers, &featherPos, true);
}

void CrossOwl_WarpCsPart5(CrossOwl* this, PlayState* play) {
    s32 pad;
    s32 temp = this->timer - 96;
    f32 lerp = 1.0f - (temp / (f32)(100 - 96));
    Camera* subCam;
    f32 temp_f2;
    f32 temp_f4;

    this->windCapsule.xzScale = ((temp * -0.1f) / 4) + 0.1f;
    this->windCapsule.yScale = ((temp * 5.9f) / 4) + 0.1f;
    this->windCapsule.yaw -= 0x2EE0;

    this->actor.world.pos.y += 100.0f;

    subCam = Play_GetCamera(play, this->subCamId);
    subCam->player = NULL;

    subCam->eye.x = LERP(this->subCamEye.x, subCam->eye.x, lerp);
    subCam->eye.y = LERP(this->subCamEye.y, subCam->eye.y, lerp);
    subCam->eye.z = LERP(this->subCamEye.z, subCam->eye.z, lerp);

    subCam->fov = LERP(this->subCamFov, subCam->fov, lerp);

    if (this->timer >= 100) {
        play->envCtx.fillScreen = true;
        play->envCtx.screenFillColor[0] = 255;
        play->envCtx.screenFillColor[1] = 255;
        play->envCtx.screenFillColor[2] = 255;
        play->envCtx.screenFillColor[3] = 255;

        this->flags &= ~OWL_WARP_FLAGS_DRAW_LENS_FLARE;
        CrossOwl_SetupAction(this, CrossOwl_WarpCsPart6);
    }
}

void CrossOwl_WarpCsPart6(CrossOwl* this, PlayState* play) {
    s32 sp2C = this->timer - 100;
    f32 envLerp;
    Color_RGB8 fogColor = { 64, 0, 0 };
    Color_RGB8 ambientColor = { 220, 220, 255 };

    if (play->envCtx.fillScreen) {
        Combo_SOHSoaringSound(3);
        play->envCtx.fillScreen = false;
        play->envCtx.screenFillColor[0] = 0;
        play->envCtx.screenFillColor[1] = 0;
        play->envCtx.screenFillColor[2] = 0;
        play->envCtx.screenFillColor[3] = 0;
    }

    envLerp = 1.0f - (sp2C / (f32)(110 - 100));
    CWEnvironment_LerpAmbientColor(play, &ambientColor, envLerp);
    CWEnvironment_LerpFogColor(play, &fogColor, envLerp);
    CWEnvironment_LerpFog(play, 2000, 4000, envLerp);

    if (this->timer >= 110) {
        CrossOwl_SetupAction(this, CrossOwl_WarpCsWait);
    }
}

void CrossOwl_WarpCsWait(CrossOwl* this, PlayState* play) {
    if (this->timer >= 130) {
        CrossOwl_SetupAction(this, CrossOwl_WarpCsWarp);
    }
}

void CrossOwl_UpdateSubCamWarpCs2(CrossOwl* this, PlayState* play, f32 lerp) {
    Vec3f eyeNext;
    Vec3f* playerPos;
    Player* player = GET_PLAYER(play);
    Camera* subCam = Play_GetCamera(play, this->subCamId);

    playerPos = &player->actor.world.pos;
    subCam->player = NULL;

    eyeNext.x = playerPos->x + 180.0f * Math_SinS(this->playerYaw) * Math_CosS(0xFA0);
    eyeNext.y = playerPos->y + 180.0f * Math_SinS(0xFA0);
    eyeNext.z = playerPos->z + 180.0f * Math_CosS(this->playerYaw) * Math_CosS(0xFA0);

    subCam->eye.x = LERP(subCam->eye.x, eyeNext.x, lerp);
    subCam->eye.y = LERP(subCam->eye.y, eyeNext.y, lerp);
    subCam->eye.z = LERP(subCam->eye.z, eyeNext.z, lerp);

    subCam->fov = LERP(subCam->fov, this->subCamFov, lerp);

    subCam->at.y += 1.4444444f;
}

void CrossOwl_UpdateSubCamWarpCs1(CrossOwl* this, PlayState* play, f32 lerp) {
    Vec3f* playerPos;
    Player* player = GET_PLAYER(play);
    Camera* subCam = Play_GetCamera(play, this->subCamId);
    Vec3f eyeNext;

    subCam->player = NULL;

    playerPos = &player->actor.world.pos;

    eyeNext.x = playerPos->x + 80.0f * Math_SinS(this->playerYaw) * Math_CosS(0xBB8);
    eyeNext.y = playerPos->y + 80.0f * Math_SinS(0xBB8);
    eyeNext.z = playerPos->z + 80.0f * Math_CosS(this->playerYaw) * Math_CosS(0xBB8);

    subCam->eye.x = LERP(subCam->eye.x, eyeNext.x, lerp);
    subCam->eye.y = LERP(subCam->eye.y, eyeNext.y, lerp);
    subCam->eye.z = LERP(subCam->eye.z, eyeNext.z, lerp);

    subCam->at.x = LERP(subCam->at.x, playerPos->x, lerp);
    subCam->at.y = LERP(subCam->at.y, playerPos->y + 40.0f, lerp);
    subCam->at.z = LERP(subCam->at.z, playerPos->z, lerp);

    subCam->fov = LERP(subCam->fov, this->subCamFov, lerp);
}

void CrossOwl_SpinAndSquishPlayer(CrossOwl* this, PlayState* play, f32 lerp) {
    Player* player = GET_PLAYER(play);

    // Spin Player
    player->actor.shape.rot.y += 0x2EE0;

    // Squish Player thin
    player->actor.scale.x = LERP(this->playerScaleX, 0.0f, lerp);
    player->actor.scale.z = LERP(this->playerScaleZ, 0.0f, lerp);
}

void CrossOwl_PlayerAndSubCamAction(CrossOwl* this, PlayState* play) {
    f32 lerp;

    if ((this->timer >= 12) && (this->timer <= 30)) {
        lerp = (this->timer - 12) / (f32)(30 - 12);
        CrossOwl_UpdateSubCamWarpCs2(this, play, lerp);
    } else if ((this->timer >= 79) && (this->timer <= 95)) {
        lerp = (this->timer - 79) / (f32)(95 - 79);
        CrossOwl_UpdateSubCamWarpCs1(this, play, lerp);
    }

    if ((this->timer >= 42) && (this->timer <= 68)) {
        lerp = (this->timer - 42) / (f32)(68 - 42);
        CrossOwl_SpinAndSquishPlayer(this, play, lerp);
    }
}

void CrossOwl_Update(Actor* thisx, PlayState* play) {
    CrossOwl* this = (CrossOwl*)thisx;
    if (thisx->params == 0x100) {
        if (this->timer < 100)
            this->timer++;
        else
            Actor_Kill(thisx);
        return;
    }
    // En_Test7 updates once per gameplay tick in MM and OoT alike.
    this->actionFunc(this, play);
    if (this->playerCamFunc)
        this->playerCamFunc(this, play);
    Camera* camera = Play_GetCamera(play, this->subCamId);
    Play_CameraSetAtEye(play, this->subCamId, &camera->at, &camera->eye);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
    Play_ChangeCameraStatus(play, this->subCamId, CAM_STAT_ACTIVE);
    this->timer++;
    CrossOwl_UpdateFeathers(play, this->feathers, this, (this->flags & OWL_WARP_FLAGS_8) != 0,
                            (this->flags & OWL_WARP_FLAGS_10) != 0);
}

s32 CrossOwl_OverrideLimbDraw(PlayState* play, CWKFSkelAnimeFlex* kfSkelAnime, s32 limbIndex, Gfx** dList, u8* flags,
                              void* thisx, Vec3f* scale, Vec3s* rot, Vec3f* pos) {
    CrossOwl* this = (CrossOwl*)thisx;
    Vec3f featherPos;

    if ((*dList != NULL) && (Rand_ZeroOne() < 0.03f)) {
        Matrix_MultVec3f(&(Vec3f){ 0, 0, 0 }, &featherPos);
        CrossOwl_AddAndChooseFeather(this->feathers, &featherPos, false);
    }
    return true;
}

static void CrossOwl_HideHud(PlayState* play) {
    // Message/interface updates follow actor updates. Apply the native instant
    // hide operation here, before the overlay draws, for the whole departure.
    Interface_ChangeHudVisibilityMode(HUD_VISIBILITY_NOTHING);
    gSaveContext.nextHudVisibilityMode = HUD_VISIBILITY_NOTHING;
    func_80082850(play, 0);
}

void CrossOwl_Draw(Actor* thisx, PlayState* play) {
    if (thisx->params == 0x100) {
        DrawSoaringWipe((CrossOwl*)thisx, play);
        return;
    }
    CrossOwl_HideHud(play);
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPush(POLY_OPA_DISP++, "mm-warp");
    gSPComboRMPush(POLY_XLU_DISP++, "mm-warp");
    CLOSE_DISPS(play->state.gfxCtx);
    s32 pad[2];
    CrossOwl* this = (CrossOwl*)thisx;
    s32 sp40;

    // Draw wings
    if (this->flags & OWL_WARP_FLAGS_DRAW_WINGS) {
        Mtx* mtxStack = Graph_Alloc(play->state.gfxCtx, this->kfSkelAnime.skeleton->dListCount * sizeof(Mtx));

        if (mtxStack != NULL)
            CWKeyframe_DrawFlex(play, &this->kfSkelAnime, mtxStack, CrossOwl_OverrideLimbDraw, NULL, &this->actor);
    }

    // Draw windCapsule encasing that surrounds player after wings
    if (this->flags & OWL_WARP_FLAGS_DRAW_WIND_CAPSULE) {
        Matrix_Push();
        Matrix_Translate(0.0f, 4000.0f, 0.0f, MTXMODE_APPLY);
        Matrix_RotateZYX(0, this->windCapsule.yaw, 0, MTXMODE_APPLY);
        Matrix_Scale(this->windCapsule.xzScale * 100.0f, this->windCapsule.yScale * 100.0f,
                     this->windCapsule.xzScale * 100.0f, MTXMODE_APPLY);
        sp40 = this->windCapsule.unk_00;
        Combo_SOHWindMaterial(play, sp40);
        OPEN_DISPS(play->state.gfxCtx);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)Combo_SOHWarpResource(gSoaringWarpCsWindCapsuleDL));
        CLOSE_DISPS(play->state.gfxCtx);
        Combo_SOHRestoreWarpMaterial(play, gSoaringWarpCsWindCapsuleTexAnim);
        Matrix_Pop();
    }

    CrossOwl_DrawFeathers(play, this->feathers);

    if (this->flags & OWL_WARP_FLAGS_DRAW_LENS_FLARE) {
        Environment_DrawLensFlare(play, &play->envCtx, &play->view, play->state.gfxCtx, this->actor.world.pos, 0, 70,
                                  5.0f, 0, false);
    }
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPop(POLY_OPA_DISP++);
    gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    CrossOwl_DrawMotionBlur(this, play);
}

#endif
