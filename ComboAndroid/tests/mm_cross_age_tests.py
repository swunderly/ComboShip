"""Cross-Games Age (OoTMM v32.3 crossAge, plan F5b 5a and the seed and menu half of 5b-9/5b-10): logic and age state.

Checks the production sources for: the MM logic age (Logic.h MM_LOGIC_AGE) and the v32.3 age macros
(macros_mm.yml:59-62); the age state (one ConfigureSeedAge contract, no load or age step needing the adult model);
OoT's age copied in at every MM entry in the critique X12 order; the check tracker refresh; the seed's value being
authoritative; the per-age generation oracle; the Stone Tower graph chosen per seed; and the receipt of every v32.3 MM
age line (fixtures/ootmm-32.3-mm-age-lines.json): each row's fork statements carry the age term in their region, every
edited statement evaluates exactly as before with Cross-Games Age off and differently with it on, and, when the OoTMM
checkout is available (--reference, or references/ootmm-stable-32.3 beside the worktree), the rows are exactly the
v32.3 lines.
"""
from pathlib import Path
import argparse
import json
import random
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mm_gear_logic_tests as cexpr  # noqa: E402 (its C expression reader and evaluator)

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--reference', type=Path)
a = p.parse_args()
root = a.source.resolve()
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def squash(text):
    """Text without comments, whitespace or macro line continuations (clang-format moves all three)."""
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    return re.sub(r'\s+', '', re.sub(r'\\\s*(?=\n|$)', ' ', text))


def body(source, signature):
    """Text of the definition that starts at signature, up to its matching brace (squashed)."""
    m = re.search(re.escape(signature) + r'[^;{]*\{', source)
    if not m:
        failures.append(f'missing definition {signature}')
        return ''
    depth, i = 0, m.end() - 1
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return squash(source[m.start():i])


benport = read('mm/2s2h/BenPort.cpp')

# ---- 5a-1: the logic age. Legacy MM stores linkAge 0 for its child Link; only an enabled adult state's age counts.
logic_h = squash(read('mm/2s2h/Rando/Logic/Logic.h'))
check('#defineMM_LOGIC_AGE(gSaveContext.save.shipSaveInfo.adultMm.enabled?gSaveContext.save.linkAge:1)' in logic_h,
      'Logic.h: MM_LOGIC_AGE must read linkAge only while adultMm.enabled')
for name, fn in (('MM_LOGIC_IS_CHILD', 'IsChild'), ('MM_LOGIC_IS_ADULT', 'IsAdult')):
    check(f'#define{name}ComboRando::MmAgeLogic::{fn}(RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE]!=0,MM_LOGIC_AGE)' in logic_h,
          f'Logic.h: {name} must read the seed option and MM_LOGIC_AGE only')
check('#defineMM_LOGIC_IS_TALLComboRando::MmAgeLogic::IsTall(HAS_ITEM(ITEM_MASK_ZORA),MM_LOGIC_IS_ADULT)' in logic_h and
      '#defineMM_LOGIC_IS_SHORTComboRando::MmAgeLogic::IsShort(HAS_ITEM(ITEM_MASK_DEKU),MM_LOGIC_IS_CHILD)' in logic_h,
      'Logic.h: is_tall/is_short must follow macros_mm.yml:61-62')
age_logic = squash(read('combo/rando/MmAgeLogic.h'))
for expr in ('return!crossAge||exploredAge==1;', 'returncrossAge&&exploredAge==0;', 'returnzoraMask||adult;',
             'returndekuMask||child;'):
    check(expr in age_logic, f'MmAgeLogic.h lacks {expr}')
for legacy in ('MM_LOGIC_STARTING_AGE', 'MM_HAS_ADULT_MASK', 'gSaveContext.save.linkAge)'):
    check(not re.search(r'#defineMM_LOGIC_IS_(CHILD|ADULT)[^#]*' + re.escape(legacy), logic_h),
          f'Logic.h: the age macros must not read {legacy}')

# ---- 5a-2: age state. Cross-Games Age stores an age only (util.c:74-81, builder.ts:319): one ConfigureSeedAge
# contract with no loadout swap for it, the new file starts child, and no age step needs the adult resources.
settings = body(read('mm/2s2h/ComboAdultSettings.h'), 'inline bool ConfigureSeedAge(')
cross_exit = settings.find('if(!maskEnabled&&startingAge==1)returntrue;')
check(0 < cross_exit < settings.find('InitializeLoadouts(state);') < settings.find('SwapHumanLoadout('),
      'ConfigureSeedAge: Cross-Games Age alone must return before the 41a2b7c0 loadout swap')
check(settings.find('state.enabled=1;') < cross_exit and settings.find('save.linkAge=1;') < cross_exit,
      'ConfigureSeedAge: Cross-Games Age must start child with the adult state enabled')
bound = body(benport, 'extern "C" int Combo_MM_AdultConfigureBoundSeed(int fileNum)')
check('if((mask||age==0)&&!Combo_MM_AdultResourcesReady())return0;' in bound and
      'returnComboAdultMM::ConfigureSeedAge(save,age,mask,cross);' in bound,
      'Combo_MM_AdultConfigureBoundSeed must not need the adult resources for Cross-Games Age')
init = body(benport, 'extern "C" COMBO_EXPORT int MM_InitRandoSaveFile(')
applied = init.find('Rando::Spoiler::ApplyToSaveContext(spoiler);')
check(0 < applied < init.find('Combo_MM_AdultConfigureBoundSeed(fileNum);'),
      'MM_InitRandoSaveFile must set the seed age state after the options are saved')
for name in ('MM_GetEffectiveHumanAge(int fileNum)', 'MM_CanFaroreTransitionAge(int fileNum, int age)',
             'MM_ApplyFaroreTransitionAge(int fileNum, int age)'):
    fn = body(benport, 'extern "C" COMBO_EXPORT int ' + name)
    for banned in ('ResourcesReady', 'BuildAdultHybridSequence', 'SwapHumanLoadout', 'ApplyAdultModelTables',
                   'ApplyAdultOverlayAge'):
        check(banned not in fn, f'{name.split("(")[0]} must not use {banned}')
check('if(save.shipSaveInfo.adultMm.enabled)save.linkAge=(u8)age;return1;' in
      body(benport, 'extern "C" COMBO_EXPORT int MM_ApplyFaroreTransitionAge(int fileNum, int age)'),
      'MM_ApplyFaroreTransitionAge must set the saved age only')

# ---- 5a-3: OoT's age reaches MM at every entry, in the critique X12 order: arrival resolution, age sync, (Plan 4's
# initial-entrance record), save-load hooks. Farore's Wind and shuffled-entrance arrivals use the same setter.
setup = squash(read('mm/src/code/title_setup.c'))
arrival = setup.find('constintsongArrival=MM_ConsumeSongArrival();')
sync = setup.find('Combo_MM_SyncCrossAgeOnEntry();')
check(0 < arrival < setup.find('gSaveContext.save.entrance=songArrival;') < sync <
      setup.find('GameInteractor_ExecuteOnSaveLoad(gSaveContext.fileNum);'),
      'title_setup.c: the age sync must follow the arrival and precede the save-load hooks')
sync_fn = body(benport, 'extern "C" int Combo_MM_SyncCrossAgeOnEntry(void)')
for term in ('constboolactive=IS_RANDO&&RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE];',
             'if(file<0||file>2||(!active&&(!state.enabled||state.maskEnabled)))return0;',
             'constintootAge=active&&sCrossAgeOotAge?sCrossAgeOotAge(file):-1;', 'state.enabled=1;',
             'MM_ApplyFaroreTransitionAge(file,ComboRando::MmAgeLogic::EntryAge(active,ootAge,save.linkAge));'):
    check(term in sync_fn, f'Combo_MM_SyncCrossAgeOnEntry lacks {term}')
check('if(!active)return1;if(ootAge==0||ootAge==1)returnootAge;returnstoredAge==0?0:1;' in
      body(read('combo/rando/MmAgeLogic.h'), 'inline int EntryAge('), 'MmAgeLogic::EntryAge differs')
check('if(arrival->crossAge&&!MM_ApplyFaroreTransitionAge(arrival->file,arrival->age))return-1;' in
      squash(read('mm/2s2h/Rando/CrossEntrance.inc.h')), 'shuffled-entrance arrivals must use the age setter')
farore = squash(read('mm/2s2h/Rando/FaroresWind.cpp'))
check('(sApplyAge?!sApplyAge(gSaveContext.fileNum,sArrivalAge)' in farore, 'Farore arrivals must use the age setter')
combo = read('combo/ComboShip.cpp')
oot_age = body(combo, 'static int Combo_CrossAgeOotAge(int fileNum)')
check('constintlive=SOH_GetFaroreAge?SOH_GetFaroreAge(fileNum):-1;' in oot_age and
      'Combo_ReadGameSave(ComboRando::GAME_OOT,fileNum);' in oot_age and
      '.at("sections").at("base").at("data").at("linkAge")' in oot_age,
      'Combo_CrossAgeOotAge must read the live OoT age, else the OoT page linkAge')
check('SaveManager::Instance->SaveData("linkAge",saveContext->linkAge);' in
      body(read('soh/soh/SaveManager.cpp'), 'void SaveManager::SaveBase('), 'OoT no longer saves linkAge in "base"')
check('GetSym(mmModule,"MM_SetCrossAgeOotAgeBridge"))set(Combo_CrossAgeOotAge);' in squash(combo),
      'ComboShip.cpp does not bind the Cross-Games Age OoT age bridge')

# ---- 5a-7: the check tracker recomputes as soon as the entry sync changes the age its macros read.
check('if(MM_GetEffectiveHumanAge(file)==before)return0;Rando::CheckTracker::MarkChecksInLogicStale();return1;' in
      sync_fn, 'an age change must mark the check tracker stale')
tracker = read('mm/2s2h/Rando/CheckTracker/CheckTracker.cpp')
refresh = body(tracker, 'void RefreshChecksInLogic()')
check('if(sChecksInLogicStale&&gGameState!=NULL)lastFrame=gGameState->frames-20;' in refresh and
      refresh.find('lastFrame=gGameState->frames-20;') < refresh.find('gGameState->frames-lastFrame<20') <
      refresh.find('sChecksInLogicStale=false;') < refresh.find('ComputeChecksInLogic(checksInLogic);'),
      'RefreshChecksInLogic must skip its interval for a stale tracker and clear the mark when it refreshes')
check('sChecksInLogicStale=true;' in body(tracker, 'void Rando::CheckTracker::MarkChecksInLogicStale()'),
      'MarkChecksInLogicStale must set the stale mark')

# ---- 5a-4: an existing seed's Cross-Games Age is authoritative; device readiness only gates new seeds.
check('returnrules.crossAge&&ComboRando::ComboFaroreIntegrationReady;' in
      body(combo, 'static bool EffectiveCrossAge(const ComboRando::FaroreRules& rules)'),
      'EffectiveCrossAge must use the seed and the native integration only')
check('MM_AdultRuntimeReady' not in combo + benport, 'MM_AdultRuntimeReady must not gate a seed\'s Cross-Games Age')
check('(id!=RO_MM_CROSS_AGE||!sMMSeedFaroreRulesBound)' in init,
      'MM_InitRandoSaveFile must keep a bound seed\'s Cross-Games Age')

# ---- 5a-5: the generation oracle explores each MM entry age (pathfind.ts:263-267, 342-346) for the seed's option alone,
# MM open at the start is entered as child, and the 41a2b7c0 Adult Mask frontier clone is gone.
check('returngSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE]!=0;' in
      body(benport, 'extern "C" COMBO_EXPORT bool MM_UseCrossAgeOracle(void)'),
      'MM_UseCrossAgeOracle must read the seed option only')
check('return2;' in body(benport, 'extern "C" COMBO_EXPORT uint8_t MM_GetOracleDefaultEntryAges(void)'),
      'MM_GetOracleDefaultEntryAges must be child')
reset = body(benport, 'extern "C" COMBO_EXPORT void Combo_MM_Rando_Reset(void)')
check('gSaveContext.save.linkAge=1;' in reset and
      'gSaveContext.save.shipSaveInfo.adultMm.enabled=gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_CROSS_AGE]!=0;'
      in reset, 'the oracle snapshot must start child with the age state enabled exactly for Cross-Games Age')
reach = body(benport, 'extern "C" COMBO_EXPORT const char* Combo_MM_Rando_GetReachableChecks(void)')
check('clone' not in reach and 'maskOwnedBits' not in reach and 'MergeTimeStates' not in reach,
      'the oracle must not clone frontiers between ages')
check('gSaveContext.save.linkAge=age;' in reach, 'the oracle must explore each entry age with its own linkAge')


def enclosing_call(text, at):
    """Name of the innermost call whose parentheses contain text[at]."""
    depth = 0
    for j in range(at - 1, -1, -1):
        if text[j] == ')':
            depth += 1
        elif text[j] == '(':
            if depth == 0:
                return re.search(r'(\w*)$', text[:j]).group(1)
            depth -= 1
    return ''


# ---- 5a-6: the Stone Tower graph (stone_tower.yml:32) is picked per seed. A seed bound after ShipInit::Init (or
# "RANDO_LOGIC") gets the right graph because every pinned choice is made when its edge is evaluated: the switch reads
# only the bound seed's options, appears only inside CHECK/CONNECTION/EXIT conditions (each a lambda), the front of the
# temple region is always built, and the temple arrival picks its region on each call, before the entrance cache.
check('#defineMM_LOGIC_STONE_TOWER_PINNED(RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE]||RANDO_SAVE_OPTIONS[RO_MM_BOOTS_HOVER])' in
      logic_h, 'MM_LOGIC_STONE_TOWER_PINNED must read the bound seed options only')
# Comments and preprocessor lines (the #define of the switch, #ifdef COMBO_BUILD) removed, layout kept.
east = '\n'.join(line for line in re.sub(r'//[^\n]*', '', read('mm/2s2h/Rando/Logic/Regions/East.cpp')).splitlines()
                 if not line.lstrip().startswith('#'))
uses = [m.start() for m in re.finditer(r'MM_LOGIC_STONE_TOWER_PINNED|COMBO_STONE_TOWER_SELECT', east)]
check(len(uses) > 30, f'only {len(uses)} Stone Tower pinned uses found')
for at in uses:
    caller = enclosing_call(east, at)
    check(caller in ('CHECK', 'CONNECTION', 'EXIT', 'COMBO_STONE_TOWER_SELECT'),
          f'East.cpp: a Stone Tower graph choice is made outside an edge condition ({caller}: {east[at - 60:at + 40]!r})')
front = east.find('Regions[RR_MM_STONE_TOWER_FRONT_OF_TEMPLE] = RandoRegion{')
check(front > 0 and enclosing_call(east, front) == 'initFunc' and
      not re.search(r'\b(if|else)\b|\?', east[max(east.rfind(';', 0, front), east.rfind('}', 0, front)) + 1:front]),
      'East.cpp: the Stone Tower front of temple region must be built unconditionally at init')
entry = body(read('mm/2s2h/Rando/Logic/Logic.cpp'), 'RandoRegionId GetRegionIdFromEntrance(s32 entrance)')
check(0 < entry.find('if(MM_LOGIC_STONE_TOWER_PINNED&&entrance==ENTRANCE(STONE_TOWER,2))returnRR_MM_STONE_TOWER_FRONT_OF_TEMPLE;')
      < entry.find('staticstd::map<s32,RandoRegionId>entranceToRegionId;') and
      'randoRegionId==RR_MM_STONE_TOWER_FRONT_OF_TEMPLE)continue;' in entry,
      'GetRegionIdFromEntrance must pick the Stone Tower temple arrival per call and keep it out of its cache')

# ---- 5b-9 (seed half): readiness. The release switch and OoT's adult model (looked up at run time, absent = not ready,
# no MM resource manager needed) decide whether a new seed may turn Cross-Games Age on; every place options are
# gathered uses that, except a bound seed's own value; the headless generator follows the release switch alone.
benport = read('mm/2s2h/BenPort.cpp')
check('staticconstexprboolkCrossAgeRuntimeComplete=' in squash(benport), 'no kCrossAgeRuntimeComplete release switch')
check('returnkCrossAgeRuntimeComplete;' in body(benport, 'static int Combo_MM_CrossAgeIntegrationEnabled(void)'),
      'the Cross-Games Age runtime does not follow its release switch')
ready = body(benport, 'extern "C" int Combo_MM_CrossAgeReady(void)')
check(ready.startswith('extern"C"intCombo_MM_CrossAgeReady(void){if(!Combo_MM_CrossAgeIntegrationEnabled())return0;') and
      'Combo_ResolveSym("2ship","Combo_MM_CrossAgeModelReady")' in ready and
      ready.endswith('returnmodelReady&&modelReady();}'),
      'Combo_MM_CrossAgeReady must need the release switch and the model loader, a missing loader reading as not ready')
for banned in ('CrossRMRegistry', 'ResourceManager', 'gPlayState', 'AdultResourcesReady'):
    check(banned not in ready, f'Combo_MM_CrossAgeReady must not use {banned}')
check('returnCombo_MM_CrossAgeIntegrationEnabled()&&(sCrossAgeHeadlessGeneration||Combo_MM_CrossAgeReady());' in
      body(benport, 'extern "C" int Combo_MM_CrossAgeOptionReady(void)'), 'Combo_MM_CrossAgeOptionReady differs')
check('returnCombo_MM_CrossAgeOptionReady();' in body(benport, 'extern "C" COMBO_EXPORT int MM_CrossAgeOptionReady(void)'),
      'MM_CrossAgeOptionReady must export the option readiness')
check('Combo_MM_CrossAgeForHeadlessGeneration();' in body(benport, 'extern "C" COMBO_EXPORT void MM_InitRandoHeadless(void)'),
      'the headless generator must follow the Cross-Games Age release switch alone')
check('returnid==RO_MM_CROSS_AGE?Combo_MM_CrossAgeOptionReady()!=0:Combo_MM_AdultResourcesReady()!=0;' in
      body(benport, 'static bool Combo_MM_HiddenAdultOptionReady(RandoOptionId id)'),
      'Cross-Games Age must have its own readiness, apart from the dormant Adult Mask')
flat = squash(benport)
check(flat.count('Combo_MM_HiddenAdultOptionReady(id)') == 5 and
      'if(!Combo_MM_AdultResourcesReady())for(constautoid:kComboHiddenAdultMmOptions)' not in flat,
      'every place adult options are gathered must use their own readiness')
check('for(constautoid:kComboHiddenAdultMmOptions)if(!Combo_MM_HiddenAdultOptionReady(id))options[id]=0;' in
      body(benport, 'extern "C" void Combo_MM_ClearUnavailableOptions(u32* options)'),
      'new seeds must leave Cross-Games Age off where it cannot run')
init = body(benport, 'extern "C" COMBO_EXPORT int MM_InitRandoSaveFile(')
check('if((id!=RO_MM_CROSS_AGE||!sMMSeedFaroreRulesBound)&&!Combo_MM_HiddenAdultOptionReady(id))' in init,
      'a new file must keep a bound seed\'s Cross-Games Age and gate only unbound options')

# ---- 5b-10 (menu half): the World access row with OoTMM's text, greyed out and unticked where the option cannot run,
# a search link, a CVar for the option, and no preset or standalone rule (critique X9, lead decision D7).
CROSS_AGE_NAME = 'Cross-Games Age'
CROSS_AGE_TEXT = 'When you enter MM as Adult Link, you will be Adult Link in MM.'
menu = read('combo/gui/ComboMenu.cpp')
goals = body(menu, 'void DrawCombinedGoals()')
row = goals.find('DrawRuleCheckbox("' + squash(CROSS_AGE_NAME) + '",&crossAge)')
check(0 < goals.find(squash('DrawRuleCheckbox("Cross-Games Farore\'s Wind", &crossFw)')) < row,
      'the Cross-Games Age row must follow Cross-Games Farore\'s Wind in World access')
for term in ('Combo_ResolveSym("2ship","MM_CrossAgeOptionReady")', 'constboolageReady=crossAgeReady&&crossAgeReady();',
             'boolcrossAge=ageReady&&ProfileUI::ReadInteger("gRando.Options.RO_MM_CROSS_AGE",0)!=0;',
             'ImGui::BeginDisabled(!ageReady);',
             'ProfileUI::WriteInteger("gRando.Options.RO_MM_CROSS_AGE",crossAge);',
             'MenuTooltip(ageReady?"' + squash(CROSS_AGE_TEXT) + '":"' + squash(CROSS_AGE_TEXT)):
    check(term in goals, f'the Cross-Games Age row lacks {term}')
links = squash(body(menu, 'void ComboMenu::DrawSearchResults(const std::string& query)'))
m = re.search(r'addLink\(MenuSearchKind::Setting,"' + squash(CROSS_AGE_NAME) + r'","([^"]*)","randomizer","Logic&access",'
              r'"Randomizer"\);', links)
check(m is not None, 'no search link to Cross-Games Age')
check('RO(RO_MM_CROSS_AGE,RO_GENERIC_NO)' in squash(read('mm/2s2h/Rando/StaticData/Options.cpp')),
      'RO_MM_CROSS_AGE must have its CVar (gRando.Options.RO_MM_CROSS_AGE) for the menu row')
check('RO_MM_CROSS_AGE' not in read('combo/rando/ComboPresets.h'), 'Cross-Games Age must not be in a preset')
check('CrossGamesAge' not in read('combo/rando/GameplayRules.h') + menu,
      'Cross-Games Age must not have a standalone enhancement')

# ---- 5a-8: every v32.3 MM age line (data/world/mm, plus macros_mm.yml:260) and the fork statements that carry it.
fixture = json.loads((Path(__file__).resolve().parent / 'fixtures/ootmm-32.3-mm-age-lines.json')
                     .read_text(encoding='utf-8'))
rows = fixture['rows']
statuses = {}
for row in rows:
    statuses[row['status']] = statuses.get(row['status'], 0) + 1
check(len(rows) == 85 and set(statuses) <= {'covered', 'needs_change', 'na_no_graph'},
      f'the receipt must list the 84 world lines and the macro line ({len(rows)} rows, {statuses})')
for switch in fixture['switches']:
    m = re.search(r'#define' + switch['name'] + r'([^#]*)', squash(read(switch['file'])))
    check(m and 'RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE]' in m.group(1),
          f'{switch["file"]}: {switch["name"]} must switch on with Cross-Games Age')
sources = {}
for row in rows:
    check(bool(row['sites']) == (row['status'] != 'na_no_graph'), f'{row["ootmm"]}: status and sites disagree')
    for site in row['sites']:
        if site['file'] not in sources:
            sources[site['file']] = read(site['file'])
        text = squash(cexpr.region(sources[site['file']], site['region']))
        statement = squash(site['text'])
        check(statement in text, f'{row["ootmm"]}: {site["file"]} {site["region"] or ""} lacks {site["text"][:90]}')
        token = site['token']
        if token in fixture['helpers']:
            helper = body(sources[site['file']], 'inline bool ' + token + '()')
            check(token + '()' in statement and fixture['helpers'][token] in helper,
                  f'{row["ootmm"]}: {token} must carry {fixture["helpers"][token]}')
        elif token:
            check(re.search(re.escape(token) + r'(?!\w)', statement) is not None, f'{row["ootmm"]}: no {token}')
for helper, needed in fixture['helpers'].items():
    check(needed in body(read('mm/2s2h/Rando/Logic/MmSwampContexts.inc.h'), 'inline bool ' + helper + '()'),
          f'MmSwampContexts.inc.h: {helper} must use {needed}')
# Each edit is additive: with Cross-Games Age off the statement evaluates exactly as before; with it on it changes.
# The age macros are written out as MmAgeLogic computes them (checked above and in combined_logic_tests.cpp), with the
# seed option as an unknown the evaluator can pin to 0.
macros = cexpr.defines(read('mm/2s2h/Rando/Logic/Logic.h'), lambda name: name.startswith('MM_LOGIC_'))
for name, expansion in (('MM_LOGIC_IS_ADULT', '(RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] && MM_LOGIC_AGE_IS_ADULT)'),
                        ('MM_LOGIC_IS_CHILD', '(!RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] || !MM_LOGIC_AGE_IS_ADULT)'),
                        ('MM_LOGIC_IS_TALL', '(HAS_ITEM(ITEM_MASK_ZORA) || MM_LOGIC_IS_ADULT)'),
                        ('MM_LOGIC_IS_SHORT', '(HAS_ITEM(ITEM_MASK_DEKU) || MM_LOGIC_IS_CHILD)')):
    macros[name] = (None, cexpr.tokenize(expansion))
rng = random.Random(20261011)
edited = 0
for row in rows:
    if row['status'] != 'needs_change':
        continue
    site = row['sites'][0]
    known = dict(macros, **cexpr.defines(sources[site['file']]))
    before, after = (cexpr.condition(row['before']), known), (cexpr.condition(site['text']), known)
    check(cexpr.compare(before, after, 800, rng, ('RO_MM_CROSS_AGE',), (), (set(), set())) is None,
          f'{row["ootmm"]}: the edit changes the rule without Cross-Games Age')
    check(cexpr.compare(before, after, 3000, rng, (), (), (set(), set())) is not None,
          f'{row["ootmm"]}: the edit changes nothing with Cross-Games Age on')
    check(squash(row['before']) not in squash(cexpr.region(sources[site['file']], site['region'])) or
          '#else' in squash(cexpr.region(sources[site['file']], site['region'])),
          f'{row["ootmm"]}: the old statement is still active')
    edited += 1
# Against OoTMM v32.3 itself: the rows are exactly its age lines, with their text.
reference = a.reference
if reference is None:
    for candidate in (root.parent.parent / 'references/ootmm-stable-32.3', root.parent / 'references/ootmm-stable-32.3'):
        if (candidate / 'data/macros/macros_mm.yml').exists():
            reference = candidate
            break
if reference is not None:
    found = set()
    for f in sorted((reference / 'data/world/mm').rglob('*.yml')):
        for i, line in enumerate(f.read_text(encoding='utf-8').splitlines(), 1):
            if re.search(r'\b(is_adult|is_child|is_tall|is_short)\b', line):
                found.add(f'data/world/mm/{f.relative_to(reference / "data/world/mm").as_posix()}:{i}')
    found.add('data/macros/macros_mm.yml:260')
    check(found == {row['ootmm'] for row in rows}, 'the receipt rows differ from the OoTMM v32.3 age lines: ' +
          str(sorted(found ^ {row['ootmm'] for row in rows})))
    for row in rows:
        rel, line = row['ootmm'].rsplit(':', 1)
        check(row['expr'] in (reference / rel).read_text(encoding='utf-8').splitlines()[int(line) - 1],
              f'{row["ootmm"]} does not read {row["expr"][:80]}')
    data_ts = (reference / 'packages/core/src/settings/data.ts').read_text(encoding='utf-8')
    setting = data_ts[data_ts.index("  key: 'crossAge',"):]
    setting = setting[:setting.index('}')]
    check(f"name: '{CROSS_AGE_NAME}'," in setting and f"description: '{CROSS_AGE_TEXT}'," in setting and
          "default: false," in setting, 'the Cross-Games Age row text differs from OoTMM data.ts')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources against the fixture only')

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print(f'Cross-Games Age: age macros, age state, entry sync, oracle and Stone Tower match OoTMM v32.3; {len(rows)} age '
      f'lines ({statuses}), {edited} edits unchanged with the option off')
