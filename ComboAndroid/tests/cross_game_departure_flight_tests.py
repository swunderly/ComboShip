"""Execute the cross-game entrance flight's departure end (CrossEntrance.inc.h) in both engines, with the real flight
(NativeArrival.h), OoT's real Play_SaveSceneFlags and TitleSetup_InitImpl, and engine stubs.

- The OoT departure's save first stores the scene's live chest, switch, clear and collect flags and the scene number,
  as OoTMM v32.3's game switch does (common/switch.c:75-85, oot/save.c:398-414 Save_DoSave(SF_OWL), PlayStoreFlags):
  a chest opened, a door unlocked and a key picked up in the room Link leaves are in the save the other game's trip
  returns to. A rolled-back commit saves them too.
- A departure belongs to the play state that queued it. A reset or a quit during the capture's fade (OnExitGame), any
  end of that play state (OnPlayDestroy) and the title's ordinary boot forget it, so a reloaded file's next transition
  stays in its own game; an arrival armed for OoT survives the combo return's own OnExitGame. Another play state's
  transition never finishes it.
- Each engine logs a pending departure it refuses at the end of the transition (owner decision D7), with the reason,
  and stays silent on an ordinary transition with nothing queued.
No gameplay is claimed.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx
from temple_probe_stubs import function

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def read(path):
    return (source/path).read_text(encoding='utf-8')


oot = read('soh/soh/Enhancements/CrossWarp/CrossEntrance.inc.h')
mm = read('mm/2s2h/Rando/CrossEntrance.inc.h')
for text, name in ((oot, 'Oot'), (mm, 'Mm')):
    # The hooks are registered at boot, as every ShipInit function is.
    assert hostcxx.contains(text, f'RegisterShipInitFunc s{name}EntranceFlightHooks(Register{name}EntranceFlightHooks);')
title = read('soh/src/code/title_setup.c')
assert hostcxx.contains(title, 'extern void SOH_CancelEntranceFlight(void);')
play_c = read('soh/src/code/z_play.c')
# Every OoT and MM transition end that can leave for the other game passes its own play state.
assert play_c.count('if (Combo_OotFinishEntranceDeparture(play)) {') == 4
assert read('mm/src/code/z_play.c').count('if (Combo_MMFinishEntranceDeparture(this)) {') == 4

common = r'''
#define COMBO_BUILD
#define COMBO_EXPORT
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include "rando/entrances/NativeArrival.h"
using s32 = int32_t; using s16 = int16_t; using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t;
std::vector<std::string> logs;
template <class... A> void Log(const std::string& format, A... args) {
    const std::vector<std::string> values{std::string(args)...};
    std::string out;
    size_t next = 0;
    for (size_t i = 0; i < format.size(); ++i) {
        if (format.compare(i, 2, "{}") == 0 && next < values.size()) {
            out += values[next++];
            ++i;
        } else {
            out += format[i];
        }
    }
    logs.push_back(out);
}
#define SPDLOG_WARN(...) Log(__VA_ARGS__)
#define SPDLOG_ERROR(...) Log(__VA_ARGS__)
std::vector<std::function<void(int32_t)>> onExitGame;
std::vector<std::function<void()>> onPlayDestroy;
#define COND_HOOK(hookType, condition, body) \
    do {                                     \
        if (condition)                       \
            hookType##Hooks().push_back(body); \
    } while (0)
auto& OnExitGameHooks() { return onExitGame; }
auto& OnPlayDestroyHooks() { return onPlayDestroy; }
void ExitGame() { for (auto& hook : onExitGame) hook(0); }
void DestroyPlay() { for (auto& hook : onPlayDestroy) hook(); }
int preflightResult = 1, commitResult = 1, preflights = 0, commits = 0;
int Preflight(int, int, int, uint32_t, int, int) { ++preflights; return preflightResult; }
int Commit(int, int, int, uint32_t, int, int) { ++commits; return commitResult; }
bool Logged(const char* reason) {
    return logs.size() == 1 && logs[0].find(reason) != std::string::npos &&
           logs[0].find("refused at the end of its transition") != std::string::npos;
}
'''

# OoT: the departure end, its hooks, the request, the real scene flag store and the title's two boots.
oot_cpp = work/'oot_departure.cpp'
oot_cpp.write_text(common + r'''
enum { SCENE_FOREST_TEMPLE = 3, SCENE_BOMBCHU_BOWLING_ALLEY = 0x4b, GAMEMODE_NORMAL = 0, INFTABLE_SWORDLESS = 0x1d };
enum { ITEM_NONE = 0xff, ITEM_SWORD_KOKIRI = 0x3b, ITEM_SWORD_MASTER = 0x3c, ITEM_SWORD_BGS = 0x3d,
       ITEM_SWORD_BROKEN = 0x55 };
constexpr int ENTR_MARKET_DAY_OUTSIDE_HAPPY_MASK_SHOP = 0x0530;
struct SavedSceneFlags {
    u32 chest = 0, swch = 0, clear = 0, collect = 0, unk = 0, rooms = 0, floors = 0;
};
bool operator==(const SavedSceneFlags& x, const SavedSceneFlags& y) {
    return x.chest == y.chest && x.swch == y.swch && x.clear == y.clear && x.collect == y.collect;
}
struct GameState {
    bool running = true;
    void (*next)(GameState*) = nullptr;
};
struct PlayState {
    GameState state;
    s16 sceneNum = SCENE_FOREST_TEMPLE;
    struct {
        struct {
            u32 swch = 0, tempSwch = 0, chest = 0, clear = 0, tempClear = 0, collect = 0, tempCollect = 0;
        } flags;
    } actorCtx;
};
struct SaveContext {
    int fileNum = 1, gameMode = GAMEMODE_NORMAL, linkAge = 0, entranceIndex = -1, cutsceneIndex = 0xfff1;
    int magic = 48, magicFillTarget = 0, magicLevel = 1;
    struct {
        u8 buttonItems[4] = { ITEM_SWORD_MASTER, 0, 0, 0 };
        u16 equipment = 2;
    } equips;
    SavedSceneFlags sceneFlags[124];
    s16 savedSceneNum = 0x34;
    u8 bgsFlag = 0;
    s16 swordHealth = 0;
} gSaveContext;
PlayState* gPlayState = nullptr;
int aregs[32];
#define AREG(n) aregs[n]
bool swordlessFlag = false;
int bowlingSwitch = 0;
s32 Flags_GetInfTable(s32) { return swordlessFlag; }
void Flags_SetInfTable(s32) { swordlessFlag = true; }
void Flags_UnsetInfTable(s32) { swordlessFlag = false; }
s32 Flags_GetSwitch(PlayState*, s32) { return bowlingSwitch; }
void Interface_LoadItemIcon1(PlayState*, u16) {}
int faroreDeparture = 0;
int SOH_FaroreDeparturePending() { return faroreDeparture; }
// What each save writes: the watched scene's stored flags and the saved scene number, as SaveManager reads them.
struct Saved {
    SavedSceneFlags flags;
    s16 savedScene;
};
std::vector<Saved> saves;
struct SaveManagerImpl {
    void SaveFile(int file) {
        assert(file == gSaveContext.fileNum);
        saves.push_back({ gSaveContext.sceneFlags[SCENE_FOREST_TEMPLE], gSaveContext.savedSceneNum });
    }
    void ThreadPoolWait() {}
} saveManager;
struct SaveManager {
    static SaveManagerImpl* Instance;
};
SaveManagerImpl* SaveManager::Instance = &saveManager;
namespace CEArrival = ComboRando::Entrances;
CEArrival::NativeEntranceFlight sOotEntranceFlight;
''' + function(play_c, 'void Play_SaveSceneFlags(PlayState* play)') + '\n'
   + function(oot, 'void OotReloadEntranceSword(PlayState* play)') + '\n'
   + function(oot, 'void RegisterOotEntranceFlightHooks()') + '\n'
   + function(oot, 'extern "C" COMBO_EXPORT void SOH_CancelEntranceFlight()') + '\n'
   + function(oot, 'extern "C" COMBO_EXPORT int SOH_RequestEntranceDeparture(') + '\n'
   + function(oot, 'extern "C" COMBO_EXPORT int Combo_OotFinishEntranceDeparture(void* state)') + r'''
// The title's boots (title_setup.c TitleSetup_InitImpl), with the loaders as stubs.
#define osSyncPrintf(...) ((void)0)
#define LUSLOG_INFO(...) ((void)0)
#define SET_NEXT_GAMESTATE(state, init, type) ((state)->next = (init))
s32 gComboReturnFileNum = -1;
int openResult = 1, contextInits = 0;
bool Sram_TryOpenSave(int, int) { return openResult; }
void SaveContext_Init() {
    // Nothing of the cross-game flight survives into the file loaded next.
    assert(!sOotEntranceFlight.departurePending() && !sOotEntranceFlight.takeArrival(gSaveContext.fileNum, 0));
    ++contextInits;
}
void Title_Init(GameState*) {}
void FileChoose_Init(GameState*) {}
void Play_Init(GameState*) {}
void SOH_CancelNativeDeparture() {}
void SOH_SetSongArrival(int) {}
int SOH_SetDungeonArrival(int, int, int) { return 0; }
int SOH_SetFaroreArrivalEntrance(int, int, int) { return 0; }
int SOH_ConsumeSongArrival() { return -1; }
int SOH_ConsumeFaroreArrival() { return -1; }
int SOH_ConsumeDungeonArrival() { return -1; }
int SOH_ConsumeEntranceArrival() { return sOotEntranceFlight.takeArrival(gSaveContext.fileNum, 0) ? 0x123 : -1; }
void GameInteractor_ExecuteOnExitGame(int) { ExitGame(); }
void GameInteractor_ExecuteOnLoadGame(int) {}
int Entrance_OverrideNextIndex(int entrance) { return entrance; }
''' + function(title, 'void TitleSetup_InitImpl(GameState* gameState)') + r'''
const SavedSceneFlags kStale{ 0x1, 0x2, 0x4, 0x8 };
void Fresh(PlayState& play) {
    play = {};
    gSaveContext = {};
    gSaveContext.sceneFlags[SCENE_FOREST_TEMPLE] = kStale;
    // This visit: a chest opened, a door unlocked and a small key picked up.
    play.actorCtx.flags.chest = kStale.chest | 0x10;
    play.actorCtx.flags.swch = kStale.swch | 0x20;
    play.actorCtx.flags.clear = kStale.clear | 0x40;
    play.actorCtx.flags.collect = kStale.collect | 0x80;
    gPlayState = &play;
    logs.clear();
    saves.clear();
    preflightResult = commitResult = 1;
    preflights = commits = 0;
}
SavedSceneFlags Live(const PlayState& play) {
    return { play.actorCtx.flags.chest, play.actorCtx.flags.swch, play.actorCtx.flags.clear,
             play.actorCtx.flags.collect };
}
bool Queue() {
    return SOH_RequestEntranceDeparture(gSaveContext.fileNum, 1, 0x40, 0);
}
int main() {
    sOotEntranceFlight.bridge(Preflight, Commit);
    RegisterOotEntranceFlightHooks();
    assert(onExitGame.size() == 1 && onPlayDestroy.size() == 1);
    PlayState play;

    // The departure's save stores this visit's flags and the scene first.
    Fresh(play);
    assert(Queue() && preflights == 1 && sOotEntranceFlight.departurePending());
    assert(Combo_OotFinishEntranceDeparture(&play) == 1 && commits == 1 && logs.empty());
    assert(saves.size() == 1 && saves[0].flags == Live(play) && saves[0].savedScene == SCENE_FOREST_TEMPLE);
    // An ordinary transition, with nothing queued: no save, no log.
    assert(Combo_OotFinishEntranceDeparture(&play) == 0 && saves.size() == 1 && logs.empty());

    // A refused commit rolls back with a second save, which keeps the flags too, and is logged.
    Fresh(play);
    commitResult = 0;
    assert(Queue() && Combo_OotFinishEntranceDeparture(&play) == 0 && commits == 1);
    assert(saves.size() == 2 && saves[0].flags == Live(play) && saves[1].flags == Live(play));
    assert(Logged("the launcher's commit refused it"));

    // The preflight refusing at the end, or the save refusing: no save, logged, nothing stays queued.
    Fresh(play);
    assert(Queue());
    preflightResult = 0;
    assert(Combo_OotFinishEntranceDeparture(&play) == 0 && saves.empty() && commits == 0);
    assert(Logged("the launcher's preflight refused it") && !sOotEntranceFlight.departurePending());
    Fresh(play);
    assert(Queue());
    gSaveContext.equips.equipment = 4;
    assert(Combo_OotFinishEntranceDeparture(&play) == 0 && saves.empty() && commits == 0);
    assert(Logged("the save before leaving refused it"));
    assert(gSaveContext.sceneFlags[SCENE_FOREST_TEMPLE] == kStale); // refused before anything was stored

    // Another play state's transition: refused and logged, and the queuing one can no longer finish it.
    Fresh(play);
    assert(Queue());
    PlayState other;
    gPlayState = &other;
    assert(Combo_OotFinishEntranceDeparture(&play) == 0 && sOotEntranceFlight.departurePending() && logs.empty());
    assert(Combo_OotFinishEntranceDeparture(&other) == 0 && Logged("it was queued by another play state"));
    gPlayState = &play;
    assert(Combo_OotFinishEntranceDeparture(&play) == 0 && saves.empty() && commits == 0 && logs.size() == 1);

    // A reset or a quit during the fade (OnExitGame), then the same file reloaded into a play state at the same
    // address: its first transition stays in OoT. The same after any other end of the play state.
    for (const bool reset : { true, false }) {
        Fresh(play);
        assert(Queue());
        reset ? ExitGame() : DestroyPlay();
        assert(!sOotEntranceFlight.departurePending());
        Fresh(play);
        assert(Combo_OotFinishEntranceDeparture(&play) == 0 && preflights == 0 && commits == 0);
        assert(saves.empty() && logs.empty());
    }
    // OnExitGame forgets the departure only: an arrival armed for OoT stays, and so does a queue made afterwards.
    Fresh(play);
    assert(sOotEntranceFlight.arm({ 1, 1, 0, 0, 0, 0x42 }, 0));
    ExitGame();
    assert(sOotEntranceFlight.takeArrival(1, 0));

    // The title's ordinary boot (a reset, or a quit to the title) forgets everything before it initialises the save
    // context; the combo return keeps the arrival it consumes, though its own OnExitGame forgets a stale departure.
    Fresh(play);
    assert(Queue() && sOotEntranceFlight.arm({ 1, 1, 0, 0, 0, 0x42 }, 0));
    GameState state;
    gComboReturnFileNum = -1;
    TitleSetup_InitImpl(&state);
    assert(contextInits == 1 && state.next == Title_Init && !state.running);
    Fresh(play);
    assert(Queue() && sOotEntranceFlight.arm({ 1, 1, 0, 0, 0, 0x42 }, 0));
    state = {};
    gComboReturnFileNum = 1;
    TitleSetup_InitImpl(&state);
    assert(contextInits == 1 && state.next == Play_Init && gSaveContext.entranceIndex == 0x123);
    assert(!sOotEntranceFlight.departurePending() && gComboReturnFileNum == -1);
    // A rejected file cancels everything, as before.
    Fresh(play);
    assert(Queue() && sOotEntranceFlight.arm({ 1, 1, 0, 0, 0, 0x42 }, 0));
    state = {};
    gComboReturnFileNum = 1;
    openResult = 0;
    TitleSetup_InitImpl(&state);
    assert(state.next == FileChoose_Init && !sOotEntranceFlight.departurePending() &&
           !sOotEntranceFlight.takeArrival(1, 0));
    std::cout << "PASS OoT departure flag store, refusal logs, reset, quit and play-state binding\n";
}
''', encoding='utf-8')
oot_exe = hostcxx.build(a.compiler, oot_cpp, includes=[source/'combo'], warnings=False)
subprocess.run([str(oot_exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)

# MM: the departure end with the real cycle flag store, and the return trip's rebuild of the cycle flags
# (title_setup.c), which every MM entry runs on the save it reloads.
mm_play = read('mm/src/code/z_play.c')
mm_title = read('mm/src/code/title_setup.c')
rebuild = function(mm_title, 'for (int i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); i++) {')
assert hostcxx.contains(rebuild, 'gSaveContext.cycleSceneFlags[i].chest = gSaveContext.save.saveInfo.permanentSceneFlags[i].chest;')
parameter = read('mm/src/code/z_parameter.c')
new_day = function(function(parameter, 'void Interface_NewDay(PlayState* play, s32 day)'),
                   'for (i = 0; i < ARRAY_COUNT(gSaveContext.save.saveInfo.permanentSceneFlags); i++) {')
# Every scene load copies the cycle flags into the saved ones (Play_Init, Interface_Init).
assert hostcxx.contains(function(parameter, 'void Interface_Init(PlayState* play)'), 'Interface_NewDay(play, CURRENT_DAY);')
mm_cpp = work/'mm_departure.cpp'
mm_cpp.write_text(common + r"""
#include <cstring>
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
enum { SCENE_LAST_BS = 0x0b, SCENE_INISIE_R = 0x18, SCENE_INISIE_BS = 0x19, SCENE_MITURIN_BS = 0x1f,
       SCENE_SEA_BS = 0x27, SCENE_HAKUGIN_BS = 0x2c, SCENE_DANPEI2TEST = 0x30, SCENE_REDEAD = 0x4b,
       SCENE_CLOCKTOWER = 0x6f };
enum { PLAYER_FORM_FIERCE_DEITY, PLAYER_FORM_GORON, PLAYER_FORM_ZORA, PLAYER_FORM_DEKU, PLAYER_FORM_HUMAN };
enum { PLAYER_MASK_NONE = 0, GAMEMODE_NORMAL = 0 };
struct CycleSceneFlags {
    u32 chest = 0, switch0 = 0, switch1 = 0, clearedRoom = 0, collectible = 0;
};
struct PermanentSceneFlags {
    u32 chest = 0, switch0 = 0, switch1 = 0, clearedRoom = 0, collectible = 0, unk_14 = 0, rooms = 0;
};
bool Same(const CycleSceneFlags& c, const PermanentSceneFlags& p) {
    return c.chest == p.chest && c.switch0 == p.switch0 && c.switch1 == p.switch1 && c.clearedRoom == p.clearedRoom &&
           c.collectible == p.collectible;
}
bool Same(const CycleSceneFlags& x, const CycleSceneFlags& y) {
    return x.chest == y.chest && x.switch0 == y.switch0 && x.switch1 == y.switch1 && x.clearedRoom == y.clearedRoom &&
           x.collectible == y.collectible;
}
struct PlayState {
    s16 sceneId = SCENE_REDEAD;
    struct {
        struct {
            u32 chest = 0, switches[4] = {}, clearedRoom = 0, collectible[4] = {};
        } sceneFlags;
    } actorCtx;
};
struct Save {
    u8 playerForm = PLAYER_FORM_HUMAN, equippedMask = PLAYER_MASK_NONE, isOwlSave = 0;
    struct {
        PermanentSceneFlags permanentSceneFlags[120];
    } saveInfo;
};
struct SaveContext {
    int fileNum = 1, gameMode = GAMEMODE_NORMAL;
    Save save;
    CycleSceneFlags cycleSceneFlags[120];
} gSaveContext;
PlayState* gPlayState = nullptr;
s16 Play_GetOriginalSceneId(s16 sceneId) { return sceneId; }
int fierceDeityAnywhere = 0, applyResult = 1, applies = 0;
int CVarGetInteger(const char*, int) { return fierceDeityAnywhere; }
int MM_GetEffectiveHumanAge(int) { return 0; }
int MM_CanFaroreTransitionAge(int, int) { return 1; }
int MM_ApplyFaroreTransitionAge(int, int) { ++applies; return applyResult; }
int OotAge(int) { return 1; }
int (*sOotAge)(int) = OotAge;
int faroreDeparture = 0;
int MM_FaroreDeparturePending() { return faroreDeparture; }
// What each save writes: the whole Save struct, as SaveManager_SaveCurrentForCombo does (cycle flags are not in it).
std::vector<Save> saves;
void SaveManager_SaveCurrentForCombo() { saves.push_back(gSaveContext.save); }
namespace CEMmArrival = ComboRando::Entrances;
CEMmArrival::NativeEntranceFlight sMmEntranceFlight;
""" + re.sub(r'\bthis\b', 'self', function(mm_play, 'void Play_SaveCycleSceneFlags(PlayState* this)')) + '\n'
   + function(mm, 'void MmStoreCycleSceneFlags(PlayState* play)') + '\n'
   + function(mm, 'bool MmEntranceNeedsFdCleanup(PlayState* play)') + '\n'
   + function(mm, 'void RegisterMmEntranceFlightHooks()') + '\n'
   + function(mm, 'extern "C" COMBO_EXPORT void MM_CancelEntranceFlight()') + '\n'
   + function(mm, 'extern "C" COMBO_EXPORT int MM_RequestEntranceDeparture(') + '\n'
   + function(mm, 'extern "C" COMBO_EXPORT int Combo_MMFinishEntranceDeparture(void* state)') + '\n'
   + function(mm, 'extern "C" COMBO_EXPORT int MM_ConsumeFinishedEntranceDeparture()') + r"""
// title_setup.c Setup_InitImpl: an MM entry reloads the save into a cleared context and rebuilds the cycle flags.
void ReturnTrip(const Save& saved) {
    gSaveContext = {};
    gSaveContext.save = saved;
    """ + rebuild + r"""
}
// Interface_NewDay's copy, which every scene load runs (Interface_Init).
void NewDay() {
    s16 i;
    """ + new_day + r"""
}
const CycleSceneFlags kEarlier{ 0x1, 0x2, 0x4, 0x8, 0x10 }, kTown{ 0x100, 0, 0x200, 0, 0x400 };
void Fresh(PlayState& play) {
    play = {};
    gSaveContext = {};
    // Earlier this cycle: a visit to Beneath the Well and Clock Town progress, each stored at a scene load.
    gSaveContext.cycleSceneFlags[SCENE_REDEAD] = kEarlier;
    gSaveContext.cycleSceneFlags[SCENE_CLOCKTOWER] = kTown;
    NewDay();
    // This visit: a chest, two switches, a cleared room and a collectible.
    auto& live = play.actorCtx.sceneFlags;
    live.chest = kEarlier.chest | 0x20;
    live.switches[0] = kEarlier.switch0 | 0x40;
    live.switches[1] = kEarlier.switch1 | 0x80;
    live.clearedRoom = kEarlier.clearedRoom | 0x100;
    live.collectible[0] = kEarlier.collectible | 0x200;
    gPlayState = &play;
    logs.clear();
    saves.clear();
    preflightResult = commitResult = applyResult = 1;
    preflights = commits = applies = 0;
}
CycleSceneFlags Live(const PlayState& play) {
    const auto& live = play.actorCtx.sceneFlags;
    return { live.chest, live.switches[0], live.switches[1], live.clearedRoom, live.collectible[0] };
}
bool Queue(int crossAge = 0) {
    return MM_RequestEntranceDeparture(gSaveContext.fileNum, 0, 0x99, crossAge);
}
int main() {
    sMmEntranceFlight.bridge(Preflight, Commit);
    RegisterMmEntranceFlightHooks();
    assert(onExitGame.empty() && onPlayDestroy.size() == 1);
    PlayState play;

    // The store copies exactly what every scene load copies (Interface_NewDay), after storing the live flags.
    Fresh(play);
    for (int scene = 0; scene < 120; ++scene)
        gSaveContext.cycleSceneFlags[scene] = { u32(scene), u32(scene * 3), u32(scene * 5), u32(scene * 7),
                                                u32(scene * 11) };
    gSaveContext.save.saveInfo.permanentSceneFlags[7].rooms = 0x55;
    play.sceneId = SCENE_CLOCKTOWER;
    play.actorCtx.sceneFlags = {};
    play.actorCtx.sceneFlags.chest = gSaveContext.cycleSceneFlags[SCENE_CLOCKTOWER].chest;
    play.actorCtx.sceneFlags.switches[0] = gSaveContext.cycleSceneFlags[SCENE_CLOCKTOWER].switch0;
    play.actorCtx.sceneFlags.switches[1] = gSaveContext.cycleSceneFlags[SCENE_CLOCKTOWER].switch1;
    play.actorCtx.sceneFlags.clearedRoom = gSaveContext.cycleSceneFlags[SCENE_CLOCKTOWER].clearedRoom;
    play.actorCtx.sceneFlags.collectible[0] = gSaveContext.cycleSceneFlags[SCENE_CLOCKTOWER].collectible;
    const auto before = gSaveContext;
    NewDay();
    const auto loaded = gSaveContext.save;
    gSaveContext = before;
    MmStoreCycleSceneFlags(&play);
    assert(std::memcmp(loaded.saveInfo.permanentSceneFlags, gSaveContext.save.saveInfo.permanentSceneFlags,
                       sizeof(loaded.saveInfo.permanentSceneFlags)) == 0);
    assert(gSaveContext.save.saveInfo.permanentSceneFlags[7].rooms == 0x55);

    // MM -> OoT -> MM: the departure's save holds this visit's progress and the cycle's, and the return trip's
    // rebuild gives it back. isOwlSave stays as it was.
    Fresh(play);
    assert(Queue() && preflights == 1);
    assert(Combo_MMFinishEntranceDeparture(&play) == 1 && commits == 1 && logs.empty() && saves.size() == 1);
    assert(Same(Live(play), saves[0].saveInfo.permanentSceneFlags[SCENE_REDEAD]));
    assert(Same(kTown, saves[0].saveInfo.permanentSceneFlags[SCENE_CLOCKTOWER]) && saves[0].isOwlSave == 0);
    // graph.c consumes the finished departure after Play_Destroy.
    DestroyPlay();
    assert(MM_ConsumeFinishedEntranceDeparture() == 1 && MM_ConsumeFinishedEntranceDeparture() == 0);
    const auto saved = saves[0];
    ReturnTrip(saved);
    assert(Same(gSaveContext.cycleSceneFlags[SCENE_REDEAD], Live(play)));
    assert(Same(gSaveContext.cycleSceneFlags[SCENE_CLOCKTOWER], kTown));
    // An ordinary transition, with nothing queued: no save, no log.
    gPlayState = &play;
    assert(Combo_MMFinishEntranceDeparture(&play) == 0 && saves.size() == 1 && logs.empty());

    // A refused commit rolls back with a second save, which keeps the progress, and is logged.
    Fresh(play);
    commitResult = 0;
    assert(Queue() && Combo_MMFinishEntranceDeparture(&play) == 0 && commits == 1 && saves.size() == 2);
    assert(Same(Live(play), saves[1].saveInfo.permanentSceneFlags[SCENE_REDEAD]));
    assert(Logged("the launcher's commit refused it") && MM_ConsumeFinishedEntranceDeparture() == 0);
    // The preflight refusing at the end, or the age refusing in the save: no save, logged, nothing stored.
    Fresh(play);
    assert(Queue());
    preflightResult = 0;
    assert(Combo_MMFinishEntranceDeparture(&play) == 0 && saves.empty());
    assert(Logged("the launcher's preflight refused it"));
    Fresh(play);
    assert(Queue(1));
    applyResult = 0;
    assert(Combo_MMFinishEntranceDeparture(&play) == 0 && saves.empty() && applies == 1);
    assert(Logged("the save before leaving refused it"));
    assert(Same(kEarlier, gSaveContext.save.saveInfo.permanentSceneFlags[SCENE_REDEAD]));
    // Another play state's transition: refused and logged.
    Fresh(play);
    assert(Queue());
    PlayState other;
    gPlayState = &other;
    assert(Combo_MMFinishEntranceDeparture(&other) == 0 && Logged("it was queued by another play state"));
    // Any end of the play state forgets the departure (a play state ending inside MM; a reset or a quit leaves MM
    // through the launcher, which cancels the whole flight), and so does the launcher's cancel.
    for (const bool destroy : { true, false }) {
        Fresh(play);
        assert(Queue());
        destroy ? DestroyPlay() : MM_CancelEntranceFlight();
        Fresh(play);
        assert(Combo_MMFinishEntranceDeparture(&play) == 0 && preflights == 0 && saves.empty() && logs.empty());
    }
    std::cout << "PASS MM departure cycle flag store and return-trip rebuild, refusal logs and play-state binding\n";
}
""", encoding='utf-8')
mm_exe = hostcxx.build(a.compiler, mm_cpp, includes=[source/'combo'], warnings=False)
subprocess.run([str(mm_exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
