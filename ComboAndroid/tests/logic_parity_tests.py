"""Compile the actual price/rule/Scarecrow predicates, with negative boundaries."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--json-include',type=Path,required=True)
p.add_argument('--build-dir',type=Path,required=True)
p.add_argument('--compiler',default=hostcxx.DEFAULT_COMPILER)
a=p.parse_args(); root=a.source.resolve(); work=a.build_dir.resolve(); work.mkdir(parents=True,exist_ok=True)
def function(text, signature):
    start=text.index(signature); opening=text.index('{',start); end=opening+1; depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
def macro(text,name):
    lines=text.splitlines();start=next(i for i,l in enumerate(lines) if l.startswith('#define '+name+' '))
    end=start
    while lines[end].endswith('\\'):end+=1
    return '\n'.join(lines[start:end+1])+'\n'
oot=(root/'soh/soh/Enhancements/randomizer/logic.cpp').read_text()
location=(root/'soh/soh/Enhancements/randomizer/location_access.cpp').read_text()
mm=(root/'mm/2s2h/Rando/Logic/Logic.h').read_text()
scope=mm[mm.index('inline thread_local bool evaluatingLiveTracker'):mm.index('\n#endif',mm.index('inline thread_local bool evaluatingLiveTracker'))]
refresh=(root/'mm/2s2h/Rando/CheckTracker/CheckTracker.cpp').read_text()
refresh=function(refresh,'void RefreshChecksInLogic(')
assert 'LiveTrackerRuleScope ruleScope;' in refresh
assert refresh.index('LiveTrackerRuleScope ruleScope;') < refresh.index('while (changed)')
assert refresh.index('LiveTrackerRuleScope ruleScope;') < refresh.index('for (RandoRegionId regionId : reachableRegions)')
central=(root/'mm/2s2h/Rando/Logic/Regions/Central.cpp').read_text()
assert central.count('EVENT(RE_LEARN_SCARECROW_SONG, HAS_PLAYABLE_OCARINA && canPlaySong(OCARINA_SONG_SCARECROW_SPAWN))')==2
wallet_hook=(root/'mm/2s2h/Rando/MiscBehavior/TycoonWallet.cpp').read_text()
assert 'RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] == RO_GENERIC_YES' in wallet_hook
assert 'gUpgradeCapacities[UPG_WALLET][3] = 5000;' in wallet_hook

tracker=(root/'combo/menu/ComboCheckTrackerOOT.inc').read_text()
tracker_refresh=function(tracker,'static void UpdateComboAvailabilitySettings(')

code=r'''
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include "rando/GameplayRules.h"
#include "rando/MmResourceAccess.h"
std::map<std::string,int> testCVars;
int CVarGetInteger(const char* key,int fallback) {
    auto it=testCVars.find(key);return it==testCVars.end()?fallback:it->second;
}
enum {RG_BOTTLE_WITH_BLUE_FIRE,RG_ICE_ARROWS,RG_LIGHT_ARROWS,RSK_BLUE_FIRE_ARROWS,RSK_SUNLIGHT_ARROWS};
struct ArrowOption {int value;int Get() const {return value;}};
struct ArrowContext {int blue=0,sun=0;ArrowOption GetOption(int key) {return {key==RSK_BLUE_FIRE_ARROWS?blue:sun};}};
namespace Rando { struct Logic {
    bool CalculatingAvailableChecks=false;int GameplayRuleValue(const char*) const;
    ArrowContext* ctx=nullptr;bool bottle=false,usableArrows=false;
    bool CanUse(int item) const {return item==RG_BOTTLE_WITH_BLUE_FIRE?bottle:usableArrows;}
    bool BlueFire();bool SunlightArrows();
}; }
#define COMBO_BUILD 1
'''+function(oot,'int Logic::GameplayRuleValue(').replace('int Logic::','int Rando::Logic::')+'\n'+\
function(oot,'bool Logic::BlueFire(').replace('bool Logic::','bool Rando::Logic::')+'\n'+\
function(oot,'bool Logic::SunlightArrows(').replace('bool Logic::','bool Rando::Logic::')+r'''
namespace RandoMM {
'''+scope+r'''
}
// Bind only the macro's external game state; the tested condition is extracted
// unchanged from the production header above, never reimplemented in the test.
enum { ITEM_OCARINA_OF_TIME,ITEM_HOOKSHOT, OCARINA_SONG_SCARECROW_SPAWN,RE_LEARN_SCARECROW_SONG,
       WEEKEVENTREG_79_08,UPG_WALLET,RO_SHUFFLE_TYCOON_WALLET,RO_GENERIC_YES };
bool instrument=true,hookshot=true,playable=true,learned=false;
int events[16]{},options[16]{},wallet=0;
struct Check {int price=0;} checks[1];
#define HAS_ITEM(item) ((item)==ITEM_OCARINA_OF_TIME?instrument:hookshot)
#define HAS_PLAYABLE_OCARINA instrument
#define CHECK_WEEKEVENTREG(flag) learned
#define RANDO_EVENTS events
#define RANDO_SAVE_CHECKS checks
#define RANDO_SAVE_OPTIONS options
#define CUR_UPG_VALUE(flag) wallet
#define COMBO_LOGIC_RULE(name) RandoMM::GameplayRuleValue(name)
bool canPlaySong(int) {return playable;}
'''+macro(mm,'CAN_HOOK_SCARECROW')+macro(mm,'MM_WALLET_CAPACITY')+macro(mm,'CAN_PAY(price)')+macro(mm,'CAN_AFFORD(rc)')+r'''
enum {RCTYPE_SHOP,RCTYPE_SCRUB,RCTYPE_MERCHANT};
enum {RO_PRICE_VANILLA,RO_PRICE_CHEAP_BALANCED,RO_PRICE_BALANCED,RO_PRICE_FIXED,RO_PRICE_RANGE,RO_PRICE_SET_BY_WALLET};
struct PriceSettingsStruct {int main,fixedPrice,range1,range2,noWallet,childWallet,adultWallet,giantWallet;};
PriceSettingsStruct shopsanityPrices{0,1,2,3,4,5,6,7},scrubPrices{8,9,10,11,12,13,14,15},merchantPrices{16,17,18,19,20,21,22,23};
int settings[24]{};
namespace Rando {struct Location {int type;int GetRCType() const {return type;} uint16_t GetVanillaPrice() const {return 40;}};
struct Option {int key;uint8_t Get() const {return static_cast<uint8_t>(settings[key]);}};
struct Context {static Context* GetInstance() {static Context c;return &c;} Option GetOption(int key) {return {key};}};}
'''+function(location,'static uint16_t GetMinimumPrice(')+r'''
enum {SLOT_BOW,SLOT_SLINGSHOT,SLOT_BOMB,SLOT_BOMBCHU};
struct TrackerSave {struct {int ammo[4]{};} inventory;int magic=0,rupees=0;} gSaveContext;
bool enableAvailableChecks=false;int refreshCount=0;
void RecalculateAvailableChecks() {++refreshCount;}
'''+tracker_refresh+r'''
int main() {
    testCVars.clear();UpdateComboAvailabilitySettings(false);assert(refreshCount==0);
    UpdateComboAvailabilitySettings(true);assert(refreshCount==1);
    UpdateComboAvailabilitySettings(true);assert(refreshCount==1);
    for(int slot=0;slot<4;++slot) {
        const int prior=refreshCount;gSaveContext.inventory.ammo[slot]=1;
        UpdateComboAvailabilitySettings(true);assert(refreshCount==prior+1);
        gSaveContext.inventory.ammo[slot]=10;UpdateComboAvailabilitySettings(true);assert(refreshCount==prior+1);
        gSaveContext.inventory.ammo[slot]=0;UpdateComboAvailabilitySettings(true);assert(refreshCount==prior+2);
    }
    int prior=refreshCount;gSaveContext.magic=48;UpdateComboAvailabilitySettings(true);assert(refreshCount==prior+1);
    gSaveContext.magic=24;UpdateComboAvailabilitySettings(true);assert(refreshCount==prior+1);
    gSaveContext.magic=0;UpdateComboAvailabilitySettings(true);assert(refreshCount==prior+2);
    prior=refreshCount;gSaveContext.rupees=19;UpdateComboAvailabilitySettings(true);assert(refreshCount==prior);
    gSaveContext.rupees=20;UpdateComboAvailabilitySettings(true);assert(refreshCount==prior);
    UpdateComboAvailabilitySettings(false);gSaveContext.rupees=0;UpdateComboAvailabilitySettings(false);
    assert(refreshCount==prior);UpdateComboAvailabilitySettings(true);assert(refreshCount==prior+1);

    Rando::Logic logic;
    ArrowContext arrowContext;logic.ctx=&arrowContext;
    // Native actor hooks accept seed flags OR runtime enhancements. Do not
    // remove a seeded ability from the tracker when only its runtime toggle is off.
    for(int tracker=0;tracker<2;++tracker) for(int native=0;native<2;++native)
      for(int runtime=0;runtime<2;++runtime) for(int gear=0;gear<2;++gear) for(int bottle=0;bottle<2;++bottle) {
        logic.CalculatingAvailableChecks=tracker;
        arrowContext.blue=arrowContext.sun=native;
        testCVars["gEnhancements.BlueFireArrows"]=runtime;testCVars["gEnhancements.SunlightArrows"]=runtime;
        logic.usableArrows=gear;logic.bottle=bottle;
        const bool arrows=gear&&(native||(tracker&&runtime));
        assert(logic.BlueFire()==(bottle||arrows));assert(logic.SunlightArrows()==arrows);
    }
    logic.CalculatingAvailableChecks=false;
    for(const auto& rule:ComboRando::kGameplayRules) {
        testCVars.clear();testCVars[rule.name]=rule.maximum;
        // Runtime ability alone must never strengthen generation.
        assert(logic.GameplayRuleValue(rule.name)==0);
        logic.CalculatingAvailableChecks=true;
        assert(logic.GameplayRuleValue(rule.name)==rule.maximum);
        logic.CalculatingAvailableChecks=false;
        if(!rule.runtimeOnly) {
            testCVars[ComboRando::LogicRuleKey(rule.name)]=rule.maximum;
            assert(logic.GameplayRuleValue(rule.name)==rule.maximum);
        }
        assert(!RandoMM::evaluatingLiveTracker);
        {
            RandoMM::LiveTrackerRuleScope live;
            assert(RandoMM::GameplayRuleValue(rule.name)==rule.maximum);
            {RandoMM::LiveTrackerRuleScope nested;assert(RandoMM::evaluatingLiveTracker);}
            assert(RandoMM::evaluatingLiveTracker);
        }
        assert(!RandoMM::evaluatingLiveTracker);
        assert(RandoMM::GameplayRuleValue(rule.name)==(rule.runtimeOnly?0:logic.GameplayRuleValue(rule.name)));
    }
    // The actual native Scarecrow actor needs a taught-this-cycle event.
    // A reachable teacher, an already taught save, or the paired free rule
    // may satisfy it, but each missing instrument/hookshot/note still blocks.
    for(int source=0;source<5;++source) for(int missing=0;missing<4;++missing) {
        testCVars.clear();events[RE_LEARN_SCARECROW_SONG]=source==1;learned=source==2;
        if(source==3) testCVars[ComboRando::LogicRuleKey("gEnhancements.Playback.SkipScarecrowSong")]=1;
        if(source==4) testCVars["gEnhancements.Playback.SkipScarecrowSong"]=1;
        instrument=missing!=1;hookshot=missing!=2;playable=missing!=3;
        assert(bool(CAN_HOOK_SCARECROW)==(missing==0&&source>=1&&source<=3));
        {RandoMM::LiveTrackerRuleScope live;
            assert(bool(CAN_HOOK_SCARECROW)==(missing==0&&(source==1||source==2||source==4)));}
    }
    // Check every price around every actual MM wallet limit, including zero.
    const int capacities[]={99,200,500,5000};
    for(int tycoon=0;tycoon<2;++tycoon) for(wallet=0;wallet<4;++wallet) for(int price=0;price<=5100;++price) {
        options[RO_SHUFFLE_TYCOON_WALLET]=tycoon?RO_GENERIC_YES:0;
        const int capacity=wallet==3&&!tycoon?500:capacities[wallet];
        checks[0].price=price;assert(bool(CAN_AFFORD(0))==(price<=capacity));
    }
    // The production estimate must use both independent endpoints for each
    // price family, including reversed ranges entered on a saved seed.
    const PriceSettingsStruct families[]={shopsanityPrices,scrubPrices,merchantPrices};
    for(int type=0;type<3;++type) {
        const auto s=families[type];Rando::Location loc{type};
        settings[s.main]=RO_PRICE_RANGE;
        settings[s.range1]=80;settings[s.range2]=4;assert(GetMinimumPrice(&loc)==20);
        settings[s.range1]=4;settings[s.range2]=80;assert(GetMinimumPrice(&loc)==20);
        settings[s.range1]=0;settings[s.range2]=80;assert(GetMinimumPrice(&loc)==0);
        settings[s.main]=RO_PRICE_FIXED;settings[s.fixedPrice]=30;assert(GetMinimumPrice(&loc)==150);
        settings[s.main]=RO_PRICE_VANILLA;assert(GetMinimumPrice(&loc)==40);
    }
    std::cout<<"PASS: actual rule readers/scoped tracker context, 40 Scarecrow negatives/positives, 40808 native wallet boundaries and all three price families\n";
}
'''
test=work/'logic_parity.cpp';test.write_text(code)
exe=hostcxx.build(a.compiler,test,werror=True,includes=[root/'combo',a.json_include])
run=subprocess.run([str(exe)],capture_output=True,text=True, timeout=hostcxx.RUN_TIMEOUT)
(work/'result.txt').write_text(run.stdout+run.stderr);print(run.stdout+run.stderr,end='');run.check_returncode()
