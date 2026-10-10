#include "MissingMmSouls.h"
extern "C" {
#include "variables.h"
}
namespace Rando {
bool HaveMissingMmSoul(unsigned index) {
    if(index>=6) return false;
    return !MissingMmSoulShuffled(RANDO_SAVE_OPTIONS,index) || MissingMmSoulOwned(gSaveContext.save.shipSaveInfo.rando.mmMissingSoulsOwned,index);
}
bool GrantMissingMmSoul(RandoItemId item) {
    const int i=MissingMmSoulIndex(item);if(i<0) return false;
    return ChangeMissingMmSoul(gSaveContext.save.shipSaveInfo.rando.mmMissingSoulsOwned,i,true);
}
bool RemoveMissingMmSoul(RandoItemId item) {
    const int i=MissingMmSoulIndex(item);if(i<0) return false;
    return ChangeMissingMmSoul(gSaveContext.save.shipSaveInfo.rando.mmMissingSoulsOwned,i,false);
}
}
static Rando::MissingSoulRoomGuard missingSoulRoom;
extern "C" void MM_MissingSoulRoomReset(PlayState* play) {missingSoulRoom.reset(play,play->roomCtx.curRoom.num);}
extern "C" void MM_MissingSoulRoomBlock(PlayState* play) {missingSoulRoom.deny(play,play->roomCtx.curRoom.num,0);}
extern "C" s32 MM_MissingSoulRoomCanClear(PlayState* play,s32 room) {return missingSoulRoom.canClear(play,room);}
