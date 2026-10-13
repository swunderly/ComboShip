"""Adult Link's Fairy Ocarina in OoT, recoloured hand (owner ruling 2026-10-12: OoT and MM look the same; t10).

OoT draws its adult hand holding the Ocarina of Time (gLinkAdultRightHandHoldingOotNear/FarDL) for adult Link with
either ocarina. With the release switch kAdultFairyRecolouredOot (soh/soh/OTRGlobals.cpp) on, adult Link holding the
Fairy Ocarina draws instead the copy of that hand MM draws for adult Link, coloured like the Fairy Ocarina, built from
OoT's own manager by the builder both games share (combo/menu/ComboFairyOcarinaRecolourHands.h; its checks are
mm_adult_fairy_recolour_tests.py's).

Synthetic lists only (no ROM-derived data). Compiles, with stand-in engine types, the whole chain OoT takes from the
ocarina to the drawn list, each step from production text: the item an ocarina button holds (sItemActions) or, for
cutscene and forced ocarina actions, the ocarina slot (func_8084B498: INV_CONTENT(ITEM_OCARINA_FAIRY), which holds
whichever ocarina the save has, the randomizer's progressive ocarina included) gives the item action; the action its
model group (sActionModelGroups), the group its right hand (gPlayerModelTypes), the hand its lists (sPlayerDListGroups)
indexed by age (Player_SetModels), and the R_HAND branch of Player_OverrideLimbDrawGameplayDefault with its hook draws
the list through Combo_OotFairyOcarinaHand. For a release build with the switch as committed and flipped, checks every
age, ocarina and LOD:
- adult Link with the Fairy Ocarina draws the recoloured copy of OoT's adult hand for the LOD drawn (switch on), adult
  Link with the Ocarina of Time OoT's own blue hand, and child Link his own Fairy Ocarina or Ocarina of Time hand;
  child Link and the Ocarina of Time never even build the copies;
- where the copies cannot be built (a custom adult list, a missing Fairy Ocarina texture, a child list that loads it
  otherwise, an adult list without exactly one Ocarina of Time texture), OoT's own blue hand is drawn, the reason is
  logged once and the build is not retried for that manager; another manager is tried afresh;
- where OoT serves another list for that hand than the one copied (a texture pack's), that list is drawn;
- the copies are built once per OoT manager, dropped at teardown and rebuilt after it; OoT's cached lists never change;
- with the switch off, every list is OoT's own and nothing is built.
Also checks that both games build the copies with the same shared builder, and that OoT never draws the recoloured hand
while MM does not.
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
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


def read(rel):
    return (root / rel).read_text(encoding='utf-8')


def block(source, start):
    """Text from start through the brace matching the first opening brace at or after it."""
    i = source.index('{', start)
    depth = 0
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return source[start:i]


def this_to_player(text):
    # OoT's C names its actor `this`; the C++ harness calls it `player`.
    return re.sub(r'\bthis\b', 'player', text)


lib = read('soh/src/code/z_player_lib.c')
player_c = read('soh/src/overlays/actors/ovl_player_actor/z_player.c')
z64player = read('soh/include/z64player.h')
otr = read('soh/soh/OTRGlobals.cpp')
benport = read('mm/2s2h/BenPort.cpp')
mm_hand = read('combo/menu/ComboFairyOcarinaHand.h')
shared = read('combo/menu/ComboFairyOcarinaRecolourHands.h')
assets = read('soh/assets/objects/object_link_boy/object_link_boy.h') + read(
    'soh/assets/objects/object_link_child/object_link_child.h')

# ---- How OoT picks the right hand for each ocarina (production text).
IAS = {'PLAYER_IA_OCARINA_FAIRY': 'ITEM_OCARINA_FAIRY', 'PLAYER_IA_OCARINA_OF_TIME': 'ITEM_OCARINA_OF_TIME'}
item_actions = block(player_c, player_c.index('static s8 sItemActions[] = {'))
for ia, item in IAS.items():
    check(re.search(r'\b' + ia + r',\s*// ' + item + r'\n', item_actions) is not None,
          f'an ocarina button holding {item} no longer takes {ia}')
check(hostcxx.contains(player_c, 'return sItemActions[item];'), 'Player_ItemToItemAction no longer reads sItemActions')
slot_at = player_c.index('void func_8084B498(Player* this) {')
slot_action = block(player_c, slot_at)
check(hostcxx.contains(slot_action, 'INV_CONTENT(ITEM_OCARINA_FAIRY) == ITEM_OCARINA_FAIRY'),
      'cutscene ocarina actions no longer pick the ocarina from the ocarina slot')
check(hostcxx.contains(player_c, 'func_8084B498(this);\nPlayer_SetModels(this, Player_ActionToModelGroup(this, '
                                 'this->itemAction));'),
      'the cutscene ocarina action no longer sets the models of the ocarina it picks')
check(hostcxx.contains(player_c, 'func_8084B498(this);\nthis->unk_6AD = 4;'),
      'the forced ocarina pull no longer picks the ocarina from the ocarina slot')
groups = {}
action_groups = block(lib, lib.index('u8 sActionModelGroups[] = {'))
for ia in IAS:
    m = re.search(r'(PLAYER_MODELGROUP_\w+),\s*// ' + ia + r'\n', action_groups)
    check(m is not None, f'{ia} has no model group')
    groups[ia] = m.group(1) if m else ''
to_group = block(lib, lib.index('s32 Player_ActionToModelGroup(Player* this, s32 itemAction) {'))
check(hostcxx.contains(to_group, 's32 modelGroup = sActionModelGroups[itemAction];') and
      to_group.count('modelGroup ==') == 1 and 'modelGroup == PLAYER_MODELGROUP_SWORD_AND_SHIELD' in to_group,
      'Player_ActionToModelGroup changes groups other than the sword and shield')
entries = re.findall(r'/\* 0x[0-9A-Fa-f]+ \*/ PLAYER_MODELGROUPENTRY_(\w+),', z64player)
right_entry = entries.index('RIGHT_HAND') if 'RIGHT_HAND' in entries else -1
check(right_entry == 2, f'the model group entries are {entries}')
model_types = block(lib, lib.index('u8 gPlayerModelTypes[PLAYER_MODELGROUP_MAX][PLAYER_MODELGROUPENTRY_MAX] = {'))
hands, tables = {}, {}
for ia, group in groups.items():
    m = re.search(r'/\* ' + group + r' \*/\s*\{([^}]*)\}', model_types)
    check(m is not None, f'{group} has no model types')
    row = [entry.strip() for entry in m.group(1).split(',')] if m else []
    hands[ia] = row[right_entry] if len(row) > right_entry >= 0 else ''
dlist_groups = block(lib, lib.index('Gfx** sPlayerDListGroups[PLAYER_MODELTYPE_MAX] = {'))
for ia, hand in hands.items():
    m = re.search(r'(\w+),\s*// ' + hand + r'\n', dlist_groups)
    check(m is not None, f'{hand} has no display lists')
    tables[ia] = m.group(1) if m else ''
check(tables.get('PLAYER_IA_OCARINA_FAIRY') == 'sPlayerRightHandOcarinaDLs',
      f'the Fairy Ocarina hand is {tables.get("PLAYER_IA_OCARINA_FAIRY")}')
check(lib.count('PLAYER_MODELTYPE_RH_OCARINA') == 2,
      'something other than the model group table and sPlayerDListGroups special-cases the Fairy Ocarina hand')
set_models = block(lib, lib.index('void Player_SetModels(Player* this, s32 modelGroup) {'))
check(hostcxx.contains(set_models, 'this->rightHandType = gPlayerModelTypes[modelGroup][PLAYER_MODELGROUPENTRY_RIGHT_'
                                   'HAND];\nthis->rightHandDLists = &sPlayerDListGroups[this->rightHandType]'
                                   '[gSaveContext.linkAge];'),
      'Player_SetModels no longer indexes the right hand lists by age')
check(hostcxx.contains(lib, 'sDListsLodOffset = lod * 2;'), 'sDListsLodOffset is no longer twice the LOD')


def table_text(name):
    m = re.search(r'Gfx\* ' + name + r'\[\] = \{(.*?)\};', lib, re.S)
    check(m is not None, f'{name} is not found')
    names = re.findall(r'\b(\w+DL)\b', m.group(1)) if m else []
    paths = []
    for dl in names:
        d = re.search(r'#define d' + dl + r' "(__OTR__[^"]+)"', assets)
        check(d is not None, f'{dl} has no path')
        paths.append(d.group(1) if d else '')
    return paths


TABLES = {name: table_text(name) for name in sorted(set(tables.values()) | {'sPlayerRightHandClosedDLs'})}
check(all(len(paths) == 4 for paths in TABLES.values()), f'the hand tables are not adult/child near, far: {TABLES}')
ADULT_NEAR = '__OTR__objects/object_link_boy/gLinkAdultRightHandHoldingOotNearDL'
ADULT_FAR = '__OTR__objects/object_link_boy/gLinkAdultRightHandHoldingOotFarDL'
check(TABLES.get('sPlayerRightHandOcarinaDLs', [None])[0::2] == [ADULT_NEAR, ADULT_FAR] and
      TABLES.get('sPlayerRightHandOotDLs', [None])[0::2] == [ADULT_NEAR, ADULT_FAR],
      'OoT no longer draws its adult hand holding the Ocarina of Time for both ocarinas')

# ---- The hook and the runtime (production text).
default_at = lib.index('s32 Player_OverrideLimbDrawGameplayDefault(PlayState* play, s32 limbIndex, Gfx** dList,')
right_at = lib.index('if (limbIndex == PLAYER_LIMB_R_HAND) {', default_at)
right_hand = block(lib, right_at)
check(lib.count('Combo_OotFairyOcarinaHand(') == 2 and right_hand.count('Combo_OotFairyOcarinaHand(') == 2,
      'Combo_OotFairyOcarinaHand is declared or called outside the R_HAND branch of the default limb draw')
hook = re.search(r'#ifdef COMBO_BUILD\n(?:\s*//[^\n]*\n)*\s*if \((.*?)\) \{\s*extern Gfx\* Combo_OotFairyOcarinaHand\('
                 r'Gfx\*, s32\);\s*\*dList = Combo_OotFairyOcarinaHand\(\*dList, (.*?)\);\s*\}\s*#endif', right_hand)
check(hook is not None, 'the hook is not a COMBO_BUILD block of its own')
check(hook is not None and hook.group(1) == 'dLists == &sPlayerRightHandOcarinaDLs[0]' and
      hook.group(2) == 'sDListsLodOffset / 2',
      'the hook does not take the adult entries of the Fairy Ocarina hands with the LOD drawn')
check(hook is not None and
      right_hand.index('*dList = ResourceMgr_LoadGfxByName(dLists[sDListsLodOffset]);') < hook.start(),
      'the hook runs before OoT loads its own list')
for rel in ('soh/soh', 'soh/src'):
    for path in (root / rel).rglob('*.c*'):
        if path.name not in ('z_player_lib.c', 'OTRGlobals.cpp'):
            check('Combo_OotFairyOcarinaHand' not in path.read_text(encoding='utf-8', errors='replace'),
                  f'{path.relative_to(root)} uses the recoloured hand')
switch = re.search(r'static constexpr bool kAdultFairyRecolouredOot = (true|false);', otr)
check(switch is not None, 'OTRGlobals.cpp has no release switch for the recoloured hand')
switch = switch.group(1) if switch else 'false'
mm_switch = re.search(r'static constexpr bool kAdultFairyRecolouredOot = (true|false);', benport)
mm_switch = mm_switch.group(1) if mm_switch else 'false'
check(not (switch == 'true' and mm_switch == 'false'), 'OoT draws the recoloured hand while MM does not')
runtime_at = otr.index('static constexpr bool kAdultFairyRecolouredOot')
hand_at = otr.index('extern "C" Gfx* Combo_OotFairyOcarinaHand(Gfx* nativeHand, s32 lod) {')
runtime = otr[runtime_at:hand_at + len(block(otr, hand_at))]
combo_at = otr.rindex('#ifdef COMBO_BUILD', 0, runtime_at)
check(otr.find('#endif', combo_at) > hand_at + len(block(otr, hand_at)) and
      otr[combo_at + 1:runtime_at].count('#') == 1,  # its #include only
      'the OoT runtime is not inside a COMBO_BUILD block of its own')
# Both games build the copies with the shared builder.
for name, text in (('OTRGlobals.cpp', otr), ('ComboFairyOcarinaHand.h', mm_hand)):
    check('#include "ComboFairyOcarinaRecolourHands.h"' in text and
          text.count('Combo_BuildFairyRecolouredHands(*rm, sFairyRecolouredSources, sFairyRecolouredCopies)') == 1,
          f'{name} does not build the copies with the shared builder')
names_at = shared.index('static const char* const kFairyRecolouredLists[2] = {')
build_at = shared.index('static const char* Combo_BuildFairyRecolouredHands(')
builder = shared[names_at:build_at + len(block(shared, build_at))]
check(f'"{ADULT_NEAR[7:]}"' in builder and f'"{ADULT_FAR[7:]}"' in builder,
      'the shared builder does not copy the hands OoT draws for adult Link')

if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)


def cxx_table(name):
    return f'static Gfx* {name}[] = {{ ' + ', '.join(f'(Gfx*)"{path}"' for path in TABLES[name]) + ' };'


HARNESS = r'''
#define COMBO_BUILD 1
#include "ComboFairyOcarinaRecolour.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
typedef int s32;
typedef float f32;
struct Gfx { struct { uintptr_t w0, w1; } words; };
namespace Ship {
struct ResourceInitData { std::string Path; uint32_t Type = 0; bool IsCustom = false; };
class IResource {
  public:
    explicit IResource(std::shared_ptr<ResourceInitData> init) : mInit(std::move(init)) {}
    virtual ~IResource() = default;
    std::shared_ptr<ResourceInitData> GetInitData() { return mInit; }
    bool IsDirty() { return dirty; }
    bool dirty = false;
  private:
    std::shared_ptr<ResourceInitData> mInit;
};
class ResourceManager {
  public:
    std::map<std::string, std::shared_ptr<IResource>> files;
    int loads = 0;
    std::shared_ptr<IResource> LoadResourceProcess(const std::string& path) {
        ++loads;
        auto it = files.find(path);
        return it == files.end() ? nullptr : it->second;
    }
};
struct CrossRMRegistry {
    static inline std::shared_ptr<ResourceManager> oot;
    static inline std::vector<void (*)()> listeners;
    static std::shared_ptr<ResourceManager> Get(const std::string& game) { return game == "oot" ? oot : nullptr; }
    static void RegisterTeardownListener(void (*listener)()) { listeners.push_back(listener); }
};
} // namespace Ship
namespace Fast {
enum class ResourceType : uint32_t { DisplayList = 0x4F444C54, Texture = 0x4F544558 };
class DisplayList : public Ship::IResource {
  public:
    using IResource::IResource;
    int UCode = 0;
    std::vector<Gfx> Instructions;
};
class Texture : public Ship::IResource {
  public:
    using IResource::IResource;
    uint8_t* ImageData = nullptr;
};
} // namespace Fast
// libultraship's path hash (ship/utils/StrHash64.h: CRC-64/ECMA-182, all ones in, no final xor).
static uint64_t CRC64(const char* text) {
    uint64_t crc = ~0ull;
    for (; *text; ++text) {
        crc ^= uint64_t(uint8_t(*text)) << 56;
        for (int k = 0; k < 8; ++k)
            crc = (crc >> 63) ? (crc << 1) ^ 0x42F0E1EBA9EA3693ull : crc << 1;
    }
    return crc;
}
static int sWarnings = 0;
#define SPDLOG_WARN(...) (++sWarnings)

@BUILDER@

@RUNTIME@

// OoT's engine around the right hand limb.
enum { PLAYER_MODELTYPE_RH_OPEN = 8, PLAYER_MODELTYPE_RH_CLOSED, PLAYER_MODELTYPE_RH_SHIELD,
       PLAYER_MODELTYPE_RH_OCARINA = 13, PLAYER_MODELTYPE_RH_OOT };
enum { PLAYER_IA_OCARINA_FAIRY = 30, PLAYER_IA_OCARINA_OF_TIME };
enum { ITEM_NONE = 0xFF, ITEM_OCARINA_FAIRY = 7, ITEM_OCARINA_TIME = 8, ITEM_OCARINA_OF_TIME = ITEM_OCARINA_TIME };
enum { PLAYER_LIMB_R_HAND = 15 };
#define PLAYER_STATE1_IN_WATER (1 << 27)
struct Actor { f32 speedXZ; };
struct Player {
    Actor actor;
    s32 itemAction;
    unsigned char rightHandType, currentShield;
    unsigned stateFlags1;
    Gfx** rightHandDLists;
};
static struct { s32 linkAge; } gSaveContext;
static s32 sRightHandType, sDListsLodOffset;
static int sOcarinaSlot = ITEM_NONE;
#define INV_CONTENT(item) ((item) == ITEM_OCARINA_FAIRY || (item) == ITEM_OCARINA_TIME ? sOcarinaSlot : ITEM_NONE)
@TABLES@
// What OoT serves for a list path (ResourceMgr_LoadGfxByName): the manager's list, as cached.
static Gfx* ResourceMgr_LoadGfxByName(Gfx* path) {
    const char* name = reinterpret_cast<const char*>(path);
    assert(std::strncmp(name, "__OTR__", 7) == 0);
    auto it = Ship::CrossRMRegistry::oot->files.find(name + 7);
    assert(it != Ship::CrossRMRegistry::oot->files.end());
    return std::static_pointer_cast<Fast::DisplayList>(it->second)->Instructions.data();
}
static void func_8084B498(Player* player) @SLOT_ACTION@
// The production chain: action -> model group -> right hand -> lists by age (Player_SetModels).
struct Hand { s32 itemAction; unsigned char type; Gfx** table; };
static const Hand kHands[] = { @HANDS@ };
static void SetModels(Player* player) {
    for (const Hand& hand : kHands)
        if (hand.itemAction == player->itemAction) {
            player->rightHandType = hand.type;
            player->rightHandDLists = &hand.table[gSaveContext.linkAge];
            return;
        }
    assert(false);
}
static void RightHandLimb(Player* player, s32 limbIndex, Gfx** dList) {
    @RIGHT_HAND@
}
// Player_OverrideLimbDrawGameplayDefault's R_HAND limb at `lod` (0 near, 1 far).
static Gfx* Drawn(Player& player, s32 lod) {
    sDListsLodOffset = lod * 2;
    sRightHandType = player.rightHandType;
    Gfx* list = nullptr;
    RightHandLimb(&player, PLAYER_LIMB_R_HAND, &list);
    return list;
}

static uint8_t sImage[1024];
static const char kNear[] = "objects/object_link_boy/gLinkAdultRightHandHoldingOotNearDL";
static const char kFar[] = "objects/object_link_boy/gLinkAdultRightHandHoldingOotFarDL";
static const char kChildNear[] = "objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaNearDL";
static const char kChildFar[] = "objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaFarDL";
static const char kDesign[] = "objects/gameplay_keep/gOcarinaofTimeDesignTex";
static const char kFairy[] = "objects/object_link_child/gLinkChildFairyOcarinaTex";
static Gfx G(uintptr_t w0, uintptr_t w1) { return Gfx{ { w0, w1 } }; }
static void Hash(std::vector<Gfx>& l, uintptr_t w0, uint64_t hash) {
    l.push_back(G(w0, 0));
    l.push_back(G(uintptr_t(hash >> 32), uintptr_t(hash & 0xFFFFFFFFu)));
}
// An instrument material: its texture, a block load and a render tile through G_SETTILESIZE, and the combiner.
static void Instrument(std::vector<Gfx>& l, uint64_t texture, uintptr_t tile = 0x0003C03C) {
    Hash(l, 0x20100000, texture);
    for (const Gfx& g : { G(0xF5100000, 0x07080200), G(0xE6000000, 0), G(0xF3000000, 0x070FF200), G(0xE7000000, 0),
                          G(0xF5100800, 0x00080200), G(0xF2000000, tile), G(0xFC127E03, 0xFFFFFDF8) })
        l.push_back(g);
}
// A hand: the port's marker, the hand's texture and mesh, the instrument material `uses` times, its mesh, the end.
static std::vector<Gfx> HandList(uint64_t instrument, unsigned uses, unsigned seed, uintptr_t tile = 0x0003C03C) {
    std::vector<Gfx> l;
    Hash(l, 0x33000000, 0x40 + seed);
    Hash(l, 0x20500000, CRC64("objects/object_link_boy/gLinkAdultHandTex"));
    Hash(l, 0x32008010, 0x1000 + seed);
    l.push_back(G(0x06000204, seed));
    for (unsigned i = 0; i < uses; ++i)
        Instrument(l, instrument, tile);
    Hash(l, 0x32008010, 0x2000 + seed);
    l.push_back(G(0xDF000000, 0));
    return l;
}
static std::shared_ptr<Fast::DisplayList> MakeList(const std::string& path, std::vector<Gfx> instructions) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    init->Type = uint32_t(Fast::ResourceType::DisplayList);
    auto list = std::make_shared<Fast::DisplayList>(init);
    list->UCode = 2;
    list->Instructions = std::move(instructions);
    return list;
}
static std::shared_ptr<Fast::Texture> MakeTexture(const char* path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    init->Type = uint32_t(Fast::ResourceType::Texture);
    auto texture = std::make_shared<Fast::Texture>(init);
    texture->ImageData = sImage;
    return texture;
}
// OoT's manager: every list of the hand tables (the adult hands holding the Ocarina of Time and the child hands
// holding the Fairy Ocarina as OoT has them), both textures; and what each list held when it was made.
struct Oot {
    std::shared_ptr<Ship::ResourceManager> rm = std::make_shared<Ship::ResourceManager>();
    std::map<std::string, std::vector<Gfx>> before;
    std::shared_ptr<Fast::Texture> fairy = MakeTexture(kFairy);
    Oot() {
        unsigned seed = 1;
        for (const Hand& hand : kHands)
            for (int i = 0; i < 4; ++i)
                Add(reinterpret_cast<const char*>(hand.table[i]) + 7, seed++);
        for (int i = 0; i < 4; ++i)
            Add(reinterpret_cast<const char*>(sPlayerRightHandClosedDLs[i]) + 7, seed++);
        rm->files[kDesign] = MakeTexture(kDesign);
        rm->files[kFairy] = fairy;
    }
    void Add(const std::string& path, unsigned seed) {
        if (rm->files.count(path))
            return;
        Set(path, path == kNear || path == kFar           ? HandList(CRC64(kDesign), 1, seed)
                  : path == kChildNear || path == kChildFar ? HandList(CRC64(kFairy), 1, seed)
                                                            : HandList(CRC64("objects/other/tex"), 1, seed));
    }
    void Set(const std::string& path, std::vector<Gfx> instructions) {
        before[path] = instructions;
        rm->files[path] = MakeList(path, std::move(instructions));
    }
    std::shared_ptr<Fast::DisplayList> List(const std::string& path) const {
        return std::static_pointer_cast<Fast::DisplayList>(rm->files.at(path));
    }
    Gfx* Native(const std::string& path) const { return List(path)->Instructions.data(); }
    bool Untouched() const {
        for (const auto& [path, instructions] : before) {
            const auto& now = List(path)->Instructions;
            if (now.size() != instructions.size())
                return false;
            for (size_t k = 0; k < now.size(); ++k)
                if (now[k].words.w0 != instructions[k].words.w0 || now[k].words.w1 != instructions[k].words.w1)
                    return false;
        }
        return true;
    }
};
static void Teardown() {
    for (auto listener : Ship::CrossRMRegistry::listeners)
        listener();
}
// The recoloured copy for `lod`: OoT's adult list with only the texture hash swapped, OoT's ucode and init data.
static bool Recoloured(const Oot& oot, Gfx* drawn, s32 lod) {
    if (!sFairyRecolouredCopies[lod] || drawn != sFairyRecolouredCopies[lod]->Instructions.data())
        return false;
    auto& copy = *sFairyRecolouredCopies[lod];
    auto source = oot.List(lod ? kFar : kNear);
    assert(sFairyRecolouredSources[lod] == source && copy.UCode == 2 && copy.GetInitData() == source->GetInitData());
    std::vector<Gfx> expected = oot.before.at(lod ? kFar : kNear);
    int swapped = 0;
    for (size_t k = 0; k + 1 < expected.size(); ++k)
        if (expected[k].words.w0 == 0x20100000 &&
            (uint64_t(expected[k + 1].words.w0) << 32 | expected[k + 1].words.w1) == CRC64(kDesign)) {
            expected[k + 1].words.w0 = uintptr_t(CRC64(kFairy) >> 32);
            expected[k + 1].words.w1 = uintptr_t(CRC64(kFairy) & 0xFFFFFFFFu);
            ++swapped;
        }
    assert(swapped == 1 && copy.Instructions.size() == expected.size());
    for (size_t k = 0; k < expected.size(); ++k)
        assert(copy.Instructions[k].words.w0 == expected[k].words.w0 &&
               copy.Instructions[k].words.w1 == expected[k].words.w1);
    return true;
}
static bool Tried() { return !sFairyRecolouredOwner.expired() || !sFairyRecolouredFailed.expired(); }

// Each way OoT picks an ocarina gives the same item action: an ocarina button holding the item, or the ocarina slot.
static s32 ButtonAction(int item) { return item == ITEM_OCARINA_FAIRY ? @BUTTON_FAIRY@ : @BUTTON_OOT@; }
static s32 SlotAction(int slot) {
    Player player = {};
    sOcarinaSlot = slot;
    func_8084B498(&player);
    return player.itemAction;
}

// What OoT draws for age `age` (0 adult, 1 child) playing `ocarina` at `lod`, the action picked from the ocarina slot
// (a cutscene or forced ocarina action) or from the ocarina button.
static Gfx* Play(int age, int ocarina, s32 lod, bool fromSlot) {
    gSaveContext.linkAge = age;
    Player player = {};
    player.itemAction = fromSlot ? SlotAction(ocarina) : ButtonAction(ocarina);
    assert(player.itemAction == (ocarina == ITEM_OCARINA_FAIRY ? PLAYER_IA_OCARINA_FAIRY : PLAYER_IA_OCARINA_OF_TIME));
    SetModels(&player);
    return Drawn(player, lod);
}
static const char* Native(int age, int ocarina, s32 lod) {
    const Hand& hand = kHands[ocarina == ITEM_OCARINA_FAIRY ? 0 : 1];
    assert(hand.itemAction == (ocarina == ITEM_OCARINA_FAIRY ? PLAYER_IA_OCARINA_FAIRY : PLAYER_IA_OCARINA_OF_TIME));
    return reinterpret_cast<const char*>(hand.table[age + 2 * lod]) + 7;
}

static void Selection(bool switchOn) {
    Oot oot;
    Ship::CrossRMRegistry::oot = oot.rm;
    // Child Link and the Ocarina of Time first: OoT's own lists, and the copies are not even built.
    for (int fromSlot = 0; fromSlot < 2; ++fromSlot)
        for (s32 lod = 0; lod < 2; ++lod) {
            for (int ocarina : { ITEM_OCARINA_FAIRY, ITEM_OCARINA_TIME })
                assert(Play(1, ocarina, lod, fromSlot) == oot.Native(Native(1, ocarina, lod)));
            assert(Play(0, ITEM_OCARINA_TIME, lod, fromSlot) == oot.Native(lod ? kFar : kNear));
        }
    assert(!Tried() && oot.rm->loads == 0);
    // Adult Link with the Fairy Ocarina: the recoloured copy for the LOD drawn, built once.
    for (int fromSlot = 0; fromSlot < 2; ++fromSlot)
        for (s32 lod = 0; lod < 2; ++lod) {
            Gfx* drawn = Play(0, ITEM_OCARINA_FAIRY, lod, fromSlot);
            assert(switchOn ? Recoloured(oot, drawn, lod) && drawn != oot.Native(lod ? kFar : kNear)
                            : drawn == oot.Native(lod ? kFar : kNear));
            for (int ocarina : { ITEM_OCARINA_FAIRY, ITEM_OCARINA_TIME })
                assert(Play(1, ocarina, lod, fromSlot) == oot.Native(Native(1, ocarina, lod)));
            assert(Play(0, ITEM_OCARINA_TIME, lod, fromSlot) == oot.Native(lod ? kFar : kNear));
        }
    assert(sWarnings == 0 && oot.Untouched());
    if (!switchOn) {
        assert(!Tried() && oot.rm->loads == 0 && Ship::CrossRMRegistry::listeners.empty());
        std::puts("Switch off: every hand is OoT's own and nothing is built");
        Ship::CrossRMRegistry::oot.reset();
        return;
    }
    const int loads = oot.rm->loads;
    assert(loads == 5); // the Fairy Ocarina texture, two adult lists, two child lists
    // A list OoT serves for that hand other than the one copied (a texture pack's) is drawn as served.
    oot.Set(kNear, HandList(CRC64(kDesign), 1, 99));
    assert(Play(0, ITEM_OCARINA_FAIRY, 0, false) == oot.Native(kNear) && Recoloured(oot, Play(0, ITEM_OCARINA_FAIRY,
                                                                                               1, false), 1));
    assert(oot.rm->loads == loads);
    // Nothing else reaches the copies: no manager, no list or a LOD that is not drawn.
    Gfx* farNative = oot.Native(kFar);
    assert(!Combo_OotFairyOcarinaHand(nullptr, 0) && Combo_OotFairyOcarinaHand(farNative, -1) == farNative &&
           Combo_OotFairyOcarinaHand(farNative, 2) == farNative);
    // Teardown drops the copies; the next draw builds them again from what OoT serves now.
    std::weak_ptr<Fast::DisplayList> weakCopy = sFairyRecolouredCopies[0];
    Teardown();
    assert(weakCopy.expired() && !Tried());
    assert(Recoloured(oot, Play(0, ITEM_OCARINA_FAIRY, 0, true), 0) && oot.rm->loads == 2 * loads);
    assert(Ship::CrossRMRegistry::listeners.size() == 1 && oot.Untouched() && sWarnings == 0);
    Teardown();
    Ship::CrossRMRegistry::oot.reset();
    assert(!Combo_OotFairyOcarinaHand(nullptr, 0) && Combo_OotFairyOcarinaHand(farNative, 1) == farNative && !Tried());
    std::puts("Adult with the Fairy Ocarina: recoloured copy per LOD, built once per manager; adult with the Ocarina "
              "of Time: OoT's blue hand; child: his own hands; a texture pack's hand as served; dropped at teardown");
}

// Where the copies cannot be built, adult Link with the Fairy Ocarina keeps OoT's own blue hand.
template <typename Break> static void Refused(const char* what, Break breakIt) {
    Oot oot;
    breakIt(oot);
    Ship::CrossRMRegistry::oot = oot.rm;
    const int warnings = sWarnings;
    for (s32 lod = 0; lod < 2; ++lod)
        assert(Play(0, ITEM_OCARINA_FAIRY, lod, lod == 1) == oot.Native(lod ? kFar : kNear));
    assert(sWarnings == warnings + 1 && !sFairyRecolouredFailed.expired() && sFairyRecolouredOwner.expired());
    const int loads = oot.rm->loads;
    for (s32 lod = 0; lod < 2; ++lod) {
        assert(Play(0, ITEM_OCARINA_FAIRY, lod, false) == oot.Native(lod ? kFar : kNear));
        assert(Play(1, ITEM_OCARINA_FAIRY, lod, true) == oot.Native(Native(1, ITEM_OCARINA_FAIRY, lod)));
    }
    assert(oot.rm->loads == loads && sWarnings == warnings + 1 && oot.Untouched()); // not retried, logged once
    // Another manager is tried afresh.
    Oot other;
    Ship::CrossRMRegistry::oot = other.rm;
    assert(Recoloured(other, Play(0, ITEM_OCARINA_FAIRY, 0, false), 0));
    Teardown();
    Ship::CrossRMRegistry::oot.reset();
    std::printf("Refused, OoT's own hand, once per manager: %s\n", what);
}

int main() {
    const bool switchOn = @SWITCH@;
    Selection(switchOn);
    if (switchOn) {
        Refused("a custom (mod) near list", [](Oot& o) { o.List(kNear)->GetInitData()->IsCustom = true; });
        Refused("a missing Fairy Ocarina texture", [](Oot& o) { o.rm->files.erase(kFairy); });
        Refused("a far child list that tiles the Fairy Ocarina texture otherwise",
                [](Oot& o) { o.Set(kChildFar, HandList(CRC64(kFairy), 1, 4, 0x0007C07C)); });
        Refused("a far list that sets the Ocarina of Time texture twice",
                [](Oot& o) { o.Set(kFar, HandList(CRC64(kDesign), 2, 2)); });
    }
    std::printf("PASS release switch %s\n", switchOn ? "on" : "off");
}
'''

print(f'Release switches as committed: OoT {switch}, MM {mm_switch}', flush=True)
hands_cxx = ', '.join(f'{{ {ia}, {hands[ia]}, {tables[ia]} }}' for ia in IAS)
button = {item: ia for ia, item in IAS.items()}
for label, value in (('committed', switch), ('flipped', 'false' if switch == 'true' else 'true')):
    text = re.sub(r'(static constexpr bool kAdultFairyRecolouredOot = )(true|false);', r'\g<1>' + value + ';', runtime)
    code = (HARNESS.replace('@BUILDER@', builder).replace('@RUNTIME@', text)
            .replace('@TABLES@', '\n'.join(cxx_table(name) for name in TABLES))
            .replace('@SLOT_ACTION@', this_to_player(slot_action[slot_action.index('{'):]))
            .replace('@HANDS@', hands_cxx).replace('@RIGHT_HAND@', this_to_player(right_hand))
            .replace('@BUTTON_FAIRY@', button['ITEM_OCARINA_FAIRY'])
            .replace('@BUTTON_OOT@', button['ITEM_OCARINA_OF_TIME'])
            .replace('@SWITCH@', value))
    source = work / f'oot_fairy_recolour_{label}.cpp'
    source.write_text(code, encoding='utf-8')
    exe = hostcxx.build(a.compiler, source, std='c++17', includes=[root / 'combo/menu'], warnings=False)
    subprocess.run([str(exe)], cwd=work, check=True, timeout=hostcxx.RUN_TIMEOUT)
print('PASS')
