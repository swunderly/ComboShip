#pragma once
namespace Rando::Souls {
// Keeps a room shut while an enemy of it was refused for a missing soul, as OoTMM v32.3 does: one flag for the play
// state, not one per room. Each room's actor list clears it (oot/room.c ParseSceneRoomHeaders_ActorsList), a refused
// enemy sets it (oot/actors.c Actor_SpawnWrapper), and while it is set the current room's temp clear for its last enemy
// does not happen (common/actors.c SetRoomClearHook, patched over oot/actors.S 0x8002508c). A room-transition plane
// (En_Holl) loads the next room's list while the old room stays loaded and the current room swaps between the two, so
// the flag is then the next room's, whichever of the two rooms a clear lands in.
class OotSoulRoomGuard {
    const void* play = nullptr;
    bool blocked = false;

  public:
    void RoomActorList(const void* playState) {
        play = playState;
        blocked = false;
    }
    void SoulRefused(const void* playState) {
        if (play != playState)
            RoomActorList(playState);
        blocked = true;
    }
    bool CanClear(const void* playState) const {
        return !(blocked && play == playState);
    }
};
} // namespace Rando::Souls
