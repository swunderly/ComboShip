#pragma once
#include <stdint.h>
/* Same native slot0..2, explicit games OoT0/MM1, full logical IDs. These
 * callbacks are independent of song destinations and Farore point storage.
 * Preflight does not arm a destination or change the foreground loop. Commit
 * returns1 only after the same-slot destination has been armed. */
typedef int (*ComboEntrancePreflight)(int fileNum,int sourceGame,int destinationGame,
                                    uint32_t logicalId,int sourceAge,int crossAge);
typedef int (*ComboEntranceCommit)(int fileNum,int sourceGame,int destinationGame,
                                 uint32_t logicalId,int sourceAge,int crossAge);
#ifdef __cplusplus
extern "C" {
#endif
int SOH_RequestEntranceDeparture(int fileNum,int destinationGame,uint32_t logicalId,int crossAge);
int MM_RequestEntranceDeparture(int fileNum,int destinationGame,uint32_t logicalId,int crossAge);
int Combo_OotFinishEntranceDeparture(void* play);
int Combo_MMFinishEntranceDeparture(void* play);
int MM_ConsumeFinishedEntranceDeparture(void);
int SOH_ConsumeEntranceArrival(void);
int MM_ConsumeEntranceArrival(void);
#ifdef __cplusplus
}
#endif
