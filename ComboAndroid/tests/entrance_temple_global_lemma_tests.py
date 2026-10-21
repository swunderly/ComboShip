"""Pin the lemmas of the Temple probe's GLOBAL node (Wallmaster limits design, Stage 2) to the exact OoTMM v32.3 blobs.

Stage 2 lets Wallmaster Shuffle (Own Game) run when the seed starts with an OoT warp song. OoTMM's opposite-age Temple
check (validateAgeTemple, entrances.ts:265-287) can then warp, through the virtual area GLOBAL. The probe gets a
virtual GLOBAL node that the opposite age enters at its spawn region, with Ship's six warp-song edges of Root Exits.
That is exact because of these lemmas:
- L3: Link's House and the Temple of Time, the two spawn areas, each exit to GLOBAL ("true"), so GLOBAL is reached at
  the opposite age as soon as it stands at its spawn; GLOBAL changes no age, has no time of its own (still) and its
  only exits that lead anywhere are the six SONG_TP_* (each can_play_tp_*, landing on the row OOT_WARP_SONG_*) and MM
  SOARING (can_play_soaring);
- each can_play_tp_* is the song, an ocarina and its notes, with no age term, as Ship's Root Exits edges are
  (logic->CanUse of the song, logic.cpp);
- Mask Shop: the only way from OoT into MM without Soaring is the Market's Mask Shop door (the reserved game link),
  and the Market reaches the Temple of Time through two unconditional exits that no own-game pool touches, so an age
  that reaches MM has reached the Temple first; MM's ways back to OoT (the Clock Tower link to the Market, MM's warp
  songs to the SONG_TP_* areas) come after it. With no song destination shuffle and no starting Soaring, MM can add
  no Temple path, for either age.
The recorded pinned run (tools/ootmm/temple-song-facts.json, from run_pinned_entrance_pools.mts --temple on
tools/ootmm/temple-song-fixtures.json) checks the same lemmas on OoTMM's own searches: in every case the starting age
stays at SPAWN, the opposite age reaches GLOBAL, an age in MM has the Market and the Temple, and the answer is the
songless walk from the spawn area or, when the song plays, from its landing. entrance_temple_song_facts_pinned_tests.py
reruns it.

Contract evidence only, not behaviour. Skipped without the optional checkout, and failing (never skipped) when the
checkout is at another revision.
"""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
import sys

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

SONGS = {'FOREST': 'RG_MINUET_OF_FOREST', 'FIRE': 'RG_BOLERO_OF_FIRE', 'WATER': 'RG_SERENADE_OF_WATER',
         'SHADOW': 'RG_NOCTURNE_OF_SHADOW', 'SPIRIT': 'RG_REQUIEM_OF_SPIRIT', 'LIGHT': 'RG_PRELUDE_OF_LIGHT'}
WARP = {s: rg.replace('RG_', 'RR_') + '_WARP' for s, rg in SONGS.items()}
# OoTMM's note macros and Ship's button items.
BUTTONS = {'a': 'RG_OCARINA_A_BUTTON', 'right': 'RG_OCARINA_C_RIGHT_BUTTON', 'left': 'RG_OCARINA_C_LEFT_BUTTON',
           'up': 'RG_OCARINA_C_UP_BUTTON', 'down': 'RG_OCARINA_C_DOWN_BUTTON'}


def blob(path):
    return registry.verified_blob(reference, path).decode('utf-8')  # raises on any other revision


def grep(pattern, *paths):
    """`git grep -n -E` at the pin, as (path, line, text); none is an empty list."""
    result = subprocess.run(['git', '-C', str(reference), 'grep', '-n', '-E', pattern, registry.PIN, '--', *paths],
                            capture_output=True, text=True, timeout=60)
    assert result.returncode in (0, 1), result.stderr
    rows = []
    for line in result.stdout.splitlines():
        _, path, number, text = line.split(':', 3)
        rows.append((path, int(number), text))
    return rows


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


def macros(path):
    result = {}
    for line in blob(path).splitlines():
        if m := re.fullmatch(r'"([^"]+)": (?:"([^"]*)"(?: *#.*)?|([^"#].*))', line):
            assert m.group(1) not in result, line
            result[m.group(1)] = m.group(2) if m.group(2) is not None else m.group(3).strip()
        else:
            assert not line.strip() or line.startswith('#'), f'{path}: unexpected line {line!r}'
    return result


def conjuncts(expr):
    depth, start, terms = 0, 0, []
    for i, c in enumerate(expr):
        depth += (c == '(') - (c == ')')
        if depth == 0 and expr.startswith(' && ', i):
            terms.append(expr[start:i])
            start = i + 4
    return terms + [expr[start:]]


def squash(text):
    return re.sub(r'\s+', '', text)


system = areas('data/world/oot/overworld/_system.yml')
kokiri = areas('data/world/oot/overworld/kokiri_forest.yml')
tot = areas('data/world/oot/overworld/temple_of_time.yml')
market = areas('data/world/oot/overworld/market.yml')
mm_system = areas('data/world/mm/overworld/_system.yml')
_, records = registry.read_registry(reference)
rows = {r['name']: r for r in records}

# 1. L3 (kokiri_forest.yml:4, temple_of_time.yml:4): both spawn areas exit to GLOBAL unconditionally, and the spawn
# areas are the only exits of SPAWN CHILD and SPAWN ADULT (_system.yml:11-21).
assert kokiri["Link's House"]['exits']['GLOBAL'] == 'true'
assert tot['Temple of Time']['exits']['GLOBAL'] == 'true'
assert system['SPAWN CHILD']['exits'] == {"Link's House": 'true'}
assert system['SPAWN ADULT']['exits'] == {'Temple of Time': 'true'}
# GLOBAL changes no age and has no time of its own: no `time` key is 'still' (builder.ts:332), so it takes the time
# of the areas that exit to it. Its exits: the six songs, MM SOARING and the dead end EGGS (lemma S1).
assert system['GLOBAL'] == {'region': 'NONE', 'age_change': 'false',
                            'exits': {**{f'SONG_TP_{s}': f'can_play_tp_{s.lower()}' for s in SONGS},
                                      'MM SOARING': 'can_play_soaring && setting(games, ootmm)', 'EGGS': 'true'}}
assert "const time = area.time || 'still';" in blob('packages/logic/src/world/builder.ts')
# Each SONG_TP_* (_system.yml:36-70) leads to its landing, which is the registry row OOT_WARP_SONG_* (entrances.yml:
# 174-179) that the probe binds; the six rows are the only one-way-song rows, none with a reverse.
song_rows = {r['from'][len('OOT SONG_TP_'):]: r for r in records if r['type'] == 'one-way-song'}
assert sorted(song_rows) == sorted(SONGS) and len(song_rows) == 6
for song, row in song_rows.items():
    area = system[f'SONG_TP_{song}']
    assert set(area) == {'region', 'age_change', 'exits'} and area['age_change'] == 'false'
    assert area['exits'] == {row['to'][len('OOT '):]: 'true'}, (song, area)
    assert row['game'] == 'oot' and row['reverse'] is None and row['from'] == f'OOT SONG_TP_{song}'

# 2. can_play_tp_* (macros_common.yml:40-45): the song (has_song_tp_*), an ocarina and the song's notes, and no age
# term; the notes are free unless OoT's buttons are shuffled (macros_oot.yml:17-22), and has_ocarina is any ocarina
# (macros_oot.yml:30).
common = macros('data/macros/macros_common.yml')
oot_macros = macros('data/macros/macros_oot.yml')
notes = {}
for song in SONGS:
    terms = conjuncts(common[f'can_play_tp_{song.lower()}'])
    assert terms[:2] == [f'has_song_tp_{song.lower()}', 'has_ocarina'], terms
    assert all(t.startswith('ocarina_button_') for t in terms[2:]) and not any('age' in t for t in terms), terms
    notes[song] = sorted(t[len('ocarina_button_'):] for t in terms[2:])
assert oot_macros['ocarina_button(x, y)'] == \
    'cond(setting(ocarinaButtonsShuffleOot), cond(setting(sharedOcarinaButtons), has(y), has(x)), true)'
for note, name in [('a', 'A'), ('right', 'C_RIGHT'), ('left', 'C_LEFT'), ('up', 'C_UP'), ('down', 'C_DOWN')]:
    assert oot_macros[f'ocarina_button_{note}'] == f'ocarina_button(BUTTON_{name}, SHARED_BUTTON_{name})'
assert oot_macros['has_ocarina'] == 'glitch_ocarina_items || has(OCARINA) || has(SHARED_OCARINA)'

# Ship's side: Root Exits holds the six warp-song edges (root.cpp:47-52), each logic->CanUse(<song>) evaluated in a
# SCENE_ID_MAX region; each warp region's only exit is its landing; CanUse of a warp song is HasItem(song), an
# ocarina (CanUse(RG_FAIRY_OCARINA), any ocarina) and the same notes as OoTMM's, with no age term; the notes are owned
# from the start when Ship's buttons are not shuffled (Logic::Reset).
randomizer = source/'soh/soh/Enhancements/randomizer'
root = (randomizer/'location_access/root.cpp').read_text(encoding='utf-8')
root_exits = root[root.index('areaTable[RR_ROOT_EXITS] = Region("Root Exits", SCENE_ID_MAX,'):]
root_exits = root_exits[:root_exits.index('});')]
edges = re.findall(r'ENTRANCE\((RR_\w+),\s*(.+?)\),\n', root_exits)
assert edges[:2] == [('RR_CHILD_SPAWN', 'logic->IsChild'), ('RR_ADULT_SPAWN', 'logic->IsAdult')], edges
assert edges[2:] == [(WARP[s], f'logic->CanUse({SONGS[s]})') for s in
                     ('FOREST', 'FIRE', 'WATER', 'SHADOW', 'SPIRIT', 'LIGHT')], edges
audit = json.loads((source/'tools/ootmm/oot-edge-audit.json').read_text(encoding='utf-8'))
landings = {}
for song, warp in WARP.items():
    block = root[root.index(f'areaTable[{warp}] = Region('):]
    block = block[:block.index('});')]
    assert re.search(r'Region\("[^"]+", SCENE_ID_MAX, TIME_DOESNT_PASS, \{RA_LINKS_POCKET\}, \{\}, \{\}, \{', block)
    exits = re.findall(r'ENTRANCE\((RR_\w+),\s*(.+?)\),', block)
    assert len(exits) == 1 and exits[0][1] == 'true', (warp, exits)
    landings[song] = exits[0][0]
logic_cpp = (randomizer/'logic.cpp').read_text(encoding='utf-8')
can_use = logic_cpp[logic_cpp.index('bool Logic::CanUse(RandomizerGet itemName) {'):]
can_use = can_use[:can_use.index('\n}\n')]
assert squash('if (!HasItem(itemName)) return false; if (!ItemUseAllowed(itemName)) { return false; }') in squash(can_use)
songs_block = can_use[can_use.index('// Songs'):can_use.index('// Misc. Items')]
for song, rg in SONGS.items():
    case = songs_block[songs_block.index(f'case {rg}:'):]
    body = case[case.index('return '):case.index(';')]
    terms = [t.strip() for t in body[len('return '):].split('&&')]
    assert terms[0] == 'CanUse(RG_FAIRY_OCARINA)', (rg, terms)
    assert sorted(terms[1:]) == sorted(f'HasItem({BUTTONS[n]})' for n in notes[song]), (rg, terms, notes[song])
assert squash('case RG_FAIRY_OCARINA: case RG_OCARINA_OF_TIME: return true;') in squash(can_use)
allowed = logic_cpp[logic_cpp.index('bool Logic::ItemUseAllowed(RandomizerGet itemName) {'):]
allowed = allowed[:allowed.index('\n}\n')]
first = allowed[:allowed.index('return true;')]
assert all(f'case {rg}:' in first for rg in SONGS.values())  # songs are always allowed
assert squash('case SCENE_ICE_CAVERN: case SCENE_ID_MAX: return true;') in squash(allowed)  # so is an ocarina here
assert 'return CheckInventory(ITEM_OCARINA_FAIRY, false);' in logic_cpp  # any ocarina
assert 'return exact ? (current == item) : (current != ITEM_NONE);' in logic_cpp
reset = logic_cpp[logic_cpp.index('void Logic::Reset(bool resetSaveContext'):]
for flag in ('A', 'C_UP', 'C_DOWN', 'C_LEFT', 'C_RIGHT'):
    assert f'SetRandoInf(RAND_INF_HAS_OCARINA_{flag}, !ocBtnShuffle);' in reset[:reset.index('// Progressive Items')]
# The landings are the rows' areas: Ship's warp-pad entrances (entrance.cpp WarpSong rows) have the rows' ids, and
# the audit binds each to OoTMM's landing (export_oot_bindings.py re-verifies it when the rows are bound).
indices = {}
for line in (source/'soh/include/tables/entrance_table.h').read_text(encoding='utf-8').splitlines():
    if m := re.match(r'/\* (0x[0-9A-F]+) \*/ DEFINE_ENTRANCE\((ENTR_\w+),', line):
        indices[m.group(2)] = int(m.group(1), 16)
entrance_cpp = (randomizer/'entrance.cpp').read_text(encoding='utf-8')
for song, row in song_rows.items():
    m = re.search(rf'\{{ EntranceType::WarpSong, {WARP[song]},\s+({landings[song]}),\s+(ENTR_\w+) \}}', entrance_cpp)
    assert m and indices[m.group(2)] == row['id'], (song, m and m.group(2))

# The probe binds exactly these six rows as its GLOBAL node's edges (oot-edge-audit.json role global, regenerated and
# re-verified by export_oot_bindings.py), each from Root Exits to its warp region with the pad as landing, and adds them
# to every query (TempleProbe.cpp Resolve, GlobalEdges).
globals_ = {r['name']: r for r in audit['rows'] if r['role'] == 'global'}
assert sorted(globals_) == sorted(r['name'] for r in song_rows.values())
for song, row in song_rows.items():
    bound = globals_[row['name']]
    assert (bound['song'], bound['parent'], bound['target'], bound['landing']) ==         (SONGS[song], 'RR_ROOT_EXITS', WARP[song], landings[song]), bound
probe = (randomizer/'TempleProbe.cpp').read_text(encoding='utf-8')
assert 'overlay.global = GlobalEdges();' in probe and 'if (b.role == TempleBindingRole::Global) {' in probe

# 3. The Mask Shop. "Market Mask Shop" is entered only from the Market (market.yml:31), a child-only door, and that
# door is the reserved game link OOT_SHOP_MASKS -> MM Clock Town (entrances.ts:888-903), which validateAgeTemple's
# world installs (isValidShuffle's changedWorld); the link back lands in the Market. The Market reaches the Temple of
# Time through the Entryway, unconditionally both ways (market.yml:25, :266-272), and no own-game Wallmaster pool can
# touch those rows (they are overworld and indoors-extra; Wallmasters take dungeon entrances, entrances.ts:657-669).
entering = grep(r'^\s+"Market Mask Shop":', 'data/world')
assert [(p, n) for p, n, _ in entering] == [('data/world/oot/overworld/market.yml', 31)], entering
assert market['Market']['exits']['Market Mask Shop'].startswith('is_child && ')
assert rows['OOT_SHOP_MASKS']['from'] == 'OOT Market' and rows['OOT_SHOP_MASKS']['to'] == 'OOT Market Mask Shop'
assert rows['MM_CLOCK_TOWER_FROM_CLOCK_TOWN']['to'] == 'MM Clock Tower'
assert rows['OOT_MARKET_FROM_MASK_SHOP']['to'] == 'OOT Market'
solver = blob('packages/logic/src/solver/entrances.ts')
assert "this.overrideEntrance('OOT_SHOP_MASKS', 'MM_CLOCK_TOWN_FROM_CLOCK_TOWER');" in solver
assert "this.overrideEntrance('MM_CLOCK_TOWER_FROM_CLOCK_TOWN', 'OOT_MARKET_FROM_MASK_SHOP');" in solver
assert market['Market']['exits']['Temple of Time Entryway'] == 'true'
assert market['Temple of Time Entryway']['exits']['Temple of Time'] == 'true'
assert rows['OOT_TEMPLE_OF_TIME_ENTRYWAY_FROM_MARKET']['type'] == 'overworld'
assert rows['OOT_TEMPLE_OF_TIME']['type'] == 'indoors-extra'
# Wallmaster destinations are the dungeon types plus those of the other active simple pools (entrances.ts:657-669,
# :881-886); the probe's envelope admits no overworld or indoors pool (TempleAdapter.h CheckTempleCapability).
wallmasters = solver[solver.index('private poolWallmasters('):]
wallmasters = wallmasters[:wallmasters.index('\n  }\n')]
for line in ['const types = new Set(this.poolsTypesDst());', "types.add('dungeon');", "types.add('dungeon-minor');",
             "types.add('dungeon-sh');", "if (this.settings.erBoss !== 'none') {"]:
    assert line in wallmasters, line
# The other ways between the games: from OoT only GLOBAL -> MM SOARING (can_play_soaring, so a starting Soaring, which
# every id refuses); from MM only WARP_SONGS -> OOT SONG_TP_* (mm _system.yml:16-25) and the Clock Tower link. No
# OoT exit names an MM area elsewhere, nor an MM exit an OoT one (git grep over every world file).
for pattern, game in [('"OOT ', 'mm'), ('"MM ', 'oot')]:
    found = subprocess.check_output(['git', '-C', str(reference), 'grep', '-l', '-F', pattern, registry.PIN, '--',
                                     f'data/world/{game}'], text=True, timeout=60).split()
    assert found == [f'{registry.PIN}:data/world/{game}/overworld/_system.yml'], found
assert [k for k in system['GLOBAL']['exits'] if k.startswith('MM ')] == ['MM SOARING']
assert {k for k in mm_system['WARP_SONGS']['exits'] if k.startswith('OOT ')} == {f'OOT SONG_TP_{s}' for s in SONGS}
assert all(not k.startswith('OOT ') for a in mm_system.values() if a is not mm_system['WARP_SONGS']
           for k in a.get('exits', {}))
# Ages: no exit changes the age; only the TIME_TRAVEL events of the Temple of Time do (L2, pinned by
# entrance_temple_reference_tests.py), so each age reaches MM, and comes back, as itself.

# The port side of the hypothesis: a starting Song of Soaring stays refused under every id, and so does song
# destination shuffle (EntranceAvailability.h TempleSettingsRefusal); no shared item family is a song (lemma S1 test).
availability = (source/'combo/rando/entrances/EntranceAvailability.h').read_text(encoding='utf-8')
refusal = availability[availability.index('inline EntranceRefusal TempleSettingsRefusal('):]
refusal = refusal[:refusal.index('\n}\n')]
assert '"Song of Soaring"' in refusal and 'f.songTravelLogic' in refusal

# 4. The recorded pinned run (temple-song-facts.json) of every fixture case.
fixture_bytes = (source/'tools/ootmm/temple-song-fixtures.json').read_bytes()
fixture = json.loads(fixture_bytes)
facts = json.loads((source/'tools/ootmm/temple-song-facts.json').read_text(encoding='utf-8'))
import hashlib
assert facts['upstreamCommit'] == registry.PIN == fixture['upstreamCommit']
assert facts['fixture']['sha256'] == hashlib.sha256(fixture_bytes).hexdigest(), 'rerun the pinned --temple run'
assert [{k: g[k] for k in ('startingAge', 'world', 'instrument')} for g in facts['groups']] == fixture['groups']
cases = discriminating = 0
for group in facts['groups']:
    opposite = 'child' if group['startingAge'] == 'adult' else 'adult'
    assert group['opposite'] == opposite
    assert group['spawnArea'] == ("OOT Link's House" if opposite == 'child' else 'OOT Temple of Time')
    assert set(group['walks']) == {group['spawnArea']} | {r['to'] for r in song_rows.values()}
    assert [c['song'] for c in group['cases']] == [None] + fixture['songs']
    answers = set()
    for case in group['cases']:
        cases += 1
        start, other = case['ages'][group['startingAge']], case['ages'][opposite]
        # L1: the starting age reaches only SPAWN and the (empty, MM-typed) ASSUMED hub of the pools.
        assert start['areas'] == 2 and not start['global'] and not start['temple'], case['id']
        # L3: the opposite age reaches GLOBAL in every case.
        assert other['global'], case['id']
        # Mask Shop: an age in MM has the Market and the Temple of Time.
        for age in ('child', 'adult'):
            a = case['ages'][age]
            assert not a['mm'] or (a['market'] and a['temple']), (case['id'], age)
        if case['song'] is None:
            assert other['songAreas'] == [] and case['temple'] == group['walks'][group['spawnArea']], case['id']
        else:
            assert case['row'] == song_rows[case['song']]['name'] and case['landing'] == song_rows[case['song']]['to']
            assert other['songAreas'] in ([], [f'OOT SONG_TP_{case["song"]}']), case['id']
            played = bool(other['songAreas'])
            # GLOBAL's only effect: the songless walk from the spawn area, or the walk from the landing.
            assert case['temple'] == (group['walks'][group['spawnArea']] or (played and group['walks'][case['landing']])), \
                case['id']
        answers.add(case['temple'])
    discriminating += len(answers) == 2
assert cases == 7 * len(fixture['groups']) and discriminating >= 3, (cases, discriminating)
print(f'GLOBAL lemmas verified at {registry.PIN}: L3, the six song edges and their notes on both sides, the Mask Shop;',
      f'{cases} recorded validateAgeTemple cases, {discriminating} groups where the song decides')
