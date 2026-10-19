#pragma once
// ComboShip: OoTMM v32.3's Master Quest scene and room hooks (src/oot/mq.c:300-437) in SoH's command runner, for a
// scene that is Master Quest from the bundled data (ResourceMgr_SceneUsesBundledMq, combo/mq/MqMode.h). This is
// the PlayState glue; the plans, buffers and collision patcher are pure (combo/mq/MqRuntime.h). Every other scene
// runs its commands exactly as before.
#ifdef COMBO_BUILD
#include <cstddef>

struct PlayState;
namespace SOH {
class Scene;
} // namespace SOH

// Play_LoadScene_Hook (mq.c:426-437, loadMqSceneMaybe mq.c:300-359): z_play_otr.cpp's OTRPlay_InitScene calls it
// just before it runs the scene's commands. For a bundled MQ scene it copies the scene's transition actors and paths
// out of the bins, patches a copy of its collision (Fire Temple, Ganon's Castle) and plans which of `scene`'s
// top-level commands to replace; for any other scene it plans none.
void ComboMq_BeginScene(PlayState* play, SOH::Scene* scene);
// Play_LoadRoom_Hook (mq.c:413-424, loadMqRoomMaybe mq.c:361-411): z_scene_otr.cpp's OTRfunc_800973FC calls it just
// before it runs the room's commands. It always swaps the two room buffers; for a room of a bundled MQ scene it
// copies the MQ room's actors and objects into the current one and plans which of `room`'s commands to replace.
void ComboMq_BeginRoom(PlayState* play, SOH::Scene* room);
// Runs the planned replacement for command `index` of `resource` and returns true, or returns false when the
// command runs unchanged. OTRScene_ExecuteCommands calls it only for the top-level header (depth 1), which is
// the only one mq.c rewrites.
bool ComboMq_RunCommand(PlayState* play, SOH::Scene* resource, size_t index);
#endif
