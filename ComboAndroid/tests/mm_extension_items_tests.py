"""MM spell extensions (plan F2 phase A): OoTMM text, switches and the logic receipt.

The fixture fixtures/ootmm-32.3-mm-spells.json holds OoTMM v32.3's names and descriptions for the MM spell
settings, the cross-game shared items and the spell tricks, plus the receipt of every MM logic site that copies an
OoTMM Din's Fire / Nayru's Love term (fork region and expression -> OoTMM file:line). This checks the production
menus, profile layout, shared-item table, trick table, tracker and region files against it. When the OoTMM v32.3
checkout is available (--reference, or references/ootmm-stable-32.3 beside the worktree), the fixture itself is
checked against data.ts, tricks.ts and the cited yml lines.
"""
from pathlib import Path
import argparse
import json
import re
import sys

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--reference', type=Path)
a = p.parse_args()
root = a.source.resolve()
fixture = json.loads((Path(__file__).resolve().parent / 'fixtures/ootmm-32.3-mm-spells.json').read_text(encoding='utf-8'))
regions_dir = root / 'mm/2s2h/Rando/Logic/Regions'
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def squash(text):
    return re.sub(r'\s+', '', text)


def region(source, rid):
    if not rid:
        return source
    start = source.find('Regions[' + rid + ']')
    if start < 0:
        return ''
    end = source.find('Regions[', start + 8)
    return source[start:] if end < 0 else source[start:end]


# ---- Settings: Randomizer > Items & checks > MM > Cross-Game Items, profile layout, released CVars.
menu = read('mm/2s2h/Rando/Menu.cpp')
layout = read('mm/2s2h/ProfileMmLayout.inc')
options = read('mm/2s2h/Rando/StaticData/Options.cpp')
positions = []
for s in fixture['settings']:
    row = f'ItemPoolCheckbox("{s["name"]}", {s["ro"]}, "{s["description"]}", 1);'
    check(row in menu, f'Menu.cpp lacks {row}')
    positions.append(menu.find(row))
    check(f'{{"label","{s["name"]}"}},{{"kind","checkbox"}},{{"tooltip","{s["description"]}"}},{{"key",Rando::StaticData::Options.at({s["ro"]}).cvar}}' in layout,
          f'ProfileMmLayout.inc lacks the {s["name"]} row')
    check(f'RO({s["ro"]}, RO_GENERIC_NO)' in options, f'{s["ro"]} is not released with a CVar')
check(positions == sorted(positions), 'Cross-Game Items rows are not in OoTMM order (data.ts:2000-2022)')

# ---- Shared items: OoTMM names/descriptions, public spells, private gear.
shared = read('combo/rando/SharedItems.h')
for s in fixture['shared']:
    m = re.search(r'\{\s*SF_\w+,\s*"' + s['family'] + r'",\s*"[^"]*",\s*"([^"]*)",\s*((?:"[^"]*"\s*)+),([^}]*)\}', shared)
    check(m is not None, f'no shared row for {s["family"]}')
    if not m:
        continue
    check(m.group(1) == s['name'], f'{s["family"]} label {m.group(1)!r}')
    tooltip = ''.join(re.findall(r'"([^"]*)"', m.group(2))[:1])
    check(tooltip == s['description'], f'{s["family"]} tooltip {tooltip!r}')
    flags = [x.strip() for x in m.group(3).split(',') if x.strip()]
    check(flags[-1] == ('true' if s['public'] else 'false'), f'{s["family"]} publicOption {flags[-1]}')

# ---- Tricks: OoTMM names/tooltips, visibility.
tricks = squash(read('combo/rando/MmTricks.h'))
for t in fixture['tricks']:
    m = re.search(r'\{\d+,"' + re.escape(t['fork']) + r'",((?:"(?:[^"\\]|\\.)*")+),((?:"(?:[^"\\]|\\.)*")+),', tricks)
    check(m is not None, f'no trick row for {t["fork"]}')
    if not m:
        continue
    name = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))).replace('\\"', '"')
    tip = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2))).replace('\\"', '"')
    check(name == squash(t['name']), f'{t["fork"]} name')
    check(tip == squash(t['tooltip']), f'{t["fork"]} tooltip')
    row = tricks[m.start():tricks.index('}', m.end())]
    check(row.endswith(',false') != t['public'], f'{t["fork"]} visibility')

# ---- Switches: Nayru's Love moves only the graphs whose OoTMM regions carry a Nayru term; Din's Fire moves none.
logic_files = [root / 'mm/2s2h/Rando/Logic/Logic.h'] + sorted(regions_dir.glob('*.cpp'))
love_defines, fire_refs = set(), []
for path in logic_files:
    text = path.read_text(encoding='utf-8')
    for m in re.finditer(r'#define\s+(\w+)((?:[^\n]*\\\n)*[^\n]*)', text):
        if 'RO_MM_SPELL_LOVE' in m.group(2) and m.group(1) != 'CAN_USE_NAYRUS_LOVE':
            love_defines.add(m.group(1))
        if 'RO_MM_SPELL_FIRE' in m.group(2) and m.group(1) != 'CAN_USE_DINS_FIRE':
            fire_refs.append(f'{path.name}:{m.group(1)}')
    if path.parent == regions_dir:
        body = re.sub(r'#define(?:[^\n]*\\\n)*[^\n]*', '', text)
        check('RO_MM_SPELL_LOVE' not in body, f'{path.name} reads RO_MM_SPELL_LOVE outside its switch macro')
        check('RO_MM_SPELL_FIRE' not in body, f'{path.name} reads RO_MM_SPELL_FIRE directly')
check(love_defines == set(fixture['nayruSwitches']), f'Nayru switches {sorted(love_defines)}')
check(not fire_refs, f'Din switches {fire_refs}')

# ---- Receipt: every mapped site carries its OoTMM term.
sources = {}
for entry in fixture['macros']:
    text = squash(read(entry['file']))
    check(squash(entry['text']) in text, f'{entry["file"]} lacks {entry["text"]}')
for group in ('din', 'nayru'):
    for entry in fixture[group]:
        source = sources.setdefault(entry['file'], (regions_dir / entry['file']).read_text(encoding='utf-8'))
        check(squash(entry['text']) in squash(region(source, entry['region'])),
              f'{entry["file"]} {entry["region"] or "(file)"} lacks {entry["text"]}')

# ---- Item tracker: the spells lead their trade slots (OoTMM trade.c kMmTrade1..3) and hide on other seeds.
settings = squash(read('mm/2s2h/Enhancements/Trackers/ItemTracker/ItemTrackerSettings.cpp'))
order = ['RI_DINS_FIRE', 'RI_MOONS_TEAR', 'RI_DEED_OCEAN', 'RI_FARORES_WIND', 'RI_ROOM_KEY', 'RI_LETTER_TO_MAMA',
         'RI_NAYRUS_LOVE', 'RI_LETTER_TO_KAFEI']
trade = settings[settings.index('.name="TradeItems"'):]
trade = trade[:trade.index('});')]
check([trade.find('{TRACKER_ITEM_RANDO,' + x + '}') for x in order] == sorted(trade.find('{TRACKER_ITEM_RANDO,' + x + '}') for x in order)
      and all(trade.find('{TRACKER_ITEM_RANDO,' + x + '}') >= 0 for x in order), 'Trade Items order')
tracker = squash(read('mm/2s2h/Enhancements/Trackers/ItemTracker/ItemTracker.cpp'))
for item, ro in (('RI_DINS_FIRE', 'RO_MM_SPELL_FIRE'), ('RI_FARORES_WIND', 'RO_MM_SPELL_WIND'), ('RI_NAYRUS_LOVE', 'RO_MM_SPELL_LOVE')):
    check(f'if(itemId=={item})return!IS_RANDO||!RANDO_SAVE_OPTIONS[{ro}];' in tracker, f'{item} tracker row is not hidden without {ro}')

# ---- The fixture against OoTMM v32.3 itself.
reference = a.reference
if reference is None:
    for candidate in (root.parent.parent / 'references/ootmm-stable-32.3', root.parent / 'references/ootmm-stable-32.3'):
        if (candidate / 'packages/core/src/settings/data.ts').exists():
            reference = candidate
            break
if reference is not None:
    data = (reference / 'packages/core/src/settings/data.ts').read_text(encoding='utf-8')
    tricks_ts = (reference / 'packages/core/src/settings/tricks.ts').read_text(encoding='utf-8')

    def setting(key):
        block = data[data.index(f"key: '{key}'"):]
        block = block[:block.index('}, {')]
        name = re.search(r'name: ("|\')(.*?)\1,', block).group(2)
        desc = re.search(r"description: ('|\")(.*?)(?<!\\)\1,", block).group(2).replace("\\'", "'")
        return name, desc

    for s in fixture['settings'] + fixture['shared']:
        check(setting(s['key']) == (s['name'], s['description']), f'fixture {s["key"]} differs from data.ts')
    for t in fixture['tricks']:
        block = tricks_ts[tricks_ts.index(f'  {t["key"]}: {{'):]
        block = block[:block.index('  },')]
        name = re.search(r'name: ("|\')(.*?)\1,', block).group(2)
        tip = re.search(r'tooltip: ("|\')(.*?)\1,', block).group(2)
        check((name, tip) == (t['name'], t['tooltip']), f'fixture {t["key"]} differs from tricks.ts')
    for entry in fixture['macros'] + fixture['din'] + fixture['nayru']:
        rel, line = entry['ootmm'].rsplit(':', 1)
        lines = (reference / rel).read_text(encoding='utf-8').splitlines()
        check(entry['expects'] in lines[int(line) - 1], f'{entry["ootmm"]} does not contain {entry["expects"]!r}')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources against the fixture only')

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print(f'MM spell extensions: {len(fixture["settings"])} settings, {len(fixture["shared"])} shared items, '
      f'{len(fixture["tricks"])} tricks, {len(fixture["din"]) + len(fixture["nayru"]) + len(fixture["macros"])} logic sites '
      f'and {len(fixture["notApplicable"])} documented N/A entries match OoTMM v32.3')
