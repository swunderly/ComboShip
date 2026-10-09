"""Exercise MM's production pickup draw, visibility and shop-description paths."""
import argparse
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--before', type=Path)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default='cl')
args = p.parse_args()
source, out = args.source.resolve(), args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)

def read(relative):
    candidate = args.before / relative if args.before else source / relative
    return (candidate if candidate.exists() else source / relative).read_text(encoding='utf-8')

def body(text, start):
    pos = text.index(start)
    opening = text.index('{', pos)
    depth = 1
    closing = opening + 1
    while depth:
        depth += (text[closing] == '{') - (text[closing] == '}')
        closing += 1
    return text[pos:closing]

save = read('mm/include/z64save.h')
record_struct = re.search(r'typedef struct RandoSaveCheck \{.*?\} RandoSaveCheck;', save, re.S).group()
record = body(read('mm/2s2h/Rando/CheckTracker/CheckTracker.cpp'), 'void Rando::CheckTracker::RecordSeenItem(')
visible = body(read('mm/2s2h/Rando/DrawItem.cpp'), 'static bool IsCheckModelVisible(')
queue = read('mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp')
draw = body(queue[queue.index('.drawItem ='):], '[](Actor* actor, PlayState* play)')
tail = queue[queue.index('Rando::MiscBehavior::BroadcastCheckObtainedIfFirst((RandoCheckId)CUSTOM_ITEM_PARAM,'):]
tail = tail[:tail.index('CUSTOM_ITEM_PARAM = randoItemId;') + len('CUSTOM_ITEM_PARAM = randoItemId;')]
assignments = '\n'.join(re.findall(r'CUSTOM_ITEM_PARAM2? = [^;]+;', tail))
shop_source = read('mm/2s2h/Rando/ActorBehavior/EnGirlA.cpp')
shop = body(shop_source, 'static std::string ShopDescriptionItemName(') if 'static std::string ShopDescriptionItemName(' in shop_source else ''

stubs = r'''
#define COMBO_BUILD 1
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include "2s2h/Rando/Types.h"
#include "rando/TrackerKnowledge.h"
using u16 = uint16_t;
using u8 = uint8_t;
using s32 = int32_t;
struct Vec3f { float x=0,y=0,z=0; };
struct MtxF { float xx=1,yx=0,zx=0,xy=0,yy=1,zy=0,xz=0,yz=0,zz=1; } modelMatrix;
struct CollisionPoly {};
struct Actor { int id=0; struct { struct { int x=0,y=0,z=0; } rot; } home; };
struct PlayState {
 int transitionMode=0,transitionTrigger=0;
 struct { bool fillScreen=false; } envCtx;
 struct { int state=0; } pauseCtx;
 MtxF viewProjectionMtxF;
 struct { Vec3f eye; } view;
 int colCtx=0;
} playState;
PlayState* gPlayState=&playState;
constexpr int TRANS_MODE_OFF=0,TRANS_TRIGGER_OFF=0,MTXMODE_APPLY=0,ACTOR_EN_GIRLA=42;
bool blocked=true;
Vec3f projection;
MtxF* Matrix_GetCurrent() { return &modelMatrix; }
void Matrix_MultZero(Vec3f* pos) { *pos={}; }
void SkinMatrix_Vec3fMtxFMultXYZW(MtxF*,Vec3f*,Vec3f* pos,float* w) { *pos=projection;*w=1; }
float OTRGetAspectRatio() { return 4.f/3.f; }
bool BgCheck_AnyLineTest2(int*,Vec3f*,Vec3f*,Vec3f*,CollisionPoly**,bool,bool,bool,bool) { return blocked; }
void Matrix_Scale(float,float,float,int) {}
#define CUSTOM_ITEM_FLAGS actor->home.rot.x
#define CUSTOM_ITEM_PARAM actor->home.rot.z
#define CUSTOM_ITEM_PARAM2 actor->home.rot.y
namespace CustomItem { constexpr int CALLED_ACTION=1<<7; }
'''
stubs += record_struct + r'''
struct TestRando { std::array<RandoSaveCheck, RC_MAX> randoSaveChecks{}; };
struct TestShip { TestRando rando; };
struct TestSave { TestShip shipSaveInfo; };
struct TestContext { TestSave save; unsigned fileNum=0; } gSaveContext;
bool randoEnabled=true,foreground=true;
#define IS_RANDO randoEnabled
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
bool Combo_MmIsForeground() { return foreground; }
RandoCheckId capturedCheck=RC_UNKNOWN;
RandoItemId capturedItem=RI_UNKNOWN;
namespace Rando {
 namespace CheckTracker { void RecordSeenItem(RandoCheckId,RandoItemId,uint8_t=1); }
 RandoItemId ConvertItem(RandoItemId item,RandoCheckId) { return item; }
 RandoItemId CurrentTrapItem(RandoCheckId) { return RI_HOOKSHOT; }
 void DrawItem(RandoItemId item,RandoCheckId check,Actor*) {
  capturedCheck=check;capturedItem=item;CheckTracker::RecordSeenItem(check,item);
 }
 namespace StaticData {
  std::string GetItemName(RandoItemId,bool,RandoCheckId,bool=false) { return "displayed item"; }
 }
}
void Require(bool condition,const char* message) { if(!condition)throw std::runtime_error(message); }
'''
tests = r'''
void Run() {
 const auto source=RC_CLOCK_TOWN_NORTH_TREE_PIECE_OF_HEART;
 const auto curiosity=RC_CURIOSITY_SHOP_SPECIAL_ITEM;
 Require(int(RI_GS_TOKEN_OCEAN)==int(curiosity),"fixture must cover the real ID 51 collision");
 auto& original=RANDO_SAVE_CHECKS[source];
 original.shuffled=true;original.obtained=true;
 auto& unvisited=RANDO_SAVE_CHECKS[curiosity];
 unvisited.shuffled=true;unvisited.seenItemId=RI_UNKNOWN;
 Actor pickup;pickup.home.rot.z=int16_t(source);
 FinishNativePickup(&pickup,RI_GS_TOKEN_OCEAN);
 pickup.home.rot.x=CustomItem::CALLED_ACTION;
 QueueDraw(&pickup,gPlayState);
 Require(capturedCheck==source,"native held-up reward used an item ID as another check ID");
 Require(capturedItem==RI_GS_TOKEN_OCEAN,"native reward model changed");
 Require(unvisited.seenItemId==RI_UNKNOWN,"unvisited Curiosity Shop incorrectly became seen");
 pickup.home.rot.y=int16_t(source);pickup.home.rot.z=int16_t(RI_COMBO_FOREIGN);
 QueueDraw(&pickup,gPlayState);
 Require(capturedCheck==source&&capturedItem==RI_COMBO_FOREIGN,"foreign pickup source regressed");

 Actor shelf;shelf.id=ACTOR_EN_GIRLA;
 Require(IsCheckModelVisible(&shelf),"invisible shelf collision prevented an on-screen stock observation");
 Actor other;
 Require(!IsCheckModelVisible(&other),"ordinary occluded rewards must remain unseen");
 projection.x=2;
 Require(!IsCheckModelVisible(&shelf),"off-screen stock must remain unseen");
 projection={};playState.envCtx.fillScreen=true;
 Require(!IsCheckModelVisible(&shelf),"fade-covered stock must remain unseen");
 playState.envCtx.fillScreen=false;playState.pauseCtx.state=1;
 Require(!IsCheckModelVisible(&shelf),"pause-covered stock must remain unseen");
 playState.pauseCtx.state=0;modelMatrix.xx=0;
 Require(!IsCheckModelVisible(&shelf),"unrevealed zero-scale stock must remain unseen");
 modelMatrix.xx=1;
}
int main() {
 try { Run(); RunShop(); std::cout<<"PASS: production MM pickup identity, shelf visibility and observation provenance\n"; }
 catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n';return 1; }
}
'''
shop_test = r'''
void RunShop() {
 auto& stock=RANDO_SAVE_CHECKS[RC_CURIOSITY_SHOP_SPECIAL_ITEM];
 stock.shuffled=true;stock.obtained=false;stock.seenItemId=RI_HOOKSHOT;stock.knowledgeSource=2;
 ShopDescriptionItemName(RI_HOOKSHOT,false,RC_CURIOSITY_SHOP_SPECIAL_ITEM);
 Require(stock.knowledgeSource==1,"selected visible stock remained hinted instead of seen");
 Rando::CheckTracker::RecordSeenItem(RC_CURIOSITY_SHOP_SPECIAL_ITEM,RI_BOW,2);
 Require(stock.knowledgeSource==1&&stock.seenItemId==RI_HOOKSHOT,"a later hint replaced a sighting");
 stock.seenItemId=RI_UNKNOWN;stock.knowledgeSource=0;
 ShopDescriptionItemName(RI_TRAP,false,RC_CURIOSITY_SHOP_SPECIAL_ITEM);
 Require(stock.knowledgeSource==1&&stock.seenItemId==RI_HOOKSHOT,"trap observation revealed its hidden identity");
 auto& remote=RANDO_SAVE_CHECKS[RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART];remote.shuffled=true;
 Rando::CheckTracker::RecordSeenItem(RC_CLOCK_TOWN_WEST_BANK_PIECE_OF_HEART,RI_BOW,2);
 Require(remote.knowledgeSource==2,"genuine hint became a physical sighting");
}
''' if shop else 'void RunShop() {}\n'
cpp = out / 'mm_shop_observation.cpp'
cpp.write_text(stubs + record + '\n' + visible + '\n' + shop + '\n'
               + 'void FinishNativePickup(Actor* actor,RandoItemId randoItemId) {\n' + assignments + '\n}\n'
               + 'auto QueueDraw = ' + draw + ';\n' + shop_test + tests, encoding='utf-8')
exe = out / 'mm_shop_observation.exe'
command = [args.compiler, '/nologo', '/EHsc', '/std:c++17', '/W4', '/wd4100', '/WX',
           '/I'+str(source/'mm'), '/I'+str(source/'combo'), str(cpp),
           '/Fe:'+str(exe), '/Fo:'+str(out/'mm_shop_observation.obj')]
subprocess.run(command, check=True)
subprocess.run([str(exe)], check=True)
