#ifdef COMBO_BUILD
#include "FaroresWind.h"
#include "rando/FaroreRules.h"
#include "rando/SharedDungeonJson.h"
#include "2s2h_assets.h"
#include "ComboExport.h"
#include "Rando/Rando.h"
#include "2s2h/SaveManager/SaveManager.h"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/Resource.h>
#include "NativeSpellResources.h"
#include <array>
#include <memory>

namespace {
static const ALIGN_ASSET(2) char sComboFaroreItemName[] = "__OTR__@oot:textures/item_name_static/gFaroresWindItemNameENGTex";
ComboNativeDepartureCommit sNativeDepartureCommit=nullptr;
ComboFaroreSaveInfo& State();
ComboReadSharedDungeon sReadDungeon=nullptr;
ComboWriteSharedDungeon sWriteDungeon=nullptr;
bool sSharedDungeonEnabled=false;
int sDungeonDepartureAge=-1,sDungeonDepartureEntrance=-1,sDungeonArrivalFile=-1,sDungeonArrivalAge=-1,sDungeonArrivalEntrance=-1;
void ReadDungeon() {
 ComboSharedDungeon d{};if(sReadDungeon&&sReadDungeon(gSaveContext.fileNum,&d))State().sharedCurrentDungeon=d;
}
void WriteDungeon() {
 if(sWriteDungeon)sWriteDungeon(gSaveContext.fileNum,&State().sharedCurrentDungeon);
}

ComboFaroreGetOotAge sOotAge=nullptr;
ComboFaroreGetOotDestination sOotDestination=nullptr;
bool sEnabled=false,sCrossGame=false,sCrossAge=false,sAdultMask=false;
ComboFaroreSyncOotPoint sSyncOot=nullptr;
ComboFaroreEffectiveAge sNativeAge=nullptr;
ComboFaroreAgeModeEnabled sNativeAgeMode=nullptr;
ComboFaroreTransitionAge sCanAge=nullptr,sApplyAge=nullptr;
int NativeAge() {return sNativeAge?sNativeAge(gSaveContext.fileNum):1;}
bool CanAge(int age) {
 if(age<0||age>1)return false;
 return sCanAge?sCanAge(gSaveContext.fileNum,age)!=0:age==1&&NativeAge()==1;
}
void SyncOot(int age) {
 if(sCrossGame&&sSyncOot&&age>=0&&age<=1) {
  const auto& p=gSaveContext.save.shipSaveInfo.faroresWind.point[age];
  sSyncOot(gSaveContext.fileNum,age,p.data,p.entrance);
 }
}
int sDepartureAge=-1,sDepartureEntrance=-1,sArrivalFile=-1,sArrivalAge=-1,sArrivalEntrance=-1;
// Explicit cross-arrival age survives queue consumption until Play_Init, like gComboCtx.fwSpawnAge.
int sSpawnAge=-1;
std::shared_ptr<Ship::ResourceManager> sOotResources,sMmResources;
std::array<std::shared_ptr<Ship::IResource>,10> sOwners,sFrameOwners;
bool sOotAlt=false,sMmAlt=false;
constexpr const char* sPaths[]={"overlays/ovl_Magic_Wind/sTex",
 "objects/gameplay_keep/gPlayerAnim_link_magic_tame","objects/gameplay_keep/gPlayerAnim_link_magic_kaze1",
 "objects/gameplay_keep/gPlayerAnim_link_magic_kaze2","objects/gameplay_keep/gPlayerAnim_link_magic_kaze3",
 "textures/icon_item_static/gItemIconFaroresWindTex","objects/object_gi_goddess/gGiMagicSpellDiamondDL",
 "objects/object_gi_goddess/gGiFaroresWindColorDL","objects/object_gi_goddess/gGiMagicSpellOrbDL",
 "textures/item_name_static/gFaroresWindItemNameENGTex"};
void DropResources() {ComboRando::ComboFaroreIntegrationReady=false;sOwners={};sFrameOwners={};sOotResources.reset();sMmResources.reset();}
ComboFaroreSaveInfo& State() {return gSaveContext.save.shipSaveInfo.faroresWind;}
bool ValidAge(int age) {return age==0||age==1;}
}
extern "C" COMBO_EXPORT void MM_SetFaroreBridge(ComboFaroreGetOotAge a,ComboFaroreGetOotDestination d) {
 sOotAge=a;sOotDestination=d;
}
extern "C" COMBO_EXPORT void MM_ConfigureFaroreRules(int wind,int cross,int age) {
 const bool enabled=wind!=0,crossGame=enabled&&cross!=0,crossAge=age!=0;
 if(sEnabled==enabled&&sCrossGame==crossGame&&sCrossAge==crossAge)return;
 sEnabled=enabled;sCrossGame=crossGame;sCrossAge=crossAge;
 sDepartureAge=sDepartureEntrance=sArrivalFile=sArrivalAge=sArrivalEntrance=-1;sSpawnAge=-1;
 sDungeonDepartureAge=sDungeonDepartureEntrance=sDungeonArrivalFile=sDungeonArrivalAge=sDungeonArrivalEntrance=-1;
}
extern "C" COMBO_EXPORT void MM_SetFaroreAgeBridge(ComboFaroreEffectiveAge get,ComboFaroreTransitionAge can,ComboFaroreTransitionAge apply) {
 sNativeAge=get;sCanAge=can;sApplyAge=apply;
}
extern "C" COMBO_EXPORT void MM_SetFaroreAgeModeBridge(ComboFaroreAgeModeEnabled mode) {sNativeAgeMode=mode;}
extern "C" COMBO_EXPORT void MM_SetFaroreAgeMode(int adultMask) {sAdultMask=adultMask!=0;}
extern "C" COMBO_EXPORT void MM_SetFaroreSyncOot(ComboFaroreSyncOotPoint sync) {sSyncOot=sync;}
extern "C" COMBO_EXPORT int MM_GetFaroreMarker(int fileNum,int age,int* marker,int* entrance) {
 if(!marker||!entrance||fileNum<0||fileNum>2||fileNum!=gSaveContext.fileNum||!ValidAge(age))return 0;
 const auto& p=State().point[age];*marker=p.data;*entrance=p.entrance;return 1;
}
extern "C" COMBO_EXPORT int MM_SyncFaroreFromOot(int fileNum,int age,int marker,int entrance) {
 if(!sCrossGame||fileNum<0||fileNum>2||fileNum!=gSaveContext.fileNum||!ValidAge(age))return 0;
 auto& foreign=State().point[age];
 // common/item/item.c SyncPoint, named native fields only.
 if(marker<=0||entrance!=COMBO_FARORE_CROSS_ENTRANCE)foreign.data=0;
 if(marker>0&&foreign.data<=0) {foreign.data=1;foreign.entrance=COMBO_FARORE_CROSS_ENTRANCE;}
 SaveManager_SaveCurrentForCombo();return 1;
}
extern "C" void Combo_MMFaroreGrantItem() {
 // Pinned item_add.c addItemTradeMm2 preserves occupied Room Key/Mama slots.
 State().owned=1;
 auto& slot=gSaveContext.save.saveInfo.inventory.items[SLOT_TRADE_KEY_MAMA];
 if(slot==ITEM_NONE)slot=ITEM_COMBO_FARORES_WIND;
}
extern "C" int Combo_MMFaroreEffectiveHumanAge() {return NativeAge();}
extern "C" int Combo_MMFaroreBridgeReady() {return sOotAge&&sOotDestination&&sSyncOot&&sNativeAge&&sNativeAgeMode&&sCanAge&&sApplyAge&&sNativeDepartureCommit&&sReadDungeon&&sWriteDungeon;}
extern "C" int Combo_MMFaroreEnabled() {return ComboRando::ComboFaroreIntegrationReady&&gSaveContext.fileNum>=0&&gSaveContext.fileNum<=2&&gSaveContext.save.shipSaveInfo.saveType==SAVETYPE_RANDO&&sEnabled&&State().owned;}
extern "C" int Combo_MMFaroreAge() {
 // comboMmFwAge: cross-age, cross-game, adult-mask, then OoT age.
 int age;
 if(sCrossAge)age=NativeAge();
 else if(sCrossGame)age=sOotAge?sOotAge(gSaveContext.fileNum):-1;
 else {
  const int mode=sNativeAgeMode?sNativeAgeMode(gSaveContext.fileNum):int(sAdultMask);
  if(mode!=0&&mode!=1)return -1;
  age=mode?NativeAge():(sOotAge?sOotAge(gSaveContext.fileNum):-1);
 }
 return ValidAge(age)?age:-1;
}
extern "C" COMBO_EXPORT int Combo_MMFaroreRefreshHumanRespawn() {
 const int age=Combo_MMFaroreAge();
 if(gSaveContext.fileNum<0||gSaveContext.fileNum>2||!ValidAge(age))return 0;
 gSaveContext.respawn[RESPAWN_MODE_HUMAN]=State().point[age];return 1;
}
extern "C" COMBO_EXPORT int MM_GetFaroreDestination(int fileNum,int age,int* entrance) {
 if(!entrance||fileNum<0||fileNum>2||fileNum!=gSaveContext.fileNum||!ValidAge(age)||(sCrossAge&&!CanAge(age)))return 0;
 const auto& fw=State().point[age];
 if(fw.data<=0||fw.entrance==COMBO_FARORE_CROSS_ENTRANCE)return 0;
 *entrance=fw.entrance;return 1;
}
extern "C" int Combo_MMFaroreHasValidPoint() {
 int age=Combo_MMFaroreAge();if(!ValidAge(age))return 0;
 auto& fw=State().point[age];
 if(fw.data<=0)return 0;
 if(fw.entrance!=COMBO_FARORE_CROSS_ENTRANCE)return 1;
 int entrance=-1;
 if(sCrossGame&&sOotDestination&&sOotDestination(gSaveContext.fileNum,age,&entrance)&&
    entrance>=0&&entrance!=COMBO_FARORE_CROSS_ENTRANCE)return 1;
 fw.data=0;gSaveContext.respawn[RESPAWN_MODE_HUMAN].data=0;return 0;
}
extern "C" void Combo_MMFaroreStorePoint(PlayState* play) {
 const int age=Combo_MMFaroreAge();if(!ValidAge(age))return;
 gSaveContext.respawn[RESPAWN_MODE_HUMAN].data=1;
 Play_SetupRespawnPoint(play,RESPAWN_MODE_HUMAN,0x6ff);
 auto& fw=State().point[age];fw=gSaveContext.respawn[RESPAWN_MODE_DOWN];
 fw.playerParams=0x6ff;fw.data=40;
 State().top[age]=gSaveContext.respawn[RESPAWN_MODE_TOP];
 ReadDungeon();State().dungeon[age]=State().currentDungeon;State().sharedDungeon[age]=State().sharedCurrentDungeon;
 SyncOot(age);
}
extern "C" int Combo_MMFaroreReturn(PlayState* play) {
 const int age=Combo_MMFaroreAge();if(!ValidAge(age)||!Combo_MMFaroreHasValidPoint())return 0;
 auto& fw=State().point[age];
 if(fw.entrance==COMBO_FARORE_CROSS_ENTRANCE) {
  int entrance=-1;
  if(!sOotDestination||!sOotDestination(gSaveContext.fileNum,age,&entrance)||entrance<0||
     entrance==COMBO_FARORE_CROSS_ENTRANCE)return 0;
  sDepartureAge=age;sDepartureEntrance=entrance;
  play->transitionTrigger=TRANS_TRIGGER_OFF;return 1;
 }
 gSaveContext.respawnFlag=8;play->transitionTrigger=TRANS_TRIGGER_START;
 play->nextEntrance=gSaveContext.respawn[RESPAWN_MODE_HUMAN].entrance;
 play->transitionType=TRANS_TYPE_FADE_WHITE_FAST;
 gSaveContext.respawn[RESPAWN_MODE_TOP]=State().top[age];
 State().currentDungeon=State().dungeon[age];State().sharedCurrentDungeon=State().sharedDungeon[age];WriteDungeon();return 1;
}
extern "C" void Combo_MMFaroreDispel() {
 const int age=Combo_MMFaroreAge();if(!ValidAge(age))return;
 auto& fw=State().point[age];
 gSaveContext.respawn[RESPAWN_MODE_HUMAN].data=-gSaveContext.respawn[RESPAWN_MODE_HUMAN].data;
 fw.data=0;Audio_PlaySfx_AtPos(&fw.pos,NA_SE_PL_MAGIC_WIND_VANISH);
 SyncOot(age);
}
extern "C" COMBO_EXPORT void MM_ConfigureSharedDungeonRules(int enabled) {sSharedDungeonEnabled=enabled!=0;}
extern "C" COMBO_EXPORT void MM_SetSharedDungeonBridge(ComboReadSharedDungeon read,ComboWriteSharedDungeon write) {sReadDungeon=read;sWriteDungeon=write;}
extern "C" COMBO_EXPORT int MM_TakeDungeonDeparture(int* age,int* entrance) {
 if(!age||!entrance||sDungeonDepartureAge<0||sDungeonDepartureEntrance<0)return 0;
 *age=sDungeonDepartureAge;*entrance=sDungeonDepartureEntrance;sDungeonDepartureAge=sDungeonDepartureEntrance=-1;return 1;
}
extern "C" COMBO_EXPORT int MM_SetDungeonArrival(int fileNum,int age,int entrance) {
 sDungeonArrivalFile=sDungeonArrivalAge=sDungeonArrivalEntrance=-1;
 if(fileNum<0||fileNum>2||age<0||age>1||entrance<0||entrance==0xfffd)return 0;
 sDungeonArrivalFile=fileNum;sDungeonArrivalAge=age;sDungeonArrivalEntrance=entrance;return 1;
}
extern "C" COMBO_EXPORT int MM_ConsumeDungeonArrival() {
 const int entrance=sDungeonArrivalEntrance,file=sDungeonArrivalFile;
 sDungeonArrivalFile=sDungeonArrivalAge=sDungeonArrivalEntrance=-1;
 if(file!=gSaveContext.fileNum||file<0||file>2||entrance<0)return -1;
 ReadDungeon();const auto& d=State().sharedCurrentDungeon;
 if(!d.valid||d.game!=1||!d.playerParams||d.entrance!=entrance)return -1;
 ComboFarore::ApplyMmDungeon(d,gSaveContext.respawn[RESPAWN_MODE_TOP]);gSaveContext.respawnFlag=-6;
 return entrance;
}
extern "C" COMBO_EXPORT int MM_TakeFaroreDeparture(int* age,int* entrance) {
 if(!age||!entrance||!ValidAge(sDepartureAge)||sDepartureEntrance<0)return 0;
 *age=sDepartureAge;*entrance=sDepartureEntrance;sDepartureAge=sDepartureEntrance=-1;return 1;
}
extern "C" COMBO_EXPORT void MM_SetNativeDepartureCommit(ComboNativeDepartureCommit commit) {sNativeDepartureCommit=commit;}
extern "C" COMBO_EXPORT void MM_CancelNativeDeparture() {
 sDepartureAge=sDepartureEntrance=sDungeonDepartureAge=sDungeonDepartureEntrance=-1;
}
extern "C" COMBO_EXPORT int MM_DispatchNativeDeparture(PlayState* play) {
 const bool dungeon=sDungeonDepartureAge>=0&&sDungeonDepartureEntrance>=0;
 const int age=dungeon?sDungeonDepartureAge:sDepartureAge;
 const int entrance=dungeon?sDungeonDepartureEntrance:sDepartureEntrance;
 const bool accepted=age>=0&&age<=1&&entrance>=0&&sNativeDepartureCommit&&
  sNativeDepartureCommit(gSaveContext.fileNum,0,dungeon?COMBO_ARRIVAL_DUNGEON:COMBO_ARRIVAL_FARORE,age,entrance);
 MM_CancelNativeDeparture();
 if(!accepted&&play) {
  if(dungeon)play->transitionTrigger=TRANS_TRIGGER_START; // retain the displaced native death/Soar fallback
  else Combo_MMFaroreCancelReturn(play); // textbox closed; release the native spell action/camera
 }
 return accepted;
}
extern "C" COMBO_EXPORT int MM_FaroreDeparturePending() {return (ValidAge(sDepartureAge)&&sDepartureEntrance>=0)||(sDungeonDepartureAge>=0&&sDungeonDepartureEntrance>=0);}
extern "C" COMBO_EXPORT int MM_SetFaroreArrivalEntrance(int fileNum,int age,int entrance) {
 sArrivalFile=sArrivalAge=sArrivalEntrance=sSpawnAge=-1;
 if(fileNum<0||fileNum>2||!ValidAge(age)||entrance<0||entrance==COMBO_FARORE_CROSS_ENTRANCE)return 0;
 sArrivalFile=fileNum;sArrivalAge=age;sArrivalEntrance=entrance;return 1;
}
extern "C" COMBO_EXPORT int MM_SetFaroreArrival(int fileNum,int age) {
 if(fileNum<0||fileNum>2||!ValidAge(age))return 0;
 int entrance;if(!MM_GetFaroreDestination(fileNum,age,&entrance))return 0;
 return MM_SetFaroreArrivalEntrance(fileNum,age,entrance);
}
extern "C" COMBO_EXPORT int MM_ConsumeFaroreArrival() {
 if(sArrivalFile!=gSaveContext.fileNum||sArrivalFile<0||sArrivalFile>2||!ValidAge(sArrivalAge)) {
  sArrivalFile=sArrivalAge=sArrivalEntrance=sSpawnAge=-1;return -1;
 }
 const int entrance=sArrivalEntrance;
 if(sCrossAge) {
  // Adult callback is registered only by a complete, resource-ready native implementation.
  if(!CanAge(sArrivalAge)||(sApplyAge?!sApplyAge(gSaveContext.fileNum,sArrivalAge):NativeAge()!=sArrivalAge)) {
   sArrivalFile=sArrivalAge=sArrivalEntrance=sSpawnAge=-1;return -1;
  }
 }
 sSpawnAge=sArrivalAge;
 // The typed arrival must precede Play_Init's room/temporary flag selection.
 Combo_MMFaroreBeforePlayerInit(nullptr);
 return entrance;
}
extern "C" void Combo_MMFaroreBeforePlayerInit(PlayState*) {
 if(sArrivalFile==gSaveContext.fileNum&&ValidAge(sArrivalAge)) {
  const auto& fw=State().point[sArrivalAge];
  gSaveContext.respawnFlag=8;
  gSaveContext.respawn[RESPAWN_MODE_TOP]=State().top[sArrivalAge];
  State().currentDungeon=State().dungeon[sArrivalAge];State().sharedCurrentDungeon=State().sharedDungeon[sArrivalAge];WriteDungeon();
  if(fw.data)gSaveContext.respawn[RESPAWN_MODE_HUMAN]=fw;
  else {auto& r=gSaveContext.respawn[RESPAWN_MODE_HUMAN];r.data=0;r.pos={0,0,0};}
 }
 sArrivalFile=sArrivalAge=sArrivalEntrance=-1;
}
// Pinned OoTMM mm/play/play.c hookPlay_Init (41a2b7c0...), before room/player init.
extern "C" void Combo_MMFarorePrepareArrival() {
 const int fwAge=ValidAge(sSpawnAge)?sSpawnAge:Combo_MMFaroreAge();
 sSpawnAge=-1;
 if(gSaveContext.respawnFlag!=8||gSaveContext.fileNum<0||gSaveContext.fileNum>2||!ValidAge(fwAge))return;
 const s32 fwSceneId=Entrance_GetSceneIdAbsolute(gSaveContext.respawn[RESPAWN_MODE_HUMAN].entrance);
 Vec3f* pos=&gSaveContext.respawn[RESPAWN_MODE_HUMAN].pos;
 if(fwSceneId==SCENE_11GORONNOSATO2||fwSceneId==SCENE_11GORONNOSATO) {
  if(CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)&&pos->x>1100.0f) {
   // The Lens Cave ledge is absent in the cleared village: use the pinned owl-side position.
   pos->x=1189.0f;pos->z=-911.0f;
  }
 } else if(fwSceneId==SCENE_21MITURINMAE) {
  if(!CHECK_WEEKEVENTREG(WEEKEVENTREG_20_01)&&ABS(pos->z)<500.0f) {
   if(pos->z>0) {pos->x=1.0f;pos->y=200.0f;pos->z=1094.0f;}
   else {pos->x=-41.0f;pos->y=12.0f;pos->z=-1353.0f;}
  }
 }
 // The native pointer init copies this point back into HUMAN; retain the adjusted position there.
 State().point[fwAge].pos=*pos;
}
extern "C" void Combo_MMFaroreCaptureDungeon(PlayState* play) {
 if(!sEnabled&&!sSharedDungeonEnabled)return;
 ReadDungeon();
    switch (play->sceneId)
    {
    case SCENE_MITURIN:
    case SCENE_HAKUGIN:
    case SCENE_SEA:
    case SCENE_INISIE_N:
    case SCENE_INISIE_R:
    case SCENE_KINSTA1:
    case SCENE_KINDAN2:
    case SCENE_REDEAD:
    case SCENE_TORIDE:
    case SCENE_KAIZOKU:
    case SCENE_PIRATE:
    case SCENE_CASTLE:
    case SCENE_RANDOM:
    case SCENE_SOUGEN:
    case SCENE_LAST_DEKU:
    case SCENE_LAST_GORON:
    case SCENE_LAST_ZORA:
    case SCENE_LAST_LINK:
    case SCENE_MITURIN_BS:
    case SCENE_HAKUGIN_BS:
    case SCENE_SEA_BS:
    case SCENE_INISIE_BS:
    case SCENE_IKNINSIDE:
    case SCENE_LAST_BS:
        if (!State().sharedCurrentDungeon.playerParams)
        {
            /* Copy to the custom death respawn */
            State().currentDungeon = gSaveContext.respawn[RESPAWN_MODE_DOWN];

            /* 0x0X = OoT, 0x8X = MM */
            State().currentDungeon.data |= 0x80;
            State().sharedCurrentDungeon=ComboFarore::MmDungeon(State().currentDungeon,NativeAge(),gSaveContext.save.playerForm);
        }
        break;
    case SCENE_SPOT00:
        break;
    case SCENE_00KEIKOKU:
        if (gSaveContext.save.entrance == 0x54c0)
            break;
        /* Fallthrough */
    default:
        State().currentDungeon = {};State().sharedCurrentDungeon={};
        break;
    }
 WriteDungeon();
}
extern "C" void Combo_MMFaroreDeathWarp(PlayState* play) {
 if(!sEnabled&&!sSharedDungeonEnabled)return;
 // Pinned save.c DeathWarpWrapper, same-engine full typed restore.
 ReadDungeon();const auto& shared=State().sharedCurrentDungeon;
 if(shared.valid&&shared.playerParams) {
  if(shared.game!=1) {sDungeonDepartureAge=shared.age;sDungeonDepartureEntrance=shared.entrance;play->transitionTrigger=TRANS_TRIGGER_OFF;return;}
  ComboFarore::ApplyMmDungeon(shared,State().currentDungeon);
 }
 const auto& dungeon=State().currentDungeon;
 if(dungeon.playerParams&&(dungeon.data&0x80)) {
  gSaveContext.respawn[RESPAWN_MODE_TOP]=dungeon;
  gSaveContext.respawn[RESPAWN_MODE_TOP].data&=0x7f;
  play->nextEntrance=dungeon.entrance;
 }
 // A foreign shared dungeon ingress needs the launcher's separate respawn seam.
}
extern "C" void Combo_MMFaroreEndCycle() {State().currentDungeon={};State().sharedCurrentDungeon={};WriteDungeon();}
extern "C" int Combo_MMFarorePrepareResources() {
 try {
  auto oot=Ship::CrossRMRegistry::Get("oot"),mm=Ship::CrossRMRegistry::Get("mm");if(!oot||!mm)return 0;
  bool cached=sOwners[0]&&sOotResources==oot&&sMmResources==mm&&sOotAlt==oot->IsAltAssetsEnabled()&&sMmAlt==mm->IsAltAssetsEnabled();
  for(size_t i=0;cached&&i<sOwners.size();++i)cached=ComboNativeSpells::Valid(sOwners[i],sPaths[i],sFrameOwners[i]);
  if(cached)return 1;
  std::array<std::shared_ptr<Ship::IResource>,10> owners,frames;
  for(size_t i=0;i<owners.size();++i) {
   owners[i]=ComboNativeSpells::Load((i>=1&&i<=4)?mm:oot,sPaths[i],frames[i]);if(!owners[i])return 0;
  }
  sOotAlt=oot->IsAltAssetsEnabled();sMmAlt=mm->IsAltAssetsEnabled();
  sOwners=std::move(owners);sFrameOwners=std::move(frames);sOotResources=std::move(oot);sMmResources=std::move(mm);
  Ship::CrossRMRegistry::RegisterTeardownListener(DropResources);return 1;
 }catch(const std::exception&){return 0;}
}
extern "C" const char* Combo_MMFaroreItemName() {
 if(!Combo_MMFarorePrepareResources())return gEmptyTexture;
 return sComboFaroreItemName;
}
extern "C" void* Combo_MMFaroreTexture() {
 return Combo_MMFarorePrepareResources()?sOwners[0]->GetRawPointer():nullptr;
}
#include "CrossEntrance.inc.h"
#endif
