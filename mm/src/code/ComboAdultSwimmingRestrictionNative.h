#pragma once
// Pinned Items_ShouldCheckItemUsabilityWhileSwimming for supported native items.
static int Combo_MM_ShouldCheckItemUsabilityWhileSwimming(Player* player,int item,int hazard){
    if(item==ITEM_MASK_ZORA)return 0;
    if(gSaveContext.save.shipSaveInfo.adultMm.enabled&&Combo_MM_AdultResourcesReady()&&player->transformation==PLAYER_FORM_HUMAN){
        switch(item){
        case ITEM_COMBO_BOOTS_IRON:case ITEM_COMBO_BOOTS_HOVER:case ITEM_COMBO_TUNIC_GORON:case ITEM_COMBO_TUNIC_ZORA:return 0;
        case ITEM_HOOKSHOT:return hazard!=PLAYER_ENV_HAZARD_UNDERWATER_FLOOR;
        }
        if(item>=ITEM_BOTTLE&&item<=ITEM_CHATEAU)return hazard!=PLAYER_ENV_HAZARD_UNDERWATER_FLOOR;
    }
    return 1;
}
