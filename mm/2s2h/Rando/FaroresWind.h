#pragma once
#include "global.h"
#include "rando/SharedDungeon.h"
#include "rando/CrossArrival.h"
#ifdef __cplusplus
extern "C" {
#endif
// Scalar bridge only. Each engine retains its own typed points and backups.
typedef int (*ComboFaroreGetOotAge)(int fileNum);
typedef int (*ComboFaroreGetOotDestination)(int fileNum, int age, int* entrance);
void MM_SetFaroreBridge(ComboFaroreGetOotAge, ComboFaroreGetOotDestination);
void MM_ConfigureFaroreRules(int spellWindMm, int crossGameFw, int crossAge);
void MM_SetFaroreAgeMode(int adultMask);
typedef int (*ComboFaroreAgeModeEnabled)(int fileNum);
void MM_SetFaroreAgeModeBridge(ComboFaroreAgeModeEnabled);
void Combo_MMFaroreGrantItem(void);
int Combo_MMFaroreRefreshHumanRespawn(void);
typedef int (*ComboFaroreEffectiveAge)(int fileNum);
typedef int (*ComboFaroreTransitionAge)(int fileNum, int age);
void MM_SetFaroreAgeBridge(ComboFaroreEffectiveAge, ComboFaroreTransitionAge can, ComboFaroreTransitionAge apply);
typedef int (*ComboFaroreSyncOotPoint)(int fileNum, int age, int marker, int entrance);
void MM_SetFaroreSyncOot(ComboFaroreSyncOotPoint);
int MM_GetFaroreMarker(int fileNum, int age, int* marker, int* entrance);
int MM_SyncFaroreFromOot(int fileNum, int age, int marker, int entrance);
void Combo_MMFaroreCaptureDungeon(PlayState*);
void Combo_MMFaroreDeathWarp(PlayState*);
void Combo_MMFaroreEndCycle(void);
int MM_TakeFaroreDeparture(int* age, int* entrance);
void MM_SetSharedDungeonBridge(ComboReadSharedDungeon, ComboWriteSharedDungeon);
int MM_TakeDungeonDeparture(int* age, int* entrance);
int MM_SetDungeonArrival(int fileNum, int age, int entrance);
int MM_ConsumeDungeonArrival(void);
int MM_SetFaroreArrival(int fileNum, int age);
int MM_SetFaroreArrivalEntrance(int fileNum, int age, int entrance);
int MM_ConsumeFaroreArrival(void);
int MM_FaroreDeparturePending(void);
void MM_SetNativeDepartureCommit(ComboNativeDepartureCommit);
int MM_DispatchNativeDeparture(PlayState*);
void MM_CancelNativeDeparture(void);
void Combo_MMFaroreCancelReturn(PlayState*);
int MM_GetFaroreDestination(int fileNum, int age, int* entrance);
int Combo_MMFaroreAge(void);
int Combo_MMFaroreEnabled(void);
int Combo_MMFaroreBridgeReady(void);
int Combo_MMFaroreHasValidPoint(void);
int Combo_MMFaroreReturn(PlayState* play);
void Combo_MMFaroreBeforePlayerInit(PlayState* play);
void Combo_MMFarorePrepareArrival(void);
void Combo_MMFaroreStorePoint(PlayState* play);
void Combo_MMFaroreDispel(void);
int Combo_MMFarorePrepareResources(void);
void Combo_MMFaroreDrawPointer(PlayState* play);
void Combo_MMFaroreInitPointer(PlayState* play);
void Combo_MMFaroreDestroyPointer(PlayState* play);
void* Combo_MMFaroreTexture(void);
// Stores the scene and cycle flags into the permanent flags a cross-game departure save writes.
void Combo_MMStoreDepartureFlags(PlayState* play);
#define COMBO_FARORE_CROSS_ENTRANCE 0xfffd
#ifdef __cplusplus
}
#endif
