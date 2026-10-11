#pragma once
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#ifdef __cplusplus
#include "OotSoulState.h"
#include "soh/Enhancements/randomizer/item.h"
namespace Rando {
class Logic;
}
namespace Rando::Souls {
void RegisterItems(std::array<Item, RG_MAX>& items);
// The soul items of the shuffled families, one each, in save-bit order.
std::vector<RandomizerGet> ShuffledItems();
int GeneralIndex(RandomizerGet item);
bool Has(Logic* logic, OotSoul soul);
bool HasItem(Logic* logic, RandomizerGet item);
void LogicDelta(Logic* logic, RandomizerGet item, bool add);
bool EnemySoul(Logic* logic, RandomizerEnemy enemy);
bool Enabled();
// True when the seed shuffles this family's souls, so a missing soul keeps its actors away.
bool FamilyShuffled(OotSoulFamily family);
bool RuntimeOwned(RandomizerGet item);
bool TcgSoulAccess(Logic* logic);
void InitStartingOwnership();
} // namespace Rando::Souls
extern "C" {
#endif
int Combo_OotSoulGive(void* play, int item);
int Combo_OotSoulPreSpawn(void* play, int actor, unsigned params);
void Combo_OotSoulResetRoom(void* play);
int Combo_OotSoulCanClear(void* play, int room);
int Combo_OotDarkLinkSoulMissing(void);
#ifdef __cplusplus
}
#endif
