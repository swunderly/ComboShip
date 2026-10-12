"""Exercise MM's production visual-observation gate against controlled camera/collision inputs.

Complements live scene testing: the engine adapters are fixtures, not a GPU visibility test.
"""
import argparse
from pathlib import Path
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = p.parse_args()
out = args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
draw = (args.source/'mm/2s2h/Rando/DrawItem.cpp').read_text(encoding='utf-8')
gate = draw[draw.index('static bool IsCheckModelVisible('):]
gate = gate[:gate.index('\n#endif')]
fixtures = r'''
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "rando/TrackerKnowledge.h"
struct Vec3f { float x, y, z; };
struct MtxF { float xx=1, yx=0, zx=0, xy=0, yy=1, zy=0, xz=0, yz=0, zz=1; } testModel;
// id selects the shop-shelf exception (covered by mm_shop_observation_tests.py); this actor is not a shelf.
struct Actor { int id = 0; Vec3f worldPos = {900, 900, 900}; } testActor;
struct CollisionContext {};
struct CollisionPoly {};
constexpr int TRANS_MODE_OFF=0, TRANS_TRIGGER_OFF=0, ACTOR_EN_GIRLA=42;
struct PlayState { int transitionMode=0, transitionTrigger=0; struct { bool fillScreen=false; unsigned char screenFillColor[4]{}; } envCtx; struct { int state=0; } pauseCtx; MtxF viewProjectionMtxF; struct { Vec3f eye; } view; CollisionContext colCtx; } play;
PlayState* gPlayState = &play;
using RandoCheckId = int;
using RandoItemId = int;
Vec3f drawnOrigin = {0, 20, 100}, clip = {0, 0, 50};
float clipW = 100, aspect = 4.0f / 3.0f;
bool obstructed = false;
int raycasts = 0, observations = 0, lastCheck = 0, lastItem = 0;
MtxF* Matrix_GetCurrent() { return &testModel; }
void Matrix_MultZero(Vec3f* p) { *p = drawnOrigin; }
float OTRGetAspectRatio() { return aspect; }
void SkinMatrix_Vec3fMtxFMultXYZW(MtxF*, Vec3f* pos, Vec3f* projected, float* w) {
    assert(pos->x == drawnOrigin.x && pos->y == drawnOrigin.y && pos->z == drawnOrigin.z);
    *projected = clip; *w = clipW;
}
bool BgCheck_AnyLineTest2(CollisionContext* ctx, Vec3f* from, Vec3f* to, Vec3f*, CollisionPoly**,
                        bool walls, bool floors, bool ceilings, bool oneFace) {
    assert(ctx == &play.colCtx);
    assert(from->x == play.view.eye.x && from->y == play.view.eye.y && from->z == play.view.eye.z);
    assert(to->x == drawnOrigin.x && to->y == drawnOrigin.y && to->z == drawnOrigin.z);
    assert(walls && floors && ceilings && !oneFace);
    ++raycasts;
    return obstructed;
}
namespace Rando { namespace CheckTracker {
void RecordSeenItem(int check, int item) { ++observations; lastCheck=check; lastItem=item; }
} }
'''
tests = r'''
int main() {
    play.view.eye = {1, 2, 3};
    assert(IsCheckModelVisible(&testActor));
    assert(!IsCheckModelVisible(nullptr));
    gPlayState = nullptr;
    assert(!IsCheckModelVisible(&testActor));
    gPlayState = &play;
    // Model submitted in front of the camera but hidden by a Palace wall.
    obstructed = true;
    RecordVisibleCheck(123, 456, &testActor);
    assert(observations == 0 && raycasts == 2);
    obstructed = false;
    RecordVisibleCheck(123, 456, &testActor);
    assert(observations == 1 && lastCheck == 123 && lastItem == 456);
    int before = raycasts;
    play.transitionMode = 3;
    assert(!IsCheckModelVisible(&testActor));
    play.transitionMode = 0; play.transitionTrigger = -20;
    assert(!IsCheckModelVisible(&testActor));
    play.transitionTrigger = 0; play.envCtx.fillScreen = true; play.envCtx.screenFillColor[3] = 255;
    assert(!IsCheckModelVisible(&testActor));
    play.envCtx.screenFillColor[3] = 1; // any drawn fill still covers the picture
    assert(!IsCheckModelVisible(&testActor));
    before = raycasts;
    play.envCtx.screenFillColor[3] = 0; // a cutscene fade left the fill on after fading out: nothing drawn
    assert(IsCheckModelVisible(&testActor));
    assert(raycasts == before + 1);
    before = raycasts;
    play.envCtx.fillScreen = false; play.pauseCtx.state = 1;
    assert(!IsCheckModelVisible(&testActor));
    play.pauseCtx.state = 0;
    for (Vec3f off : {Vec3f{-101,0,50}, {101,0,50}, {0,-101,50}, {0,101,50}, {0,0,-101}, {0,0,101}}) {
        clip = off;
        assert(!IsCheckModelVisible(&testActor));
    }
    clip = {0,0,50};
    for (float w : {-100.0f, 0.0f, std::numeric_limits<float>::infinity(), std::nanf("")}) {
        clipW = w;
        assert(!IsCheckModelVisible(&testActor));
    }
    clipW = 100;
    clip.x = std::nanf("");
    assert(!IsCheckModelVisible(&testActor));
    clip = {0,0,50};
    for (float a : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::nanf("")}) {
        aspect = a;
        assert(!IsCheckModelVisible(&testActor));
    }
    aspect = 4.0f / 3.0f;
    for (float* scale : {&testModel.xx, &testModel.yy, &testModel.zz}) {
        *scale = 0;
        assert(!IsCheckModelVisible(&testActor));
        *scale = 1;
    }
    assert(raycasts == before); // invisible/off-screen cases do not need collision work
    // Wide side panels show more than 4:3, narrow viewports show less.
    aspect = 16.0f / 9.0f; clip.x = 120;
    assert(IsCheckModelVisible(&testActor));
    clip.x = 135;
    assert(!IsCheckModelVisible(&testActor));
    aspect = 1; clip.x = 80;
    assert(!IsCheckModelVisible(&testActor));
    clip.x = 70;
    assert(IsCheckModelVisible(&testActor));
    // Reproject from the drawn (bobbing/translated) model and current camera every frame.
    drawnOrigin = {-30,40,70}; play.view.eye = {8,9,10};
    assert(IsCheckModelVisible(&testActor));
    obstructed = true;
    assert(!IsCheckModelVisible(&testActor)); // a moving door closes
    obstructed = false;
    assert(IsCheckModelVisible(&testActor)); // then opens
    std::cout << "PASS: production MM viewport, clipping, model transform, hidden scale, transitions and obstruction guards\n";
}
'''
cpp = out/'mm_check_visibility.cpp'
cpp.write_text(fixtures + gate + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, std='c++17', includes=[args.source/'combo'], werror=True)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
