"""Run production OoT pool/filter/oracle candidate code on all 64 shop slots.

Engine dependencies are stubbed; this checks eligibility and candidate routing,
not an entire generated seed or physical gameplay.
"""
import argparse
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--before', type=Path)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default='cl')
a = p.parse_args()
source, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
base = source / 'soh/soh/Enhancements/randomizer'

def extract(text, signature):
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'

tracker_path = Path('soh/soh/Enhancements/randomizer/randomizer_check_tracker.cpp')
tracker = (a.before / tracker_path if a.before else source / tracker_path).read_text(encoding='utf-8')
shuffled = extract(tracker, 'bool IsCheckShuffled(')
visible = extract(tracker, 'bool IsVisibleInCheckTracker(')
pool = extract((base/'SeedContext.cpp').read_text(encoding='utf-8'), 'void Context::GenerateLocationPool(')
oracle = extract((source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8'),
                 'extern "C" COMBO_EXPORT const char* Combo_SOH_Rando_GetReachableChecks(')
load_shop = re.search(r'    showShops =\s*.*?;', extract(tracker, 'void LoadSettings() {'), re.S).group()
slots = re.findall(r'locationTable\[(RC_\w+)\]\s*=\s*Location::Base\([^\n]*?RCTYPE_SHOP,',
                   (base/'location_list.cpp').read_text(encoding='utf-8'))
assert len(slots) == 64 and len(set(slots)) == 64, slots

symbols = sorted(set(re.findall(r'\b(?:RC_|RCQUEST_|RCTYPE_|RCAREA_|RSK_|RO_|SCENE_|RR_|RG_)[A-Z0-9_]+',
                                 shuffled + visible + pool + oracle + load_shop + ' '.join(slots) + ' RO_GENERIC_ON')))
flags = sorted(set(re.findall(r'\b(?:show[A-Z0-9]\w*|alwaysShowGS|hideShopUnshuffledChecks|fortressNormal|fortressFast)\b',
                             shuffled + visible)))
code = r'''
#define COMBO_BUILD 1
#define COMBO_EXPORT
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>
using RandomizerCheck=int;
enum {
'''
code += ','.join(f'{symbol}={i+1}' for i,symbol in enumerate(symbols)) + '};\n'
code += 'bool ' + ','.join(flag+'=true' for flag in flags) + ';\n'
code += r'''
bool randomizerEnabled=true;
#define IS_RANDO randomizerEnabled
struct { struct { struct { int id=0; } quest; } ship; } gSaveContext;
namespace Rando {
struct Location {
 int check=0,type=RCTYPE_SHOP,area=0,scene=0,params=0,quest=RCQUEST_BOTH;
 std::string name;
 int GetRandomizerCheck()const{return check;} int GetRCType()const{return type;}
 int GetArea()const{return area;} int GetScene()const{return scene;}
 int GetActorParams()const{return params;} int GetQuest()const{return quest;}
 const std::string& GetName()const{return name;}
 bool IsOverworld()const{return true;} bool IsDungeon()const{return false;}
 bool IsVanillaCompletion()const{return false;} bool IsDungeonForCollectibleShuffle()const{return false;}
};
namespace StaticData {
 std::vector<Location> table;
 Location* GetLocation(int rc){return &*std::find_if(table.begin(),table.end(),[&](auto& x){return x.check==rc;});}
 auto& GetLocationTable(){return table;}
}
struct Option {
 int value=0; int Get()const{return value;}
 bool Is(int n)const{return value==n;} bool IsNot(int n)const{return value!=n;}
 explicit operator bool()const{return value!=0;}
};
struct ItemLocation {bool excluded=false;bool IsExcluded()const{return excluded;}};
struct Dungeon {std::vector<int> locations;bool IsMQ()const{return false;}};
struct Dungeons {
 Dungeon dungeon;
 std::vector<Dungeon*> GetDungeonList(){return {&dungeon};}
};
struct Fishsanity {bool GetFishLocationIncluded(Location*)const{return true;}};
struct Context {
 std::map<int,Option> mOptions; std::map<int,ItemLocation> items;
 std::vector<int> allLocations,overworldLocations;
 std::unordered_set<int> inactive;
 int mGBKCondition=0,mGanonsSoulCondition=0;
 Fishsanity fish;Fishsanity* mFishsanity=&fish;Dungeons dungeons;
 static Context* GetInstance(){static Context instance;return &instance;}
 Option& GetOption(int key){return mOptions[key];}
 Dungeons* GetDungeons(){return &dungeons;} Dungeon* GetDungeon(int){return &dungeons.dungeon;}
 Fishsanity* GetFishsanity(){return mFishsanity;}
 ItemLocation* GetItemLocation(int rc){return &items[rc];}
 bool IsLocationInCurrentCatalog(int rc)const{return !inactive.contains(rc);}
 bool IsQuestOfLocationActive(int rc)const{return IsLocationInCurrentCatalog(rc);}
 void AddLocation(int rc,std::vector<int>* target=nullptr){(target?*target:allLocations).push_back(rc);}
 void GenerateLocationPool();
};
auto ctx=Context::GetInstance();
namespace SilverRupees {bool IsCheck(int){return false;}bool IsShuffledCheck(int){return true;}}
}
struct MockRandomizer {
 struct ShopIdentity {int enGirlAShopItem=0;};
 std::map<int,ShopIdentity> identities;
 ShopIdentity IdentifyShopItem(int scene,int slot){return identities[scene*8+slot-1];}
 int GetRandoSettingValue(int key){return Rando::ctx->GetOption(key).Get();}
};
struct OTRGlobals {
 MockRandomizer* gRandomizer=nullptr;Rando::Context* gRandoContext=nullptr;
 static OTRGlobals* Instance;
};
MockRandomizer mockRandomizer;OTRGlobals globals{&mockRandomizer,Rando::ctx};OTRGlobals* OTRGlobals::Instance=&globals;
namespace RandomizerCheckObjects {bool AreaIsDungeon(int){return false;}bool AreaIsOverworld(int){return true;}}
namespace ComboRando {bool& OotSongOracle(){static bool value=false;return value;}}
struct Region {bool Child(){return true;}bool Adult(){return true;}} region;
Region* RegionTable(int){return &region;}
bool sComboPortalOpen=false;
std::vector<int> searched;
std::vector<int> ReachabilitySearch(const std::vector<int>& candidates){
 searched=candidates;auto result=candidates;
 std::erase_if(result,[](int rc){return std::none_of(Rando::StaticData::table.begin(),Rando::StaticData::table.end(),[&](auto& loc){return loc.check==rc;});});
 return result;
}
void Require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
'''
code += 'namespace Rando {\n' + pool + '}\n' + shuffled + visible + oracle
code += 'void LoadShopSetting(){\n' + load_shop + '\n}\n'
code += 'const std::vector<int> shopChecks={' + ','.join(slots) + '};\n'
code += r'''
void Run() {
 auto ctx=Rando::ctx;
 for(size_t i=0;i<shopChecks.size();++i) {
  Rando::StaticData::table.push_back({shopChecks[i],RCTYPE_SHOP,0,int(i/8),int(i%8),RCQUEST_BOTH,"shop "+std::to_string(i)});
 }
 ctx->GetOption(RSK_WINCON).value=RO_WINCON_ANYWHERE;
 ctx->GetOption(RSK_SHUFFLE_MASTER_SWORD).value=RO_GENERIC_ON;
 // Off, refill-only, two one-time rewards (screenshot), and eight one-time rewards.
 for(int oneTimeCount:{-1,0,2,8}) {
  ctx->GetOption(RSK_SHOPSANITY).value=oneTimeCount<0?RO_SHOPSANITY_OFF:RO_SHOPSANITY_OFF+1;
  LoadShopSetting();
  ctx->GenerateLocationPool();
  Require(ctx->allLocations==shopChecks,"generation pool omitted a shop slot");
  for(size_t i=0;i<shopChecks.size();++i) {
   // Native stock has real EnGirlA IDs; unique rewards have SI_RANDOMIZED_ITEM (50).
   mockRandomizer.identities[int(i)]={int(i%8)<oneTimeCount?50:int(i%8)};
  }
  for(bool hideNativeStock:{false,true}) {
   hideShopUnshuffledChecks=hideNativeStock;
   unsigned shown=0;
   for(int rc:shopChecks) shown+=IsVisibleInCheckTracker(rc);
   Require(shown==(oneTimeCount<0?0u:64u),"tracker omitted shuffled refill stock");
  }
  auto result=nlohmann::json::parse(Combo_SOH_Rando_GetReachableChecks());
  (void)result;
  for(int rc:shopChecks)
   Require(std::find(searched.begin(),searched.end(),rc)!=searched.end(),"generation oracle dropped a shelf slot");
 }
 // Loaded-seed catalog and explicit exclusions remain authoritative.
 auto rc=shopChecks.front();
 ctx->items[rc].excluded=true;Require(!IsVisibleInCheckTracker(rc),"excluded shelf became visible");ctx->items[rc].excluded=false;
 ctx->inactive.insert(rc);Require(!IsCheckShuffled(rc)&&!IsVisibleInCheckTracker(rc),"retired shelf became visible");
 ctx->GenerateLocationPool();Require(ctx->allLocations.size()==63,"generation ignored catalog retirement");
 Combo_SOH_Rando_GetReachableChecks();Require(std::find(searched.begin(),searched.end(),rc)==searched.end(),"oracle ignored catalog retirement");
 // The normal pool/visibility controls are unchanged for unrelated checks.
 ctx->inactive.clear();auto& loc=*Rando::StaticData::GetLocation(rc);loc.type=RCTYPE_POT;
 showOverworldPots=showDungeonPots=false;Require(!IsVisibleInCheckTracker(rc),"unshuffled pot became visible");
 showOverworldPots=true;Require(IsVisibleInCheckTracker(rc),"shuffled pot became hidden");
 std::cout<<"PASS: 64 shop slots, off/refill-only/two/eight rewards, generation pool and oracle candidates, exclusions/catalog and conditional pot filters\n";
}
int main(){try{Run();}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
'''
cpp, exe = out/'oot_shop_visibility.cpp', out/'oot_shop_visibility.exe'
cpp.write_text(code, encoding='utf-8')
subprocess.run([a.compiler,'/nologo','/EHsc','/std:c++20','/W4','/wd4100','/WX',
                '/I'+str(a.json_include.resolve()),str(cpp),
                '/Fe:'+str(exe),'/Fo:'+str(out/'oot_shop_visibility.obj')],check=True)
subprocess.run([str(exe)],check=True)
