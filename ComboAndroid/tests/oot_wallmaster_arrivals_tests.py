"""M1-14 arrival identity only: real native enums, registry/candidate coverage and source reproduction.

No engine, archive, device, MQ actor evidence or runtime capability is exercised or claimed.
"""
from pathlib import Path
import argparse
import copy
import json
import os
import subprocess
import sys
import tempfile

import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_oot_wallmaster_arrivals as tool
import export_registry as registry

# Reviewed indices and regions, independent of exporter output and native table parsing.
EXPECTED = [
    ('OOT_BOTTOM_OF_THE_WELL', 0x098, 'RR_BOTW_ENTRYWAY'),
    ('OOT_DEKU_TREE', 0x000, 'RR_DEKU_TREE_ENTRYWAY'),
    ('OOT_DODONGO_CAVERN', 0x004, 'RR_DODONGOS_CAVERN_ENTRYWAY'),
    ('OOT_GERUDO_TRAINING_GROUNDS', 0x008, 'RR_GERUDO_TRAINING_GROUND_ENTRYWAY'),
    ('OOT_ICE_CAVERN', 0x088, 'RR_ICE_CAVERN_ENTRYWAY'),
    ('OOT_JABU_JABU', 0x028, 'RR_JABU_JABUS_BELLY_ENTRYWAY'),
    ('OOT_TEMPLE_FIRE', 0x165, 'RR_FIRE_TEMPLE_ENTRYWAY'),
    ('OOT_TEMPLE_FOREST', 0x169, 'RR_FOREST_TEMPLE_ENTRYWAY'),
    ('OOT_TEMPLE_SHADOW', 0x037, 'RR_SHADOW_TEMPLE_ENTRYWAY'),
    ('OOT_TEMPLE_SPIRIT', 0x082, 'RR_SPIRIT_TEMPLE_ENTRYWAY'),
    ('OOT_TEMPLE_WATER', 0x010, 'RR_WATER_TEMPLE_ENTRYWAY'),
]
raw = (source/tool.JSON).read_bytes()
fixture = json.loads(raw)
assert fixture['upstreamCommit'] == registry.PIN
assert [(r['name'], r['logicalId'], r['arrivalRegion']) for r in fixture['rows']] == EXPECTED
assert sum(r['type'] == 'dungeon' for r in fixture['rows']) == 8
assert sum(r['type'] == 'dungeon-minor' for r in fixture['rows']) == 3
for path, data in tool.outputs(fixture).items():
    assert b'\r' not in data and (source/path).read_bytes() == data, path
    attr = subprocess.check_output(['git', '-C', str(source), '-c', 'core.autocrlf=true', 'check-attr',
                                    'eol', '--', path], text=True, timeout=30)
    assert attr.strip().endswith(': eol: lf'), attr

# The pure Wallmaster own-game contract admits these 11 arrivals, not the reverse-side pool members.
m1 = json.loads((source/'tools/ootmm/m1-contract.json').read_text(encoding='utf-8'))
combinations = [c for c in m1['combinations'] if c['settings']['erWallmasters'] == 'ownGame' and
                c['settings']['erOneWays'] == 'none']
assert len(combinations) == 1
pool = next(p for p in combinations[0]['pools'] if p['name'] == 'WALLMASTERS')
candidates = [c['destinations'] for c in pool['candidates'] if c['sources'][0].startswith('OOT_')]
assert candidates == [[r[0] for r in EXPECTED]]

# Compile generated native entries with the REAL enum definitions and entrance table, not copied enum values.
cpp = work/'oot_wallmaster_arrivals.cpp'
cpp.write_text(r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <set>
#include <string_view>
#include "rando/entrances/OotNativeArrival.h"
#include "rando/entrances/OotWallmasterArrivalNames.h"
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define RANDO_ENUM_END(name) };
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerRegion.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
enum NativeEntrance {
#define DEFINE_ENTRANCE(name, ...) name,
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
struct Arrival { std::string_view name; uint32_t logical; int index; RandomizerRegion region; };
const std::array<Arrival, 11> arrivals = {{
#include "soh/Enhancements/randomizer/combo_oot_wallmaster_arrivals.inc"
}};
const std::array<Arrival, 11> expected = {{
''' + '\n'.join(f'{{ "{name}", 0x{index:x}u, 0x{index:x}, {region} }},' for name, index, region in EXPECTED) + r'''
}};
int main() {
    using namespace ComboRando::Entrances;
    assert(kOotWallmasterArrivalRegistry == kRegistrySourceSha256);
    std::set<uint32_t> ids;
    std::set<int> regions;
    for (size_t i = 0; i < arrivals.size(); ++i) {
        const auto& a = arrivals[i];
        assert(a.name == expected[i].name && a.logical == expected[i].logical);
        assert(a.index == expected[i].index && a.region == expected[i].region);
        assert(a.name == kOotWallmasterArrivalRows[i]);
        const auto* record = Find(a.name);
        assert(record && record->game == Game::Oot && record->id == a.logical);
        assert(record->type == Type::Dungeon || record->type == Type::DungeonMinor);
        const auto native = ResolveOotNativeArrival(a.logical);
        assert(native && native->entrance == static_cast<uint32_t>(a.index));
        assert(!native->grotto && !native->returning);
        assert(ids.insert(a.logical).second && regions.insert(a.region).second);
    }
}
''', encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', source/'soh', source/'soh/include'],
                    msvc=['/Zc:preprocessor'], werror=True)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)

records = json.loads((source/'tools/ootmm/registry-source.json').read_text())['entrances']
inputs = [records, tool.native.entrance_indices(), tool.native.shuffle_table(), tool.entrance_scenes(),
          tool.region_definitions(), tool.native.arrival_aliases()]
assert tool.bind(*inputs) == [{**r, 'evidence': r['evidence'][:2]} for r in fixture['rows']]


def deku_table(data):
    return next(r for r in data[2] if r['entrance'] == 'ENTR_DEKU_TREE_ENTRANCE')


mutations = [
    lambda d: d[0].remove(next(r for r in d[0] if r['name'] == 'OOT_DEKU_TREE')),
    lambda d: d[2].append(copy.deepcopy(deku_table(d))),
    lambda d: deku_table(d).update(type='Boss'),
    lambda d: d[1].update(ENTR_DEKU_TREE_ENTRANCE=0xffff),
    lambda d: deku_table(d).update(reverse='ENTR_WATER_TEMPLE_ENTRANCE'),
    lambda d: d[4].pop('RR_DEKU_TREE_ENTRYWAY'),
    lambda d: d[3]['ENTR_DEKU_TREE_ENTRANCE'].update(scene='SCENE_WATER_TEMPLE'),
    lambda d: d[3]['ENTR_DEKU_TREE_ENTRANCE'].update(index=1),
    lambda d: d[5].update({0: 0x123}),
    lambda d: (deku_table(d).update(target='RR_WATER_TEMPLE_ENTRYWAY'),
               d[3]['ENTR_DEKU_TREE_ENTRANCE'].update(scene='SCENE_WATER_TEMPLE')),
]
for i, mutate in enumerate(mutations):
    altered = copy.deepcopy(inputs)
    mutate(altered)
    try:
        tool.bind(*altered)
    except ValueError:
        pass
    else:
        raise AssertionError(f'Arrival identity mutation {i} was accepted')
print('Real native enums/table and registry: 11 unique arrival identities; 10 invalid bindings rejected.')

reference = os.environ.get('COMBO_OOTMM_REFERENCE')
reference = Path(reference) if reference else next((p/'references/ootmm-stable-32.3' for p in source.parents
                                                  if (p/'references/ootmm-stable-32.3/.git').exists()), None)
if reference is None:
    print('SKIPPED: exact local OoTMM v32.3 checkout needed for source reproduction')
    sys.exit(0)


def run(output, check=False, ref=reference):
    return subprocess.run([sys.executable, '-B', str(source/'tools/ootmm/export_oot_wallmaster_arrivals.py'),
                           '--reference', str(ref), '--output-root', str(output), *(['--check'] if check else [])],
                          capture_output=True, text=True, timeout=60)


result = run(source, check=True)
assert result.returncode == 0, result.stdout + result.stderr
with tempfile.TemporaryDirectory(prefix='oot-arrivals-', dir=work) as scratch:
    scratch = Path(scratch)
    result = run(scratch)
    assert result.returncode == 0, result.stdout + result.stderr
    for path in tool.outputs(fixture):
        assert (scratch/path).read_bytes() == (source/path).read_bytes()
        damaged = (scratch/path).read_bytes() + b'\n'
        (scratch/path).write_bytes(damaged)
        assert run(scratch, check=True).returncode != 0
        assert (scratch/path).read_bytes() == damaged
        (scratch/path).write_bytes((source/path).read_bytes())
    before = {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}
    bad = run(scratch, ref=source)
    assert bad.returncode != 0 and 'Expected exact OoTMM v32.3 revision' in bad.stderr
    assert before == {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}
print('Exact-pin arrival export reproduces; all stale files fail read-only and wrong HEAD writes nothing.')
