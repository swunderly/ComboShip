"""Export the OoT Wallmaster logic edges of v32.3 and check the reviewed anchor audit against them.

oot-wallmaster-edges.json is generated from exact Git blobs at the pin: the 13 OoT Wallmaster source
rows (registry, area presence per quest, the EnWallmas_GetDestEr room rule), the 23 world exits into
their areas (11 vanilla, 12 MQ) with each source area's own ingress, the 24 soul_wallmaster lines, and
the pinned macro and event definitions the edge expressions use. The extraction reuses the M1 contract
reader (export_m1_contract.py) and the runtime rule parser (export_oot_wallmaster_sources.py).

oot-wallmaster-anchor-audit.json is reviewed input, never generated. Every edge must have exactly one
entry with the pinned expression and source, a status (ready, needs-node, needs-event, false), and for
every status but false a native anchor region defined at its cited file:line, a native condition that
contains the native form of every atom the pinned expression keeps once the port's absent settings
are false, and evidence lines that still read as cited (pinned lines need the reference). With
--oot-archive the recorded actor-room evidence is also checked against the user's extracted OoT archive
(read-only; never a ROM file). --check reports drift without writing.
"""
from pathlib import Path
import argparse
import json
import re
import struct
import sys
import zipfile

import export_m1_contract as contract
import export_oot_wallmaster_sources as runtime_sources
from export_registry import DEFAULT_OUTPUT, PIN, publish, read_registry, verified_blob

REPO = Path(__file__).resolve().parents[2]
OUTPUT = 'tools/ootmm/oot-wallmaster-edges.json'
AUDIT = 'tools/ootmm/oot-wallmaster-anchor-audit.json'
MACROS_OOT = 'data/macros/macros_oot.yml'
MACROS_COMMON = 'data/macros/macros_common.yml'
STATUSES = ('ready', 'needs-node', 'needs-event', 'false')
QUEST_DIRS = {'vanilla': 'data/world/oot/dungeons/', 'mq': 'data/world/oot/dungeons_mq/'}
NATIVE_DIR = 'soh/soh/Enhancements/randomizer/location_access/dungeons/'
NATIVE_SCENES = {'Forest': 'SCENE_FOREST_TEMPLE', 'Shadow': 'SCENE_SHADOW_TEMPLE', 'Spirit': 'SCENE_SPIRIT_TEMPLE',
                 'BotW': 'SCENE_BOTTOM_OF_THE_WELL', 'GTG': 'SCENE_GERUDO_TRAINING_GROUND',
                 'Ganon': 'SCENE_INSIDE_GANONS_CASTLE'}
RUNTIME_SCENES = {'ForestTemple': 'Forest', 'ShadowTemple': 'Shadow', 'SpiritTemple': 'Spirit',
                  'BottomOfTheWell': 'BotW', 'GerudoTrainingGround': 'GTG', 'InsideGanonsCastle': 'Ganon'}
WALLMASTER_ACTOR = 0x0011
WALLMASTER_TYPES = {0: 'timer', 1: 'proximity', 2: 'flag', 3: 'shadowtag'}  # z_en_wallmas.h WMT_*


# --- Pinned expressions -------------------------------------------------------------------------

TOKEN = re.compile(r'\s*(\(|\)|&&|\|\||!|[A-Za-z_][A-Za-z0-9_]*(?:\([^()]*\))?)')


def parse_expression(text):
    """'a && (b || event(X))' into nested ('and'|'or'|'not', ...) tuples over atom strings; fails closed."""
    tokens, pos = [], 0
    while pos < len(text):
        match = TOKEN.match(text, pos)
        if not match:
            if text[pos:].strip():
                raise ValueError(f'Unsupported expression syntax: {text!r}')
            break
        tokens.append(match[1])
        pos = match.end()
    tokens.append(None)
    state = {'i': 0}

    def peek():
        return tokens[state['i']]

    def take():
        state['i'] += 1
        return tokens[state['i'] - 1]

    def parse_or():
        terms = [parse_and()]
        while peek() == '||':
            take()
            terms.append(parse_and())
        return terms[0] if len(terms) == 1 else ('or', *terms)

    def parse_and():
        terms = [parse_not()]
        while peek() == '&&':
            take()
            terms.append(parse_not())
        return terms[0] if len(terms) == 1 else ('and', *terms)

    def parse_not():
        if peek() == '!':
            take()
            return ('not', parse_not())
        token = take()
        if token == '(':
            inner = parse_or()
            if take() != ')':
                raise ValueError(f'Unbalanced expression: {text!r}')
            return inner
        if token is None or token in (')', '&&', '||'):
            raise ValueError(f'Unexpected token in {text!r}')
        return re.sub(r'\s+', '', token)

    tree = parse_or()
    if peek() is not None:
        raise ValueError(f'Trailing tokens in {text!r}')
    return tree


def atoms(tree):
    if isinstance(tree, str):
        return {tree}
    return set().union(*(atoms(t) for t in tree[1:]))


def reduce_false(tree, false_atoms):
    """Substitute the atoms the port treats as false and simplify; returns True, False or a tree."""
    if isinstance(tree, str):
        return False if tree in false_atoms else (True if tree == 'true' else tree)
    if tree[0] == 'not':
        inner = reduce_false(tree[1], false_atoms)
        return (not inner) if isinstance(inner, bool) else ('not', inner)
    parts = [reduce_false(t, false_atoms) for t in tree[1:]]
    absorbing, neutral = (False, True) if tree[0] == 'and' else (True, False)
    if absorbing in parts:
        return absorbing
    parts = [p for p in parts if p is not neutral]
    if not parts:
        return neutral
    return parts[0] if len(parts) == 1 else (tree[0], *parts)


def macro_table(blobs):
    """name -> (file, line, body) for the OoT and common macro files; a name defined twice fails."""
    table = {}
    for path in (MACROS_OOT, MACROS_COMMON):
        for number, line in enumerate(blobs[path].decode('utf-8').split('\n'), 1):
            match = re.match(r'"([A-Za-z_][A-Za-z0-9_]*)(\([^)]*\))?": "((?:[^"\\]|\\.)*)"', line)
            if match:
                if match[1] in table:
                    raise ValueError(f'Macro {match[1]} defined twice')
                table[match[1]] = {'file': path, 'line': number, 'name': match[1] + (match[2] or ''),
                                   'body': match[3]}
    return table


def atom_name(atom):
    return atom.split('(', 1)[0]


# --- Fixture --------------------------------------------------------------------------------------

def quest_world_areas(data, quest):
    """Area name -> parsed area (with events) for the OoT dungeon files of one quest."""
    section = 'oot' if quest == 'vanilla' else 'mq'
    result = {}
    for (sec, _), areas in data.parsed.items():
        if sec == section:
            for name, area in areas.items():
                if area['file'].startswith(QUEST_DIRS[quest]):
                    result[name] = area
    return result


def build(reference):
    verified_blob(reference, 'data/defs/entrances.yml')  # refuse any other HEAD before reading anything else
    world_files = contract.git_lines(reference, 'ls-tree', '-r', '--name-only', PIN, '--', 'data/world')
    paths = [contract.DATA_BUILD_TS, contract.BUILDER_TS, MACROS_OOT, MACROS_COMMON, runtime_sources.SOURCE,
             *world_files]
    blobs = contract.pinned_blobs(reference, paths)
    _, records = read_registry(reference)
    by_name = {r['name']: r for r in records}
    data = contract.WorldData(blobs, set(world_files))
    worlds = {'vanilla': data.world(())[0], 'mq': data.world(tuple(data.sets['mq']))[0]}
    quest_areas = {quest: quest_world_areas(data, quest) for quest in QUEST_DIRS}
    rules = runtime_sources.parse_source(blobs[runtime_sources.SOURCE], records)['rules']
    macros = macro_table(blobs)

    rows = sorted((r for r in records if r['type'] == 'wallmaster' and r['game'] == 'oot'), key=lambda r: r['id'])
    sources, edges = [], []
    for row in rows:
        quests = [q for q in QUEST_DIRS if row['from'] in quest_areas[q]]
        runtime = []
        for rule in rules:
            rooms = sorted(int(k) for k, v in rule['rooms'].items() if v == row['name'])
            if rooms or rule['default'] == row['name']:
                runtime.append({'scene': rule['scene'], 'dungeon': RUNTIME_SCENES[rule['context']],
                                'rooms': rooms, 'default': rule['default'] == row['name']})
        if len(runtime) != 1 or not quests:
            raise ValueError(f'{row["name"]} has no single runtime scene or no area')
        carried = [{'quest': q, 'file': quest_areas[q][row['from']]['file'], 'line': quest_areas[q][row['from']]['line'],
                    'exits': {k: v[0] for k, v in quest_areas[q][row['from']]['maps'].get('exits', {}).items()}}
                   for q in quests]
        if any(c['exits'] != {'VOID': 'true'} for c in carried):
            raise ValueError(f'{row["name"]} area is not a plain VOID buffer')
        sources.append({'name': row['name'], 'id': f'0x{row["id"]:x}', 'area': row['from'], 'to': row['to'],
                        'quests': quests, 'runtime': runtime[0], 'areaLines': carried})
        for quest in quests:
            for hit in contract.exits_into([worlds[quest]], row['from']):
                if hit['quest'] != quest:
                    raise ValueError(f'Edge into {row["from"]} outside its quest files')
                area = worlds[quest][hit['from']]
                tree = parse_expression(hit['expression'])
                ingress = contract.exits_into([worlds[quest]], hit['from'])
                edges.append({
                    'id': f'{hit["file"]}:{hit["line"]}', 'quest': quest, 'dungeon': runtime[0]['dungeon'],
                    'from': hit['from'], 'fromArea': {'file': area['file'], 'line': area['line'],
                                                     'region': area['region']},
                    'source': row['name'], 'expression': hit['expression'], 'atoms': sorted(atoms(tree)),
                    'ingress': [{'file': i['file'], 'line': i['line'], 'from': i['from'],
                                 'expression': i['expression']} for i in ingress]})
    edges.sort(key=lambda e: (e['id'].rsplit(':', 1)[0], int(e['id'].rsplit(':', 1)[1])))
    for number, edge in enumerate(edges, 1):
        edge['n'] = number
    edges = [{'n': e.pop('n'), **e} for e in edges]

    # Events an edge expression names, with the events their own expressions name, from the edge's area set.
    events, pending = {}, [(e['quest'], a) for e in edges for a in e['atoms'] if a.startswith('event(')]
    while pending:
        quest, atom = pending.pop()
        name = atom[len('event('):-1]
        if name in events:
            continue
        found = [(area_name, area) for area_name, area in quest_areas[quest].items()
                 if name in area['maps'].get('events', {})]
        if len(found) != 1:
            raise ValueError(f'Event {name} defined {len(found)} times')
        area_name, area = found[0]
        expression, line = area['maps']['events'][name]
        events[name] = {'quest': quest, 'area': area_name, 'file': area['file'], 'line': line,
                        'expression': expression}
        pending += [(quest, a) for a in atoms(parse_expression(expression)) if a.startswith('event(')]

    # Macro definitions of every atom, and one level below them.
    named = {atom_name(a) for e in edges for a in e['atoms']} | \
        {atom_name(a) for ev in events.values() for a in atoms(parse_expression(ev['expression']))}
    named = {n for n in named if n in macros}
    below = {w for n in named for w in re.findall(r'[A-Za-z_][A-Za-z0-9_]*', macros[n]['body']) if w in macros}
    macro_defs = {n: macros[n] for n in sorted(named | below)}

    soul_lines = []
    for line in contract.git_lines(reference, 'grep', '-n', '-w', 'soul_wallmaster', PIN, '--', 'data/world/oot'):
        _, file, number, _ = line.split(':', 3)
        soul_lines.append({'file': file, 'line': int(number), 'edge': f'{file}:{number}' in {e['id'] for e in edges}})

    counts = {'sources': len(sources), 'edges': len(edges),
              'vanillaEdges': sum(e['quest'] == 'vanilla' for e in edges),
              'mqEdges': sum(e['quest'] == 'mq' for e in edges), 'soulWallmasterLines': len(soul_lines)}
    if counts != {'sources': 13, 'edges': 23, 'vanillaEdges': 11, 'mqEdges': 12, 'soulWallmasterLines': 24}:
        raise ValueError(f'Unexpected v32.3 Wallmaster counts {counts}')
    if any(by_name[s['name']]['reverse'] for s in sources):
        raise ValueError('An OoT Wallmaster row has a reverse')
    cited = {e['id'].rsplit(':', 1)[0] for e in edges} | {i['file'] for e in edges for i in e['ingress']} | \
        {ev['file'] for ev in events.values()} | {a['file'] for s in sources for a in s['areaLines']}
    return {
        'upstreamCommit': PIN,
        'scope': 'OoT Wallmaster sources and the world exits into their areas, vanilla and MQ dungeon files',
        'inputs': {p: contract.sha(blobs[p]) for p in paths if not p.startswith('data/world/') or p in cited},
        'counts': counts,
        'sources': sources,
        'edges': edges,
        'events': {k: events[k] for k in sorted(events)},
        'macros': macro_defs,
        'soulWallmasterLines': soul_lines,
    }


# --- Reviewed audit -------------------------------------------------------------------------------

def squash(text):
    return re.sub(r'\s+', '', text)


class Checker:
    """Collects every audit problem so one run reports them all."""
    def __init__(self, repo, blobs):
        self.repo, self.blobs, self.errors, self.cache = Path(repo), blobs, [], {}

    def fail(self, message):
        self.errors.append(message)

    def lines(self, item):
        if item.get('pinned'):
            if self.blobs is None:
                return None
            data = self.blobs.get(item['file'])
            if data is None:
                self.fail(f'Pinned evidence file not read: {item["file"]}')
                return None
            return data.decode('utf-8').split('\n')
        if item['file'] not in self.cache:
            path = self.repo/item['file']
            self.cache[item['file']] = path.read_text(encoding='utf-8').splitlines() if path.is_file() else None
        return self.cache[item['file']]

    def evidence(self, item, where):
        if set(item) - {'file', 'line', 'text', 'pinned'} or not {'file', 'line', 'text'} <= set(item):
            self.fail(f'{where}: evidence needs exactly file, line, text (and pinned)')
            return
        lines = self.lines(item)
        if lines is None:
            if not item.get('pinned'):
                self.fail(f'{where}: missing file {item["file"]}')
            return
        if not 1 <= item['line'] <= len(lines) or not squash(lines[item['line'] - 1]).startswith(squash(item['text'])):
            self.fail(f'{where}: evidence moved or changed at {item["file"]}:{item["line"]}')


def pinned_items(audit):
    """Every pinned file an audit cites, so one batch read covers them."""
    found = set()

    def walk(value):
        if isinstance(value, dict):
            if value.get('pinned') and 'file' in value:
                found.add(value['file'])
            for v in value.values():
                walk(v)
        elif isinstance(value, list):
            for v in value:
                walk(v)
    walk(audit)
    return sorted(found)


def classify(fixture, dungeon, room):
    """The source row EnWallmas_GetDestEr picks for (dungeon scene, actor room), or None (UNREACHABLE)."""
    rules = [s for s in fixture['sources'] if s['runtime']['dungeon'] == dungeon]
    explicit = [s['name'] for s in rules if room in s['runtime']['rooms']]
    if explicit:
        return explicit[0]
    default = [s['name'] for s in rules if s['runtime']['default']]
    return default[0] if default else None


def verify_anchor(check, anchor, edge, where):
    if not {'region', 'file', 'line'} <= set(anchor):
        check.fail(f'{where}: an anchor needs region, file and line')
        return
    check.evidence({'file': anchor['file'], 'line': anchor['line'], 'text': f'areaTable[{anchor["region"]}] = Region('},
                   f'{where} anchor')
    if not anchor['file'].startswith(NATIVE_DIR):
        check.fail(f'{where}: anchor outside the native dungeon files')
    lines = check.lines({'file': anchor['file']}) or []
    if 0 < anchor['line'] <= len(lines) and NATIVE_SCENES[edge['dungeon']] not in lines[anchor['line'] - 1]:
        check.fail(f'{where}: anchor is not a {edge["dungeon"]} region')
    if ('_MQ_' in anchor['region']) != (edge['quest'] == 'mq'):
        check.fail(f'{where}: anchor quest does not match the edge quest')


def verify_audit(audit, fixture, repo, blobs=None):
    """Return the list of problems; blobs (pinned path -> bytes) enables the pinned evidence lines."""
    check = Checker(repo, blobs)
    if audit.get('upstreamCommit') != PIN:
        check.fail('Audit was reviewed against another reference')
    atoms_table = audit.get('predicateAtoms', {})
    false_atoms = {name for name, entry in atoms_table.items() if entry.get('native') == 'false'}
    for name, entry in atoms_table.items():
        if not entry.get('native') or not entry.get('pinned'):
            check.fail(f'Atom {name}: needs native and pinned')
        for item in entry.get('pinned', []) + entry.get('evidence', []):
            check.evidence(item, f'atom {name}')
        if entry.get('native') == 'false':
            for absent in entry.get('absent', []):
                text = (check.repo/absent['file']).read_text(encoding='utf-8')
                if any(token in text for token in absent['tokens']):
                    check.fail(f'Atom {name}: {absent["file"]} now has {absent["tokens"]}')
            if not entry.get('absent'):
                check.fail(f'Atom {name}: a false atom must show where the setting is absent')
    for fact in audit.get('actorFacts', []):
        for item in fact.get('evidence', []):
            check.evidence(item, f'actor fact {fact.get("fact", "?")[:40]}')

    entries = {}
    for entry in audit.get('edges', []):
        if entry.get('id') in entries:
            check.fail(f'Duplicate audit entry {entry.get("id")}')
        entries[entry.get('id')] = entry
    edges = {e['id']: e for e in fixture['edges']}
    if set(entries) != set(edges):
        check.fail(f'Audit edges differ from the fixture: missing {sorted(set(edges) - set(entries))}, '
                   f'extra {sorted(set(entries) - set(edges))}')
    blocked = {edge for blocker in audit.get('openBlockers', []) for edge in blocker.get('edges', [])}
    verified_quests = audit.get('archive', {}).get('verifiedQuests', [])
    for edge_id in sorted(set(entries) & set(edges), key=lambda i: edges[i]['n']):
        entry, edge, where = entries[edge_id], edges[edge_id], f'edge {edges[edge_id]["n"]:02d} {edge_id}'
        for key in ('n', 'quest', 'source', 'from', 'expression'):
            if entry.get(key) != edge[key]:
                check.fail(f'{where}: {key} {entry.get(key)!r} is not the pinned {edge[key]!r}')
        status = entry.get('status')
        if status not in STATUSES:
            check.fail(f'{where}: unknown status {status!r}')
            continue
        reduced = reduce_false(parse_expression(edge['expression']), false_atoms)
        if (status == 'false') != (reduced is False):
            check.fail(f'{where}: status {status} but the expression reduces to {reduced!r} with the port\'s '
                       'absent settings false')
        unknown = [a for a in edge['atoms'] if a not in atoms_table]
        if unknown:
            check.fail(f'{where}: atoms without a native translation {unknown}')
        if not entry.get('evidence') or not entry.get('note'):
            check.fail(f'{where}: every status needs evidence and a note')
        for item in entry.get('evidence', []):
            check.evidence(item, where)
        if entry.get('anchor'):
            verify_anchor(check, entry['anchor'], edge, where)
        elif status != 'false':
            check.fail(f'{where}: a {status} edge needs a native anchor')
        if status == 'false':
            continue
        if (entry.get('ingress') or {}).get('relation') not in audit.get('relations', {}):
            check.fail(f'{where}: the ingress comparison needs a relation from the relations list')
        condition = entry.get('condition') or {}
        native = condition.get('native', '')
        kept = sorted(atoms(reduced)) if not isinstance(reduced, bool) else []
        if sorted(condition.get('pinnedAtoms', [])) != kept:
            check.fail(f'{where}: pinnedAtoms {condition.get("pinnedAtoms")} are not the kept atoms {kept}')
        for atom in kept:
            translation = atoms_table.get(atom, {}).get('native')
            if translation and translation not in native:
                check.fail(f'{where}: condition lacks the native form of {atom}: {translation}')
        for extra in condition.get('ingressAtoms', []) + condition.get('portAtoms', []):
            if extra.get('native', '\0') not in native or not extra.get('evidence'):
                check.fail(f'{where}: extra atom {extra.get("native")!r} is not in the condition or has no evidence')
            for item in extra.get('evidence', []):
                check.evidence(item, f'{where} extra atom')
        if status in ('needs-node', 'needs-event'):
            proposal = entry.get('proposal') or {}
            if not proposal.get('copiedFrom') or not proposal.get('add'):
                check.fail(f'{where}: a {status} edge needs a proposal with copiedFrom and add')
            for item in proposal.get('copiedFrom', []):
                check.evidence(item, f'{where} proposal')
    for edge_id in sorted(set(entries) & set(edges), key=lambda i: edges[i]['n']):
        entry, edge, where = entries[edge_id], edges[edge_id], f'edge {edges[edge_id]["n"]:02d} {edge_id}'
        room = entry.get('actorRoom') or {}
        if room.get('status') == 'verified':
            if room.get('dungeon') != edge['dungeon'] or edge['quest'] not in verified_quests:
                check.fail(f'{where}: actor-room evidence names another dungeon or a quest without an archive')
            captures = [wm for wm in room.get('wallmasters', []) if wm.get('role') == 'capture']
            if not captures:
                check.fail(f'{where}: verified actor-room evidence names no capturing Wallmaster')
            for wm in captures:
                if classify(fixture, edge['dungeon'], wm['room']) != edge['source']:
                    check.fail(f'{where}: room {wm["room"]} does not classify as {edge["source"]} at runtime')
        elif room.get('status') == 'pending':
            if edge_id not in blocked or not room.get('expectedRooms'):
                check.fail(f'{where}: pending actor-room evidence needs expectedRooms and an openBlockers entry')
            for expected_room in room.get('expectedRooms', []):
                if classify(fixture, edge['dungeon'], expected_room) != edge['source']:
                    check.fail(f'{where}: expected room {expected_room} does not classify as {edge["source"]}')
        else:
            check.fail(f'{where}: actorRoom status must be verified or pending')
    for extra in audit.get('archive', {}).get('unloggedCaptures', []):
        if classify(fixture, extra['dungeon'], extra['wallmaster']['room']) != extra.get('classifiedAs'):
            check.fail(f'Unlogged capture in {extra["dungeon"]} room {extra["wallmaster"]["room"]} is not '
                       f'classified as {extra.get("classifiedAs")}')
    work = audit.get('workList', {})
    expected = {'M1-13': sorted(i for i, e in entries.items() if e.get('status') in ('needs-node', 'needs-event')),
                'M1-14': sorted(i for i, e in entries.items() if e.get('status') == 'ready')}
    for unit, ids in expected.items():
        if sorted(work.get(unit, [])) != ids:
            check.fail(f'workList {unit} is not the {unit} edge set')
    return check.errors


# --- Extracted OoT archive (local, read-only) -----------------------------------------------------

class ResourceReader:
    """A little-endian LUS resource body after its 0x40-byte header."""
    def __init__(self, data):
        self.data, self.at = data, 0x40

    def take(self, fmt):
        value = struct.unpack_from('<' + fmt, self.data, self.at)
        self.at += struct.calcsize('<' + fmt)
        return value[0] if len(value) == 1 else value

    def string(self):
        size = self.take('i')
        self.at += size
        return self.data[self.at - size:self.at].decode('utf-8')


def scene_commands(data):
    """Binary scene/room commands (soh/soh/resource/importer/scenecommand); unknown ones fail closed."""
    reader = ResourceReader(data)
    actors, transitions = [], []
    for _ in range(reader.take('I')):
        command = reader.take('i')
        if command in (0x00, 0x01):  # SetStartPositionList, SetActorList
            entries = [reader.take('H3h3hH') for _ in range(reader.take('I'))]
            if command == 0x01:
                actors += [{'actor': e[0], 'pos': list(e[1:4]), 'params': e[7]} for e in entries]
        elif command in (0x02, 0x19):  # SetCsCamera, SetCameraSettings
            reader.take('bi')
        elif command in (0x03, 0x17):  # SetCollisionHeader, SetCutscenes
            reader.string()
        elif command == 0x04:  # SetRoomList
            for _ in range(reader.take('i')):
                reader.string()
                reader.take('ii')
        elif command == 0x06:  # SetEntranceList
            reader.take(f'{2 * reader.take("I")}b')
        elif command == 0x07:  # SetSpecialObjects
            reader.take('bh')
        elif command == 0x0A:  # SetMesh
            reader.take('b')
            kind = reader.take('b')
            for _ in range(1 if kind == 1 else reader.take('b')):
                if kind == 0:
                    reader.take('b')
                elif kind == 2:
                    reader.take('b4h')
                elif kind == 1:
                    reader.take('B')
                    reader.string()
                    reader.string()
                    for _ in range(reader.take('I')):
                        reader.take('HB')
                        reader.string()
                        reader.take('IIHHBBHH')
                    reader.take('b')
                else:
                    raise ValueError(f'Unknown mesh type {kind}')
                reader.string()
                reader.string()
        elif command in (0x0B, 0x13):  # SetObjectList, SetExitList
            reader.take(f'{reader.take("I")}H')
        elif command == 0x0C:  # SetLightList
            for _ in range(reader.take('I')):
                reader.take('B3h4Bh')
        elif command == 0x0D:  # SetPathways
            for _ in range(reader.take('I')):
                reader.string()
        elif command == 0x0E:  # SetTransitionActorList
            for _ in range(reader.take('I')):
                front, _, back, _, actor, x, y, z, _, params = reader.take('BBBBh3hhH')
                transitions.append({'rooms': [front, back], 'actor': actor, 'pos': [x, y, z], 'params': params})
        elif command == 0x0F:  # SetLightingSettings
            for _ in range(reader.take('i')):
                reader.take('18bhH')
        elif command == 0x18:  # SetAlternateHeaders
            for _ in range(reader.take('I')):
                if reader.string():
                    raise ValueError('Alternate headers are not modelled')
        elif command in (0x05, 0x11):  # SetWind, SetSkyboxSettings
            reader.take('4b')
        elif command in (0x10, 0x15):  # SetTimeSettings, SetSoundSettings
            reader.take('3b')
        elif command == 0x12:  # SetSkyboxModifier
            reader.take('2b')
        elif command == 0x08:  # SetRoomBehavior
            reader.take('bi')
        elif command == 0x16:  # SetEchoSettings
            reader.take('b')
        elif command != 0x14:  # EndMarker
            raise ValueError(f'Unknown scene command 0x{command:x}')
    if reader.at != len(data):
        raise ValueError('Trailing bytes after the scene commands')
    return actors, transitions


def read_archive_scene(archive, quest, scene):
    """Room number -> actor list, and the scene's transition actors, from an opened archive."""
    folder = f'scenes/{"mq" if quest == "mq" else "nonmq"}/{scene}_scene/'
    rooms = {}
    for name in archive.namelist():
        match = re.fullmatch(re.escape(folder + scene) + r'_room_(\d+)', name)
        if match:
            rooms[int(match[1])] = scene_commands(archive.read(name))[0]
    if not rooms:
        raise ValueError(f'No rooms for {folder}')
    return rooms, scene_commands(archive.read(folder + scene + '_scene'))[1]


def hexint(value):
    return int(value, 16) if isinstance(value, str) else value


def verify_archive(audit, archive_path, quest='vanilla'):
    """Problems with the audit's actor-room claims against an extracted archive (opened read-only)."""
    errors = []
    scenes = audit['archive']['scenes']
    with zipfile.ZipFile(archive_path) as archive:
        loaded = {dungeon: read_archive_scene(archive, quest, scene) for dungeon, scene in scenes.items()}
    claimed = set()

    def actor_matches(dungeon, item, where):
        rooms, _ = loaded[dungeon]
        actors = rooms.get(item['room'], [])
        if not 0 <= item['index'] < len(actors):
            errors.append(f'{where}: no actor {item["index"]} in room {item["room"]}')
            return
        actual = actors[item['index']]
        if actual['actor'] != hexint(item['actor']) or actual['params'] != hexint(item['params']) or \
                ('pos' in item and actual['pos'] != item['pos']):
            errors.append(f'{where}: room {item["room"]} actor {item["index"]} is {actual}, not as recorded')

    def door_matches(dungeon, item, where):
        _, transitions = loaded[dungeon]
        if not any(sorted(t['rooms']) == sorted(item['rooms']) and t['actor'] == hexint(item['actor']) and
                   t['pos'] == item['pos'] for t in transitions):
            errors.append(f'{where}: no transition actor {item}')

    def walk(dungeon, wallmasters, supporting, where):
        for wm in wallmasters:
            actor_matches(dungeon, {**wm, 'actor': WALLMASTER_ACTOR}, where)
            claimed.add((dungeon, wm['room'], wm['index']))
            actual = loaded[dungeon][0].get(wm['room'], [])
            if wm['index'] < len(actual):
                params = actual[wm['index']]['params']
                if WALLMASTER_TYPES.get(params & 0xFF) != wm.get('type'):
                    errors.append(f'{where}: Wallmaster type {params & 0xFF} is not {wm.get("type")}')
        for item in supporting:
            (door_matches if 'rooms' in item else actor_matches)(dungeon, item, where)

    for entry in audit['edges']:
        room = entry.get('actorRoom', {})
        if entry['quest'] == quest and room.get('status') == 'verified':
            walk(room['dungeon'], room['wallmasters'], room.get('supporting', []), f'edge {entry["n"]:02d}')
    for extra in audit['archive'].get('unloggedCaptures', []):
        if extra['quest'] == quest:
            walk(extra['dungeon'], [extra['wallmaster']], extra.get('supporting', []), 'unlogged capture')
    present = {(dungeon, room, index) for dungeon, (rooms, _) in loaded.items() for room, actors in rooms.items()
               for index, actor in enumerate(actors) if actor['actor'] == WALLMASTER_ACTOR}
    if present != claimed:
        errors.append(f'Archive Wallmasters not accounted for: {sorted(present - claimed)}; '
                      f'claimed but absent: {sorted(claimed - present)}')
    return errors


# --- Entry point ----------------------------------------------------------------------------------

def fixture_bytes(value):
    return contract.contract_bytes(value)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--reference', type=Path, required=True, help='Local checkout at exact OoTMM v32.3 commit')
    parser.add_argument('--output-root', type=Path, default=DEFAULT_OUTPUT, help='Destination repository root')
    parser.add_argument('--check', action='store_true', help='Compare expected bytes; never modify output')
    parser.add_argument('--oot-archive', type=Path, help='Extracted OoT archive (oot.o2r) to check actor rooms')
    args = parser.parse_args()
    fixture = build(args.reference)
    # The reviewed audit and the sources it cites are this checkout's; --output-root only receives the fixture.
    audit = json.loads((REPO/AUDIT).read_text(encoding='utf-8'))
    errors = verify_audit(audit, fixture, REPO, contract.pinned_blobs(args.reference, pinned_items(audit)))
    if args.oot_archive:
        errors += verify_archive(audit, args.oot_archive)
    if errors:
        sys.exit('Anchor audit does not hold:\n  ' + '\n  '.join(errors))
    publish(args.output_root, {OUTPUT: fixture_bytes(fixture)}, args.check)
    statuses = {s: sum(e['status'] == s for e in audit['edges']) for s in STATUSES}
    print(f'{"Verified" if args.check else "Exported"} {fixture["counts"]["edges"]} OoT Wallmaster edges and '
          f'{fixture["counts"]["sources"]} sources; audit {statuses}'
          f'{"; archive actor rooms verified" if args.oot_archive else ""}')


if __name__ == '__main__':
    main()
