#ifdef COMBO_BUILD
#include "CheckTracker/CheckTracker.h"
#include <cmath>
#endif
#include "Rando/Rando.h"
#include "Rando/FaroresWind.h"
#include "Rando/Spells.h"
#include "Rando/ActorBehavior/ExpandedSoulsNative.h"
namespace Rando {
bool DrawExpandedSoulItem(RandoItemId);
}
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/Rando/DrawFuncs.h"
#include "2s2h_assets.h"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_gi_melody/object_gi_melody.h"
#include "assets/objects/object_gi_key/object_gi_key.h"
#include "assets/objects/object_gi_bosskey/object_gi_bosskey.h"
#include "objects/object_gi_hearts/object_gi_hearts.h"
#include "objects/object_gi_liquid/object_gi_liquid.h"
#include "objects/object_sek/object_sek.h"
#include "objects/object_st/object_st.h"

#include "assets/overlays/ovl_Arrow_Ice/ovl_Arrow_Ice.h"
#include "assets/objects/object_gi_purse/object_gi_purse.h"

Gfx* ResourceMgr_LoadGfxByName(const char* path);
#ifdef COMBO_BUILD
float OTRGetAspectRatio(void);
#endif
}

#ifdef COMBO_BUILD
// ComboShip: cross-game foreign-item rendering. A check holding RI_COMBO_FOREIGN actually holds an
// OOT item; MM_DrawComboForeign renders the real OOT model via "@oot:" cross-RM routing (sentinel
// blue rupee when it can't be resolved). Bodies live in the combo-owned TU-glue header, included
// after the engine headers above (outside the extern "C" block — it is C++). Mirror of soh's
// Randomizer_DrawComboForeign.
#include "ComboForeignDrawMM.h"

namespace Rando {
bool DrawComboForeignRenewable(const char* name) {
    return MM_DrawComboForeignNamed(name);
}
bool CanDrawComboForeignRenewable(const char* name) {
    ComboForeignDrawInfoOOT info{};
    return name && name[0] && ComboFillForeignDrawInfoOOT(RC_UNKNOWN, info, name) == ComboForeignResolveOOT::Ok;
}
} // namespace Rando

void Rando::LatchComboForeign(RandoCheckId randoCheckId) {
    ComboLatchForeignDrawOOT(randoCheckId);
}

const char* Rando::ComboForeignLatchedName(RandoCheckId randoCheckId) {
    return ComboForeignLatchedNameOOT(randoCheckId);
}

const char* Rando::ComboForeignLiveName(RandoCheckId randoCheckId) {
    return ComboForeignLiveNameOOT(randoCheckId);
}
#endif

s32 StrayFairyOverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* thisx,
                               Gfx** gfx) {
    if (limbIndex == STRAY_FAIRY_LIMB_RIGHT_FACING_HEAD) {
        *dList = NULL;
    }

    return false;
}

void DrawStrayFairy(RandoItemId randoItemId) {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    switch (randoItemId) {
        case RI_WOODFALL_STRAY_FAIRY:
            AnimatedMat_Draw(gPlayState, (AnimatedMaterial*)&gStrayFairyWoodfallTexAnim);
            break;
        case RI_SNOWHEAD_STRAY_FAIRY:
            AnimatedMat_Draw(gPlayState, (AnimatedMaterial*)&gStrayFairySnowheadTexAnim);
            break;
        case RI_GREAT_BAY_STRAY_FAIRY:
            AnimatedMat_Draw(gPlayState, (AnimatedMaterial*)&gStrayFairyGreatBayTexAnim);
            break;
        case RI_STONE_TOWER_STRAY_FAIRY:
            AnimatedMat_Draw(gPlayState, (AnimatedMaterial*)&gStrayFairyStoneTowerTexAnim);
            break;
        default: // STRAY_FAIRY_AREA_CLOCK_TOWN
            AnimatedMat_Draw(gPlayState, (AnimatedMaterial*)&gStrayFairyClockTownTexAnim);
            break;
    }

    Matrix_ReplaceRotation(&gPlayState->billboardMtxF);
    Matrix_Scale(0.03f, 0.03f, 0.03f, MTXMODE_APPLY);

    // Kind of a hack to draw the stray fairy, the drawback of this is that all stray fairies in the scene will animate
    // together, but worse is that the more there are the faster their animation will play (because of the
    // SkelAnime_Update below). This is still better than the previous solution which hand drew the fairy with DL
    // calls...
    static bool initialized = false;
    static SkelAnime skelAnime;
    static Vec3s jointTable[STRAY_FAIRY_LIMB_MAX];
    static u32 lastUpdate = 0;
    if (!initialized) {
        initialized = true;
        SkelAnime_InitFlex(gPlayState, &skelAnime, (FlexSkeletonHeader*)&gStrayFairySkel,
                           (AnimationHeader*)&gStrayFairyFlyingAnim, jointTable, jointTable, STRAY_FAIRY_LIMB_MAX);
    }
    if (gPlayState != NULL && lastUpdate != gPlayState->state.frames) {
        lastUpdate = gPlayState->state.frames;
        SkelAnime_Update(&skelAnime);
    }
    POLY_XLU_DISP = SkelAnime_DrawFlex(gPlayState, skelAnime.skeleton, skelAnime.jointTable, skelAnime.dListCount,
                                       StrayFairyOverrideLimbDraw, NULL, NULL, POLY_XLU_DISP);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawSong(RandoItemId randoItemId) {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);

    switch (randoItemId) {
        case RI_SONG_SUN:
            gDPSetEnvColor(POLY_XLU_DISP++, 237, 231, 62, 255);
            break;
        case RI_SONG_DOUBLE_TIME:
        case RI_SONG_INVERTED_TIME:
        case RI_SONG_TIME:
            gDPSetEnvColor(POLY_XLU_DISP++, 98, 177, 211, 255);
            break;
        case RI_SONG_HEALING:
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 150, 230, 255);
            break;
        case RI_SONG_STORMS:
            gDPSetEnvColor(POLY_XLU_DISP++, 146, 146, 146, 255);
            break;
        case RI_SONG_SARIA:
        case RI_SONG_SONATA:
            gDPSetEnvColor(POLY_XLU_DISP++, 98, 255, 98, 255);
            break;
        case RI_SONG_SOARING:
            gDPSetEnvColor(POLY_XLU_DISP++, 200, 160, 255, 255);
            break;
        case RI_SONG_ELEGY:
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 98, 0, 255);
            break;
        case RI_SONG_LULLABY_INTRO:
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 100, 100, 255);
            break;
        case RI_SONG_LULLABY:
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 20, 20, 255);
            break;
        case RI_SONG_OATH:
            gDPSetEnvColor(POLY_XLU_DISP++, 98, 0, 98, 255);
            break;
        case RI_SONG_EPONA:
            gDPSetEnvColor(POLY_XLU_DISP++, 146, 87, 49, 255);
            break;
        case RI_SONG_NOVA:
            gDPSetEnvColor(POLY_XLU_DISP++, 20, 20, 255, 255);
            break;
        default:
            break;
    }

    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)&gGiSongNoteDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawDoubleDefense() {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPGrayscale(POLY_XLU_DISP++, true);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)&gGiHeartBorderDL);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, 255, 0, 0, 100);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)&gGiHeartContainerDL);
    gSPGrayscale(POLY_XLU_DISP++, false);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawMilkRefill() {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gPlayState->state.gfxCtx, G_TX_RENDERTILE, -gPlayState->state.frames,
                                             gPlayState->state.frames, 32, 32, 1, -gPlayState->state.frames,
                                             gPlayState->state.frames, 32, 32, -1, 1, -1, 1));
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    // Container Color
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 200, 200, 200, 255);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gGiPotionContainerPotDL);
    // Liquid Color
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 200, 200, 200, 255);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gGiPotionContainerLiquidDL);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    // Pattern Color
    // Milk
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 13, 33, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 100, 100, 255, 255);
    gDPLoadTextureBlock(POLY_XLU_DISP++, gGiPotionContainerBluePatternTex, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 32, 0,
                        G_TX_MIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_CLAMP, 4, 5, G_TX_NOLOD, G_TX_NOLOD);
    // Chateau
    // gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 250, 225, 78, 255);
    // gDPSetEnvColor(POLY_XLU_DISP++, 184, 42, 119, 255);
    // gDPLoadTextureBlock(POLY_XLU_DISP++, gGiPotionContainerRedPatternTex, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 32, 0,
    // G_TX_MIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_CLAMP, 4, 5, G_TX_NOLOD, G_TX_NOLOD);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gGiPotionContainerPatternDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawOwlStatue() {
    Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
    Matrix_Translate(0, -3000, 0, MTXMODE_APPLY);
    Gfx_DrawDListOpa(gPlayState, (Gfx*)gOwlStatueOpenedDL);
}

static Gfx gGiSmallKeyCopyDL[75];

void DrawSmallKey(RandoItemId randoItemId) {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    switch (randoItemId) {
        case RI_WOODFALL_KEY_RING:
        case RI_WOODFALL_SMALL_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 236, 120, 186, 255);
            break;
        case RI_SNOWHEAD_KEY_RING:
        case RI_SNOWHEAD_SMALL_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 129, 173, 70, 255);
            break;
        case RI_GREAT_BAY_KEY_RING:
        case RI_GREAT_BAY_SMALL_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 99, 90, 183, 255);
            break;
        case RI_STONE_TOWER_KEY_RING:
        case RI_STONE_TOWER_SMALL_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 177, 165, 83, 255);
            break;
        default:
            break;
    }

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, gGiSmallKeyCopyDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

static Gfx gGiBossKeyCopyDL[87];

void DrawBossKey(RandoItemId randoItemId) {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    switch (randoItemId) {
        case RI_WOODFALL_BOSS_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 236, 120, 186, 255);
            break;
        case RI_SNOWHEAD_BOSS_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 129, 173, 70, 255);
            break;
        case RI_GREAT_BAY_BOSS_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 99, 90, 183, 255);
            break;
        case RI_STONE_TOWER_BOSS_KEY:
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_OPA_DISP++, 177, 165, 83, 255);
            break;
        default:
            break;
    }

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, gGiBossKeyCopyDL);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gGiBossKeyGemDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

static Gfx gSkulltulaTokenFlameCopyDL[76];

void DrawSkulltulaToken(RandoItemId randoItemId, Actor* actor) {
    // It is not known why this happens, but the eyes on the skulltula tokens disappear if they are are perfectly
    // parallel with the camera. This most likely a problem in our Fast3D (maybe z-index stuff?).
    // Tilting the token down by 16 units seems to be enough to get it to always render the eyes without being
    // noticeable that it is tilted. This issue was most prevalent for tokens in shops.
    Matrix_RotateXS(16, MTXMODE_APPLY);

    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gSkulltulaTokenDL);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    if (randoItemId == RI_GS_TOKEN_OCEAN) {
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 0, 255, 255, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 0, 0, 255, 255);
    } else {
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 0, 255, 170, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, 0, 255, 0, 255);
    }

    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gPlayState->state.gfxCtx, G_TX_RENDERTILE, gPlayState->state.frames * 0,
                                             -(gPlayState->state.frames * 5), 32, 32, 1, gPlayState->state.frames * 0,
                                             gPlayState->state.frames * 0, 32, 64, 0, -5, 0, 0));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gSkulltulaTokenFlameCopyDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawTrapModel() {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);
    Matrix_Scale(0.03f, 0.03f, 0.03f, MTXMODE_APPLY);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gTrapDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawSkeletonKey() {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    Matrix_Scale(0.8f, 0.8f, 0.8f, MTXMODE_APPLY);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 170, 255);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gSkeletonKeyDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawTriforcePiece(RandoItemId randoItemId) {
    Gfx* triforcePieceModels[3] = {
        (Gfx*)gTriforcePiece0DL,
        (Gfx*)gTriforcePiece1DL,
        (Gfx*)gTriforcePiece2DL,
    };

    u16 currentTriforcePieces = gSaveContext.save.shipSaveInfo.rando.foundTriforcePieces;

    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    Matrix_Scale(0.03f, 0.03f, 0.03f, MTXMODE_APPLY);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    if (randoItemId >= RI_TRIFORCE_POWER && randoItemId <= RI_TRIFORCE_WISDOM) {
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)triforcePieceModels[randoItemId - RI_TRIFORCE_POWER]);
    } else if (currentTriforcePieces >= RANDO_SAVE_OPTIONS[RO_TRIFORCE_PIECES_REQUIRED]) {
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gTriforcePieceCompletedDL);
    } else {
        if (randoItemId == RI_TRIFORCE_PIECE_PREVIOUS) {
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)triforcePieceModels[(currentTriforcePieces - 1) % 3]);
        } else {
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)triforcePieceModels[currentTriforcePieces % 3]);
        }
    }

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void DrawAbilityItem(RandoItemId randoItemId, Actor* actor) {
    Gfx* abilityItemModel[1] = {
        (Gfx*)gGiFlippersDL,
    };

    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)abilityItemModel[randoItemId - RI_ABILITY_SWIM]);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

#ifdef COMBO_BUILD
// Bit zero is reserved for segmented addresses by the native GBI bridge.
static const ALIGN_ASSET(2) char sComboSpellDiamondDL[] =
    "__OTR__@oot:objects/object_gi_goddess/gGiMagicSpellDiamondDL";
static const ALIGN_ASSET(2) char sComboFaroresWindColorDL[] =
    "__OTR__@oot:objects/object_gi_goddess/gGiFaroresWindColorDL";
static const ALIGN_ASSET(2) char sComboSpellOrbDL[] = "__OTR__@oot:objects/object_gi_goddess/gGiMagicSpellOrbDL";
static const ALIGN_ASSET(2) char sComboNayrusLoveColorDL[] =
    "__OTR__@oot:objects/object_gi_goddess/gGiNayrusLoveColorDL";
static const ALIGN_ASSET(2) char sComboDinsFireColorDL[] = "__OTR__@oot:objects/object_gi_goddess/gGiDinsFireColorDL";
// Native Fairy pickup/world rendering copies OOT's exact OPA0/XLU1 recipe.
extern "C" int Combo_MM_AdultMaskResourcesReady(void);
extern "C" Gfx* Combo_MM_GetAdultMaskBinding(int index);
extern "C" int Combo_MM_AdultGearGiReady(void);
#include "rando/AdultGearDrawRecipe.h"
static void DrawAdultGearItem(int kind) {
    const auto* recipe = ComboRando::AdultGearRecipe(kind);
    if (!recipe || !Combo_MM_AdultGearGiReady())
        return;
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    const int opaCount = recipe->xlu < 0 ? recipe->count : recipe->xlu;
    for (int i = 0; i < opaCount; ++i)
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)recipe->paths[i]);
    if (recipe->xlu >= 0) {
        Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
        for (int i = recipe->xlu; i < recipe->count; ++i)
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)recipe->paths[i]);
    }
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}
static void DrawAdultMaskItem() {
    if (!Combo_MM_AdultMaskResourcesReady())
        return;
    Gfx* first = Combo_MM_GetAdultMaskBinding(1);
    Gfx* second = Combo_MM_GetAdultMaskBinding(0);
    if (!first || !second)
        return;
    // Exact pinned drawgi.yml MASK_ADULT: Opa01, params MASK_ADULT_1 then MASK_ADULT_0.
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, first);
    gSPDisplayList(POLY_OPA_DISP++, second);
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}
static void DrawFaroresWindItem() {
    if (!Combo_MMFarorePrepareResources())
        return;
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gPlayState->state.gfxCtx, 0, gPlayState->state.frames * 2,
                                             -(gPlayState->state.frames * 6), 32, 32, 1, gPlayState->state.frames,
                                             -(gPlayState->state.frames * 2), 32, 32, 2, -6, 1, -2));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)sComboSpellDiamondDL);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)sComboFaroresWindColorDL);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)sComboSpellOrbDL);
    CLOSE_DISPS(gPlayState->state.gfxCtx);
    const int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}
static void DrawExtraSpellItem(int spell) {
    if (!Combo_MMSpellPrepareResources(spell))
        return;
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gPlayState->state.gfxCtx, 0, gPlayState->state.frames * 2,
                                             -(gPlayState->state.frames * 6), 32, 32, 1, gPlayState->state.frames,
                                             -(gPlayState->state.frames * 2), 32, 32, 2, -6, 1, -2));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)sComboSpellDiamondDL);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)(spell == 1 ? sComboNayrusLoveColorDL : sComboDinsFireColorDL));
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)sComboSpellOrbDL);
    CLOSE_DISPS(gPlayState->state.gfxCtx);
    const int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

static void DrawFairyOcarinaItem() {
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)"__OTR__@oot:objects/object_gi_ocarina_0/gGiOcarinaFairyDL");
    Gfx_SetupDL25_Xlu(gPlayState->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)"__OTR__@oot:objects/object_gi_ocarina_0/gGiOcarinaFairyHolesDL");
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}
#endif

void DrawOcarinaButtonItem(RandoItemId randoItemId, Actor* actor) {
    Gfx* ocarinaButtonModel[5] = {
        (Gfx*)gOcarinaAButtonDL,     (Gfx*)gOcarinaCDownButtonDL, (Gfx*)gOcarinaCRightButtonDL,
        (Gfx*)gOcarinaCLeftButtonDL, (Gfx*)gOcarinaCUpButtonDL,
    };

    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)ocarinaButtonModel[randoItemId - RI_OCARINA_BUTTON_A]);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

// clang-format off
std::unordered_map<RandoItemId, std::function<void()>> soulDrawMap = {
    { RI_SOUL_BOSS_IGOS, DrawIgosDuIkana },
    { RI_SOUL_ENEMY_BIO_BABA, DrawBioBabaSoul },
    { RI_SOUL_ENEMY_FLOORMASTER, DrawWallmaster },
    { RI_SOUL_ENEMY_FLYING_POT, DrawFlyingPot },
    { RI_SOUL_ENEMY_REAL_BOMBCHU, DrawRealBombchu },
    { RI_SOUL_ENEMY_SKULLWALLTULA, DrawSkulltula },

    { RI_SOUL_ENEMY_ALIEN,          DrawAlien },
    { RI_SOUL_ENEMY_ARMOS,          DrawArmos },
    { RI_SOUL_ENEMY_BAD_BAT,        DrawBat },
    { RI_SOUL_ENEMY_BEAMOS,         DrawBeamos },
    { RI_SOUL_ENEMY_BUBBLE,         DrawBubble },
    { RI_SOUL_ENEMY_BOE,            DrawBoe },
    { RI_SOUL_ENEMY_CHUCHU,         DrawChuchu },
    { RI_SOUL_ENEMY_CAPTAIN_KEETA,  DrawCaptainKeeta },
    { RI_SOUL_ENEMY_DEATH_ARMOS,    DrawDeathArmos },
    { RI_SOUL_ENEMY_DEEP_PYTHON,    DrawDeepPython },
    { RI_SOUL_ENEMY_DEKU_BABA,      DrawDekuBaba },
    { RI_SOUL_ENEMY_DEXIHAND,       DrawDexihand },
    { RI_SOUL_ENEMY_DINOLFOS,       DrawDinolfos },
    { RI_SOUL_ENEMY_DODONGO,        DrawDodongo },
    { RI_SOUL_ENEMY_DRAGONFLY,      DrawDragonfly },
    { RI_SOUL_ENEMY_EENO,           DrawEeno },
    { RI_SOUL_ENEMY_EYEGORE,        DrawEyegore },
    { RI_SOUL_ENEMY_FREEZARD,       DrawFreezard },
    { RI_SOUL_ENEMY_GARO,           DrawGaro },
    { RI_SOUL_ENEMY_GEKKO,          DrawGekko },
    { RI_SOUL_ENEMY_GIANT_BEE,      DrawGiantBee },
    { RI_SOUL_ENEMY_GOMESS,         DrawGomess },
    { RI_SOUL_ENEMY_GUAY,           DrawGuay },
    { RI_SOUL_ENEMY_HIPLOOP,        DrawHiploop },
    { RI_SOUL_ENEMY_IGOS_DU_IKANA,  DrawIgosDuIkana },
    { RI_SOUL_ENEMY_IRON_KNUCKLE,   DrawIronKnuckle },
    { RI_SOUL_ENEMY_KEESE,          DrawKeese },
    { RI_SOUL_ENEMY_LEEVER,         DrawLeever },
    { RI_SOUL_ENEMY_LIKE_LIKE,      DrawLikeLike },
    { RI_SOUL_ENEMY_MAD_SCRUB,      DrawMadScrub },
    { RI_SOUL_ENEMY_NEJIRON,        DrawNejiron },
    { RI_SOUL_ENEMY_OCTOROK,        DrawOctorok },
    { RI_SOUL_ENEMY_PEAHAT,         DrawPeahat },
    { RI_SOUL_ENEMY_PIRATE,         DrawPirate },
    { RI_SOUL_ENEMY_POE,            DrawPoe },
    { RI_SOUL_ENEMY_REDEAD,         DrawRedead },
    { RI_SOUL_ENEMY_SHELLBLADE,     DrawShellBlade },
    { RI_SOUL_ENEMY_SKULLFISH,      DrawSkullfish },
    { RI_SOUL_ENEMY_SKULLTULA,      DrawSkulltula },
    { RI_SOUL_ENEMY_SNAPPER,        DrawSnapper },
    { RI_SOUL_ENEMY_STALCHILD,      DrawStalchild },
    { RI_SOUL_ENEMY_TAKKURI,        DrawTakkuri },
    { RI_SOUL_ENEMY_TEKTITE,        DrawTektite },
    { RI_SOUL_ENEMY_WALLMASTER,     DrawWallmaster },
    { RI_SOUL_ENEMY_WART,           DrawWart },
    { RI_SOUL_ENEMY_WIZROBE,        DrawWizrobe },
    { RI_SOUL_ENEMY_WOLFOS,         DrawWolfos },
};
// clang-format on

void DrawSoul(RandoItemId randoItemId) {
    auto it = soulDrawMap.find(randoItemId);
    if (it != soulDrawMap.end()) {
        it->second();
    }
}

void DrawSparkles(RandoItemId randoItemId, Actor* actor) {
    if (actor == NULL) {
        return;
    }

    if (gGameState->frames % 2 == 0) {
        return;
    }

    static Vec3f sVelocity = { 0.0f, 0.0f, 0.0f };
    static Vec3f sAccel = { 0.0f, 0.0f, 0.0f };
    static Color_RGBA8 sPrimColor = { 255, 255, 255, 255 };
    static Color_RGBA8 sEnvColor = { 255, 128, 0, 255 };
    Vec3f newPos;

    newPos.x = Rand_CenteredFloat(10.0f) + actor->world.pos.x;
    newPos.y = (Rand_ZeroOne() * 10.0f) + actor->world.pos.y;
    newPos.z = Rand_CenteredFloat(10.0f) + actor->world.pos.z;

    if (actor->id == ACTOR_EN_SI) {
        newPos.y = (Rand_ZeroOne() * 10.0f) + actor->world.pos.y - 5.0f;
    } else if (actor->id == ACTOR_EN_ITEM00) {
        newPos.x = Rand_CenteredFloat(20.0f) + actor->world.pos.x;
        newPos.y = (Rand_ZeroOne() * 10.0f) + actor->world.pos.y + 10.0f;
        newPos.z = Rand_CenteredFloat(20.0f) + actor->world.pos.z;
    }

    EffectSsKirakira_SpawnDispersed(gPlayState, &newPos, &sVelocity, &sAccel, &sPrimColor, &sEnvColor, 2000, 16);
}

void DrawTycoonWallet() {
    OPEN_DISPS(gPlayState->state.gfxCtx);

    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);

    // Wallet body - use Giant's color DL for render state, then override to purple
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiGiantsWalletColorDL);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 150, 0, 200, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 80, 0, 120, 255);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiWalletDL);

    // Rupee outer - keep Giant's Wallet default colors
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiGiantsWalletRupeeOuterColorDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiWalletRupeeOuterDL);

    // String - keep Giant's Wallet default colors
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiGiantsWalletStringColorDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiWalletStringDL);

    // Rupee inner - keep Giant's Wallet default colors
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiGiantsWalletRupeeInnerColorDL);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)&gGiWalletRupeeInnerDL);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

#ifdef COMBO_BUILD
static bool IsCheckModelVisible(Actor* actor) {
    if (!actor || !gPlayState)
        return false;
    // The camera can sweep past a reward while a scene fade or pause overlay hides the view.
    if (gPlayState->transitionMode != TRANS_MODE_OFF || gPlayState->transitionTrigger != TRANS_TRIGGER_OFF ||
        gPlayState->envCtx.fillScreen || gPlayState->pauseCtx.state != 0)
        return false;
    // Some rewards keep drawing with zero scale until their reveal animation.
    const MtxF* model = Matrix_GetCurrent();
    if (model->xx * model->xx + model->yx * model->yx + model->zx * model->zx < 0.000001f ||
        model->xy * model->xy + model->yy * model->yy + model->zy * model->zy < 0.000001f ||
        model->xz * model->xz + model->yz * model->yz + model->zz * model->zz < 0.000001f)
        return false;

    // Use the translated model origin, not the actor's feet or its generous culling volume.
    Vec3f pos;
    Matrix_MultZero(&pos);
    Vec3f projected;
    float w;
    SkinMatrix_Vec3fMtxFMultXYZW(&gPlayState->viewProjectionMtxF, &pos, &projected, &w);
    // Match Fast3D's horizontal aspect correction, including narrow phone viewports.
    const float horizontalScale = OTRGetAspectRatio() / (4.0f / 3.0f);
    if (!(std::isfinite(w) && w > 0 && std::isfinite(horizontalScale) && horizontalScale > 0 && projected.z >= -w &&
          projected.z <= w && fabsf(projected.x) <= w * horizontalScale && fabsf(projected.y) <= w))
        return false;

    // Match OoT's shop observer: invisible shelf/counter interaction collision
    // blocks a world ray even though the displayed stock is visible on screen.
    // This runs only for a submitted shop-item model, after the frustum checks.
    if (actor->id == ACTOR_EN_GIRLA)
        return true;

    // A submitted model can still be behind a maze wall, floor, ceiling or moving door.
    // Test both faces and all collision categories; camera-only tests ignore some barriers.
    Vec3f eye = gPlayState->view.eye;
    Vec3f hit;
    CollisionPoly* poly = nullptr;
    return !BgCheck_AnyLineTest2(&gPlayState->colCtx, &eye, &pos, &hit, &poly, true, true, true, false);
}

static void RecordVisibleCheck(RandoCheckId check, RandoItemId shown, Actor* actor) {
    if (IsCheckModelVisible(actor))
        Rando::CheckTracker::RecordSeenItem(check, shown);
}
#endif

void Rando::DrawItem(RandoItemId randoItemId, RandoCheckId randoCheckId, Actor* actor) {
#ifdef COMBO_BUILD
    // Traps/junk/progressives recurse into the displayed item, which records itself. Foreign models report
    // below only on successful drawing; their fallback blue rupee must not reveal the hidden placement.
    // The visibility ray test runs only while the sighting would still change the tracker.
    switch (randoItemId) {
        case RI_NONE:
        case RI_JUNK:
        case RI_TRAP:
        case RI_COMBO_FOREIGN:
        case RI_PROGRESSIVE_OCARINA:
        case RI_PROGRESSIVE_LULLABY:
        case RI_PROGRESSIVE_MAGIC:
        case RI_PROGRESSIVE_BOW:
        case RI_PROGRESSIVE_BOMB_BAG:
        case RI_PROGRESSIVE_SWORD:
        case RI_PROGRESSIVE_WALLET:
            break;
        default:
            if (Rando::CheckTracker::NeedsSighting(randoCheckId, randoItemId))
                RecordVisibleCheck(randoCheckId, randoItemId, actor);
            break;
    }
#endif
    // Apply hilites with actor world pos before drawing
    if (actor != NULL) {
        func_800B8118(actor, gPlayState, 0);
        func_800B8050(actor, gPlayState, 0);
    }

    if (Rando::ExpandedSoulIndex(randoItemId) >= 0 && Rando::DrawExpandedSoulItem(randoItemId))
        return;

    switch (randoItemId) {
#ifdef COMBO_BUILD
        case RI_BOOTS_IRON:
            DrawAdultGearItem(0);
            break;
        case RI_BOOTS_HOVER:
            DrawAdultGearItem(1);
            break;
        case RI_TUNIC_GORON:
            DrawAdultGearItem(2);
            break;
        case RI_TUNIC_ZORA:
            DrawAdultGearItem(3);
            break;
        case RI_MASK_ADULT:
            DrawAdultMaskItem();
            break;
        case RI_FARORES_WIND:
            DrawFaroresWindItem();
            break;
        case RI_DINS_FIRE:
            DrawExtraSpellItem(2);
            break;
        case RI_NAYRUS_LOVE:
            DrawExtraSpellItem(1);
            break;
        case RI_OCARINA_FAIRY:
            DrawFairyOcarinaItem();
            break;
#endif

        case RI_JUNK:
            Rando::DrawItem(Rando::CurrentJunkItem(randoCheckId), randoCheckId, actor);
            break;
        case RI_GREAT_BAY_KEY_RING:
        case RI_GREAT_BAY_SMALL_KEY:
        case RI_SNOWHEAD_KEY_RING:
        case RI_SNOWHEAD_SMALL_KEY:
        case RI_STONE_TOWER_KEY_RING:
        case RI_STONE_TOWER_SMALL_KEY:
        case RI_WOODFALL_KEY_RING:
        case RI_WOODFALL_SMALL_KEY:
            DrawSmallKey(randoItemId);
            break;
        case RI_GREAT_BAY_BOSS_KEY:
        case RI_SNOWHEAD_BOSS_KEY:
        case RI_STONE_TOWER_BOSS_KEY:
        case RI_WOODFALL_BOSS_KEY:
            DrawBossKey(randoItemId);
            break;
        case RI_SONG_TIME:
        case RI_SONG_STORMS:
        case RI_SONG_SUN:
        case RI_SONG_HEALING:
        case RI_SONG_SARIA:
        case RI_SONG_SOARING:
        case RI_SONG_SONATA:
        case RI_SONG_ELEGY:
        case RI_SONG_LULLABY_INTRO:
        case RI_SONG_LULLABY:
        case RI_SONG_OATH:
        case RI_SONG_EPONA:
        case RI_SONG_NOVA:
        case RI_SONG_DOUBLE_TIME:
        case RI_SONG_INVERTED_TIME:
            DrawSong(randoItemId);
            break;
        case RI_CLOCK_TOWN_STRAY_FAIRY:
        case RI_WOODFALL_STRAY_FAIRY:
        case RI_SNOWHEAD_STRAY_FAIRY:
        case RI_GREAT_BAY_STRAY_FAIRY:
        case RI_STONE_TOWER_STRAY_FAIRY:
            DrawStrayFairy(randoItemId);
            break;
        case RI_DOUBLE_DEFENSE:
            DrawDoubleDefense();
            break;
        case RI_MILK_REFILL:
            DrawMilkRefill();
            break;
        case RI_GS_TOKEN_SWAMP:
        case RI_GS_TOKEN_OCEAN:
            DrawSkulltulaToken(randoItemId, actor);
            break;
        case RI_OWL_CLOCK_TOWN_SOUTH:
        case RI_OWL_GREAT_BAY_COAST:
        case RI_OWL_IKANA_CANYON:
        case RI_OWL_MILK_ROAD:
        case RI_OWL_MOUNTAIN_VILLAGE:
        case RI_OWL_SNOWHEAD:
        case RI_OWL_SOUTHERN_SWAMP:
        case RI_OWL_STONE_TOWER:
        case RI_OWL_WOODFALL:
        case RI_OWL_ZORA_CAPE:
            DrawOwlStatue();
            break;
        case RI_TIME_DAY_1:
        case RI_TIME_NIGHT_1:
        case RI_TIME_DAY_2:
        case RI_TIME_NIGHT_2:
        case RI_TIME_DAY_3:
        case RI_TIME_NIGHT_3:
        case RI_TIME_PROGRESSIVE:
            DrawClock(randoItemId, actor);
            break;
        case RI_WALLET_TYCOON:
            DrawTycoonWallet();
            break;
        case RI_PROGRESSIVE_OCARINA:
        case RI_PROGRESSIVE_LULLABY:
        case RI_PROGRESSIVE_MAGIC:
        case RI_PROGRESSIVE_BOW:
        case RI_PROGRESSIVE_BOMB_BAG:
        case RI_PROGRESSIVE_SWORD:
        case RI_PROGRESSIVE_WALLET:
            Rando::DrawItem(Rando::ConvertItem(randoItemId, randoCheckId), randoCheckId, actor);
            break;
        case RI_SOUL_ENEMY_ALIEN:
        case RI_SOUL_ENEMY_ARMOS:
        case RI_SOUL_ENEMY_BAD_BAT:
        case RI_SOUL_ENEMY_BEAMOS:
        case RI_SOUL_ENEMY_BOE:
        case RI_SOUL_ENEMY_BUBBLE:
        case RI_SOUL_ENEMY_CAPTAIN_KEETA:
        case RI_SOUL_ENEMY_CHUCHU:
        case RI_SOUL_ENEMY_DEATH_ARMOS:
        case RI_SOUL_ENEMY_DEEP_PYTHON:
        case RI_SOUL_ENEMY_DEKU_BABA:
        case RI_SOUL_ENEMY_DEXIHAND:
        case RI_SOUL_ENEMY_DINOLFOS:
        case RI_SOUL_ENEMY_DODONGO:
        case RI_SOUL_ENEMY_DRAGONFLY:
        case RI_SOUL_ENEMY_EENO:
        case RI_SOUL_ENEMY_EYEGORE:
        case RI_SOUL_ENEMY_FREEZARD:
        case RI_SOUL_ENEMY_GARO:
        case RI_SOUL_ENEMY_GEKKO:
        case RI_SOUL_ENEMY_GIANT_BEE:
        case RI_SOUL_ENEMY_GOMESS:
        case RI_SOUL_ENEMY_GUAY:
        case RI_SOUL_ENEMY_HIPLOOP:
        case RI_SOUL_ENEMY_IGOS_DU_IKANA:
        case RI_SOUL_ENEMY_IRON_KNUCKLE:
        case RI_SOUL_ENEMY_KEESE:
        case RI_SOUL_ENEMY_LEEVER:
        case RI_SOUL_ENEMY_LIKE_LIKE:
        case RI_SOUL_ENEMY_MAD_SCRUB:
        case RI_SOUL_ENEMY_NEJIRON:
        case RI_SOUL_ENEMY_OCTOROK:
        case RI_SOUL_ENEMY_PEAHAT:
        case RI_SOUL_ENEMY_PIRATE:
        case RI_SOUL_ENEMY_POE:
        case RI_SOUL_ENEMY_REDEAD:
        case RI_SOUL_ENEMY_SHELLBLADE:
        case RI_SOUL_ENEMY_SKULLFISH:
        case RI_SOUL_ENEMY_SKULLTULA:
        case RI_SOUL_ENEMY_SNAPPER:
        case RI_SOUL_ENEMY_STALCHILD:
        case RI_SOUL_ENEMY_TAKKURI:
        case RI_SOUL_ENEMY_TEKTITE:
        case RI_SOUL_ENEMY_WALLMASTER:
        case RI_SOUL_ENEMY_WART:
        case RI_SOUL_ENEMY_WIZROBE:
        case RI_SOUL_BOSS_IGOS:
        case RI_SOUL_ENEMY_BIO_BABA:
        case RI_SOUL_ENEMY_FLOORMASTER:
        case RI_SOUL_ENEMY_FLYING_POT:
        case RI_SOUL_ENEMY_REAL_BOMBCHU:
        case RI_SOUL_ENEMY_SKULLWALLTULA:
        case RI_SOUL_ENEMY_WOLFOS:
            DrawSoul(randoItemId);
            break;
        case RI_SOUL_BOSS_GOHT:
            DrawGoht();
            break;
        case RI_SOUL_BOSS_GYORG:
            DrawGyorg();
            break;
        case RI_SOUL_BOSS_MAJORA:
            DrawMajora();
            break;
        case RI_SOUL_BOSS_ODOLWA:
            DrawOdolwa();
            break;
        case RI_SOUL_BOSS_TWINMOLD:
            DrawTwinmold();
            break;
        case RI_FROG_BLUE:
        case RI_FROG_CYAN:
        case RI_FROG_PINK:
        case RI_FROG_WHITE:
            DrawMinifrog(randoItemId, actor);
            break;
        case RI_ABILITY_SWIM:
            DrawAbilityItem(randoItemId, actor);
            break;
        case RI_TRIFORCE_POWER:
        case RI_TRIFORCE_COURAGE:
        case RI_TRIFORCE_WISDOM:
        case RI_TRIFORCE_PIECE_PREVIOUS:
        case RI_TRIFORCE_PIECE:
            DrawTriforcePiece(randoItemId);
            break;
        case RI_SKELETON_KEY:
            DrawSkeletonKey();
            break;
        case RI_TRAP:
            Rando::DrawItem(Rando::CurrentTrapItem(randoCheckId), randoCheckId, actor);
            break;
        case RI_MAX_TRAP:
            DrawTrapModel();
            break;
        case RI_OCARINA_BUTTON_A:
        case RI_OCARINA_BUTTON_C_DOWN:
        case RI_OCARINA_BUTTON_C_LEFT:
        case RI_OCARINA_BUTTON_C_RIGHT:
        case RI_OCARINA_BUTTON_C_UP:
            DrawOcarinaButtonItem(randoItemId, actor);
            break;
        case RI_NONE:
        case RI_UNKNOWN:
            break;
#ifdef COMBO_BUILD
        case RI_COMBO_FOREIGN: {
            // ComboShip: this MM check holds an OOT item — render the real OOT model (sentinel blue
            // rupee on any failure). The originating check identity is passed straight through.
            // Foreign drawing changes the model matrix, so sample visibility before it runs.
            const bool visible =
                Rando::CheckTracker::NeedsSighting(randoCheckId, randoItemId) && IsCheckModelVisible(actor);
            if (MM_DrawComboForeign(randoCheckId) && visible)
                Rando::CheckTracker::RecordSeenItem(randoCheckId, randoItemId);
            break;
        }
#endif
        default:
            GetItem_Draw(gPlayState, Rando::StaticData::Items[randoItemId].drawId);
            break;
    }

    switch (randoItemId) {
        case RI_NONE:
        case RI_ABILITY_SWIM:
        case RI_PROGRESSIVE_MAGIC:
        case RI_SINGLE_MAGIC:
        case RI_DOUBLE_MAGIC:
        case RI_TIME_PROGRESSIVE:
            DrawSparkles(randoItemId, actor);
            break;
        default:
            break;
    }
}

static RegisterShipInitFunc initializeGICopyDLs(
    []() {
        // Small keys
        Gfx* baseDL = ResourceMgr_LoadGfxByName(gGiSmallKeyDL);
        memcpy(gGiSmallKeyCopyDL, baseDL, sizeof(gGiSmallKeyCopyDL));
        gGiSmallKeyCopyDL[5] = gsDPNoOp();
        gGiSmallKeyCopyDL[6] = gsDPNoOp();

        // Boss keys
        baseDL = ResourceMgr_LoadGfxByName(gGiBossKeyDL);
        memcpy(gGiBossKeyCopyDL, baseDL, sizeof(gGiBossKeyCopyDL));
        gGiBossKeyCopyDL[5] = gsDPNoOp();
        gGiBossKeyCopyDL[6] = gsDPNoOp();

        // Token Flame
        baseDL = ResourceMgr_LoadGfxByName(gSkulltulaTokenFlameDL);
        memcpy(gSkulltulaTokenFlameCopyDL, baseDL, sizeof(gSkulltulaTokenFlameCopyDL));
        gSkulltulaTokenFlameCopyDL[5] = gsDPNoOp();
        gSkulltulaTokenFlameCopyDL[6] = gsDPNoOp();
    },
    {});
