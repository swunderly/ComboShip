#pragma once
// Master Quest dungeon data bundled from OoTMM v32.3 (README.md in this folder), decoded into native structs.
// Pure C++17 without game headers, so host tests build it unchanged. The inputs are OoTMM's mq_rooms.bin,
// mq_scenes.bin and mq_maps.bin, written by OoTMM scripts/make_mq.ts and read by OoTMM src/oot/mq.c; every
// field in them is big-endian.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ComboMq {

// OoTMM's Master Quest dungeon index: make_mq.ts DUNGEONS and include/combo/dungeon.h MQ_*. It is also the bit
// of the mqDungeons setting and the dungeon field inside the bins.
enum DungeonId : uint8_t {
    kDekuTree,
    kDodongosCavern,
    kJabuJabu,
    kForestTemple,
    kFireTemple,
    kWaterTemple,
    kSpiritTemple,
    kShadowTemple,
    kBottomOfTheWell,
    kIceCavern,
    kGerudoTrainingGround,
    kGanonsCastle,
};
constexpr int kDungeonCount = 12;

// SoH scene id to OoTMM dungeon index, as mq.c mqDungeonId: scenes 0x00-0x09 give 0-9, Gerudo Training Ground
// (0x0B) gives 10 and Inside Ganon's Castle (0x0D) gives 11. Every other scene, boss rooms included, gives -1.
constexpr int DungeonFromScene(int sceneId) {
    if (sceneId >= 0x00 && sceneId <= 0x09)
        return sceneId;
    if (sceneId == 0x0B)
        return kGerudoTrainingGround;
    if (sceneId == 0x0D)
        return kGanonsCastle;
    return -1;
}

// Buffer limits of mq.c (ROOM_ACTORS_MAX, ROOM_OBJECTS_MAX, ROOM_BUFFER_SIZE, SCENE_TRANSITION_ACTORS_MAX,
// SCENE_BUFFER_SIZE). OoTMM DMAs a room or scene entry's whole blob into these buffers.
constexpr size_t kRoomActorsMax = 64;
constexpr size_t kRoomObjectsMax = 15;
constexpr size_t kRoomBufferSize = 0x10 * kRoomActorsMax + 2 * kRoomObjectsMax;
constexpr size_t kSceneTransitionActorsMax = 64;
constexpr size_t kSceneBufferSize = 0x10 * kSceneTransitionActorsMax;

// make_mq.ts PATHS: how many scene paths each dungeon carries (Fire 4, Water 2, Spirit 5, Bottom of the Well 2,
// Ice Cavern 5, Ganon's Castle 6). A dungeon with 0 keeps its vanilla path list. The path header array has no
// terminator inside the entry for Water and Ice, so the count comes from here, never from a zero byte.
constexpr uint8_t kPathCounts[kDungeonCount] = { 0, 0, 0, 0, 4, 2, 5, 0, 2, 5, 0, 6 };

// make_mq.ts extracts minimap marks only for scenes below 10, so Gerudo Training Ground and Ganon's Castle have none.
constexpr int kMinimapDungeonCount = 10;
// mq_maps.bin is the MQ pause-map mark table: 34 floors of 3 records each (make_mq.ts makeMaps).
constexpr size_t kPauseFloorCount = 34;
constexpr size_t kPauseMarksPerFloor = 3;
constexpr size_t kPausePointsMax = 12;
constexpr size_t kMinimapPointsMax = 12;

struct Vec3sPOD {
    int16_t x, y, z;
};
// The layout of SoH's ActorEntry (z64.h): a room actor list entry.
struct ActorEntryPOD {
    int16_t id;
    Vec3sPOD pos;
    Vec3sPOD rot;
    int16_t params;
};
static_assert(sizeof(ActorEntryPOD) == 0x10, "ActorEntry is 0x10 bytes");
// The layout of SoH's TransitionActorEntry (z64.h): sides[0] is the front room, sides[1] the back room.
struct TransitionActorPOD {
    struct Side {
        int8_t room;
        int8_t effects;
    } sides[2];
    int16_t id;
    Vec3sPOD pos;
    int16_t rotY;
    int16_t params;
};
static_assert(sizeof(TransitionActorPOD) == 0x10, "TransitionActorEntry is 0x10 bytes");
// The layout of SoH's MapMarkPoint, MapMarkIconData and MapMarkData (z64map_mark.h). All single bytes.
struct MinimapPointPOD {
    int8_t chestFlag;
    uint8_t x, y;
};
struct MinimapIconsPOD {
    int8_t markType; // -1 none, 0 chest, 1 boss
    uint8_t count;
    MinimapPointPOD points[kMinimapPointsMax];
};
struct MinimapRoomPOD {
    MinimapIconsPOD icons[3];
};
static_assert(sizeof(MinimapRoomPOD) == 0x72, "MapMarkData is 0x72 bytes");
// SoH's PauseMapMarkPoint and PauseMapMarkData (z64.h) with native floats. The N64 vertex pointer becomes hasVtx:
// OoTMM's kaleido hook points every non-null one at the overlay's own vertices (mq.c comboMqKaleidoHook).
struct PausePointPOD {
    int16_t chestFlag;
    float x, y;
};
struct PauseMarkPOD {
    int16_t markType; // -1 none, 0 chest, 1 boss
    int32_t unk04;
    bool hasVtx;
    int32_t vtxCount;
    int32_t count;
    PausePointPOD points[kPausePointsMax];
};
// Collision patches (mq.c PATCH_POLY and PATCH_POLY_TYPE). Only Fire Temple has any in the shipped data.
struct PolyPatch {
    uint16_t id;
    uint8_t type;
    uint8_t flags; // the top 3 bits of flags_vIA
};
struct SurfacePatch {
    uint32_t id;
    uint32_t hi;
    uint32_t lo;
};

struct Room {
    uint8_t dungeon = 0;
    uint8_t room = 0;
    // May be empty: OoTMM then sets the vanilla actor list's count to 0 (ten rooms have no actors).
    std::vector<ActorEntryPOD> actors;
    std::vector<int16_t> objects;
};
struct Scene {
    std::vector<TransitionActorPOD> transitions;
    // False when the entry's pathOffset is 0xffff: the vanilla path list stays.
    bool hasPaths = false;
    std::vector<std::vector<Vec3sPOD>> paths;
    // Whole 0x72-byte entries of the MQ map-mark data, by room. Empty for Gerudo Training Ground and Ganon's Castle.
    std::vector<MinimapRoomPOD> minimap;
    std::vector<PolyPatch> polyPatches;
    std::vector<SurfacePatch> surfacePatches;
};
struct Dungeon {
    std::vector<Room> rooms; // indexed by room id; ids are contiguous from 0
    Scene scene;
};
struct Data {
    Dungeon dungeons[kDungeonCount];
    PauseMarkPOD pauseFloors[kPauseFloorCount][kPauseMarksPerFloor] = {};
    size_t RoomCount() const;
    const Room* FindRoom(int dungeon, int room) const;
};

struct Bins {
    const uint8_t* rooms = nullptr;
    size_t roomsSize = 0;
    const uint8_t* scenes = nullptr;
    size_t scenesSize = 0;
    const uint8_t* maps = nullptr;
    size_t mapsSize = 0;
};

// Decodes and checks the three files. Fails closed: on a truncated file or any offset, size, count or field out of
// range it returns false, leaves `out` unchanged and says why in `error`.
bool Decode(const Bins& bins, Data& out, std::string& error);
// The three files compiled into this library (CMake/ComboEmbedBinary.cmake, pinned in combo/mq/ComboMq.cmake).
Bins EmbeddedBins();
// EmbeddedBins() decoded on first use. nullptr when they do not decode; BundledError() then says why.
const Data* Bundled();
const std::string& BundledError();

} // namespace ComboMq
