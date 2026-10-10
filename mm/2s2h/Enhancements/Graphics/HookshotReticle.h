#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// Included after the native player/collision/Gfx types.
Gfx* MM_GetHookshotReticleDisplayList(PlayState* play, Player* player, CollisionPoly* poly, s32 bgId);
#ifdef __cplusplus
}
#endif
