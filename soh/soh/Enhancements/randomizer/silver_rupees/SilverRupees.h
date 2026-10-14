#pragma once
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#ifdef __cplusplus
#include "SilverRupeeState.h"
#include "soh/Enhancements/randomizer/item.h"
#include "soh/Enhancements/randomizer/location.h"
namespace Rando {
class Logic;
}
namespace Rando::SilverRupees {
void RegisterItems(std::array<Item, RG_MAX>& items);
void RegisterLocations(std::array<Location, RC_MAX>& locations);
bool IsCheck(RandomizerCheck check);
bool IsShuffledCheck(RandomizerCheck check);
bool IsItem(RandomizerGet item);
int PuzzleForItem(RandomizerGet item);
bool HasPuzzle(Logic* logic, size_t slot);
void LogicDelta(Logic* logic, RandomizerGet item, bool add);
void AddPool(std::vector<RandomizerGet>& pool);
int CountAfterReceiving(RandomizerGet item);
} // namespace Rando::SilverRupees
extern "C" {
#endif
int Combo_SilverShuffleEnabled(void);
void Combo_SilverResetRoom(void);
int Combo_SilverActorInit(void* actor, void* play);
int Combo_SilverActorIdle(void* actor, void* play);
int Combo_SilverGive(void* play, int item);
void Combo_SilverUpdate(void* play);
#ifdef __cplusplus
}
#endif
