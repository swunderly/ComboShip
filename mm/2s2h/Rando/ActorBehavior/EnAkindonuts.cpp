#include "ActorBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/ShipUtils.h"
#include "2s2h/CustomMessage/CustomMessage.h"

extern "C" {
#include "variables.h"

#include "overlays/actors/ovl_En_Akindonuts/z_en_akindonuts.h"
void Flags_SetRandoInf(s32 flag);
void func_80BEF360(EnAkindonuts* enAkindonuts, PlayState* play);
void func_80BEEFA8(EnAkindonuts* enAkindonuts, PlayState* play);
}


#ifdef COMBO_BUILD
#include "RenewableStock.h"
#include "ComboExport.h"
#include "ComboRenewableStockABI.h"
#include "Rando/MiscBehavior/MiscBehavior.h"
#include "2s2h/CustomItem/CustomItem.h"
namespace Rando { bool DrawComboForeignRenewable(const char*); bool CanDrawComboForeignRenewable(const char*); }
static FnScrubStockQuery sScrubOwnerQuery = nullptr;
static FnScrubStockGrant sScrubOwnerGrant = nullptr;
extern "C" COMBO_EXPORT void MM_SetScrubStockBridge(FnScrubStockQuery query, FnScrubStockGrant grant) {
    sScrubOwnerQuery = query; sScrubOwnerGrant = grant;
}


static const RandoCheckId kScrubChecks[] = {RC_SOUTHERN_SWAMP_SCRUB_BEANS, RC_GORON_VILLAGE_SCRUB_BOMB_BAG,
    RC_ZORA_HALL_SCRUB_POTION_REFILL, RC_IKANA_CANYON_SCRUB_POTION_REFILL};
static const RandoInf kScrubFlags[] = {RANDO_INF_PURCHASED_BEANS_FROM_SOUTHERN_SWAMP_SCRUB,
    RANDO_INF_PURCHASED_BOMB_BAG_FROM_GORON_VILLAGE_SCRUB,
    RANDO_INF_PURCHASED_POTION_FROM_ZORA_HALL_SCRUB, RANDO_INF_PURCHASED_POTION_FROM_IKANA_CANYON_SCRUB};
static const int kScrubPrices[] = {10, 200, 40, 100};

// Pinned En_Akindonuts_ItemOverride gives the Goron vendor a recovery heart on every repeat.
static RandoItemId ScrubStock(unsigned id) {
    return id == 1 ? RI_RECOVERY_HEART : Rando::RenewableStockItem(RANDO_SAVE_CHECKS[kScrubChecks[id]].randoItemId);
}

static bool ScrubForeignStock(unsigned id, ComboRenewableStock& stock) {
    if (id == 1 || RANDO_SAVE_CHECKS[kScrubChecks[id]].randoItemId != RI_COMBO_FOREIGN ||
        !sScrubOwnerQuery || !sScrubOwnerGrant || gSaveContext.fileNum > 2) return false;
    const auto* foreign = Rando::MiscBehavior::MM_LookupForeign(kScrubChecks[id]);
    if (!foreign || foreign->itemGame != ComboRando::GAME_OOT || foreign->itemName.empty()) return false;
    if (!sScrubOwnerQuery(gSaveContext.fileNum, foreign->itemName.c_str(), &stock)) return false;
    if (stock.kind == 1) return true;
    if (stock.kind != 2 || !stock.name[0] || !stock.drawName[0] || stock.nativeGi <= 0 || (stock.buyable != 0 && stock.buyable != 1) ||
        (stock.fastBuy != 0 && stock.fastBuy != 1) ||
        !std::memchr(stock.name, 0, sizeof(stock.name)) || !std::memchr(stock.drawName, 0, sizeof(stock.drawName))) return false;
    if (!stock.fastBuy && !Rando::CanDrawComboForeignRenewable(stock.drawName)) stock.buyable = 0;
    return true;
}

// Supported native projection of pinned isItemBuyable/isItemLicensed. No permanent-check test:
// repeats consume stock, not another unique randomized check.
static bool ScrubStockBuyable(RandoItemId item) {
    const auto& p = gSaveContext.save.saveInfo.playerData;
    switch (item) {
        case RI_UNKNOWN: return false; // Foreign stock requires an owning-engine resolver.
        case RI_RECOVERY_HEART: return p.health < p.healthCapacity;
        case RI_MAGIC_JAR_SMALL: case RI_MAGIC_JAR_BIG:
            return p.isMagicAcquired && p.magic < (p.isDoubleMagicAcquired ? 0x60 : 0x30);
        case RI_DEKU_STICK: return AMMO(ITEM_DEKU_STICK) < CUR_CAPACITY(UPG_DEKU_STICKS);
        case RI_DEKU_NUT: case RI_DEKU_NUTS_5: case RI_DEKU_NUTS_10:
            return AMMO(ITEM_DEKU_NUT) < CUR_CAPACITY(UPG_DEKU_NUTS);
        case RI_BOMBS_5: case RI_BOMBS_10:
            return CUR_UPG_VALUE(UPG_BOMB_BAG) && AMMO(ITEM_BOMB) < CUR_CAPACITY(UPG_BOMB_BAG);
        case RI_BOMBCHU: case RI_BOMBCHU_5: case RI_BOMBCHU_10:
            // Native MM and the pinned default bombBag mode both clamp chus to the real Bomb Bag capacity.
            return INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU && AMMO(ITEM_BOMBCHU) < CUR_CAPACITY(UPG_BOMB_BAG);
        case RI_ARROWS_10: case RI_ARROWS_30: case RI_ARROWS_50:
            return CUR_UPG_VALUE(UPG_QUIVER) && AMMO(ITEM_BOW) < CUR_CAPACITY(UPG_QUIVER);
        case RI_MAGIC_BEAN:
            // Preserve the pinned inventory-slot comparison, not a new ammo-cap interpretation.
            return INV_CONTENT(ITEM_MAGIC_BEANS) == ITEM_MAGIC_BEANS && INV_CONTENT(ITEM_MAGIC_BEANS) < 20;
        case RI_SHIELD_HERO: return GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_NONE;
        case RI_RED_POTION_REFILL: case RI_GREEN_POTION_REFILL: case RI_BLUE_POTION_REFILL:
        case RI_FAIRY_REFILL: case RI_MILK_REFILL: case RI_CHATEAU_ROMANI_REFILL:
            return Inventory_HasEmptyBottle();
        default: return false;
    }
}

// -1 delegates first purchases and unshuffled vendors to the original native actor.
extern "C" int Combo_MM_ScrubRepeatPrecond(EnAkindonuts* actor) {
    if (!IS_RANDO || !actor) return -1;
    const unsigned id = ENAKINDONUTS_GET_3(&actor->actor);
    if(id>=4)return -1;
    // A local renewable bean never creates its own inventory license, even
    // on the first randomized purchase or an unshuffled randomizer vendor.
    if(RANDO_SAVE_CHECKS[kScrubChecks[id]].randoItemId==RI_MAGIC_BEAN&&INV_CONTENT(ITEM_MAGIC_BEANS)!=ITEM_MAGIC_BEANS)return 2;
    if(!RANDO_SAVE_CHECKS[kScrubChecks[id]].shuffled||!Flags_GetRandoInf(kScrubFlags[id]))return -1;
    bool buyable = ScrubStockBuyable(ScrubStock(id));
    if (id != 1 && RANDO_SAVE_CHECKS[kScrubChecks[id]].randoItemId == RI_COMBO_FOREIGN) {
        ComboRenewableStock stock{};
        buyable = ScrubForeignStock(id, stock) && (stock.kind == 1 ? ScrubStockBuyable(RI_RECOVERY_HEART) : stock.buyable);
    }
    if (!buyable) return 2;
    if (gSaveContext.save.saveInfo.playerData.rupees < kScrubPrices[id]) return 1;
    actor->unk_364 = -kScrubPrices[id];
    static const int nativeGi[] = {GI_MAGIC_BEANS, GI_BOMB_BAG_40, GI_POTION_GREEN, GI_POTION_BLUE};
    actor->unk_366 = nativeGi[id];
    return 3;
}
#endif

#ifdef COMBO_BUILD
struct ScrubPendingForeign {
    EnAkindonuts* scrub=nullptr;int fileNum=-1;unsigned id=0;uint64_t generation=0;
    ComboRenewableStock stock{};bool given=false;
};
static std::unordered_map<s16, ScrubPendingForeign> sScrubForeignPending;
static s16 sScrubForeignTicket=0;
static void ScrubGiveForeign(Actor* actor, PlayState* play) {
    auto found=sScrubForeignPending.find(CUSTOM_ITEM_PARAM);
    if(found==sScrubForeignPending.end()) return;
    auto& pending=found->second;
    if(pending.given) return;
    const bool valid=pending.scrub && pending.fileNum==gSaveContext.fileNum &&
        pending.generation==Rando::MiscBehavior::ComboRandoGen() &&
        gSaveContext.save.saveInfo.playerData.rupees>=kScrubPrices[pending.id];
    const bool granted=valid && sScrubOwnerGrant && sScrubOwnerGrant(pending.fileNum,pending.stock.nativeGi);
    CustomMessage::Entry message{.textboxType=2,.icon=Rando::StaticData::GetIconForZMessage(RI_COMBO_FOREIGN),
        .msg=granted ? "You got "+std::string(pending.stock.name)+"!" : "Oh, it seems I can't sell you that right now."};
    if(CUSTOM_ITEM_FLAGS&CustomItem::GIVE_ITEM_CUTSCENE) CustomMessage::SetActiveCustomMessage(message.msg,message);
    else CustomMessage::StartTextbox(message.msg+"\x1C\x02\x10",message);
    pending.given=granted;
    if(pending.scrub) {
        if(granted) pending.scrub->actor.parent=&GET_PLAYER(play)->actor; // Native acknowledgement performs exactly one price debit.
        else {pending.scrub->unk_32C &= ~0x40;pending.scrub->actionFunc=func_80BEEFA8;}
        pending.scrub=nullptr;
    }
}
static void ScrubDrawForeign(Actor* actor, PlayState* play) {
    auto found=sScrubForeignPending.find(CUSTOM_ITEM_PARAM);
    if(found==sScrubForeignPending.end()) return;
    Matrix_Scale(30.0f,30.0f,30.0f,MTXMODE_APPLY);
    Rando::DrawComboForeignRenewable(found->second.stock.drawName);
}
static bool ScrubQueueForeign(EnAkindonuts* scrub,unsigned id,const ComboRenewableStock& stock) {
    for(const auto& pending:sScrubForeignPending) if(pending.second.scrub==scrub) return true;
    if(std::holds_alternative<GIEventNone>(GameInteractor::Instance->currentEvent) && GameInteractor::Instance->events.empty())
        sScrubForeignPending.clear();
    if(sScrubForeignPending.size()>=128) return false;
    sScrubForeignTicket=sScrubForeignTicket==32767?1:sScrubForeignTicket+1;
    if(sScrubForeignPending.count(sScrubForeignTicket)) return false;
    sScrubForeignPending.emplace(sScrubForeignTicket,ScrubPendingForeign{scrub,gSaveContext.fileNum,id,
        Rando::MiscBehavior::ComboRandoGen(),stock,false});
    GameInteractor::Instance->events.emplace_back(GIEventGiveItem{.showGetItemCutscene=!stock.fastBuy,
        .param=sScrubForeignTicket,.giveItem=ScrubGiveForeign,.drawItem=ScrubDrawForeign});
    return true;
}
#endif

void EnAkindonuts_ReplacePurchaseMessage(RandoCheckId randoCheckId, RandoInf randoInf, s32 cost, u16* textId,
                                         bool* loadFromMessageTable) {
    auto& randoSaveCheck = RANDO_SAVE_CHECKS[randoCheckId];

    // Standalone keeps its original repeat dialogue; COMBO names the renewable product.
    if (!randoSaveCheck.shuffled
#ifndef COMBO_BUILD
        || (Flags_GetRandoInf(randoInf) && randoCheckId != RC_GORON_VILLAGE_SCRUB_BOMB_BAG)
#endif
    ) {
        return;
    }

    RandoItemId offered = randoSaveCheck.randoItemId;
#ifdef COMBO_BUILD
    if (Flags_GetRandoInf(randoInf)) offered = randoCheckId == RC_GORON_VILLAGE_SCRUB_BOMB_BAG ? RI_RECOVERY_HEART : Rando::RenewableStockItem(offered);
#endif
    std::string renewableName;
#ifdef COMBO_BUILD
    if(Flags_GetRandoInf(randoInf) && offered==RI_UNKNOWN) {
        unsigned id=0;while(id<4 && kScrubChecks[id]!=randoCheckId) ++id;
        ComboRenewableStock stock{};
        if(id<4 && ScrubForeignStock(id,stock)) {
            if(stock.kind==1) offered=RI_RECOVERY_HEART;
            else renewableName=stock.name;
        }
    }
#endif
    auto entry = CustomMessage::LoadVanillaMessageTableEntry(*textId);
    entry.msg = offered == RI_UNKNOWN && renewableName.empty() ? "Oh, it seems I can't sell you that right now.\xE0" :
        "I'll sell you %g{{item}}%w for %r{{rupees}} Rupees%w!\xE0";

    CustomMessage::Replace(&entry.msg, "{{item}}",
                           !renewableName.empty() ? renewableName : (offered == RI_UNKNOWN ? std::string("unavailable stock") : Rando::StaticData::GetRevealedItemName(offered, true, randoCheckId)));
    CustomMessage::Replace(&entry.msg, "{{rupees}}", std::to_string(cost));

    CustomMessage::LoadCustomMessageIntoFont(entry);
    *loadFromMessageTable = false;
}

void EnAkindonuts_ReplaceNotEligibleMessage(RandoInf randoInf, u16* textId, bool* loadFromMessageTable) {
    if (
#ifdef COMBO_BUILD
        true
#else
        !Flags_GetRandoInf(randoInf)
#endif
    ) {
        auto entry = CustomMessage::LoadVanillaMessageTableEntry(*textId);
        entry.msg = "Oh, it seems I can't sell you that right now. Sorry for the trouble.\xE0";
        CustomMessage::LoadCustomMessageIntoFont(entry);
        *loadFromMessageTable = false;
    }
}

void EnAkindonuts_IsEligible(RandoCheckId randoCheckId, RandoInf randoInf, bool* should) {
    auto& randoSaveCheck = RANDO_SAVE_CHECKS[randoCheckId];

#ifdef COMBO_BUILD
    if(randoSaveCheck.randoItemId==RI_MAGIC_BEAN&&INV_CONTENT(ITEM_MAGIC_BEANS)!=ITEM_MAGIC_BEANS){*should=false;return;}
#endif
    if (!randoSaveCheck.shuffled || Flags_GetRandoInf(randoInf)) {
        return;
    }
    *should = true;
}

// This handles the checks for the business scrubs in the Southern Swamp, Goron Village, Zora Hall, and Ikana Canyon.
void Rando::ActorBehavior::InitEnAkindonutsBehavior() {

#ifdef COMBO_BUILD
    COND_ID_HOOK(OnActorDestroy,ACTOR_EN_AKINDONUTS,IS_RANDO,[](Actor* actor) {
        for(auto& pending:sScrubForeignPending) if(pending.second.scrub && &pending.second.scrub->actor==actor) pending.second.scrub=nullptr;
    });
    COND_HOOK(OnSceneInit,IS_RANDO,[](s16 sceneId,s8 spawnNum) {sScrubForeignPending.clear();});
    COND_VB_SHOULD(VB_GIVE_ITEM_FROM_OFFER, IS_RANDO, {
        GetItemId* item = va_arg(args, GetItemId*);
        Actor* actor = va_arg(args, Actor*);
        if (actor && actor->id == ACTOR_EN_AKINDONUTS) {
            auto* scrub = reinterpret_cast<EnAkindonuts*>(actor);
            const unsigned id = ENAKINDONUTS_GET_3(actor);
            if (id < 4 && (scrub->unk_32C & 0x40) && RANDO_SAVE_CHECKS[kScrubChecks[id]].shuffled &&
                Flags_GetRandoInf(kScrubFlags[id])) {
                if(id!=1 && RANDO_SAVE_CHECKS[kScrubChecks[id]].randoItemId==RI_COMBO_FOREIGN) {
                    ComboRenewableStock foreign{};
                    if(!ScrubForeignStock(id,foreign)) *should=false;
                    else if(foreign.kind==1) {
                        if(!ScrubStockBuyable(RI_RECOVERY_HEART)) *should=false;
                        else *item=Rando::StaticData::Items[RI_RECOVERY_HEART].getItemId;
                    } else {
                        *should=false;
                        if(foreign.buyable) ScrubQueueForeign(scrub,id,foreign);
                    }
                    return;
                }
                const auto stock = ScrubStock(id);
                if (!ScrubStockBuyable(stock)) *should = false;
                else *item = Rando::StaticData::Items[stock].getItemId;
            }
        }
    });
#endif

    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_AKINDONUTS, IS_RANDO, [](Actor* actor) {
        EnAkindonuts* enAkindonuts = (EnAkindonuts*)actor;
        if (enAkindonuts->actionFunc == func_80BEF360) {
            if (enAkindonuts->unk_32C & 0x40) {
                RandoInf randoInf = RANDO_INF_MAX;
                switch (ENAKINDONUTS_GET_3(&enAkindonuts->actor)) {
                    case 0:
                        randoInf = RANDO_INF_PURCHASED_BEANS_FROM_SOUTHERN_SWAMP_SCRUB;
                        break;
                    case 1:
                        randoInf = RANDO_INF_PURCHASED_BOMB_BAG_FROM_GORON_VILLAGE_SCRUB;
                        break;
                    case 2:
                        randoInf = RANDO_INF_PURCHASED_POTION_FROM_ZORA_HALL_SCRUB;
                        break;
                    case 3:
                        randoInf = RANDO_INF_PURCHASED_POTION_FROM_IKANA_CANYON_SCRUB;
                        break;
                }
                if (randoInf != RANDO_INF_MAX && !Flags_GetRandoInf(randoInf)) {
                    Flags_SetRandoInf(randoInf);
                    // Happen at the end of the repeatable purchase chain
                    Rupees_ChangeBy(enAkindonuts->unk_364);
                    enAkindonuts->unk_32C &= ~0x40;
                    enAkindonuts->unk_2DC(enAkindonuts, gPlayState);
                    enAkindonuts->actionFunc = func_80BEEFA8;
                }
            } else {
                // Happen at the end of the trade item chain
                enAkindonuts->unk_2DC(enAkindonuts, gPlayState);
                enAkindonuts->actionFunc = func_80BEEFA8;
            }
        }
    });

    COND_VB_SHOULD(VB_AKINDONUTS_CONSIDER_ELIGIBLE_FOR_BEAN_REFILL, IS_RANDO, {
        EnAkindonuts_IsEligible(RC_SOUTHERN_SWAMP_SCRUB_BEANS, RANDO_INF_PURCHASED_BEANS_FROM_SOUTHERN_SWAMP_SCRUB,
                                should);
    });

    // Do you know what magic beans are, sir?...
    COND_ID_HOOK(OnOpenText, 0x15E9, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        EnAkindonuts_ReplacePurchaseMessage(RC_SOUTHERN_SWAMP_SCRUB_BEANS,
                                            RANDO_INF_PURCHASED_BEANS_FROM_SOUTHERN_SWAMP_SCRUB, 10, textId,
                                            loadFromMessageTable);
    });

    // I sell Magic Beans to Deku Scrubs,...
    COND_ID_HOOK(OnOpenText, 0x15E1, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        if (!RANDO_SAVE_CHECKS[RC_SOUTHERN_SWAMP_SCRUB_BEANS].shuffled) {
            return;
        }

        auto entry = CustomMessage::LoadVanillaMessageTableEntry(*textId);
        entry.msg = "I sell %g{{item}} %wand %gMagic Beans %wto Deku Scrubs, but recently I've been thinking about "
                    "relocating to a new area.\xE0";

        CustomMessage::Replace(
            &entry.msg, "{{item}}",
            Rando::StaticData::GetRevealedItemName(RANDO_SAVE_CHECKS[RC_SOUTHERN_SWAMP_SCRUB_BEANS].randoItemId, true,
                                           RC_SOUTHERN_SWAMP_SCRUB_BEANS));
        CustomMessage::LoadCustomMessageIntoFont(entry);
        *loadFromMessageTable = false;
    });

    // Oh, you don't know how to use magic beans?...
    COND_ID_HOOK(OnOpenText, 0x15EC, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        EnAkindonuts_ReplaceNotEligibleMessage(RANDO_INF_PURCHASED_BEANS_FROM_SOUTHERN_SWAMP_SCRUB, textId,
                                               loadFromMessageTable);
    });

    COND_VB_SHOULD(VB_AKINDONUTS_CONSIDER_ELIGIBLE_FOR_POTION_REFILL, IS_RANDO, {
        EnAkindonuts* enAkindonuts = va_arg(args, EnAkindonuts*);
        switch (ENAKINDONUTS_GET_3(&enAkindonuts->actor)) {
            case 2:
                EnAkindonuts_IsEligible(RC_ZORA_HALL_SCRUB_POTION_REFILL,
                                        RANDO_INF_PURCHASED_POTION_FROM_ZORA_HALL_SCRUB, should);
                break;
            case 3:
                EnAkindonuts_IsEligible(RC_IKANA_CANYON_SCRUB_POTION_REFILL,
                                        RANDO_INF_PURCHASED_POTION_FROM_IKANA_CANYON_SCRUB, should);
                break;
        }
    });

    // I'll sell you a Green Potion for 40 Rupees!
    COND_ID_HOOK(OnOpenText, 0x1612, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        EnAkindonuts_ReplacePurchaseMessage(RC_ZORA_HALL_SCRUB_POTION_REFILL,
                                            RANDO_INF_PURCHASED_POTION_FROM_ZORA_HALL_SCRUB, 40, textId,
                                            loadFromMessageTable);
    });

    // Don't you need any Blue Potion in case you get cursed?...
    COND_ID_HOOK(OnOpenText, 0x1626, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        EnAkindonuts_ReplacePurchaseMessage(RC_IKANA_CANYON_SCRUB_POTION_REFILL,
                                            RANDO_INF_PURCHASED_POTION_FROM_IKANA_CANYON_SCRUB, 100, textId,
                                            loadFromMessageTable);
    });

    // You get the potion only! if you don't have an empty bottle... (Used for both potion scrubs)
    COND_ID_HOOK(OnOpenText, 0x1613, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        Player* player = GET_PLAYER(gPlayState);
        if (player->talkActor == nullptr || player->talkActor->id != ACTOR_EN_AKINDONUTS) {
            return;
        }
        switch (ENAKINDONUTS_GET_3(player->talkActor)) {
            case 2:
                EnAkindonuts_ReplaceNotEligibleMessage(RANDO_INF_PURCHASED_POTION_FROM_ZORA_HALL_SCRUB, textId,
                                                       loadFromMessageTable);
                break;
            case 3:
                EnAkindonuts_ReplaceNotEligibleMessage(RANDO_INF_PURCHASED_POTION_FROM_IKANA_CANYON_SCRUB, textId,
                                                       loadFromMessageTable);
                break;
        }
    });

    // TODO: Should there be a bomb bag requirement here still?
    COND_VB_SHOULD(VB_AKINDONUTS_CONSIDER_ELIGIBLE_FOR_BOMB_BAG, IS_RANDO, {
        EnAkindonuts_IsEligible(RC_GORON_VILLAGE_SCRUB_BOMB_BAG, RANDO_INF_PURCHASED_BOMB_BAG_FROM_GORON_VILLAGE_SCRUB,
                                should);
    });

    COND_VB_SHOULD(VB_AKINDONUTS_CONSIDER_BOMB_BAG_PURCHASED, IS_RANDO, {
        if (RANDO_SAVE_CHECKS[RC_GORON_VILLAGE_SCRUB_BOMB_BAG].shuffled) {
            *should = Flags_GetRandoInf(RANDO_INF_PURCHASED_BOMB_BAG_FROM_GORON_VILLAGE_SCRUB);
        }
    });

    // I'll give you my Biggest Bomb Bag...
    COND_ID_HOOK(OnOpenText, 0x1600, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        EnAkindonuts_ReplacePurchaseMessage(RC_GORON_VILLAGE_SCRUB_BOMB_BAG,
                                            RANDO_INF_PURCHASED_BOMB_BAG_FROM_GORON_VILLAGE_SCRUB, 200, textId,
                                            loadFromMessageTable);
    });

    // If you don't have a Big Bomb Bag...
    COND_ID_HOOK(OnOpenText, 0x1602, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        EnAkindonuts_ReplaceNotEligibleMessage(RANDO_INF_PURCHASED_BOMB_BAG_FROM_GORON_VILLAGE_SCRUB, textId,
                                               loadFromMessageTable);
    });

    // What? You already have a Big Bomb Bag?...
    COND_ID_HOOK(OnOpenText, 0x1601, IS_RANDO, [](u16* textId, bool* loadFromMessageTable) {
        if (RANDO_SAVE_CHECKS[RC_GORON_VILLAGE_SCRUB_BOMB_BAG].shuffled) {
            auto entry = CustomMessage::LoadVanillaMessageTableEntry(*textId);
            entry.msg = "What? You already bought that from me, Only one purchase per customer allowed!\xE0";
            CustomMessage::LoadCustomMessageIntoFont(entry);
            *loadFromMessageTable = false;
        }
    });
}
