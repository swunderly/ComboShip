"""Pin lemma S1 of the Wallmaster limits design (Stage 1) to the exact OoTMM v32.3 blobs.

Lemma S1: with no song item at the start in either game, every song exit of OoTMM's GLOBAL, WARP_SONGS and
SOARING areas is false in validateAgeTemple, so "Play warp songs across games" (OoTMM's songSoaringOot and
song*Mm) and Ageless Song of Soaring (agelessSoaring) cannot change its answer. A song exit is an exit into a
warp-song or Soaring area: OoT SONG_TP_* and MM SOARING, MM SOARING and OOT SONG_TP_*. The other exits of those
areas are unconditional but lead only to virtual dead ends (EGGS, ACCESS) or to WARP_SONGS, whose own exits are
ACCESS and song exits; they do not depend on any song setting. The Temple probe (TempleProbe.cpp) therefore needs
no song node while no song starts in either game, which the port refuses for every Wallmaster id.

Contract evidence only, not behaviour. Skipped without the optional checkout, and failing (never skipped) when the
checkout is at another revision.
"""
from pathlib import Path
import argparse
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

SONGS = ('FOREST', 'FIRE', 'WATER', 'SHADOW', 'SPIRIT', 'LIGHT')
OWLS = ('CLOCK_TOWN', 'MILK_ROAD', 'SOUTHERN_SWAMP', 'WOODFALL', 'MOUNTAIN_VILLAGE', 'SNOWHEAD', 'GREAT_BAY',
        'ZORA_CAPE', 'IKANA_CANYON', 'STONE_TOWER')


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
    """The four line shapes the pinned _system.yml files use; anything else fails closed."""
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
    """Every `"name": expr` macro of one pinned macros file, quoted or not, without a trailing comment after a quoted
    one; a name defined twice fails."""
    result = {}
    for line in blob(path).splitlines():
        if m := re.fullmatch(r'"([^"]+)": (?:"([^"]*)"(?: *#.*)?|([^"#].*))', line):
            assert m.group(1) not in result, line
            result[m.group(1)] = m.group(2) if m.group(2) is not None else m.group(3).strip()
        else:
            assert not line.strip() or line.startswith('#'), f'{path}: unexpected line {line!r}'
    return result


def conjuncts(expr):
    """The top-level `&&` terms of an expression without parentheses around them."""
    depth, start, terms = 0, 0, []
    for i, c in enumerate(expr):
        depth += (c == '(') - (c == ')')
        if depth == 0 and expr.startswith(' && ', i):
            terms.append(expr[start:i])
            start = i + 4
    return terms + [expr[start:]]


oot = areas('data/world/oot/overworld/_system.yml')
mm = areas('data/world/mm/overworld/_system.yml')

# 1. The song exits, and the only songless ways out of the three areas (oot _system.yml:23-34, mm _system.yml:4-39).
assert oot['GLOBAL']['exits'] == {**{f'SONG_TP_{s}': f'can_play_tp_{s.lower()}' for s in SONGS},
                                  'MM SOARING': 'can_play_soaring && setting(games, ootmm)', 'EGGS': 'true'}
assert mm['GLOBAL']['exits'] == {'ACCESS': 'true', 'WARP_SONGS': 'true', 'SOARING': 'can_play_soaring'}
assert mm['WARP_SONGS']['exits'] == {'ACCESS': 'true', **{f'OOT SONG_TP_{s}': f'can_play_tp_{s.lower()} && '
                                                          'setting(games, ootmm)' for s in SONGS}}
owl_areas = ['Owl Clock Town', 'Owl Milk Road', 'Owl Swamp', 'Owl Woodfall', 'Owl Mountain', 'Owl Snowhead',
             'Owl Great Bay', 'Owl Zora Cape', 'Owl Ikana', 'Owl Stone Tower']
assert mm['SOARING']['exits'] == {area: f'has(OWL_{owl})' for area, owl in zip(owl_areas, OWLS)}
# The songless exits end in virtual areas with no exit: EGGS has locations only, ACCESS one event only.
assert set(oot['EGGS']) == {'region', 'age_change', 'locations'} and oot['EGGS']['region'] == 'EGGS'
assert set(mm['ACCESS']) == {'region', 'events'} and mm['ACCESS']['events'] == {'ACCESS': 'can_reset_time_impl'}
# Each song area's only exit is its landing, behind the song exit.
for s in SONGS:
    assert set(oot[f'SONG_TP_{s}']) == {'region', 'age_change', 'exits'} and len(oot[f'SONG_TP_{s}']['exits']) == 1

# 2. Every exit into a song area, in every pinned world file, has the song's macro as a top-level conjunct: the
# GLOBAL and WARP_SONGS song exits above, and MM SOARING exits elsewhere (Pirates' Fortress, behind the Zora Mask).
song_targets = {**{f'SONG_TP_{s}': f'can_play_tp_{s.lower()}' for s in SONGS},
                **{f'OOT SONG_TP_{s}': f'can_play_tp_{s.lower()}' for s in SONGS},
                'MM SOARING': 'can_play_soaring', 'SOARING': 'can_play_soaring'}
entries = grep(r'^\s+"(SONG_TP_[A-Z]+|OOT SONG_TP_[A-Z]+|MM SOARING|SOARING)":', 'data/world')
counted = {}
for path, number, text in entries:
    m = re.fullmatch(r'\s+"([^"]+)": "(.*)"', text)
    assert m and m.group(1) in song_targets, (path, number, text)
    assert song_targets[m.group(1)] in conjuncts(m.group(2)), (path, number, text)
    counted[m.group(1)] = counted.get(m.group(1), 0) + 1
assert counted == {**{f'SONG_TP_{s}': 1 for s in SONGS}, **{f'OOT SONG_TP_{s}': 1 for s in SONGS},
                   'MM SOARING': 1, 'SOARING': 8}, counted
# The songless virtual areas are entered only from _system.yml; WARP_SONGS is entered from MM areas, but its exits
# are pinned above.
assert [(p, n) for p, n, _ in grep(r'^\s+"(EGGS|ACCESS)":', 'data/world')] == [
    ('data/world/mm/overworld/_system.yml', 7), ('data/world/mm/overworld/_system.yml', 19),
    ('data/world/oot/overworld/_system.yml', 34)]

# 3. Every song macro needs a song item (macros_common.yml:4-41, macros_oot.yml:33, macros_mm.yml:113): the song,
# its shared form or enough of its notes.
common = macros('data/macros/macros_common.yml')
assert common['has_shared_count(a, b, n)'] == 'has(a, n) || has(b, n)'
for song in [f'tp_{s.lower()}' for s in SONGS] + ['soaring']:
    item = song.upper()
    has = re.fullmatch(rf'has\(SONG_{item}\) \|\| has\(SHARED_SONG_{item}\) \|\| '
                       rf'has_shared_count\(SONG_NOTE_{item}, SHARED_SONG_NOTE_{item}, [0-9]+\)',
                       common[f'has_song_{song}'])
    assert has, common[f'has_song_{song}']
    play = common[f'can_play_{song}' if song != 'soaring' else 'can_play_soaring_raw']
    assert conjuncts(play)[0] == f'has_song_{song}', play
assert macros('data/macros/macros_oot.yml')['can_play_soaring'] == \
    '(setting(agelessSoaring) || is_child) && can_play_soaring_raw'
assert macros('data/macros/macros_mm.yml')['can_play_soaring'] == 'can_play_soaring_raw'
# No macros file defines them again.
names = r'"(can_play_tp_[a-z]+|has_song_tp_[a-z]+|can_play_soaring(_raw)?|has_song_soaring|has_shared_count)[("]'
defined = sorted((p, re.match(r'"([^"(]+)', t).group(1)) for p, _, t in grep('^' + names, 'data/macros'))
assert defined == sorted([('data/macros/macros_common.yml', n) for n in
                          ['has_shared_count'] + [f'has_song_tp_{s.lower()}' for s in SONGS] +
                          [f'can_play_tp_{s.lower()}' for s in SONGS] + ['has_song_soaring', 'can_play_soaring_raw']] +
                         [('data/macros/macros_oot.yml', 'can_play_soaring'),
                          ('data/macros/macros_mm.yml', 'can_play_soaring')]), defined

# 4. validateAgeTemple has the starting items only: a fresh Pathfinder with this.startingItems, run with no item
# placement (entrances.ts:265-287), so a collected location adds nothing (pathfind.ts collectLocation).
solver = blob('packages/logic/src/solver/entrances.ts')
pathfind = blob('packages/logic/src/pathfind/pathfind.ts')
temple = solver[solver.index('private validateAgeTemple(world: World)'):solver.index('private changeWorldAssumePools(')]
assert 'new Pathfinder(worlds, this.settings, this.startingItems);' in temple
assert 'agePathfinder.run(null, { recursive: true, singleWorld: this.worldId });' in temple
assert temple.count('.run(') == 1
assert 'items: new Map(startingItems),' in pathfind
assert pathfind.count('this.opts.items') == 1 and 'const playerItem = this.opts.items?.get(globalLoc);' in pathfind

# 5. The song settings change no world or macro expression but can_play_soaring's age term: agelessSoaring is read
# once, in macros_oot.yml:33; songSoaringOot and song*Mm never. In the logic package they add the items to the pool
# (world/transform.ts) and, only with Song Events Shuffle (default off, data.ts:1094-1110), song-event candidates.
foreign = r'songSoaringOot|song(Minuet|Bolero|Serenade|Requiem|Nocturne|Prelude)Mm'
assert grep(foreign, 'data') == []
assert [(p, n) for p, n, _ in grep('agelessSoaring', 'data')] == [('data/macros/macros_oot.yml', 33)]
uses = {p for p, _, _ in grep(foreign, 'packages/logic/src')}
assert uses == {'packages/logic/src/world/transform.ts', 'packages/logic/src/misc/song-events.ts'}, uses
transform = blob('packages/logic/src/world/transform.ts')
for setting, item in [('songSoaringOot', 'OOT_SONG_SOARING'), ('songMinuetMm', 'MM_SONG_TP_FOREST'),
                      ('songBoleroMm', 'MM_SONG_TP_FIRE'), ('songSerenadeMm', 'MM_SONG_TP_WATER'),
                      ('songRequiemMm', 'MM_SONG_TP_SPIRIT'), ('songNocturneMm', 'MM_SONG_TP_SHADOW'),
                      ('songPreludeMm', 'MM_SONG_TP_LIGHT')]:
    assert f'}} else if (settings.{setting}) {{\n      this.addItem(Items.{item});\n    }}' in transform, setting
events = blob('packages/logic/src/misc/song-events.ts')
run = events[events.index('  run() {'):]
assert 'if (this.state.settings.songEventsShuffleOot) {' in run and 'if (this.state.settings.songEventsShuffleMm) {' in run
data = blob('packages/core/src/settings/data.ts')
for key in ('songEventsShuffleOot', 'songEventsShuffleMm'):
    block = data[data.index(f"key: '{key}',"):]
    assert block[:block.index('}')].rstrip().endswith('default: false'), key

# 6. The port side of the hypothesis: it has no Song Events Shuffle, no song notes and no shared warp song or Soaring
# family, and every Wallmaster id refuses a starting warp song or Song of Soaring in either game before any draw
# (EntranceAvailability.h), with the native probe refusing a starting warp song again (TempleProbe.cpp).
port = [p for root in ('combo', 'soh/soh', 'mm/2s2h') for p in (source/root).rglob('*')
        if p.suffix in ('.h', '.cpp', '.c', '.inc', '.hpp')]
for path in port:
    text = path.read_text(encoding='utf-8', errors='replace')
    assert 'songEventsShuffle' not in text and 'SONG_NOTE' not in text, path
shared = (source/'combo/rando/SharedItems.h').read_text(encoding='utf-8')
families = re.findall(r'^\s+(SF_[A-Z_]+)[,=]', shared[shared.index('enum'):shared.index('SF_COUNT')], re.M)
assert families and not [f for f in families if re.search('MINUET|BOLERO|SERENADE|REQUIEM|NOCTURNE|PRELUDE|SOAR', f)]
availability = (source/'combo/rando/entrances/EntranceAvailability.h').read_text(encoding='utf-8')
assert ('for (std::string_view song : { "Song of Soaring", "Minuet of Forest", "Bolero of Fire", "Serenade of Water",\n'
        '                                           "Nocturne of Shadow", "Requiem of Spirit", "Prelude of Light" })'
        in availability)
assert 'for (const auto* list : { &f.ootStartingItems, &f.mmStartingItems, &f.foreignStartingItems })' in availability
# MM's starting items are read by 2Ship's spoiler names (RI_SONG_SOARING) in seeds and profiles, by display names in
# generation; the refusal takes both.
assert 'if (const auto item = MmStartingItemName(stored); item == song)' in availability
for reader in ('combo/rando/ComboSeedFormat.h', 'combo/rando/entrances/EntranceProfile.h'):
    assert 'MmStartingItemName(item.get<std::string>())' in (source/reader).read_text(encoding='utf-8'), reader
probe = (source/'soh/soh/Enhancements/randomizer/TempleProbe.cpp').read_text(encoding='utf-8')
assert 'throw Failure(kUnsupported, "starting warp songs are not supported");' in probe
assert 'if (!IsSharedStarterName(name) || IsWarpSong(it->second)) {' in probe

print('Lemma S1 verified at', registry.PIN + ':', f'{len(entries)} song exits, each behind its song item;',
      'the song settings change no exit without one')
