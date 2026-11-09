"""A song or owl departure is dropped once it has left: the next trip into either game stays there.

0.4.15 RC1 (and every build since the cross-game owl routes, 0.4.14 included): Link leaves MM by Song of Soaring for an
owl whose shuffled route leads to an OoT warp pad. MM's return hook (BenPort.cpp, OnGameStateMainStart) takes the
departure at the start of a frame and hands Link to OoT, but the rest of that frame still runs (game.c GameState_Update:
the hook, then the play state's update), and the owl's Soaring cutscene (z_en_test7.c EnTest7_WarpCsWarp, the action it
keeps until the scene ends) asks for its warp on every frame, so it queued the same departure again before MM stopped.
The first later trip into MM, by an OoT warp song to an owl or by the Mask Shop door, then found it queued: about 0.2 s
after arriving Link was sent back to the OoT pad of that old departure. The second trip worked, as the bounce took it.

The fix: entering a game drops a song departure still queued from that game's last frames. MM's entry
(MM_SetComboEntryIsResume, the launcher's call on every way into MM) drops it with the other stale return requests
(Combo_ClearReturnRequests); OoT's (SOH_ResumeGame) drops OoT's own, which a Farore's Wind or dungeon departure leaving in
the same frame leaves queued (SOH_DispatchNativeDeparture takes only its own; MM's has the same gap).

The harness compiles from production text MM's return hook, its song departure and arrival functions, the owl's
EnTest7_WarpCsWarp, MM's Farore's Wind departure queue (FaroresWind.cpp), OoT's departure hook (OTRGlobals.cpp,
OnGameFrameUpdate) with its song departure and arrival functions, SOH_ResumeGame's entry statements, and the
launcher's two return callbacks (ComboShip.cpp Combo_OnMMReturn, Combo_OnOOTSceneSwitch), runs whole frames in each
engine's order, and checks:
- Soaring from MM to an OoT pad lands on that pad; the next trip into MM by an OoT warp song to an owl, and by the
  Mask Shop door, stays in MM (it bounced back to the old pad without the fix);
- a later Soaring departure and an OoT song played in MM still land where their routes lead;
- a song departure queued beside a Farore's Wind departure in the same frame is not acted on by the next trip into
  that game, in MM and in OoT.
No gameplay is claimed.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source = a.source.resolve()
work = a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def read(path):
    return (source/path).read_text(encoding='utf-8-sig')


def block(text, start):
    """text[start:] up to the brace that closes the first '{' after start."""
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' '*len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end+1]
    raise AssertionError('Unterminated production block')


def function(text, signature):
    """The production definition (not a prototype), with C linkage dropped so each engine's copy lives in its own
    namespace."""
    return block(text, hostcxx.find(text, signature + '{')).replace('extern "C" ', '')


benport = read('mm/2s2h/BenPort.cpp')
farore = read('mm/2s2h/Rando/FaroresWind.cpp')
test7 = read('mm/src/overlays/actors/ovl_En_Test7/z_en_test7.c')
mm_game = read('mm/src/code/game.c')
otr = read('soh/soh/OTRGlobals.cpp')
oot_game = read('soh/src/code/game.c')
combo = read('combo/ComboShip.cpp')

# The frame order the harness runs. MM: the return hook, then the play state's update with its actors. OoT: the play
# state's update, then the departure hook, which stops the loop.
mm_update = function(mm_game, 'void GameState_Update(GameState* gameState)')
assert hostcxx.find(mm_update, 'GameInteractor_ExecuteOnGameStateMainStart();') < \
    hostcxx.find(mm_update, 'gameState->main(gameState);'), 'MM no longer runs its return hook before the frame'
oot_main = oot_game[:hostcxx.find(oot_game, 'GameInteractor_ExecuteOnGameFrameUpdate();')]
assert 'gameState->main(gameState);' in oot_main[oot_main.rindex('GameInteractor_ExecuteOnGameStateMainStart();'):]
# The owl keeps EnTest7_WarpCsWarp as its action, which EnTest7_Update runs on every frame.
assert hostcxx.contains(function(test7, 'void EnTest7_Update(Actor* thisx, PlayState* play)'),
                        'this->actionFunc(this, play);')
assert hostcxx.contains(test7, 'EnTest7_SetupAction(this, EnTest7_WarpCsWarp);')
warp_cs = re.sub(r'\bthis\b', 'self', function(test7, 'void EnTest7_WarpCsWarp(EnTest7* this, PlayState* play)'))
assert 'Combo_MMSongWarp(' in warp_cs

hook = hostcxx.find(benport, 'GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>([]() {')
mm_main_start = block(benport, benport.index('[]()', hook))
assert 'gComboReturnCallback(' in mm_main_start and 'sSongDeparture >= 0' in mm_main_start
switch_state = hostcxx.find(otr, 'static int sComboSwitchFileNum = -1;')
oot_hook = hostcxx.find(otr, 'GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {',
                        switch_state)
oot_frame_update = block(otr, otr.index('[]()', oot_hook))
assert 'sSongDeparture >= 6' in oot_frame_update and 'gComboSceneSwitchCallback(' in oot_frame_update
# SOH_ResumeGame's statements from the return slot's choice up to the frame loop's reset: OoT's entry.
resume = function(otr, 'extern "C" COMBO_EXPORT void SOH_ResumeGame(void)')
oot_entry = resume[hostcxx.find(resume, 'gComboReturnFileNum = sComboResetPending'):
                   hostcxx.find(resume, 'SOH_ResetFrameLoopForResume();')]

cpp = work/'cross_game_song_departure_bounce.cpp'
cpp.write_text(r'''
#include "ComboGameMode.h"
#include <cstdio>
#include <iostream>
#include <memory>
#define COMBO_EXPORT
#define COMBO_BUILD 1
#define IS_RANDO true
#define SPDLOG_ERROR(...) ((void)0)
typedef signed char s8;
typedef short s16;
typedef int s32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef float f32;
struct Vec3f { f32 x, y, z; };
enum { TRANS_TRIGGER_OFF, TRANS_TRIGGER_START };
enum { TRANS_TYPE_FADE_BLACK = 2, TRANS_TYPE_FADE_WHITE = 3 };
// The launcher installs one song map in both engines (combo/rando/SongTravel.h).
namespace ComboRando {
struct SongTravel {
    bool enabled = true;
    int destinations[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    int destination(int source) const { return source >= 0 && source < 16 ? destinations[source] : -1; }
};
inline SongTravel& ActiveSongTravel() { static SongTravel map; return map; }
inline constexpr int kOotSongEntrances[] = { 0x600, 0x4F6, 0x604, 0x1F1, 0x568, 0x5F4 };
}
int failures = 0;
void Check(bool ok, const char* what) {
    if (!ok) {
        ++failures;
        std::printf("FAIL: %s\n", what);
    }
}

// MM: the return hook and the song departure (BenPort.cpp), the Farore's Wind departure queue (FaroresWind.cpp) and
// the owl's Soaring cutscene (z_en_test7.c).
namespace Mm {
enum { ENTR_SCENE_GREAT_BAY_COAST = 1, ENTR_SCENE_ZORA_CAPE, ENTR_SCENE_SNOWHEAD, ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER,
       ENTR_SCENE_SOUTH_CLOCK_TOWN, ENTR_SCENE_MILK_ROAD, ENTR_SCENE_WOODFALL, ENTR_SCENE_SOUTHERN_SWAMP_POISONED,
       ENTR_SCENE_IKANA_CANYON, ENTR_SCENE_STONE_TOWER, ENTR_SCENE_SOUTHERN_SWAMP_CLEARED,
       ENTR_SCENE_MOUNTAIN_VILLAGE_SPRING };
#define ENTRANCE(scene, spawn) ((((ENTR_SCENE_##scene) & 0x7F) << 9) | (((spawn) & 0x1F) << 4))
namespace ComboRando {
using ::ComboRando::ActiveSongTravel;
inline constexpr int kMmSongEntrances[] = { ENTRANCE(GREAT_BAY_COAST, 11), ENTRANCE(ZORA_CAPE, 6),
                                            ENTRANCE(SNOWHEAD, 3),         ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8),
                                            ENTRANCE(SOUTH_CLOCK_TOWN, 9), ENTRANCE(MILK_ROAD, 4),
                                            ENTRANCE(WOODFALL, 4),         ENTRANCE(SOUTHERN_SWAMP_POISONED, 10),
                                            ENTRANCE(IKANA_CANYON, 4),     ENTRANCE(STONE_TOWER, 3) };
}
enum { SCENE_SECOM = 0x4B };
enum { WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE = 1, WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE = 2 };
#define CHECK_WEEKEVENTREG(flag) CheckWeekEventReg(flag)
bool CheckWeekEventReg(int) { return false; }
enum { OCARINA_MODE_WARP_TO_GREAT_BAY_COAST = 0x1C, OCARINA_MODE_WARP_TO_ENTRANCE = 0x26 };
enum { RESPAWN_MODE_TOP = 2, PLAYER_START_MODE_OWL = 0xA, NA_BGM_DISABLED = 0xFF, AMBIENCE_ID_DISABLED = 0xFF };
#define PLAYER_PARAMS(startBgCamIndex, initMode) ((((initMode) & 0xF) << 8) | ((startBgCamIndex) & 0xFF))
#define OWL_WARP_CS_GET_OCARINA_MODE(thisx) ((thisx)->params)
struct Actor { s16 params; struct { Vec3f pos; } world; };
struct OwlWarpFeather { int unused; };
struct EnTest7 {
    Actor actor;
    struct { s16 yaw; } windCapsule;
    OwlWarpFeather feathers[1];
};
struct PlayState { s16 sceneId; u16 nextEntrance; u8 transitionTrigger; u8 transitionType; };
struct RespawnData { u16 entrance; s32 playerParams; };
constexpr int GAMEMODE_NORMAL = 0;
struct SaveContext {
    int gameMode = GAMEMODE_NORMAL;
    RespawnData respawn[8];
    s8 respawnFlag;
    u8 seqId, ambienceId;
    struct { struct { s32 pauseSaveEntrance; } shipSaveInfo; } save;
} gSaveContext;
PlayState* gPlayState = nullptr;
static u16 sOwlWarpEntrances[10] = {};
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) { *dest = *src; }
f32 Rand_ZeroOne() { return 0.5f; }
void EnTest7_AddAndChooseFeather(OwlWarpFeather*, Vec3f*, bool) {}
void func_80169F78(PlayState*) {}
namespace Ship {
struct Window { virtual ~Window() = default; };
struct Context {
    std::shared_ptr<Window> window;
    std::shared_ptr<Window> GetWindow() { return window; }
    static Context* GetRawInstance() { static Context context; return &context; }
};
}
namespace Fast {
struct Fast3dWindow : Ship::Window { bool running = true; void SetIsRunning(bool value) { running = value; } };
}
int gMMComboGameMode = COMBO_MODE_COMBINED;
int gComboEntryIsResume = 0;
void* Combo_ResolveSym(const char*, const char*) { return nullptr; }
void ProcessForeignSongReply() {}
void Combo_MMStoreDepartureFlags(PlayState*) {}
void SaveManager_SaveDepartureForCombo() {}
void Environment_StopTime() {}
void (*gComboReturnCallback)(int kind) = nullptr;
// FaroresWind.cpp's queue; the launcher's commit (Combo_CommitNativeDeparture) accepts it.
int sDepartureAge = -1, sDepartureEntrance = -1, sDungeonDepartureAge = -1, sDungeonDepartureEntrance = -1;
int nativeDepartures = 0;
bool ValidAge(int age) { return age == 0 || age == 1; }
''' + function(farore, 'extern "C" COMBO_EXPORT int MM_TakeDungeonDeparture(int* age, int* entrance)') + '\n'
    + function(farore, 'extern "C" COMBO_EXPORT int MM_TakeFaroreDeparture(int* age, int* entrance)') + '\n'
    + function(farore, 'extern "C" COMBO_EXPORT void MM_CancelNativeDeparture()') + '\n'
    + function(farore, 'extern "C" COMBO_EXPORT int MM_FaroreDeparturePending()') + r'''
int MM_DispatchNativeDeparture(PlayState*) {
    MM_CancelNativeDeparture();
    ++nativeDepartures;
    return 1;
}
static int sSongDeparture = -1;
static int sSongArrival = -1;
static bool sComboReturnPending = false;
static bool sComboResetReturnPending = false;
static bool sComboOwlSaveQuitPending = false;
''' + function(benport, 'static void Combo_ClearReturnRequests(void)') + '\n'
    + function(benport, 'extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(int isResume)') + '\n'
    + function(benport, 'extern "C" COMBO_EXPORT int MM_TakeSongDeparture()') + '\n'
    + function(benport, 'extern "C" COMBO_EXPORT void MM_SetSongArrival(int d)') + '\n'
    + function(benport, 'extern "C" int MM_ConsumeSongArrival()') + '\n'
    + function(benport, 'extern "C" void Combo_MMCompleteWarp(PlayState* play, int d)') + '\n'
    + function(benport, 'extern "C" int Combo_MMSongWarp(PlayState* play, int owl)') + '\n'
    + warp_cs + '\n'
    + 'auto MainStart = ' + mm_main_start + r''';
}

// OoT: the departure hook and the song departure (OTRGlobals.cpp), and SOH_ResumeGame's entry.
namespace Oot {
enum { RESPAWN_MODE_RETURN = 1 };
struct PlayState { s16 sceneNum; s32 nextEntranceIndex; u8 transitionTrigger; u8 transitionType; };
struct RespawnData { s32 entranceIndex; s32 playerParams; s32 data; };
struct SaveContext {
    int fileNum;
    s16 savedSceneNum;
    RespawnData respawn[3];
    s32 respawnFlag, cutsceneIndex, nextTransitionType;
} gSaveContext;
struct GameState { bool running = true; } state;
GameState* gGameState = &state;
PlayState* gPlayState = nullptr;
struct SaveManager {
    void SaveFile(int) {}
    void ThreadPoolWait() {}
    static SaveManager* Instance;
};
SaveManager manager;
SaveManager* SaveManager::Instance = &manager;
void Play_SaveSceneFlags(PlayState*) {}
int gComboGameMode = COMBO_MODE_COMBINED;
void (*gComboSceneSwitchCallback)(int fileNum) = nullptr;
s32 gComboReturnFileNum = -1;
static bool sComboResetPending = false;
static bool sComboSwitchPending = false;
static int sComboSwitchFileNum = -1;
// OoT's Farore's Wind queue (its own FaroresWind.cpp); the launcher's commit accepts it.
int farorePending = 0, nativeDepartures = 0;
int SOH_FaroreDeparturePending() { return farorePending; }
int SOH_DispatchNativeDeparture(PlayState*) {
    farorePending = 0;
    ++nativeDepartures;
    return 1;
}
int SOH_TakeFaroreDeparture(int*, int*) { return 0; }
int SOH_TakeDungeonDeparture(int*, int*) { return 0; }
static int sSongDeparture = -1;
static int sSongArrival = -1;
static bool sMmVisitedForSongs = false;
''' + function(otr, 'extern "C" void Combo_SOHCompleteWarp(PlayState* play, int destination)') + '\n'
    + function(otr, 'extern "C" COMBO_EXPORT int SOH_TakeSongDeparture()') + '\n'
    + function(otr, 'extern "C" COMBO_EXPORT void SOH_SetSongArrival(int d)') + '\n'
    + function(otr, 'extern "C" int SOH_ConsumeSongArrival()') + '\n'
    + function(otr, 'extern "C" int Combo_OOTSongWarp(PlayState* play, int source)') + '\n'
    + 'auto FrameUpdate = ' + oot_frame_update + ';\n'
    + 'void Enter() {\n' + oot_entry + '\n}\n' + r'''
}

// The launcher's return callbacks (ComboShip.cpp).
namespace Launcher {
using FaroreDeparture = int (*)(int*, int*);
using FaroreArrival = int (*)(int, int, int);
int Arm(int, int, int) { return 1; }
static int g_mmReturnKind = 0;
static int g_MmSaveInMemorySlot = -1;
static bool g_pendingOOTReturn = false;
static int g_PendingMMFileNum = -1;
static bool FaroreSlot(int fileNum) { return fileNum >= 0 && fileNum <= 2; }
static int (*SOH_TakeSongDeparture)() = Oot::SOH_TakeSongDeparture, (*MM_TakeSongDeparture)() = Mm::MM_TakeSongDeparture;
static FaroreDeparture SOH_TakeFaroreDeparture = Oot::SOH_TakeFaroreDeparture,
                       MM_TakeFaroreDeparture = Mm::MM_TakeFaroreDeparture;
static FaroreArrival SOH_SetFaroreArrivalEntrance = Arm, MM_SetFaroreArrivalEntrance = Arm;
static FaroreDeparture SOH_TakeDungeonDeparture = Oot::SOH_TakeDungeonDeparture,
                       MM_TakeDungeonDeparture = Mm::MM_TakeDungeonDeparture;
static FaroreArrival SOH_SetDungeonArrival = Arm, MM_SetDungeonArrival = Arm;
static void (*SOH_SetSongArrival)(int) = Oot::SOH_SetSongArrival, (*MM_SetSongArrival)(int) = Mm::MM_SetSongArrival;
static void (*MM_SetComboEntryIsResume)(int) = Mm::MM_SetComboEntryIsResume;
''' + function(combo, 'static void Combo_OnOOTSceneSwitch(int fileNum)') + '\n'
    + function(combo, 'static void Combo_OnMMReturn(int kind)') + r'''
}

constexpr int kSlot = 1;
std::shared_ptr<Mm::Fast::Fast3dWindow> window;
Mm::PlayState mmPlay;
Oot::PlayState ootPlay;
Mm::EnTest7 owl;
bool owlActive = false;

// The launcher's loop (ComboShip.cpp main): MM resumes on the shared window with a fresh play state.
void ResumeMm() {
    Launcher::g_pendingOOTReturn = false;
    Launcher::g_MmSaveInMemorySlot = kSlot;
    mmPlay = {};
    Mm::gPlayState = &mmPlay;
    window->running = true;
}
// MM runs up to `frames` frames, each in GameState_Update's order: the return hook, then the play state's update with
// the owl's cutscene. Returns whether MM handed Link to OoT.
bool RunMm(int frames) {
    for (int i = 0; i < frames; ++i) {
        Mm::MainStart();
        if (owlActive)
            Mm::EnTest7_WarpCsWarp(&owl, &mmPlay);
        if (!window->running) {
            owlActive = false; // MM's next start builds a new play state
            return true;
        }
    }
    return false;
}
void ResumeOot() {
    Launcher::g_PendingMMFileNum = -1;
    Oot::gSaveContext.fileNum = kSlot;
    Oot::Enter();
    ootPlay = {};
    Oot::gPlayState = &ootPlay;
    Oot::state.running = true;
}
// OoT runs up to `frames` frames: the play state's update, then the departure hook. Returns whether OoT handed Link
// to MM.
bool RunOot(int frames) {
    for (int i = 0; i < frames; ++i) {
        Oot::FrameUpdate();
        if (!Oot::state.running)
            return true;
    }
    return false;
}
// Song of Soaring in MM, choosing `owlIndex` (0 Great Bay Coast .. 9 Stone Tower): the owl's cutscene ends in
// EnTest7_WarpCsWarp, which it keeps until the scene ends.
void Soar(int owlIndex) {
    owl = {};
    owl.actor.params = s16(Mm::OCARINA_MODE_WARP_TO_GREAT_BAY_COAST + owlIndex);
    owlActive = true;
}

int main() {
    window = std::make_shared<Mm::Fast::Fast3dWindow>();
    Mm::Ship::Context::GetRawInstance()->window = window;
    Mm::gComboReturnCallback = Launcher::Combo_OnMMReturn;
    Oot::gComboSceneSwitchCallback = Launcher::Combo_OnOOTSceneSwitch;
    // The seed's shuffled warps: the Clock Town owl (source 10) leads to the Prelude pad (5) and Prelude to that owl;
    // Serenade (2) leads to the Mountain Village owl (source 9) and that owl to the Lake Hylia pad.
    auto& map = ComboRando::ActiveSongTravel();
    map.destinations[10] = 5;
    map.destinations[5] = 10;
    map.destinations[2] = 9;
    map.destinations[9] = 2;
    const int prelude = ComboRando::kOotSongEntrances[5];

    // The file was last played in MM.
    Mm::MM_SetComboEntryIsResume(1);
    ResumeMm();
    Check(!RunMm(3), "MM stays while nothing departs");

    for (bool maskShop : { false, true }) {
        // Song of Soaring at the Clock Town owl: Link lands on the Prelude pad.
        Soar(4);
        Check(RunMm(3), "Soaring to an OoT pad leaves MM");
        Check(Launcher::g_pendingOOTReturn && Launcher::g_mmReturnKind == 0, "the Soaring departure is a portal return");
        ResumeOot();
        Check(Oot::SOH_ConsumeSongArrival() == prelude, "Soaring to the Prelude pad lands on it");
        Check(!RunOot(3), "OoT stays after the Soaring arrival");
        // Back into MM: Serenade, whose route leads to the Mountain Village owl, or the Mask Shop door.
        if (maskShop) {
            Oot::sComboSwitchPending = true; // OnSceneInit: the Happy Mask Shop
            Oot::sComboSwitchFileNum = kSlot;
        } else {
            Check(Oot::Combo_OOTSongWarp(&ootPlay, 2) == 1, "Serenade takes its shuffled route");
        }
        Check(RunOot(1), "the trip back into MM leaves OoT");
        Check(Launcher::g_PendingMMFileNum == kSlot, "the launcher enters MM for the slot");
        ResumeMm();
        if (maskShop)
            Check(Mm::MM_ConsumeSongArrival() == -1, "the Mask Shop door is no song arrival");
        else
            Check(Mm::MM_ConsumeSongArrival() == Mm::ComboRando::kMmSongEntrances[3],
                  "Serenade lands at the Mountain Village owl");
        Oot::sSongArrival = -1;
        const bool bounced = RunMm(10);
        Check(!bounced, maskShop ? "the first MM entry by the Mask Shop door after Soaring to OoT stays in MM"
                                 : "the first MM entry by a warp song after Soaring to OoT stays in MM");
        if (bounced) {
            std::printf("  MM handed Link back to OoT with song arrival %d (the earlier Soaring departure's pad is 5)\n",
                        Oot::sSongArrival);
            ResumeOot();
            Oot::SOH_ConsumeSongArrival();
            Oot::sComboSwitchPending = true;
            Oot::sComboSwitchFileNum = kSlot;
            RunOot(1);
            ResumeMm();
            Check(!RunMm(10), "the second entry stays (the bounce took the old departure)");
        }
    }

    // An OoT song played in MM (ovl_Combo_Song_Warp asks once) still leaves for its pad, and an owl leading within MM
    // stays in MM and starts the owl's transition.
    Mm::Combo_MMCompleteWarp(&mmPlay, 3);
    Check(RunMm(1), "an OoT song played in MM leaves for OoT");
    ResumeOot();
    Check(Oot::SOH_ConsumeSongArrival() == ComboRando::kOotSongEntrances[3], "the OoT song lands on its pad");
    Oot::sComboSwitchPending = true;
    Oot::sComboSwitchFileNum = kSlot;
    Check(RunOot(1), "the Mask Shop door leaves OoT");
    ResumeMm();
    Check(!RunMm(5), "MM stays after that OoT song");
    Soar(0);
    Check(!RunMm(3) && mmPlay.transitionTrigger == TRANS_TRIGGER_START &&
              mmPlay.nextEntrance == Mm::ComboRando::kMmSongEntrances[0],
          "an owl leading within MM stays in MM and starts its transition");
    owlActive = false;

    // MM: a song departure queued in the frame a Farore's Wind departure leaves. The native departure leaves; the
    // song departure must not act on the next entry into MM.
    ResumeMm();
    Mm::Combo_MMCompleteWarp(&mmPlay, 5);
    Mm::sDepartureAge = 0;
    Mm::sDepartureEntrance = 0x1234;
    Check(RunMm(1) && Mm::nativeDepartures == 1, "the Farore's Wind departure leaves MM");
    ResumeOot();
    Oot::sComboSwitchPending = true;
    Oot::sComboSwitchFileNum = kSlot;
    Check(RunOot(1), "the Mask Shop door leaves OoT");
    ResumeMm();
    Oot::sSongArrival = -1;
    Check(!RunMm(5), "the first MM entry after a Farore's Wind departure from MM stays in MM");

    // OoT: the same, a warp song to an MM owl queued in the frame a Farore's Wind departure leaves OoT.
    Mm::sComboReturnPending = true; // OnSceneInit: the Clock Tower door to South Clock Town
    Check(RunMm(1), "the Clock Tower door leaves MM");
    ResumeOot();
    Check(Oot::Combo_OOTSongWarp(&ootPlay, 2) == 1, "Serenade takes its shuffled route");
    Oot::farorePending = 1;
    Check(RunOot(1) && Oot::nativeDepartures == 1, "the Farore's Wind departure leaves OoT");
    ResumeMm();
    Mm::sComboReturnPending = true; // OnSceneInit: the Clock Tower door to South Clock Town
    Check(RunMm(1), "the Clock Tower door leaves MM");
    Launcher::g_PendingMMFileNum = -1;
    ResumeOot();
    Check(!RunOot(5) && Launcher::g_PendingMMFileNum == -1,
          "the first OoT entry after a Farore's Wind departure from OoT stays in OoT");

    if (failures)
        return 1;
    std::puts("Soaring from MM to an OoT pad, then a warp song or the Mask Shop door into MM: Link stays in MM; later "
              "departures land where their routes lead; a song departure beside a Farore's Wind one is dropped in "
              "both games");
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source/'combo'],
                    msvc=['/utf-8', '/wd4100', '/wd4101', '/wd4127', '/wd4189', '/wd4244', '/wd4505'],
                    gnu=['-Wno-unused-parameter', '-Wno-unused-variable', '-Wno-unused-function',
                         '-Wno-unused-but-set-variable', '-Wno-missing-field-initializers'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
