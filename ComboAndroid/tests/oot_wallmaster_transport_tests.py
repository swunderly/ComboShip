"""Execute the real vanilla capture adapter, actor hook and cleared-room condition with native enums.

Source/room parity is independently checked by oot_wallmaster_source_tests.py. No gameplay is claimed.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess
import sys
import hostcxx
from temple_probe_stubs import function

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_oot_wallmaster_spawn as exporter
from export_registry import json_bytes
fixture = json.loads((source/exporter.OUTPUT).read_bytes())
assert fixture['revision'] == exporter.PIN
assert json_bytes(fixture) == (source/exporter.OUTPUT).read_bytes()
assert 'PATCH_START 0x808728c0' in fixture['transportHook']
reference = source.parents[1]/'references/ootmm-stable-32.3'
if reference.exists():
    assert exporter.build(reference) == fixture
    subprocess.run([sys.executable, '-B', str(source/'tools/ootmm/export_oot_wallmaster_spawn.py'),
                    '--reference', str(reference), '--output-root', str(source), '--check'], check=True, timeout=90)
randomizer = source/'soh/soh/Enhancements/randomizer'
runtime = (randomizer/'combo_entrance_runtime.cpp').read_text(encoding='utf-8')
runtime = '\n'.join(line for line in runtime.splitlines() if not line.startswith('#include'))
capture = (source/'soh/src/overlays/actors/ovl_En_Wallmas/z_en_wallmas.c').read_text(encoding='utf-8')
hook_start = capture.index('if (this->timer == 0x1E)')
hook = function(capture[hook_start:], 'if (this->timer == 0x1E)')
hook = re.sub(r'\bthis\b', 'self', hook)
assert 'if (!Combo_OOTWallmasterTransport(play, &self->actor))' in hook
assert hook.count('Play_TriggerRespawn(play);') == 1
actor = (source/'soh/src/code/z_actor.c').read_text(encoding='utf-8')
condition_start = actor.index('if ((objBankIndex < 0) ||')
condition = actor[condition_start+len('if '):actor.index('{', condition_start)].strip()
assert actor.index('Combo_OotSoulPreSpawn(play, actorId, (u16)params) <= 0') < condition_start
play = (source/'soh/src/code/z_play.c').read_text(encoding='utf-8')
assert 'Entrance_OverrideNextIndex' not in play
assert 'Entrance_OverrideNextIndex(' not in runtime
assert 'Play_TriggerRespawn(' not in runtime
# Savewarp's dungeon selection is scene-derived, not a custom saved dungeon-ingress record.
sram = (source/'soh/src/code/z_sram.c').read_text(encoding='utf-8')
assert 'switch (gSaveContext.savedSceneNum)' in sram
assert 'gSaveContext.entranceIndex = sDungeonEntrances[gSaveContext.savedSceneNum];' in sram

cpp = work/'oot_transport.cpp'
cpp.write_text(r'''
#define COMBO_BUILD
#define SPDLOG_DEBUG(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define CVAR_ENHANCEMENT(x) x
#define IS_RANDO isRando
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include "rando/entrances/OotUnifiedLayout.h"
#include "rando/entrances/OotWallmasterSource.h"
#include "rando/entrances/OotNativeArrival.h"
#include "z64scene_enum.h"
#include "z64actor_enum.h"
namespace CE = ComboRando::Entrances;
using Json = nlohmann::json;
bool isRando = true;
int enemyRandomizer = 0;
int CVarGetInteger(const char*, int) { return enemyRandomizer; }
constexpr int RSK_SHUFFLE_ENTRANCES = 1, WMT_SHADOWTAG = 3;
constexpr int TRANS_TRIGGER_START = 20, TRANS_TYPE_FADE_BLACK = 30, NA_SE_OC_ABYSS = 4, ACTORCAT_ENEMY = 5;
struct Actor { int params = 0; signed char room = 0; };
struct PlayState {
    int sceneNum = 0, nextEntranceIndex = -123, transitionTrigger = -456, transitionType = -789;
    struct { struct { int num = 0; } curRoom; } roomCtx;
};
struct Save { int respawnFlag = 77, farore = 88, voidPosition = 99, faroreDungeon = 66; } gSaveContext;
// Cross-Games Farore's Wind's dungeon record (FaroresWind.cpp): a capture forgets it before it sets up the transition.
PlayState* capturing = nullptr;
int dungeonClears = 0;
void Combo_OotFaroreClearDungeon() {
    assert(capturing && capturing->nextEntranceIndex == -123 && capturing->transitionTrigger == -456);
    ++dungeonClears;
    gSaveContext.faroreDungeon = 0;
}
namespace Rando {
struct Context {
    bool shuffle = false;
    static std::shared_ptr<Context> GetInstance() { static auto p = std::make_shared<Context>(); return p; }
    bool GetOption(int) { return shuffle; }
};
namespace ComboEntrances {
CE::OotQuests quests{};
CE::OotQuests ReadQuests() { return quests; }
''' + function((randomizer/'combo_entrance_overlay.cpp').read_text(encoding='utf-8'),
               'bool HasVanillaWallmasterLayout()') + r'''
}
}
''' + runtime + r'''
struct EnWallmas { Actor actor; int timer = 0x1e; };
int respawns = 0, sfx = 0;
void Play_TriggerRespawn(PlayState*) { ++respawns; }
void Sfx_PlaySfxCentered(int) { ++sfx; }
void NativeCapture(EnWallmas* self, PlayState* play) {
''' + hook + r'''
}
struct ActorDB { int category; };
int clearFlag = 0;
int Flags_GetClear(PlayState*, int) { return clearFlag; }
bool SpawnRejected(PlayState* play, int actorId, int category, int objBankIndex) {
    ActorDB db{category}; auto* dbEntry = &db;
    return
''' + condition + r''';
}
void CheckUnchanged(const PlayState& play) {
    assert(play.nextEntranceIndex == -123 && play.transitionTrigger == -456 && play.transitionType == -789);
    assert(gSaveContext.respawnFlag == 77 && gSaveContext.farore == 88 && gSaveContext.voidPosition == 99);
    assert(gSaveContext.faroreDungeon == 66);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    std::ifstream input(argv[1]);
    const auto fixture = Json::parse(input);
    CE::EntranceLayout layout;
    layout.required = {std::string(CE::kVanillaWallmasterCapability)};
    layout.settings.erWallmasters = CE::Mode::OwnGame;
    CE::Overrides overrides;
    for (const auto& c : fixture.at("combinations"))
        if (c.at("id") == "erWallmasters=ownGame;erOneWays=none")
            for (const auto& pair : c.at("sampleLayout"))
                overrides[CE::Find(pair[0].get<std::string>())->name] = CE::Find(pair[1].get<std::string>())->name;
    assert(overrides.size() == 13);
    layout.overrides = overrides;
    PlayState play;
    capturing = &play;
    EnWallmas actor;
    play.sceneNum = SCENE_FOREST_TEMPLE;
    assert(!Combo_OOTWallmasterTransport(&play, &actor.actor)); CheckUnchanged(play);
    NativeCapture(&actor, &play); assert(respawns == 1 && sfx == 1); CheckUnchanged(play);
    CE::ActiveOotUnified().install({{"entranceLayout", CE::EncodeEntranceLayout(layout)}},
                                 CE::CapabilitySet::AtLeast(CE::Stage::Hidden));
    assert(Rando::ComboEntrances::HasVanillaWallmasterLayout());
    const int scenes[] = {SCENE_FOREST_TEMPLE, SCENE_SHADOW_TEMPLE, SCENE_SPIRIT_TEMPLE,
        SCENE_BOTTOM_OF_THE_WELL, SCENE_GERUDO_TRAINING_GROUND, SCENE_INSIDE_GANONS_CASTLE, SCENE_LINKS_HOUSE};
    const CE::OotWallmasterScene contexts[] = {CE::OotWallmasterScene::ForestTemple, CE::OotWallmasterScene::ShadowTemple,
        CE::OotWallmasterScene::SpiritTemple, CE::OotWallmasterScene::BottomOfTheWell,
        CE::OotWallmasterScene::GerudoTrainingGround, CE::OotWallmasterScene::InsideGanonsCastle,
        CE::OotWallmasterScene::Unknown};
    int captures = 0;
    for (size_t s = 0; s < std::size(scenes); ++s) for (unsigned room = 0; room < 256; ++room) {
        play = {}; gSaveContext = {};
        play.sceneNum = scenes[s]; play.roomCtx.curRoom.num = (room + 1) & 255;
        actor.actor.room = static_cast<signed char>(room);
        const auto* source = CE::OotWallmasterSource(contexts[s], static_cast<uint8_t>(room));
        const bool bound = source && CE::OotUnifiedSourceBound(source->name);
        const auto before = respawns;
        NativeCapture(&actor, &play);
        assert(respawns == before + (bound ? 0 : 1));
        if (!bound) { CheckUnchanged(play); continue; }
        const auto destination = CE::Find(overrides.at(source->name))->id;
        const auto native = CE::ResolveOotNativeArrival(destination);
        assert(native && play.nextEntranceIndex == static_cast<int>(native->entrance));
        assert(play.transitionTrigger == TRANS_TRIGGER_START && play.transitionType == TRANS_TYPE_FADE_BLACK);
        assert(gSaveContext.respawnFlag == 0 && gSaveContext.farore == 88 && gSaveContext.voidPosition == 99);
        assert(gSaveContext.faroreDungeon == 0 && dungeonClears == ++captures);
    }
    play = {}; play.sceneNum = SCENE_FOREST_TEMPLE; gSaveContext = {};
    actor.actor.room = 0;
    for (int disabled = 0; disabled < 4; ++disabled) {
        isRando = disabled != 0; enemyRandomizer = disabled == 1;
        Rando::Context::GetInstance()->shuffle = disabled == 2;
        Rando::ComboEntrances::quests[0] = disabled == 3;
        assert(!Combo_OOTWallmasterTransport(&play, &actor.actor)); CheckUnchanged(play);
        assert(!Combo_OOTWallmasterSpawnInClearedRoom(&play, ACTOR_EN_WALLMAS));
    }
    isRando = true; enemyRandomizer = 0; Rando::Context::GetInstance()->shuffle = false;
    Rando::ComboEntrances::quests = {};
    actor.actor.params = WMT_SHADOWTAG;
    assert(!Combo_OOTWallmasterTransport(&play, &actor.actor)); CheckUnchanged(play);
    assert(!Combo_OOTWallmasterTransport(nullptr, &actor.actor));
    assert(!Combo_OOTWallmasterTransport(&play, nullptr));
    for (int object : {-1, 0}) for (int clear : {0, 1}) for (int category : {0, ACTORCAT_ENEMY})
        for (int id : {int(ACTOR_EN_WALLMAS), int(ACTOR_EN_TEST), int(ACTOR_EN_GOMA)}) {
            clearFlag = clear;
            const bool expected = object < 0 || (category == ACTORCAT_ENEMY && clear && id != ACTOR_EN_WALLMAS);
            assert(SpawnRejected(&play, id, category, object) == expected);
        }
    CE::ActiveOotUnified().clear(); clearFlag = 1;
    assert(SpawnRejected(&play, ACTOR_EN_WALLMAS, ACTORCAT_ENEMY, 0));
    // Assumptions alone must never enable live transport or the spawn exception.
    CE::ActiveOotUnifiedAssumptions() = CE::OotUnifiedAssumptions{};
    actor.actor.params = 0;
    assert(!Combo_OOTWallmasterTransport(&play, &actor.actor));
    assert(SpawnRejected(&play, ACTOR_EN_WALLMAS, ACTORCAT_ENEMY, 0));
    CE::ActiveOotUnifiedAssumptions().reset();
    assert(captures > 0 && dungeonClears == captures); // Only a capture that moves Link forgets the dungeon.
    std::cout << "PASS 1792 actor-room captures, native fallbacks, direct arrivals, Farore dungeon record and "
                 "cleared-room spawn guard\n";
}
''', encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[source/'combo', a.json_include, source/'soh/include'],
                   defines=['_CRT_SECURE_NO_WARNINGS'], msvc=['/bigobj', '/Zc:preprocessor'])
subprocess.run([str(exe), str(source/'tools/ootmm/m1-contract.json')], check=True, timeout=hostcxx.RUN_TIMEOUT)

# The C insertion sites also compile and keep their native branch when COMBO_BUILD is absent.
native_cpp = work/'non_combo_capture.cpp'
native_cpp.write_text(r'''
#include <cassert>
struct Actor {};
struct EnWallmas { Actor actor; int timer = 0x1e; };
struct PlayState { struct { struct { int num = 0; } curRoom; } roomCtx; };
int calls = 0;
constexpr int NA_SE_OC_ABYSS = 4, ACTORCAT_ENEMY = 5;
void Sfx_PlaySfxCentered(int) {}
void Play_TriggerRespawn(PlayState*) { ++calls; }
void NativeCapture(EnWallmas* self, PlayState* play) {
''' + hook + r'''
}
struct ActorDB { int category; };
int Flags_GetClear(PlayState*, int) { return 1; }
bool SpawnRejected(PlayState* play, int category, int objBankIndex) {
    ActorDB db{category}; auto* dbEntry = &db;
    return
''' + condition + r''';
}
int main() {
    EnWallmas actor; PlayState play;
    NativeCapture(&actor, &play); assert(calls == 1);
    actor.timer = 29; NativeCapture(&actor, &play); assert(calls == 1);
    assert(SpawnRejected(&play, ACTORCAT_ENEMY, 0));
    assert(SpawnRejected(&play, 0, -1));
    assert(!SpawnRejected(&play, 0, 0));
}
''', encoding='utf-8')
native_exe = hostcxx.build(a.compiler, native_cpp)
subprocess.run([str(native_exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)

# Combo_OotFaroreClearDungeon forgets Cross-Games Farore's Wind's dungeon record as the scene hook does outside a
# dungeon: only with Cross-Games Farore's Wind on, reading the shared copy first and writing the cleared one back.
# Without it, a death after a capture returns to the entrance of the dungeon the capture left. OoTMM v32.3 clears
# that record first (oot/actors/En/En_Wallmas.c:50, comboClearCustomRespawn(CUSTOM_RESPAWN_MODE_DUNGEON_ENTRANCE)).
farore = (source/'soh/soh/Enhancements/CrossWarp/FaroresWind.cpp').read_text(encoding='utf-8')
clear = function(farore, 'extern "C" void Combo_OotFaroreClearDungeon(void)')
outside = function(farore, 'extern "C" void Combo_OotFaroreCaptureDungeon(PlayState* play)')
outside = outside[hostcxx.find(outside, 'default:'):]
for statement in ('gSaveContext.ship.faroresWind.sharedCurrentDungeon = {};',
                  'gSaveContext.ship.faroresWind.currentDungeon = {};'):
    assert hostcxx.contains(outside, statement) and hostcxx.contains(clear, statement), statement
farore_cpp = work/'farore_clear_dungeon.cpp'
farore_cpp.write_text(r'''
#include <cassert>
#include <string>
#include <vector>
struct Respawn { int entranceIndex = 0, playerParams = 0; bool operator==(const Respawn&) const = default; };
struct Shared { bool valid = false; int game = 0, entrance = 0, playerParams = 0;
                bool operator==(const Shared&) const = default; };
struct { struct { struct { Respawn currentDungeon; Shared sharedCurrentDungeon; } faroresWind; } ship; } gSaveContext;
const Respawn forest{0x169, 0xdff};
Shared store{true, 0, 0x169, 0xdff}; // the copy both games share
bool sCross = false;
std::vector<std::string> calls;
void ReadDungeon() { calls.push_back("read"); gSaveContext.ship.faroresWind.sharedCurrentDungeon = store; }
void WriteDungeon() {
    calls.push_back("write");
    assert(gSaveContext.ship.faroresWind.currentDungeon == Respawn{});
    store = gSaveContext.ship.faroresWind.sharedCurrentDungeon;
}
''' + clear + r'''
int main() {
    gSaveContext.ship.faroresWind.currentDungeon = forest;
    Combo_OotFaroreClearDungeon(); // Cross-Games Farore's Wind off: its record is not in use
    assert(calls.empty() && gSaveContext.ship.faroresWind.currentDungeon == forest && store.valid);
    sCross = true;
    Combo_OotFaroreClearDungeon();
    assert((calls == std::vector<std::string>{"read", "write"}) && store == Shared{});
    assert(gSaveContext.ship.faroresWind.currentDungeon == Respawn{} &&
           gSaveContext.ship.faroresWind.sharedCurrentDungeon == Shared{});
}
''', encoding='utf-8')
farore_exe = hostcxx.build(a.compiler, farore_cpp)
subprocess.run([str(farore_exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
