#ifdef COMBO_BUILD
#include "FaroresWind.h"
#include "rando/SharedDungeonJson.h"
#include "ComboExport.h"
#include "soh/SaveManager.h"
#include "rando/NativeFaroreArrivalAdapter.h"
#include <utility>
namespace {
ComboNativeDepartureCommit sNativeDepartureCommit=nullptr;
ComboReadSharedDungeon sReadDungeon=nullptr;
ComboWriteSharedDungeon sWriteDungeon=nullptr;
bool sSharedDungeonEnabled=false;
int sDungeonDepartureAge=-1,sDungeonDepartureEntrance=-1,sDungeonArrivalFile=-1,sDungeonArrivalAge=-1,sDungeonArrivalEntrance=-1;
void ReadDungeon() {
 ComboSharedDungeon d{};if(sReadDungeon&&sReadDungeon(gSaveContext.fileNum,&d))gSaveContext.ship.faroresWind.sharedCurrentDungeon=d;
}
void WriteDungeon() {
 if(sWriteDungeon)sWriteDungeon(gSaveContext.fileNum,&gSaveContext.ship.faroresWind.sharedCurrentDungeon);
}

constexpr int kCross=0xfffd;
bool sCross=false,sCrossAge=false;
ComboFaroreGetMmDestination sMmDestination=nullptr;
ComboFaroreSyncMmPoint sSyncMm=nullptr;
int sDepartureAge=-1,sDepartureEntrance=-1,sArrivalFile=-1,sArrivalAge=-1,sArrivalEntrance=-1;
bool ValidFile(int f) {return f>=0&&f<=2;}
bool ValidAge(int a) {return a>=0&&a<=1;}
FaroresWindData& Point(int age) {return age==gSaveContext.linkAge?gSaveContext.fw:gSaveContext.ship.backupFW;}
void SwapOotAgeEquipment() {
 ItemEquips* prevAge;
 ItemEquips* nextAge;
 if(gSaveContext.linkAge==0) {prevAge=&gSaveContext.adultEquips;nextAge=&gSaveContext.childEquips;}
 else {prevAge=&gSaveContext.childEquips;nextAge=&gSaveContext.adultEquips;}
 *prevAge=gSaveContext.equips;
 if(gSaveContext.infTable[29])prevAge->buttonItems[0]=ITEM_NONE;
 gSaveContext.equips=*nextAge;
 for(int i=0;i<3;++i) {
  u8 item=gSaveContext.equips.buttonItems[i+1];
  // Pinned extra trade-bottle bytes are reserved in the native OoT item schema.
  // This reload does not grant or register their unavailable item/action assets.
  if((item>=ITEM_BOTTLE&&item<=ITEM_POE)||(item>=0x9f&&item<=0xa6))
   item=gSaveContext.inventory.items[gSaveContext.equips.cButtonSlots[i]];
  gSaveContext.equips.buttonItems[i+1]=item;
 }
 gSaveContext.infTable[29]=gSaveContext.equips.buttonItems[0]==ITEM_NONE?1:0;
 const int shield=(gSaveContext.equips.equipment>>4)&0xf;
 if(shield&&!(gSaveContext.inventory.equipment&(1u<<(shield+3))))gSaveContext.equips.equipment&=0xff0f;
}
void SyncMm() {
 if(sCross&&sSyncMm&&ValidFile(gSaveContext.fileNum)&&ValidAge(gSaveContext.linkAge)) {
  const auto& p=gSaveContext.fw;sSyncMm(gSaveContext.fileNum,gSaveContext.linkAge,p.set,p.entranceIndex);
 }
}
nlohmann::json RespawnJson(const RespawnData& p) {
 return {{"pos",{{"x",p.pos.x},{"y",p.pos.y},{"z",p.pos.z}}},{"yaw",p.yaw},
 {"playerParams",p.playerParams},{"entranceIndex",p.entranceIndex},{"roomIndex",p.roomIndex},
 {"data",p.data},{"tempSwchFlags",p.tempSwchFlags},{"tempCollectFlags",p.tempCollectFlags}};
}
RespawnData ReadRespawn(const nlohmann::json& j) {
 RespawnData p{};j.at("pos").at("x").get_to(p.pos.x);j.at("pos").at("y").get_to(p.pos.y);
 j.at("pos").at("z").get_to(p.pos.z);j.at("yaw").get_to(p.yaw);j.at("playerParams").get_to(p.playerParams);
 j.at("entranceIndex").get_to(p.entranceIndex);j.at("roomIndex").get_to(p.roomIndex);
 j.at("data").get_to(p.data);j.at("tempSwchFlags").get_to(p.tempSwchFlags);
 j.at("tempCollectFlags").get_to(p.tempCollectFlags);return p;
}
}
void Combo_OotFaroreSave(SaveContext* save,int,bool) {
 const auto& fw=save->ship.faroresWind;
 nlohmann::json j={{"top",nlohmann::json::array()},{"dungeon",nlohmann::json::array()},
 {"currentDungeon",RespawnJson(fw.currentDungeon)},{"sharedDungeon",fw.sharedDungeon},{"sharedCurrentDungeon",fw.sharedCurrentDungeon}};
 for(int age=0;age<2;++age) {j["top"].push_back(RespawnJson(fw.top[age]));j["dungeon"].push_back(RespawnJson(fw.dungeon[age]));}
 SaveManager::Instance->SaveData("state",j);
}
void Combo_OotFaroreLoad() {
 nlohmann::json j;SaveManager::Instance->LoadData("state",j);
 ComboOotFaroreSaveInfo restored{};
 if(!j.is_null()) {
  if(j.at("top").size()!=2||j.at("dungeon").size()!=2)throw std::runtime_error("Invalid Farore age backups");
  for(int age=0;age<2;++age) {restored.top[age]=ReadRespawn(j.at("top").at(age));restored.dungeon[age]=ReadRespawn(j.at("dungeon").at(age));}
  restored.currentDungeon=ReadRespawn(j.at("currentDungeon"));
  if(j.contains("sharedDungeon"))j.at("sharedDungeon").get_to(restored.sharedDungeon);
  if(j.contains("sharedCurrentDungeon"))j.at("sharedCurrentDungeon").get_to(restored.sharedCurrentDungeon);
 }
 gSaveContext.ship.faroresWind=restored;
}
extern "C" COMBO_EXPORT void SOH_SetFaroreBridge(ComboFaroreGetMmDestination d,ComboFaroreSyncMmPoint s) {sMmDestination=d;sSyncMm=s;}
extern "C" COMBO_EXPORT void SOH_ConfigureFaroreRules(int cross,int age) {
 const bool crossGame=cross!=0,crossAge=age!=0;
 if(sCross==crossGame&&sCrossAge==crossAge)return;
 sCross=crossGame;sCrossAge=crossAge;
 sDepartureAge=sDepartureEntrance=sArrivalFile=sArrivalAge=sArrivalEntrance=-1;
 sDungeonDepartureAge=sDungeonDepartureEntrance=sDungeonArrivalFile=sDungeonArrivalAge=sDungeonArrivalEntrance=-1;
}
extern "C" int Combo_OotFaroreCrossEnabled() {return sCross;}
// Pinned Age_SetRawOot: saved equipment/FW-age swap only, without arrival or scene mutation.
extern "C" COMBO_EXPORT int SOH_CanApplySavedHumanAge(int fileNum,int age){
 return ValidFile(fileNum)&&ValidAge(age)&&gSaveContext.fileNum==fileNum&&ValidAge(gSaveContext.linkAge)&&SaveManager::Instance!=nullptr;
}
extern "C" COMBO_EXPORT int SOH_ApplySavedHumanAge(int fileNum,int age){
 if(!SOH_CanApplySavedHumanAge(fileNum,age))return 0;
 if(gSaveContext.linkAge==age)return 1;
 SaveManager::Instance->ThreadPoolWait();
 SwapOotAgeEquipment();std::swap(gSaveContext.fw,gSaveContext.ship.backupFW);gSaveContext.linkAge=age;
 return 1;
}
extern "C" COMBO_EXPORT int SOH_GetFaroreAge(int fileNum) {
 return ValidFile(fileNum)&&fileNum==gSaveContext.fileNum&&ValidAge(gSaveContext.linkAge)?gSaveContext.linkAge:-1;
}
extern "C" COMBO_EXPORT int SOH_GetFaroreMarker(int fileNum,int age,int* marker,int* entrance) {
 if(!marker||!entrance||SOH_GetFaroreAge(fileNum)<0||!ValidAge(age))return 0;
 const auto& p=Point(age);*marker=p.set;*entrance=p.entranceIndex;return 1;
}
extern "C" COMBO_EXPORT int SOH_GetFaroreDestination(int fileNum,int age,int* entrance) {
 int marker,entry;if(!entrance||!SOH_GetFaroreMarker(fileNum,age,&marker,&entry)||marker<=0||entry==kCross)return 0;
 *entrance=entry;return 1;
}
extern "C" COMBO_EXPORT int SOH_SyncFaroreFromMm(int fileNum,int age,int marker,int entrance) {
 if(!sCross||SOH_GetFaroreAge(fileNum)<0||!ValidAge(age))return 0;
 // Drain the native full-save worker before mutating the dormant SaveContext.
 SaveManager::Instance->ThreadPoolWait();
 auto& foreign=Point(age);
 if(marker<=0||entrance!=kCross)foreign.set=0;
 if(marker>0&&foreign.set<=0) {foreign.set=1;foreign.entranceIndex=kCross;}
 SaveManager::Instance->SaveFile(fileNum);SaveManager::Instance->ThreadPoolWait();return 1;
}
extern "C" void Combo_OotFaroreValidate() {
 if(!sCross||gSaveContext.fw.set<=0||gSaveContext.fw.entranceIndex!=kCross)return;
 int entrance;if(!sMmDestination||!sMmDestination(gSaveContext.fileNum,gSaveContext.linkAge,&entrance)) {
  gSaveContext.fw.set=0;gSaveContext.respawn[RESPAWN_MODE_TOP].data=0;
 }
}
extern "C" void Combo_OotFaroreStorePoint(PlayState*) {
 if(!sCross||!ValidAge(gSaveContext.linkAge))return;
 auto& state=gSaveContext.ship.faroresWind;const int age=gSaveContext.linkAge;
 ReadDungeon();state.top[age]=gSaveContext.respawn[RESPAWN_MODE_TOP];state.dungeon[age]=state.currentDungeon;state.sharedDungeon[age]=state.sharedCurrentDungeon;
 SyncMm();
}
extern "C" int Combo_OotFaroreReturn(PlayState* play) {
 const int age=gSaveContext.linkAge;if(!sCross||!ValidAge(age))return 0;
 if(sCross&&gSaveContext.fw.entranceIndex==kCross) {
  int entrance;if(!sMmDestination||!sMmDestination(gSaveContext.fileNum,age,&entrance))return 0;
  sDepartureAge=age;sDepartureEntrance=entrance;play->transitionTrigger=TRANS_TRIGGER_OFF;return 1;
 }
 gSaveContext.ship.faroresWind.currentDungeon=gSaveContext.ship.faroresWind.dungeon[age];gSaveContext.ship.faroresWind.sharedCurrentDungeon=gSaveContext.ship.faroresWind.sharedDungeon[age];WriteDungeon();return 0;
}
extern "C" void Combo_OotFaroreDispel() {SyncMm();}
extern "C" COMBO_EXPORT void SOH_SetNativeDepartureCommit(ComboNativeDepartureCommit commit) {sNativeDepartureCommit=commit;}
extern "C" COMBO_EXPORT void SOH_CancelNativeDeparture() {
 sDepartureAge=sDepartureEntrance=sDungeonDepartureAge=sDungeonDepartureEntrance=-1;
}
extern "C" COMBO_EXPORT int SOH_DispatchNativeDeparture(PlayState* play) {
 const bool dungeon=sDungeonDepartureAge>=0&&sDungeonDepartureEntrance>=0;
 const int age=dungeon?sDungeonDepartureAge:sDepartureAge;
 const int entrance=dungeon?sDungeonDepartureEntrance:sDepartureEntrance;
 const bool accepted=age>=0&&age<=1&&entrance>=0&&sNativeDepartureCommit&&
  sNativeDepartureCommit(gSaveContext.fileNum,1,dungeon?COMBO_ARRIVAL_DUNGEON:COMBO_ARRIVAL_FARORE,age,entrance);
 SOH_CancelNativeDeparture();
 if(!accepted&&play) {
  if(dungeon)play->transitionTrigger=TRANS_TRIGGER_START; // retain the displaced native death/Soar fallback
  else Combo_OotFaroreCancelReturn(play); // textbox closed; release the native spell action/camera
 }
 return accepted;
}
extern "C" COMBO_EXPORT int SOH_FaroreDeparturePending() {return (ValidAge(sDepartureAge)&&sDepartureEntrance>=0)||(sDungeonDepartureAge>=0&&sDungeonDepartureEntrance>=0);}
extern "C" COMBO_EXPORT void SOH_ConfigureSharedDungeonRules(int enabled) {sSharedDungeonEnabled=enabled!=0;}
extern "C" COMBO_EXPORT void SOH_SetSharedDungeonBridge(ComboReadSharedDungeon read,ComboWriteSharedDungeon write) {sReadDungeon=read;sWriteDungeon=write;}
extern "C" COMBO_EXPORT int SOH_TakeDungeonDeparture(int* age,int* entrance) {
 if(!age||!entrance||sDungeonDepartureAge<0||sDungeonDepartureEntrance<0)return 0;
 *age=sDungeonDepartureAge;*entrance=sDungeonDepartureEntrance;sDungeonDepartureAge=sDungeonDepartureEntrance=-1;return 1;
}
extern "C" COMBO_EXPORT int SOH_SetDungeonArrival(int fileNum,int age,int entrance) {
 sDungeonArrivalFile=sDungeonArrivalAge=sDungeonArrivalEntrance=-1;
 if(fileNum<0||fileNum>2||age<0||age>1||entrance<0||entrance==0xfffd)return 0;
 sDungeonArrivalFile=fileNum;sDungeonArrivalAge=age;sDungeonArrivalEntrance=entrance;return 1;
}
extern "C" COMBO_EXPORT int SOH_ConsumeDungeonArrival() {
 const int entrance=sDungeonArrivalEntrance,file=sDungeonArrivalFile;
 sDungeonArrivalFile=sDungeonArrivalAge=sDungeonArrivalEntrance=-1;
 if(file!=gSaveContext.fileNum||file<0||file>2||entrance<0)return -1;
 ReadDungeon();const auto& d=gSaveContext.ship.faroresWind.sharedCurrentDungeon;
 if(!d.valid||d.game!=0||!d.playerParams||d.entrance!=entrance)return -1;
 ComboFarore::ApplyOotDungeon(d,gSaveContext.respawn[RESPAWN_MODE_RETURN]);gSaveContext.respawnFlag=2;
 return entrance;
}
extern "C" COMBO_EXPORT int SOH_TakeFaroreDeparture(int* age,int* entrance) {
 if(!age||!entrance||!ValidAge(sDepartureAge)||sDepartureEntrance<0)return 0;
 *age=sDepartureAge;*entrance=sDepartureEntrance;sDepartureAge=sDepartureEntrance=-1;return 1;
}
extern "C" COMBO_EXPORT int SOH_SetFaroreArrivalEntrance(int fileNum,int age,int entrance) {
 sArrivalFile=sArrivalAge=sArrivalEntrance=-1;
 if(!ValidFile(fileNum)||!ValidAge(age)||entrance<0||entrance==kCross)return 0;
 sArrivalFile=fileNum;sArrivalAge=age;sArrivalEntrance=entrance;return 1;
}
extern "C" COMBO_EXPORT int SOH_ConsumeFaroreArrival() {
 const int age=sArrivalAge,file=sArrivalFile,entrance=sArrivalEntrance;
 sArrivalFile=sArrivalAge=sArrivalEntrance=-1;
 if(SOH_GetFaroreAge(file)<0||!ValidAge(age)||entrance<0)return -1;
 if(sCrossAge&&gSaveContext.linkAge!=age) {
  SwapOotAgeEquipment();std::swap(gSaveContext.fw,gSaveContext.ship.backupFW);gSaveContext.linkAge=age;
 }
 int isFwSpawn=1;
 ComboFarore::RestoreNativeOotArrival(Point(age),gSaveContext.ship.faroresWind.dungeon[age],
  gSaveContext.respawn[RESPAWN_MODE_TOP],gSaveContext.ship.faroresWind.currentDungeon,gSaveContext.respawnFlag,isFwSpawn);
 gSaveContext.ship.faroresWind.sharedCurrentDungeon=gSaveContext.ship.faroresWind.sharedDungeon[age];WriteDungeon();
 return entrance;
}
extern "C" void Combo_OotFaroreCaptureDungeon(PlayState* play) {
 if(!sCross&&!sSharedDungeonEnabled)return;
 ReadDungeon();
    switch (play->sceneNum)
    {
    case SCENE_DEKU_TREE:
    case SCENE_DODONGOS_CAVERN:
    case SCENE_JABU_JABU:
    case SCENE_FOREST_TEMPLE:
    case SCENE_FIRE_TEMPLE:
    case SCENE_WATER_TEMPLE:
    case SCENE_SPIRIT_TEMPLE:
    case SCENE_SHADOW_TEMPLE:
    case SCENE_BOTTOM_OF_THE_WELL:
    case SCENE_ICE_CAVERN:
    case SCENE_GANONS_TOWER:
    case SCENE_GERUDO_TRAINING_GROUND:
    case SCENE_INSIDE_GANONS_CASTLE:
    case SCENE_DEKU_TREE_BOSS:
    case SCENE_DODONGOS_CAVERN_BOSS:
    case SCENE_JABU_JABU_BOSS:
    case SCENE_FOREST_TEMPLE_BOSS:
    case SCENE_FIRE_TEMPLE_BOSS:
    case SCENE_WATER_TEMPLE_BOSS:
    case SCENE_SHADOW_TEMPLE_BOSS:
    case SCENE_SPIRIT_TEMPLE_BOSS:
    case SCENE_GANONDORF_BOSS:
    case SCENE_GANON_BOSS:
        if (!gSaveContext.ship.faroresWind.sharedCurrentDungeon.playerParams)
        {
            /* Copy to the custom death respawn */
            gSaveContext.ship.faroresWind.currentDungeon=gSaveContext.respawn[RESPAWN_MODE_DOWN];
            if (gSaveContext.entranceIndex == 0x3f0 || gSaveContext.entranceIndex == 0x3f4)
            {
                /* Copy pos, yaw, playerParams, entrance and roomIndex */
                auto& d=gSaveContext.ship.faroresWind.currentDungeon;
                d.pos={-5.0f,-150.0f,754.0f};d.yaw=-32768;d.playerParams=0xdff;
                d.entranceIndex=ENTR_SPIRIT_TEMPLE_ENTRANCE;d.roomIndex=0;
            }
        }
        if(!gSaveContext.ship.faroresWind.sharedCurrentDungeon.playerParams)gSaveContext.ship.faroresWind.sharedCurrentDungeon=ComboFarore::OotDungeon(gSaveContext.ship.faroresWind.currentDungeon,gSaveContext.linkAge);
        break;
    default:
        gSaveContext.ship.faroresWind.sharedCurrentDungeon={};
        gSaveContext.ship.faroresWind.currentDungeon={};
        break;
    }
 WriteDungeon();
}
extern "C" void Combo_OotFaroreDeathWarp(PlayState* play) {
 if(!sCross&&!sSharedDungeonEnabled)return;
 ReadDungeon();const auto& shared=gSaveContext.ship.faroresWind.sharedCurrentDungeon;
 if(shared.valid&&shared.playerParams) {
  if(shared.game!=0) {sDungeonDepartureAge=shared.age;sDungeonDepartureEntrance=shared.entrance;play->transitionTrigger=TRANS_TRIGGER_OFF;return;}
  ComboFarore::ApplyOotDungeon(shared,gSaveContext.ship.faroresWind.currentDungeon);
 }
 const auto& dungeon=gSaveContext.ship.faroresWind.currentDungeon;
 if(dungeon.playerParams&&!(dungeon.data&0x80)) {
  gSaveContext.respawn[RESPAWN_MODE_RETURN]=dungeon;gSaveContext.respawn[RESPAWN_MODE_DOWN]=dungeon;
  play->nextEntranceIndex=dungeon.entranceIndex;gSaveContext.respawnFlag=2;
 } else if(!dungeon.playerParams&&play->gameOverCtx.state==5)gSaveContext.respawnFlag=-2;
 // Foreign dungeon ingress requires the separate typed shared-dungeon bridge.
}
#include "CrossEntrance.inc.h"
#endif
