#pragma once
// Keaton, Bunny and Scents fitted to adult Link (OoTMM v32.3 Player_AfterMaskLoaded, generator/src/mm/actors/
// Player.c:2099-2135). Once the mask has loaded, OoTMM writes a call to the 1.25 scale list at word offsets of the mask
// object; each falls on an N64 command of the mask's list (word offset minus the list's, halved). A pipe sync becomes
// a call to the whole list, a gsSPTexture a call to its matrix part. OoTMM writes into Link's own copy of the mask
// object, so the port gives Player its own copy of the mask's cached list with the calls written in
// (FittedMaskList) and leaves MM's shared list, which Kafei also draws, unchanged; a list that does not hold the
// expected command (a custom asset) is drawn as it is.
#include "ComboAdultOotModel.h"
#include <iterator>
namespace ComboAdultMM {
struct AdultMaskScalePatch {
    s32 mask;
    const char* list;
    uint8_t command;
    bool withSync; // true: replaces a pipe sync; false: replaces a gsSPTexture, and calls the matrix part only
};
static const AdultMaskScalePatch kAdultMaskScalePatches[] = {
    // Keaton: object word 0x12A, list object_mask_ki_tan_DL_0004A0 (word 0x128).
    { PLAYER_MASK_KEATON, "__OTR__objects/object_mask_ki_tan/object_mask_ki_tan_DL_0004A0", 1, true },
    // Bunny: words 0x188, 0x1E0, 0x1EC, 0x224, 0x230, list object_mask_rabit_DL_000610 (word 0x184).
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 2, true },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 46, false },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 52, true },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 80, false },
    { PLAYER_MASK_BUNNY, "__OTR__objects/object_mask_rabit/object_mask_rabit_DL_000610", 86, true },
    // Scents: word 0x1C6, list object_mask_bu_san_DL_000710 (word 0x1C4).
    { PLAYER_MASK_SCENTS, "__OTR__objects/object_mask_bu_san/object_mask_bu_san_DL_000710", 1, true },
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
// Player's fitted copies of the Keaton, Bunny and Scents lists, and whether Player draws them (adult Link).
inline int AdultMaskCopy(s32 mask) {
    return mask == PLAYER_MASK_KEATON ? 0 : mask == PLAYER_MASK_BUNNY ? 1 : mask == PLAYER_MASK_SCENTS ? 2 : -1;
}
static std::array<std::vector<Gfx>, 3> sAdultMaskCopies;
static bool sAdultMaskFit = false;
} // namespace ComboAdultMM
// Fits the worn mask to adult Link with a fresh copy of its list. Player_Init and the end of a mask load call it for
// Player, during an update, so no display list still to be drawn names the copy it replaces.
extern "C" void Combo_MM_ApplyAdultMaskScale(s32 mask, s32 adult) {
    using namespace ComboAdultMM;
    sAdultMaskFit = adult != 0;
    const int copy = AdultMaskCopy(mask);
    if (!adult || copy < 0)
        return;
    std::array<long, std::size(kAdultMaskScalePatches)> slots;
    std::vector<Gfx> fitted;
    if (AdultMaskScaleSlots(mask, slots)) {
        std::vector<long> at;
        std::vector<Gfx> calls;
        const char* path = nullptr;
        for (size_t i = 0; i < std::size(kAdultMaskScalePatches); ++i)
            if (kAdultMaskScalePatches[i].mask == mask) {
                path = kAdultMaskScalePatches[i].list;
                at.push_back(slots[i]);
                calls.push_back(gsSPDisplayList(AdultMaskScaleDL(kAdultMaskScalePatches[i].withSync)));
            }
        const auto list =
            std::static_pointer_cast<Fast::DisplayList>(Ship::CrossRMRegistry::Get("mm")->LoadResource(path));
        fitted = ComboAdultOotModel::FittedMaskList(list->Instructions.data(), list->Instructions.size(), at.data(),
                                                    calls.data(), at.size());
    }
    sAdultMaskCopies[copy] = std::move(fitted);
}
// The list Player draws for `mask` instead of MM's shared one (z_player_lib.c), or NULL.
extern "C" Gfx* Combo_MM_AdultMaskDL(s32 mask) {
    using namespace ComboAdultMM;
    const int copy = AdultMaskCopy(mask);
    return sAdultMaskFit && copy >= 0 && !sAdultMaskCopies[copy].empty() ? sAdultMaskCopies[copy].data() : nullptr;
}
