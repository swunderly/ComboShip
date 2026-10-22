#pragma once
#ifdef COMBO_BUILD
#include <stdint.h>
struct PlayState;
struct Actor;
#ifdef __cplusplus
extern "C" {
#endif
int Combo_OOTWallmasterTransport(struct PlayState* play, struct Actor* actor);
// CrossEntrance.inc.h: queue a Full Wallmaster capture's departure to an MM dungeon by its MM registry id.
int Combo_OotRequestWallmasterDeparture(uint32_t mmId);
int Combo_OOTWallmasterSpawnInClearedRoom(struct PlayState* play, int actorId, int params);
#ifdef __cplusplus
}
#endif
#endif
