"""Compare the hidden C++ model with pinned OoTMM text and ordered conditions."""
from pathlib import Path
from types import SimpleNamespace
import argparse
import importlib.util
import json
import random
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--reference', type=Path)
args = parser.parse_args()
root, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
fixture = json.loads((root/'ComboAndroid/tests/fixtures/ootmm-32.3-entrance-settings.json').read_text())
rows = fixture['settings']
included = [r for r in rows if r['inScope']]
assert len(rows) == 44 and len(included) == 40

# The fixture is checked against the actual pinned Git blob whenever present.
# Independently assert generated output is reproducible from the checked-in fixture.
spec = importlib.util.spec_from_file_location('export_settings', root/'tools/ootmm/export_entrance_settings.py')
exporter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(exporter)
expected = exporter.generate(rows, fixture['sourceSha256'])
assert (root/'combo/rando/entrances/OotmmSettingsData.inc').read_text() == expected
references = [args.reference] if args.reference else [
    root.parent/'references/ootmm-stable-32.3', root.parent.parent/'references/ootmm-stable-32.3']
reference = next((p for p in references if p and (p/'.git').exists()), None)
if reference:
    exporter.export(reference, root, True)
else:
    print('Reference checkout absent; using the pinned checked-in metadata fixture.')


def condition(text, state):
    if not text or text in ('hasOoT', 'hasMM', 'hasOoTMM'):
        return True
    expression = re.sub(r'^\([xs]: any\) => ', '', text)
    expression = re.sub(r'has(?:OoTMM|OoT|MM)\([xs]\)', 'True', expression)
    expression = expression.replace('!==', '!=').replace('===', '==')
    expression = expression.replace('&&', ' and ').replace('||', ' or ')
    expression = re.sub(r'!(?!=)', 'not ', expression)
    # Only the checked-in, pinned reference expressions are evaluated. This oracle
    # uses string-valued TypeScript settings, independently of the C++ enum mapping.
    return bool(eval(expression, {'__builtins__': {}}, {'x': SimpleNamespace(**state), 's': SimpleNamespace(**state)}))


def normalize(case):
    state = {}
    for row in rows:
        value = case.get(row['key'], 0) if row['inScope'] else 0
        maximum = len(row['values']) if row['values'] else 2
        value = value if 0 <= value < maximum else 0
        state[row['key']] = row['values'][value]['value'] if row['values'] else bool(value)
    state['alterLostWoodsExits'] = False
    for _ in range(45):
        before = dict(state)
        for row in rows:
            if not condition(row['condition'], state):
                state[row['key']] = row['default']
        if before == state:
            return {r['key']: next((i for i, v in enumerate(r['values']) if v['value'] == state[r['key']]), 0)
                    if r['values'] else int(state[r['key']]) for r in included}
    raise AssertionError('Reference conditions did not converge')


cases = [{}, {'erRegions': 1, 'erOverworld': 2, 'erRegionsExtra': 1},
         {'erDungeons': 1, 'erPirateFortress': 1, 'erPiratesWorld': 1, 'erOverworld': 1},
         {'erIndoors': 2, 'erIndoorsGameLinks': 1, 'erMixedIndoors': 1, 'erMixed': 1},
         {'erIndoors': 2, 'erIndoorsGameLinks': 1, 'erMixedIndoors': 1, 'erMixed': 2},
         {'erSpawns': 1}, {'erSpawns': 2}, {'erSpawns': 3}]
for row in included:
    for value in (-1, 0, 1, 2, 3, 255, 256):
        cases.append({row['key']: value})
rng = random.Random(323)
for _ in range(1000):
    cases.append({r['key']: rng.randrange(len(r['values']) if r['values'] else 2) for r in included})

cpp = r'''
#include "rando/entrances/OotmmSettings.h"
#include "rando/entrances/MmOrdinaryGenerationSettings.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
static_assert(static_cast<int>(Mode::None) == 0 && static_cast<int>(Mode::OwnGame) == 1 && static_cast<int>(Mode::Full) == 2);
static_assert(static_cast<int>(Spawns::None) == 0 && static_cast<int>(Spawns::Child) == 1 &&
              static_cast<int>(Spawns::Adult) == 2 && static_cast<int>(Spawns::Both) == 3);
int main(int argc, char** argv) {
    assert(argc == 3);
    Settings defaults;
    assert(!defaults.erMajorDungeons && !defaults.erIndoorsMajor && MakePools(defaults).empty());
    Json metadata = Json::array(), result = Json::array();
    const auto shipping = ActiveCapabilities();
    for (const auto& row : kEntranceSettings) {
        assert(row.get(defaults) == 0 && row.defaultValue == 0 && !row.visible(shipping));
        assert(row.supports(0, shipping) && !row.supports(-1, shipping) && !row.supports(1, shipping) &&
               !row.supports(2, shipping));
        Json choices = Json::array();
        for (const auto& choice : row.choices)
            choices.push_back({{"value", choice.value}, {"name", choice.name}, {"description", choice.description}});
        metadata.push_back({{"key", row.key}, {"cvar", row.cvar}, {"name", row.name},
                            {"description", row.description}, {"choices", choices}});
    }
    for (int injected : {-1, 0, 1, 2, 3, 255, 256}) {
        size_t calls = 0;
        const auto guarded = ReadEntranceSettings([&](const char* cvar, int defaultValue) {
            assert(std::string_view(cvar).starts_with("gCombo.Rando.Entrances."));
            assert(defaultValue == 0); ++calls; return injected;
        }, shipping);
        assert(calls == std::size(kEntranceSettings) && MakePools(guarded).empty());
        for (const auto& row : kEntranceSettings) assert(row.get(guarded) == 0);
    }
    // The private v1 helper must keep its old pools after default corrections.
    for (int mask = 0; mask < 16; ++mask) {
        auto legacy = MmOrdinaryGenerationSettings(mask & 1, mask & 2, mask & 4, mask & 8);
        Settings previous;
        previous.games = Games::Mm; previous.erMajorDungeons = previous.erIndoorsMajor = true;
        if (mask & 3) previous.erIndoors = Mode::OwnGame;
        previous.erIndoorsTelescopes = bool(mask & 2);
        if (mask & 4) previous.erOverworld = Mode::OwnGame;
        if (mask & 8) previous.erDungeons = Mode::OwnGame;
        const auto a = MakePools(legacy), b = MakePools(previous);
        assert(a.size() == b.size());
        for (size_t i = 0; i < a.size(); ++i)
            assert(a[i].name == b[i].name && a[i].src == b[i].src && a[i].dst == b[i].dst &&
                   a[i].ownGame == b[i].ownGame && a[i].alias == b[i].alias);
    }
    Json cases; std::ifstream(argv[1]) >> cases;
    for (const auto& input : cases) {
        Settings s;
        s.games = Games::Mm; s.erWarps = Warps::Full;
        s.erOneWaysSongs = s.erOneWaysStatues = s.erOneWaysWoods = true;
        for (const auto& row : kEntranceSettings) row.set(s, input.value(row.key, 0));
        ValidateEntranceSettings(s);
        assert(s.games == Games::Both && s.erWarps == Warps::None &&
               !s.erOneWaysSongs && !s.erOneWaysStatues && !s.erOneWaysWoods);
        Json output = Json::object();
        for (const auto& row : kEntranceSettings) output[row.key] = row.get(s);
        ValidateEntranceSettings(s);
        for (const auto& row : kEntranceSettings) assert(output.at(row.key) == row.get(s));
        result.push_back(output);
    }
    std::ofstream(argv[2]) << Json{{"metadata", metadata}, {"cases", result}}.dump();
    std::cout << "Hidden settings capabilities/defaults and legacy pool construction passed.\n";
}
'''
source = work/'entrance_settings.cpp'
source.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(args.compiler, source, includes=[root/'combo', args.json_include], werror=True)
input_path, output_path = work/'cases.json', work/'results.json'
input_path.write_text(json.dumps(cases), encoding='utf-8')
subprocess.run([str(exe), str(input_path), str(output_path)], check=True, timeout=hostcxx.RUN_TIMEOUT)
actual = json.loads(output_path.read_text())
for row, compiled in zip(included, actual['metadata'], strict=True):
    for key in ('key', 'name', 'description'):
        assert compiled[key] == row[key].replace('<br>', '\n'), (row['key'], key)
    assert compiled['cvar'] == 'gCombo.Rando.Entrances.' + row['key']
    expected_choices = [{k: value[k].replace('<br>', '\n') for k in ('value', 'name', 'description')}
                        for value in row['values']]
    if not expected_choices:
        expected_choices = [{'value': v, 'name': '', 'description': ''} for v in ('false', 'true')]
    assert compiled['choices'] == expected_choices, row['key']
for case, compiled in zip(cases, actual['cases'], strict=True):
    assert compiled == normalize(case), (case, compiled, normalize(case))
assert actual['cases'][1]['erRegions'] == 0 and actual['cases'][1]['erOverworld'] == 2
assert actual['cases'][2]['erPirateFortress'] == 0
assert actual['cases'][3]['erIndoorsGameLinks'] == 0
assert actual['cases'][4]['erIndoorsGameLinks'] == 1
print(f'All40 exact setting names/descriptions/choices match; {len(cases)} reference normalization cases passed.')
