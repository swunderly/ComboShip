#include "Rando/ActorBehavior/MissingMmSouls.h"
#include "ExpandedSoulsNative.h"
extern "C" {
#include "variables.h"
}
namespace Rando {
bool HaveExpandedSoul(ExpandedSoul soul) {
 const unsigned i=static_cast<unsigned>(soul);
 if(i>=59) return false;
 if(!ExpandedSoulOption(RANDO_SAVE_OPTIONS,i)) return true;
 return (gSaveContext.save.shipSaveInfo.rando.expandedSoulsOwned[i>>3]&(1u<<(i&7)))!=0;
}
bool GrantExpandedSoulItem(RandoItemId item) {
 const int i=ExpandedSoulIndex(item);if(i<0)return false;
 gSaveContext.save.shipSaveInfo.rando.expandedSoulsOwned[i>>3]|=1u<<(i&7);return true;
}
bool RemoveExpandedSoulItem(RandoItemId item) {
 const int i=ExpandedSoulIndex(item);if(i<0)return false;
 gSaveContext.save.shipSaveInfo.rando.expandedSoulsOwned[i>>3]&=~(1u<<(i&7));return true;
}
}
extern "C" s32 MM_ExpandedSoulPreSpawn(PlayState* play,s16 actorId,s32* params) {
 if(!IS_RANDO) return 1;
 const int missing=Rando::EvaluateMissingMmSoulPreSpawn(actorId,static_cast<unsigned>(*params),
  RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS],gSaveContext.gameMode==GAMEMODE_END_CREDITS,
  RANDO_SAVE_OPTIONS[RO_SHUFFLE_BOSS_SOULS],RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS],
  gSaveContext.save.shipSaveInfo.rando.mmMissingSoulsOwned);
 if(missing<=0) return missing;
 Rando::ExpandedSoulsOwnership state{};
 std::copy_n(gSaveContext.save.shipSaveInfo.rando.expandedSoulsOwned,8,state.begin());
 const Rando::ExpandedSoulsOptions options{true,RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]==RO_GENERIC_YES,
 RANDO_SAVE_OPTIONS[RO_SHUFFLE_ANIMAL_SOULS]==RO_GENERIC_YES,RANDO_SAVE_OPTIONS[RO_SHUFFLE_MISC_SOULS]==RO_GENERIC_YES};
 unsigned variable=static_cast<unsigned>(*params);
 const int result=Rando::EvaluateExpandedSoulPreSpawn(play,actorId,variable,gSaveContext.gameMode==GAMEMODE_END_CREDITS,options,state);
 *params=static_cast<s32>(variable);return result;
}
