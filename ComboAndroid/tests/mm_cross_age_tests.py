"""Cross-Games Age (OoTMM v32.3 crossAge, plan F5b 5a and the seed and menu half of 5b-9/5b-10): logic and age state.

Checks the production sources for: the MM logic age (Logic.h MM_LOGIC_AGE) and the v32.3 age macros
(macros_mm.yml:59-62).
"""
from pathlib import Path
import argparse
import re
import sys

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
check('if(id!=RO_MM_CROSS_AGE||!sMMSeedFaroreRulesBound)' in init,
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

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print('Cross-Games Age: logic age and age macros match OoTMM v32.3')
