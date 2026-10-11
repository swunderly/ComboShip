#pragma once
// Goron/Zora Tunic colour on child Link. OoTMM v32.3 patches the 11 tunic combiners of MM's object_link_child to
// TEXEL0*ENV*SHADE (actors/Player.S:891-945, FC12FE04 FFFFFDFE) and sets sTunicColors[tunic] as the env colour
// (actors/Player.c:1244-1249, 1606-1629). MM's own DLs colour the tunic with PRIM 30,105,27 instead, and 2S2H's
// Link tunic cosmetic replaces that PRIM ("setPrim", the command after each combiner). While a Goron or Zora Tunic
// is worn the same combiners are patched here, to a call that also sets the env colour, so the equipment DLs that
// set ENV themselves (which OoTMM patches separately, Player.S:948-1215) cannot change it. With the default tunic
// the vanilla or cosmetic PRIM colour stays, as OoTMM's default tunic colour is its human tunic cosmetic
// (cosmetics/index.ts:393-395); its Goron and Zora colours follow OoT's tunic cosmetics (:383-391).
static Gfx sComboGearTunicDL[] = {
    gsDPSetCombineLERP(TEXEL0, 0, ENVIRONMENT, 0, 0, 0, 0, 1, COMBINED, 0, SHADE, 0, 0, 0, 0, 1),
    gsDPSetEnvColor(30, 105, 27, 255),
    gsSPEndDisplayList(),
};
// The combiners before 2S2H's "setPrim" patches (CosmeticEditor.cpp humanTunicPatch indices minus one).
static const char* const sComboGearTunicDLs[] = {
    "objects/object_link_child/gLinkHumanRightThighDL",    "objects/object_link_child/gLinkHumanLeftThighDL",
    "objects/object_link_child/gLinkHumanWaistDL",         "objects/object_link_child/gLinkHumanCollarDL",
    "objects/object_link_child/gLinkHumanTorsoDL",         "objects/object_link_child/gLinkHumanHeadDL",
    "objects/object_link_child/gLinkHumanHatDL",           "objects/object_link_child/gLinkHumanRightShoulderDL",
    "objects/object_link_child/gLinkHumanRightShoulderDL", "objects/object_link_child/gLinkHumanLeftShoulderDL",
    "objects/object_link_child/gLinkHumanLeftShoulderDL",
};
static const s32 sComboGearTunicIndices[] = { 9, 9, 4, 4, 4, 91, 9, 9, 64, 9, 64 };
static const char* const sComboGearTunicPatches[] = {
    "comboGearTunic0", "comboGearTunic1", "comboGearTunic2", "comboGearTunic3", "comboGearTunic4", "comboGearTunic5",
    "comboGearTunic6", "comboGearTunic7", "comboGearTunic8", "comboGearTunic9", "comboGearTunic10",
};
static s32 sComboGearTunicPatched = 0;

static Color_RGB8 Combo_MM_GetGearTunicColor(s32 tunic) {
    const Color_RGB8 base = *Combo_MM_GetSavedTunicColor(tunic);
    if (tunic == 2 && CVarGetInteger("gCosmetics.Link.GoronTunic.Changed", 0))
        return CVarGetColor24("gCosmetics.Link.GoronTunic.Value", base);
    if (tunic == 3 && CVarGetInteger("gCosmetics.Link.ZoraTunic.Changed", 0))
        return CVarGetColor24("gCosmetics.Link.ZoraTunic.Value", base);
    return base;
}

// The command at a combiner site, or NULL for a custom or shorter list (custom models keep their own colours).
static const Gfx* Combo_MM_GearTunicSite(s32 i) {
    if (ResourceGetIsCustomByName(sComboGearTunicDLs[i]) ||
        ResourceGetSizeByName(sComboGearTunicDLs[i]) <= sComboGearTunicIndices[i] * sizeof(Gfx))
        return NULL;
    return &ResourceMgr_LoadGfxByName(sComboGearTunicDLs[i])[sComboGearTunicIndices[i]];
}

// tunic: the worn gear tunic (2 Goron, 3 Zora) or 0. Patches only authentic combiners.
static void Combo_MM_SyncGearTunicColor(s32 tunic) {
    const Gfx call = gsSPDisplayList(sComboGearTunicDL);
    const s32 want = tunic == 2 || tunic == 3;
    if (want) {
        const Color_RGB8 color = Combo_MM_GetGearTunicColor(tunic);
        const Gfx env = gsDPSetEnvColor(color.r, color.g, color.b, 255);
        sComboGearTunicDL[1] = env;
        // A resource reload or a cosmetic reset drops the patch; check one site each frame.
        if (sComboGearTunicPatched) {
            const Gfx* cmd = Combo_MM_GearTunicSite(0);
            if (cmd == NULL || cmd->words.w0 != call.words.w0 || cmd->words.w1 != call.words.w1)
                sComboGearTunicPatched = 0;
        }
    }
    if (want == sComboGearTunicPatched)
        return;
    for (s32 i = 0; i < ARRAY_COUNT(sComboGearTunicDLs); i++) {
        if (!want) {
            ResourceMgr_UnpatchGfxByName(sComboGearTunicDLs[i], sComboGearTunicPatches[i]);
            continue;
        }
        const Gfx* cmd = Combo_MM_GearTunicSite(i);
        if (cmd != NULL && (((u32)cmd->words.w0 == 0xFC11FE04 && (u32)cmd->words.w1 == 0xFFFFF3F8) ||
                            (cmd->words.w0 == call.words.w0 && cmd->words.w1 == call.words.w1)))
            ResourceMgr_PatchGfxByName(sComboGearTunicDLs[i], sComboGearTunicPatches[i], sComboGearTunicIndices[i],
                                       call);
    }
    sComboGearTunicPatched = want;
}
