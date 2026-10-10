#pragma once
// Pinned mm/z_parameter.c Interface_ShouldStartHazardTimer and displaced native condition.
static int Combo_MM_ShouldStartHazardTimer(Player* player,int hazard){
    if(gSaveContext.save.shipSaveInfo.adultMm.enabled&&Combo_MM_AdultResourcesReady())
        return hazard==PLAYER_ENV_HAZARD_HOTROOM||hazard==PLAYER_ENV_HAZARD_UNDERWATER_FREE||
            (player->transformation!=PLAYER_FORM_ZORA&&hazard==PLAYER_ENV_HAZARD_UNDERWATER_FLOOR);
    return hazard==PLAYER_ENV_HAZARD_HOTROOM||hazard==PLAYER_ENV_HAZARD_UNDERWATER_FREE;
}

// The pinned tunic nibble is stored in appended Adult MM state in this port.
static int Combo_MM_GetHazardTunic(int legacyTunic) {
    if(gSaveContext.save.shipSaveInfo.adultMm.enabled&&Combo_MM_AdultResourcesReady())
        return gSaveContext.save.shipSaveInfo.adultMm.tunic;
    return legacyTunic;
}
