#pragma once
// Pinned trade.c five-entry order and kaleido native C-up/L cycle.
namespace ComboAdultMM {
inline u8 GearPauseNextItem(int slot) {
    if (slot != SLOT_TRADE_KEY_MAMA && slot != SLOT_TRADE_COUPLE)
        return ITEM_NONE;
    const bool trade2 = slot == SLOT_TRADE_KEY_MAMA;
    const auto& ship = gSaveContext.save.shipSaveInfo;
    unsigned flags = trade2 ? ship.adultMm.gearTrade2Bits : ship.adultMm.gearTrade3Bits;
    if (flags & ~6u)
        return ITEM_NONE;
    if (trade2) {
        if (ship.faroresWind.owned)
            flags |= 1;
        if (Flags_GetRandoInf(RANDO_INF_OBTAINED_ROOM_KEY))
            flags |= 8;
        if (Flags_GetRandoInf(RANDO_INF_OBTAINED_LETTER_TO_MAMA))
            flags |= 16;
    } else {
        if (ship.spellLoveOwned)
            flags |= 1;
        if (Flags_GetRandoInf(RANDO_INF_OBTAINED_LETTER_TO_KAFEI))
            flags |= 8;
        if (Flags_GetRandoInf(RANDO_INF_OBTAINED_PENDANT_OF_MEMORIES))
            flags |= 16;
    }
    const u8 table2[] = { ITEM_COMBO_FARORES_WIND, ITEM_COMBO_BOOTS_IRON, ITEM_COMBO_TUNIC_GORON, ITEM_ROOM_KEY,
                          ITEM_LETTER_MAMA };
    const u8 table3[] = { ITEM_COMBO_NAYRUS_LOVE, ITEM_COMBO_BOOTS_HOVER, ITEM_COMBO_TUNIC_ZORA, ITEM_LETTER_TO_KAFEI,
                          ITEM_PENDANT_OF_MEMORIES };
    const auto& current = gSaveContext.save.saveInfo.inventory.items[slot];
    const u8 next = ComboAdultGearInventory::Next(current, flags, trade2 ? table2 : table3);
    return next;
}
} // namespace ComboAdultMM

extern "C" int Combo_MM_AdultGearPauseTick(PlayState* play, int slot) {
    if (!play || gSaveContext.fileNum > 2 || !gSaveContext.save.shipSaveInfo.adultMm.enabled ||
        !Combo_MM_AdultResourcesReady())
        return 0;
    if (slot != SLOT_TRADE_KEY_MAMA && slot != SLOT_TRADE_COUPLE)
        return 0;
    auto& pause = play->pauseCtx;
    if (pause.state != PAUSE_STATE_MAIN || pause.mainState != PAUSE_MAIN_STATE_IDLE || pause.pageIndex != PAUSE_ITEM ||
        pause.cursorSpecialPos || pause.itemDescriptionOn)
        return 0;
    auto& current = gSaveContext.save.saveInfo.inventory.items[slot];
    const u8 next = ComboAdultMM::GearPauseNextItem(slot);
    if (next == ITEM_NONE || next == current)
        return 0;
    pause.cursorColorSet = PAUSE_CURSOR_COLOR_SET_BLUE;
    if (!(play->state.input[0].press.button & (BTN_L | BTN_CUP)))
        return 0;
    current = next;
    auto& equips = gSaveContext.save.saveInfo.equips;
    for (int button = 1; button < 4; ++button)
        if (equips.cButtonSlots[0][button] == slot) {
            equips.buttonItems[0][button] = next;
            Interface_LoadItemIconImpl(play, button);
        }
    auto& dpad = gSaveContext.save.shipSaveInfo.dpadEquips;
    for (int button = 0; button < 4; ++button)
        if (dpad.dpadSlots[0][button] == slot) {
            dpad.dpadItems[0][button] = next;
            Interface_Dpad_LoadItemIconImpl(play, button);
        }
    pause.namedItem = PAUSE_ITEM_NONE;
    Audio_PlaySfx(NA_SE_SY_CURSOR);
    return 1;
}
