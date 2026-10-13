"""Both Save Editors as mirrors: the shared kit, the same tabs in the same order, and no CVar lost.

The OoT (soh/soh/Enhancements/debugger/debugSaveEditor.cpp) and MM (mm/2s2h/DeveloperTools/SaveEditor.cpp) editors
draw their combo branches with combo/gui/ComboSaveEditorKit.h. This reads the editors as the combo build compiles them
(COMBO_BUILD branches only) and checks:

1. The kit: Android sizes are MM's touch sizes, the colours are the values both games' UIWidgets::ColorValues give
   those names, and the tap rule (one possible item toggles, several open a picker) and the clock text, built and run.
2. Tabs: both editors draw General, Inventory, Equipment, Quest Status, Dungeon Items, Flags, Player in that order;
   MM then adds Randomizer and, last, Reg Editor. OoT's file number is not editable. Quest Status, Flags and Player
   use the same headings and labels; MM's check list is collapsed and its cross-send button developer-only.
3. Every CVar the editors read before T8, and the two window CVars, is still read or still has its widget.
4. OoT's tap rule (Sam's example 1), run on the production GetLegalInventorySlotItems, ToggleInventorySlot, item
   table and item slots: with "Restrict to valid items" on, Boomerang and the other one-item slots toggle while
   Hookshot, Ocarina, bottles and trade slots open the picker. On does what picking the item does (set it, refresh
   the C/D-pad buttons) plus capacity level 1 and full ammo when there is no capacity yet; off does what None does.
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
check(tabs(mm) == SHARED + ['Randomizer', 'Reg Editor'], f'MM Save Editor tabs: {tabs(mm)}')
# MM's sword, shield, quiver and bomb bag are pickers on Equipment; Quest Status no longer cycles or gives them.
mm_equipment = function(mm, 'void DrawEquipmentTab() {')
check(all(f'Kit::LevelPicker("{name}"' in mm_equipment for name in ('Sword', 'Shield', 'Quiver', 'Bomb Bag')),
      'MM Equipment lacks the sword, shield, quiver or bomb bag picker')
mm_quest = function(mm, 'void DrawQuestStatusTab() {')
check('SET_EQUIP_VALUE' not in mm_quest and 'NextQuestInSlot(QUEST_SWORD)' not in mm_quest and
      'NextQuestInSlot(QUEST_SHIELD)' not in mm_quest, 'MM Quest Status still edits the sword or shield')
for text, game in ((oot, 'OoT'), (mm, 'MM')):
    quest_tab = function(text, 'void DrawQuestStatusTab() {')
    headings = re.findall(r'SeparatorText\("([^"]*)"\)', quest_tab)
    check(headings[:5] == ['Boss rewards', 'Songs', 'Quest items', 'Gold Skulltula tokens', 'Heart pieces'],
          f'{game} Quest Status headings: {headings}')
    check('Kit::kHeartPieces' in quest_tab, f'{game} heart pieces are not a Red bar')
check('"File Number"' not in oot and not re.search(r'&gSaveContext\.fileNum\b', oot),
      'the OoT Save Editor can still edit the file number')

# Flags, Player and Randomizer (spec 6.8-6.10).
oot_flags = function(oot, 'void DrawFlagsTab() {')
check('Combobox("Flag type"' in oot_flags and 'TreeNode("Player State")' not in oot_flags and
      hostcxx.contains(oot_flags, 'if (keepGsCountUpdated && changed)'),
      'OoT Flags: no Flag type list, player state still there, or the token count is rewritten every frame')
mm_flags = function(mm, 'void DrawFlagsTab() {')
check('UIWidgets::Combobox("Flag type"' in mm_flags and 'HEART_FLAGS' not in mm_flags and
      'collectedHearts' not in mm_flags, 'MM Flags: no Flag type list, or collectedHearts still offered')
for text, game in ((oot, 'OoT'), (mm, 'MM')):
    player_tab = function(text, 'void DrawPlayerTab() {')
    for label in ('"Position & rotation"', '"Speed"', '"XZ speed"', '"Y velocity"', '"Ledge height"',
                  '"Invincibility timer"', '"Gravity"', '"Equipped gear"', '"Button items"', '"Clear all##buttons"',
                  '"Player state"', 'Kit::kNeedsPlay'):
        check(label in player_tab, f'{game} Player tab lacks {label}')
mm_rando = function(mm, 'void DrawRandoTab() {')
check(hostcxx.contains(mm_rando, 'if (CVarGetInteger("gDeveloperTools.DebugEnabledMM", 0)) {') and
      mm_rando.index('gDeveloperTools.DebugEnabledMM') < mm_rando.index('Cross-send debug item to OOT') and
      mm_rando.index('CollapsingHeader("Check list (shows item locations)")') < mm_rando.index('BeginTable("Check List"') and
      'DrawRandoItemGives();' in mm_rando and 'DrawRandoItemGives' not in function(mm, 'void DrawItemsAndMasksTab() {'),
      'MM Randomizer tab: cross-send not developer-only, check list not collapsed, or gives not moved here')

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

# ---- 4. OoT tap rule: production pieces.
inventory = function(oot, 'void DrawInventoryTab() {')
check(hostcxx.contains(inventory, 'const Kit::Tap tap = Kit::ToggleOrPick(restrictToValid, legalItems.size());') and
      hostcxx.contains(inventory, 'if (tap == Kit::Tap::Toggle) { ToggleInventorySlot(index, legalItems.front().id, '
                                  'syncButtons); } else { selectedIndex = index; ImGui::OpenPopup(itemPopupPicker); }'),
      'the OoT inventory tap does not follow the tap rule')
check(hostcxx.contains(inventory, 'gSaveContext.inventory.items[selectedIndex] = slotEntry.id; if (syncButtons) { '
                                  'SyncButtonItemsForSlot(selectedIndex); }'),
      'the OoT picker no longer sets the item and refreshes the buttons')
check('ImGui::Text("Ammo")' not in inventory and 'Kit::AmmoCount(' in inventory,
      'OoT ammo counts are not under their icons')
z_inventory = read('soh/src/code/z_inventory.c')
item_slots = re.search(r'u8 gItemSlots\[\] = \{(.*?)\};', z_inventory, re.S).group(1)
capacities = re.search(r'u16 gUpgradeCapacities\[\]\[4\] = \{(.*?)\};', z_inventory, re.S).group(1)
mapping = read('soh/soh/SohGui/ImGuiUtils.cpp')
mapping = mapping[mapping.index('std::map<uint32_t, ItemMapEntry> itemMapping = {'):]
mapping = mapping[:mapping.index('};') + 2]
oot_harness = r'''
typedef unsigned char u8;
typedef signed char s8;
typedef short s16;
typedef unsigned short u16;
#include "z64item.h"
struct ItemMapEntry {
    uint32_t id;
    std::string name, nameFaded, texturePath;
};
#define ITEM_MAP_ENTRY(id) { id, { id, #id, #id "_Faded", "" } }
''' + mapping + r'''
u8 gItemSlots[] = {''' + item_slots + r'''};
u16 gUpgradeCapacities[][4] = {''' + capacities + r'''};
struct {
    struct {
        u8 items[24];
        s8 ammo[16];
    } inventory;
} gSaveContext;
int gUpgrades[8];
#define CUR_UPG_VALUE(upg) gUpgrades[upg]
#define CUR_CAPACITY(upg) gUpgradeCapacities[upg][CUR_UPG_VALUE(upg)]
#define AMMO(item) gSaveContext.inventory.ammo[gItemSlots[item]]
void Inventory_ChangeUpgrade(int upgrade, int value) { gUpgrades[upgrade] = value; }
std::vector<int> gSynced;
static void SyncButtonItemsForSlot(uint8_t slot) { gSynced.push_back(slot); }
''' + function(oot_raw, 'static std::vector<ItemMapEntry> GetLegalInventorySlotItems(InventorySlot slot)') + '\n' + \
    function(oot, 'static int AmmoUpgradeForItem(uint8_t item)') + '\n' + \
    function(oot, 'static void ToggleInventorySlot(InventorySlot slot, uint8_t item, bool syncButtons)') + '\n'

# ---- 1b and 4. The tap rule and the clock text, built and run.
code = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "gui/ComboSaveEditorKit.h"
''' + oot_harness + r'''
using namespace ComboSaveEditorKit;
void Fresh() {
    gSaveContext = {};
    std::fill(std::begin(gSaveContext.inventory.items), std::end(gSaveContext.inventory.items), (u8)ITEM_NONE);
    std::fill(std::begin(gUpgrades), std::end(gUpgrades), 0);
    gSynced.clear();
}
bool NoUpgrades() {
    return std::all_of(std::begin(gUpgrades), std::end(gUpgrades), [](int v) { return v == 0; });
}
void OotTapRule() {
    const std::vector<int> toggles = { SLOT_STICK,      SLOT_NUT,          SLOT_BOMB,      SLOT_BOW,
                                       SLOT_ARROW_FIRE, SLOT_DINS_FIRE,    SLOT_SLINGSHOT, SLOT_BOMBCHU,
                                       SLOT_ARROW_ICE,  SLOT_FARORES_WIND, SLOT_BOOMERANG, SLOT_LENS,
                                       SLOT_BEAN,       SLOT_HAMMER,       SLOT_ARROW_LIGHT, SLOT_NAYRUS_LOVE };
    for (int slot = 0; slot < 24; slot++) {
        const auto legal = GetLegalInventorySlotItems((InventorySlot)slot);
        const bool toggle = ToggleOrPick(true, legal.size()) == Tap::Toggle;
        assert(toggle == (std::find(toggles.begin(), toggles.end(), slot) != toggles.end()));
        assert(ToggleOrPick(false, legal.size()) == Tap::Pick);
        if (toggle)
            assert(gItemSlots[legal.front().id] == slot);
    }
    // Sam's examples: Boomerang just toggles, Hookshot asks Hookshot or Longshot.
    const auto boomerang = GetLegalInventorySlotItems(SLOT_BOOMERANG);
    assert(boomerang.size() == 1 && boomerang.front().id == ITEM_BOOMERANG);
    const auto hookshot = GetLegalInventorySlotItems(SLOT_HOOKSHOT);
    assert(hookshot.size() == 2 && hookshot[0].id == ITEM_HOOKSHOT && hookshot[1].id == ITEM_LONGSHOT);
    assert(GetLegalInventorySlotItems(SLOT_OCARINA).size() == 2 && GetLegalInventorySlotItems(SLOT_BOTTLE_4).size() > 1);

    // On does what picking the item does (set it, refresh the buttons), off what None does.
    Fresh();
    ToggleInventorySlot(SLOT_BOOMERANG, ITEM_BOOMERANG, true);
    assert(gSaveContext.inventory.items[SLOT_BOOMERANG] == ITEM_BOOMERANG && gSynced == std::vector<int>{ SLOT_BOOMERANG });
    assert(NoUpgrades());
    ToggleInventorySlot(SLOT_BOOMERANG, ITEM_BOOMERANG, true);
    assert(gSaveContext.inventory.items[SLOT_BOOMERANG] == ITEM_NONE && gSynced.size() == 2);
    ToggleInventorySlot(SLOT_BOOMERANG, ITEM_BOOMERANG, false); // buttons left alone when sync is off
    assert(gSaveContext.inventory.items[SLOT_BOOMERANG] == ITEM_BOOMERANG && gSynced.size() == 2);
    // An ammo item without capacity gets level 1 and full ammo; removing it keeps the capacity.
    for (auto [slot, item, upgrade, full] : { std::tuple{ SLOT_BOW, ITEM_BOW, UPG_QUIVER, 30 },
                                             std::tuple{ SLOT_BOMB, ITEM_BOMB, UPG_BOMB_BAG, 20 },
                                             std::tuple{ SLOT_STICK, ITEM_STICK, UPG_STICKS, 10 },
                                             std::tuple{ SLOT_NUT, ITEM_NUT, UPG_NUTS, 20 },
                                             std::tuple{ SLOT_SLINGSHOT, ITEM_SLINGSHOT, UPG_BULLET_BAG, 30 } }) {
        Fresh();
        ToggleInventorySlot((InventorySlot)slot, item, true);
        assert(gSaveContext.inventory.items[slot] == item && gUpgrades[upgrade] == 1 &&
               gSaveContext.inventory.ammo[slot] == full);
        ToggleInventorySlot((InventorySlot)slot, item, true);
        assert(gSaveContext.inventory.items[slot] == ITEM_NONE && gUpgrades[upgrade] == 1);
        // With a capacity already, only the item changes.
        gUpgrades[upgrade] = 2;
        gSaveContext.inventory.ammo[slot] = 5;
        ToggleInventorySlot((InventorySlot)slot, item, true);
        assert(gUpgrades[upgrade] == 2 && gSaveContext.inventory.ammo[slot] == 5);
    }
    Fresh(); // Bombchus have no capacity upgrade: their ammo is left to the count under the icon.
    ToggleInventorySlot(SLOT_BOMBCHU, ITEM_BOMBCHU, true);
    assert(gSaveContext.inventory.ammo[SLOT_BOMBCHU] == 0 && NoUpgrades());
}
int main() {
    OotTapRule();
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
binary = hostcxx.build(a.compiler, source, includes=[root / 'combo', root / 'soh/include', a.imgui_include],
                        warnings=False)
subprocess.run([str(binary)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Save Editor tabs, kit, CVars and OoT tap rule: OK')
