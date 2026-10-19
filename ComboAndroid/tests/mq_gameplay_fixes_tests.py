"""OoTMM v32.3's smaller Master Quest gameplay rules for a dungeon that is MQ from the bundled data (MQ port unit U6).

Besides the scene and room data (mq.c), OoTMM reads its MQ flag (Config_IsMq) in a few gameplay rules. This harness
compiles the production code each rule touches, with engine stubs, and runs it for a bundled MQ dungeon, a dungeon
from a real oot-mq.o2r (archive) and a vanilla one:
- MQ Water's locked doors: OoTMM's bins put transition 0x16 on switch 2 (scripts/make_mq.ts:226-229), so a bundled seed
  counts {2, 21} as spent keys (dungeon.cpp GetDoorFlags), and FixWaterMQLock.cpp, which moves the archive's door from
  switch 3 to 4, leaves a bundled door alone.
- MQ Shadow's switch 7 on a new save (src/oot/save.c:136-139): Randomizer_InitSaveFile sets it for a bundled MQ
  Shadow, after Sram_InitSave has made the file a randomizer file, so the resolver reads the seed.
- Fire's first door (0x17) and Water's water-raise door (0x15), which OoTMM opens only in vanilla dungeons
  (src/oot/doors.c:47-53): SoH's new-save switches stay off for a bundled seed's MQ dungeons, through the real resolver.
- MQ Spirit's room-6 silver blocks (src/oot/actors/Obj/Obj_Oshihiki.c:5-13): for a child, Obj_Oshihiki's Init reads
  switch 0x09 as unset in a bundled MQ Spirit, and SoH's kill hook (hook_handlers.cpp) runs only for archive MQ. The
  three blocks come from the bins.
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default='')
args = parser.parse_args()
repo = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import hostcxx  # noqa: E402
import mq_bins  # noqa: E402

rando = repo / 'soh/soh/Enhancements/randomizer'
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def read(path):
    return Path(path).read_text(encoding='utf-8')


def function(text, signature):
    """The definition that starts at `signature` (whitespace-insensitive), through its closing brace."""
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    depth = 0
    for at in range(opening, len(text)):
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            return text[start:at + 1]
    raise AssertionError(f'unbalanced body after {signature}')


def statement(text, needle):
    """The whole line holding `needle`, through the next ';'."""
    start = text.rindex('\n', 0, hostcxx.find(text, needle)) + 1
    return text[start:text.index(';', start) + 1]


def block(text, needle):
    """The whole line holding `needle` (in an `if (...) {` line), through its closing brace."""
    start = text.rindex('\n', 0, hostcxx.find(text, needle)) + 1
    return function(text[start:], text[start:text.index('\n', start)].strip())


# --- MQ Water's locked doors -----------------------------------------------------------------------------------------
dungeon_cpp = read(rando / 'dungeon.cpp')
rows = {}
for key in ('FIRE_TEMPLE', 'WATER_TEMPLE'):
    row = dungeon_cpp[hostcxx.find(dungeon_cpp, f'dungeonList[{key}] ='):]
    row = row[:row.index(');')]
    rows[key] = re.findall(r'\{([\d,\s]*)\}', row)[-3:]  # vanillaDoorFlags, randoDoorFlags, MQDoorFlags
water_fix = read(repo / 'soh/soh/Enhancements/Fixes/FixWaterMQLock.cpp')

doors = r'''#include <cstdint>
#include <cstdio>
#include <vector>
#define COMBO_BUILD 1
using s8 = int8_t; using s16 = int16_t; using s32 = int32_t; using u8 = uint8_t; using u16 = uint16_t;
using u32 = uint32_t;
enum SceneID { SCENE_FIRE_TEMPLE = 4, SCENE_WATER_TEMPLE = 5 };
struct SavedSceneFlags { u32 chest, swch, clear, collect; };
struct SaveContext { SavedSceneFlags sceneFlags[16]; };
struct PlayState { s16 sceneNum; struct { struct { u32 swch; } flags; } actorCtx; };
struct Actor { s16 params; };
struct EnDoor { Actor actor; };
PlayState* gPlayState = nullptr;
bool gRando = true, gGameMq = false, gBundled = false;
#define IS_RANDO (gRando)
extern "C" uint32_t ResourceMgr_SceneUsesBundledMq(s16) { return gBundled; }
extern "C" uint32_t ResourceMgr_GameUsesBundledMq() { return gBundled; }
extern "C" uint32_t ResourceMgr_IsGameMasterQuest() { return gGameMq; }
struct DungeonInfo {
    bool masterQuest;
    SceneID scene;
    std::vector<uint8_t> vanillaDoorFlags, randoDoorFlags, MQDoorFlags;
    bool IsMQ() const { return masterQuest; }
    const std::vector<uint8_t>* GetDoorFlags() const;
};
'''
doors += statement(dungeon_cpp, 'static const std::vector<uint8_t> kBundledMqWaterDoorFlags') + '\n'
doors += function(dungeon_cpp, 'const std::vector<uint8_t>* DungeonInfo::GetDoorFlags() const') + '\n'
doors += function(dungeon_cpp, 'int8_t FindUsedSmallKeys(') + '\n'
doors += function(water_fix, 'static void OnInitEnDoor(void* refActor)') + '\n'
doors += r'''#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "failed: %s at line %d\n", #x, __LINE__); return 1; } } while (0)
using Flags = std::vector<uint8_t>;
int main() {
    DungeonInfo water{ false, SCENE_WATER_TEMPLE, { @WATER0@ }, { @WATER1@ }, { @WATER2@ } };
    DungeonInfo fire{ false, SCENE_FIRE_TEMPLE, { @FIRE0@ }, { @FIRE1@ }, { @FIRE2@ } };
    // A bundled MQ Water seed spends its keys on OoTMM's switches; a real oot-mq.o2r keeps SoH's.
    water.masterQuest = true;
    gBundled = true;
    REQUIRE(*water.GetDoorFlags() == (Flags{ 2, 21 }));
    gBundled = false;
    REQUIRE(*water.GetDoorFlags() == water.MQDoorFlags && water.MQDoorFlags == (Flags{ 4, 21 }));
    // Vanilla Water and every other dungeon keep their rows, bundled or not.
    water.masterQuest = false;
    gBundled = true;
    REQUIRE(*water.GetDoorFlags() == water.randoDoorFlags);
    gRando = false;
    REQUIRE(*water.GetDoorFlags() == water.vanillaDoorFlags);
    gRando = true;
    fire.masterQuest = true;
    REQUIRE(*fire.GetDoorFlags() == fire.MQDoorFlags);
    // The key OoTMM's door 0x16 took counts as spent: one used key with switch 2 set, in the scene or saved.
    water.masterQuest = true;
    SaveContext save{};
    save.sceneFlags[SCENE_WATER_TEMPLE].swch = 1u << 2;
    REQUIRE(FindUsedSmallKeys(&save, SCENE_WATER_TEMPLE, water.GetDoorFlags()) == 1);
    PlayState play{ SCENE_WATER_TEMPLE, { { (1u << 2) | (1u << 21) } } };
    gPlayState = &play;
    REQUIRE(FindUsedSmallKeys(&save, SCENE_WATER_TEMPLE, water.GetDoorFlags()) == 2);
    gBundled = false;
    REQUIRE(FindUsedSmallKeys(&save, SCENE_WATER_TEMPLE, water.GetDoorFlags()) == 1);
    // FixWaterMQLock moves the archive's door 0x16 (switch 3) to switch 4, and leaves a bundled door alone.
    EnDoor door{ { 22659 } };
    gGameMq = true;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22660);
    gBundled = true;
    door.actor.params = 22659;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22659);
    door.actor.params = 22658;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22658);
    gGameMq = false;
    gBundled = false;
    door.actor.params = 22659;
    OnInitEnDoor(&door);
    REQUIRE(door.actor.params == 22659);
    puts("doors: bundled MQ Water counts {2, 21}, a real oot-mq.o2r {4, 21} with FixWaterMQLock");
    return 0;
}
'''
for key, prefix in (('WATER_TEMPLE', 'WATER'), ('FIRE_TEMPLE', 'FIRE')):
    for index, flags in enumerate(rows[key]):
        doors = doors.replace(f'@{prefix}{index}@', flags)

# --- A new save's MQ flags (savefile.cpp Randomizer_InitSaveFile) ---------------------------------------------------
savefile = read(rando / 'savefile.cpp')
init_save = function(savefile, 'extern "C" void Randomizer_InitSaveFile()')
shadow = block(init_save, 'ResourceMgr_SceneUsesBundledMq(SCENE_SHADOW_TEMPLE)')
# The resolver sees the seed's MQ choice there: Sram_InitSave makes the file a randomizer file first (IS_RANDO).
sram = read(repo / 'soh/src/code/z_sram.c')
check(re.search(r'gSaveContext\.ship\.quest\.id = QUEST_RANDOMIZER;\s*Randomizer_InitSaveFile\(\);', sram),
      'z_sram.c: Sram_InitSave must set the randomizer quest just before Randomizer_InitSaveFile')

new_save = r'''#include <cstdint>
#include <cstdio>
#define COMBO_BUILD 1
using s16 = int16_t; using u32 = uint32_t;
enum SceneID { SCENE_FIRE_TEMPLE = 4, SCENE_WATER_TEMPLE = 5, SCENE_SHADOW_TEMPLE = 7 };
struct SavedSceneFlags { u32 chest, swch, clear, collect; };
struct { SavedSceneFlags sceneFlags[16]; } gSaveContext;
u32 gBundledScenes = 0; // bit per scene: ResourceMgr_SceneUsesBundledMq
extern "C" uint32_t ResourceMgr_SceneUsesBundledMq(s16 scene) { return (gBundledScenes >> scene) & 1; }
void ShadowSwitch() {
''' + shadow + r'''
}
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "failed: %s at line %d\n", #x, __LINE__); return 1; } } while (0)
int main() {
    // OoTMM's switch 7 for a bundled MQ Shadow (save.c:136-139), on top of what the save already has.
    gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch = 1u << 21;
    gBundledScenes = 1u << SCENE_SHADOW_TEMPLE;
    ShadowSwitch();
    REQUIRE(gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch == ((1u << 21) | (1u << 7)));
    // Vanilla or archive MQ Shadow (another dungeon bundled) leaves the save alone.
    gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch = 0;
    gBundledScenes = 1u << SCENE_WATER_TEMPLE;
    ShadowSwitch();
    gBundledScenes = 0;
    ShadowSwitch();
    REQUIRE(gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch == 0 && gSaveContext.sceneFlags[SCENE_WATER_TEMPLE].swch == 0);
    puts("new save: bundled MQ Shadow starts with switch 7, archive MQ and vanilla without");
    return 0;
}
'''

# OoTMM opens two doors only in vanilla dungeons (src/oot/doors.c:47-53): Fire's first door (switch 0x17, without small
# key shuffle) and Water's water-raise door (0x15). SoH sets those switches on a new save, also only when the dungeon is
# not MQ (savefile.cpp). Here the production lines run on the real resolver (combo/mq/MqMode.h) with the inputs a new
# randomizer save has, so a bundled seed's MQ dungeons keep both doors locked.
keysanity = statement(init_save, 'u8 keysanity =')
fire_door = block(init_save, 'ResourceMgr_IsSceneMasterQuest(SCENE_FIRE_TEMPLE)')
water_door = block(init_save, 'ResourceMgr_IsSceneMasterQuest(SCENE_WATER_TEMPLE)')
check('(1 << 0x17)' in fire_door and '!keysanity' in fire_door and '(1 << 0x15)' in water_door,
      'savefile.cpp: the Fire and Water door switches moved')
save_doors = r'''#include <cstdint>
#include <cstdio>
#include "mq/MqMode.h"
#define COMBO_BUILD 1
using s16 = int16_t; using u8 = uint8_t; using u32 = uint32_t;
enum SceneID { SCENE_FIRE_TEMPLE = 4, SCENE_WATER_TEMPLE = 5, SCENE_SHADOW_TEMPLE = 7 };
enum { RSK_KEYSANITY };
enum { RO_DUNGEON_ITEM_LOC_STARTWITH, RO_DUNGEON_ITEM_LOC_VANILLA, RO_DUNGEON_ITEM_LOC_OWN_DUNGEON,
       RO_DUNGEON_ITEM_LOC_ANY_DUNGEON, RO_DUNGEON_ITEM_LOC_OVERWORLD, RO_DUNGEON_ITEM_LOC_ANYWHERE };
struct SavedSceneFlags { u32 chest, swch, clear, collect; };
struct { SavedSceneFlags sceneFlags[16]; } gSaveContext;
int gKeysanity = RO_DUNGEON_ITEM_LOC_OWN_DUNGEON;
int Randomizer_GetSettingValue(int) { return gKeysanity; }
// ResourceManagerHelpers.cpp's two predicates, on the resolver's inputs for each scene.
ComboMq::MqModeInputs gInputs;
u32 gMqScenes = 0; // the seed's MQ dungeons (DungeonInfo::IsMQ), a bit per scene
static ComboMq::MqModeInputs InputsFor(s16 scene) {
    ComboMq::MqModeInputs in = gInputs;
    in.dungeon = ((gMqScenes >> scene) & 1) ? ComboMq::DungeonQuest::MasterQuest : ComboMq::DungeonQuest::Vanilla;
    return in;
}
extern "C" uint32_t ResourceMgr_IsSceneMasterQuest(s16 scene) { return ComboMq::ResolveMqMode(InputsFor(scene)).gameplayMq; }
extern "C" uint32_t ResourceMgr_SceneUsesBundledMq(s16 scene) {
    return ComboMq::ResolveMqMode(InputsFor(scene)).source == ComboMq::MqDataSource::Bundled;
}
// Randomizer_InitSaveFile's MQ-dependent lines, in their production order.
static void NewSave() {
    for (auto& flags : gSaveContext.sceneFlags)
        flags = {};
''' + '\n'.join((keysanity, fire_door, water_door, shadow)) + r'''
}
static bool Swch(int scene, int flag) { return (gSaveContext.sceneFlags[scene].swch >> flag) & 1; }
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "failed: %s at line %d\n", #x, __LINE__); return 1; } } while (0)
int main() {
    const u32 mq = (1u << SCENE_FIRE_TEMPLE) | (1u << SCENE_WATER_TEMPLE) | (1u << SCENE_SHADOW_TEMPLE);
    // A new randomizer save of a bundled seed, on an install with the bundled data and no oot-mq.o2r.
    gInputs.hasOriginal = true;
    gInputs.bundledAvailable = true;
    gInputs.quest = ComboMq::QuestKind::Randomizer;
    gInputs.seedSource = ComboMq::SeedMqSource::Bundled;
    gMqScenes = mq;
    NewSave();
    REQUIRE(!Swch(SCENE_FIRE_TEMPLE, 0x17) && !Swch(SCENE_WATER_TEMPLE, 0x15) && Swch(SCENE_SHADOW_TEMPLE, 7));
    gMqScenes = 0;
    NewSave();
    REQUIRE(Swch(SCENE_FIRE_TEMPLE, 0x17) && Swch(SCENE_WATER_TEMPLE, 0x15) && !Swch(SCENE_SHADOW_TEMPLE, 7));
    gKeysanity = RO_DUNGEON_ITEM_LOC_ANYWHERE;
    NewSave();
    REQUIRE(!Swch(SCENE_FIRE_TEMPLE, 0x17) && Swch(SCENE_WATER_TEMPLE, 0x15));
    gKeysanity = RO_DUNGEON_ITEM_LOC_OWN_DUNGEON;
    // A seed without the marker on an install with a real oot-mq.o2r: SoH's doors, no switch 7.
    gInputs.seedSource = ComboMq::SeedMqSource::Legacy;
    gInputs.hasArchiveMq = true;
    gMqScenes = mq;
    NewSave();
    REQUIRE(!Swch(SCENE_FIRE_TEMPLE, 0x17) && !Swch(SCENE_WATER_TEMPLE, 0x15) && !Swch(SCENE_SHADOW_TEMPLE, 7));
    // Why Sram_InitSave sets the quest first: before it, a bundled seed's MQ dungeons would read as vanilla.
    gInputs.seedSource = ComboMq::SeedMqSource::Bundled;
    gInputs.hasArchiveMq = false;
    gInputs.quest = ComboMq::QuestKind::Other;
    NewSave();
    REQUIRE(Swch(SCENE_FIRE_TEMPLE, 0x17) && Swch(SCENE_WATER_TEMPLE, 0x15) && !Swch(SCENE_SHADOW_TEMPLE, 7));
    puts("save doors: a bundled seed's MQ Fire and Water keep doors 0x17 and 0x15 locked, as OoTMM's doors.c does");
    return 0;
}
'''

# --- MQ Spirit's silver blocks in room 6 ---------------------------------------------------------------------------
# OoTMM patches the switch read in Obj_Oshihiki's Init (0x80a4f574, the call after ObjOshihiki_CheckType in the
# overlay at 0x80a4efc0): a child in MQ Spirit reads switch 0x09 as unset (src/oot/actors/Obj/Obj_Oshihiki.c:5-13).
# SoH instead kills the block in the hole for a child (hook_handlers.cpp). Bundled MQ follows OoTMM; archive keeps SoH.
oshihiki = read(repo / 'soh/src/overlays/actors/ovl_Obj_Oshihiki/z_obj_oshihiki.c')
hooks = read(rando / 'hook_handlers.cpp')
actor_init = function(hooks, 'void RandomizerOnActorInitHandler(void* actorRef)')
spirit_hook = block(actor_init, 'if (actor->id == ACTOR_OBJ_OSHIHIKI && LINK_IS_CHILD')
check('Actor_Kill(actor);' in spirit_hook and 'actor->params == 0x9C7' in spirit_hook,
      'hook_handlers.cpp: SoH\'s MQ Spirit block hook moved')
decoded = mq_bins.decode(*mq_bins.load(repo / 'combo/mq/data'))
blocks = [actor[7] & 0xFFFF for room in decoded['rooms'] if (room['dungeon'], room['room']) == (6, 6)
          for actor in room['actors'] if actor[0] & 0xFFFF == 0x00FF]
check(blocks == [0x09C3, 0x09C7, 0x3FC3], f'MQ Spirit room 6 push blocks changed: {[hex(b) for b in blocks]}')
init_code = function(oshihiki, 'void ObjOshihiki_Init(Actor* thisx, PlayState* play2)')
init_code = '\n'.join(line for line in init_code.splitlines() if 'osSyncPrintf' not in line and '//' not in line)

spirit = r'''#include <cstdint>
#include <cstdio>
#define COMBO_BUILD 1
using s8 = int8_t; using s16 = int16_t; using s32 = int32_t; using u8 = uint8_t; using u32 = uint32_t;
enum { SCENE_FOREST_TEMPLE = 3, SCENE_SPIRIT_TEMPLE = 6 };
enum { ACTOR_OBJ_OSHIHIKI = 0x00FF };
enum { PUSHBLOCK_SMALL_START_ON, PUSHBLOCK_MEDIUM_START_ON, PUSHBLOCK_LARGE_START_ON, PUSHBLOCK_HUGE_START_ON,
       PUSHBLOCK_SMALL_START_OFF, PUSHBLOCK_MEDIUM_START_OFF, PUSHBLOCK_LARGE_START_OFF, PUSHBLOCK_HUGE_START_OFF };
#define MASS_IMMOVABLE 0xFF
#define LINK_AGE_ADULT 0
#define LINK_AGE_CHILD 1
struct { s32 linkAge; } gSaveContext;
#define LINK_IS_CHILD (gSaveContext.linkAge == LINK_AGE_CHILD)
struct Actor { s16 id; s16 params; s8 room; struct { u8 mass; } colChkInfo; bool killed; };
struct DynaPolyActor { Actor actor; };
struct ObjOshihiki { DynaPolyActor dyna; };
struct PlayState { s16 sceneNum; u32 swch; };
PlayState* gPlayState = nullptr;
bool gGameMq = false, gBundled = false;
extern "C" uint32_t ResourceMgr_IsGameMasterQuest() { return gGameMq; }
extern "C" uint32_t ResourceMgr_GameUsesBundledMq() { return gBundled; }
extern "C" uint32_t ResourceMgr_SceneUsesBundledMq(s16) { return gBundled; }
s32 Flags_GetSwitch(PlayState* play, s32 flag) { return flag < 32 ? (play->swch >> flag) & 1 : 0; }
void Actor_Kill(Actor* actor) { actor->killed = true; }
static int sInitChain[1];
void Actor_ProcessInitChain(Actor*, int*) {}
void ObjOshihiki_CheckType(ObjOshihiki*, PlayState*) {}
void ObjOshihiki_SetScale(ObjOshihiki*, PlayState*) {}
void ObjOshihiki_SetTexture(ObjOshihiki*, PlayState*) {}
void ObjOshihiki_SetColor(ObjOshihiki*, PlayState*) {}
void ObjOshihiki_RestoreFireTempleBlock(ObjOshihiki*, PlayState*) {}
void ObjOshihiki_ResetFloors(ObjOshihiki*) {}
void ObjOshihiki_SetupOnActor(ObjOshihiki*, PlayState*) {}
''' + function(oshihiki, 'static s32 ObjOshihiki_GetSwitchFlag(PlayState* play, s32 flag)') + '\n' + init_code + r'''
void SpiritBlockHook(Actor* actor) {
''' + spirit_hook + r'''
}
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "failed: %s at line %d\n", #x, __LINE__); return 1; } } while (0)
static const s16 kBlocks[] = { @BLOCKS@ };
// Spawns MQ Spirit room 6's push blocks as the room list does (Init, then the randomizer's actor-init hook) and
// returns which of them stay: bit i for kBlocks[i].
static int Spawn(s16 scene, bool child, bool switch9) {
    PlayState play{ scene, switch9 ? 1u << 9 : 0u };
    gPlayState = &play;
    gSaveContext.linkAge = child ? LINK_AGE_CHILD : LINK_AGE_ADULT;
    int present = 0;
    for (int i = 0; i < (int)(sizeof(kBlocks) / sizeof(kBlocks[0])); i++) {
        ObjOshihiki block{ { { ACTOR_OBJ_OSHIHIKI, kBlocks[i], 6, { 0 }, false } } };
        ObjOshihiki_Init(&block.dyna.actor, &play);
        SpiritBlockHook(&block.dyna.actor);
        present |= block.dyna.actor.killed ? 0 : 1 << i;
    }
    return present;
}
int main() {
    const int above = 1, hole = 2, fixed = 4; // 0x9C3 (start on), 0x9C7 (start off), 0x3FC3 (no switch)
    // Bundled MQ: OoTMM's read. A child sees the room as before any push, whatever switch 0x09 says.
    gGameMq = gBundled = true;
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, true, true) == (above | fixed));
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, true, false) == (above | fixed));
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, false, true) == (hole | fixed));
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, false, false) == (above | fixed));
    // Another dungeon's blocks read their real switch.
    REQUIRE(Spawn(SCENE_FOREST_TEMPLE, true, true) == (hole | fixed));
    // A real oot-mq.o2r keeps SoH's hook: a child after the push finds neither block.
    gBundled = false;
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, true, true) == fixed);
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, true, false) == (above | fixed));
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, false, true) == (hole | fixed));
    // Vanilla: native reads.
    gGameMq = false;
    REQUIRE(Spawn(SCENE_SPIRIT_TEMPLE, true, true) == (hole | fixed));
    puts("spirit block: a bundled MQ child reads switch 0x09 unset (OoTMM); archive MQ keeps SoH's kill hook");
    return 0;
}
'''
spirit = re.sub(r'\bthis\b', 'self', spirit).replace('@BLOCKS@', ', '.join(hex(b) for b in blocks))

PROGRAMS = {'doors': doors, 'new_save': new_save, 'save_doors': save_doors, 'spirit_block': spirit}
for name, code in PROGRAMS.items():
    source = work / f'mq_gameplay_{name}.cpp'
    source.write_text(code, encoding='utf-8')
    exe = hostcxx.build(args.compiler or hostcxx.DEFAULT_COMPILER, source, warnings=False, includes=[repo / 'combo'])
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    print(result.stdout.strip())
    check(result.returncode == 0, f'{name}: {result.stdout}{result.stderr}'.strip())

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: OoTMM\'s MQ gameplay rules hold for bundled MQ, and archive MQ and vanilla keep SoH\'s')
