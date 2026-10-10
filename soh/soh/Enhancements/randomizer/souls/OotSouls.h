#pragma once
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#ifdef __cplusplus
#include "OotSoulState.h"
#include "soh/Enhancements/randomizer/item.h"
namespace Rando {class Logic;}
namespace Rando::Souls {
void RegisterItems(std::array<Item,RG_MAX>& items);
void AddPool(std::vector<RandomizerGet>& pool);
int GeneralIndex(RandomizerGet item);
bool Has(Logic* logic,OotSoul soul);
bool HasItem(Logic* logic,RandomizerGet item);
void LogicDelta(Logic* logic,RandomizerGet item,bool add);
bool EnemySoul(Logic* logic,RandomizerEnemy enemy);
bool Enabled();
bool FamilyVisible(unsigned family);
bool RuntimeOwned(RandomizerGet item);
bool TcgSoulAccess(Logic* logic);
void AddUnshuffledStarts(std::vector<RandomizerGet>& starts);
void InitStartingOwnership();
}
extern "C" {
#endif
int Combo_OotSoulGive(void* play,int item);
int Combo_OotSoulPreSpawn(void* play,int actor,unsigned params);
void Combo_OotSoulResetRoom(void* play);
int Combo_OotSoulCanClear(void* play,int room);
#ifdef __cplusplus
}
#endif
