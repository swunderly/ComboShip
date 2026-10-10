#ifndef SOH_RESTORATIONS_SHADOW_DOOR_UNLOAD_H
#define SOH_RESTORATIONS_SHADOW_DOOR_UNLOAD_H

/* Compatibility recreation of the N64 Graveyard stair-reload door unload.
 * The port's heap, actor overlays and object loading differ from N64, so this
 * implements the explicitly chosen five-trip setup without leaking memory or
 * changing saved switches. The staircase actor owns this visit-local state.
 */
#define SHADOW_DOOR_UNLOAD_CYCLES 5u

typedef struct ShadowDoorUnloadState {
    signed char startRoom;
    signed char lastRoom;
    unsigned char cycles;
} ShadowDoorUnloadState;

static inline int ShadowDoorUnload_IsRoom(int room) {
    return room == 0 || room == 1;
}

static inline void ShadowDoorUnload_Reset(ShadowDoorUnloadState* state, int room) {
    state->startRoom = state->lastRoom = ShadowDoorUnload_IsRoom(room) ? (signed char)room : -1;
    state->cycles = 0;
}

static inline int ShadowDoorUnload_IsStaircase(int scene, unsigned int params,
                                              float x, float y, float z, int rotY) {
    /* Actor_SpawnTransitionActors adds the transition-list index in bits 10-15. */
    return scene == 0x53 && (params & 0x03FFu) == 0x013Fu && x == 1519.0f && y == 239.0f &&
           z == 93.0f && rotY == 0x4000;
}

/* Called only after a real staircase room load has completed. Initial scene
 * loads, repeated notifications and nonalternating room changes earn no trip.
 */
static inline void ShadowDoorUnload_RecordRoomLoad(ShadowDoorUnloadState* state, int enabled,
                                                  int authenticStaircase, int previousRoom, int loadedRoom) {
    if (!enabled) {
        ShadowDoorUnload_Reset(state, loadedRoom);
        return;
    }
    if (!authenticStaircase) {
        return;
    }
    if (!ShadowDoorUnload_IsRoom(previousRoom) || !ShadowDoorUnload_IsRoom(loadedRoom)) {
        ShadowDoorUnload_Reset(state, loadedRoom);
        return;
    }
    if (loadedRoom == state->lastRoom) {
        return;
    }
    if (previousRoom != state->lastRoom) {
        ShadowDoorUnload_Reset(state, loadedRoom);
        return;
    }
    state->lastRoom = (signed char)loadedRoom;
    if (loadedRoom == state->startRoom && state->cycles < SHADOW_DOOR_UNLOAD_CYCLES) {
        ++state->cycles;
    }
}

static inline int ShadowDoorUnload_IsActive(const ShadowDoorUnloadState* state, int enabled) {
    return enabled && state->cycles >= SHADOW_DOOR_UNLOAD_CYCLES;
}

static inline int ShadowDoorUnload_IsDoor(int scene, int room, int actorId, unsigned int params) {
    return scene == 0x53 && room == 0 && actorId == 0x009C && (params & 0xFFu) == 0;
}

#endif
