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

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print('Cross-Games Age: logic age and age macros match OoTMM v32.3')
