"""Run the production OoT scene restriction table and warp-song refusal with Ship's table and with OoTMM's.

A seed made since the port follows OoTMM v32.3's scene restriction table records the marker in its "oot" section
(combo/rando/OotSceneRestrictions.h); the launcher binds it for the loaded slot, and the game uses it only in a
randomizer save. oot_scene_restrictions_reference_tests.py pins the bytes to OoTMM's file. This harness compiles
z_parameter.c's sRestrictionFlags and Interface_SetSceneRestrictions, OTRGlobals.cpp's hooks and z_message_PAL.c's
MSGMODE_SONG_PLAYED_ACT from source, with stubs only for engine services.

--check table: every scene of the table, with and without the marker and in a vanilla save, against SoH's unmodified
  function on SoH's table and on OoTMM's (SoH's with the header's bytes written in): a vanilla save and every save
  whose seed has no marker (every seed and save from before) keep SoH's table, and a marked seed has OoTMM's in a
  randomizer save. Then the marker itself: which seeds have it (the committed 0.4.12 corpus seeds do not), the vanilla
  preparation dropping it, and the writers and the slot bind that use it.
--check warp: OoT's warp songs where warpSongs is 3. Vanilla refuses them with "You can't warp here!", Ship's
  randomizer does not, and a randomizer seed on OoTMM's table refuses them again, as OoTMM keeps vanilla's refusal.
  Then each changed place, through the table: the Windmill and Dampe's grave, the Gerudo Training Ground, inside
  Ganon's Castle, Granny's Potion Shop and the Shooting Gallery.
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
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--check', choices=('table', 'warp'), required=True)
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def read(path):
    return (source/path).read_text(encoding='utf-8')


def block(text, start):
    """The brace-balanced block whose opening brace is the first one at or after start."""
    brace = text.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def function(text, signature):
    return block(text, hostcxx.find(text, signature))


def squash(text):
    return re.sub(r'\s+', '', text)


parameter = read('soh/src/code/z_parameter.c')
typedef_end = hostcxx.find(parameter, '} RestrictionFlags;')
typedef = parameter[parameter.rindex('typedef struct {', 0, typedef_end):parameter.index(';', typedef_end) + 1] + '\n'
table = parameter[hostcxx.find(parameter, 'static RestrictionFlags sRestrictionFlags[] = {'):]
table = table[:table.index('};') + 2] + '\n'
restrict = function(parameter, 'void Interface_SetSceneRestrictions(PlayState* play) {')
hook = '#ifdef COMBO_BUILD\n            Combo_ApplyOotSceneRestrictions(play);\n#endif\n'
assert restrict.count(hook) == 1 and restrict.index(hook) < restrict.index('BetterFarore') and \
    restrict.index('osSyncPrintf(VT_RST);') < restrict.index(hook), 'the table hook moved'
assert hostcxx.contains(parameter[:hostcxx.find(parameter, 'void Interface_SetSceneRestrictions(')],
                        '#ifdef COMBO_BUILD') and 'void Combo_ApplyOotSceneRestrictions(PlayState* play);' in parameter
upstream = restrict.replace(hook, '')  # SoH's own function: the hook is the port's only change
globals_cpp = read('soh/soh/OTRGlobals.cpp')
flag = 'static std::atomic<bool> sOotmmSceneRestrictions{ false };\n'
assert globals_cpp.count(flag) == 1
hooks = flag + ''.join(function(globals_cpp, signature) for signature in (
    'extern "C" COMBO_EXPORT void SOH_SetOotmmSceneRestrictions(int enabled) {',
    'extern "C" int Combo_OotmmSceneRestrictions(void) {',
    'extern "C" void Combo_ApplyOotSceneRestrictions(PlayState* play) {'))
scene_table = read('soh/include/tables/scene_table.h')

prelude = r'''
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "rando/OotSceneRestrictions.h"
using u8 = uint8_t; using s16 = int16_t; using u16 = uint16_t; using s32 = int32_t;
#define COMBO_BUILD 1
#define COMBO_EXPORT
#define osSyncPrintf(...) ((void)0)
#define VT_FGCOL(color) ""
#define VT_RST ""
#define CVAR_ENHANCEMENT(name) "gEnhancements." name
static bool isRando = true;
#define IS_RANDO isRando
static int betterFarore = 0;
static int CVarGetInteger(const char* name, int) {
    assert(!std::strcmp(name, "gEnhancements.BetterFarore"));
    return betterFarore;
}
#define DEFINE_SCENE(_0, _1, enumValue, _3, _4, _5) enumValue,
enum SceneID {
@SCENES@
    SCENE_ID_MAX
};
#undef DEFINE_SCENE
struct InterfaceContext {
    struct {
        u8 hGauge, bButton, aButton, bottles, tradeItems, hookshot, ocarina, warpSongs;
        u8 sunsSong, farores, dinsNayrus, all;
    } restrictions;
};
struct MessageContext {
    int stateTimer = 0, lastPlayedSong = 0, ocarinaAction = 0, ocarinaMode = 0;
    s16 disableWarpSongs = 0;
};
struct PlayState {
    InterfaceContext interfaceCtx{};
    MessageContext msgCtx;
    s16 sceneNum = 0;
};
@HOOKS@
// SoH's function on SoH's table, the port's on SoH's table, and SoH's on SoH's table with OoTMM's bytes written in.
namespace soh {
@TYPEDEF@@TABLE@@UPSTREAM@}
namespace port {
@TYPEDEF@@TABLE@@PORT@}
namespace ootmm {
@TYPEDEF@@TABLE@@UPSTREAM@}
using Restrictions = std::array<u8, 12>;
static Restrictions Run(void (*set)(PlayState*), int scene) {
    PlayState play;
    std::memset(&play.interfaceCtx, 0xAA, sizeof(play.interfaceCtx)); // every field must be written
    play.sceneNum = static_cast<s16>(scene);
    set(&play);
    const auto& r = play.interfaceCtx.restrictions;
    return { r.hGauge, r.bButton,  r.aButton,  r.bottles,    r.tradeItems, r.hookshot,
             r.ocarina, r.warpSongs, r.sunsSong, r.farores, r.dinsNayrus, r.all };
}
enum { OCARINA = 6, WARP_SONGS = 7, FARORES = 9 };
// OoTMM's table: SoH's, with the bytes of kOotmmSceneRestrictionBytes written in, as OoTMM writes them into the ROM.
static void WriteOotmmBytes() {
    for (const auto& b : ComboRando::kOotmmSceneRestrictionBytes) {
        int written = 0;
        for (auto& entry : ootmm::sRestrictionFlags) {
            if (entry.scene != b.scene)
                continue;
            (b.field == 1 ? entry.flags1 : b.field == 2 ? entry.flags2 : entry.flags3) = b.value;
            ++written;
        }
        assert(written == 1 && b.field >= 1 && b.field <= 3);
    }
}
// The three tables the player can have: a vanilla save, a randomizer save whose seed has no marker, and a marked one.
enum Table { VANILLA, SHIP_RANDO, OOTMM_RANDO };
static Restrictions Bound(Table table, int scene) {
    isRando = table != VANILLA;
    SOH_SetOotmmSceneRestrictions(table == OOTMM_RANDO);
    return Run(port::Interface_SetSceneRestrictions, scene);
}
'''


def program(main, headers=''):
    text = headers + prelude
    for name, value in (('@SCENES@', scene_table), ('@HOOKS@', hooks), ('@TYPEDEF@', typedef), ('@TABLE@', table),
                        ('@UPSTREAM@', upstream), ('@PORT@', restrict)):
        text = text.replace(name, value)
    return text + main


TABLE_MAIN = r'''
int main(int argc, char** argv) {
    WriteOotmmBytes();
    int scenes = 0, changed[2] = {};
    for (const auto& entry : soh::sRestrictionFlags) {
        if (entry.scene == 0xFF)
            break;
        ++scenes;
        for (betterFarore = 0; betterFarore < 2; ++betterFarore) {
            const auto ship = Run(soh::Interface_SetSceneRestrictions, entry.scene);
            const auto ootmm = Run(ootmm::Interface_SetSceneRestrictions, entry.scene);
            // A vanilla save keeps SoH's table, also with a marked seed bound (a vanilla file has none).
            for (const bool marked : { false, true }) {
                isRando = false;
                SOH_SetOotmmSceneRestrictions(marked);
                assert(Run(port::Interface_SetSceneRestrictions, entry.scene) == ship);
            }
            // A randomizer save whose seed has no marker, every seed and save from before it: SoH's table.
            assert(Bound(SHIP_RANDO, entry.scene) == ship);
            // A randomizer save whose seed has it: OoTMM's.
            assert(Bound(OOTMM_RANDO, entry.scene) == ootmm);
            for (int i = 0; i < 12; ++i)
                changed[betterFarore] += ship[i] != ootmm[i];
        }
    }
    assert(scenes == 100);
    // OoTMM changes six restrictions; Better Farore's Wind (SoH) already lifts two of them in Ship's table.
    assert(changed[0] == 6 && changed[1] == 4);
    betterFarore = 0;
    for (const int scene : { SCENE_GERUDO_TRAINING_GROUND, SCENE_INSIDE_GANONS_CASTLE }) {
        const auto ship = Bound(SHIP_RANDO, scene), ootmm = Bound(OOTMM_RANDO, scene);
        assert(ship[WARP_SONGS] == 3 && ship[FARORES] == 1 && ship[OCARINA] == 0);
        assert(ootmm[WARP_SONGS] == 0 && ootmm[FARORES] == 0 && ootmm[OCARINA] == 0);
    }
    for (const int scene : { SCENE_POTION_SHOP_GRANNY, SCENE_SHOOTING_GALLERY }) {
        const auto ship = Bound(SHIP_RANDO, scene), ootmm = Bound(OOTMM_RANDO, scene);
        assert(ship[OCARINA] == 1 && ship[WARP_SONGS] == 1);
        assert(ootmm[OCARINA] == 0 && ootmm[WARP_SONGS] == 1);
    }
    assert(Bound(SHIP_RANDO, SCENE_WINDMILL_AND_DAMPES_GRAVE) == Bound(OOTMM_RANDO, SCENE_WINDMILL_AND_DAMPES_GRAVE));
    assert(Bound(OOTMM_RANDO, SCENE_WINDMILL_AND_DAMPES_GRAVE)[WARP_SONGS] == 3);

    // Which seeds have the marker: only this build's, in a mode that randomizes OoT.
    using nlohmann::json;
    using ComboRando::SeedUsesOotmmSceneRestrictions;
    json marked = { { "oot", { { "settings", json::object() } } } };
    assert(!SeedUsesOotmmSceneRestrictions(marked) && !SeedUsesOotmmSceneRestrictions(json()));
    ComboRando::StampOotmmSceneRestrictions(marked["oot"]);
    assert(marked["oot"]["sceneRestrictions"] == "ootmm-32.3" && SeedUsesOotmmSceneRestrictions(marked));
    for (const char* mode : { "combined", "oot-randomizer", "mm-randomizer", "oot-vanilla", "mm-vanilla", "unknown" }) {
        auto seed = marked;
        seed["gameMode"] = mode;
        const std::string name = mode;
        assert(SeedUsesOotmmSceneRestrictions(seed) == (name == "combined" || name == "oot-randomizer"));
    }
    for (const json other : { json("ootmm-33.0"), json(1), json(true), json::object() }) {
        auto seed = marked;
        seed["oot"]["sceneRestrictions"] = other;
        assert(!SeedUsesOotmmSceneRestrictions(seed));
    }
    auto top = marked; // the marker belongs to the "oot" section
    top["sceneRestrictions"] = top["oot"]["sceneRestrictions"];
    top["oot"].erase("sceneRestrictions");
    assert(!SeedUsesOotmmSceneRestrictions(top));
    // A vanilla game made from a draft of a marked seed plays with vanilla's table.
    json setup = { { "gameMode", "oot-vanilla" }, { "oot", marked["oot"] } };
    setup["mm"]["settings"] = json::object();
    const auto vanilla = ComboRando::PrepareVanillaRun(setup, json::object(), "seed", 1, 2, 0);
    assert(!vanilla["oot"].contains("sceneRestrictions") && !SeedUsesOotmmSceneRestrictions(vanilla));
    // The seeds of earlier releases have no marker: the committed 0.4.12 corpus.
    for (int i = 1; i < argc; ++i) {
        std::ifstream file(argv[i]);
        const auto seed = json::parse(file);
        assert(seed.contains("oot") && !seed["oot"].contains("sceneRestrictions"));
        assert(!SeedUsesOotmmSceneRestrictions(seed));
    }
    std::printf("PASS %d scenes: vanilla saves and seeds without the marker (%d committed 0.4.12 seeds) keep SoH's "
                "table; marked randomizer seeds have OoTMM's\n", scenes, argc - 1);
}
'''

if args.check == 'table':
    corpus = sorted((source/'ComboAndroid/tests/fixtures/wallmasters-vanilla-v1/seeds').glob('*.json'))
    cpp = work/'scene_restrictions_table.cpp'
    cpp.write_text(program(TABLE_MAIN, '#include "rando/SingleGameFill.h"\n'), encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, warnings=False, includes=[source/'combo', args.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/utf-8', '/bigobj'], exe=work/'table.exe')
    subprocess.run([str(exe), *map(str, corpus)], check=True, timeout=hostcxx.RUN_TIMEOUT)

    # Where the marker is written and read.
    launcher = read('combo/ComboShip.cpp')
    bind = squash(function(launcher, 'static void Combo_OnOOTSaveLoad(int fileNum) {'))
    order = [bind.find(squash(needle)) for needle in (
        'if (SOH_SetOotmmSceneRestrictions) SOH_SetOotmmSceneRestrictions(0);', 'if (!ComboIsValidSlot(fileNum))',
        'const auto originalSeed = c["combo"].value("rando", nlohmann::json::object());',
        'ootmmSceneRestrictions = ComboRando::SeedUsesOotmmSceneRestrictions(originalSeed);',
        'if (SOH_SetOotmmSceneRestrictions) SOH_SetOotmmSceneRestrictions(ootmmSceneRestrictions);',
        'RestoreGameplayRules(rules, fileNum);')]
    assert -1 not in order and order == sorted(order), f'the slot bind no longer sets the table first: {order}'
    assert hostcxx.contains(launcher, 'SOH_SetOotmmSceneRestrictions = (void (*)(int))GetSym(sohModule, '
                                      '"SOH_SetOotmmSceneRestrictions");')
    assert hostcxx.contains(launcher, 'SOH_SetOnLoadSaveCallback(Combo_OnOOTSaveLoad);')
    stamp = 'if (ComboRando::ModeRandomizesOot(gameMode)) ComboRando::StampOotmmSceneRestrictions(consolidated["oot"]);'
    assert squash(launcher).count(squash(stamp)) == 1 and launcher.count('StampOotmmSceneRestrictions(') == 1, \
        'RunComboFill must stamp each seed that randomizes OoT'
    headless = read('combo/ComboRandoHeadless.cpp')
    order = [squash(headless).find(squash(needle)) for needle in (
        'bool ootmmSceneRestrictions = true;', 'mqSource = ComboMq::ReadMqDataSource(oot);',
        'ootmmSceneRestrictions = ComboRando::OotmmSceneRestrictionsMarked(oot);',
        'if (ootmmSceneRestrictions) ComboRando::StampOotmmSceneRestrictions(consolidated["oot"]);')]
    assert -1 not in order and order == sorted(order), 'comborando must stamp new seeds and keep a reproduced one\'s'
    # The launcher binds the table on OnLoadGame, which file select runs as it queues Play_Init, which reads the table.
    assert squash('RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t fileNum) { if (gComboSaveLoadCallback) {') \
        in squash(globals_cpp)
    choose = read('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c')
    load = function(choose, 'void FileChoose_LoadGame(GameState* thisx) {')
    assert hostcxx.find(load, 'SET_NEXT_GAMESTATE(&this->state, Play_Init, PlayState);') < \
        hostcxx.find(load, 'GameInteractor_ExecuteOnLoadGame(gSaveContext.fileNum);')
    assert hostcxx.contains(read('soh/src/code/z_play.c'), 'Interface_SetSceneRestrictions(play);')
else:
    message = read('soh/src/code/z_message_PAL.c')
    act = message[hostcxx.find(message, 'case MSGMODE_SONG_PLAYED_ACT:') + len('case MSGMODE_SONG_PLAYED_ACT:'):
                  hostcxx.find(message, 'case MSGMODE_DISPLAY_SONG_PLAYED:')]
    assert 'extern int Combo_OotmmSceneRestrictions(void);' in message[:hostcxx.find(message, '#endif')]
    assert hostcxx.contains(act, '(interfaceCtx->restrictions.warpSongs == 3 && Combo_OotmmSceneRestrictions()) ||')
    soaring = function(globals_cpp, 'static void UpdateNativeSongDialogue(')
    assert hostcxx.contains(soaring, 'if (gPlayState->interfaceCtx.restrictions.warpSongs) {') and \
        hostcxx.contains(soaring, 'Message_StartTextbox(gPlayState, 0x88C, nullptr);'), 'Soaring\'s refusal moved'
    main = r'''
enum { OCARINA_SONG_SARIAS = 6, OCARINA_SONG_EPONAS = 7, OCARINA_SONG_SCARECROW_SPAWN = 12 };
enum { OCARINA_ACTION_PLAYBACK_MINUET = 0x0F, OCARINA_ACTION_PLAYBACK_SARIA = 0x15, OCARINA_ACTION_CHECK_MINUET = 0x1C,
       OCARINA_ACTION_FREE_PLAY_DONE = 0x29 };
enum { OCARINA_MODE_01 = 1, OCARINA_MODE_03 = 3, OCARINA_MODE_04 = 4, OCARINA_MODE_0B = 0x0B };
struct { u16 eventInf[4]; } gSaveContext{};
static int dreg[100];
#define DREG(n) dreg[n]
static int textbox = -1, closed = 0;
static s16 sLastPlayedSong = 0xFF;
static void Message_StartTextbox(PlayState*, u16 id, void*) { textbox = id; }
static void Message_CloseTextbox(PlayState*) { ++closed; }
static void GameInteractor_ExecuteOnOcarinaSongAction() {}
static int Combo_SOHForeignSongPerformanceActive() { return 0; }
static void Combo_SOHForeignSongPerformanceFinished() {}
static void SongPlayedAct(PlayState* play) {
    MessageContext* msgCtx = &play->msgCtx;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;
    switch (0) {
        case 0:
@ACT@
    }
}
// Plays a learned OoT song to its end, as free play does, with the scene's restrictions; true when it was refused.
static bool Refused(PlayState& play, int song) {
    textbox = -1;
    closed = 0;
    play.msgCtx.stateTimer = 1;
    play.msgCtx.lastPlayedSong = song;
    play.msgCtx.ocarinaAction = OCARINA_ACTION_FREE_PLAY_DONE;
    play.msgCtx.ocarinaMode = 0;
    SongPlayedAct(&play);
    if (song >= OCARINA_SONG_SARIAS) {
        assert(textbox == -1 && closed == 1); // not a warp song
        return false;
    }
    if (textbox == 0x88C) { // "You can't warp here!"
        assert(play.msgCtx.ocarinaMode == OCARINA_MODE_04);
        return true;
    }
    assert(textbox == 0x88D + song && play.msgCtx.ocarinaMode == OCARINA_MODE_01); // "Warp to [place]?"
    return false;
}
int main() {
    // The refusal by itself: where warpSongs is 3, vanilla refuses, Ship's randomizer does not, and a randomizer seed
    // on OoTMM's table refuses as vanilla does. disableWarpSongs refuses everywhere.
    for (int song = 0; song <= OCARINA_SONG_SARIAS; ++song)
        for (int warpSongs = 0; warpSongs < 4; ++warpSongs)
            for (const Table table : { VANILLA, SHIP_RANDO, OOTMM_RANDO })
                for (const bool disabled : { false, true }) {
                    isRando = table != VANILLA;
                    SOH_SetOotmmSceneRestrictions(table == OOTMM_RANDO);
                    PlayState play;
                    play.interfaceCtx.restrictions.warpSongs = static_cast<u8>(warpSongs);
                    play.msgCtx.disableWarpSongs = disabled;
                    const bool refused =
                        song < OCARINA_SONG_SARIAS && (disabled || (warpSongs == 3 && table != SHIP_RANDO));
                    assert(Refused(play, song) == refused);
                }
    // Each changed place, through the table: { scene, vanilla, Ship's randomizer, OoTMM's randomizer }, where 0 is a
    // warp, 1 "You can't warp here!", and 2 an ocarina that cannot be used there.
    struct Place { int scene, outcome[3]; };
    const Place places[] = {
        { SCENE_WINDMILL_AND_DAMPES_GRAVE, { 1, 0, 1 } },
        { SCENE_GERUDO_TRAINING_GROUND, { 1, 0, 0 } },
        { SCENE_INSIDE_GANONS_CASTLE, { 1, 0, 0 } },
        { SCENE_POTION_SHOP_GRANNY, { 2, 2, 0 } },
        { SCENE_SHOOTING_GALLERY, { 2, 2, 0 } },
    };
    for (const auto& place : places)
        for (const Table table : { VANILLA, SHIP_RANDO, OOTMM_RANDO }) {
            const auto restrictions = Bound(table, place.scene);
            PlayState play;
            play.interfaceCtx.restrictions.warpSongs = restrictions[WARP_SONGS];
            const int outcome = restrictions[OCARINA] ? 2 : Refused(play, 0) ? 1 : 0;
            assert(outcome == place.outcome[table]);
            // OoT's Soaring (UpdateNativeSongDialogue) is refused wherever warpSongs is not 0.
            const bool soaring = restrictions[OCARINA] == 0 && restrictions[WARP_SONGS] == 0;
            assert(soaring == (table == OOTMM_RANDO && (place.scene == SCENE_GERUDO_TRAINING_GROUND ||
                                                       place.scene == SCENE_INSIDE_GANONS_CASTLE)));
        }
    std::puts("PASS OoT's warp songs: refused where warpSongs is 3 in vanilla and on OoTMM's table, not in Ship's "
              "randomizer; the Windmill refuses them only on OoTMM's table, which also opens the Training Ground, "
              "inside Ganon's Castle, Granny's and the Shooting Gallery");
}
'''
    cpp = work/'scene_restrictions_warp.cpp'
    cpp.write_text(program(main.replace('@ACT@', act)), encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, warnings=False, includes=[source/'combo', args.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/utf-8'], exe=work/'warp.exe')
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print(f'PASS {args.check}')
