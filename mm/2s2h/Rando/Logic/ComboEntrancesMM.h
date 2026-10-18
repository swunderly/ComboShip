#pragma once
#ifdef COMBO_BUILD
#include "2s2h/Rando/Logic/Logic.h"
namespace Rando::Logic {
// Called after vanilla exit traversal. Source predicates/time are preserved.
bool ExploreMmWallmasterEntrances(RandoRegionId region, uint64_t sourceTimes, std::set<RandoRegionId>& reachable,
                                  std::unordered_map<RandoRegionId, RegionTimeState>& times);
} // namespace Rando::Logic
extern "C" {
int MM_ApplyEntranceExpansionForCombo(const char* seedJson);
const char* MM_DumpEntranceExpansionForCombo();
int MM_SetEntranceExpansionAssumptionsForCombo(const char* candidateJson);
void MM_ClearEntranceExpansionAssumptionsForCombo();
int MM_SetEntranceGenerationSettingsForCombo(const char* settingsJson);
int Combo_MMCompleteOrdinaryEntrance(PlayState* play);
void Combo_MMClearOrdinaryTransitionMarker(void);
int Combo_MMWallmasterTransport(PlayState* play, Actor* actor);
int Combo_MMWallmasterErActive(void);
int Combo_MMWallmasterSoulSpawnAllowed(int actorId);
int Combo_MMEntranceShuffleActive(void);
void Combo_MMRecordInitialEntrance(int entrance);
void Combo_MMReturnToInitialEntranceOnPlayInit(void);
void Combo_MMReturnToInitialEntranceAfterSkip(void);
void Combo_MMQueueResetDawn(void);
void Combo_MMAfterPlayInit(PlayState* play);
}
#endif
