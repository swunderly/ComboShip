#pragma once
#include "ComboRenewableStockABI.h"
#include "ComboRenewableStockOOTRules.h"

static bool Combo_OOTStockBuyable(int gi) {
    switch (gi) {
        case GI_HEART: return gSaveContext.health < gSaveContext.healthCapacity;
        case GI_MAGIC_SMALL: case GI_MAGIC_LARGE:
            return gSaveContext.isMagicAcquired && gSaveContext.magic < (gSaveContext.isDoubleMagicAcquired ? 0x60 : 0x30);
        case GI_STICKS_1: case GI_STICKS_5: case GI_STICKS_10:
            return AMMO(ITEM_STICK) < CUR_CAPACITY(UPG_STICKS);
        case GI_NUTS_5: case GI_NUTS_5_2: case GI_NUTS_10:
            return AMMO(ITEM_NUT) < CUR_CAPACITY(UPG_NUTS);
        case GI_BOMBS_1: case GI_BOMBS_5: case GI_BOMBS_10: case GI_BOMBS_20: case GI_BOMBS_30:
            return CUR_UPG_VALUE(UPG_BOMB_BAG) && AMMO(ITEM_BOMB) < CUR_CAPACITY(UPG_BOMB_BAG);
        case GI_BOMBCHUS_5: case GI_BOMBCHUS_10: case GI_BOMBCHUS_20:
            if (!OTRGlobals::Instance || !OTRGlobals::Instance->gRandoContext) return false;
            return (INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU ||
                (OTRGlobals::Instance->gRandoContext->GetOption(RSK_BOMBCHU_BAG).Is(RO_BOMBCHU_BAG_NONE) && CUR_UPG_VALUE(UPG_BOMB_BAG))) &&
                AMMO(ITEM_BOMBCHU) < OTRGlobals::Instance->gRandoContext->GetBombchuCapacity();
        case GI_ARROWS_SMALL: case GI_ARROWS_MEDIUM: case GI_ARROWS_LARGE:
            return CUR_UPG_VALUE(UPG_QUIVER) && AMMO(ITEM_BOW) < CUR_CAPACITY(UPG_QUIVER);
        case GI_SEEDS_5: case GI_SEEDS_30:
            return CUR_UPG_VALUE(UPG_BULLET_BAG) && AMMO(ITEM_SLINGSHOT) < CUR_CAPACITY(UPG_BULLET_BAG);
        case GI_SHIELD_DEKU: return !CHECK_OWNED_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_DEKU);
        case GI_SHIELD_HYLIAN: return !CHECK_OWNED_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_HYLIAN);
        case GI_MILK: case GI_POTION_RED: case GI_POTION_GREEN: case GI_POTION_BLUE:
        case GI_FAIRY: case GI_FISH: case GI_BUGS: case GI_BLUE_FIRE: case GI_POE: case GI_BIG_POE:
            return Inventory_HasEmptyBottle();
        default: return false;
    }
}

static RandomizerGet Combo_OOTStockNameId(int gi, RandomizerGet original) {
    switch(gi) {
        case GI_HEART:return RG_RECOVERY_HEART;
        case GI_STICKS_1:return RG_DEKU_STICK_1;
        case GI_NUTS_5:case GI_NUTS_5_2:return RG_DEKU_NUTS_5;
        case GI_NUTS_10:return RG_DEKU_NUTS_10;
        case GI_BOMBS_5:return RG_BOMBS_5;case GI_BOMBS_10:return RG_BOMBS_10;case GI_BOMBS_20:return RG_BOMBS_20;
        case GI_BOMBCHUS_5:return RG_BOMBCHU_5;case GI_BOMBCHUS_10:return RG_BOMBCHU_10;case GI_BOMBCHUS_20:return RG_BOMBCHU_20;
        case GI_ARROWS_SMALL:return RG_ARROWS_5;case GI_ARROWS_MEDIUM:return RG_ARROWS_10;case GI_ARROWS_LARGE:return RG_ARROWS_30;
        case GI_SEEDS_30:return RG_DEKU_SEEDS_30;
        case GI_SHIELD_DEKU:return RG_DEKU_SHIELD;case GI_SHIELD_HYLIAN:return RG_HYLIAN_SHIELD;
        case GI_MILK:return RG_MILK;case GI_FISH:return RG_FISH;
        case GI_POTION_RED:return RG_RED_POTION_REFILL;case GI_POTION_GREEN:return RG_GREEN_POTION_REFILL;case GI_POTION_BLUE:return RG_BLUE_POTION_REFILL;
        case GI_FAIRY:return RG_BUY_FAIRYS_SPIRIT;case GI_BUGS:return RG_BUY_BOTTLE_BUG;
        case GI_BLUE_FIRE:return RG_BUY_BLUE_FIRE;case GI_POE:return RG_BUY_POE;
        default:return original;
    }
}

extern "C" COMBO_EXPORT int SOH_QueryScrubRenewable(int fileNum, const char* product, ComboRenewableStock* out) {
    if (!out) return 0;
    *out = {};
    if (fileNum < 0 || fileNum > 2 || gSaveContext.fileNum != fileNum || !product || !SaveManager::Instance) return 0;
    try {
        const auto found = Rando::StaticData::itemNameToEnum.find(product);
        if (found == Rando::StaticData::itemNameToEnum.end()) return 0;
        const auto rg = found->second;
        int gi = Combo_OOTFilledBottleRenewable(rg);
        if (!gi) {
            const auto entry = Rando::StaticData::RetrieveItem(rg).GetRawGIEntry_Copy();
            if (entry.modIndex != MOD_NONE || entry.tableId != TABLE_VANILLA || !Combo_OOTRenewableGi(entry.getItemId)) {
                out->kind = 1; // Pinned giRenew is the finder MM Recovery Heart, not an OoT permanent grant.
                return 1;
            }
            gi = entry.getItemId;
        }
        out->kind = 2; out->nativeGi = gi; out->buyable = Combo_OOTStockBuyable(gi);
        out->fastBuy = gi != GI_SHIELD_DEKU && gi != GI_SHIELD_HYLIAN;
        // Existing registry metadata provides a real owning draw name; no model/texture substitutions.
        const auto nameId=Combo_OOTStockNameId(gi,rg);
        const auto drawName=Rando::StaticData::RetrieveItem(nameId).GetName().english;
        std::string name=drawName;
        if(name.rfind("Bottle with ",0)==0) name.erase(0,12);
        if(name.rfind("Buy ",0)==0) name.erase(0,4);
        if (name.empty() || drawName.empty() || name.size() >= sizeof(out->name)) { *out = {}; return 0; }
        std::memcpy(out->name, name.c_str(), name.size()+1);
        if (drawName.size() >= sizeof(out->drawName)) { *out = {}; return 0; }
        std::memcpy(out->drawName, drawName.c_str(), drawName.size()+1);
        return 1;
    } catch (...) { *out = {}; return 0; }
}

extern "C" COMBO_EXPORT int SOH_GrantScrubRenewable(int fileNum, int gi) {
    if (fileNum < 0 || fileNum > 2 || gSaveContext.fileNum != fileNum || !SaveManager::Instance ||
        !Combo_OOTRenewableGi(gi, true) || !Combo_OOTStockBuyable(gi)) return 0;
    const auto before = gSaveContext;
    try {
        const auto entry = ItemTable_RetrieveEntry(MOD_NONE, static_cast<s16>(gi));
        if (entry.modIndex != MOD_NONE || entry.tableId != TABLE_VANILLA || entry.getItemId != gi) return 0;
        Combo_GrantResolvedOOT(entry); // Real owning Item_Give and existing owning native save persistence.
        return 1;
    } catch (...) { gSaveContext = before; return 0; }
}
