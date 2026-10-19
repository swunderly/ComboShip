"""SoH's own Master Quest tables against the bundled OoTMM v32.3 bins (MQ port unit U2, tests only).

SoH's randomizer was written against the MQ archive. Every check here decodes combo/mq/data with mq_bins.py and reads
this checkout's SoH sources, so it shows the OoTMM data places what SoH's location, silver rupee, door and Wallmaster
tables expect. Each location is matched the way its runtime hook identifies it (file:line at each rule), and every
placed actor of a shuffled kind must be claimed by exactly one location.
- Chests (En_Box params), gold skulltulas (En_Sw params as z_en_sw.c:224-232 rewrites them, the token's params),
  collectables (En_Item00 params), scrubs (En_Shopnuts params) and the gossip stone: every MQ row matches the bins;
  the Forest boss key chest is spawned by Bg_Mori_Hineri (z_bg_mori_hineri.c:175).
- Position-keyed rows (pots, crates, grass, rocks, icicles, red ice, freestanding items, signs, song and stone fairies)
  match an actor of their runtime id at (x, z); dungeon wonder items match (room, actor index); fountain fairies need
  one fairy spawner; the cow needs a cow. The ShuffleBeehives table has no MQ rows. Wonder items that never drop
  (tag points, proximity switches) are no location, and SoH leaves out four grass and two DC wonder items by name.
- The six broken actors OoTMM removes (mq.c:70-77) are claimed by no location, and each one's object is missing from
  its room, so SoH never spawns them either.
- Silver rupees: SilverRupeeState.h's MasterQuest table equals OoTMM v32.3 src/common/sr.c, and the bins' rupees,
  through SoH's PhysicalId and SilverRupeeData.inc, give each puzzle that many rupees and one tracker with its flag.
- Locked doors: the switch flags of locked En_Door and Door_Shutter transition actors equal the MQ door flags
  dungeon.cpp gives a bundled seed in every dungeon. For Water that is kBundledMqWaterDoorFlags {2, 21}, while the
  MQDoorFlags row keeps {4, 21} for a real oot-mq.o2r, whose door FixWaterMQLock.cpp moves to switch 4.
- Wallmasters (C17): En_Wallmas rooms per dungeon.
- Sheik in Ice Cavern: the Serenade Sheik is in a main room list beside the iron boots chest the rando hooks read, so
  the check survives OoTMM dropping Ice Cavern's alternate setup 4.
"""
from pathlib import Path
from collections import Counter, defaultdict
import argparse
import hashlib
import os
import re
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default='')
args = parser.parse_args()
repo = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import hostcxx  # noqa: E402
import mq_bins  # noqa: E402

soh = repo / 'soh'
rando = soh / 'soh/Enhancements/randomizer'
actors_dir = soh / 'src/overlays/actors'
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def read(path):
    return Path(path).read_text(encoding='utf-8')


def table(path, macro, field):
    """Enum name -> index from a /* 0x.. */ DEFINE_... table."""
    out = {}
    for number, body in re.findall(r'/\* (0x[0-9A-F]+) \*/ ' + macro + r'(?:_\w+)?\(([^)]*)\)', read(path)):
        names = [part.strip() for part in body.split(',')]
        out[names[field] if len(names) > field else names[0]] = int(number, 16)
    return out


def enum_value(path, name):
    return int(re.search(r'/\*\s*(-?(?:0x)?[0-9A-Fa-f]+)\s*\*/\s*' + name + r'\b', read(path))[1], 0)


ACTOR = table(soh / 'include/tables/actor_table.h', 'DEFINE_ACTOR', 1)
actor_names = {value: name for name, value in ACTOR.items()}
OBJECT = table(soh / 'include/tables/object_table.h', 'DEFINE_OBJECT', 1)
SCENE = table(soh / 'include/tables/scene_table.h', 'DEFINE_SCENE', 2)
DUNGEON_SCENES = ['SCENE_DEKU_TREE', 'SCENE_DODONGOS_CAVERN', 'SCENE_JABU_JABU', 'SCENE_FOREST_TEMPLE',
                  'SCENE_FIRE_TEMPLE', 'SCENE_WATER_TEMPLE', 'SCENE_SPIRIT_TEMPLE', 'SCENE_SHADOW_TEMPLE',
                  'SCENE_BOTTOM_OF_THE_WELL', 'SCENE_ICE_CAVERN', 'SCENE_GERUDO_TRAINING_GROUND',
                  'SCENE_INSIDE_GANONS_CASTLE']
# ComboMqData.h DungeonFromScene and OoTMM's mqDungeonId: scenes 0-9, 0x0B and 0x0D are dungeons 0-11.
assert [SCENE[name] for name in DUNGEON_SCENES] == list(range(10)) + [0x0B, 0x0D]
DUNGEON_OF = {name: index for index, name in enumerate(DUNGEON_SCENES)}
NAMES = mq_bins.DUNGEONS
FOREST, WATER, SPIRIT, ICE, GANON = 3, 5, 6, 9, 11

decoded = mq_bins.decode(*mq_bins.load(repo / 'combo/mq/data'))
room_data = {(room['dungeon'], room['room']): room for room in decoded['rooms']}
scene_data = {scene['dungeon']: scene for scene in decoded['scenes']}
placed = defaultdict(list)  # dungeon -> [(room, index, id, x, y, z, params)]
for room in decoded['rooms']:
    for index, (actor, x, y, z, _, _, _, params) in enumerate(room['actors']):
        placed[room['dungeon']].append((room['room'], index, actor & 0xFFFF, x, y, z, params & 0xFFFF))


def actors_of(dungeon, *ids):
    wanted = {ACTOR[name] for name in ids}
    return [entry for entry in placed[dungeon] if entry[2] in wanted]


# --- Location rows -------------------------------------------------------------------------------------------------

def split_args(text):
    out, depth, current, quoted = [], 0, '', False
    for char in text:
        if quoted:
            quoted = char != '"'
        elif char == '"':
            quoted = True
        elif char in '({[':
            depth += 1
        elif char in ')}]':
            depth -= 1
        elif char == ',' and depth == 0:
            out.append(current.strip())
            current = ''
            continue
        current += char
    return out + [current.strip()] if current.strip() else out


def calls(text, pattern):
    """(match, argument list) for each call whose name and opening parenthesis match pattern."""
    for match in re.finditer(pattern, text):
        depth, at = 1, match.end()
        while depth:
            depth += {'(': 1, ')': -1}.get(text[at], 0)
            at += 1
        yield match, split_args(text[match.end():at - 1])


# Each Location kind's actor, from its constructor in location.cpp (None: the row passes it).
location_cpp = read(rando / 'location.cpp')
KIND_ACTOR = {}
for match in re.finditer(r'Rando::Location Rando::Location::(\w+)\(', location_cpp):
    body = location_cpp[match.end():location_cpp.index('\n}', match.end())]
    fixed = re.findall(r'\b(ACTOR_[A-Z0-9_]+)\b', body)
    KIND_ACTOR.setdefault(match[1], fixed[0] if fixed else None)


def number(text):
    pair = re.fullmatch(r'TWO_ACTOR_PARAMS\(\s*(-?\w+)\s*,\s*(-?\w+)\s*\)', text)
    return (int(pair[1], 0), int(pair[2], 0)) if pair else int(text, 0)


rows = []
for path in [rando / 'location_list.cpp'] + sorted(rando.glob('Shuffle*.cpp')):
    for match, row in calls(read(path), r'locationTable\[(RC_\w+)\]\s*=\s*Location::(\w+)\('):
        if row[1] not in ('RCQUEST_MQ', 'RCQUEST_BOTH'):
            continue
        rest = [part for part in row[2:] if not part.startswith(('RCTYPE_', 'RCAREA_'))]
        actor = rest.pop(0) if rest[0].startswith('ACTOR_') else KIND_ACTOR[match[2]]
        if match[2] == 'Sign':
            actor = next(part for part in row[2:] if part.startswith('ACTOR_'))
        scene = rest.pop(0)
        params = number(rest[0]) if rest and not rest[0].startswith('"') else None
        if scene in DUNGEON_OF:
            rows.append(dict(rc=match[1], kind=match[2], quest=row[1], actor=actor, dungeon=DUNGEON_OF[scene],
                             params=params, file=path.name))
        else:
            check(row[1] != 'RCQUEST_MQ', f'{match[1]}: an MQ row outside the 12 dungeon scenes ({scene})')
check(len([row for row in rows if row['quest'] == 'RCQUEST_MQ']) > 900, 'too few MQ rows parsed')
check(not any(row['file'] == 'ShuffleBeehives.cpp' for row in rows), 'ShuffleBeehives.cpp has dungeon rows now')


def sw_params(params):
    """z_en_sw.c:224-232: the params a list-spawned En_Sw (and the token it drops) ends up with."""
    if params & 0x8000:
        params = (params & 0x1FFF) | ((((params - 0x8000) & 0xE000) >> 13) + 1) << 13
    if (params & 0xE000) >> 13:
        params = (params & 0xE0FF) | ((((params & 0x1F00) >> 8) - 1) & 0x1F) << 8
    return params


sw_source = read(actors_dir / 'ovl_En_Sw/z_en_sw.c')
check(hostcxx.contains(sw_source, 'phi_v0 = (((thisx->params - 0x8000) & 0xE000) >> 0xD) + 1;') and
      hostcxx.contains(sw_source, 'phi_v0 = ((thisx->params & 0x1F00) >> 8) - 1;'), 'z_en_sw.c params rewrite changed')
FAIRY_SPAWNER = enum_value(actors_dir / 'ovl_En_Elf/z_en_elf.h', 'FAIRY_SPAWNER')

# Runtime identity of each row kind: the placed actors that produce it and how their key is formed.
XZ_ACTORS = {  # location actor -> placed actors keyed by (s16)pos.x, (s16)pos.z
    'ACTOR_OBJ_TSUBO': ['ACTOR_OBJ_TSUBO'],  # ShufflePots.cpp:137
    'ACTOR_OBJ_KIBAKO2': ['ACTOR_OBJ_KIBAKO2'],  # ShuffleCrates.cpp:271
    'ACTOR_OBJ_KIBAKO': ['ACTOR_OBJ_KIBAKO'],  # ShuffleCrates.cpp:283 (home.pos; list actors start there)
    'ACTOR_EN_KUSA': ['ACTOR_EN_KUSA'],  # ShuffleGrass.cpp:194
    'ACTOR_EN_ISHI': ['ACTOR_EN_ISHI', 'ACTOR_OBJ_BOMBIWA', 'ACTOR_OBJ_HAMISHI'],  # ShuffleRocks.cpp:232-287
    'ACTOR_BG_ICE_TURARA': ['ACTOR_BG_ICE_TURARA'],  # ShuffleIcicles.cpp:125
    'ACTOR_BG_ICE_SHELTER': ['ACTOR_BG_ICE_SHELTER'],  # ShuffleRedIce.cpp:162
    'ACTOR_EN_ITEM00': ['ACTOR_EN_ITEM00'],  # ShuffleFreestanding.cpp:45
    'ACTOR_EN_KANBAN': ['ACTOR_EN_KANBAN'],  # ShuffleSigns.cpp:122, 163
    'ACTOR_EN_WONDER_TALK2': ['ACTOR_EN_WONDER_TALK2'],  # ShuffleSigns.cpp:181
}
claims = Counter()  # (dungeon, room, index) -> rows claiming that placed actor
matched = {}
for row in rows:
    dungeon, params, kind = row['dungeon'], row['params'], row['kind']
    if kind == 'GSToken':
        hits = [a for a in actors_of(dungeon, 'ACTOR_EN_SW') if sw_params(a[6]) == params & 0xFFFF]
    elif kind == 'Base' and row['actor'] == 'ACTOR_EN_DNS':
        # hook_handlers.cpp:1929 identifies the hiding En_Shopnuts by its params; :1224 reads 0x06 as 0x03.
        hits = [a for a in actors_of(dungeon, 'ACTOR_EN_SHOPNUTS') if (3 if a[6] == 6 else a[6]) == params]
    elif kind == 'Base' and row['actor'] == 'ACTOR_EN_COW':
        hits = [a for a in actors_of(dungeon, 'ACTOR_EN_COW') if a[6] == 0][:1]  # ShuffleCows.cpp:44-48: key 0
    elif kind == 'Base' and row['actor'] == 'ACTOR_ID_MAX':
        continue  # RC_SHEIK_IN_ICE_CAVERN, checked below
    elif kind == 'OtherHint':
        continue  # boss key hints and Sheik: hook_handlers.cpp:3147-3161 spawns that Sheik, no room data
    elif kind == 'HintStone':
        # randomizer.cpp:809 maps the MQ Dodongo's Cavern stone (params 15892) to the shared row.
        want = 15892 if row['rc'] == 'RC_DODONGOS_CAVERN_GOSSIP_STONE' else params
        hits = [a for a in actors_of(dungeon, 'ACTOR_EN_GS') if a[6] == want]
    elif kind == 'WonderItem':
        # ShuffleWonderItems.cpp:137-141: in dungeons the key is (room, index in the room's actor list).
        hits = [a for a in actors_of(dungeon, 'ACTOR_EN_WONDER_ITEM') if (a[0], a[1]) == params]
    elif kind == 'FountainFairy':
        # ShuffleFairies.cpp:128-133: one FAIRY_SPAWNER En_Elf spawns indices 0-7.
        hits = [a for a in actors_of(dungeon, 'ACTOR_EN_ELF') if a[6] == FAIRY_SPAWNER]
        check(len(hits) == 1 and 0 <= params < 8, f'{row["rc"]}: fountain spawners {hits}')
        matched[row['rc']] = hits
        continue
    elif kind in ('StoneFairy', 'SongFairy'):
        # ShuffleFairies.cpp:190 keys the gossip stone's z, :164 the Shot_Sun's z.
        source = 'ACTOR_EN_GS' if kind == 'StoneFairy' else 'ACTOR_SHOT_SUN'
        hits = [a for a in actors_of(dungeon, source) if a[5] == params[1]]
        check(len(hits) == 1, f'{row["rc"]}: {source} at z {params[1]}: {hits}')
        matched[row['rc']] = hits
        continue
    elif isinstance(params, tuple):
        check(row['actor'] in XZ_ACTORS, f'{row["rc"]}: no identity rule for {kind} {row["actor"]}')
        hits = [a for a in actors_of(dungeon, *XZ_ACTORS.get(row['actor'], [])) if (a[3], a[5]) == params]
    elif row['actor'] == 'ACTOR_EN_BOX' and row['rc'] == 'RC_FOREST_TEMPLE_MQ_BOSS_KEY_CHEST':
        spawner = read(actors_dir / 'ovl_Bg_Mori_Hineri/z_bg_mori_hineri.c')
        check(hostcxx.contains(spawner, 'ACTOR_EN_BOX, -1515.0f, 1440.0f, -3475.0f, -0x4000, 0x4000, 0, 0x27EE') and
              params == 0x27EE, 'Forest boss key chest is no longer Bg_Mori_Hineri\'s')
        hits = actors_of(dungeon, 'ACTOR_BG_MORI_HINERI')
        check(hits, 'MQ Forest has no Bg_Mori_Hineri to spawn the boss key chest')
        matched[row['rc']] = hits
        continue
    elif kind in ('Chest', 'Collectable'):
        hits = [a for a in actors_of(dungeon, row['actor']) if a[6] == params & 0xFFFF]
    else:
        check(False, f'{row["rc"]}: no identity rule for {kind} {row["actor"]}')
        continue
    matched[row['rc']] = hits
    check(len(hits) >= 1, f'{row["rc"]} ({kind}, {row["quest"]}, {params}): nothing in the MQ bins')
    check(len({(a[2], a[6]) for a in hits}) <= 1 and (kind == 'Chest' or len(hits) <= 1),
          f'{row["rc"]}: several actors match {hits}')
    for hit in hits:
        claims[(dungeon, hit[0], hit[1])] += 1

# Every placed actor of a shuffled kind is claimed by exactly one row; En_Box once per params (a chest may be listed in
# two rooms). Unclaimed: the broken DC grass (below) and two DT room 8 grass ~1060 units under the room's floor that
# SoH's MQ grass table leaves out.
UNLISTED_GRASS = {(0, -2732, -674), (0, -2489, -753), (1, 1713, -1246), (1, 3189, -485)}
# A wonder item is a location only in a mode whose update function drops something (EnWonderItem_DropCollectible or
# the VB_WONDER_DROP_ITEM hook, ShuffleWonderItems.cpp:302); tag points and proximity switches never do. DC room 4's two
# bomb-triggered magic drops (params 0x18DB, 0x18DE) are left out of SoH's table, as of OoTMM v32.3's pool.
wonder_source = read(actors_dir / 'ovl_En_Wonder_Item/z_en_wonder_item.c')
wonder_init = wonder_source[wonder_source.index('\nvoid EnWonderItem_Init(Actor* thisx, PlayState* play) {'):]
wonder_init = wonder_init[:wonder_init.index('\n}\n')]
wonder_drops = {name for name, body in re.findall(r'\nvoid (EnWonderItem_\w+)\(EnWonderItem\* this, PlayState\* play\) '
                                                  r'\{(.*?)\n\}\n', wonder_source, re.S)
                if 'EnWonderItem_DropCollectible(' in body or 'VB_WONDER_DROP_ITEM' in body}
WONDER_DROP_MODES = set()
for mode, body in re.findall(r'case (WONDERITEM_\w+):(.*?)break;', wonder_init, re.S):
    update = re.search(r'this->updateFunc = (EnWonderItem_\w+);', body)
    if update and update[1] in wonder_drops:
        WONDER_DROP_MODES.add(enum_value(actors_dir / 'ovl_En_Wonder_Item/z_en_wonder_item.h', mode))
check(WONDER_DROP_MODES == {0, 2, 3, 5, 8, 9}, f'wonder item modes that drop changed: {sorted(WONDER_DROP_MODES)}')
UNLISTED_WONDER = {(1, 4, 3), (1, 4, 4)}  # (dungeon, room, index)
shuffled = ['ACTOR_OBJ_TSUBO', 'ACTOR_OBJ_KIBAKO2', 'ACTOR_OBJ_KIBAKO', 'ACTOR_EN_KUSA', 'ACTOR_EN_ISHI',
            'ACTOR_OBJ_BOMBIWA', 'ACTOR_OBJ_HAMISHI', 'ACTOR_BG_ICE_TURARA', 'ACTOR_BG_ICE_SHELTER', 'ACTOR_EN_ITEM00',
            'ACTOR_EN_WONDER_ITEM', 'ACTOR_EN_SHOPNUTS']
counted = Counter()
for dungeon in range(12):
    for room, index, actor, x, _, z, params in actors_of(dungeon, *shuffled):
        unlisted = (actor == ACTOR['ACTOR_EN_KUSA'] and (dungeon, x, z) in UNLISTED_GRASS) or (
            actor == ACTOR['ACTOR_EN_WONDER_ITEM'] and
            ((params >> 11) & 0x1F not in WONDER_DROP_MODES or (dungeon, room, index) in UNLISTED_WONDER))
        if unlisted:  # z_en_wonder_item.c:125: the mode is params >> 11
            check(not claims[(dungeon, room, index)], f'{NAMES[dungeon]} room {room} actor {index} (0x{actor:04X} '
                  f'params 0x{params:04X} at {x}, {z}) is listed now')
            counted['unlisted'] += 1
            continue
        counted[actor] += 1
        check(claims[(dungeon, room, index)] == 1, f'{NAMES[dungeon]} room {room} actor {index} (0x{actor:04X} '
              f'params 0x{params:04X} at {x}, {z}) is claimed by {claims[(dungeon, room, index)]} rows')
    for room, index, actor, _, _, _, params in actors_of(dungeon, 'ACTOR_EN_SW', 'ACTOR_EN_BOX'):
        if actor == ACTOR['ACTOR_EN_SW'] and not sw_params(params) & 0xE000:
            continue  # z_en_sw.c: type 0 is a plain Skullwalltula, types 1-3 gold skulltulas
        counted['chest' if actor == ACTOR['ACTOR_EN_BOX'] else 'gold skulltula'] += 1
        check(claims[(dungeon, room, index)] == 1, f'{NAMES[dungeon]} room {room} actor {index} (0x{actor:04X} '
              f'params 0x{params:04X}) is claimed by {claims[(dungeon, room, index)]} rows')
unlisted = counted.pop('unlisted', 0)
print(f'Placed MQ actors each claimed by one location ({unlisted} unlisted): ' +
      ', '.join(f'{actor_names.get(kind, kind)} {n}' for kind, n in sorted(counted.items(), key=lambda item: -item[1])))
check((counted['chest'], counted['gold skulltula'], counted[ACTOR['ACTOR_OBJ_TSUBO']], unlisted) == (126, 44, 199, 26),
      f'placed actor counts changed: {dict(counted)}, {unlisted} unlisted')

# --- Broken actors -------------------------------------------------------------------------------------------------
# OoTMM mq.c:70-77 sets these actors' ids to 0xFFFF (dungeon, room, index).
BROKEN = [(1, 0x9, 0x0), (1, 0xa, 0x6), (2, 0x7, 0x1), (6, 0x1, 0x2), (6, 0x1, 0x3), (6, 0x17, 0x0)]
EXPECTED_OBJECT = ['OBJECT_KUSA', 'OBJECT_KUSA', 'OBJECT_RU1', 'OBJECT_FIREFLY', 'OBJECT_FIREFLY', 'OBJECT_ST']
overlay_of = {}
for name, enum in re.findall(r'DEFINE_ACTOR\((\w+), (ACTOR_\w+),', read(soh / 'include/tables/actor_table.h')):
    overlay_of[enum] = actors_dir / f'ovl_{name}'
for (dungeon, room, index), expected in zip(BROKEN, EXPECTED_OBJECT):
    actor, x, _, z, _, _, _, params = room_data[(dungeon, room)]['actors'][index]
    name = actor_names[actor & 0xFFFF]
    source = ''.join(read(path) for path in sorted(overlay_of[name].glob('*.c')))
    by_params = re.search(r'sObjectIds\[\] = \{([^}]*)\}', source)
    if by_params and hostcxx.contains(source, 'sObjectIds[thisx->params & 3]'):  # En_Kusa picks by params & 3
        needed = [part.strip() for part in by_params[1].split(',')][params & 3]
    else:
        needed = re.search(r'InitVars = \{\s*' + name + r',\s*\w+,\s*[^,]+,\s*(OBJECT_\w+)', source)[1]
    check(needed == expected, f'{NAMES[dungeon]} room {room} broken actor {index} ({name}) needs {needed}')
    check(OBJECT[needed] not in room_data[(dungeon, room)]['objects'],
          f'{NAMES[dungeon]} room {room}: broken actor {index} has its object {needed}')
    check(not claims[(dungeon, room, index)], f'{NAMES[dungeon]} room {room}: broken actor {index} is a location')
    keyed = [row['rc'] for row in rows if row['dungeon'] == dungeon and isinstance(row['params'], tuple) and
             (row['params'] == (x, z) or (row['kind'] == 'WonderItem' and row['params'] == (room, index)))]
    check(not keyed, f'{NAMES[dungeon]} room {room}: locations at broken actor {index}: {keyed}')

# --- Sheik in Ice Cavern -------------------------------------------------------------------------------------------
serenade = enum_value(actors_dir / 'ovl_En_Xc/z_en_xc.h', 'SHEIK_TYPE_SERENADE')
sheiks = [a for a in actors_of(ICE, 'ACTOR_EN_XC') if a[6] == serenade]
boots = [a for a in actors_of(ICE, 'ACTOR_EN_BOX') if a[6] & 0x1F == 0x02]
hooks = read(rando / 'hook_handlers.cpp')
check(len(sheiks) == 1 and len(boots) == 1 and sheiks[0][0] == boots[0][0],
      f'MQ Ice Cavern: Serenade Sheik {sheiks} and chest 0x02 {boots} are not one main-list room')
check(all(hostcxx.contains(hooks, f'case {hook}') for hook in ('VB_SHIEK_PREPARE_TO_GIVE_SERENADE_OF_WATER',
                                                                'VB_BE_ELIGIBLE_FOR_SERENADE_OF_WATER')) and
      hooks.count('Flags_GetTreasure(gPlayState, 0x2)') >= 2, 'the Serenade hooks no longer read chest flag 0x02')
check(any(row['rc'] == 'RC_SHEIK_IN_ICE_CAVERN' and row['quest'] == 'RCQUEST_BOTH' for row in rows) and
      matched.get('RC_ICE_CAVERN_MQ_IRON_BOOTS_CHEST') == boots, 'Sheik in Ice Cavern rows changed')

# --- Silver rupees -------------------------------------------------------------------------------------------------
silver_dir = rando / 'silver_rupees'
dumper = work / 'mq_silver_dump.cpp'
dumper.write_text('''#include "SilverRupeeState.h"
#include <cstdio>
int main() {
    using namespace Rando::SilverRupees;
    for (size_t slot = 0; slot < PuzzleCount; ++slot) {
        const auto mq = GetPuzzle(slot, true);
        std::printf("puzzle %u %u %u %u\\n", unsigned(slot), unsigned(mq.scene), unsigned(mq.flag), unsigned(mq.count));
    }
    for (unsigned scene = 0; scene < 0x20; ++scene)
        for (int room = 0; room < 0x20; ++room)
            for (unsigned local = 0; local < 0x10; ++local)
                if (const auto id = PhysicalId(static_cast<uint8_t>(scene), room, local, true))
                    std::printf("id %u %d %u %u\\n", scene, room, local, unsigned(*id));
}
''', encoding='utf-8')
exe = hostcxx.build(args.compiler or hostcxx.DEFAULT_COMPILER, dumper, includes=[silver_dir])
dump = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT, check=True).stdout
master_quest = {}
physical = {}
for line in dump.splitlines():
    kind, *values = line.split()
    values = [int(value) for value in values]
    if kind == 'puzzle':
        master_quest[values[0]] = tuple(values[1:])
    else:
        physical[tuple(values[:3])] = values[3]

# OoTMM v32.3 src/common/sr.c:9-28 (scene, flag, count per puzzle) with SilverRupees_Init's MQ overrides (:38-84).
SR_SCENES = {'SCE_OOT_DODONGO_CAVERN': 'SCENE_DODONGOS_CAVERN', 'SCE_OOT_BOTTOM_OF_THE_WELL': 'SCENE_BOTTOM_OF_THE_WELL',
             'SCE_OOT_TEMPLE_SPIRIT': 'SCENE_SPIRIT_TEMPLE', 'SCE_OOT_TEMPLE_SHADOW': 'SCENE_SHADOW_TEMPLE',
             'SCE_OOT_ICE_CAVERN': 'SCENE_ICE_CAVERN', 'SCE_OOT_GERUDO_TRAINING_GROUND': 'SCENE_GERUDO_TRAINING_GROUND',
             'SCE_OOT_INSIDE_GANON_CASTLE': 'SCENE_INSIDE_GANONS_CASTLE'}
OOTMM_SR = [('SCE_OOT_DODONGO_CAVERN', 0x00, 0), ('SCE_OOT_BOTTOM_OF_THE_WELL', 0x1f, 5),
            ('SCE_OOT_TEMPLE_SPIRIT', 0x05, 5), ('SCE_OOT_TEMPLE_SPIRIT', 0x0a, 5), ('SCE_OOT_TEMPLE_SPIRIT', 0x02, 5),
            ('SCE_OOT_TEMPLE_SHADOW', 0x01, 5), ('SCE_OOT_TEMPLE_SHADOW', 0x00, 0), ('SCE_OOT_TEMPLE_SHADOW', 0x09, 5),
            ('SCE_OOT_TEMPLE_SHADOW', 0x08, 5), ('SCE_OOT_ICE_CAVERN', 0x08, 5), ('SCE_OOT_ICE_CAVERN', 0x09, 5),
            ('SCE_OOT_GERUDO_TRAINING_GROUND', 0x1c, 5), ('SCE_OOT_GERUDO_TRAINING_GROUND', 0x0c, 5),
            ('SCE_OOT_GERUDO_TRAINING_GROUND', 0x1b, 5), ('SCE_OOT_INSIDE_GANON_CASTLE', 0x0b, 5),
            ('SCE_OOT_INSIDE_GANON_CASTLE', 0x12, 5), ('SCE_OOT_INSIDE_GANON_CASTLE', 0x09, 5),
            ('SCE_OOT_INSIDE_GANON_CASTLE', 0x0e, 5)]
OOTMM_SR_MQ = {0x00: (5, 0x25), 0x01: (0, 0), 0x02: (5, 0x37), 0x03: (5, 0x00), 0x04: (0, 0), 0x06: (10, 0x03),
               0x07: (5, 0x11), 0x08: (10, 0x08), 0x09: (0, 0), 0x0a: (0, 0), 0x0b: (5, 0x1c), 0x0c: (6, 0x0c),
               0x0d: (3, 0x1b), 0x0e: (5, 0x0b), 0x0f: (5, 0x02), 0x10: (5, 0x01), 0x11: (0, 0)}  # slot: count, flag
SR_C_BLOB = 'ca7a419729dbc5a19c90f940bd2908e89d55a107'  # git blob of src/common/sr.c at v32.3 (413573d2)
reference = os.environ.get('COMBO_OOTMM_REFERENCE')
reference = Path(reference) if reference else next(
    (parent / 'references/ootmm-stable-32.3' for parent in repo.parents
     if (parent / 'references/ootmm-stable-32.3/packages/generator/src/common/sr.c').is_file()), None)
sr_c = reference / 'packages/generator/src/common/sr.c' if reference else None
if sr_c and sr_c.is_file():
    blob = sr_c.read_bytes().replace(b'\r\n', b'\n')  # a core.autocrlf checkout; git hashes the LF form
    if hashlib.sha1(b'blob %d\0' % len(blob) + blob).hexdigest() == SR_C_BLOB:
        text = blob.decode('utf-8')
        check(re.findall(r'\{ (SCE_OOT_\w+),\s*(0x[0-9a-f]+), (\d+) \}', text) ==
              [(scene, f'0x{flag:02x}', str(count)) for scene, flag, count in OOTMM_SR], 'sr.c defaults differ')
        slots = {name: int(value, 16) for name, value in re.findall(r'#define (SR_\w+)\s+(0x[0-9a-f]+)',
                                                                     read(reference / 'packages/generator/include/combo/sr.h'))}
        overrides = {slots[slot]: (int(count), int(flag, 0)) for slot, count, flag in
                     re.findall(r'SilverRupees_SetData\((SR_\w+),\s*(\d+),\s*(0x[0-9a-f]+|\d+)\)', text)}
        check(overrides == OOTMM_SR_MQ, f'sr.c MQ overrides differ: {overrides}')
        print(f'OoTMM sr.c read from {sr_c}')
    else:
        print(f'{sr_c} is not the v32.3 blob; the pinned table here was not re-read')
else:
    print('No OoTMM v32.3 checkout here; the pinned sr.c table was not re-read (set COMBO_OOTMM_REFERENCE)')
ootmm_mq = {}
for slot, (scene, flag, count) in enumerate(OOTMM_SR):
    count, flag = OOTMM_SR_MQ.get(slot, (count, flag))
    ootmm_mq[slot] = (SCENE[SR_SCENES[scene]], flag, count)
check(master_quest == ootmm_mq, f'SilverRupeeState.h MasterQuest {master_quest} != OoTMM v32.3 {ootmm_mq}')

data_inc = read(silver_dir / 'SilverRupeeData.inc')
slot_of = {item: int(slot) for item, slot in re.findall(r'SR_ITEM\((RG_\w+), (\d+),', data_inc)}
sr_checks = {int(rid, 16): (rc, item, scene) for rc, rid, item, scene in re.findall(
    r'SR_CHECK\((RC_\w+), \w+, (0x[0-9a-f]+), (RG_\w+), (SCENE_\w+), RCQUEST_MQ,', data_inc)}
tracker_type = enum_value(actors_dir / 'ovl_En_G_Switch/z_en_g_switch.h', 'ENGSWITCH_SILVER_TRACKER')
rupee_type = enum_value(actors_dir / 'ovl_En_G_Switch/z_en_g_switch.h', 'ENGSWITCH_SILVER_RUPEE')
rupees, trackers, hit_ids = Counter(), defaultdict(list), Counter()
for dungeon in range(12):
    scene = SCENE[DUNGEON_SCENES[dungeon]]
    for room in sorted(r for d, r in room_data if d == dungeon):
        local, slots = 0, set()  # SilverRupees.cpp:211-215: roomSpawnIndex counts this room's rupees
        for actor, *_, params in room_data[(dungeon, room)]['actors']:
            params &= 0xFFFF
            if actor & 0xFFFF != ACTOR['ACTOR_EN_G_SWITCH'] or (params >> 12) & 0xF != rupee_type:
                continue
            rid = physical.get((scene, room, local))
            local += 1
            row = sr_checks.get(rid)
            if check(row is not None and row[2] == DUNGEON_SCENES[dungeon],
                     f'{NAMES[dungeon]} room {room} silver rupee {local - 1}: physical id {rid} has no MQ row'):
                rupees[slot_of[row[1]]] += 1
                hit_ids[rid] += 1
                slots.add(slot_of[row[1]])
        for actor, *_, params in room_data[(dungeon, room)]['actors']:
            params &= 0xFFFF
            if actor & 0xFFFF == ACTOR['ACTOR_EN_G_SWITCH'] and (params >> 12) & 0xF == tracker_type:
                # z_en_g_switch.c:88, 100: switch flag params & 0x3F, rupee count params >> 6.
                check(len(slots) == 1, f'{NAMES[dungeon]} room {room}: tracker beside puzzles {slots}')
                for slot in slots:
                    trackers[slot].append((params & 0x3F, params >> 6))
check(set(hit_ids) == set(sr_checks) and set(hit_ids.values()) == {1},
      f'MQ silver rupee rows without exactly one rupee in the bins: '
      f'{sorted(set(sr_checks) - set(hit_ids))} {[i for i, n in hit_ids.items() if n != 1]}')
for slot, (_, flag, count) in master_quest.items():
    check(rupees[slot] == count, f'silver puzzle {slot}: {rupees[slot]} rupees in the bins, MasterQuest {count}')
    check(trackers[slot] == ([(flag, count)] if count else []),
          f'silver puzzle {slot}: trackers {trackers[slot]}, MasterQuest flag 0x{flag:02x} count {count}')

# --- Locked doors --------------------------------------------------------------------------------------------------
locked_door = enum_value(actors_dir / 'ovl_En_Door/z_en_door.h', 'DOOR_LOCKED')
locked_shutter = enum_value(actors_dir / 'ovl_Door_Shutter/z_door_shutter.h', 'SHUTTER_KEY_LOCKED')
door_flags = {}
for _, info in calls(read(rando / 'dungeon.cpp'), r'=\s*DungeonInfo\('):
    if info[-4] in DUNGEON_OF:  # ..., scene, vanillaDoorFlags, randoDoorFlags, mqDoorFlags
        door_flags[DUNGEON_OF[info[-4]]] = {int(flag) for flag in re.findall(r'\d+', info[-1])}
check(len(door_flags) == 12, f'dungeon.cpp door flags for {sorted(door_flags)}')
# A bundled seed's MQ Water flags (U6): dungeon.cpp's GetDoorFlags returns kBundledMqWaterDoorFlags when
# ResourceMgr_SceneUsesBundledMq, because OoTMM's make_mq.ts:226-229 makes transition 0x16 switch 2. The MQDoorFlags
# row keeps {4, 21} for a real oot-mq.o2r, whose door FixWaterMQLock.cpp moves from switch 3 to 4 (22659 -> 22660).
dungeon_cpp = read(rando / 'dungeon.cpp')
bundled_water = re.search(r'kBundledMqWaterDoorFlags = \{([^}]*)\}', dungeon_cpp)
check(bundled_water and hostcxx.contains(dungeon_cpp, 'if (scene == SCENE_WATER_TEMPLE && '
                                                       'ResourceMgr_SceneUsesBundledMq(scene)) {'
                                                       'return &kBundledMqWaterDoorFlags;'),
      'dungeon.cpp: GetDoorFlags must give a bundled MQ Water seed kBundledMqWaterDoorFlags')
bundled_door_flags = dict(door_flags)
if bundled_water:
    bundled_door_flags[WATER] = {int(flag) for flag in re.findall(r'\d+', bundled_water[1])}
for dungeon in range(12):
    locked = set()
    for transition in scene_data[dungeon]['transitions']:
        actor, params = transition[4] & 0xFFFF, transition[9] & 0xFFFF
        if actor == ACTOR['ACTOR_EN_DOOR'] and (params >> 7) & 7 == locked_door:  # z_en_door.c:158, 168
            locked.add(params & 0x3F)
        elif actor == ACTOR['ACTOR_DOOR_SHUTTER'] and (params >> 6) & 0xF == locked_shutter:  # z_door_shutter.c:245
            locked.add(params & 0x3F)
    soh_flags = bundled_door_flags.get(dungeon)
    check(locked == soh_flags, f'{NAMES[dungeon]} locked doors: bins {sorted(locked)}, SoH {sorted(soh_flags)}')
    doors_in_rooms = actors_of(dungeon, 'ACTOR_EN_DOOR', 'ACTOR_DOOR_SHUTTER')
    check(not doors_in_rooms, f'{NAMES[dungeon]}: doors in room lists {doors_in_rooms}')
water_fix = read(soh / 'soh/Enhancements/Fixes/FixWaterMQLock.cpp')
water_door = scene_data[WATER]['transitions'][0x16]
check(hostcxx.contains(water_fix, 'enDoor->actor.params == 22659') and hostcxx.contains(water_fix, '= 22660;') and
      22659 == (0x16 << 10) | 0x0083 and water_door[4] == ACTOR['ACTOR_EN_DOOR'] and water_door[9] == 0x0082 and
      door_flags.get(WATER) == {22660 & 0x3F, 21},
      f'MQ Water transition 0x16 is {water_door}, archive MQ flags {door_flags.get(WATER)}; FixWaterMQLock.cpp no '
      'longer targets the archive\'s door or dungeon.cpp no longer counts its new switch')
check(hostcxx.contains(water_fix, '!ResourceMgr_GameUsesBundledMq() &&'),
      'FixWaterMQLock.cpp must leave a bundled MQ Water door alone (archive only)')

# --- Wallmasters (C17) ---------------------------------------------------------------------------------------------
wallmasters = Counter((NAMES[dungeon], room) for dungeon in range(12)
                      for room, *_ in actors_of(dungeon, 'ACTOR_EN_WALLMAS'))
check(wallmasters == Counter({('Forest', 19): 1, ('Forest', 20): 1, ('Spirit', 5): 2, ('Spirit', 8): 2,
                              ('Spirit', 15): 4, ('BotW', 0): 1, ('BotW', 1): 1, ('BotW', 3): 1, ('GTG', 2): 2,
                              ('Ganon', 18): 1}), f'MQ Wallmaster rooms changed: {sorted(wallmasters.items())}')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print(f'PASS: {len(rows)} MQ and shared dungeon rows match the OoTMM v32.3 bins and claim every placed actor once '
      f'({sum(counted.values())}); the six broken actors are unplaceable and unlisted; silver puzzles, locked doors '
      '(bundled MQ Water on switch 2) and Wallmaster rooms agree; the Serenade Sheik needs no alternate setup')
