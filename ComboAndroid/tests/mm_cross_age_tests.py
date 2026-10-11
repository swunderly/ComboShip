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

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print('Cross-Games Age: logic age and age macros match OoTMM v32.3')
