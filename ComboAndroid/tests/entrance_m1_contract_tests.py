"""The pinned M1 contract fixture (tools/ootmm/m1-contract.json).

Independent constants from the pinned reference are checked against the fixture on every host. With the
exact OoTMM v32.3 checkout present, the exporter must also reproduce the bytes, reject stale output
read-only, refuse a wrong HEAD and fail when a guarded upstream body changes.
"""
from pathlib import Path
import argparse
import json
import os
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_m1_contract as contract  # noqa: E402

FIXTURE = source/contract.OUTPUT
raw = FIXTURE.read_bytes()
data = json.loads(raw)
assert b'\r' not in raw and contract.contract_bytes(data) == raw  # canonical bytes, independent of checkout
attributes = subprocess.check_output(['git', '-C', str(source), '-c', 'core.autocrlf=true', 'check-attr', 'eol',
                                      '--', contract.OUTPUT], text=True, timeout=30)
assert attributes.strip().endswith(': eol: lf'), attributes
assert data['upstreamCommit'] == '413573d22d1d53e8a81931a70927f625b73488b9'
rows = data['rows']

# Wallmaster rows: 13 OoT synthetic sources 0xf10-0xf1c and 4 MM 0xff00-0xff03 (entrances.yml:182-194, 517-520).
wallmasters = {name: row for name, row in rows.items() if row['type'] == 'wallmaster'}
assert sorted(r['id'] for r in wallmasters.values() if r['game'] == 'oot') == list(range(0xf10, 0xf1d))
assert sorted(r['id'] for r in wallmasters.values() if r['game'] == 'mm') == list(range(0xff00, 0xff04))
assert all(r['to'] == ('OOT VOID' if r['game'] == 'oot' else 'MM VOID') and not r['reverse']
           for r in wallmasters.values())

# 23 OoT edges (11 vanilla, 12 MQ) and 4 MM edges into Wallmaster areas, all soul-gated; 24 OoT
# soul_wallmaster lines, the extra one being spirit_temple_mq.yml:92.
sources = data['sources']
oot_edges = [e for s in sources.values() if s['type'] == 'wallmaster' and s['game'] == 'oot' for e in s['incoming']]
mm_edges = [e for s in sources.values() if s['type'] == 'wallmaster' and s['game'] == 'mm' for e in s['incoming']]
assert len(oot_edges) == 23 and len(mm_edges) == 4
assert sum(e['quest'] == 'vanilla' for e in oot_edges) == 11 and sum(e['quest'] == 'mq' for e in oot_edges) == 12
assert all('soul_wallmaster' in e['expression'] for e in oot_edges + mm_edges)
assert {(e['file'], e['line']) for e in mm_edges} == {
    ('data/world/mm/dungeons/beneath_the_well.yml', 51), ('data/world/mm/dungeons/beneath_the_well.yml', 91),
    ('data/world/mm/dungeons/beneath_the_well.yml', 170), ('data/world/mm/overworld/ikana_graveyard.yml', 159)}
souls = {(line['file'], line['line']) for line in data['soulWallmasterLines']}
assert len(souls) == 24 and souls - {(e['file'], e['line']) for e in oot_edges} == {
    ('data/world/oot/dungeons_mq/spirit_temple_mq.yml', 92)}
assert all(c['expression'] == 'true' for s in sources.values() if s['type'] == 'wallmaster' for c in s['carried'])
# One-way source expressions a placed row keeps (gerudo_valley.yml:118, death_mountain_trail.yml:72, ...).
carried = {name: [c['expression'] for c in s['carried']] for name, s in sources.items() if s['type'] != 'wallmaster'}
assert carried == {
    'OOT_LAKE_HYLIA_FROM_VALLEY': ['true'], 'OOT_VILLAGE_OWL': ['is_child'], 'OOT_FIELD_OWL': ['is_child'],
    'MM_SWAMP_FROM_IKANA_CANYON': ['true'], 'MM_IKANA_CASTLE_INTERIOR_KEG': ['event(IKANA_CASTLE_LIGHT2)'],
    'MM_IKANA_CASTLE_INTERIOR_BLOCK': ['event(IKANA_CASTLE_LIGHT)'], 'MM_VOID_GREAT_BAY': ['true'],
    'MM_VOID_GREAT_BAY_BY_PINNACLE_ROCK': ['true'], 'MM_VOID_ZORA_CAPE': ['true'], 'MM_VOID_PINNACLE_ROCK': ['true']}

# The settings matrix: 2 combinations without one-ways plus 2 x 32 with them.
combinations = {c['id']: c for c in data['combinations']}
assert len(combinations) == len(data['combinations']) == 66
FLAGS = ['erOneWaysMajor', 'erOneWaysIkana', 'erOneWaysOwls', 'erOneWaysWaterVoids', 'erOneWaysAnywhere']


def combo(wallmasters=False, *flags, one_ways=None):
    one_ways = bool(flags) if one_ways is None else one_ways
    return combinations[';'.join([f'erWallmasters={"ownGame" if wallmasters else "none"}',
                                  f'erOneWays={"ownGame" if one_ways else "none"}'] + [f for f in FLAGS if f in flags])]


def pool(entry, name):
    found = [p for p in entry['pools'] if p['name'] == name]
    assert len(found) == 1, (entry['id'], name)
    return found[0]


def candidates(entry, name):
    return {src: tuple(g['destinations']) for g in pool(entry, name)['candidates'] for src in g['sources']}


def per_game(entry, name, game):
    """Distinct candidate sets of the pool's sources in one game."""
    return {dsts for src, dsts in candidates(entry, name).items() if rows[src]['game'] == game}


for entry in data['combinations']:
    s = entry['settings']
    assert s['erOneWays'] != 'none' or not any(s[f] for f in FLAGS)
    assert entry['erEnabled'] == entry['isEntranceShuffle'] == (
        s['erWallmasters'] != 'none' or s['erOneWaysMajor'] or s['erOneWaysIkana'] or s['erOneWaysOwls'])
    for p in entry['pools']:
        assert p['ownGame'] and p['alias'] == (p['name'] == 'WALLMASTERS' or s['erOneWaysAnywhere'])
        for g in p['candidates']:
            assert all(rows[src]['game'] == rows[dst]['game'] for src in g['sources'] for dst in g['destinations'])
    # Reset-gated: every MM destination without no-sot (entrances.ts:203-217). VOID dead ends: water voids.
    dsts = {d for p in entry['pools'] for g in p['candidates'] for d in g['destinations']}
    assert entry['resetGated'] == sorted(d for d in dsts if rows[d]['game'] == 'mm' and not rows[d]['noSot'])
    assert entry['deadEnds'] == sorted(d for d in dsts if rows[d]['type'] == 'one-way-water-void')
    pairs = {(a, b) for p in entry['pools'] for g in p['candidates'] for a in g['sources'] for b in g['destinations']}
    assert all(tuple(pair) in pairs for pair in entry['sampleLayout'])
assert not combo(False).get('pools') and combo(False)['sampleLayout'] == []
assert not combo(False, 'erOneWaysAnywhere')['pools']  # no one-way type selected: the alias pool is empty
assert not combo(False, 'erOneWaysWaterVoids')['erEnabled'] and not combo(False, 'erOneWaysAnywhere')['erEnabled']

# Wallmasters alone: 11 OoT and 7 MM candidates; all 7 MM ones are no-sot (the frozen 4 -> 7 profile).
alone = combo(True)
assert [p['name'] for p in alone['pools']] == ['WALLMASTERS']
assert [len(x) for x in per_game(alone, 'WALLMASTERS', 'oot')] == [11]
assert [len(x) for x in per_game(alone, 'WALLMASTERS', 'mm')] == [7]
assert alone['resetGated'] == [] and alone['deadEnds'] == []
assert next(iter(per_game(alone, 'WALLMASTERS', 'mm'))) == (
    'MM_SPIDER_HOUSE_OCEAN', 'MM_SPIDER_HOUSE_SWAMP', 'MM_TEMPLE_GREAT_BAY', 'MM_TEMPLE_SNOWHEAD',
    'MM_TEMPLE_STONE_TOWER', 'MM_TEMPLE_STONE_TOWER_INVERTED', 'MM_TEMPLE_WOODFALL')

# With Major, Ikana, Owls and Water Voids (not Anywhere): 14 / 14, and permutations OoT 3<->3, MM 7<->7.
full = combo(True, 'erOneWaysMajor', 'erOneWaysIkana', 'erOneWaysOwls', 'erOneWaysWaterVoids')
assert [p['name'] for p in full['pools']] == ['ONE_WAYS', 'WALLMASTERS']
assert [len(x) for x in per_game(full, 'WALLMASTERS', 'oot')] == [14]
assert [len(x) for x in per_game(full, 'WALLMASTERS', 'mm')] == [14]
one_ways = pool(full, 'ONE_WAYS')
assert one_ways['srcCount'] == one_ways['dstCount'] == {'oot': 3, 'mm': 7} and not one_ways['alias']
assert all(len(dsts) == 3 for dsts in per_game(full, 'ONE_WAYS', 'oot'))
assert all(len(dsts) == 7 for dsts in per_game(full, 'ONE_WAYS', 'mm'))

# C25: per-flag counts with one-ways alone; Major alone is OoT 1<->1 and MM 1<->1, the identity.
for flag, oot, mm in [('erOneWaysMajor', 1, 1), ('erOneWaysIkana', 0, 2), ('erOneWaysOwls', 2, 0),
                      ('erOneWaysWaterVoids', 0, 4)]:
    single = pool(combo(False, flag), 'ONE_WAYS')
    assert single['srcCount'] == single['dstCount'] == {'oot': oot, 'mm': mm}, flag
assert candidates(combo(False, 'erOneWaysMajor'), 'ONE_WAYS') == {
    'OOT_LAKE_HYLIA_FROM_VALLEY': ('OOT_LAKE_HYLIA_FROM_VALLEY',),
    'MM_SWAMP_FROM_IKANA_CANYON': ('MM_SWAMP_FROM_IKANA_CANYON',)}

# Anywhere: one-way types leave the Wallmaster destinations; the alias one-way pool takes dungeon rows and
# their exits, and only the owls lose a self-loop row (entrances.ts:459-498).
anywhere = combo(True, *FLAGS)
assert [p['name'] for p in anywhere['pools']] == ['WALLMASTERS', 'ONE_WAYS']
assert per_game(anywhere, 'WALLMASTERS', 'oot') == per_game(alone, 'WALLMASTERS', 'oot')
anywhere_oot = candidates(anywhere, 'ONE_WAYS')
assert len(anywhere_oot['OOT_LAKE_HYLIA_FROM_VALLEY']) == 25
assert 'OOT_LAKE_HYLIA_FROM_TEMPLE_WATER' not in anywhere_oot['OOT_FIELD_OWL']
assert 'OOT_MOUNTAIN_TRAIL_FROM_DODONGO_CAVERN' not in anywhere_oot['OOT_VILLAGE_OWL']
assert len(anywhere_oot['OOT_FIELD_OWL']) == len(anywhere_oot['OOT_VILLAGE_OWL']) == 24
assert len(anywhere_oot['MM_SWAMP_FROM_IKANA_CANYON']) == 21

# MQ source table: all 64 choices of Forest, Spirit, Shadow, BotW, GTG and Ganon. MQ Spirit + MQ BotW with
# vanilla Shadow and Ganon gives 12 OoT sources for 11 destinations (Forest and GTG either way).
mq = data['mqSources']
assert sorted(mq['dungeons']) == sorted(['Forest', 'Shadow', 'Spirit', 'BotW', 'GTG', 'Ganon'])
assert len(mq['table']) == 64 and len({tuple(r['mq']) for r in mq['table']}) == 64
by_mq = {frozenset(r['mq']): r['ootSources'] for r in mq['table']}
assert len(by_mq[frozenset()]) == 9 and len(by_mq[frozenset(mq['dungeons'])]) == 10
assert max(len(v) for v in by_mq.values()) == 12
twelve = {key for key, v in by_mq.items() if len(v) == 12}
assert {key - {'Forest', 'GTG'} for key in twelve} == {frozenset({'Spirit', 'BotW'})} and len(twelve) == 4
assert {frozenset(s['mq']) for s in mq['capacityShortfalls']} == twelve
for shortfall in mq['capacityShortfalls']:
    assert shortfall['ootSources'] == 12 and alone['id'] in shortfall['combinations']
    assert full['id'] not in shortfall['combinations'] and anywhere['id'] in shortfall['combinations']
presence = data['registryAreaPresence']
assert presence['never'] == [] and all(rows[n]['type'] == 'wallmaster' for n in presence['questDependentRows'])

# C14: M1 never changes hint regions or dungeon entrances; checked for every combination.
assert data['regionInvariance']['extraHintRegions'] == [False, True] and len(data['regionInvariance']['checks']) == 4

# er_enabled / isEntranceShuffle count Major, Ikana, Owls and Wallmasters, never Water Voids or Anywhere.
tables = data['truthTables']
assert tables['erEnabled']['file'] == 'data/macros/macros_mm.yml' and tables['erEnabled']['line'] == 305
for table in tables.values():
    assert sorted(table['m1Keys']) == ['erOneWaysIkana', 'erOneWaysMajor', 'erOneWaysOwls', 'erWallmasters']
logic_sites = {(s['file'], s['line']) for s in data['erLogicSites']['lines']}
for site in [('data/world/mm/overworld/twin_island.yml', 14), ('data/world/mm/overworld/twin_island.yml', 19),
             ('data/world/mm/dungeons/beneath_the_well.yml', 106), ('data/world/mm/dungeons/snowhead_temple.yml', 128)]:
    assert site in logic_sites

# ER-wide runtime sites, as cited by the plan, and every pinned use of the flags inside one of them.
runtime = data['runtime']
cited = {(s['flag'], s['file'].split('packages/generator/src/')[1], tuple(s['lines'])) for s in runtime['sites']}
for site in [('CFG_ER_ANY', 'mm/play/play.c', (484, 494)), ('CFG_ER_ANY', 'mm/play/play.c', (736, 741)),
             ('CFG_ER_ANY', 'mm/play/play.c', (997, 1002)), ('CFG_ER_ANY', 'oot/play/play.c', (182, 186)),
             ('CFG_ER_ANY', 'oot/save.c', (38, 38)), ('CFG_ER_WALLMASTERS', 'oot/actors.c', (402, 412)),
             ('CFG_ER_WALLMASTERS', 'mm/actors.c', (637, 647))]:
    assert site in cited, site
assert runtime['hotWater'] == {'group': 'PG_ER', 'address': '0x8010eba0', 'instructions': ['jr ra', 'nop']}
assert len(runtime['occurrences']) == 9
assert {s['text'] for s in runtime['enableConditions']} >= {'ER: isEntranceShuffle(settings),',
                                                             'ER_ANY: isEntranceShuffle(settings),'}

# Spoiler 'Entrances' format (spoiler.ts:237-273) and the two default game-link lines of every ootmm seed.
spoiler = data['spoiler']
assert spoiler['noErSection'][0] == 'Entrances' and spoiler['noErSection'][-1] == ''
assert all(' -> ' in line and line.startswith('  ') for line in spoiler['noErSection'][1:-1])
assert spoiler['noErSection'][2].startswith('  OOT Market to OOT Market Mask Shop (OOT_SHOP_MASKS) ')

# Re-pin guard: the cited entrances.ts ranges.
guards = data['guardedBodies']
for name, lines in [('computeAllEntrances', [179, 201]), ('placeSingleExpr', [203, 217]), ('isSelfLoop', [459, 482]),
                    ('isAssignableConstraints', [484, 498]), ('entrancesForTypes', [601, 606]),
                    ('poolEntrancesForTypes', [608, 627]), ('poolWallmasters', [657, 669]),
                    ('poolOneWaysAnywhere', [742, 756]), ('poolOneWays', [758, 784]),
                    ('makePoolsSimple', [791, 861]), ('makePools', [863, 879]), ('poolsTypesDst', [881, 886]),
                    ('POLARITY_ANY_OVERWORLD', [43, 48]), ('POLARITY_OUT', [74, 80]),
                    ('processRegions', [1044, 1059])]:
    assert guards[name]['lines'] == lines and guards[name]['sha256'] == contract.EXPECTED_BODY_SHA256[name], name

# The restricted world reader fails closed on syntax it does not model and lists ignored area keys.
ignored = []
sample = '"Area":\n  region: NONE\n  exits:\n    "VOID": "true"\n  exit:\n    "X": "true"\n'
assert contract.parse_world_file('x.yml', sample, 'oot', ignored)['OOT Area']['maps']['exits'] == {'VOID': ('true', 4)}
assert ignored == [{'file': 'x.yml', 'line': 5, 'area': 'OOT Area', 'key': 'exit'}]
for bad in ['"Area":\n  exits: [a]\n', '"Area":\n\texits:\n', '"Area":\n  exits:\n    "A": &x "true"\n',
            '"Area": "x"\n', '"Area":\n  exits:\n    "A": "true"\n     "B": "true"\n', '  region: NONE\n']:
    try:
        contract.parse_world_file('x.yml', bad, 'oot', [])
    except ValueError:
        pass
    else:
        raise AssertionError(f'Unsupported world YAML was accepted: {bad!r}')
print('M1 contract constants, matrix, MQ table, edges, truth tables, runtime sites and spoiler format verified.')

reference = os.environ.get('COMBO_OOTMM_REFERENCE')
reference = Path(reference) if reference else next(
    (parent/'references/ootmm-stable-32.3' for parent in source.parents
     if (parent/'references/ootmm-stable-32.3/.git').exists()), None)
if reference is None:
    print('SKIPPED: set COMBO_OOTMM_REFERENCE to the exact local OoTMM v32.3 checkout to re-run the exporter')
    sys.exit(0)


def run(output, check=False, ref=reference):
    command = [sys.executable, str(source/'tools/ootmm/export_m1_contract.py'), '--reference', str(ref),
               '--output-root', str(output)] + (['--check'] if check else [])
    return subprocess.run(command, capture_output=True, text=True, timeout=300,
                          env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))


result = run(source, check=True)
assert result.returncode == 0, result.stdout + result.stderr
with tempfile.TemporaryDirectory(prefix='m1-contract-', dir=work) as scratch:
    scratch = Path(scratch)
    result = run(scratch)
    assert result.returncode == 0, result.stdout + result.stderr
    assert (scratch/contract.OUTPUT).read_bytes() == raw
    damaged = raw + b'\n'
    (scratch/contract.OUTPUT).write_bytes(damaged)
    assert run(scratch, check=True).returncode != 0
    assert (scratch/contract.OUTPUT).read_bytes() == damaged  # --check never writes
    before = {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}
    rejected = run(scratch, ref=source)  # this repository is not the pinned upstream revision
    assert rejected.returncode != 0 and 'Expected exact OoTMM v32.3 revision' in rejected.stderr
    assert before == {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}

# A changed cited body fails the export until the re-implementation is reviewed again.
blobs = contract.pinned_blobs(reference, [contract.ENTRANCES_TS, contract.BUILDER_TS, contract.HELPERS_TS,
                                          contract.SPOILER_TS, contract.SETTINGS_UTIL_TS, contract.CORE_UTIL_TS])
contract.guarded_bodies(blobs)
blobs[contract.ENTRANCES_TS] = blobs[contract.ENTRANCES_TS].replace(b"types.add('dungeon-sh');",
                                                                    b"types.add('dungeon-pf');", 1)
try:
    contract.guarded_bodies(blobs)
except RuntimeError as error:
    assert 'poolWallmasters changed' in str(error) and 're-review' in str(error)
else:
    raise AssertionError('A changed pinned pool body was accepted')
print('Exporter reproduces the fixture at the exact pin, rejects stale output read-only, refuses a wrong HEAD '
      'and fails on a changed guarded body.')
