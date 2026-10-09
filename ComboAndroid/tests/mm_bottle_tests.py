"""Exercise the production bottle predicate, including occupied Gold Dust."""
from pathlib import Path
import argparse, re, subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
a = p.parse_args()
text = (a.source/'mm/2s2h/Rando/Logic/Logic.cpp').read_text(encoding='utf-8')
match = re.search(r'bool HasUsableBottle\(\) \{.*?\n\}', text, re.S)
assert match, 'Production bottle predicate missing'
grants = (a.source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
grant = re.search(r'        // New bottles occupy.*?\n        // Dungeon items',grants,re.S)
assert grant, 'Production new-bottle grant block missing'
grant = grant.group().rsplit('// Dungeon items',1)[0]
regions=(a.source/'mm/2s2h/Rando/Logic/Regions/North.cpp').read_text()
smithyGate=re.search(r'CHECK\(RC_MOUNTAIN_VILLAGE_SMITHY_GILDED_SWORD, (.*?)\),',regions).group(1)
assert 'CAN_ACCESS(GOLD_DUST)' not in smithyGate, 'Reachable shuffled reward cannot imply owned Gold Dust'
playthrough=(a.source/'combo/rando/ComboPlaythrough.h').read_text()
collection=re.search(r'            for \(size_t i = 0; i < placements.size\(\); \+\+i\) \{\n                if \(credited\[i\] \|\| excludeIdx.count\(i\)\).*?\n            \}',playthrough,re.S).group()


code = r'''
#include <cassert>
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>
constexpr int SLOT_BOTTLE_1=0, SLOT_BOTTLE_6=5;
constexpr int ITEM_NONE=255, ITEM_BOTTLE=1, ITEM_GOLD_DUST=2;
struct { struct { struct { struct { int items[6]; } inventory; } saveInfo; } save; } gSaveContext;
bool timeSong=false, smithy=false, blacksmithSoul=true; int wallet=0;
constexpr int ITEM_POTION_RED=3;
enum { RI_BOTTLE_EMPTY,RI_BOTTLE_CHATEAU_ROMANI,RI_BOTTLE_MILK,RI_BOTTLE_GOLD_DUST,RI_BOTTLE_RED_POTION };
namespace Rando { namespace ExpandedSoul { constexpr int NPC_BLACKSMITHS=0; }
bool HaveExpandedSoul(int) { return blacksmithSoul; }
namespace StaticData {
struct Item { int itemId; } Items[]={{ITEM_BOTTLE},{4},{5},{ITEM_GOLD_DUST},{6}};
} }
#define SLOT(x) SLOT_BOTTLE_1
#define CAN_PLAY_SONG(x) timeSong
#define CAN_ACCESS(x) smithy
#define GET_CUR_UPG_VALUE(x) wallet
#define HAS_BOTTLE HasUsableBottle()
#define HAS_BOTTLE_ITEM(x) (std::find(gSaveContext.save.saveInfo.inventory.items,gSaveContext.save.saveInfo.inventory.items+6,x)!=gSaveContext.save.saveInfo.inventory.items+6)
'''+match.group()+'\nvoid Grant(int ri) { switch(ri) {\n'+grant+'\n} }\nbool Gilded() { return '+smithyGate+'; }\n'+r'''
void CollectFixed(bool accessible) {
 constexpr int GAME_OOT=0;
 struct Placement { int checkGame,itemGame; std::string check,item; };
 std::vector<Placement> placements={{1,1,"race","Gold Dust"}};
 std::vector<char> credited(1,0); std::unordered_set<size_t> excludeIdx;
 auto ootReach=std::make_shared<std::unordered_set<std::string>>();
 auto mmReach=std::make_shared<std::unordered_set<std::string>>();
 if(accessible)mmReach->insert("race");
 std::vector<std::string> ootOwned,mmOwned; bool changed=false;
''' + collection + r'''
 assert(changed==accessible);
 if(!mmOwned.empty())Grant(RI_BOTTLE_GOLD_DUST);
}
''' +r'''
int main() {
    auto& slots=gSaveContext.save.saveInfo.inventory.items;
    std::fill(slots,slots+6,ITEM_NONE);
    assert(!HasUsableBottle());
    slots[5]=ITEM_BOTTLE;
    assert(HasUsableBottle()); // A stolen/absent first slot is not decisive.
    slots[5]=ITEM_NONE; slots[0]=ITEM_GOLD_DUST;
    assert(!HasUsableBottle()); // Showing an exchange item does not empty it.
    smithy=true;
    assert(!HasUsableBottle()); // Still need capacity for the smithy fee.
    wallet=1;
    assert(HasUsableBottle());
    smithy=false;
    assert(!HasUsableBottle());
    timeSong=true;
    assert(HasUsableBottle()); // Native cycle reset clears Gold Dust.
    timeSong=false; wallet=0; slots[3]=ITEM_BOTTLE;
    assert(HasUsableBottle());
    slots[3]=3; // An ordinary consumable bottle can be emptied.
    assert(HasUsableBottle());
    std::fill(slots,slots+6,ITEM_NONE);
    smithy=true; wallet=1;
    Grant(RI_BOTTLE_EMPTY);
    assert(!Gilded()); // Race is reachable; randomized reward is not owned.
    CollectFixed(false); assert(!Gilded()); // Inaccessible fixed reward supplies nothing.
    CollectFixed(true); // Actual production fixed/placed reward collection loop and oracle grant.
    assert(Gilded()); // A collected fixed/placed reward now supplies ownership.
    blacksmithSoul=false; assert(!Gilded()); blacksmithSoul=true;
    smithy=false; assert(!Gilded()); smithy=true;
    wallet=0; assert(!Gilded()); wallet=1;
    std::fill(slots,slots+6,ITEM_NONE);
    Grant(RI_BOTTLE_EMPTY); Grant(RI_BOTTLE_GOLD_DUST);
    assert(slots[0]==ITEM_BOTTLE && slots[1]==ITEM_GOLD_DUST && HasUsableBottle());
    std::fill(slots,slots+6,ITEM_NONE);
    Grant(RI_BOTTLE_GOLD_DUST); Grant(RI_BOTTLE_EMPTY);
    assert(slots[0]==ITEM_GOLD_DUST && slots[1]==ITEM_BOTTLE && HasUsableBottle());
    Grant(RI_BOTTLE_RED_POTION); Grant(RI_BOTTLE_MILK); Grant(RI_BOTTLE_CHATEAU_ROMANI);
    assert(slots[2]==ITEM_POTION_RED && slots[3]==5 && slots[4]==4);
    Grant(RI_BOTTLE_EMPTY); Grant(RI_BOTTLE_GOLD_DUST);
    assert(slots[5]==ITEM_BOTTLE && slots[0]==ITEM_GOLD_DUST);
}
'''
a.build_dir.mkdir(parents=True,exist_ok=True)
source=a.build_dir/'bottles.cpp'; source.write_text(code)
binary=a.build_dir/'bottles.exe'
subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/W4','/WX','/UNDEBUG',str(source),
                '/Fo'+str(a.build_dir/'bottles.obj'),'/Fe'+str(binary)],check=True,cwd=a.build_dir)
subprocess.run([str(binary)],check=True)
