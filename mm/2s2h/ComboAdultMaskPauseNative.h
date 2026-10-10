#pragma once
extern "C" {
#include "z64interface.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
}
namespace ComboAdultMM {
inline bool MaskPauseReady() {
    return gSaveContext.fileNum <= 2 && gSaveContext.save.shipSaveInfo.adultMm.enabled &&
           Combo_MM_AdultResourcesReady() && LoadAdultMaskIcons();
}
inline void ReloadMaskSlot(PlayState* play) {
    const u8 item = gSaveContext.save.saveInfo.inventory.items[SLOT_MASK_FIERCE_DEITY];
    auto& equips = gSaveContext.save.saveInfo.equips;
    for (int button = 1; button < 4; ++button) {
        if (equips.cButtonSlots[0][button] == SLOT_MASK_FIERCE_DEITY)
            equips.buttonItems[0][button] = item;
        if (play)
            Interface_LoadItemIconImpl(play, button);
    }
    // Native port extension of the same slot refresh; inactive age loadouts are preserved.
    auto& dpad = gSaveContext.save.shipSaveInfo.dpadEquips;
    for (int button = 0; button < 4; ++button) {
        if (dpad.dpadSlots[0][button] == SLOT_MASK_FIERCE_DEITY)
            dpad.dpadItems[0][button] = item;
        if (play)
            Interface_Dpad_LoadItemIconImpl(play, button);
    }
}
} // namespace ComboAdultMM
extern "C" int Combo_MM_AdultMaskVisibleItem(int maskIndex, int item, int given) {
    if (maskIndex != SLOT_MASK_FIERCE_DEITY - ITEM_NUM_SLOTS || !ComboAdultMM::MaskPauseReady())
        return item;
    return ComboAdultMaskInventory::Visible(u8(item), gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits, given != 0);
}
extern "C" int Combo_MM_AdultMaskIsGiven(int maskIndex, int item, int given) {
    if (!given)
        return 0;
    if (maskIndex != SLOT_MASK_FIERCE_DEITY - ITEM_NUM_SLOTS || !ComboAdultMM::MaskPauseReady())
        return 1;
    return ComboAdultMaskInventory::Visible(u8(item), gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits, true) ==
           ITEM_NONE;
}
extern "C" int Combo_MM_AdultMaskPauseTick(PlayState* play, int maskIndex, int given) {
    if (!play || maskIndex != SLOT_MASK_FIERCE_DEITY - ITEM_NUM_SLOTS || !ComboAdultMM::MaskPauseReady())
        return 0;
    auto& pause = play->pauseCtx;
    if (pause.state != PAUSE_STATE_MAIN || pause.mainState != PAUSE_MAIN_STATE_IDLE || pause.pageIndex != PAUSE_MASK ||
        pause.cursorSpecialPos || pause.itemDescriptionOn)
        return 0;
    auto& slot = gSaveContext.save.saveInfo.inventory.items[SLOT_MASK_FIERCE_DEITY];
    const u8 owned = gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits;
    if (owned != 3 || slot == ITEM_NONE)
        return 0;
    pause.cursorColorSet = PAUSE_CURSOR_COLOR_SET_BLUE;
    bool changed = false;
    const u8 visible = ComboAdultMaskInventory::Visible(slot, owned, given != 0);
    if (visible != ITEM_NONE && visible != slot) {
        slot = visible;
        changed = true;
    }
    if (play->state.input[0].press.button & (BTN_L | BTN_CUP)) {
        const u8 next = ComboAdultMaskInventory::NextVisible(slot, owned, given != 0);
        if (next != ITEM_NONE) {
            slot = next;
            changed = true;
        }
    }
    if (changed) {
        ComboAdultMM::ReloadMaskSlot(play);
        Audio_PlaySfx(NA_SE_SY_CURSOR);
        pause.namedItem = PAUSE_ITEM_NONE;
    }
    return changed;
}

extern "C" void Combo_MM_DrawAdultMaskExtraIcon(PlayState* play, int maskIndex, int item, int given,
                                                const Vtx* source) {
    if (!play || !source || maskIndex != SLOT_MASK_FIERCE_DEITY - ITEM_NUM_SLOTS || !ComboAdultMM::MaskPauseReady())
        return;
    const u8 owned = gSaveContext.save.shipSaveInfo.adultMm.maskOwnedBits;
    const u8 primary = ComboAdultMaskInventory::Visible(u8(item), owned, given != 0);
    if (primary == ITEM_NONE)
        return;
    const u8 next = ComboAdultMaskInventory::NextVisible(primary, owned, given != 0);
    if (next == ITEM_NONE || next == primary)
        return;
    Vtx* vertices = (Vtx*)GRAPH_ALLOC(play->state.gfxCtx, 4 * sizeof(Vtx));
    // Exact pinned GetVtxBuffer: copy primary quad; +16 left-X and -16 top-Y.
    ComboAdultMaskInventory::CopyExtraIconQuad(vertices, source);
    OPEN_DISPS(play->state.gfxCtx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, play->pauseCtx.alpha);
    gSPVertex(POLY_OPA_DISP++, (uintptr_t)vertices, 4, 0);
    gDPLoadTextureBlock(POLY_OPA_DISP++, gItemIcons[next], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
    CLOSE_DISPS(play->state.gfxCtx);
}
