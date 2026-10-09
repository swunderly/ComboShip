"""Exercise native collision/blockstop code with contracted and N64-style math."""
from pathlib import Path
import argparse, subprocess, re
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--compiler',required=True);a=p.parse_args()
s=a.source.resolve();w=a.build_dir.resolve();w.mkdir(parents=True,exist_ok=True)
def function(text,signature):
    start=text.index(signature);opening=text.index(') {',start)+2;end=opening+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
collision=(s/'soh/src/code/z_bgcheck.c').read_text(encoding='utf-8')
block=(s/'soh/src/overlays/actors/ovl_Obj_Blockstop/z_obj_blockstop.c').read_text(encoding='utf-8')
save_scene=(s/'soh/src/code/z_play.c').read_text(encoding='utf-8')
flags=(s/'soh/src/code/z_actor.c').read_text(encoding='utf-8')
push_block=(s/'soh/src/overlays/actors/ovl_Obj_Oshihiki/z_obj_oshihiki.c').read_text(encoding='utf-8')
prelude=r'''#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
using f32=float;using s32=int;using s16=int16_t;using u8=uint8_t;
struct Vec3f{f32 x,y,z;};struct Vec3s{s16 x,y,z;};
struct Plane{Vec3f normal;f32 originDist;};
struct CollisionPoly{Vec3s normal;s16 dist;};
#define COLPOLY_NORMAL_FRAC (1.0f / 32767)
#define IS_ZERO(v) (fabsf(v)<0.008f)
#define LUSLOG_INFO(...) ((void)0)
enum{FLAG_SCENE_SWITCH,ACTOR_OBJ_OSHIHIKI=255,PUSHBLOCK_HUGE_START_ON=3,PUSHBLOCK_HUGE_START_OFF=7,NA_SE_SY_CORRECT_CHIME=11,NA_SE_SY_TRE_BOX_APPEAR=12,SCENE_FIRE_TEMPLE=4,BGCHECK_SCENE=50};
using u16=uint16_t;
struct Actor{struct{Vec3f pos;struct{s16 y=0;}rot;}home,world;Vec3f scale{.2f,.2f,.2f};int params=0,id=0,room=0;CollisionPoly*floorPoly=nullptr;bool killed=false;};
struct ObjBlockstop{Actor actor;};struct DynaPolyActor{Actor actor;};
struct ObjOshihiki{DynaPolyActor dyna;};
struct CollisionContext{CollisionPoly poly;DynaPolyActor*block=nullptr;};
struct PlayState{CollisionContext colCtx;struct{struct{uint32_t swch=0,tempSwch=0,chest=0,clear=0,collect=0;}flags;}actorCtx;int sceneNum=11;};
struct SavedSceneFlags{uint32_t chest=0,swch=0,clear=0,collect=0;};
struct{SavedSceneFlags sceneFlags[128];struct{Vec3f fireTempleBlockPos{};u8 fireTempleBlockSeated=0;}ship;}gSaveContext;
bool isMq=false;
bool ResourceMgr_IsSceneMasterQuest(int){return isMq;}
int chimes=0,lastSound=0,hooks=0;
f32 Math_SinS(s16 a){return sinf(a*.00009587379924285f);}
f32 Math_CosS(s16 a){return cosf(a*.00009587379924285f);}
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
// Exact decoded lower-floor rectangle; native polygon/probe/push/fall behavior
// is independently executed against the user's archive by the geometry contract.
f32 BgCheck_EntityRaycastFloor6(CollisionContext*,CollisionPoly** out,s32* bg,Actor*,Vec3f* p,f32){
    static CollisionPoly floor={{0,32767,0},-2080};*bg=BGCHECK_SCENE;*out=nullptr;
    if(p->x<2340||p->x>2580||p->z<280||p->z>400)return -32000;
    *out=&floor;return 2080;
}
void GameInteractor_ExecuteOnSceneFlagSet(int,int,int){++hooks;}
void Actor_Kill(Actor*a){a->killed=true;}
void Sfx_PlaySfxCentered(int s){++chimes;lastSound=s;}
void CollisionPoly_GetNormalF(CollisionPoly*p,f32*x,f32*y,f32*z){*x=p->normal.x*COLPOLY_NORMAL_FRAC;*y=p->normal.y*COLPOLY_NORMAL_FRAC;*z=p->normal.z*COLPOLY_NORMAL_FRAC;}
void CollisionPoly_GetVertices(CollisionPoly*,Vec3s*,Vec3f*){}
void Math3D_LineSplitRatio(Vec3f*a,Vec3f*b,f32 t,Vec3f*out){*out={a->x+(b->x-a->x)*t,a->y+(b->y-a->y)*t,a->z+(b->z-a->z)*t};}
// The test ray is inside a broad horizontal block face. Triangle containment is
// independent of the plane-boundary arithmetic being tested here.
bool Math3D_TriChkPointParaXDist(Vec3f*,Vec3f*,Vec3f*,Plane*,f32,f32,f32){return true;}
bool Math3D_TriChkPointParaYDist(Vec3f*,Vec3f*,Vec3f*,Plane*,f32,f32,f32){return true;}
bool Math3D_TriChkLineSegParaZDist(Vec3f*,Vec3f*,Vec3f*,Plane*,f32,f32,f32){return true;}
'''
code=prelude+'\n'+function(collision,'s32 CollisionPoly_LineVsPoly(')+'\n'
code+=r'''s32 BgCheck_EntityLineTest2(CollisionContext*c,Vec3f*a,Vec3f*b,Vec3f*out,CollisionPoly**poly,bool,bool,bool,bool,int*bg,Actor*){
    if(!c->block)return false;
    bool hit=CollisionPoly_LineVsPoly(&c->poly,nullptr,a,b,out,false,1.0f);
    if(hit){*poly=&c->poly;*bg=1;}return hit;
}
DynaPolyActor*DynaPoly_GetActor(CollisionContext*c,int){return c->block;}
'''
for signature in ('s32 Flags_GetSwitch(','void Flags_SetSwitch('):code+=function(flags,signature)+'\n'
code+=function(save_scene,'void Play_SaveSceneFlags(')+'\n'
for signature in ('void ObjBlockstop_Init(','void ObjBlockstop_Update('):
    # Skip the forward declarations before the actual definition.
    code+=function(block[block.index('void ObjBlockstop_Init(Actor* thisx, PlayState* play) {'):],signature)+'\n'
points_begin=push_block.index('static Vec3f sColCheckPoints[5]')
code+=push_block[points_begin:push_block.index('};',points_begin)+2]+'\n'
code+=function(push_block,'static s32 ObjOshihiki_IsFireTempleBlockSeatedPosition(ObjOshihiki* this, PlayState* play, Vec3f* pos) {')+'\n'
for signature in ('static s32 ObjOshihiki_IsFireTempleHoleBlock(', 'static void ObjOshihiki_RestoreFireTempleBlock(', 'static void ObjOshihiki_SaveFireTempleBlock('):
    code+=function(push_block,signature)+'\n'
code+=r'''#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"failed: %s at %d\n",#x,__LINE__);return 1;}}while(0)
int main(int argc,char**){
    int missed=0;
    // GTG blockstop is on the -41 plane; repeat for both huge block variants.
    for(int sample=0;sample<2;++sample){
        int height=-41;
        PlayState p;DynaPolyActor b;b.actor.id=ACTOR_OBJ_OSHIHIKI;b.actor.params=sample==0?PUSHBLOCK_HUGE_START_ON:PUSHBLOCK_HUGE_START_OFF;
        p.colCtx.block=&b;p.colCtx.poly={{0,-32767,0},(s16)height};
        ObjBlockstop stop;stop.actor.params=11;stop.actor.home.pos={0,(float)height,0};stop.actor.world.pos=stop.actor.home.pos;
        ObjBlockstop_Init(&stop.actor,&p);ObjBlockstop_Update(&stop.actor,&p);
        if(!Flags_GetSwitch(&p,11)){++missed;continue;}
        REQUIRE(stop.actor.killed && lastSound==NA_SE_SY_CORRECT_CHIME);
        // Native switch state survives room reconstruction and saved scene restoration.
        Play_SaveSceneFlags(&p);PlayState reloaded;reloaded.actorCtx.flags.swch=gSaveContext.sceneFlags[11].swch;
        ObjBlockstop second;second.actor.params=11;ObjBlockstop_Init(&second.actor,&reloaded);
        REQUIRE(second.actor.killed && Flags_GetSwitch(&reloaded,11));
    }
    printf("missed seated blocks: %d\n",missed);
    if(argc>1)return missed>0?0:1; // Baseline must reproduce the failure.
    REQUIRE(missed==0);
    PlayState empty;ObjBlockstop pending;pending.actor.params=11;
    ObjBlockstop_Init(&pending.actor,&empty);ObjBlockstop_Update(&pending.actor,&empty);
    REQUIRE(!pending.actor.killed && !empty.actorCtx.flags.swch);
    DynaPolyActor unrelated;unrelated.actor.id=42;empty.colCtx.block=&unrelated;
    empty.colCtx.poly={{0,-32767,0},-100};pending.actor.home.pos={0,-100,0};pending.actor.world.pos={0,-99,0};
    ObjBlockstop_Update(&pending.actor,&empty);REQUIRE(!pending.actor.killed && !empty.actorCtx.flags.swch);
    unrelated.actor.id=ACTOR_OBJ_OSHIHIKI;unrelated.actor.params=0;
    ObjBlockstop_Update(&pending.actor,&empty);REQUIRE(pending.actor.killed && lastSound==NA_SE_SY_TRE_BOX_APPEAR);
    PlayState fire;fire.sceneNum=SCENE_FIRE_TEMPLE;ObjOshihiki fb;
    fb.dyna.actor.room=4;fb.dyna.actor.params=0xff42;fb.dyna.actor.world.pos={2279,2260,340};
    ObjOshihiki_SaveFireTempleBlock(&fb,&fire);REQUIRE(!gSaveContext.ship.fireTempleBlockSeated);
    fb.dyna.actor.world.pos={2400.5f,2080,340};isMq=true;
    ObjOshihiki_SaveFireTempleBlock(&fb,&fire);REQUIRE(!gSaveContext.ship.fireTempleBlockSeated);
    isMq=false;fb.dyna.actor.room=5;
    ObjOshihiki_SaveFireTempleBlock(&fb,&fire);REQUIRE(!gSaveContext.ship.fireTempleBlockSeated);
    fb.dyna.actor.room=4;fb.dyna.actor.params=0xff41;
    ObjOshihiki_SaveFireTempleBlock(&fb,&fire);REQUIRE(!gSaveContext.ship.fireTempleBlockSeated);
    fb.dyna.actor.params=0xff42;ObjOshihiki_SaveFireTempleBlock(&fb,&fire);
    REQUIRE(gSaveContext.ship.fireTempleBlockSeated==1);
    fb.dyna.actor.world.pos=fb.dyna.actor.home.pos={2279,2260,340};
    ObjOshihiki_RestoreFireTempleBlock(&fb,&fire);REQUIRE(fb.dyna.actor.world.pos.y==2080 && fb.dyna.actor.home.pos.x==2400.5f && fb.dyna.actor.home.pos.z==340);
    gSaveContext.ship.fireTempleBlockPos.y=NAN;fb.dyna.actor.world.pos={2279,2260,340};
    ObjOshihiki_RestoreFireTempleBlock(&fb,&fire);REQUIRE(fb.dyna.actor.world.pos.y==2260);
    puts("native collision, switches, Fire Temple landing/restoration and negative controls passed");return 0;
}
'''
# C actor source calls its local variable 'this', a reserved C++ keyword.
code=re.sub(r'\bthis\b','self',code)
h=w/'block.cpp';h.write_text(code)
for kind,options in [('baseline',['-ffp-contract=fast']),('fixed',['-fno-fast-math','-ffp-contract=off'])]:
    exe=w/(kind+'.exe');obj=w/(kind+'.obj');cmd=[a.compiler,'--target=x86_64-pc-windows-msvc','-O2','-mfma',*options,'-c',str(h),'-o',str(obj)]
    subprocess.run(cmd,check=True);subprocess.run(['cl','/nologo',str(obj),'/Fe:'+str(exe),'/link','libcmt.lib','libucrt.lib','libvcruntime.lib','kernel32.lib'],check=True);subprocess.run([str(exe)]+(['baseline'] if kind=='baseline' else []),check=True)
cmake=(s/'soh/CMakeLists.txt').read_text(encoding='utf-8');assert 'target_compile_options(${PROJECT_NAME} PRIVATE -fno-fast-math -ffp-contract=off' in cmake
print('Android OoT target uses the verified non-contracting flags.')

