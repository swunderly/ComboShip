"""MM pause name bar: every OoT item or song MM can hold is named in English.

Kaleido_LoadItemNameStatic and the combo name sources it calls are extracted verbatim from production and run on
host stubs for the cross-game resource registry, for every item id. Every OoT name they return must be an English
item_name_static texture that every supported OoT ROM provides (soh/assets/xml/*), aligned for the interpreter's
path check. A name that OoT cannot provide falls back to MM's own, never to nothing. The only Japanese names left
are MM's own leftover entries.
"""
from pathlib import Path
import argparse
import re
import subprocess
import xml.etree.ElementTree as ElementTree
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def read(rel):
    return (source/rel).read_text(encoding='utf-8')


def function(text, signature):
    start = hostcxx.find(text, signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


def statement(text, needle):
    """The whole declaration containing needle, from the end of the previous one to its semicolon."""
    at = hostcxx.find(text, needle)
    start = max(text.rfind(c, 0, at) for c in ';{}') + 1
    return text[start:text.index(';', at) + 1].strip() + '\n'


nes = read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope_NES.c')
bridge = read('mm/2s2h/BenPort.cpp')
gear = read('mm/2s2h/ComboAdultGearIconResources.h')
mask = read('mm/2s2h/ComboAdultMaskIconResources.h')
farore = read('mm/2s2h/Rando/FaroresWind.cpp')
spells = read('mm/2s2h/Rando/Spells.cpp')
native = read('mm/2s2h/Rando/NativeSpellResources.h')
item_enum = read('mm/include/z64item.h').split('typedef enum ItemId {', 1)[1].split('} ItemId;', 1)[0]

items = {}
for value, name in re.findall(r'/\*\s*0x([0-9A-Fa-f]+)\s*\*/\s*(ITEM_\w+)', item_enum):
    items.setdefault(name, int(value, 16))
for name, value in re.findall(r'\b(ITEM_COMBO_\w+)\s*=\s*0x([0-9A-Fa-f]+)', item_enum):
    items[name] = int(value, 16)

# OoT's English item names, as every supported OoT ROM version extracts them.
versions = sorted((source/'soh/assets/xml').glob('*/textures/item_name_static.xml'))
assert versions, 'no OoT item_name_static.xml found'
oot_names = None
for path in versions:
    names = {t.get('Name') for t in ElementTree.parse(path).iter('Texture')
             if t.get('Format') == 'ia4' and t.get('Width') == '128' and t.get('Height') == '16'}
    oot_names = names if oot_names is None else oot_names & names

load_name = function(nes, 'void Kaleido_LoadItemNameStatic(void** segment, u32 texIndex) {')
# C converts the table's const pointers to void* implicitly; C++ needs the cast spelled out.
load_name = re.sub(r'\*segment = (?!\(void\*\))', '*segment = (void*)', load_name)
textures = sorted(set(re.findall(r'\bg\w+Tex\b', load_name)))

code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <set>
#include <string>
using u32 = uint32_t;
#define COMBO_BUILD
#define ALIGN_ASSET(x) alignas(x)
#define ARRAY_COUNTU(a) (sizeof(a) / sizeof((a)[0]))
static const char gEmptyTexture[] = "gEmptyTexture";
namespace Ship {
struct IResource { std::string path; };
struct ResourceManager {
    std::set<std::string> files;
    std::shared_ptr<IResource> LoadResourceProcess(const std::string& path) {
        return files.count(path) ? std::make_shared<IResource>(IResource{ path }) : nullptr;
    }
};
std::shared_ptr<ResourceManager> sOot;
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game) { return game == "oot" ? sOot : nullptr; }
};
} // namespace Ship
// NativeSpellResources.h's item_name_static rule (IA4 128x16, or an alt replacement) is checked on its text.
namespace ComboNativeSpells {
bool Valid(const std::shared_ptr<Ship::IResource>& r, const char* path, const std::shared_ptr<Ship::IResource>& = {}) {
    return r && r->path == path;
}
} // namespace ComboNativeSpells
bool gearReady = true, maskReady = true, faroreReady = true, spellsReady = true;
'''
code += ''.join(f'constexpr int {name} = 0x{value:X};\n' for name, value in sorted(items.items()))
code += ''.join(f'static const char {name}[] = "{name}";\n' for name in textures)
code += 'namespace ComboAdultMM {\n'
for i in range(4):
    code += statement(gear, f'sGearNamePaths{i}[] =')
code += statement(gear, 'static const char* const sGearNamePaths[] =')
for i in range(2):
    code += statement(mask, f'sAdultMaskIconPaths{i}[] =')
code += statement(mask, 'static const char* const sAdultMaskIconPaths[] =')
code += 'bool LoadGearIcons() { return gearReady; }\nbool LoadAdultMaskIcons() { return maskReady; }\n'
code += '} // namespace ComboAdultMM\n'
code += statement(farore, 'char sComboFaroreItemName[] =')
for name in ('sComboLoveItemName', 'sComboFireItemName', 'sComboEmptyItemName'):
    code += statement(spells, f'char {name}[] =')
code += 'int Combo_MMFarorePrepareResources() { return faroreReady; }\n'
code += 'int Combo_MMSpellPrepareResources(int) { return spellsReady; }\n'
code += function(gear, 'extern "C" const char* Combo_MM_AdultGearItemName(int item) {')
code += function(mask, 'extern "C" const char* Combo_MM_AdultMaskItemName(void) {')
code += function(farore, 'extern "C" const char* Combo_MMFaroreItemName() {')
code += function(spells, 'extern "C" const char* Combo_MMSpellItemName(int item) {')
code += function(bridge, 'extern "C" const char* Combo_MM_OotItemName(int item) {')
code += load_name
code += 'static const char* const kOotNames[] = {\n'
code += ''.join(f'    "textures/item_name_static/{name}",\n' for name in sorted(oot_names))
code += r'''};
const char* Name(u32 id) {
    void* segment = nullptr;
    Kaleido_LoadItemNameStatic(&segment, id);
    return (const char*)segment;
}
int main() {
    Ship::sOot = std::make_shared<Ship::ResourceManager>();
    for (const char* name : kOotNames)
        Ship::sOot->files.insert(name);
    for (u32 id = 0; id < 256; ++id) {
        const char* name = Name(id);
        std::printf("%02X %s\n", id, name ? name : "(null)");
    }
    // OoT's English names cannot load: MM's own entries stay, never nothing or an invented name.
    for (int missingRegistry = 0; missingRegistry < 2; ++missingRegistry) {
        if (missingRegistry)
            Ship::sOot.reset();
        else
            Ship::sOot->files.clear();
        assert(!std::strcmp(Name(ITEM_OCARINA_FAIRY), "gItemNameFairyOcarinaJPNTex"));
        assert(!std::strcmp(Name(ITEM_SONG_SARIA), "gItemNameNocturneOfShadowJPNTex"));
        assert(!std::strcmp(Name(ITEM_SONG_SUN), "gItemNameSunsSongJPNTex"));
    }
    return 0;
}
'''
test = work/'mm-pause-item-names.cpp'
test.write_text(code, encoding='utf-8')
output = subprocess.run([str(hostcxx.build(args.compiler, test))], check=True, capture_output=True, text=True,
                        timeout=hostcxx.RUN_TIMEOUT).stdout
names = {int(line[:2], 16): line[3:] for line in output.splitlines()}
assert len(names) == 256, output

failures = []
OOT_PREFIX = '__OTR__@oot:textures/item_name_static/'
sources = (bridge, gear, mask, farore, spells)
for item, name in sorted(names.items()):
    if not name.startswith(OOT_PREFIX):
        continue
    leaf = name[len(OOT_PREFIX):]
    if not leaf.endswith('ENGTex'):
        failures.append(f'item 0x{item:02X}: OoT name {leaf} is not English')
    if leaf not in oot_names:
        failures.append(f'item 0x{item:02X}: {leaf} is not a 128x16 IA4 name in every OoT ROM version')
    # The interpreter treats an odd pointer as a segmented address, so a path must be 2-byte aligned.
    declarations = [statement(text, f'"{name}"') for text in sources if f'"{name}"' in text]
    if not declarations or not all('alignas(2)' in d or 'ALIGN_ASSET(2)' in d for d in declarations):
        failures.append(f'item 0x{item:02X}: {name} is not declared 2-byte aligned')

# Every OoT item the combo puts in MM's inventory is named in English.
cross_game = ['ITEM_OCARINA_FAIRY', 'ITEM_COMBO_FARORES_WIND', 'ITEM_COMBO_NAYRUS_LOVE', 'ITEM_COMBO_DINS_FIRE',
              'ITEM_COMBO_BOOTS_IRON', 'ITEM_COMBO_BOOTS_HOVER', 'ITEM_COMBO_TUNIC_GORON', 'ITEM_COMBO_TUNIC_ZORA']
# OoT songs 2Ship's randomizer can put on MM's Quest page (Saria's Song, Sun's Song shuffles).
oot_songs = ['ITEM_SONG_SARIA', 'ITEM_SONG_SUN']
for item in cross_game + oot_songs:
    if not names[items[item]].startswith(OOT_PREFIX):
        failures.append(f'{item}: named {names[items[item]]}, not by OoT\'s English name')
# Adult Link's mask (0xBC) uses the combo's own name texture.
if names[0xBC] != '__OTR__textures/combo_adult_mask_names/gComboAdultMaskNameTex':
    failures.append(f'item 0xBC: named {names[0xBC]}')

# MM's own leftover entries: no MM item, quest slot or combo item is named by them.
leftovers = {'ITEM_SLINGSHOT', 'ITEM_LONGSHOT', 'ITEM_HYLIAN_LOACH', 'ITEM_OBABA_DRINK', 'ITEM_TINGLE_MAP',
             'ITEM_BOW_FIRE', 'ITEM_BOW_ICE', 'ITEM_BOW_LIGHT', 'ITEM_SWORD_DEITY', 'ITEM_WALLET_DEFAULT',
             'ITEM_WALLET_ADULT', 'ITEM_WALLET_GIANT', 'ITEM_FISHING_ROD', 'ITEM_SKULL_TOKEN', 'ITEM_HEART_PIECE',
             'ITEM_71', 'ITEM_72'}
japanese = {item for item, name in names.items() if 'JPN' in name}
expected = {items[name] for name in leftovers}
if japanese != expected:
    failures.append('Japanese names: unexpected ' + str(sorted(hex(i) for i in japanese - expected)) +
                    ', missing ' + str(sorted(hex(i) for i in expected - japanese)))

# The validity rule the stub stands for.
if not (hostcxx.contains(native, 'if (name.starts_with("textures/item_name_static/"))') and
        hostcxx.contains(native, 'return Texture(r, Fast::TextureType::GrayscaleAlpha4bpp, 128, 16, 1024);')):
    failures.append('NativeSpellResources.h no longer checks item_name_static textures as IA4 128x16')
# Both name panels (items and owl warp) name the cursor item through this function.
if len(re.findall(r'Kaleido_LoadItemNameStatic\(&pauseCtx->nameSegment,', nes)) != 2:
    failures.append('z_kaleido_scope_NES.c: expected the two name panels to call Kaleido_LoadItemNameStatic')

if failures:
    raise SystemExit('\n'.join(failures))
print(f'MM pause names: {len(names)} item ids, {len(cross_game) + 1} cross-game items and {len(oot_songs)} OoT songs '
      f'in English, {len(japanese)} MM leftovers, checked against {len(versions)} OoT ROM versions.')
