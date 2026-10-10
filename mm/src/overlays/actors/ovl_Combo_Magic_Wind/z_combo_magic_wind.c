// OoTMM MIT 41a2b7c0d7dbbb53fed928652f8271bc336c113f; native texture and curve adapters.
#ifdef COMBO_BUILD
/*
 * File: z_magic_wind.c
 * Overlay: ovl_Magic_Wind
 * Description: Farore's Wind
 */

#include "z_combo_magic_wind.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_UPDATE_DURING_OCARINA)

void MagicWind_Init(Actor* thisx, PlayState* play);
void MagicWind_Destroy(Actor* thisx, PlayState* play);
void MagicWind_Update(Actor* thisx, PlayState* play);
void MagicWind_Draw(Actor* thisx, PlayState* play);

void MagicWind_Shrink(MagicWind* this, PlayState* play);
void MagicWind_WaitForTimer(MagicWind* this, PlayState* play);
void MagicWind_FadeOut(MagicWind* this, PlayState* play);
void MagicWind_WaitAtFullSize(MagicWind* this, PlayState* play);
void MagicWind_Grow(MagicWind* this, PlayState* play);

static const ALIGN_ASSET(2) char sComboWindTexture[] = "__OTR__@oot:overlays/ovl_Magic_Wind/sTex";
static Vtx sCylinderVtx[36] = {
#include "sCylinderVtx.vtx.inc"
};

static Gfx sInnerCylinderDL[33] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock((uintptr_t)sComboWindTexture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, G_TX_NOMIRROR | G_TX_WRAP,
                         G_TX_NOMIRROR | G_TX_WRAP, 6, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsDPLoadMultiBlock((uintptr_t)sComboWindTexture, 0x0100, 1, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0,
                       G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 6, 6, 14, 14),
    gsDPSetCombineLERP(TEXEL1, PRIMITIVE, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, PRIMITIVE,
                       ENVIRONMENT, COMBINED, ENVIRONMENT, COMBINED, 0, SHADE, 0),
    gsDPSetRenderMode(G_RM_PASS, G_RM_AA_ZB_XLU_SURF2),
    gsSPClearGeometryMode(G_CULL_BACK | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsDPSetPrimColor(0, 0x80, 255, 255, 170, 255),
    gsDPSetEnvColor(150, 255, 0, 0),
    gsSPDisplayList(0x08000000 | 1), // Native segmented-address marker (DisplayListFactory).
    gsSPVertex(sCylinderVtx, 18, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 3, 1, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 7, 5, 0),
    gsSP2Triangles(3, 8, 1, 0, 3, 9, 8, 0),
    gsSP2Triangles(10, 2, 11, 0, 10, 0, 2, 0),
    gsSP2Triangles(9, 12, 8, 0, 9, 13, 12, 0),
    gsSP2Triangles(7, 14, 5, 0, 7, 15, 14, 0),
    gsSP2Triangles(15, 11, 14, 0, 15, 10, 11, 0),
    gsSP2Triangles(13, 16, 12, 0, 13, 17, 16, 0),
    gsSPEndDisplayList(),
};

static Gfx sOuterCylinderDL[33] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock((uintptr_t)sComboWindTexture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, G_TX_NOMIRROR | G_TX_WRAP,
                         G_TX_NOMIRROR | G_TX_WRAP, 6, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsDPLoadMultiBlock((uintptr_t)sComboWindTexture, 0x0100, 1, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0,
                       G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 6, 6, 15, 15),
    gsDPSetCombineLERP(TEXEL1, PRIMITIVE, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, PRIMITIVE,
                       ENVIRONMENT, COMBINED, ENVIRONMENT, COMBINED, 0, SHADE, 0),
    gsDPSetRenderMode(G_RM_PASS, G_RM_AA_ZB_XLU_SURF2),
    gsSPClearGeometryMode(G_CULL_BACK | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsDPSetPrimColor(0, 0x80, 255, 255, 170, 255),
    gsDPSetEnvColor(0, 150, 0, 0),
    gsSPDisplayList(0x09000000 | 1), // Native segmented-address marker (DisplayListFactory).
    gsSPVertex(&sCylinderVtx[18], 18, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 3, 1, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 7, 5, 0),
    gsSP2Triangles(3, 8, 1, 0, 3, 9, 8, 0),
    gsSP2Triangles(10, 2, 11, 0, 10, 0, 2, 0),
    gsSP2Triangles(9, 12, 8, 0, 9, 13, 12, 0),
    gsSP2Triangles(7, 14, 5, 0, 7, 15, 14, 0),
    gsSP2Triangles(15, 11, 14, 0, 15, 10, 11, 0),
    gsSP2Triangles(13, 16, 12, 0, 13, 17, 16, 0),
    gsSPEndDisplayList(),
};

static u8 ovl_Magic_WindCurveAnime_Ref_001BD0[27] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static s16 ovl_Magic_WindCurveAnime_Copy_001BEC[25] = {
    1024, 1024, 1024, 0, 0, 0, 0, 0, 0, 512, 512, 0, 0, 0, 0, 0, 0, 717, 717, 0, 0, 0, 0, 0, 0,
};

static CurveInterpKnot ovl_Magic_WindCurveAnime_CurveInterpKnot_001C20[4] = {
    { 0x000C, 0x0001, 1, 1, 0.000000f },
    { 0x0014, 0x003C, 0, 0, 1.500000f },
    { 0x000C, 0x0001, 1, 1, 0.000000f },
    { 0x0014, 0x003C, 0, 0, 1.000000f },
};

static CurveAnimationHeader sAnim = { ovl_Magic_WindCurveAnime_Ref_001BD0,
                                      ovl_Magic_WindCurveAnime_CurveInterpKnot_001C20,
                                      ovl_Magic_WindCurveAnime_Copy_001BEC, 1, 60 };

static SkelCurveLimb sRootLimb = { 0x01, LIMB_DONE, { NULL, NULL } };

static SkelCurveLimb sInnerCylinderLimb = { LIMB_DONE, 0x02, { NULL, sInnerCylinderDL } };

static SkelCurveLimb sOuterCylinderLimb = { LIMB_DONE, LIMB_DONE, { NULL, sOuterCylinderDL } };

static SkelCurveLimb* sSkelLimbs[3] = {
    &sRootLimb,
    &sInnerCylinderLimb,
    &sOuterCylinderLimb,
};

static CurveSkeletonHeader sSkel = { sSkelLimbs, ARRAY_COUNT(sSkelLimbs) };

static u8 sAlphaUpdVals[] = {
    0x00, 0x03, 0x04, 0x07, 0x09, 0x0A, 0x0D, 0x0F, 0x11, 0x12, 0x15, 0x16, 0x19, 0x1B, 0x1C, 0x1F, 0x21, 0x23,
};

void MagicWind_SetupAction(MagicWind* this, MagicWindFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void MagicWind_Init(Actor* thisx, PlayState* play) {
    MagicWind* this = (MagicWind*)thisx;
    Player* player = GET_PLAYER(play);

    if (!Combo_MMFarorePrepareResources() || !SkelCurve_Init(play, &this->skelCurve, &sSkel, &sAnim)) {
        Actor_Kill(thisx);
        return;
    }
    this->actor.room = -1;
    switch (this->actor.params) {
        case 0:
            SkelCurve_SetAnim(&this->skelCurve, &sAnim, 0.0f, 60.0f, 0.0f, 1.0f);
            this->timer = 29;
            MagicWind_SetupAction(this, MagicWind_WaitForTimer);
            break;
        case 1:
            SkelCurve_SetAnim(&this->skelCurve, &sAnim, 60.0f, 0.0f, 60.0f, -1.0f);
            MagicWind_SetupAction(this, MagicWind_Shrink);
            Player_PlaySfx(player, NA_SE_PL_MAGIC_WIND_WARP);
            break;
    }
}

void MagicWind_Destroy(Actor* thisx, PlayState* play) {
    MagicWind* this = (MagicWind*)thisx;
    SkelCurve_Destroy(play, &this->skelCurve);
    Magic_Reset(play);
}

void MagicWind_UpdateAlpha(f32 alpha) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sAlphaUpdVals); i++) {
        sCylinderVtx[sAlphaUpdVals[i]].n.a = alpha * 255.0f;
    }
}

void MagicWind_WaitForTimer(MagicWind* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->timer > 0) {
        this->timer--;
        return;
    }

    Player_PlaySfx(player, NA_SE_PL_MAGIC_WIND_NORMAL);
    MagicWind_UpdateAlpha(1.0f);
    MagicWind_SetupAction(this, MagicWind_Grow);
    SkelCurve_Update(play, &this->skelCurve);
}

void MagicWind_Grow(MagicWind* this, PlayState* play) {
    if (SkelCurve_Update(play, &this->skelCurve)) {
        MagicWind_SetupAction(this, MagicWind_WaitAtFullSize);
        this->timer = 50;
    }
}

void MagicWind_WaitAtFullSize(MagicWind* this, PlayState* play) {
    if (this->timer > 0) {
        this->timer--;
    } else {
        MagicWind_SetupAction(this, MagicWind_FadeOut);
        this->timer = 30;
    }
}

void MagicWind_FadeOut(MagicWind* this, PlayState* play) {
    if (this->timer > 0) {
        MagicWind_UpdateAlpha((f32)this->timer * (1.0f / 30.0f));
        this->timer--;
    } else {
        Actor_Kill(&this->actor);
    }
}

void MagicWind_Shrink(MagicWind* this, PlayState* play) {
    if (SkelCurve_Update(play, &this->skelCurve)) {
        Actor_Kill(&this->actor);
    }
}

void MagicWind_Update(Actor* thisx, PlayState* play) {
    MagicWind* this = (MagicWind*)thisx;

    /* See `ACTOROVL_ALLOC_ABSOLUTE` */
    /*! @bug This condition is too broad, the actor will also be killed by warp songs. But warp songs do not use an */
    /*! actor which uses `ACTOROVL_ALLOC_ABSOLUTE`. There is no reason to kill the actor in this case. */
    /* if ((play->msgCtx.msgMode == MSGMODE_OCARINA_CORRECT_PLAYBACK) || (play->msgCtx.msgMode == MSGMODE_SONG_PLAYED))
     * { */
    /*     Actor_Kill(thisx); */
    /*     return; */
    /* } */

    this->actionFunc(this, play);
}

s32 MagicWind_OverrideLimbDraw(PlayState* play, SkelCurve* skelCurve, s32 limbIndex, Actor* thisx) {
    OPEN_DISPS(play->state.gfxCtx);

    if (limbIndex == 1) {
        gSPSegment(POLY_XLU_DISP++, 8,
                   Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, (play->state.frames * 9) & 0xFF,
                                    0xFF - ((play->state.frames * 0xF) & 0xFF), 0x40, 0x40, 1,
                                    (play->state.frames * 0xF) & 0xFF, 0xFF - ((play->state.frames * 0x1E) & 0xFF),
                                    0x40, 0x40));

    } else if (limbIndex == 2) {
        gSPSegment(POLY_XLU_DISP++, 9,
                   Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, (play->state.frames * 3) & 0xFF,
                                    0xFF - ((play->state.frames * 5) & 0xFF), 0x40, 0x40, 1,
                                    (play->state.frames * 6) & 0xFF, 0xFF - ((play->state.frames * 0xA) & 0xFF), 0x40,
                                    0x40));
    }

    CLOSE_DISPS(play->state.gfxCtx);

    return 1;
}

void MagicWind_Draw(Actor* thisx, PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    MagicWind* this = (MagicWind*)thisx;

    OPEN_DISPS(gfxCtx);

    /* Texture and native arrays are owned, so no foreign segment base is installed. */

    if (this->actionFunc != MagicWind_WaitForTimer) {
        POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, 25); /* SETUPDL_25 */
        SkelCurve_Draw(&this->actor, play, &this->skelCurve, MagicWind_OverrideLimbDraw, NULL, 1, NULL);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

ActorProfile Combo_Magic_Wind_Profile = {
    ACTOR_COMBO_MAGIC_WIND,
    ACTORCAT_ITEMACTION,
    FLAGS,
    1,
    sizeof(MagicWind),
    (ActorFunc)MagicWind_Init,
    (ActorFunc)MagicWind_Destroy,
    (ActorFunc)MagicWind_Update,
    (ActorFunc)MagicWind_Draw,
    NULL,
};

#endif
