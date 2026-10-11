#pragma once
// Pinned drawgi.yml OOT_BOOTS_IRON Opa0_Xlu1, HOVER Opa0, TUNIC Opa1023.
namespace ComboRando {
struct AdultGearGiRecipe {
    int count, xlu;
    const char* paths[4];
};
inline const AdultGearGiRecipe* AdultGearRecipe(int kind) {
    // Fast3D and ResourceMgr_OTRSigCheck reject odd addresses before checking
    // the OTR signature. Keep the strings themselves aligned; aligning only
    // the pointer table leaves pooled literals free to land at odd addresses.
    alignas(2) static const char ironBoots[] = "__OTR__@oot:objects/object_gi_boots_2/gGiIronBootsDL";
    alignas(2) static const char ironRivets[] = "__OTR__@oot:objects/object_gi_boots_2/gGiIronBootsRivetsDL";
    alignas(2) static const char hoverBoots[] = "__OTR__@oot:objects/object_gi_hoverboots/gGiHoverBootsDL";
    alignas(2) static const char goronCollar[] = "__OTR__@oot:objects/object_gi_clothes/gGiGoronCollarColorDL";
    alignas(2) static const char tunicCollar[] = "__OTR__@oot:objects/object_gi_clothes/gGiTunicCollarDL";
    alignas(2) static const char goronTunic[] = "__OTR__@oot:objects/object_gi_clothes/gGiGoronTunicColorDL";
    alignas(2) static const char tunic[] = "__OTR__@oot:objects/object_gi_clothes/gGiTunicDL";
    alignas(2) static const char zoraCollar[] = "__OTR__@oot:objects/object_gi_clothes/gGiZoraCollarColorDL";
    alignas(2) static const char zoraTunic[] = "__OTR__@oot:objects/object_gi_clothes/gGiZoraTunicColorDL";
    static const AdultGearGiRecipe recipes[] = { { 2, 1, { ironBoots, ironRivets } },
                                                 { 1, -1, { hoverBoots } },
                                                 { 4, -1, { goronCollar, tunicCollar, goronTunic, tunic } },
                                                 { 4, -1, { zoraCollar, tunicCollar, zoraTunic, tunic } } };
    return kind >= 0 && kind < 4 ? &recipes[kind] : nullptr;
}
} // namespace ComboRando
