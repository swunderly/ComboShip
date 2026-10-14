"""OoTMM's own entrance pool code, run with the pinned Node, agrees with the M1 contract fixture (C19).

tools/ootmm/compare_m1_pinned_node.py runs tools/ootmm/run_pinned_entrance_pools.mts on the exact OoTMM v32.3
checkout and compares, for every combination in tools/ootmm/m1-contract.json, the active pools, the candidates
of every source, the can_reset_time destinations, the water-void dead ends, er_enabled and isEntranceShuffle,
plus the OoT Wallmaster sources of every vanilla/MQ choice. The comparer must then name each change in an
altered fixture and in altered facts, report saved facts made from other contract bytes (an altered sampleLayout
is invisible otherwise), and refuse a Node that is not the pinned one. The facts must list the runner's three
in-memory changes to OoTMM code (helpers.ts: node:fs shim and __dirname prefix; entrances.ts: export line).

Reports SKIPPED when the pinned Node (COMBO_PINNED_NODE, or tooling/node/node-v24.21.0-* above this checkout)
or the OoTMM checkout with its node_modules (COMBO_OOTMM_REFERENCE, or references/ootmm-stable-32.3) is absent.
"""
from pathlib import Path
import argparse
import copy
import hashlib
import json
import os
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
COMPARER = source/'tools/ootmm/compare_m1_pinned_node.py'
FIXTURE = source/'tools/ootmm/m1-contract.json'
NODE_VERSION = 'v24.21.0'


def find_node():
    configured = os.environ.get('COMBO_PINNED_NODE')
    if configured:
        return Path(configured)
    names = ['node.exe', 'bin/node']
    for parent in source.parents:
        for folder in sorted((parent/'tooling/node').glob(f'node-{NODE_VERSION}-*')):
            for name in names:
                if (folder/name).is_file():
                    return folder/name
    return None


def find_reference():
    configured = os.environ.get('COMBO_OOTMM_REFERENCE')
    if configured:
        return Path(configured)
    return next((parent/'references/ootmm-stable-32.3' for parent in source.parents
                 if (parent/'references/ootmm-stable-32.3/.git').exists()), None)


node, reference = find_node(), find_reference()
if node is None or not node.is_file():
    print(f'SKIPPED: the pinned Node {NODE_VERSION} is not installed (set COMBO_PINNED_NODE, or unpack it under '
          f'tooling/node/ above this checkout)')
    sys.exit(0)
if reference is None or not (reference/'.git').exists():
    print('SKIPPED: set COMBO_OOTMM_REFERENCE to the exact local OoTMM v32.3 checkout to run its pool code')
    sys.exit(0)
missing = [p for p in ('lodash-es', 'yaml', 'csv') if not (reference/'node_modules'/p).exists()]
if missing:
    print(f'SKIPPED: the OoTMM checkout has no node_modules for {", ".join(missing)}; nothing is installed here')
    sys.exit(0)


def compare(*extra, timeout=120):
    command = [sys.executable, str(COMPARER), *map(str, extra)]
    return subprocess.run(command, capture_output=True, text=True, encoding='utf-8', timeout=timeout,
                          env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))


# A Node that is not the pinned one is refused before anything runs.
refused = compare('--reference', reference, '--node', sys.executable, '--facts', work/'refused.json')
assert refused.returncode != 0 and f'Expected the pinned Node {NODE_VERSION}' in refused.stderr, refused.stderr
assert not (work/'refused.json').exists()

# The real run: every compared fact agrees.
facts_path, report_path = work/'facts.json', work/'report.json'
for stale in (facts_path, report_path):
    stale.unlink(missing_ok=True)
result = compare('--reference', reference, '--node', node, '--facts', facts_path, '--out', report_path, timeout=840)
assert result.returncode == 0, result.stdout + result.stderr
report = json.loads(report_path.read_text(encoding='utf-8'))
facts = json.loads(facts_path.read_text(encoding='utf-8'))
contract = json.loads(FIXTURE.read_text(encoding='utf-8'))
assert report['result'] == 'agree' and report['divergences'] == [], report['divergences'][:5]
assert report['node'] == facts['node'] == NODE_VERSION
# The facts name the exact contract bytes they were made from.
fixture_sha256 = hashlib.sha256(FIXTURE.read_bytes()).hexdigest()
assert facts['contract']['sha256'] == report['contract']['sha256'] == fixture_sha256
assert report['checked']['contract'] == 1
# The only changes to OoTMM code: two to the core build's helpers.ts, one line appended to entrances.ts.
helpers = 'packages/core/build/helpers.ts'
entrances = 'packages/logic/src/solver/entrances.ts'
changes = report['inMemoryChanges']
assert changes == facts['inMemoryChanges']
assert sorted(changes) == [f"{helpers}: import from 'node:fs' resolved to the in-memory shim",
                           f'{helpers}: prefixed to line 1',
                           f'{entrances}: appended'], sorted(changes)
assert changes[f'{entrances}: appended'] == '\nexport { WorldShuffler, LogicPassEntrances };\n'
prefix = changes[f'{helpers}: prefixed to line 1']
assert prefix.startswith('const __dirname = ') and prefix.endswith('; ') and '\n' not in prefix, prefix
assert Path(json.loads(prefix[len('const __dirname = '):-2])) == (reference/'packages/core/build').resolve()
shim = changes[f"{helpers}: import from 'node:fs' resolved to the in-memory shim"]
assert {line.split('(')[0] for line in shim.splitlines() if line.startswith('export function')} == {
    f'export function {name}' for name in ('readFileSync', 'globSync', 'mkdirSync', 'writeFileSync')}, shim
assert len(facts['combinations']) == len(contract['combinations']) == 66
assert len(facts['mqSources']['table']) == 64
for kind in ('settings', 'erEnabled', 'isEntranceShuffle', 'pools', 'opts', 'types', 'members', 'candidates',
             'resetGated', 'deadEnds', 'sampleLayout', 'regions', 'mq', 'capacity', 'rows', 'inputs', 'placement',
             'contract'):
    assert report['checked'].get(kind, 0) > 0, kind
assert report['checked']['candidates'] >= 700 and report['checked']['erEnabled'] == 66
# One real seeded placement per combination succeeds; in the four MQ Spirit + MQ BotW contexts (vanilla Shadow and
# Ganon) every Wallmaster combination with too few OoT destinations fails, as the fixture's capacity flags say.
assert all(c['placement']['placed'] for c in facts['combinations'])
failed = {'+'.join(sorted(r['mq'])) for r in facts['mqSources']['table']
          if any(not a['placed'] for a in r['placements'].values())}
assert failed == {'+'.join(sorted(s['mq'])) for s in contract['mqSources']['capacityShortfalls']} and len(failed) == 4
# Every OoTMM source and data file ran from its pinned blob; entrances.ts is the one the fixture hashed.
assert facts['servedBlobs'][entrances] == contract['inputs'][entrances]
assert 'data/defs/entrances.yml' in facts['servedBlobs'] and 'packages/core/build/index.ts' in facts['servedBlobs']
print(f'Pinned OoTMM agrees with the M1 fixture on {sum(report["checked"].values())} facts '
      f'({facts["seconds"]["wall"]:.0f} s with {facts["jobs"]} Node workers).')


def divergences(fixture=None, altered_facts=None):
    paths = []
    if fixture is not None:
        paths += ['--contract', work/'altered-contract.json']
        (work/'altered-contract.json').write_text(json.dumps(fixture), encoding='utf-8')
    target = facts_path
    if altered_facts is not None:
        target = work/'altered-facts.json'
        target.write_text(json.dumps(altered_facts), encoding='utf-8')
    out = work/'altered-report.json'
    outcome = compare('--facts', target, '--out', out, *paths)
    assert outcome.returncode == 1, outcome.stdout + outcome.stderr
    found = json.loads(out.read_text(encoding='utf-8'))
    assert found['result'] == 'diverge'
    return {(d['fact'], d['where']) for d in found['divergences']}, found['divergences']


# An altered fixture: every change is named, with the pinned value and a pinned file:line.
alone ='erWallmasters=ownGame;erOneWays=none'
full = 'erWallmasters=ownGame;erOneWays=ownGame;erOneWaysMajor;erOneWaysIkana;erOneWaysOwls;erOneWaysWaterVoids'
altered = copy.deepcopy(contract)
ids = {c['id']: c for c in altered['combinations']}
mm_group = next(g for g in ids[alone]['pools'][0]['candidates'] if g['sources'][0].startswith('MM_'))
mm_group['destinations'] = mm_group['destinations'][:-1]
ids[full]['erEnabled'] = False
ids[full]['deadEnds'] = ids[full]['deadEnds'][1:]
ids[full]['resetGated'] = ids[full]['resetGated'] + ['MM_TEMPLE_WOODFALL']
altered['mqSources']['table'][0]['ootSources'] = altered['mqSources']['table'][0]['ootSources'][1:]
altered['mqSources']['capacityShortfalls'][0]['combinations'].pop()
altered['rows']['MM_TEMPLE_WOODFALL']['noSot'] = False
named, details = divergences(fixture=altered)
shortfall = '+'.join(sorted(altered['mqSources']['capacityShortfalls'][0]['mq']))
made_from = ('contract', 'facts made from this contract (sha256)')
assert {(f, w) for f, w in named if f != 'candidates'} == {
    made_from,  # the saved facts were made from the committed fixture, not this one
    ('erEnabled', full), ('deadEnds', full), ('resetGated', full), ('mq', 'OoT Wallmaster sources, MQ [none]'),
    ('capacity', f'MQ [{shortfall}]'),
    ('placement', f'MQ [{shortfall}] attempts placed'),  # the dropped combination's failed attempt is unexpected
    ('rows', 'MM_TEMPLE_WOODFALL')}, named
assert {w for f, w in named if f == 'candidates'} == {f'{alone} WALLMASTERS {s}' for s in mm_group['sources']}
assert all(d['source'].split(' ')[0].count(':') == 1 for d in details)  # every divergence cites file:line
erm = next(d for d in details if d['fact'] == 'erEnabled')
assert erm['fixture'] is False and erm['pinned'] is True and erm['source'].startswith('data/macros/macros_mm.yml:305')

# Saved facts against a fixture whose sampleLayout changed: the layout facts were computed in Node from the
# contract the run read, so only the contract hash can tell. An incomplete layout (last pair dropped) and a pair
# moved to another destination inside its candidates must both fail instead of agreeing.
anywhere = full + ';erOneWaysAnywhere'
layouts = copy.deepcopy(contract)
ids = {c['id']: c for c in layouts['combinations']}
ids[anywhere]['sampleLayout'].pop()
allowed = {s: g['destinations'] for p in ids[alone]['pools'] for g in p['candidates'] for s in g['sources']}
moved = next(i for i, (s, d) in enumerate(ids[alone]['sampleLayout']) if len(allowed[s]) > 1)
src, dst = ids[alone]['sampleLayout'][moved]
ids[alone]['sampleLayout'][moved] = [src, next(d for d in allowed[src] if d != dst)]
named, details = divergences(fixture=layouts)
assert named == {made_from}, named
assert details[0]['fixture'] == hashlib.sha256(json.dumps(layouts).encode('utf-8')).hexdigest()
assert details[0]['pinned'] == fixture_sha256 and 'rerun with --node' in details[0]['source']
# Facts from before the hash was recorded carry no contract and are not taken as evidence either.
unbound = {k: v for k, v in facts.items() if k != 'contract'}
named, details = divergences(altered_facts=unbound)
assert named == {made_from} and details[0]['pinned'] is None, details

# Altered facts (as if OoTMM had answered differently) are named the same way.
changed = copy.deepcopy(facts)
pinned = {c['id']: c for c in changed['combinations']}
pinned[alone]['sampleLayout']['regions']['extraHintRegions=true']['sampleLayoutUnchanged'] = False
pinned[full]['resetGateDependsOnSource'] = ['MM_TEMPLE_WOODFALL']
pinned[full]['isEntranceShuffle'] = False
changed['servedBlobs'][entrances] = '0' * 64
changed['mqSources']['otherRowsDependOnMq'] = {'Spirit': [alone]}
pinned[alone]['placement']['pairs'][0][1] = 'NOT_A_DESTINATION'
short = next(r for r in changed['mqSources']['table'] if r['placements'].get(alone, {}).get('placed') is False)
short['placements'][alone]['placed'] = True
named, _ = divergences(altered_facts=changed)
assert named == {('regions', f'{alone} extraHintRegions=true'), ('resetGated', f'{full} (same for every source)'),
                 ('isEntranceShuffle', full), ('inputs', entrances),
                 ('mq', 'rows other than OoT Wallmaster sources that change with the MQ choice'),
                 ('placement', f'{alone} pairs outside the fixture candidates'),
                 ('placement', f'MQ [{"+".join(sorted(short["mq"]))}] attempts placed')}, named
print('The comparer names every altered fixture value and every altered pinned fact, rejects facts made from '
      'another contract, and refuses another Node.')
