#pragma once
#include "global.h"
#include "rando/SharedDungeon.h"
#include "rando/CrossArrival.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*ComboFaroreGetMmDestination)(int fileNum, int age, int* entrance);
typedef int (*ComboFaroreSyncMmPoint)(int fileNum, int age, int marker, int entrance);
void SOH_SetFaroreBridge(ComboFaroreGetMmDestination, ComboFaroreSyncMmPoint);
void SOH_ConfigureFaroreRules(int crossGameFw, int crossAge);
int SOH_GetFaroreAge(int fileNum);
int SOH_GetFaroreMarker(int fileNum, int age, int* marker, int* entrance);
int SOH_GetFaroreDestination(int fileNum, int age, int* entrance);
int SOH_SyncFaroreFromMm(int fileNum, int age, int marker, int entrance);
int SOH_FaroreDeparturePending(void);
void SOH_SetNativeDepartureCommit(ComboNativeDepartureCommit);
int SOH_DispatchNativeDeparture(PlayState*);
void SOH_CancelNativeDeparture(void);
void Combo_OotFaroreCancelReturn(PlayState*);
int SOH_TakeFaroreDeparture(int* age, int* entrance);
void SOH_SetSharedDungeonBridge(ComboReadSharedDungeon, ComboWriteSharedDungeon);
int SOH_TakeDungeonDeparture(int* age, int* entrance);
int SOH_SetDungeonArrival(int fileNum, int age, int entrance);
int SOH_ConsumeDungeonArrival(void);
int SOH_SetFaroreArrivalEntrance(int fileNum, int age, int entrance);
int SOH_ConsumeFaroreArrival(void);
int Combo_OotFaroreCrossEnabled(void);
void Combo_OotFaroreValidate(void);
int Combo_OotFaroreReturn(PlayState*);
void Combo_OotFaroreStorePoint(PlayState*);
void Combo_OotFaroreDispel(void);
void Combo_OotFaroreCaptureDungeon(PlayState*);
void Combo_OotFaroreClearDungeon(void);
void Combo_OotFaroreDeathWarp(PlayState*);
#ifdef __cplusplus
}
void Combo_OotFaroreLoad();
void Combo_OotFaroreSave(SaveContext*, int, bool);
#endif
