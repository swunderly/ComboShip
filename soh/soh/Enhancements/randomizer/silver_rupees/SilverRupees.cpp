// Adapted from OoTMM 41a2b7c0d7dbbb53fed928652f8271bc336c113f; full MIT notice in LICENSE.
#include "SilverRupees.h"
#include "global.h"
#include "soh/frame_interpolation.h"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/logic.h"
#include "soh/Enhancements/randomizer/RCToRandInf.h"
#include "soh/Enhancements/randomizer/randomizer_check_tracker.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "overlays/actors/ovl_En_G_Switch/z_en_g_switch.h"
#include "objects/object_gi_rupy/object_gi_rupy.h"
#include "objects/object_gi_purse/object_gi_purse.h"
#include <algorithm>
#include <map>

namespace Rando::SilverRupees {
struct NativeItem { RandomizerGet item; uint8_t slot; bool pouch; RandomizerSettingKey setting; const char* name; };
const NativeItem nativeItems[] = {
#define SR_ITEM(item,slot,pouch,setting,name) {item,slot,pouch,setting,name},
#include "SilverRupeeData.inc"
#undef SR_ITEM
};
struct NativeCheck { RandomizerCheck check; RandomizerInf inf; uint8_t id; RandomizerGet item; SceneID scene; RandomizerCheckQuest quest; const char* name; };
const NativeCheck nativeChecks[] = {
#define SR_CHECK(check,inf,id,item,scene,quest,name) {check,inf,id,item,scene,quest,name},
#include "SilverRupeeData.inc"
#undef SR_CHECK
};
unsigned roomSpawnIndex=0;
const NativeItem* FindItem(RandomizerGet item) { for(const auto& row:nativeItems)if(row.item==item)return &row;return nullptr; }
const NativeCheck* FindCheck(RandomizerCheck check) { for(const auto& row:nativeChecks)if(row.check==check)return &row;return nullptr; }
bool IsCheck(RandomizerCheck check) { return FindCheck(check)!=nullptr; }
bool IsShuffledCheck(RandomizerCheck check) {
    const auto ctx = Context::GetInstance();
    // Native save load restores shuffle/layout settings without rebuilding the
    // generation-only location pool. Use those saved rules for tracker visibility.
    return IsCheck(check) && ctx->GetOption(RSK_SHUFFLE_SILVER_RUPEES).Get() != 0 &&
           ctx->IsQuestOfLocationActive(check);
}
bool IsItem(RandomizerGet item) { return FindItem(item)!=nullptr; }
int PuzzleForItem(RandomizerGet item) { auto row=FindItem(item);return row ? row->slot : -1; }
bool IsMQ(size_t slot) {
    auto dungeon=Context::GetInstance()->GetDungeons()->GetDungeonFromScene(static_cast<SceneID>(Vanilla.at(slot).scene));
    return dungeon && dungeon->IsMQ();
}
bool HasPuzzle(Logic* logic,size_t slot) {
    const auto puzzle=GetPuzzle(slot,IsMQ(slot));
    return puzzle.count && logic->GetSaveContext()->ship.quest.data.randomizer.silverRupeeCounts[slot]>=puzzle.count;
}
void LogicDelta(Logic* logic,RandomizerGet item,bool add) {
    auto row=FindItem(item);if(!row)return;
    const auto amount=row->pouch ? GetPuzzle(row->slot,IsMQ(row->slot)).count : 1;
    auto& count=logic->GetSaveContext()->ship.quest.data.randomizer.silverRupeeCounts[row->slot];
    // Oracle ownership stays additive so RemoveItem reverses over-cap/duplicate
    // grants exactly. Runtime storage below alone saturates at puzzle maximum.
    count=add ? count+amount : (count>=amount ? count-amount : 0);
}
}
// The native display macros declare interpolation callbacks in their enclosing
// namespace. Keep the draw callback in global scope, as native item drawers do.
static void DrawSilver(PlayState* play,GetItemEntry* entry) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++,MATRIX_NEWMTX(play->state.gfxCtx),G_MTX_MODELVIEW|G_MTX_LOAD);
    const auto* row=entry ? Rando::SilverRupees::FindItem(static_cast<RandomizerGet>(entry->drawItemId)) : nullptr;
    if(row && row->pouch) {
        // Pinned DrawGi_Wallet: gray body, normal string, silver rupee emblem.
        gDPSetPrimColor(POLY_OPA_DISP++,0,0,50,50,50,255);
        gDPSetEnvColor(POLY_OPA_DISP++,15,15,15,255);
        gSPDisplayList(POLY_OPA_DISP++,(Gfx*)gGiWalletDL);
        gDPSetPrimColor(POLY_OPA_DISP++,0,0,255,255,255,255);
        gDPSetEnvColor(POLY_OPA_DISP++,50,50,50,255);
        gSPDisplayList(POLY_OPA_DISP++,(Gfx*)gGiWalletRupeeOuterDL);
        gSPDisplayList(POLY_OPA_DISP++,(Gfx*)gGiAdultWalletStringColorDL);
        gSPDisplayList(POLY_OPA_DISP++,(Gfx*)gGiWalletStringDL);
        gDPSetPrimColor(POLY_OPA_DISP++,0,0,255,255,255,255);
        gDPSetEnvColor(POLY_OPA_DISP++,50,50,50,255);
        gSPDisplayList(POLY_OPA_DISP++,(Gfx*)gGiWalletRupeeInnerDL);
    } else {
    gDPSetPrimColor(POLY_OPA_DISP++,0,0x80,255,255,255,255);
    gDPSetEnvColor(POLY_OPA_DISP++,50,50,50,255);
    gSPDisplayList(POLY_OPA_DISP++,(Gfx*)gGiRupeeInnerDL);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPMatrix(POLY_XLU_DISP++,MATRIX_NEWMTX(play->state.gfxCtx),G_MTX_MODELVIEW|G_MTX_LOAD);
    gDPSetPrimColor(POLY_XLU_DISP++,0,0x80,255,255,255,255);
    gDPSetEnvColor(POLY_XLU_DISP++,50,50,50,255);
    gSPDisplayList(POLY_XLU_DISP++,(Gfx*)gGiRupeeOuterDL);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}
namespace Rando::SilverRupees {
void RegisterItems(std::array<Item,RG_MAX>& items) {
    for(const auto& row:nativeItems) {
        items[row.item]=Item(row.item,Text{row.name,row.name,row.name},ITEMTYPE_ITEM,row.item,true,LOGIC_NONE,RHT_NONE,
                            row.item,row.pouch ? OBJECT_GI_PURSE : OBJECT_GI_RUPY,
                            row.pouch ? GID_WALLET_ADULT : GID_RUPEE_BLUE,TEXT_RANDOMIZER_CUSTOM_ITEM,0x80,CHEST_ANIM_LONG,
                            ITEM_CATEGORY_MAJOR,MOD_RANDOMIZER);
        items[row.item].SetCustomDrawFunc(DrawSilver);
    }
}
void RegisterLocations(std::array<Location,RC_MAX>& locations) {
    for(const auto& row:nativeChecks) {
        locations[row.check]=Location::Base(row.check,row.quest,RCTYPE_STANDARD,ACTOR_EN_G_SWITCH,row.scene,
                                            row.id,std::string(row.name),std::string(row.name),RHT_NONE,row.item,
                                            SpoilerCollectionCheck::RandomizerInf(row.inf));
        rcToRandomizerInf[row.check]=row.inf;
    }
}
void AddPool(std::vector<RandomizerGet>& pool) {
    auto ctx=Context::GetInstance();
    if(!ctx->GetOption(RSK_SHUFFLE_SILVER_RUPEES).Get())return;
    std::map<RandomizerGet,unsigned> counts;
    for(auto check:ctx->allLocations)if(auto row=FindCheck(check))++counts[row->item];
    for(const auto& [item,count]:counts) {
        auto row=FindItem(item);
        if(ctx->GetOption(row->setting).Get()) {
            auto pouch=std::find_if(std::begin(nativeItems),std::end(nativeItems),[&](const auto& other){return other.pouch && other.setting==row->setting;});
            pool.push_back(pouch->item);
            pool.insert(pool.end(),count-1,RG_BLUE_RUPEE);
        } else pool.insert(pool.end(),count,item);
    }
}
void DrawPhysical(Actor* actor,PlayState* play) {
    auto silver=reinterpret_cast<EnGSwitch*>(actor);
    const NativeCheck* check=nullptr;
    for(const auto& row:nativeChecks)if(row.id==silver->silverRandoId) {check=&row;break;}
    if(!check)return;
    func_8002EBCC(actor,play,0);func_8002ED80(actor,play,0);
    Matrix_Scale(12.5f,12.5f,12.5f,MTXMODE_APPLY);
    auto entry=OTRGlobals::Instance->gRandomizer->GetItemFromKnownCheck(check->check,GI_RUPEE_BLUE);
#ifdef COMBO_BUILD
    CheckTracker::RecordObservedItem(actor,entry);
#endif
    GetItemEntry_Draw(play,entry);
}
}
extern "C" int Combo_SilverShuffleEnabled() {
    return IS_RANDO && Rando::Context::GetInstance()->GetOption(RSK_SHUFFLE_SILVER_RUPEES).Get()!=0;
}
extern "C" void Combo_SilverResetRoom() { Rando::SilverRupees::roomSpawnIndex=0; }
extern "C" int Combo_SilverActorInit(void* actor,void* context) {
    using namespace Rando::SilverRupees;
    auto silver=static_cast<EnGSwitch*>(actor);auto play=static_cast<PlayState*>(context);
    if(!Combo_SilverShuffleEnabled() || silver->type!=ENGSWITCH_SILVER_RUPEE)return 0;
    auto dungeon=Rando::Context::GetInstance()->GetDungeons()->GetDungeonFromScene(static_cast<SceneID>(play->sceneNum));
    const auto id=PhysicalId(play->sceneNum,silver->actor.room,roomSpawnIndex++,dungeon && dungeon->IsMQ());
    if(!id)return 0;
    const NativeCheck* check=nullptr;
    for(const auto& row:nativeChecks)if(row.id==*id){check=&row;break;}
    if(!check)return 0;
    silver->silverRandoId=*id;silver->silverRandomized=1;
    if(Flags_GetRandomizerInf(check->inf)){Actor_Kill(&silver->actor);return 1;}
    silver->actor.draw=DrawPhysical;
    return 1;
}
extern "C" int Combo_SilverActorIdle(void* actor,void* context) {
    using namespace Rando::SilverRupees;
    auto silver=static_cast<EnGSwitch*>(actor);auto play=static_cast<PlayState*>(context);
    if(!silver->silverRandomized)return 0;
    for(const auto& row:nativeChecks)if(row.id==silver->silverRandoId) {
        if(Actor_HasParent(&silver->actor,play)) {
            Flags_SetRandomizerInf(row.inf);Actor_Kill(&silver->actor);
        } else GiveItemEntryFromActor(&silver->actor,play,
                     OTRGlobals::Instance->gRandomizer->GetItemFromKnownCheck(row.check,GI_RUPEE_BLUE),30.0f,50.0f);
        break;
    }
    return 1;
}
extern "C" int Combo_SilverGive(void* context,int item) {
    using namespace Rando::SilverRupees;
    auto row=FindItem(static_cast<RandomizerGet>(item));if(!row)return 0;
    auto play=static_cast<PlayState*>(context);
    ReceivedState received;
    auto& counts=gSaveContext.ship.quest.data.randomizer.silverRupeeCounts;
    for(size_t i=0;i<PuzzleCount;++i)received.counts[i]=std::min<uint16_t>(counts[i],255);
    auto result=Receive(received,row->slot,IsMQ(row->slot),row->pouch,
                        play ? std::optional<uint8_t>(play->sceneNum) : std::nullopt);
    counts[row->slot]=result.count;
    Rupees_ChangeBy(result.rupees);
    auto puzzle=GetPuzzle(row->slot,IsMQ(row->slot));
    if(result.permanentSwitch)gSaveContext.sceneFlags[puzzle.scene].swch|=1u<<puzzle.flag;
    return 1;
}
extern "C" void Combo_SilverUpdate(void* context) {
    using namespace Rando::SilverRupees;
    auto play=static_cast<PlayState*>(context);
    if(!play || !Combo_SilverShuffleEnabled() || gSaveContext.gameMode!=GAMEMODE_NORMAL)return;
    auto link=GET_PLAYER(play);
    if(Message_GetState(&play->msgCtx)!=TEXT_STATE_NONE ||
       (link->stateFlags1&(PLAYER_STATE1_CLIMBING_LEDGE|PLAYER_STATE1_CLIMBING_LADDER)))return;
    const auto& counts=gSaveContext.ship.quest.data.randomizer.silverRupeeCounts;
    for(size_t i=0;i<PuzzleCount;++i) {
        auto puzzle=GetPuzzle(i,IsMQ(i));
        if(puzzle.count && puzzle.scene==play->sceneNum && counts[i]>=puzzle.count) {
            Flags_SetSwitch(play,puzzle.flag);
            if(i==11 && play->roomCtx.curRoom.num==2)Flags_SetTempClear(play,2);
        }
    }
}
