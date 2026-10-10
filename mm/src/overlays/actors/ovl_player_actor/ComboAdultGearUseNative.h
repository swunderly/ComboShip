#pragma once
// Exact pinned Player_CustomUseItem boots/tunics behavior. The private native
// identities avoid pinned action IDs already occupied by the port's spells.
// Consume before ItemToItemAction: equipment never changes heldItemAction.
static int Combo_MM_AdultTryUseGear(PlayState* play, Player* player, ItemId item) {
    int boots=0,tunic=0;
    switch(item) {
    case ITEM_COMBO_BOOTS_IRON: boots=1; break;
    case ITEM_COMBO_BOOTS_HOVER: boots=2; break;
    case ITEM_COMBO_TUNIC_GORON: tunic=2; break;
    case ITEM_COMBO_TUNIC_ZORA: tunic=3; break;
    default: return 0;
    }
    // No partial action path can bypass the complete Adult capability gate.
    if(!gSaveContext.save.shipSaveInfo.adultMm.enabled||!Combo_MM_AdultResourcesReady())return 1;
    if(player->transformation!=PLAYER_FORM_HUMAN)return 1;
    if(boots) {
        if(gSaveContext.save.shipSaveInfo.adultMm.boots==boots)boots=0;
        gSaveContext.save.shipSaveInfo.adultMm.boots=boots;
        if(!boots)player->currentBoots=PLAYER_BOOTS_HYLIAN;
        func_80123140(play,player); // native MM Player_SetBootData
    } else {
        if(gSaveContext.save.shipSaveInfo.adultMm.tunic==tunic)tunic=0;
        gSaveContext.save.shipSaveInfo.adultMm.tunic=tunic;
    }
    Player_PlaySfx(player,0x835); // pinned NA_SE_PL_CHANGE_ARMS
    return 1;
}
