"""M1-09: OoTMM's CFG_ER_ANY runtime sites, in both games.

OoTMM v32.3 (413573d) turns CFG_ER_ANY on with isEntranceShuffle (world-config.ts:30), the switch M1-08
copied into combo/rando/entrances/EntranceRuntimeFlags.h. The fixture (tools/ootmm/m1-contract.json,
runtime.sites) cites five CFG_ER_ANY sites. This checks each one in the port:
  - mm/play/play.c:997-1002: every MM boot records the entrance it entered by, else Clock Town from the
    Clock Tower. title_setup.c calls Combo_MMRecordInitialEntrance after the arrival and the Cross-Games Age
    sync (critique X12), and the logical, unfixed variant is kept (critic correction C10);
  - :736-741: Song of Time and the moon crash return there, with the season fixup run again (C10): at the
    start of Play_Init, before its own season remap, and in the two cutscene skips that reach the same
    arrival inside Play_Init;
  - :484-494: Stone Tower switch 0x14 is set in the inverted scene and cleared in the normal one, at the end
    of Play_Init (OoTMM's Play_AfterInit, which also keeps gLastEntrance, :467);
  - oot/save.c:37-42: a Link's House save is not honoured. z_sram.c reads a flag the save preflight takes
    from the slot's seed, because Sram_StageOpenedSave picks the spawn before the OnLoadGame bind (C9);
  - oot/play/play.c:182-186: the end-of-game save at Ganon's Tower is not applicable, because the port never
    saves when Ganon is beaten (the evidence is checked below).
The MM boss time-speed rule (:496-508) reads g.initialEntrance only for boss entrances, which need boss ER:
not applicable in M1 either.

Compiled with the production code and engine stubs, as shipped and with COMBO_ENTRANCE_DEV: every site acts
only while an installed layout shuffles entrances, never with no layout, a private MM profile, a refused
layout, or settings that do not shuffle entrances (Water Voids or Anywhere alone); a slot's flags never leak
into the next slot's first load; and each MM boot kind (portal entry, owl save, Remember Save Location, a
resume without it, a new MM-start file, each arrival) records the entrance it resolved.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess
import sys
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--reference', type=Path)
args = parser.parse_args()
root = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []
PIN = '413573d'
GEN = 'packages/generator/src'


def check(ok, message):
    if not ok:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def function(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function ' + signature)


def squash(text):
    return re.sub(r'\s+', '', re.sub(r'//[^\n]*|/\*[\s\S]*?\*/', '', text))


def order(text, needles, message):
    positions = [text.find(squash(n)) for n in needles]
    check(all(p >= 0 for p in positions) and positions == sorted(positions), f'{message}: {positions}')


fixture = json.loads(read('tools/ootmm/m1-contract.json'))
runtime = fixture['runtime']

# ---- The fixture: the five cited CFG_ER_ANY sites, their switch and every flag occurrence.
SITES = [('mm/play/play.c', [484, 494], 'ported: Combo_MMAfterPlayInit (Stone Tower switch 0x14)'),
         ('mm/play/play.c', [736, 741], 'ported: Combo_MMReturnToInitialEntranceOnPlayInit / AfterSkip'),
         ('mm/play/play.c', [997, 1002], 'ported: Combo_MMRecordInitialEntrance (title_setup.c)'),
         ('oot/play/play.c', [182, 186], 'not applicable: the port has no end-of-game save'),
         ('oot/save.c', [38, 38], 'ported: z_sram.c Sram_StageOpenedSave, Link\'s House')]
cited = [(s['file'], s['lines']) for s in runtime['sites'] if s['flag'] == 'CFG_ER_ANY']
check(cited == [(f'{GEN}/{path}', lines) for path, lines, _ in SITES], f'fixture CFG_ER_ANY sites changed: {cited}')
for hit in runtime['occurrences']:
    if 'CFG_ER_ANY' in hit['text']:
        check(any(hit['file'] == f'{GEN}/{p}' and l[0] <= hit['line'] <= l[1] for p, l, _ in SITES),
              f'uncited CFG_ER_ANY occurrence {hit}')
enable = [c for c in runtime['enableConditions'] if c['flag'] == 'CFG_ER_ANY']
check([(c['file'], c['line'], c['text']) for c in enable] ==
      [('packages/generator/lib/combo/randomizer/world-config.ts', 30, 'ER_ANY: isEntranceShuffle(settings),')],
      'CFG_ER_ANY is no longer isEntranceShuffle')
uses = [(u['file'], u['line']) for u in runtime['mmInitialEntranceUses']]
check(uses == [(f'{GEN}/mm/play/play.c', line) for line in (497, 739, 999, 1001)],
      f'g.initialEntrance uses changed: {uses} (497 is the boss time speed, not applicable without boss ER)')

# ---- The port, as far as text can show it.
# MM boot: the record follows the arrival and the age sync, precedes the save-load hooks, and no combo boot
# returns before it.
setup = squash(read('mm/src/code/title_setup.c'))
order(setup, ['if (gComboStartFileNum >= 0) {', 'const int songArrival = MM_ConsumeSongArrival();',
              'gSaveContext.save.entrance = ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8);', 'Combo_MM_SyncCrossAgeOnEntry();',
              'Combo_MMRecordInitialEntrance(gSaveContext.save.entrance);',
              'GameInteractor_ExecuteOnSaveLoad(gSaveContext.fileNum);'],
      'title_setup.c: arrival, age sync, initial-entrance record, save-load hooks')
branch = setup.find(squash('if (gComboStartFileNum >= 0) {'))
record = setup.find(squash('Combo_MMRecordInitialEntrance(gSaveContext.save.entrance);'))
check('return;' not in setup[branch:record], 'title_setup.c: a combo MM boot can skip the initial-entrance record')
check(setup.count('Combo_MMRecordInitialEntrance(') == 2, 'title_setup.c records the initial entrance elsewhere')
# Play_Init: the return before the season remap (and after the -1 title return); the after-init hook last.
play = read('mm/src/code/z_play.c')
init = squash(function(play, 'void Play_Init(GameState* thisx) {'))
order(init, ['if (gSaveContext.save.entrance == -1) {', 'Combo_MMFarorePrepareArrival();',
             'Combo_MMReturnToInitialEntranceOnPlayInit();',
             'if ((gSaveContext.nextCutsceneIndex == 0xFFEF) || (gSaveContext.nextCutsceneIndex == 0xFFF0)) {',
             'gSaveContext.save.cutsceneIndex = gSaveContext.nextCutsceneIndex;', 'func_800EDDB0(this);'],
      'Play_Init: Song of Time / moon crash return before the season remap and the transition cutscene hooks')
check(init.endswith(squash('GameInteractor_ExecuteOnSceneInit(sceneIdAbsolute, spawnNum); #ifdef COMBO_BUILD '
                           'Combo_MMFaroreCaptureDungeon(this); Combo_MMAfterPlayInit(this); #endif }')),
      'Play_Init no longer ends with the after-init hook (OoTMM Play_AfterInit)')
check(squash('if (!GameInteractor_Should(VB_PLAY_TRANSITION_CS, true)) {') in
      squash(function(read('mm/src/code/z_demo.c'), 'void func_800EDDB0(PlayState* play) {')),
      'func_800EDDB0 no longer runs the transition-cutscene hooks the skips use')
# The skips: Song of Time (0xFFF7 at the cutscene map, spawn 0 only) and the moon crash.
sot = squash(read('mm/2s2h/Enhancements/Songs/SkipSoTCutscenes.cpp'))
check(squash('if ((gSaveContext.save.entrance == ENTRANCE(CUTSCENE, 0) || gSaveContext.save.entrance == '
             'ENTRANCE(CUTSCENE, 1)) && gSaveContext.save.cutsceneIndex == 0xFFF7) {') in sot,
      'SkipSoTCutscenes no longer keys on the Song of Time cutscene')
check(squash('} else { gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0); #ifdef COMBO_BUILD '
             'Combo_MMReturnToInitialEntranceAfterSkip(); '
             'if (gSaveContext.save.time == CLOCK_TIME(6, 0) - 1) Combo_MMQueueResetDawn(); #endif }') in sot and
      sot.count('Combo_MMReturnToInitialEntranceAfterSkip();') == 1,
      'SkipSoTCutscenes: the Song of Time arrival must return to the initial entrance, the flashback must not')
moon = squash(read('mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipMoonCrash.cpp'))
check(squash('gSaveContext.save.entrance = ENTRANCE(CLOCK_TOWER_INTERIOR, 3); #ifdef COMBO_BUILD '
             'Combo_MMReturnToInitialEntranceAfterSkip(); '
             'if (gSaveContext.save.day == 0) Combo_MMQueueResetDawn(); #endif }') in moon,
      'SkipMoonCrash: the moon crash arrival must return to the initial entrance')
# No other code writes the moon crash arrival into the save inside Play_Init.
writers = []
for path in [p for d in ('mm/src', 'mm/2s2h') for p in sorted((root / d).rglob('*.[ch]*'))]:
    text = squash(path.read_text(encoding='utf-8', errors='replace'))
    writers += [path.name] * text.count('gSaveContext.save.entrance=ENTRANCE(CLOCK_TOWER_INTERIOR,3);')
check(writers == ['SkipMoonCrash.cpp'], f'moon crash arrival writers changed: {writers}')
# The MM sites read only the installed layout (Combo_MMEntranceShuffleActive), never the generation's settings.
entrances_mm = read('mm/2s2h/Rando/Logic/ComboEntrancesMM.cpp')
er_start = hostcxx.find(entrances_mm, '// OoTMM v32.3 CFG_ER_ANY in MM')
er_block = entrances_mm[er_start:entrances_mm.index(function(entrances_mm, 'extern "C" void Combo_MMAfterPlayInit('))
                        + len(function(entrances_mm, 'extern "C" void Combo_MMAfterPlayInit('))]
for name in ('Combo_MMRecordInitialEntrance(', 'Combo_MMReturnToInitialEntranceOnPlayInit(',
             'Combo_MMReturnToInitialEntranceAfterSkip(', 'Combo_MMAfterPlayInit('):
    check('Combo_MMEntranceShuffleActive()' in function(er_block, 'extern "C" void ' + name),
          f'{name} must read the installed layout')
check('Combo_MMEntranceShuffleActive()' in function(er_block, 'extern "C" void Combo_MMQueueResetDawn('),
      'skipped dawn must act only with an installed entrance-shuffle layout')
for banned in ('ActiveMmGenerationSettings', 'MmUnifiedErEnabled', 'ActiveMmWallmasters'):
    check(banned not in er_block, f'the MM CFG_ER_ANY sites must not read {banned}')
check(re.search(r'extern "C" int Combo_MMEntranceShuffleActive\(void\) \{\s*return IS_RANDO && '
                r'CE::MmUnifiedEntranceShuffle\(\);', entrances_mm) is not None,
      'Combo_MMEntranceShuffleActive reads more than the installed layout')
# OoT: the Link's House rule, the flag's only reader, and where its value comes from.
sram = read('soh/src/code/z_sram.c')
stage = squash(function(sram, 'static void Sram_StageOpenedSave(void) {'))
order(stage, ['default:', 'if (CVarGetInteger(CVAR_ENHANCEMENT("RememberSaveLocation"), 0) &&',
              'if (gSaveContext.savedSceneNum != SCENE_LINKS_HOUSE #ifdef COMBO_BUILD || Combo_OOTEntranceShuffleActive() '
              '#endif ) {',
              'gSaveContext.entranceIndex = ENTR_LINKS_HOUSE_CHILD_SPAWN;'],
      'z_sram.c: Remember Save Location first, then the Link\'s House rule under CFG_ER_ANY')
check(squash('if (!Save_TryLoadFile(fileNum, initializeFullContext)) return 0; Sram_StageOpenedSave(); return 1;')
      in squash(function(sram, 'int Sram_TryOpenSave(int fileNum, int initializeFullContext) {')),
      'Sram_TryOpenSave no longer loads (and preflights) the save before staging it')
readers = []
for path in [p for d in ('soh/src', 'soh/soh', 'soh/include') for p in sorted((root / d).rglob('*.[ch]*'))]:
    text = path.read_text(encoding='utf-8', errors='replace')
    readers += [path.name] * len(re.findall(r'\bCombo_OOTEntranceShuffleActive\(\)', text))
check(readers == ['z_sram.c'],
      f'Combo_OOTEntranceShuffleActive readers changed: {readers}')
save_manager = read('soh/soh/SaveManager.cpp')
order(squash(function(save_manager, 'bool SaveManager::LoadFile(int fileNum, bool initializeFullContext) {')),
      ['ComboRando::CheckComboSeedPreflight(parsedSave);', 'ComboRando::ReadKokiriSaveRevision(parsedSave);',
       'SOH_SetEntranceRuntimeFlags(ComboRando::TakeComboEntranceRuntimeFlags(parsedSave));',
       'comboLoadState.store(ComboLoadState::Loading);', 'SaveContext_Init();', 'saveBlock = std::move(parsedSave);'],
      'SaveManager::LoadFile: the runtime flags after the preflight, before any save context change')
file_choose = squash(function(read('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c'),
                              'void FileChoose_LoadGame(GameState* thisx) {'))
order(file_choose, ['Sram_TryOpenSave(this->buttonIndex, 0)', 'GameInteractor_ExecuteOnLoadGame(gSaveContext.fileNum);'],
      'FileChoose_LoadGame: the spawn is staged before OnLoadGame binds the seed (C9)')
oot_setup = squash(read('soh/src/code/title_setup.c'))
order(oot_setup, ['Sram_TryOpenSave(attemptedFile, 1)', 'GameInteractor_ExecuteOnLoadGame(gSaveContext.fileNum);',
                  'gSaveContext.entranceIndex = entranceArrival >= 0 ?'],
      'OoT title_setup.c: an MM->OoT return overrides the staged spawn with its arrival')
otr = read('soh/soh/OTRGlobals.cpp')
flag_block = otr[hostcxx.find(otr, 'static std::atomic<uint32_t> sEntranceRuntimeFlags{ 0 };'):]
flag_block = flag_block[:flag_block.index(function(flag_block, 'extern "C" int Combo_OOTEntranceShuffleActive(void) {'))
                        + len(function(flag_block, 'extern "C" int Combo_OOTEntranceShuffleActive(void) {'))]
check('extern "C" COMBO_EXPORT void SOH_SetEntranceRuntimeFlags(uint32_t flags) {' in flag_block,
      'OTRGlobals.cpp no longer exports SOH_SetEntranceRuntimeFlags')
setters = []
for path in [root / 'combo/ComboShip.cpp', root / 'combo/ComboRandoHeadless.cpp'] + \
        [p for d in ('soh/src', 'soh/soh') for p in sorted((root / d).rglob('*.[ch]*'))]:
    text = path.read_text(encoding='utf-8', errors='replace')
    setters += [path.name] * len(re.findall(r'\bSOH_SetEntranceRuntimeFlags\((?!uint32_t)', text))
check(sorted(setters) == ['ComboShip.cpp', 'ComboShip.cpp', 'SaveManager.cpp'],
      f'SOH_SetEntranceRuntimeFlags callers changed: {setters}')
launcher = read('combo/ComboShip.cpp')
check(squash('if (mmInitResult == 0) { g_MmSaveInMemorySlot = fileNum; entranceRestore->Commit(); '
             'if (SOH_SetEntranceRuntimeFlags) '
             'SOH_SetEntranceRuntimeFlags(ComboRando::SeedEntranceRuntimeFlags(seed)); }')
      in squash(function(launcher, 'static void Combo_OnOOTSaveInit(int fileNum) {')),
      'Combo_OnOOTSaveInit: only successful save creation commits the layout and its flags')
read_save = squash(function(launcher, 'static const char* Combo_ReadGameSave(int game, int fileNum) {'))
order(read_save, ['ComboRando::Profiles::ValidateSavedCatalogs(inspected, ComboRando::Entrances::ActiveCapabilities());',
                  'entranceFlags = ComboRando::Profiles::ContainerEntranceRuntimeFlags(inspected, '
                  'ComboRando::Entrances::ActiveCapabilities());',
                  '} catch (const ComboRando::UnsupportedComboSeedFormat& error) {',
                  'if (entranceFlags && it != c.end() && it->is_object()) {',
                  'page[ComboRando::kComboEntranceRuntimeFlags] = entranceFlags;'],
      'Combo_ReadGameSave: the flags of an accepted layout only, on a copy of the page')
check(squash('const bool entrancesInstalled = InstallEntranceSeed(seedForEntrances); if (SOH_SetEntranceRuntimeFlags) '
             'SOH_SetEntranceRuntimeFlags(entrancesInstalled ? ComboRando::SeedEntranceRuntimeFlags(seedForEntrances) '
             ': 0u);') in squash(function(launcher, 'static void Combo_OnOOTSaveLoad(int fileNum) {')),
      'Combo_OnOOTSaveLoad: the bound slot\'s flags follow the installed layout')
check(squash('SOH_SetEntranceRuntimeFlags = (void (*)(uint32_t))GetSym(sohModule, "SOH_SetEntranceRuntimeFlags");')
      in squash(launcher), 'ComboShip.cpp does not resolve SOH_SetEntranceRuntimeFlags')
# oot/play/play.c:182-186 is not applicable. OoTMM's endGame (:153-207) saves at Ganon's Tower under ER when the
# Sages' Chamber ending starts (:443-445), unless it is a credit warp (:161-163). The port never saves there:
# when Ganon dies, z_boss_ganon2.c warps Adult Link to the Temple of Time without saving unless the combined
# goal is met, and otherwise rolls the credits as OoTMM does. The Triforce Hunt save on completion
# (hook_handlers.cpp) is OoTMM's credit warp, which endGame skips.
ganon = squash(read('soh/src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.c'))
gate = ganon[ganon.find(squash('#ifdef COMBO_BUILD if (gComboFinalBossDefeated == NULL ||')):]
gate = gate[:gate.find('#endif') + len('#endif')]
check(squash('play->nextEntranceIndex = ENTR_TEMPLE_OF_TIME_WARP_PAD; gSaveContext.nextCutsceneIndex = 0xFFEF;') in gate
      and re.search(r'Save_|SaveFile|PerformSave|SaveManager', gate) is None,
      'the unfinished-goal Ganon ending now saves or goes elsewhere')
check(squash('#endif play->nextEntranceIndex = ENTR_CHAMBER_OF_THE_SAGES_0; gSaveContext.nextCutsceneIndex = 0xFFF2;')
      in ganon, 'the finished-goal Ganon ending changed')
# OoTMM's endGame writes the saved scene (gSave.info.sceneId) for the end of the game. The port writes the saved
# scene only as the current play's scene (pause menu and Play_PerformSave, which autosave and the Triforce Hunt
# completion use), for a new file, for the oracle's context, and when copying a slot: never an end-of-game place.
scene_writers = {}
for path in [p for d in ('soh/src', 'soh/soh', 'soh/include', 'combo') for p in sorted((root / d).rglob('*.[ch]*'))]:
    for value in re.findall(r'\bsavedSceneNum\s*=(?!=)\s*([^;]+);', path.read_text(encoding='utf-8', errors='replace')):
        scene_writers.setdefault(path.name, []).append(value.strip())
check(scene_writers == {'z_play.c': ['play->sceneNum'], 'z_kaleido_scope_PAL.c': ['play->sceneNum'],
                        'logic.cpp': ['SCENE_LINKS_HOUSE'], 'savefile.cpp': ['-1'],
                        'SaveManager.cpp': ['SCENE_LINKS_HOUSE', '0x51', '0x51', 'src.savedSceneNum']},
      f'OoT saved-scene writers changed (an end-of-game save would make oot/play/play.c:182-186 applicable): '
      f'{scene_writers}')
check(squash('GameInteractor::State::TriforceHuntCreditsWarpActive && randomizerQueuedChecks.empty()') in
      squash(read('soh/soh/Enhancements/randomizer/hook_handlers.cpp')), 'the Triforce Hunt save is not the credit warp')

# ---- OoTMM itself, when the checkout is there (canonical Git blobs at the pin).
reference = args.reference
if reference is None:
    for candidate in (root.parent / 'references/ootmm-stable-32.3', root.parent.parent / 'references/ootmm-stable-32.3'):
        if (candidate / GEN).exists():
            reference = candidate
            break
if reference is not None:
    import hashlib

    def blob(path):
        return subprocess.run(['git', '-C', str(reference), 'show', f'{PIN}:{path}'], capture_output=True, check=True,
                              timeout=60).stdout.decode('utf-8').split('\n')

    for site in runtime['sites']:
        if site['flag'] != 'CFG_ER_ANY':
            continue
        text = '\n'.join(blob(site['file'])[site['lines'][0] - 1:site['lines'][1]])
        check(hashlib.sha256(text.encode('utf-8')).hexdigest() == site['sha256'], f'{site["file"]}:{site["lines"]}')
    mm_play, oot_play, oot_save = (blob(f'{GEN}/{p}') for p in ('mm/play/play.c', 'oot/play/play.c', 'oot/save.c'))
    for lines, line, text in ((mm_play, 467, 'gLastEntrance = gSave.entrance;'),
                              (mm_play, 488, 'Flags_SetSwitch(play, 0x14);'),
                              (mm_play, 492, 'ClearSwitchFlag(play, 0x14);'),
                              (mm_play, 738, '/* Song of Time / Moon crash */'),
                              (mm_play, 740, 'applyCustomEntrance(&entrance);'),
                              (oot_play, 153, 'static void endGame(void)'),
                              (oot_play, 162, 'if (g.isCreditWarp)'),
                              (oot_play, 194, 'Save_DoSave(NULL, SF_PASSIVE);'),
                              (oot_play, 428, 'case ENTR_OOT_SPAWN_ADULT:'),
                              (oot_play, 429, 'gSave.entrance = ENTR_OOT_WARP_SONG_TEMPLE;'),
                              (oot_play, 443, 'case ENTR_OOT_SAGES_CHAMBER_END:'),
                              (oot_play, 444, 'endGame();'),
                              (oot_save, 40, 'gSave.entrance = ENTR_OOT_SPAWN_CHILD;'),
                              (oot_save, 44, 'entrance = gSave.age == AGE_CHILD ? ENTR_OOT_SPAWN_CHILD : '
                                             'ENTR_OOT_SPAWN_ADULT;')):
        check(lines[line - 1].strip() == text, f'pinned line moved: {text}')
    check(squash('if ((gSave.entrance == ENTR_MM_CLOCK_TOWN_FROM_CLOCK_TOWER && gLastEntrance == '
                 'ENTR_MM_CLOCK_TOWN_FROM_SONG_OF_TIME) || gSave.entrance == ENTR_MM_CLOCK_TOWER_MOON_CRASH)')
          == squash(mm_play[735]), 'play.c:736 condition changed')
    scenes = '\n'.join(blob('data/defs/scenes.yml'))
    for name, value in (('MM_STONE_TOWER', '0x58'), ('MM_STONE_TOWER_INVERTED', '0x59'), ('OOT_LINK_HOUSE', '0x34')):
        check(re.search(rf'^{name}:\s+{value}$', scenes, re.M) is not None, f'scenes.yml {name}')
    link = blob(f'{GEN}/link_mm.in')
    check('Flags_SetSwitch = 0x800b5bf4;' in link and 'ClearSwitchFlag = 0x800b5c34;' in link,
          'link_mm.in switch functions moved')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources against the fixture only')
# OoTMM's ClearSwitchFlag (0x800b5c34) follows Flags_SetSwitch (0x800b5bf4) by one function: the port's
# Flags_UnsetSwitch, after Flags_GetSwitch and Flags_SetSwitch in z_actor.c.
actor = read('mm/src/code/z_actor.c')
names = re.findall(r'^(?:s32|void) (Flags_\w+Switch)\(PlayState\* play, s32 flag\) \{', actor, re.M)
check(names[:3] == ['Flags_GetSwitch', 'Flags_SetSwitch', 'Flags_UnsetSwitch'], f'z_actor.c switch functions: {names}')

if failures:
    sys.exit('\n'.join(failures))

# ---- Shared C++ pieces.
scene_h = read('mm/include/z64scene.h')
scene_enum = 'typedef enum {\n' + scene_h.split('    /* 0x00 */ ENTR_SCENE_MAYORS_RESIDENCE,', 1)[1].split(
    '} EntranceSceneId;', 1)[0]
scene_enum = scene_enum.replace('typedef enum {\n', 'typedef enum {\n    ENTR_SCENE_MAYORS_RESIDENCE,', 1) + \
    '} EntranceSceneId;\n'
entrance_macro = next(line for line in scene_h.splitlines() if line.startswith('#define ENTRANCE(scene, spawn)'))
mm_scenes = dict((name, int(index, 16)) for index, name in re.findall(
    r'^/\* (0x[0-9A-F]+) \*/ DEFINE_SCENE\(\w+,\s+(SCENE_F4[01]),', read('mm/include/tables/scene_table.h'), re.M))
oot_scenes = dict((name, int(index, 16)) for index, name in re.findall(
    r'^/\* (0x[0-9A-F]+) \*/ DEFINE_SCENE\(\w+, \w+, (SCENE_LINKS_HOUSE|SCENE_FAIRYS_FOUNTAIN|SCENE_GROTTOS|'
    r'SCENE_TEMPLE_OF_TIME),', read('soh/include/tables/scene_table.h'), re.M))
oot_entrances = dict((name, int(index, 16)) for index, name in re.findall(
    r'^/\* (0x[0-9A-F]+) \*/ DEFINE_ENTRANCE\((ENTR_LINKS_HOUSE_CHILD_SPAWN|ENTR_TEMPLE_OF_TIME_WARP_PAD),',
    read('soh/include/tables/entrance_table.h'), re.M))
assert len(mm_scenes) == 2 and len(oot_scenes) == 4 and len(oot_entrances) == 2, (mm_scenes, oot_scenes, oot_entrances)

combos = []
for c in fixture['combinations']:
    s = c['settings']
    sets = [f's.erWallmasters = CE::Mode::{"OwnGame" if s["erWallmasters"] == "ownGame" else "None"};',
            f's.erOneWays = CE::Mode::{"OwnGame" if s["erOneWays"] == "ownGame" else "None"};']
    sets += [f's.{k} = {str(s[k]).lower()};' for k in ('erOneWaysMajor', 'erOneWaysIkana', 'erOneWaysOwls',
                                                       'erOneWaysWaterVoids', 'erOneWaysAnywhere')]
    routes = ', '.join('{"%s", "%s"}' % (a, b) for a, b in c['sampleLayout'])
    combos.append('    {[] { CE::Settings s; %s return s; }(), %s, {%s}, "%s"},' % (
        ' '.join(sets), str(c['isEntranceShuffle']).lower(), routes, c['id']))
combinations = r'''
struct Combination {
    CE::Settings settings;
    bool entranceShuffle;
    std::vector<std::pair<std::string, std::string>> sample;
    const char* id;
};
const std::vector<Combination> kCombinations = {
''' + '\n'.join(combos) + r'''
};
CE::CapabilitySet WholeCatalog() {
    auto caps = CE::CapabilitySet::AtLeast(CE::Stage::Hidden);
    caps.devBuild = true;
    return caps;
}
// A seed with the fixture's sample layout of a combination (every pool filled, sources sorted).
Json SampleSeed(const Combination& c) {
    CE::EntranceLayout layout;
    layout.required = {CE::UnifiedRequirement(c.settings, {})};
    layout.settings = c.settings;
    CE::Overrides routes;
    for (const auto& [src, dst] : c.sample)
        routes[CE::LayoutV1::FindRow(src)->name] = CE::LayoutV1::FindRow(dst)->name;
    layout.overrides = routes;
    return {{"fileType", ComboRando::kEntranceSeedFileType}, {"version", ComboRando::kEntranceSeedVersion},
            {"entranceLayout", CE::EncodeEntranceLayout(layout)}};
}
const Combination& Find(const char* id) {
    for (const auto& c : kCombinations)
        if (std::string(c.id) == id)
            return c;
    assert(false);
    return kCombinations.front();
}
Json TrancheSeed() {
    CE::EntranceLayout layout;
    layout.required = {std::string(CE::kMmTrancheCapability)};
    layout.settings.erWallmasters = CE::Mode::OwnGame;
    CE::Overrides routes;
    const char* to[] = {"MM_TEMPLE_WOODFALL", "MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_GREAT_BAY", "MM_SPIDER_HOUSE_SWAMP"};
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i)
        routes[CE::LayoutV1::FindRow(CE::kMmWallmasterSources[i])->name] = CE::LayoutV1::FindRow(to[i])->name;
    layout.overrides = routes;
    return {{"fileType", ComboRando::kEntranceSeedFileType}, {"version", ComboRando::kEntranceSeedVersion},
            {"entranceLayout", CE::EncodeEntranceLayout(layout)}};
}
Json LegacySeed() {
    std::vector<CE::SavedOverride> rows;
    for (size_t i = 0; i < CE::kMmWallmasterSources.size(); ++i) {
        const auto* src = CE::Find(CE::kMmWallmasterSources[i]);
        const auto* dst = CE::Find(CE::kMmBaseWallmasterDestinations[i]);
        rows.push_back({std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id});
    }
    return {{"fileType", ComboRando::kComboSeedFileType}, {"entranceExpansion", CE::EncodeMmWallmasterExtension(rows)}};
}
// The flags against the fixture: CFG_ER_ANY is isEntranceShuffle, so Water Voids alone and Anywhere alone
// turn nothing on.
void TruthTable() {
    assert(kCombinations.size() == 66);
    for (const auto& c : kCombinations)
        assert(CE::EntranceRuntimeFlags(c.settings) == (c.entranceShuffle ? CE::kEntranceRuntimeErAny : 0u));
    assert(!Find("erWallmasters=none;erOneWays=ownGame;erOneWaysWaterVoids").entranceShuffle);
    assert(!Find("erWallmasters=none;erOneWays=ownGame;erOneWaysAnywhere").entranceShuffle);
    assert(CE::EntranceRuntimeFlags({}) == 0 && CE::kEntranceRuntimeFlagsKnown == CE::kEntranceRuntimeErAny);
}
'''

# ---- Compiled, MM: the boot record, the Song of Time / moon crash return, the Stone Tower switch.
includes = '\n'.join(re.findall(r'^#include "rando/entrances/[^"]+"$', entrances_mm, re.M))
queue_decl = re.search(r'void Combo_MMQueueResetDawn\(void\);',
                       read('mm/2s2h/Rando/Logic/ComboEntrancesMM.h')).group(0)
file_locals = entrances_mm[hostcxx.find(entrances_mm, 'namespace CE = ComboRando::Entrances;'):
                           hostcxx.find(entrances_mm, 'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(')]
exports = '\n'.join(function(entrances_mm, s) for s in (
    'extern "C" COMBO_EXPORT int MM_ApplyEntranceExpansionForCombo(',
    'extern "C" int Combo_MMEntranceShuffleActive(',
))
assert exports.count('CE::ActiveCapabilities()') == 1
exports = exports.replace('CE::ActiveCapabilities()', 'HarnessCapabilities()')
setup_text = read('mm/src/code/title_setup.c')
boot = setup_text[hostcxx.find(setup_text, 's32 loadedOwlSave;'):]
boot = boot[:hostcxx.find(boot, 'Combo_MMRecordInitialEntrance(gSaveContext.save.entrance);')]
boot += 'Combo_MMRecordInitialEntrance(gSaveContext.save.entrance);\n    }\n'
mm_program = r'''
#define NOMINMAX
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
''' + includes + r'''
#include "rando/entrances/MmRegionalFlags.h" // through 2Ship's Logic.h in the game
#include "rando/entrances/UnifiedBridge.h"
#define COMBO_EXPORT
static int loggedErrors = 0;
template <class... A> void LogError(const char*, const A&...) { ++loggedErrors; }
#define SPDLOG_ERROR(...) LogError(__VA_ARGS__)
static bool harnessRando = true;
#define IS_RANDO harnessRando
using s16 = int16_t;
using s32 = int32_t;
using u16 = uint16_t;
using Json = nlohmann::json;
enum RandoRegionId {
    RR_BENEATH_THE_GRAVEYARD_DAMPE = 1,
    RR_BENEATH_THE_WELL_FREEZARD_ROOM,
    RR_BENEATH_THE_WELL_TWO_SPIKED_BARS,
    RR_BENEATH_THE_WELL_FOUR_SPIKED_BARS,
};
static ComboRando::Entrances::CapabilitySet harnessCaps = ComboRando::Entrances::ActiveCapabilities();
static ComboRando::Entrances::CapabilitySet HarnessCapabilities() { return harnessCaps; }
// z64scene.h and the scene table.
''' + scene_enum + entrance_macro + '\n' + \
    ''.join(f'constexpr s16 {name} = {value};\n' for name, value in mm_scenes.items()) + r'''
// z64save.h and z64actor.h, as far as these read them.
enum WeekEvent {
    WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE,
    WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE,
    WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE,
    WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE,
};
std::set<int> weekEvents;
#define CHECK_WEEKEVENTREG(flag) (weekEvents.count(flag) != 0)
enum { PLAYER_FORM_DEKU, PLAYER_FORM_HUMAN = 4 };
enum { PLAYER_MASK_NONE, PLAYER_MASK_DEKU = 7 };
struct {
    struct {
        s32 entrance = 0;
        s32 cutsceneIndex = 0;
        int time = 0, day = 0;
        bool isOwlSave = false;
        int playerForm = PLAYER_FORM_DEKU, equippedMask = PLAYER_MASK_DEKU;
        struct {
            s32 pauseSaveEntrance = -1;
        } shipSaveInfo;
    } save;
    u16 nextCutsceneIndex = 0xFFEF;
    bool flashSaveAvailable = false;
    int gameMode = -1;
} gSaveContext;
struct GameState {
    bool running = true;
    void (*init)(GameState*) = nullptr;
    size_t size = 0;
};
struct DayTelopState { GameState state; };
void DayTelop_Init(GameState*) {}
enum { GAMEMODE_NORMAL = 0 };
#define CLOCK_TIME(h, m) ((h) * 60 + (m))
#define STOP_GAMESTATE(state) ((state)->running = false)
#define SET_NEXT_GAMESTATE(state, next, bytes) ((state)->init = (next), (state)->size = (bytes))
struct PlayState {
    s16 sceneId;
    GameState state;
};
std::vector<std::pair<s32, s16>> switches; // +flag set, -flag unset, by scene
void Flags_SetSwitch(PlayState* play, s32 flag) { switches.push_back({flag, play->sceneId}); }
void Flags_UnsetSwitch(PlayState* play, s32 flag) { switches.push_back({-flag, play->sceneId}); }
''' + file_locals + '\n' + exports + '\nextern "C" ' + queue_decl + '\n' + er_block + r'''
// title_setup.c's combo branch, as far as the record, with its engine calls stubbed.
int gComboStartFileNum = -1, gComboOwlBlobSlot = -1, gComboEntryIsResume = 0;
bool rememberSaveLocation = false, synced = false;
s32 owlEntrance = -1;
int stubEntrance = -1, stubDungeon = -1, stubFarore = -1, stubSong = -1;
void Combo_AdoptOOTGlobalOptions() {}
void Combo_LoadMMSaveFile(int) {}
void Combo_ApplyOwlSaveOpen(int) {
    gSaveContext.save.isOwlSave = true;
    gSaveContext.save.entrance = owlEntrance;
}
int CVarGetInteger(const char* name, int fallback) {
    return std::string(name) == "gEnhancements.Saving.RememberSaveLocation" ? rememberSaveLocation : fallback;
}
int MM_ConsumeEntranceArrival(void) { return stubEntrance; }
int MM_ConsumeDungeonArrival(void) { return stubDungeon; }
int MM_ConsumeFaroreArrival(void) { return stubFarore; }
int MM_ConsumeSongArrival(void) { return stubSong; }
int Combo_MM_SyncCrossAgeOnEntry(void) { synced = true; return 1; }
void Boot() {
    synced = false;
    gComboStartFileNum = 0;
''' + boot + r'''
    assert(synced);
}

namespace CE = ComboRando::Entrances;
''' + combinations + r'''
constexpr s32 kClockTown = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
constexpr s32 kSongOfTime = ENTRANCE(CUTSCENE, 0);
constexpr s32 kMoonCrash = ENTRANCE(CLOCK_TOWER_INTERIOR, 3);
// The MM boot kinds (D10 and critic correction C10): each lands as before and, under entrance shuffle, records
// the logical variant of where it landed.
struct BootKind {
    const char* name;
    bool owl, resume, rsl;
    s32 owlAt, pauseAt;
    int entrance, dungeon, farore, song;
    std::set<int> cleared;
    s32 lands, initial; // where this boot lands, and the logical entrance it records under entrance shuffle
};
const std::vector<BootKind> kBoots = {
    {"portal entry", false, false, false, -1, -1, -1, -1, -1, -1, {}, kClockTown, kClockTown},
    {"portal entry over an owl save", true, false, false, ENTRANCE(GREAT_BAY_COAST, 11), -1, -1, -1, -1, -1, {},
     kClockTown, kClockTown},
    {"owl save resume", true, true, true, ENTRANCE(SOUTHERN_SWAMP_CLEARED, 10), ENTRANCE(MILK_ROAD, 0), -1, -1, -1, -1,
     {WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE}, ENTRANCE(SOUTHERN_SWAMP_CLEARED, 10),
     ENTRANCE(SOUTHERN_SWAMP_POISONED, 10)},
    {"Remember Save Location resume", false, true, true, -1, ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 2), -1, -1, -1, -1,
     {WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE}, ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 2), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 2)},
    {"resume without Remember Save Location (dormant MM, Song of Time save)", false, true, false, -1,
     ENTRANCE(IKANA_CANYON, 1), -1, -1, -1, -1, {}, kClockTown, kClockTown},
    {"new MM-start file", false, true, true, -1, -1, -1, -1, -1, -1, {}, kClockTown, kClockTown},
    {"shuffled-entrance arrival", false, false, false, -1, -1, ENTRANCE(GORON_VILLAGE_SPRING, 1), -1, -1, -1,
     {WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE}, ENTRANCE(GORON_VILLAGE_SPRING, 1), ENTRANCE(GORON_VILLAGE_WINTER, 1)},
    {"dungeon arrival (game-over continue)", false, true, true, -1, ENTRANCE(MILK_ROAD, 0), -1,
     ENTRANCE(WOODFALL_TEMPLE, 0), -1, -1, {}, ENTRANCE(WOODFALL_TEMPLE, 0), ENTRANCE(WOODFALL_TEMPLE, 0)},
    {"Farore's Wind arrival", false, false, false, -1, -1, -1, -1, ENTRANCE(ZORA_CAPE, 6), -1, {},
     ENTRANCE(ZORA_CAPE, 6), ENTRANCE(ZORA_CAPE, 6)},
    {"song arrival into a cleared swamp", false, false, false, -1, -1, -1, -1, -1, ENTRANCE(SOUTHERN_SWAMP_POISONED, 10),
     {WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE}, ENTRANCE(SOUTHERN_SWAMP_CLEARED, 10), ENTRANCE(SOUTHERN_SWAMP_POISONED, 10)},
};
void Run(const BootKind& b) {
    gComboOwlBlobSlot = b.owl ? 1 : -1;
    gComboEntryIsResume = b.resume;
    rememberSaveLocation = b.rsl;
    owlEntrance = b.owlAt;
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = b.pauseAt;
    stubEntrance = b.entrance;
    stubDungeon = b.dungeon;
    stubFarore = b.farore;
    stubSong = b.song;
    weekEvents = b.cleared;
    gSaveContext.save.entrance = 0x7777;
    Boot();
}
// The play that ran before this one, as Play_Init's end hook records it.
void Played(s32 entrance, s16 scene = 0) {
    PlayState play{scene};
    gSaveContext.save.entrance = entrance;
    Combo_MMAfterPlayInit(&play);
}
// Play_Init's start with this arrival; returns where it goes.
s32 Arrive(s32 entrance, u16 next = 0xFFEF) {
    gSaveContext.save.entrance = entrance;
    gSaveContext.nextCutsceneIndex = next;
    Combo_MMReturnToInitialEntranceOnPlayInit();
    return gSaveContext.save.entrance;
}
// A cutscene skip landing on `entrance` inside Play_Init (cutscene index already 0); returns where it goes.
s32 Skip(s32 entrance) {
    gSaveContext.save.entrance = entrance;
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    Combo_MMReturnToInitialEntranceAfterSkip();
    assert(gSaveContext.nextCutsceneIndex == 0xFFEF);
    return gSaveContext.save.entrance;
}
// No site acts: the boot lands where it lands, Song of Time and the moon crash keep their native arrival,
// and the Stone Tower switch is left alone.
void Inert() {
    PlayState play{};
    Combo_MMQueueResetDawn();
    Combo_MMAfterPlayInit(&play);
    assert(play.state.running && play.state.init == nullptr);
    for (const auto& b : kBoots) {
        Run(b);
        assert(gSaveContext.save.entrance == b.lands);
        Played(kSongOfTime);
        assert(Arrive(kClockTown) == kClockTown && Arrive(kMoonCrash) == kMoonCrash);
        assert(Skip(kClockTown) == kClockTown && gSaveContext.save.cutsceneIndex == 0);
        assert(Skip(kMoonCrash) == kMoonCrash);
    }
    switches.clear();
    Played(ENTRANCE(STONE_TOWER, 0), SCENE_F40);
    Played(ENTRANCE(STONE_TOWER_INVERTED, 0), SCENE_F41);
    assert(switches.empty());
}
void Active() {
    // Reproduction: the SoT skip returns into an indoor scene with no EnTest4 to schedule dawn.
    // Execute the production queue and after-init functions, observing the native gamestate boundary.
    for (int previousDay : {0, 1, 2}) { // normal reset and each clock-shuffle daytime start
        PlayState indoor{};
        gSaveContext.save.day = previousDay;
        gSaveContext.save.time = CLOCK_TIME(6, 0) - 1;
        Combo_MMQueueResetDawn();
        Combo_MMAfterPlayInit(&indoor);
        assert(!indoor.state.running && indoor.state.init == DayTelop_Init);
        assert(indoor.state.size == sizeof(DayTelopState));
        assert(gSaveContext.save.time == CLOCK_TIME(6, 0));
        assert(gSaveContext.save.day == previousDay); // native DayTelop, not this adapter, advances it
        PlayState returned{};
        Combo_MMAfterPlayInit(&returned);
        assert(returned.state.running && returned.state.init == nullptr); // one-shot consumed
    }
    PlayState outdoor{};
    outdoor.state.running = false; // EnTest4 already scheduled native DayTelop
    outdoor.state.init = DayTelop_Init;
    outdoor.state.size = 123;
    Combo_MMQueueResetDawn();
    Combo_MMAfterPlayInit(&outdoor);
    assert(!outdoor.state.running && outdoor.state.size == 123); // no duplicate scheduling
    Combo_MMQueueResetDawn();
    Combo_MMRecordInitialEntrance(ENTRANCE(STONE_TOWER, 0)); // boot clears stale pending work
    PlayState nextBoot{};
    Combo_MMAfterPlayInit(&nextBoot);
    assert(nextBoot.state.running && nextBoot.state.init == nullptr);
    Combo_MMQueueResetDawn();
    harnessRando = false;
    PlayState inactive{};
    Combo_MMAfterPlayInit(&inactive);
    harnessRando = true;
    Combo_MMAfterPlayInit(&inactive);
    assert(inactive.state.running && inactive.state.init == nullptr); // stale request cannot leak
    // Actual unskipped moon-crash arrival requests the same native DayTelop for the Day0 main save.
    Combo_MMRecordInitialEntrance(ENTRANCE(STONE_TOWER, 0));
    gSaveContext.save.day = 0;
    assert(Arrive(kMoonCrash) == ENTRANCE(STONE_TOWER, 0));
    PlayState moonArrival{};
    Combo_MMAfterPlayInit(&moonArrival);
    assert(!moonArrival.state.running && moonArrival.state.init == DayTelop_Init);
    PlayState moonReturned{};
    Combo_MMAfterPlayInit(&moonReturned);
    assert(moonReturned.state.running && moonReturned.state.init == nullptr);
    // A main save already on Day1 must not advance to Day2.
    gSaveContext.save.day = 1;
    assert(Arrive(kMoonCrash) == ENTRANCE(STONE_TOWER, 0));
    PlayState normalDay{};
    Combo_MMAfterPlayInit(&normalDay);
    assert(normalDay.state.running && normalDay.state.init == nullptr);
    // Unskipped Song of Time has the same Day0 dependency as moon crash: its full clock cutscene
    // does not advance dawn when the Clock Town arrival is redirected into a scene without EnTest4.
    gSaveContext.save.day = 0;
    Played(kSongOfTime);
    assert(Arrive(kClockTown) == ENTRANCE(STONE_TOWER, 0));
    PlayState ordinarySong{};
    Combo_MMAfterPlayInit(&ordinarySong);
    assert(!ordinarySong.state.running && ordinarySong.state.init == DayTelop_Init);
    // Daytime clock-shuffle starts advance their previous day; a nighttime start stays untouched.
    for (int previousDay : {1, 2}) {
        gSaveContext.save.day = previousDay;
        gSaveContext.save.time = CLOCK_TIME(6, 0) - 1;
        Played(kSongOfTime);
        assert(Arrive(kClockTown) == ENTRANCE(STONE_TOWER, 0));
        PlayState daytime{};
        Combo_MMAfterPlayInit(&daytime);
        assert(!daytime.state.running && daytime.state.init == DayTelop_Init);
        assert(gSaveContext.save.day == previousDay);
    }
    gSaveContext.save.day = 1;
    gSaveContext.save.time = CLOCK_TIME(18, 0);
    Played(kSongOfTime);
    assert(Arrive(kClockTown) == ENTRANCE(STONE_TOWER, 0));
    PlayState nighttime{};
    Combo_MMAfterPlayInit(&nighttime);
    assert(nighttime.state.running && nighttime.state.init == nullptr);
    gSaveContext.save.time = CLOCK_TIME(6, 0);
    for (const auto& b : kBoots) {
        // The boot itself lands exactly as before; it records the logical variant of where it landed.
        Run(b);
        assert(gSaveContext.save.entrance == b.lands);
        // Song of Time resets the cycle, so a cleared temple is not cleared any more.
        weekEvents.clear();
        Played(kSongOfTime);
        assert(Arrive(kClockTown) == b.initial && gSaveContext.nextCutsceneIndex == 0xFFEF);
        // Play_Init runs again after the day telop: the arrival is the initial entrance now, nothing changes.
        assert(Arrive(b.initial) == b.initial);
        // The moon crash, from any play.
        Played(ENTRANCE(TERMINA_FIELD, 12));
        assert(Arrive(kMoonCrash) == b.initial);
        // The cutscene skips land inside Play_Init.
        assert(Skip(kClockTown) == b.initial && gSaveContext.save.cutsceneIndex == 0);
        assert(Skip(kMoonCrash) == b.initial);
        // A cleared temple that stays cleared: the season fixup runs again (C10), back to where the boot landed.
        weekEvents = b.cleared;
        Played(kSongOfTime);
        assert(Arrive(kClockTown) == b.lands && gSaveContext.nextCutsceneIndex == 0xFFEF);
        assert(Skip(kMoonCrash) == b.lands);
    }
    // Only Clock Town right after the Song of Time cutscene returns: not the boot's own arrival there, nor an
    // arrival after any other play (the intro skip lands there inside the first play, too).
    const auto& farore = kBoots[8];
    assert(std::string(farore.name) == "Farore's Wind arrival" && farore.initial != kClockTown);
    Run(farore);
    assert(Arrive(kClockTown) == kClockTown);
    Played(ENTRANCE(TERMINA_FIELD, 0));
    assert(Arrive(kClockTown) == kClockTown);
    Played(kSongOfTime);
    assert(Arrive(kClockTown) == farore.initial);
    // Woodfall with its temple cleared: the fixup's scene layer goes to the cutscene index it is read from.
    Run({"Woodfall", false, false, false, -1, -1, -1, ENTRANCE(WOODFALL, 1), -1, -1, {}, ENTRANCE(WOODFALL, 1),
         ENTRANCE(WOODFALL, 1)});
    weekEvents = {WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE};
    Played(kSongOfTime);
    assert(Arrive(kClockTown) == ENTRANCE(WOODFALL, 1) && gSaveContext.nextCutsceneIndex == 0xFFF0);
    assert(Skip(kClockTown) == ENTRANCE(WOODFALL, 1) && gSaveContext.save.cutsceneIndex == 0xFFF0);
    weekEvents.clear();
    // Stone Tower switch 0x14: set in the inverted scene, cleared in the normal one, nothing elsewhere.
    switches.clear();
    Played(ENTRANCE(STONE_TOWER_INVERTED, 0), SCENE_F41);
    Played(ENTRANCE(STONE_TOWER, 0), SCENE_F40);
    Played(ENTRANCE(IKANA_CANYON, 0), static_cast<s16>(SCENE_F40 - 1));
    assert((switches == std::vector<std::pair<s32, s16>>{{0x14, SCENE_F41}, {-0x14, SCENE_F40}}));
    harnessRando = false;
    Played(ENTRANCE(STONE_TOWER_INVERTED, 0), SCENE_F41);
    Played(kSongOfTime);
    assert(Arrive(kClockTown) == kClockTown && switches.size() == 2);
    harnessRando = true;
}

int main() {
    assert(SourceForRegion(RR_BENEATH_THE_GRAVEYARD_DAMPE) == "MM_WALLMASTER_DAMPE"); // file-local, unused here
    // The pinned entrances and scenes these sites name.
    assert(kClockTown == static_cast<s32>(CE::Find("MM_CLOCK_TOWN_FROM_CLOCK_TOWER")->id));
    assert(kSongOfTime == static_cast<s32>(CE::Find("MM_CLOCK_TOWN_FROM_SONG_OF_TIME")->id));
    assert(kMoonCrash == static_cast<s32>(CE::Find("MM_CLOCK_TOWER_MOON_CRASH")->id));
    static_assert(SCENE_F40 == 0x58 && SCENE_F41 == 0x59); // scenes.yml MM_STONE_TOWER(_INVERTED)
    TruthTable();
    harnessCaps = WholeCatalog();
    // No layout, a private MM profile, a layout this build refuses.
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1);
    Inert();
    assert(MM_ApplyEntranceExpansionForCombo(LegacySeed().dump().c_str()) == 1 && CE::ActiveMmWallmasters().active());
    Inert();
    harnessCaps = CE::CapabilitySet::AtLeast(CE::Stage::Public);
    assert(MM_ApplyEntranceExpansionForCombo(TrancheSeed().dump().c_str()) == 0);
    Inert();
    harnessCaps = WholeCatalog();
    // A Water Voids layout shuffles no entrance in OoTMM's sense; the MM installer has no binding for its rows yet.
    const auto voids = SampleSeed(Find("erWallmasters=none;erOneWays=ownGame;erOneWaysWaterVoids"));
    CE::Validate(CE::DecodeEntranceLayout(voids.at("entranceLayout")), WholeCatalog());
    assert(MM_ApplyEntranceExpansionForCombo(voids.dump().c_str()) == 0);
    Inert();
    // The installed tranche layout: every site.
    assert(MM_ApplyEntranceExpansionForCombo(TrancheSeed().dump().c_str()) == 1 && Combo_MMEntranceShuffleActive());
    Active();
    // A bind clears it, even before the next MM boot records anything.
    assert(MM_ApplyEntranceExpansionForCombo("{}") == 1);
    Played(kSongOfTime);
    assert(Arrive(kClockTown) == kClockTown);
    Inert();
    // This build: a release build refuses the tranche, a Phase A dev build installs it.
    harnessCaps = CE::ActiveCapabilities();
    const int installed = MM_ApplyEntranceExpansionForCombo(TrancheSeed().dump().c_str());
#ifdef COMBO_ENTRANCE_DEV
    assert(installed == 1);
    Active();
#else
    assert(installed == 0);
    Inert();
#endif
    std::cout << "MM CFG_ER_ANY: the initial entrance, the Song of Time and moon crash return and the Stone Tower "
                 "switch act only under an installed entrance-shuffle layout\n";
}
'''

# ---- Compiled, OoT: the preflight flags, the Link's House rule, and the bind.
stage_text = function(sram, 'static void Sram_StageOpenedSave(void) {')
default = stage_text[stage_text.index('        default:'):]
default = default[:default.index('\n    }\n')]
read_save_text = function(launcher, 'static const char* Combo_ReadGameSave(int game, int fileNum) {')
assert read_save_text.count('ComboRando::Entrances::ActiveCapabilities()') == 2
read_save_text = read_save_text.replace('ComboRando::Entrances::ActiveCapabilities()', 'HarnessCapabilities()')
oot_program = r'''
#define NOMINMAX
#include <atomic>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "ComboRandomizerSavedSeed.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/UnifiedBridge.h"
#define COMBO_EXPORT
using Json = nlohmann::json;
namespace CE = ComboRando::Entrances;
// OTRGlobals.cpp, with IS_RANDO from the save.
static bool ootRando = true;
#define IS_RANDO ootRando
''' + flag_block + r'''
// Sram_StageOpenedSave's default case (z_sram.c), with the tables' values.
''' + ''.join(f'constexpr int {name} = {value};\n' for name, value in {**oot_scenes, **oot_entrances}.items()) + r'''
constexpr int YEARS_CHILD = 5, YEARS_ADULT = 17;
struct {
    int savedSceneNum = 0, entranceIndex = 0;
    bool adult = false;
} gSaveContext;
#define LINK_AGE_IN_YEARS (gSaveContext.adult ? YEARS_ADULT : YEARS_CHILD)
#define CVAR_ENHANCEMENT(var) "gEnhancements." var
bool rememberSaveLocation = false;
int CVarGetInteger(const char* name, int fallback) {
    return std::string(name) == "gEnhancements.RememberSaveLocation" ? rememberSaveLocation : fallback;
}
void StageDefault() {
    switch (gSaveContext.savedSceneNum) {
        case -1:
            break;
''' + default + r'''
    }
}
// Combo_ReadGameSave with the launcher's container service stubbed (as combo_seed_format_tests does).
std::mutex g_containerMutex;
std::map<int, Json> g_containerCache;
std::filesystem::path ComboContainerPath(int slot) { return "slot" + std::to_string(slot) + ".combosav"; }
bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
Json& LoadOrCreateContainer(int slot) { return g_containerCache[slot]; }
// The build's capability set, which the launcher reads as ActiveCapabilities().
static CE::CapabilitySet harnessCaps = CE::ActiveCapabilities();
static CE::CapabilitySet HarnessCapabilities() { return harnessCaps; }
''' + read_save_text + r'''
''' + combinations + r'''
Json Container(const Json& seed, int savedScene, bool adult) {
    const Json data = {{"savedSceneNum", savedScene}, {"linkAge", adult ? 0 : 1}};
    const Json page = {{"version", 1}, {"sections", {{"base", {{"data", data}}}}}};
    return {{"oot", page}, {"combo", {{"rando", seed}}}};
}
// SaveManager::LoadFile's order, as combo_seed_format_tests runs the real one: read the slot, preflight it, take
// the flags, then Sram_StageOpenedSave. Returns the spawn, or -1 when the slot is refused.
int Load(int slot) {
    Json page = Json::parse(Combo_ReadGameSave(ComboRando::GAME_OOT, slot));
    try {
        ComboRando::CheckComboSeedPreflight(page);
    } catch (const ComboRando::UnsupportedComboSeedFormat&) { return -1; }
    const auto stored = g_containerCache.at(slot).at("oot");
    SOH_SetEntranceRuntimeFlags(ComboRando::TakeComboEntranceRuntimeFlags(page));
    assert(page == stored); // the page applied is the page stored
    const auto& base = page.at("sections").at("base").at("data");
    gSaveContext.savedSceneNum = base.at("savedSceneNum");
    gSaveContext.adult = base.at("linkAge") == 0;
    gSaveContext.entranceIndex = 0x7777;
    StageDefault();
    return gSaveContext.entranceIndex;
}
// The OnLoadGame pair, in either order: Shipwright's Entrance_Init (Ship ER is off with M1, D1, so it leaves the
// staged spawn alone) and the launcher's bind, which sets the installed layout's flags.
void Bind(const Json& seed, bool installed, bool bindFirst) {
    const int staged = gSaveContext.entranceIndex;
    const auto bind = [&] { SOH_SetEntranceRuntimeFlags(installed ? ComboRando::SeedEntranceRuntimeFlags(seed) : 0u); };
    const auto shipEntranceInit = [] {}; // Entrance_SetSavewarpEntrance runs only with Ship ER, never with M1 (D1)
    if (bindFirst) {
        bind();
        shipEntranceInit();
    } else {
        shipEntranceInit();
        bind();
    }
    assert(gSaveContext.entranceIndex == staged);
}

int main() {
    assert(ENTR_LINKS_HOUSE_CHILD_SPAWN == static_cast<int>(CE::Find("OOT_SPAWN_CHILD")->id));
    // OoTMM's adult spawn is the Prelude arrival in the Temple of Time (oot/play/play.c:428-430).
    assert(ENTR_TEMPLE_OF_TIME_WARP_PAD == static_cast<int>(CE::Find("OOT_WARP_SONG_TEMPLE")->id));
    static_assert(SCENE_LINKS_HOUSE == 0x34); // scenes.yml OOT_LINK_HOUSE
    TruthTable();
    // The marker: taken off the page, masked to the known bits, malformed values turn nothing on.
    for (const Json& value : {Json(1u), Json(0xffffu), Json("1"), Json(-1), Json(1.5), Json::object()}) {
        Json page = {{"version", 1}, {ComboRando::kComboEntranceRuntimeFlags, value}};
        const auto flags = ComboRando::TakeComboEntranceRuntimeFlags(page);
        assert(flags == (value.is_number_unsigned() ? CE::kEntranceRuntimeErAny : 0u));
        assert(page == Json({{"version", 1}}));
    }
    Json untouched = {{"version", 1}};
    assert(ComboRando::TakeComboEntranceRuntimeFlags(untouched) == 0 && untouched == Json({{"version", 1}}));
    Json notObject = Json::array();
    assert(ComboRando::TakeComboEntranceRuntimeFlags(notObject) == 0);
    // A seed's flags: the layout's isEntranceShuffle, none without a layout; Water Voids alone shuffles nothing.
    const auto whole = WholeCatalog();
    const auto voids = SampleSeed(Find("erWallmasters=none;erOneWays=ownGame;erOneWaysWaterVoids"));
    assert(ComboRando::Profiles::ContainerEntranceRuntimeFlags(Container(TrancheSeed(), 0, true), whole) ==
           CE::kEntranceRuntimeErAny);
    assert(ComboRando::Profiles::ContainerEntranceRuntimeFlags(Container(voids, 0, true), whole) == 0);
    assert(ComboRando::Profiles::ContainerEntranceRuntimeFlags(Container(LegacySeed(), 0, true), whole) == 0);
    assert(ComboRando::Profiles::ContainerEntranceRuntimeFlags(Json(), whole) == 0);
    assert(ComboRando::SeedEntranceRuntimeFlags(Json{{"entranceLayout", Json::object()}}) == 0);
    for (const auto& c : kCombinations)
        if (!c.sample.empty())
            assert(ComboRando::SeedEntranceRuntimeFlags(SampleSeed(c)) ==
                   (c.entranceShuffle ? CE::kEntranceRuntimeErAny : 0u));

    // Link's House: the native rule without the flag, by age with it (oot/save.c:37-44). Remember Save Location
    // keeps its place before the rule, grottos and fairy fountains excepted.
    struct Case { int scene; bool adult, rsl, er; int spawn; };
    const int keep = 0x7777;
    for (const Case& t : std::vector<Case>{
             {SCENE_LINKS_HOUSE, true, false, false, ENTR_LINKS_HOUSE_CHILD_SPAWN},
             {SCENE_LINKS_HOUSE, true, false, true, ENTR_TEMPLE_OF_TIME_WARP_PAD},
             {SCENE_LINKS_HOUSE, false, false, false, ENTR_LINKS_HOUSE_CHILD_SPAWN},
             {SCENE_LINKS_HOUSE, false, false, true, ENTR_LINKS_HOUSE_CHILD_SPAWN},
             {SCENE_TEMPLE_OF_TIME, true, false, false, ENTR_TEMPLE_OF_TIME_WARP_PAD},
             {SCENE_TEMPLE_OF_TIME, false, false, true, ENTR_LINKS_HOUSE_CHILD_SPAWN},
             {SCENE_LINKS_HOUSE, true, true, false, keep},
             {SCENE_LINKS_HOUSE, true, true, true, keep},
             {SCENE_GROTTOS, true, true, true, ENTR_TEMPLE_OF_TIME_WARP_PAD},
             {SCENE_FAIRYS_FOUNTAIN, false, true, false, ENTR_LINKS_HOUSE_CHILD_SPAWN}}) {
        SOH_SetEntranceRuntimeFlags(t.er ? CE::kEntranceRuntimeErAny : 0u);
        rememberSaveLocation = t.rsl;
        gSaveContext.savedSceneNum = t.scene;
        gSaveContext.adult = t.adult;
        gSaveContext.entranceIndex = keep;
        StageDefault();
#ifdef COMBO_BUILD
        assert(gSaveContext.entranceIndex == t.spawn);
#else
        assert(gSaveContext.entranceIndex == (t.er && t.spawn != keep && t.scene == SCENE_LINKS_HOUSE
                                                  ? ENTR_LINKS_HOUSE_CHILD_SPAWN : t.spawn));
#endif
    }
    rememberSaveLocation = false;
    // The flag needs a randomizer save, and unknown bits do nothing.
    SOH_SetEntranceRuntimeFlags(0xfffffffeu);
    assert(!Combo_OOTEntranceShuffleActive());
    SOH_SetEntranceRuntimeFlags(CE::kEntranceRuntimeErAny);
    ootRando = false;
    assert(!Combo_OOTEntranceShuffleActive());
    ootRando = true;
    assert(Combo_OOTEntranceShuffleActive());

#ifdef COMBO_BUILD
    // C9 through the launcher's own preflight read. Slot 0: the tranche layout; slot 1: no layout; slot 2: a
    // private MM profile. Each saved by an adult in Link's House.
    g_containerCache[0] = Container(TrancheSeed(), SCENE_LINKS_HOUSE, true);
    g_containerCache[1] = Container(Json{{"fileType", ComboRando::kComboSeedFileType}}, SCENE_LINKS_HOUSE, true);
    g_containerCache[2] = Container(LegacySeed(), SCENE_LINKS_HOUSE, true);
    const auto bytes = [](int slot) { return g_containerCache.at(slot).at("oot").dump(); };
    // Pages without a layout are the stored bytes, as before.
    assert(Combo_ReadGameSave(ComboRando::GAME_OOT, 1) == bytes(1));
    assert(Combo_ReadGameSave(ComboRando::GAME_OOT, 2) == bytes(2));
    assert(std::string(Combo_ReadGameSave(ComboRando::GAME_MM, 0)).empty());
    // A build with the tranche (Phase A dev builds): the very first load of slot 0, before any bind, starts at
    // the Temple of Time, and either OnLoadGame order keeps it.
    harnessCaps = WholeCatalog();
    for (bool bindFirst : {true, false}) {
        SOH_SetEntranceRuntimeFlags(0);
        assert(Load(0) == ENTR_TEMPLE_OF_TIME_WARP_PAD);
        Bind(TrancheSeed(), true, bindFirst);
        assert(Combo_OOTEntranceShuffleActive());
        // Slot 1 next: its own preflight clears the flag before its spawn, so slot 0's rule never leaks.
        assert(Load(1) == ENTR_LINKS_HOUSE_CHILD_SPAWN && !Combo_OOTEntranceShuffleActive());
        Bind(Json::object(), true, bindFirst);
        assert(Load(0) == ENTR_TEMPLE_OF_TIME_WARP_PAD);
        // A bind whose install fails turns the flag off for the rest of the session.
        Bind(TrancheSeed(), false, bindFirst);
        assert(!Combo_OOTEntranceShuffleActive());
        assert(Load(2) == ENTR_LINKS_HOUSE_CHILD_SPAWN);
    }
    // A build without it (every release build in Phase A): the layout slot is refused at the preflight, before
    // any flag is set.
    harnessCaps = CE::CapabilitySet::AtLeast(CE::Stage::Public);
    SOH_SetEntranceRuntimeFlags(0);
    assert(Load(0) == -1 && !Combo_OOTEntranceShuffleActive());
    assert(Json::parse(Combo_ReadGameSave(ComboRando::GAME_OOT, 0)).contains(ComboRando::kComboSeedPreflightError));
    assert(!Json::parse(Combo_ReadGameSave(ComboRando::GAME_OOT, 0)).contains(ComboRando::kComboEntranceRuntimeFlags));
    // This build's own set.
    harnessCaps = CE::ActiveCapabilities();
    assert(Load(0) == (harnessCaps.Active(CE::kMmTrancheCapability) ? ENTR_TEMPLE_OF_TIME_WARP_PAD : -1));
    for (int slot : {1, 2}) {
        assert(Load(slot) == ENTR_LINKS_HOUSE_CHILD_SPAWN && !Combo_OOTEntranceShuffleActive());
        Bind(g_containerCache.at(slot).at("combo").at("rando"), true, slot == 1);
        assert(!Combo_OOTEntranceShuffleActive());
    }
#endif
    std::cout << "OoT CFG_ER_ANY: the slot's flags reach the spawn choice before the bind, a Link's House save is "
                 "not honoured under entrance shuffle, and nothing changes without it\n";
}
'''
for name, program, defines in (
        ('er_any_mm', mm_program, ['COMBO_BUILD']), ('er_any_mm_dev', mm_program, ['COMBO_BUILD', 'COMBO_ENTRANCE_DEV=1']),
        ('er_any_oot', oot_program, ['COMBO_BUILD']), ('er_any_oot_native', oot_program, [])):
    cpp = work / (name + '.cpp')
    cpp.write_text(program, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[root / 'combo', args.json_include],
                        defines=['_CRT_SECURE_NO_WARNINGS'] + defines, werror=True, msvc=['/bigobj'],
                        gnu=['-Wno-missing-field-initializers'])
    subprocess.run([str(exe)], cwd=work, check=True, timeout=hostcxx.RUN_TIMEOUT)
print('CFG_ER_ANY: initial entrance, Song of Time and moon crash return, Stone Tower switch and Link\'s House ported; '
      'the end-of-game save is not applicable; every site acts only under an installed entrance-shuffle layout.')
