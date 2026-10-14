"""Cross-check the M1 contract fixture against OoTMM's own entrance pool code, run with the pinned Node.

tools/ootmm/m1-contract.json comes from export_m1_contract.py, a Python re-implementation of the v32.3 pool
rules. run_pinned_entrance_pools.mts runs the pinned TypeScript itself for every combination in the fixture
and every vanilla/MQ choice of the Wallmaster dungeons; this compares the two and reports each disagreement
with both values and the pinned file:line of the rule involved. It never edits the fixture.

    python tools/ootmm/compare_m1_pinned_node.py --reference <ootmm-stable-32.3> --node <node.exe> \
        [--facts facts.json] [--out report.json] [--jobs N]
    python tools/ootmm/compare_m1_pinned_node.py --facts facts.json [--out report.json]   # saved facts only

Exit status 0 only when every compared fact agrees.
"""
import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from export_registry import PIN, verified_blob

TOOLS = Path(__file__).resolve().parent
CONTRACT = TOOLS/'m1-contract.json'
RUNNER = TOOLS/'run_pinned_entrance_pools.mts'
PINNED_NODE_VERSION = 'v24.21.0'
NODE_FLAGS = ['--experimental-transform-types', '--no-warnings']

E = 'packages/logic/src/solver/entrances.ts'
# The pinned rule behind each compared fact (OoTMM 413573d).
RULES = {
    'settings': 'packages/core/src/settings/util.ts:33-130 (validateSettingsStep, validateSettings)',
    'erEnabled': 'data/macros/macros_mm.yml:305 (er_enabled)',
    'isEntranceShuffle': 'packages/logic/src/helpers.ts:6-56 (isEntranceShuffle)',
    'pools': f'{E}:863-879 (makePools), {E}:567-591 (placePools drops pools without sources)',
    'opts': f'{E}:657-669 (poolWallmasters), {E}:742-784 (poolOneWaysAnywhere, poolOneWays)',
    'types': f'{E}:657-669, {E}:742-784, {E}:791-886 (pool types, poolsTypesDst)',
    'members': f'{E}:179-201 (computeAllEntrances), {E}:601-627 (entrancesForTypes, poolEntrancesForTypes)',
    'candidates': f'{E}:513 (candidate filter), {E}:139-163 (polarity), {E}:459-498 (isSelfLoop, '
                  'isAssignableConstraints)',
    'resetGated': f'{E}:203-217 (placeSingleExpr: can_reset_time unless no-sot)',
    'deadEnds': 'packages/logic/src/world/builder.ts:279-391 (loadAreas: the arrival area has no exits)',
    'sampleLayout': f'{E}:513 (candidate filter), {E}:500-565 (placePoolsRecursive: one destination per source)',
    'regions': f'{E}:349-386 (changedWorld), {E}:1044-1059 (processRegions)',
    'mq': f'packages/logic/src/world/builder.ts:302-305 (MQ area sets), {E}:179-201 (computeAllEntrances)',
    'capacity': f'{E}:500-565 (placePoolsRecursive: an alias pool still needs a distinct destination per source)',
    'placement': f'{E}:905-934 (run), {E}:500-599 (placePoolsRecursive, placePools), {E}:388-416 (isValidShuffle)',
    'rows': 'packages/core/src/data/data.ts:31-62 (ENTRANCES, from data/defs/entrances.yml)',
    'inputs': 'tools/ootmm/m1-contract.json:inputs (sha256 of the pinned Git blobs)',
}


def expand(groups):
    return {src: list(group['destinations']) for group in groups for src in group['sources']}


class Report:
    def __init__(self):
        self.divergences, self.checked = [], {}

    def check(self, kind, where, fixture, pinned, rule=None):
        self.checked[kind] = self.checked.get(kind, 0) + 1
        if fixture != pinned:
            self.divergences.append({'fact': kind, 'where': where, 'fixture': fixture, 'pinned': pinned,
                                     'source': RULES[rule or kind]})


def compare(contract, facts):
    report = Report()
    report.check('inputs', 'fixture upstreamCommit', PIN, contract['upstreamCommit'])
    report.check('inputs', 'facts upstreamCommit', contract['upstreamCommit'], facts['upstreamCommit'])
    for path, digest in sorted(facts['servedBlobs'].items()):
        if path in contract['inputs']:
            report.check('inputs', path, contract['inputs'][path], digest)
    report.check('inputs', 'entrances.ts served', True, E in facts['servedBlobs'])

    fixture_ids = sorted(c['id'] for c in contract['combinations'])
    report.check('settings', 'normalized M1 combinations (makeSettings fixed points of the 128 raw inputs)',
                 fixture_ids, facts['settingsFixedPoints'])
    for name, row in sorted(contract['rows'].items()):
        report.check('rows', name, row, facts['rows'].get(name))

    pinned_by_id = {c['id']: c for c in facts['combinations']}
    report.check('pools', 'combination list', [c['id'] for c in contract['combinations']],
                 [c['id'] for c in facts['combinations']])
    for entry in contract['combinations']:
        cid, pinned = entry['id'], pinned_by_id.get(entry['id'])
        if pinned is None:
            continue
        report.check('settings', cid, entry['settings'], pinned['normalized'])
        report.check('settings', f'{cid} (requested)', entry['settings'], pinned['requested'])
        report.check('erEnabled', cid, entry['erEnabled'], pinned['erEnabled'])
        report.check('isEntranceShuffle', cid, entry['isEntranceShuffle'], pinned['isEntranceShuffle'])
        report.check('pools', cid, [p['name'] for p in entry['pools']], [p['name'] for p in pinned['pools']])
        for pool in entry['pools']:
            other = next((p for p in pinned['pools'] if p['name'] == pool['name']), None)
            if other is None:
                continue
            where = f'{cid} {pool["name"]}'
            report.check('opts', where, {'ownGame': pool['ownGame'], 'alias': pool['alias']},
                         {'ownGame': other['ownGame'], 'alias': other['alias']})
            report.check('types', f'{where} srcTypes', pool['srcTypes'], other['srcTypes'])
            report.check('types', f'{where} dstTypes', pool['dstTypes'], other['dstTypes'])
            candidates = expand(pool['candidates'])
            report.check('members', f'{where} sources', sorted(candidates), other['src'])
            report.check('members', f'{where} destinations', pool['dst'], other['dst'])
            report.check('members', f'{where} srcCount', pool['srcCount'], other['srcCount'])
            report.check('members', f'{where} dstCount', pool['dstCount'], other['dstCount'])
            for src in sorted(set(candidates) | set(other['candidates'])):
                report.check('candidates', f'{where} {src}', candidates.get(src), other['candidates'].get(src))
        report.check('resetGated', cid, entry['resetGated'], pinned['resetGated'])
        report.check('resetGated', f'{cid} (same for every source)', [], pinned['resetGateDependsOnSource'])
        report.check('deadEnds', cid, entry['deadEnds'], pinned['deadEnds'])
        layout = pinned['sampleLayout']
        report.check('sampleLayout', f'{cid} pairs outside the pinned candidates', [], layout['pairsOutsideCandidates'])
        report.check('sampleLayout', f'{cid} sources without a pair', [], layout['sourcesWithoutPair'])
        report.check('sampleLayout', f'{cid} sources paired twice', [], layout['sourcesPairedTwice'])
        for mode, result in sorted(layout['regions'].items()):
            report.check('regions', f'{cid} {mode}', {'sampleLayoutUnchanged': True, 'allCandidatesUnchanged': True},
                         result)
        # One real OoTMM placement attempt with a fixed seed: the layout is RNG-dependent, but it must succeed
        # (the fixture has a complete layout) and stay inside the fixture's candidate sets.
        placement = pinned['placement']
        report.check('placement', f'{cid} placed', True, placement['placed'])
        if placement['placed']:
            allowed = {src: set(dsts) for pool in entry['pools'] for src, dsts in expand(pool['candidates']).items()}
            pairs = [tuple(pair) for pair in placement['pairs']]
            report.check('placement', f'{cid} pairs outside the fixture candidates', [],
                         [list(p) for p in pairs if p[1] not in allowed.get(p[0], ())])
            report.check('placement', f'{cid} fixture sources left unplaced', [],
                         sorted(set(allowed) - {src for src, _ in pairs}))
            pool_of = {src: pool['name'] for pool in entry['pools'] for g in pool['candidates'] for src in g['sources']}
            used = [(pool_of.get(src), dst) for src, dst in pairs]
            report.check('placement', f'{cid} destinations used twice in a pool', [],
                         sorted({f'{p} {d}' for p, d in used if used.count((p, d)) > 1}))
    report.check('mq', 'world areas do not depend on the M1 keys', True, facts['areaKeysInvariantAcrossCombinations'])

    mq, pinned_mq = contract['mqSources'], facts['mqSources']
    report.check('mq', 'Wallmaster dungeons', sorted(mq['dungeons']), sorted(pinned_mq['dungeons']))
    fixture_table = {'+'.join(sorted(r['mq'])): sorted(r['ootSources']) for r in mq['table']}
    pinned_table = {'+'.join(sorted(r['mq'])): r['ootSources'] for r in pinned_mq['table']}
    report.check('mq', 'vanilla/MQ contexts', sorted(fixture_table), sorted(pinned_table))
    for key in sorted(fixture_table):
        report.check('mq', f'OoT Wallmaster sources, MQ [{key or "none"}]', fixture_table[key],
                     pinned_table.get(key))
    report.check('mq', 'rows other than OoT Wallmaster sources that change with the MQ choice', {},
                 pinned_mq['otherRowsDependOnMq'])

    def shortfalls(table):
        return {'+'.join(sorted(s['mq'])): {'ootSources': s['ootSources'], 'combinations': sorted(s['combinations'])}
                for s in table}
    fixture_short, pinned_short = shortfalls(mq['capacityShortfalls']), shortfalls(pinned_mq['capacityShortfalls'])
    report.check('capacity', 'contexts with more OoT Wallmaster sources than destinations', sorted(fixture_short),
                 sorted(pinned_short))
    for key in sorted(set(fixture_short) & set(pinned_short)):
        report.check('capacity', f'MQ [{key}]', fixture_short[key], pinned_short[key])
    # Real attempts in each vanilla/MQ context: every shortfall fails, and Wallmasters alone places elsewhere.
    alone = 'erWallmasters=ownGame;erOneWays=none'
    attempts = {'+'.join(sorted(r['mq'])): r.get('placements', {}) for r in pinned_mq['table']}
    for key in sorted(fixture_table):
        expected = {cid: False for cid in fixture_short.get(key, {}).get('combinations', [])}
        if alone not in expected:
            expected[alone] = True
        report.check('placement', f'MQ [{key or "none"}] attempts placed', expected,
                     {cid: a['placed'] for cid, a in sorted(attempts.get(key, {}).items())})
    return report


def run_node(node, reference, contract, facts_path, jobs):
    version = subprocess.run([str(node), '--version'], capture_output=True, text=True, timeout=60).stdout.strip()
    if version != PINNED_NODE_VERSION:
        raise RuntimeError(f'Expected the pinned Node {PINNED_NODE_VERSION}, found {version or "nothing"} at {node}')
    verified_blob(reference, 'data/defs/entrances.yml')  # refuses any other HEAD
    command = [str(node), *NODE_FLAGS, str(RUNNER), '--reference', str(reference), '--contract', str(contract),
               '--out', str(facts_path)] + (['--jobs', str(jobs)] if jobs else [])
    result = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', timeout=1800)
    if result.returncode != 0:
        raise RuntimeError(f'Pinned Node run failed ({result.returncode}):\n{result.stdout}{result.stderr}')
    return version, result.stdout.strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--reference', type=Path, help='Local checkout at the exact OoTMM v32.3 commit')
    parser.add_argument('--node', type=Path, help=f'Pinned Node {PINNED_NODE_VERSION}; omit to compare saved --facts')
    parser.add_argument('--facts', type=Path, help='Facts file to write (with --node) or read (without)')
    parser.add_argument('--contract', type=Path, default=CONTRACT,
                        help='Fixture to compare (default: the committed one)')
    parser.add_argument('--out', type=Path, help='Write the JSON report here')
    parser.add_argument('--jobs', type=int, default=0, help='Node worker threads (default: the runner decides)')
    args = parser.parse_args()
    if args.node is None and args.facts is None:
        parser.error('give --node and --reference to run OoTMM, or --facts to compare saved facts')

    contract_bytes = args.contract.read_bytes()
    contract = json.loads(contract_bytes)
    with tempfile.TemporaryDirectory(prefix='m1-pinned-') as scratch:
        facts_path = args.facts or Path(scratch)/'facts.json'
        run = None
        if args.node is not None:
            if args.reference is None:
                parser.error('--node needs --reference')
            run = run_node(args.node, args.reference.resolve(), args.contract.resolve(), facts_path, args.jobs)
        facts_bytes = facts_path.read_bytes()
    facts = json.loads(facts_bytes)
    report = compare(contract, facts)
    result = {
        'result': 'agree' if not report.divergences else 'diverge',
        'upstreamCommit': PIN,
        'node': facts['node'],
        'contract': {'path': str(args.contract), 'sha256': hashlib.sha256(contract_bytes).hexdigest()},
        'factsSha256': hashlib.sha256(facts_bytes).hexdigest(),
        'injectedLine': facts['injectedLine'],
        'packages': facts['packages'],
        'coreBuildDiffersFromCheckoutDist': sorted(n for n, v in facts['coreBuild'].items()
                                                   if not v['checkoutDistMatches']),
        'seconds': facts['seconds'],
        'checked': dict(sorted(report.checked.items())),
        'divergences': report.divergences,
    }
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=1) + '\n', encoding='utf-8')
    if run:
        print(run[1])
    total = sum(report.checked.values())
    if report.divergences:
        print(f'{len(report.divergences)} of {total} compared facts diverge from pinned OoTMM:', file=sys.stderr)
        for d in report.divergences[:50]:
            print(f'  [{d["fact"]}] {d["where"]}\n    fixture: {json.dumps(d["fixture"])}\n'
                  f'    pinned:  {json.dumps(d["pinned"])}\n    rule:    {d["source"]}', file=sys.stderr)
        sys.exit(1)
    print(f'All {total} compared facts agree with pinned OoTMM ({len(contract["combinations"])} combinations, '
          f'{len(facts["mqSources"]["table"])} MQ contexts, Node {facts["node"]}).')


if __name__ == '__main__':
    main()
