"""Execute production tracker invalidation for transitions and active age rules."""
from pathlib import Path
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--compiler', default='cl')
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
native = (source/'soh/soh/Enhancements/randomizer/randomizer_check_tracker.cpp').read_text(encoding='utf-8')
snapshot = (source/'combo/menu/ComboCheckTrackerOOT.inc').read_text(encoding='utf-8')

def function(signature, text=native):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

prelude = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "soh/Enhancements/randomizer/randomizerEnums.h"
#include "rando/GameplayRules.h"
enum SceneID { SCENE_KOKIRI_SHOP, SCENE_BAZAAR, SCENE_POTION_SHOP_MARKET,
    SCENE_BOMBCHU_SHOP, SCENE_POTION_SHOP_KAKARIKO, SCENE_GORON_SHOP, SCENE_ZORA_SHOP,
    SCENE_WATER_TEMPLE_TEST };
bool loaded=true;
namespace GameInteractor { bool IsSaveLoaded() { return loaded; } }
bool doAreaScroll=false, recalculateAvailable=false;
bool enableAvailableChecks=false;
enum {SLOT_BOW,SLOT_SLINGSHOT,SLOT_BOMB,SLOT_BOMBCHU};
struct TrackerSave {struct {int ammo[4]{};} inventory;int magic=0,rupees=0;} gSaveContext;
int runtimeTimeTravelMode=0;
int runtimeTunic=0;
int CVarGetInteger(const char* key, int fallback) {
    if(std::string(key)=="gEnhancements.TimeTravel") return runtimeTimeTravelMode;
    if(std::string(key)=="gCheats.SuperTunic") return runtimeTunic;
    return fallback;
}
RandomizerRegion availableChecksStartingRegion=RR_ROOT;
RandoAgeTime availableChecksStartingAgeTime=RAT_NONE;
RandomizerCheckArea currentArea=RCAREA_KOKIRI_FOREST, previousArea=RCAREA_KOKIRI_FOREST;
RandomizerCheckArea destination=RCAREA_WATER_TEMPLE;
std::set<RandomizerCheckArea> spoiled;
int shopCalls=0, spoilCalls=0;
uint32_t lastShop=0;
struct PlayState { int16_t nextEntranceIndex=16; } play;
PlayState* gPlayState=&play;
std::vector<int16_t> spoilingEntrances={16};
RandomizerCheckArea GetCheckArea() { return destination; }
bool IsAreaSpoiled(RandomizerCheckArea area) { return spoiled.count(area)!=0; }
namespace RandomizerCheckObjects {
    bool AreaIsOverworld(RandomizerCheckArea area) { return area==RCAREA_KOKIRI_FOREST; }
}
void SetShopSeen(uint32_t scene, bool save) { assert(!save); ++shopCalls; lastShop=scene; }
void SetAreaSpoiled(RandomizerCheckArea area) { spoiled.insert(area); ++spoilCalls; }
void RecalculateAvailableChecks(RandomizerRegion startingRegion=RR_ROOT,
                                RandoAgeTime startingAgeTime=RAT_NONE);
'''

tests = r'''
void Reset(bool alreadySpoiled=true) {
    loaded=true;doAreaScroll=false;recalculateAvailable=false;
    // A transition must discard an old explicit search origin and age. The
    // deferred search will derive both from the new live entrance/save context.
    availableChecksStartingRegion=RR_KOKIRI_FOREST;
    availableChecksStartingAgeTime=RAT_CHILD_DAY;
    currentArea=RCAREA_KOKIRI_FOREST;previousArea=RCAREA_KOKIRI_FOREST;
    destination=RCAREA_WATER_TEMPLE;play.nextEntranceIndex=16;
    spoiled.clear();if(alreadySpoiled) spoiled.insert(destination);
    shopCalls=spoilCalls=0;
}
void ExpectFreshSearch() {
    assert(recalculateAvailable);
    assert(availableChecksStartingRegion==RR_ROOT);
    assert(availableChecksStartingAgeTime==RAT_NONE);
    assert(doAreaScroll && currentArea==destination);
}
int main() {
    // A runtime ability change must request a fresh search even without a
    // transition. Unchanged snapshots must reuse the cached result.
    Reset();UpdateComboAvailabilitySettings(true);
    assert(enableAvailableChecks && recalculateAvailable);
    assert(availableChecksStartingRegion==RR_ROOT && availableChecksStartingAgeTime==RAT_NONE);
    recalculateAvailable=false;UpdateComboAvailabilitySettings(true);
    assert(!recalculateAvailable);
    for(int mode=1;mode<=4;++mode) {
        runtimeTimeTravelMode=mode;UpdateComboAvailabilitySettings(true);
        assert(recalculateAvailable);
        recalculateAvailable=false;UpdateComboAvailabilitySettings(true);
        assert(!recalculateAvailable);
    }
    runtimeTimeTravelMode=0;UpdateComboAvailabilitySettings(true);
    assert(recalculateAvailable);
    recalculateAvailable=false;runtimeTunic=1;UpdateComboAvailabilitySettings(true);
    assert(recalculateAvailable);
    recalculateAvailable=false;UpdateComboAvailabilitySettings(true);assert(!recalculateAvailable);
    recalculateAvailable=false;UpdateComboAvailabilitySettings(false);
    runtimeTimeTravelMode=3;UpdateComboAvailabilitySettings(false);
    assert(!enableAvailableChecks && !recalculateAvailable);
    UpdateComboAvailabilitySettings(true);
    assert(enableAvailableChecks && recalculateAvailable);

    Reset();loaded=false;
    CheckTrackerTransition(SCENE_WATER_TEMPLE_TEST);
    assert(!recalculateAvailable && !doAreaScroll && currentArea==RCAREA_KOKIRI_FOREST);
    assert(shopCalls==0 && spoilCalls==0);

    // An already spoiled shuffled destination still changes the search origin.
    Reset();CheckTrackerTransition(SCENE_WATER_TEMPLE_TEST);ExpectFreshSearch();
    assert(previousArea==RCAREA_KOKIRI_FOREST && spoilCalls==0);

    // SwitchAge reloads the same area, so area equality must not suppress this.
    Reset();currentArea=destination;
    CheckTrackerTransition(SCENE_WATER_TEMPLE_TEST);ExpectFreshSearch();
    assert(previousArea==destination && spoilCalls==0);
    recalculateAvailable=false;
    CheckTrackerTransition(SCENE_WATER_TEMPLE_TEST);ExpectFreshSearch();

    Reset(false);CheckTrackerTransition(SCENE_WATER_TEMPLE_TEST);ExpectFreshSearch();
    assert(spoilCalls==1 && IsAreaSpoiled(destination));
    Reset(false);play.nextEntranceIndex=17;
    CheckTrackerTransition(SCENE_WATER_TEMPLE_TEST);ExpectFreshSearch();assert(spoilCalls==0);
    Reset(false);destination=RCAREA_KOKIRI_FOREST;play.nextEntranceIndex=17;
    CheckTrackerTransition(SCENE_WATER_TEMPLE_TEST);ExpectFreshSearch();assert(spoilCalls==1);

    for(uint32_t scene=SCENE_KOKIRI_SHOP;scene<=SCENE_ZORA_SHOP;++scene) {
        Reset();CheckTrackerTransition(scene);ExpectFreshSearch();
        assert(shopCalls==1 && lastShop==scene && spoilCalls==0);
    }
    std::cout<<"PASS: active age-rule changes, unchanged snapshot cache, tracker re-enable, unloaded guard, shuffled dungeon entry, same-area age changes, repeated transitions, area spoils and seven shops\n";
}
'''

test = work/'tracker_transition.cpp'
test.write_text(prelude + function('void RecalculateAvailableChecks(RandomizerRegion startingRegion') + '\n' +
                function('static void UpdateComboAvailabilitySettings(bool wantLogic)', snapshot) + '\n' +
                function('void CheckTrackerTransition(uint32_t sceneNum)') + '\n' + tests, encoding='utf-8')
exe = work/'tracker_transition.exe'
compiled = subprocess.run([args.compiler, '/nologo', '/EHsc', '/std:c++20', '/Zc:preprocessor', '/W4', '/WX', '/UNDEBUG',
    '/I'+str(source/'soh'), '/I'+str(source/'combo'), '/I'+str(args.json_include.resolve()),
    str(test), '/Fe:'+str(exe), '/Fo:'+str(work/'tracker_transition.obj')],
    capture_output=True, text=True)
(work/'compile.log').write_text(compiled.stdout+compiled.stderr, encoding='utf-8')
if compiled.returncode:
    print('\n'.join((compiled.stdout+compiled.stderr).splitlines()[:20]))
compiled.check_returncode()
run = subprocess.run([str(exe)], capture_output=True, text=True)
(work/'result.txt').write_text(run.stdout+run.stderr, encoding='utf-8')
print(run.stdout+run.stderr, end='')
run.check_returncode()
