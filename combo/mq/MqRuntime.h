#pragma once
// OoTMM v32.3's Master Quest scene and room hooks (src/oot/mq.c:62-437) as pure C++17 over the decoded bins
// (ComboMqData.h). OoTMM rewrites the vanilla scene and room headers just before the engine runs them. SoH runs
// parsed resources that its resource manager caches and shares, so the soh glue
// (soh/soh/Enhancements/randomizer/ComboMqRuntime.cpp) asks this header which commands to replace and runs them on
// the buffers below, and never writes a cached resource. The templates take SoH's own structs in the glue and
// layout-equal ones in the host tests (ComboAndroid/tests/mq_runtime_plan_tests.cpp).
#include "ComboMqData.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ComboMq {

// The scene and room commands mq.c rewrites (SoH's SOH::SceneCommandID), and the id the glue gives a missing one.
enum SceneCommandId : uint8_t {
    kCmdActorList = 0x01,
    kCmdCollisionHeader = 0x03,
    kCmdObjectList = 0x0B,
    kCmdPathList = 0x0D,
    kCmdTransitionActorList = 0x0E,
    kCmdEndMarker = 0x14,
    kCmdAlternateHeaders = 0x18,
    kCmdMissing = 0xFF, // SOH::SceneCommandID::Error; OTRScene_ExecuteCommands skips a null command
};

// What runs in place of one vanilla command.
enum class CommandOverride : uint8_t {
    None,                           // the vanilla command, unchanged
    TransitionActors,               // 0x0E: the MQ transition actors (mq.c:324-327)
    Paths,                          // 0x0D: the MQ paths (mq.c:333-340)
    PathsInsteadOfAlternateHeaders, // Ice Cavern's 0x18 becomes its path list; no alternate header runs (mq.c:328-340)
    Collision,                      // 0x03: the collision with mq.c's patches (mq.c:341-344, 142-218)
    RoomActors,                     // 0x01: the MQ room's actors (mq.c:386-389)
    RoomObjects,                    // 0x0B: the MQ room's objects (mq.c:390-393)
};

// mqPatchCollisions changes something only in Ganon's Castle, whose polygons it hardcodes (mq.c:157-175), and in a
// scene whose entry carries patches, Fire Temple in the shipped data (mq.c:177-217). Every other scene's 0x03
// command runs unchanged.
inline bool HasCollisionPatches(int dungeon, const Scene& scene) {
    return dungeon == kGanonsCastle || !scene.polyPatches.empty() || !scene.surfacePatches.empty();
}

// loadMqSceneMaybe's switch over one command of the vanilla scene header (mq.c:319-345).
inline CommandOverride SceneCommandOverride(int dungeon, const Scene& scene, uint8_t command) {
    switch (command) {
        case kCmdTransitionActorList:
            return CommandOverride::TransitionActors;
        case kCmdAlternateHeaders:
            // Only Ice Cavern's alternate-header command falls through to the path case (mq.c:328-332), and like
            // a 0x0D it keeps its vanilla meaning when the entry has no paths (mq.c:334-335).
            if (dungeon != kIceCavern || !scene.hasPaths)
                return CommandOverride::None;
            return CommandOverride::PathsInsteadOfAlternateHeaders;
        case kCmdPathList:
            // pathOffset 0xffff keeps the vanilla paths (mq.c:334-335).
            return scene.hasPaths ? CommandOverride::Paths : CommandOverride::None;
        case kCmdCollisionHeader:
            return HasCollisionPatches(dungeon, scene) ? CommandOverride::Collision : CommandOverride::None;
        default:
            return CommandOverride::None;
    }
}

// loadMqRoomMaybe's switch over one command of the vanilla room header (mq.c:381-394).
constexpr CommandOverride RoomCommandOverride(uint8_t command) {
    return command == kCmdActorList    ? CommandOverride::RoomActors
           : command == kCmdObjectList ? CommandOverride::RoomObjects
                                       : CommandOverride::None;
}

// One scene or room load's overrides, by command index of its top-level header. mq.c patches the header that
// gSegments[2] or gSegments[3] points at (mq.c:316, 377), not the alternate headers it lists, so a plan answers
// only for that one resource; OTRScene_ExecuteCommands asks only at depth 1 (CommandDepth).
struct CommandPlan {
    const void* resource = nullptr; // the SOH::Scene this plan rewrites
    std::vector<CommandOverride> commands;

    CommandOverride At(const void* running, size_t index) const {
        if (running == nullptr || running != resource || index >= commands.size())
            return CommandOverride::None;
        return commands[index];
    }
};

// The header walks of loadMqSceneMaybe (mq.c:314-347) and loadMqRoomMaybe (mq.c:375-396): every command up to the
// end marker 0x14. The engine never runs a command after it, and mq.c never rewrites one.
template <class Override>
CommandPlan PlanCommands(const void* resource, const std::vector<uint8_t>& commands, Override overrideOf) {
    CommandPlan plan;
    plan.resource = resource;
    plan.commands.assign(commands.size(), CommandOverride::None);
    for (size_t i = 0; i < commands.size() && commands[i] != kCmdEndMarker; i++)
        plan.commands[i] = overrideOf(commands[i]);
    return plan;
}
inline CommandPlan PlanSceneCommands(const void* resource, int dungeon, const Scene& scene,
                                     const std::vector<uint8_t>& commands) {
    return PlanCommands(resource, commands,
                        [&](uint8_t command) { return SceneCommandOverride(dungeon, scene, command); });
}
inline CommandPlan PlanRoomCommands(const void* resource, const std::vector<uint8_t>& commands) {
    return PlanCommands(resource, commands, RoomCommandOverride);
}

// How deeply OTRScene_ExecuteCommands is nested: 1 for a scene or room header, 2 for an alternate header that
// Scene_CommandAlternateHeaderList runs from it. Only depth 1 is the header mq.c rewrites.
class CommandDepth {
  public:
    explicit CommandDepth(int& depth) : depth_(depth) {
        ++depth_;
    }
    ~CommandDepth() {
        --depth_;
    }
    CommandDepth(const CommandDepth&) = delete;
    CommandDepth& operator=(const CommandDepth&) = delete;
    bool IsTopLevel() const {
        return depth_ == 1;
    }

  private:
    int& depth_;
};

// kMqBrokenActors (mq.c:62-77): room actors OoTMM removes when CFG_NO_BROKEN_ACTORS is set (mq.c:398-410), which is
// its default (restoreBrokenActors off). Combo has no option to restore them, so they are always removed.
struct BrokenActor {
    uint8_t dungeon;
    uint8_t room;
    uint16_t actorIndex;
};
inline constexpr BrokenActor kMqBrokenActors[] = {
    { kDodongosCavern, 0x9, 0x0 }, { kDodongosCavern, 0xa, 0x6 }, { kJabuJabu, 0x7, 0x1 },
    { kSpiritTemple, 0x1, 0x2 },   { kSpiritTemple, 0x1, 0x3 },   { kSpiritTemple, 0x17, 0x0 },
};
// mq.c:407 writes 0xffff over the actor id; the spawn then refuses the negative id (OoTMM src/oot/actors.c:342,
// SoH's Actor_Spawn under COMBO_BUILD). The entry keeps its place, so later actors keep their list index.
inline constexpr int16_t kRemovedActorId = -1;

template <class Vec3sT> void CopyVec3s(const Vec3sPOD& in, Vec3sT& out) {
    out.x = in.x;
    out.y = in.y;
    out.z = in.z;
}
template <class ActorEntryT> void CopyActorEntry(const ActorEntryPOD& in, ActorEntryT& out) {
    out.id = in.id;
    CopyVec3s(in.pos, out.pos);
    CopyVec3s(in.rot, out.rot);
    out.params = in.params;
}
template <class TransitionEntryT> void CopyTransitionActor(const TransitionActorPOD& in, TransitionEntryT& out) {
    for (int side = 0; side < 2; side++) {
        out.sides[side].room = in.sides[side].room;
        out.sides[side].effects = in.sides[side].effects;
    }
    out.id = in.id;
    CopyVec3s(in.pos, out.pos);
    out.rotY = in.rotY;
    out.params = in.params;
}

// sMqBufferRoom and sMqBufferRoom2 (mq.c:79-91, 126-131): every room load swaps them, MQ room or not, and an MQ
// room's lists go into the current one. So the previous room's lists stay valid while the next room loads; the
// engine keeps two rooms during a transition. ActorEntryT is SoH's ActorEntry in the glue.
template <class ActorEntryT> class RoomBuffers {
  public:
    struct Buffer {
        ActorEntryT actors[kRoomActorsMax];
        int16_t objects[kRoomObjectsMax];
        uint8_t numActors;  // (u8)actorCount (mq.c:387)
        uint8_t numObjects; // (u8)objectCount (mq.c:391)
    };

    // swapMqRooms (mq.c:126-131), the first thing every room load does (mq.c:367).
    void Swap() {
        current_ ^= 1;
    }
    // The MQ room's lists into the current buffer (mq.c:373, the DMA of the room's blob), then the broken actors'
    // ids set to kRemovedActorId (mq.c:398-410).
    Buffer& Load(const Room& room) {
        Buffer& buffer = buffers_[current_];
        buffer.numActors = static_cast<uint8_t>(room.actors.size());
        buffer.numObjects = static_cast<uint8_t>(room.objects.size());
        for (size_t i = 0; i < room.actors.size() && i < kRoomActorsMax; i++)
            CopyActorEntry(room.actors[i], buffer.actors[i]);
        for (size_t i = 0; i < room.objects.size() && i < kRoomObjectsMax; i++)
            buffer.objects[i] = room.objects[i];
        for (const BrokenActor& broken : kMqBrokenActors)
            if (broken.dungeon == room.dungeon && broken.room == room.room && broken.actorIndex < room.actors.size())
                buffer.actors[broken.actorIndex].id = kRemovedActorId;
        return buffer;
    }
    Buffer& Current() {
        return buffers_[current_];
    }
    const Buffer& Previous() const {
        return buffers_[current_ ^ 1];
    }

  private:
    Buffer buffers_[2] = {};
    int current_ = 0; // sMqBufferRoomPtr; the other buffer is sMqBufferRoom2Ptr
};

// sMqBufferScene (mq.c:83-88): the transition actors and paths an MQ scene load copies out of mq_scenes.bin
// (mq.c:311). Every load refills it, so the ids Actor_SpawnTransitionActors negates start positive again. A refill
// with the same scene keeps its storage, so the lists it handed out stay at the same addresses; the glue keeps one
// buffer per dungeon. TransitionEntryT, PathT and Vec3sT are SoH's TransitionActorEntry, Path and Vec3s in the glue.
template <class TransitionEntryT, class PathT, class Vec3sT> class SceneBuffer {
  public:
    void Load(const Scene& scene) {
        transitionCount_ = static_cast<uint8_t>(scene.transitions.size()); // (u8) as mq.c:325
        for (size_t i = 0; i < scene.transitions.size() && i < kSceneTransitionActorsMax; i++)
            CopyTransitionActor(scene.transitions[i], transitions_[i]);
        // The relocation of mq.c:349-358 as native pointers. The decoder sized the list from make_mq.ts PATHS, so
        // Water's and Ice Cavern's, which have no zero count inside their entry, end at their last path; the list
        // then gets the zero-count entry mq.c's loop stops at (mq.c:353), which no path index reaches.
        const size_t count = scene.hasPaths ? scene.paths.size() : 0;
        points_.resize(count);
        paths_.resize(count == 0 ? 0 : count + 1);
        for (size_t p = 0; p < count; p++) {
            points_[p].resize(scene.paths[p].size());
            for (size_t k = 0; k < scene.paths[p].size(); k++)
                CopyVec3s(scene.paths[p][k], points_[p][k]);
            paths_[p].count = static_cast<uint8_t>(scene.paths[p].size());
            paths_[p].points = points_[p].data();
        }
        if (count != 0) {
            paths_.back().count = 0;
            paths_.back().points = nullptr;
        }
    }
    TransitionEntryT* Transitions() {
        return transitions_;
    }
    uint8_t TransitionCount() const {
        return transitionCount_;
    }
    // The path list, or nullptr for a scene without MQ paths; PathCount() paths, then the zero-count entry.
    PathT* Paths() {
        return paths_.empty() ? nullptr : paths_.data();
    }
    size_t PathCount() const {
        return paths_.empty() ? 0 : paths_.size() - 1;
    }

  private:
    TransitionEntryT transitions_[kSceneTransitionActorsMax] = {};
    uint8_t transitionCount_ = 0;
    std::vector<std::vector<Vec3sT>> points_;
    std::vector<PathT> paths_;
};

// mq.c:157-174: the Ganon's Castle polygons OoTMM sets to type 57 with flags 4 on every MQ Ganon's Castle load.
inline constexpr uint16_t kGanonPatchedPolys[] = { 2403, 2404, 2405, 2406, 2407, 2408, 2409, 2410,
                                                   2473, 2474, 2475, 2476, 2477, 2478, 2479, 2480 };
inline constexpr uint8_t kGanonPatchType = 57;
inline constexpr uint8_t kGanonPatchFlags = 4;

// PATCH_POLY (mq.c:133-136): the polygon's type, and the top 3 bits (the ignore flags) of flags_vIA.
template <class PolyT> void PatchPoly(PolyT& poly, uint8_t type, uint8_t flags) {
    poly.type = type;
    poly.flags_vIA = static_cast<uint16_t>((poly.flags_vIA & 0x1fff) | (flags << 13));
}
// PATCH_POLY_TYPE (mq.c:138-140): hi is the surface type's first word, which SoH keeps in data[0], and lo its
// second, data[1] (SoH's CollisionHeaderFactory reads the file's words the other way round).
template <class SurfaceT> void PatchSurface(SurfaceT& surface, uint32_t hi, uint32_t lo) {
    surface.data[0] = hi;
    surface.data[1] = lo;
}

template <class PolyT, class SurfaceT> struct PatchedCollision {
    std::vector<PolyT> polys;
    std::vector<SurfaceT> surfaces;
};

// mqPatchCollisions (mq.c:142-218) on copies in `out`; the inputs are only read. Ganon's Castle's hardcoded polygons
// first, then the entry's polygon patches and then its surface-type patches, each in file order, so a later patch
// wins as in mq.c. OoTMM writes in place without checking the ids; here every id is checked first, and when one is
// out of range nothing is patched, `out` is left as it was and `error` says why. Patching the same lists into the
// same `out` again keeps its storage, so a header pointing at it stays valid.
template <class PolyT, class SurfaceT>
bool PatchCollision(int dungeon, const Scene& scene, const PolyT* polys, size_t polyCount, const SurfaceT* surfaces,
                    size_t surfaceCount, PatchedCollision<PolyT, SurfaceT>& out, std::string& error) {
    const bool ganon = dungeon == kGanonsCastle;
    if (ganon)
        for (uint16_t id : kGanonPatchedPolys)
            if (id >= polyCount) {
                error = "Ganon's Castle polygon " + std::to_string(id) + " is beyond the " + std::to_string(polyCount) +
                        " polygons";
                return false;
            }
    for (const PolyPatch& patch : scene.polyPatches)
        if (patch.id >= polyCount) {
            error = "polygon patch " + std::to_string(patch.id) + " is beyond the " + std::to_string(polyCount) +
                    " polygons";
            return false;
        }
    for (const SurfacePatch& patch : scene.surfacePatches)
        if (patch.id >= surfaceCount) {
            error = "surface type patch " + std::to_string(patch.id) + " is beyond the " +
                    std::to_string(surfaceCount) + " surface types";
            return false;
        }
    if ((polyCount != 0 && polys == nullptr) || (surfaceCount != 0 && surfaces == nullptr)) {
        error = "the collision has no polygon or surface type list";
        return false;
    }

    out.polys.assign(polys, polys + polyCount);
    out.surfaces.assign(surfaces, surfaces + surfaceCount);
    if (ganon)
        for (uint16_t id : kGanonPatchedPolys)
            PatchPoly(out.polys[id], kGanonPatchType, kGanonPatchFlags);
    for (const PolyPatch& patch : scene.polyPatches)
        PatchPoly(out.polys[patch.id], patch.type, patch.flags);
    for (const SurfacePatch& patch : scene.surfacePatches)
        PatchSurface(out.surfaces[patch.id], patch.hi, patch.lo);
    error.clear();
    return true;
}

} // namespace ComboMq
