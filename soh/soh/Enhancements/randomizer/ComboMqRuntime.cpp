#include "ComboMqRuntime.h"
#ifdef COMBO_BUILD
#include "mq/MqRoomLog.h"
#include "mq/MqRuntime.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/CollisionHeader.h"
#include "soh/resource/type/scenecommand/SetCollisionHeader.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "global.h"

// soh/soh/z_scene_otr.cpp: the bodies of the actor-list, object-list and transition-actor-list commands. The MQ
// lists run through them unchanged, since mq.c rewrites only a command's count and pointer (mq.c:324-327, 386-393).
void OTRScene_SetActorList(PlayState* play, ActorEntry* actors, u32 numActors);
void OTRScene_SetObjectList(PlayState* play, const s16* objects, size_t numObjects);
void OTRScene_SetTransitionActorList(PlayState* play, TransitionActorEntry* actors, u32 numActors);

namespace {

using RoomBuffers = ComboMq::RoomBuffers<ActorEntry>;
using SceneBuffer = ComboMq::SceneBuffer<TransitionActorEntry, Path, Vec3s>;

// A patched copy of a scene's collision (mq.c:341-344), never the cached resource, which vanilla visits and other
// seeds share. Only the polygon and surface-type lists are copied; the vertices, cameras and water boxes stay the
// resource's, which `source` keeps alive for as long as BgCheck may use this header.
struct CollisionCopy {
    std::shared_ptr<SOH::CollisionHeader> source;
    SOH::CollisionHeaderData header;
    ComboMq::PatchedCollision<SOH::CollisionPoly, SOH::SurfaceType> patched;
};

// What the scene and room loads leave behind, allocated once and never destroyed, like ComboMqData.cpp's decoded
// bins: no destructor at exit releases a cached resource after the resource manager is gone. Each dungeon has its
// own scene buffer and collision copies, refilled in place on every load of that dungeon, so the lists and header
// the PlayState points at keep their addresses for as long as the game runs, as the cached resources' do (a SoH
// save state restores those pointers from any scene). A copy whose collision resource was replaced is kept too.
struct Runtime {
    ComboMq::CommandPlan scenePlan;                   // the scene header being loaded and its overrides
    ComboMq::CommandPlan roomPlan;                    // the room header being loaded and its overrides
    int sceneDungeon = -1;                            // the planned scene's OoTMM dungeon
    SceneBuffer sceneBuffers[ComboMq::kDungeonCount]; // sMqBufferScene (mq.c:88)
    // The patched collision of each planned 0x03 command, by dungeon and command index.
    std::vector<std::unique_ptr<CollisionCopy>> collisions[ComboMq::kDungeonCount];
    // Copies of collision resources the cache has since replaced (an alternate-asset toggle reloads one). A save
    // state taken before may still point at their headers, so they are never freed.
    std::vector<std::unique_ptr<CollisionCopy>> replacedCollisions;
    RoomBuffers roomBuffers;             // sMqBufferRoom and sMqBufferRoom2 (mq.c:86-91)
    RoomBuffers::Buffer* room = nullptr; // the buffer holding the planned room's lists
    // For the developer room log only (ComboMq_LogRoom): the planned scene's collision could not be patched, and
    // the planned room's MQ objects went into the object bank.
    bool collisionFailed = false;
    bool roomObjectsSet = false;
};

Runtime& State() {
    static Runtime* const state = new Runtime();
    return *state;
}

std::vector<uint8_t> CommandIds(const SOH::Scene* resource) {
    std::vector<uint8_t> ids;
    ids.reserve(resource->commands.size());
    for (const auto& command : resource->commands)
        ids.push_back(command == nullptr ? ComboMq::kCmdMissing : static_cast<uint8_t>(command->cmdId));
    return ids;
}

// The bundled MQ data when the current scene plays as MQ from it: mqDungeonId and Config_IsMq
// (mq.c:229-233, 269-273), here the seed's own choice through the resolver (combo/mq/MqMode.h).
const ComboMq::Data* BundledScene(PlayState* play, int& dungeon) {
    dungeon = ComboMq::DungeonFromScene(play->sceneNum);
    if (dungeon < 0 || !ResourceMgr_SceneUsesBundledMq(play->sceneNum))
        return nullptr;
    return ComboMq::Bundled();
}

// mqPatchCollisions (mq.c:142-218) into `copy` from the collision that command `cmd` (a 0x03) loads. Returns false,
// with the reason logged, when it cannot be patched exactly; the scene then keeps its vanilla collision.
bool PatchCollisionCopy(std::unique_ptr<CollisionCopy>& copy, int dungeon, const ComboMq::Scene& scene,
                        SOH::ISceneCommand* cmd) {
    auto* setCollision = static_cast<SOH::SetCollisionHeader*>(cmd);
    const std::shared_ptr<SOH::CollisionHeader> source =
        setCollision != nullptr ? setCollision->collisionHeader : nullptr;
    std::string error;
    if (source == nullptr) {
        error = "the scene's collision did not load";
    } else if (source->collisionHeaderData.polyList != source->polygons.data() ||
               source->collisionHeaderData.surfaceTypeList != source->surfaceTypes.data()) {
        // Only the resource's own lists have a known length.
        error = "the scene's collision lists were replaced";
    } else {
        // The same collision is patched into the same copy again, which keeps its storage. Another collision gets a
        // new copy, and the patched copy it replaces is kept (replacedCollisions).
        if (copy != nullptr && copy->source != nullptr && copy->source != source)
            State().replacedCollisions.push_back(std::move(copy));
        if (copy == nullptr)
            copy = std::make_unique<CollisionCopy>();
        if (ComboMq::PatchCollision(dungeon, scene, source->polygons.data(), source->polygons.size(),
                                    source->surfaceTypes.data(), source->surfaceTypes.size(), copy->patched, error)) {
            copy->source = source;
            copy->header = source->collisionHeaderData;
            copy->header.polyList = copy->patched.polys.data();
            copy->header.surfaceTypeList = copy->patched.surfaces.data();
            return true;
        }
    }
    SPDLOG_ERROR("[ComboShip] Master Quest collision of OoTMM dungeon {} left vanilla: {}", dungeon, error);
    return false;
}

// How many entries of the patched copy differ from the resource's, for the room log.
template <class T> size_t ChangedEntries(const std::vector<T>& patched, const std::vector<T>& source) {
    size_t changed = 0;
    for (size_t i = 0; i < patched.size() && i < source.size(); i++)
        changed += std::memcmp(&patched[i], &source[i], sizeof(T)) != 0;
    return changed;
}

} // namespace

void ComboMq_BeginScene(PlayState* play, SOH::Scene* scene) {
    Runtime& state = State();
    state.scenePlan = {};
    state.roomPlan = {};
    state.sceneDungeon = -1;
    state.collisionFailed = false;
    int dungeon;
    const ComboMq::Data* data = BundledScene(play, dungeon);
    if (data == nullptr || scene == nullptr)
        return;
    const ComboMq::Scene& mq = data->dungeons[dungeon].scene;

    // mq.c:311: the scene's lists, fresh for this load.
    SceneBuffer& buffer = state.sceneBuffers[dungeon];
    buffer.Load(mq);
    // mq.c:314-347: which top-level commands to replace; the collision is patched before any command runs.
    ComboMq::CommandPlan plan = ComboMq::PlanSceneCommands(scene, dungeon, mq, CommandIds(scene));
    std::vector<std::unique_ptr<CollisionCopy>>& collisions = state.collisions[dungeon];
    if (collisions.size() < plan.commands.size())
        collisions.resize(plan.commands.size());
    for (size_t i = 0; i < plan.commands.size(); i++)
        if (plan.commands[i] == ComboMq::CommandOverride::Collision &&
            !PatchCollisionCopy(collisions[i], dungeon, mq, scene->commands[i].get())) {
            plan.commands[i] = ComboMq::CommandOverride::None;
            state.collisionFailed = true;
        }
    state.scenePlan = std::move(plan);
    state.sceneDungeon = dungeon;
    SPDLOG_INFO("[ComboShip] Master Quest scene {0:#x} (OoTMM dungeon {1}): {2} transition actors, {3} paths",
                play->sceneNum, dungeon, buffer.TransitionCount(), buffer.PathCount());
}

void ComboMq_BeginRoom(PlayState* play, SOH::Scene* room) {
    Runtime& state = State();
    // mq.c:367: every room load swaps the buffers, before it looks for an MQ room.
    state.roomBuffers.Swap();
    state.roomPlan = {};
    state.room = nullptr;
    state.roomObjectsSet = false;
    int dungeon;
    const ComboMq::Data* data = BundledScene(play, dungeon);
    if (data == nullptr || room == nullptr)
        return;
    // findMqOverrideRoom matches the dungeon and play->roomCtx.curRoom.num (mq.c:283).
    const ComboMq::Room* mq = data->FindRoom(dungeon, play->roomCtx.curRoom.num);
    if (mq == nullptr)
        return;
    state.room = &state.roomBuffers.Load(*mq);
    state.roomPlan = ComboMq::PlanRoomCommands(room, CommandIds(room));
    SPDLOG_INFO("[ComboShip] Master Quest room {0} of scene {1:#x}: {2} actors, {3} objects", play->roomCtx.curRoom.num,
                play->sceneNum, state.room->numActors, state.room->numObjects);
}

bool ComboMq_RunCommand(PlayState* play, SOH::Scene* resource, size_t index) {
    Runtime& state = State();
    ComboMq::CommandOverride action = state.scenePlan.At(resource, index);
    if (action == ComboMq::CommandOverride::None)
        action = state.roomPlan.At(resource, index);
    switch (action) {
        case ComboMq::CommandOverride::TransitionActors: {
            SceneBuffer& buffer = state.sceneBuffers[state.sceneDungeon];
            OTRScene_SetTransitionActorList(play, buffer.Transitions(), buffer.TransitionCount());
            return true;
        }
        case ComboMq::CommandOverride::Paths:
        case ComboMq::CommandOverride::PathsInsteadOfAlternateHeaders:
            // The Ice Cavern case returns here like a 0x0D: its alternate header never runs (mq.c:328-340).
            play->setupPathList = state.sceneBuffers[state.sceneDungeon].Paths();
            return true;
        case ComboMq::CommandOverride::Collision:
            BgCheck_Allocate(&play->colCtx, play,
                             (CollisionHeader*)&state.collisions[state.sceneDungeon][index]->header);
            return true;
        case ComboMq::CommandOverride::RoomActors:
            OTRScene_SetActorList(play, state.room->actors, state.room->numActors);
            return true;
        case ComboMq::CommandOverride::RoomObjects:
            OTRScene_SetObjectList(play, state.room->objects, state.room->numObjects);
            state.roomObjectsSet = true;
            return true;
        default:
            return false;
    }
}

void ComboMq_LogRoom(PlayState* play) {
    if (!CVarGetInteger(ComboMq::kRoomLogCVar, 0))
        return;
    int dungeon;
    const bool mq = BundledScene(play, dungeon) != nullptr;
    if (dungeon < 0)
        return;
    Runtime& state = State();
    ComboMq::RoomLog log;
    log.scene = play->sceneNum;
    log.room = play->roomCtx.curRoom.num;
    log.dungeon = dungeon;
    log.mq = mq;
    log.layer = gSaveContext.sceneLayer;
    // Read back from the PlayState, so the line shows what the room's commands left, MQ or not. The ten MQ rooms the
    // bins give no actors have no 0x01 in their vanilla header for mq.c to rewrite (mq.c:386-389): they spawn none,
    // which is the MQ room's own empty list, so they log mq:0 too.
    const bool actorCommand = std::find(state.roomPlan.commands.begin(), state.roomPlan.commands.end(),
                                        ComboMq::CommandOverride::RoomActors) != state.roomPlan.commands.end();
    log.actors = state.room != nullptr &&
                         (play->setupActorList == state.room->actors || (state.room->numActors == 0 && !actorCommand))
                     ? ComboMq::LogSource::Mq
                     : ComboMq::LogSource::Vanilla;
    for (int i = 0; play->setupActorList != nullptr && i < play->numSetupActors; i++)
        log.actorList.push_back({ play->setupActorList[i].id, static_cast<uint16_t>(play->setupActorList[i].params) });
    log.objects = state.roomObjectsSet ? ComboMq::LogSource::Mq : ComboMq::LogSource::Vanilla;
    // The bank holds -id for an object until it has loaded (OTRfunc_800982FC).
    for (int i = play->objectCtx.unk_09; i < play->objectCtx.num && i < OBJECT_EXCHANGE_BANK_MAX; i++)
        log.objectList.push_back(static_cast<int16_t>(ABS(play->objectCtx.status[i].id)));
    SceneBuffer& buffer = state.sceneBuffers[dungeon];
    log.transitions =
        play->transiActorCtx.list == buffer.Transitions() ? ComboMq::LogSource::Mq : ComboMq::LogSource::Vanilla;
    log.transitionCount = play->transiActorCtx.numActors;
    if (buffer.Paths() != nullptr && play->setupPathList == buffer.Paths()) {
        log.paths = ComboMq::LogSource::Mq;
        log.pathCount = buffer.PathCount();
    }
    log.collision = state.collisionFailed ? ComboMq::LogCollision::Failed : ComboMq::LogCollision::Vanilla;
    for (const std::unique_ptr<CollisionCopy>& copy : state.collisions[dungeon])
        if (copy != nullptr && copy->source != nullptr && (const void*)play->colCtx.colHeader == &copy->header) {
            log.collision = ComboMq::LogCollision::Patched;
            log.changedPolys = ChangedEntries(copy->patched.polys, copy->source->polygons);
            log.changedSurfaces = ChangedEntries(copy->patched.surfaces, copy->source->surfaceTypes);
        }
    SPDLOG_INFO("{}", ComboMq::FormatRoomLog(log));
}
#endif
