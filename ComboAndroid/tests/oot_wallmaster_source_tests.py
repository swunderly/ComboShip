"""Execute the private source helper against all rooms in the pinned actor's switch."""
from pathlib import Path
import argparse
import json
import re
import subprocess
import sys
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_oot_wallmaster_sources as exporter

fixture = json.loads((source/exporter.OUTPUT).read_text(encoding='utf-8'))
assert fixture['upstreamCommit'] == '413573d22d1d53e8a81931a70927f625b73488b9'
assert fixture['source'] == exporter.SOURCE
assert fixture['sourceSha256'] == '2d9e7729ea8fc152c3932d368cd1dea9911d103bf71eec35499028448186824d'
function = fixture['functionSource']
assert exporter.SourceParser(function).parse() == fixture['rules']
assert exporter.SourceParser(function.replace('\n', '\r\n')).parse() == fixture['rules']
expected_names = [
    'FOREST_CORRIDOR_WEST', 'FOREST_CORRIDOR_EAST', 'SHADOW', 'SPIRIT_CHILD_RUPEES',
    'SPIRIT_ADULT_CLIMB', 'SPIRIT_CHILD_SUN', 'SPIRIT_STATUE', 'BOTW_MAIN',
    'BOTW_BASEMENT', 'BOTW_PIT', 'GTG', 'GANON_SPIRIT', 'GANON_LIGHT',
]
expected = {f'OOT_WALLMASTER_{suffix}': 0xf10 + index for index, suffix in enumerate(expected_names)}
assert {r['name']: r['id'] for r in fixture['sources']} == expected
assert all(r['game'] == 'oot' and r['type'] == 'wallmaster' and r['reverse'] is None
           and r['to'] == 'OOT VOID' for r in fixture['sources'])

# These are changed semantics or unsupported syntax, not merely substring-presence checks.
invalid = [
    function.replace('this->room', 'play->room'),
    function.replace('play->sceneId', 'this->sceneId'),
    function.replace('room == 0x14', 'room != 0x14'),
    function.replace('room == 0x14', 'room == 0x114'),
    function.replace('room == 0x14', 'room == 024'),
    function.replace('room = this->room;', 'room = this->room; room = 0;'),
    function.replace('switch (room)', 'switch (play->room)', 1),
    function.replace('case 0x05:', 'case 0x02:'),
    function.replace('case 0x05:', 'case 0x100:'),
    function.replace('case SCE_OOT_TEMPLE_SHADOW:', 'case SCE_OOT_TEMPLE_FOREST:'),
    function.replace('case SCE_OOT_TEMPLE_SHADOW:', 'case SCE_OOT_UNKNOWN:'),
    function.replace('case SCE_OOT_TEMPLE_SHADOW:\n        return ENTR_OOT_WALLMASTER_SHADOW;', ''),
    function.replace('return ENTR_OOT_WALLMASTER_SHADOW;', 'return ENTR_MM_WALLMASTER_DAMPE;'),
    function.replace('return ENTR_OOT_WALLMASTER_SHADOW;', 'if (room) return ENTR_OOT_WALLMASTER_SHADOW;'),
    function.replace('default:\n        UNREACHABLE();', 'default:\n        return ENTR_OOT_WALLMASTER_SHADOW;'),
    function[:-1] + 'return ENTR_OOT_WALLMASTER_SHADOW; }',
    function + ' return ENTR_OOT_WALLMASTER_SHADOW;',
    function[:-1],
]
for changed in invalid:
    assert changed != function
    try:
        exporter.SourceParser(changed).parse()
    except ValueError:
        pass
    else:
        raise AssertionError('Changed/unsupported source accepted: ' + changed)

records = json.loads((source/'tools/ootmm/registry-source.json').read_text(encoding='utf-8'))['entrances']
for changed in [function + '\n' + function, function[:-1],
                function.replace('ENTR_OOT_WALLMASTER_SHADOW', 'ENTR_OOT_WALLMASTER_UNKNOWN')]:
    try:
        exporter.parse_source(changed.encode(), records)
    except ValueError:
        pass
    else:
        raise AssertionError('Incomplete/unknown source definition accepted')
for field, wrong in [('game', 'mm'), ('type', 'dungeon'), ('reverse', 'OOT_TEMPLE_FOREST'), ('to', 'OOT SPAWN')]:
    damaged = [dict(r, **{field: wrong}) if r['name'] in expected else r for r in records]
    try:
        exporter.parse_source(function.encode(), damaged)
    except ValueError:
        pass
    else:
        raise AssertionError('Wrong source metadata accepted: ' + field)

# Compile the reference function independently as well as the production helper.
# Only the C identifier `this` is renamed for C++; UNREACHABLE becomes a null sentinel.
reference_function = re.sub(r'\bthis\b', 'self', function)
constants = [f'constexpr int ENTR_{name} = {value};' for name, value in expected.items()]
constants += [f'constexpr int {r["scene"]} = {index};' for index, r in enumerate(fixture['rules'])]
contexts = ['OotWallmasterScene::' + r['context'] for r in fixture['rules']] + ['OotWallmasterScene::Unknown']
cpp = work/'oot_wallmaster_source.cpp'
cpp.write_text(r'''
#include "rando/entrances/OotWallmasterSource.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::Entrances;
namespace Reference {
using u8 = uint8_t;
struct Actor { u8 room; };
struct PlayState { int sceneId; };
''' + '\n'.join(constants) + '\n#define UNREACHABLE() return -1\n' + reference_function + r'''
#undef UNREACHABLE
}
int main() {
    const OotWallmasterScene scenes[] = {''' + ', '.join(contexts) + r'''};
    for (int scene = 0; scene < 7; ++scene) {
        for (int room = 0; room < 256; ++room) {
            Reference::Actor actor{static_cast<uint8_t>(room)};
            Reference::PlayState play{scene};
            const int reference = Reference::EnWallmas_GetDestEr(&actor, &play);
            const auto* actual = OotWallmasterSource(scenes[scene], actor.room);
            assert((actual == nullptr) == (reference == -1));
            if (actual) {
                assert(actual->id == static_cast<uint32_t>(reference));
                assert(actual->game == Game::Oot && actual->type == Type::Wallmaster);
                assert(actual->reverse.empty() && actual->to == "OOT VOID");
                assert(Find(Game::Oot, actual->id) == actual && Find(actual->name) == actual);
                std::cout << actual->name << '\n';
            } else {
                std::cout << "-\n";
            }
            assert(OotWallmasterSource(static_cast<OotWallmasterScene>(255), actor.room) == nullptr);
        }
    }
}
''', encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo'], werror=True)
actual = subprocess.check_output([str(exe)], text=True, timeout=hostcxx.RUN_TIMEOUT).splitlines()
expected_rows = [rule['rooms'].get(str(room), rule['default']) or '-'
                 for rule in fixture['rules'] for room in range(256)] + ['-'] * 256
assert len(actual) == 7 * 256 and actual == expected_rows
assert set(actual) - {'-'} == set(expected)
print('All 1792 scene/room cases match actual pinned function and production helper; '
      '13 identities, unknown contexts and 25 strict parser/metadata rejection cases passed.')
