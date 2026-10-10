// Pinned OoTMM MM point-pointer behavior with named native save fields.
#ifdef COMBO_BUILD
#include "global.h"
#include "2s2h/Rando/FaroresWind.h"
extern int Combo_MMFaroreEffectiveHumanAge(void);
#include "assets/objects/gameplay_keep/gameplay_keep.h"
static LightInfo D_8015BC00;
static LightNode* D_8015BC10;
static s32 D_8015BC14;
static f32 D_8015BC18;

void Combo_MMFaroreInitPointer(PlayState* play) {
    Vec3f lightPos;
    int fwAge = Combo_MMFaroreAge();
    if (fwAge < 0)
        return;
    RespawnData* fw = &gSaveContext.save.shipSaveInfo.faroresWind.point[fwAge];

    if (fw->data) {
        gSaveContext.respawn[RESPAWN_MODE_HUMAN] = *fw;
    } else {
        gSaveContext.respawn[RESPAWN_MODE_HUMAN].data = 0;
        gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.x = 0.0f;
        gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.y = 0.0f;
        gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.z = 0.0f;
    }

    lightPos = fw->pos;
    lightPos.y += 60.0f;

    Lights_PointNoGlowSetInfo(&D_8015BC00, lightPos.x, lightPos.y, lightPos.z, 0xFF, 0xFF, 0xFF, -1);

    D_8015BC10 = LightContext_InsertLight(play, &play->lightCtx, &D_8015BC00);
    D_8015BC14 = 0;
    D_8015BC18 = 0.0f;
}

void Combo_MMFaroreDrawPointer(PlayState* play) {
    if (!Combo_MMFaroreEnabled() || Combo_MMFaroreAge() < 0 || !D_8015BC10)
        return;
    s32 lightRadius = -1;
    s32 params;

    OPEN_DISPS(play->state.gfxCtx);

    params = gSaveContext.respawn[RESPAWN_MODE_HUMAN].data;

    if (params && gSaveContext.respawn[RESPAWN_MODE_HUMAN].entrance != COMBO_FARORE_CROSS_ENTRANCE) {
        f32 yOffset = (Combo_MMFaroreEffectiveHumanAge() == 0) ? 80.0f : 60.0f;
        f32 ratio = 1.0f;
        s32 alpha = 255;
        s32 temp = params - 40;

        if (temp < 0) {
            gSaveContext.respawn[RESPAWN_MODE_HUMAN].data = ++params;
            ratio = ABS(params) * 0.025f;
            D_8015BC14 = 60;
            D_8015BC18 = 1.0f;
        } else if (D_8015BC14) {
            D_8015BC14--;
        } else if (D_8015BC18 > 0.0f) {
            static Vec3f effectVel = { 0.0f, -0.05f, 0.0f };
            static Vec3f effectAccel = { 0.0f, -0.025f, 0.0f };
            static Color_RGBA8 effectPrimCol = { 255, 255, 255, 0 };
            static Color_RGBA8 effectEnvCol = { 100, 200, 0, 0 };
            Vec3f* curPos = &gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos;
            Vec3f* nextPos = &gSaveContext.respawn[RESPAWN_MODE_DOWN].pos;
            f32 prevNum = D_8015BC18;
            Vec3f dist;
            f32 diff = Math_Vec3f_DistXYZAndStoreDiff(nextPos, curPos, &dist);
            Vec3f effectPos;
            f32 factor;
            f32 length;
            f32 dx;
            f32 speed;

            if (diff < 20.0f) {
                D_8015BC18 = 0.0f;
                Math_Vec3f_Copy(curPos, nextPos);
            } else {
                length = diff * (1.0f / D_8015BC18);
                speed = 20.0f / length;
                speed = CLAMP_MIN(speed, 0.05f);
                Math_StepToF(&D_8015BC18, 0.0f, speed);
                factor = (diff * (D_8015BC18 / prevNum)) / diff;
                curPos->x = nextPos->x + (dist.x * factor);
                curPos->y = nextPos->y + (dist.y * factor);
                curPos->z = nextPos->z + (dist.z * factor);
                length *= 0.5f;
                dx = diff - length;
                yOffset += sqrtf(SQ(length) - SQ(dx)) * 0.2f;
            }

            effectPos.x = curPos->x + Rand_CenteredFloat(6.0f);
            effectPos.y = curPos->y + 80.0f + (6.0f * Rand_ZeroOne());
            effectPos.z = curPos->z + Rand_CenteredFloat(6.0f);

            EffectSsKirakira_SpawnDispersed(play, &effectPos, &effectVel, &effectAccel, &effectPrimCol, &effectEnvCol,
                                            1000, 16);

            if (D_8015BC18 == 0.0f) {
                gSaveContext.respawn[RESPAWN_MODE_HUMAN] = gSaveContext.respawn[RESPAWN_MODE_DOWN];
                gSaveContext.respawn[RESPAWN_MODE_HUMAN].playerParams = 0x6ff;
                gSaveContext.respawn[RESPAWN_MODE_HUMAN].data = 40;
            }
        } else if (temp > 0) {
            Vec3f* curPos = &gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos;
            f32 nextRatio = 1.0f - temp * 0.1f;
            f32 curRatio = 1.0f - (f32)(temp - 1) * 0.1f;
            Vec3f eye;
            Vec3f dist;
            f32 diff;

            if (nextRatio > 0.0f) {
                eye.x = play->view.eye.x;
                eye.y = play->view.eye.y - yOffset;
                eye.z = play->view.eye.z;
                diff = Math_Vec3f_DistXYZAndStoreDiff(&eye, curPos, &dist);
                diff = (diff * (nextRatio / curRatio)) / diff;
                curPos->x = eye.x + (dist.x * diff);
                curPos->y = eye.y + (dist.y * diff);
                curPos->z = eye.z + (dist.z * diff);
            }

            alpha = 255 - (temp * 30);

            if (alpha < 0) {
                gSaveContext.save.shipSaveInfo.faroresWind.point[Combo_MMFaroreAge()].data = 0;
                gSaveContext.respawn[RESPAWN_MODE_HUMAN].data = 0;
                alpha = 0;
            } else {
                gSaveContext.respawn[RESPAWN_MODE_HUMAN].data = ++params;
            }

            ratio = 1.0f + ((f32)temp * 0.2); /* required to match */
        }

        lightRadius = 500.0f * ratio;

        s32 fwSceneId = Entrance_GetSceneIdAbsolute(gSaveContext.respawn[RESPAWN_MODE_HUMAN].entrance);
        s32 fwRoomId = gSaveContext.respawn[RESPAWN_MODE_HUMAN].roomIndex;

        s32 sceneMatches = fwSceneId == play->sceneId;

        f32 yPos = gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.y + yOffset;

        if (!sceneMatches) {
            switch (play->sceneId) {
                case SCENE_17SETUGEN:
                    sceneMatches = fwSceneId == SCENE_17SETUGEN2;
                    break;
                case SCENE_11GORONNOSATO:
                    sceneMatches = fwSceneId == SCENE_11GORONNOSATO2;
                    break;
                case SCENE_10YUKIYAMANOMURA:
                    sceneMatches = fwSceneId == SCENE_10YUKIYAMANOMURA2;
                    break;
                case SCENE_17SETUGEN2:
                    sceneMatches = fwSceneId == SCENE_17SETUGEN;
                    break;
                case SCENE_11GORONNOSATO2:
                    sceneMatches = fwSceneId == SCENE_11GORONNOSATO;
                    break;
                case SCENE_10YUKIYAMANOMURA2:
                    sceneMatches = fwSceneId == SCENE_10YUKIYAMANOMURA;
                    break;
                case SCENE_20SICHITAI2:
                    sceneMatches = fwSceneId == SCENE_20SICHITAI;
                    break;
                case SCENE_20SICHITAI:
                    sceneMatches = fwSceneId == SCENE_20SICHITAI2;
                    break;
                case SCENE_INISIE_N:
                    sceneMatches = fwSceneId == SCENE_INISIE_R;
                    yPos *= -1.0f;
                    break;
                case SCENE_INISIE_R:
                    sceneMatches = fwSceneId == SCENE_INISIE_N;
                    yPos *= -1.0f;
                    break;
            }
        }

        s32 shouldDraw = sceneMatches && (D_8015BC18 != 0.0f || fwRoomId == play->roomCtx.curRoom.num ||
                                          fwRoomId == play->roomCtx.prevRoom.num);

        if ((play->csCtx.state == CS_STATE_IDLE) && shouldDraw) {
            f32 scale = 0.025f * ratio;

            POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, 25); /* SETUPDL_25 */

            Matrix_Translate(gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.x, yPos,
                             gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.z, MTXMODE_NEW);
            Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
            Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
            Matrix_Push();

            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 128, 128, 255, 255, 200, alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 100, 200, 0, 255);

            Matrix_RotateZF(BINANG_TO_RAD_ALT2((play->gameplayFrames * 1500) & 0xFFFF), MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_Finalize(play->state.gfxCtx),
                      G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            gSPDisplayList(POLY_XLU_DISP++, (uintptr_t)gEffFlash1DL); /* gEffFlash1DL */

            Matrix_Pop();
            Matrix_RotateZF(BINANG_TO_RAD_ALT2(~((play->gameplayFrames * 1200) & 0xFFFF)), MTXMODE_APPLY);

            gSPMatrix(POLY_XLU_DISP++, Matrix_Finalize(play->state.gfxCtx),
                      G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            gSPDisplayList(POLY_XLU_DISP++, (uintptr_t)gEffFlash1DL); /* gEffFlash1DL */

            Lights_PointNoGlowSetInfo(&D_8015BC00, gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.x, yPos,
                                      gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos.z, 255, 255, 255, lightRadius);
        }
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

void Combo_MMFaroreDestroyPointer(PlayState* play) {
    if (D_8015BC10)
        LightContext_RemoveLight(play, &play->lightCtx, D_8015BC10);
    D_8015BC10 = NULL;
}
#endif
