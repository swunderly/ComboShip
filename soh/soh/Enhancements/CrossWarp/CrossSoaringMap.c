#ifdef COMBO_BUILD
#include "CrossWarp.h"
#include "MMWarpAssets.h"
#include "soh/ResourceManagerHelpers.h"
#ifndef COMBO_PAUSE_STATE_DEFS_H
#define COMBO_PAUSE_STATE_DEFS_H

#define PAUSE_STATE_OWLWARP_0       0x101
#define PAUSE_STATE_OWLWARP_1       0x102
#define PAUSE_STATE_OWLWARP_2       0x103
#define PAUSE_STATE_OWLWARP_3       0x104
#define PAUSE_STATE_OWLWARP_SELECT  0x106
#define PAUSE_STATE_OWLWARP_CONFIRM 0x107
#define PAUSE_STATE_OWLWARP_6       0x112

#define PAUSE_ITEM_NONE 999

#define PAUSE_CURSOR_PAGE_LEFT 10
#define PAUSE_CURSOR_PAGE_RIGHT 11

#define PAGE_BG_COLS 3
#define PAGE_BG_ROWS 5
#define PAGE_BG_QUADS (PAGE_BG_COLS * PAGE_BG_ROWS)
#define PAGE_BG_QUAD_WIDTH 80
#define PAGE_BG_QUAD_HEIGHT 32
#define PAGE_BG_QUAD_TEX_WIDTH 80
#define PAGE_BG_QUAD_TEX_HEIGHT 32

#define TMEM_SIZE 0x1000

#define WORLD_MAP_IMAGE_WIDTH 216
#define WORLD_MAP_IMAGE_HEIGHT 128
#define WORLD_MAP_IMAGE_FRAG_HEIGHT ((TMEM_SIZE / 2) / (WORLD_MAP_IMAGE_WIDTH * G_IM_SIZ_8b_BYTES))
#define WORLD_MAP_IMAGE_FRAG_NUM (((WORLD_MAP_IMAGE_HEIGHT - 1) / WORLD_MAP_IMAGE_FRAG_HEIGHT) + 1)
#define WORLD_MAP_NUM_CLOUDS 15

#define VTX_PAGE_MAP_WORLD_QUADS 36  /* VTX_PAGE_MAP_WORLD */

#endif


#define PAUSE_WORLD_MAP 4
#define VTX_PAGE_MAX 1
#define OWL_WARP_MAX 11
#define VTX_PAGE_MAP_WORLD 0
#define QUAD_MAP_PAGE_WORLD_CLOUDS_FIRST 15
#define QUAD_MAP_PAGE_WORLD_WARP_FIRST 41
#define QUAD_MAP_PAGE_WORLD_IMAGE_FIRST 51
#define PAUSE_QUAD_CURSOR_MAX 5
#define PAUSE_MAIN_STATE_IDLE 0
#define PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG 8
static PauseContext sSavedPause;
static PlayState* sMapPlay;
static uint32_t sChoices;
static int sStick, sRepeat;
static float sMapPageRoll;
static float sMapVerticalOffset;
static int sMapReply;
static s16 sSavedUpdateRate;
static u16 sSavedHud;
static s16 R_PAUSE_OWLWARP_ALPHA=120;
extern uint32_t SOH_ForeignSongChoices(void);
extern void SOH_ForeignSongChoose(int);
extern void Combo_SOHMapConfirm(int source);
extern void KaleidoScope_SetView(PauseContext*, f32, f32, f32);
extern Gfx* KaleidoScope_DrawPageSections(Gfx*, Vtx*, void*);
static s16 sVtxPageMapWorldQuadsX[VTX_PAGE_MAP_WORLD_QUADS] = {
    -41,  /* mapPageVtx[60] QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_1 */
    9,    /* mapPageVtx[64] QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_2 */
    43,   /* mapPageVtx[68] QUAD_MAP_WORLD_CLOUDS_WOODFALL_1 */
    -21,  /* mapPageVtx[72] QUAD_MAP_WORLD_CLOUDS_WOODFALL_2 */
    -37,  /* mapPageVtx[76] QUAD_MAP_WORLD_CLOUDS_WOODFALL_3 */
    43,   /* mapPageVtx[80] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_1 */
    -69,  /* mapPageVtx[84] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_2 */
    -69,  /* mapPageVtx[88] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_3 */
    -32,  /* mapPageVtx[92] QUAD_MAP_WORLD_CLOUDS_ROMANI_RANCH */
    -109, /* mapPageVtx[96] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_1 */
    -45,  /* mapPageVtx[100] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_2 */
    -109, /* mapPageVtx[104] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_3 */
    -45,  /* mapPageVtx[108] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_4 */
    59,   /* mapPageVtx[112] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_1 */
    -5,   /* mapPageVtx[116] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_2 */
    -70,  /* mapPageVtx[120] QUAD_MAP_REGION_GREAT_BAY */
    -52,  /* mapPageVtx[124] QUAD_MAP_REGION_ZORA_HALL */
    -11,  /* mapPageVtx[128] QUAD_MAP_REGION_ROMANI_RANCH */
    -2,   /* mapPageVtx[132] QUAD_MAP_REGION_DEKU_PALACE */
    8,    /* mapPageVtx[136] QUAD_MAP_REGION_WOODFALL */
    2,    /* mapPageVtx[140] QUAD_MAP_REGION_CLOCK_TOWN */
    1,    /* mapPageVtx[144] QUAD_MAP_REGION_SNOWHEAD */
    43,   /* mapPageVtx[148] QUAD_MAP_REGION_IKANA_GRAVEYARD */
    52,   /* mapPageVtx[152] QUAD_MAP_REGION_IKANA_CANYON */
    54,   /* mapPageVtx[156] QUAD_MAP_REGION_GORON_VILLAGE */
    62,   /* mapPageVtx[160] QUAD_MAP_REGION_STONE_TOWER */
    -80,  /* mapPageVtx[164] QUAD_MAP_OWL_WARP_GREAT_BAY_COAST */
    -64,  /* mapPageVtx[168] QUAD_MAP_OWL_WARP_ZORA_CAPE */
    -9,   /* mapPageVtx[172] QUAD_MAP_OWL_WARP_SNOWHEAD */
    -3,   /* mapPageVtx[176] QUAD_MAP_OWL_WARP_MOUNTAIN_VILLAGE */
    -7,   /* mapPageVtx[180] QUAD_MAP_OWL_WARP_CLOCK_TOWN */
    -16,  /* mapPageVtx[184] QUAD_MAP_OWL_WARP_MILK_ROAD */
    -1,   /* mapPageVtx[188] QUAD_MAP_OWL_WARP_WOODFALL */
    23,   /* mapPageVtx[192] QUAD_MAP_OWL_WARP_SOUTHERN_SWAMP */
    44,   /* mapPageVtx[196] QUAD_MAP_OWL_WARP_IKANA_CANYON */
    54,   /* mapPageVtx[200] QUAD_MAP_OWL_WARP_STONE_TOWER */
};

static s16 gVtxPageMapWorldQuadsWidth[VTX_PAGE_MAP_WORLD_QUADS] = {
    80,  /* mapPageVtx[60]  QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_1 */
    64,  /* mapPageVtx[64]  QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_2 */
    64,  /* mapPageVtx[68]  QUAD_MAP_WORLD_CLOUDS_WOODFALL_1 */
    64,  /* mapPageVtx[72]  QUAD_MAP_WORLD_CLOUDS_WOODFALL_2 */
    16,  /* mapPageVtx[76]  QUAD_MAP_WORLD_CLOUDS_WOODFALL_3 */
    64,  /* mapPageVtx[80]  QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_1 */
    112, /* mapPageVtx[84]  QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_2 */
    112, /* mapPageVtx[88]  QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_3 */
    48,  /* mapPageVtx[92]  QUAD_MAP_WORLD_CLOUDS_ROMANI_RANCH */
    64,  /* mapPageVtx[96]  QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_1 */
    64,  /* mapPageVtx[100] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_2 */
    64,  /* mapPageVtx[104] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_3 */
    64,  /* mapPageVtx[108] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_4 */
    48,  /* mapPageVtx[112] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_1 */
    64,  /* mapPageVtx[116] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_2 */
    8,   /* mapPageVtx[120] QUAD_MAP_REGION_GREAT_BAY */
    8,   /* mapPageVtx[124] QUAD_MAP_REGION_ZORA_HALL */
    8,   /* mapPageVtx[128] QUAD_MAP_REGION_ROMANI_RANCH */
    8,   /* mapPageVtx[132] QUAD_MAP_REGION_DEKU_PALACE */
    8,   /* mapPageVtx[136] QUAD_MAP_REGION_WOODFALL */
    8,   /* mapPageVtx[140] QUAD_MAP_REGION_CLOCK_TOWN */
    8,   /* mapPageVtx[144] QUAD_MAP_REGION_SNOWHEAD */
    8,   /* mapPageVtx[148] QUAD_MAP_REGION_IKANA_GRAVEYARD */
    8,   /* mapPageVtx[152] QUAD_MAP_REGION_IKANA_CANYON */
    8,   /* mapPageVtx[156] QUAD_MAP_REGION_GORON_VILLAGE */
    8,   /* mapPageVtx[160] QUAD_MAP_REGION_STONE_TOWER */
    24,  /* mapPageVtx[164] QUAD_MAP_OWL_WARP_GREAT_BAY_COAST */
    24,  /* mapPageVtx[168] QUAD_MAP_OWL_WARP_ZORA_CAPE */
    24,  /* mapPageVtx[172] QUAD_MAP_OWL_WARP_SNOWHEAD */
    24,  /* mapPageVtx[176] QUAD_MAP_OWL_WARP_MOUNTAIN_VILLAGE */
    24,  /* mapPageVtx[180] QUAD_MAP_OWL_WARP_CLOCK_TOWN */
    24,  /* mapPageVtx[184] QUAD_MAP_OWL_WARP_MILK_ROAD */
    24,  /* mapPageVtx[188] QUAD_MAP_OWL_WARP_WOODFALL */
    24,  /* mapPageVtx[192] QUAD_MAP_OWL_WARP_SOUTHERN_SWAMP */
    24,  /* mapPageVtx[196] QUAD_MAP_OWL_WARP_IKANA_CANYON */
    24,  /* mapPageVtx[200] QUAD_MAP_OWL_WARP_STONE_TOWER */
};
static s16 sVtxPageMapWorldQuadsY[VTX_PAGE_MAP_WORLD_QUADS] = {
    11,  /* mapPageVtx[60] QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_1 */
    22,  /* mapPageVtx[64] QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_2 */
    -16, /* mapPageVtx[68] QUAD_MAP_WORLD_CLOUDS_WOODFALL_1 */
    -16, /* mapPageVtx[72] QUAD_MAP_WORLD_CLOUDS_WOODFALL_2 */
    -16, /* mapPageVtx[76] QUAD_MAP_WORLD_CLOUDS_WOODFALL_3 */
    59,  /* mapPageVtx[80] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_1 */
    24,  /* mapPageVtx[84] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_2 */
    59,  /* mapPageVtx[88] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_3 */
    -11, /* mapPageVtx[92] QUAD_MAP_WORLD_CLOUDS_ROMANI_RANCH */
    -5,  /* mapPageVtx[96] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_1 */
    -5,  /* mapPageVtx[100] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_2 */
    59,  /* mapPageVtx[104] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_3 */
    59,  /* mapPageVtx[108] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_4 */
    39,  /* mapPageVtx[112] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_1 */
    31,  /* mapPageVtx[116] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_2 */
    -10, /* mapPageVtx[120] QUAD_MAP_REGION_GREAT_BAY */
    -36, /* mapPageVtx[124] QUAD_MAP_REGION_ZORA_HALL */
    -19, /* mapPageVtx[128] QUAD_MAP_REGION_ROMANI_RANCH */
    -53, /* mapPageVtx[132] QUAD_MAP_REGION_DEKU_PALACE */
    -27, /* mapPageVtx[136] QUAD_MAP_REGION_WOODFALL */
    -2,  /* mapPageVtx[140] QUAD_MAP_REGION_CLOCK_TOWN */
    41,  /* mapPageVtx[144] QUAD_MAP_REGION_SNOWHEAD */
    11,  /* mapPageVtx[148] QUAD_MAP_REGION_IKANA_GRAVEYARD */
    -3,  /* mapPageVtx[152] QUAD_MAP_REGION_IKANA_CANYON */
    45,  /* mapPageVtx[156] QUAD_MAP_REGION_GORON_VILLAGE */
    18,  /* mapPageVtx[160] QUAD_MAP_REGION_STONE_TOWER */
    -8,  /* mapPageVtx[164] QUAD_MAP_OWL_WARP_GREAT_BAY_COAST */
    -38, /* mapPageVtx[168] QUAD_MAP_OWL_WARP_ZORA_CAPE */
    39,  /* mapPageVtx[172] QUAD_MAP_OWL_WARP_SNOWHEAD */
    26,  /* mapPageVtx[176] QUAD_MAP_OWL_WARP_MOUNTAIN_VILLAGE */
    1,   /* mapPageVtx[180] QUAD_MAP_OWL_WARP_CLOCK_TOWN */
    -7,  /* mapPageVtx[184] QUAD_MAP_OWL_WARP_MILK_ROAD */
    -28, /* mapPageVtx[188] QUAD_MAP_OWL_WARP_WOODFALL */
    -27, /* mapPageVtx[192] QUAD_MAP_OWL_WARP_SOUTHERN_SWAMP */
    -1,  /* mapPageVtx[196] QUAD_MAP_OWL_WARP_IKANA_CANYON */
    24,  /* mapPageVtx[200] QUAD_MAP_OWL_WARP_STONE_TOWER */
};
static s16 gVtxPageMapWorldQuadsHeight[VTX_PAGE_MAP_WORLD_QUADS] = {
    40, /* mapPageVtx[60] QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_1 */
    31, /* mapPageVtx[64] QUAD_MAP_WORLD_CLOUDS_CLOCK_TOWN_2 */
    53, /* mapPageVtx[68] QUAD_MAP_WORLD_CLOUDS_WOODFALL_1 */
    53, /* mapPageVtx[72] QUAD_MAP_WORLD_CLOUDS_WOODFALL_2 */
    53, /* mapPageVtx[76] QUAD_MAP_WORLD_CLOUDS_WOODFALL_3 */
    52, /* mapPageVtx[80] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_1 */
    35, /* mapPageVtx[84] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_2 */
    35, /* mapPageVtx[88] QUAD_MAP_WORLD_CLOUDS_SNOWHEAD_3 */
    32, /* mapPageVtx[92] QUAD_MAP_WORLD_CLOUDS_ROMANI_RANCH */
    64, /* mapPageVtx[96] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_1 */
    64, /* mapPageVtx[100] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_2 */
    64, /* mapPageVtx[104] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_3 */
    64, /* mapPageVtx[108] QUAD_MAP_WORLD_CLOUDS_GREAT_BAY_4 */
    82, /* mapPageVtx[112] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_1 */
    61, /* mapPageVtx[116] QUAD_MAP_WORLD_CLOUDS_STONE_TOWER_2 */
    8,  /* mapPageVtx[120] QUAD_MAP_REGION_GREAT_BAY */
    8,  /* mapPageVtx[124] QUAD_MAP_REGION_ZORA_HALL */
    8,  /* mapPageVtx[128] QUAD_MAP_REGION_ROMANI_RANCH */
    8,  /* mapPageVtx[132] QUAD_MAP_REGION_DEKU_PALACE */
    8,  /* mapPageVtx[136] QUAD_MAP_REGION_WOODFALL */
    8,  /* mapPageVtx[140] QUAD_MAP_REGION_CLOCK_TOWN */
    8,  /* mapPageVtx[144] QUAD_MAP_REGION_SNOWHEAD */
    8,  /* mapPageVtx[148] QUAD_MAP_REGION_IKANA_GRAVEYARD */
    8,  /* mapPageVtx[152] QUAD_MAP_REGION_IKANA_CANYON */
    8,  /* mapPageVtx[156] QUAD_MAP_REGION_GORON_VILLAGE */
    8,  /* mapPageVtx[160] QUAD_MAP_REGION_STONE_TOWER */
    12, /* mapPageVtx[164] QUAD_MAP_OWL_WARP_GREAT_BAY_COAST */
    12, /* mapPageVtx[168] QUAD_MAP_OWL_WARP_ZORA_CAPE */
    12, /* mapPageVtx[172] QUAD_MAP_OWL_WARP_SNOWHEAD */
    12, /* mapPageVtx[176] QUAD_MAP_OWL_WARP_MOUNTAIN_VILLAGE */
    12, /* mapPageVtx[180] QUAD_MAP_OWL_WARP_CLOCK_TOWN */
    12, /* mapPageVtx[184] QUAD_MAP_OWL_WARP_MILK_ROAD */
    12, /* mapPageVtx[188] QUAD_MAP_OWL_WARP_WOODFALL */
    12, /* mapPageVtx[192] QUAD_MAP_OWL_WARP_SOUTHERN_SWAMP */
    12, /* mapPageVtx[196] QUAD_MAP_OWL_WARP_IKANA_CANYON */
    12, /* mapPageVtx[200] QUAD_MAP_OWL_WARP_STONE_TOWER */
};

static s16* sVtxPageQuadsX[VTX_PAGE_MAX] = {
    sVtxPageMapWorldQuadsX,   /* VTX_PAGE_MAP_WORLD */
};

static s16* sVtxPageQuadsWidth[VTX_PAGE_MAX] = {
    gVtxPageMapWorldQuadsWidth,   /* VTX_PAGE_MAP_WORLD */
};

static s16* sVtxPageQuadsY[VTX_PAGE_MAX] = {
    sVtxPageMapWorldQuadsY,   /* VTX_PAGE_MAP_WORLD */
};

static s16* sVtxPageQuadsHeight[VTX_PAGE_MAX] = {
    gVtxPageMapWorldQuadsHeight,   /* VTX_PAGE_MAP_WORLD */
};

static const char* sCloudTextures[] = {
    gWorldMapClockTownCloud1Tex,  // TINGLE_MAP_CLOCK_TOWN
    gWorldMapClockTownCloud2Tex,  // TINGLE_MAP_CLOCK_TOWN
    gWorldMapWoodfallCloud1Tex,   // TINGLE_MAP_WOODFALL
    gWorldMapWoodfallCloud2Tex,   // TINGLE_MAP_WOODFALL
    gWorldMapWoodfallCloud3Tex,   // TINGLE_MAP_WOODFALL
    gWorldMapSnowheadCloud1Tex,   // TINGLE_MAP_SNOWHEAD
    gWorldMapSnowheadCloud2Tex,   // TINGLE_MAP_SNOWHEAD
    gWorldMapSnowheadCloud3Tex,   // TINGLE_MAP_SNOWHEAD
    gWorldMapRomaniRanchCloudTex, // TINGLE_MAP_ROMANI_RANCH
    gWorldMapGreatBayCloud1Tex,   // TINGLE_MAP_GREAT_BAY
    gWorldMapGreatBayCloud2Tex,   // TINGLE_MAP_GREAT_BAY
    gWorldMapGreatBayCloud3Tex,   // TINGLE_MAP_GREAT_BAY
    gWorldMapGreatBayCloud4Tex,   // TINGLE_MAP_GREAT_BAY
    gWorldMapStoneTowerCloud1Tex, // TINGLE_MAP_STONE_TOWER
    gWorldMapStoneTowerCloud2Tex, // TINGLE_MAP_STONE_TOWER
};
static const char* sMapPageBgTextures[] = {
    // Column 0
    gPauseMap00Tex,
    gPauseMap01Tex,
    gPauseMap02Tex,
    gPauseMap03Tex,
    gPauseMap04Tex,
    // Column 1
    gPauseMap10ENGTex,
    gPauseMap11Tex,
    gPauseMap12Tex,
    gPauseMap13Tex,
    gPauseMap14Tex,
    // Column 2
    gPauseMap20Tex,
    gPauseMap21Tex,
    gPauseMap22Tex,
    gPauseMap23Tex,
    gPauseMap24Tex,
};
static f32 sOwlWarpWorldMapCursorsX[OWL_WARP_MAX - 1] = {
    -50.0f, /* OWL_WARP_GREAT_BAY_COAST */
    -38.0f, /* OWL_WARP_ZORA_CAPE */
    6.0f,   /* OWL_WARP_SNOWHEAD */
    11.0f,  /* OWL_WARP_MOUNTAIN_VILLAGE */
    8.0f,   /* OWL_WARP_CLOCK_TOWN */
    0.0f,   /* OWL_WARP_MILK_ROAD */
    12.0f,  /* OWL_WARP_WOODFALL */
    31.0f,  /* OWL_WARP_SOUTHERN_SWAMP */
    48.0f,  /* OWL_WARP_IKANA_CANYON */
    56.0f,  /* OWL_WARP_STONE_TOWER */
};
static f32 sOwlWarpWorldMapCursorsY[OWL_WARP_MAX - 1] = {
    -14.0f, /* OWL_WARP_GREAT_BAY_COAST */
    -39.0f, /* OWL_WARP_ZORA_CAPE */
    23.0f,  /* OWL_WARP_SNOWHEAD */
    11.0f,  /* OWL_WARP_MOUNTAIN_VILLAGE */
    -8.0f,  /* OWL_WARP_CLOCK_TOWN */
    -15.0f, /* OWL_WARP_MILK_ROAD */
    -31.0f, /* OWL_WARP_WOODFALL */
    -30.0f, /* OWL_WARP_SOUTHERN_SWAMP */
    -10.0f, /* OWL_WARP_IKANA_CANYON */
    11.0f,  /* OWL_WARP_STONE_TOWER */
};

static s16 sCursorPrimColorTarget[][3] = {
    { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 0 }, { 255, 255, 0 }, { 100, 150, 255 }, { 100, 255, 255 },
};
static s16 sCursorEnvColorTarget[][3] = {
    { 0, 0, 0 }, { 170, 170, 170 }, { 0, 0, 0 }, { 255, 160, 0 }, { 0, 0, 100 }, { 0, 150, 255 },
};

static f32 sCursorX;
static f32 sCursorY;
static f32 sCursorWidth;
static f32 sCursorHeight;
static f32 sCursorShrinkRate;
static s16 sCursorSpinPhase = 0xE000;
static s16 sCursorPrimR = 0;
static s16 sCursorPrimG = 0;
static s16 sCursorPrimB = 0;
static s16 sCursorEnvR = 0;
static s16 sCursorEnvG = 0;
static s16 sCursorEnvB = 0;
static f32 sCursorCirclesX[4];
static f32 sCursorCirclesY[4];

static void KaleidoScope_DrawWorldMap(PlayState* play) {
    s16 n;
    s16 j;
    s16 k;
    s16 i;
    PauseContext* pauseCtx = &play->pauseCtx;

    void* mapImage = Combo_SOHWarpResource(gWorldMapImageTex);
    
    

    OPEN_DISPS(play->state.gfxCtx);

    /* KaleidoScope_SetCursorVtxPos(pauseCtx, pauseCtx->cursorSlot[PAUSE_MAP] * 4, pauseCtx->mapPageVtx); */

    /* Draw the world map angled */
    /* Because it is at an angle, vertices are used to place it. */
    /* The structure of the loops here is to satisfy the constraints of both TMEM and the size of the vertex cache. */
    /* - Each loop iteration loads 9 rows, because 9 is the largest number smaller than */
    /*   `TMEM_SIZE / 2 / textureWidth`. */
    /* - Each loop is at most 8 iterations long because each row uses 4 vertices and the vertex cache has size */
    /*   `32 = 8 * 4`. */
    /* . */
    /* Hence there is one loop of length 8, one of length 6, and then the remaining `128 - (8 + 6) * 9 = 2` rows are */
    /* drawn at the end. */

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetTextureFilter(POLY_OPA_DISP++, G_TF_POINT);
    gDPLoadTLUT_pal256(POLY_OPA_DISP++, gWorldMapImageTLUT);
    gDPSetTextureLUT(POLY_OPA_DISP++, G_TT_RGBA16);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);

    /* Set the vertices for the first 8 quads attached to the world map texture. */
    gSPVertex(POLY_OPA_DISP++, &pauseCtx->mapPageVtx[QUAD_MAP_PAGE_WORLD_IMAGE_FIRST * 4], 8 * 4, 0);

    /* Process the first 72 rows of pixels for gWorldMapImageTex, 9 rows at a time over 8 iterations */
    /* Loop over quadIndex of this loop (i), quadIndex of the entire texture (k), vtxIndex (j) */
    for (i = 0, k = 0, j = 0; i < 8; i++, k++, j += 4) {
        gDPLoadTextureBlock(
            POLY_OPA_DISP++, (u8*)mapImage + k * (WORLD_MAP_IMAGE_WIDTH * WORLD_MAP_IMAGE_FRAG_HEIGHT),
            G_IM_FMT_CI, G_IM_SIZ_8b, WORLD_MAP_IMAGE_WIDTH, WORLD_MAP_IMAGE_FRAG_HEIGHT, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gSP1Quadrangle(POLY_OPA_DISP++, j, j + 2, j + 3, j + 1, 0);
    }

    /* Set the vertices for the last 7 quads attached to the world map texture: */
    /* 6 quads with a height of 9, 1 quad with a height of 2 */
    gSPVertex(POLY_OPA_DISP++, &pauseCtx->mapPageVtx[(QUAD_MAP_PAGE_WORLD_IMAGE_FIRST + 8) * 4], (6 + 1) * 4, 0);

    /* Process the next 54 rows of pixels for gWorldMapImageTex, 9 rows at a time over 6 iterations */
    /* Loop over quadIndex of this loop (i), quadIndex of the entire texture (k), vtxIndex (j) */
    for (i = 0, j = 0; i < 6; i++, k++, j += 4) {
        gDPLoadTextureBlock(
            POLY_OPA_DISP++, (u8*)mapImage + k * (WORLD_MAP_IMAGE_WIDTH * WORLD_MAP_IMAGE_FRAG_HEIGHT),
            G_IM_FMT_CI, G_IM_SIZ_8b, WORLD_MAP_IMAGE_WIDTH, WORLD_MAP_IMAGE_FRAG_HEIGHT, 0,
            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gSP1Quadrangle(POLY_OPA_DISP++, j, j + 2, j + 3, j + 1, 0);
    }

    /* Process the last 2 rows of pixels for gWorldMapImageTex */
    gDPLoadTextureBlock(
        POLY_OPA_DISP++, (u8*)mapImage + k * (WORLD_MAP_IMAGE_WIDTH * WORLD_MAP_IMAGE_FRAG_HEIGHT),
        G_IM_FMT_CI, G_IM_SIZ_8b, WORLD_MAP_IMAGE_WIDTH, WORLD_MAP_IMAGE_HEIGHT % WORLD_MAP_IMAGE_FRAG_HEIGHT, 0,
        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

    gSP1Quadrangle(POLY_OPA_DISP++, j, j + 2, j + 3, j + 1, 0);

    Gfx_SetupDL_42Opa(play->state.gfxCtx);

    gDPPipeSync(POLY_OPA_DISP++);

    gDPSetTextureFilter(POLY_OPA_DISP++, G_TF_BILERP);

    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 215, 235, 235, pauseCtx->alpha);
    gDPSetEnvColor(POLY_OPA_DISP++, 40, 60, 100, 0);

    /* Draw clouds over the world map */
    /* Iterate over cloud bits (n) */
    for (n = 0; n < WORLD_MAP_NUM_CLOUDS; n++) {
        if (!(((void)0, Combo_SOHMapClouds()) & (1 << n))) {

            gSPVertex(POLY_OPA_DISP++, &pauseCtx->mapPageVtx[(QUAD_MAP_PAGE_WORLD_CLOUDS_FIRST + n) * 4], 4, 0);

            gDPLoadTextureBlock(POLY_OPA_DISP++, sCloudTextures[n], G_IM_FMT_IA, G_IM_SIZ_8b, gVtxPageMapWorldQuadsWidth[n], gVtxPageMapWorldQuadsHeight[n], 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                                G_TX_NOLOD);

            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
        }
    }

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetRenderMode(POLY_OPA_DISP++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 0, 0, 0, R_PAUSE_OWLWARP_ALPHA);
    gDPFillRectangle(POLY_OPA_DISP++, 50, 62, 270, 190);

    Gfx_SetupDL_42Opa(play->state.gfxCtx);

    /* Selecting an owl warp */
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPLoadTextureBlock(POLY_OPA_DISP++, gWorldMapOwlFaceTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 24, 12, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);

    /* Loop over OwlWarpId (i), unused vtxIndex (j), unused (k) */
    for (i = 0, j = 0; i <= 9; i++, k++, j += 4) { /* OWL_WARP_STONE_TOWER */
        if (pauseCtx->worldMapPoints[i]) {
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->mapPageVtx[(QUAD_MAP_PAGE_WORLD_WARP_FIRST + i) * 4], 4, 0);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
        }
    }

    gDPPipeSync(POLY_OPA_DISP++);

    CLOSE_DISPS(play->state.gfxCtx);
}

static void KaleidoScope_DrawOwlWarpInfoPanel(PlayState* play)
{
    static s16 sPauseLRCursorColorTargets[][4] = {
        { 180, 210, 255, 220 },
        { 100, 100, 150, 220 },
    };
    static s16 sPauseLRCursorColorTimer = 20;
    static s16 sPauseLRCursorColorIndex = 0;
    static s16 sPauseLRCursorRed;
    static s16 sPauseLRCursorGreen;
    static s16 sPauseLRCursorBlue;
    static s16 sPauseLRCursorAlpha;
    PauseContext* pauseCtx = &play->pauseCtx;
    s16 stepR;
    s16 stepG;
    s16 stepB;
    s16 stepA;
    s16 y;
    s16 i;
    s16 j;

    OPEN_DISPS(play->state.gfxCtx);

    stepR =
        ABS(sPauseLRCursorRed - sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][0]) / sPauseLRCursorColorTimer;
    stepG = ABS(sPauseLRCursorGreen - sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][1]) /
            sPauseLRCursorColorTimer;
    stepB = ABS(sPauseLRCursorBlue - sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][2]) /
            sPauseLRCursorColorTimer;
    stepA = ABS(sPauseLRCursorAlpha - sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][3]) /
            sPauseLRCursorColorTimer;

    if (sPauseLRCursorRed >= sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][0]) {
        sPauseLRCursorRed -= stepR;
    } else {
        sPauseLRCursorRed += stepR;
    }

    if (sPauseLRCursorGreen >= sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][1]) {
        sPauseLRCursorGreen -= stepG;
    } else {
        sPauseLRCursorGreen += stepG;
    }

    if (sPauseLRCursorBlue >= sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][2]) {
        sPauseLRCursorBlue -= stepB;
    } else {
        sPauseLRCursorBlue += stepB;
    }

    if (sPauseLRCursorAlpha >= sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][3]) {
        sPauseLRCursorAlpha -= stepA;
    } else {
        sPauseLRCursorAlpha += stepA;
    }

    sPauseLRCursorColorTimer--;
    if (sPauseLRCursorColorTimer == 0) {
        sPauseLRCursorRed = sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][0];
        sPauseLRCursorGreen = sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][1];
        sPauseLRCursorBlue = sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][2];
        sPauseLRCursorAlpha = sPauseLRCursorColorTargets[sPauseLRCursorColorIndex][3];
        sPauseLRCursorColorTimer = 20;
        sPauseLRCursorColorIndex ^= 1;
    }

    y = pauseCtx->infoPanelOffsetY - 76;
    for (j = 0, i = 0; i < 7; i++, j += 4) {
        pauseCtx->infoPanelVtx[j + 0].v.ob[0] = pauseCtx->infoPanelVtx[j + 2].v.ob[0] = -72;

        pauseCtx->infoPanelVtx[j + 1].v.ob[0] = pauseCtx->infoPanelVtx[j + 3].v.ob[0] = 0;

        pauseCtx->infoPanelVtx[j + 0].v.ob[1] = pauseCtx->infoPanelVtx[j + 1].v.ob[1] = y;

        pauseCtx->infoPanelVtx[j + 2].v.ob[1] = pauseCtx->infoPanelVtx[j + 3].v.ob[1] = y - 24;

        pauseCtx->infoPanelVtx[j + 0].v.ob[2] = pauseCtx->infoPanelVtx[j + 1].v.ob[2] =
            pauseCtx->infoPanelVtx[j + 2].v.ob[2] = pauseCtx->infoPanelVtx[j + 3].v.ob[2] = 0;

        pauseCtx->infoPanelVtx[j + 0].v.flag = pauseCtx->infoPanelVtx[j + 1].v.flag =
            pauseCtx->infoPanelVtx[j + 2].v.flag = pauseCtx->infoPanelVtx[j + 3].v.flag = 0;

        pauseCtx->infoPanelVtx[j + 0].v.tc[0] = pauseCtx->infoPanelVtx[j + 0].v.tc[1] =
            pauseCtx->infoPanelVtx[j + 1].v.tc[1] = pauseCtx->infoPanelVtx[j + 2].v.tc[0] = 0;

        pauseCtx->infoPanelVtx[j + 1].v.tc[0] = pauseCtx->infoPanelVtx[j + 3].v.tc[0] = 72 * (1 << 5);

        pauseCtx->infoPanelVtx[j + 2].v.tc[1] = pauseCtx->infoPanelVtx[j + 3].v.tc[1] = 24 * (1 << 5);

        pauseCtx->infoPanelVtx[j + 0].v.cn[0] = pauseCtx->infoPanelVtx[j + 2].v.cn[0] =
            pauseCtx->infoPanelVtx[j + 0].v.cn[1] = pauseCtx->infoPanelVtx[j + 2].v.cn[1] =
                pauseCtx->infoPanelVtx[j + 0].v.cn[2] = pauseCtx->infoPanelVtx[j + 2].v.cn[2] =
                    pauseCtx->infoPanelVtx[j + 1].v.cn[0] = pauseCtx->infoPanelVtx[j + 3].v.cn[0] =
                        pauseCtx->infoPanelVtx[j + 1].v.cn[1] = pauseCtx->infoPanelVtx[j + 3].v.cn[1] =
                            pauseCtx->infoPanelVtx[j + 1].v.cn[2] = pauseCtx->infoPanelVtx[j + 3].v.cn[2] = 200;

        pauseCtx->infoPanelVtx[j + 0].v.cn[3] = pauseCtx->infoPanelVtx[j + 2].v.cn[3] =
            pauseCtx->infoPanelVtx[j + 1].v.cn[3] = pauseCtx->infoPanelVtx[j + 3].v.cn[3] = pauseCtx->alpha;
    }

    pauseCtx->infoPanelVtx[4].v.ob[0] = pauseCtx->infoPanelVtx[6].v.ob[0] = pauseCtx->infoPanelVtx[0].v.ob[0] + 72;

    pauseCtx->infoPanelVtx[5].v.ob[0] = pauseCtx->infoPanelVtx[7].v.ob[0] = pauseCtx->infoPanelVtx[4].v.ob[0] + 72;

    if ((pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_LEFT) && (pauseCtx->unk_1E4 == PAUSE_MAIN_STATE_IDLE)) {
        pauseCtx->infoPanelVtx[8].v.ob[0] = pauseCtx->infoPanelVtx[10].v.ob[0] = WREG(16);

        pauseCtx->infoPanelVtx[9].v.ob[0] = pauseCtx->infoPanelVtx[11].v.ob[0] = pauseCtx->infoPanelVtx[8].v.ob[0] + 24;

        pauseCtx->infoPanelVtx[8].v.ob[1] = pauseCtx->infoPanelVtx[9].v.ob[1] = 10;

        pauseCtx->infoPanelVtx[10].v.ob[1] = pauseCtx->infoPanelVtx[11].v.ob[1] =
            pauseCtx->infoPanelVtx[8].v.ob[1] - 32;
    } else {
        pauseCtx->infoPanelVtx[8].v.ob[0] = pauseCtx->infoPanelVtx[10].v.ob[0] = WREG(16) + 3;

        pauseCtx->infoPanelVtx[9].v.ob[0] = pauseCtx->infoPanelVtx[11].v.ob[0] = pauseCtx->infoPanelVtx[8].v.ob[0] + 18;

        pauseCtx->infoPanelVtx[8].v.ob[1] = pauseCtx->infoPanelVtx[9].v.ob[1] = 10 - 3;

        pauseCtx->infoPanelVtx[10].v.ob[1] = pauseCtx->infoPanelVtx[11].v.ob[1] =
            pauseCtx->infoPanelVtx[8].v.ob[1] - 26;
    }

    if ((pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_RIGHT) && (pauseCtx->unk_1E4 == PAUSE_MAIN_STATE_IDLE)) {
        pauseCtx->infoPanelVtx[12].v.ob[0] = pauseCtx->infoPanelVtx[14].v.ob[0] = WREG(17);

        pauseCtx->infoPanelVtx[13].v.ob[0] = pauseCtx->infoPanelVtx[15].v.ob[0] =
            pauseCtx->infoPanelVtx[12].v.ob[0] + 24;

        pauseCtx->infoPanelVtx[12].v.ob[1] = pauseCtx->infoPanelVtx[13].v.ob[1] = 10;

        pauseCtx->infoPanelVtx[14].v.ob[1] = pauseCtx->infoPanelVtx[15].v.ob[1] =
            pauseCtx->infoPanelVtx[12].v.ob[1] - 32;
    } else {
        pauseCtx->infoPanelVtx[12].v.ob[0] = pauseCtx->infoPanelVtx[14].v.ob[0] = WREG(17) + 3;

        pauseCtx->infoPanelVtx[13].v.ob[0] = pauseCtx->infoPanelVtx[15].v.ob[0] =
            pauseCtx->infoPanelVtx[12].v.ob[0] + 18;

        pauseCtx->infoPanelVtx[12].v.ob[1] = pauseCtx->infoPanelVtx[13].v.ob[1] = 10 - 3;

        pauseCtx->infoPanelVtx[14].v.ob[1] = pauseCtx->infoPanelVtx[15].v.ob[1] =
            pauseCtx->infoPanelVtx[12].v.ob[1] - 26;
    }

    pauseCtx->infoPanelVtx[9].v.tc[0] = pauseCtx->infoPanelVtx[11].v.tc[0] = pauseCtx->infoPanelVtx[13].v.tc[0] =
        pauseCtx->infoPanelVtx[15].v.tc[0] = 24 * (1 << 5);

    pauseCtx->infoPanelVtx[10].v.tc[1] = pauseCtx->infoPanelVtx[11].v.tc[1] = pauseCtx->infoPanelVtx[14].v.tc[1] =
        pauseCtx->infoPanelVtx[15].v.tc[1] = 32 * (1 << 5);

    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

    Matrix_Translate(0.0f, 0.0f, -144.0f, MTXMODE_NEW);
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);

    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 150, 140, 90, 255);
    gSPVertex(POLY_OPA_DISP++, &pauseCtx->infoPanelVtx[0], 16, 0);

    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)Combo_SOHWarpResource(gItemNamePanelDL));

    if (pauseCtx->cursorSpecialPos != 0) {
        j = (pauseCtx->cursorSpecialPos * 4) - 32;
        pauseCtx->cursorVtx[0].v.ob[0] = pauseCtx->infoPanelVtx[j].v.ob[0];
        pauseCtx->cursorVtx[0].v.ob[1] = pauseCtx->infoPanelVtx[j].v.ob[1];
    }

    y = pauseCtx->infoPanelOffsetY - 80;
    pauseCtx->infoPanelVtx[16].v.ob[1] = pauseCtx->infoPanelVtx[17].v.ob[1] = y;

    pauseCtx->infoPanelVtx[18].v.ob[1] = pauseCtx->infoPanelVtx[19].v.ob[1] = pauseCtx->infoPanelVtx[16].v.ob[1] - 16;

    pauseCtx->infoPanelVtx[18].v.tc[1] = pauseCtx->infoPanelVtx[19].v.tc[1] = 16 * (1 << 5);

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetEnvColor(POLY_OPA_DISP++, 20, 30, 40, 0);

    if ((pauseCtx->state == PAUSE_STATE_OWLWARP_SELECT) && (pauseCtx->namedItem != PAUSE_ITEM_NONE) &&
        (pauseCtx->nameDisplayTimer < 40)) {
        pauseCtx->infoPanelVtx[16].v.ob[0] = pauseCtx->infoPanelVtx[18].v.ob[0] = -63;

        pauseCtx->infoPanelVtx[17].v.ob[0] = pauseCtx->infoPanelVtx[19].v.ob[0] =
            pauseCtx->infoPanelVtx[16].v.ob[0] + 128;

        pauseCtx->infoPanelVtx[17].v.tc[0] = pauseCtx->infoPanelVtx[19].v.tc[0] = 128 * (1 << 5);

        gSPVertex(POLY_OPA_DISP++, &pauseCtx->infoPanelVtx[16], 4, 0);

        if (pauseCtx->nameColorSet == 1) { /* PAUSE_NAME_COLOR_SET_GREY */
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 70, 70, 70, 255);
        } else {
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
        }

        gDPLoadTextureBlock_4b(POLY_OPA_DISP++, pauseCtx->nameSegment, G_IM_FMT_IA, 128, 16, 0, G_TX_NOMIRROR | G_TX_WRAP,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);

        gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

static void KaleidoScope_DrawCursor(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    s16 i;
    s16 stepR;
    s16 stepG;
    s16 stepB;
    static s16 sCursorColorTimer = 10;
    static s16 sCursorColorTargetIndex = 0;


    stepR =
        ABS(sCursorPrimR - sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][0]) /
        sCursorColorTimer;
    stepG =
        ABS(sCursorPrimG - sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][1]) /
        sCursorColorTimer;
    stepB =
        ABS(sCursorPrimB - sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][2]) /
        sCursorColorTimer;

    if (sCursorPrimR >= sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][0]) {
        sCursorPrimR -= stepR;
    } else {
        sCursorPrimR += stepR;
    }

    if (sCursorPrimG >= sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][1]) {
        sCursorPrimG -= stepG;
    } else {
        sCursorPrimG += stepG;
    }

    if (sCursorPrimB >= sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][2]) {
        sCursorPrimB -= stepB;
    } else {
        sCursorPrimB += stepB;
    }

    stepR =
        ABS(sCursorEnvR - sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][0]) /
        sCursorColorTimer;
    stepG =
        ABS(sCursorEnvG - sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][1]) /
        sCursorColorTimer;
    stepB =
        ABS(sCursorEnvB - sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][2]) /
        sCursorColorTimer;

    if (sCursorEnvR >= sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][0]) {
        sCursorEnvR -= stepR;
    } else {
        sCursorEnvR += stepR;
    }

    if (sCursorEnvG >= sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][1]) {
        sCursorEnvG -= stepG;
    } else {
        sCursorEnvG += stepG;
    }

    if (sCursorEnvB >= sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][2]) {
        sCursorEnvB -= stepB;
    } else {
        sCursorEnvB += stepB;
    }

    sCursorColorTimer--;
    if (sCursorColorTimer == 0) {
        sCursorPrimR = sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][0];
        sCursorPrimG = sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][1];
        sCursorPrimB = sCursorPrimColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][2];
        sCursorEnvR = sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][0];
        sCursorEnvG = sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][1];
        sCursorEnvB = sCursorEnvColorTarget[pauseCtx->cursorColorSet + sCursorColorTargetIndex][2];
        sCursorColorTargetIndex ^= 1;
        sCursorColorTimer = 10;
    }

    OPEN_DISPS(play->state.gfxCtx);

    if ((pauseCtx->unk_1E4 == PAUSE_MAIN_STATE_IDLE) ||
        (pauseCtx->unk_1E4 == PAUSE_MAIN_STATE_IDLE_CURSOR_ON_SONG)) {
        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0,
                          PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, sCursorPrimR, sCursorPrimG, sCursorPrimB, 255);
        gDPSetEnvColor(POLY_OPA_DISP++, sCursorEnvR, sCursorEnvG, sCursorEnvB, 255);

        Matrix_Translate(sCursorX, sCursorY, -50.0f, MTXMODE_NEW);
        Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);

        for (i = 0; i < 4; i++) {
            Matrix_Push();
            Matrix_Translate(sCursorCirclesX[i], sCursorCirclesY[i], -50.0f, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPPipeSync(POLY_OPA_DISP++);
            gDPLoadTextureBlock(POLY_OPA_DISP++, gPauseMenuCursorTex, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 16, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->cursorVtx[0], 4, 0);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
            Matrix_Pop();
        }

        gDPPipeSync(POLY_OPA_DISP++);
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

static void KaleidoScope_UpdateCursorSize(PlayState* play)
{
    PauseContext* pauseCtx = &play->pauseCtx;

    pauseCtx->cursorVtx[0].v.ob[0] = pauseCtx->cursorVtx[2].v.ob[0] = -8;
    pauseCtx->cursorVtx[1].v.ob[0] = pauseCtx->cursorVtx[3].v.ob[0] = pauseCtx->cursorVtx[0].v.ob[0] + 16;

    pauseCtx->cursorVtx[0].v.ob[1] = pauseCtx->cursorVtx[1].v.ob[1] = 8;
    pauseCtx->cursorVtx[2].v.ob[1] = pauseCtx->cursorVtx[3].v.ob[1] = pauseCtx->cursorVtx[0].v.ob[1] - 16;

    if (pauseCtx->cursorSpecialPos == 0) {
        sCursorX = sOwlWarpWorldMapCursorsX[pauseCtx->cursorPoint[PAUSE_WORLD_MAP]];
        sCursorY = sOwlWarpWorldMapCursorsY[pauseCtx->cursorPoint[PAUSE_WORLD_MAP]];
        sCursorHeight = 15.0f;
        sCursorWidth = 15.0f;

        sCursorSpinPhase += 0x300;
    } else {
        if (pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_LEFT) {
            sCursorX = -93.0f;
        } else { /* PAUSE_CURSOR_PAGE_RIGHT */
            sCursorX = 101.0f;
        }
        sCursorSpinPhase += 0x300;
        sCursorHeight = 16.0f;
        sCursorWidth = 16.0f;
        sCursorY = -4.0f;
    }

    if (sCursorWidth < sCursorShrinkRate) {
        sCursorWidth = 0;
    } else {
        sCursorWidth -= sCursorShrinkRate;
    }

    if (sCursorHeight < sCursorShrinkRate) {
        sCursorHeight = 0.0f;
    } else {
        sCursorHeight -= sCursorShrinkRate;
    }

    if (sCursorShrinkRate != 0.0f) {
        sCursorShrinkRate -= 1.0f;
    }

    sCursorCirclesX[0] = Math_SinS(sCursorSpinPhase) * sCursorWidth;
    sCursorCirclesX[1] = Math_SinS(sCursorSpinPhase + 0x4000) * sCursorWidth;
    sCursorCirclesX[2] = Math_SinS(sCursorSpinPhase + 0x8000) * sCursorWidth;
    sCursorCirclesX[3] = Math_SinS(sCursorSpinPhase + 0xC000) * sCursorWidth;
    sCursorCirclesY[0] = Math_CosS(sCursorSpinPhase) * sCursorHeight;
    sCursorCirclesY[1] = Math_CosS(sCursorSpinPhase + 0x4000) * sCursorHeight;
    sCursorCirclesY[2] = Math_CosS(sCursorSpinPhase + 0x8000) * sCursorHeight;
    sCursorCirclesY[3] = Math_CosS(sCursorSpinPhase + 0xC000) * sCursorHeight;
}

static s16 KaleidoScope_SetPageVertices(PlayState* play, Vtx* vtx, s16 vtxPage, s16 numQuads) {
    PauseContext* pauseCtx = &play->pauseCtx;
    s16* quadsX;
    s16* quadsWidth;
    s16* quadsY;
    s16* quadsHeight;
    s16 pageBgQuadX;
    s16 pageBgQuadY;
    s16 i;
    s16 j;
    s16 k;

    /* Vertices for KaleidoScope_DrawPageSections */

    pageBgQuadX = 0 - (PAGE_BG_COLS * PAGE_BG_QUAD_WIDTH) / 2 - PAGE_BG_QUAD_WIDTH;

    /* For each column */
    for (k = 0, i = 0; i < 3; i++) {
        pageBgQuadX += PAGE_BG_QUAD_WIDTH;

        /* For each row */
        for (pageBgQuadY = (PAGE_BG_ROWS * PAGE_BG_QUAD_HEIGHT) / 2, j = 0; j < 5; j++, k += 4, pageBgQuadY -= 32) {
            vtx[k + 0].v.ob[0] = vtx[k + 2].v.ob[0] = pageBgQuadX;
            vtx[k + 1].v.ob[0] = vtx[k + 3].v.ob[0] = vtx[k + 0].v.ob[0] + PAGE_BG_QUAD_WIDTH;

            vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = pageBgQuadY + pauseCtx->offsetY;
            vtx[k + 2].v.ob[1] = vtx[k + 3].v.ob[1] = vtx[k + 0].v.ob[1] - PAGE_BG_QUAD_HEIGHT;

            vtx[k + 0].v.ob[2] = vtx[k + 1].v.ob[2] = vtx[k + 2].v.ob[2] = vtx[k + 3].v.ob[2] = 0;

            vtx[k + 0].v.flag = vtx[k + 1].v.flag = vtx[k + 2].v.flag = vtx[k + 3].v.flag = 0;

            vtx[k + 0].v.tc[0] = vtx[k + 0].v.tc[1] = 0;
            vtx[k + 1].v.tc[1] = vtx[k + 2].v.tc[0] = 0;
            vtx[k + 1].v.tc[0] = vtx[k + 3].v.tc[0] = PAGE_BG_QUAD_TEX_WIDTH * (1 << 5);
            vtx[k + 2].v.tc[1] = vtx[k + 3].v.tc[1] = PAGE_BG_QUAD_TEX_HEIGHT * (1 << 5);

            vtx[k + 0].v.cn[0] = vtx[k + 2].v.cn[0] = vtx[k + 0].v.cn[1] = vtx[k + 2].v.cn[1] = vtx[k + 0].v.cn[2] =
                vtx[k + 2].v.cn[2] = 0;

            vtx[k + 1].v.cn[0] = vtx[k + 3].v.cn[0] = vtx[k + 1].v.cn[1] = vtx[k + 3].v.cn[1] = vtx[k + 1].v.cn[2] =
                vtx[k + 3].v.cn[2] = 0;

            vtx[k + 0].v.cn[3] = vtx[k + 2].v.cn[3] = vtx[k + 1].v.cn[3] = vtx[k + 3].v.cn[3] = pauseCtx->alpha;
        }
    }

    if (numQuads != 0) {
        quadsX = sVtxPageMapWorldQuadsX;
        quadsWidth = gVtxPageMapWorldQuadsWidth;
        quadsY = sVtxPageMapWorldQuadsY;
        quadsHeight = gVtxPageMapWorldQuadsHeight;

        for (i = 0; i < numQuads; i++, k += 4) {
            vtx[k + 2].v.ob[0] = vtx[k + 0].v.ob[0] = quadsX[i];

            vtx[k + 1].v.ob[0] = vtx[k + 3].v.ob[0] = vtx[k + 0].v.ob[0] + quadsWidth[i];

            vtx[k + 0].v.ob[1] = vtx[k + 1].v.ob[1] = quadsY[i] + pauseCtx->offsetY;

            vtx[k + 2].v.ob[1] = vtx[k + 3].v.ob[1] = vtx[k + 0].v.ob[1] - quadsHeight[i];

            vtx[k + 0].v.ob[2] = vtx[k + 1].v.ob[2] = vtx[k + 2].v.ob[2] = vtx[k + 3].v.ob[2] = 0;

            vtx[k + 0].v.flag = vtx[k + 1].v.flag = vtx[k + 2].v.flag = vtx[k + 3].v.flag = 0;

            vtx[k + 0].v.tc[0] = vtx[k + 0].v.tc[1] = vtx[k + 1].v.tc[1] = vtx[k + 2].v.tc[0] = 0;
            vtx[k + 1].v.tc[0] = vtx[k + 3].v.tc[0] = quadsWidth[i] << 5;
            vtx[k + 2].v.tc[1] = vtx[k + 3].v.tc[1] = quadsHeight[i] << 5;

            vtx[k + 0].v.cn[0] = vtx[k + 2].v.cn[0] = vtx[k + 0].v.cn[1] = vtx[k + 2].v.cn[1] = vtx[k + 0].v.cn[2] =
                vtx[k + 2].v.cn[2] = 255;

            vtx[k + 1].v.cn[0] = vtx[k + 3].v.cn[0] = vtx[k + 1].v.cn[1] = vtx[k + 3].v.cn[1] = vtx[k + 1].v.cn[2] =
                vtx[k + 3].v.cn[2] = 255;

            vtx[k + 0].v.cn[3] = vtx[k + 2].v.cn[3] = vtx[k + 1].v.cn[3] = vtx[k + 3].v.cn[3] = pauseCtx->alpha;
        }
    }
    return k;
}

static void KaleidoScope_SetVertices(PlayState* play, GraphicsContext* gfxCtx)
{
    PauseContext* pauseCtx = &play->pauseCtx;
    s16 i;
    s16 j;
    s16 vtx_y;

    pauseCtx->offsetY = 0;

    if ((pauseCtx->state == PAUSE_STATE_OWLWARP_3) || (pauseCtx->state >= PAUSE_STATE_OWLWARP_6)) {
        pauseCtx->offsetY = 80;
    }

    pauseCtx->mapPageVtx = Graph_Alloc(
        gfxCtx, ((PAGE_BG_QUADS + VTX_PAGE_MAP_WORLD_QUADS + WORLD_MAP_IMAGE_FRAG_NUM) * 4) * sizeof(Vtx));
    j = KaleidoScope_SetPageVertices(play, pauseCtx->mapPageVtx, VTX_PAGE_MAP_WORLD, VTX_PAGE_MAP_WORLD_QUADS);

    for (i = 0, vtx_y = 59; i < WORLD_MAP_IMAGE_FRAG_NUM; i++, j += 4, vtx_y -= WORLD_MAP_IMAGE_FRAG_HEIGHT) {
        pauseCtx->mapPageVtx[j + 2].v.ob[0] = 0 - (WORLD_MAP_IMAGE_WIDTH / 2) - 1;
        pauseCtx->mapPageVtx[j + 0].v.ob[0] = pauseCtx->mapPageVtx[j + 2].v.ob[0];

        pauseCtx->mapPageVtx[j + 1].v.ob[0] = pauseCtx->mapPageVtx[j + 3].v.ob[0] =
            pauseCtx->mapPageVtx[j + 0].v.ob[0] + WORLD_MAP_IMAGE_WIDTH;

        pauseCtx->mapPageVtx[j + 0].v.ob[1] = pauseCtx->mapPageVtx[j + 1].v.ob[1] = vtx_y + pauseCtx->offsetY;

        pauseCtx->mapPageVtx[j + 2].v.ob[1] = pauseCtx->mapPageVtx[j + 3].v.ob[1] =
            pauseCtx->mapPageVtx[j + 0].v.ob[1] - WORLD_MAP_IMAGE_FRAG_HEIGHT;

        pauseCtx->mapPageVtx[j + 0].v.ob[2] = pauseCtx->mapPageVtx[j + 1].v.ob[2] =
            pauseCtx->mapPageVtx[j + 2].v.ob[2] = pauseCtx->mapPageVtx[j + 3].v.ob[2] = 0;

        pauseCtx->mapPageVtx[j + 0].v.flag = pauseCtx->mapPageVtx[j + 1].v.flag =
            pauseCtx->mapPageVtx[j + 2].v.flag = pauseCtx->mapPageVtx[j + 3].v.flag = 0;

        pauseCtx->mapPageVtx[j + 0].v.tc[0] = pauseCtx->mapPageVtx[j + 0].v.tc[1] =
            pauseCtx->mapPageVtx[j + 1].v.tc[1] = pauseCtx->mapPageVtx[j + 2].v.tc[0] = 0;

        pauseCtx->mapPageVtx[j + 1].v.tc[0] = pauseCtx->mapPageVtx[j + 3].v.tc[0] =
            WORLD_MAP_IMAGE_WIDTH * (1 << 5);
        pauseCtx->mapPageVtx[j + 2].v.tc[1] = pauseCtx->mapPageVtx[j + 3].v.tc[1] =
            WORLD_MAP_IMAGE_FRAG_HEIGHT * (1 << 5);

        pauseCtx->mapPageVtx[j + 0].v.cn[0] = pauseCtx->mapPageVtx[j + 2].v.cn[0] =
            pauseCtx->mapPageVtx[j + 0].v.cn[1] = pauseCtx->mapPageVtx[j + 2].v.cn[1] =
                pauseCtx->mapPageVtx[j + 0].v.cn[2] = pauseCtx->mapPageVtx[j + 2].v.cn[2] =
                    pauseCtx->mapPageVtx[j + 1].v.cn[0] = pauseCtx->mapPageVtx[j + 3].v.cn[0] =
                        pauseCtx->mapPageVtx[j + 1].v.cn[1] = pauseCtx->mapPageVtx[j + 3].v.cn[1] =
                            pauseCtx->mapPageVtx[j + 1].v.cn[2] = pauseCtx->mapPageVtx[j + 3].v.cn[2] =
                                pauseCtx->mapPageVtx[j + 0].v.cn[3] = pauseCtx->mapPageVtx[j + 2].v.cn[3] =
                                    pauseCtx->mapPageVtx[j + 1].v.cn[3] = pauseCtx->mapPageVtx[j + 3].v.cn[3] =
                                        pauseCtx->alpha;
    }

    pauseCtx->mapPageVtx[j - 2].v.ob[1] = pauseCtx->mapPageVtx[j - 1].v.ob[1] =
        pauseCtx->mapPageVtx[j - 4].v.ob[1] - (WORLD_MAP_IMAGE_HEIGHT % WORLD_MAP_IMAGE_FRAG_HEIGHT);

    pauseCtx->mapPageVtx[j - 2].v.tc[1] = pauseCtx->mapPageVtx[j - 1].v.tc[1] =
        (WORLD_MAP_IMAGE_HEIGHT % WORLD_MAP_IMAGE_FRAG_HEIGHT) * (1 << 5);

    pauseCtx->cursorVtx = Graph_Alloc(play->state.gfxCtx, (PAUSE_QUAD_CURSOR_MAX * 4) * sizeof(Vtx));

    for (i = 0; i < (PAUSE_QUAD_CURSOR_MAX * 4); i++) {
        pauseCtx->cursorVtx[i].v.ob[0] = pauseCtx->cursorVtx[i].v.ob[1] = pauseCtx->cursorVtx[i].v.ob[2] = 0;

        pauseCtx->cursorVtx[i].v.flag = 0;

        pauseCtx->cursorVtx[i].v.tc[0] = pauseCtx->cursorVtx[i].v.tc[1] = 0;

        pauseCtx->cursorVtx[i].v.cn[0] = pauseCtx->cursorVtx[i].v.cn[1] = pauseCtx->cursorVtx[i].v.cn[2] =
            pauseCtx->cursorVtx[i].v.cn[3] = 255;
    }

    /* PAUSE_QUAD_CURSOR_0 */
    pauseCtx->cursorVtx[1].v.tc[0] = pauseCtx->cursorVtx[2].v.tc[1] = pauseCtx->cursorVtx[3].v.tc[0] =
        pauseCtx->cursorVtx[3].v.tc[1]
        /* PAUSE_QUAD_CURSOR_1 */
        = pauseCtx->cursorVtx[5].v.tc[0] = pauseCtx->cursorVtx[6].v.tc[1] = pauseCtx->cursorVtx[7].v.tc[0] =
            pauseCtx->cursorVtx[7].v.tc[1]
        /* PAUSE_QUAD_CURSOR_2 */
        = pauseCtx->cursorVtx[9].v.tc[0] = pauseCtx->cursorVtx[10].v.tc[1] = pauseCtx->cursorVtx[11].v.tc[0] =
            pauseCtx->cursorVtx[11].v.tc[1]
        /* PAUSE_QUAD_CURSOR_3 */
        = pauseCtx->cursorVtx[13].v.tc[0] = pauseCtx->cursorVtx[14].v.tc[1] = pauseCtx->cursorVtx[15].v.tc[0] =
            pauseCtx->cursorVtx[15].v.tc[1] = 16 * (1 << 5);

    /* PAUSE_QUAD_CURSOR_4 */
    pauseCtx->cursorVtx[17].v.tc[0] = pauseCtx->cursorVtx[18].v.tc[1] = pauseCtx->cursorVtx[19].v.tc[0] =
        pauseCtx->cursorVtx[19].v.tc[1] = 32 * (1 << 5);

    pauseCtx->infoPanelVtx = Graph_Alloc(gfxCtx, 28 * sizeof(Vtx));
}


static const char* sOwlNames[]={gMapPointGreatBayCoastENGTex,gMapPointZoraCapeENGTex,
    gMapPointSnowheadENGTex,gMapPointMountainVillageENGTex,gMapPointClockTownENGTex,gMapPointMilkRoadENGTex,
    gMapPointWoodfallENGTex,gMapPointSouthernSwampENGTex,gMapPointIkanaCanyonENGTex,gMapPointStoneTowerENGTex};
static void SetSelection(PlayState* play, int owl) {
    PauseContext* pauseCtx=&play->pauseCtx;
    pauseCtx->cursorPoint[PAUSE_WORLD_MAP]=owl;
    pauseCtx->namedItem=owl; pauseCtx->nameDisplayTimer=0;
    pauseCtx->nameSegment=(void*)sOwlNames[owl];
    sCursorShrinkRate=4;
}
int Combo_SOHSoaringMapActive(void) { return sMapPlay!=NULL; }
int Combo_SOHSoaringMapBegin(PlayState* play, uint32_t choices) {
    if(sMapPlay || !(choices&0xffc0) || !Combo_SOHPrepareWarpAssets()) return 0;
    // MM waits for Oceff_Wipe6 to finish before pausing the world. Otherwise
    // the actor stops updating and its last screen wipe stays behind the map.
    for(Actor* actor=play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head; actor; actor=actor->next) {
        if(actor->id==ACTOR_COMBO_OWL_WARP && actor->params==0x100 && actor->update) return 1;
    }
    sSavedPause=play->pauseCtx; sMapPlay=play; sChoices=choices;
    sSavedUpdateRate=R_UPDATE_RATE; R_UPDATE_RATE=2;
    play->pauseCtx.state=PAUSE_STATE_OWLWARP_3;
    play->pauseCtx.eye=(Vec3f){-64,0,0};
    play->pauseCtx.alpha=0; play->pauseCtx.infoPanelOffsetY=-80;
    play->pauseCtx.cursorSpecialPos=0; play->pauseCtx.unk_1E4=0;
    play->pauseCtx.cursorColorSet=4; play->pauseCtx.nameColorSet=0;
    for(int owl=0;owl<10;owl++) play->pauseCtx.worldMapPoints[owl]=(choices>>(owl+6))&1;
    for(int owl=0;owl<10;owl++) if(choices&(1u<<(owl+6))) { SetSelection(play,owl); break; }
    sStick=0; sRepeat=0; sMapPageRoll=320; sMapVerticalOffset=-6240;
    sMapReply=-1; R_PAUSE_OWLWARP_ALPHA=0;
    sSavedHud=gSaveContext.hudVisibilityMode;
    Message_CloseTextbox(play);
    Interface_SetDoAction(play, DO_ACTION_DECIDE);
    Interface_LoadActionLabelB(play, DO_ACTION_STOP);
    return 1;
}
void Combo_SOHSoaringMapClose(PlayState* play) {
    if(sMapPlay!=play) return;
    play->pauseCtx=sSavedPause; sMapPlay=NULL; R_UPDATE_RATE=sSavedUpdateRate;
    Interface_SetDoAction(play, DO_ACTION_NONE);
    Interface_ChangeHudVisibilityMode(sSavedHud);
}
void Combo_SOHSoaringMapFinish(PlayState* play, int reply) {
    if(sMapPlay!=play || play->pauseCtx.state==PAUSE_STATE_OWLWARP_6) return;
    sMapReply=reply; sMapVerticalOffset=-6240;
    play->pauseCtx.state=PAUSE_STATE_OWLWARP_6;
    Message_CloseTextbox(play);
}
int Combo_SOHSoaringMapUpdate(PlayState* play) {
    if(sMapPlay!=play) return 0;
    if(play->pauseCtx.state==PAUSE_STATE_OWLWARP_3 || play->pauseCtx.state==PAUSE_STATE_OWLWARP_6) {
        // Native MM page counters, once per 30 Hz pause tick.
        {
            if(play->pauseCtx.state==PAUSE_STATE_OWLWARP_3) {
                sMapPageRoll-=40;
                play->pauseCtx.alpha=MIN(play->pauseCtx.alpha+31,255);
                play->pauseCtx.infoPanelOffsetY+=10;
                R_PAUSE_OWLWARP_ALPHA=MIN(R_PAUSE_OWLWARP_ALPHA+20,120);
                if(sMapPageRoll<=0) {
                    sMapPageRoll=0; sMapVerticalOffset=0;
                    play->pauseCtx.alpha=255; play->pauseCtx.infoPanelOffsetY=0;
                    play->pauseCtx.state=PAUSE_STATE_OWLWARP_SELECT;
                }
            } else {
                sMapPageRoll+=40;
                play->pauseCtx.alpha=MAX(play->pauseCtx.alpha-63,0);
                play->pauseCtx.infoPanelOffsetY-=10;
                R_PAUSE_OWLWARP_ALPHA=MAX(R_PAUSE_OWLWARP_ALPHA-60,0);
                if(sMapPageRoll>=160) {
                    int reply=sMapReply;
                    Combo_SOHSoaringMapClose(play);
                    SOH_ForeignSongChoose(reply);
                }
            }
        }
        return 1;
    }
    if(play->pauseCtx.state==PAUSE_STATE_OWLWARP_CONFIRM) { Message_Update(play); return 1; }
    Input* input=&play->state.input[0];
    if(input->press.button&(BTN_B|BTN_START)) { SOH_ForeignSongChoose(-1); return 1; }
    if(input->press.button&BTN_A) {
        input->press.button &= ~BTN_A;
        Audio_PlaySoundGeneral(NA_SE_SY_DECIDE,&gSfxDefaultPos,4,&gSfxDefaultFreqAndVolScale,
            &gSfxDefaultFreqAndVolScale,&gSfxDefaultReverb);
        Combo_SOHMapConfirm(play->pauseCtx.cursorPoint[PAUSE_WORLD_MAP]+6);
        play->pauseCtx.state=PAUSE_STATE_OWLWARP_CONFIRM;
        return 1;
    }
    int stick=input->cur.stick_x>30 ? 1 : input->cur.stick_x< -30 ? -1 : 0;
    int direction=(input->press.button&BTN_DRIGHT) ? 1 : (input->press.button&BTN_DLEFT) ? -1 : 0;
    if(stick) { if(stick!=sStick || --sRepeat<=0) {direction=stick; sRepeat=stick!=sStick ? 10 : 3;} }
    sStick=stick;
    if(direction) {
        int current=play->pauseCtx.cursorPoint[PAUSE_WORLD_MAP];
        for(int step=1;step<=10;step++) {
            int owl=(current+direction*step+10)%10;
            if(sChoices&(1u<<(owl+6))) { SetSelection(play,owl); break; }
        }
        Audio_PlaySoundGeneral(NA_SE_SY_CURSOR,&gSfxDefaultPos,4,&gSfxDefaultFreqAndVolScale,
            &gSfxDefaultFreqAndVolScale,&gSfxDefaultReverb);
    }
    return 1;
}
int Combo_SOHSoaringMapDraw(PlayState* play) {
    if(sMapPlay!=play) return 0;
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPush(POLY_OPA_DISP++, "mm-warp");
    KaleidoScope_SetView(&play->pauseCtx,-64,0,0);
    KaleidoScope_SetVertices(play,play->state.gfxCtx);
    Gfx_SetupDL_42Opa(play->state.gfxCtx);
    gDPSetCombineLERP(POLY_OPA_DISP++,TEXEL0,0,PRIMITIVE,0,TEXEL0,0,SHADE,0,TEXEL0,0,PRIMITIVE,0,TEXEL0,0,SHADE,0);
    gDPSetPrimColor(POLY_OPA_DISP++,0,0,180,180,120,255);
    Matrix_RotateY(-1.57f,MTXMODE_NEW);
    Matrix_Translate(0,sMapVerticalOffset/100,-93,MTXMODE_APPLY);
    Matrix_Scale(.78f,.78f,.78f,MTXMODE_APPLY);
    Matrix_RotateX(-sMapPageRoll/100,MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++,MATRIX_NEWMTX(play->state.gfxCtx),G_MTX_NOPUSH|G_MTX_LOAD|G_MTX_MODELVIEW);
    POLY_OPA_DISP=KaleidoScope_DrawPageSections(POLY_OPA_DISP,play->pauseCtx.mapPageVtx,sMapPageBgTextures);
    Matrix_RotateY(-1.57f,MTXMODE_NEW);
    Matrix_Translate(0,(play->pauseCtx.state==PAUSE_STATE_OWLWARP_3 ||
        play->pauseCtx.state==PAUSE_STATE_OWLWARP_6) ? -80.9f : -.9f,-144,MTXMODE_APPLY);
    Matrix_RotateX(-sMapPageRoll/100,MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++,MATRIX_NEWMTX(play->state.gfxCtx),G_MTX_NOPUSH|G_MTX_LOAD|G_MTX_MODELVIEW);
    KaleidoScope_DrawWorldMap(play);
    Gfx_SetupDL_42Opa(play->state.gfxCtx);
    KaleidoScope_SetView(&play->pauseCtx,0,0,64);
    KaleidoScope_DrawOwlWarpInfoPanel(play); KaleidoScope_UpdateCursorSize(play);
    if(play->pauseCtx.state==PAUSE_STATE_OWLWARP_SELECT) KaleidoScope_DrawCursor(play);
    gSPComboRMPop(POLY_OPA_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    return 1;
}

#endif
