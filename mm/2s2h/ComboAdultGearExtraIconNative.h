#pragma once
// Exact pinned GetVtxBuffer/DrawIcon alternate trade-slot geometry, native owning icon paths.
extern "C" void Combo_MM_AdultGearDrawExtraIcon(PlayState* play, int slot, const Vtx* source) {
    if (!play || !source || gSaveContext.fileNum > 2 || !Combo_MM_GearActive())
        return;
    if (slot != SLOT_TRADE_KEY_MAMA && slot != SLOT_TRADE_COUPLE)
        return;
    const u8 primary = gSaveContext.save.saveInfo.inventory.items[slot];
    const u8 next = ComboAdultMM::GearPauseNextItem(slot);
    if (next == ITEM_NONE || next == primary || !gItemIcons[next])
        return;
    Vtx* vtx = (Vtx*)GRAPH_ALLOC(play->state.gfxCtx, 4 * sizeof(Vtx));
    // Allocate in the current native graph arena instead of retaining vertices across frames.
    for (int i = 0; i < 4; ++i)
        vtx[i] = source[i];
    vtx[0].v.ob[0] += 0x10;
    vtx[2].v.ob[0] += 0x10;
    vtx[0].v.ob[1] -= 0x10;
    vtx[1].v.ob[1] -= 0x10;
    const bool restricted = !gPlayerFormItemRestrictions[GET_PLAYER_FORM][next];
    OPEN_DISPS(play->state.gfxCtx);
    if (restricted) {
        gDPSetGrayscaleColor(POLY_OPA_DISP++, 109, 109, 109, 255);
        gSPGrayscale(POLY_OPA_DISP++, true);
    }
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, play->pauseCtx.alpha);
    gSPVertex(POLY_OPA_DISP++, (uintptr_t)vtx, 4, 0);
    gDPLoadTextureBlock(POLY_OPA_DISP++, gItemIcons[next], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
    if (restricted)
        gSPGrayscale(POLY_OPA_DISP++, false);
    CLOSE_DISPS(play->state.gfxCtx);
}
