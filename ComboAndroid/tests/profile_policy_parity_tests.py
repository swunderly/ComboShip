"""Check that the profile editor's copy of SoH's option callbacks still matches settings.cpp.

ProfileOotOptionPolicy.h replays every OPT_CALLBACK body (and the three helpers they call) on a
profile document instead of live CVars. A change to one copy that misses the other would make
the profile editor hide, disable or clamp options differently from native generation.
"""
from pathlib import Path
import argparse
import re

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
root = p.parse_args().source.resolve()
native = (root / 'soh/soh/Enhancements/randomizer/settings.cpp').read_text(encoding='utf-8')
policy = (root / 'combo/gui/ProfileOotOptionPolicy.h').read_text(encoding='utf-8')


def body(text, opening):
    depth = 0
    for i in range(opening, len(text)):
        depth += (text[i] == '{') - (text[i] == '}')
        if depth == 0:
            return text[opening + 1:i]
    raise AssertionError('unbalanced braces')


def normalize(code):
    # Comments and layout may differ; so may the splitting of long string literals.
    code = re.sub(r'//[^\n]*', '', code)
    code = re.sub(r'\s+', '', code)
    return code.replace('""', '')


callbacks = {}, {}
for m in re.finditer(r'OPT_CALLBACK\((RSK_\w+),\s*\{', native):
    callbacks[0][m.group(1)] = body(native, m.end() - 1)
for m in re.finditer(r'void Callback_(RSK_\w+)\(\)\s*\{', policy):
    callbacks[1][m.group(1)] = body(policy, m.end() - 1)
for helper in ['HandleShopsanityPriceUI', 'HandleMixedEntrancePoolsUI', 'HandleStartingAgeUI']:
    found = re.search(r'void Settings::' + helper + r'\(\)\s*\{', native), re.search(r'void ' + helper + r'\(\)\s*\{', policy)
    assert all(found), ('helper missing', helper)
    callbacks[0][helper] = body(native, found[0].end() - 1)
    callbacks[1][helper] = body(policy, found[1].end() - 1)

assert callbacks[0].keys() == callbacks[1].keys(), (
    'callbacks only in settings.cpp: %s; only in the profile policy: %s' %
    (sorted(callbacks[0].keys() - callbacks[1].keys()), sorted(callbacks[1].keys() - callbacks[0].keys())))
# Every native callback also needs a registration in the policy constructor.
for key in callbacks[0]:
    if key.startswith('RSK_'):
        assert 'callbacks[%s]' % key in policy, ('callback not registered in the profile policy', key)
different = sorted(key for key in callbacks[0] if normalize(callbacks[0][key]) != normalize(callbacks[1][key]))
assert not different, ('profile policy callbacks differ from settings.cpp', different)
print('PASS: %d option callbacks and helpers match settings.cpp' % len(callbacks[0]))
