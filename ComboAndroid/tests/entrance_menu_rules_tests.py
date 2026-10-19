"""Compile the shared entrance availability/draft boundary in DEV and SHIPPING."""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx
p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
a.build_dir.mkdir(parents=True, exist_ok=True)
# Compile the actual headless profile/CLI precedence boundary, without loading engines.
headless = (a.source/'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
def extract(signature):
    start = hostcxx.find(headless, signature)
    opening = headless.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (headless[end] == '{') - (headless[end] == '}')
        end += 1
    return headless[start:end]
(a.build_dir/'headless_settings.inc').write_text('#include "rando/entrances/EntranceProfile.h"\n' + extract('struct HeadlessEntranceSettings {') + ';\n' +
    extract('HeadlessEntranceSettings EntranceSettingsForHeadless('), encoding='utf-8', newline='\n')
for dev in (0, 1):
    exe = hostcxx.build(a.compiler, a.source/'ComboAndroid/tests/entrance_menu_rules_tests.cpp',
        includes=[a.source/'combo', a.json_include, a.build_dir], defines=(['COMBO_ENTRANCE_DEV=1'] if dev else []) + ['_CRT_SECURE_NO_WARNINGS'],
        msvc=['/bigobj', '/O2'], gnu=['-O2'], exe=a.build_dir/f'menu_rules_{dev}.exe')
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
# The profile page uses the same predicate on the draft's own Home mode. Its Generate button refuses the setting in
# every randomizer mode, as generation does (ProfileBeginGeneration for a solo mode), not only in combined mode; vanilla
# preparation ignores it (A-08).
profiles = (a.source/'combo/gui/ComboRandomizerProfiles.cpp').read_text(encoding='utf-8')
def body(signature):
    start = hostcxx.find(profiles, signature)
    return profiles[start:profiles.index('\n}\n', start)]
assert hostcxx.contains(body('static std::string EntranceUnavailable()'), 'Entrances::ProfileWallmasterRefusal(document,')
assert hostcxx.contains(body('void DrawEntranceSetting(int index)'), 'const auto unavailable = EntranceUnavailable();')
generation = body('void DrawGeneration()')
assert hostcxx.contains(generation, 'const auto entranceRefusal = !VanillaMode(mode) && '
                        'Entrances::Active(MenuEntranceSettings().erWallmasters) ? EntranceUnavailable() : "";'), \
    'Generate must refuse Wallmaster Shuffle in the solo modes'
assert hostcxx.contains(generation, 'ImGui::BeginDisabled(!Editable() || running || !entranceRefusal.empty());')
# P1-03 (owner decision D3): the row, its tooltip and the Generate button show the plain sentence, and so does the
# generation error, whose log line keeps the developer text. The OoT entrance rows name no developer concept.
assert hostcxx.contains(body('static std::string EntranceUnavailable()'), '.player;')
assert hostcxx.contains(body('void DrawWidget(const CwWidget& w'), 'if (blockedByEntrances) '
                        'Tooltip("Turn off Wallmaster Shuffle to use this option.");')
for word in ('GLOBAL', 'deferred', 'One-Way', "Ship's own"):
    assert word not in profiles, word
ship = (a.source/'combo/ComboShip.cpp').read_text(encoding='utf-8')
refused = ship[hostcxx.find(ship, 'GenerateEntranceLayout(unified, sEntranceNative, sTempleProbe);'):]
refused = refused[hostcxx.find(refused, '} catch (const ComboRando::Entrances::UnifiedEntranceRefused& error) {'):]
refused = refused[:hostcxx.find(refused, 'return;')]
assert hostcxx.contains(refused, 'std::cerr << "[ComboShip] RunComboFill: entrance generation refused: " << '
                        'error.what() << "\\n";')
assert hostcxx.contains(refused, 'fail(error.player().c_str());')
assert hostcxx.contains(ship, 'fail(ComboRando::EntranceGenerationRefusal(modeFacts).player.c_str());')
# Each sentence names a row the player can find under that name (EntranceAvailability.h).
named = {
    'soh/soh/Enhancements/randomizer/settings.cpp': 'OPT_U8(RSK_LOGIC_RULES, "Logic", {"Glitchless", "No Logic"},',
    'soh/soh/Enhancements/ExtraModes/EnemyRandomizer.cpp': 'AddWidget(path, "Enemy Randomizer", WIDGET_CVAR_COMBOBOX)',
    'combo/gui/ComboMenuSearch.h': 'return "Warp Songs and Soaring Spots Shuffle";',
    'combo/rando/GameplayRules.h': '{ "gCombo.Enhancements.ForeignSongs", "Play warp songs across games",',
}
for path, row in named.items():
    assert hostcxx.contains((a.source/path).read_text(encoding='utf-8'), row), (path, row)
# Ship's OoT entrance shuffles (reason 2). The menu moves some of them off the Entrances heading the Wallmaster row is
# under (ComboNavigation.h MenuGroupRoute), so the sentence names each moved row by the label the player sees, and its
# page.
availability = (a.source/'combo/rando/entrances/EntranceAvailability.h').read_text(encoding='utf-8')
listed = availability[hostcxx.find(availability, 'inline constexpr const char* kShipEntranceSettings[] = {'):]
ship_keys = re.findall(r'"gRandoSettings\.(\w+)"', listed[:listed.index('};')])
assert len(ship_keys) == 9, ship_keys
declaration = availability[hostcxx.find(availability, 'inline constexpr const char* kWallmasterShipEntrancesText ='):]
sentence = ''.join(re.findall(r'"([^"]*)"', declaration[:declaration.index(';')]))
navigation = (a.source/'combo/gui/ComboNavigation.h').read_text(encoding='utf-8')
oot_logic = navigation[hostcxx.find(navigation, 'if (oot && section == "Randomizer" && page == "Logic/Access") {'):]
oot_logic = oot_logic[:oot_logic.index('\n    }\n')]
routes = re.findall(r'if \(([^)]*)\)\s*return \{ "randomizer", "([^"]+)" \};', oot_logic)
moved = {key: page for condition, page in routes for key in re.findall(r'key == "gRandoSettings\.(\w+)"', condition)}
native = (a.source/'soh/soh/Enhancements/randomizer/settings.cpp').read_text(encoding='utf-8')
named_rows = []
for key in ship_keys:
    option = r'OPT_\w+\(RSK_\w+, "([^"]+)",[^\n]*CVAR_RANDOMIZER_SETTING\("' + key + r'"\)'
    label = re.search(option, native).group(1)
    assert not re.search(r'\{ "gRandoSettings\.' + key + r'", "', profiles), f'{key} is renamed on the profile page'
    if key in moved:
        assert label in sentence and moved[key] in sentence, (key, label, moved[key], sentence)
        named_rows.append(label)
assert sorted(named_rows) == ['Owl Drops', 'Warp Songs'], named_rows
