"""MM spells: the cast gate, the Farore's Wind button, and Save Editor ownership against a real pickup.

Builds one program from the production functions and runs it:

1. Cast gate truth tables. Combo_MMSpellUseItem / Combo_MMSpellStart (ComboSpellPlayer.inc.h) over
   Combo_MMSpellEnabled (Spells.cpp), and Combo_MMFaroreUseItem (ComboFarorePlayer.inc.h) over Combo_MMFaroreEnabled
   (FaroresWind.cpp), for every combination of their inputs. As in OoTMM v32.3 Player_CustomUseItem, ownership is
   not checked: the spell on the button casts when the seed has it, the device integration and the spell's resources
   are ready, Link is human and the magic suffices (Farore's Wind: or a warp point is set).
2. The Farore's Wind button (z_parameter.c COMBO_MM_FARORE_RESTRICTED) is greyed out exactly where Song of Soaring is
   restricted (OoTMM z_parameter.c Interface_GetItemRestriction), on the C buttons and on the D-pad. Termina Field
   and Woodfall Temple are not such scenes.
3. Save Editor. For every trade-slot item and both seed-option states, the slot picker and Give All record the same
   ownership as the real pickup (Rando::GiveItem's cases after Rando::IsItemObtainable), except that Farore's Wind,
   which IsItemObtainable does not gate, is owned only in a seed with Farore's Wind (MM), the only seed that casts it.
   The picker and the one-item toggle both add through AddSlotItem, which with "Restrict to valid items" on refuses an
   item the save cannot own; the toggle gives the Bow and bombs capacity level 1 and leaves it when removing them.
   After Give All the 2S2H pause cycle (KaleidoItemPage.cpp BuildAvailableItemsList) offers every trade item the seed
   has, so both title deeds, the spells and the gear, and the ocarina slot shows the Ocarina of Time. Reset then
   clears that ownership, so the cycle offers nothing in the emptied slots, and takes the boots and tunic off Link. A
   non-rando save gets no rando ownership and Reset leaves its rando state alone. A save whose spells came from the
   0.4.1 Give All (slot set, owned flags 0) casts without any change to the save.
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
p.add_argument('--reference', type=Path)
a = p.parse_args()
root = a.source.resolve()
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def squash(text):
    return re.sub(r'\s+', '', text)


def function(source, signature):
    """Production text from signature through the brace that closes its body."""
    at = hostcxx.find(source, signature)
    depth, i = 0, source.index('{', at)
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return source[at:i]


def no_this(code):
    """The player overlay is C; its parameter named this becomes self in the C++ harness."""
    return re.sub(r'\bthis\b', 'self', code)


spells = read('mm/2s2h/Rando/Spells.cpp')
farore = read('mm/2s2h/Rando/FaroresWind.cpp')
spell_player = read('mm/src/overlays/actors/ovl_player_actor/ComboSpellPlayer.inc.h')
farore_player = read('mm/src/overlays/actors/ovl_player_actor/ComboFarorePlayer.inc.h')
parameter = read('mm/src/code/z_parameter.c')
editor = read('mm/2s2h/DeveloperTools/SaveEditor.cpp')
kaleido = read('mm/2s2h/Rando/MiscBehavior/KaleidoItemPage.cpp')
give = read('mm/2s2h/Rando/GiveItem.cpp')
convert = read('mm/2s2h/Rando/ConvertItem.cpp')
benport = read('mm/2s2h/BenPort.cpp')
rando_h = read('mm/2s2h/Rando/Rando.h')
static_items = read('mm/2s2h/Rando/StaticData/Items.cpp')
inventory = read('mm/src/code/z_inventory.c')

# ---- Real pickup: Rando::GiveItem's case for each trade-slot item, after IsItemObtainable's option cases.
TRADE_RIS = ['RI_MOONS_TEAR', 'RI_DEED_LAND', 'RI_DEED_SWAMP', 'RI_DEED_MOUNTAIN', 'RI_DEED_OCEAN', 'RI_ROOM_KEY',
             'RI_LETTER_TO_MAMA', 'RI_LETTER_TO_KAFEI', 'RI_PENDANT_OF_MEMORIES', 'RI_FARORES_WIND', 'RI_NAYRUS_LOVE',
             'RI_DINS_FIRE', 'RI_BOOTS_IRON', 'RI_BOOTS_HOVER', 'RI_TUNIC_GORON', 'RI_TUNIC_ZORA']
give_body = function(give, 'void Rando::GiveItem(RandoItemId randoItemId)')
pickup_cases = []
for ri in TRADE_RIS:
    m = re.search(r'case ' + ri + r':\n.*?break;', give_body, re.S)
    check(m is not None, f'Rando::GiveItem has no case {ri}')
    if m:
        pickup_cases.append(m.group())
obtainable = re.search(r'case RI_DINS_FIRE:.*?RO_MM_TUNIC_ZORA\] == RO_GENERIC_YES;',
                       function(convert, 'bool Rando::IsItemObtainable('), re.S)
check(obtainable is not None, 'IsItemObtainable no longer gates the MM spells and gear on their options')
item_of = {}
for ri in TRADE_RIS:
    m = re.search(r'RI\(' + ri + r',\s*"[^"]*",\s*"(?:[^"\\]|\\.)*",\s*\w+,\s*(\w+),', static_items)
    check(m is not None, f'StaticData::Items has no row for {ri}')
    if m:
        item_of[ri] = m.group(1)

# ---- gItemSlots (z_inventory.c, COMBO_BUILD), replayed as assignments so later designators win as in C.
slots = inventory[inventory.index('u8 gItemSlots[256] = {'):]
slots = slots[slots.index('{') + 1:slots.index('};')]
slots = re.sub(r'#ifdef COMBO_BUILD\n(.*?)#else\n.*?#endif\n', r'\1', slots, flags=re.S)
slots = re.sub(r'//[^\n]*', '', slots)
slot_init = ['int at = 0;']
for element in (e.strip() for e in slots.split(',')):
    if not element:
        continue
    m = re.match(r'\[(.+?)\]\s*=\s*(.*)$', element, re.S)
    slot_init.append(f'at = {m.group(1)}; gItemSlots[at++] = {m.group(2).strip()};' if m else
                     f'gItemSlots[at++] = {element};')

# ---- Save Editor pieces.
# The combo build's slot (T8): the picker and the toggle add through AddSlotItem; Give all and Reset are functions.
slot_code = function(editor, '// ComboShip: one inventory slot, by the tap rule both editors share')
check(hostcxx.contains(slot_code, 'if (tap == Kit::Tap::Toggle) { ToggleSlotItem(slot); }') and
      re.search(r'Kit::PickerEntry\(entry\+\+,[^;]*\)\)\s*\{\s*AddSlotItem\(selectedInventorySlot, id\);', slot_code),
      'the combo slot does not add through AddSlotItem')
add_slot = function(editor, 'static bool AddSlotItem(InventorySlot slot, ItemId item)')
check('GrantSlotItemOwnership(item)' in add_slot, 'AddSlotItem records no ownership')
toggle_slot = function(editor, 'static void ToggleSlotItem(InventorySlot slot)')
check('AddSlotItem(slot, item)' in toggle_slot, 'the toggle does not add through AddSlotItem')
items_tab = function(editor, '// ComboShip: the Inventory tab, mirrored by OoT')
check('GiveAllSlotItems();' in items_tab and 'ResetSlotItems();' in items_tab, 'the combo Inventory tab lost Give all/Reset')
give_all = function(editor, 'static void GiveAllSlotItems()')
give_all = give_all[give_all.index('{'):]
check('GiveAllSlotItem(static_cast<InventorySlot>(i))' in give_all, 'Give All does not use GiveAllSlotItem')
reset = function(editor, 'static void ResetSlotItems()')
reset = reset[reset.index('{'):]
check('ClearSlotItemOwnership();' in reset, 'Reset does not clear the ownership Give All records')
trade_table = re.search(r'static const std::pair<ItemId, RandoInf> kSlotTradeItems\[\] = \{.*?\};', editor, re.S)
check(trade_table is not None, 'SaveEditor.cpp has no kSlotTradeItems table')
safe_lists = re.search(r'std::vector<ItemId> safeItemsForInventorySlot\[SLOT_MASK_FIERCE_DEITY \+ 1\] = \{\};', editor)
check(safe_lists is not None, 'safeItemsForInventorySlot declaration changed')

# ---- Farore's Wind button: the macro, used in the C-button and D-pad restriction checks.
macro = re.search(r'#define COMBO_MM_FARORE_RESTRICTED\(play, item\)(?:[^\n]*\\\n)*[^\n]*', parameter)
check(macro is not None, 'z_parameter.c has no COMBO_MM_FARORE_RESTRICTED')
part2 = squash(function(parameter, 'static void Interface_UpdateButtonsPart2Native(PlayState* play)'))
for button, status in (('GET_CUR_FORM_BTN_ITEM(i);', 'gSaveContext.buttonStatus[i]!=BTN_DISABLED'),
                       ('DPAD_GET_CUR_FORM_BTN_ITEM(j);', 'gSaveContext.shipSaveContext.dpad.status[j]!=BTN_DISABLED')):
    expected = ('ItemIditemId=' + button + 'if(GameInteractor_Should(VB_ITEM_BE_RESTRICTED,#ifdefCOMBO_BUILD'
                '!gPlayerFormItemRestrictions[GET_PLAYER_FORM][itemId]||COMBO_MM_FARORE_RESTRICTED(play,itemId),#else'
                '!gPlayerFormItemRestrictions[GET_PLAYER_FORM][itemId],#endif&itemId)){//ItemnotusableincurrentplayerFormif('
                + status + ')')
    check(expected in part2, f'the {button} restriction check does not grey Farore\'s Wind where Soaring is restricted')

# Scenes where Song of Soaring is restricted (scene_table.h RESTRICTIONS_SET argument 8 or a named pattern).
patterns = {name: args for name, args in re.findall(r'#define (RESTRICTIONS_\w+) RESTRICTIONS_SET\(([^)]*)\)', parameter)}
soaring_scenes = []
for scene, flags in re.findall(r'DEFINE_SCENE\(\w+,\s*(SCENE_\w+),\s*\w+,\s*\w+,\s*(RESTRICTIONS_\w+(?:\([^)]*\))?)',
                               read('mm/include/tables/scene_table.h')):
    args = flags[flags.index('(') + 1:-1] if '(' in flags else patterns[flags]
    if int(args.split(',')[7]) != 0:
        soaring_scenes.append(scene)
check('SCENE_00KEIKOKU' not in soaring_scenes and 'SCENE_MITURIN' not in soaring_scenes,
      'Song of Soaring is restricted in Termina Field or Woodfall Temple')
check('SCENE_OKUJOU' in soaring_scenes and 'SCENE_SOUGEN' in soaring_scenes,
      f'unexpected Song of Soaring restrictions: {soaring_scenes}')

# ---- The OoTMM v32.3 rules this copies.
reference = a.reference
if reference is None:
    for candidate in (root.parent.parent / 'references/ootmm-stable-32.3', root.parent / 'references/ootmm-stable-32.3'):
        if (candidate / 'packages/generator/src/mm/z_parameter.c').exists():
            reference = candidate
            break
if reference is not None:
    mm = reference / 'packages/generator/src/mm'
    check('if (item == ITEM_MM_SPELL_WIND && play->interfaceCtx.restrictions.songOfSoaring)' in
          (mm / 'z_parameter.c').read_text(encoding='utf-8'), 'OoTMM z_parameter.c no longer greys Farore\'s Wind')
    use = function((mm / 'actors/Player.c').read_text(encoding='utf-8'),
                   's32 Player_CustomUseItem(Player* this, PlayState* play, s32 itemAction)')
    use = use[:use.index('s32 boots')]
    check('gMmExtraTrade' not in use and 'gSaveContext.save.info.playerData.magic >= sMagicSpellCosts[magicSpell]' in use,
          'OoTMM Player_CustomUseItem now checks more than the magic and the Farore point')
    print(f'OoTMM reference checked: {reference}')
else:
    print('OoTMM reference not found; checked the production sources only')

code = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <map>
#include <utility>
#include <vector>
typedef unsigned char u8;
typedef signed char s8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
#include "z64item.h"
#include "Rando/Types.h"
#include "Rando/Ocarinas.h"
#include "ComboAdultGearInventory.h"
#define COMBO_EXPORT
typedef enum { SAVETYPE_VANILLA, SAVETYPE_RANDO } SaveType;
struct RespawnData { s32 data, entrance; };
struct ComboFaroreSaveInfo { u8 owned; RespawnData point[2]; };
struct RandoSaveInfo { u32 randoSaveOptions[RO_MAX]; u8 ocarinaOwnedBits; };
struct ComboAdultMmSave { u8 boots, tunic, gearTrade2Bits, gearTrade3Bits; };
struct ShipSaveInfo { SaveType saveType; RandoSaveInfo rando; ComboFaroreSaveInfo faroresWind; u8 spellFireOwned, spellLoveOwned; ComboAdultMmSave adultMm; };
struct Inventory { u8 items[48]; };
struct PlayerData { s16 magic; };
struct SaveInfo { Inventory inventory; PlayerData playerData; };
struct Save { SaveInfo saveInfo; ShipSaveInfo shipSaveInfo; };
struct SaveContext { s32 fileNum; Save save; u8 magicCapacity; u8 magicState; };
SaveContext gSaveContext;
bool gRandoInf[RANDO_INF_MAX];
void Flags_SetRandoInf(s32 flag) { gRandoInf[flag] = true; }
void Flags_ClearRandoInf(s32 flag) { gRandoInf[flag] = false; }
bool Flags_GetRandoInf(s32 flag) { return gRandoInf[flag]; }
u8 gItemSlots[256];
void* gPlayState = nullptr;
void Item_Give(void*, int item) { gSaveContext.save.saveInfo.inventory.items[gItemSlots[item]] = u8(item); }
''' + re.search(r'#define IS_RANDO .*', rando_h).group() + '\n' + re.search(r'#define RANDO_SAVE_OPTIONS .*', rando_h).group() + r'''
namespace ComboRando {
bool ComboFaroreIntegrationReady = false;
enum SharedFamily { SF_SPELL_WIND, SF_SPELL_FIRE, SF_SPELL_LOVE, SF_BOOTS_IRON, SF_BOOTS_HOVER, SF_TUNIC_GORON, SF_TUNIC_ZORA };
struct SharedFamilyRow { bool publicOption; };
SharedFamilyRow SharedFamilyByIndex(SharedFamily) { return { true }; }
}
extern "C" int Combo_MM_GearOptionReady(void) { return 1; }
bool gGearActive = true;
extern "C" int Combo_MM_GearActive(void) { return gGearActive; }
namespace Rando {
bool IsItemObtainable(RandoItemId randoItemId, int = 0) {
    switch (randoItemId) {
''' + obtainable.group() + r'''
        default:
            return true;
    }
}
namespace StaticData {
struct Item { ItemId itemId; };
std::map<RandoItemId, Item> Items = {
''' + ''.join(f'    {{ {ri}, {{ {item} }} }},\n' for ri, item in item_of.items()) + r'''};
}
}
''' + function(benport, 'extern "C" COMBO_EXPORT int MM_GrantAdultGearItem(int fileNum, int item)') + r'''

// ---- Spells and Farore's Wind: production grants and enabled checks.
bool sEnabled = false;
ComboFaroreSaveInfo& State() { return gSaveContext.save.shipSaveInfo.faroresWind; }
''' + function(spells, 'extern "C" int Combo_MMSpellEnabled(int spell)') + '\n' + \
    function(spells, 'extern "C" void Combo_MMSpellGrantItem(int spell)') + '\n' + \
    function(farore, 'extern "C" void Combo_MMFaroreGrantItem()') + '\n' + \
    function(farore, 'extern "C" int Combo_MMFaroreEnabled()') + r'''

void RealPickup(RandoItemId randoItemId) {
    if (!Rando::IsItemObtainable(randoItemId))
        return; // ConvertItem turns it into junk
    switch (randoItemId) {
''' + '\n'.join(pickup_cases) + r'''
        default:
            break;
    }
}

// ---- Player cast paths.
enum { PLAYER_FORM_DEKU = 3, PLAYER_FORM_HUMAN = 4 };
enum { PLAYER_UNKAA5_5 = 5 };
enum { PLAYER_IA_NONE, PLAYER_IA_COMBO_FARORES_WIND = 0x53, PLAYER_IA_COMBO_NAYRUS_LOVE, PLAYER_IA_COMBO_DINS_FIRE };
enum { MAGIC_STATE_IDLE, MAGIC_STATE_CONSUME_SETUP };
enum { MAGIC_CONSUME_WAIT_PREVIEW = 4 };
enum { CS_ID_GLOBAL_TALK = 0x7B };
constexpr int NA_SE_SY_ERROR = 0x4806;
struct PlayerAnimationHeader {};
const char gPlayerAnim_link_magic_tame[] = "tame";
struct ActionVar { int actionVar1; };
struct Player { int transformation, unk_AA5, itemAction; ActionVar av1; int csId, stateFlags3, skelAnime, currentBoots; };
struct Restrictions { u8 songOfSoaring; };
struct InterfaceContext { Restrictions restrictions; };
struct PlayState { InterfaceContext interfaceCtx; };
int gErrorSounds = 0, gStarted = 0;
bool gResources = true, gConsume = true, gValidPoint = false;
int gFaroreAge = 0;
void Audio_PlaySfx(int sfx) { gErrorSounds += sfx == NA_SE_SY_ERROR; }
extern "C" int Combo_MMSpellPrepareResources(int) { return gResources; }
extern "C" int Combo_MMFarorePrepareResources(void) { return gResources; }
extern "C" int Combo_MMFaroreAge(void) { return gFaroreAge; }
extern "C" int Combo_MMFaroreHasValidPoint(void) { return gValidPoint; }
int Magic_Consume(PlayState*, int, int) { return gConsume; }
void Combo_MMSpellCasting(Player*, PlayState*) {}
void Player_SetAction_PreserveItemAction(PlayState*, Player*, void (*)(Player*, PlayState*), int) { gStarted++; }
void PlayerAnimation_PlayOnceSetSpeed(PlayState*, int*, PlayerAnimationHeader*, float) {}
void func_8083249C(Player*) {}
void func_8082DAD4(Player*) {}
''' + re.search(r'static u8 sMagicSpellCosts\[\] =\s*\{[^}]*\};', spell_player).group() + '\n' + \
    no_this(function(spell_player, 'void Combo_MMSpellUseItem(Player* this,PlayState* play,int spell)')) + '\n' + \
    no_this(function(spell_player, 'void Combo_MMSpellStart(Player* this,PlayState* play,int spell)')) + '\n' + \
    no_this(function(farore_player, 'void Combo_MMFaroreUseItem(Player* this,PlayState* play)')) + '\n' + \
    macro.group() + r'''

// ---- Save Editor and the 2S2H pause cycle.
InventorySlot selectedInventorySlot = SLOT_NONE;
enum { PLAYER_BOOTS_HYLIAN = 1 };
Player gEditorPlayer{};
#define GET_PLAYER(play) (&gEditorPlayer)
int gBootDataCalls = 0;
void func_80123140(void*, Player*) {
    assert(gSaveContext.save.shipSaveInfo.adultMm.boots == 0); // the use path saves the boots first
    gBootDataCalls++;
}
void Inventory_DeleteItem(int, int slot) { gSaveContext.save.saveInfo.inventory.items[slot] = u8(ITEM_NONE); }
''' + safe_lists.group() + '\n' + trade_table.group() + '\n' + \
    function(editor, 'static bool IsInventoryItemPubliclyAvailable(ItemId item)') + '\n' + \
    function(editor, 'void initSafeItemsForInventorySlot()') + '\n' + \
    function(editor, 'static bool GrantSlotItemOwnership(ItemId item)') + '\n' + \
    function(editor, 'static ItemId GiveAllSlotItem(InventorySlot slot)') + '\n' + \
    function(editor, 'static void TakeOffComboGear()') + '\n' + \
    function(editor, 'static void ClearSlotItemOwnership()') + '\n' + \
    function(kaleido, 'u8 GetNextListEntry(const std::vector<u8>& list, u8 current)') + '\n' + \
    function(kaleido, 'std::vector<u8> BuildAvailableItemsList(u8 slot)') + r'''
u8 gAmmo[256];
int gUpgrades[8];
#define AMMO(item) gAmmo[item]
#define CUR_CAPACITY(upgrade) 1
#define CUR_UPG_VALUE(upgrade) gUpgrades[upgrade]
void Inventory_ChangeUpgrade(int upgrade, int value) { gUpgrades[upgrade] = value; }
bool safeMode = true; // "Restrict to valid items"
''' + add_slot + '\n' + toggle_slot + r'''
void Pick(ItemId id) {
    AddSlotItem(selectedInventorySlot, id);
}
void GiveAll() ''' + give_all + r'''
void Reset() ''' + reset + r'''

struct Ownership {
    std::vector<bool> inf;
    u8 fire, love, wind, gear2, gear3, ocarina;
    bool operator==(const Ownership& o) const {
        return inf == o.inf && fire == o.fire && love == o.love && wind == o.wind && gear2 == o.gear2 &&
               gear3 == o.gear3 && ocarina == o.ocarina;
    }
};
Ownership Snapshot() {
    const auto& s = gSaveContext.save.shipSaveInfo;
    return { std::vector<bool>(std::begin(gRandoInf), std::end(gRandoInf)), s.spellFireOwned, s.spellLoveOwned,
             s.faroresWind.owned, s.adultMm.gearTrade2Bits, s.adultMm.gearTrade3Bits, s.rando.ocarinaOwnedBits };
}
bool NoOwnership() {
    const auto& s = gSaveContext.save.shipSaveInfo;
    return std::none_of(std::begin(gRandoInf), std::end(gRandoInf), [](bool b) { return b; }) && !s.spellFireOwned &&
           !s.spellLoveOwned && !s.faroresWind.owned && !s.adultMm.gearTrade2Bits && !s.adultMm.gearTrade3Bits &&
           !s.rando.ocarinaOwnedBits;
}
void Fresh(bool rando, bool options) {
    gSaveContext = {};
    std::fill(std::begin(gUpgrades), std::end(gUpgrades), 0);
    std::fill(std::begin(gAmmo), std::end(gAmmo), u8(0));
    std::fill(std::begin(gRandoInf), std::end(gRandoInf), false);
    gSaveContext.save.shipSaveInfo.saveType = rando ? SAVETYPE_RANDO : SAVETYPE_VANILLA;
    std::fill(std::begin(gSaveContext.save.saveInfo.inventory.items), std::end(gSaveContext.save.saveInfo.inventory.items),
              u8(ITEM_NONE));
    auto& o = gSaveContext.save.shipSaveInfo.rando.randoSaveOptions;
    o[RO_MM_SPELL_WIND] = o[RO_MM_SPELL_LOVE] = o[RO_MM_SPELL_FIRE] = options;
    for (int gear : { RO_MM_BOOTS_IRON, RO_MM_BOOTS_HOVER, RO_MM_TUNIC_GORON, RO_MM_TUNIC_ZORA })
        o[gear] = options ? RO_GENERIC_YES : RO_GENERIC_NO;
}
// Device and Link ready to cast: integration, Farore rules, resources, human, magic.
void ReadyToCast(PlayState& play, Player& player) {
    ComboRando::ComboFaroreIntegrationReady = true;
    sEnabled = gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_SPELL_WIND] != 0;
    gResources = gConsume = true;
    gValidPoint = false;
    gFaroreAge = 0;
    gSaveContext.magicCapacity = 48;
    gSaveContext.magicState = MAGIC_STATE_IDLE;
    gSaveContext.save.saveInfo.playerData.magic = 48;
    play = {};
    player = {};
    player.transformation = PLAYER_FORM_HUMAN;
}
bool Casts(int spell) { // 0 Farore's Wind, 1 Nayru's Love, 2 Din's Fire
    PlayState play{};
    Player player{};
    const auto saved = gSaveContext;
    ReadyToCast(play, player);
    if (spell == 0)
        Combo_MMFaroreUseItem(&player, &play);
    else
        Combo_MMSpellUseItem(&player, &play, spell);
    const bool cast = player.unk_AA5 == PLAYER_UNKAA5_5;
    gSaveContext = saved;
    return cast;
}
std::vector<u8> List(std::initializer_list<int> items) {
    std::vector<u8> out;
    for (int item : items)
        out.push_back(u8(item));
    return out;
}

int main() {
''' + '\n'.join('    ' + s for s in slot_init) + r'''
    initSafeItemsForInventorySlot();
    const std::vector<std::pair<ItemId, RandoItemId>> trade = {
''' + ''.join(f'        {{ {item_of[ri]}, {ri} }},\n' for ri in TRADE_RIS if ri in item_of) + r'''    };

    // 1a. Din's Fire / Nayru's Love: every input of the use and start paths. Owning the spell does not matter.
    for (int spell = 1; spell <= 2; ++spell) {
        for (int bits = 0; bits < (1 << 11); ++bits) {
            const bool ready = bits & 1, rando = bits & 2, file = bits & 4, option = bits & 8, owned = bits & 16,
                       human = bits & 32, resources = bits & 64, capacity = bits & 128, idle = bits & 256,
                       magic = bits & 512, consume = bits & 1024;
            Fresh(rando, false);
            gSaveContext.fileNum = file ? 1 : 3;
            gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[spell == 1 ? RO_MM_SPELL_LOVE : RO_MM_SPELL_FIRE] = option;
            (spell == 1 ? gSaveContext.save.shipSaveInfo.spellLoveOwned : gSaveContext.save.shipSaveInfo.spellFireOwned) = owned;
            ComboRando::ComboFaroreIntegrationReady = ready;
            gResources = resources;
            gConsume = consume;
            gSaveContext.magicCapacity = capacity ? 48 : 0;
            gSaveContext.magicState = idle ? MAGIC_STATE_IDLE : MAGIC_STATE_CONSUME_SETUP;
            gSaveContext.save.saveInfo.playerData.magic = magic ? sMagicSpellCosts[spell] : sMagicSpellCosts[spell] - 1;
            PlayState play{};
            Player player{};
            player.transformation = human ? PLAYER_FORM_HUMAN : PLAYER_FORM_DEKU;
            gErrorSounds = gStarted = 0;
            Combo_MMSpellUseItem(&player, &play, spell);
            const bool enabled = ready && rando && file && option;
            const bool cast = enabled && human && resources && capacity && idle && magic;
            assert((player.unk_AA5 == PLAYER_UNKAA5_5) == cast);
            assert(!cast || player.itemAction == (spell == 1 ? PLAYER_IA_COMBO_NAYRUS_LOVE : PLAYER_IA_COMBO_DINS_FIRE));
            assert(gErrorSounds == (cast ? 0 : 1));
            Combo_MMSpellStart(&player, &play, spell);
            assert(gStarted == (enabled && resources && consume ? 1 : 0));
        }
    }
    // 1b. Farore's Wind: every input of the use path. Owning it does not matter; a set point needs no magic.
    for (int bits = 0; bits < (1 << 13); ++bits) {
        const bool ready = bits & 1, rando = bits & 2, file = bits & 4, rules = bits & 8, owned = bits & 16,
                   age = bits & 32, human = bits & 64, soaring = bits & 128, resources = bits & 256, point = bits & 512,
                   capacity = bits & 1024, idle = bits & 2048, magic = bits & 4096;
        Fresh(rando, false);
        gSaveContext.fileNum = file ? 2 : -1;
        gSaveContext.save.shipSaveInfo.faroresWind.owned = owned;
        ComboRando::ComboFaroreIntegrationReady = ready;
        sEnabled = rules;
        gFaroreAge = age ? 1 : -1;
        gResources = resources;
        gValidPoint = point;
        gSaveContext.magicCapacity = capacity ? 48 : 0;
        gSaveContext.magicState = idle ? MAGIC_STATE_IDLE : MAGIC_STATE_CONSUME_SETUP;
        gSaveContext.save.saveInfo.playerData.magic = magic ? 12 : 11;
        PlayState play{};
        play.interfaceCtx.restrictions.songOfSoaring = soaring ? 3 : 0;
        Player player{};
        player.transformation = human ? PLAYER_FORM_HUMAN : PLAYER_FORM_DEKU;
        gErrorSounds = 0;
        Combo_MMFaroreUseItem(&player, &play);
        const bool cast = ready && rando && file && rules && age && human && !soaring && resources &&
                          (point || (capacity && idle && magic));
        assert((player.unk_AA5 == PLAYER_UNKAA5_5) == cast);
        assert(!cast || player.itemAction == PLAYER_IA_COMBO_FARORES_WIND);
        assert(gErrorSounds == (cast ? 0 : 1));
    }

    // 2. The Farore's Wind button is greyed out exactly where Song of Soaring is restricted.
    for (int soaring = 0; soaring <= 3; ++soaring) {
        PlayState play{};
        play.interfaceCtx.restrictions.songOfSoaring = u8(soaring);
        for (int item : { ITEM_COMBO_FARORES_WIND, ITEM_COMBO_DINS_FIRE, ITEM_COMBO_NAYRUS_LOVE, ITEM_OCARINA_OF_TIME,
                          ITEM_DEED_LAND, ITEM_NONE })
            assert(bool(COMBO_MM_FARORE_RESTRICTED(&play, item)) == (item == ITEM_COMBO_FARORES_WIND && soaring != 0));
    }

    // 3a. Each trade-slot item: the editor records what the real pickup records, with the seed option off and on.
    for (bool options : { false, true }) {
        for (const auto& [item, randoItem] : trade) {
            if (item == ITEM_COMBO_FARORES_WIND && !options) {
                // Not cast in this seed (Combo_MMFaroreEnabled), so the editor does not own it. With "Restrict to
                // valid items" on the picker refuses it, as Give All leaves it out; with it off the picker only sets
                // the slot.
                Fresh(true, options);
                assert(!GrantSlotItemOwnership(item));
                assert(NoOwnership());
                selectedInventorySlot = SLOT_TRADE_KEY_MAMA;
                Pick(item);
                assert(gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_KEY_MAMA] == ITEM_NONE && NoOwnership());
                safeMode = false;
                Pick(item);
                safeMode = true;
                assert(gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_KEY_MAMA] == item && NoOwnership());
                assert(BuildAvailableItemsList(SLOT_TRADE_KEY_MAMA) == List({ item })); // nothing to switch to
                continue;
            }
            Fresh(true, options);
            RealPickup(randoItem);
            const Ownership real = Snapshot();
            Fresh(true, options);
            const bool owned = GrantSlotItemOwnership(item);
            assert(Snapshot() == real);
            assert(owned == Rando::IsItemObtainable(randoItem));
            if (gItemSlots[item] == SLOT_TRADE_DEED || gItemSlots[item] == SLOT_TRADE_KEY_MAMA ||
                gItemSlots[item] == SLOT_TRADE_COUPLE) {
                Fresh(true, options);
                selectedInventorySlot = static_cast<InventorySlot>(gItemSlots[item]);
                Pick(item);
                // Restrict to valid items (on) refuses an item the save cannot own; off, the picker only sets the slot.
                assert(gSaveContext.save.saveInfo.inventory.items[gItemSlots[item]] == (owned ? item : ITEM_NONE));
                assert(Snapshot() == real);
                if (!owned) {
                    safeMode = false;
                    Pick(item);
                    safeMode = true;
                    assert(gSaveContext.save.saveInfo.inventory.items[gItemSlots[item]] == item);
                    assert(Snapshot() == real);
                }
            }
            Fresh(false, options); // a non-rando save gets no rando ownership
            GrantSlotItemOwnership(item);
            assert(NoOwnership());
        }
    }
    // A pick into another slot (Restrict to valid items off) records nothing.
    Fresh(true, true);
    selectedInventorySlot = SLOT_BOTTLE_1;
    safeMode = false;
    Pick(ITEM_DEED_LAND);
    safeMode = true;
    assert(gSaveContext.save.saveInfo.inventory.items[SLOT_BOTTLE_1] == ITEM_DEED_LAND && NoOwnership());

    // 3a'. The one-item toggle (T8): every slot that holds one valid item toggles it through AddSlotItem. The Bow and
    // bombs get capacity level 1 when they have none; removing them keeps it.
    for (int slot = SLOT_OCARINA; slot <= SLOT_MASK_FIERCE_DEITY; slot++) {
        if (safeItemsForInventorySlot[slot].size() != 1)
            continue;
        Fresh(true, true);
        const InventorySlot s = static_cast<InventorySlot>(slot);
        ToggleSlotItem(s);
        assert(gSaveContext.save.saveInfo.inventory.items[slot] == safeItemsForInventorySlot[slot][0] && NoOwnership());
        ToggleSlotItem(s);
        assert(gSaveContext.save.saveInfo.inventory.items[slot] == ITEM_NONE);
    }
    assert(safeItemsForInventorySlot[SLOT_BOW].size() == 1 && safeItemsForInventorySlot[SLOT_BOMB].size() == 1 &&
           safeItemsForInventorySlot[SLOT_BOMBCHU].size() == 1);
    Fresh(true, true);
    ToggleSlotItem(SLOT_BOW);
    assert(gUpgrades[UPG_QUIVER] == 1 && gAmmo[ITEM_BOW] == 1);
    ToggleSlotItem(SLOT_BOW);
    assert(gSaveContext.save.saveInfo.inventory.items[SLOT_BOW] == ITEM_NONE && gUpgrades[UPG_QUIVER] == 1);
    ToggleSlotItem(SLOT_BOMB);
    assert(gUpgrades[UPG_BOMB_BAG] == 1 && gAmmo[ITEM_BOMB] == 1);
    gUpgrades[UPG_BOMB_BAG] = 3;
    ToggleSlotItem(SLOT_BOMBCHU); // a bomb bag already: only the item changes
    assert(gSaveContext.save.saveInfo.inventory.items[SLOT_BOMBCHU] == ITEM_BOMBCHU && gUpgrades[UPG_BOMB_BAG] == 3 &&
           gAmmo[ITEM_BOMBCHU] == 0);

    // 3b. Both title deeds through the picker: the slot shows the last one, the pause cycle offers both.
    Fresh(true, true);
    selectedInventorySlot = SLOT_TRADE_DEED;
    Pick(ITEM_DEED_LAND);
    Pick(ITEM_DEED_OCEAN);
    assert(gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_DEED] == ITEM_DEED_OCEAN);
    assert(BuildAvailableItemsList(SLOT_TRADE_DEED) == List({ ITEM_DEED_LAND, ITEM_DEED_OCEAN }));

    // 3c. Give All in a seed with every MM spell and gear option.
    Fresh(true, true);
    GiveAll();
    const auto& items = gSaveContext.save.saveInfo.inventory.items;
    const auto& ship = gSaveContext.save.shipSaveInfo;
    for (const auto& [item, randoItem] : trade) {
        Fresh(true, true);
        RealPickup(randoItem);
        const Ownership one = Snapshot();
        Fresh(true, true);
        GiveAll();
        const Ownership all = Snapshot();
        for (size_t i = 0; i < one.inf.size(); ++i)
            assert(!one.inf[i] || all.inf[i]);
        assert((one.fire & ~all.fire) == 0 && (one.love & ~all.love) == 0 && (one.wind & ~all.wind) == 0 &&
               (one.gear2 & ~all.gear2) == 0 && (one.gear3 & ~all.gear3) == 0);
    }
    assert(ship.spellFireOwned && ship.spellLoveOwned && ship.faroresWind.owned);
    assert(ship.adultMm.gearTrade2Bits == 6 && ship.adultMm.gearTrade3Bits == 6);
    assert(items[SLOT_TRADE_DEED] == ITEM_COMBO_DINS_FIRE && items[SLOT_TRADE_KEY_MAMA] == ITEM_COMBO_FARORES_WIND &&
           items[SLOT_TRADE_COUPLE] == ITEM_COMBO_NAYRUS_LOVE);
    assert(items[SLOT_OCARINA] == ITEM_OCARINA_OF_TIME && ship.rando.ocarinaOwnedBits == 3);
    gGearActive = true;
    assert(BuildAvailableItemsList(SLOT_TRADE_DEED) == List({ ITEM_COMBO_DINS_FIRE, ITEM_MOONS_TEAR, ITEM_DEED_LAND,
                                                             ITEM_DEED_SWAMP, ITEM_DEED_OCEAN, ITEM_DEED_MOUNTAIN }));
    assert(BuildAvailableItemsList(SLOT_TRADE_KEY_MAMA) == List({ ITEM_COMBO_FARORES_WIND, ITEM_COMBO_BOOTS_IRON,
                                                                 ITEM_COMBO_TUNIC_GORON, ITEM_ROOM_KEY, ITEM_LETTER_MAMA }));
    assert(BuildAvailableItemsList(SLOT_TRADE_COUPLE) == List({ ITEM_COMBO_NAYRUS_LOVE, ITEM_COMBO_BOOTS_HOVER,
                                                               ITEM_COMBO_TUNIC_ZORA, ITEM_LETTER_TO_KAFEI,
                                                               ITEM_PENDANT_OF_MEMORIES }));
    assert(Casts(0) && Casts(1) && Casts(2));

    // 3d. Give All in a seed without the MM spell and gear options: those stay unowned and out of the slots, which
    // show the last native item as upstream 2S2H's Give All does (the Tingle Map is native to the couple's slot).
    Fresh(true, false);
    GiveAll();
    assert(!ship.spellFireOwned && !ship.spellLoveOwned && !ship.faroresWind.owned);
    assert(ship.adultMm.gearTrade2Bits == 0 && ship.adultMm.gearTrade3Bits == 0);
    assert(items[SLOT_TRADE_DEED] == ITEM_DEED_OCEAN && items[SLOT_TRADE_KEY_MAMA] == ITEM_LETTER_MAMA &&
           items[SLOT_TRADE_COUPLE] == ITEM_TINGLE_MAP);
    assert(BuildAvailableItemsList(SLOT_TRADE_DEED) == List({ ITEM_MOONS_TEAR, ITEM_DEED_LAND, ITEM_DEED_SWAMP,
                                                             ITEM_DEED_OCEAN, ITEM_DEED_MOUNTAIN }));
    assert(BuildAvailableItemsList(SLOT_TRADE_KEY_MAMA) == List({ ITEM_ROOM_KEY, ITEM_LETTER_MAMA }));
    // Only Farore's Wind (MM) off: Din's Fire and Nayru's Love still show, the Room Key/Mama slot shows Letter to Mama.
    Fresh(true, true);
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MM_SPELL_WIND] = 0;
    GiveAll();
    assert(ship.spellFireOwned && ship.spellLoveOwned && !ship.faroresWind.owned);
    assert(items[SLOT_TRADE_DEED] == ITEM_COMBO_DINS_FIRE && items[SLOT_TRADE_KEY_MAMA] == ITEM_LETTER_MAMA &&
           items[SLOT_TRADE_COUPLE] == ITEM_COMBO_NAYRUS_LOVE);
    assert(BuildAvailableItemsList(SLOT_TRADE_KEY_MAMA) == List({ ITEM_COMBO_BOOTS_IRON, ITEM_COMBO_TUNIC_GORON,
                                                                 ITEM_ROOM_KEY, ITEM_LETTER_MAMA }));

    // 3e. Give All in a non-rando save: upstream's native items and the Ocarina of Time, no rando ownership.
    Fresh(false, true);
    GiveAll();
    assert(NoOwnership());
    assert(items[SLOT_TRADE_DEED] == ITEM_DEED_OCEAN && items[SLOT_TRADE_KEY_MAMA] == ITEM_LETTER_MAMA &&
           items[SLOT_TRADE_COUPLE] == ITEM_TINGLE_MAP && items[SLOT_OCARINA] == ITEM_OCARINA_OF_TIME);

    // 3f. Give All, wear the boots and a tunic, Reset: the slots are empty, the pause cycle offers nothing in them,
    // nothing stays owned, and Link takes the gear off as using it again does.
    Fresh(true, true);
    GiveAll();
    gSaveContext.save.shipSaveInfo.adultMm.boots = 1;
    gSaveContext.save.shipSaveInfo.adultMm.tunic = 2;
    gEditorPlayer = {};
    gEditorPlayer.transformation = PLAYER_FORM_HUMAN;
    gEditorPlayer.currentBoots = 6;
    gPlayState = &gEditorPlayer;
    gBootDataCalls = 0;
    Reset();
    gPlayState = nullptr;
    assert(NoOwnership());
    for (int slot : { SLOT_TRADE_DEED, SLOT_TRADE_KEY_MAMA, SLOT_TRADE_COUPLE, SLOT_OCARINA }) {
        assert(items[slot] == ITEM_NONE);
        assert(BuildAvailableItemsList(u8(slot)).empty());
    }
    assert(ship.adultMm.boots == 0 && ship.adultMm.tunic == 0);
    assert(gEditorPlayer.currentBoots == PLAYER_BOOTS_HYLIAN && gBootDataCalls == 1);
    // Outside gameplay only the save changes.
    Fresh(true, true);
    GiveAll();
    gSaveContext.save.shipSaveInfo.adultMm.boots = 2;
    gBootDataCalls = 0;
    Reset();
    assert(NoOwnership() && ship.adultMm.boots == 0 && gBootDataCalls == 0);
    // A non-rando save: Reset leaves its rando state alone.
    Fresh(false, true);
    GiveAll();
    gRandoInf[RANDO_INF_OBTAINED_DEED_LAND] = true;
    gSaveContext.save.shipSaveInfo.adultMm.boots = 1;
    Reset();
    assert(gRandoInf[RANDO_INF_OBTAINED_DEED_LAND] && ship.adultMm.boots == 1);
    assert(items[SLOT_TRADE_DEED] == ITEM_NONE && items[SLOT_OCARINA] == ITEM_NONE);

    // 4. A save from 0.4.1's Give All: spells in the slots, no owned flags. It casts as it is and nothing changes it.
    Fresh(true, true);
    auto& slots = gSaveContext.save.saveInfo.inventory.items;
    slots[SLOT_TRADE_DEED] = ITEM_COMBO_DINS_FIRE;
    slots[SLOT_TRADE_KEY_MAMA] = ITEM_COMBO_FARORES_WIND;
    slots[SLOT_TRADE_COUPLE] = ITEM_COMBO_NAYRUS_LOVE;
    const SaveContext before = gSaveContext;
    assert(Casts(0) && Casts(1) && Casts(2));
    assert(std::memcmp(&before.save, &gSaveContext.save, sizeof(Save)) == 0);
    // The pause cycle lists the unowned item in its slot, so A switches to the one owned item beside it (the
    // emulator's File 3: Farore's Wind with the Iron Boots, Nayru's Love with the Hover Boots and the Zora Tunic).
    assert(BuildAvailableItemsList(SLOT_TRADE_DEED) == List({ ITEM_COMBO_DINS_FIRE }));
    gSaveContext.save.shipSaveInfo.adultMm.gearTrade2Bits = 2; // Iron Boots
    gSaveContext.save.shipSaveInfo.adultMm.gearTrade3Bits = 6; // Hover Boots and Zora Tunic
    const auto mama = BuildAvailableItemsList(SLOT_TRADE_KEY_MAMA);
    assert(mama == List({ ITEM_COMBO_FARORES_WIND, ITEM_COMBO_BOOTS_IRON }));
    assert(GetNextListEntry(mama, ITEM_COMBO_FARORES_WIND) == ITEM_COMBO_BOOTS_IRON);
    assert(BuildAvailableItemsList(SLOT_TRADE_COUPLE) ==
           List({ ITEM_COMBO_NAYRUS_LOVE, ITEM_COMBO_BOOTS_HOVER, ITEM_COMBO_TUNIC_ZORA }));
    // Once switched, the owned item is listed only once.
    slots[SLOT_TRADE_KEY_MAMA] = ITEM_COMBO_BOOTS_IRON;
    assert(BuildAvailableItemsList(SLOT_TRADE_KEY_MAMA) == List({ ITEM_COMBO_BOOTS_IRON }));
    std::puts("MM spell cast gate, Farore's Wind button and Save Editor ownership: OK");
}
'''

if failures:
    print('\n'.join(failures))
    sys.exit(1)
a.build_dir.mkdir(parents=True, exist_ok=True)
source = a.build_dir / 'mm_spell_cast_and_editor.cpp'
source.write_text(code, encoding='utf-8')
binary = hostcxx.build(a.compiler, source, includes=[root / 'mm/include', root / 'mm/2s2h'], defines=['COMBO_BUILD'])
subprocess.run([str(binary)], check=True, timeout=hostcxx.RUN_TIMEOUT)
print(f'Song of Soaring is restricted in {len(soaring_scenes)} scenes: {", ".join(soaring_scenes)}')
