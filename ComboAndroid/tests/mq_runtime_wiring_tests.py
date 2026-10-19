"""OoTMM's Master Quest scene and room hooks wired into SoH's command runner (MQ port unit U4).

OoTMM rewrites the scene header just before Play_InitScene runs it and the room header just before the room-load
routine runs it (src/oot/mq.c:413-437), and only those top-level headers: an alternate header they list keeps its
vanilla lists. The production OTRPlay_InitScene (z_play_otr.cpp), OTRfunc_800973FC, OTRScene_ExecuteCommands,
Scene_CommandAlternateHeaderList and the actor-list, object-list and transition-actor-list bodies (z_scene_otr.cpp)
are extracted verbatim and run on engine stubs, with ComboMq_BeginScene, ComboMq_BeginRoom and ComboMq_RunCommand
recorded. Checks:
- the scene and room hooks run just before their commands, for the resource those commands come from;
- ComboMq_RunCommand is asked only at depth 1, for every command up to the end marker, and a command it runs does
  not also run its vanilla handler; an alternate header's commands run vanilla, and Ice Cavern's 0x18 replaced by
  its paths never runs its alternate header (mq.c:328-340);
- the refactored list commands behave exactly as the handlers they replaced, transcribed below;
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
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
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
    function(scene_otr, 'bool OTRfunc_800982FC('),
    function(scene_otr, 'void OTRScene_SetObjectList('),
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

code = r'''
#define COMBO_BUILD 1
#include "mq/MqRuntime.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using u8 = uint8_t;
using u32 = uint32_t;
#define osSyncPrintf(...) ((void)0)
#define VT_FGCOL(color) ""
#define VT_RST ""
#define VIRTUAL_TO_PHYSICAL(addr) ((uintptr_t)(addr))
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define OBJECT_INVALID 0
#define OBJECT_EXCHANGE_BANK_MAX 128
#define GET_PLAYER(play) (&(play)->player)
#define YREG(n) gYReg[n]

struct Vec3s { s16 x, y, z; };
struct ActorEntry { s16 id; Vec3s pos; Vec3s rot; s16 params; };
struct TransitionActorEntry { struct { s8 room; s8 effects; } sides[2]; s16 id; Vec3s pos; s16 rotY; s16 params; };
namespace SOH {
enum class SceneCommandID : uint8_t { EndMarker = 0x14 };
struct ISceneCommand {
    virtual ~ISceneCommand() = default;
    SceneCommandID cmdId{};
    virtual void* GetRawPointer() { return nullptr; }
};
struct Scene { std::vector<std::shared_ptr<ISceneCommand>> commands; };
struct SetAlternateHeaders : ISceneCommand { std::vector<std::shared_ptr<Scene>> headers; };
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
} // namespace SOH
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
    void* setupExitList = nullptr; void* cUpElfMsgs = nullptr; void* setupPathList = nullptr;
    u8 numSetupActors = 0; ActorEntry* setupActorList = nullptr;
    ObjectContext objectCtx{}; int lightCtx = 0; int state = 0; TransitionActorContext transiActorCtx{};
    RoomContext roomCtx{}; s16 skyboxId = 0; void* sceneSegment = nullptr; Player player{}; ActorContext actorCtx{};
};
struct SaveContext { s32 sceneLayer = 0; s32 worldMapArea = 0; };
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
bool Vanilla(PlayState*, SOH::ISceneCommand* cmd) {
    Log("Vanilla", cmd, static_cast<size_t>(cmd->cmdId));
    return false;
}
'''
code += extracted
code += r'''
int sCommandDepthAtCall() { return sCommandDepth; }

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

std::string Describe() {
    std::string text;
    for (const Event& e : events)
        text += e.what + "(" + std::to_string(e.index) + "@" + std::to_string(e.depth) + ") ";
    return text;
}
void Expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n  events: %s\n", what, Describe().c_str());
        std::exit(1);
    }
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
exe = hostcxx.build(args.compiler, harness, std='c++17', includes=[source / 'combo'], warnings=False,
                    msvc=('/utf-8',))
result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
check(result.returncode == 0 and result.stdout.strip() == 'ok',
      'the runner harness failed:\n' + result.stdout + result.stderr)

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
      'originals; Actor_Spawn refuses a negative id first; no cached resource is written')
