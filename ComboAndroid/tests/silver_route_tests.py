"""Compile the production silver route registrar and all pinned predicates."""
from pathlib import Path
import argparse,json,re,subprocess
import hostcxx
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--compiler',default=hostcxx.DEFAULT_COMPILER);p.add_argument('--json-include',type=Path,required=True);a=p.parse_args()
r=a.source.resolve()/'soh/soh/Enhancements/randomizer';d=r/'silver_rupees';work=a.build_dir.resolve();work.mkdir(parents=True,exist_ok=True)
fixtures=Path(__file__).resolve().parent/'fixtures'
up=json.loads((fixtures/'silver-upstream-access.json').read_text())['access'];receipt=json.loads((fixtures/'silver-native-route-map.json').read_text())['access'];inc=(d/'SilverRoutes.inc').read_text()
assert len(up)==len(receipt)==188
assert len({x['check'] for x in up})==154 and len({x['region'] for x in up})==39
for original,ported in zip(up,receipt):
 for key in ['file','region','check','condition']:assert original[key]==ported[key],(key,original,ported)
 assert ported['nativeRegion'] and ported['virtualRegion'] and 'UNRESOLVED' not in inc
owners=re.findall(r'SR_VIRTUAL\((RR_\w+), (RC_\w+), (SCENE_\w+)',inc)
assert len(owners)==154 and len({x[1] for x in owners})==154
assert len(re.findall(r'^SR_ROUTE\(',inc,re.M))==188
for enum,prefix in [('RandomizerRegion.h','RR_SR_'),('LogicVal.h','LOGIC_SR_')]:
 text=(r/'randomizerEnums'/enum).read_text();ids=re.findall(r'RANDO_ENUM_ITEM\((\w+)\)',text)
 assert len(ids)==len(set(ids));first=next(i for i,x in enumerate(ids) if x.startswith(prefix));assert all(x.startswith(prefix) for x in ids[first:-1])
header=(d/'SilverLogic.h').read_text();header=re.sub(r'^#include (?!"SilverRoutes.inc")[^\n]*\n','',header,flags=re.M).replace('#pragma once','')
def predicate(file,kind,id):
 s=(r/('location_access/dungeons/'+file+'.cpp')).read_text();needle=kind+'('+id+',';a=s.index(needle)+len(needle);i=a;depth=1
 while depth:
  depth+=(s[i]=='(')-(s[i]==')');i+=1
 return s[a:i-1].strip()
gates={
 'fireDoor':predicate('ganons_castle','ENTRANCE','RR_GANONS_CASTLE_FIRE_TRIAL_BARRED_DOOR'),
 'mqFireDoor':predicate('ganons_castle','ENTRANCE','RR_GANONS_CASTLE_MQ_FIRE_TRIAL_BARRED_DOOR'),
 'mqShadowDoor':predicate('ganons_castle','ENTRANCE','RR_GANONS_CASTLE_MQ_SHADOW_TRIAL_FINAL_ROOM'),
 'waterChest':predicate('gerudo_training_ground','LOCATION','RC_GERUDO_TRAINING_GROUND_UNDERWATER_SILVER_RUPEE_CHEST')}
ids=sorted(set(re.findall(r'\b(?:RR|RC|RG|RT|RSK|LOGIC|SCENE)_\w+',inc+header+'\n'.join(gates.values())))|{'RG_RUPEE_SILVER_DC'})
# Native soul policy supplies the actual SceneID enum and scene constants.
ids=[id for id in ids if not id.startswith('SCENE_')]
code='''#include <cstdio>
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <set>
'''+ '#define OOT_SOUL_POLICY_HOST_ENUMS\n#include \"'+(r/'souls/OotSoulState.h').as_posix()+'\"\n'+ 'enum Id { '+','.join(ids)+', ID_MAX }; using RandomizerGet=int;\n'+'''
namespace Rando { struct Logic {bool IsAdult=true,IsChild=false,all=true; std::set<int> items,flags;
 bool CanUse(int x){return all||items.count(x);} bool HasItem(int x){return CanUse(x);} bool Get(int x){return all||flags.count(x);}
 bool HasFireSource(){return all||CanUse(RG_DINS_FIRE)||CanUse(RG_FIRE_ARROWS);} bool HasExplosives(){return all;} bool BlastOrSmash(){return all;} bool BlueFire(){return all;} bool CanUseSword(){return all;} int FireTimer(){return all?100:0;} int WaterTimer(){return all?100:0;} bool CanStandingShield(){return all;}
};}
// This route harness uses the production soul ownership helper. Legacy route
// cases leave shuffling disabled; the final cases require both enemy souls.
namespace Rando::Souls { GeneralOotSoulsOwnership routeSouls{}; GeneralOotSoulOptions routeSoulOptions{};
bool Has(Rando::Logic*,OotSoul soul){return HaveGeneralOotSoul(routeSouls,routeSoulOptions,soul);} }
struct Option { int Get(){return mode;} static int mode;};int Option::mode=0;struct Context {Option GetOption(int){return {};} bool GetTrickOption(int){return false;}} context;auto*ctx=&context;auto logic=std::make_shared<Rando::Logic>();
namespace SilverRupees {int PuzzleForItem(int x){return x;} bool solved=false;bool HasPuzzle(Rando::Logic*,size_t){return solved;}}
struct Location{int id;std::function<bool()> fn;};struct Event{int id;std::function<bool()> fn;};struct Exit{int target;std::function<bool()> fn;};
#define LOCATION(id,condition) Location{id,[]{return condition;}}
#define EVENT_ACCESS(id,condition) Event{id,[]{return condition;}}
struct Region {int scene=-1;std::vector<Event>events;std::vector<Location>locations;std::vector<Exit>exits;Region()=default;
 Region(std::string,int s,std::vector<Event> e,std::vector<Location>l,std::vector<Exit>x):scene(s),events(e),locations(l),exits(x){}
 void AddExit(int,int target,std::function<bool()>fn,const char*){exits.push_back({target,fn});}};
std::vector<Region>areaTable(ID_MAX);
'''+header+ '\nnamespace SilverLogic=Rando::SilverLogic;\n'+'\n'.join(f'bool {name}(){{return {expr};}}' for name,expr in gates.items())+'\nint main(){\n'
# Assign native scene ownership from actual containing dungeon definitions.
parents=set(re.findall(r'SR_(?:ROUTE|EVENT)\((RR_\w+)',inc))
for parent in sorted(parents):
 found=[]
 for f in (r/'location_access/dungeons').glob('*.cpp'):
  m=re.search(r'areaTable\['+parent+r'\]\s*=\s*Region\("[^"\n]*",\s*(SCENE_\w+)',f.read_text())
  if m:found.append(m[1])
 assert len(found)==1,(parent,found)
 code+=f'areaTable[{parent}].scene={found[0]};\n'
for scene in sorted({x[2] for x in owners}):code+=f'Rando::SilverLogic::RegisterRoutes({scene});\n'
code+='''int exits=0;for(auto&r:areaTable)for(auto&e:r.exits){assert(!e.fn());++exits;}printf("exits %d\\n",exits);fflush(stdout);assert(exits==188);
Option::mode=1;assert(!Rando::SilverLogic::Gate(RG_RUPEE_SILVER_DC,true));SilverRupees::solved=true;assert(Rando::SilverLogic::Gate(RG_RUPEE_SILVER_DC,false));Option::mode=0;assert(Rando::SilverLogic::Gate(RG_RUPEE_SILVER_DC,true));assert(!Rando::SilverLogic::Gate(RG_RUPEE_SILVER_DC,false));Option::mode=1;
std::set<int> reachable;for(auto&r:areaTable)for(auto&e:r.exits)if(e.fn())reachable.insert(e.target);
'''
for region,check,scene in owners:code+=f'assert(areaTable[{region}].locations.size()==1);assert(areaTable[{region}].locations[0].id=={check});assert(areaTable[{region}].locations[0].fn());assert(reachable.count({region}));\n'
code+='''logic->all=false;logic->IsAdult=false;logic->IsChild=true;int blocked=0;for(auto&r:areaTable)for(auto&e:r.exits)blocked+=!e.fn();assert(blocked>80);
// A solved received puzzle never removes the physical pickup location.
for(auto&r:areaTable)for(auto&l:r.locations)assert(l.fn());
logic->items.insert(RG_LENS_OF_TRUTH);logic->items.insert(RG_HOVER_BOOTS);logic->IsAdult=true;logic->IsChild=false;
auto access=[](int parent,int target){for(auto&e:areaTable[parent].exits)if(e.target==target&&e.fn())return true;return false;};
logic->items.clear();assert(!access(RR_GANONS_CASTLE_LIGHT_TRIAL_BOULDER_ROOM,RR_SR_GANON_CASTLE_SILVER_RUPEE_LIGHT_CENTER_TOP));
logic->items.insert(RG_HOOKSHOT);assert(access(RR_GANONS_CASTLE_LIGHT_TRIAL_BOULDER_ROOM,RR_SR_GANON_CASTLE_SILVER_RUPEE_LIGHT_CENTER_TOP));
logic->items.clear();assert(!access(RR_GANONS_CASTLE_FIRE_TRIAL_FROM_OPEN,RR_SR_GANON_CASTLE_SILVER_RUPEE_FIRE_FAR_RIGHT));
logic->items.insert(RG_GORON_TUNIC);assert(!access(RR_GANONS_CASTLE_FIRE_TRIAL_FROM_OPEN,RR_SR_GANON_CASTLE_SILVER_RUPEE_FIRE_FAR_RIGHT));
logic->items.insert(RG_GOLDEN_GAUNTLETS);assert(access(RR_GANONS_CASTLE_FIRE_TRIAL_FROM_OPEN,RR_SR_GANON_CASTLE_SILVER_RUPEE_FIRE_FAR_RIGHT));
logic->items.clear();logic->IsAdult=false;logic->IsChild=true;
assert(!access(RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM,RR_SR_GERUDO_TRAINING_GROUNDS_SILVER_RUPEE_SLOPE_FRONT_ABOVE));
logic->items.insert(RG_HOOKSHOT);assert(!access(RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM,RR_SR_GERUDO_TRAINING_GROUNDS_SILVER_RUPEE_SLOPE_FRONT_ABOVE));
logic->IsAdult=true;logic->IsChild=false;assert(access(RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM,RR_SR_GERUDO_TRAINING_GROUNDS_SILVER_RUPEE_SLOPE_FRONT_ABOVE));
logic->IsAdult=false;logic->IsChild=true;logic->items.insert(RG_LONGSHOT);assert(access(RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM,RR_SR_GERUDO_TRAINING_GROUNDS_SILVER_RUPEE_SLOPE_FRONT_ABOVE));
logic->items.clear();SilverRupees::solved=true;assert(!fireDoor());assert(!mqFireDoor());assert(!mqShadowDoor());assert(!waterChest());
logic->items.insert(RG_LONGSHOT);assert(!fireDoor());logic->items.insert(RG_GORON_TUNIC);assert(fireDoor());assert(mqFireDoor());
SilverRupees::solved=false;assert(!fireDoor());assert(!mqFireDoor());logic->items.insert(RG_HOVER_BOOTS);assert(mqFireDoor());
logic->items.insert(RG_LENS_OF_TRUTH);assert(!mqShadowDoor());SilverRupees::solved=true;assert(mqShadowDoor());
logic->items.insert(RG_OPEN_CHEST);assert(waterChest());logic->items.clear();assert(!waterChest());
Option::mode=0;logic->all=true;assert(fireDoor());assert(mqFireDoor());assert(mqShadowDoor());assert(waterChest());
Option::mode=1;Rando::Souls::routeSoulOptions={true,true,false,false,false};
assert(!access(RR_GERUDO_TRAINING_GROUND_HAMMER_ROOM,RR_SR_GERUDO_TRAINING_GROUNDS_SILVER_RUPEE_LAVA_BACK_CENTER));
Rando::Souls::ChangeGeneralOotSoul(Rando::Souls::routeSouls,Rando::Souls::GeneralOotSoulIndex(Rando::Souls::OotSoul::ENEMY_KEESE),true);
assert(!access(RR_GERUDO_TRAINING_GROUND_HAMMER_ROOM,RR_SR_GERUDO_TRAINING_GROUNDS_SILVER_RUPEE_LAVA_BACK_CENTER));
Rando::Souls::ChangeGeneralOotSoul(Rando::Souls::routeSouls,Rando::Souls::GeneralOotSoulIndex(Rando::Souls::OotSoul::ENEMY_TORCH_SLUG),true);
assert(access(RR_GERUDO_TRAINING_GROUND_HAMMER_ROOM,RR_SR_GERUDO_TRAINING_GROUNDS_SILVER_RUPEE_LAVA_BACK_CENTER));
return 0;}'''
cpp=work/'silver_routes.cpp';cpp.write_text(code)
exe=hostcxx.build(a.compiler,cpp,std='c++17',warnings=False,includes=[d,a.source/'soh/include',a.source/'soh',a.json_include])
subprocess.run([str(exe)],check=True, timeout=hostcxx.RUN_TIMEOUT)
print('188 pinned alternatives / 154 unique owners / 39 source regions: production registrar predicates passed')
