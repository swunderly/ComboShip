#pragma once
// Exact MIT pinned Player.c hover effect geometry/color/alpha/scroll transform.
// Two textures resolve against their distinct owning user archives.
const Vtx sHoverBootsCircleVtx[] = {
#include "sHoverBootsCircleVtx.vtx.inc"
};

static const ALIGN_ASSET(2) char gComboHoverCircleTex[] = "__OTR__@oot:objects/gameplay_keep/gEffUnknown13Tex";
static const ALIGN_ASSET(2) char gComboHoverCircleDetailTex[] =
    "__OTR__@mm:objects/gameplay_keep/gameplay_keep_Tex_01AAB0";

const Gfx gComboHoverBootsCircleDL[] = {
    gsDPPipeSync(),
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock(gComboHoverCircleTex, G_IM_FMT_I, G_IM_SIZ_8b, 16, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR
                         | G_TX_CLAMP, 4, 5, G_TX_NOLOD, 1),
    gsDPLoadMultiBlock(gComboHoverCircleDetailTex, 0x0100, 1, G_IM_FMT_I, G_IM_SIZ_8b, 16, 32, 0, G_TX_MIRROR | G_TX_WRAP,
                       G_TX_NOMIRROR | G_TX_WRAP, 4, 5, 15, 14),
    gsDPSetCombineLERP(TEXEL1, PRIMITIVE, ENV_ALPHA, TEXEL0, 0, 0, 0, TEXEL0, PRIMITIVE, 0, COMBINED, ENVIRONMENT,
                       COMBINED, 0, PRIMITIVE, 0),
    gsDPSetRenderMode(G_RM_PASS, G_RM_ZB_CLD_SURF2),
    gsSPClearGeometryMode(G_CULL_BACK | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsSPDisplayList(0x09000000 | 1),
    gsSPVertex(sHoverBootsCircleVtx, 13, 0),
    gsSP2Triangles(0, 1, 2, 0, 3, 4, 2, 0),
    gsSP2Triangles(5, 6, 2, 0, 7, 8, 2, 0),
    gsSP2Triangles(9, 10, 2, 0, 11, 12, 2, 0),
    gsSPEndDisplayList(),
};


static Vec3s sHoverBootsRot={0,0,0};
static s32 sHoverBootCircleAlpha=255;
extern int Combo_MM_HoverCircleResourcesReady(void);
static void Combo_MM_DrawHoverCircle(PlayState* play,Player* player){
 if(!Combo_MM_GearActive()||!Combo_MM_HoverCircleResourcesReady())return;
 OPEN_DISPS(play->state.gfxCtx);
    if (Combo_MM_GetCustomBoots(player) == 2 && !(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
        && !(player->stateFlags1 & (1 << 23)) && player->unk_734[0] != 0)
    {
        s32 sp5C;
        s32 hoverBootsTimer = player->unk_734[0];

        if (player->unk_734[0] < 19) {
            if (hoverBootsTimer >= 15) {
                sHoverBootCircleAlpha = (19 - hoverBootsTimer) * 51.0f;
            } else if (hoverBootsTimer < 19) {
                sp5C = hoverBootsTimer;

                if (sp5C > 9) {
                    sp5C = 9;
                }

                sHoverBootCircleAlpha = (-sp5C * 4) + 36;
                sHoverBootCircleAlpha = SQ(sHoverBootCircleAlpha);
                sHoverBootCircleAlpha = (s32)((Math_CosS(sHoverBootCircleAlpha) * 100.0f) + 100.0f) + 55.0f;
                sHoverBootCircleAlpha *= sp5C * (1.0f / 9.0f);
            }

            Matrix_SetTranslateRotateYXZ(player->actor.world.pos.x, player->actor.world.pos.y + 2.0f,
                                         player->actor.world.pos.z, &sHoverBootsRot);
            Matrix_Scale(4.0f, 4.0f, 4.0f, MTXMODE_APPLY);

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++,play->state.gfxCtx);
            gSPSegment(POLY_XLU_DISP++, 0x09,
                       Gfx_TwoTexScroll(play->state.gfxCtx, G_TX_RENDERTILE, 0, 0, 16, 32, 1, 0,
                                        (play->gameplayFrames * -15) % 128, 16, 32));
            gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 255, sHoverBootCircleAlpha);
            gDPSetEnvColor(POLY_XLU_DISP++, 120, 90, 30, 128);
            gSPDisplayList(POLY_XLU_DISP++, gComboHoverBootsCircleDL);
        }
    }

 CLOSE_DISPS(play->state.gfxCtx);
}
