"""Compile the production King Zora predicate and actual native reward hook."""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
logic = (source/'soh/soh/Enhancements/randomizer/logic.cpp').read_text(encoding='utf-8')
method = logic.split('bool Logic::CanClaimKingZoraTrade() {', 1)[1].split('\n}\n', 1)[0]
method = 'bool Logic::CanClaimKingZoraTrade() {' + method + '\n}\n'
hooks = (source/'soh/soh/Enhancements/randomizer/hook_handlers.cpp').read_text(encoding='utf-8')
native = hooks.split('case VB_ADULT_KING_ZORA_ITEM_GIVE: {', 1)[1].split('\n        case VB_TRADE_FROG:', 1)[0]
native = native.replace('EnKz* enKz = va_arg(args, EnKz*);', '')
native = native.rsplit('break;', 1)[0]
native = 'void NativeGive(EnKz* enKz) { bool keepGiving=true; bool* should=&keepGiving;\n' + native + '\n}\n'
domain = (source/'soh/soh/Enhancements/randomizer/location_access/overworld/zoras_domain.cpp').read_text(encoding='utf-8')
assert re.search(r'LOCATION\(RC_ZD_TRADE_PRESCRIPTION,\s*logic->CanClaimKingZoraTrade\(\)\)', domain)
assert 'EVENT_ACCESS(LOGIC_KING_ZORA_THAWED,     logic->IsAdult && logic->BlueFire())' in domain
item_header = (source/'soh/include/z64item.h').read_text(encoding='utf-8')
names = ['ITEM_POCKET_EGG', 'ITEM_POCKET_CUCCO', 'ITEM_COJIRO', 'ITEM_ODD_MUSHROOM',
         'ITEM_ODD_POTION', 'ITEM_SAW', 'ITEM_SWORD_BROKEN', 'ITEM_PRESCRIPTION',
         'ITEM_FROG', 'ITEM_EYEDROPS', 'ITEM_CLAIM_CHECK', 'ITEM_NONE']
constants = []
for name in names:
    value = re.search(r'/\* (0x[0-9A-Fa-f]+) \*/ '+name+r'\b', item_header).group(1)
    constants.append(f'constexpr int {name}={value};')
prelude = r'''
#define COMBO_BUILD 1
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include "rando/OotTradeRules.h"
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name,
#define RANDO_ENUM_END(name) };
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
NATIVE_ITEM_CONSTANTS
constexpr int LOGIC_KING_ZORA_THAWED=1;
std::map<std::string,int> cvars;
int CVarGetInteger(const char* key,int fallback) {
    auto it=cvars.find(key); return it==cvars.end()?fallback:it->second;
}
// This fixture supplies the generation context of the production predicate.
// logic_parity_tests separately compiles and checks the real context reader.
int GameplayRuleValue(const char* key) {
    return CVarGetInteger((std::string("gCombo.Logic.")+key).c_str(),0);
}
class Logic {
public:
    bool IsAdult=true, thawed=true, shield=true;
    std::set<RandomizerGet> items{RG_SPEAK_ZORA};
    bool Get(int key) {assert(key==LOGIC_KING_ZORA_THAWED); return thawed;}
    bool HasItem(RandomizerGet item) {return items.count(item)!=0;}
    bool CanUse(RandomizerGet item) {return HasItem(item)&&IsAdult;}
    bool CanShield() {return shield&&IsAdult;}
    bool CanClaimKingZoraTrade();
};
#define CVAR_ENHANCEMENT(x) "gEnhancements." x
constexpr int BTN_R=1, EQUIP_TYPE_SHIELD=0, EQUIP_VALUE_SHIELD_NONE=0;
constexpr int ITEM_TRADE_ADULT=0, EXCH_ITEM_PRESCRIPTION=1;
constexpr int RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION=1;
constexpr int RAND_INF_ADULT_TRADES_HAS_PRESCRIPTION=2, RAND_INF_KING_ZORA_THAWED=3;
struct Input {struct {int button=0;} cur;};
struct PlayState {struct {Input input[1];} state;};
PlayState play;
PlayState* gPlayState=&play;
struct EnKz {bool isTrading=false;};
int tradeItem=ITEM_NONE, equippedShield=0, exchange=0;
std::set<int> flags;
#define CHECK_BTN_ANY(button,mask) ((button)&(mask))
#define CUR_EQUIP_VALUE(type) equippedShield
#define INV_CONTENT(slot) tradeItem
int Actor_GetPlayerExchangeItemId(PlayState*) {return exchange;}
void Flags_SetRandomizerInf(int flag) {flags.insert(flag);}
void Flags_UnsetRandomizerInf(int flag) {flags.erase(flag);}
int Randomizer_GetNextAdultTradeItem() {return ITEM_NONE;}
void Inventory_ReplaceItem(PlayState*,int from,int to) {if(tradeItem==from) tradeItem=to;}
'''.replace('NATIVE_ITEM_CONSTANTS', '\n'.join(constants))
tests = r'''
int main() {
    // Every prerequisite combination is tested against the actual production helper.
    for(int bits=0;bits<128;++bits) {
        const bool adult=bits&1, thawed=bits&2, speaks=bits&4, prescription=bits&8;
        const bool enabled=bits&16, shield=bits&32, earlier=bits&64;
        assert(ComboRando::CanClaimKingZoraTrade(adult,thawed,speaks,prescription,enabled,shield,earlier)
            == (adult&&thawed&&speaks&&(prescription||(enabled&&shield&&earlier))));
    }
    const RandomizerGet earlierItems[]={RG_POCKET_EGG,RG_COJIRO,RG_ODD_MUSHROOM,
        RG_ODD_POTION,RG_POACHERS_SAW,RG_BROKEN_SWORD};
    Logic logic;
    cvars["gEnhancements.EarlyEyeballFrog"]=1;
    for(auto item:earlierItems) {
        logic=Logic{}; logic.items.insert(item);
        assert(!logic.CanClaimKingZoraTrade()); // runtime-only must not change logic
    }
    cvars["gCombo.Logic.gEnhancements.EarlyEyeballFrog"]=1;
    for(auto item:earlierItems) {
        logic=Logic{}; logic.items.insert(item); assert(logic.CanClaimKingZoraTrade());
        logic.IsAdult=false; assert(!logic.CanClaimKingZoraTrade()); logic.IsAdult=true;
        logic.thawed=false; assert(!logic.CanClaimKingZoraTrade()); logic.thawed=true;
        logic.shield=false; assert(!logic.CanClaimKingZoraTrade()); logic.shield=true;
        logic.items.erase(RG_SPEAK_ZORA); assert(!logic.CanClaimKingZoraTrade());
    }
    for(auto item:{RG_EYEBALL_FROG,RG_EYEDROPS,RG_CLAIM_CHECK,RG_BLUE_RUPEE}) {
        logic=Logic{};logic.items.insert(item);assert(!logic.CanClaimKingZoraTrade());
    }
    logic=Logic{}; assert(!logic.CanClaimKingZoraTrade()); // empty trade slot
    cvars["gCombo.Logic.gEnhancements.EarlyEyeballFrog"]=0;
    logic.items.insert(RG_PRESCRIPTION); logic.shield=false;
    assert(logic.CanClaimKingZoraTrade()); // ordinary trade never needs restoration or shield
    logic.items.erase(RG_SPEAK_ZORA); assert(!logic.CanClaimKingZoraTrade());
    // Execute the unchanged native hook, preserving the original reward flags and inventory behavior.
    EnKz king;
    for(int enabled=0;enabled<2;++enabled) for(int shield=0;shield<2;++shield)
      for(int held=0;held<2;++held) for(int item=ITEM_POCKET_EGG;item<=ITEM_CLAIM_CHECK;++item) {
        cvars["gEnhancements.EarlyEyeballFrog"]=enabled;
        equippedShield=shield; play.state.input[0].cur.button=held?BTN_R:0;
        tradeItem=item; exchange=0; flags.clear(); king.isTrading=false;
        NativeGive(&king);
        const bool early=enabled&&shield&&held&&item<ITEM_FROG;
        assert(flags.count(RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION)==unsigned(early));
        assert(flags.count(RAND_INF_KING_ZORA_THAWED)==unsigned(!early));
        assert(tradeItem==(early&&item==ITEM_PRESCRIPTION?ITEM_NONE:item));
      }
    tradeItem=ITEM_NONE;flags.clear();equippedShield=1;play.state.input[0].cur.button=BTN_R;
    cvars["gEnhancements.EarlyEyeballFrog"]=1;NativeGive(&king);
    assert(!flags.count(RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION));
    for(int enabled=0;enabled<2;++enabled) {
        cvars["gEnhancements.EarlyEyeballFrog"]=enabled;
        tradeItem=ITEM_PRESCRIPTION;exchange=EXCH_ITEM_PRESCRIPTION;king.isTrading=true;flags.clear();
        equippedShield=0;play.state.input[0].cur.button=0;NativeGive(&king);
        assert(flags.count(RAND_INF_ADULT_TRADES_ZD_TRADE_PRESCRIPTION)&&tradeItem==ITEM_NONE);
    }
    std::cout<<"PASS: production King Zora predicate, independent runtime/logic, all prerequisites, six early trade items, later/empty rejection and actual native shield-delay reward hook\n";
}
'''
test = work/'early_frog_rules.cpp'
test.write_text(prelude + method + native + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, test, werror=True, includes=[source/'combo', source/'soh'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
