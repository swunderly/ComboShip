#ifdef COMBO_BUILD
#include "global.h"
#include "BenPort.h"
#include "align_asset_macro.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
// OoT ovl_Demo_Kankyo's native departure, adapted only for MM engine types
// and the final game handoff. Resources remain in the user's OoT archive.
extern void Combo_MMCompleteWarp(PlayState*, int);
extern void Interface_UpdateHudAlphas(PlayState*, s16);
enum { DEMOKANKYO_WARP_OUT = 15 };
typedef struct {
    Vec3f unk_0, unk_C;
    f32 unk_18, unk_1C;
    s16 unk_20;
    u8 unk_22, unk_23;
    s16 unk_24;
} CWWarpSparkle;
typedef struct {
    Actor actor;
    u8 sparkleCounter, warpTimer;
    CWWarpSparkle unk_150[30];
    int destination;
    s16 subCamId;
    Vec3f origin;
    s16 yaw, camKey;
    f32 camFrame;
    ActorFunc playerDraw;
    u16 savedHud;
} CrossSongWarp;

typedef struct {
    s8 continueFlag;
    s8 cameraRoll;
    s16 nextPointFrame;
    f32 viewAngle;
    Vec3s pos;
} CWWarpPoint;
static CWWarpPoint sWarpOutCameraPoints[] = {
    { 0, 0, 8, 45.0f, { 0x0000, 0x0000, 0xFFE5 } },  { 0, 0, 8, 45.0f, { 0x0000, 0x0000, 0xFFE5 } },
    { 0, 0, 8, 45.0f, { 0xFFE6, 0x0000, 0x0000 } },  { 0, 0, 8, 45.0f, { 0x0000, 0x0017, 0x0024 } },
    { 0, 0, 8, 45.0f, { 0x001C, 0x0032, 0xFFFF } },  { 0, 0, 8, 45.0f, { 0x0001, 0x0018, 0xFFD9 } },
    { 0, 0, 8, 45.0f, { 0xFFE6, 0xFFFA, 0x0003 } },  { 0, 0, 8, 45.0f, { 0x0000, 0x0025, 0x0037 } },
    { 0, 0, 8, 45.0f, { 0x004F, 0x0066, 0x0029 } },  { 0, 0, 8, 45.0f, { 0x00A6, 0x00AD, 0x0006 } },
    { 0, 0, 5, 45.0f, { 0x010D, 0x015A, 0xFF4C } },  { 0, 0, 5, 45.0f, { 0x019F, 0x0245, 0xFE35 } },
    { -1, 0, 5, 45.0f, { 0x01CE, 0x036F, 0xFCC2 } }, { -1, 0, 5, 45.0f, { 0x01CE, 0x036F, 0xFCC2 } },
};
void CWWarpSplineCalc(f32 u, Vec3f* pos, f32* roll, f32* viewAngle, f32* point0, f32* point1, f32* point2,
                      f32* point3) {
    f32 coeff[4];

    u = CLAMP_MAX(u, 1.0f);

    coeff[0] = (1.0f - u) * (1.0f - u) * (1.0f - u) / 6.0f;
    coeff[1] = u * u * u / 2.0f - u * u + 2.0f / 3.0f;
    coeff[2] = -u * u * u / 2.0f + u * u / 2.0f + u / 2.0f + 1.0f / 6.0f;
    coeff[3] = u * u * u / 6.0f;

    pos->x = (coeff[0] * point0[0]) + (coeff[1] * point1[0]) + (coeff[2] * point2[0]) + (coeff[3] * point3[0]);
    pos->y = (coeff[0] * point0[1]) + (coeff[1] * point1[1]) + (coeff[2] * point2[1]) + (coeff[3] * point3[1]);
    pos->z = (coeff[0] * point0[2]) + (coeff[1] * point1[2]) + (coeff[2] * point2[2]) + (coeff[3] * point3[2]);
    *roll = (coeff[0] * point0[3]) + (coeff[1] * point1[3]) + (coeff[2] * point2[3]) + (coeff[3] * point3[3]);
    *viewAngle = (coeff[0] * point0[4]) + (coeff[1] * point1[4]) + (coeff[2] * point2[4]) + (coeff[3] * point3[4]);
}

s32 CWWarpSplineStep(Vec3f* pos, f32* roll, f32* fov, CWWarpPoint* point, s16* keyFrame, f32* curFrame) {
    s32 ret = false;
    f32 pointData[4][5];
    s32 i;
    f32 progress = *curFrame;
    s32 key = *keyFrame;
    f32 speed1 = 0.0f;
    f32 speed2 = 0.0f;
    f32 advance;

    if (key < 0) {
        progress = 0.0f;
    }

    if ((point[key].continueFlag == -1) || (point[key + 1].continueFlag == -1) || (point[key + 2].continueFlag == -1)) {
        return true;
    }

    for (i = 0; i < 4; i++) {
        pointData[i][0] = point[key + i].pos.x;
        pointData[i][1] = point[key + i].pos.y;
        pointData[i][2] = point[key + i].pos.z;
        pointData[i][3] = point[key + i].cameraRoll;
        pointData[i][4] = point[key + i].viewAngle;
    }

    CWWarpSplineCalc(progress, pos, roll, fov, pointData[0], pointData[1], pointData[2], pointData[3]);

    if (point[*keyFrame + 1].nextPointFrame != 0) {
        speed1 = 1.0f / point[*keyFrame + 1].nextPointFrame;
    }

    if (point[*keyFrame + 2].nextPointFrame != 0) {
        speed2 = 1.0f / point[*keyFrame + 2].nextPointFrame;
    }
    advance = (*curFrame * (speed2 - speed1)) + speed1;
    if (advance < 0.0f) {
        advance = 0;
    }
    *curFrame += advance;
    if (*curFrame >= 1) {
        if (point[++*keyFrame + 3].continueFlag == -1) {
            *keyFrame = 0;
            ret = true;
        }
        *curFrame -= 1;
    }

    return ret;
}

static CWWarpPoint sEye[] = {
    { 0, 0, 30, 60.324856f, { 31, 68, 59 } }, { 0, 0, 30, 60.324856f, { 32, 68, 60 } },
    { 0, 0, 7, 60.324856f, { 31, 69, 59 } },  { 0, 0, 7, 60.324856f, { 31, 64, 59 } },
    { 0, 0, 30, 60.324856f, { 31, 64, 59 } }, { 0, 0, 1000, 60.324856f, { 31, 64, 59 } },
    { 0, 0, 30, 60.324856f, { 31, 64, 59 } }, { -1, 0, 30, 60.324856f, { 31, 64, 59 } },
};
static CWWarpPoint sAt[] = {
    { 0, 0, 30, 60.324856f, { 12, 40, 22 } }, { 0, 0, 30, 60.324856f, { 11, 38, 22 } },
    { 0, 0, 7, 60.324856f, { 11, 39, 22 } },  { 0, 0, 7, 60.324856f, { 57, 86, 21 } },
    { 0, 0, 30, 60.324856f, { 57, 86, 21 } }, { 0, 0, 1000, 60.324856f, { 57, 86, 21 } },
    { 0, 0, 30, 60.324856f, { 57, 86, 21 } }, { -1, 0, 30, 60.324856f, { 57, 86, 21 } },
};
static Vec3f RelativeToPlayer(CrossSongWarp* this, Vec3f local) {
    return (Vec3f){ this->origin.x + local.x * Math_CosS(this->yaw) + local.z * Math_SinS(this->yaw),
                    this->origin.y + local.y,
                    this->origin.z + local.z * Math_CosS(this->yaw) - local.x * Math_SinS(this->yaw) };
}
static const Color_RGB8 sWarpSparkleEnvColors[] = { { 0, 200, 0 },   { 255, 50, 0 },   { 0, 150, 255 },
                                                    { 255, 150, 0 }, { 200, 50, 255 }, { 200, 255, 0 } };
static const ALIGN_ASSET(2) char sFlashDL[] = "__OTR__@oot:objects/gameplay_keep/gEffFlash1DL";

void CWWarp_Vec3fCopy(Vec3f* src, Vec3f* dst) {
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

Vec3f* CWWarp_Vec3fAddVecSph(Vec3f* dst, Vec3f* vec, VecGeo* sph) {
    Vec3f result;
    Vec3f sphVec;

    sphVec = OLib_VecGeoToVec3f(sph);
    result.x = vec->x + sphVec.x;
    result.y = vec->y + sphVec.y;
    result.z = vec->z + sphVec.z;
    *dst = result;
    return dst;
}

void CWWarp_Vec3fAddPosRot(PosRot* posRot, Vec3f* vec, Vec3f* dst) {
    VecGeo sph;
    Vec3f vecCopy;

    CWWarp_Vec3fCopy(vec, &vecCopy);
    sph = OLib_Vec3fToVecGeo(&vecCopy);
    sph.yaw += posRot->rot.y;
    CWWarp_Vec3fAddVecSph(dst, &posRot->pos, &sph);
}

static void CrossSongWarp_Init(Actor* actor, PlayState* play) {
    CrossSongWarp* this = (CrossSongWarp*)actor;
    Player* player = GET_PLAYER(play);
    this->destination = actor->params;
    actor->params = DEMOKANKYO_WARP_OUT;
    actor->room = -1;
    this->warpTimer = 35;
    this->origin = player->actor.world.pos;
    this->yaw = player->actor.shape.rot.y;
    this->playerDraw = player->actor.draw;
    // Native Player_Action_63 releases the instrument cutscene before starting
    // a departure. Otherwise its camera can override this actor's subcamera.
    Camera* previous = GET_ACTIVE_CAM(play);
    const Vec3f eye = previous->eye, at = previous->at;
    const f32 fov = previous->fov;
    CutsceneManager_Stop(play->playerCsIds[PLAYER_CS_ID_ITEM_OCARINA]);
    play->interfaceCtx.bButtonInterfaceDoActionActive = false;
    player->actor.flags &= ~ACTOR_FLAG_OCARINA_INTERACTION;
    this->savedHud = gSaveContext.hudVisibility;
    Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
    this->subCamId = Play_CreateSubCamera(play);
    Play_SetCameraAtEye(play, this->subCamId, &at, &eye);
    Play_SetCameraFov(play, this->subCamId, fov);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
    Play_ChangeCameraStatus(play, this->subCamId, CAM_STATUS_ACTIVE);
    Player_SetCsActionWithHaltedActors(play, actor, PLAYER_CSACTION_WAIT);
    player->stateFlags1 |= PLAYER_STATE1_20000000;
    Audio_PlaySfx(NA_SE_EV_LINK_WARP_IN);
}

static void CrossSongWarp_Destroy(Actor* actor, PlayState* play) {
    CrossSongWarp* this = (CrossSongWarp*)actor;
    Player* player = GET_PLAYER(play);
    if (player) {
        player->actor.draw = this->playerDraw;
        player->stateFlags1 &= ~PLAYER_STATE1_20000000;
        Player_SetCsAction(play, actor, PLAYER_CSACTION_END);
    }
    play->envCtx.fillScreen = false;
    Play_ClearCamera(play, this->subCamId);
    Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_ACTIVE);
    Interface_SetHudVisibility(this->savedHud);
}

static void CrossSongWarp_Update(Actor* actor, PlayState* play) {
    CrossSongWarp* this = (CrossSongWarp*)actor;
    Player* player = GET_PLAYER(play);
    // OoT DemoKankyo_SetupType: the native white flash and disappearance.
    const int remaining = this->warpTimer;
    play->envCtx.fillScreen = false;
    play->envCtx.screenFillColor[0] = play->envCtx.screenFillColor[1] = play->envCtx.screenFillColor[2] = 255;
    if (remaining < 21 && remaining >= 15) {
        play->envCtx.fillScreen = true;
        play->envCtx.screenFillColor[3] = 255 - 255 * (remaining - 15.0f) / 5.0f;
    }
    if (remaining < 15 && remaining >= 4) {
        play->envCtx.fillScreen = true;
        play->envCtx.screenFillColor[3] = 255 * (remaining - 4.0f) / 10.0f;
    }
    if (remaining == 15 && player)
        player->actor.draw = NULL;
    if (this->warpTimer)
        this->warpTimer--;
    if (this->warpTimer <= 1) {
        // gChildWarpInCS eye/at splines, beginning at the native flash end.
        Vec3f eye, at;
        float roll, fov;
        s16 key = this->camKey;
        f32 frame = this->camFrame;
        CWWarpSplineStep(&eye, &roll, &fov, sEye, &key, &frame);
        CWWarpSplineStep(&at, &roll, &fov, sAt, &this->camKey, &this->camFrame);
        eye = RelativeToPlayer(this, eye);
        at = RelativeToPlayer(this, at);
        Play_SetCameraAtEye(play, this->subCamId, &at, &eye);
        Play_SetCameraFov(play, this->subCamId, fov);
    }
}

static void CrossSongWarp_HideHud(PlayState* play) {
    // Message/interface updates run after actors and can restore the instrument
    // HUD. Reapply at draw time, before Play_PostWorldDraw draws it.
    play->interfaceCtx.bButtonInterfaceDoActionActive = false;
    Interface_SetHudVisibility(HUD_VISIBILITY_NONE);
    gSaveContext.nextHudVisibility = HUD_VISIBILITY_NONE;
    Interface_UpdateHudAlphas(play, 0);
}

static void CrossSongWarp_Draw(Actor* thisx, PlayState* play) {
    static f32 sWarpRoll;
    static f32 sWarpFoV;
    // the following 2 vars are unused
    static Vec3f D_8098CF98;

    s16 i;
    f32 temp_f22;
    CrossSongWarp* this = (CrossSongWarp*)thisx;
    Gfx* disp;
    Player* player = GET_PLAYER(play);
    Vec3f camPos;
    f32 translateX;
    f32 translateY;
    f32 translateZ;
    PosRot posRot;

    CrossSongWarp_HideHud(play);
    OPEN_DISPS(play->state.gfxCtx);

    if (this->sparkleCounter < 30) {
        this->sparkleCounter += 2;
    }
    for (i = this->sparkleCounter - 1; i >= 0; i--) {
        FrameInterpolation_RecordOpenChild("Kankyo Warp Sparkles", i);

        temp_f22 = 1.0f - (i / (f32)this->sparkleCounter);

        switch (this->unk_150[i].unk_22) {
            case 0:
                this->unk_150[i].unk_20 = 0;
                this->unk_150[i].unk_1C = 0;
                this->unk_150[i].unk_0.x = (s16)((Rand_ZeroOne() - 0.5f) * 16.0f * temp_f22);
                this->unk_150[i].unk_0.y = (s16)((Rand_ZeroOne() - 0.5f) * 16.0f * temp_f22);
                this->unk_150[i].unk_0.z = (s16)((Rand_ZeroOne() - 0.5f) * 16.0f * temp_f22);
                this->unk_150[i].unk_23 = 0;

                this->unk_150[i].unk_22++;

            case 1:
                if (this->actor.params == DEMOKANKYO_WARP_OUT) {
                    if (CWWarpSplineStep(&camPos, &sWarpRoll, &sWarpFoV, sWarpOutCameraPoints, &this->unk_150[i].unk_20,
                                         &this->unk_150[i].unk_1C) != 0) {
                        this->unk_150[i].unk_22++;
                    }
                }
                posRot = Actor_GetWorld(&player->actor);
                CWWarp_Vec3fAddPosRot(&posRot, &camPos, &D_8098CF98);
                break;
            case 2:
                if (this->actor.params == DEMOKANKYO_WARP_OUT) {
                    if (i == 0) {
                        Combo_MMCompleteWarp(play, this->destination);
                        // Native DemoKankyo keeps the departure actor alive until
                        // scene teardown. Killing it here restores the hidden player,
                        // camera and HUD in the final MM frame before game handoff.
                        this->unk_150[i].unk_22++;
                    }
                }
                break;
        }

        this->unk_150[i].unk_C.x = D_8098CF98.x;
        this->unk_150[i].unk_C.y = D_8098CF98.y;
        this->unk_150[i].unk_C.z = D_8098CF98.z;

        switch (this->unk_150[i].unk_23) {
            case 0:
                this->unk_150[i].unk_18 = Rand_ZeroOne();
                this->unk_150[i].unk_23++;
            case 1:
                Math_SmoothStepToF(&this->unk_150[i].unk_18, 1.0f, 0.5f, 0.4f, 0.2f);
                if (this->unk_150[i].unk_18 >= 1.0f) {
                    this->unk_150[i].unk_23 = 2;
                }
                break;
            case 2:
                Math_SmoothStepToF(&this->unk_150[i].unk_18, 0.0f, 0.5f, 0.3f, 0.2f);
                if (this->unk_150[i].unk_18 <= 0.0f) {
                    this->unk_150[i].unk_0.x = (s16)((Rand_ZeroOne() - 0.5f) * 16.0f * temp_f22);
                    this->unk_150[i].unk_0.y = (s16)((Rand_ZeroOne() - 0.5f) * 16.0f * temp_f22);
                    this->unk_150[i].unk_0.z = (s16)((Rand_ZeroOne() - 0.5f) * 16.0f * temp_f22);
                    this->unk_150[i].unk_18 = 0.0f;
                    this->unk_150[i].unk_23 = 1;
                }
                break;
        }

        translateX = this->unk_150[i].unk_C.x + this->unk_150[i].unk_0.x;
        translateY = this->unk_150[i].unk_C.y + this->unk_150[i].unk_0.y;
        translateZ = this->unk_150[i].unk_C.z + this->unk_150[i].unk_0.z;

        if (this->unk_150[i].unk_22 < 2) {
            disp = (Gfx*)sFlashDL;
            Matrix_Translate(translateX, translateY, translateZ, MTXMODE_NEW);
            Matrix_Scale(this->unk_150[i].unk_18 * (0.018f * temp_f22), this->unk_150[i].unk_18 * (0.018f * temp_f22),
                         this->unk_150[i].unk_18 * (0.018f * temp_f22), MTXMODE_APPLY);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_XLU_DISP++, sWarpSparkleEnvColors[play->msgCtx.songPlayed].r,
                           sWarpSparkleEnvColors[play->msgCtx.songPlayed].g,
                           sWarpSparkleEnvColors[play->msgCtx.songPlayed].b, 255);
            Gfx_SetupDL25_Xlu(play->state.gfxCtx);
            Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
            Matrix_RotateZF(DEG_TO_RAD(this->unk_150[i].unk_24), MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, disp);
            this->unk_150[i].unk_24 += 0x190;
        }

        FrameInterpolation_RecordCloseChild();
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

ActorProfile Combo_Song_Warp_Profile = { ACTOR_COMBO_SONG_WARP,
                                         ACTORCAT_ITEMACTION,
                                         ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED |
                                             ACTOR_FLAG_UPDATE_DURING_OCARINA | ACTOR_FLAG_FREEZE_EXCEPTION |
                                             ACTOR_FLAG_UPDATE_DURING_SOARING_AND_SOT_CS,
                                         GAMEPLAY_KEEP,
                                         sizeof(CrossSongWarp),
                                         CrossSongWarp_Init,
                                         CrossSongWarp_Destroy,
                                         CrossSongWarp_Update,
                                         CrossSongWarp_Draw };
int Combo_MMStartWarp(PlayState* play, int destination, int song) {
    if (destination < 0 || destination >= 16 || song < 0 || song >= 6)
        return 0;
    Player* player = GET_PLAYER(play);
    play->msgCtx.songPlayed = song;
    return Actor_Spawn(&play->actorCtx, play, ACTOR_COMBO_SONG_WARP, player->actor.world.pos.x,
                       player->actor.world.pos.y, player->actor.world.pos.z, 0, 0, 0, destination) != NULL;
}

#endif
