"""SoH's own Master Quest map marks against the bundled OoTMM v32.3 bins (MQ port unit U2, tests only).

OoTMM replaces the whole pause-map mark table with mq_maps.bin (src/oot/mq.c:452-477) and copies each dungeon's block
of minimap marks from mq_scenes.bin over the vanilla ones (mq.c:479-515). For MQ, SoH draws
gPauseMapMarkDataTableMasterQuest (ovl_kaleido_scope/z_lmap_mark_data.c) and gMapMarkDataTableMq
(ovl_map_mark_data/z_map_mark_data.c). Both draw loops (z_lmap_mark.c PauseMapMark_DrawForDungeon, z_map_mark.c
MapMark_DrawForDungeon) walk records until a NONE record, on past an entry's three if none is NONE, and draw `count`
points of each with the record's vertex list, so two tables are draw-equivalent where those walks give the same
records. C zero-fills a partly initialised entry, and so does the parser here.
- All 34 pause floors are draw-equivalent except floor 17 (Fire map 4), where make_mq.ts:401-409 adds a sixth chest
  {0, 46.5, -17.5}.
- All minimaps are draw-equivalent except Fire minimap 3, where make_mq.ts:285-294 adds chest {0, 72, 60}.
- OoTMM copies whole blocks: a dungeon's entries plus zero padding to 16 bytes. Every vanilla table has the same number
  of entries, so the padding never lands on a vanilla entry, except Jabu-Jabu: its block is 16 entries with no padding,
  and the vanilla entry 16 it leaves in place is NONE, as is entry 16 of SoH's MQ table.
"""
from pathlib import Path
import argparse
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
args = parser.parse_args()
repo = args.source.resolve()
sys.path.insert(0, str(Path(__file__).resolve().parent))
import mq_bins  # noqa: E402

soh = repo / 'soh'
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


DEFINES = {}
for header in ('include/z64.h', 'include/z64map_mark.h'):
    DEFINES.update({name: int(value) for name, value in re.findall(
        r'#define ((?:PAUSE_)?MAP_MARK_(?:NONE|CHEST|BOSS)) (-?\d+)', (soh / header).read_text(encoding='utf-8'))})
NONE = DEFINES['MAP_MARK_NONE']
assert DEFINES['PAUSE_MAP_MARK_NONE'] == NONE and len(DEFINES) == 6


def initializer(text, name):
    """The initializer of `name[] = {...};` as nested lists of atoms."""
    text = re.sub(r'/\*.*?\*/', '', re.sub(r'//[^\n]*', '', text), flags=re.S)
    start = text.index('{', text.index(name + '[] = {'))
    tokens = re.findall(r'[{},]|[^\s{},]+', text[start:])

    def parse(at):
        assert tokens[at] == '{'
        items, at = [], at + 1
        while tokens[at] != '}':
            if tokens[at] == '{':
                item, at = parse(at)
            else:
                item, at = tokens[at], at + 1
            items.append(item)
            if tokens[at] == ',':
                at += 1
        return items, at + 1

    return parse(0)[0]


def atom(value):
    if isinstance(value, list):
        raise ValueError(f'unexpected braces {value}')
    if value in DEFINES:
        return DEFINES[value]
    if value == 'NULL':
        return 0
    if re.fullmatch(r'-?\d+\.\d*f?', value):
        return mq_bins.float_bits(float(value.rstrip('f')))
    if re.fullmatch(r'-?(0x[0-9A-Fa-f]+|\d+)', value):
        return int(value, 0)
    return value  # a vertex list name


def points(value, floats):
    """Twelve [flag, x, y] points, brace elision and zero fill as in C."""
    value = value if isinstance(value, list) else [value]
    if any(not isinstance(item, list) for item in value):
        flat = [atom(item) for item in value]
        value = [flat[i:i + 3] for i in range(0, len(flat), 3)]
    else:
        value = [[atom(field) for field in item] for item in value]
    zero = mq_bins.float_bits(0.0) if floats else 0
    value = [item + [0, zero, zero][len(item):] for item in value]
    return value + [[0, zero, zero]] * (12 - len(value))


def records(entries, pause):
    """Every record of a table of 3-record entries, in memory order: (markType, vtx, vtxCount, count, points)."""
    out = []
    for entry in entries:
        assert len(entry) <= 3, entry
        for record in entry + [[]] * (3 - len(entry)):
            if pause:  # markType, unk_04, vtx, vtxCount, count, points[12]
                fields = [atom(field) for field in record[:5]] + [0] * (5 - len(record[:5]))
                out.append((fields[0], fields[2] != 0, fields[3], fields[4],
                            points(record[5] if len(record) > 5 else [], True)))
            else:  # markType, count, points[12]
                fields = [atom(field) for field in record[:2]] + [0] * (2 - len(record[:2]))
                out.append((fields[0], True, 0, fields[1], points(record[2] if len(record) > 2 else [], False)))
    return out


def draw(table, entry):
    """What the draw loop draws for an entry: records until NONE, each with its first `count` points."""
    out = []
    for at in range(entry * 3, len(table)):
        mark, vtx, vtx_count, count, marks = table[at]
        if mark == NONE:
            return out
        if count > 0:
            out.append((mark, vtx, vtx_count, tuple(tuple(point) for point in marks[:count])))
    return out + ['runs past the end of the table']


rooms_bin, scenes_bin, maps_bin = mq_bins.load(repo / 'combo/mq/data')
decoded = mq_bins.decode(rooms_bin, scenes_bin, maps_bin)

# Pause maps: mq_maps.bin replaces the whole table, 34 floors of 3 records (mq.c:454, 467).
pause_source = (soh / 'src/overlays/misc/ovl_kaleido_scope/z_lmap_mark_data.c').read_text(encoding='utf-8')
soh_pause = records(initializer(pause_source, 'gPauseMapMarkDataTableMasterQuest'), True)
mq_body = pause_source[pause_source.index('gPauseMapMarkDataTableMasterQuest[] = {'):]
labels = re.findall(r'^    // (.*)\n    \{', mq_body[:mq_body.index('\n};')], re.M)
bin_pause = [(mark, vtx, vtx_count, count, [list(point) for point in marks])
             for floor in decoded['pause'] for mark, _, vtx, vtx_count, count, marks in floor]
check(len(soh_pause) == len(bin_pause) == 34 * 3 and len(labels) == 34,
      f'pause floors: SoH {len(soh_pause) // 3} ({len(labels)} labels), bins {len(bin_pause) // 3}')
FIRE_FLOOR = 17
check(labels[FIRE_FLOOR] == 'Fire Temple map 4', f'pause floor 17 is {labels[FIRE_FLOOR]!r}')
for floor in range(34):
    ours, theirs = draw(soh_pause, floor), draw(bin_pause, floor)
    if floor == FIRE_FLOOR:
        # make_mq.ts:401-409: count 5 -> 6 and a sixth chest, flag 0 at (46.5, -17.5).
        chest = (0, mq_bins.float_bits(46.5), mq_bins.float_bits(-17.5))
        added = [(ours[0][0], ours[0][1], ours[0][2], ours[0][3] + (chest,))] + ours[1:] if ours else None
        check(theirs == added and theirs != ours, f'pause floor 17: bins {theirs} is not SoH {ours} plus {chest}')
    else:
        check(ours == theirs, f'pause floor {floor} ({labels[floor]}): SoH {ours} != bins {theirs}')
    drawn = [item for item in ours + theirs if not isinstance(item, str)]
    check(all(vtx and vtx_count == 4 for _, vtx, vtx_count, _ in drawn),
          f'pause floor {floor}: a drawn record has no 4-vertex list')

# Minimaps: each dungeon's block of whole entries plus padding, copied over the vanilla table (mq.c:497-514).
minimap_source = (soh / 'src/overlays/misc/ovl_map_mark_data/z_map_mark_data.c').read_text(encoding='utf-8')


def per_dungeon(table):
    names = re.findall(r'\w+', minimap_source[minimap_source.index(table + '[] = {') + len(table) + 6:].split('}')[0])
    return [records(initializer(minimap_source, name), False) for name in names]


soh_vanilla, soh_mq = per_dungeon('gMapMarkDataTableVanilla'), per_dungeon('gMapMarkDataTableMq')
headers = {h['dungeon']: h for h in mq_bins.scene_headers(scenes_bin)}
check(len(soh_vanilla) == len(soh_mq) == 10, 'SoH minimap tables do not cover dungeons 0-9')
check(all(not headers[d]['minimapSize'] for d in (10, 11)), 'GTG or Ganon has an MQ minimap block')
JABU, FIRE, FIRE_ROOM = 2, 4, 3
for scene in decoded['scenes'][:10]:
    dungeon, name = scene['dungeon'], mq_bins.DUNGEONS[scene['dungeon']]
    entries = len(scene['minimap'])
    size, offset = headers[dungeon]['minimapSize'], headers[dungeon]['minimapOffset']
    padding = scenes_bin[offset + entries * 0x72:offset + size]
    check(size % 16 == 0 and len(padding) < 16 and not any(padding), f'{name}: minimap block {size} bytes, '
          f'padding {padding.hex()}')
    vanilla = soh_vanilla[dungeon]
    if dungeon == JABU:
        check(not padding and len(vanilla) == (entries + 1) * 3, f'Jabu: {entries} MQ and {len(vanilla) // 3} vanilla '
              'entries; OoTMM keeps vanilla entry 16')
        check(vanilla[entries * 3][0] == NONE and soh_mq[dungeon][entries * 3][0] == NONE,
              'Jabu minimap 16 is not NONE in both the vanilla table and SoH\'s MQ table')
    else:
        check(len(vanilla) == entries * 3, f'{name}: {entries} MQ and {len(vanilla) // 3} vanilla entries, so '
              'OoTMM\'s padding lands on a vanilla entry')
    copied = [(mark, True, 0, count, [list(point) for point in marks]) for entry in scene['minimap']
              for mark, count, marks in entry] + vanilla[entries * 3:]
    check(len(copied) == len(soh_mq[dungeon]), f'{name}: OoTMM has {len(copied) // 3} minimaps, SoH MQ '
          f'{len(soh_mq[dungeon]) // 3}')
    for room in range(len(copied) // 3):
        ours, theirs = draw(soh_mq[dungeon], room), draw(copied, room)
        if (dungeon, room) == (FIRE, FIRE_ROOM):
            # make_mq.ts:285-294: chest count 1, flag 0 at (72, 60), then NONE.
            check(ours == [] and theirs == [(0, True, 0, ((0, 72, 60),))],
                  f'Fire minimap 3: SoH {ours}, bins {theirs}')
        else:
            check(ours == theirs, f'{name} minimap {room}: SoH {ours} != OoTMM {theirs}')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: SoH\'s MQ pause maps and minimaps draw what the OoTMM v32.3 bins draw, except OoTMM\'s two Fire Temple '
      'chests (pause floor 17, minimap 3); Jabu-Jabu minimap 16 is NONE in both')
