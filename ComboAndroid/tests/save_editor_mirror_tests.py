"""Both Save Editors as mirrors: the shared kit, the same tabs in the same order, and no CVar lost.

The OoT (soh/soh/Enhancements/debugger/debugSaveEditor.cpp) and MM (mm/2s2h/DeveloperTools/SaveEditor.cpp) editors
draw their combo branches with combo/gui/ComboSaveEditorKit.h. This reads the editors as the combo build compiles them
(COMBO_BUILD branches only) and checks:

1. The kit: Android sizes are MM's touch sizes, the colours are the values both games' UIWidgets::ColorValues give
   those names, and the tap rule (one possible item toggles, several open a picker) and the clock text, built and run.
2. Tabs: both editors draw General, Inventory, Equipment, Quest Status, Dungeon Items, Flags, Player in that order;
   MM then adds Randomizer and, last, Reg Editor. OoT's file number is not editable.
3. Every CVar the editors read before T8, and the two window CVars, is still read or still has its widget.
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
p.add_argument('--imgui-include', type=Path, required=True)
a = p.parse_args()
root = a.source.resolve()
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def combo_build(text):
    """The text the combo build compiles: COMBO_BUILD branches kept, the other branches of those conditionals dropped.

    Conditionals on anything else are kept whole."""
    out, stack = [], []  # stack entries: None for other conditionals, else [taken, active]
    for line in text.split('\n'):
        directive = line.strip()
        if directive.startswith('#ifdef COMBO_BUILD') or directive.startswith('#if defined(COMBO_BUILD)') and \
                '&&' not in directive and '||' not in directive:
            stack.append([True, True])
            continue
        if directive.startswith('#ifndef COMBO_BUILD'):
            stack.append([False, False])
            continue
        if directive.startswith(('#if', '#ifdef', '#ifndef')):
            stack.append(None)
        elif directive.startswith(('#elif', '#else')) and stack and stack[-1] is not None:
            entry = stack[-1]
            entry[1] = not entry[0] and directive.startswith('#else')
            entry[0] = entry[0] or entry[1]
            continue
        elif directive.startswith('#endif') and stack:
            if stack.pop() is not None:
                continue
        if all(entry is None or entry[1] for entry in stack):
            out.append(line)
    return '\n'.join(out)


def function(text, signature):
    at = hostcxx.find(text, signature)
    depth, i = 0, text.index('{', at)
    while True:
        depth += (text[i] == '{') - (text[i] == '}')
        i += 1
        if depth == 0:
            return text[at:i]


kit = read('combo/gui/ComboSaveEditorKit.h')
oot_raw = read('soh/soh/Enhancements/debugger/debugSaveEditor.cpp')
mm_raw = read('mm/2s2h/DeveloperTools/SaveEditor.cpp')
oot = combo_build(oot_raw)
mm = combo_build(mm_raw)

# ---- 1. Kit sizes and colours.
android = kit[kit.index('#if defined(__ANDROID__)'):kit.index('#else', kit.index('#if defined(__ANDROID__)'))]
sizes = dict(re.findall(r'inline constexpr (?:float|int) (k\w+) = ([\d.]+)f?;', android))
check(sizes == {'kIconSize': '120.0', 'kGridWidth': '136.0', 'kGridHeight': '180.0', 'kGridPadding': '16.0',
                'kGridTopMargin': '28.0', 'kAmmoWidth': '60.0', 'kAmmoPadding': '8.0', 'kPickerColumns': '5'},
      f'the kit Android sizes are not MM\'s touch sizes: {sizes}')
for game in ('soh/soh/SohGui/UIWidgets.hpp', 'mm/2s2h/BenGui/UIWidgets.hpp'):
    values = dict(re.findall(r'\{ Colors::(\w+), ImVec4\(([^)]*)\) \}', read(game)))
    for name in ('Red', 'DarkRed', 'Green', 'DarkGreen'):
        m = re.search(r'inline constexpr ImVec4 k' + name + r'\(([^)]*)\);', kit)
        check(m is not None and values.get(name) == m.group(1), f'kit k{name} differs from {game} Colors::{name}')
for role, colour in (('kHealth', 'kDarkRed'), ('kHealthMax', 'kRed'), ('kMagic', 'kDarkGreen'), ('kRupees', 'kGreen'),
                     ('kHeartPieces', 'kRed'), ('kGiveAll', 'kGreen'), ('kReset', 'kRed')):
    check(f'inline constexpr ImVec4 {role} = {colour};' in kit, f'kit {role} is not {colour}')

# ---- 2. Tabs.
tab_names = dict(re.findall(r'inline constexpr const char\* (kTab\w+) = "([^"]*)";', kit))


def tabs(text):
    element = function(text, 'void SaveEditorWindow::DrawElement() {')
    found = []
    # BeginTabItem("Name"), BeginTabItem(kTabName) or a { kTabName, DrawFunction } table row, in source order.
    for token in re.findall(r'(?:BeginTabItem\(|\{ )(?:"([^"]*)"|(?:ComboSaveEditorKit::)?(kTab\w+))', element):
        found.append(token[0] or tab_names.get(token[1], token[1]))
    return found


SHARED = ['General', 'Inventory', 'Equipment', 'Quest Status', 'Dungeon Items', 'Flags', 'Player']
check(tabs(oot) == SHARED, f'OoT Save Editor tabs: {tabs(oot)}')
check(tabs(mm) == [t for t in SHARED if t != 'Equipment'] + ['Randomizer', 'Reg Editor'], f'MM Save Editor tabs: {tabs(mm)}')
check('"File Number"' not in oot and not re.search(r'&gSaveContext\.fileNum\b', oot),
      'the OoT Save Editor can still edit the file number')

# ---- 3. CVars: what the editors read at android-source 620b2d3c, and the window CVars.
for text, game, cvars in ((oot, 'OoT', ('CVAR_SETTING("DisableChanges")', 'CVAR_ENHANCEMENT("DpadEquips")')),
                          (mm, 'MM', ('"gEnhancements.Graphics.24HoursClock"', '"gEnhancements.Dpad.DpadEquips"',
                                      '"gSettings.Menu.Theme"'))):
    for cvar in cvars:
        check(cvar in text, f'the {game} Save Editor no longer reads {cvar}')
check(hostcxx.contains(read('soh/soh/SohGui/SohMenuDevTools.cpp'),
                       'AddWidget(path, "Popout Save Editor", WIDGET_WINDOW_BUTTON).CVar(CVAR_WINDOW("SaveEditor"))'),
      'the OoT pop-out button lost gOpenWindows.SaveEditor')
check(hostcxx.contains(read('mm/2s2h/BenGui/BenMenu.cpp'),
                       'AddWidget(path, "Popout Save Editor", WIDGET_WINDOW_BUTTON).CVar("gWindows.SaveEditor")'),
      'the MM pop-out button lost gWindows.SaveEditor')

# ---- 1b. The tap rule and the clock text, built and run.
code = r'''
#include <cassert>
#include <cstdio>
#include <string>
#include "gui/ComboSaveEditorKit.h"
using namespace ComboSaveEditorKit;
int main() {
    static_assert(ToggleOrPick(true, 1) == Tap::Toggle);
    for (size_t candidates : { 0, 2, 3, 13 })
        assert(ToggleOrPick(true, candidates) == Tap::Pick);
    for (size_t candidates : { 0, 1, 2 })
        assert(ToggleOrPick(false, candidates) == Tap::Pick); // no restriction: every slot opens the picker
    assert(std::string(TapHint(Tap::Toggle)) == "Tap to add or remove.");
    assert(std::string(TapHint(Tap::Pick)) == "Tap to choose.");
    assert(ClockText(0x0000) == "12:00 am");
    assert(ClockText(0x4000) == "6:00 am");
    assert(ClockText(0x8000) == "12:00 pm");
    assert(ClockText(0xC000) == "6:00 pm");
    assert(ClockText(0xC001) == "6:00 pm");
    assert(ClockText(0xFFFF) == "11:59 pm");
    assert(ClockText(0x4000, true) == "06:00" && ClockText(0xC000, true) == "18:00");
    // MM's own text (TIME_TO_MINUTES_F) for every value.
    for (int t = 0; t <= 0xFFFF; t++) {
        const int minutes = (int)(t * (24.0f * 60.0f / 0x10000));
        char text[16];
        const int hours = minutes / 60;
        std::snprintf(text, sizeof(text), "%d:%02d %s", hours % 12 ? hours % 12 : 12, minutes % 60, hours >= 12 ? "pm" : "am");
        assert(ClockText((unsigned short)t) == text);
    }
    std::puts("Save Editor kit: OK");
}
'''

if failures:
    print('\n'.join(failures))
    sys.exit(1)
a.build_dir.mkdir(parents=True, exist_ok=True)
source = a.build_dir / 'save_editor_mirror.cpp'
source.write_text(code, encoding='utf-8')
binary = hostcxx.build(a.compiler, source, includes=[root / 'combo', a.imgui_include], warnings=False)
subprocess.run([str(binary)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Save Editor tabs, kit and CVars: OK')
