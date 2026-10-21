"""Generate the OoT native edge bindings of the opposite-age Temple probe from the reviewed audit.

Inputs: the exact v32.3 registry blob, tools/ootmm/oot-edge-audit.json, and this checkout's Ship
sources (entranceShuffleTable, entrance_table.h, location_access regions, OotNativeArrival.h).
Every audit claim is re-verified: the registry row and role, a unique native exit from the parent to
the target, the Ship table row and entrance index, the reverse pairing, and each file:line of
evidence. A GLOBAL row (limits design, Stage 2) binds one warp song of Root Exits: its native exit
is logic->CanUse of the song, its warp region's only exit is the landing, and the Ship table row of
that landing carries the row's id. --check reports drift without writing.
"""
from pathlib import Path
import hashlib
import json
import re
from export_registry import PIN, arguments, publish, read_registry

REPO = Path(__file__).resolve().parents[2]
AUDIT = 'tools/ootmm/oot-edge-audit.json'
RANDOMIZER = 'soh/soh/Enhancements/randomizer'
INC = RANDOMIZER + '/combo_oot_edge_bindings.inc'
NAMES = 'combo/rando/entrances/OotEdgeBindingNames.h'
ROLE_TYPES = {'candidate': {'one-way', 'one-way-owl'}, 'rootPolicy': {'spawn-child', 'spawn-adult'},
              'global': {'one-way-song'}}
# The OoTMM song area each Ship warp song leaves GLOBAL for (oot _system.yml:27-32).
GLOBAL_SONGS = {'RG_MINUET_OF_FOREST': 'FOREST', 'RG_BOLERO_OF_FIRE': 'FIRE', 'RG_SERENADE_OF_WATER': 'WATER',
                'RG_NOCTURNE_OF_SHADOW': 'SHADOW', 'RG_REQUIEM_OF_SPIRIT': 'SPIRIT', 'RG_PRELUDE_OF_LIGHT': 'LIGHT'}
ROOT_CONDITION = {'spawn-child': 'logic->IsChild', 'spawn-adult': 'logic->IsAdult'}


def squash(text):
    return re.sub(r'\s+', '', text)


def read(relative):
    return (REPO/relative).read_text(encoding='utf-8')


def shuffle_table():
    text = read(RANDOMIZER + '/entrance.cpp')
    start = text.index('std::vector<EntranceInfoPair> entranceShuffleTable = {')
    text = text[start:text.index('// clang-format on', start)]
    link = r'\{\s*EntranceType::(\w+),\s*(RR_\w+),\s*(RR_\w+),\s*(ENTR_\w+)\s*\}'
    rows = []
    for m in re.finditer(r'\{\s*' + link + r'\s*,\s*(NO_RETURN_ENTRANCE|' + link + r')\s*\}', text):
        rows.append({'type': m.group(1), 'parent': m.group(2), 'target': m.group(3), 'entrance': m.group(4),
                     'reverse': None if m.group(5) == 'NO_RETURN_ENTRANCE' else m.group(9)})
    if len(rows) < 100:
        raise ValueError('Ship entranceShuffleTable was not parsed')
    return rows


def entrance_indices():
    indices = {}
    for line in read('soh/include/tables/entrance_table.h').splitlines():
        m = re.match(r'/\* (0x[0-9A-F]+) \*/ DEFINE_ENTRANCE\((ENTR_\w+),', line)
        if m:
            if int(m.group(1), 16) != len(indices) or m.group(2) in indices:
                raise ValueError('Entrance table comment does not match its ordinal: ' + line)
            indices[m.group(2)] = len(indices)
    return indices


def region_exits():
    """Map each region to its ENTRANCE(...) list (target, condition) across the location_access sources."""
    regions = {}
    for path in sorted((REPO/RANDOMIZER/'location_access').rglob('*.cpp')):
        text = path.read_text(encoding='utf-8')
        starts = [m for m in re.finditer(r'areaTable\[(RR_\w+)\]\s*=\s*Region\(', text)]
        for i, m in enumerate(starts):
            end = starts[i + 1].start() if i + 1 < len(starts) else len(text)
            if m.group(1) in regions:
                raise ValueError('Region defined twice: ' + m.group(1))
            block = text[m.start():end]
            exits = []
            for e in re.finditer(r'ENTRANCE\((RR_\w+),', block):
                depth, j = 1, e.end()
                while depth:
                    depth += (block[j] == '(') - (block[j] == ')')
                    j += 1
                args = block[e.end():j - 1]
                exits.append((e.group(1), args.rsplit(', false', 1)[0].strip() if args.endswith(', false') else args.strip()))
            regions[m.group(1)] = exits
    return regions


def arrival_aliases():
    text = read('combo/rando/entrances/OotNativeArrival.h')
    return {int(a, 16): int(b, 16) for a, b in
            re.findall(r'case (0x[0-9a-f]+):\s*return OotNativeArrival\{ (0x[0-9a-f]+) \};', text)}


def verify(records, original):
    audit = json.loads(read(AUDIT))
    registry_hash = hashlib.sha256(original).hexdigest()
    if audit['upstreamCommit'] != PIN or audit['registrySha256'] != registry_hash:
        raise ValueError('Audit was reviewed against another reference or registry')
    if f'"{registry_hash}"' not in read('combo/rando/entrances/Registry.h'):
        raise ValueError('Registry.h does not carry the audited registry hash')
    rows = {r['name']: r for r in records}
    table, indices, regions, aliases = shuffle_table(), entrance_indices(), region_exits(), arrival_aliases()
    root_exits = regions['RR_ROOT_EXITS']
    bindings = []
    for row in audit['rows']:
        record = rows[row['name']]
        if record['game'] != 'oot' or record['type'] not in ROLE_TYPES[row['role']]:
            raise ValueError('Registry row is not a tranche-1 ' + row['role'] + ': ' + row['name'])
        exits = [c for t, c in regions[row['parent']] if t == row['target']]
        if len(exits) != 1:
            raise ValueError(f'Expected one native exit {row["parent"]} -> {row["target"]}, found {len(exits)}')
        # A GLOBAL row's Ship table row is its warp region's landing; the Root Exits edge itself is not shuffled.
        shuffled = (row['target'], row['landing']) if row['role'] == 'global' else (row['parent'], row['target'])
        if row['role'] == 'global':
            song = row['song']
            if (row['parent'] != 'RR_ROOT_EXITS' or exits != [f'logic->CanUse({song})'] or
                    record['from'] != f'OOT SONG_TP_{GLOBAL_SONGS[song]}'):
                raise ValueError('Root Exits warp song does not match its OoTMM song area: ' + row['name'])
            if regions[row['target']] != [(row['landing'], 'true')]:
                raise ValueError('Warp region does not lead only to its landing: ' + row['name'])
        matches = [t for t in table if (t['parent'], t['target']) == shuffled]
        if len(matches) != 1 or matches[0]['entrance'] != row['nativeEntrance']:
            raise ValueError('Ship entranceShuffleTable disagrees for ' + row['name'])
        # Tranche 1 binds only rows without a pinned reverse; Ship must agree that none exists.
        if record['reverse'] is not None or matches[0]['reverse'] is not None:
            raise ValueError('Reverse pairing is outside tranche 1: ' + row['name'])
        index = indices[row['nativeEntrance']]
        if row['indexRule'] == 'equal':
            if index != record['id']:
                raise ValueError('Native index differs from the logical id: ' + row['name'])
        elif row['indexRule'] == 'shipwright-repurposed':
            if aliases.get(record['id']) != int(row['alias'], 16) or index == record['id']:
                raise ValueError('Alias evidence does not hold for ' + row['name'])
        else:
            raise ValueError('Unknown index rule for ' + row['name'])
        if row['role'] == 'rootPolicy':
            spawn = [c for t, c in root_exits if t == row['parent']]
            if spawn != [ROOT_CONDITION[record['type']]]:
                raise ValueError('Root Exits spawn condition is not a bare age check for ' + row['name'])
        for item in row['evidence']:
            lines = read(item['file']).splitlines()
            if not squash(lines[item['line'] - 1]).startswith(squash(item['text'])):
                raise ValueError(f'Evidence moved or changed: {item["file"]}:{item["line"]}')
        bindings.append({**row, 'id': record['id']})
    if len({b['name'] for b in bindings}) != len(bindings):
        raise ValueError('Duplicate audit row')
    if sorted(GLOBAL_SONGS) != sorted(b['song'] for b in bindings if b['role'] == 'global'):
        raise ValueError('GLOBAL needs exactly the six warp songs of Root Exits')
    return audit, sorted(bindings, key=lambda b: (b['role'] != 'candidate', b['name']))


def outputs(audit, bindings):
    header = ['// Generated by tools/ootmm/export_oot_bindings.py from tools/ootmm/oot-edge-audit.json; do not hand-edit.',
              f'// OoTMM {PIN},', f'// registry {audit["registrySha256"]}.']
    role = {'candidate': 'Candidate', 'rootPolicy': 'RootPolicy', 'global': 'Global'}
    inc = header + ['// Row, logical id, role, native parent -> target region, Ship entrance index.']
    for b in bindings:
        inc.append(f'{{ "{b["name"]}", 0x{b["id"]:x}u, TempleBindingRole::{role[b["role"]]}, {b["parent"]}, '
                   f'{b["target"]}, {b["nativeEntrance"]} }},')
    candidates = [b['name'] for b in bindings if b['role'] == 'candidate']
    roots = [b['name'] for b in bindings if b['role'] == 'rootPolicy']
    songs = [b['name'] for b in bindings if b['role'] == 'global']
    names = header + ['#pragma once', '#include <array>', '#include <string_view>', '',
                      'namespace ComboRando::Entrances {',
                      'inline constexpr std::string_view kOotEdgeBindingRegistry =',
                      f'    "{audit["registrySha256"]}";',
                      '// OoT rows the native Temple probe can delete or add as candidate edges (tranche 1).',
                      f'inline constexpr std::array<std::string_view, {len(candidates)}> kOotTempleCandidateRows = {{',
                      *[f'    "{n}",' for n in candidates], '};',
                      '// Rows bound only for the probe\'s root policy (the opposite-age spawn), never candidates.',
                      f'inline constexpr std::array<std::string_view, {len(roots)}> kOotTempleRootPolicyRows = {{',
                      *[f'    "{n}",' for n in roots], '};',
                      "// Root Exits' warp songs, the edges of the probe's virtual GLOBAL node (limits design, Stage 2).",
                      f'inline constexpr std::array<std::string_view, {len(songs)}> kOotTempleGlobalRows = {{',
                      *[f'    "{n}",' for n in songs], '};', '} // namespace ComboRando::Entrances']
    return {INC: ('\n'.join(inc) + '\n').encode('utf-8'), NAMES: ('\n'.join(names) + '\n').encode('utf-8')}


def main():
    args = arguments(__doc__)
    original, records = read_registry(args.reference)
    audit, bindings = verify(records, original)
    publish(args.output_root, outputs(audit, bindings), args.check)
    print(f'Verified {len(bindings)} OoT edge bindings against the v32.3 registry and Ship sources')


if __name__ == '__main__':
    main()
