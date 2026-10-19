"""SoH's Master Quest predicate split into a gameplay answer and an asset answer (MQ port unit U3).

SoH's ResourceMgr_IsSceneMasterQuest used to decide both whether a dungeon plays as MQ and whether its files come from
a real oot-mq.o2r's scenes/mq tree. OoTMM plays MQ from the vanilla scene files (src/oot/mq.c patches its lists into
them), so for a seed marked for the bundled data the two answers differ (combo/mq/MqMode.h). This scan checks:
- the ten asset callers (the /nonmq/ rewrite, the scene path, CrossEntrance's spawn check and the seven _MQ texture
  branches of z_scene_table.c) ask the asset predicate, and every other caller still asks the gameplay predicate;
- no new caller of either predicate, and no new direct use of the MQ archive flag, appears without updating this list;
- both predicates come from the resolver, with SoH's inputs mapped as SoH read them;
- randomizer.h's *_SMALL_KEY_MAX macros agree with DungeonInfo's key counts, so the macro (gameplay predicate) and
  DungeonInfo::GetSmallKeyCount (the seed's IsMQ) give the same count for a bundled seed;
- the bundled data is gated to NTSC-U 1.0 vanilla archives and never sets the MQ archive flag.
"""
from pathlib import Path
import argparse
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
args = parser.parse_args()
repo = args.source.resolve()
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def code(path):
    """The file without comments."""
    text = (repo / path).read_text(encoding='utf-8-sig')
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def body(text, signature):
    """The brace-balanced body that follows `signature`."""
    start = text.index('{', text.index(signature))
    depth = 0
    for at in range(start, len(text)):
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            return text[start:at + 1]
    raise AssertionError(f'unbalanced body after {signature}')


SOURCES = [p for tree in ('soh/soh', 'soh/src', 'soh/include', 'combo', 'mm/2s2h', 'mm/src')
           for p in sorted((repo / tree).rglob('*')) if p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inc')]
TEXTS = {p.relative_to(repo).as_posix(): code(p.relative_to(repo)) for p in SOURCES}


def uses(name):
    """Calls of `name` per file, leaving out the definitions and declarations in ResourceManagerHelpers."""
    pattern = re.compile(r'(?<![\w:])' + re.escape(name) + r'\s*\(')
    found = {}
    for path, text in TEXTS.items():
        if path.startswith('soh/soh/ResourceManagerHelpers.'):
            continue
        count = len(pattern.findall(text))
        if count:
            found[path] = count
    return found


# The gameplay callers (they follow the seed) and the asset callers (they pick the files).
GAMEPLAY_SCENE = {
    'soh/soh/Enhancements/ExtraModes/EnemyRandomizer.cpp': 3,
    'soh/soh/Enhancements/ExtraModes/MirroredWorld.cpp': 2,
    'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp': 1,
    'soh/soh/Enhancements/randomizer/randomizer.h': 8,
    'soh/soh/Enhancements/randomizer/randomizer_item_tracker.cpp': 1,
    'soh/soh/Enhancements/randomizer/savefile.cpp': 3,
    'soh/src/overlays/actors/ovl_Obj_Oshihiki/z_obj_oshihiki.c': 1,
    'soh/src/overlays/gamestates/ovl_select/z_select.c': 3,
}
GAMEPLAY_GAME = {
    'soh/soh/Enhancements/Fixes/FixWaterMQLock.cpp': 1,
    'soh/soh/Enhancements/TimeSavers/CrawlSpeed.cpp': 1,
    'soh/soh/Enhancements/randomizer/hook_handlers.cpp': 1,
    'soh/soh/Enhancements/randomizer/randomizer.cpp': 3,
    'soh/src/code/z_map_mark.c': 1,
    'soh/src/overlays/actors/ovl_En_Mag/z_en_mag.c': 3,
    'soh/src/overlays/misc/ovl_kaleido_scope/z_lmap_mark.c': 1,
}
ASSET_SCENE = {'soh/soh/Enhancements/CrossWarp/CrossEntrance.inc.h': 1}
ASSET_GAME = {'soh/src/code/z_scene_table.c': 7, 'soh/soh/z_play_otr.cpp': 1}

for name, expected in (('ResourceMgr_IsSceneMasterQuest', GAMEPLAY_SCENE),
                       ('ResourceMgr_IsGameMasterQuest', GAMEPLAY_GAME),
                       ('ResourceMgr_SceneUsesMqAssets', ASSET_SCENE),
                       ('ResourceMgr_GameUsesMqAssets', ASSET_GAME)):
    found = uses(name)
    check(found == expected, f'{name} callers changed: {found} != {expected}')

# The asset callers themselves.
check(re.search(r'sceneVersion = ResourceMgr_GameUsesMqAssets\(\) \? "mq" : "nonmq";',
                TEXTS['soh/soh/z_play_otr.cpp']), 'z_play_otr.cpp: the scene path must use the asset predicate')
check(re.search(r'version = dungeon \? \(ResourceMgr_SceneUsesMqAssets\(scene\) \? "mq" : "nonmq"\) : "shared";',
                TEXTS['soh/soh/Enhancements/CrossWarp/CrossEntrance.inc.h']),
      'CrossEntrance.inc.h: the spawn check must use the asset predicate')
table = TEXTS['soh/src/code/z_scene_table.c']
branches = re.findall(r'if \((ResourceMgr_\w+)\(\)\) \{\s*gSPSegment\([^;]*?(\w+_MQ)\[', table)
check(len(branches) == 7 and all(predicate == 'ResourceMgr_GameUsesMqAssets' for predicate, _ in branches),
      f'z_scene_table.c: the seven _MQ texture branches must use the asset predicate: {branches}')

helpers = TEXTS['soh/soh/ResourceManagerHelpers.cpp']
rewrite = body(helpers, 'ResourceMgr_GetResourceByNameHandlingMQ(const char* path)')
check('if (ResourceMgr_GameUsesMqAssets())' in rewrite and '"/mq/"' in rewrite,
      'GetResourceByNameHandlingMQ: the /nonmq/ rewrite must use the asset predicate')

# Both predicates come from the resolver (COMBO_BUILD branch), the bundled one from its data source.
gameplay = body(helpers, 'u32 IsSceneMasterQuest(s16 sceneNum)')
check(re.search(r'#ifdef COMBO_BUILD\s*return ComboMq::ResolveMqMode\(MqModeInputsForScene\(sceneNum\)\)\.gameplayMq;'
                r'\s*#else', gameplay), 'IsSceneMasterQuest must return the resolver\'s gameplay answer')
assets = body(helpers, 'uint32_t ResourceMgr_SceneUsesMqAssets(s16 sceneNum)')
check(re.search(r'#ifdef COMBO_BUILD\s*return ComboMq::ResolveMqMode\(MqModeInputsForScene\(sceneNum\)\)\.assetsMq;'
                r'\s*#else\s*return IsSceneMasterQuest\(sceneNum\);\s*#endif', assets),
      'ResourceMgr_SceneUsesMqAssets must return the resolver\'s asset answer (SoH\'s answer without COMBO_BUILD)')
bundled = body(helpers, 'uint32_t ResourceMgr_SceneUsesBundledMq(s16 sceneNum)')
check('.source == ComboMq::MqDataSource::Bundled' in bundled, 'ResourceMgr_SceneUsesBundledMq must key on Bundled')
check('ResourceMgr_SceneUsesMqAssets(gPlayState->sceneNum)' in body(helpers, 'uint32_t ResourceMgr_GameUsesMqAssets()'),
      'ResourceMgr_GameUsesMqAssets must ask about the current scene')

# The inputs, read as SoH's IsSceneMasterQuest read them.
inputs = body(helpers, 'static ComboMq::MqModeInputs MqModeInputsForScene(s16 sceneNum)')
for pattern, what in (
        (r'mqMode == WARP_MODE_OVERRIDE_MQ_AS_VANILLA\s*\? ComboMq::DebugWarpOverride::ForceMq',
         'MQ_AS_VANILLA makes the scene MQ (SoH returned true)'),
        (r'mqMode == WARP_MODE_OVERRIDE_VANILLA_AS_MQ \? ComboMq::DebugWarpOverride::ForceVanilla',
         'VANILLA_AS_MQ makes the scene vanilla (SoH returned false)'),
        (r'CVarGetInteger\(CVAR_GENERAL\("BetterDebugWarpScreenMQMode"\), WARP_MODE_OVERRIDE_OFF\)', 'the warp CVar'),
        (r'in\.hasOriginal = OTRGlobals::Instance->HasOriginal\(\);', 'hasOriginal'),
        (r'in\.hasArchiveMq = OTRGlobals::Instance->HasMasterQuest\(\);', 'the MQ archive flag'),
        (r'in\.bundledAvailable = OTRGlobals::Instance->HasBundledMasterQuest\(\);', 'the bundled flag'),
        (r'IS_MASTER_QUEST \? ComboMq::QuestKind::MasterQuest\s*: IS_RANDO\s*\? ComboMq::QuestKind::Randomizer',
         'the quest'),
        (r'GetDungeonFromScene\(sceneNum\)', 'the seed\'s dungeon for the scene'),
        (r'dungeon == nullptr \? ComboMq::DungeonQuest::NotADungeon\s*: dungeon->IsMQ\(\)\s*\? '
         r'ComboMq::DungeonQuest::MasterQuest', 'the dungeon\'s quest'),
        (r'in\.seedSource = randoContext->GetMqDataSource\(\);', 'the loaded seed\'s marker')):
    check(re.search(pattern, inputs), f'MqModeInputsForScene: {what}')
check(re.search(r'if \(IS_RANDO && randoContext != nullptr\)', inputs),
      'MqModeInputsForScene: the dungeon and marker are read only for a randomizer file')

# Direct uses of the MQ archive flag stay where they were: settings and catalog gates, the debug warp screen, the
# quest carousel and the resolver's inputs. A new one must be reviewed against the bundled source first.
ARCHIVE_FLAG = {
    'soh/soh/Enhancements/randomizer/settings.cpp': 3,
    'soh/soh/OTRGlobals.cpp': 1,
}
found = {}
for path, text in TEXTS.items():
    count = len(re.findall(r'(?:->|\.)HasMasterQuest\(\)', text))
    if count and not path.startswith('soh/soh/ResourceManagerHelpers.'):
        found[path] = count
check(found == ARCHIVE_FLAG, f'HasMasterQuest() callers changed: {found} != {ARCHIVE_FLAG}')
check(len(re.findall(r'(?:->|\.)HasMasterQuest\(\)', helpers)) == 5,
      'ResourceManagerHelpers.cpp: HasMasterQuest() only in the inputs, the legacy branch, '
      'ResourceMgr_GameHasMasterQuest, ResourceMgr_GameCanPlayMasterQuest and ResourceMgr_SaveMqPlayable')
GAME_HAS_MQ = {
    'soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c': 3,
    'soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_PAL.c': 2,
    'soh/src/overlays/gamestates/ovl_select/z_select.c': 3,
}
found = uses('ResourceMgr_GameHasMasterQuest')
check(found == GAME_HAS_MQ, f'ResourceMgr_GameHasMasterQuest callers changed: {found} != {GAME_HAS_MQ}')

# Key-count macros against DungeonInfo (vanillaKeyCount, mqKeyCount).
dungeons = TEXTS['soh/soh/Enhancements/randomizer/dungeon.cpp']
counts = {scene: (int(vanilla), int(mq)) for vanilla, mq, scene in re.findall(
    r'RA_\w+, (\d+), (\d+),\s*RSK_MQ_\w+,\s*(SCENE_\w+),', dungeons)}
check(len(counts) == 12, f'dungeon.cpp: expected 12 DungeonInfo rows, found {len(counts)}')
macros = re.findall(r'#define (\w+)_SMALL_KEY_MAX \(ResourceMgr_IsSceneMasterQuest\((SCENE_\w+)\) \? (\d+) : (\d+)\)',
                    TEXTS['soh/soh/Enhancements/randomizer/randomizer.h'])
check(len(macros) == 8, f'randomizer.h: expected 8 MQ-dependent key macros, found {len(macros)}')
for name, scene, mq, vanilla in macros:
    check(counts.get(scene) == (int(vanilla), int(mq)),
          f'{name}_SMALL_KEY_MAX gives {mq}/{vanilla} but DungeonInfo has {counts.get(scene)}')

# The bundled data's availability: NTSC-U 1.0 vanilla archives only, and never the MQ archive flag.
globals_cpp = TEXTS['soh/soh/OTRGlobals.cpp']
check('static_assert(ComboMq::kBundledMqRomVersion == OOT_NTSC_US_10' in globals_cpp,
      'OTRGlobals.cpp: the bundled ROM version must be pinned to OOT_NTSC_US_10')
setter = body(globals_cpp, 'ComboMq::BundledMqStatus OTRGlobals::SetBundledMasterQuest(')
check('hasMasterQuest' not in setter and 'ComboMq::BundledMqAvailability(hasOriginal' in setter and
      'ComboMq::Bundled() == nullptr' in setter, 'SetBundledMasterQuest must use the pure gate and decode the data')
loop = globals_cpp[globals_cpp.index('auto versions = context->GetResourceManager()'):]
loop = loop[:loop.index('SetBundledMasterQuest(originalVersions)')]
mq_case = loop[loop.index('case OOT_PAL_MQ:'):loop.index('hasMasterQuest = true;')]
original_case = loop[loop.index('case OOT_NTSC_US_10:'):]
check('originalVersions.push_back' not in mq_case and
      re.search(r'hasOriginal = true;\s*#ifdef COMBO_BUILD\s*originalVersions\.push_back\(version\);', original_case),
      'Initialize must pass only the vanilla archives\' versions to the gate')
headless = body(globals_cpp, 'OTRGlobals::OTRGlobals(HeadlessRandoTag)')
check('ReadGameVersionFromO2r(' in headless and 'SetBundledMasterQuest(originalVersions);' in headless,
      'the headless generator must apply the same gate from oot.o2r\'s version')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: the ten asset callers ask the asset predicate, every other caller the gameplay predicate, both from the '
      'resolver; key macros agree with DungeonInfo; bundled MQ is gated to NTSC-U 1.0')
