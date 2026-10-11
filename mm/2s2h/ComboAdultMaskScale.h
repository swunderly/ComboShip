#pragma once
// Keaton, Bunny and Scents fitted to adult Link (OoTMM v32.3 Player_AfterMaskLoaded, generator/src/mm/actors/
// Player.c:2099-2135). Once the mask has loaded, OoTMM writes a call to the 1.25 scale list at word offsets of the mask
// object; each falls on an N64 command of the mask's list (word offset minus the list's, halved). A pipe sync becomes
// a call to the whole list, a gsSPTexture a call to its matrix part. The port keeps the masks' lists cached, so the
// commands are patched in place for adult Link and given back otherwise; a list that does not hold the expected
// command (a custom asset) is left alone.
#include "ComboAdultOotModel.h"
#include <iterator>
extern "C" void ResourceMgr_PatchGfxByName(const char* path, const char* patchName, int index, Gfx instruction);
extern "C" void ResourceMgr_UnpatchGfxByName(const char* path, const char* patchName);
namespace ComboAdultMM {
struct AdultMaskScalePatch {
    s32 mask;
    const char* list;
    uint8_t command;
    bool withSync; // true: replaces a pipe sync; false: replaces a gsSPTexture, and calls the matrix part only
    const char* name;
};
static const AdultMaskScalePatch kAdultMaskScalePatches[] = {
    // Keaton: object word 0x12A, list object_mask_ki_tan_DL_0004A0 (word 0x128).
    { PLAYER_MASK_KEATON, "__OTR__objects/object_mask_ki_tan/object_mask_ki_tan_DL_0004A0", 1, true,
      "comboAdultMaskScale0" },
    // Bunny: words 0x188, 0x1E0, 0x1EC, 0x224, 0x230, list object_mask_rabit_DL_000610 (word 0x184).
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 2, true,
      "comboAdultMaskScale1" },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 46, false,
      "comboAdultMaskScale2" },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 52, true,
      "comboAdultMaskScale3" },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 80, false,
      "comboAdultMaskScale4" },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 86, true,
      "comboAdultMaskScale5" },
    // Scents: word 0x1C6, list object_mask_bu_san_DL_000710 (word 0x1C4).
    { PLAYER_MASK_SCENTS, "__OTR__objects/object_mask_bu_san/object_mask_bu_san_DL_000710", 1, true,
      "comboAdultMaskScale6" },
};
// The slots of `mask`'s patches in its list, or false when the list does not hold the commands OoTMM replaces (or the
// calls that already replace them).
inline bool AdultMaskScaleSlots(s32 mask, std::array<long, std::size(kAdultMaskScalePatches)>& slots) {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    if (!mm)
        return false;
    for (size_t i = 0; i < std::size(kAdultMaskScalePatches); ++i) {
        const AdultMaskScalePatch& patch = kAdultMaskScalePatches[i];
        slots[i] = -1;
        if (patch.mask != mask)
            continue;
        auto resource = mm->LoadResource(patch.list);
        if (!AdultResourceIs(resource, static_cast<uint32_t>(Fast::ResourceType::DisplayList)) ||
            resource->GetInitData()->IsCustom)
            return false;
        const auto& list = std::static_pointer_cast<Fast::DisplayList>(resource)->Instructions;
        slots[i] = ComboAdultOotModel::N64CommandSlot(list.data(), list.size(), patch.command);
        if (slots[i] < 0)
            return false;
        const Gfx call = gsSPDisplayList(AdultMaskScaleDL(patch.withSync));
        const Gfx& command = list[slots[i]];
        const uint8_t expected = patch.withSync ? G_RDPPIPESYNC : G_TEXTURE;
        if ((command.words.w0 >> 24) != expected &&
            (command.words.w0 != call.words.w0 || command.words.w1 != call.words.w1))
            return false;
    }
    return true;
}
} // namespace ComboAdultMM
// Fits the worn mask to adult Link and gives every other mask its own list back. Player_Init and the end of a mask
// load call it for Player.
extern "C" void Combo_MM_ApplyAdultMaskScale(s32 mask, s32 adult) {
    using namespace ComboAdultMM;
    std::array<long, std::size(kAdultMaskScalePatches)> slots;
    const bool fit = adult && AdultMaskScaleSlots(mask, slots);
    Ship::ResourceManagerScope scope(Ship::CrossRMRegistry::Get("mm")); // the patch helpers use the active manager
    for (size_t i = 0; i < std::size(kAdultMaskScalePatches); ++i) {
        const AdultMaskScalePatch& patch = kAdultMaskScalePatches[i];
        if (fit && patch.mask == mask)
            ResourceMgr_PatchGfxByName(patch.list, patch.name, int(slots[i]),
                                       gsSPDisplayList(AdultMaskScaleDL(patch.withSync)));
        else
            ResourceMgr_UnpatchGfxByName(patch.list, patch.name);
    }
}
