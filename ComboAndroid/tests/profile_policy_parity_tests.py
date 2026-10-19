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

# CreateOptions' Master Quest gate is not a callback. The policy disables and enables the same rows with the same text,
# on the profile catalogue's mqPlayable, which SOH_DumpProfileCatalog computes with the native gate's own condition.
start = native.index("// Don't show any MQ options if both quests aren't available")
gate = native[start:native.index('// clang-format on', start)]
condition = 'OTRGlobals::Instance->CanPlayMasterQuest() && OTRGlobals::Instance->HasOriginal()'
opening = 'if (!(%s)) {' % condition
assert opening in gate, 'settings.cpp: the native MQ gate changed'
native_rows = body(gate, gate.index(opening) + len(opening) - 1), body(gate, gate.index('} else {') + 7)
found = re.search(r'void RefreshMasterQuestAvailability\(bool mqPlayable\)\s*\{', policy)
assert found, 'the profile policy lost its copy of the MQ gate'
copy = body(policy, found.end() - 1)
policy_rows = copy[copy.index('if (!mqPlayable) {') + len('if (!mqPlayable) {'):copy.index('} else {')], \
    body(copy, copy.index('} else {') + 7)
assert all(normalize(a) == normalize(b) for a, b in zip(native_rows, policy_rows)), \
    'the profile policy disables or enables the MQ rows differently from settings.cpp'
assert native_rows[0].count('.Disable(') == 15 and native_rows[1].count('.Enable()') == 15
assert re.search(r'RefreshMasterQuestAvailability\(mqPlayable != catalog\.end\(\) && mqPlayable->is_boolean\(\) &&\s*'
                 r'mqPlayable->get<bool>\(\)\);', policy), 'the policy must read the catalogue\'s mqPlayable'
catalogue = (root / 'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
assert 'data["mqPlayable"] = %s;' % condition in catalogue, 'the catalogue\'s mqPlayable must be the native condition'
# Both say why the rows are greyed out with the same text from the same fact, the vanilla game being loaded
# (combo/mq/MqMode.h MqOptionsDisabledText), which the catalogue publishes as hasOriginal.
found = re.search(r'const char\* Settings::MasterQuestDisabledText\(\)\s*\{', native)
assert found and 'return ComboMq::MqOptionsDisabledText(OTRGlobals::Instance->HasOriginal());' in \
    body(native, found.end() - 1), 'settings.cpp must give MqOptionsDisabledText\'s reason for the MQ rows'
found = re.search(r'const char\* MasterQuestDisabledText\(\) const\s*\{', policy)
assert found and 'return ComboMq::MqOptionsDisabledText(catalogHasOriginal);' in body(policy, found.end() - 1), \
    'the profile policy must give the same reason'
assert re.search(r'catalogHasOriginal = hasOriginal != catalog\.end\(\) && hasOriginal->is_boolean\(\) && '
                 r'hasOriginal->get<bool>\(\);', policy) and \
    'data["hasOriginal"] = OTRGlobals::Instance->HasOriginal();' in catalogue, \
    'the profile policy must read the catalogue\'s hasOriginal'
print('PASS: %d option callbacks and helpers and the Master Quest gate match settings.cpp' % len(callbacks[0]))
