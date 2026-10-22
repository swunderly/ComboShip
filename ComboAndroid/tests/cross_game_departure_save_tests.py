"""A cross-game departure saves the room's and the cycle's scene flags, so they are still set on the return.

OoT: the departure save (soh OTRGlobals.cpp, the OnGameFrameUpdate hook behind the Happy Mask Shop door, a cross-game
warp song or Soaring, and Farore's Wind) wrote the file while the room's chest, switch, clear and collect flags were
still only in play->actorCtx.flags, which Play_Destroy stores after it. The return loads that save (title_setup.c), so
a chest opened or a door unlocked in the departure room was closed again while the spent key stayed spent. OoTMM's
departure is an owl save that stores them first (common/switch.c comboGameSwitch: Save_DoSave(play, SF_OWL); oot/save.c
Save_DoSave: PlayStoreFlags and the scene).

MM: the departure saves (2s2h BenPort.cpp, the OnGameStateMainStart hook behind a song, Farore's Wind and the Clock
Tower door, and Rando/CrossEntrance.inc.h's entrance departure) wrote the Save without the live room's flags. Each
scene load copies the cycle's flags into the saved permanent ones (Interface_Init: Interface_NewDay), but the room
Link leaves from keeps its changes in play->actorCtx.sceneFlags, and the return rebuilds the cycle's flags from the
saved ones (title_setup.c, as Sram_OpenSave does). OoTMM's MM departure stores them (mm/save.c Save_DoSave:
PlayStoreFlags, then PrepareSave), as MM's pause save and the autosave do here (MmStoreCycleSceneFlags).

The harness compiles the production OoT hook, Play_SaveSceneFlags, MM's Play_SaveCycleSceneFlags, the departure store
(CrossEntrance.inc.h MmStoreCycleSceneFlags, FaroresWind.cpp Combo_MMStoreDepartureFlags) and title_setup.c's return
copy, plays a departure from a room with an opened chest and an unlocked door, returns, and checks the flags; the same
hook without the store (the shipped one) loses them. The MM departure sites must store before they save.
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
repo, out = args.source.resolve(), args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def read(path):
    return (repo / path).read_text(encoding='utf-8-sig')


def block(text, start):
    """The brace block that opens at or after `start`, braces included."""
    at = text.index('{', start)
    depth = 0
    for end in range(at, len(text)):
        depth += {'{': 1, '}': -1}.get(text[end], 0)
        if depth == 0:
            return text[at:end + 1]
    raise ValueError('unterminated block')


def function(text, signature):
    first = hostcxx.find(text, signature)
    return text[first:text.index('{', first)] + block(text, first) + '\n'


def ordered(text, *needles):
    at = 0
    for needle in needles:
        try:
            at = hostcxx.find(text, needle, at) + 1
        except ValueError:
            failures.append(f'out of order or missing: {needle}')
            return


# OoT: the departure hook, as one function, and the same hook without the scene flag store (the shipped one).
otr = read('soh/soh/OTRGlobals.cpp')
save_call = hostcxx.find(otr, 'SaveManager::Instance->SaveFile(sComboSwitchFileNum);')
hook_at = otr.rindex('RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]()', 0, save_call)
hook = block(otr, hook_at)
store = re.search(r'\n[ \t]*if \(gPlayState != nullptr\) \{\s*Play_SaveSceneFlags\(gPlayState\);\s*'
                  r'gSaveContext\.savedSceneNum = gPlayState->sceneNum;\s*\}', hook)
check(store, 'the OoT departure hook stores the scene flags and the scene before its save')
shipped = hook.replace(store.group(0), '') if store else hook
oot_play = read('soh/src/code/z_play.c')
save_scene = function(oot_play, 'void Play_SaveSceneFlags(PlayState* play)')

# MM: the departure store, MM's own cycle store, and the return's copy into the cycle.
farore = read('mm/2s2h/Rando/FaroresWind.cpp')
mm_store = function(read('mm/2s2h/Rando/CrossEntrance.inc.h'), 'void MmStoreCycleSceneFlags(PlayState* play)') +     function(farore, 'extern "C" void Combo_MMStoreDepartureFlags(PlayState* play)').replace('extern "C" ', '')
mm_cycle = re.sub(r'\bthis\b', 'self', function(read('mm/src/code/z_play.c'),
                                                'void Play_SaveCycleSceneFlags(PlayState* this)'))
title = read('mm/src/code/title_setup.c')
copy_at = hostcxx.find(title, '// Copy permanent flags into cycle flags, the same way Sram_OpenSave does.')
loop_at = title.index('for (', copy_at)
mm_return = title[loop_at:title.index('{', loop_at)] + block(title, loop_at)

# Every MM departure save stores first: the hook's save and the entrance departure's.
benport = read('mm/2s2h/BenPort.cpp')
main_start = benport[hostcxx.find(benport, 'RegisterGameHook<GameInteractor::OnGameStateMainStart>([]()'):]
ordered(main_start, 'if (!isOwlSaveQuit && !isReset && gSaveContext.gameMode == GAMEMODE_NORMAL) {',
        'Combo_MMStoreDepartureFlags(gPlayState);', 'SaveManager_SaveCurrentForCombo();',
        'MM_DispatchNativeDeparture(gPlayState)')
entrance = function(read('mm/2s2h/Rando/CrossEntrance.inc.h'),
                    'extern "C" COMBO_EXPORT int Combo_MMFinishEntranceDeparture(void* state)')
ordered(entrance, 'MM_ApplyFaroreTransitionAge(request.file, request.age)', 'return false;',
        'MmStoreCycleSceneFlags(play);', 'SaveManager_SaveCurrentForCombo();', 'return true;')
check(hostcxx.contains(read('mm/2s2h/Rando/FaroresWind.h'), 'void Combo_MMStoreDepartureFlags(PlayState* play);'),
      'FaroresWind.h declares the MM departure store for BenPort.cpp')

harness = r'''
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
using u32 = uint32_t;
using s16 = int16_t;
using s32 = int32_t;
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

namespace oot {
// z64save.h SavedSceneFlags and the parts of the save and play state the hook reads.
struct SavedSceneFlags {
    u32 chest, swch, clear, collect, unk, rooms, floors;
};
struct SaveContext {
    s32 fileNum = 0;
    s16 savedSceneNum = 0x34; // Link's House: the last save before this departure
    SavedSceneFlags sceneFlags[124]{};
    int smallKeys = 1;
};
struct PlayState {
    s16 sceneNum = 4; // the Fire Temple
    struct {
        struct {
            u32 swch, tempSwch, chest, clear, tempClear, collect, tempCollect;
        } flags;
    } actorCtx{};
};
struct GameState {
    bool running = true;
};
SaveContext gSaveContext, disk;
PlayState* gPlayState = nullptr;
GameState game, *gGameState = &game;
enum { COMBO_MODE_COMBINED = 0 };
int gComboGameMode = COMBO_MODE_COMBINED;
int sSongDeparture = -1, switched = -1;
bool farore = false, sComboSwitchPending = false;
int sComboSwitchFileNum = -1;
void (*gComboSceneSwitchCallback)(int) = [](int file) { switched = file; };
int SOH_FaroreDeparturePending() { return farore; }
int SOH_DispatchNativeDeparture(PlayState*) { switched = 99; farore = false; return 1; }
struct SaveManager {
    static SaveManager* Instance;
    void SaveFile(int) { disk = gSaveContext; }
    void ThreadPoolWait() {}
};
SaveManager manager;
SaveManager* SaveManager::Instance = &manager;
''' + save_scene + r'''
void DepartureHook() ''' + hook + r'''
void ShippedDepartureHook() ''' + shipped + r'''
} // namespace oot

namespace mm {
// z64save.h's scene flags, the parts of SaveContext they live in, and z64actor.h ActorContextSceneFlags.
struct CycleSceneFlags {
    u32 chest, switch0, switch1, clearedRoom, collectible;
};
struct PermanentSceneFlags {
    u32 chest, switch0, switch1, clearedRoom, collectible, unk_14, rooms;
};
struct SaveContext {
    struct {
        struct {
            PermanentSceneFlags permanentSceneFlags[120];
        } saveInfo;
    } save{};
    CycleSceneFlags cycleSceneFlags[120]{};
};
struct PlayState {
    s16 sceneId = 0x1B; // Woodfall Temple
    struct {
        struct {
            u32 switches[4], chest, clearedRoom, clearedRoomTemp, collectible[4];
        } sceneFlags;
    } actorCtx{};
};
enum { SCENE_INISIE_R = 0x19, SCENE_INISIE_N = 0x18 };
s16 Play_GetOriginalSceneId(s16 sceneId) { return sceneId == SCENE_INISIE_R ? SCENE_INISIE_N : sceneId; }
SaveContext gSaveContext;
decltype(SaveContext::save) disk;
''' + mm_cycle + mm_store + r'''
// title_setup.c's combo entry: a fresh SaveContext (SaveContext_Init clears it), the saved Save, then the copy.
void ReturnToMm() {
    gSaveContext = {};
    gSaveContext.save = disk;
    ''' + mm_return + r'''
}
} // namespace mm

#define REQUIRE(x) do { if (!(x)) { std::fprintf(stderr, "FAIL: %s (line %d)\n", #x, __LINE__); return 1; } } while (0)
enum Route { Song, Farore, Door };

// An OoT room with a chest opened and a door unlocked with a small key, then a departure and the return's load.
static int OotTrip(Route route, bool shipped, bool& kept) {
    using namespace oot;
    gSaveContext = {};
    gSaveContext.fileNum = 2;
    PlayState play;
    gPlayState = &play;
    play.actorCtx.flags.chest = 1u << 3;
    play.actorCtx.flags.swch = 1u << 10;
    play.actorCtx.flags.clear = 1u << 2;
    play.actorCtx.flags.collect = 1u << 5;
    gSaveContext.smallKeys = 0;
    sSongDeparture = route == Song ? 6 : -1;
    farore = route == Farore;
    sComboSwitchPending = route == Door; // the Happy Mask Shop's OnSceneInit sets both
    sComboSwitchFileNum = route == Door ? 2 : -1;
    switched = -1;
    game.running = true;
    (shipped ? ShippedDepartureHook : DepartureHook)();
    REQUIRE(switched == (route == Farore ? 99 : 2) && !game.running);
    // The return loads the departure save and the room reads its flags (Actor_InitContext).
    const auto& room = disk.sceneFlags[play.sceneNum];
    REQUIRE(disk.smallKeys == 0); // the spent key stays spent either way
    kept = room.chest == (1u << 3) && room.swch == (1u << 10) && room.clear == (1u << 2) && room.collect == (1u << 5);
    REQUIRE(kept == (disk.savedSceneNum == play.sceneNum));
    gPlayState = nullptr;
    sSongDeparture = -1;
    return 0;
}

// An MM cycle with a chest opened earlier in another scene (which that scene's exit and the next scene load stored),
// and a chest opened and a door unlocked in this room, then a departure (the store, then the save) and the return.
static int MmTrip(bool store, bool& kept) {
    using namespace mm;
    gSaveContext = {};
    gSaveContext.cycleSceneFlags[0x2D].chest = 1u << 7; // Southern Swamp, opened earlier this cycle
    gSaveContext.save.saveInfo.permanentSceneFlags[0x2D].chest = 1u << 7; // Interface_NewDay at this scene's load
    gSaveContext.save.saveInfo.permanentSceneFlags[0x2D].rooms = 1u << 1;
    PlayState play;
    play.actorCtx.sceneFlags.chest = 1u << 4;
    play.actorCtx.sceneFlags.switches[0] = 1u << 12;
    play.actorCtx.sceneFlags.switches[1] = 1u << 1;
    play.actorCtx.sceneFlags.switches[2] = 1u << 9; // temporary: never saved
    play.actorCtx.sceneFlags.clearedRoom = 1u << 3;
    play.actorCtx.sceneFlags.collectible[0] = 1u << 6;
    if (store)
        Combo_MMStoreDepartureFlags(&play);
    disk = gSaveContext.save; // SaveManager_SaveCurrentForCombo writes the Save
    ReturnToMm();
    const auto& room = gSaveContext.cycleSceneFlags[play.sceneId];
    const auto& swamp = gSaveContext.cycleSceneFlags[0x2D];
    REQUIRE(swamp.chest == (1u << 7)); // an earlier scene's flags survive either way
    kept = room.chest == (1u << 4) && room.switch0 == (1u << 12) && room.switch1 == (1u << 1) &&
           room.clearedRoom == (1u << 3) && room.collectible == (1u << 6);
    REQUIRE(disk.saveInfo.permanentSceneFlags[0x2D].rooms == (1u << 1)); // visited rooms are the save's own
    // Without a play state (outside gameplay) there is no scene to store, and nothing changes.
    gSaveContext = {};
    gSaveContext.cycleSceneFlags[0x2D].chest = 1u << 7;
    Combo_MMStoreDepartureFlags(nullptr);
    REQUIRE(gSaveContext.save.saveInfo.permanentSceneFlags[0x2D].chest == 0);
    return 0;
}

int main() {
    for (Route route : { Song, Farore, Door }) {
        bool kept = false, shippedKept = true;
        REQUIRE(OotTrip(route, false, kept) == 0 && OotTrip(route, true, shippedKept) == 0);
        REQUIRE(kept);
        // The Happy Mask Shop door departs from a room just entered: Play_Destroy stored the room left, and the shop
        // has nothing to lose. A song and Farore's Wind depart from the room itself, and the shipped save lost it.
        REQUIRE(route == Door || !shippedKept);
    }
    bool kept = false, shippedKept = true;
    REQUIRE(MmTrip(true, kept) == 0 && kept);
    REQUIRE(MmTrip(false, shippedKept) == 0 && !shippedKept);
    std::puts("PASS: a departure stores the room's and the cycle's flags; the return finds them in both games");
    return 0;
}
'''

if not failures:
    cpp = out / 'cross_game_departure_save.cpp'
    cpp.write_text(harness, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, std='c++20', warnings=False)
    run = subprocess.run([str(exe)], cwd=out, capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    print(run.stdout + run.stderr, end='')
    check(run.returncode == 0, 'the departure save harness failed')

if failures:
    print('\n'.join('FAIL: ' + failure for failure in failures))
    raise SystemExit(1)
print('PASS: cross-game departure saves keep the scene flags in OoT and MM')
