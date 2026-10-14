"""Run extracted production cross-game song recognition and its refusal where songs are restricted.

OoTMM v32.3 recognises a cross-game song without looking at scene restrictions
(mm/ocarina.c:232-242, oot/ocarina.c:140-146) and refuses it after the
performance: MM shows Soaring's restricted-song text 0x1B95 (mm/ocarina.c:483-488)
and OoT shows "You can't warp here!" 0x88C (oot/ocarina.c:638-640). The
recognisers, MM's MSGMODE_18 and native Soaring text-closing code, and OoT's
song dialogue come from source; the stubs replace only engine services.
"""
from pathlib import Path
import argparse
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def block(text, start):
    """The brace-balanced block whose opening brace is the first one at or after start."""
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def function(text, signature):
    return block(text, hostcxx.find(text, signature))


bridge = (source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
message = (source/'mm/src/code/z_message.c').read_text(encoding='utf-8')
mode18 = message[hostcxx.find(message, 'case MSGMODE_18:') + len('case MSGMODE_18:'):hostcxx.find(message, 'case MSGMODE_19:')]
closing = message[hostcxx.find(message, 'void Message_Update(PlayState* play) {'):]
closing = closing[hostcxx.find(closing, 'case MSGMODE_TEXT_CLOSING:'):]
soaring_closed = block(closing, hostcxx.find(closing, 'if (sLastPlayedSong == OCARINA_SONG_SOARING)'))
globals_cpp = (source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')

common = r'''
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include "rando/ForeignSongs.h"
using u8=uint8_t; using s16=int16_t; using u16=uint16_t; using s32=int32_t;
#define COMBO_BUILD
bool isRando=true;
#define IS_RANDO isRando
namespace ComboRando {
struct Travel {bool foreignSongs=true;};
inline Travel& ActiveSongTravel() {static Travel travel; return travel;}
}
static std::atomic<int> sForeignSongRequest{ -1 }, sForeignSongReply{ -2 };
static std::atomic<bool> sForeignSongPerformed{ false };
int textbox=-1,closed=0;
void Played(uint8_t* pitches,int song) {
    for(int n=0;n<ComboRando::kForeignLengths[song];n++)
        pitches[n]=ComboRando::kSongPitches[ComboRando::kForeignNotes[song][n]];
}
'''

mm = common + r'''
// z64ocarina.h values.
enum { OCARINA_SONG_ELEGY=3, OCARINA_SONG_EPONAS=8, OCARINA_SONG_SOARING=9, OCARINA_SONG_SCARECROW_SPAWN=22 };
enum { OCARINA_ACTION_FREE_PLAY=0x01, OCARINA_ACTION_PROMPT_SONATA=0x12, OCARINA_ACTION_CHECK_SONATA=0x22,
       OCARINA_ACTION_FREE_PLAY_DONE=0x32 };
enum { OCARINA_MODE_ACTIVE=0x01, OCARINA_MODE_EVENT=0x03, OCARINA_MODE_END=0x04,
       OCARINA_MODE_PLAYED_SCARECROW_SPAWN=0x0D, OCARINA_MODE_1B=0x1B, OCARINA_MODE_PROCESS_RESTRICTED_SONG=0x27 };
enum { SCENE_F40=1, SCENE_F41, SCENE_IKANAMAE, SCENE_CASTLE, SCENE_IKNINSIDE, SCENE_IKANA, SCENE_INISIE_N,
       SCENE_INISIE_R, SCENE_INISIE_BS, SCENE_RANDOM, SCENE_REDEAD, SCENE_TOUGITES, SCENE_SECOM, SCENE_CLOCKTOWER=99 };
enum { VB_ELEGY_CHECK_SCENE, PAUSE_ITEM=0, PAUSE_MAP=1, PAUSE_STATE_OWL_WARP_0=2, HUD_VISIBILITY_A_B=3,
       DO_ACTION_STOP=4 };
struct MessageContext {int ocarinaAction=0,ocarinaMode=0,songPlayed=0,lastPlayedSong=0,stateTimer=0,
                       ocarinaSongEffectActive=0;};
struct InterfaceContext {struct {u8 songOfSoaring=0;} restrictions;};
struct PauseContext {int unk_2C8=0,unk_2CA=0,pageIndex=0,state=0; int cursorPoint[5]{};};
struct GameState {};
struct PlayState {GameState state; MessageContext msgCtx; InterfaceContext interfaceCtx; PauseContext pauseCtx;
                  int sceneId=SCENE_CLOCKTOWER;};
PlayState* gPlayState=nullptr;
struct {struct {struct {struct {int owlActivationFlags=1;} playerData;} saveInfo;} save; int prevHudVisibility=0;}
    gSaveContext;
#define MM_HAS_ACTIVATED_OWL_STATUES(flags) ((flags)!=0)
uint32_t liveSources=0;
extern "C" uint32_t MM_GetLiveSongSources() {return liveSources;}
s16 sLastPlayedSong=0xFF;
bool gHorsePlayedEponasSong=false;
int owlMap=0;
void Message_CloseTextbox(PlayState*) {++closed;}
void Message_StartTextbox(PlayState*,u16 id,void*) {textbox=id;}
bool GameInteractor_Should(int,bool should) {return should;}
int Map_CurRoomHasMapI(PlayState*) {return 0;}
void func_800F4A10(PlayState*) {++owlMap;}
void Interface_SetBButtonInterfaceDoAction(PlayState*,int) {}
void GameState_SetFramerateDivisor(GameState*,int) {}
int ShrinkWindow_Letterbox_GetSizeTarget() {return 0;}
void ShrinkWindow_Letterbox_SetSizeTarget(int) {}
void Audio_PlaySfx_PauseMenuOpenOrClose(int) {}
'''
for signature in ('extern "C" int Combo_MMForeignSongPerformanceActive(',
                  'extern "C" void Combo_MMForeignSongPerformanceFinished(',
                  'extern "C" int Combo_MMDropRestrictedForeignSong(',
                  'extern "C" int Combo_MMRecognizeForeignSong('):
    mm += function(bridge, signature)
mm += 'void Mode18(PlayState* play) {\n    MessageContext* msgCtx=&play->msgCtx;\n    switch(0) { case 0:\n'
mm += mode18 + '    }\n}\n'
mm += 'void SoaringTextClosed(PlayState* play) {\n    MessageContext* msgCtx=&play->msgCtx;\n'
mm += '    InterfaceContext* interfaceCtx=&play->interfaceCtx; PauseContext* pauseCtx=&play->pauseCtx;\n'
mm += '    switch(0) { case 0:\n' + soaring_closed + '    }\n}\n'
mm += r'''
int main() {
    static PlayState play;
    gPlayState=&play;
    uint8_t prelude[8]{};
    Played(prelude,5);
    liveSources=1u<<5; // Prelude of Light, playable with MM's instrument and buttons
    for(int restricted=0;restricted<2;++restricted) {
        play=PlayState{};
        play.interfaceCtx.restrictions.songOfSoaring=restricted ? 3 : 0;
        sForeignSongRequest=-1;sForeignSongReply=-2;sForeignSongPerformed=false;
        sLastPlayedSong=0xFF;textbox=-1;closed=0;owlMap=0;
        assert(!Combo_MMDropRestrictedForeignSong()); // no song played
        // The notes are recognised in a restricted scene too (mm/ocarina.c:232-242).
        play.msgCtx.ocarinaAction=OCARINA_ACTION_FREE_PLAY;
        assert(Combo_MMRecognizeForeignSong(prelude,6)==1 && Combo_MMForeignSongPerformanceActive());
        // The performance plays as Soaring, the staff state the recogniser published, and ends in MSGMODE_18.
        play.msgCtx.ocarinaAction=OCARINA_ACTION_FREE_PLAY_DONE;
        play.msgCtx.songPlayed=play.msgCtx.lastPlayedSong=sLastPlayedSong=OCARINA_SONG_SOARING;
        play.msgCtx.ocarinaMode=OCARINA_MODE_ACTIVE;
        play.msgCtx.stateTimer=1;
        Mode18(&play);
        assert(closed==1);
        if(!restricted) {
            // The warp confirmation follows; Soaring's own owl-map action is withheld.
            assert(sForeignSongRequest==5 && sForeignSongPerformed && sLastPlayedSong==0xFF);
            assert(play.msgCtx.ocarinaMode==OCARINA_MODE_ACTIVE);
            continue;
        }
        // Restricted: the song is dropped and finishes as native Soaring...
        assert(sForeignSongRequest==-1 && !sForeignSongPerformed && !Combo_MMForeignSongPerformanceActive());
        assert(sLastPlayedSong==OCARINA_SONG_SOARING && play.msgCtx.ocarinaMode==OCARINA_MODE_EVENT);
        // ...whose restriction, when that textbox closes, shows 0x1B95 (OoTMM mm/ocarina.c:484-487).
        SoaringTextClosed(&play);
        assert(textbox==0x1B95 && play.msgCtx.ocarinaMode==OCARINA_MODE_PROCESS_RESTRICTED_SONG);
        assert(sLastPlayedSong==0xFF && !owlMap);
        // The next performance is recognised again.
        play.msgCtx.ocarinaAction=OCARINA_ACTION_FREE_PLAY;
        assert(Combo_MMRecognizeForeignSong(prelude,6)==1);
    }
    puts("MM: OoT warp songs are recognised where Soaring is restricted and refused with 0x1B95 after playing.");
}
'''

oot = common + r'''
enum { OCARINA_ACTION_FREE_PLAY=1 };
enum { BTN_CRIGHT=0x0001, BTN_CLEFT=0x0002, BTN_CDOWN=0x0004, BTN_CUP=0x0008, BTN_START=0x1000, BTN_B=0x4000,
       BTN_A=0x8000 };
enum { TEXT_STATE_CHOICE=4, TEXT_STATE_EVENT=5, TEXT_STATE_DONE=6, PAUSE_STATE_OWLWARP_SELECT=20 };
struct Input {struct {u16 button=0;} cur,press;};
struct MessageContext {int ocarinaAction=0,ocarinaMode=0,choiceIndex=0; s16 disableWarpSongs=0;};
struct PlayState {
    struct {Input input[4];} state;
    MessageContext msgCtx;
    struct {struct {u8 warpSongs=0;} restrictions;} interfaceCtx;
    struct {int state=0;} pauseCtx;
};
PlayState* gPlayState=nullptr;
bool linkIsChild=true,learned=true,playable=true;
#define LINK_IS_CHILD linkIsChild
namespace Rando {struct Logic {};}
std::shared_ptr<Rando::Logic> LiveSongLogic() {return std::make_shared<Rando::Logic>();}
// The instrument, button and age gates are ForeignSoaringSources's (song_travel_tests, song_live_arrival_tests).
static uint32_t OotSoaringSources(Rando::Logic*,uint32_t knowledge,bool,bool=false) {return playable ? knowledge : 0;}
extern "C" int MM_HasLiveSoaringSong() {return learned;}
void* Combo_ResolveSym(const char* module,const char* name) {
    assert(!std::strcmp(module,"2ship") && !std::strcmp(name,"MM_HasLiveSoaringSong"));
    return reinterpret_cast<void*>(&MM_HasLiveSoaringSong);
}
static bool sNativeSongOpen=false;
static int sNativeSongSource=-1;
bool mapActive=false;
int mapOpened=0,dialogues=0;
extern "C" uint32_t SOH_ForeignSongChoices() {return sForeignSongRequest.load()<0 ? 0 : 1u<<10;} // one owl
int Combo_SOHSoaringMapActive() {return mapActive;}
int Combo_SOHSoaringMapBegin(PlayState*,uint32_t) {++mapOpened;return mapActive=true;}
static void OpenNativeSongDialogue(uint32_t,bool=false) {++dialogues;sNativeSongOpen=true;}
void Message_CloseTextbox(PlayState*) {++closed;}
void Message_StartTextbox(PlayState*,u16 id,void*) {textbox=id;}
int Message_GetState(MessageContext*) {return TEXT_STATE_DONE;}
bool Message_ShouldAdvance(PlayState*) {return true;}
'''
for signature in ('extern "C" int Combo_SOHForeignSongPerformanceActive(',
                  'extern "C" void Combo_SOHForeignSongPerformanceFinished(',
                  'extern "C" int Combo_SOHRecognizeForeignSong(',
                  'static void UpdateNativeSongDialogue('):
    oot += function(globals_cpp, signature)
oot += r'''
int main() {
    static PlayState play;
    gPlayState=&play;
    uint8_t soaring[8]{};
    Played(soaring,6);
    for(int restricted=0;restricted<2;++restricted) {
        play=PlayState{};
        play.interfaceCtx.restrictions.warpSongs=restricted ? 3 : 0;
        play.msgCtx.disableWarpSongs=1; // OoTMM's Soaring never reads the scene's warp-song flag
        sForeignSongRequest=-1;sForeignSongReply=-2;sForeignSongPerformed=false;
        sNativeSongOpen=false;sNativeSongSource=-1;mapActive=false;
        textbox=-1;closed=0;mapOpened=0;dialogues=0;
        // The notes are recognised where warp songs are restricted too (oot/ocarina.c:140-146).
        play.msgCtx.ocarinaAction=OCARINA_ACTION_FREE_PLAY;
        assert(Combo_SOHRecognizeForeignSong(soaring,6)==1 && Combo_SOHForeignSongPerformanceActive());
        Combo_SOHForeignSongPerformanceFinished();
        // Nothing opens while the last note is held.
        play.state.input[0].cur.button=BTN_CUP;
        UpdateNativeSongDialogue();
        assert(textbox==-1 && !mapOpened && !dialogues);
        play.state.input[0].cur.button=0;
        UpdateNativeSongDialogue();
        if(!restricted) {
            assert(mapOpened==1 && textbox==-1 && sForeignSongRequest==6); // the owl map
            continue;
        }
        // Restricted: "You can't warp here!" and the ocarina ends, with no map (oot/ocarina.c:638-640, 912-914).
        assert(textbox==0x88C && play.msgCtx.ocarinaMode==4 && !mapOpened && !dialogues);
        assert(sForeignSongRequest==-1 && sForeignSongReply==-2 && !Combo_SOHForeignSongPerformanceActive());
        UpdateNativeSongDialogue();
        assert(!mapOpened && !dialogues);
        assert(Combo_SOHRecognizeForeignSong(soaring,6)==1); // the next performance is recognised again
    }
    // Unchanged gates: a song that cannot be played is not recognised.
    sForeignSongRequest=-1;playable=false;
    assert(!Combo_SOHRecognizeForeignSong(soaring,6));
    puts("OoT: Soaring is recognised where warp songs are restricted and refused with 0x88C after playing.");
}
'''

for name, code in (('foreign-song-restriction-mm.cpp', mm), ('foreign-song-restriction-oot.cpp', oot)):
    test = work/name
    test.write_text(code, encoding='utf-8')
    exe = hostcxx.build(args.compiler, test, includes=[source/'combo'])
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
