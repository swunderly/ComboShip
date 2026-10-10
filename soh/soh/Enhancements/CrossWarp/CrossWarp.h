#pragma once
#include "global.h"
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
