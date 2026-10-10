"""Compile the production OoT refill events and item-use predicates."""
from pathlib import Path
import argparse
import re
import subprocess
import json
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root = a.source.resolve()
work = a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
contracts = json.loads((Path(__file__).parent/'fixtures/ootmm-32.3-contracts.json').read_text())
# This is the exact community macro, not an inferred current-balance policy.
assert contracts['walletMacros']['can_use_wallet(n)'] == 'has_rupees && has_wallet(n)'
assert contracts['walletMacros']['wallet_price(range, id)'].startswith('price(range, id, 0) || (has_rupees &&')


def function(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    end = opening + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

oot = (root / 'soh/soh/Enhancements/randomizer/logic.cpp').read_text()
location = (root / 'soh/soh/Enhancements/randomizer/location_access.cpp').read_text()
sources = (root / 'combo/rando/OotResourceSources.h').read_text()
assert 'RegisterOotResourceSources();' in function(location, 'void RegionTable_Init(')
assert 'OotGrottoResourceEvents();' in function(location, 'void RegionTable_Init(')
assert 'memset(inLogic, false, sizeof(inLogic));' in function(oot, 'void Logic::Reset(')

enum_dir = root / 'soh/soh/Enhancements/randomizer/randomizerEnums'
enums = '\n'.join((enum_dir / name).read_text() for name in
                  ('LogicVal.h', 'RandomizerGet.h', 'RandomizerRegion.h'))
source_body = sources[sources.index('inline std::vector<EventAccess>'):]
tricks = sorted(set(re.findall(r'\bRT_[A-Z0-9_]+', source_body)))
has_item = function(oot, 'bool Logic::HasItem(')
flag_block = has_item[has_item.index('        case RG_QUIVER_INF:'):]
flag_block = flag_block[:flag_block.index('            // Boss Souls')]
flag_items = re.findall(r'case (RG_[A-Z0-9_]+):', flag_block)
mapping = function(oot, 'std::map<RandomizerGet, uint32_t> StaticData::RandoGetToRandInf =')
flag_pairs = [(item, flag) for item, flag in re.findall(r'\{ (RG_[A-Z0-9_]+), (RAND_INF_[A-Z0-9_]+) \}', mapping)
              if item in flag_items]
assert {item for item, _ in flag_pairs} == set(flag_items)
flag_bindings = '\nenum { ' + ','.join(sorted({flag for _, flag in flag_pairs})) + ' };\n'
flag_bindings += 'namespace Rando { namespace StaticData {\nstd::map<RandomizerGet,uint32_t> RandoGetToRandInf = {\n'
flag_bindings += ',\n'.join('{'+item+','+flag+'}' for item, flag in flag_pairs)
flag_bindings += '}; }\nstd::set<uint32_t> flags;\nbool CheckRandoInf(uint32_t flag) {return flags.count(flag)!=0;}\n'
flag_bindings += 'bool ProductionFlagOwnership(RandomizerGet itemName) {switch(itemName) {\n'+flag_block+'default:return false;}}}\n'

# External inputs are bound explicitly. The full CanUse/BombchuRefill,
# source registration, event propagation and wallet predicates are compiled
# unchanged from production; this is not a copy of those conditions.
code = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>
#define COMBO_BUILD 1
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name,
#define RANDO_ENUM_END(name) };
''' + enums + flag_bindings + '\nenum {' + ','.join(tricks) + r'''};
enum { RSK_ENABLE_BOMBCHU_DROPS, RSK_LOGIC_RULES, RO_AMMO_DROPS_ON=1, RO_LOGIC_GLITCHLESS=0 };
struct Option { int value; bool Is(int v) const {return value==v;} };
struct Context {
    bool drops=true,lensless=false;
    Option GetOption(int key) {return {key==RSK_ENABLE_BOMBCHU_DROPS?int(drops):0};}
    bool GetTrickOption(int) {return lensless;}
};
Context context; Context* ctx=&context;
#define SPDLOG_INFO(...)
enum {ITEM_BOW,ITEM_SLINGSHOT,ITEM_BOMB,ITEM_BOMBCHU};
struct SaveContext {int magic=0,rupees=0;};
namespace Rando {struct Logic {
    std::set<RandomizerGet> owned;
    bool inLogic[LOGIC_MAX]{};
    bool IsChild=false,IsAdult=false,AtDay=false,AtNight=false;
    bool allowed=true,bottle=false,pots=true,shrubs=false,sword=false,smash=false;
    bool CalculatingAvailableChecks=false;
    uint8_t ammo[4]{}; SaveContext save;
    uint8_t GetAmmo(uint32_t item) const {return ammo[item];}
    SaveContext* GetSaveContext() {return &save;}
    Context* ctx=&context;
    bool HasItem(RandomizerGet item) const {return owned.count(item)!=0;}
    bool ItemUseAllowed(RandomizerGet) const {return allowed;}
    bool Get(LogicVal event) const {return inLogic[event];}
    void Set(LogicVal event,bool value) {inLogic[event]=value;}
    bool HasBottle() const {return bottle;}
    bool BombchusEnabled() const {return true;}
    bool CanBreakPots() const {return pots;}
    bool CanCutShrubs() const {return shrubs;}
    bool CanJumpslashExceptHammer() const {return sword;}
    bool BlastOrSmash() const {return smash;}
    bool CanUse(RandomizerGet);
    bool BombchuRefill();
};}
std::shared_ptr<Rando::Logic> logic=std::make_shared<Rando::Logic>();
using ConditionFn=bool(*)();
struct EventAccess {
    LogicVal event; const char* name; ConditionFn condition; std::string description;
    bool GetEvent() const {return logic->Get(event);}
    bool CheckConditionAtAgeTime(bool& age,bool& time);
    bool ConditionsMet() const {return condition();}
    void EventOccurred() {logic->Set(event,true);}
};
std::string CleanConditionString(const char* text) {return text;}
#define EVENT_ACCESS(event,condition) EventAccess{event,#event,[]{return condition;},#condition}
struct Region {
    std::vector<EventAccess> events;
    bool childDay=false,childNight=false,adultDay=false,adultNight=false;
    bool UpdateEvents();
};
Region areaTable[RR_MAX];
uint16_t GetWalletCapacity();
enum {PT_EVENT_ACCESS};
void StartPerformanceTimer(int) {} void StopPerformanceTimer(int) {}
''' + function(oot, 'bool Logic::CanUse(').replace('bool Logic::', 'bool Rando::Logic::') + '\n' + \
function(oot, 'bool Logic::BombchuRefill(').replace('bool Logic::', 'bool Rando::Logic::') + '\n' + \
function(location, 'bool EventAccess::CheckConditionAtAgeTime(') + '\n' + \
function(location, 'bool Region::UpdateEvents(') + '\n' + \
function(location, 'uint16_t GetWalletCapacity(') + '\n' + source_body + r'''
void Reset() {
    *logic=Rando::Logic{}; context=Context{};
    for(auto& region:areaTable) region=Region{};
    RegisterOotResourceSources();
}
void Search(RandomizerRegion region,bool child) {
    auto& r=areaTable[region];r.childDay=child;r.adultDay=!child;
    while(r.UpdateEvents()) {}
    logic->IsChild=child;logic->IsAdult=!child;
}
int main() {
    // The production ownership switch must recognize each native infinite
    // upgrade, read its saved flag, and keep the eight flags independent.
    const RandomizerGet nativeFlags[]={RG_QUIVER_INF,RG_BOMB_BAG_INF,RG_BULLET_BAG_INF,
        RG_STICK_UPGRADE_INF,RG_NUT_UPGRADE_INF,RG_MAGIC_INF,RG_BOMBCHU_INF,RG_WALLET_INF};
    for(auto item:nativeFlags) {
        Rando::flags.clear();assert(!Rando::ProductionFlagOwnership(item));
        Rando::flags.insert(Rando::StaticData::RandoGetToRandInf.at(item));
        for(auto other:nativeFlags) assert(Rando::ProductionFlagOwnership(other)==(item==other));
    }
    // Capacity/ownership alone must not manufacture renewable supplies.
    // Each actual use remains age-, ownership- and item-restriction-sensitive.
    const RandomizerGet items[]={RG_FAIRY_BOW,RG_FAIRY_SLINGSHOT,RG_BOMB_BAG,RG_MAGIC_SINGLE};
    const LogicVal source[]={LOGIC_RENEWABLE_ARROWS,LOGIC_RENEWABLE_SEEDS,
                            LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU,LOGIC_RENEWABLE_MAGIC};
    const LogicVal shop[]={LOGIC_BUY_ARROW,LOGIC_BUY_SEED,LOGIC_BUY_BOMB,LOGIC_BUY_MAGIC_POTION};
    for(int item=0;item<4;++item) for(int mode=0;mode<3;++mode)
      for(int owned=0;owned<2;++owned) for(int adult=0;adult<2;++adult)
      for(int allowed=0;allowed<2;++allowed) for(int bottle=0;bottle<2;++bottle) {
        Reset();if(owned) logic->owned.insert(items[item]);
        logic->IsAdult=adult!=0;logic->IsChild=adult==0;
        logic->allowed=allowed!=0;logic->bottle=bottle!=0;
        if(mode==1)logic->Set(source[item],true);
        if(mode==2)logic->Set(shop[item],true);
        const bool supplies=mode==1||(mode==2&&(item!=3||bottle));
        const bool age=item==0?adult!=0:item==1?adult==0:true;
        assert(logic->CanUse(items[item])==(owned&&allowed&&age&&supplies));
      }
    // Tracker and generation use the same renewable-supply contract. A finite
    // balance must not change logical reachability in either direction.
    for(int item=0;item<4;++item) for(int live=0;live<2;++live)
      for(int stocked=0;stocked<2;++stocked) for(int owned=0;owned<2;++owned)
      for(int adult=0;adult<2;++adult) for(int allowed=0;allowed<2;++allowed) {
        Reset();if(owned)logic->owned.insert(items[item]);
        logic->CalculatingAvailableChecks=live!=0;
        logic->IsAdult=adult!=0;logic->IsChild=adult==0;logic->allowed=allowed!=0;
        if(item<3)logic->ammo[item]=stocked?10:0;else logic->save.magic=stocked?48:0;
        assert(!logic->CanUse(items[item]));
      }
    Reset();logic->ammo[ITEM_BOMBCHU]=10;assert(!logic->BombchuRefill());
    logic->CalculatingAvailableChecks=true;assert(!logic->BombchuRefill());
    logic->ammo[ITEM_BOMBCHU]=0;assert(!logic->BombchuRefill());
    // Native infinite upgrades are actual refill capabilities. They still
    // require the equipment, allowed use and the correct age.
    const RandomizerGet infinite[]={RG_QUIVER_INF,RG_BULLET_BAG_INF,RG_BOMB_BAG_INF,RG_MAGIC_INF};
    for(int item=0;item<4;++item) for(int owned=0;owned<2;++owned)
      for(int adult=0;adult<2;++adult) for(int allowed=0;allowed<2;++allowed) {
        Reset();logic->owned.insert(infinite[item]);if(owned)logic->owned.insert(items[item]);
        logic->IsAdult=adult!=0;logic->IsChild=adult==0;logic->allowed=allowed!=0;
        const bool age=item==0?adult!=0:item==1?adult==0:true;
        assert(logic->CanUse(items[item])==(owned&&allowed&&age));
      }
    Reset();logic->owned.insert(RG_BOMBCHU_INF);assert(logic->BombchuRefill());
    // A child-only source cannot refill an adult bow or an unreachable age.
    Reset();logic->owned={RG_FAIRY_BOW,RG_FAIRY_SLINGSHOT,RG_MAGIC_SINGLE,RG_BOMB_BAG};
    Search(RR_GERUDO_VALLEY,false);
    assert(!logic->CanUse(RG_FAIRY_BOW));assert(!logic->CanUse(RG_MAGIC_SINGLE));
    Search(RR_GERUDO_VALLEY,true);
    assert(logic->CanUse(RG_FAIRY_SLINGSHOT));assert(logic->CanUse(RG_MAGIC_SINGLE));
    assert(logic->CanUse(RG_BOMB_BAG));
    // Reaching the other age's source supplies that age's weapon.
    Reset();logic->owned={RG_FAIRY_BOW,RG_FAIRY_SLINGSHOT};
    Search(RR_KOKIRI_FOREST,true);assert(logic->CanUse(RG_FAIRY_SLINGSHOT));
    logic->IsChild=false;logic->IsAdult=true;assert(!logic->CanUse(RG_FAIRY_BOW));
    Search(RR_KOKIRI_FOREST,false);assert(logic->CanUse(RG_FAIRY_BOW));
    // Shuffled lift/attack abilities cannot be assumed for pot-only refills.
    Reset();logic->pots=false;logic->owned={RG_MAGIC_SINGLE};
    Search(RR_SPIRIT_TEMPLE_FOYER,true);assert(!logic->CanUse(RG_MAGIC_SINGLE));
    logic->pots=true;Search(RR_SPIRIT_TEMPLE_FOYER,true);assert(logic->CanUse(RG_MAGIC_SINGLE));
    // Grass refills need a grass-breaking ability; possession of a bow alone
    // does not cut this grass. The solver then converges after equipment changes.
    Reset();logic->owned={RG_FAIRY_BOW,RG_MAGIC_SINGLE};
    Search(RR_LAKE_HYLIA,false);assert(!logic->CanUse(RG_FAIRY_BOW));assert(!logic->CanUse(RG_MAGIC_SINGLE));
    logic->shrubs=true;Search(RR_LAKE_HYLIA,false);assert(logic->CanUse(RG_FAIRY_BOW));assert(logic->CanUse(RG_MAGIC_SINGLE));
    // Generic grottos carry their own events, including age-specific ammunition.
    Reset();areaTable[RR_KF_STORMS_GROTTO].events=OotGrottoResourceEvents();
    logic->shrubs=true;Search(RR_KF_STORMS_GROTTO,true);
    assert(logic->Get(LOGIC_RENEWABLE_SEEDS));assert(!logic->Get(LOGIC_RENEWABLE_ARROWS));
    // A song-refilled Great Fairy needs the instrument, knowledge and all notes.
    const RandomizerGet lullaby[]={RG_ZELDAS_LULLABY,RG_FAIRY_OCARINA,RG_OCARINA_C_LEFT_BUTTON,
                                  RG_OCARINA_C_RIGHT_BUTTON,RG_OCARINA_C_UP_BUTTON};
    for(int missing=-1;missing<5;++missing) {
      Reset();for(int i=0;i<5;++i)if(i!=missing)logic->owned.insert(lullaby[i]);
      Search(RR_DMT_GREAT_FAIRY_FOUNTAIN,true);
      assert(logic->Get(LOGIC_RENEWABLE_MAGIC)==(missing<0));
    }
    // Drop mode is permission to drop Bombchus, not access to a drop location.
    for(int drops=0;drops<2;++drops)for(int sourceMode=0;sourceMode<4;++sourceMode) {
      Reset();context.drops=drops!=0;
      if(sourceMode==1)logic->Set(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU,true);
      if(sourceMode==2)logic->Set(LOGIC_BUY_BOMBCHUS,true);
      if(sourceMode==3)logic->Set(LOGIC_COULD_PLAY_BOWLING,true);
      assert(logic->BombchuRefill()==(sourceMode>=2||(drops&&sourceMode==1)));
    }
    // Real finite wallet limits still apply. Free checks stay free without
    // money; capacity alone cannot make a positive-price purchase possible.
    for(int sourceMode=0;sourceMode<2;++sourceMode)for(int tier=0;tier<5;++tier) {
      Reset();const RandomizerGet wallets[]={RG_CHILD_WALLET,RG_ADULT_WALLET,RG_GIANT_WALLET,RG_TYCOON_WALLET};
      if(tier)logic->owned.insert(wallets[tier-1]);
      if(sourceMode)logic->Set(LOGIC_RENEWABLE_RUPEES,true);
      const int capacities[]={0,99,200,500,999};
      assert(GetWalletCapacity()==(sourceMode?capacities[tier]:0));
      assert(0<=GetWalletCapacity());
    }
    for(int live=0;live<2;++live)for(int sourceMode=0;sourceMode<2;++sourceMode)
      for(int tier=0;tier<5;++tier)for(int held: {-1,0,19,20,50,100,300,999,2000}) {
        Reset();const RandomizerGet wallets[]={RG_CHILD_WALLET,RG_ADULT_WALLET,RG_GIANT_WALLET,RG_TYCOON_WALLET};
        if(tier)logic->owned.insert(wallets[tier-1]);
        logic->CalculatingAvailableChecks=live!=0;logic->save.rupees=held;
        if(sourceMode)logic->Set(LOGIC_RENEWABLE_RUPEES,true);
        const int capacities[]={0,99,200,500,999};
        assert(GetWalletCapacity()==(sourceMode?capacities[tier]:0));
      }
    Reset();assert(!logic->Get(LOGIC_RENEWABLE_SEEDS));assert(!logic->Get(LOGIC_RENEWABLE_ARROWS));
    assert(!logic->Get(LOGIC_RENEWABLE_MAGIC));assert(!logic->Get(LOGIC_RENEWABLE_RUPEES));
    std::cout<<"PASS: production refill events, ownership/age/restriction boundaries, pot/grass collection, fairy notes, Bombchu supply and finite wallets\n";
}
'''
test = work / 'resource_sources.cpp'
test.write_text(code)
exe = hostcxx.build(a.compiler, test, werror=True)
run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
(work / 'result.txt').write_text(run.stdout + run.stderr)
print(run.stdout + run.stderr, end='')
run.check_returncode()
