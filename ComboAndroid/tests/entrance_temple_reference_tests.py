"""Pin the opposite-age Temple lemmas to the exact OoTMM v32.3 blobs.

Contract evidence only, not behaviour: the native probe (TempleProbe.cpp) relies on
these facts of the pinned reference. Skipped without the optional checkout, and
failing (never skipped) when the checkout is at another revision.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
reference = os.environ.get('COMBO_OOTMM_REFERENCE')
if reference:
    reference = Path(reference)
else:
    reference = next((parent/'references/ootmm-stable-32.3' for parent in source.parents
                      if (parent/'references/ootmm-stable-32.3/.git').exists()), None)
if reference is None:
    print('SKIPPED: set COMBO_OOTMM_REFERENCE to the exact local OoTMM v32.3 checkout')
    sys.exit(0)

sys.path.insert(0, str(source/'tools/ootmm'))
import export_registry as registry


def blob(path):
    return registry.verified_blob(reference, path).decode('utf-8')  # raises on any other revision


def method(text, signature):
    """Body of the one TypeScript method whose header starts with signature."""
    assert text.count(signature) == 1, signature
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('unterminated ' + signature)


def areas(path):
    """The four line shapes the pinned world files use; anything else fails closed."""
    result, area, section = {}, None, None
    for line in blob(path).splitlines():
        if not line.strip():
            continue
        if m := re.fullmatch(r'"([^"]+)":', line):
            assert m.group(1) not in result, line
            area, section = result.setdefault(m.group(1), {}), None
        elif m := re.fullmatch(r'  ([a-z_]+):(?: (.+))?', line):
            section = m.group(1)
            area[section] = m.group(2) if m.group(2) is not None else {}
        elif m := re.fullmatch(r'    "([^"]+)": "(.*)"', line) or re.fullmatch(r'    ([A-Za-z0-9_]+): "(.*)"', line):
            assert isinstance(area.get(section), dict), line
            area[section][m.group(1)] = m.group(2)
        else:
            raise AssertionError(f'{path}: unexpected line {line!r}')
    return result


solver = blob('packages/logic/src/solver/entrances.ts')
pathfind = blob('packages/logic/src/pathfind/pathfind.ts')

# validateAgeTemple (entrances.ts:265-286): only the two physical spawn exits change; the starting
# age's exit becomes false and the opposite exit a bare age check; starting items, never all items.
temple = method(solver, 'private validateAgeTemple(world: World)')
for line in ["a.exits['OOT SPAWN ADULT'] = exprAge(AGE_ADULT);", "a.exits['OOT SPAWN CHILD'] = exprFalse();",
             "a.exits['OOT SPAWN ADULT'] = exprFalse();", "a.exits['OOT SPAWN CHILD'] = exprAge(AGE_CHILD);",
             'new Pathfinder(worlds, this.settings, this.startingItems);',
             'agePathfinder.run(null, { recursive: true, singleWorld: this.worldId });',
             "const target = 'OOT Temple of Time';", 'return (ws.ages[otherAge].areas.has(target));']:
    assert line in temple, line
assert temple.count('a.exits[') == 4 and 'allItems' not in temple and 'assumedItems' not in temple
# Called only from isValidShuffle, after the all-items goal checks (:388-416), never by run() or validate().
assert solver.count('this.validateAgeTemple(') == 1
assert 'this.validateAgeTemple(newWorld)' in method(solver, 'private isValidShuffle(')
assert "if (['ootmm', 'oot'].includes(this.settings.games) && !this.validateAgeTemple(newWorld))" in solver

# changeWorldAssumePools (:289-347): ASSUMED is installed for every candidate because an array is
# always truthy; it is an MM-typed, time-still area reached from OOT SPAWN, empty without a large pool.
assume = method(solver, 'private changeWorldAssumePools(')
for line in ['const assumedSets: Set<Entrance>[] = [];', 'if (cardinality > 5000 && pool.src.size > 50) {',
             'if (assumedSets) {', "game: 'mm',", "time: 'still',", "world.areas['ASSUMED'] = assumedArea;",
             "world.areas['OOT SPAWN'].exits['ASSUMED'] = exprTrue();", "if (this.settings.regionState === 'dungeonBeaten') {",
             'if (this.isAssignableConstraints(oldName, newName, !!pool.opts.ownGame)) {',
             'this.placeSingleExpr(world, oldName, newName);', 'assumedExits[ee.to] = exprTrue();']:
    assert line in assume, line
# changedWorld (:349-385) deletes override and pool sources by area pair; placeSingleExpr (:203-218)
# keeps the original source expression, ORs a duplicate and gates MM destinations without no-sot.
changed = method(solver, 'private changedWorld(')
assert changed.count('delete newWorld.areas[oldE.from].exits[oldE.to];') == 2
single = method(solver, 'private placeSingleExpr(')
for line in ['let expr = this.getExpr(original);',
             "if (entranceReplacement.game === 'mm' && !(entranceReplacement.flags.includes('no-sot'))) {",
             'expr = exprOr([expr, areaFrom.exits[entranceReplacement.to]]);']:
    assert line in single, line
assert 'const from = this.world.areas[eData.from];' in method(solver, 'private getExpr(')
assert "this.overrideEntrance('OOT_SHOP_MASKS', 'MM_CLOCK_TOWN_FROM_CLOCK_TOWER');" in solver
assert "this.overrideEntrance('MM_CLOCK_TOWER_FROM_CLOCK_TOWN', 'OOT_MARKET_FROM_MASK_SHOP');" in solver

# Pathfinder (:98-109, :823-836): starting items are inventory and licenses with empty renewables and
# events; the virtual OOT SPAWN is explored at both ages.
state = pathfind[pathfind.index('const defaultWorldState = '):pathfind.index('const defaultWorldStates = ')]
for line in ['items: new Map(startingItems),', 'licenses: new Map(startingItems),', 'renewables: new Map(),',
             'events: new Set(),']:
    assert line in state, line
assert "this.exploreArea(worldId, AGE_CHILD, 'OOT SPAWN', cloneAreaData(initAreaData), 'OOT SPAWN');" in pathfind
assert "this.exploreArea(worldId, AGE_ADULT, 'OOT SPAWN', cloneAreaData(initAreaData), 'OOT SPAWN');" in pathfind
# L2: age changes only on the time-travel-at-will event (:14, :343, :540), never by a song.
assert "const EVENT_TIME_TRAVEL = 'OOT_TIME_TRAVEL_AT_WILL';" in pathfind
assert pathfind.count('EVENT_TIME_TRAVEL') == 3 and pathfind.count('otherAge') == 2

system = areas('data/world/oot/overworld/_system.yml')
tot = areas('data/world/oot/overworld/temple_of_time.yml')
kokiri = areas('data/world/oot/overworld/kokiri_forest.yml')
market = areas('data/world/oot/overworld/market.yml')
mm_system = areas('data/world/mm/overworld/_system.yml')

# L1: SPAWN has exactly the two physical spawn exits (_system.yml:4-21), which the probe rewrites, and
# no events; with ASSUMED empty and no dungeonBeaten events the starting age stays at SPAWN.
assert system['SPAWN']['exits'] == {
    'SPAWN CHILD': 'is_child && (setting(startingAge, child) || event(TIME_TRAVEL))',
    'SPAWN ADULT': 'is_adult && (setting(startingAge, adult) || event(TIME_TRAVEL))'}
assert set(system['SPAWN']) == {'region', 'age_change', 'exits'}
assert system['SPAWN CHILD']['exits'] == {"Link's House": 'true'}
assert system['SPAWN ADULT']['exits'] == {'Temple of Time': 'true'}

# L2: TIME_TRAVEL and TIME_TRAVEL_AT_WILL are defined only on Temple of Time (temple_of_time.yml:9-10),
# so no age change can happen before the opposite age has reached the target.
definitions = subprocess.check_output(['git', '-C', str(reference), 'grep', '-n', '-E',
                                       r'^\s+TIME_TRAVEL(_AT_WILL)?:', registry.PIN, '--', 'data/world'],
                                      text=True, timeout=60).splitlines()
assert [line.split(':')[1] for line in definitions] == ['data/world/oot/overworld/temple_of_time.yml'] * 2
assert set(tot['Temple of Time']['events']) == {'DOOR_OF_TIME_OPEN', 'TIME_TRAVEL', 'TIME_TRAVEL_AT_WILL'}

# L3: GLOBAL (warp songs, Soaring) hangs off physical areas, including both spawn targets; recorded for
# the later warp-song widening. GLOBAL's only MM edge needs Soaring (_system.yml:23-34).
assert kokiri["Link's House"]['exits']['GLOBAL'] == 'true'
assert tot['Temple of Time']['exits']['GLOBAL'] == 'true'
assert system['GLOBAL']['exits']['MM SOARING'] == 'can_play_soaring && setting(games, ootmm)'
assert all(v.startswith('can_play_tp_') for k, v in system['GLOBAL']['exits'].items() if k.startswith('SONG_TP_'))

# L4: with own-game pools, the reserved game link and no starting Soaring or warp songs, OoT enters MM
# only through the child-only Mask Shop door in Market (market.yml:31), and Market reaches Temple of
# Time through two unconditional exits that no T1 pool deletes (market.yml:25, :266-272). Any MM detour
# therefore starts after the target is reached. (OoT macros DO read MM events under shared settings,
# macros_oot.yml:70-233, so the lemma rests on this structure, not on event separation.)
assert market['Market']['exits']['Market Mask Shop'].startswith('is_child && ')
assert market['Market']['exits']['Temple of Time Entryway'] == 'true'
assert market['Temple of Time Entryway']['exits']['Temple of Time'] == 'true'
assert 'event(MM_' in blob('data/macros/macros_oot.yml')
assert set(mm_system['WARP_SONGS']['exits']) - {'ACCESS'} == {f'OOT SONG_TP_{s}' for s in
                                                              ('FOREST', 'FIRE', 'WATER', 'SHADOW', 'SPIRIT', 'LIGHT')}
assert all(v.startswith('can_play_tp_') for k, v in mm_system['WARP_SONGS']['exits'].items() if k != 'ACCESS')
for pattern, game in [('"OOT ', 'mm'), ('"MM ', 'oot')]:
    found = subprocess.check_output(['git', '-C', str(reference), 'grep', '-l', '-F', pattern, registry.PIN, '--',
                                     f'data/world/{game}'], text=True, timeout=60).split()
    assert found == [f'{registry.PIN}:data/world/{game}/overworld/_system.yml'], found
_, records = registry.read_registry(reference)
rows = {r['name']: r for r in records}
assert rows['OOT_TEMPLE_OF_TIME_ENTRYWAY_FROM_MARKET']['type'] == 'overworld'
assert rows['OOT_TEMPLE_OF_TIME']['type'] == 'indoors-extra'
assert {rows[n]['type'] for n in ('OOT_LAKE_HYLIA_FROM_VALLEY', 'OOT_FIELD_OWL', 'OOT_VILLAGE_OWL')} == {
    'one-way', 'one-way-owl'}
assert rows['OOT_SHOP_MASKS']['from'] == 'OOT Market' and rows['OOT_SHOP_MASKS']['to'] == 'OOT Market Mask Shop'

# The tranche-1 native bindings are reproducible from the audit, read-only under --check, and bound to
# this exact pin (tools/ootmm/export_oot_bindings.py).
outputs = ['soh/soh/Enhancements/randomizer/combo_oot_edge_bindings.inc',
           'combo/rando/entrances/OotEdgeBindingNames.h', 'tools/ootmm/oot-edge-audit.json']
attributes = subprocess.check_output(['git', '-C', str(source), '-c', 'core.autocrlf=true', 'check-attr', 'eol',
                                      '--', *outputs], text=True, timeout=30)
assert all(line.endswith(': eol: lf') for line in attributes.splitlines()) and len(attributes.splitlines()) == 3


def export(output, check=False, ref=reference):
    command = [sys.executable, str(source/'tools/ootmm/export_oot_bindings.py'), '--reference', str(ref),
               '--output-root', str(output)] + (['--check'] if check else [])
    return subprocess.run(command, capture_output=True, text=True, timeout=60)


work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='oot-bindings-', dir=work) as scratch:
    scratch = Path(scratch)
    result = export(source, check=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = export(scratch)
    assert result.returncode == 0, result.stdout + result.stderr
    for target in outputs[:2]:
        assert (scratch/target).read_bytes() == (source/target).read_bytes(), target
        damaged = (scratch/target).read_bytes() + b'// deliberate stale output\n'
        (scratch/target).write_bytes(damaged)
        assert export(scratch, check=True).returncode != 0
        assert (scratch/target).read_bytes() == damaged
        (scratch/target).write_bytes((source/target).read_bytes())
    rejected = export(scratch, ref=source)
    assert rejected.returncode != 0 and 'Expected exact OoTMM v32.3 revision' in rejected.stderr
print('Pinned opposite-age Temple lemmas L1-L4 and tranche-1 bindings verified at', registry.PIN)
