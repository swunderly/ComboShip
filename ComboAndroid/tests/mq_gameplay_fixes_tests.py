"""OoTMM v32.3's smaller Master Quest gameplay rules for a dungeon that is MQ from the bundled data (MQ port unit U6).

Besides the scene and room data (mq.c), OoTMM reads its MQ flag (Config_IsMq) in a few gameplay rules. Where a rule
is code, this harness compiles the production lines it touches with engine stubs and runs them for a bundled MQ
dungeon, a dungeon from a real oot-mq.o2r (archive) and a vanilla one; where it is data, it compares SoH's tables with
OoTMM's, re-read from a pinned OoTMM v32.3 checkout when one is beside this one:
- MQ Water's locked doors: OoTMM's bins put transition 0x16 on switch 2 (scripts/make_mq.ts:226-229), so a bundled
  seed counts {2, 21} as spent keys (dungeon.cpp GetDoorFlags), and FixWaterMQLock.cpp, which moves the archive's door
  from switch 3 to 4, leaves a bundled door alone.
- MQ Shadow's switch 7 on a new save (src/oot/save.c:136-139): Randomizer_InitSaveFile sets it for a bundled MQ
  Shadow, after Sram_InitSave has made the file a randomizer file, so the resolver reads the seed.
- Fire's first door (0x17) and Water's water-raise door (0x15), which OoTMM opens only in vanilla dungeons
  (src/oot/doors.c:47-53): SoH's new-save switches stay off for a bundled seed's MQ dungeons, through the real resolver.
- Small-key maxima (src/common/data.c:31-81): randomizer.h's macros give a bundled seed's MQ dungeons OoTMM's MQ
  counts, and DungeonInfo's mqKeyCount agrees.
- MQ scrub prices (packages/logic/src/price.ts:97-130): SoH's MQ scrub rows charge OoTMM's MQ prices item for item;
  by physical scrub only MQ Dodongo's Cavern's shield and potion scrubs differ (OoTMM's pool labels).
- Ruto (src/oot/play/play.c:65-86): OoTMM sets her top-floor flag on Jabu-Jabu entry only when Jabu-Jabu is not MQ;
  Combo never sets it outside En_Ru1, which matches the MQ case.
- Ice Cavern's alternate setup 4, gone in MQ (src/oot/mq.c:328-339): the Serenade check is its flag, which En_Xc sets
  from the main room list whether or not its cutscene (played in place, skipped in the randomizer) runs.
- Chest positions (src/common/csmc/csmc_chest.c:94-119): OoTMM moves MQ Ganon's Castle chest 0x04 after resizing it;
  SoH never resizes chests, so there is nothing to move.
- MQ Spirit's room-6 silver blocks (src/oot/actors/Obj/Obj_Oshihiki.c:5-13): for a child, Obj_Oshihiki's Init reads
  switch 0x09 as unset in a bundled MQ Spirit, and SoH's kill hook (hook_handlers.cpp) runs only for archive MQ. The
  three blocks come from the bins.
"""
from pathlib import Path
import argparse
import hashlib
import os
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


# The OoTMM v32.3 (413573d2) checkout beside this one, if any: the pinned tables below are re-read from it.
reference = os.environ.get('COMBO_OOTMM_REFERENCE')
reference = Path(reference) if reference else next(
    (parent / 'references/ootmm-stable-32.3' for parent in repo.parents
     if (parent / 'references/ootmm-stable-32.3/packages/generator/src/common/sr.c').is_file()), None)


def ootmm_file(path, blob):
    """The text of an OoTMM v32.3 file whose git blob is `blob`, or None without such a checkout."""
    source = reference / path if reference else None
    if not (source and source.is_file()):
        print(f'No OoTMM v32.3 checkout here; {path} was not re-read (set COMBO_OOTMM_REFERENCE)')
        return None
    data = source.read_bytes().replace(b'\r\n', b'\n')  # a core.autocrlf checkout; git hashes the LF form
    if hashlib.sha1(b'blob %d\0' % len(data) + data).hexdigest() != blob:
        print(f'{source} is not the v32.3 blob; the pinned table was not re-read')
        return None
    print(f'OoTMM {path} read from {reference}')
    return data.decode('utf-8')


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
doors += r'''#define REQUIRE(x) \
    do { if (!(x)) { fprintf(stderr, "failed: %s at line %d\n", #x, __LINE__); return 1; } } while (0)
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
    REQUIRE(gSaveContext.sceneFlags[SCENE_SHADOW_TEMPLE].swch == 0 &&
            gSaveContext.sceneFlags[SCENE_WATER_TEMPLE].swch == 0);
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
extern "C" uint32_t ResourceMgr_IsSceneMasterQuest(s16 scene) {
    return ComboMq::ResolveMqMode(InputsFor(scene)).gameplayMq;
}
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

# --- Small-key maxima ----------------------------------------------------------------------------------------------
# OoTMM's MQ key counts (src/common/data.c:31-81, comboInitData). SoH has the same numbers in randomizer.h's
# *_SMALL_KEY_MAX macros (on the gameplay predicate) and DungeonInfo's mqKeyCount, so nothing changes; this runs the
# production macros on the real resolver to show a bundled seed's MQ dungeons get OoTMM's maxima.
OOTMM_MQ_KEYS = {'FOREST_TEMPLE': 6, 'FIRE_TEMPLE': 5, 'WATER_TEMPLE': 2, 'SHADOW_TEMPLE': 6, 'SPIRIT_TEMPLE': 7,
                 'BOTTOM_OF_THE_WELL': 2, 'GERUDO_TRAINING_GROUND': 3, 'INSIDE_GANONS_CASTLE': 3}
OOTMM_SCENE = {'SCE_OOT_TEMPLE_FOREST': 'FOREST_TEMPLE', 'SCE_OOT_TEMPLE_FIRE': 'FIRE_TEMPLE',
               'SCE_OOT_TEMPLE_WATER': 'WATER_TEMPLE', 'SCE_OOT_TEMPLE_SHADOW': 'SHADOW_TEMPLE',
               'SCE_OOT_TEMPLE_SPIRIT': 'SPIRIT_TEMPLE', 'SCE_OOT_BOTTOM_OF_THE_WELL': 'BOTTOM_OF_THE_WELL',
               'SCE_OOT_GERUDO_TRAINING_GROUND': 'GERUDO_TRAINING_GROUND',
               'SCE_OOT_INSIDE_GANON_CASTLE': 'INSIDE_GANONS_CASTLE'}
data_c = ootmm_file('packages/generator/src/common/data.c', 'afa6112bcee5a3b5110ae4ff7f6ea6c769143af8')
if data_c:
    mq_keys = {OOTMM_SCENE[scene]: int(count) for scene, count in re.findall(
        r'if \(Config_IsMq\(MQ_\w+\)\)\s*\{\s*g\.maxKeysOot\[(SCE_OOT_\w+)\] = (\d+);', data_c)}
    check(mq_keys == OOTMM_MQ_KEYS, f'data.c MQ key maxima differ: {mq_keys}')
macros = re.findall(r'#define \w+_SMALL_KEY_MAX \(ResourceMgr_IsSceneMasterQuest\(SCENE_(\w+)\) \? \d+ : \d+\)',
                    read(rando / 'randomizer.h'))
check(sorted(macros) == sorted(OOTMM_MQ_KEYS), f'randomizer.h key macros cover {macros}')
mq_counts = {scene: int(mq) for mq, scene in re.findall(r'RA_\w+, \d+, (\d+),\s*RSK_MQ_\w+,\s*SCENE_(\w+),',
                                                         dungeon_cpp)}
for scene, count in OOTMM_MQ_KEYS.items():
    check(mq_counts.get(scene) == count, f'DungeonInfo mqKeyCount for {scene} is {mq_counts.get(scene)}, OoTMM {count}')
check(all(mq_counts.get(scene) == 0 for scene in ('DEKU_TREE', 'DODONGOS_CAVERN', 'JABU_JABU', 'ICE_CAVERN')),
      'dungeons without small keys must keep none in MQ')
macro_lines = '\n'.join(re.findall(r'#define \w+_SMALL_KEY_MAX \(ResourceMgr_IsSceneMasterQuest\(SCENE_\w+\)[^\n]*',
                                   read(rando / 'randomizer.h')))
key_counts = r'''#include <cstdint>
#include <cstdio>
#include "mq/MqMode.h"
using s16 = int16_t;
enum SceneID { SCENE_FOREST_TEMPLE = 3, SCENE_FIRE_TEMPLE = 4, SCENE_WATER_TEMPLE = 5, SCENE_SPIRIT_TEMPLE = 6,
               SCENE_SHADOW_TEMPLE = 7, SCENE_BOTTOM_OF_THE_WELL = 8, SCENE_GERUDO_TRAINING_GROUND = 0x0B,
               SCENE_INSIDE_GANONS_CASTLE = 0x0D };
ComboMq::MqModeInputs gInputs;
extern "C" uint32_t ResourceMgr_IsSceneMasterQuest(s16) { return ComboMq::ResolveMqMode(gInputs).gameplayMq; }
''' + macro_lines + r'''
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr, "failed: %s at line %d\n", #x, __LINE__); return 1; } } while (0)
int main() {
    // Every dungeon MQ from the bundled data, on an install without oot-mq.o2r.
    gInputs.hasOriginal = true;
    gInputs.bundledAvailable = true;
    gInputs.quest = ComboMq::QuestKind::Randomizer;
    gInputs.seedSource = ComboMq::SeedMqSource::Bundled;
    gInputs.dungeon = ComboMq::DungeonQuest::MasterQuest;
    REQUIRE(FOREST_TEMPLE_SMALL_KEY_MAX == @FOREST_TEMPLE@ && FIRE_TEMPLE_SMALL_KEY_MAX == @FIRE_TEMPLE@ &&
            WATER_TEMPLE_SMALL_KEY_MAX == @WATER_TEMPLE@ && SPIRIT_TEMPLE_SMALL_KEY_MAX == @SPIRIT_TEMPLE@ &&
            SHADOW_TEMPLE_SMALL_KEY_MAX == @SHADOW_TEMPLE@ &&
            BOTTOM_OF_THE_WELL_SMALL_KEY_MAX == @BOTTOM_OF_THE_WELL@ &&
            GERUDO_TRAINING_GROUND_SMALL_KEY_MAX == @GERUDO_TRAINING_GROUND@ &&
            GANONS_CASTLE_SMALL_KEY_MAX == @INSIDE_GANONS_CASTLE@);
    // The same seed's vanilla dungeons keep SoH's vanilla maxima (not OoTMM's MQ ones).
    gInputs.dungeon = ComboMq::DungeonQuest::Vanilla;
    REQUIRE(FOREST_TEMPLE_SMALL_KEY_MAX == 5 && WATER_TEMPLE_SMALL_KEY_MAX == 6 && SPIRIT_TEMPLE_SMALL_KEY_MAX == 5 &&
            GERUDO_TRAINING_GROUND_SMALL_KEY_MAX == 9 && GANONS_CASTLE_SMALL_KEY_MAX == 2);
    puts("key counts: a bundled seed's MQ dungeons get OoTMM's MQ small-key maxima");
    return 0;
}
'''
for scene, count in OOTMM_MQ_KEYS.items():
    key_counts = key_counts.replace(f'@{scene}@', str(count))

# --- MQ scrub prices -----------------------------------------------------------------------------------------------
# OoTMM's logic gives MQ dungeons their own vanilla scrub prices (packages/logic/src/price.ts:15-22, 97-130, indexed by
# the scrub id of each pool row, data/pool/pool_oot.csv), and En_Dns charges prices[id] for the id its params give
# (src/oot/actors/En/En_Dns.c:98-118). SoH's MQ scrub rows (location_list.cpp) carry the MQ game's prices, used
# whether scrubs are shuffled or not. Item for item they equal OoTMM's; nothing changes. By physical scrub they differ
# only in MQ Dodongo's Cavern: OoTMM's pool gives ids 0x1e and 0x1f each other's item (Staircase: red potion, Tunnel
# Side: Deku shield), while En_Dns gives the shield scrub (room 2, SoH's Staircase) 0x1f and the potion scrub 0x1e, so
# OoTMM charges the shield scrub 40 and the potion scrub 50; SoH keeps the game's 50 and 40.
OOTMM_MQ_SCRUB_PRICES = {'DT': [50], 'DC': [40, 15, 50, 40], 'JJ': [0], 'GC': [40, 40, 70, 40, 20]}
SCRUB_BASE = {'DT': 0x1b, 'DC': 0x1c, 'JJ': 0x20, 'GC': 0x21}  # OOT_SCRUBS: 27 overworld ids, then these
OOTMM_MQ_SCRUB_ROWS = {  # pool_oot.csv "MQ ... Scrub" rows: (dungeon, id) -> vanilla item
    ('DT', 0x1b): 'SHIELD_DEKU', ('DC', 0x1c): 'DEKU_SEEDS_30', ('DC', 0x1d): 'STICK', ('DC', 0x1f): 'POTION_RED',
    ('DC', 0x1e): 'SHIELD_DEKU', ('GC', 0x21): 'POTION_GREEN', ('GC', 0x22): 'BOMBS_5', ('GC', 0x23): 'ARROWS_10',
    ('GC', 0x24): 'POTION_RED', ('GC', 0x25): 'NUTS_5'}
EN_DNS_IDS = {  # En_Dns.c KEY(scene, 0, item) -> id; item is the En_Dns params (SCRUB_ITEM_*)
    ('DT', 0x04): 0x1b, ('DC', 0x03): 0x1c, ('DC', 0x01): 0x1d, ('DC', 0x00): 0x1e, ('DC', 0x07): 0x1e,
    ('DC', 0x04): 0x1f, ('JJ', 0x00): 0x20, ('GC', 0x08): 0x21, ('GC', 0x05): 0x22, ('GC', 0x06): 0x23,
    ('GC', 0x07): 0x24, ('GC', 0x00): 0x25}
POOL_ITEM_PARAMS = {'SHIELD_DEKU': 0x04, 'DEKU_SEEDS_30': 0x03, 'STICK': 0x01, 'POTION_RED': 0x07,
                    'POTION_GREEN': 0x08, 'BOMBS_5': 0x05, 'ARROWS_10': 0x06, 'NUTS_5': 0x00}
SCRUB_DUNGEONS = {'DT': ('SCENE_DEKU_TREE', 'DEKU_TREE', 'SCE_OOT_DEKU_TREE', 0),
                  'DC': ('SCENE_DODONGOS_CAVERN', 'DODONGO_CAVERN', 'SCE_OOT_DODONGO_CAVERN', 1),
                  'JJ': ('SCENE_JABU_JABU', 'INSIDE_JABU_JABU', 'SCE_OOT_INSIDE_JABU_JABU', 2),
                  'GC': ('SCENE_INSIDE_GANONS_CASTLE', 'INSIDE_GANON_CASTLE', 'SCE_OOT_INSIDE_GANON_CASTLE', 11)}
price_ts = ootmm_file('packages/logic/src/price.ts', '86f6e336ac0c1d1286a94ff176e8ab1edd24c6e5')
if price_ts:
    read_prices = {name: [int(v) for v in values.split(',')] for name, values in
                   re.findall(r'OOT_SCRUBS_(DT|DC|JJ|GC)_MQ: \[([\d,\s]+)\]', price_ts)}
    check(read_prices == OOTMM_MQ_SCRUB_PRICES, f'price.ts MQ scrub prices differ: {read_prices}')
    check(re.search(r'OOT_SCRUBS_OVERWORLD: \[([\d,\s]+)\]', price_ts)[1].count(',') + 1 == SCRUB_BASE['DT'],
          'price.ts: the dungeon scrub ids no longer start at 0x1b')
pool = ootmm_file('data/pool/pool_oot.csv', '446d2639ddf1fc13c473e3e5cfa5245369dffb4a')
if pool:
    scene_dungeon = {pool_scene: key for key, (_, pool_scene, _, _) in SCRUB_DUNGEONS.items()}
    read_rows = {(scene_dungeon[scene], int(rid, 16)): item for scene, rid, item in re.findall(
        r'^MQ [^,]*Scrub[^,]*,\s*scrub,\s*\w+,\s*(\w+),\s*(0x[0-9a-f]+),\s*(\w+)', pool, flags=re.M)}
    check(read_rows == OOTMM_MQ_SCRUB_ROWS, f'pool_oot.csv MQ scrub rows differ: {read_rows}')
en_dns = ootmm_file('packages/generator/src/oot/actors/En/En_Dns.c', '53e58bd40f0974a94273b538a327b1e6362a0828')
if en_dns:
    codes = {name: int(value, 16) for name, value in re.findall(r'#define SCRUB_ITEM_(\w+)\s+(0x[0-9a-f]+)', en_dns)}
    scene_key = {sce: key for key, (_, _, sce, _) in SCRUB_DUNGEONS.items()}
    read_ids = {(scene_key[scene], codes[item]): int(rid, 16) for scene, item, rid in re.findall(
        r'case KEY\((SCE_OOT_\w+), 0, SCRUB_ITEM_(\w+)\):\s*return (0x[0-9a-f]+);', en_dns) if scene in scene_key}
    check(read_ids == EN_DNS_IDS, f'En_Dns.c dungeon scrub ids differ: {read_ids}')
ootmm_price = {(key, rid): OOTMM_MQ_SCRUB_PRICES[key][rid - SCRUB_BASE[key]] for key, rid in
               [(key, SCRUB_BASE[key] + i) for key in SCRUB_BASE for i in range(len(OOTMM_MQ_SCRUB_PRICES[key]))]}
# SoH's MQ scrub rows: (scene, params) -> vanilla price. IdentifyScrub reads params 0x06 as 0x03 (hook_handlers.cpp).
soh_scrubs = {(scene, int(params, 16)): int(price) for scene, params, price in re.findall(
    r'Location::Base\(RC_\w+,\s*RCQUEST_MQ,\s*RCTYPE_SCRUB,\s*ACTOR_EN_DNS,\s*(SCENE_\w+),\s*(0x[0-9A-Fa-f]+),'
    r'[^;]*?,\s*(\d+)\);', read(rando / 'location_list.cpp'))}
check(hostcxx.contains(function(read(rando / 'hook_handlers.cpp'),
                                'static ScrubIdentity IdentifyScrub(s32 sceneNum, s32 actorParams, s32 respawnData)'),
                       'if (actorParams == 0x06) actorParams = 0x03;'), 'hook_handlers.cpp: IdentifyScrub changed')
by_item, by_scrub = [], []
for key, (scene, _, _, dungeon) in SCRUB_DUNGEONS.items():
    placed = sorted(actor[7] & 0xFFFF for room in mq_bins.decode_rooms(mq_bins.load(repo / 'combo/mq/data')[0])
                    if room['dungeon'] == dungeon for actor in room['actors'] if actor[0] & 0xFFFF == 0x0195)
    rows = sorted(params for (row_scene, params) in soh_scrubs if row_scene == scene)
    check(sorted(0x03 if params == 0x06 else params for params in placed) == rows,
          f'{key}: MQ scrubs in the bins {placed}, SoH rows {rows}')
    for params in placed:
        soh_price = soh_scrubs.get((scene, 0x03 if params == 0x06 else params))
        # By item: the OoTMM row whose vanilla item this scrub sells.
        item_ids = [rid for (row_key, rid), item in OOTMM_MQ_SCRUB_ROWS.items()
                    if row_key == key and POOL_ITEM_PARAMS[item] == params]
        check(len(item_ids) == 1, f'{key} scrub 0x{params:02x}: OoTMM rows {item_ids}')
        if item_ids and ootmm_price[(key, item_ids[0])] != soh_price:
            by_item.append((key, params, ootmm_price[(key, item_ids[0])], soh_price))
        # By physical scrub: the id En_Dns gives these params.
        if ootmm_price[(key, EN_DNS_IDS[(key, params)])] != soh_price:
            by_scrub.append((key, params, ootmm_price[(key, EN_DNS_IDS[(key, params)])], soh_price))
check(not by_item, f'MQ scrub prices differ from OoTMM item for item (dungeon, params, OoTMM, SoH): {by_item}')
check(sorted(by_scrub) == [('DC', 0x04, 40, 50), ('DC', 0x07, 50, 40)],
      f'MQ scrub prices by physical scrub (dungeon, params, OoTMM, SoH): {by_scrub}; only MQ DC\'s shield and potion '
      'scrubs should differ (OoTMM\'s pool labels ids 0x1e and 0x1f the other way round)')
print(f'scrub prices: SoH\'s MQ scrub prices equal OoTMM\'s item for item ({len(soh_scrubs)} scrubs); by physical '
      f'scrub only MQ DC\'s shield and potion scrubs differ')

# --- Ruto in Jabu-Jabu ---------------------------------------------------------------------------------------------
# OoTMM's eventFixes puts Ruto on the first floor on every entry to Jabu-Jabu, but only when Jabu-Jabu is not MQ
# (src/oot/play/play.c:65-86: EV_OOT_INF_RUTO_TOP_FLOOR, SoH's INFTABLE_147). Combo has none of those Ruto fixes, so
# MQ Jabu-Jabu already matches OoTMM: only En_Ru1 itself sets INFTABLE_147 (z_en_ru1.c), once Link carries her up.
# A port of the fix must keep OoTMM's MQ exception.
ruto_setters = {}
for tree in ('soh/soh', 'soh/src', 'combo'):
    for path in sorted((repo / tree).rglob('*')):
        if path.suffix in ('.c', '.cpp', '.h', '.inc'):
            text = path.read_text(encoding='utf-8', errors='replace')
            count = len(re.findall(r'Flags_SetInfTable\(\s*(?:INFTABLE_147|0x147)\s*\)|infTable\[(?:20|0x14)\]\s*\|=',
                                   text))
            if count:
                ruto_setters[path.relative_to(repo).as_posix()] = count
check(ruto_setters == {'soh/src/overlays/actors/ovl_En_Ru1/z_en_ru1.c': 1},
      f'Ruto\'s top-floor flag (INFTABLE_147) is set by {ruto_setters}; OoTMM sets it on entry only when Jabu-Jabu is '
      'not MQ (play.c:76)')
print('ruto: only En_Ru1 sets the top-floor flag, so MQ Jabu-Jabu places Ruto as OoTMM does')

# --- Ice Cavern's alternate setup 4 and the Serenade check ----------------------------------------------------------
# OoTMM turns MQ Ice Cavern's alternate-header command (0x18) into its path list, so the scene's only alternate setup,
# 4 (the Serenade cutscene header), is gone in MQ (src/oot/mq.c:328-339); ComboMqRuntime does the same (U4,
# mq_runtime_wired_into_scene_commands loads it on layer 4). Nothing in the Serenade check needs that setup:
# - the check is the Serenade flag, collected from any quest (ShuffleSongs.cpp, RCQUEST_BOTH, EventChkInf);
# - En_Xc sets that flag in the main room list (mq_bins_match_soh_location_door_and_silver_tables finds the Serenade
#   Sheik beside chest 0x02), whether or not its cutscene plays, and plays the cutscene in place from the scene file
#   (Cutscene_SetSegment), never through a scene setup;
# - the randomizer skips that cutscene anyway (timesaver_hook_handlers.cpp).
songs = read(rando / 'ShuffleSongs.cpp')
check(re.search(r'locationTable\[RC_SHEIK_IN_ICE_CAVERN\]\s*=\s*Location::Base\(RC_SHEIK_IN_ICE_CAVERN,\s*RCQUEST_BOTH,'
                r'[^;]*SpoilerCollectionCheck::EventChkInf\(EVENTCHKINF_LEARNED_SERENADE_OF_WATER\)', songs),
      'ShuffleSongs.cpp: Sheik in Ice Cavern must be the Serenade flag in either quest')
serenade_cs = function(read(repo / 'soh/src/overlays/actors/ovl_En_Xc/z_en_xc.c'), 's32 EnXc_SerenadeCS(EnXc* this, ')
cutscene_only = ''
while hostcxx.contains(serenade_cs, 'if (GameInteractor_Should(VB_PLAY_SERENADE_OF_WATER_CS, true)) {'):
    cut = block(serenade_cs, 'if (GameInteractor_Should(VB_PLAY_SERENADE_OF_WATER_CS, true)) {')
    cutscene_only += cut
    serenade_cs = serenade_cs.replace(cut, '')
eligible = block(serenade_cs, 'VB_BE_ELIGIBLE_FOR_SERENADE_OF_WATER')
check('Flags_SetEventChkInf(EVENTCHKINF_LEARNED_SERENADE_OF_WATER);' in eligible and
      'Flags_SetEventChkInf' not in cutscene_only and
      'Cutscene_SetSegment(play, &gIceCavernSerenadeCs);' in cutscene_only,
      'z_en_xc.c: EnXc_SerenadeCS must set the Serenade flag outside its cutscene branches')
timesavers = read(repo / 'soh/soh/Enhancements/TimeSavers/timesaver_hook_handlers.cpp')
song_cases = timesavers[hostcxx.find(timesavers, 'case VB_PLAY_SERENADE_OF_WATER_CS:'):]
song_cases = song_cases[:song_cases.index('break;')]
check(hostcxx.contains(song_cases, '|| IS_RANDO) { *should = false;'),
      'timesaver_hook_handlers.cpp: the randomizer must skip the Serenade cutscene')
print('ice cavern: the Serenade check is its flag, set from the main room list without setup 4')

# --- Chest positions -----------------------------------------------------------------------------------------------
# OoTMM's container-matches-contents resizes chests (big for major items and boss keys, small otherwise) and moves two
# that would clip once resized (src/common/csmc/csmc_chest.c:94-119): Ganon's Castle's major chest, flag 0x04 in MQ and
# 0x11 in vanilla, back 10 units, and Spirit's small chest 0x04. SoH's containers-match-contents changes textures only:
# EnBox keeps the size its params give (z_en_box.c), so no chest grows or shrinks and neither move applies, MQ or not.
en_box = read(repo / 'soh/src/overlays/actors/ovl_En_Box/z_en_box.c')
type_writes = len(re.findall(r'->type\s*=(?!=)', en_box))
check(type_writes == 1 and hostcxx.contains(en_box, 'this->type = thisx->params >> 12 & 0xF;'),
      f'z_en_box.c: a chest\'s type (its size) must come from its params alone ({type_writes} writes)')
update_texture = function(en_box, 'void EnBox_UpdateTexture(EnBox* this, PlayState* play)')
size_switch = block(update_texture, 'switch (this->type) {')
check(update_texture.count('Actor_SetScale') == size_switch.count('Actor_SetScale') == 2,
      'z_en_box.c: EnBox_UpdateTexture must size a chest by its type only')
resized = {}
for tree in ('soh/soh', 'combo'):
    for path in sorted((repo / tree).rglob('*')):
        if path.suffix in ('.c', '.cpp', '.h', '.inc'):
            count = len(re.findall(r'\b(?:chest|enBox|box)->(?:type\s*=(?!=)|dyna\.actor\.world\.pos\.[xz]\s*[-+]=)',
                                   path.read_text(encoding='utf-8', errors='replace')))
            if count:
                resized[path.relative_to(repo).as_posix()] = count
check(not resized, f'chest size or position changed outside EnBox: {resized}; OoTMM\'s csmc_chest.c:101-119 moves '
      'Ganon\'s Castle chest 0x04 (MQ) / 0x11 and Spirit chest 0x04 once it resizes them')
print('chests: SoH never resizes a chest, so OoTMM\'s resize offsets (MQ Ganon chest 0x04) have nothing to correct')

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

PROGRAMS = {'doors': doors, 'new_save': new_save, 'save_doors': save_doors, 'key_counts': key_counts,
            'spirit_block': spirit}
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
