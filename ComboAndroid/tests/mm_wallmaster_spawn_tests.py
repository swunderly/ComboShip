"""Compare the full native spawn path with the pinned MM Wallmaster clear hook."""
from pathlib import Path
import argparse
import json
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def extract(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function')


actor_text = (source/'mm/src/code/z_actor.c').read_text(encoding='utf-8')
spawn = extract(actor_text, 'Actor* Actor_SpawnAsChildAndCutscene(')
bridge = (source/'mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp').read_text(encoding='utf-8')
active = extract(bridge, 'extern "C" COMBO_EXPORT int Combo_MMWallmasterErActive(')
fixture = json.loads((source/'tools/ootmm/mm-wallmaster-spawn-source.json').read_text(encoding='utf-8'))
assert fixture['revision'] == '413573d22d1d53e8a81931a70927f625b73488b9'
wrapper = fixture['spawnWrapperFunction']
assert wrapper.index('ret = canSpawnSoul(') < wrapper.index('if (ret <= 0)') < wrapper.index(
    'sActorIdToSpawn = actorId;') < wrapper.index('return _Actor_SpawnAsChildAndCutscene(')

support = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "rando/entrances/Wallmasters.h"
namespace CE = ComboRando::Entrances;
using s16 = int16_t;
using s32 = int32_t;
using u32 = uint32_t;
using f32 = float;
enum { ACTOR_EN_WALLMAS = 1, ACTOR_OTHER = 2, ACTOR_BOSS_05 = 3, ACTOR_EN_PART = 4,
       ACTORCAT_ENEMY = 5, ACTORCAT_PROP = 6, OBJECT_SLOT_NONE = -1, CS_ID_NONE = -1,
       HALFDAYBIT_ALL = 0xffff, CFG_ER_WALLMASTERS = 7 };
struct ActorOverlay { int numLoaded; };
struct PlayState;
struct Actor;
using ActorFunc = void (*)(Actor*, PlayState*);
struct Actor {
    ActorOverlay* overlayEntry;
    s16 id;
    u32 flags;
    int objectSlot;
    ActorFunc init, destroy, update, draw;
    int room;
    Actor* parent;
    Actor* child;
    struct { struct { float x,y,z; } pos; struct { s16 x,y,z; } rot; } home;
    int params, csId;
    u32 halfDaysBits;
};
struct ActorProfile {
    s16 id;
    int type, objectId;
    std::size_t instanceSize;
    u32 flags;
    ActorFunc init, destroy, update, draw;
};
struct ActorContext { int totalLoadedActors; };
struct ObjectContext {};
struct PlayState { ObjectContext objectCtx; struct { struct { int num; } curRoom; } roomCtx; };
bool rando = false, roomClear = false, profileAvailable = true, arenaAvailable = true;
int soulResult = 1, slot = 4;
int soulCalls, blockCalls, overlayLoads, overlayFrees, objectCalls, arenaCalls, initCalls, listIndex;
ActorProfile loadedProfile;
ActorOverlay gActorOverlayTable[8]{};
Actor storage;
uintptr_t gSegments[16]{};
int currentActorListIndex;
#define IS_RANDO rando
#define COMBO_EXPORT
s32 MM_ExpandedSoulPreSpawn(PlayState*, s16, s32* params) {
    ++soulCalls;
    *params = 0x14567;
    return soulResult;
}
void MM_MissingSoulRoomBlock(PlayState*) { ++blockCalls; }
ActorProfile* Actor_LoadOverlay(ActorContext*, s16) { ++overlayLoads; return profileAvailable ? &loadedProfile : nullptr; }
int Object_GetSlot(ObjectContext*, int) { ++objectCalls; return slot; }
int Flags_GetClear(PlayState*, int room) { assert(room == 7); return roomClear; }
void Actor_FreeOverlay(ActorOverlay*) { ++overlayFrees; }
Actor* ZeldaArena_Malloc(std::size_t size) {
    assert(size == sizeof(Actor)); ++arenaCalls;
    return arenaAvailable ? &storage : nullptr;
}
void SetActorListIndex(Actor*, int index) { listIndex = index; }
void Actor_AddToCategory(ActorContext* ctx, Actor*, int) { ++ctx->totalLoadedActors; }
void Actor_Init(Actor*, PlayState*) { ++initCalls; gSegments[6] = 0x9876; }
void Callback(Actor*, PlayState*) {}
int Config_Flag(int flag) {
    assert(flag == CFG_ER_WALLMASTERS);
#ifdef COMBO_BUILD
    return rando && CE::ActiveMmWallmasters().active();
#else
    return 0;
#endif
}
static s16 sActorIdToSpawn;
void ResetCounters() {
    soulCalls = blockCalls = overlayLoads = overlayFrees = objectCalls = arenaCalls = initCalls = 0;
    listIndex = -1;
    currentActorListIndex = 42;
    gSegments[6] = 0x1234;
    std::memset(gActorOverlayTable, 0, sizeof(gActorOverlayTable));
    std::memset(&storage, 0, sizeof(storage));
}
void InstallLayout() {
    std::vector<CE::SavedOverride> rows;
    for (std::size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i) {
        const auto* src = CE::Find(CE::kMmWallmasterSources[i]);
        const auto* dst = CE::Find(CE::kMmBaseWallmasterDestinations[i]);
        assert(src && dst);
        rows.push_back({std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id});
    }
    CE::ActiveMmWallmasters().install(rows);
}
'''
checks = r'''
int main() {
    PlayState play{};
    play.roomCtx.curRoom.num = 7;
    unsigned cases = 0;
    // Deliberately vary requested actor and loaded loadedProfile independently: the
    // reference tracks the requested ID, while vanilla's boss exception uses the loadedProfile.
    for (int installed = 0; installed <= 1; ++installed) {
        CE::ActiveMmWallmasters().clear();
        if (installed) InstallLayout();
        for (int randomizer = 0; randomizer <= 1; ++randomizer) {
            rando = randomizer != 0;
#ifdef COMBO_BUILD
            assert(Combo_MMWallmasterErActive() == (installed && randomizer));
#endif
            for (int cleared = 0; cleared <= 1; ++cleared)
            for (s16 requested : std::array<s16, 3>{ACTOR_EN_WALLMAS, ACTOR_OTHER, ACTOR_BOSS_05})
            for (s16 loaded : std::array<s16, 3>{ACTOR_EN_WALLMAS, ACTOR_OTHER, ACTOR_BOSS_05})
            for (int type : {ACTORCAT_ENEMY, ACTORCAT_PROP})
            for (int objectSlot : std::array<int, 2>{OBJECT_SLOT_NONE, 4}) {
                roomClear = cleared != 0;
                slot = objectSlot;
                loadedProfile = {loaded, type, 10, sizeof(Actor), 0x456, Callback, Callback, Callback, Callback};
                sActorIdToSpawn = requested;
                const bool expected = slot > OBJECT_SLOT_NONE &&
                    !(type == ACTORCAT_ENEMY && GetRoomClearFlagForActor(&play, 7) && loaded != ACTOR_BOSS_05);
                ResetCounters();
                ActorContext ctx{};
                Actor* actor = Actor_SpawnAsChildAndCutscene(&ctx, &play, requested, 1, 2, 3, 4, 5, 6,
                                                            123, 0xff, 0, nullptr);
                assert((actor != nullptr) == expected);
                assert(soulCalls == 1 && blockCalls == 0 && overlayLoads == 1 && objectCalls == 1);
                assert(overlayFrees == static_cast<int>(!expected) && arenaCalls == static_cast<int>(expected) && initCalls == static_cast<int>(expected));
                if (actor) {
                    assert(ctx.totalLoadedActors == 1 && actor == &storage);
                    assert(actor->id == loaded && actor->objectSlot == slot && actor->room == 7);
                    assert(actor->params == 0x4567 && actor->csId == CS_ID_NONE && actor->halfDaysBits == HALFDAYBIT_ALL);
                    assert(actor->home.pos.x == 1 && actor->home.pos.y == 2 && actor->home.pos.z == 3);
                    assert(actor->home.rot.x == 4 && actor->home.rot.y == 5 && actor->home.rot.z == 6);
                    assert(actor->flags == 0x456 && actor->init == Callback && actor->destroy == Callback);
                    assert(actor->update == Callback && actor->draw == Callback);
                    assert(actor->overlayEntry == &gActorOverlayTable[requested]);
                    assert(gActorOverlayTable[requested].numLoaded == 1 && gSegments[6] == 0x1234);
                    assert(listIndex == 42 && currentActorListIndex == -1);
                }
                ++cases;
            }
        }
    }
    rando = true;
    InstallLayout();
    roomClear = true;
    slot = 4;
    loadedProfile = {ACTOR_EN_WALLMAS, ACTORCAT_ENEMY, 10, sizeof(Actor), 0, Callback, Callback, Callback, Callback};
    for (int soul : {-1, 0}) {
        soulResult = soul;
        ResetCounters();
        ActorContext ctx{};
        assert(!Actor_SpawnAsChildAndCutscene(&ctx, &play, ACTOR_EN_WALLMAS, 0,0,0,0,0,0,0,0,0,nullptr));
        assert(soulCalls == 1 && blockCalls == static_cast<int>(soul == 0));
        assert(overlayLoads == 0 && objectCalls == 0 && arenaCalls == 0 && initCalls == 0);
    }
    soulResult = 1;
    // The actor cap, missing overlay and failed allocation keep their old behavior.
    for (int failure = 0; failure < 3; ++failure) {
        ResetCounters();
        ActorContext ctx{failure == 0 ? 255 : 0};
        profileAvailable = failure != 1;
        arenaAvailable = failure != 2;
        roomClear = false;
        assert(!Actor_SpawnAsChildAndCutscene(&ctx, &play, ACTOR_EN_WALLMAS, 0,0,0,0,0,0,0,0,0,nullptr));
        assert(soulCalls == 1 && initCalls == 0);
        assert(overlayLoads == static_cast<int>(failure != 0) && overlayFrees == static_cast<int>(failure == 2));
        assert(arenaCalls == static_cast<int>(failure == 2));
    }
    profileAvailable = arenaAvailable = true;
    ResetCounters();
    loadedProfile.id = ACTOR_EN_PART;
    Actor parent{};
    parent.room = 2;
    ActorContext ctx{};
    Actor* child = Actor_SpawnAsChildAndCutscene(&ctx, &play, ACTOR_EN_PART, 1,2,3,4,5,6,7,8,9,&parent);
    assert(child && child->room == 2 && child->parent == &parent && parent.child == child);
    assert(child->objectSlot == 6 && child->home.rot.z == 0 && child->csId == 8 && child->halfDaysBits == 9);
    // Clearing a seed layout immediately removes the exception, even in rando.
    CE::ActiveMmWallmasters().clear();
#ifdef COMBO_BUILD
    assert(!Combo_MMWallmasterErActive());
#endif
    roomClear = true;
    loadedProfile.id = ACTOR_EN_WALLMAS;
    assert(!Actor_SpawnAsChildAndCutscene(&ctx, &play, ACTOR_EN_WALLMAS, 0,0,0,0,0,0,0,0,0,nullptr));
    std::printf("Actual spawn: %u pinned clear-flag cases plus soul/object/boss/allocation/init/layout guards passed\n", cases);
}
'''
for combo in (True, False):
    cpp = work/('mm_wallmaster_spawn_combo.cpp' if combo else 'mm_wallmaster_spawn_standalone.cpp')
    cpp.write_text(support + (active if combo else '') + '\n' + fixture['roomClearFunction'] + '\n' + spawn + checks,
                   encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo'],
                       defines=['COMBO_BUILD'] if combo else [], werror=True,
                       msvc=['/wd4101'], gnu=['-Wno-unused-variable'])  # native function retains a decomp padding local
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Combo and standalone full spawn paths match pinned MM room-clear behavior.')
