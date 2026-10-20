"""The OoT Wallmaster edge fixture and native anchor audit (unit M1-12).

On every host: the fixture's independent constants (13 sources 0xf10-0xf1c, 23 edges of which 11 vanilla
and 12 MQ, 24 soul_wallmaster lines, the same edges as the M1 contract), the audit schema (every edge has
a status, a ready edge has an anchor, a native condition and evidence, edges an open blocker holds never
reach the M1-13/M1-14 work lists), the native condition parser, the audit's anchors and evidence lines
in this checkout, and audit mutations the verifier must reject. Also on every host, the 12 MQ edges'
actor-room evidence against the MQ room lists this repository bundles (combo/mq/data/mq_rooms.bin, which
closed blocker C17). Locally, the vanilla actor-room evidence against the user's extracted OoT archive.
With the exact OoTMM v32.3 checkout: exporter reproduction, read-only stale checks, wrong-HEAD refusal and
the pinned evidence lines.
"""
from pathlib import Path
import argparse
import copy
import json
import os
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--archive', default='')
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source/'tools/ootmm'))
import export_m1_contract as contract  # noqa: E402
import export_oot_wallmaster_edges as edges_tool  # noqa: E402

PIN = '413573d22d1d53e8a81931a70927f625b73488b9'
raw = (source/edges_tool.OUTPUT).read_bytes()
fixture = json.loads(raw)
audit_raw = (source/edges_tool.AUDIT).read_bytes()
audit = json.loads(audit_raw)
assert b'\r' not in raw and edges_tool.fixture_bytes(fixture) == raw
assert b'\r' not in audit_raw and contract.contract_bytes(audit) == audit_raw
for path in (edges_tool.OUTPUT, edges_tool.AUDIT):
    attributes = subprocess.check_output(['git', '-C', str(source), '-c', 'core.autocrlf=true', 'check-attr', 'eol',
                                          '--', path], text=True, timeout=30)
    assert attributes.strip().endswith(': eol: lf'), attributes
assert fixture['upstreamCommit'] == audit['upstreamCommit'] == PIN

# Constants from the pinned registry (entrances.yml:182-194) and world files: 13 sources, 23 edges, 24 soul lines.
assert fixture['counts'] == {'sources': 13, 'edges': 23, 'vanillaEdges': 11, 'mqEdges': 12, 'soulWallmasterLines': 24}
sources = {s['name']: s for s in fixture['sources']}
assert sorted(int(s['id'], 16) for s in sources.values()) == list(range(0xf10, 0xf1d))
assert all(s['to'] == 'OOT VOID' for s in sources.values())
quests = {name: s['quests'] for name, s in sources.items()}
assert quests['OOT_WALLMASTER_SHADOW'] == quests['OOT_WALLMASTER_GANON_LIGHT'] == \
    quests['OOT_WALLMASTER_SPIRIT_CHILD_RUPEES'] == ['vanilla']
assert quests['OOT_WALLMASTER_SPIRIT_CHILD_SUN'] == quests['OOT_WALLMASTER_SPIRIT_STATUE'] == \
    quests['OOT_WALLMASTER_BOTW_BASEMENT'] == quests['OOT_WALLMASTER_BOTW_PIT'] == ['mq']
# Runtime room rule (En_Wallmas.c:6-45): Forest 0x14 East, Spirit 02/05/08/0f, BotW 00/01/03, Ganon 0x0a Light.
rooms = {name: (s['runtime']['rooms'], s['runtime']['default']) for name, s in sources.items()}
assert rooms['OOT_WALLMASTER_FOREST_CORRIDOR_EAST'] == ([20], False)
assert rooms['OOT_WALLMASTER_FOREST_CORRIDOR_WEST'] == rooms['OOT_WALLMASTER_GANON_SPIRIT'] == ([], True)
assert [rooms[f'OOT_WALLMASTER_SPIRIT_{n}'][0] for n in ('CHILD_RUPEES', 'STATUE', 'CHILD_SUN', 'ADULT_CLIMB')] == \
    [[2], [5], [8], [15]]
assert [rooms[f'OOT_WALLMASTER_BOTW_{n}'][0] for n in ('MAIN', 'BASEMENT', 'PIT')] == [[0], [1], [3]]
assert rooms['OOT_WALLMASTER_GANON_LIGHT'] == ([10], False)
edges = fixture['edges']
assert [e['n'] for e in edges] == list(range(1, 24))
assert sum(e['quest'] == 'vanilla' for e in edges) == 11 and all('soul_wallmaster' in e['atoms'] for e in edges)
assert all(e['ingress'] for e in edges)
non_edge = [(s['file'], s['line']) for s in fixture['soulWallmasterLines'] if not s['edge']]
assert non_edge == [('data/world/oot/dungeons_mq/spirit_temple_mq.yml', 92)]
assert sorted(fixture['events']) == ['GANON_MQ_SPIRIT_ENDING_SUNS', 'GANON_MQ_SPIRIT_ZOMBIES']
assert fixture['macros']['has_lens']['body'] == 'has_lens_strict || trick(OOT_LENS)'

# The same 23 edges, with the same expressions, as the M1 contract (export_m1_contract.py).
m1 = json.loads((source/contract.OUTPUT).read_text(encoding='utf-8'))
m1_edges = {f'{e["file"]}:{e["line"]}': e['expression'] for name, s in m1['sources'].items()
            if s['type'] == 'wallmaster' and s['game'] == 'oot' for e in s['incoming']}
assert m1_edges == {e['id']: e['expression'] for e in edges}

# The expression parser and the false-atom reduction behind the false status.
tree = edges_tool.parse_expression('(has_bombchu || climb_anywhere || hookshot_anywhere) && soul_wallmaster')
assert edges_tool.reduce_false(tree, {'climb_anywhere', 'hookshot_anywhere'}) == ('and', 'has_bombchu', 'soul_wallmaster')
assert edges_tool.reduce_false(edges_tool.parse_expression('hookshot_anywhere && climb_anywhere && soul_wallmaster'),
                               {'climb_anywhere', 'hookshot_anywhere'}) is False
assert edges_tool.atoms(edges_tool.parse_expression('soul_wallmaster && event(GANON_MQ_SPIRIT_ENDING_SUNS)')) == \
    {'soul_wallmaster', 'event(GANON_MQ_SPIRIT_ENDING_SUNS)'}
for bad in ['a &&', '(a || b', 'a b', 'a & b']:
    try:
        edges_tool.parse_expression(bad)
    except ValueError:
        pass
    else:
        raise AssertionError(f'Unsupported expression accepted: {bad!r}')

# The native condition parser: call arguments stay inside their atom, and the tree compares by structure.
assert edges_tool.parse_native('a->B(x, y) && (c || !d::E(f.g()))') == \
    ('and', 'a->B(x,y)', ('or', 'c', ('not', 'd::E(f.g())')))
assert edges_tool.canonical(edges_tool.parse_native('a && (b && c)')) == \
    edges_tool.canonical(edges_tool.parse_native('c && b && a'))
assert edges_tool.canonical(edges_tool.parse_native('a || b && c')) != \
    edges_tool.canonical(edges_tool.parse_native('(a || b) && c'))
for bad in ['a ? b : c', 'a == b', 'a(', '[]{return a;}()', 'a &&']:
    try:
        edges_tool.parse_native(bad)
    except ValueError:
        pass
    else:
        raise AssertionError(f'Unsupported native condition accepted: {bad!r}')

# Audit schema: one entry per edge, every edge has a status, a ready edge has an anchor, a condition and evidence.
entries = {e['id']: e for e in audit['edges']}
assert set(entries) == {e['id'] for e in edges} and len(entries) == 23
by_n = {e['n']: e for e in audit['edges']}
assert all(e['status'] in edges_tool.STATUSES for e in audit['edges'])
assert by_n[16]['status'] == 'false' and by_n[19]['status'] == 'ready'
# Edge 19 reads the two COMBO_BUILD events copied from the pinned Sun Room Sun Side (ganon_castle_mq.yml:275-276).
assert 'logic->Get(LOGIC_GANON_MQ_SPIRIT_ENDING_SUNS)' in by_n[19]['condition']['native']
assert [e['text'].split(',')[0] for e in by_n[19]['evidence'] if e['text'].startswith('EVENT_ACCESS(')] == \
    ['EVENT_ACCESS(LOGIC_GANON_MQ_SPIRIT_ZOMBIES', 'EVENT_ACCESS(LOGIC_GANON_MQ_SPIRIT_ENDING_SUNS']
# Edge 21's anchor was chosen from MQ Spirit room 5's positions once C17 closed: the main floor.
assert by_n[21]['status'] == 'ready' and by_n[21]['ingress']['relation'] == 'native-stronger'
assert by_n[21]['anchor']['region'] == 'RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM'
assert {n for n, e in by_n.items() if e['status'] == 'ready'} == set(range(1, 24)) - {16}
for entry in audit['edges']:
    assert entry['evidence'] and entry['note'], entry['n']
    if entry['status'] != 'false':
        assert entry['anchor']['region'].startswith('RR_') and entry['condition']['native'], entry['n']
    room = entry['actorRoom']
    assert room['status'] == 'verified' and any(w['role'] == 'capture' for w in room['wallmasters']), entry['n']
    if entry['quest'] == 'mq':  # the rooms C17 predicted are the rooms the bundled MQ data has
        assert all(w['room'] in room['expectedRooms'] for w in room['wallmasters']), entry['n']
assert audit['archive']['verifiedQuests'] == ['vanilla', 'mq-ootmm-32.3']
assert audit['archive']['mqSource'] == {'file': 'combo/mq/data/mq_rooms.bin', 'sha256': edges_tool.MQ_ROOMS_SHA256,
                                        'origin': audit['archive']['mqSource']['origin'],
                                        'dungeonIds': edges_tool.MQ_DUNGEON_IDS}
assert audit['openBlockers'] == []  # C17 closed offline from the bundled MQ room lists
# Every edge has verified actor rooms and no blocker: the statuses alone give the work lists.
assert audit['workList'] == {'M1-13': [],
                             'M1-14': sorted(e['id'] for e in audit['edges'] if e['status'] == 'ready'),
                             'blocked': {}}
# Edge 23: both MQ sun-block Wallmasters are spawned by lit floor suns, so the native light means is a port atom.
assert [a['native'] for a in by_n[23]['condition']['portAtoms']] == \
    ['logic->HasItem(RG_POWER_BRACELET) || logic->SunlightArrows()']
# Edge 06 copies the pinned Light End lens gate (ganon_castle.yml:57) instead of the lens-free native ingress.
assert by_n[6]['ingress']['relation'] == 'equal-with-ingress-atom' and \
    'ctx->GetTrickOption(RT_LENS_GANON) || logic->CanUse(RG_LENS_OF_TRUTH)' in by_n[6]['condition']['native']
assert 'native-weaker' not in audit['relations']
assert by_n[7]['anchor']['region'] == 'RR_GANONS_CASTLE_SPIRIT_TRIAL_BEFORE_SWITCH' and \
    'RG_FAIRY_BOW' not in by_n[7]['condition']['native']  # the capture never needs the Bow
assert by_n[17]['condition']['native'].startswith('logic->SmallKeys(SCENE_FOREST_TEMPLE, 5) && ')

# The verifier: anchors and evidence lines of this checkout (pinned lines need the reference, below).
assert edges_tool.verify_audit(audit, fixture, source) == []


def rejected(change, blobs=None):
    mutated = copy.deepcopy(audit)
    change(mutated)
    return edges_tool.verify_audit(mutated, fixture, source, blobs)


def edge(a, n):
    return a['edges'][n - 1]


mutations = {
    'anchor line moved': lambda a: edge(a, 1)['anchor'].update(line=edge(a, 1)['anchor']['line'] + 1),
    'MQ edge on a vanilla region': lambda a: edge(a, 12)['anchor'].update(edge(a, 1)['anchor']),
    'native evidence moved': lambda a: edge(a, 2)['evidence'][0].update(line=edge(a, 2)['evidence'][0]['line'] + 1),
    'false edge marked ready': lambda a: edge(a, 16).update(status='ready'),
    'live edge marked false': lambda a: edge(a, 15).update(status='false'),
    'kept atom dropped': lambda a: edge(a, 7)['condition'].update(
        native=edge(a, 7)['condition']['native'].replace('logic->CanUse(RG_BOMBCHU_5) && ', '')),
    'ingress atom dropped': lambda a: edge(a, 17)['condition'].update(
        native=edge(a, 17)['condition']['native'].replace('logic->SmallKeys(SCENE_FOREST_TEMPLE, 5) && ', '')),
    'edge missing': lambda a: a['edges'].pop(5),
    'pinned expression changed': lambda a: edge(a, 9).update(expression='true'),
    'ready edge without evidence': lambda a: edge(a, 3).update(evidence=[]),
    'needs-event without proposal': lambda a: (edge(a, 19).update(status='needs-event'), edge(a, 19).pop('proposal')),
    'MQ room evidence reopened without a blocker': lambda a: edge(a, 12)['actorRoom'].update(status='pending'),
    'capture room of another source': lambda a: edge(a, 10)['actorRoom']['wallmasters'][0].update(room=15),
    'MQ evidence without its quest source': lambda a: a['archive']['verifiedQuests'].remove('mq-ootmm-32.3'),
    'MQ capture outside the predicted rooms': lambda a: edge(a, 13)['actorRoom'].update(expectedRooms=[3]),
    'sun-block port atom dropped': lambda a: edge(a, 23)['condition'].update(
        native='Rando::Souls::Has(logic.get(), Rando::Souls::OotSoul::ENEMY_WALLMASTER)'),
    'work list out of date': lambda a: a['workList']['M1-14'].pop(),
    'kept conjunction weakened to ||': lambda a: edge(a, 7)['condition'].update(
        native=edge(a, 7)['condition']['native'].replace('RG_BOMBCHU_5) &&', 'RG_BOMBCHU_5) ||')),
    'ingress conjunction weakened to ||': lambda a: edge(a, 17)['condition'].update(
        native=edge(a, 17)['condition']['native'].replace('5) &&', '5) ||')),
    'condition widened by || true': lambda a: edge(a, 2)['condition'].update(
        native=edge(a, 2)['condition']['native'] + ' || true'),
    'lens ingress dropped from the condition': lambda a: edge(a, 6)['condition'].update(
        native=edge(a, 6)['condition']['native'].split(' && ', 1)[1]),
    'ingress relation loosened': lambda a: edge(a, 6)['ingress'].update(relation='native-weaker'),
    'loosening relation listed': lambda a: a['relations'].update({'native-weaker': 'native needs less'}),
    'provisional relation on a ready edge': lambda a: edge(a, 21)['ingress'].update(relation='provisional'),
    'needs-evidence without a blocker': lambda a: edge(a, 21).update(status='needs-evidence'),
    'needs-event edge left in M1-14': lambda a: edge(a, 19).update(status='needs-event'),
    'ganon events dropped from the condition': lambda a: edge(a, 19)['condition'].update(
        native='Rando::Souls::Has(logic.get(), Rando::Souls::OotSoul::ENEMY_WALLMASTER)'),
    'absent setting now present': lambda a: a['predicateAtoms']['climb_anywhere']['absent'][0]['tokens'].append('RSK_'),
}
for name, change in mutations.items():
    assert rejected(change), f'Audit mutation not rejected: {name}'
print(f'Edge fixture constants, audit schema, anchors and {len(mutations)} rejected audit mutations verified.')

# Every host: the MQ actor-room evidence against the bundled MQ room lists (C17), which the bundled-MQ runtime loads.
mq_bin = source/edges_tool.MQ_ROOMS
assert edges_tool.verify_mq_rooms(audit, mq_bin) == []
mq_rooms = edges_tool.load_mq_rooms(mq_bin)
found = sorted((dungeon, room, index, f'0x{actor["params"]:04x}') for dungeon, (rooms_of, _) in mq_rooms.items()
               for room, actors in rooms_of.items() for index, actor in enumerate(actors)
               if actor['actor'] == edges_tool.WALLMASTER_ACTOR)
# The En_Wallmas actors of the MQ bins (the design's C17 table); MQ Shadow has none.
assert found == sorted([('Forest', 19, 0, '0x0001'), ('Forest', 20, 2, '0x0001'), ('Spirit', 5, 0, '0x0001'),
                        ('Spirit', 5, 20, '0x3302'), ('Spirit', 8, 18, '0x3a02'), ('Spirit', 8, 20, '0x3b02'),
                        ('Spirit', 15, 2, '0x0000'), ('Spirit', 15, 7, '0x3a02'), ('Spirit', 15, 9, '0x3b02'),
                        ('Spirit', 15, 11, '0x3d02'), ('BotW', 0, 3, '0x0001'), ('BotW', 1, 4, '0x0000'),
                        ('BotW', 3, 0, '0x0000'), ('GTG', 2, 17, '0x0001'), ('GTG', 2, 21, '0x1102'),
                        ('Ganon', 18, 24, '0x3902')]), found
# Edge 21's anchor: the room-5 proximity Wallmaster on the main floor, the switch-flag one above the inner left hand.
assert mq_rooms['Spirit'][0][5][0]['pos'] == [66, 353, -1188] and \
    mq_rooms['Spirit'][0][5][20]['pos'] == [426, 1041, -1237]


def mutated(n, change):
    copied = copy.deepcopy(audit)
    change({e['n']: e for e in copied['edges']}[n])
    return edges_tool.verify_mq_rooms(copied, mq_bin)


assert any('edge 21' in e for e in mutated(21, lambda e: e['actorRoom']['wallmasters'][0].update(pos=[60, 353, -1188])))
assert any('not accounted for' in e for e in mutated(22, lambda e: e['actorRoom']['wallmasters'].pop(0)))
assert any('type 0' in e for e in mutated(13, lambda e: e['actorRoom']['wallmasters'][0].update(type='proximity')))
damaged_bin = work/'mq_rooms_damaged.bin'
damaged_bin.write_bytes(mq_bin.read_bytes()[:-1] + bytes([mq_bin.read_bytes()[-1] ^ 1]))
try:
    edges_tool.verify_mq_rooms(audit, damaged_bin)
except ValueError as error:
    assert 'not OoTMM v32.3 mq_rooms.bin' in str(error)
else:
    raise AssertionError('A changed MQ room list was read')
print('All 12 MQ edges verified from the pinned bundled MQ room lists: 16 Wallmasters in 10 rooms.')

# Local only: the actor-room evidence against the user's extracted OoT archive (never a ROM file).
archive = args.archive or os.environ.get('COMBO_OOT_ARCHIVE', '')
if not archive:
    for folder in (source.parent/'local-roms', source.parent.parent/'local-roms'):
        found = sorted(folder.rglob('oot.o2r')) if folder.is_dir() else []
        if found:
            archive = str(found[0])
            break
if archive and Path(archive).is_file():
    assert edges_tool.verify_archive(audit, archive) == []
    moved = copy.deepcopy(audit)
    moved['edges'][0]['actorRoom']['wallmasters'][0]['pos'] = [0, 0, 0]
    assert edges_tool.verify_archive(moved, archive)
    unclaimed = copy.deepcopy(audit)
    unclaimed['archive']['unloggedCaptures'].pop()
    assert any('not accounted for' in e for e in edges_tool.verify_archive(unclaimed, archive))
    print('Vanilla actor-room evidence verified against the extracted OoT archive.')
else:
    print('Actor-room evidence not re-read: no extracted OoT archive here (set COMBO_OOT_ARCHIVE); local only.')

reference = os.environ.get('COMBO_OOTMM_REFERENCE')
reference = Path(reference) if reference else next(
    (parent/'references/ootmm-stable-32.3' for parent in source.parents
     if (parent/'references/ootmm-stable-32.3/.git').exists()), None)
if reference is None:
    print('SKIPPED: set COMBO_OOTMM_REFERENCE to the exact local OoTMM v32.3 checkout to re-run the exporter')
    sys.exit(0)

blobs = contract.pinned_blobs(reference, edges_tool.pinned_items(audit))
assert edges_tool.verify_audit(audit, fixture, source, blobs) == []
assert rejected(lambda a: edge(a, 6)['evidence'][4].update(line=edge(a, 6)['evidence'][4]['line'] + 1), blobs)
assert rejected(lambda a: a['predicateAtoms']['has_lens']['pinned'][0].update(line=237), blobs)

# The MQ actor-room records keep the pinned area/room evidence C17 predicted them from; those citations must not
# drift silently. Exercise every recorded area-room citation without inventing any.
altered_room_citations = 0
for n in range(12, 24):
    for index in range(len(by_n[n]['actorRoom']['areaRoomEvidence'])):
        for field, value in [('line', 1), ('text', 'invalid area-room evidence'),
                             ('file', 'data/macros/macros_common.yml')]:
            problems = rejected(lambda a, n=n, index=index, field=field, value=value:
                                edge(a, n)['actorRoom']['areaRoomEvidence'][index].update({field: value}), blobs)
            assert any(f'edge {n:02d} ' in problem and 'actor-room area evidence' in problem
                       for problem in problems), f'Unchecked actor-room {field} citation for edge {n}'
            altered_room_citations += 1
assert altered_room_citations == 60
print('All 60 altered MQ actor-room area evidence citations rejected against pinned blobs.')


def run(output, check=False, ref=reference):
    command = [sys.executable, str(source/'tools/ootmm/export_oot_wallmaster_edges.py'), '--reference', str(ref),
               '--output-root', str(output)] + (['--check'] if check else [])
    return subprocess.run(command, capture_output=True, text=True, timeout=300,
                          env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))


result = run(source, check=True)
assert result.returncode == 0, result.stdout + result.stderr
with tempfile.TemporaryDirectory(prefix='oot-wallmaster-edges-', dir=work) as scratch:
    scratch = Path(scratch)
    result = run(scratch)
    assert result.returncode == 0, result.stdout + result.stderr
    assert (scratch/edges_tool.OUTPUT).read_bytes() == raw
    damaged = raw.replace(b'"soul_wallmaster"', b'"soul_wallmasters"', 1)
    (scratch/edges_tool.OUTPUT).write_bytes(damaged)
    assert run(scratch, check=True).returncode != 0
    assert (scratch/edges_tool.OUTPUT).read_bytes() == damaged  # --check never writes
    before = {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}
    rejected_run = run(scratch, ref=source)  # this repository is not the pinned upstream revision
    assert rejected_run.returncode != 0 and 'Expected exact OoTMM v32.3 revision' in rejected_run.stderr
    assert before == {p.relative_to(scratch): p.read_bytes() for p in scratch.rglob('*') if p.is_file()}
print('Exporter reproduces the edge fixture at the pin, rejects stale output read-only and refuses a wrong HEAD.')
