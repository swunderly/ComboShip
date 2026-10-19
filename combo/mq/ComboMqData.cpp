#include "ComboMqData.h"
#include "ComboMqBlobs.h"

#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>

namespace ComboMq {
namespace {

static_assert(std::numeric_limits<float>::is_iec559 && sizeof(float) == 4, "the pause map stores IEEE-754 floats");

constexpr size_t kRoomsHeaderSize = 0x20;  // mq_rooms.bin: u32 count, then zero
constexpr size_t kRoomEntrySize = 0x20;    // mq.c MqRoomHeader
constexpr size_t kScenesHeaderSize = 0x40; // mq_scenes.bin: u32 count, then zero
constexpr size_t kSceneEntrySize = 0x40;   // mq.c MqSceneHeader
constexpr size_t kPathHeaderSize = 8;      // u8 count, 3 zero bytes, u32 blob-relative points offset
constexpr size_t kPauseMarkSize = 0xa4;    // PauseMapMarkData
constexpr size_t kPauseFloorSize = kPauseMarkSize * kPauseMarksPerFloor;
constexpr size_t kMapsSize = kPauseFloorSize * kPauseFloorCount;
constexpr int kRoomIdLimit = 0x80; // SoH's room number is an s8

struct Reader {
    const uint8_t* data;
    size_t size;

    bool Has(uint64_t offset, uint64_t length) const {
        return offset <= size && length <= size - offset;
    }
    uint8_t U8(size_t at) const {
        return data[at];
    }
    uint16_t U16(size_t at) const {
        return static_cast<uint16_t>((data[at] << 8) | data[at + 1]);
    }
    uint32_t U32(size_t at) const {
        return (static_cast<uint32_t>(data[at]) << 24) | (static_cast<uint32_t>(data[at + 1]) << 16) |
               (static_cast<uint32_t>(data[at + 2]) << 8) | data[at + 3];
    }
    int8_t S8(size_t at) const {
        return static_cast<int8_t>(U8(at));
    }
    int16_t S16(size_t at) const {
        return static_cast<int16_t>(U16(at));
    }
    int32_t S32(size_t at) const {
        return static_cast<int32_t>(U32(at));
    }
    float F32(size_t at) const {
        uint32_t bits = U32(at);
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
    bool Zero(size_t at, size_t length) const {
        for (size_t i = 0; i < length; i++)
            if (data[at + i] != 0)
                return false;
        return true;
    }
};

std::string Hex(uint64_t value) {
    char text[24];
    std::snprintf(text, sizeof(text), "0x%llx", static_cast<unsigned long long>(value));
    return text;
}

bool Fail(std::string& error, const std::string& message) {
    error = message;
    return false;
}

Vec3sPOD ReadVec3s(const Reader& r, size_t at) {
    return { r.S16(at), r.S16(at + 2), r.S16(at + 4) };
}

bool DecodeRooms(const Reader& r, Data& data, std::string& error) {
    const std::string file = "mq_rooms.bin: ";
    if (r.data == nullptr || !r.Has(0, kRoomsHeaderSize))
        return Fail(error, file + "shorter than its " + Hex(kRoomsHeaderSize) + "-byte header");
    if (!r.Zero(4, kRoomsHeaderSize - 4))
        return Fail(error, file + "header bytes 0x4-0x1f are not zero");
    const uint32_t count = r.U32(0);
    if (count == 0 || !r.Has(kRoomsHeaderSize, uint64_t{ count } * kRoomEntrySize))
        return Fail(error, file + "entry table of " + std::to_string(count) + " rooms does not fit the file");
    const uint64_t tableEnd = kRoomsHeaderSize + uint64_t{ count } * kRoomEntrySize;
    std::vector<bool> seen(static_cast<size_t>(kDungeonCount) * kRoomIdLimit);
    for (uint32_t i = 0; i < count; i++) {
        const size_t entry = kRoomsHeaderSize + size_t{ i } * kRoomEntrySize;
        const std::string where = file + "room entry " + std::to_string(i) + ": ";
        const uint32_t size = r.U32(entry);
        const uint32_t offset = r.U32(entry + 0x04);
        const uint8_t dungeon = r.U8(entry + 0x08);
        const uint8_t room = r.U8(entry + 0x09);
        const uint16_t actorCount = r.U16(entry + 0x0a);
        const uint16_t actorOffset = r.U16(entry + 0x0c);
        const uint16_t objectCount = r.U16(entry + 0x0e);
        const uint16_t objectOffset = r.U16(entry + 0x10);
        if (!r.Zero(entry + 0x12, kRoomEntrySize - 0x12))
            return Fail(error, where + "reserved bytes 0x12-0x1f are not zero");
        if (dungeon >= kDungeonCount)
            return Fail(error, where + "dungeon " + std::to_string(dungeon) + " is not an MQ dungeon");
        if (room >= kRoomIdLimit)
            return Fail(error, where + "room " + std::to_string(room) + " is out of range");
        if (seen[size_t{ dungeon } * kRoomIdLimit + room])
            return Fail(error, where + "dungeon " + std::to_string(dungeon) + " room " + std::to_string(room) +
                                   " appears twice");
        seen[size_t{ dungeon } * kRoomIdLimit + room] = true;
        if (offset < tableEnd || !r.Has(offset, size))
            return Fail(error, where + "blob " + Hex(offset) + "+" + Hex(size) + " is outside the data area");
        if (size > kRoomBufferSize)
            return Fail(error, where + "blob size " + Hex(size) + " exceeds OoTMM's room buffer");
        if (actorCount > kRoomActorsMax || objectCount > kRoomObjectsMax)
            return Fail(error, where + std::to_string(actorCount) + " actors and " + std::to_string(objectCount) +
                                   " objects exceed OoTMM's room limits");
        if (uint64_t{ actorOffset } + uint64_t{ actorCount } * sizeof(ActorEntryPOD) > size ||
            uint64_t{ objectOffset } + uint64_t{ objectCount } * 2 > size)
            return Fail(error, where + "actor or object list runs past the blob");

        Room decoded;
        decoded.dungeon = dungeon;
        decoded.room = room;
        decoded.actors.reserve(actorCount);
        for (size_t a = 0; a < actorCount; a++) {
            const size_t at = offset + actorOffset + a * sizeof(ActorEntryPOD);
            decoded.actors.push_back({ r.S16(at), ReadVec3s(r, at + 0x02), ReadVec3s(r, at + 0x08), r.S16(at + 0x0e) });
        }
        decoded.objects.reserve(objectCount);
        for (size_t o = 0; o < objectCount; o++)
            decoded.objects.push_back(r.S16(offset + objectOffset + o * 2));
        std::vector<Room>& rooms = data.dungeons[dungeon].rooms;
        if (rooms.size() <= room)
            rooms.resize(size_t{ room } + 1);
        rooms[room] = std::move(decoded);
    }
    for (int dungeon = 0; dungeon < kDungeonCount; dungeon++) {
        const std::vector<Room>& rooms = data.dungeons[dungeon].rooms;
        if (rooms.empty())
            return Fail(error, file + "dungeon " + std::to_string(dungeon) + " has no rooms");
        for (size_t room = 0; room < rooms.size(); room++)
            if (!seen[size_t(dungeon) * kRoomIdLimit + room])
                return Fail(error,
                            file + "dungeon " + std::to_string(dungeon) + " is missing room " + std::to_string(room));
    }
    return true;
}

bool DecodeScenes(const Reader& r, Data& data, std::string& error) {
    const std::string file = "mq_scenes.bin: ";
    if (r.data == nullptr || !r.Has(0, kScenesHeaderSize))
        return Fail(error, file + "shorter than its " + Hex(kScenesHeaderSize) + "-byte header");
    if (!r.Zero(4, kScenesHeaderSize - 4))
        return Fail(error, file + "header bytes 0x4-0x3f are not zero");
    const uint32_t count = r.U32(0);
    if (count != kDungeonCount || !r.Has(kScenesHeaderSize, uint64_t{ count } * kSceneEntrySize))
        return Fail(error, file + "entry table of " + std::to_string(count) + " scenes does not fit the file or the " +
                               std::to_string(kDungeonCount) + " MQ dungeons");
    const uint64_t tableEnd = kScenesHeaderSize + uint64_t{ count } * kSceneEntrySize;
    bool seen[kDungeonCount] = {};
    for (uint32_t i = 0; i < count; i++) {
        const size_t entry = kScenesHeaderSize + size_t{ i } * kSceneEntrySize;
        const std::string where = file + "scene entry " + std::to_string(i) + ": ";
        const uint32_t size = r.U32(entry);
        const uint32_t offset = r.U32(entry + 0x04);
        const uint8_t dungeon = r.U8(entry + 0x08);
        const uint16_t transitionCount = r.U16(entry + 0x0a);
        const uint16_t transitionOffset = r.U16(entry + 0x0c);
        const uint16_t pathOffset = r.U16(entry + 0x0e);
        const uint32_t minimapSize = r.U32(entry + 0x10);
        const uint32_t minimapOffset = r.U32(entry + 0x14);
        const uint32_t polyCount = r.U32(entry + 0x18);
        const uint32_t polyOffset = r.U32(entry + 0x1c);
        const uint32_t surfaceCount = r.U32(entry + 0x20);
        const uint32_t surfaceOffset = r.U32(entry + 0x24);
        if (r.U8(entry + 0x09) != 0 || !r.Zero(entry + 0x28, kSceneEntrySize - 0x28))
            return Fail(error, where + "reserved bytes are not zero");
        if (dungeon >= kDungeonCount)
            return Fail(error, where + "dungeon " + std::to_string(dungeon) + " is not an MQ dungeon");
        if (seen[dungeon])
            return Fail(error, where + "dungeon " + std::to_string(dungeon) + " appears twice");
        seen[dungeon] = true;
        if (offset < tableEnd || !r.Has(offset, size))
            return Fail(error, where + "blob " + Hex(offset) + "+" + Hex(size) + " is outside the data area");
        if (size > kSceneBufferSize)
            return Fail(error, where + "blob size " + Hex(size) + " exceeds OoTMM's scene buffer");
        if (transitionCount > kSceneTransitionActorsMax ||
            uint64_t{ transitionOffset } + uint64_t{ transitionCount } * sizeof(TransitionActorPOD) > size)
            return Fail(error, where + std::to_string(transitionCount) + " transition actors at " +
                                   Hex(transitionOffset) + " do not fit the blob");

        Scene scene;
        scene.transitions.reserve(transitionCount);
        for (size_t t = 0; t < transitionCount; t++) {
            const size_t at = offset + transitionOffset + t * sizeof(TransitionActorPOD);
            TransitionActorPOD actor;
            actor.sides[0] = { r.S8(at), r.S8(at + 1) };
            actor.sides[1] = { r.S8(at + 2), r.S8(at + 3) };
            actor.id = r.S16(at + 0x04);
            actor.pos = ReadVec3s(r, at + 0x06);
            actor.rotY = r.S16(at + 0x0c);
            actor.params = r.S16(at + 0x0e);
            scene.transitions.push_back(actor);
        }

        const size_t pathCount = kPathCounts[dungeon];
        if (pathOffset == 0xffff) {
            if (pathCount != 0)
                return Fail(error, where + "has no paths, but make_mq.ts extracts " + std::to_string(pathCount));
        } else {
            if (pathCount == 0)
                return Fail(error,
                            where + "has paths, but make_mq.ts extracts none for dungeon " + std::to_string(dungeon));
            if (pathOffset % 4 != 0 || uint64_t{ pathOffset } + pathCount * kPathHeaderSize > size)
                return Fail(error, where + "path headers at " + Hex(pathOffset) + " do not fit the blob");
            scene.hasPaths = true;
            for (size_t p = 0; p < pathCount; p++) {
                const size_t header = offset + pathOffset + p * kPathHeaderSize;
                const uint8_t points = r.U8(header);
                const uint32_t pointsOffset = r.U32(header + 4);
                // OoTMM's relocation loop stops at a zero count, so a zero count would end the list early there.
                if (points == 0 || !r.Zero(header + 1, 3) ||
                    uint64_t{ pointsOffset } + uint64_t{ points } * sizeof(Vec3sPOD) > pathOffset)
                    return Fail(error, where + "path " + std::to_string(p) + " (" + std::to_string(points) +
                                           " points at " + Hex(pointsOffset) + ") is malformed");
                std::vector<Vec3sPOD> path;
                path.reserve(points);
                for (size_t k = 0; k < points; k++)
                    path.push_back(ReadVec3s(r, offset + pointsOffset + k * sizeof(Vec3sPOD)));
                scene.paths.push_back(std::move(path));
            }
        }

        if (dungeon < kMinimapDungeonCount) {
            if (minimapSize < sizeof(MinimapRoomPOD) || !r.Has(minimapOffset, minimapSize))
                return Fail(error, where + "minimap " + Hex(minimapOffset) + "+" + Hex(minimapSize) +
                                       " is empty or outside the file");
            const size_t rooms = minimapSize / sizeof(MinimapRoomPOD);
            scene.minimap.resize(rooms);
            for (size_t room = 0; room < rooms; room++) {
                for (size_t icon = 0; icon < 3; icon++) {
                    const size_t at = minimapOffset + room * sizeof(MinimapRoomPOD) + icon * sizeof(MinimapIconsPOD);
                    MinimapIconsPOD& icons = scene.minimap[room].icons[icon];
                    icons.markType = r.S8(at);
                    icons.count = r.U8(at + 1);
                    if (icons.markType < -1 || icons.markType > 1 || icons.count > kMinimapPointsMax)
                        return Fail(error, where + "minimap room " + std::to_string(room) + " record " +
                                               std::to_string(icon) + " has mark type " +
                                               std::to_string(icons.markType) + " and " + std::to_string(icons.count) +
                                               " points");
                    for (size_t point = 0; point < kMinimapPointsMax; point++)
                        icons.points[point] = { r.S8(at + 2 + point * 3), r.U8(at + 3 + point * 3),
                                                r.U8(at + 4 + point * 3) };
                }
            }
        } else if (minimapSize != 0) {
            return Fail(error, where + "has minimap data, but make_mq.ts extracts none for dungeon " +
                                   std::to_string(dungeon));
        }

        if (!r.Has(polyOffset, uint64_t{ polyCount } * 4))
            return Fail(error, where + std::to_string(polyCount) + " polygon patches at " + Hex(polyOffset) +
                                   " do not fit the file");
        scene.polyPatches.reserve(polyCount);
        for (size_t p = 0; p < polyCount; p++) {
            const size_t at = polyOffset + p * 4;
            const PolyPatch patch = { r.U16(at), r.U8(at + 2), r.U8(at + 3) };
            if (patch.flags > 7)
                return Fail(error, where + "polygon patch " + std::to_string(p) + " has flags " + Hex(patch.flags) +
                                       " beyond the 3 ignore bits");
            scene.polyPatches.push_back(patch);
        }
        if (!r.Has(surfaceOffset, uint64_t{ surfaceCount } * 0x10))
            return Fail(error, where + std::to_string(surfaceCount) + " surface patches at " + Hex(surfaceOffset) +
                                   " do not fit the file");
        scene.surfacePatches.reserve(surfaceCount);
        for (size_t p = 0; p < surfaceCount; p++) {
            const size_t at = surfaceOffset + p * 0x10;
            scene.surfacePatches.push_back({ r.U32(at), r.U32(at + 4), r.U32(at + 8) });
        }
        data.dungeons[dungeon].scene = std::move(scene);
    }
    return true;
}

bool DecodeMaps(const Reader& r, Data& data, std::string& error) {
    const std::string file = "mq_maps.bin: ";
    if (r.data == nullptr || r.size != kMapsSize)
        return Fail(error, file + "is " + Hex(r.size) + " bytes, not " + Hex(kMapsSize));
    for (size_t floor = 0; floor < kPauseFloorCount; floor++) {
        for (size_t record = 0; record < kPauseMarksPerFloor; record++) {
            const size_t at = floor * kPauseFloorSize + record * kPauseMarkSize;
            PauseMarkPOD& mark = data.pauseFloors[floor][record];
            mark.markType = r.S16(at);
            mark.unk04 = r.S32(at + 0x04);
            mark.hasVtx = r.U32(at + 0x08) != 0;
            mark.vtxCount = r.S32(at + 0x0c);
            mark.count = r.S32(at + 0x10);
            if (mark.markType < -1 || mark.markType > 1 || mark.count < 0 ||
                mark.count > static_cast<int32_t>(kPausePointsMax))
                return Fail(error, file + "floor " + std::to_string(floor) + " record " + std::to_string(record) +
                                       " has mark type " + std::to_string(mark.markType) + " and " +
                                       std::to_string(mark.count) + " points");
            for (size_t point = 0; point < kPausePointsMax; point++) {
                const size_t p = at + 0x14 + point * 0x0c;
                mark.points[point] = { r.S16(p), r.F32(p + 4), r.F32(p + 8) };
            }
        }
    }
    return true;
}

struct BundledState {
    Data data;
    std::string error;
    bool ok = false;
};

// Decoded once and never destroyed, so no static destructor at exit can see it freed.
const BundledState& GetBundledState() {
    static const BundledState* const state = [] {
        auto* decoded = new BundledState();
        decoded->ok = Decode(EmbeddedBins(), decoded->data, decoded->error);
        return decoded;
    }();
    return *state;
}

} // namespace

size_t Data::RoomCount() const {
    size_t count = 0;
    for (const Dungeon& dungeon : dungeons)
        count += dungeon.rooms.size();
    return count;
}

const Room* Data::FindRoom(int dungeon, int room) const {
    if (dungeon < 0 || dungeon >= kDungeonCount || room < 0)
        return nullptr;
    const std::vector<Room>& rooms = dungeons[dungeon].rooms;
    return static_cast<size_t>(room) < rooms.size() ? &rooms[room] : nullptr;
}

bool Decode(const Bins& bins, Data& out, std::string& error) {
    auto decoded = std::make_unique<Data>();
    if (!DecodeRooms({ bins.rooms, bins.roomsSize }, *decoded, error) ||
        !DecodeScenes({ bins.scenes, bins.scenesSize }, *decoded, error) ||
        !DecodeMaps({ bins.maps, bins.mapsSize }, *decoded, error))
        return false;
    out = std::move(*decoded);
    error.clear();
    return true;
}

Bins EmbeddedBins() {
    Bins bins;
    bins.rooms = Blobs::kMqRooms;
    bins.roomsSize = Blobs::kMqRoomsSize;
    bins.scenes = Blobs::kMqScenes;
    bins.scenesSize = Blobs::kMqScenesSize;
    bins.maps = Blobs::kMqMaps;
    bins.mapsSize = Blobs::kMqMapsSize;
    return bins;
}

const Data* Bundled() {
    const BundledState& state = GetBundledState();
    return state.ok ? &state.data : nullptr;
}

const std::string& BundledError() {
    return GetBundledState().error;
}

} // namespace ComboMq
