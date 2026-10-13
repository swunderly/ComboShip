"""Soaring departure in OoT: when the scene is shown again, and through which camera.

The departure's update, its tick-100 white flash and the draw gate are extracted verbatim from CrossOwl.c and run on
engine stubs. As in MM's En_Test7, the scene stays hidden through the draw that follows tick 99 and comes back with
the white frame. In a pre-rendered room (room shape type 1) the view goes back to the scene's own camera at that
same tick, because z_room.c draws such a room's background only for its fixed camera. 3D rooms keep MM's free
camera to the end.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def function(text, signature):
    start = hostcxx.find(text, signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


owl = (source/'soh/soh/Enhancements/CrossWarp/CrossOwl.c').read_text(encoding='utf-8')
room = (source/'soh/src/code/z_room.c').read_text(encoding='utf-8')
play_draw = (source/'soh/src/code/z_play.c').read_text(encoding='utf-8')

# Why the hand-back is needed: room shape 1 draws its background only for a PREREND_FIXED camera, its depth mesh
# always, and the room is hidden while the departure's gate is active.
handlers = room.split('sRoomDrawHandlers[])(PlayState* play, Room* room, u32 flags) = {', 1)[1].split('};', 1)[0]
assert [h.strip() for h in handlers.split(',') if h.strip()][1] == 'func_80096F6C', handlers
for gate in ('sp9C = (camera->setting == CAM_SET_PREREND_FIXED);', 'sp98 = (camera->setting == CAM_SET_PREREND_FIXED);'):
    assert hostcxx.contains(room, gate), gate
assert hostcxx.contains(play_draw, 'const int soaringCutscene = Combo_SOHSoaringCutsceneActive(play);')
assert hostcxx.contains(play_draw, 'if (!soaringCutscene) {')

code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
using f32 = float;
using s32 = int32_t;
using s16 = int16_t;
using u8 = uint8_t;
#define LERP(x, y, scale) (((y) - (x)) * (scale) + (x))
enum { CAM_ID_MAIN = 0, CAM_STAT_WAIT = 1, CAM_STAT_ACTIVE = 7, ACTORCAT_ITEMACTION = 7, ACTOR_COMBO_OWL_WARP = 900,
       OWL_WARP_FLAGS_DRAW_LENS_FLARE = 1 << 2, OWL_WARP_FLAGS_8 = 1 << 3, OWL_WARP_FLAGS_10 = 1 << 4 };
struct Vec3f { f32 x = 0, y = 0, z = 0; };
struct Player;
struct Camera { Vec3f eye, at; f32 fov = 60; Player* player = nullptr; s16 status = CAM_STAT_ACTIVE; };
struct MeshHeader { struct { u8 type = 0; } base; };
struct PlayState;
struct Actor {
    s16 id = 0, params = 0;
    Actor* next = nullptr;
    void (*update)(Actor*, PlayState*) = nullptr;
    struct { Vec3f pos; } world;
};
struct PlayState {
    Camera cameras[2];
    s16 activeCamera = 1;
    struct { struct { Actor* head = nullptr; } actorLists[12]; } actorCtx;
    struct { u8 fillScreen = 0; u8 screenFillColor[4] = {}; } envCtx;
    struct { struct { MeshHeader* meshHeader = nullptr; } curRoom; } roomCtx;
};
struct OwlWarpFeather { int type = 0; };
struct OwlWarpWindCapsule { f32 unk_00 = 0, unk_04 = 0, xzScale = 0, yScale = 0; s16 yaw = 0; };
struct CrossOwl;
typedef void (*CrossOwlActionFunc)(CrossOwl*, PlayState*);
struct CrossOwl {
    Actor actor;
    s32 flags = 0;
    OwlWarpWindCapsule windCapsule;
    OwlWarpFeather feathers[1];
    s32 timer = 0;
    CrossOwlActionFunc actionFunc = nullptr, playerCamFunc = nullptr;
    Vec3f subCamEye, subCamAt;
    f32 subCamFov = 60;
    s16 subCamId = 1;
    u8 departed = 0, sceneCamRestored = 0;
};
Camera* Play_GetCamera(PlayState* play, s16 id) { return &play->cameras[id]; }
void Play_CameraSetAtEye(PlayState* play, s16 id, Vec3f* at, Vec3f* eye) {
    play->cameras[id].at = *at;
    play->cameras[id].eye = *eye;
}
// z_play.c: activating a camera makes it the one the frame is drawn from.
s16 Play_ChangeCameraStatus(PlayState* play, s16 id, s16 status) {
    if (status == CAM_STAT_ACTIVE)
        play->activeCamera = id;
    return play->cameras[id].status = status;
}
void CrossOwl_SetupAction(CrossOwl* self, CrossOwlActionFunc actionFunc) { self->actionFunc = actionFunc; }
void CrossOwl_WarpCsPart6(CrossOwl*, PlayState*) {}
void CrossOwl_UpdateFeathers(PlayState*, OwlWarpFeather*, CrossOwl*, s32, s32) {}
void Actor_Kill(Actor*) {}
'''
for signature in ('int Combo_SOHSoaringCutsceneActive(PlayState* play) {',
                  'void CrossOwl_WarpCsPart5(CrossOwl* this, PlayState* play) {',
                  'void CrossOwl_Update(Actor* thisx, PlayState* play) {'):
    code += function(owl, signature)
# The departure is C. Only rename C's legal `this` identifier for the C++ harness.
code = re.sub(r'\bthis\b', 'self', code)
code += r'''
// meshType < 0: no room mesh header.
static void Depart(int meshType) {
    MeshHeader mesh;
    PlayState play;
    if (meshType >= 0) {
        mesh.base.type = (u8)meshType;
        play.roomCtx.curRoom.meshHeader = &mesh;
    }
    const bool prerendered = meshType == 1;
    // CrossOwl_Init: the sub-camera is active and the scene's own camera waits.
    play.cameras[CAM_ID_MAIN].status = CAM_STAT_WAIT;
    CrossOwl owl;
    owl.actor.id = ACTOR_COMBO_OWL_WARP;
    owl.actor.params = 3;
    owl.actor.update = CrossOwl_Update;
    owl.timer = 97; // Part4 hands over to Part5 in the tick-96 update.
    owl.actionFunc = CrossOwl_WarpCsPart5;
    play.actorCtx.actorLists[ACTORCAT_ITEMACTION].head = &owl.actor;
    for (s32 tick = 97; tick < 130; ++tick) {
        assert(owl.timer == tick);
        CrossOwl_Update(&owl.actor, &play);
        // The frame drawn after this update.
        const bool sceneShown = !Combo_SOHSoaringCutsceneActive(&play);
        assert(sceneShown == (tick >= 100)); // MM: hidden through the draw after tick 99
        if (tick <= 100)
            assert(play.envCtx.fillScreen == (tick == 100)); // the scene comes back under the white frame
        const bool sceneCamera = prerendered && tick >= 100;
        assert(play.activeCamera == (sceneCamera ? CAM_ID_MAIN : owl.subCamId));
        assert(play.cameras[CAM_ID_MAIN].status == (sceneCamera ? CAM_STAT_ACTIVE : CAM_STAT_WAIT));
        assert(play.cameras[owl.subCamId].status == (sceneCamera ? CAM_STAT_WAIT : CAM_STAT_ACTIVE));
        assert((owl.sceneCamRestored != 0) == sceneCamera);
    }
}
int main() {
    for (int meshType = -1; meshType <= 2; ++meshType)
        Depart(meshType);
    puts("Soaring departure: scene shown from the white frame, through a pre-rendered room's own camera only.");
}
'''
test = work/'soaring-prerender-camera.cpp'
test.write_text(code, encoding='utf-8')
subprocess.run([str(hostcxx.build(args.compiler, test))], check=True, timeout=hostcxx.RUN_TIMEOUT)
