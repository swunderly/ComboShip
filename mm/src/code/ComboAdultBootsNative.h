#pragma once
// Pinned Player.c custom boot table and runtime register application.
enum{COMBO_CUSTOM_BOOTS_IRON=1,COMBO_CUSTOM_BOOTS_HOVER=2};
int Combo_MM_IsLinkAdult(void);int Combo_MM_GearActive(void);
static s16 sComboAdultBootData[7][18] = {
    /* Child Link - Iron Boots */
    { 200, 1000, 300, 700, 550, 270, 1000, 0, 120, 800, 300, -160, 600, 590, 750, 125, 200, 130 },

    /* Child Link - Hover Boots */
    { 200, 1000, 300, 700, 550, 270, 600, 600, 120, 800, 550, -100, 600, 540, 270, 25, 0, 130 },

    /* Child Link - Iron Boots (Underwater) */
    { 80, 800, 150, 700, 480, 270, 600, 50, 120, 800, 550, -40, 400, 540, 270, 25, 0, 80 },

    /* Adult Link - Kokiri Boots */
    { 200, 1000, 300, 700, 550, 270, 700, 300, 120, 800, 600, -100, 600, 590, 750, 125, 200, 130 },

    /* Adult Link - Iron Boots */
    { 200, 1000, 300, 700, 550, 270, 1000, 0, 120, 800, 300, -160, 600, 590, 750, 125, 200, 130 },

    /* Adult Link - Hover Boots */
    { 200, 1000, 300, 700, 550, 270, 600, 600, 120, 800, 550, -100, 600, 540, 270, 25, 0, 130 },

    /* Adult Link - Iron Boots (Underwater) */
    { 80, 800, 150, 700, 480, 270, 600, 50, 120, 800, 550, -40, 400, 540, 270, 25, 0, 80 },
};

void Combo_MM_AdultCheckBoots(PlayState* play)
{
    Player* player = GET_PLAYER(play);
    // Custom boots are gear; the adult row without boots belongs to Adult Link in MM.
    const s32 gear = Combo_MM_GearActive();
    if((!gear&&!Combo_MM_IsLinkAdult())||player->actor.id!=ACTOR_PLAYER)return;
    if(gSaveContext.save.shipSaveInfo.adultMm.boots>2)return;
    if (player->transformation == PLAYER_FORM_HUMAN)
    {
        s32 isAdult = Combo_MM_IsLinkAdult();
        const s32 boots = gear ? gSaveContext.save.shipSaveInfo.adultMm.boots : 0;
        player->actor.flags &= ~ACTOR_FLAG_CAN_PRESS_HEAVY_SWITCHES;
        player->actor.flags |= ACTOR_FLAG_CAN_PRESS_SWITCHES;

        if (boots > 0 || isAdult)
        {
            s32 currentBoots = boots;
            s16* bootRegs;

            REG(27) = 2000;
            REG(48) = 370;

            switch(currentBoots)
            {
            case COMBO_CUSTOM_BOOTS_IRON:
                player->currentBoots = 6; /* PLAYER_BOOTS_GORON */
                player->actor.flags |= ACTOR_FLAG_CAN_PRESS_HEAVY_SWITCHES;
                if (player->stateFlags1 & PLAYER_STATE1_8000000)
                {
                    currentBoots = 3; /* Iron Underwater */
                }
                REG(27) = 500;
                REG(48) = 100;
                break;
            case COMBO_CUSTOM_BOOTS_HOVER:
                player->currentBoots = 0; /* PLAYER_BOOTS_FIERCE_DEITY */
                player->actor.flags &= ~ACTOR_FLAG_CAN_PRESS_SWITCHES;
                break;
            }

            if (isAdult)
            {
                currentBoots += 4;
            }

            bootRegs = sComboAdultBootData[currentBoots - 1];
            REG(19) = bootRegs[0];
            REG(30) = bootRegs[1];
            REG(32) = bootRegs[2];
            REG(34) = bootRegs[3];
            REG(35) = bootRegs[4];
            REG(36) = bootRegs[5];
            REG(37) = bootRegs[6];
            REG(38) = bootRegs[7];
            REG(39) = bootRegs[8];
            REG(43) = bootRegs[9];
            R_RUN_SPEED_LIMIT = bootRegs[10];
            REG(68) = bootRegs[11];
            REG(69) = bootRegs[12];
            IREG(66) = bootRegs[13];
            IREG(67) = bootRegs[14];
            IREG(68) = bootRegs[15];
            IREG(69) = bootRegs[16];
            MREG(95) = bootRegs[17];

            if (play->roomCtx.curRoom.type == ROOM_TYPE_INDOORS)
            {
                R_RUN_SPEED_LIMIT = 500;
            }
        }
    }
    else
    {
        gSaveContext.save.shipSaveInfo.adultMm.boots = 0;
        gSaveContext.save.shipSaveInfo.adultMm.tunic = 0;
    }
}
