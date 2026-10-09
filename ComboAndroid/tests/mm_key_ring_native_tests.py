"""Compile production MM ring grant dispatch and dungeon restrictions verbatim.
Only engine storage and scene/check metadata are supplied by the harness.
"""
from pathlib import Path
import argparse, subprocess
p=argparse.ArgumentParser()
p.add_argument('--source',type=Path,required=True)
p.add_argument('--build-dir',type=Path,required=True)
p.add_argument('--compiler',default='cl')
a=p.parse_args(); source=a.source.resolve(); work=a.build_dir.resolve();work.mkdir(parents=True,exist_ok=True)
def function(text,signature):
    start=text.index(signature); begin=text.index('{',start); end=begin+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]+'\n'
r=source/'mm/2s2h';header=(r/'Rando/Rando.h').read_text(encoding='utf-8')
give=(r/'Rando/GiveItem.cpp').read_text(encoding='utf-8');oracle=(r/'BenPort.cpp').read_text(encoding='utf-8')
def ring_cases(text):
    return text[text.index('        case RI_WOODFALL_KEY_RING:'):text.index('        case RI_WOODFALL_SMALL_KEY:',text.index('        case RI_WOODFALL_KEY_RING:'))]
placement=(r/'Rando/Logic/PlacementConstraints.cpp').read_text(encoding='utf-8')
starting=(r/'Rando/StartingItems.cpp').read_text(encoding='utf-8')
pools=(r/'Rando/Logic/GeneratePools.cpp').read_text(encoding='utf-8')
starting_keys=function(starting,'    if (randoSaveInfo.randoSaveOptions[RO_PLACEMENT_SMALL_KEYS]')
pool_tail=pools[pools.index('    ReplaceKeyRingPool('):pools.index('    // Traps')]
code=r'''#include "Rando/KeyRings.h"
#include "Rando/Ocarinas.h"
#include <cassert>
#include <map>
#include <cstring>
#include <iostream>
using s8 = int8_t;
struct State {
    struct Save {
        struct Info { struct Inventory { s8 dungeonKeys[10]; } inventory; } saveInfo;
        struct Ship { struct Rando { s8 foundDungeonKeys[10]; unsigned randoSaveOptions[RO_MAX]; } rando; } shipSaveInfo;
    } save;
} gSaveContext;
#define DUNGEON_KEY_COUNT(i) gSaveContext.save.saveInfo.inventory.dungeonKeys[i]
#define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.randoSaveOptions
using RandoSaveInfo = State::Save::Ship::Rando;
enum { ITEM_TINGLE_MAP = 100, ITEM_DUNGEON_MAP, ITEM_COMPASS };
using SceneId = int;
enum {SCENE_MITURIN,SCENE_MITURIN_BS,SCENE_HAKUGIN,SCENE_HAKUGIN_BS,SCENE_SEA,SCENE_SEA_BS,SCENE_INISIE_N,SCENE_INISIE_R,SCENE_INISIE_BS};
enum {DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE,DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE,DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE,DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE};
namespace Rando {
namespace StaticData {
struct Item { RandoItemType randoItemType; int itemId = 0; };
struct Check { SceneId sceneId; };
std::map<RandoItemId,Item> Items;
std::map<RandoCheckId,Check> Checks;
}
'''+function(header,'inline bool GiveKeyRing(')+'''
std::vector<RandoItemId> ComputedStartingKeys(RandoSaveInfo& randoSaveInfo) {
    std::vector<RandoItemId> startingItems;
'''+starting_keys+'''
    return startingItems;
}
void NativePoolTail(RandoSaveInfo& saveInfo, std::vector<RandoItemId> startingItems, std::vector<RandoItemId>& itemPool) {
'''+pool_tail+'''
}
}
void RuntimeRing(RandoItemId randoItemId) { switch (randoItemId) {
'''+ring_cases(give)+'''default: assert(false); }}
void OracleRing(RandoItemId ri) { switch (ri) {
'''+ring_cases(oracle)+'''default: assert(false); }}
namespace Rando { namespace Logic {
'''+''.join(function(placement,s) for s in ['int SceneIdToDungeon(','int RandoItemIdToDungeon(','bool IsItemAllowedAtCheck('])+'''}}
int main() {
    for (const auto& ring : Rando::keyRings) {
        std::memset(&gSaveContext,0,sizeof(gSaveContext));
        std::memset(gSaveContext.save.saveInfo.inventory.dungeonKeys,-1,10);
        std::memset(gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys,-1,10);
        RuntimeRing(ring.ring); State runtime = gSaveContext;
        std::memset(gSaveContext.save.saveInfo.inventory.dungeonKeys,-1,10);
        std::memset(gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys,-1,10);
        OracleRing(ring.ring);
        assert(std::memcmp(&runtime,&gSaveContext,sizeof(State)) == 0);
        DUNGEON_KEY_COUNT(ring.dungeon)=0;
        RuntimeRing(ring.ring); OracleRing(ring.ring);
        assert(DUNGEON_KEY_COUNT(ring.dungeon)==0);
        Rando::StaticData::Items[ring.ring]={RITYPE_SMALL_KEY};
        RANDO_SAVE_OPTIONS[RO_PLACEMENT_SMALL_KEYS]=RO_DUNGEON_ITEM_ANYWHERE;
        assert(Rando::Logic::RandoItemIdToDungeon(ring.ring)==-1);
        RANDO_SAVE_OPTIONS[RO_PLACEMENT_SMALL_KEYS]=RO_DUNGEON_ITEM_OWN_DUNGEON;
        assert(Rando::Logic::RandoItemIdToDungeon(ring.ring)==ring.dungeon);
        const int scenes[] = {SCENE_MITURIN,SCENE_HAKUGIN,SCENE_SEA,SCENE_INISIE_R};
        for(int i=0;i<4;++i) {
            Rando::StaticData::Checks[RC_UNKNOWN]={scenes[i]};
            assert(Rando::Logic::IsItemAllowedAtCheck(ring.ring,RC_UNKNOWN)==(ring.dungeon==i));
        }
    }
    gSaveContext.save.shipSaveInfo.rando.foundDungeonKeys[1]=2; DUNGEON_KEY_COUNT(1)=0;
    RuntimeRing(RI_SNOWHEAD_KEY_RING); assert(DUNGEON_KEY_COUNT(1)==1);
    DUNGEON_KEY_COUNT(1)=0; OracleRing(RI_SNOWHEAD_KEY_RING); assert(DUNGEON_KEY_COUNT(1)==0);
    auto& save = gSaveContext.save.shipSaveInfo.rando;
    for(const auto& ring : Rando::keyRings) save.randoSaveOptions[ring.option]=RO_GENERIC_YES;
    save.randoSaveOptions[RO_STARTING_HEALTH]=3;
    save.randoSaveOptions[RO_PLACEMENT_SMALL_KEYS]=RO_DUNGEON_ITEM_START_WITH;
    auto starting=Rando::ComputedStartingKeys(save);
    assert(starting.size()==4);
    for(const auto& ring : Rando::keyRings) assert(std::count(starting.begin(),starting.end(),ring.ring)==1);
    auto vanillaPool=[](){std::vector<RandoItemId> p; for(const auto& ring : Rando::keyRings) p.insert(p.end(),ring.maximum,ring.key);return p;};
    auto pool=vanillaPool();
    Rando::NativePoolTail(save,starting,pool); assert(pool.empty());
    save.randoSaveOptions[RO_PLACEMENT_SMALL_KEYS]=RO_DUNGEON_ITEM_ANYWHERE;
    assert(Rando::ComputedStartingKeys(save).empty());
    pool=vanillaPool(); Rando::NativePoolTail(save,{},pool); assert(pool.size()==4);
    save.randoSaveOptions[RO_PLENTIFUL_ITEMS]=RO_GENERIC_YES;
    pool=vanillaPool(); Rando::NativePoolTail(save,{},pool); assert(pool.size()==8);
    for(const auto& ring : Rando::keyRings) assert(std::count(pool.begin(),pool.end(),ring.ring)==2);
    save.randoSaveOptions[RO_PLACEMENT_SMALL_KEYS]=RO_DUNGEON_ITEM_START_WITH;
    for(const auto& ring : Rando::keyRings) save.randoSaveOptions[ring.option]=RO_GENERIC_NO;
    auto originalStarting=Rando::ComputedStartingKeys(save);assert(originalStarting.size()==9);
    std::cout << "Native runtime/oracle ring dispatch and own-dungeon restrictions passed.\\n";
}
'''
f=work/'mm-key-ring-native.cpp';f.write_text(code,encoding='utf-8');exe=work/'mm-key-ring-native.exe'
if Path(a.compiler).stem.lower() in ('cl','clang-cl'):
    cmd=[a.compiler,'/nologo','/std:c++20','/EHsc','/W4','/UNDEBUG','/I'+str(r),str(f),'/Fe:'+str(exe),'/Fo:'+str(work/'mm-key-ring-native.obj')]
else:
    cmd=[a.compiler,'-std=c++20','-Wall','-Wextra','-UNDEBUG','-I'+str(r),str(f),'-o',str(exe)]
run=subprocess.run(cmd,capture_output=True,text=True);(work/'compile.log').write_text(run.stdout+run.stderr,encoding='utf-8')
if run.returncode: print(run.stdout+run.stderr)
run.check_returncode();subprocess.run([str(exe)],check=True)
