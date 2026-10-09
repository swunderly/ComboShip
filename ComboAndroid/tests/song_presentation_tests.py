"""Exercise native cross-song skip routing and camera handoff without game data.

The functions under test are extracted verbatim from production. The harness
supplies engine services only, including separate active and main cameras.
"""
from pathlib import Path
import argparse
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default='cl')
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


owl = (source/'soh/soh/Enhancements/CrossWarp/CrossOwl.c').read_text(encoding='utf-8')
skip = (source/'mm/2s2h/Enhancements/Songs/SkipSoaringCutscene.cpp').read_text(encoding='utf-8')
warp = (source/'mm/src/overlays/actors/ovl_Combo_Song_Warp/z_combo_song_warp.c').read_text(encoding='utf-8')
message = (source/'soh/src/code/z_message_PAL.c').read_text(encoding='utf-8')
bridge = (source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
conditions = []
for begin, end in [('MSGMODE_SETUP_DISPLAY_SONG_PLAYED', 'MSGMODE_SONG_DEMONSTRATION_SELECT_INSTRUMENT'),
                   ('MSGMODE_DISPLAY_SONG_PLAYED_TEXT_BEGIN', 'MSGMODE_DISPLAY_SONG_PLAYED_TEXT')]:
    section = message.split('case ' + begin + ':', 1)[1].split('case ' + end + ':', 1)[0]
    setting = section.index('CVarGetInteger(CVAR_ENHANCEMENT("FastOcarinaPlayback")')
    start = section.rfind('if (', 0, setting) + len('if (')
    depth, end_pos = 1, start
    while depth:
        depth += (section[end_pos] == '(') - (section[end_pos] == ')')
        end_pos += 1
    conditions.append(section[start:end_pos-1])

code = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
using s16=int16_t; using u16=uint16_t; using f32=float;
#define COMBO_BUILD
#define CVAR_ENHANCEMENT(x) "gEnhancements." x
enum { ACTOR_COMBO_OWL_WARP=900, DEMOKANKYO_WARP_OUT=15, CAM_ID_MAIN=0, CAM_STATUS_WAIT=1,
       CAM_STATUS_ACTIVE=2, PLAYER_CSACTION_WAIT=3, PLAYER_CSACTION_END=4,
       PLAYER_CS_ID_ITEM_OCARINA=0, ACTOR_FLAG_OCARINA_INTERACTION=16,
       PLAYER_STATE1_20000000=32, HUD_VISIBILITY_NONE=1, HUD_VISIBILITY_NOTHING=1, NA_SE_EV_LINK_WARP_IN=77,
       ENTEST7_ARRIVE=-1, OCARINA_MODE_WARP_TO_GREAT_BAY_COAST=27, OCARINA_MODE_WARP_TO_ENTRANCE=37,
       SCENE_SECOM=8, IKANA_CANYON=9, RESPAWN_MODE_TOP=0, PLAYER_START_MODE_OWL=5,
       TRANS_TRIGGER_START=1, TRANS_TYPE_FADE_BLACK=2, OCARINA_SONG_TIME=10,
       OCARINA_SONG_STORMS=11, OCARINA_SONG_SUNS=12 };
struct Vec3f {float x=0,y=0,z=0;};
struct Rot {s16 y=0;};
struct PosRot {Vec3f pos; Rot rot;};
struct Actor {int params=0,room=0,flags=0; PosRot world; struct {Rot rot;} shape; void (*draw)()=nullptr;};
struct Player {Actor actor; int stateFlags1=0;};
struct Camera {Vec3f eye,at; float fov=0;};
struct PlayState {
    Player player;
    Camera cameras[2]; int active=1, sceneId=0, nextEntrance=0,transitionTrigger=0,transitionType=0;
    int playerCsIds[1]={99};
    struct {bool bButtonInterfaceDoActionActive=true;} interfaceCtx;
    struct {bool fillScreen=false;} envCtx;
    struct {int lastPlayedSong=0;} msgCtx;
};
struct {int hudVisibility=7,nextHudVisibility=7,hudVisibilityMode=7,nextHudVisibilityMode=7,respawnFlag=0; struct {int playerParams=0;} respawn[1];
    struct {struct {int pauseSaveEntrance=-1;} shipSaveInfo;} save;} gSaveContext;
int sSongArrival=-1;
namespace ComboRando {constexpr int kMmSongEntrances[10]={100,101,102,103,104,105,106,107,108,109};}
struct CrossSongWarp {Actor actor; int destination=0,warpTimer=0; Vec3f origin; s16 yaw=0,subCamId=0;
    void (*playerDraw)()=nullptr; u16 savedHud=0;};
#define GET_PLAYER(p) (&(p)->player)
#define GET_ACTIVE_CAM(p) (&(p)->cameras[(p)->active])
#define OWL_WARP_CS_GET_OCARINA_MODE(a) ((a)->params)
#define ENTRANCE(scene,spawn) ((scene)*100+(spawn))
#define PLAYER_PARAMS(original,mode) ((original)|(mode))
PlayState* gPlayState=nullptr;
int fast=0, skipSoaring=0,assets=0,spawns=0,completed=-1,route=-1,routeCalls=0;
bool remapped=false,stopped=false,createdAfterStop=false,restored=false;
int sOwlWarpEntrancesForMods[10]={100,101,102,103,104,105,106,107,108,109};
int CVarGetInteger(const char* name,int fallback) {
    if (!std::strcmp(name,"gEnhancements.FastOcarinaPlayback")) return fast;
    if (!std::strcmp(name,"gEnhancements.Songs.SkipSoaringCutscene")) return skipSoaring;
    return fallback;
}
int Combo_SOHPrepareWarpAssets() {++assets;return 1;}
Actor spawned;
Actor* Actor_Spawn(void*,PlayState*,int,float,float,float,int,int,int,int) {++spawns;return &spawned;}
void Combo_SOHCompleteWarp(PlayState*,int destination) {completed=destination;}
int Combo_MMSongWarp(PlayState*,int owl) {++routeCalls;route=owl;return remapped;}
void func_80169F78(PlayState*) {restored=true;}
void CutsceneManager_Stop(int id) {assert(id==99);stopped=true;gPlayState->active=0;}
int hudAlpha=255;
void Interface_SetHudVisibility(int value) {
    if(gSaveContext.hudVisibility!=value) {
        gSaveContext.hudVisibility=value;gSaveContext.nextHudVisibility=value;
    }
}
void Interface_UpdateHudAlphas(PlayState*,s16 alpha) {
    assert(gSaveContext.nextHudVisibility==HUD_VISIBILITY_NONE);hudAlpha=alpha;
}
void Interface_ChangeHudVisibilityMode(int value) {
    if(gSaveContext.hudVisibilityMode!=value) {
        gSaveContext.hudVisibilityMode=value;gSaveContext.nextHudVisibilityMode=value;
    }
}
void func_80082850(PlayState*,s16 alpha) {
    assert(gSaveContext.nextHudVisibilityMode==HUD_VISIBILITY_NOTHING);hudAlpha=alpha;
}
s16 Play_CreateSubCamera(PlayState*) {createdAfterStop=stopped;return 1;}
void Play_SetCameraAtEye(PlayState* play,int camera,const Vec3f* at,const Vec3f* eye) {
    play->cameras[camera].at=*at;play->cameras[camera].eye=*eye;
}
void Play_SetCameraFov(PlayState* play,int camera,float fov) {play->cameras[camera].fov=fov;}
void Play_ChangeCameraStatus(PlayState*,int,int) {}
void Play_ClearCamera(PlayState*,int) {}
void Player_SetCsActionWithHaltedActors(PlayState*,Actor*,int) {}
void Player_SetCsAction(PlayState*,Actor*,int) {}
void Audio_PlaySfx(int) {}
int Combo_SOHForeignSongPerformanceActive() {return 1;}
'''
# Actor contexts are opaque to the spawn stub.
code = code.replace('Player player;', 'Player player; int actorCtx=0;')
code += function(owl, 'void Combo_SOHStartSoaringEffect(')
code += function(owl, 'int Combo_SOHStartSoaring(')
code += function(owl, 'static void CrossOwl_HideHud(')
owl_draw = function(owl, 'void CrossOwl_Draw(Actor* thisx, PlayState* play) {')
assert owl_draw.index('if(thisx->params==0x100)') < owl_draw.index('CrossOwl_HideHud(play);')
code += function(skip, 'static void SkipSoaringCutscene(')
code += function(warp, 'static void CrossSongWarp_Init(')
code += function(warp, 'static void CrossSongWarp_Destroy(')
code += function(warp, 'static void CrossSongWarp_HideHud(')
assert 'CrossSongWarp_HideHud(play);' in function(warp, 'static void CrossSongWarp_Draw(')
code += function(bridge, 'extern "C" int MM_ConsumeSongArrival(')
# The native actors are C. Only rename C's legal `this` identifier when
# compiling their unmodified bodies alongside the C++ enhancement hook.
code = re.sub(r'\bthis\b', 'self', code)
for i, expression in enumerate(conditions):
    code += f'bool fullPlayback{i}(PlayState* play) {{return ({expression});}}\n'
code += r'''
void drawPlayer() {}
int main() {
    PlayState play;gPlayState=&play;
    for (int enabled=0;enabled<2;++enabled) {
        fast=enabled;assets=spawns=0;
        Combo_SOHStartSoaringEffect(&play);
        assert(assets==1-enabled && spawns==1-enabled);
        assert(fullPlayback0(&play)==!enabled && fullPlayback1(&play)==!enabled);
    }
    play.msgCtx.lastPlayedSong=OCARINA_SONG_TIME;
    assert(fullPlayback0(&play) && fullPlayback1(&play));
    // Every selected owl route, including ones landing in OoT, keeps its exact
    // destination when departure is skipped. Arrival remains engine-native.
    for(int d=0;d<16;++d) {
        skipSoaring=1;completed=-1;spawns=0;
        assert(Combo_SOHStartSoaring(&play,d)==1 && completed==d && spawns==0);
        skipSoaring=0;completed=-1;
        assert(Combo_SOHStartSoaring(&play,d)==1 && completed==-1 && spawns==1);
    }
    assert(!Combo_SOHStartSoaring(&play,-1) && !Combo_SOHStartSoaring(&play,16));
    // Remember Save Location must not override the owl landing with an idle
    // spawn. Ordinary resumes retain their saved entrance untouched.
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance=42;
    assert(MM_ConsumeSongArrival()==-1 && gSaveContext.save.shipSaveInfo.pauseSaveEntrance==42);
    for(int d=6;d<16;++d) {
        sSongArrival=d;gSaveContext.save.shipSaveInfo.pauseSaveEntrance=42;
        assert(MM_ConsumeSongArrival()==100+d-6 && sSongArrival==-1);
        assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance==-1);
    }
    for(int owl=0;owl<10;++owl) {
        Actor actor;actor.params=OCARINA_MODE_WARP_TO_GREAT_BAY_COAST+owl;
        play.sceneId=0;play.nextEntrance=-1;bool should=true;remapped=true;routeCalls=0;
        SkipSoaringCutscene(&actor,&should);
        assert(!should && route==owl && routeCalls==1 && play.nextEntrance==-1);
        remapped=false;should=true;
        SkipSoaringCutscene(&actor,&should);
        assert(!should && play.nextEntrance==100+owl);
    }
    Actor actor;actor.params=ENTEST7_ARRIVE;bool should=true;routeCalls=0;
    SkipSoaringCutscene(&actor,&should);
    assert(should && routeCalls==0);
    actor.params=OCARINA_MODE_WARP_TO_GREAT_BAY_COAST;play.sceneId=SCENE_SECOM;should=true;
    SkipSoaringCutscene(&actor,&should);
    assert(!should && routeCalls==0 && play.nextEntrance==ENTRANCE(IKANA_CANYON,6));
    actor.params=OCARINA_MODE_WARP_TO_ENTRANCE;play.sceneId=0;restored=false;
    SkipSoaringCutscene(&actor,&should);
    assert(restored && gSaveContext.respawnFlag==-6);
    // The instrument camera and main camera deliberately disagree. Releasing
    // the cutscene must not snap the departure back to the old gameplay view.
    play.cameras[0].eye={1,2,3};play.cameras[0].at={4,5,6};play.cameras[0].fov=40;
    play.cameras[1].eye={11,12,13};play.cameras[1].at={14,15,16};play.cameras[1].fov=61;
    play.player.actor.flags=ACTOR_FLAG_OCARINA_INTERACTION;
    play.player.actor.draw=drawPlayer;
    CrossSongWarp warp;warp.actor.params=3;
    CrossSongWarp_Init(&warp.actor,&play);
    assert(stopped && createdAfterStop && !play.interfaceCtx.bButtonInterfaceDoActionActive);
    assert(!(play.player.actor.flags&ACTOR_FLAG_OCARINA_INTERACTION));
    assert(play.cameras[1].eye.x==11 && play.cameras[1].at.y==15 && play.cameras[1].fov==61);
    assert(gSaveContext.hudVisibility==HUD_VISIBILITY_NONE && warp.actor.room==-1);
    // The close-textbox/interface pass follows actor updates and restores the
    // old HUD. The departure draw must suppress that same frame's overlay.
    gSaveContext.hudVisibility=7;gSaveContext.nextHudVisibility=7;hudAlpha=255;
    play.interfaceCtx.bButtonInterfaceDoActionActive=true;
    CrossSongWarp_HideHud(&play);
    assert(gSaveContext.hudVisibility==HUD_VISIBILITY_NONE && hudAlpha==0);
    assert(!play.interfaceCtx.bButtonInterfaceDoActionActive);
    // Native fading completes by setting next visibility idle while the
    // current visibility remains NONE; the setter alone would be a no-op.
    gSaveContext.nextHudVisibility=0;hudAlpha=255;
    CrossSongWarp_HideHud(&play);
    assert(gSaveContext.nextHudVisibility==HUD_VISIBILITY_NONE && hudAlpha==0);
    CrossSongWarp_Destroy(&warp.actor,&play);
    assert(gSaveContext.hudVisibility==7 && play.player.actor.draw==drawPlayer);
    // OoT's message-close restore has the same ordering. The native instant
    // hide helper must also handle a completed fade with an idle next mode.
    for(int nextMode=7;nextMode>=0;nextMode-=7) {
        gSaveContext.hudVisibilityMode=nextMode ? 7 : HUD_VISIBILITY_NOTHING;
        gSaveContext.nextHudVisibilityMode=nextMode;hudAlpha=255;
        CrossOwl_HideHud(&play);
        assert(gSaveContext.hudVisibilityMode==HUD_VISIBILITY_NOTHING && hudAlpha==0);
    }
    puts("Native cross-warp playback skips, all 16 routed departures, vanilla fallbacks, arrival retention and active-camera handoff passed.");
}
'''
test = work/'song-presentation.cpp'
test.write_text(code, encoding='utf-8')
exe = work/'song-presentation.exe'
if Path(args.compiler).stem.lower() in ('cl', 'clang-cl'):
    command = [args.compiler, '/nologo', '/std:c++20', '/EHsc', '/W4', '/UNDEBUG',
               str(test), '/Fe:'+str(exe), '/Fo:'+str(work/'song-presentation.obj')]
else:
    command = [args.compiler, '-std=c++20', '-Wall', '-Wextra', '-UNDEBUG', str(test), '-o', str(exe)]
compiled = subprocess.run(command, capture_output=True, text=True)
(work/'compile.log').write_text(compiled.stdout+compiled.stderr, encoding='utf-8')
if compiled.returncode:
    print(compiled.stdout+compiled.stderr)
compiled.check_returncode()
subprocess.run([str(exe)], check=True)
