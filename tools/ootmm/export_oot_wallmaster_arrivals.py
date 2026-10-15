"""Export only the 11 native OoT Wallmaster arrival bindings (M1-14 prerequisite).

The exact v32.3 dungeon/dungeon-minor registry IDs identify native entrance-table entries directly.
Each must have one Dungeon shuffle-table pair, a matching reverse ID, and one defined ENTRYWAY in
the entrance's scene. This is arrival identity metadata, not a source-edge/actor audit or capability.
The separate opposite-age Temple binding contract is neither extended nor overwritten.
"""
from pathlib import Path
import hashlib
import json
import re

import export_oot_bindings as native
from export_registry import PIN, REGISTRY_SOURCE, arguments, publish, read_registry

REPO = Path(__file__).resolve().parents[2]
JSON = 'tools/ootmm/oot-wallmaster-arrivals.json'
INC = native.RANDOMIZER + '/combo_oot_wallmaster_arrivals.inc'
NAMES = 'combo/rando/entrances/OotWallmasterArrivalNames.h'
TABLE = 'soh/include/tables/entrance_table.h'
REGIONS = native.RANDOMIZER + '/randomizerEnums/RandomizerRegion.h'
DESTINATION_TYPES = {'dungeon', 'dungeon-minor', 'dungeon-sh'}


def read(path):
    return (REPO/path).read_text(encoding='utf-8')


def evidence(path, number, line):
    return {'file': path, 'line': number, 'text': line.strip()}


def entrance_scenes():
    result = {}
    for number, line in enumerate(read(TABLE).splitlines(), 1):
        match = re.match(r'/\* (0x[0-9A-F]+) \*/ DEFINE_ENTRANCE\((ENTR_\w+), (SCENE_\w+), (\d+),', line)
        if match:
            if match[2] in result:
                raise ValueError('Duplicate native entrance: ' + match[2])
            result[match[2]] = {'index': int(match[1], 16), 'scene': match[3], 'spawn': int(match[4]),
                                'evidence': evidence(TABLE, number, line)}
    return result


def region_definitions():
    result = {}
    for path in sorted((REPO/native.RANDOMIZER/'location_access').rglob('*.cpp')):
        relative = path.relative_to(REPO).as_posix()
        for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
            match = re.search(r'areaTable\[(RR_\w+)\]\s*=\s*Region\("[^"]*",\s*(SCENE_\w+),', line)
            if match:
                if match[1] in result:
                    raise ValueError('Duplicate native region: ' + match[1])
                result[match[1]] = {'scene': match[2], 'evidence': evidence(relative, number, line)}
    return result


def bind(records, indices, table, scenes, regions, aliases):
    """Fail closed on ambiguous arrival identity; do not infer a source predicate or MQ actor room."""
    destinations = [r for r in records if r['game'] == 'oot' and r['type'] in DESTINATION_TYPES]
    if len(destinations) != 11 or sum(r['type'] == 'dungeon' for r in destinations) != 8 or \
            sum(r['type'] == 'dungeon-minor' for r in destinations) != 3:
        raise ValueError('Expected exactly eight OoT dungeon and three minor-dungeon arrivals')
    by_name = {r['name']: r for r in records}
    bindings = []
    for row in sorted(destinations, key=lambda r: r['name']):
        matches = [t for t in table if indices.get(t['entrance']) == row['id']]
        if len(matches) != 1 or matches[0]['type'] != 'Dungeon':
            raise ValueError('Expected one native Dungeon arrival for ' + row['name'])
        match = matches[0]
        if row['id'] in aliases:
            raise ValueError('Aliased arrival requires a separate review: ' + row['name'])
        if not row['reverse'] or not match['reverse'] or row['reverse'] not in by_name or \
                indices.get(match['reverse']) != by_name[row['reverse']]['id']:
            raise ValueError('Native reverse index differs from the pinned reverse: ' + row['name'])
        if by_name[row['reverse']]['reverse'] != row['name']:
            raise ValueError('Pinned reverse does not return to ' + row['name'])
        region = match['target']
        if region not in regions or not region.endswith('_ENTRYWAY') or \
                '/location_access/dungeons/' not in regions[region]['evidence']['file']:
            raise ValueError('Expected a defined dungeon entryway: ' + row['name'])
        arrival = scenes.get(match['entrance'])
        if not arrival or arrival['index'] != row['id'] or arrival['scene'] != regions[region]['scene']:
            raise ValueError('Native entrance index/scene and arrival region disagree: ' + row['name'])
        bindings.append({'name': row['name'], 'logicalId': row['id'], 'type': row['type'], 'area': row['to'],
                         'registryLine': row['sourceLine'], 'nativeEntrance': match['entrance'],
                         'nativeIndex': arrival['index'], 'arrivalRegion': region, 'nativeScene': arrival['scene'],
                         'spawn': arrival['spawn'], 'nativeParent': match['parent'],
                         'reverse': row['reverse'], 'nativeReverse': match['reverse'],
                         'evidence': [arrival['evidence'], regions[region]['evidence']]})
    for field in ('name', 'logicalId', 'nativeIndex', 'arrivalRegion', 'nativeScene'):
        if len({b[field] for b in bindings}) != len(bindings):
            raise ValueError('Arrival catalog has duplicate ' + field)
    return bindings


def build(original, records):
    digest = hashlib.sha256(original).hexdigest()
    if f'"{digest}"' not in read('combo/rando/entrances/Registry.h') or \
            f'"{PIN}"' not in read('combo/rando/entrances/Registry.h'):
        raise ValueError('Registry.h differs from the exact pinned registry identity')
    indices, table = native.entrance_indices(), native.shuffle_table()
    rows = bind(records, indices, table, entrance_scenes(), region_definitions(), native.arrival_aliases())
    enum_text = read(REGIONS)
    shuffle_path = native.RANDOMIZER + '/entrance.cpp'
    shuffle_lines = read(shuffle_path).splitlines()
    for row in rows:
        if len(re.findall(r'RANDO_ENUM_ITEM\(' + row['arrivalRegion'] + r'\)', enum_text)) != 1:
            raise ValueError('Arrival region missing or repeated in native enum: ' + row['name'])
        matching = [(n, line) for n, line in enumerate(shuffle_lines, 1)
                    if re.search(r'\b' + row['nativeEntrance'] + r'\b', line) and
                    'EntranceType::Dungeon' in line]
        if len(matching) != 1:
            raise ValueError('Ambiguous native shuffle-table evidence: ' + row['name'])
        row['evidence'].append(evidence(shuffle_path, *matching[0]))
    inputs = {TABLE, REGIONS, shuffle_path, 'combo/rando/entrances/OotNativeArrival.h',
              *(item['file'] for row in rows for item in row['evidence'])}
    return {'upstreamCommit': PIN, 'registrySource': REGISTRY_SOURCE, 'registrySha256': digest,
            'scope': 'Only the 11 OoT dungeon/minor-dungeon Wallmaster destination identities. No source, actor, '
                     'runtime installation, Temple capability or MQ actor evidence is certified.',
            'nativeSourceSha256': {p: hashlib.sha256(read(p).encode()).hexdigest() for p in sorted(inputs)},
            'rows': rows}


def outputs(data):
    banner = ['// Generated by tools/ootmm/export_oot_wallmaster_arrivals.py; do not hand-edit.',
              f'// OoTMM {PIN}; registry {data["registrySha256"]}.',
              '// Arrival identity only; does not admit a layout, source edge or capability.']
    inc = banner + ['// Row, logical id, native entrance index, native arrival region.']
    for row in data['rows']:
        inc.append(f'{{ "{row["name"]}", 0x{row["logicalId"]:x}u, {row["nativeEntrance"]}, '
                   f'{row["arrivalRegion"]} }},')
    names = banner + ['#pragma once', '#include <array>', '#include <string_view>', '',
                     'namespace ComboRando::Entrances {',
                     'inline constexpr std::string_view kOotWallmasterArrivalRegistry =',
                     f'    "{data["registrySha256"]}";',
                     'inline constexpr std::array<std::string_view, 11> kOotWallmasterArrivalRows = {',
                     *[f'    "{row["name"]}",' for row in data['rows']], '};',
                     '} // namespace ComboRando::Entrances']
    return {JSON: (json.dumps(data, indent=2) + '\n').encode(),
            INC: ('\n'.join(inc) + '\n').encode(), NAMES: ('\n'.join(names) + '\n').encode()}


def main():
    args = arguments(__doc__)
    original, records = read_registry(args.reference)
    data = build(original, records)
    publish(args.output_root, outputs(data), args.check)
    print('Verified 11 distinct OoT Wallmaster arrival indices, scenes and native entryway regions.')


if __name__ == '__main__':
    main()
