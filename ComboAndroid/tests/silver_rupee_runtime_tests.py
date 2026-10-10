"""Compile native Silver adapters and tracker filters, including saved-rule cold-load cases."""
from pathlib import Path
import argparse,re,subprocess,json
import hostcxx
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--compiler',default=hostcxx.DEFAULT_COMPILER);p.add_argument('--silver-source',type=Path);a=p.parse_args()
source=a.source.resolve();work=a.build_dir.resolve();work.mkdir(parents=True,exist_ok=True)
base=source/'soh/soh/Enhancements/randomizer';text=(a.silver_source or base/'silver_rupees/SilverRupees.cpp').read_text()
def extract(sig,original=None):
 original=text if original is None else original
 start=hostcxx.find(original,sig);at=original.index('{',start)+1;depth=1
 while depth:
  depth+=(original[at]=='{')-(original[at]=='}');at+=1
 if sig.startswith(('const ','struct ')):at+=1 # keep the declaration's ';'
 return original[start:at]+'\n'
seed=(base/'SeedContext.cpp').read_text()
tracker=(base/'randomizer_check_tracker.cpp').read_text()
active=extract('bool Context::IsQuestOfLocationActive(',seed)
shuffled=extract('bool IsCheckShuffled(',tracker)
visible=extract('bool IsVisibleInCheckTracker(',tracker)
misc=(base/'randomizerEnums/RandomizerMiscEnums.h').read_text()
quest=re.search(r'RANDO_ENUM_BEGIN\(RandomizerCheckQuest\)(.*?)RANDO_ENUM_END\(RandomizerCheckQuest\)',misc,re.S)[1]
quests=re.findall(r'RANDO_ENUM_ITEM\((RCQUEST_\w+)\)',quest)
assert quests==['RCQUEST_VANILLA','RCQUEST_MQ','RCQUEST_BOTH']
settings=(base/'settings.cpp').read_text()
assert re.search(r'OPT_U8\(RSK_SHUFFLE_SILVER_RUPEES,\s*"Silver Rupee Shuffle",\s*\{"Vanilla",\s*"Own Dungeon",\s*"Anywhere"\}',settings)
code=r"""
#include "SilverRupeeState.h"
#include <array>
#include <cassert>
#include <map>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstdio>
using RandomizerGet=int;using RandomizerCheck=int;using RandomizerInf=int;using RandomizerSettingKey=int;using SceneID=int;using RandomizerCheckQuest=int;
enum {RG_BLUE_RUPEE=1,GI_RUPEE_BLUE=1,ENGSWITCH_SILVER_TRACKER=0,ENGSWITCH_SILVER_RUPEE=1,ENGSWITCH_ARCHERY_POT=2,ENGSWITCH_TARGET_RUPEE=3,
SCENE_DODONGOS_CAVERN=1,SCENE_SPIRIT_TEMPLE=6,SCENE_SHADOW_TEMPLE=7,SCENE_BOTTOM_OF_THE_WELL=8,SCENE_ICE_CAVERN=9,SCENE_GERUDO_TRAINING_GROUND=11,SCENE_INSIDE_GANONS_CASTLE=13,
RSK_SHUFFLE_SILVER_RUPEES=10000,GAMEMODE_NORMAL=0,TEXT_STATE_NONE=0,PLAYER_STATE1_CLIMBING_LEDGE=0x4000,PLAYER_STATE1_CLIMBING_LADDER=0x200000};
"""
code+='enum {'+','.join(quests)+'};\n'
# Symbols retain distinct identities; persisted numeric values are verified separately.
data=(base/'silver_rupees/SilverRupeeData.inc').read_text();symbols=[]
for token in re.findall(r'\b(?:RG_|RC_|RAND_INF_|RSK_)[A-Z0-9_]+',data):
 if token not in symbols:symbols.append(token)
code+='enum {'+','.join(f'{name}={20000+i}' for i,name in enumerate(symbols))+'};\n'
extra=[]
for token in re.findall(r'\b(?:RC_|RCTYPE_|RCAREA_|RSK_|RO_)[A-Z0-9_]+',shuffled+visible+' RCTYPE_STANDARD'):
 if token not in symbols and token!='RSK_SHUFFLE_SILVER_RUPEES' and token not in extra:extra.append(token)
code+='enum {'+','.join(f'{name}={40000+i}' for i,name in enumerate(extra))+'};\n'
flags=sorted(set(re.findall(r'\b(?:show[A-Z0-9]\w*|alwaysShowGS|hideShopUnshuffledChecks|fortressNormal|fortressFast)\b',shuffled+visible)))
code+='bool '+','.join(name+'=true' for name in flags)+';\n#define IS_RANDO 1\n'
code+=r"""
struct Counts {uint16_t silverRupeeCounts[18]{};};
struct Save {struct {struct {struct {Counts randomizer;}data;int id=0;}quest;}ship;struct {uint32_t swch=0;}sceneFlags[124];int gameMode=0;};
Save gSaveContext;
struct Player {uint32_t stateFlags1=0;};
struct PlayState {int sceneNum=0,msgCtx=0;struct {struct {int num=0;}curRoom;}roomCtx;Player player;};
#define GET_PLAYER(play) (&(play)->player)
struct Actor {int room=0;bool parent=false,killed=false;void(*draw)(Actor*,PlayState*)=nullptr;};
struct EnGSwitch {Actor actor;int type=0;uint16_t silverRandoId=0;uint8_t silverRandomized=0;};
struct GetItemEntry {int check=0;};
namespace Rando {struct Context;}
struct MockRando {GetItemEntry GetItemFromKnownCheck(int check,int){return {check};}
 struct ShopItem {int enGirlAShopItem=50;};ShopItem IdentifyShopItem(int,int){return {};}
 int GetRandoSettingValue(int){return RO_LOGIC_NO_LOGIC;}};
struct OTRGlobals {MockRando* gRandomizer=nullptr;Rando::Context* gRandoContext=nullptr;static OTRGlobals* Instance;};
MockRando mockRando;OTRGlobals globals{&mockRando};OTRGlobals* OTRGlobals::Instance=&globals;
namespace Rando {
struct Option {int value=0;int Get()const{return value;}};
struct Dungeon {bool mq=false;bool IsMQ()const{return mq;}bool IsVanilla()const{return !mq;}};
struct Dungeons {Dungeon dungeon;Dungeon* GetDungeonFromScene(SceneID){return &dungeon;}};
struct Location {int quest=RCQUEST_VANILLA,scene=0,area=0,type=RCTYPE_STANDARD;
 int GetQuest()const{return quest;}int GetScene()const{return scene;}int GetArea()const{return area;}
 int GetRCType()const{return type;}int GetActorParams()const{return 0;}
 bool IsVanillaCompletion()const{return false;}bool IsDungeon()const{return true;}};
namespace StaticData {std::map<int,Location> locations;Location* GetLocation(int rc){return &locations.at(rc);}}
struct ItemLocation {bool excluded=false;bool IsExcluded()const{return excluded;}};
struct Fishsanity {bool GetFishLocationIncluded(Location*)const{return true;}};
struct Context {std::map<int,Option> options;Dungeons dungeons;std::vector<int> allLocations;
 Dungeons* mDungeons=&dungeons;std::map<int,ItemLocation> itemLocations;Fishsanity fish;
 static Context* GetInstance(){static Context value;return &value;}
 Option& GetOption(int key){return options[key];}Dungeons* GetDungeons(){return &dungeons;}
 bool IsQuestOfLocationActive(RandomizerCheck);
 // Silver identities and the unrelated controls in this harness are active in both catalogs.
 // The separate Kokiri catalog suite exercises retirement/canonicalization.
 bool IsLocationInCurrentCatalog(RandomizerCheck)const{return true;}
 ItemLocation* GetItemLocation(int rc){return &itemLocations[rc];}Fishsanity* GetFishsanity(){return &fish;}};
struct Logic {Save save;Save* GetSaveContext(){return &save;}};
}
namespace RandomizerCheckObjects {bool AreaIsOverworld(int area){return area==0;}bool AreaIsDungeon(int area){return area>0&&area!=RCAREA_INVALID;}}
bool enabled=true;int grants=0,lastCheck=-1,rupees=0,tempClear=0;std::map<int,bool> collected;std::vector<int> switches;
int Combo_SilverShuffleEnabled(){return enabled;}
bool Flags_GetRandomizerInf(int inf){auto it=collected.find(inf);return it!=collected.end()&&it->second;}
void Flags_SetRandomizerInf(int inf){collected[inf]=true;}
void Actor_Kill(Actor* actor){actor->killed=true;}
bool Actor_HasParent(Actor* actor,PlayState*){return actor->parent;}
void GiveItemEntryFromActor(Actor*,PlayState*,GetItemEntry entry,float,float){++grants;lastCheck=entry.check;}
void Rupees_ChangeBy(int amount){rupees+=amount;}
int Message_GetState(int* state){return *state;}
void Flags_SetSwitch(PlayState*,int flag){switches.push_back(flag);}
void Flags_SetTempClear(PlayState*,int){++tempClear;}
namespace Rando::SilverRupees {
void DrawPhysical(Actor*,PlayState*){}
"""
code+=extract('struct NativeItem')+extract('const NativeItem nativeItems[]')
code+=extract('struct NativeCheck')+extract('const NativeCheck nativeChecks[]')
code+='unsigned roomSpawnIndex=0;\n'
for sig in ['const NativeItem* FindItem','const NativeCheck* FindCheck','bool IsCheck(', 'bool IsShuffledCheck(', 'bool IsMQ','bool HasPuzzle','void LogicDelta','void AddPool']:code+=extract(sig)
code+='}\n'
code+='namespace Rando {\n'+active+'}\n'+shuffled+visible
for sig in ['extern "C" void Combo_SilverResetRoom','extern "C" int Combo_SilverActorInit','extern "C" int Combo_SilverActorIdle','extern "C" int Combo_SilverGive','extern "C" void Combo_SilverUpdate']:code+=extract(sig)
code+=r"""
using namespace Rando::SilverRupees;
int main(){
 auto ctx=Rando::Context::GetInstance();PlayState play;EnGSwitch actor;actor.type=ENGSWITCH_SILVER_RUPEE;play.sceneNum=8;
 globals.gRandoContext=ctx;
 for(const auto& check:nativeChecks)Rando::StaticData::locations[check.check]={check.quest,check.scene,check.scene,RCTYPE_STANDARD};
 enabled=false;Combo_SilverResetRoom();assert(!Combo_SilverActorInit(&actor,&play)&&roomSpawnIndex==0);enabled=true;
 for(int type:{0,2,3}){actor.type=type;assert(!Combo_SilverActorInit(&actor,&play)&&roomSpawnIndex==0);}actor.type=1;
 // Receiving a foreign pouch solves the puzzle without claiming any physical check.
 Combo_SilverGive(nullptr,RG_POUCH_SILVER_BOTW);assert(gSaveContext.ship.quest.data.randomizer.silverRupeeCounts[1]==5&&rupees==25&&collected.empty());
 assert(gSaveContext.sceneFlags[8].swch==(1u<<31));
 assert(Combo_SilverActorInit(&actor,&play)&&actor.silverRandomized&&!actor.actor.killed&&actor.silverRandoId==5);
 assert(Combo_SilverActorIdle(&actor,&play)&&grants==1&&lastCheck==RC_BOTTOM_OF_THE_WELL_SILVER_RUPEE_1&&collected.empty());
 actor.actor.parent=true;Combo_SilverActorIdle(&actor,&play);assert(actor.actor.killed&&collected[RAND_INF_BOTTOM_OF_THE_WELL_SILVER_RUPEE_1]);
 Combo_SilverResetRoom();EnGSwitch again;again.type=1;Combo_SilverActorInit(&again,&play);assert(again.actor.killed&&roomSpawnIndex==1);
 EnGSwitch second;second.type=1;Combo_SilverActorInit(&second,&play);assert(!second.actor.killed&&second.silverRandoId==6); // A solved puzzle must not hide the other four rewards.
 Combo_SilverGive(nullptr,RG_POUCH_SILVER_BOTW);assert(rupees==50&&gSaveContext.ship.quest.data.randomizer.silverRupeeCounts[1]==5);
 auto untouched=collected;ctx->dungeons.dungeon.mq=true;Combo_SilverGive(nullptr,RG_POUCH_SILVER_DC);assert(collected==untouched&&gSaveContext.ship.quest.data.randomizer.silverRupeeCounts[0]==5&&gSaveContext.sceneFlags[1].swch==0);
 play.sceneNum=1;switches.clear();Combo_SilverUpdate(&play);assert(switches==std::vector<int>{0x25});
 switches.clear();play.msgCtx=1;Combo_SilverUpdate(&play);assert(switches.empty());play.msgCtx=0;
 play.player.stateFlags1=PLAYER_STATE1_CLIMBING_LADDER;Combo_SilverUpdate(&play);assert(switches.empty());play.player.stateFlags1=0;
 // Raw oracle counts reverse exactly after over-cap duplicate pouches.
 Rando::Logic logic;LogicDelta(&logic,RG_RUPEE_SILVER_DC,true);LogicDelta(&logic,RG_POUCH_SILVER_DC,true);LogicDelta(&logic,RG_POUCH_SILVER_DC,true);
 assert(logic.save.ship.quest.data.randomizer.silverRupeeCounts[0]==11&&HasPuzzle(&logic,0));
 LogicDelta(&logic,RG_POUCH_SILVER_DC,false);LogicDelta(&logic,RG_POUCH_SILVER_DC,false);assert(logic.save.ship.quest.data.randomizer.silverRupeeCounts[0]==1&&!HasPuzzle(&logic,0));
 LogicDelta(&logic,RG_RUPEE_SILVER_DC,false);assert(logic.save.ship.quest.data.randomizer.silverRupeeCounts[0]==0);
 // Every selected layout retains its exact physical count with pouch padding.
 for(bool mq:{false,true}){
  ctx->dungeons.dungeon.mq=mq;ctx->allLocations.clear();for(const auto& check:nativeChecks)if((check.quest==RCQUEST_MQ)==mq)ctx->allLocations.push_back(check.check);
  ctx->GetOption(RSK_SHUFFLE_SILVER_RUPEES).value=0;std::vector<int> pool;AddPool(pool);assert(pool.empty());
  for(const auto& check:nativeChecks)assert(!IsShuffledCheck(check.check));
  ctx->GetOption(RSK_SHUFFLE_SILVER_RUPEES).value=2;AddPool(pool);assert(pool.size()==(mq?74:80));
  for(const auto& check:nativeChecks)assert(IsShuffledCheck(check.check)==((check.quest==RCQUEST_MQ)==mq));
  assert(!IsShuffledCheck(-1));
  for(const auto& item:nativeItems)ctx->GetOption(item.setting).value=1;
  pool.clear();AddPool(pool);assert(pool.size()==(mq?74:80));
  for(const auto& item:nativeItems)if(item.pouch)assert(std::count(pool.begin(),pool.end(),item.item)<=1);
  for(const auto& item:nativeItems)if(!item.pouch)assert(std::count(pool.begin(),pool.end(),item.item)==0);
  for(const auto& item:nativeItems)ctx->GetOption(item.setting).value=0;
 }
 // SaveManager restores options and dungeon layout without generating allLocations.
 // Test the actual Context quest predicate and both native tracker filter layers.
 unsigned savedCases=0;
 for(bool mq:{false,true})for(int mode:{0,1,2})for(int poolState:{0,1,2}){
  ctx->dungeons.dungeon.mq=mq;ctx->GetOption(RSK_SHUFFLE_SILVER_RUPEES).value=mode;ctx->allLocations.clear();
  for(const auto& check:nativeChecks)if(poolState==2||(poolState==1&&((check.quest==RCQUEST_MQ)!=mq)))ctx->allLocations.push_back(check.check);
  for(const auto& check:nativeChecks){
   bool expected=mode!=0&&((check.quest==RCQUEST_MQ)==mq);
   assert(IsShuffledCheck(check.check)==expected);
   assert(IsCheckShuffled(check.check)==expected);
   assert(IsVisibleInCheckTracker(check.check)==expected);
   ctx->itemLocations[check.check].excluded=true;assert(!IsVisibleInCheckTracker(check.check));
   ctx->itemLocations[check.check].excluded=false;++savedCases;
  }
  assert(!IsShuffledCheck(-1)&&!IsShuffledCheck(0x7fffffff));
 }
 assert(savedCases==2772);
 ctx->allLocations.clear();ctx->dungeons.dungeon.mq=false;ctx->GetOption(RSK_SHUFFLE_SILVER_RUPEES).value=1;
 for(const auto& check:nativeChecks){
  auto& loc=Rando::StaticData::locations.at(check.check);auto original=loc;
  loc.quest=999;assert(!IsShuffledCheck(check.check)&&!IsCheckShuffled(check.check)&&!IsVisibleInCheckTracker(check.check));loc=original;
  loc.area=RCAREA_INVALID;assert(!IsCheckShuffled(check.check)&&!IsVisibleInCheckTracker(check.check));loc=original;
  for(int type:{RCTYPE_GOSSIP_STONE,RCTYPE_STATIC_HINT,RCTYPE_CHEST_GAME}){loc.type=type;assert(!IsCheckShuffled(check.check)&&!IsVisibleInCheckTracker(check.check));}loc=original;
 }
 // Ordinary conditional checks still obey the real tracker type filters.
 Rando::StaticData::locations[-2]={RCQUEST_BOTH,8,8,RCTYPE_POT};
 showOverworldPots=showDungeonPots=false;assert(!IsCheckShuffled(-2)&&!IsVisibleInCheckTracker(-2));
 showDungeonPots=true;assert(IsCheckShuffled(-2)&&IsVisibleInCheckTracker(-2));
 Rando::StaticData::locations[-2].type=RCTYPE_GRASS;showOverworldGrass=showDungeonGrass=false;
 assert(!IsCheckShuffled(-2)&&!IsVisibleInCheckTracker(-2));showDungeonGrass=true;assert(IsCheckShuffled(-2)&&IsVisibleInCheckTracker(-2));
 puts("PASS: all 154 Silver checks, 2772 saved-rule/layout/pool states, native tracker/exclusion filters, invalid quest/area/type rejection; existing actor/grant/oracle/pool contracts preserved.");
}
"""
file=work/'silver-runtime.cpp';file.write_text(code)
subprocess.run([str(hostcxx.build(a.compiler,file,includes=[base/'silver_rupees']))],check=True, timeout=hostcxx.RUN_TIMEOUT)
