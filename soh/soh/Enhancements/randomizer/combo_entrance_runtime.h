#pragma once
#ifdef COMBO_BUILD
struct PlayState;
struct Actor;
#ifdef __cplusplus
extern "C" {
#endif
int Combo_OOTWallmasterTransport(struct PlayState* play, struct Actor* actor);
int Combo_OOTWallmasterSpawnInClearedRoom(struct PlayState* play, int actorId, int params);
#ifdef __cplusplus
}
#endif
#endif
