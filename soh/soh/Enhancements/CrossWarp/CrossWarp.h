#pragma once
#include "global.h"
// MM's owl-warp pause states, shared by the OoT soaring map and its song replies.
#define PAUSE_STATE_OWLWARP_3 0x104
#define PAUSE_STATE_OWLWARP_SELECT 0x106
#define PAUSE_STATE_OWLWARP_CONFIRM 0x107
#define PAUSE_STATE_OWLWARP_6 0x112
#ifdef __cplusplus
extern "C" {
#endif
int Combo_SOHPrepareWarpAssets(void);
int Combo_SOHWarpVertexCount(const char*);
void Combo_SOHWarpMaterial(PlayState*, const char*, int);
void Combo_SOHRestoreWarpMaterial(PlayState*, const char*);
void Combo_SOHStartSoaringEffect(PlayState*);
void Combo_SOHWindMaterial(PlayState*, int);
void* Combo_SOHWarpResource(const char* path);
uint32_t Combo_SOHMapClouds(void);
int Combo_SOHSoaringMapBegin(PlayState* play, uint32_t choices);
int Combo_SOHSoaringMapUpdate(PlayState* play);
int Combo_SOHSoaringMapDraw(PlayState* play);
void Combo_SOHSoaringMapClose(PlayState* play);
void Combo_SOHSoaringMapFinish(PlayState* play, int reply);
int Combo_SOHSoaringMapActive(void);
int Combo_SOHSoaringCutsceneActive(PlayState* play);
int Combo_SOHStartSoaring(PlayState* play, int destination);
void Combo_SOHCompleteWarp(PlayState* play, int destination);
#ifdef __cplusplus
}
#endif
