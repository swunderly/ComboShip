"""MM spell extensions (plan F2 phase A): OoTMM text, switches and the logic receipt.

The fixture fixtures/ootmm-32.3-mm-spells.json holds OoTMM v32.3's names and descriptions for the MM spell
settings, the cross-game shared items and the spell tricks, plus the receipt of every MM logic site that copies an
OoTMM Din's Fire / Nayru's Love term (fork region and expression -> OoTMM file:line) and of the sites where OoTMM's
expression has no Din's Fire although the native one had short-range fire ('dinNotCounted'). This checks the
production menus, profile layout, shared-item table, trick table, tracker and region files against it, and fails
on any logic line that counts Din's Fire without being a receipt site. When the OoTMM v32.3 checkout is available
(--reference, or references/ootmm-stable-32.3 beside the worktree), the fixture itself is checked against data.ts,
tricks.ts, the cited yml lines and the macros that count Din's Fire.
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
    # Din's Fire and Nayru's Love go through SpellPoolCheckbox, which greys them out where they cannot be cast.
    spell = {'RO_MM_SPELL_LOVE': 1, 'RO_MM_SPELL_FIRE': 2}.get(s['ro'])
    row = (f'SpellPoolCheckbox("{s["name"]}", {s["ro"]}, {spell}, "{s["description"]}");' if spell else
           f'ItemPoolCheckbox("{s["name"]}", {s["ro"]}, "{s["description"]}", 1);')
    check(row in menu, f'Menu.cpp lacks {row}')
    positions.append(menu.find(row))
    check(f'{{"label","{s["name"]}"}},{{"kind","checkbox"}},{{"tooltip","{s["description"]}"}},{{"key",Rando::StaticData::Options.at({s["ro"]}).cvar}}' in layout,
          f'ProfileMmLayout.inc lacks the {s["name"]} row')
    check(f'RO({s["ro"]}, RO_GENERIC_NO)' in options, f'{s["ro"]} is not released with a CVar')
check(positions == sorted(positions), 'Cross-Game Items rows are not in OoTMM order (data.ts:2000-2022)')

# ---- Shared items: OoTMM names/descriptions, public spells and gear.
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


def site_file(name):
    """A receipt file: a bare name is a region file, anything else is relative to the worktree."""
    return regions_dir / name if '/' not in name else root / name


def source_of(name):
    return sources.setdefault(name, site_file(name).read_text(encoding='utf-8'))


for entry in fixture['macros']:
    text = squash(read(entry['file']))
    check(squash(entry['text']) in text, f'{entry["file"]} lacks {entry["text"]}')
for group in ('din', 'nayru', 'dinNotCounted'):
    for entry in fixture[group]:
        check(squash(entry['text']) in squash(region(source_of(entry['file']), entry['region'])),
              f'{entry["file"]} {entry["region"] or "(file)"} lacks {entry["text"]}')

# ---- Din's Fire only where OoTMM has it. The fork's short-range fire macros count Din's Fire, including the native
# torch macro (CAN_LIGHT_TORCH_NEAR_ANOTHER, OoTMM has_fire_sticks). Every logic line that uses one must be a receipt
# site whose OoTMM expression counts Din's Fire, and the sites where OoTMM's expression has no Din's Fire (the
# 'dinNotCounted' entries) must use none of them. CAN_USE_GBT_GEYSER (all three spells, hidden trick) is covered by
# its notApplicable entry.
DIN_TOKENS = {'CAN_USE_DINS_FIRE', 'CAN_USE_FIRE_SHORT_RANGE', 'CAN_LIGHT_TORCH_NEAR_ANOTHER'}
logic_dir = root / 'mm/2s2h/Rando/Logic'
inventory_files = sorted(regions_dir.glob('*.cpp')) + sorted(logic_dir.glob('Mm*.inc.h'))
changed = True
while changed:  # local macros built on them, e.g. SpiderHouses.cpp COMBO_WORLD36_FIRE_STICKS
    changed = False
    for path in inventory_files:
        for m in re.finditer(r'#define\s+(\w+)((?:[^\n]*\\\n)*[^\n]*)', path.read_text(encoding='utf-8')):
            if m.group(1) not in DIN_TOKENS and set(re.findall(r'\w+', m.group(2))) & DIN_TOKENS:
                DIN_TOKENS.add(m.group(1))
                changed = True


def statement(text, at):
    """The call that starts at text[at] (squashed), up to its matching parenthesis."""
    depth, i = 0, text.index('(', at)
    while True:
        depth += (text[i] == '(') - (text[i] == ')')
        i += 1
        if depth == 0:
            return text[at:i]


for entry in fixture['dinNotCounted']:
    body = squash(region(source_of(entry['file']), entry['region']))
    at = body.find(squash(entry['text']))
    if at >= 0:
        used = set(re.findall(r'\w+', statement(body, at))) & DIN_TOKENS
        check(not used, f'{entry["file"]} {entry["region"] or "(file)"} counts Din\'s Fire ({sorted(used)}) where '
                        f'{entry["ootmm"]} does not')
for path in inventory_files:
    name = path.name if path.parent == regions_dir else path.relative_to(root).as_posix()
    entries = [squash(e['text']) for g in ('din', 'nayru') for e in fixture[g] if e['file'] == name]
    for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
        if line.lstrip().startswith('#define') or not set(re.findall(r'\w+', line.split('//')[0])) & DIN_TOKENS:
            continue
        s = squash(line.split('//')[0])
        tail = s.rstrip(');,')  # a line can close a statement that starts above it
        check(any(e in s or tail in e for e in entries),
              f'{name}:{number} counts Din\'s Fire but is not a receipt site: {line.strip()[:120]}')

# ---- Item tracker: the spells lead their trade slots (OoTMM trade.c kMmTrade1..3) and hide on other seeds.
settings = squash(read('mm/2s2h/Enhancements/Trackers/ItemTracker/ItemTrackerSettings.cpp'))
order = ['RI_DINS_FIRE', 'RI_MOONS_TEAR', 'RI_DEED_OCEAN', 'RI_FARORES_WIND', 'RI_ROOM_KEY', 'RI_LETTER_TO_MAMA',
         'RI_NAYRUS_LOVE', 'RI_LETTER_TO_KAFEI']
trade = settings[settings.index('.name="TradeItems"'):]
trade = trade[:trade.index('});')]
check([trade.find('{TRACKER_ITEM_RANDO,' + x + '}') for x in order] == sorted(trade.find('{TRACKER_ITEM_RANDO,' + x + '}') for x in order)
      and all(trade.find('{TRACKER_ITEM_RANDO,' + x + '}') >= 0 for x in order), 'Trade Items order')
tracker_source = read('mm/2s2h/Enhancements/Trackers/ItemTracker/ItemTracker.cpp')
tracker = squash(tracker_source)
for item, ro in (('RI_DINS_FIRE', 'RO_MM_SPELL_FIRE'), ('RI_FARORES_WIND', 'RO_MM_SPELL_WIND'), ('RI_NAYRUS_LOVE', 'RO_MM_SPELL_LOVE')):
    check(f'if(itemId=={item})return!IS_RANDO||!RANDO_SAVE_OPTIONS[{ro}];' in tracker, f'{item} tracker row is not hidden without {ro}')

# ---- The combo tracker shows that group from its own toggle (Trackers > Item Tracker > Groups), after Quest, since
# 2S2H's default groups leave it out. Unset, the toggle is on where its hidden rows show: on seeds with any of the MM
# spells, boots or tunics (OoT tracks its own in its default Items and Equipment groups), or a layout that has it.
controls = squash(read('combo/gui/ComboTrackerControls.cpp'))
check('Combo_ResolveSym("2ship","MM_ComboTrackerTradeDefault")' in controls
      and 'Checkbox("Tradeitems(MM)","gCombo.Tracker.Trade",tradeDefault?tradeDefault():0);' in controls,
      'the item tracker has no Trade items (MM) group toggle')
check(controls.index('"gCombo.Tracker.Quest"') < controls.index('"gCombo.Tracker.Trade"') < controls.index('"gCombo.Tracker.Dungeons"'),
      'the Trade items toggle is not listed where the group is drawn')
default = tracker_source[tracker_source.index('int MM_ComboTrackerTradeDefault() {'):]
default = default[:default.index('\n}\n')]
hidden = set(re.findall(r'return !IS_RANDO \|\| !RANDO_SAVE_OPTIONS\[(RO_\w+)\];', tracker_source))
check(len(hidden) == 7 and set(re.findall(r'RO_MM_\w+', default)) == hidden and 'ComboTrackerLayoutHasTrade()' in default,
      'the Trade group default does not follow the rows it hides')
draw = squash(tracker_source[tracker_source.index('void MM_ComboItemTrackerDrawContents() {'):])
check(':trade?"gCombo.Tracker.Trade"' in draw and 'CVarGetInteger(key,trade?tradeDefault:1)' in draw
      and 'groups.insert(quest==groups.end()?quest:quest+1,&*trade);' in draw
      and draw.index('if(!ComboTrackerLayoutHasTrade())') < draw.index('groups.insert('),
      'the combo tracker does not add the Trade Items group after Quest')

# ---- Item tracker, default Inventory group (TRACKER_ITEM_SLOT): a slot holding a spell shows the spell's own icon.
# Spells.cpp / FaroresWind.cpp put the spells into empty trade slots, and their ids lie above ITEM_RECOVERY_HEART, where
# the slot otherwise falls back to the slot's first safe item (a lit Moon's Tear for Din's Fire). Evaluate the
# production fallback condition for each item id.
item_h = read('mm/include/z64item.h')
item_ids, value = {}, -1
for line in item_h[item_h.index('typedef enum ItemId {'):item_h.index('} ItemId;')].splitlines()[1:]:
    m = re.match(r'\s*(?:/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*)?(ITEM_\w+)(?:\s*=\s*(\w+))?', line)
    if not m:
        continue
    explicit = m.group(3)
    value = (int(explicit, 0) if explicit[0].isdigit() else item_ids[explicit]) if explicit else value + 1
    item_ids[m.group(2)] = value
slot_case = tracker_source[tracker_source.index('case TRACKER_ITEM_SLOT: {'):]
slot_case = slot_case[:slot_case.index('} break;')]
combo_branch = slot_case[slot_case.index('#ifdef COMBO_BUILD'):slot_case.index('#else')]
spell_expr = re.search(r'const bool (\w+) =(.*?);', combo_branch, re.S)
fallback = re.search(r'if \((.*)\) \{\s*const auto& safe = safeItemsForInventorySlot', slot_case)
check(spell_expr is not None and fallback is not None, 'Inventory tracker slot fallback not found')
if spell_expr and fallback:
    def c_to_py(expression):
        expression = expression.replace('||', ' or ').replace('&&', ' and ')
        return '(' + re.sub(r'!(?!=)', ' not ', expression) + ')'

    def falls_back(item):
        names = dict(item_ids, vanillaItemId=item)
        names[spell_expr.group(1)] = eval(c_to_py(spell_expr.group(2)), {}, names)
        return eval(c_to_py(fallback.group(1)), {}, names)

    for name in ('ITEM_COMBO_DINS_FIRE', 'ITEM_COMBO_NAYRUS_LOVE', 'ITEM_COMBO_FARORES_WIND'):
        check(item_ids[name] >= item_ids['ITEM_RECOVERY_HEART'] and not falls_back(item_ids[name]),
              f'the Inventory tracker draws another item for {name}')
        check(re.search(r'\[' + name + r'\]\s*=\s*\(TexturePtr\)\w+', read('mm/src/code/z_inventory.c')) is not None,
              f'{name} has no gItemIcons entry')
    for name, expected in (('ITEM_NONE', True), ('ITEM_RECOVERY_HEART', True), ('ITEM_MOONS_TEAR', False)):
        check(falls_back(item_ids[name]) == expected, f'Inventory tracker fallback changed for {name}')
check('for (const TexturePtr entry : gItemIcons)' in read('mm/2s2h/ShipUtils.cpp'), 'gItemIcons textures are not loaded')

# ---- Runtime readiness: a device that cannot cast Din's Fire or Nayru's Love never generates a seed that needs it.
# The cast checks the integration (Combo_MMSpellEnabled) and the spell's own resources (Combo_MMSpellUseItem,
# ComboMagic*_Init); the seed options follow the same readiness, kept per spell when the integration comes up.


def body(source, signature):
    """Text of the function or initializer that starts at signature, up to its matching brace."""
    at = source.index(signature)
    depth, i = 0, source.index('{', at)
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return squash(source[at:i])


spells_cpp = read('mm/2s2h/Rando/Spells.cpp')
benport = read('mm/2s2h/BenPort.cpp')
ready = body(spells_cpp, 'extern "C" COMBO_EXPORT int MM_SetNativeSpellIntegrationReady(')
check('constintmask=MM_PrepareNativeSpellsForCombo();if(mask&1){ComboRando::ComboFaroreIntegrationReady=true;sReadySpells=mask&6;}' in ready,
      'MM_SetNativeSpellIntegrationReady does not keep the Love/Fire readiness bits')
check('sReadySpells=0;' in body(spells_cpp, 'void DropSpellResources()'), 'resource teardown keeps the spells ready')
check('returnComboRando::ComboFaroreIntegrationReady&&(sReadySpells&(spell==1?2:4))?1:0;'
      in body(spells_cpp, 'extern "C" int Combo_MMSpellOptionReady('), 'Combo_MMSpellOptionReady does not need both')
check('{RO_MM_SPELL_LOVE,1},{RO_MM_SPELL_FIRE,2}' in squash(benport), 'kComboMmSpellOptions does not number the spells as Spells.cpp')
zero_options = 'for(constauto&[id,spell]:kComboMmSpellOptions)if(!Combo_MMSpellOptionReady(spell))'
check(zero_options + 'options[id]=0;' in body(benport, 'extern "C" void Combo_MM_ClearUnavailableOptions('),
      'generation keeps a spell the device cannot cast')
check(zero_options + 'options[Rando::StaticData::Options[id].name]=uint32_t(0);'
      in body(benport, 'extern "C" COMBO_EXPORT int MM_InitRandoSaveFile('), 'a new MM save keeps an unready spell')
check(zero_options + 'j[Rando::StaticData::Options[id].cvar]=0;'
      in body(benport, 'extern "C" COMBO_EXPORT const char* MM_DumpRandoSettings('), 'the settings snapshot keeps an unready spell')
check('Combo_MM_ClearUnavailableOptions(saveInfo.randoSaveOptions);' in body(benport, 'extern "C" COMBO_EXPORT const char* MM_DumpRandoStaticData('),
      'the generation dump does not drop unready spells')
check('Combo_MM_ClearHiddenAdultOptions' not in benport + menu, 'an options path still clears only the adult options')
check('Combo_MMSpellsForHeadlessGeneration();' in body(benport, 'extern "C" COMBO_EXPORT void MM_InitRandoHeadless('),
      'the headless generator cannot add the spells')
check(squash('SpellPoolCheckbox("Din\'s Fire (MM)", RO_MM_SPELL_FIRE, 2,') in squash(menu)
      and squash('SpellPoolCheckbox("Nayru\'s Love (MM)", RO_MM_SPELL_LOVE, 1,') in squash(menu),
      'the Cross-Game Items rows do not grey out unready spells')
check('Combo_MM_ClearUnavailableOptions(randoSaveInfo.randoSaveOptions);' in squash(menu), 'pool counts keep unready spells')

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
    for entry in fixture['macros'] + fixture['din'] + fixture['nayru'] + fixture['dinNotCounted']:
        rel, line = entry['ootmm'].rsplit(':', 1)
        lines = (reference / rel).read_text(encoding='utf-8').splitlines()
        check(entry['expects'] in lines[int(line) - 1], f'{entry["ootmm"]} does not contain {entry["expects"]!r}')
    # OoTMM macros that count Din's Fire (can_use_din and everything built on it, e.g. has_fire_sticks).
    macros = {}
    for rel in ('data/macros/macros_common.yml', 'data/macros/macros_mm.yml'):
        for m in re.finditer(r'^"(\w+)(?:\([^)]*\))?":\s*"(.*)"', (reference / rel).read_text(encoding='utf-8'), re.M):
            macros[m.group(1)] = set(re.findall(r'\b[a-z_][a-z0-9_]*\b', m.group(2)))
    din_terms = {'can_use_din', 'can_use_din_raw'}
    changed = True
    while changed:
        changed = False
        for name, uses in macros.items():
            if name not in din_terms and uses & din_terms:
                din_terms.add(name)
                changed = True

    def terms(expression):
        return set(re.findall(r'\b[a-z_][a-z0-9_]*\b', expression))

    for entry in fixture['din']:
        check(terms(entry['expects']) & din_terms, f'{entry["ootmm"]}: {entry["expects"]!r} does not count Din\'s Fire')
    for entry in fixture['dinNotCounted']:
        rel, line = entry['ootmm'].rsplit(':', 1)
        expression = (reference / rel).read_text(encoding='utf-8').splitlines()[int(line) - 1].split(':', 1)[1]
        check(not terms(expression) & din_terms, f'{entry["ootmm"]} counts Din\'s Fire after all')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources against the fixture only')

if failures:
    print('\n'.join(failures))
    sys.exit(1)
print(f'MM spell extensions: {len(fixture["settings"])} settings, {len(fixture["shared"])} shared items, '
      f'{len(fixture["tricks"])} tricks, {len(fixture["din"]) + len(fixture["nayru"]) + len(fixture["macros"])} logic sites, '
      f'{len(fixture["dinNotCounted"])} sites without Din\'s Fire '
      f'and {len(fixture["notApplicable"])} documented N/A entries match OoTMM v32.3')
