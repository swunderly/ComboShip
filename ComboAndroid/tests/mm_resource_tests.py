"""Compile unchanged production MM resource propagation against explicit inputs."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
contracts = json.loads((Path(__file__).parent/'fixtures/ootmm-32.3-contracts.json').read_text())
reference = Path(__file__).resolve().parents[3]/'references/ootmm-stable-32.3'
if reference.is_dir():
    for name, evidence in contracts['sources'].items():
        assert hashlib.sha256((reference/name).read_bytes()).hexdigest() == evidence['sha256'], name
    macros = (reference/'data/macros/macros_mm.yml').read_text(encoding='utf-8')
    for name in ('has_rupees', 'has_magic', 'has_arrows', 'bombchu_license', 'bombchu_source', 'has_bombchu'):
        expression = contracts['mmResourceContracts'][name]
        assert f'"{name}": "{expression}"' in macros, name
    def event(file, area, key):
        text = (reference/file).read_text(encoding='utf-8')
        heading = re.search(r'^"'+re.escape(area)+r'":', text, re.M)
        assert heading, area
        block = text[heading.end():].split('\n"', 1)[0]
        match = re.search(r'^    "?'+re.escape(key)+r'"?: "([^"]+)"', block, re.M)
        assert match, (area, key)
        return match.group(1)
    for label, file, area, key in (
        ('fieldRupees','termina_field','Termina Field','RUPEES'),
        ('fieldArrows','termina_field','Termina Field','ARROWS'),
        ('fieldBombs','termina_field','Termina Field','BOMBS_OR_BOMBCHU'),
        ('fieldMagic','termina_field','Termina Field','MAGIC'),
        ('fieldSticks','termina_field','Termina Field','STICKS'),
        ('swampSticks','southern_swamp','Swamp Front','STICKS'),
        ('swampMagic','southern_swamp','Swamp Front','MAGIC'),
        ('woodfallSticks','woodfall','Owl Woodfall','STICKS'),
        ('woodfallNuts','woodfall','Owl Woodfall','NUTS'),
        ('honeyDay3','clock_town','Honey & Darling Game','HD_REWARD_3'),
        ('honeyAllDays','clock_town','Honey & Darling Game','Honey & Darling Reward All Days'),
    ):
        assert event('data/world/mm/overworld/'+file+'.yml',area,key) == contracts['mmResourceContracts'][label], label
source = (root/'mm/2s2h/Rando/Logic/Logic.cpp').read_text(encoding='utf-8')
resource_start = source.index('namespace {\nuint32_t RefillResource')
body = source[resource_start:source.index('\n#endif', resource_start)]
stock_header = root/'mm/2s2h/Rando/ActorBehavior/RenewableStock.h'
bean_header = root/'mm/2s2h/Rando/ActorBehavior/BeanSources.h'
bean_definition = bean_header.read_text(encoding='utf-8').split('namespace Rando {', 1)[1]
ids = sorted(set(re.findall(r'\b(?:RC|RR|RI|ACTOR)_[A-Z0-9_]+', body + stock_header.read_text(encoding='utf-8') + bean_definition)))
logic_header = (root/'mm/2s2h/Rando/Logic/Logic.h').read_text(encoding='utf-8')
time_enum = re.search(r'enum TimeSlice\s*\{([^}]+)\}', logic_header).group(1)
time_count = len(re.findall(r'\bTIME_(?:DAY|NIGHT)[123]_[AP]M_\d\d_\d\d\b', time_enum))
assert time_count == 46, 'Resource propagation scaffold must cover every production time point'
enum = 'enum { ' + ','.join(ids) + ' };\n'
code = r'''
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
'''
code += '#include "' + (root/'combo/rando/MmResourceAccess.h').as_posix() + '"\n'
code += '#include "' + (root/'combo/rando/entrances/MmRegionalFlags.h').as_posix() + '"\n' + enum
code += r'''
using RandoItemId=int; using RandoRegionId=int; using RandoCheckId=int; using RandoCheckType=int;
'''
code += '#include "' + stock_header.as_posix() + '"\n'
code += 'constexpr int productionTimeSliceCount=' + str(time_count) + ';\n'
code += r'''
constexpr int ITEM_BOMBCHU=10,ITEM_HOOKSHOT=11,ITEM_MAGIC_BEANS=15;
constexpr int RCTYPE_UNKNOWN=0,RCTYPE_SHOP=12,RCTYPE_COW=13,RCTYPE_TINGLE_SHOP=14;
enum {RE_ACCESS_MILK_REFILL,RE_ACCESS_RED_POTION_REFILL,RE_ACCESS_GREEN_POTION_REFILL,RE_ACCESS_BLUE_POTION_REFILL,RE_ACCESS_BEANS_REFILL};
bool randoEvents[5]={false};
#define RANDO_EVENTS randoEvents
bool deku=false,goron=false,zora=false,sword=false,hook=false,explosive=false,bow=false;
bool babaSoul=false,chuSoul=false,bottle=false;
bool beanLicense=false;
#define INV_CONTENT(item) (beanLicense ? ITEM_MAGIC_BEANS : -1)
unsigned wallet=99;
#define CAN_PAY(price) ComboRando::mmResourceAccess.CanPay((price),wallet)
#define HAS_BOTTLE bottle
#define HAS_ITEM(x) ((x)==ITEM_HOOKSHOT && hook)
#define CAN_USE_SWORD sword
#define CAN_BE_DEKU deku
#define CAN_BE_GORON goron
#define CAN_BE_ZORA zora
#define CAN_USE_EXPLOSIVE explosive
#define CAN_USE_BOW bow
bool HaveEnemySoul(int actor) {return actor==ACTOR_EN_SLIME?chuSoul:babaSoul;}
namespace Rando { namespace StaticData {
struct Definition {int randoCheckType=0;};
std::map<int,Definition> Checks;
}}
struct SavedCheck {RandoItemId randoItemId=0;bool shuffled=true;unsigned price=0;};
std::map<int,SavedCheck> saved;
#define RANDO_SAVE_CHECKS saved
struct RandoRegion {
    std::map<int,std::pair<std::function<bool()>,const char*>> checks;
};
struct RegionTimeState {
    uint64_t timeSlices=0;
    ComboRando::Entrances::MmFlagDomain regionalFlags{};
};
std::map<int,RandoRegion> Regions;
uint64_t gCurrentRegionTime=0;
namespace TimeLogic {
uint64_t EvaluateConditionTimes(const std::function<bool()>& condition,uint64_t times) {
    const auto old=gCurrentRegionTime;uint64_t result=0;
    for(int i=0;i<productionTimeSliceCount;++i) if(times & (uint64_t(1)<<i)) {
        gCurrentRegionTime=uint64_t(1)<<i;
        if(condition()) result|=gCurrentRegionTime;
    }
    gCurrentRegionTime=old;return result;
}}
'''
code += 'namespace Rando {' + bean_definition + '\n'
code += body
code += r'''
int main() {
    using namespace ComboRando;
    for(unsigned capacity: {99u,200u,500u,5000u}) {
        MmResourceScope search;
        assert(!mmResourceAccess.CanPay(20,capacity));
        assert(mmResourceAccess.CanPay(0,capacity));
        mmResourceAccess.Add(MmRupees);
        assert(mmResourceAccess.CanPay(capacity,capacity));
        assert(!mmResourceAccess.CanPay(capacity+1,capacity));
    }
    {
        MmResourceScope search;
        assert(!mmResourceAccess.CanUse(true,MmMagic));
        assert(!mmResourceAccess.CanUse(true,MmArrows));
        mmResourceAccess.Add(MmMagic|MmArrows);
        assert(mmResourceAccess.CanUse(true,MmMagic));
        assert(!mmResourceAccess.CanUse(false,MmMagic));
        assert(!mmResourceAccess.CanUse(false,MmArrows));
        const auto outer=mmResourceAccess.sources;
        {MmResourceScope nested;assert(mmResourceAccess.sources==0);mmResourceAccess.Add(MmBombs);}
        assert(mmResourceAccess.sources==outer);
    }
    assert(!mmResourceAccess.active && mmResourceAccess.sources==0);
    assert(OutdoorResources(RR_TERMINA_FIELD)==(MmRupees|MmArrows|MmBombs));
    babaSoul=true;deku=true;
    assert(OutdoorResources(RR_TERMINA_FIELD)&MmSticks);
    assert(OutdoorResources(RR_TERMINA_FIELD)&MmNuts);
    deku=false;assert(!(OutdoorResources(RR_TERMINA_FIELD)&MmNuts));
    hook=true;assert(OutdoorResources(RR_TERMINA_FIELD)&MmNuts);
    babaSoul=false;assert(!(OutdoorResources(RR_TERMINA_FIELD)&MmNuts));
    chuSoul=true;assert(OutdoorResources(RR_TERMINA_FIELD)&MmMagic);
    assert(!(OutdoorResources(RR_TERMINA_FIELD)&MmBombchus));
    assert(OutdoorResources(RR_CLOCK_TOWN_LAUNDRY)==MmMagic);
    assert(OutdoorResources(RR_WOODFALL_OWL_STATUE_PLATFORM)==(MmSticks|MmNuts));
    assert(OutdoorResources(RR_SOUTHERN_SWAMP_NORTH)&MmSticks);
    assert(RefillResource(RI_GREEN_POTION_REFILL)==0);
    bottle=true;assert(RefillResource(RI_GREEN_POTION_REFILL)==MmMagic);
    assert(RefillResource(RI_MAGIC_JAR_SMALL)==MmMagic);
    assert(RefillResource(-1)==0);
    // A Well event is not a northern spring. Only a reachable local source
    // unlocks local hot water; scoped state must not survive a new search.
    {
        MmResourceScope northSearch;
        Regions[RR_GORON_GRAVEYARD]={};
        Regions[RR_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO]={};
        std::unordered_map<int,RegionTimeState> northTimes{
            {RR_GORON_GRAVEYARD,{1}},
            {RR_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO,{1}}};
        goron=false;
        UpdateResourceSources({RR_GORON_GRAVEYARD},northTimes);
        assert(!mmResourceAccess.Has(MmHotWaterNorth));
        goron=true;
        assert(UpdateResourceSources({RR_GORON_GRAVEYARD},northTimes));
        assert(mmResourceAccess.Has(MmHotWaterNorth));
        mmResourceAccess.sources=0;goron=false;
        assert(UpdateResourceSources({RR_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO},northTimes));
        assert(mmResourceAccess.Has(MmHotWaterNorth));
    }
    // A paid shelf cannot supply its own starting money. A future valid visit
    // can supply refills; the live hour is irrelevant and must be restored.
    constexpr int room=-20,check=-21;
    Regions[room].checks[check]={[](){return mmResourceAccess.CanPay(20,99) && gCurrentRegionTime==2;},"paid"};
    Rando::StaticData::Checks[check]={RCTYPE_SHOP};saved[check]={RI_DEKU_NUTS_5};
    Regions[RR_CLOCK_TOWN_NORTH]={};
    std::unordered_map<int,RegionTimeState> times{{room,{3}},{RR_CLOCK_TOWN_NORTH,{1}}};
    MmResourceScope search;gCurrentRegionTime=uint64_t(1)<<40;
    assert(!UpdateResourceSources({room},times));assert(!mmResourceAccess.Has(MmNuts));
    assert(UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times));
    UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times);
    assert(mmResourceAccess.Has(MmRupees)&&mmResourceAccess.Has(MmNuts));
    assert(gCurrentRegionTime==(uint64_t(1)<<40));
    mmResourceAccess.sources=0;times[room].timeSlices=1;
    UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times);
    assert(!mmResourceAccess.Has(MmNuts));
    times[room].timeSlices=0;mmResourceAccess.sources=0;
    assert(!UpdateResourceSources({room},times));
    // Vanilla shelves have no rolled price but still require renewable money.
    times[room].timeSlices=2;Regions[room].checks[check]={[](){return true;},"vanilla"};
    saved[check].shuffled=false;
    Rando::StaticData::Checks[check].randoCheckType=RCTYPE_SHOP;
    assert(!UpdateResourceSources({room},times));
    UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times);
    UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times);
    assert(mmResourceAccess.Has(MmNuts));
    // Ignore legacy random metadata on an unchanged shelf without mutating
    // the save. The same price on a shuffled shelf is a genuine wallet gate.
    saved[check].price=150;
    Regions[room].checks[check]={[](){return CAN_PAY(saved[check].price);},"legacy price"};
    mmResourceAccess.sources=MmRupees;
    UpdateResourceSources({room},times);
    assert(mmResourceAccess.Has(MmNuts)&&saved[check].price==150);
    saved[check].shuffled=true;mmResourceAccess.sources=MmRupees;
    UpdateResourceSources({room},times);
    assert(!mmResourceAccess.Has(MmNuts)&&saved[check].price==150);
    saved[check].shuffled=false;
    // Milk from a paid shelf is a renewable trade resource, independent of the
    // live hour. An inaccessible shelf or missing bottle cannot supply it.
    saved[check]={RI_MILK_REFILL,false,20};
    Regions[room].checks[check]={[](){return CAN_PAY(20);},"milk"};
    mmResourceAccess.sources=0;
    assert(!UpdateResourceSources({room},times));
    assert(!RANDO_EVENTS[RE_ACCESS_MILK_REFILL]);
    mmResourceAccess.sources=MmRupees;
    assert(UpdateResourceSources({room},times));
    assert(RANDO_EVENTS[RE_ACCESS_MILK_REFILL]);
    RANDO_EVENTS[RE_ACCESS_MILK_REFILL]=false;bottle=false;
    assert(!UpdateResourceSources({room},times));
    assert(!RANDO_EVENTS[RE_ACCESS_MILK_REFILL]);
    bottle=true;Regions[room].checks[check]={[](){return false;},"locked milk"};
    assert(!UpdateResourceSources({room},times));
    assert(!RANDO_EVENTS[RE_ACCESS_MILK_REFILL]);
    // A cow does not impose the shelf's money gate.
    Rando::StaticData::Checks[check]={RCTYPE_COW};
    mmResourceAccess.sources=0;Regions[room].checks[check]={[](){return true;},"cow milk"};
    assert(UpdateResourceSources({room},times));
    assert(RANDO_EVENTS[RE_ACCESS_MILK_REFILL]);
    Rando::StaticData::Checks[check]={RCTYPE_SHOP};saved[check]={RI_DEKU_NUTS_5,false,150};
    mmResourceAccess.sources=MmRupees;
    Regions[room].checks[check]={[]()->bool {throw std::runtime_error("test");},"throws"};
    try {UpdateResourceSources({room},times);assert(false);} catch(const std::runtime_error&) {}
    assert(saved[check].price==150&&gCurrentRegionTime==(uint64_t(1)<<40));
    Regions[room].checks.clear();
    Regions[room].checks[RC_MILK_BAR_PURCHASE_CHATEAU]={[](){return true;},"chateau"};
    Rando::StaticData::Checks[RC_MILK_BAR_PURCHASE_CHATEAU]={RCTYPE_SHOP};
    saved[RC_MILK_BAR_PURCHASE_CHATEAU]={RI_CHATEAU_ROMANI_REFILL,false};
    mmResourceAccess.sources=0;wallet=99;
    UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times);assert(!mmResourceAccess.Has(MmMagic));
    wallet=200;UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times);assert(mmResourceAccess.Has(MmMagic));
    times[room].timeSlices=2;Rando::StaticData::Checks[check].randoCheckType=0;
    UpdateResourceSources({room,RR_CLOCK_TOWN_NORTH},times);assert(!mmResourceAccess.Has(MmNuts));
    // Scrub resources come from the assigned renewable stock, never from the
    // shop's vanilla label. One-time shelves cannot renew bottle contents.
    Regions[room].checks.clear();
    constexpr int scrub=RC_ZORA_HALL_SCRUB_POTION_REFILL;
    Rando::StaticData::Checks[scrub]={0};
    Regions[room].checks[scrub]={[](){return true;},"assigned stock"};
    saved[scrub]={RI_BLUE_POTION_REFILL,true,500};
    times[room].timeSlices=uint64_t(1)<<(productionTimeSliceCount-1);
    wallet=99;bottle=true;mmResourceAccess.sources=MmRupees;
    for(auto& event:randoEvents) event=false;
    assert(UpdateResourceSources({room},times));
    assert(RANDO_EVENTS[RE_ACCESS_BLUE_POTION_REFILL]);
    assert(mmResourceAccess.Has(MmMagic));
    assert(saved[scrub].price==500 && gCurrentRegionTime==(uint64_t(1)<<40));
    for(int item: {RI_RECOVERY_HEART,RI_UNKNOWN,RI_COMBO_FOREIGN}) {
        saved[scrub].randoItemId=item;mmResourceAccess.sources=MmRupees;
        for(auto& event:randoEvents) event=false;
        assert(!UpdateResourceSources({room},times));
        assert(!RANDO_EVENTS[RE_ACCESS_BLUE_POTION_REFILL]);
        assert(!mmResourceAccess.Has(MmMagic));
    }
    saved[scrub].randoItemId=RI_BLUE_POTION_REFILL;
    bottle=false;assert(!UpdateResourceSources({room},times));
    bottle=true;mmResourceAccess.sources=0;assert(!UpdateResourceSources({room},times));
    mmResourceAccess.sources=MmRupees;wallet=39;assert(!UpdateResourceSources({room},times));
    wallet=99;Regions[room].checks[scrub].first=[](){return false;};
    assert(!UpdateResourceSources({room},times));
    Regions[room].checks.clear();
    for(int oneTime: {RC_BOMB_SHOP_ITEM_03,RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM,RC_CURIOSITY_SHOP_SPECIAL_ITEM}) {
        Rando::StaticData::Checks[oneTime]={RCTYPE_SHOP};
        saved[oneTime]={RI_BLUE_POTION_REFILL,true,1};
        Regions[room].checks[oneTime]={[](){return true;},"one-time shelf"};
    }
    assert(!UpdateResourceSources({room},times));
    assert(!RANDO_EVENTS[RE_ACCESS_BLUE_POTION_REFILL]);
    Regions[room].checks.clear();
    Regions[room].checks[scrub]={[](){return true;},"licensed bean refill"};
    saved[scrub].randoItemId=RI_MAGIC_BEAN;
    assert(!UpdateResourceSources({room},times));
    assert(!RANDO_EVENTS[RE_ACCESS_BEANS_REFILL] && !beanLicense);
    beanLicense=true;
    assert(UpdateResourceSources({room},times));
    assert(RANDO_EVENTS[RE_ACCESS_BEANS_REFILL]);
    std::cout<<"PASS: production MM source predicates, wallets, equipment, refill purchases, soul gates, time masks, scope isolation\n";
}
'''
target=work/'mm_resource.cpp'
target.write_text(code,encoding='utf-8')
exe=work/'mm_resource.exe'
compiled=subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/W4','/WX','/UNDEBUG',str(target),
    '/Fe:'+str(exe),'/Fo:'+str(work/'mm_resource.obj')],capture_output=True,text=True)
(work/'compile.log').write_text(compiled.stdout+compiled.stderr)
if compiled.returncode: print(compiled.stdout+compiled.stderr)
compiled.check_returncode()
subprocess.run([str(exe)],check=True)
