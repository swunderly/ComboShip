"""Compile production mask ownership, initialization, pool and menu behavior.

Only unrelated engine services are bound by the harness. The tested conditions
and selectors are extracted unchanged from the candidate's production source.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx


def braced(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    tokens = re.finditer(
        r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]',
        text[opening:], re.S)
    for token in tokens:
        if token.group() == '{':
            depth += 1
        elif token.group() == '}':
            depth -= 1
            if depth == 0:
                return text[start:opening + token.end()]
    raise AssertionError(f'Unterminated production block: {signature}')


def read(root, relative):
    return (root / relative).read_text(encoding='utf-8')


parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--json-include', type=Path, required=True)
args = parser.parse_args()
root, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)

logic = read(root, 'soh/soh/Enhancements/randomizer/logic.cpp')
has_item = braced(logic, 'bool Logic::HasItem(')
start = has_item.index('        case RG_SKULL_MASK:')
following = re.search(r'^\s*case RG_(?:QUIVER_INF|POWER_BRACELET):',
                      has_item[start:], re.M)
assert following, 'Mask branches must precede the next non-mask item case'
mask_cases = has_item[start:start + following.start()]
assert 'case RG_MASK_OF_TRUTH:' in mask_cases

savefile = read(root, 'soh/soh/Enhancements/randomizer/savefile.cpp')
completed = braced(savefile,
    'if (Randomizer_GetSettingValue(RSK_MASK_QUEST) == RO_MASK_QUEST_COMPLETED)')
repair = braced(savefile, 'extern "C" void Randomizer_RepairLegacyMaskInventory()')
save_manager = read(root, 'soh/soh/SaveManager.cpp')
assert save_manager.index('Randomizer_RepairLegacyMaskInventory();') < save_manager.index('        InitMeta(fileNum);')
assert 'Randomizer_RepairLegacyMaskInventory();' in braced(savefile, 'extern "C" void Randomizer_InitSaveFile()')
trade = read(root, 'soh/soh/Enhancements/randomizer/ShuffleTradeItems.c')
selectors = '\n'.join(braced(trade, f'u8 Randomizer_Get{direction}ChildTradeItem()')
                      for direction in ('Next', 'Prev'))
pool_source = read(root, 'soh/soh/Enhancements/randomizer/3drando/item_pool.cpp')
mask_pool = braced(pool_source,
    'if (ctx->GetOption(RSK_MASK_QUEST).Is(RO_MASK_QUEST_SHUFFLE))')
native = read(root, 'soh/soh/OTRGlobals.cpp')
profile = braced(native, 'static int SOH_ApplyProfileRandoSettingsImpl(')
restore = braced(native, 'extern "C" COMBO_EXPORT void SOH_RestoreRandoSettings(')
normalizer = 'NormalizeOotMaskQuestForGeneration'
assert normalizer in profile, 'Profile generation must normalize the unsupported Vanilla request'
dump = braced(native, 'extern "C" COMBO_EXPORT const char* SOH_DumpRandoStaticData(')
assert normalizer not in dump, 'Static data inspection must preserve saved seed modes'
for file in ('combo/ComboShip.cpp', 'combo/ComboRandoHeadless.cpp'):
    assert normalizer in read(root, file), 'Each new-generation caller must normalize before fill'
for signature in ('extern "C" COMBO_EXPORT void SOH_RestoreRandoSettings(',
                  'extern "C" COMBO_EXPORT void SOH_PrepRandoContext(',
                  'static void EnsureOracleInit() {',
                  'extern "C" COMBO_EXPORT void SOH_ApplyRandoPlacements(',
                  'extern "C" COMBO_EXPORT void Combo_SOH_Rando_Reset('):
    assert normalizer not in braced(native, signature), \
        f'Saved-seed restoration must preserve the original mode: {signature}'

market = read(root, 'soh/soh/Enhancements/randomizer/location_access/overworld/market.cpp')
shop = market[market.index('    areaTable[RR_MARKET_MASK_SHOP] = Region('):]
shop = shop[:re.search(r'^\s*\}\);', shop, re.M).end()]
starting = read(root, 'soh/soh/Enhancements/randomizer/3drando/starting_inventory.cpp')
starting_function = braced(starting, 'void GenerateStartingInventory()')
assert 'OotMaskQuestHasStartingMasks' in starting_function, \
    'The oracle must receive the same Completed masks as runtime save initialization'
starting_guard = starting_function.index('#ifdef COMBO_BUILD')
starting_masks = starting_function[starting_guard:starting_function.index('#endif', starting_guard) + len('#endif')]
assert 'OotMaskQuestHasStartingMasks' in starting_masks
starting_bunny = re.search(r'^\s*AddItemToInventory\(RG_BUNNY_HOOD,.*?;', starting_function, re.M).group()
starting_add = braced(starting, 'static void AddItemToInventory(')
settings_source = read(root, 'soh/soh/Enhancements/randomizer/settings.cpp')
mask_options = re.findall(r'^\s*OPT_U8\(RSK_MASK_QUEST,.*?;', settings_source, re.M)
assert len(mask_options) == 2, 'Keep separate Combo and standalone mask defaults'
combo_default, standalone_default = (
    re.search(r',\s*(\w+)\);$', option.strip()).group(1) for option in mask_options)

json_include = args.json_include.resolve()
assert (json_include/'nlohmann/json.hpp').exists(), 'Supply --json-include with the production nlohmann header directory'

prelude = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "rando/OotMaskQuest.h"
#include <nlohmann/json.hpp>
using u8=uint8_t;
using RandomizerGet=int;
enum { RO_MASK_QUEST_VANILLA, RO_MASK_QUEST_COMPLETED, RO_MASK_QUEST_SHUFFLE };
enum { RSK_MASK_QUEST, RSK_STARTING_BUNNY_HOOD };
enum { ITEM_WEIRD_EGG, ITEM_CHICKEN, ITEM_LETTER_ZELDA, ITEM_MASK_KEATON,
       ITEM_MASK_SKULL, ITEM_MASK_SPOOKY, ITEM_MASK_BUNNY, ITEM_MASK_GORON,
       ITEM_MASK_ZORA, ITEM_MASK_GERUDO, ITEM_MASK_TRUTH, ITEM_TRADE_CHILD,
       ITEM_NONE=255 };
enum { RAND_INF_CHILD_TRADES_HAS_WEIRD_EGG, RAND_INF_CHILD_TRADES_HAS_CHICKEN,
       RAND_INF_CHILD_TRADES_HAS_ZELDAS_LETTER, RAND_INF_CHILD_TRADES_HAS_MASK_KEATON,
       RAND_INF_CHILD_TRADES_HAS_MASK_SKULL, RAND_INF_CHILD_TRADES_HAS_MASK_SPOOKY,
       RAND_INF_CHILD_TRADES_HAS_MASK_BUNNY, RAND_INF_CHILD_TRADES_HAS_MASK_GORON,
       RAND_INF_CHILD_TRADES_HAS_MASK_ZORA, RAND_INF_CHILD_TRADES_HAS_MASK_GERUDO,
       RAND_INF_CHILD_TRADES_HAS_MASK_TRUTH, FLAG_COUNT };
enum { RG_KEATON_MASK, RG_SKULL_MASK, RG_SPOOKY_MASK, RG_BUNNY_HOOD,
       RG_GORON_MASK, RG_ZORA_MASK, RG_GERUDO_MASK, RG_MASK_OF_TRUTH, RG_SPEAK_HYLIAN };
enum { LOGIC_BORROW_SKULL_MASK, LOGIC_BORROW_RIGHT_MASKS, LOGIC_KAKARIKO_GATE_OPEN,
       LOGIC_CAN_BORROW_MASKS, LOGIC_BORROW_SPOOKY_MASK, LOGIC_BORROW_BUNNY_HOOD };
enum { INFTABLE_GATE_GUARD_PUT_ON_KEATON_MASK, EVENTCHKINF_PAID_BACK_BUNNY_HOOD_FEE };
struct Option {
    int value=0;
    int Get() const {return value;}
    bool Is(int other) const {return value==other;}
    explicit operator bool() const {return value!=0;}
};
struct Context { Option options[2]; Option& GetOption(int key) {return options[key];} };
Context maskContext;
Context* ctx=&maskContext;
bool flags[FLAG_COUNT]{}, events[3]{};
bool maskPaid=false, guardMask=false;
bool randoLoaded=true;
using RandomizerInf=int;
#define IS_RANDO randoLoaded
u8 tradeSlot=ITEM_LETTER_ZELDA;
struct {uint16_t itemGetInf[4]{};} gSaveContext;
int Randomizer_GetSettingValue(int key) {return ctx->GetOption(key).Get();}
bool Flags_GetRandomizerInf(int flag) {return flags[flag];}
void Flags_SetRandomizerInf(int flag) {flags[flag]=true;}
void Flags_SetInfTable(int) {guardMask=true;}
void Flags_SetEventChkInf(int) {maskPaid=true;}
bool CheckRandoInf(int flag) {return flags[flag];}
bool Get(int event) {return events[event];}
#define INV_CONTENT(item) tradeSlot
std::vector<int> pool;
void AddItemToPool(int item,int,int,int,int) {pool.push_back(item);}
void Reset() {
    std::fill(flags,flags+FLAG_COUNT,false);
    std::fill(events,events+3,false);
    maskContext=Context{}; gSaveContext={};
    maskPaid=guardMask=false; randoLoaded=true; tradeSlot=ITEM_LETTER_ZELDA; pool.clear();
}
enum { SCENE_HAPPY_MASK_SHOP, RR_MARKET_MASK_SHOP };
struct ShopLogic {
    bool HasItem(int) const {return true;}
    bool Get(int) const {return true;}
    bool CanRead() const {return true;}
};
ShopLogic shopLogic;
ShopLogic* logic=&shopLogic;
struct EventEntry {bool condition;};
struct CheckEntry {bool condition;};
struct ExitEntry {bool condition;};
struct Region {
    size_t eventCount=0,checkCount=0,exitCount=0;
    Region()=default;
    Region(const char*,int,std::initializer_list<EventEntry> regionEvents,
           std::initializer_list<CheckEntry> checks,std::initializer_list<ExitEntry> exits)
        :eventCount(regionEvents.size()),checkCount(checks.size()),exitCount(exits.size()) {}
};
Region areaTable[2];
#define EVENT_ACCESS(id,condition) EventEntry{condition}
#define LOCATION(id,condition) CheckEntry{condition}
#define ENTRANCE(id,condition) ExitEntry{condition}
#define COMBO_EXPORT
#define CVAR_RANDOMIZER_SETTING(suffix) "gRandoSettings." suffix
std::map<std::string,int> cvars;
std::map<std::string,std::string> stringCvars;
int writes=0;
int CVarGetInteger(const char* key,int fallback) {
    auto it=cvars.find(key);return it==cvars.end()?fallback:it->second;
}
void CVarSetInteger(const char* key,int value) {cvars[key]=value;++writes;}
void CVarSetString(const char* key,const char* value) {stringCvars[key]=value;++writes;}
namespace Rando {
struct ProfileOption {
    std::string key;int count,defaultValue;
    const std::string& GetCVarName() const {return key;}
    int GetMenuOptionDefault() const {return defaultValue;}
    uint64_t GetOptionCount() const {return static_cast<uint64_t>(count);}
    std::vector<std::string> GetDeclaredOptions() const {return std::vector<std::string>(static_cast<size_t>(count));}
};
struct Settings {
    std::vector<ProfileOption> options={{"gRandoSettings.CompleteMaskQuest",3,COMBO_MENU_DEFAULT},
                                      {"gRandoSettings.UnrelatedOption",2,0}};
    static Settings* GetInstance() {static Settings settings;return &settings;}
    const std::vector<ProfileOption>& GetAllOptions() const {return options;}
    const ProfileOption& GetOption(int key) const {return options.at(static_cast<size_t>(key));}
};
}
'''

prelude = prelude.replace('COMBO_MENU_DEFAULT', combo_default)
code = prelude + '\n#define COMBO_BUILD 1\nbool ComboHasMask(int itemName) {switch(itemName) {\n' + mask_cases + r'''
    default:return false;
}}
#undef COMBO_BUILD
bool StandaloneHasMask(int itemName) {switch(itemName) {
''' + mask_cases + r'''
    default:return false;
}}
void InitializeCompletedMasks() {
''' + completed + '\n}\n' + selectors + '\nvoid BuildMaskPool() {\n' + mask_pool + '\n}\n'
code += '\nstd::vector<RandomizerGet> StartingInventory;\n' + starting_add
code += '\n#define COMBO_BUILD 1\n' + repair + '\n#undef COMBO_BUILD\n'
code += '\n#define COMBO_BUILD 1\nvoid BuildComboMaskStarts() {StartingInventory.clear();\n' + starting_masks + '\n' + starting_bunny + r'''
}
#undef COMBO_BUILD
void BuildStandaloneMaskStarts() {StartingInventory.clear();
''' + starting_masks + '\n' + starting_bunny + '\n}\n'
code += '\n#define COMBO_BUILD 1\nvoid BuildComboShop() {\n' + shop + r'''
}
#undef COMBO_BUILD
void BuildStandaloneShop() {
''' + shop + '\n}\n#define COMBO_BUILD 1\n' + profile + '\n' + restore + '\n'

code += r'''
int main() {
    int cases=0;
''' + f'    static_assert({combo_default}==RO_MASK_QUEST_SHUFFLE);\n    static_assert({standalone_default}==RO_MASK_QUEST_VANILLA);\n' + r'''
    for(int mode:{RO_MASK_QUEST_VANILLA,RO_MASK_QUEST_COMPLETED,RO_MASK_QUEST_SHUFFLE}) {
        for(int owned=0;owned<4;++owned) for(int virtualEvents=0;virtualEvents<8;++virtualEvents) {
            Reset(); maskContext.options[RSK_MASK_QUEST].value=mode;
            flags[RAND_INF_CHILD_TRADES_HAS_MASK_SKULL]=(owned&1)!=0;
            flags[RAND_INF_CHILD_TRADES_HAS_MASK_TRUTH]=(owned&2)!=0;
            for(int bit=0;bit<3;++bit)events[bit]=(virtualEvents&(1<<bit))!=0;
            // Borrowing the shop, selling the entire chain, and opening the
            // Kakariko gate cannot create a mask in the cross-game runtime.
            assert(ComboHasMask(RG_SKULL_MASK)==((owned&1)!=0));
            assert(ComboHasMask(RG_MASK_OF_TRUTH)==((owned&2)!=0));
            // Preserve standalone Shipwright's three original modes.
            assert(StandaloneHasMask(RG_SKULL_MASK)==
                (mode==0?events[0]:mode==1?events[2]:flags[RAND_INF_CHILD_TRADES_HAS_MASK_SKULL]));
            assert(StandaloneHasMask(RG_MASK_OF_TRUTH)==
                (mode==0?events[1]:mode==1?events[2]:flags[RAND_INF_CHILD_TRADES_HAS_MASK_TRUTH]));
            ++cases;
        }
        Reset(); maskContext.options[RSK_MASK_QUEST].value=mode;
        InitializeCompletedMasks();
        assert(maskPaid==(mode==RO_MASK_QUEST_COMPLETED));
        assert(guardMask==(mode==RO_MASK_QUEST_COMPLETED));
        for(int flag=RAND_INF_CHILD_TRADES_HAS_MASK_KEATON;flag<=RAND_INF_CHILD_TRADES_HAS_MASK_TRUTH;++flag)
            assert(flags[flag]==(mode==RO_MASK_QUEST_COMPLETED));
        assert(ComboHasMask(RG_SKULL_MASK)==(mode==RO_MASK_QUEST_COMPLETED));
        assert(ComboHasMask(RG_MASK_OF_TRUTH)==(mode==RO_MASK_QUEST_COMPLETED));
        if(mode==RO_MASK_QUEST_COMPLETED) {
            // The real pause-menu selectors expose every starting mask even
            // without reaching the Happy Mask Shop or opening Kakariko.
            std::set<int> seen;
            for(int i=0;i<8;++i) {tradeSlot=Randomizer_GetNextChildTradeItem();seen.insert(tradeSlot);}
            assert(seen.size()==8 && *seen.begin()==ITEM_MASK_KEATON && *seen.rbegin()==ITEM_MASK_TRUTH);
            seen.clear();
            for(int i=0;i<8;++i) {tradeSlot=Randomizer_GetPrevChildTradeItem();seen.insert(tradeSlot);}
            assert(seen.size()==8 && seen.count(ITEM_MASK_TRUTH));
        } else {
            assert(Randomizer_GetNextChildTradeItem()==ITEM_NONE);
            assert(Randomizer_GetPrevChildTradeItem()==ITEM_NONE);
        }
        for(int mask:{RG_SKULL_MASK,RG_MASK_OF_TRUTH}) {
            Reset(); maskContext.options[RSK_MASK_QUEST].value=mode;
            const int flag=mask==RG_SKULL_MASK?RAND_INF_CHILD_TRADES_HAS_MASK_SKULL:RAND_INF_CHILD_TRADES_HAS_MASK_TRUTH;
            const int item=mask==RG_SKULL_MASK?ITEM_MASK_SKULL:ITEM_MASK_TRUTH;
            flags[flag]=true;
            assert(ComboHasMask(mask));
            assert(Randomizer_GetNextChildTradeItem()==item);
            assert(Randomizer_GetPrevChildTradeItem()==item);
            tradeSlot=static_cast<u8>(item);
            assert(Randomizer_GetNextChildTradeItem()==item && Randomizer_GetPrevChildTradeItem()==item);
        }
        // An old seed is repaired using owned starting items, never a borrowed
        // mask or edited placement. Existing progress bits and the selected
        // inventory slot survive; applying the repair twice is harmless.
        Reset(); maskContext.options[RSK_MASK_QUEST].value=mode;
        flags[RAND_INF_CHILD_TRADES_HAS_ZELDAS_LETTER]=true;
        tradeSlot=ITEM_MASK_SKULL;
        Randomizer_RepairLegacyMaskInventory();
        for(int flag=RAND_INF_CHILD_TRADES_HAS_MASK_KEATON;flag<=RAND_INF_CHILD_TRADES_HAS_MASK_TRUTH;++flag)
            assert(flags[flag]==(mode==RO_MASK_QUEST_VANILLA));
        const std::vector<bool> savedFlags(flags,flags+FLAG_COUNT);
        Randomizer_RepairLegacyMaskInventory();
        assert(std::equal(flags,flags+FLAG_COUNT,savedFlags.begin()));
        assert(tradeSlot==ITEM_MASK_SKULL && flags[RAND_INF_CHILD_TRADES_HAS_ZELDAS_LETTER]);
        assert(!maskPaid && !guardMask && gSaveContext.itemGetInf[3]==0);
        Reset();maskContext.options[RSK_MASK_QUEST].value=mode;randoLoaded=false;
        Randomizer_RepairLegacyMaskInventory();
        assert(std::none_of(flags,flags+FLAG_COUNT,[](bool owned){return owned;}));
        for(bool startingBunny:{false,true}) {
            Reset();maskContext.options[RSK_MASK_QUEST].value=mode;
            maskContext.options[RSK_STARTING_BUNNY_HOOD].value=startingBunny;
            BuildComboMaskStarts();
            const std::set<int> starters(StartingInventory.begin(),StartingInventory.end());
            assert(starters.size()==StartingInventory.size()); // Completed never duplicates a selected Bunny Hood.
            assert(starters.size()==(mode!=RO_MASK_QUEST_SHUFFLE?8u:static_cast<size_t>(startingBunny)));
            if(mode!=RO_MASK_QUEST_SHUFFLE)
                for(int mask=RG_KEATON_MASK;mask<=RG_MASK_OF_TRUTH;++mask)assert(starters.count(mask));
            BuildStandaloneMaskStarts();
            assert(StartingInventory.size()==static_cast<size_t>(startingBunny));
        }
        Reset(); maskContext.options[RSK_MASK_QUEST].value=mode;
        BuildComboShop();assert(areaTable[RR_MARKET_MASK_SHOP].eventCount==0);
        assert(areaTable[RR_MARKET_MASK_SHOP].checkCount==0 && areaTable[RR_MARKET_MASK_SHOP].exitCount==1);
        BuildStandaloneShop();assert(areaTable[RR_MARKET_MASK_SHOP].eventCount==5);
        assert(areaTable[RR_MARKET_MASK_SHOP].checkCount==2 && areaTable[RR_MARKET_MASK_SHOP].exitCount==1);
        maskContext.options[RSK_MASK_QUEST].value=ComboRando::NormalizeOotMaskQuestForGeneration(mode);
        BuildMaskPool();
        std::set<int> masks(pool.begin(),pool.end());
        assert(pool.size()==(mode==RO_MASK_QUEST_COMPLETED?0u:8u));
        assert(masks.size()==pool.size());
        if(mode!=RO_MASK_QUEST_COMPLETED) {
            for(int mask=RG_KEATON_MASK;mask<=RG_MASK_OF_TRUTH;++mask)assert(masks.count(mask));
            pool.clear();maskContext.options[RSK_STARTING_BUNNY_HOOD].value=1;
            BuildMaskPool();assert(pool.size()==7);
            assert(std::find(pool.begin(),pool.end(),RG_BUNNY_HOOD)==pool.end());
        }
        // Compile the real profile application and restore entry points:
        // requested Vanilla normalizes for generation, but loading the same
        // legacy snapshot preserves its serialized mode. The load repair is
        // applied separately, after settings and owned flags are both loaded.
        const nlohmann::json input={{"gRandoSettings.CompleteMaskQuest",mode},
                                    {"gRandoSettings.UnrelatedOption",1}};
        cvars.clear();stringCvars.clear();writes=0;
        const auto raw=input.dump();
        assert(SOH_ApplyProfileRandoSettingsImpl(raw.c_str(),false)==1);
        assert(writes==0 && cvars.empty());
        assert(SOH_ApplyProfileRandoSettingsImpl(raw.c_str(),true)==1);
        assert(cvars.at("gRandoSettings.CompleteMaskQuest")==
               (mode==RO_MASK_QUEST_VANILLA?RO_MASK_QUEST_SHUFFLE:mode));
        assert(cvars.at("gRandoSettings.UnrelatedOption")==1);
        Reset();SOH_RestoreRandoSettings(raw.c_str());
        assert(cvars.at("gRandoSettings.CompleteMaskQuest")==mode);
        assert(!ComboHasMask(RG_SKULL_MASK) && !ComboHasMask(RG_MASK_OF_TRUTH));
        ++cases;
    }
    for(const auto& invalid:{nlohmann::json(-1),nlohmann::json(3),nlohmann::json("0"),nlohmann::json(false)}) {
        const auto raw=nlohmann::json{{"gRandoSettings.CompleteMaskQuest",invalid}}.dump();
        writes=0;assert(SOH_ApplyProfileRandoSettingsImpl(raw.c_str(),true)==0 && writes==0);
    }
    cvars.clear();writes=0;
    assert(SOH_ApplyProfileRandoSettingsImpl("{}",true)==1);
    assert(cvars.at("gRandoSettings.CompleteMaskQuest")==RO_MASK_QUEST_SHUFFLE);
    std::cout<<"PASS: "<<cases<<" actual mask cases; owned flags, starts/menu, native mask pool, portal, generation normalization and idempotent legacy repair\n";
}
'''

file = work / 'oot-mask-quest.cpp'
file.write_text(code, encoding='utf-8')
binary = hostcxx.build(args.compiler, file, werror=True, includes=[root/'combo', json_include])
subprocess.run([str(binary)], check=True, cwd=work, timeout=hostcxx.RUN_TIMEOUT)
