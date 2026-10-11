"""Export entrance data from the exact OoTMM v32.3 Git blobs, without network access.

Working-tree line endings and local edits never affect the source hash. Generated
files are LF UTF-8; --check reports drift without modifying any output.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

PIN = '413573d22d1d53e8a81931a70927f625b73488b9'
REGISTRY_SOURCE = 'data/defs/entrances.yml'
DEFAULT_OUTPUT = Path(__file__).resolve().parents[2]


def verified_blob(reference, path):
    reference = Path(reference).resolve()
    head = subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'],
                                   text=True, timeout=30).strip()
    if head != PIN:
        raise RuntimeError(f'Expected exact OoTMM v32.3 revision {PIN}, found {head}')
    return subprocess.check_output(['git', '-C', str(reference), 'show', f'{PIN}:{path}'], timeout=30)


class FlowParser:
    def __init__(self, source):
        self.source, self.pos = source, 0

    def skip(self):
        while self.pos < len(self.source) and self.source[self.pos].isspace():
            self.pos += 1

    def value(self):
        self.skip()
        first = self.source[self.pos]
        if first in '[{':
            self.pos += 1
            result = [] if first == '[' else {}
            closing = ']' if first == '[' else '}'
            while True:
                self.skip()
                if self.source[self.pos] == closing:
                    self.pos += 1
                    return result
                part = self.value()
                if first == '{':
                    self.skip()
                    if self.source[self.pos] != ':':
                        raise ValueError('Expected map colon')
                    self.pos += 1
                    if part in result:
                        raise ValueError('Duplicate map key')
                    result[part] = self.value()
                else:
                    result.append(part)
                self.skip()
                if self.source[self.pos] == ',':
                    self.pos += 1
                elif self.source[self.pos] != closing:
                    raise ValueError('Expected flow delimiter')
        if first in '\"\'':
            quote = first
            self.pos += 1
            result = ''
            while self.pos < len(self.source):
                char = self.source[self.pos]
                self.pos += 1
                if char == quote:
                    if quote == "'" and self.source[self.pos:self.pos + 1] == "'":
                        self.pos += 1
                        result += "'"
                        continue
                    return result
                if char == '\\' and quote == '"':
                    raise ValueError('Escaped scalar needs a full YAML reader')
                result += char
            raise ValueError('Unterminated quoted scalar')
        start = self.pos
        while self.pos < len(self.source) and self.source[self.pos] not in ',:{}[]#':
            self.pos += 1
        token = self.source[start:self.pos].strip()
        if not token or token.startswith(('&', '*', '!')):
            raise ValueError('Unsupported scalar')
        return token

    def parse(self):
        result = self.value()
        self.skip()
        if self.pos < len(self.source) and self.source[self.pos] != '#':
            raise ValueError('Unexpected trailing YAML')
        return result


def parse_registry(original):
    records = []
    names = set()
    for line_no, line in enumerate(original.decode().splitlines(), 1):
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        match = re.fullmatch(r'([A-Z0-9_]+):\s*(\{.*)', line)
        if not match:
            raise ValueError(f'Unrecognized registry syntax at {line_no}')
        name, body = match.groups()
        if name in names:
            raise ValueError(f'Duplicate entrance name {name}')
        names.add(name)
        fields = FlowParser(body).parse()
        allowed = {'game', 'id', 'type', 'areas', 'reverse', 'maps', 'submaps', 'flags', 'debug'}
        if set(fields) - allowed:
            raise ValueError(f'New field at {name}: {set(fields) - allowed}')
        entry = {'name': name, 'game': fields['game'], 'id': int(fields['id'], 0),
                 'type': fields['type'], 'reverse': fields.get('reverse'),
                 'from': fields.get('areas', ['NONE', 'NONE'])[0],
                 'to': fields.get('areas', ['NONE', 'NONE'])[1],
                 'fromMap': fields.get('maps', ['NONE', 'NONE'])[0],
                 'toMap': fields.get('maps', ['NONE', 'NONE'])[1],
                 'fromSubmap': fields.get('submaps', ['NONE', 'NONE'])[0],
                 'toSubmap': fields.get('submaps', ['NONE', 'NONE'])[1],
                 'flags': fields.get('flags', []), 'sourceLine': line_no}
        if entry['game'] not in ('oot', 'mm') or not 0 <= entry['id'] < 0x80000000:
            raise ValueError(f'Unsupported game/ID at {name}')
        if any(flag != 'no-sot' for flag in entry['flags']):
            raise ValueError(f'Unsupported flag at {name}')
        records.append(entry)
    by_name = {e['name']: e for e in records}
    keys = set()
    for entry in records:
        key = (entry['game'], entry['id'])
        if key in keys:
            raise ValueError(f'Duplicate per-game ID: {key}')
        keys.add(key)
        if entry['reverse']:
            reverse = by_name[entry['reverse']]
            if reverse['reverse'] != entry['name'] or reverse['game'] != entry['game']:
                raise ValueError(f'Invalid reverse pair: {entry["name"]}')
    return records


def read_registry(reference):
    original = verified_blob(reference, REGISTRY_SOURCE)
    records = parse_registry(original)
    if len(records) != 608:
        raise RuntimeError('Unexpected v32.3 registry size')
    return original, records


def enum_name(value):
    return ''.join(part.capitalize() for part in value.split('-'))


def json_bytes(value):
    return (json.dumps(value, indent=2, ensure_ascii=True) + '\n').encode('utf-8')


def initializer(values):
    # Stable wrapping, independent of a locally installed clang-format version.
    lines, line = [], '    { '
    for index, value in enumerate(values):
        token = value + (', ' if index + 1 < len(values) else ' },')
        if len(line) + len(token) > 120:
            lines.append(line.rstrip())
            line = '      '
        line += token
    return '\n'.join([*lines, line])


def registry_header(original, records):
    q = lambda value: json.dumps(value, ensure_ascii=True)
    types = sorted({entry['type'] for entry in records})
    lines = [
        '// Derived from OoTMM v32.3 data/defs/entrances.yml; see LICENSE.OoTMM.',
        '// Generated by tools/ootmm/export_registry.py; do not hand-edit.',
        '// Hashes use canonical Git blob bytes. Existing v1 envelopes use LegacyEnvelope.h.',
        '#pragma once', '#include <array>', '#include <cstdint>', '#include <string_view>',
        'namespace ComboRando::Entrances {',
        f'inline constexpr std::string_view kUpstreamCommit = {q(PIN)};',
        'inline constexpr std::string_view kRegistrySourceSha256 =',
        f'    {q(hashlib.sha256(original).hexdigest())};',
        'enum class Game : uint8_t { Oot, Mm };', 'enum class Type : uint8_t {',
        *['    ' + enum_name(t) + (',' if i + 1 < len(types) else '') for i, t in enumerate(types)],
        '};', 'struct Record {', '    std::string_view name;', '    Game game;', '    uint32_t id;',
        '    Type type;', '    std::string_view reverse, from, to, fromMap, toMap, fromSubmap, toSubmap;',
        '    bool noSongOfTime;', '    uint16_t sourceLine;', '};', '// clang-format off',
        f'inline constexpr std::array<Record, {len(records)}> kRegistry = {{ {{']
    for entry in records:
        values = [q(entry['name']), f'Game::{"Oot" if entry["game"] == "oot" else "Mm"}',
                  hex(entry['id']) + 'u', 'Type::' + enum_name(entry['type']), q(entry['reverse'] or ''),
                  *[q(entry[field]) for field in ('from', 'to', 'fromMap', 'toMap', 'fromSubmap', 'toSubmap')],
                  'true' if 'no-sot' in entry['flags'] else 'false', str(entry['sourceLine'])]
        lines.append(initializer(values))
    lines += ['} };', '// clang-format on',
              'inline const Record* Find(std::string_view name) {',
              '    for (const auto& e : kRegistry)', '        if (e.name == name)', '            return &e;',
              '    return nullptr;', '}', 'inline const Record* Find(Game game, uint32_t id) {',
              '    for (const auto& e : kRegistry)', '        if (e.game == game && e.id == id)',
              '            return &e;', '    return nullptr;', '}',
              'inline const Record* Reverse(const Record& e) {', '    return Find(e.reverse);', '}',
              '} // namespace ComboRando::Entrances', '']
    return '\n'.join(lines).encode('utf-8')


def publish(output_root, outputs, check):
    mismatches = []
    for relative, data in outputs.items():
        destination = output_root / relative
        if check:
            if not destination.is_file() or destination.read_bytes() != data:
                mismatches.append(relative)
        else:
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
    if mismatches:
        raise RuntimeError('Generated output differs: ' + ', '.join(mismatches))


def arguments(description):
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--reference', type=Path, required=True, help='Local checkout at exact OoTMM v32.3 commit')
    parser.add_argument('--output-root', type=Path, default=DEFAULT_OUTPUT, help='Destination repository root')
    parser.add_argument('--check', action='store_true', help='Compare expected bytes; never modify output')
    return parser.parse_args()


def main():
    args = arguments(__doc__)
    original, records = read_registry(args.reference)
    manifest = {'upstreamCommit': PIN, 'source': REGISTRY_SOURCE,
                'sourceSha256': hashlib.sha256(original).hexdigest(), 'entrances': records}
    publish(args.output_root, {
        'combo/rando/entrances/Registry.h': registry_header(original, records),
        'combo/rando/entrances/LICENSE.OoTMM': verified_blob(args.reference, 'LICENSE'),
        'tools/ootmm/registry-source.json': json_bytes(manifest),
    }, args.check)
    print(f'{"Verified" if args.check else "Exported"} {len(records)} entrances from {PIN}')


if __name__ == '__main__':
    main()
