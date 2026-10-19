"""OoTMM's Master Quest scene and room hooks wired into SoH's command runner (MQ port unit U4).

OoTMM rewrites the scene header just before Play_InitScene runs it and the room header just before the room-load
routine runs it (src/oot/mq.c:413-437), and only those top-level headers: an alternate header they list keeps its
vanilla lists. The production OTRPlay_InitScene (z_play_otr.cpp), OTRfunc_800973FC, OTRScene_ExecuteCommands,
Scene_CommandAlternateHeaderList, the collision and path commands and the actor-list, object-list and
transition-actor-list bodies (z_scene_otr.cpp) are extracted verbatim and run on engine stubs: stub headers that
hold SoH's own list, path and collision structs, copied out of its headers. Two programs run them.

The first records ComboMq_BeginScene, ComboMq_BeginRoom and ComboMq_RunCommand. Checks:
- the scene and room hooks run just before their commands, for the resource those commands come from;
- ComboMq_RunCommand is asked only at depth 1, for every command up to the end marker, and a command it runs does
  not also run its vanilla handler; an alternate header's commands run vanilla, and Ice Cavern's 0x18 replaced by
  its paths never runs its alternate header (mq.c:328-340);
- the refactored list commands behave exactly as the handlers they replaced, transcribed below.

The second links the real ComboMqRuntime.cpp, the decoder and the embedded bins, and loads Fire Temple: the MQ scene,
rooms A, B and A, the same scene and room vanilla, the MQ scene and room A again, then a replaced and an unpatchable
collision, Ice Cavern's alternate headers and a scene outside the dungeons. Checks:
- an MQ scene's 0x0E and 0x0D set the bins' transition actors and paths, its 0x03 hands BgCheck a copied header with
  mq.c's patches, and the resource's own lists stay untouched (mq.c:142-218, 300-359);
- an MQ room's 0x01 and 0x0B set the bins' lists through the vanilla bodies, in buffers swapped on every room load,
  MQ or not, so the previous room's lists stay intact (mq.c:79-91, 361-411);
- a vanilla load of the same resources, and any scene outside the 12 dungeons, sets the resources' own lists;
- a reload refills the scene lists and collision at the same addresses; a copy whose collision resource is replaced
  is kept with its resource, so a save state's pointer into it stays valid; a collision that cannot be patched stays
  vanilla with the reason logged.

Also scanned:
- Actor_Spawn refuses a negative id before ActorDB_Retrieve under COMBO_BUILD (mq.c:398-410, actors.c:342);
- the MQ runtime never writes a cached resource: no GetRawPointer, no const_cast, no write through the collision
  resource.
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--blobs', type=Path, required=True, help='CMake\'s generated combo_mq dir (ComboMqBlobs.h/.cpp)')
args = parser.parse_args()
source, work, blobs = args.source.resolve(), args.build_dir.resolve(), args.blobs.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def function(text, signature):
    """The definition, not a declaration, that starts with `signature`, up to its closing brace."""
    start = hostcxx.find(text, signature)
    while True:
        at = text.index('(', start) + 1
        depth = 1
        while depth:
            depth += (text[at] == '(') - (text[at] == ')')
            at += 1
        if text[at:].lstrip().startswith('{'):
            break
        start = hostcxx.find(text, signature, at)
    brace = text.index('{', at)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def without_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


# Without comments: a commented-out line in OTRfunc_800973FC opens a brace.
scene_otr = without_comments((source / 'soh/soh/z_scene_otr.cpp').read_text(encoding='utf-8'))
play_otr = without_comments((source / 'soh/soh/z_play_otr.cpp').read_text(encoding='utf-8'))
depth_declaration = 'static int sCommandDepth = 0;'
check(depth_declaration in scene_otr, 'z_scene_otr.cpp: the command depth counter is missing')
extracted = ''.join([
    depth_declaration + '\n',
    's32 OTRScene_ExecuteCommands(PlayState* play, SOH::Scene* scene);\n',
    function(scene_otr, 'void OTRScene_SetActorList('),
    function(scene_otr, 'bool Scene_CommandCollisionHeader('),
    function(scene_otr, 'bool OTRfunc_800982FC('),
    function(scene_otr, 'void OTRScene_SetObjectList('),
    function(scene_otr, 'bool Scene_CommandPathList('),
    function(scene_otr, 'void OTRScene_SetTransitionActorList('),
    function(scene_otr, 'bool Scene_CommandActorList('),
    function(scene_otr, 'bool Scene_CommandObjectList('),
    function(scene_otr, 'bool Scene_CommandTransitionActorList('),
    function(scene_otr, 'bool Scene_CommandAlternateHeaderList('),
    'bool (*sceneCommands[0x1A])(PlayState*, SOH::ISceneCommand*);\n',
    function(scene_otr, 's32 OTRScene_ExecuteCommands('),
    function(scene_otr, 'extern "C" s32 OTRfunc_800973FC('),
    function(play_otr, 'void OTRPlay_InitScene('),
])


def typedef_struct(path, name):
    """SoH's own `typedef struct { ... } name;` in `path`, so a stub keeps its exact layout."""
    text = (source / path).read_text(encoding='utf-8')
    end = hostcxx.find(text, '} ' + name + ';')
    return text[text.rindex('typedef struct', 0, end):text.index(';', end) + 1] + '\n'


# Stub headers for the extracted functions and for the real ComboMqRuntime.cpp, which finds them through its own
# includes. The list, path and collision structs are SoH's.
stubs = work / 'stubs'
stub_headers = {
    'z64math.h': ''.join([
        '#pragma once\n#include <cstddef>\n#include <cstdint>\n',
        'typedef int8_t s8;\ntypedef uint8_t u8;\ntypedef int16_t s16;\ntypedef uint16_t u16;\n',
        'typedef int32_t s32;\ntypedef uint32_t u32;\n',
        typedef_struct('soh/include/z64math.h', 'Vec3s'),
    ]),
    'global.h': ''.join([
        '#pragma once\n#include "z64math.h"\n',
        r'''#define osSyncPrintf(...) ((void)0)
#define VT_FGCOL(color) ""
#define VT_RST ""
#define VIRTUAL_TO_PHYSICAL(addr) ((uintptr_t)(addr))
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define OBJECT_INVALID 0
#define OBJECT_EXCHANGE_BANK_MAX 128
#define GET_PLAYER(play) (&(play)->player)
#define YREG(n) gYReg[n]
''',
        typedef_struct('soh/include/z64.h', 'ActorEntry'),
        typedef_struct('soh/include/z64.h', 'TransitionActorEntry'),
        typedef_struct('soh/include/z64scene.h', 'Path'),
        *(typedef_struct('soh/include/z64bgcheck.h', name)
          for name in ('CollisionPoly', 'CamData', 'WaterBox', 'SurfaceType', 'CollisionHeader')),
        r'''struct CollisionContext { CollisionHeader* colHeader; };
struct ObjectStatus { s16 id; void* segment; };
struct ObjectContext { u8 num; u8 unk_09; ObjectStatus status[OBJECT_EXCHANGE_BANK_MAX]; };
struct Room { s8 num; void* segment; };
struct RoomContext { Room curRoom; Room prevRoom; s8 status; void* unk_34; void* roomToLoad; };
struct TransitionActorContext { u8 numActors; TransitionActorEntry* list; };
struct Player { int unused; };
struct ActorContext { int unused; };
struct PlayState {
    s16 sceneNum = 0x09;
    s32 curSpawn = 0;
    void* linkActorEntry = nullptr; void* unk_11DFC = nullptr; void* setupEntranceList = nullptr;
    void* setupExitList = nullptr; void* cUpElfMsgs = nullptr; Path* setupPathList = nullptr;
    u8 numSetupActors = 0; ActorEntry* setupActorList = nullptr;
    ObjectContext objectCtx{}; int lightCtx = 0; int state = 0; TransitionActorContext transiActorCtx{};
    RoomContext roomCtx{}; s16 skyboxId = 0; void* sceneSegment = nullptr; Player player{}; ActorContext actorCtx{};
    CollisionContext colCtx{};
};
struct SaveContext { s32 sceneLayer = 0; s32 worldMapArea = 0; };
extern SaveContext gSaveContext;
extern uintptr_t gSegments[16];
extern int gYReg[32];
extern "C" void BgCheck_Allocate(CollisionContext* colCtx, PlayState* play, CollisionHeader* colHeader);
''']),
    'soh/ResourceManagerHelpers.h': '#pragma once\n#include "z64math.h"\n'
                                    'extern "C" uint32_t ResourceMgr_SceneUsesBundledMq(s16 sceneNum);\n',
    'soh/resource/type/Scene.h': r'''#pragma once
#include <cstdint>
#include <memory>
#include <vector>
namespace SOH {
enum class SceneCommandID : uint8_t { EndMarker = 0x14 };
class ISceneCommand {
  public:
    virtual ~ISceneCommand() = default;
    SceneCommandID cmdId{};
    virtual void* GetRawPointer() { return nullptr; }
};
class Scene {
  public:
    std::vector<std::shared_ptr<ISceneCommand>> commands;
};
} // namespace SOH
''',
    'soh/resource/type/CollisionHeader.h': ''.join([
        '#pragma once\n#include <vector>\n#include "z64math.h"\nnamespace SOH {\n',
        *(typedef_struct('soh/soh/resource/type/CollisionHeader.h', name)
          for name in ('CollisionPoly', 'WaterBox', 'CamData', 'SurfaceType', 'CollisionHeaderData')),
        r'''class CollisionHeader {
  public:
    CollisionHeaderData* GetPointer() { return &collisionHeaderData; }
    CollisionHeaderData collisionHeaderData;
    std::vector<Vec3s> vertices;
    std::vector<CollisionPoly> polygons;
    std::vector<SurfaceType> surfaceTypes;
    std::vector<CamData> camData;
    std::vector<WaterBox> waterBoxes;
};
} // namespace SOH
''']),
    'soh/resource/type/scenecommand/SetCollisionHeader.h': r'''#pragma once
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/CollisionHeader.h"
namespace SOH {
class SetCollisionHeader : public ISceneCommand {
  public:
    CollisionHeaderData* GetPointer() { return collisionHeader == nullptr ? nullptr : collisionHeader->GetPointer(); }
    void* GetRawPointer() override { return GetPointer(); }
    std::shared_ptr<CollisionHeader> collisionHeader;
};
} // namespace SOH
''',
    # The log macros still compile their arguments; the harness counts the errors.
    'spdlog/spdlog.h': r'''#pragma once
extern int gSpdlogErrors;
template <class... Args> void SpdlogStub(bool error, const char*, const Args&...) { gSpdlogErrors += error; }
#define SPDLOG_INFO(...) SpdlogStub(false, __VA_ARGS__)
#define SPDLOG_ERROR(...) SpdlogStub(true, __VA_ARGS__)
''',
}
for name, text in stub_headers.items():
    (stubs / name).parent.mkdir(parents=True, exist_ok=True)
    (stubs / name).write_text(text, encoding='utf-8')
# Every engine header ComboMqRuntime.cpp includes is a stub here, never one of soh's own.
runtime_text = (source / 'soh/soh/Enhancements/randomizer/ComboMqRuntime.cpp').read_text(encoding='utf-8')
for include in re.findall(r'#include [<"]([^>"]+)[>"]', runtime_text):
    check(include in stub_headers or include in ('ComboMqRuntime.h', 'mq/MqRuntime.h') or '.' not in include,
          f'ComboMqRuntime.cpp includes {include}, which the glue harness has no stub for')

# Both programs: the stubs, the scene commands the extracted functions cast to, and the engine calls they make.
prelude = r'''
#include "mq/MqRuntime.h"
#include "global.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetCollisionHeader.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace SOH {
''' + typedef_struct('soh/soh/resource/type/Path.h', 'PathData') + r'''struct SetAlternateHeaders : ISceneCommand { std::vector<std::shared_ptr<Scene>> headers; };
struct SetActorList : ISceneCommand {
    uint32_t numActors = 0;
    std::vector<ActorEntry> actorList;
    void* GetRawPointer() override { return actorList.data(); }
};
struct SetObjectList : ISceneCommand {
    uint32_t numObjects = 0;
    std::vector<int16_t> objects;
    void* GetRawPointer() override { return objects.data(); }
};
struct SetTransitionActorList : ISceneCommand {
    uint32_t numTransitionActors = 0;
    std::vector<TransitionActorEntry> transitionActorList;
    void* GetRawPointer() override { return transitionActorList.data(); }
};
struct SetPathways : ISceneCommand {
    std::vector<PathData*> paths;
    PathData** GetPointer() { return paths.data(); }
    void* GetRawPointer() override { return GetPointer(); }
};
} // namespace SOH
// The casts between SoH's resource structs and the engine's that z_scene_otr.cpp and ComboMqRuntime.cpp make.
static_assert(sizeof(CollisionHeader) == sizeof(SOH::CollisionHeaderData) &&
                  offsetof(CollisionHeader, polyList) == offsetof(SOH::CollisionHeaderData, polyList) &&
                  offsetof(CollisionHeader, surfaceTypeList) == offsetof(SOH::CollisionHeaderData, surfaceTypeList) &&
                  sizeof(CollisionPoly) == sizeof(SOH::CollisionPoly) && sizeof(SurfaceType) == sizeof(SOH::SurfaceType),
              "the engine's CollisionHeader is the resource's CollisionHeaderData");
static_assert(sizeof(Path) == sizeof(SOH::PathData) && offsetof(Path, points) == offsetof(SOH::PathData, points),
              "the engine's Path is the resource's PathData");
SaveContext gSaveContext;
uintptr_t gSegments[16];
int gYReg[32];

// The call log.
struct Event { std::string what; const void* resource; size_t index; int depth; };
std::vector<Event> events;
int sCommandDepthAtCall();
void Log(const std::string& what, const void* resource = nullptr, size_t index = 0) {
    events.push_back({ what, resource, index, sCommandDepthAtCall() });
}
int killed = 0;
void Actor_KillAllWithMissingObject(PlayState*, ActorContext*) { killed++; }
void Combo_OotSoulResetRoom(void*) { Log("SoulReset"); }
void Combo_SilverResetRoom() { Log("SilverReset"); }
void Object_InitBank(PlayState*, ObjectContext*) { Log("Object_InitBank"); }
void LightContext_Init(PlayState*, int*) {}
void TransitionActor_InitContext(int*, TransitionActorContext*) {}
void func_80096FD4(PlayState*, Room*) {}
void GameInteractor_ExecuteAfterSceneCommands(s16) { Log("AfterSceneCommands"); }
void Play_InitEnvironment(PlayState*, s16) { Log("Play_InitEnvironment"); }
void Player_SetBootData(PlayState*, Player*) { Log("Player_SetBootData"); }
void Actor_SpawnTransitionActors(PlayState*, ActorContext*) { Log("Actor_SpawnTransitionActors"); }
extern "C" void BgCheck_Allocate(CollisionContext* colCtx, PlayState*, CollisionHeader* colHeader) {
    colCtx->colHeader = colHeader;
}
bool Vanilla(PlayState*, SOH::ISceneCommand* cmd) {
    Log("Vanilla", cmd, static_cast<size_t>(cmd->cmdId));
    return false;
}
'''
# Both programs, after the extracted functions.
common = r'''
int sCommandDepthAtCall() { return sCommandDepth; }

std::string Describe() {
    std::string text;
    for (const Event& e : events)
        text += e.what + "(" + std::to_string(e.index) + "@" + std::to_string(e.depth) + ") ";
    return text;
}
void Expect(bool condition, const std::string& what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n  events: %s\n", what.c_str(), Describe().c_str());
        std::exit(1);
    }
}
'''

# The first program records the hooks.
code = prelude + r'''
// The plan the recorded ComboMq_RunCommand answers from: (resource, index) pairs it replaces.
std::vector<std::pair<const void*, size_t>> planned;
void ComboMq_BeginScene(PlayState*, SOH::Scene* scene) { Log("BeginScene", scene); }
void ComboMq_BeginRoom(PlayState*, SOH::Scene* room) { Log("BeginRoom", room); }
bool ComboMq_RunCommand(PlayState*, SOH::Scene* resource, size_t index) {
    Log("RunCommand", resource, index);
    for (auto& entry : planned)
        if (entry.first == resource && entry.second == index)
            return true;
    return false;
}
'''
code += extracted
code += common
code += r'''
// The handlers as they were before the MQ port, transcribed from z_scene_otr.cpp.
bool OriginalActorList(PlayState* play, SOH::ISceneCommand* cmd) {
    SOH::SetActorList* cmdActor = (SOH::SetActorList*)cmd;
    Combo_OotSoulResetRoom(play);
    Combo_SilverResetRoom();
    play->numSetupActors = cmdActor->numActors;
    play->setupActorList = (ActorEntry*)cmdActor->GetRawPointer();
    return false;
}
bool OriginalObjectList(PlayState* play, SOH::ISceneCommand* cmd) {
    SOH::SetObjectList* cmdObj = (SOH::SetObjectList*)cmd;
    s32 i;
    s32 j;
    s32 k;
    s16* objectEntry = (s16*)cmdObj->GetRawPointer();
    (void)objectEntry;
    k = 0;
    i = play->objectCtx.unk_09;
    for (i = play->objectCtx.unk_09, k = 0; i < play->objectCtx.num; i++, k++) {
        if (k >= cmdObj->objects.size() || play->objectCtx.status[i].id != cmdObj->objects[k]) {
            for (j = i; j < play->objectCtx.num; j++) {
                play->objectCtx.status[j].id = OBJECT_INVALID;
            }
            Actor_KillAllWithMissingObject(play, &play->actorCtx);
            break;
        }
    }
    for (; k < cmdObj->objects.size(); k++, i++) {
        if (i < OBJECT_EXCHANGE_BANK_MAX - 1) {
            OTRfunc_800982FC(&play->objectCtx, i, cmdObj->objects[k]);
        }
    }
    play->objectCtx.num = i;
    return false;
}
bool OriginalTransitionActorList(PlayState* play, SOH::ISceneCommand* cmd) {
    SOH::SetTransitionActorList* cmdActor = (SOH::SetTransitionActorList*)cmd;
    play->transiActorCtx.numActors = cmdActor->numTransitionActors;
    play->transiActorCtx.list = (TransitionActorEntry*)cmdActor->GetRawPointer();
    for (s32 i = 0; i < play->transiActorCtx.numActors; i++) {
        play->transiActorCtx.list[i].id = ABS(play->transiActorCtx.list[i].id);
    }
    return false;
}

std::shared_ptr<SOH::ISceneCommand> Command(int id) {
    auto cmd = std::make_shared<SOH::ISceneCommand>();
    cmd->cmdId = static_cast<SOH::SceneCommandID>(id);
    return cmd;
}
std::shared_ptr<SOH::Scene> Header(std::vector<int> ids, std::shared_ptr<SOH::Scene> layer4 = nullptr) {
    auto scene = std::make_shared<SOH::Scene>();
    for (int id : ids) {
        if (id == 0x18) {
            auto alt = std::make_shared<SOH::SetAlternateHeaders>();
            alt->cmdId = static_cast<SOH::SceneCommandID>(0x18);
            alt->headers = { nullptr, nullptr, nullptr, layer4 };
            scene->commands.push_back(alt);
        } else {
            scene->commands.push_back(Command(id));
        }
    }
    return scene;
}

// The commands RunCommand was asked about, as (resource, index), and the vanilla handlers that ran, as command ids.
std::vector<std::pair<const void*, size_t>> Asked() {
    std::vector<std::pair<const void*, size_t>> asked;
    for (const Event& e : events)
        if (e.what == "RunCommand") {
            Expect(e.depth == 1, "RunCommand asked below the top-level header");
            asked.push_back({ e.resource, e.index });
        }
    return asked;
}
std::vector<std::pair<const void*, size_t>> Ran() {
    std::vector<std::pair<const void*, size_t>> ran;
    for (const Event& e : events)
        if (e.what == "Vanilla")
            ran.push_back({ e.resource, e.index });
    return ran;
}
size_t At(const char* what) {
    for (size_t i = 0; i < events.size(); i++)
        if (events[i].what == what)
            return i;
    Expect(false, what);
    return 0;
}

void CheckScene() {
    for (auto& handler : sceneCommands)
        handler = Vanilla;
    sceneCommands[0x18] = Scene_CommandAlternateHeaderList;
    // A layer-4 alternate header, as Jabu-Jabu's and Ice Cavern's, and a top-level header that lists it first.
    auto alt = Header({ 0x0E, 0x01, 0x14 });
    auto top = Header({ 0x18, 0x0E, 0x01, 0x0D, 0x14, 0x0E }, alt);
    const void* T = top.get();
    const void* A = alt.get();
    const void* topCmd[6];
    for (int i = 0; i < 6; i++)
        topCmd[i] = top->commands[i].get();
    // A plan that also names the alternate header's commands: they must never be asked.
    planned = { { T, 1 }, { T, 2 }, { A, 0 }, { A, 1 } };
    PlayState play;
    play.sceneSegment = top.get();

    // Normal play (layer 0): the scene hook, then every top-level command up to the end marker; the planned ones
    // replace their vanilla handlers.
    events.clear();
    gSaveContext.sceneLayer = 0;
    OTRPlay_InitScene(&play, 0);
    Expect(events[0].what == "Object_InitBank", "InitScene's own resets come first");
    Expect(At("BeginScene") < At("RunCommand") && events[At("BeginScene")].resource == T,
           "the scene hook runs for the scene, before its commands");
    Expect(At("RunCommand") < At("AfterSceneCommands"), "the commands run before the after-scene hook");
    Expect((Asked() == std::vector<std::pair<const void*, size_t>>{ { T, 0 }, { T, 1 }, { T, 2 }, { T, 3 } }),
           "RunCommand is asked for each top-level command before the end marker");
    Expect((Ran() == std::vector<std::pair<const void*, size_t>>{ { topCmd[3], 0x0D } }),
           "only the unplanned 0x0D runs vanilla; the 0x18 runs its own handler, which has no layer 0");
    Expect(sCommandDepth == 0, "the depth is back to 0");

    // Layer 4: the alternate header runs nested with its vanilla lists, and the top-level header stops after it.
    events.clear();
    gSaveContext.sceneLayer = 4;
    OTRPlay_InitScene(&play, 0);
    Expect((Asked() == std::vector<std::pair<const void*, size_t>>{ { T, 0 } }),
           "the alternate header's commands are never asked, and the top-level header stops after it");
    Expect((Ran() == std::vector<std::pair<const void*, size_t>>{ { alt->commands[0].get(), 0x0E },
                                                                   { alt->commands[1].get(), 0x01 } }),
           "the alternate header's commands run vanilla");
    Expect(sCommandDepth == 0, "the depth is back to 0 after the nested header");

    // Ice Cavern: the planned 0x18 becomes the path list, so the alternate header never runs, even on layer 4, and
    // the rest of the top-level header runs.
    planned = { { T, 0 }, { T, 1 } };
    events.clear();
    OTRPlay_InitScene(&play, 0);
    Expect((Asked() == std::vector<std::pair<const void*, size_t>>{ { T, 0 }, { T, 1 }, { T, 2 }, { T, 3 } }),
           "a replaced 0x18 does not stop the header");
    Expect((Ran() == std::vector<std::pair<const void*, size_t>>{ { topCmd[2], 0x01 }, { topCmd[3], 0x0D } }),
           "a replaced 0x18 runs no alternate header");
    gSaveContext.sceneLayer = 0;
}

void CheckRoom() {
    for (auto& handler : sceneCommands)
        handler = Vanilla;
    sceneCommands[0x18] = Scene_CommandAlternateHeaderList;
    auto alt = Header({ 0x0B, 0x01, 0x14 });
    auto room = Header({ 0x18, 0x0B, 0x01, 0x14, 0x01 }, alt);
    const void* R = room.get();
    planned = { { R, 1 }, { R, 2 }, { alt.get(), 0 }, { alt.get(), 1 } };
    PlayState play;
    play.roomCtx.roomToLoad = room.get();

    // No room has finished loading: neither hook nor command runs.
    events.clear();
    play.roomCtx.status = 0;
    Expect(OTRfunc_800973FC(&play, &play.roomCtx) == 1 && events.empty(), "an idle room context does nothing");

    for (int layer : { 0, 4 }) {
        gSaveContext.sceneLayer = layer;
        events.clear();
        play.roomCtx.status = 1;
        Expect(OTRfunc_800973FC(&play, &play.roomCtx) == 1, "the room loads");
        Expect(events[0].what == "BeginRoom" && events[0].resource == R, "the room hook runs first, for the room");
        Expect(At("RunCommand") < At("Player_SetBootData") && At("Player_SetBootData") <
               At("Actor_SpawnTransitionActors"), "the room's commands run before its transition actors spawn");
        if (layer == 0) {
            Expect((Asked() == std::vector<std::pair<const void*, size_t>>{ { R, 0 }, { R, 1 }, { R, 2 } }),
                   "RunCommand is asked for each room command before the end marker");
            Expect(Ran().empty(), "the planned room lists replace the vanilla ones");
        } else {
            Expect((Asked() == std::vector<std::pair<const void*, size_t>>{ { R, 0 } }),
                   "a room's alternate header is not asked");
            Expect(Ran().size() == 2, "a room's alternate header runs its vanilla lists");
        }
        Expect(sCommandDepth == 0, "the depth is back to 0");
    }
    gSaveContext.sceneLayer = 0;
}

// The refactored list commands against the handlers they replaced, on the same inputs.
void CheckRefactorsMatchOriginals() {
    unsigned seed = 12345;
    auto next = [&]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7fff; };
    for (int round = 0; round < 2000; round++) {
        PlayState a, b;
        a.objectCtx.num = static_cast<u8>(next() % 20);
        a.objectCtx.unk_09 = static_cast<u8>(next() % (a.objectCtx.num + 1));
        for (int i = 0; i < OBJECT_EXCHANGE_BANK_MAX; i++)
            a.objectCtx.status[i].id = static_cast<s16>(next() % 6);
        SOH::SetObjectList objects;
        objects.cmdId = static_cast<SOH::SceneCommandID>(0x0B);
        const size_t count = next() % 24;
        for (size_t i = 0; i < count; i++)
            objects.objects.push_back(static_cast<int16_t>(next() % 6));
        if (round % 7 == 0) // an object list as long as the bank
            objects.objects.assign(OBJECT_EXCHANGE_BANK_MAX + 3, 1);
        objects.numObjects = static_cast<uint32_t>(objects.objects.size());
        b = a;
        killed = 0;
        Scene_CommandObjectList(&a, &objects);
        const int killedNew = killed;
        killed = 0;
        OriginalObjectList(&b, &objects);
        Expect(killedNew == killed && a.objectCtx.num == b.objectCtx.num && a.objectCtx.unk_09 == b.objectCtx.unk_09,
               "the object list matches the original handler");
        for (int i = 0; i < OBJECT_EXCHANGE_BANK_MAX; i++)
            Expect(a.objectCtx.status[i].id == b.objectCtx.status[i].id, "object bank ids match the original");

        SOH::SetTransitionActorList transitions;
        transitions.cmdId = static_cast<SOH::SceneCommandID>(0x0E);
        const size_t actors = next() % 40;
        for (size_t i = 0; i < actors; i++)
            transitions.transitionActorList.push_back({ {}, static_cast<s16>(next() % 200 - 100), {}, 0, 0 });
        // A count past 255 truncates to the same u8 either way.
        transitions.numTransitionActors = static_cast<uint32_t>(actors + 256 * (round % 3));
        SOH::SetTransitionActorList copy = transitions;
        PlayState c, d;
        Scene_CommandTransitionActorList(&c, &transitions);
        OriginalTransitionActorList(&d, &copy);
        Expect(c.transiActorCtx.numActors == d.transiActorCtx.numActors && c.transiActorCtx.numActors == actors &&
               c.transiActorCtx.list == transitions.transitionActorList.data() &&
               d.transiActorCtx.list == copy.transitionActorList.data(), "transition list pointer and count");
        for (size_t i = 0; i < actors; i++)
            Expect(transitions.transitionActorList[i].id == copy.transitionActorList[i].id &&
                   transitions.transitionActorList[i].id >= 0, "transition ids match the original");

        SOH::SetActorList list;
        list.cmdId = static_cast<SOH::SceneCommandID>(0x01);
        list.actorList.resize(next() % 60);
        list.numActors = static_cast<uint32_t>(list.actorList.size() + (round % 4 == 0 ? 256 : 0));
        PlayState e, f;
        events.clear();
        Scene_CommandActorList(&e, &list);
        const size_t resets = events.size();
        events.clear();
        OriginalActorList(&f, &list);
        Expect(resets == 2 && events.size() == 2 && e.numSetupActors == f.numSetupActors &&
               e.setupActorList == f.setupActorList && e.setupActorList == list.actorList.data(),
               "the actor list and its resets match the original");
    }
}

int main() {
    CheckScene();
    CheckRoom();
    CheckRefactorsMatchOriginals();
    std::puts("ok");
    return 0;
}
'''
harness = work / 'mq_runtime_wiring.cpp'
harness.write_text(code, encoding='utf-8')
exe = hostcxx.build(args.compiler, harness, std='c++17', includes=[stubs, source / 'combo'], defines=('COMBO_BUILD',),
                    warnings=False, msvc=('/utf-8',))
result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
check(result.returncode == 0 and result.stdout.strip() == 'ok',
      'the runner harness failed:\n' + result.stdout + result.stderr)

# The second program runs the real ComboMqRuntime.cpp on Fire Temple, Ice Cavern and Hyrule Field.
glue = prelude + r'''
#include "soh/Enhancements/randomizer/ComboMqRuntime.h"
#include <set>
int gSpdlogErrors = 0;
// The scenes ResourceMgr_SceneUsesBundledMq calls bundled MQ (the resolver, combo/mq/MqMode.h).
std::set<s16> bundledScenes;
extern "C" uint32_t ResourceMgr_SceneUsesBundledMq(s16 sceneNum) {
    return bundledScenes.count(sceneNum) != 0;
}
'''
glue += extracted
glue += common
glue += r'''
constexpr s16 kFireScene = 0x04, kIceScene = 0x09, kHyruleField = 0x51;
constexpr int kRoomA = 0, kRoomB = 1;
const ComboMq::Data* gData = nullptr;

template <class T> std::shared_ptr<T> Make(int id) {
    auto cmd = std::make_shared<T>();
    cmd->cmdId = static_cast<SOH::SceneCommandID>(id);
    return cmd;
}
TransitionActorEntry Transition(s16 id, s8 front, s8 back) {
    TransitionActorEntry entry{};
    entry.sides[0].room = front;
    entry.sides[1].room = back;
    entry.id = id;
    entry.pos = { id, 2, 3 };
    return entry;
}

// A collision resource as CollisionHeaderFactory leaves it: its header points at its own lists.
std::shared_ptr<SOH::CollisionHeader> Collision(size_t polys, size_t surfaces) {
    auto col = std::make_shared<SOH::CollisionHeader>();
    col->vertices.assign(4, Vec3s{ 1, 2, 3 });
    col->polygons.resize(polys);
    for (size_t i = 0; i < polys; i++) {
        SOH::CollisionPoly& poly = col->polygons[i];
        poly.type = static_cast<u16>(i % 50);
        poly.flags_vIA = static_cast<u16>(i * 40503u);
        poly.flags_vIB = static_cast<u16>(i * 7u);
        poly.vIC = static_cast<u16>(i);
        poly.normal = { 0, 0x7fff, 0 };
        poly.dist = static_cast<s16>(i);
    }
    col->surfaceTypes.resize(surfaces);
    for (size_t i = 0; i < surfaces; i++)
        col->surfaceTypes[i] = { { static_cast<u32>(0x10000 + i), static_cast<u32>(0x20000 + i) } };
    col->camData.resize(1);
    col->waterBoxes.resize(2);
    SOH::CollisionHeaderData& header = col->collisionHeaderData;
    header = {};
    header.minBounds = { -1, -2, -3 };
    header.maxBounds = { 1, 2, 3 };
    header.numVertices = static_cast<u16>(col->vertices.size());
    header.vtxList = col->vertices.data();
    header.numPolygons = static_cast<u16>(polys);
    header.polyList = col->polygons.data();
    header.surfaceTypeList = col->surfaceTypes.data();
    header.cameraDataList = col->camData.data();
    header.cameraDataListLen = col->camData.size();
    header.numWaterBoxes = static_cast<u16>(col->waterBoxes.size());
    header.waterBoxes = col->waterBoxes.data();
    return col;
}

// A vanilla scene header: sound, transition actors, camera, collision, paths, exits, the end marker, and a 0x0E
// after it that never runs.
struct FakeScene {
    std::shared_ptr<SOH::Scene> header = std::make_shared<SOH::Scene>();
    std::shared_ptr<SOH::SetTransitionActorList> transitions = Make<SOH::SetTransitionActorList>(0x0E);
    std::shared_ptr<SOH::SetCollisionHeader> collision = Make<SOH::SetCollisionHeader>(0x03);
    std::shared_ptr<SOH::SetPathways> paths = Make<SOH::SetPathways>(0x0D);
    std::vector<Vec3s> points{ { 7, 8, 9 }, { 10, 11, 12 } };
    SOH::PathData path{ 2, nullptr };
    explicit FakeScene(std::shared_ptr<SOH::CollisionHeader> col) {
        transitions->transitionActorList = { Transition(0x2E, 0, 1), Transition(-0x23, 1, 2) };
        transitions->numTransitionActors = 2;
        collision->collisionHeader = std::move(col);
        path.points = points.data();
        paths->paths = { &path };
        auto afterEnd = Make<SOH::SetTransitionActorList>(0x0E);
        afterEnd->transitionActorList = { Transition(0x2E, 3, 4) };
        afterEnd->numTransitionActors = 1;
        header->commands = { Make<SOH::ISceneCommand>(0x15), transitions, Make<SOH::ISceneCommand>(0x19), collision,
                             paths, Make<SOH::ISceneCommand>(0x13), Make<SOH::ISceneCommand>(0x14), afterEnd };
    }
    FakeScene(const FakeScene&) = delete;
    FakeScene& operator=(const FakeScene&) = delete;
};
// A vanilla room header: echo, behavior, mesh, objects, actors, the end marker.
struct FakeRoom {
    std::shared_ptr<SOH::Scene> header = std::make_shared<SOH::Scene>();
    std::shared_ptr<SOH::SetActorList> actors = Make<SOH::SetActorList>(0x01);
    std::shared_ptr<SOH::SetObjectList> objects = Make<SOH::SetObjectList>(0x0B);
    explicit FakeRoom(s16 salt) {
        actors->actorList = { { static_cast<s16>(0x10 + salt), { 1, 2, 3 }, { 0, 0, 0 }, 0 },
                              { static_cast<s16>(0x20 + salt), { 4, 5, 6 }, { 0, 0, 0 }, 1 } };
        actors->numActors = 2;
        objects->objects = { static_cast<s16>(0x30 + salt), static_cast<s16>(0x40 + salt) };
        objects->numObjects = 2;
        header->commands = { Make<SOH::ISceneCommand>(0x16), Make<SOH::ISceneCommand>(0x08),
                             Make<SOH::ISceneCommand>(0x0A), objects, actors, Make<SOH::ISceneCommand>(0x14) };
    }
};

void LoadScene(PlayState& play, s16 sceneNum, FakeScene& scene) {
    play.sceneNum = sceneNum;
    play.sceneSegment = scene.header.get();
    play.colCtx.colHeader = nullptr;
    play.transiActorCtx = {};
    events.clear();
    OTRPlay_InitScene(&play, 0);
    Expect(sCommandDepth == 0, "the depth is back to 0 after the scene");
}
void LoadRoom(PlayState& play, FakeRoom& room, int num) {
    play.roomCtx.curRoom.num = static_cast<s8>(num);
    play.roomCtx.status = 1;
    play.roomCtx.roomToLoad = room.header.get();
    play.objectCtx.num = play.objectCtx.unk_09 = 0;
    play.numSetupActors = 0;
    play.setupActorList = nullptr;
    events.clear();
    Expect(OTRfunc_800973FC(&play, &play.roomCtx) == 1 && sCommandDepth == 0, "the room loads");
}
// The command ids whose vanilla handler is Vanilla and ran, in order.
std::vector<int> RanIds() {
    std::vector<int> ids;
    for (const Event& e : events)
        if (e.what == "Vanilla")
            ids.push_back(static_cast<int>(e.index));
    return ids;
}
int Count(const char* what) {
    int count = 0;
    for (const Event& e : events)
        count += e.what == what;
    return count;
}

bool SameVec(const Vec3s& a, const ComboMq::Vec3sPOD& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
bool SameActors(const ActorEntry* list, int dungeon, int room) {
    const ComboMq::Room& mq = *gData->FindRoom(dungeon, room);
    for (size_t i = 0; i < mq.actors.size(); i++) {
        const ComboMq::ActorEntryPOD& want = mq.actors[i];
        if (list[i].id != want.id || !SameVec(list[i].pos, want.pos) || !SameVec(list[i].rot, want.rot) ||
            list[i].params != want.params)
            return false;
    }
    return true;
}

// The scene's 0x0E and 0x0D set the dungeon's MQ lists from the runtime's buffer, never the resource's.
void ExpectMqSceneLists(const PlayState& play, const FakeScene& scene, int dungeon, const std::string& when) {
    const ComboMq::Scene& mq = gData->dungeons[dungeon].scene;
    const TransitionActorEntry* list = play.transiActorCtx.list;
    Expect(list != nullptr && list != scene.transitions->transitionActorList.data() &&
               play.transiActorCtx.numActors == mq.transitions.size(),
           when + ": the transition actors are the MQ list");
    for (size_t i = 0; i < mq.transitions.size(); i++) {
        const ComboMq::TransitionActorPOD& want = mq.transitions[i];
        Expect(list[i].sides[0].room == want.sides[0].room && list[i].sides[0].effects == want.sides[0].effects &&
                   list[i].sides[1].room == want.sides[1].room && list[i].sides[1].effects == want.sides[1].effects &&
                   list[i].id == (want.id < 0 ? -want.id : want.id) && SameVec(list[i].pos, want.pos) &&
                   list[i].rotY == want.rotY && list[i].params == want.params,
               when + ": transition actor " + std::to_string(i) + " is the bins'");
    }
    const Path* paths = play.setupPathList;
    Expect(mq.hasPaths && paths != nullptr && paths != reinterpret_cast<const Path*>(&scene.path),
           when + ": the paths are the MQ list");
    for (size_t p = 0; p < mq.paths.size(); p++) {
        Expect(paths[p].count == mq.paths[p].size(), when + ": path " + std::to_string(p) + " count");
        for (size_t k = 0; k < mq.paths[p].size(); k++)
            Expect(SameVec(paths[p].points[k], mq.paths[p][k]), when + ": path " + std::to_string(p) + " point");
    }
    Expect(paths[mq.paths.size()].count == 0, when + ": the path list ends in the zero-count entry");
}
// The 0x03 hands BgCheck a copy of the resource's header with mq.c's patches (mq.c:133-140, 177-217) over its own
// lists, written out here, and the resource's vertices, cameras and water boxes.
void ExpectPatchedCollision(const CollisionHeader* header, const SOH::CollisionHeaderData& resource,
                            const std::vector<SOH::CollisionPoly>& polys,
                            const std::vector<SOH::SurfaceType>& surfaces, const std::string& when) {
    std::vector<SOH::CollisionPoly> wantPolys = polys;
    std::vector<SOH::SurfaceType> wantSurfaces = surfaces;
    const ComboMq::Scene& fire = gData->dungeons[ComboMq::kFireTemple].scene;
    for (const ComboMq::PolyPatch& patch : fire.polyPatches) {
        wantPolys[patch.id].type = patch.type;
        wantPolys[patch.id].flags_vIA = static_cast<u16>((wantPolys[patch.id].flags_vIA & 0x1fff) | (patch.flags << 13));
    }
    for (const ComboMq::SurfacePatch& patch : fire.surfacePatches) {
        wantSurfaces[patch.id].data[0] = patch.hi;
        wantSurfaces[patch.id].data[1] = patch.lo;
    }
    Expect(std::memcmp(wantPolys.data(), polys.data(), polys.size() * sizeof(SOH::CollisionPoly)) != 0 &&
               std::memcmp(wantSurfaces.data(), surfaces.data(), surfaces.size() * sizeof(SOH::SurfaceType)) != 0,
           "Fire Temple's patches change its polygons and surface types");
    Expect(header != nullptr && reinterpret_cast<const void*>(header) != &resource,
           when + ": BgCheck gets a copy of the collision header");
    Expect(header->vtxList == resource.vtxList && header->numVertices == resource.numVertices &&
               header->numPolygons == resource.numPolygons &&
               static_cast<const void*>(header->cameraDataList) == resource.cameraDataList &&
               header->cameraDataListLen == resource.cameraDataListLen && header->numWaterBoxes == resource.numWaterBoxes &&
               static_cast<const void*>(header->waterBoxes) == resource.waterBoxes &&
               header->minBounds.x == resource.minBounds.x && header->maxBounds.z == resource.maxBounds.z,
           when + ": the copy keeps the resource's vertices, cameras and water boxes");
    Expect(reinterpret_cast<const void*>(header->polyList) != resource.polyList &&
               reinterpret_cast<const void*>(header->surfaceTypeList) != resource.surfaceTypeList,
           when + ": the copy has its own polygon and surface type lists");
    Expect(std::memcmp(header->polyList, wantPolys.data(), wantPolys.size() * sizeof(SOH::CollisionPoly)) == 0 &&
               std::memcmp(header->surfaceTypeList, wantSurfaces.data(),
                           wantSurfaces.size() * sizeof(SOH::SurfaceType)) == 0,
           when + ": the copy's lists carry exactly mq.c's patches");
}
void ExpectResourceUntouched(const SOH::CollisionHeader& resource, const std::vector<SOH::CollisionPoly>& polys,
                             const std::vector<SOH::SurfaceType>& surfaces, const std::string& when) {
    Expect(resource.collisionHeaderData.polyList == resource.polygons.data() &&
               resource.collisionHeaderData.surfaceTypeList == resource.surfaceTypes.data() &&
               std::memcmp(resource.polygons.data(), polys.data(), polys.size() * sizeof(SOH::CollisionPoly)) == 0 &&
               std::memcmp(resource.surfaceTypes.data(), surfaces.data(),
                           surfaces.size() * sizeof(SOH::SurfaceType)) == 0,
           when + ": the cached collision resource is untouched");
}
// The room's 0x01 and 0x0B set the MQ room's lists through the vanilla bodies, with their per-room resets.
void ExpectMqRoom(const PlayState& play, const FakeRoom& room, int dungeon, int num, const std::string& when) {
    const ComboMq::Room& mq = *gData->FindRoom(dungeon, num);
    Expect(play.setupActorList != nullptr && play.setupActorList != room.actors->actorList.data() &&
               play.numSetupActors == mq.actors.size() &&
               SameActors(play.setupActorList, dungeon, num),
           when + ": the actors are the MQ room's");
    Expect(play.objectCtx.num == mq.objects.size(), when + ": the object count is the MQ room's");
    for (size_t k = 0; k < mq.objects.size(); k++)
        Expect(play.objectCtx.status[k].id == -mq.objects[k], when + ": object " + std::to_string(k));
    Expect(Count("SoulReset") == 1 && Count("SilverReset") == 1, when + ": the actor list's per-room resets ran once");
    Expect((RanIds() == std::vector<int>{ 0x16, 0x08, 0x0A }), when + ": every other room command ran vanilla");
}
void ExpectVanillaRoom(const PlayState& play, const FakeRoom& room, const std::string& when) {
    Expect(play.setupActorList == room.actors->actorList.data() && play.numSetupActors == 2,
           when + ": the actors are the resource's");
    Expect(play.objectCtx.num == 2 && play.objectCtx.status[0].id == -room.objects->objects[0] &&
               play.objectCtx.status[1].id == -room.objects->objects[1],
           when + ": the objects are the resource's");
}
void ExpectVanillaScene(const PlayState& play, const FakeScene& scene, const std::string& when) {
    Expect(play.transiActorCtx.list == scene.transitions->transitionActorList.data() &&
               play.transiActorCtx.numActors == 2 && scene.transitions->transitionActorList[1].id == 0x23,
           when + ": the transition actors are the resource's");
    Expect(play.setupPathList == reinterpret_cast<const Path*>(&scene.path), when + ": the paths are the resource's");
    const SOH::CollisionHeaderData* resource = scene.collision->GetPointer();
    Expect(reinterpret_cast<const void*>(play.colCtx.colHeader) == resource, when + ": BgCheck gets the resource's header");
    Expect((RanIds() == std::vector<int>{ 0x15, 0x19, 0x13 }), when + ": the other commands ran vanilla");
}

void CheckRealRuntime() {
    gData = ComboMq::Bundled();
    Expect(gData != nullptr, "the bundled bins decode");
    for (auto& handler : sceneCommands)
        handler = Vanilla;
    sceneCommands[0x01] = Scene_CommandActorList;
    sceneCommands[0x03] = Scene_CommandCollisionHeader;
    sceneCommands[0x0B] = Scene_CommandObjectList;
    sceneCommands[0x0D] = Scene_CommandPathList;
    sceneCommands[0x0E] = Scene_CommandTransitionActorList;
    sceneCommands[0x18] = Scene_CommandAlternateHeaderList;
    gSaveContext.sceneLayer = 0;
    const int fire = ComboMq::kFireTemple;
    const ComboMq::Room* roomA = gData->FindRoom(fire, kRoomA);
    const ComboMq::Room* roomB = gData->FindRoom(fire, kRoomB);
    Expect(roomA != nullptr && roomB != nullptr && !roomA->actors.empty() && !roomB->actors.empty() &&
               !roomA->objects.empty() && !roomB->objects.empty() &&
               (roomA->actors[0].id != roomB->actors[0].id || roomA->actors[0].pos.x != roomB->actors[0].pos.x),
           "Fire Temple's MQ rooms A and B have actors and objects, and differ");

    // Fire Temple from the bundled MQ data. The test keeps only weak and pristine views of the collision; the
    // scene's command holds it, as the resource cache does.
    bundledScenes = { kFireScene, kIceScene, kHyruleField };
    auto collision = Collision(4857, 68);
    const std::vector<SOH::CollisionPoly> polys = collision->polygons;
    const std::vector<SOH::SurfaceType> surfaces = collision->surfaceTypes;
    const std::weak_ptr<SOH::CollisionHeader> firstCollision = collision;
    auto scene = std::make_unique<FakeScene>(std::move(collision));
    FakeRoom vanillaA(1), vanillaB(2);
    PlayState play;

    LoadScene(play, kFireScene, *scene);
    ExpectMqSceneLists(play, *scene, fire, "MQ scene");
    ExpectPatchedCollision(play.colCtx.colHeader, *scene->collision->GetPointer(), polys, surfaces, "MQ scene");
    ExpectResourceUntouched(*scene->collision->collisionHeader, polys, surfaces, "MQ scene");
    Expect((RanIds() == std::vector<int>{ 0x15, 0x19, 0x13 }), "MQ scene: the other commands ran vanilla");
    Expect(scene->transitions->transitionActorList[1].id == -0x23, "MQ scene: the vanilla transition list never ran");
    TransitionActorEntry* const mqTransitions = play.transiActorCtx.list;
    const Path* const mqPaths = play.setupPathList;
    const CollisionHeader* const mqCollision = play.colCtx.colHeader;

    // Rooms A, B, A: each MQ room's lists go into the buffer the swap made current (mq.c:126-131, 367), and the
    // other buffer keeps the previous room's.
    LoadRoom(play, vanillaA, kRoomA);
    ExpectMqRoom(play, vanillaA, fire, kRoomA, "room A");
    const ActorEntry* const bufferA = play.setupActorList;
    LoadRoom(play, vanillaB, kRoomB);
    ExpectMqRoom(play, vanillaB, fire, kRoomB, "room B");
    const ActorEntry* const bufferB = play.setupActorList;
    Expect(bufferB != bufferA, "room B loads into the other buffer");
    Expect(SameActors(bufferA, fire, kRoomA), "room A's lists stay intact while room B loads");
    LoadRoom(play, vanillaA, kRoomA);
    ExpectMqRoom(play, vanillaA, fire, kRoomA, "room A again");
    Expect(play.setupActorList == bufferA, "room A again loads into the first buffer");
    Expect(SameActors(bufferB, fire, kRoomB), "room B's lists stay intact while room A loads again");

    // The engine uses the MQ lists in place: the spawn negates transition ids, and anything may change.
    for (int i = 0; i < play.transiActorCtx.numActors; i++) {
        mqTransitions[i].id = static_cast<s16>(-mqTransitions[i].id);
        mqTransitions[i].pos.x = 12345;
    }

    // The same dungeon and resources without MQ: every command sets the resource's own list.
    bundledScenes.erase(kFireScene);
    LoadScene(play, kFireScene, *scene);
    ExpectVanillaScene(play, *scene, "vanilla scene");
    LoadRoom(play, vanillaA, kRoomA);
    ExpectVanillaRoom(play, vanillaA, "vanilla room A");
    Expect(SameActors(bufferA, fire, kRoomA), "a vanilla room load writes no MQ buffer");

    // MQ again: the scene lists and collision are refilled at the same addresses; one vanilla room load swapped the
    // buffers too, so room A lands where it did.
    bundledScenes.insert(kFireScene);
    LoadScene(play, kFireScene, *scene);
    ExpectMqSceneLists(play, *scene, fire, "MQ scene again");
    ExpectPatchedCollision(play.colCtx.colHeader, *scene->collision->GetPointer(), polys, surfaces, "MQ scene again");
    Expect(play.transiActorCtx.list == mqTransitions && play.setupPathList == mqPaths &&
               play.colCtx.colHeader == mqCollision,
           "MQ scene again: the lists and collision keep their addresses");
    LoadRoom(play, vanillaA, kRoomA);
    ExpectMqRoom(play, vanillaA, fire, kRoomA, "room A after the vanilla visit");
    Expect(play.setupActorList == bufferA, "the vanilla room load swapped the room buffers (mq.c:367)");
    ExpectResourceUntouched(*scene->collision->collisionHeader, polys, surfaces, "MQ scene again");

    // The cache replaces the collision resource. The new one gets its own copy; the old copy, which a save state
    // may still point at, stays with its resource and its patched lists.
    auto replaced = std::make_unique<FakeScene>(Collision(4857, 68));
    scene.reset();
    Expect(!firstCollision.expired(), "the patched copy holds its collision resource");
    LoadScene(play, kFireScene, *replaced);
    ExpectMqSceneLists(play, *replaced, fire, "replaced collision");
    ExpectPatchedCollision(play.colCtx.colHeader, *replaced->collision->GetPointer(), polys, surfaces,
                           "replaced collision");
    Expect(play.colCtx.colHeader != mqCollision, "a replaced collision resource gets a new copy");
    Expect(!firstCollision.expired(), "the replaced copy keeps its collision resource");
    ExpectPatchedCollision(mqCollision, *firstCollision.lock()->GetPointer(), polys, surfaces, "the replaced copy");
    const CollisionHeader* const replacedCollision = play.colCtx.colHeader;

    // A collision that cannot be patched exactly, or did not load, stays vanilla with the reason logged; the
    // scene's other lists are still MQ.
    auto shortCollision = Collision(100, 4);
    auto unpatchable = std::make_unique<FakeScene>(shortCollision);
    const int errors = gSpdlogErrors;
    LoadScene(play, kFireScene, *unpatchable);
    ExpectMqSceneLists(play, *unpatchable, fire, "unpatchable collision");
    Expect(reinterpret_cast<const void*>(play.colCtx.colHeader) == &shortCollision->collisionHeaderData &&
               gSpdlogErrors == errors + 1,
           "a collision too short for the patches stays vanilla, with the reason logged");
    auto missing = std::make_unique<FakeScene>(nullptr);
    LoadScene(play, kFireScene, *missing);
    ExpectMqSceneLists(play, *missing, fire, "missing collision");
    Expect(play.colCtx.colHeader == nullptr && gSpdlogErrors == errors + 2,
           "a collision that did not load stays vanilla, with the reason logged");
    LoadScene(play, kFireScene, *replaced);
    Expect(play.colCtx.colHeader != replacedCollision && play.colCtx.colHeader != mqCollision,
           "the copy replaced by another resource is kept, and a new one patched");
    ExpectPatchedCollision(play.colCtx.colHeader, *replaced->collision->GetPointer(), polys, surfaces, "replaced again");
    ExpectPatchedCollision(replacedCollision, *replaced->collision->GetPointer(), polys, surfaces, "the kept copy");
    ExpectPatchedCollision(mqCollision, *firstCollision.lock()->GetPointer(), polys, surfaces, "the first copy");

    // Ice Cavern on layer 4: its 0x18 sets the MQ paths in place of its alternate header, which never runs, and
    // the rest of the top-level header runs with the MQ transition actors (mq.c:328-340).
    auto ice = std::make_unique<FakeScene>(Collision(10, 2));
    auto layer4 = std::make_shared<SOH::Scene>();
    auto layer4Transitions = Make<SOH::SetTransitionActorList>(0x0E);
    layer4Transitions->transitionActorList = { Transition(0x2E, 5, 6) };
    layer4Transitions->numTransitionActors = 1;
    layer4->commands = { layer4Transitions, Make<SOH::ISceneCommand>(0x14) };
    auto alternates = Make<SOH::SetAlternateHeaders>(0x18);
    alternates->headers = { nullptr, nullptr, nullptr, layer4 };
    ice->header->commands = { alternates, ice->transitions, ice->collision, Make<SOH::ISceneCommand>(0x13),
                              Make<SOH::ISceneCommand>(0x14) };
    gSaveContext.sceneLayer = 4;
    LoadScene(play, kIceScene, *ice);
    ExpectMqSceneLists(play, *ice, ComboMq::kIceCavern, "MQ Ice Cavern");
    Expect(reinterpret_cast<const void*>(play.colCtx.colHeader) == ice->collision->GetPointer(),
           "MQ Ice Cavern keeps its vanilla collision");
    Expect((RanIds() == std::vector<int>{ 0x13 }), "MQ Ice Cavern: its alternate header never runs");
    bundledScenes.erase(kIceScene);
    LoadScene(play, kIceScene, *ice);
    Expect(play.transiActorCtx.list == layer4Transitions->transitionActorList.data() &&
               play.setupPathList == nullptr && play.colCtx.colHeader == nullptr,
           "vanilla Ice Cavern runs its layer-4 header and stops");
    gSaveContext.sceneLayer = 0;

    // A scene outside the 12 dungeons plays vanilla even when the predicate answers bundled MQ.
    auto field = std::make_unique<FakeScene>(Collision(10, 2));
    LoadScene(play, kHyruleField, *field);
    ExpectVanillaScene(play, *field, "Hyrule Field");
    LoadRoom(play, vanillaA, kRoomA);
    ExpectVanillaRoom(play, vanillaA, "a Hyrule Field room");
}

int main() {
    CheckRealRuntime();
    std::puts("ok");
    return 0;
}
'''
glue_harness = work / 'mq_runtime_glue.cpp'
glue_harness.write_text(glue, encoding='utf-8')
exe = hostcxx.build(args.compiler, glue_harness, std='c++20', includes=[stubs, source / 'soh', source / 'combo', blobs],
                    defines=('COMBO_BUILD',), warnings=False, msvc=('/utf-8', '/bigobj'),
                    sources=[source / 'soh/soh/Enhancements/randomizer/ComboMqRuntime.cpp',
                             source / 'combo/mq/ComboMqData.cpp', blobs / 'ComboMqBlobs.cpp'])
result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
check(result.returncode == 0 and result.stdout.strip() == 'ok',
      'the real ComboMqRuntime.cpp failed:\n' + result.stdout + result.stderr)

# Actor_Spawn: OoTMM's spawn wrapper refuses a negative id before anything else sees it (actors.c:342).
actor = without_comments((source / 'soh/src/code/z_actor.c').read_text(encoding='utf-8'))
spawn = function(actor, 'Actor* Actor_Spawn(ActorContext* actorCtx, PlayState* play, s16 actorId,')
guard = re.search(r'#ifdef COMBO_BUILD\s*if \(actorId < 0\) \{\s*return NULL;\s*\}\s*#endif', spawn)
check(guard is not None and guard.start() < spawn.index('ActorDB_Retrieve(actorId)'),
      'Actor_Spawn must return NULL for a negative id under COMBO_BUILD before ActorDB_Retrieve')
check(re.search(r'\{\s*Actor\* actor;\s*s32 objBankIndex;\s*u32 temp;\s*#ifdef COMBO_BUILD', spawn),
      'the negative-id guard must be the first statement of Actor_Spawn')

# The MQ runtime never writes a cached resource.
runtime = without_comments((source / 'soh/soh/Enhancements/randomizer/ComboMqRuntime.cpp').read_text(encoding='utf-8'))
pure = without_comments((source / 'combo/mq/MqRuntime.h').read_text(encoding='utf-8'))
for name, text in (('ComboMqRuntime.cpp', runtime), ('MqRuntime.h', pure)):
    check('GetRawPointer' not in text and 'const_cast' not in text,
          f'{name}: no GetRawPointer or const_cast; the MQ lists live in owned buffers')
check(not re.search(r'\b(source|setCollision)->[\w.\[\]>-]*\s*=(?!=)', runtime),
      'ComboMqRuntime.cpp: nothing is written through the cached collision resource')
check('copy->header = source->collisionHeaderData;' in runtime and
      'copy->header.polyList = copy->patched.polys.data();' in runtime and
      'copy->header.surfaceTypeList = copy->patched.surfaces.data();' in runtime,
      'ComboMqRuntime.cpp: BgCheck gets a copied header pointing at the patched copies')
check(re.search(r'case ComboMq::CommandOverride::Collision:\s*BgCheck_Allocate\(&play->colCtx, play,\s*'
                r'\(CollisionHeader\*\)&state\.collisions\[state\.sceneDungeon\]\[index\]->header\);', runtime),
      'ComboMqRuntime.cpp: the collision override allocates the copy')
# The replaced lists run through the vanilla commands' bodies (with their per-room resets and ABS loop).
for case, call in (('TransitionActors', r'SceneBuffer& buffer = state\.sceneBuffers\[state\.sceneDungeon\];\s*'
                                        r'OTRScene_SetTransitionActorList\(play, buffer\.Transitions\(\), '
                                        r'buffer\.TransitionCount\(\)\);'),
                   ('RoomActors', r'OTRScene_SetActorList\(play, state\.room->actors, state\.room->numActors\);'),
                   ('RoomObjects', r'OTRScene_SetObjectList\(play, state\.room->objects, state\.room->numObjects\);')):
    check(re.search(r'case ComboMq::CommandOverride::' + case + r':\s*\{?\s*' + call + r'\s*return true;', runtime),
          f'ComboMqRuntime.cpp: the {case} override must run the vanilla command body')
check(re.search(r'case ComboMq::CommandOverride::Paths:\s*case ComboMq::CommandOverride::PathsInsteadOfAlternateHeaders:'
                r'\s*play->setupPathList = state\.sceneBuffers\[state\.sceneDungeon\]\.Paths\(\);\s*return true;',
                runtime), 'ComboMqRuntime.cpp: both path overrides set the scene buffer\'s path list and go on')
# The begin hooks decide from the resolver's bundled answer, as mq.c from Config_IsMq.
check(re.search(r'ComboMq::DungeonFromScene\(play->sceneNum\);\s*if \(dungeon < 0 \|\| '
                r'!ResourceMgr_SceneUsesBundledMq\(play->sceneNum\)\)', runtime),
      'ComboMqRuntime.cpp: an MQ scene is a bundled-MQ dungeon scene (mqDungeonId, Config_IsMq)')
room = function(runtime, 'void ComboMq_BeginRoom(')
check(room.index('state.roomBuffers.Swap();') < room.index('BundledScene(play, dungeon)'),
      'ComboMq_BeginRoom must swap the room buffers before it looks for an MQ room (mq.c:367)')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: the scene and room hooks run just before their commands; only the top-level header is asked for '
      'overrides and a replaced command skips its vanilla handler; the refactored list commands match the '
      'originals; the real ComboMqRuntime.cpp sets the bins\' lists, swaps the room buffers, hands BgCheck a '
      'patched copy that outlives its replacement, and leaves vanilla loads alone; Actor_Spawn refuses a negative '
      'id first; no cached resource is written')
