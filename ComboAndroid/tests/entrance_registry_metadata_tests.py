"""Compare the compiled registry and native grotto context with pinned source manifests."""
from pathlib import Path
import argparse
import json
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
registry = json.loads((source/'tools/ootmm/registry-source.json').read_text(encoding='utf-8'))
context = json.loads((source/'tools/ootmm/grotto-context-source.json').read_text(encoding='utf-8'))
pin = '413573d22d1d53e8a81931a70927f625b73488b9'
assert registry['upstreamCommit'] == context['upstreamCommit'] == pin
assert registry['sourceSha256'] == 'b95aee4e8dfb5b9015aca60ad98f11aeb12e86fec42528e9dddd4043119f709d'
assert context['runtimeSourceSha256'] == '58bcbd23ba45aa533e5ec28e73dea0eb4a09b362b8161180610fdc41c93e616b'
assert len(registry['entrances']) == 608
cpp = work/'registry_metadata.cpp'
cpp.write_text(r'''
#include "rando/entrances/GrottoMetadata.h"
#include "rando/entrances/LegacyEnvelope.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
int main() {
    assert(kUpstreamCommit != kLegacyUpstreamCommit);
    assert(kRegistrySourceSha256 != kLegacyRegistrySourceSha256);
    Json result = {{"pin", kUpstreamCommit}, {"registryHash", kRegistrySourceSha256},
                   {"runtimeHash", kGrottoRuntimeSourceSha256}, {"records", Json::array()},
                   {"codes", kMmGenericGrottoData}, {"exits", Json::array()}};
    for (const auto& r : kRegistry) {
        assert(Find(r.name) == &r && Find(r.game, r.id) == &r);
        if (!r.reverse.empty()) assert(Reverse(*Reverse(r)) == &r);
        result["records"].push_back({r.name, r.game == Game::Mm ? "mm" : "oot", r.id,
            static_cast<int>(r.type), r.reverse, r.from, r.to, r.fromMap, r.toMap,
            r.fromSubmap, r.toSubmap, r.noSongOfTime, r.sourceLine});
    }
    for (const auto& e : kMmGrottoExits)
        result["exits"].push_back({e.entranceName, e.nativeEntrance, e.room, e.x, e.y, e.z});
    const auto& e = kMmGrottoExitMountainWinter;
    result["winter"] = {e.entranceName, e.nativeEntrance, e.room, e.x, e.y, e.z};
    assert(!Find("MM_BOSS_IGOS") && !Find("MM_BOSS_MAJORA") && !Find("OOT_BOSS_GANONDORF"));
    std::cout << result.dump();
}
''', encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include], werror=True)
actual = json.loads(subprocess.check_output([str(exe)], timeout=hostcxx.RUN_TIMEOUT))
assert actual['pin'] == pin and actual['registryHash'] == registry['sourceSha256']
assert actual['runtimeHash'] == context['runtimeSourceSha256']
types = sorted({record['type'] for record in registry['entrances']})
expected = [[r['name'], r['game'], r['id'], types.index(r['type']), r['reverse'] or '',
             *[r[key] for key in ('from', 'to', 'fromMap', 'toMap', 'fromSubmap', 'toSubmap')],
             'no-sot' in r['flags'], r['sourceLine']] for r in registry['entrances']]
assert actual['records'] == expected
by_name = {r['name']: r for r in registry['entrances']}
runtime_row = lambda row: [row[0], by_name[row[0]]['id'], *map(int, row[1:])]
assert actual['codes'] == context['genericCodes']
assert actual['exits'] == [runtime_row(row) for row in context['exitEntries']]
assert actual['winter'] == runtime_row(context['winterEntry'])
print('Compiled 608-row registry and all MM grotto context match the exact v32.3 source manifests.')
