"""Export the pinned M1 contract (Wallmaster and one-way shuffle, own game) from exact v32.3 Git blobs.

The pool rules are re-implemented here from packages/logic/src/solver/entrances.ts, not from
combo/rando/entrances/Shuffle.h, and run over the pinned registry and world area files:
computeAllEntrances (179-201), entrancesForTypes/poolEntrancesForTypes (601-627), poolWallmasters
(657-669), poolOneWaysAnywhere/poolOneWays (742-784), makePoolsSimple/makePools/poolsTypesDst
(791-886), the polarity sets (43-80), isSelfLoop/isAssignableConstraints (459-498) and the no-sot
gate of placeSingleExpr (203-217). processRegions/propagateDungeons (960-1059) also run, to pin that
M1 leaves hint regions and dungeon entrances unchanged. Only settings reachable in M1 are modelled:
every other er* key must stay at its default, or the export fails. Python 3 and Git are the only
dependencies; a changed cited function body fails the export until this file is reviewed again.
"""
import hashlib
import itertools
import json
import re
import subprocess
from pathlib import Path

from export_entrance_settings import records as setting_records
from export_registry import PIN, arguments, publish, read_registry, verified_blob

OUTPUT = 'tools/ootmm/m1-contract.json'
ENTRANCES_TS = 'packages/logic/src/solver/entrances.ts'
BUILDER_TS = 'packages/logic/src/world/builder.ts'
DATA_BUILD_TS = 'packages/core/build/index.ts'
SETTINGS_TS = 'packages/core/src/settings/data.ts'
MACROS_COMMON = 'data/macros/macros_common.yml'
MACROS_MM = 'data/macros/macros_mm.yml'
HELPERS_TS = 'packages/logic/src/helpers.ts'
SPOILER_TS = 'packages/generator/lib/combo/spoiler.ts'
SETTINGS_UTIL_TS = 'packages/core/src/settings/util.ts'
CORE_UTIL_TS = 'packages/core/src/util.ts'
GENERATOR_SRC = 'packages/generator/src'

M1_FLAGS = ('erOneWaysMajor', 'erOneWaysIkana', 'erOneWaysOwls', 'erOneWaysWaterVoids', 'erOneWaysAnywhere')
M1_KEYS = ('erWallmasters', 'erOneWays') + M1_FLAGS
SOURCE_TYPES = ('wallmaster', 'one-way', 'one-way-ikana', 'one-way-owl', 'one-way-water-void')
GAME_LINK_SRC = ('OOT_MARKET_FROM_MASK_SHOP', 'MM_CLOCK_TOWN_FROM_CLOCK_TOWER')
GAME_LINK_DST = ('OOT_SHOP_MASKS', 'MM_CLOCK_TOWER_FROM_CLOCK_TOWN')
# connectGamesDefault for games 'ootmm' (entrances.ts:888-903).
GAME_LINKS = (('OOT_SHOP_MASKS', 'MM_CLOCK_TOWN_FROM_CLOCK_TOWER'),
              ('MM_CLOCK_TOWER_FROM_CLOCK_TOWN', 'OOT_MARKET_FROM_MASK_SHOP'))
INVALID_REGIONS = ('NONE', 'ENTRANCE', 'BUFFER', 'BUFFER_DELAYED')


def pinned_blobs(reference, paths):
    """Exact blobs at the pin: one HEAD check, then one batch read."""
    verified_blob(reference, 'data/defs/entrances.yml')  # raises on any other HEAD
    request = ''.join(f'{PIN}:{path}\n' for path in paths).encode('utf-8')
    data = subprocess.run(['git', '-C', str(Path(reference).resolve()), 'cat-file', '--batch'], input=request,
                          capture_output=True, check=True, timeout=120).stdout
    result, pos = {}, 0
    for path in paths:
        end = data.index(b'\n', pos)
        header = data[pos:end].decode('utf-8').split()
        if len(header) != 3 or header[1] != 'blob':
            raise RuntimeError(f'Missing pinned blob {path}')
        size = int(header[2])
        result[path] = data[end + 1:end + 1 + size]
        pos = end + 1 + size + 1
    return result


def git_lines(reference, *args):
    return subprocess.run(['git', '-C', str(Path(reference).resolve()), *args], capture_output=True, check=True,
                          text=True, encoding='utf-8', timeout=120).stdout.splitlines()


def sha(text):
    return hashlib.sha256(text.encode('utf-8') if isinstance(text, str) else text).hexdigest()


# --- Pinned TypeScript bodies -------------------------------------------------------------------

def ts_block(text, start_pattern, end_line):
    """Lines from the single line matching start_pattern to the first following line equal to end_line."""
    lines = text.split('\n')
    starts = [i for i, line in enumerate(lines) if re.match(start_pattern, line)]
    if len(starts) != 1:
        raise ValueError(f'Expected one pinned block for {start_pattern}')
    for end in range(starts[0], len(lines)):
        if lines[end] == end_line:
            return {'lines': [starts[0] + 1, end + 1], 'text': '\n'.join(lines[starts[0]:end + 1])}
    raise ValueError(f'Unterminated pinned block {start_pattern}')


def method(text, name):
    return ts_block(text, r'  (private )?' + re.escape(name) + r'\(', '  }')


def constant(text, name):
    return ts_block(text, r'(export )?const ' + re.escape(name) + r'\b', ']);' if 'POLARITY' in name else '];')


ENTRANCE_BODIES = {
    'POLARITY_ANY_OVERWORLD': constant, 'POLARITY_IN': constant, 'POLARITY_OUT': constant,
    'DUNGEON_ENTRANCES': constant,
    'entrancePolarity': method, 'entrancePolarityMatch': method, 'computeAllEntrances': method,
    'placeSingleExpr': method, 'placeSingle': method, 'changedWorld': method, 'isSelfLoop': method,
    'isAssignableConstraints': method, 'placePools': method, 'entrancesForTypes': method,
    'poolEntrancesForTypes': method, 'overrideEntrance': method, 'poolWallmasters': method,
    'poolOneWaysAnywhere': method, 'poolOneWays': method, 'makePoolsSimple': method, 'makePools': method,
    'poolsTypesDst': method, 'connectGamesDefault': method, 'run': method, 'changeRegion': method,
    'isRegionInvalid': method, 'propagateRegionsStep': method, 'propagateDungeons': method,
    'propagateRegions': method, 'replaceAllRegions': method, 'simplifyRegions': method,
    'processRegions': method,
}


def guarded_bodies(blobs):
    text = blobs[ENTRANCES_TS].decode('utf-8')
    # run() exists in both classes; WorldShuffler.run is the first one and is the one cited.
    bodies = {}
    for name, extract in ENTRANCE_BODIES.items():
        if name == 'run':
            first = text.index('\n  run() {\n') + 1
            end = text.index('\n  }\n', first)
            line = text[:first].count('\n') + 1
            body = text[first:end + 4]
            bodies[name] = {'lines': [line, line + body.count('\n')], 'text': body}
        else:
            bodies[name] = extract(text, name)
    builder = blobs[BUILDER_TS].decode('utf-8')
    bodies['DUNGEONS_REGIONS'] = ts_block(builder, r'export const DUNGEONS_REGIONS', '};')
    bodies['loadAreas'] = method(builder, 'loadAreas')
    helpers = blobs[HELPERS_TS].decode('utf-8')
    bodies['isEntranceShuffle'] = ts_block(helpers, r'export function isEntranceShuffle\(', '}')
    spoiler = blobs[SPOILER_TS].decode('utf-8')
    bodies['writeEntrances'] = method(spoiler, 'writeEntrances')
    settings_util = blobs[SETTINGS_UTIL_TS].decode('utf-8')
    bodies['validateSettingsStep'] = ts_block(settings_util, r'function validateSettingsStep\(', '}')
    bodies['validateSettings'] = ts_block(settings_util, r'export function validateSettings\(', '}')
    bodies['gameId'] = ts_block(blobs[CORE_UTIL_TS].decode('utf-8'), r'export function gameId\(', '}')
    files = {'DUNGEONS_REGIONS': BUILDER_TS, 'loadAreas': BUILDER_TS, 'isEntranceShuffle': HELPERS_TS,
             'writeEntrances': SPOILER_TS, 'validateSettingsStep': SETTINGS_UTIL_TS,
             'validateSettings': SETTINGS_UTIL_TS, 'gameId': CORE_UTIL_TS}
    result = {}
    for name, body in bodies.items():
        source = files.get(name, ENTRANCES_TS)
        digest = sha(body['text'])
        expected = EXPECTED_BODY_SHA256.get(name)
        if expected != digest:
            raise RuntimeError(f'Pinned body {source}:{name} changed ({digest}); re-review export_m1_contract.py')
        result[name] = {'file': source, 'lines': body['lines'], 'sha256': digest}
    return result, bodies


# sha256 of each pinned body this file re-implements or relies on. A re-pin that changes one of them
# fails until the matching Python here is reviewed again and the value is updated.
EXPECTED_BODY_SHA256 = {
    'POLARITY_ANY_OVERWORLD': 'f012960f589e568fd5e4465135437b911a36b37843fc6e137a02fdd179897f08',  # 43-48
    'POLARITY_IN': 'a085a0d3efd7993fd4cca16bf1d28868b59f729cccf28f813e4a4cd1670b73e2',  # 50-72
    'POLARITY_OUT': 'f38c95a4005f6fa140eef25b7438d2ce9701a5b89aa7b1173e83e794252dd370',  # 74-80
    'DUNGEON_ENTRANCES': '619acd72098d6d0176a24df7f7475af97d54a520584b1b94b920d03873f66626',  # 82-109
    'entrancePolarity': '62d3003318cfde7179f70a83d89e1f1538a065aa1a3bceeb5491d198d2eb2999',  # 139-151
    'entrancePolarityMatch': 'f55dcbb28cccbdec9fd902bd43c68a497c37a4fe22bece58ba020f6592d15818',  # 153-163
    'computeAllEntrances': 'b8317c9ae52784a4af53ccd4bf6674bed16df9884db0f9b2a798c767242834d9',  # 179-201
    'placeSingleExpr': 'df1bbfb4c0c61960643dda4bb835f605a31f1601f525e5b95b5ce463bd69f348',  # 203-217
    'placeSingle': '1968001da5c65176c214e5d29d6205f83ea585b69c5cb8538eb677596d8ae5ce',  # 219-263
    'changedWorld': '7bde0e479fd6c72f4d4d5ac01c1ef131931decf65ac1c0d80c2f5aea2bd4ecca',  # 349-386
    'isSelfLoop': '49d9f5e628768e7aeb11f49cfe98b2e50d16c5d5d46adcdab3441276160cfc1c',  # 459-482
    'isAssignableConstraints': 'd25ae5775da24159d33a9bacc6aee06831421f847cfbd6242e298c3efd674a87',  # 484-498
    'placePools': '603568b1437f319e1def5d865c3e994eeac9b146e902c5d96e9542c62f484713',  # 567-599
    'entrancesForTypes': '966f15e918ebe5a21df1722f015ea1688004a29dd0d9b59f1a49fd801fe3fe7b',  # 601-606
    'poolEntrancesForTypes': '1ee422aec4351203229a0a682ed9df63d791379cc7b4c9d8fece2af73e2fed8b',  # 608-627
    'overrideEntrance': 'fef675b72a4047014a90fb9c8b15fecc169224236ae613b442d12f4726654ed5',  # 629-636
    'poolWallmasters': 'b3bcc76dafe81a24585b82b73d7f85750c731fb25358405325f1fe5998de25fd',  # 657-669
    'poolOneWaysAnywhere': '58941e7b8a4c02ba3a2a752333a0d4ca4382f6d325b66ee45ccc63b3cf513bbe',  # 742-756
    'poolOneWays': '906b6d16b769111d6e35fd10eea4f7452240774ee55309e5d68bdc48d67882c5',  # 758-784
    'makePoolsSimple': '6960506634d4f66ec278c421403937a8bc44d9e0df2caed625e20f0e44bf25ed',  # 791-861
    'makePools': '32d3922b770b23aab19dae3665f3df703ae50e6ce14c0170f584fa3cf951f83e',  # 863-879
    'poolsTypesDst': '01634e3187f1ad1afcbb497937c3ab908b3760a8293822699adfdec53675b61f',  # 881-886
    'connectGamesDefault': '1470faaff6e21de894c9e1b5729771ecc1e9a6ce4cc59368ab4efe251c8c79a2',  # 888-903
    'run': '13b4ce01e8637e49cbca074e095d4df28f624bb5f97778583a8761c98c60e53e',  # 905-934
    'changeRegion': 'ab5569e98251a20748974b1417887dac0259500f7ef55dbfb3c2821bcdbbeb2f',  # 960-968
    'isRegionInvalid': '850bf38e74956e0f012dba6be9b52a1ee12ee842c15f4c3b39efbfab8ce2b695',  # 970-972
    'propagateRegionsStep': 'b9f970968927620b824552bb9699f26e16588a5adc8de2fb8d045a0e5176184b',  # 974-994
    'propagateDungeons': '96d20a702a83287174e32f70368772d1c15b58433b30785d8e13b31758a972de',  # 996-1016
    'propagateRegions': 'd786a5ceb98efca8b4fbca17f8cd2251214680d36a6096a5ed6b485f862d884b',  # 1018-1025
    'replaceAllRegions': '817590b2b7b390b6dd8c34b5e1636f01f5f2554eb1ca11b2ffae65b4a5909f09',  # 1027-1035
    'simplifyRegions': '32eb65c165364e91bf54089ac20cd4e88edd42f1c2d89f0ef9ea74709eb9ba37',  # 1037-1042
    'processRegions': '8d9bf5a96a0a96c41e15600b5d98dc53f2ef665d48aa323d9685e311854d2210',  # 1044-1059
    'DUNGEONS_REGIONS': 'c2c48a3f09191774240fb5feba2d51c443cf2ae2eaeb83021559fb98a1261334',  # 26-55
    'loadAreas': 'b34be51e73a96baf1f7e4d5babdb8d04e3546d1a4dae23e9bc08e1a5e4dde698',  # 279-391
    'isEntranceShuffle': 'ee2589fa32451b9837005d6e2a9ca31a76280ecac2c5a2d378d9daf38154bafa',  # 6-56
    'writeEntrances': '4ba241754a79e9e41dd0a7031d75fee961b439edd864a4a4d8266b8d3b4efb09',  # 237-273
    'validateSettingsStep': 'ca494000d86ac722988f04d2f5f3b5ccc17ad9c073dbdc28d5ba171cb674ac59',  # 33-120
    'validateSettings': 'be1181b69427ba2d0eab368c8c36bd7b26479ee63818788568dd2d3de20a76ce',  # 122-130
    'gameId': 'fb57ba0e423718f45fcb8ecb1757fefc0c981ddfbb95822388bf82a2ceee093b',  # 54-66
}


# --- Pinned world areas ---------------------------------------------------------------------------

def game_id(game, name, char):
    """gameId (packages/core/src/util.ts:54-66)."""
    for prefix in ('OOT', 'MM', 'SHARED'):
        if name.startswith(prefix + char):
            return name
    return game.upper() + char + name


SCALAR = re.compile(r'"([^"\\]*)"|([A-Za-z0-9_]+)')


def yaml_line(text):
    """'key: value  # comment': a quoted or bare key, then nothing, a double-quoted scalar or a plain one."""
    match = SCALAR.match(text)
    if not match or text[match.end():match.end() + 1] != ':':
        raise ValueError('Unsupported YAML key')
    key = match[1] if match[1] is not None else match[2]
    rest = text[match.end() + 1:]
    stripped = rest.lstrip(' ')
    if not stripped or stripped.startswith('#'):
        return key, None
    if stripped == rest:
        raise ValueError('Missing space after YAML colon')
    if stripped.startswith('"'):
        value = SCALAR.match(stripped)
        tail = stripped[value.end():] if value else 'unterminated'
        if not value or (tail.strip() and not re.match(r'\s+#', tail)):
            raise ValueError('Unsupported quoted YAML scalar')
        return key, value[1]
    plain = re.split(r'\s+#', stripped, maxsplit=1)[0].rstrip()
    if stripped[0] in '[]{}&*!|>\'%@`,?:-' or ': ' in plain or plain.endswith(':'):
        raise ValueError('Unsupported plain YAML scalar')
    return key, plain


AREA_SCALARS = {'region', 'dungeon', 'boss', 'age_change', 'time'}
AREA_MAPS = {'exits', 'events', 'locations', 'gossip', 'stay'}


def parse_world_file(path, text, game, ignored):
    """Restricted reader for the three-level world YAML; other syntax fails closed.

    loadAreas reads only the fields above, so another area-level key is kept out of the world and
    listed in `ignored` (the pinned data has e.g. an 'exit:' map and an event written at field level).
    """
    areas = {}
    area = field = None
    field_indent = entry_indent = None
    for line_no, raw in enumerate(text.split('\n'), 1):
        if not raw.strip() or raw.lstrip(' ').startswith('#'):
            continue
        if '\t' in raw:
            raise ValueError(f'{path}:{line_no}: tab')
        indent = len(raw) - len(raw.lstrip(' '))
        try:
            key, value = yaml_line(raw.strip())
        except ValueError as error:
            raise ValueError(f'{path}:{line_no}: {error}') from None
        if indent == 0:
            if value is not None:
                raise ValueError(f'{path}:{line_no}: area with a scalar')
            name = game_id(game, key, ' ')
            if name in areas:
                raise ValueError(f'{path}:{line_no}: duplicate area')
            area = areas[name] = {'file': path, 'line': line_no, 'scalars': {}, 'maps': {}, 'ignored': []}
            field = field_indent = None
            continue
        if area is None:
            raise ValueError(f'{path}:{line_no}: entry outside an area')
        if field_indent is None:
            field_indent = indent
        if indent == field_indent:
            if key in area['scalars'] or key in area['maps'] or key in area['ignored']:
                raise ValueError(f'{path}:{line_no}: duplicate field')
            if key not in (AREA_MAPS if value is None else AREA_SCALARS):
                area['ignored'].append(key)
                ignored.append({'file': path, 'line': line_no, 'area': name, 'key': key})
                field = '#' + key if value is None else None  # an ignored map still owns its entries
                if field:
                    area['maps'][field], entry_indent = {}, None
            elif value is None:
                field, entry_indent = key, None
                area['maps'][key] = {}
            else:
                area['scalars'][key] = value
                field = None
            continue
        if indent < field_indent or field is None or value is None:
            raise ValueError(f'{path}:{line_no}: unsupported nesting')
        if entry_indent is None:
            entry_indent = indent
        if indent != entry_indent:
            raise ValueError(f'{path}:{line_no}: inconsistent indentation')
        if key in area['maps'][field]:
            raise ValueError(f'{path}:{line_no}: duplicate key')
        area['maps'][field][key] = (value, line_no)
    return areas


def data_world(build_text):
    """DATA_WORLD from packages/core/build/index.ts: set name -> list of yml globs, per section."""
    block = build_text.split('const DATA_WORLD = {\n', 1)[1].split('\n};\n', 1)[0]
    sections, current = {}, None
    for line in block.split('\n'):
        section = re.fullmatch(r'  (\w+): \{', line)
        entry = re.fullmatch(r"    (\w+): loadYaml\('([^']+)'\),", line)
        empty = re.fullmatch(r'    (\w+): \{\},', line)
        if section:
            current = sections[section[1]] = {}
        elif entry:
            current[entry[1]] = entry[2]
        elif empty:
            current[empty[1]] = None
        elif line != '  },':
            raise ValueError(f'Unsupported DATA_WORLD line: {line}')
    if list(sections) != ['oot', 'mq', 'mm', 'mm_us', 'mm_jp']:
        raise ValueError('Unexpected DATA_WORLD sections')
    return sections


def glob_files(pattern, files):
    if pattern.endswith('/**/*.yml'):
        prefix = 'data/' + pattern[:-len('**/*.yml')]
        matched = sorted(f for f in files if f.startswith(prefix) and f.endswith('.yml'))
    else:
        matched = ['data/' + pattern] if 'data/' + pattern in files else []
    if not matched:
        raise ValueError(f'No pinned world file for {pattern}')
    return matched


class WorldData:
    def __init__(self, blobs, world_files):
        self.sets = data_world(blobs[DATA_BUILD_TS].decode('utf-8'))
        self.parsed, self.ignored, parsed_files = {}, [], set()
        for section, game in (('oot', 'oot'), ('mq', 'oot'), ('mm', 'mm'), ('mm_us', 'mm'), ('mm_jp', 'mm')):
            for name, pattern in self.sets[section].items():
                merged = {}
                for path in (glob_files(pattern, world_files) if pattern else []):
                    if path in parsed_files:
                        raise ValueError(f'World file loaded by two area sets: {path}')
                    parsed_files.add(path)
                    areas = parse_world_file(path, blobs[path].decode('utf-8'), game, self.ignored)
                    if set(areas) & set(merged):
                        raise ValueError(f'Area defined twice in one area set: {path}')
                    merged.update(areas)
                self.parsed[(section, name)] = merged
        if parsed_files != set(world_files):
            raise ValueError('A pinned world file is not loaded by DATA_WORLD')
        dungeon_regions = blobs[BUILDER_TS].decode('utf-8').split('export const DUNGEONS_REGIONS', 1)[1]
        dungeon_regions = dungeon_regions.split('\n};', 1)[0]
        self.dungeon_regions = dict(re.findall(r'  (\w+): "(\w+)",', dungeon_regions))

    def area_sets(self, mq=(), jp=False):
        """Area sets in loadAreas order (builder.ts:298-311), with the vanilla/MQ choice applied."""
        result = []
        for name in self.sets['oot']:
            section = 'mq' if name in self.sets['mq'] and name in mq else 'oot'
            result.append(('oot', name, self.parsed[(section, name)]))
        for name in self.sets['mm']:
            areas = dict(self.parsed[('mm', name)])
            if name in self.sets['mm_us'] and name in self.sets['mm_jp']:
                areas.update(self.parsed[('mm_jp' if jp else 'mm_us', name)])
            result.append(('mm', name, areas))
        return result

    def world(self, mq=(), jp=False):
        """Area name -> game, region, exits, locations, as loadAreas builds them."""
        world, regions = {}, {}
        for game, _, areas in self.area_sets(mq, jp):
            for name, area in areas.items():
                scalars, maps = area['scalars'], area['maps']
                region = scalars.get('region')
                if region not in (None,) + INVALID_REGIONS:
                    region = game_id(game, region, '_')
                if scalars.get('dungeon'):
                    region = self.dungeon_regions[scalars['dungeon']]
                if region is None:
                    raise ValueError(f'Undefined region for area {name}')
                exits = {game_id(game, k, ' '): v for k, v in maps.get('exits', {}).items()}
                locations = [game_id(game, k, ' ') for k in maps.get('locations', {})]
                world[name] = {'game': game, 'region': region, 'exits': exits, 'locations': locations,
                               'file': area['file'], 'line': area['line']}
                for location in locations:
                    if regions.get(location, 'NONE') == 'NONE':
                        regions[location] = region
        return world, regions


# --- Pinned settings and pools ------------------------------------------------------------------

def condition_holds(condition, s):
    """The pinned conds of the M1 keys: a conjunction of hasGame(x) and x.key ===/!== 'value'."""
    body = re.sub(r'^\(\w: any\) => ', '', condition)
    if not body:
        return True
    for atom in body.split(' && '):
        if re.fullmatch(r'has(OoT|MM|OoTMM)(\(\w\))?', atom):
            continue  # games === 'ootmm'
        match = re.fullmatch(r"\w\.(\w+) (===|!==) '(\w+)'", atom)
        if not match:
            raise ValueError(f'Unsupported setting condition: {condition}')
        key, op, value = match.groups()
        if (s[key] == value) != (op == '==='):
            return False
    return True


def validate_settings(rows, s):
    """validateSettings (util.ts:79-130) restricted to the M1 keys: reset failed conds until stable."""
    s = dict(s)
    while True:
        changed = False
        for row in rows:
            if row['key'] not in M1_KEYS:
                continue
            value = s[row['key']]
            if value != row['default'] and not condition_holds(row['condition'], s):
                s[row['key']], changed = row['default'], True
            elif row['type'] == 'enum':
                choice = next(v for v in row['values'] if v['value'] == value)
                if not condition_holds(choice['condition'], s):
                    s[row['key']], changed = row['default'], True
        if not changed:
            return s


def m1_combinations(rows):
    defaults = {row['key']: row['default'] for row in rows}
    combos = []
    for wallmasters, one_ways in itertools.product(('none', 'ownGame'), repeat=2):
        for bits in itertools.product((False, True), repeat=len(M1_FLAGS)):
            s = dict(defaults, erWallmasters=wallmasters, erOneWays=one_ways, **dict(zip(M1_FLAGS, bits)))
            if validate_settings(rows, s) == s:
                combos.append(s)
    if len(combos) != 66:
        raise ValueError(f'Expected 66 normalized M1 combinations, found {len(combos)}')
    return combos


def combination_id(s):
    flags = [key for key in M1_FLAGS if s[key]]
    return ';'.join([f'erWallmasters={s["erWallmasters"]}', f'erOneWays={s["erOneWays"]}'] + flags)


class Pools:
    """The pinned pool rules for one world (entrances.ts), restricted to M1-reachable settings."""

    def __init__(self, ts, records, available, settings, defaults):
        for key, value in settings.items():
            if key.startswith('er') and key not in M1_KEYS and value != defaults[key]:
                raise ValueError(f'{key} is outside the M1 model')
        self.s = settings
        self.by_name = {r['name']: r for r in records}
        self.available = available  # computeAllEntrances, in registry order
        self.polarity = {name: re.findall(r"'([^']+)'", ts[name]['text'])
                         for name in ('POLARITY_ANY_OVERWORLD', 'POLARITY_IN', 'POLARITY_OUT')}

    def pool_one_ways(self):
        mapping = (('erOneWaysMajor', 'one-way'), ('erOneWaysIkana', 'one-way-ikana'),
                   ('erOneWaysSongs', 'one-way-song'), ('erOneWaysStatues', 'one-way-statue'),
                   ('erOneWaysOwls', 'one-way-owl'), ('erOneWaysWoods', 'one-way-woods'),
                   ('erOneWaysWaterVoids', 'one-way-water-void'))
        types = [t for key, t in mapping if self.s[key]]
        return {'src': types, 'dst': list(types), 'opts': {'ownGame': self.s['erOneWays'] == 'ownGame'}}

    def make_pools_simple(self):
        pools = {}
        if self.s['erOneWays'] != 'none' and not self.s['erOneWaysAnywhere']:
            pools['ONE_WAYS'] = self.pool_one_ways()
        return pools

    def pools_types_dst(self):
        return list(dict.fromkeys(t for pool in self.make_pools_simple().values() for t in pool['dst']))

    def extended(self):
        types = list(dict.fromkeys(self.pools_types_dst() + ['dungeon', 'dungeon-minor', 'dungeon-sh']))
        return types  # erBoss is 'none' in M1, so 'boss' is never added

    def make_pools(self):
        pools = self.make_pools_simple()
        if self.s['erWallmasters'] != 'none':
            pools['WALLMASTERS'] = {'src': ['wallmaster'], 'dst': self.extended(),
                                    'opts': {'alias': True, 'ownGame': self.s['erWallmasters'] == 'ownGame'}}
        if self.s['erOneWays'] != 'none' and self.s['erOneWaysAnywhere']:
            one_ways = self.pool_one_ways()
            pools['ONE_WAYS'] = {'src': one_ways['src'],
                                 'dst': list(dict.fromkeys(self.extended() + one_ways['dst'])),
                                 'opts': {'alias': True, 'ownGame': self.s['erOneWays'] == 'ownGame'}}
        return pools

    def entrances_for_types(self, types):
        selected = [n for n in self.available if self.by_name[n]['type'] in types]
        reverses = [self.by_name[n]['reverse'] for n in selected if self.by_name[n]['reverse']]
        return selected + reverses

    def pool_entrances(self, descr):
        used_src = {src for src, _ in GAME_LINKS}
        used_dst = {dst for _, dst in GAME_LINKS}
        src = [n for n in self.entrances_for_types(descr['src']) if n not in used_src]
        dst = self.entrances_for_types(descr['dst'])
        if not descr['opts'].get('alias'):
            dst = [n for n in dst if n not in used_dst]
        src = set(src) - set(GAME_LINK_SRC)
        dst = set(dst) - set(GAME_LINK_DST)
        return src, dst

    def polarity_of(self, name):
        kind = self.by_name[name]['type']
        if self.s['erOverworld'] != 'none' and kind in self.polarity['POLARITY_ANY_OVERWORLD']:
            return 'any'
        if kind in self.polarity['POLARITY_IN']:
            return 'in'
        if kind in self.polarity['POLARITY_OUT']:
            return 'out'
        return 'any'

    def is_self_loop(self, src, dst):
        a, b = self.by_name[src], self.by_name[dst]
        if a['fromMap'] == 'NONE' or b['toMap'] != a['fromMap']:
            return False
        if a['fromMap'] != a['toMap'] or b['fromMap'] != b['toMap']:
            return True
        return a['fromSubmap'] == b['toSubmap']

    def assignable(self, src, dst, own_game):
        if own_game and self.by_name[src]['game'] != self.by_name[dst]['game']:
            return False
        a, b = self.polarity_of(src), self.polarity_of(dst)
        if not self.s['erNoPolarity'] and a != 'any' and b != 'any' and a != b:
            return False
        return self.s['erSelfLoops'] or not self.is_self_loop(src, dst)

    def placed(self):
        """placePools up to the first sample: pools after empty-pool removal (entrances.ts:567-593)."""
        result = []
        for name, descr in self.make_pools().items():
            src, dst = self.pool_entrances(descr)
            if not src:
                if dst and not descr['opts'].get('alias'):
                    raise ValueError(f'Unbalanced pool: {name}')
                continue
            candidates = {o: sorted(n for n in dst if self.assignable(o, n, descr['opts'].get('ownGame', False)))
                          for o in sorted(src)}
            result.append({'name': name, 'srcTypes': descr['src'], 'dstTypes': descr['dst'],
                           'ownGame': bool(descr['opts'].get('ownGame')), 'alias': bool(descr['opts'].get('alias')),
                           'src': sorted(src), 'dst': sorted(dst), 'candidates': candidates})
        return result


# --- processRegions / propagateDungeons (entrances.ts:960-1059) --------------------------------------

def process_regions(world, location_regions, overrides, dungeon_entrances, extra_hint_regions):
    """Run changedWorld's region-relevant edits and processRegions on one world."""
    order = list(world)
    region = {name: area['region'] for name, area in world.items()}
    exits = {name: list(area['exits']) for name, area in world.items()}
    locations = {name: area['locations'] for name, area in world.items()}
    loc_region = dict(location_regions)
    for src, dst in overrides:
        exits[src['from']] = [x for x in exits[src['from']] if x != src['to']]
    for src, dst in overrides:
        if dst['to'] not in exits[src['from']]:
            exits[src['from']].append(dst['to'])
    marked = {d: ('replace', d) for d in dungeon_entrances if d not in {s['name'] for s, _ in overrides}}
    for src, dst in overrides:
        if src['name'] in dungeon_entrances and dst['name'] in dungeon_entrances:
            marked[dst['name']] = ('replace', src['name'])
    reverse = {}
    for src, dst in overrides:
        reverse.setdefault(dst['name'], []).append(src)

    def change(name, new, force=False):
        region[name] = new
        for location in locations[name]:
            if loc_region[location] == 'ENTRANCE' or force:
                loc_region[location] = new

    def replace_all(old, new):
        for name in order:
            if region[name] == old:
                change(name, new, True)

    def propagate_dungeons():
        for entrance in dungeon_entrances:
            if entrance in marked:
                continue
            for src in reverse.get(entrance, []):
                if region[src['from']] not in INVALID_REGIONS:
                    marked[entrance] = ('region', region[src['from']])
                    break

    def propagate():
        changed = True
        while changed:
            changed = False
            for name in order:
                if region[name] in INVALID_REGIONS:
                    continue
                for target in exits[name]:
                    if target not in region:
                        raise ValueError(f'Unknown exit: {target}')
                    if region[target] == 'ENTRANCE':
                        change(target, region[name])
                        changed = True
        propagate_dungeons()

    if not extra_hint_regions:
        replace_all('OOT_GANON_CASTLE_TOWER', 'OOT_GANON_CASTLE')
        replace_all('MM_TEMPLE_STONE_TOWER_INVERTED', 'MM_TEMPLE_STONE_TOWER')
        replace_all('MM_BUTLER_RACE', 'ENTRANCE')
        replace_all('MM_GORON_RACETRACK', 'ENTRANCE')
    propagate()
    replace_all('BUFFER', 'ENTRANCE')
    propagate()
    replace_all('BUFFER_DELAYED', 'ENTRANCE')
    propagate()
    replace_all('ENTRANCE', 'NAMELESS')
    propagate_dungeons()
    return region, loc_region, marked


def sample_layout(pools):
    """One complete own-game layout: each source takes the next free candidate after its own index."""
    layout = []
    for pool in pools:
        used = set()
        sources = pool['src']
        for index, src in enumerate(sources):
            options = pool['candidates'][src]
            rotated = options[index + 1:] + options[:index + 1] if options else []
            choice = next((d for d in rotated if d not in used), None)
            if choice is None:
                raise ValueError(f'No sample destination for {src}')
            used.add(choice)
            layout.append((src, choice))
        if not pool['alias'] and used != set(pool['dst']):
            raise ValueError(f'Sample layout leaves {pool["name"]} unbalanced')
    return layout


# --- Pinned logic macros and runtime sites ----------------------------------------------------------

def er_enabled_terms(macros):
    lines = [(i, line) for i, line in enumerate(macros.split('\n'), 1) if line.startswith('"er_enabled": ')]
    if len(lines) != 1:
        raise ValueError('Expected one er_enabled macro')
    line_no, line = lines[0]
    expression = re.fullmatch(r'"er_enabled": "(.*)"', line)[1]
    terms = []
    for term in expression.split(' || '):
        plain = re.fullmatch(r'setting\((\w+)\)', term)
        negated = re.fullmatch(r'!setting\((\w+), (\w+)\)', term)
        if plain:
            terms.append((plain[1], None))
        elif negated:
            terms.append((negated[1], negated[2]))
        else:
            raise ValueError(f'Unsupported er_enabled term: {term}')
    return line_no, terms


def is_entrance_shuffle_terms(body):
    lines = [line.strip() for line in body.split('\n')[1:-1]]
    if lines[-1] != 'return false;':
        raise ValueError('isEntranceShuffle must end with return false')
    terms = []
    for test, result in zip(lines[:-1:2], lines[1:-1:2]):
        plain = re.fullmatch(r'if \(settings\.(\w+)\)', test)
        negated = re.fullmatch(r"if \(settings\.(\w+) !== '(\w+)'\)", test)
        if result != 'return true;' or not (plain or negated):
            raise ValueError(f'Unsupported isEntranceShuffle line: {test}')
        terms.append((plain[1], None) if plain else (negated[1], negated[2]))
    if len(lines) != 2 * len(terms) + 1:
        raise ValueError('Unsupported isEntranceShuffle shape')
    return terms


def evaluate(terms, s):
    return any(bool(s[key]) if value is None else s[key] != value for key, value in terms)


# Cited ER-wide sites (plan section 2, facts runtime/*): file, first line, last line, anchor text.
RUNTIME_SITES = (
    ('CFG_ER_ANY', 'mm/play/play.c', 484, 494, 'if (Config_Flag(CFG_ER_ANY))'),
    ('CFG_ER_ANY', 'mm/play/play.c', 736, 741, 'entrance = g.initialEntrance;'),
    ('CFG_ER_ANY', 'mm/play/play.c', 997, 1002, 'g.initialEntrance = entrance;'),
    ('CFG_ER_ANY', 'oot/play/play.c', 182, 186, 'if (Config_Flag(CFG_ER_ANY))'),
    ('CFG_ER_ANY', 'oot/save.c', 38, 38, '!Config_Flag(CFG_ER_ANY)'),
    ('PG_ER', 'mm/patches/hot_water.S', 4, 9, 'PATCH_START 0x8010eba0'),
    ('CFG_ER_WALLMASTERS', 'oot/actors.c', 402, 412, 'Config_Flag(CFG_ER_WALLMASTERS)'),
    ('CFG_ER_WALLMASTERS', 'mm/actors.c', 637, 647, 'Config_Flag(CFG_ER_WALLMASTERS)'),
    ('PG_ER_WALLMASTERS', 'oot/actors/En/En_Wallmas.S', 3, 11, 'PATCH_START 0x808728c0'),
    ('PG_ER_WALLMASTERS', 'mm/actors/En/En_Wallmas.S', 3, 11, 'PATCH_START 0x808758ac'),
    ('PG_ER_WALLMASTERS', 'oot/actors/En/En_Wallmas.c', 48, 55, 'void EnWallmas_TakePlayerEr('),
    ('PG_ER_WALLMASTERS', 'mm/actors/En/En_Wallmas.c', 26, 33, 'void EnWallmas_TakePlayerEr('),
)
ENABLE_SITES = (
    ('PG_ER', 'packages/generator/lib/combo/patch-build/index.ts', 58, 'ER: isEntranceShuffle(settings),'),
    ('PG_ER_WALLMASTERS', 'packages/generator/lib/combo/patch-build/index.ts', 68,
     "ER_WALLMASTERS: settings.erWallmasters !== 'none',"),
    ('CFG_ER_ANY', 'packages/generator/lib/combo/randomizer/world-config.ts', 30,
     'ER_ANY: isEntranceShuffle(settings),'),
    ('CFG_ER_WALLMASTERS', 'packages/generator/lib/combo/randomizer/world-config.ts', 167,
     "ER_WALLMASTERS: settings.erWallmasters !== 'none',"),
)


def runtime_sites(reference, blobs):
    sites = []
    for flag, path, first, last, anchor in RUNTIME_SITES:
        lines = blobs[f'{GENERATOR_SRC}/{path}'].decode('utf-8').split('\n')[first - 1:last]
        text = '\n'.join(lines)
        if anchor not in text:
            raise ValueError(f'Cited runtime site moved: {path}:{first}')
        sites.append({'flag': flag, 'file': f'{GENERATOR_SRC}/{path}', 'lines': [first, last], 'anchor': anchor,
                      'sha256': sha(text)})
    enables = []
    for flag, path, line, text in ENABLE_SITES:
        if blobs[path].decode('utf-8').split('\n')[line - 1].strip() != text:
            raise ValueError(f'Cited enable condition moved: {path}:{line}')
        enables.append({'flag': flag, 'file': path, 'line': line, 'text': text})
    occurrences = []
    for hit in git_lines(reference, 'grep', '-n', '-w', '-E', 'CFG_ER_ANY|CFG_ER_WALLMASTERS|PG_ER|PG_ER_WALLMASTERS',
                         PIN, '--', GENERATOR_SRC):
        _, path, line, text = hit.split(':', 3)
        covered = any(site['file'] == path and site['lines'][0] <= int(line) <= site['lines'][1] for site in sites)
        if not covered:
            raise ValueError(f'Uncited ER-wide runtime site: {path}:{line}')
        occurrences.append({'file': path, 'line': int(line), 'text': text.strip()})
    hot_water = blobs[f'{GENERATOR_SRC}/mm/patches/hot_water.S'].decode('utf-8')
    patch = re.search(r'PATCH_GROUP (\w+)\n\nPATCH_START (0x[0-9a-f]+)\n  (.*)\n   (.*)\nPATCH_END', hot_water)
    if not patch or patch.groups() != ('PG_ER', '0x8010eba0', 'jr ra', 'nop'):
        raise ValueError('Unexpected PG_ER hot water patch')
    initial = [{'file': path, 'line': int(line), 'text': text.strip()} for path, line, text in
               (hit.split(':', 3)[1:] for hit in git_lines(reference, 'grep', '-n', '-w', 'initialEntrance', PIN, '--',
                                                         f'{GENERATOR_SRC}/mm'))]
    return {'sites': sites, 'enableConditions': enables, 'occurrences': occurrences,
            'hotWater': {'group': 'PG_ER', 'address': '0x8010eba0', 'instructions': ['jr ra', 'nop']},
            'mmInitialEntranceUses': initial}


def er_logic_sites(reference, blobs):
    """Every MM logic line that reaches er_enabled through the MM macros (common, then mm)."""
    bodies = {}
    for path in (MACROS_COMMON, MACROS_MM):
        for line_no, line in enumerate(blobs[path].decode('utf-8').split('\n'), 1):
            if line.strip() and not line.lstrip().startswith('#'):
                try:
                    key, value = yaml_line(line.strip())
                except ValueError as error:
                    raise ValueError(f'{path}:{line_no}: {error}') from None
                bodies[key.split('(', 1)[0]] = value
    closure = {'er_enabled'}
    while True:
        grown = {name for name, body in bodies.items()
                 if any(re.search(r'\b' + re.escape(used) + r'\b', body) for used in closure)}
        if grown <= closure:
            break
        closure |= grown
    pattern = '|'.join(sorted(closure))
    result = []
    for hit in git_lines(reference, 'grep', '-n', '-w', '-E', pattern, PIN, '--', 'data/world/mm', MACROS_COMMON,
                         MACROS_MM):
        _, path, line, text = hit.split(':', 3)
        uses = sorted(set(re.findall(r'\b(' + pattern + r')\b', text)))
        result.append({'file': path, 'line': int(line), 'uses': uses})
    return {'macros': sorted(closure), 'lines': result}


# --- Assembly --------------------------------------------------------------------------------------

def counts(names, by_name):
    result = {'oot': 0, 'mm': 0}
    for name in names:
        result[by_name[name]['game']] += 1
    return result


def quest_of(path):
    return 'mq' if '/dungeons_mq/' in path else 'vanilla' if path.startswith('data/world/oot/dungeons/') else 'any'


def exits_into(worlds, target):
    """Every world exit whose gameId target is the area, across the vanilla and MQ files."""
    found = {}
    for world in worlds:
        for name, area in world.items():
            if target in area['exits']:
                expression, line = area['exits'][target]
                found[(area['file'], line)] = {'file': area['file'], 'line': line, 'from': name,
                                               'expression': expression, 'quest': quest_of(area['file'])}
    return [found[key] for key in sorted(found)]


def carried_exits(worlds, row):
    """The source expression a placed row keeps (getExpr), from every file that defines its area."""
    found = {}
    for world in worlds:
        area = world.get(row['from'])
        if area:
            expression, line = area['exits'][row['to']]
            found[(area['file'], line)] = {'file': area['file'], 'line': line, 'expression': expression,
                                           'quest': quest_of(area['file'])}
    return [found[key] for key in sorted(found)]


def build(reference):
    verified_blob(reference, 'data/defs/entrances.yml')  # refuse any other HEAD before reading anything else
    world_files = git_lines(reference, 'ls-tree', '-r', '--name-only', PIN, '--', 'data/world')
    runtime_paths = sorted({f'{GENERATOR_SRC}/{site[1]}' for site in RUNTIME_SITES} |
                           {site[1] for site in ENABLE_SITES})
    paths = [ENTRANCES_TS, BUILDER_TS, DATA_BUILD_TS, SETTINGS_TS, SETTINGS_UTIL_TS, CORE_UTIL_TS, MACROS_COMMON,
             MACROS_MM, HELPERS_TS, SPOILER_TS,
             *world_files, *runtime_paths]
    blobs = pinned_blobs(reference, paths)
    _, records = read_registry(reference)
    by_name = {r['name']: r for r in records}
    guards, bodies = guarded_bodies(blobs)
    rows = setting_records(blobs[SETTINGS_TS].decode('utf-8'))
    defaults = {row['key']: row['default'] for row in rows}
    combos = m1_combinations(rows)
    data = WorldData(blobs, set(world_files))
    mq_keys = list(data.sets['mq'])
    dungeon_entrances = re.findall(r"'(\w+)'", bodies['DUNGEON_ENTRANCES']['text'])
    if len(dungeon_entrances) != 26 or any(name not in by_name for name in dungeon_entrances):
        raise ValueError('Unexpected DUNGEON_ENTRANCES')

    # computeAllEntrances: which registry areas exist for each vanilla/MQ choice (C7 evidence).
    def present(mq, jp=False):
        return {name for _, _, areas in data.area_sets(mq, jp) for name in areas}
    registry_areas = sorted({r[f] for r in records for f in ('from', 'to') if r[f] != 'NONE'})
    vanilla_areas, jp_areas = present(()), present((), True)
    by_jp = {'us': sorted(a for a in registry_areas if a in vanilla_areas and a not in jp_areas),
             'jp': sorted(a for a in registry_areas if a in jp_areas and a not in vanilla_areas)}
    by_quest = {}
    for key in mq_keys:
        mq_areas = present((key,))
        only_vanilla = sorted(a for a in registry_areas if a in vanilla_areas and a not in mq_areas)
        only_mq = sorted(a for a in registry_areas if a in mq_areas and a not in vanilla_areas)
        if only_vanilla or only_mq:
            by_quest[key] = {'vanilla': only_vanilla, 'mq': only_mq}
    conditional = [a for q in by_quest.values() for a in q['vanilla'] + q['mq']]
    if len(conditional) != len(set(conditional)):
        raise ValueError('A registry area depends on more than one vanilla/MQ choice')
    never = sorted(a for a in registry_areas if a not in vanilla_areas and a not in conditional + by_jp['jp'])
    jp_rows = sorted(r['name'] for r in records if {r['from'], r['to']} & set(by_jp['us'] + by_jp['jp']))

    def available(mq):
        areas = present(mq)
        return [r['name'] for r in records if (r['from'] == 'NONE' or r['from'] in areas) and
                (r['to'] == 'NONE' or r['to'] in areas)]
    quest_rows = sorted(r['name'] for r in records if r['from'] in conditional or r['to'] in conditional)
    vanilla_rows = available(())
    vanilla_world, vanilla_regions = data.world(())
    mq_world, _ = data.world(tuple(mq_keys))

    # Every row an M1 pool can select, in any MQ context: only the OoT Wallmaster sources depend on it,
    # and every reverse entrancesForTypes adds is itself available (Shuffle.h rejects a missing one).
    m1_types = set(SOURCE_TYPES) | {'dungeon', 'dungeon-minor', 'dungeon-sh'}
    m1_selectable = {r['name'] for r in records if r['type'] in m1_types}
    m1_selectable |= {by_name[n]['reverse'] for n in m1_selectable if by_name[n]['reverse']}
    if any(by_name[n]['type'] != 'wallmaster' for n in m1_selectable & set(quest_rows)) or \
            m1_selectable & set(jp_rows):
        raise ValueError('An M1 destination depends on the vanilla/MQ choice or the MM JP layouts')
    if not m1_selectable - {n for n in m1_selectable if by_name[n]['type'] == 'wallmaster'} <= set(vanilla_rows):
        raise ValueError('An M1 row is absent from the pinned world')

    # Settings matrix in the all-vanilla world.
    erm_line, erm_terms = er_enabled_terms(blobs[MACROS_MM].decode('utf-8'))
    ies_terms = is_entrance_shuffle_terms(bodies['isEntranceShuffle']['text'])
    links = [(by_name[a], by_name[b]) for a, b in GAME_LINKS]
    no_er = {extra: process_regions(vanilla_world, vanilla_regions, links, dungeon_entrances, extra)
             for extra in (False, True)}
    transient = set(INVALID_REGIONS[1:]) | {'MM_BUTLER_RACE', 'MM_GORON_RACETRACK'}
    oot_wallmasters = [r['name'] for r in records if r['type'] == 'wallmaster' and r['game'] == 'oot']
    matrix, m1_rows, wallmaster_oot_dst = [], set(), {}
    for s in combos:
        pools = Pools(bodies, records, vanilla_rows, s, defaults).placed()
        entry = {'id': combination_id(s), 'settings': {k: s[k] for k in M1_KEYS},
                 'erEnabled': evaluate(erm_terms, s), 'isEntranceShuffle': evaluate(ies_terms, s), 'pools': []}
        destinations = set()
        for pool in pools:
            groups = {}
            for src, options in pool['candidates'].items():
                groups.setdefault(tuple(options), []).append(src)
            entry['pools'].append({
                'name': pool['name'], 'ownGame': pool['ownGame'], 'alias': pool['alias'],
                'srcTypes': pool['srcTypes'], 'dstTypes': pool['dstTypes'],
                'srcCount': counts(pool['src'], by_name), 'dstCount': counts(pool['dst'], by_name),
                'dst': pool['dst'],
                'candidates': [{'sources': sources, 'destinations': list(options)}
                               for options, sources in sorted(groups.items(), key=lambda g: g[1][0])]})
            destinations |= {d for options in pool['candidates'].values() for d in options}
            m1_rows |= set(pool['src']) | set(pool['dst'])
            if pool['name'] == 'WALLMASTERS':
                shared = {tuple(pool['candidates'][o]) for o in pool['src'] if o in oot_wallmasters}
                if len(shared) != 1:
                    raise ValueError('OoT Wallmaster sources do not share one candidate set')
                wallmaster_oot_dst[entry['id']] = len(shared.pop())
        entry['resetGated'] = sorted(d for d in destinations
                                     if by_name[d]['game'] == 'mm' and 'no-sot' not in by_name[d]['flags'])
        entry['deadEnds'] = sorted(d for d in destinations if not vanilla_world[by_name[d]['to']]['exits'])
        # C14: an M1 edit only touches areas whose region processRegions never assigns, and no M1 source
        # is a dungeon entrance, so hint regions and dungeonsEntrances match the no-ER world for every
        # layout. The union of all candidate edges and one complete layout are also run through it.
        touched = {by_name[d]['to'] for d in destinations} | {by_name[o]['to'] for p in pools for o in p['src']}
        if any(vanilla_world[a]['region'] in transient for a in touched):
            raise ValueError(f'M1 change reaches a region-propagated area in {entry["id"]}')
        if any(o in dungeon_entrances for p in pools for o in p['src']):
            raise ValueError('An M1 source is a dungeon entrance')
        layout = sample_layout(pools)
        union = [(by_name[o], by_name[d]) for p in pools for o, ds in p['candidates'].items() for d in ds]
        for edits in (union, [(by_name[o], by_name[d]) for o, d in layout]):
            for extra in (False, True):
                if process_regions(vanilla_world, vanilla_regions, links + edits, dungeon_entrances,
                                   extra) != no_er[extra]:
                    raise ValueError(f'processRegions output changed in {entry["id"]}')
        entry['sampleLayout'] = [list(pair) for pair in layout]
        matrix.append(entry)

    # MQ source table: active OoT Wallmaster sources for every choice of the dungeons with Wallmasters.
    wallmaster_dungeons = [k for k in mq_keys if any(
        by_name[n]['from'] in data.parsed[(section, k)] for n in oot_wallmasters for section in ('oot', 'mq'))]
    mq_table, shortfalls = [], []
    for bits in itertools.product((False, True), repeat=len(wallmaster_dungeons)):
        mq = tuple(k for k, bit in zip(wallmaster_dungeons, bits) if bit)
        rows_here = set(available(mq))
        sources = [n for n in oot_wallmasters if n in rows_here]
        mq_table.append({'mq': list(mq), 'ootSources': sources})
        short = [cid for cid, n in wallmaster_oot_dst.items() if len(sources) > n]
        if short:
            shortfalls.append({'mq': list(mq), 'ootSources': len(sources), 'combinations': short})

    # Exits into every M1 source area and the expression each source keeps, from vanilla and MQ files.
    worlds = (vanilla_world, mq_world)
    sources = {}
    for name in [r['name'] for r in records if r['type'] in SOURCE_TYPES]:
        row = by_name[name]
        sources[name] = {'game': row['game'], 'type': row['type'], 'from': row['from'], 'to': row['to'],
                         'carried': carried_exits(worlds, row), 'incoming': exits_into(worlds, row['from'])}
    soul_lines = [{'file': hit[1], 'line': int(hit[2])} for hit in
                  (line.split(':', 3) for line in git_lines(reference, 'grep', '-n', '-w', 'soul_wallmaster', PIN,
                                                            '--', 'data/world/oot'))]

    # Spoiler 'Entrances' section (spoiler.ts:237-272) for the two default game links of every ootmm seed.
    spoiler = bodies['writeEntrances']['text']
    key_template = '${srcEntry.from} to ${srcEntry.to} (${src})'
    value_template = '${dstEntry.to} from ${dstEntry.from} (${dst})'
    if f'let key = `{key_template}`;' not in spoiler or f'let value = `{value_template}`;' not in spoiler or \
            '${key.padEnd(longestEntranceEntry)} -> ${value}' not in spoiler or \
            'Object.keys(niceEntrances).sort()' not in spoiler:
        raise ValueError('Unexpected spoiler Entrances format')
    lines = {f'{by_name[s]["from"]} to {by_name[s]["to"]} ({s})': f'{by_name[d]["to"]} from {by_name[d]["from"]} ({d})'
             for s, d in GAME_LINKS}
    width = max(len(k) for k in lines)
    section = ['Entrances'] + [f'  {k.ljust(width)} -> {lines[k]}' for k in sorted(lines)] + ['']

    rows_used = sorted(m1_rows | set(oot_wallmasters))
    return {
        'upstreamCommit': PIN,
        'scope': 'games=ootmm; erWallmasters and erOneWays in {none, ownGame}; the five one-way flags; '
                 'every other er* key at its default (erNoPolarity, erSelfLoops and erDecoupled false); '
                 'pools and candidates in the all-vanilla world',
        'inputs': {path: sha(blobs[path]) for path in paths},
        'guardedBodies': guards,
        'registryAreaPresence': {'never': never, 'byQuest': by_quest, 'questDependentRows': quest_rows,
                                 'byMmJpLayout': by_jp, 'mmJpLayoutDependentRows': jp_rows},
        'ignoredWorldKeys': data.ignored,
        'rows': {n: {k: by_name[n][k] for k in ('game', 'id', 'type', 'reverse', 'from', 'to', 'fromMap', 'toMap')}
                 | {'noSot': 'no-sot' in by_name[n]['flags']} for n in rows_used},
        'regionInvariance': {
            'transientRegions': sorted(transient),
            'checks': ['no M1 source is a DUNGEON_ENTRANCES row, so changedWorld marks every dungeon as itself',
                       'no source or candidate arrival area starts in a transient region',
                       'processRegions output equals the no-ER world with every candidate edge added at once',
                       'processRegions output equals the no-ER world for the sampleLayout of the combination'],
            'extraHintRegions': [False, True]},
        'combinations': matrix,
        'mqSources': {'dungeons': wallmaster_dungeons, 'table': mq_table, 'capacityShortfalls': shortfalls},
        'sources': sources,
        'soulWallmasterLines': soul_lines,
        'truthTables': {'erEnabled': {'file': MACROS_MM, 'line': erm_line,
                                      'm1Keys': [k for k, _ in erm_terms if k in M1_KEYS]},
                        'isEntranceShuffle': {'file': HELPERS_TS, 'lines': guards['isEntranceShuffle']['lines'],
                                              'm1Keys': [k for k, _ in ies_terms if k in M1_KEYS]}},
        'erLogicSites': er_logic_sites(reference, blobs),
        'runtime': runtime_sites(reference, blobs),
        'spoiler': {'file': SPOILER_TS, 'lines': guards['writeEntrances']['lines'], 'header': 'Entrances',
                    'indent': '  ', 'key': key_template, 'value': value_template, 'separator': ' -> ',
                    'keyPadding': 'padEnd(longest key)', 'order': 'keys sorted by UTF-16 code unit',
                    'noErSection': section},
    }


def contract_bytes(value):
    """Indented JSON with every list of scalars on one line, so the fixture stays reviewable."""
    def render(item, depth):
        pad = '  ' * depth
        if isinstance(item, dict):
            if not item:
                return '{}'
            inner = [f'{pad}  {json.dumps(k)}: {render(v, depth + 1)}' for k, v in item.items()]
            return '{\n' + ',\n'.join(inner) + f'\n{pad}}}'
        if isinstance(item, list):
            if all(not isinstance(v, (dict, list)) for v in item):
                return json.dumps(item, ensure_ascii=True)
            inner = [f'{pad}  {render(v, depth + 1)}' for v in item]
            return '[\n' + ',\n'.join(inner) + f'\n{pad}]'
        return json.dumps(item, ensure_ascii=True)
    return (render(value, 0) + '\n').encode('utf-8')


def main():
    args = arguments(__doc__)
    contract = build(args.reference)
    publish(args.output_root, {OUTPUT: contract_bytes(contract)}, args.check)
    print(f'{"Verified" if args.check else "Exported"} the M1 contract: {len(contract["combinations"])} combinations, '
          f'{len(contract["mqSources"]["table"])} MQ source contexts')


if __name__ == '__main__':
    main()
