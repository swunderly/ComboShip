"""Randomizer tab after the T8 menu audit: nothing that was there is lost, and both worlds share headings.

The fixture lists the MM catalog rows (mm/2s2h/ProfileMmLayout.inc) as they were before the audit
(android-source 1e47735c). Every one must still have a row on a sidebar page DrawCustom draws, and a renamed row
must keep its old label as a search alias. MM headings must come from the page's fixed heading list
(combo/gui/ComboRandomizerLayout.h), which OoT's rows use too. The live trap settings that left the Randomizer tab
(not read by the seed) must keep a widget in Rando/Menu.cpp and a pointer on the Traps page. The combined goal
controls must still write their keys.
"""
from pathlib import Path
import argparse
import json
import re

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
root = p.parse_args().source.resolve()
fixture = json.loads((Path(__file__).resolve().parent / 'fixtures/randomizer-menu-homes.json').read_text(encoding='utf-8'))
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def body(text, signature):
    start = text.index(signature)
    i = text.index('{', start)
    depth = 0
    for j in range(i, len(text)):
        depth += (text[j] == '{') - (text[j] == '}')
        if depth == 0:
            return text[start:j + 1]
    raise AssertionError('unbalanced ' + signature)


# ---- MM catalog: every earlier option keeps a row on a page DrawCustom draws.
layout = read('mm/2s2h/ProfileMmLayout.inc')
rows = []
for line in layout.splitlines():
    if 'data["layout"].push_back' not in line:
        continue
    field = lambda name: (re.search(r'\{"%s","((?:[^"\\]|\\.)*)"\}' % name, line) or [None, None])[1]
    option = re.search(r'\{"key",Rando::StaticData::Options\.at\((RO_[A-Z0-9_]+)\)', line)
    rows.append({'page': field('page'), 'card': field('card'), 'heading': field('heading'), 'label': field('label'),
                 'aliases': field('aliases') or '', 'option': option.group(1) if option else None, 'line': line})
native_pages = {'Logic/Conditions', 'Check Pool', 'Item Pool', 'Dungeon Items', 'Traps', 'Starting Items', 'Hints'}
by_option = {}
for row in rows:
    check(row['page'] in native_pages, f'{row["label"]}: page {row["page"]!r} is not a native MM Rando sidebar')
    check(row['option'] is not None, f'{row["label"]}: no option key')
    check(row['option'] not in by_option, f'{row["option"]} has two rows')
    by_option[row['option']] = row
for option, label in fixture['mmLayoutRows']:
    row = by_option.get(option)
    check(row is not None, f'{option} ({label}) lost its Randomizer row')
    if row and row['label'] != label:
        check(label in row['aliases'].split('|') or label == row['aliases'],
              f'{option}: renamed {label!r} -> {row["label"]!r} without keeping the old label as an alias')
check(len(rows) == len(fixture['mmLayoutRows']), f'{len(rows)} MM rows, expected {len(fixture["mmLayoutRows"])}')
menu = read('combo/gui/ComboMenu.cpp')
check('row.value("aliases", std::string())' in menu, 'search does not index the MM catalog aliases')

# ---- Shared headings: MM card headings come from the page's fixed list, which OoT's rows also use.
headings_h = read('combo/gui/ComboRandomizerLayout.h')
pages, second_column = {}, {}
for m in re.finditer(r'\{\s*"([^"]+)",\s*\{([^{}]*)\},\s*\{([^{}]*)\}\s*\}',
                     body(headings_h, 'inline constexpr RandomizerPageHeadings')):
    pages[m.group(1)] = re.findall(r'"([^"]+)"', m.group(2))
    second_column[m.group(1)] = re.findall(r'"([^"]+)"', m.group(3))
    check(set(second_column[m.group(1)]) <= set(pages[m.group(1)]), f'{m.group(1)}: a second-column heading is not on it')
route = {'Logic/Conditions': 'Logic & access', 'Check Pool': 'Items & checks', 'Item Pool': 'Items & checks',
         'Dungeon Items': 'Dungeons', 'Traps': 'Hints & traps', 'Hints': 'Hints & traps',
         'Starting Items': 'Starting inventory'}
for row in rows:
    page = route[row['page']]
    allowed = pages.get(page, []) + (['Other'] if page == 'Starting inventory' else [])
    check(row['heading'] in allowed, f'{row["label"]}: heading {row["heading"]!r} is not one of {page}\'s {allowed}')
for page in route.values():
    check(page in pages, f'{page} has no fixed heading list')
# ---- One column rule: an MM card sits in the column of its headings, the same column OoT's rows of that heading use;
# a card whose headings straddle the columns could not follow it.
cards = {}
for row in rows:
    cards.setdefault((row['page'], row['card']), set()).add(row['heading'] in second_column.get(route[row['page']], []))
for (page, card), columns in cards.items():
    check(len(columns) == 1, f'MM card {card} on {page} has headings in both columns')
custom = body(read('combo/gui/ComboRandomizerProfiles.cpp'), 'void DrawCustom(')
check('RandomizerHeadingColumn(*layout, heading.c_str())' in custom, "MM cards do not take their headings' column")
check('RandomizerHeadingColumn(*layout, layout->headings[row.group])' in body(headings_h, 'ArrangeOotRandomizerPage('),
      "OoT rows do not take their headings' column")
navigation = read('combo/gui/ComboNavigation.h')
order = re.findall(r'\{ "([^"]+)", "', body(navigation, 'inline constexpr RandomizerPageInfo kRandomizerPages[]'))
check(order.index('Dungeons') == order.index('Items & checks') + 1, 'Dungeons does not follow Items & checks')

# ---- Live trap settings: not on the Randomizer tab, still a widget, and a pointer where they were.
profiles = read('combo/gui/ComboRandomizerProfiles.cpp')
native_menu = read('mm/2s2h/Rando/Menu.cpp')
for cvar in fixture['trapCVars']:
    check(f'"{cvar}"' not in profiles, f'{cvar} is still drawn on the Randomizer tab')
    check(f'"{cvar}"' in native_menu, f'{cvar} lost its native widget (Rando/Menu.cpp)')
check('Trap look and effects: Enhancements > Extra modes > Traps and Randomizer extras > Item appearance.' in profiles,
      'the Traps page lost its pointer to the live trap settings')

# ---- Seed options without a logic effect stay, marked.
check('"No logic effect. Fixed when the seed is made."' in profiles, 'Swordless Epona Items lost its marker')
check('No logic effect. Fixed when the seed is made.' in by_option['RO_PURCHASE_INFINITE_RUPEES']['line'],
      'Repeatable Rupee Purchases lost its marker')
check(by_option['RO_PURCHASE_INFINITE_RUPEES']['heading'] == 'Shops & merchants',
      'Repeatable Rupee Purchases is not with the shops')
placement = read('combo/gui/ComboMenuPlacement.h')
for cvar in ('gRandoSettings.SwordlessEponaItems', 'gRandoSettings.SkipChildStealth'):
    check(f'"{cvar}", "randomizer", "Gameplay rules", "Seed settings (no logic effect)"' in placement,
          f'{cvar} left the Gameplay rules heading "Seed settings (no logic effect)"')
sidebar = body(profiles, 'void DrawSidebar(')
check('gRandoSettings.SkipChildStealth' in sidebar and 'ReadInteger' in sidebar,
      'DrawSidebar no longer hides Skip Child Stealth while it is off')
check('"Seed settings (no logic effect)"' not in sidebar, 'DrawSidebar draws a second seed-settings heading')

# ---- One row style: Randomizer rows of both worlds (MM's through CatalogRow) and the goal counts use the slider and
# checkbox rows of the other tabs and the gameplay rule rows.
widget = body(profiles, 'void DrawWidget(const CwWidget& w, const GameMenu& game, bool showHidden)')
check('ComboMenu_SliderRowInt(' in widget and 'ImGui::SliderInt(' not in widget,
      'Randomizer sliders do not use the one slider row')
check('ComboMenu_CheckboxRow(' in widget and 'ImGui::Checkbox(' not in widget,
      'Randomizer checkboxes do not use the one checkbox row')
check('ImGui::Checkbox(' not in body(profiles, 'static void CatalogRow('), 'an MM Randomizer checkbox has another style')
check('"defaults"' in body(profiles, 'static void CatalogRow('), 'MM Randomizer sliders have no Default to return to')
check('{ "gRandoSettings.BigPoeTargetCount", "Big Poes required for reward"' in profiles,
      'Big Poe count does not read "<thing> required for <reward>" like the MM count rules')
# Starting inventory > Other: OoT's counts (Starting Hearts, Gold Skulltula Tokens) and choices (Link's Pocket) are
# full-width rows after the icons, the counts the one slider row as MM's Starting Hearts is; inside an icon cell their
# labels wrapped a letter per line.
icons = body(profiles, 'static std::string StartingIcons(int world)')
check('ImGui::SliderInt(' not in icons, 'a starting count is a bare slider, not the one slider row')
check(re.search(r'row\.at\("kind"\)\s*==\s*"StartingItemCount"\s*\|\|\s*row\.value\("tiers", Json::array\(\)\)\.empty\(\)',
                icons) is not None, 'OoT starting counts and choices are not taken out of the icon grid')
wide = body(icons, 'const auto drawWideRows = [&]')
check('widget.kind = CW_SLIDER_INT;' in wide and 'DrawWidget(widget, ComboMenuModel::Get().Oot());' in wide and
      '"defaults"' in wide, 'OoT starting counts do not use the slider row with a Default')
check(icons.index('ImGui::EndTable();') < icons.rindex('drawWideRows();'), 'the wide rows are not drawn after the icons')
oot_layout = read('soh/soh/ProfileOotStartingLayout.inc')
check(re.search(r'RSK_STARTING_HEARTS\);[^\n]*\{"category","Other"\},\{"kind","StartingItemCount"\}', oot_layout) and
      by_option['RO_STARTING_HEALTH']['heading'] == 'Other' and '{"kind","slider"}' in by_option['RO_STARTING_HEALTH']['line'],
      'Starting Hearts is not a count under Other in both worlds')

# ---- One name for an option both worlds list under the same heading: OoT's display label is MM's row label.
display = dict(re.findall(r'\{ "(gRandoSettings\.\w+)", "([^"]+)"', body(profiles, 'static constexpr OptionDisplay kOptionDisplay[]')))
for oot_key, mm_option in (('gRandoSettings.ShuffleBossSouls', 'RO_SHUFFLE_BOSS_SOULS'),
                           ('gRandoSettings.SoulsEnemyOot', 'RO_SHUFFLE_ENEMY_SOULS'),
                           ('gRandoSettings.ShuffleFreestanding', 'RO_SHUFFLE_FREESTANDING_ITEMS'),
                           ('gRandoSettings.ShuffleSwim', 'RO_SHUFFLE_SWIM'),
                           ('gRandoSettings.ShuffleOcarinaButtons', 'RO_SHUFFLE_OCARINA_BUTTONS'),
                           ('gRandoSettings.IncludeTycoonWallet', 'RO_SHUFFLE_TYCOON_WALLET')):
    mm_label = by_option[mm_option]['label']
    check(display.get(oot_key) == mm_label,
          f'{oot_key} reads {display.get(oot_key)!r} in the OoT view but MM names it {mm_label!r}')

# ---- Combined goal controls still write their keys.
goals = body(menu, 'void DrawCombinedGoals()')
for key in fixture['combinedGoalKeys']:
    check(key in goals, f'DrawCombinedGoals no longer writes {key}')
for name in ('"Goal"', '"Triforce Goal"', '"Triforce Pieces"', '"Logic"', '"Starting world"', '"Ganon & Majora"',
             '"Any Final Boss"', '"All Locations"', '"Beatable Only"'):
    check(name in goals, f'DrawCombinedGoals lacks the OoTMM name {name}')
check('ImGui::SliderInt(' not in goals, 'the Triforce counts do not use the one slider row')
for alias in ('victory condition', 'reachability'):
    check(alias in body(menu, 'void ComboMenu::DrawSearchResults(const std::string& query)'),
          f'search lost the old name {alias!r}')

if failures:
    raise SystemExit('FAIL:\n  ' + '\n  '.join(failures))
print(f'PASS: {len(rows)} MM rows kept with aliases, shared headings, trap settings homed, seed-only rows marked')
