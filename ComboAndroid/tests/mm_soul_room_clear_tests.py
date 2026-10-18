"""Execute both native room-clear setters against the production missing-soul guard."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--actor-source', type=Path)
a = p.parse_args()
source = a.source.resolve()
work = a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def extract(text, signature):
    start = hostcxx.find(text, signature)
    at = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[at] == '{') - (text[at] == '}')
        at += 1
    return text[start:at] + '\n'


actor = (a.actor_source or source / 'mm/src/code/z_actor.c').read_text(encoding='utf-8-sig')
adapter = (source / 'mm/2s2h/Rando/ActorBehavior/MissingMmSouls.cpp').read_text(encoding='utf-8-sig')
code = r'''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "MissingMmSoulsState.h"
using s32 = int32_t;
using u8 = uint8_t;
enum { FLAG_CYCL_SCENE_CLEARED_ROOM = 1 };
struct PlayState {
    int sceneId = 4;
    struct { struct { int num = 7; } curRoom; } roomCtx;
    struct { struct { uint32_t clearedRoom = 0, clearedRoomTemp = 0; } sceneFlags; } actorCtx;
};
static Rando::MissingSoulRoomGuard missingSoulRoom;
int notifications = 0;
void GameInteractor_ExecuteOnSceneFlagSet(int, int, int) { ++notifications; }
void Check(bool value, const char* description) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", description); std::exit(1); }
}
'''
for signature in ['extern "C" void MM_MissingSoulRoomReset(',
                  'extern "C" void MM_MissingSoulRoomBlock(',
                  'extern "C" s32 MM_MissingSoulRoomCanClear(']:
    code += extract(adapter, signature)
for signature in ['s32 Flags_GetClear(', 'void Flags_SetClear(', 'void Flags_SetClearTemp(']:
    code += extract(actor, signature)
code += r'''
int main() {
    PlayState play;
    const auto bit = 1u << play.roomCtx.curRoom.num;
    MM_MissingSoulRoomReset(&play);
    MM_MissingSoulRoomBlock(&play);
    Flags_SetClearTemp(&play, 7);
    Check(play.actorCtx.sceneFlags.clearedRoomTemp == 0,
          "a missing enemy soul must block the temporary clear read by doors");
    Flags_SetClear(&play, 7);
    Check(play.actorCtx.sceneFlags.clearedRoom == 0 && notifications == 0,
          "a rejected permanent clear must not set flags or emit notifications");
    Flags_SetClearTemp(&play, 2);
    Check(play.actorCtx.sceneFlags.clearedRoomTemp == (1u << 2),
          "the guard must not block another room's clear");
    // Reload resets the guard; a subsequent denied spawn must immediately block again.
    MM_MissingSoulRoomReset(&play);
    MM_MissingSoulRoomBlock(&play);
    Flags_SetClearTemp(&play, 7);
    Check(!(play.actorCtx.sceneFlags.clearedRoomTemp & bit), "denied respawn must restore the block");
    // With no denied spawn after the native reset, both normal clear paths remain usable.
    MM_MissingSoulRoomReset(&play);
    Flags_SetClearTemp(&play, 7);
    Check(play.actorCtx.sceneFlags.clearedRoomTemp & bit, "ordinary temporary clears must still work");
    Check(play.actorCtx.sceneFlags.clearedRoom == 0 && notifications == 0,
          "temporary clears must not become permanent or notify permanent-flag listeners");
    Flags_SetClear(&play, 7);
    Check(play.actorCtx.sceneFlags.clearedRoom == bit && notifications == 1,
          "ordinary permanent clears must still notify once");
    Flags_SetClear(&play, 7);
    Check(notifications == 1, "repeated permanent clears must not notify twice");
    std::puts("PASS: production missing-soul guard protects both native clear paths; reset and notification semantics preserved");
}
'''
cpp = work / 'mm-soul-room-clear.cpp'
cpp.write_text(code, encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source / 'mm/2s2h/Rando/ActorBehavior', a.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
