#pragma once
#include <fast/resource/type/Texture.h>
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sAdultMaskIconOwner;
static std::array<std::shared_ptr<Fast::Texture>, 2> sAdultMaskIcons{};
static bool sAdultMaskIconListener = false, sAdultMaskIconTables = false;
static TexturePtr sAdultMaskPreviousIcon;
static u8 sAdultMaskPreviousSlot;
static std::array<u8, PLAYER_FORM_MAX> sAdultMaskPreviousRestrictions{};
alignas(2) static const char sAdultMaskIconPaths0[] = "__OTR__textures/combo_adult_mask_icons/gComboAdultMaskIconTex";
alignas(2) static const char sAdultMaskIconPaths1[] = "__OTR__textures/combo_adult_mask_names/gComboAdultMaskNameTex";
static const char* const sAdultMaskIconPaths[] = { sAdultMaskIconPaths0, sAdultMaskIconPaths1 };
inline void ApplyAdultMaskIconTables(bool enabled) {
    if (enabled && !sAdultMaskIconTables) {
        sAdultMaskPreviousIcon = gItemIcons[0xbc];
        sAdultMaskPreviousSlot = gItemSlots[0xbc];
        gItemIcons[0xbc] = (TexturePtr)sAdultMaskIconPaths[0];
        gItemSlots[0xbc] = SLOT_MASK_FIERCE_DEITY;
        for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
            sAdultMaskPreviousRestrictions[form] = gPlayerFormItemRestrictions[form][0xbc];
            gPlayerFormItemRestrictions[form][0xbc] = form == PLAYER_FORM_HUMAN;
        }
        sAdultMaskIconTables = true;
    } else if (!enabled && sAdultMaskIconTables) {
        gItemIcons[0xbc] = sAdultMaskPreviousIcon;
        gItemSlots[0xbc] = sAdultMaskPreviousSlot;
        for (int form = 0; form < PLAYER_FORM_MAX; ++form)
            gPlayerFormItemRestrictions[form][0xbc] = sAdultMaskPreviousRestrictions[form];
        sAdultMaskIconTables = false;
    }
}
inline void DropAdultMaskIcons() {
    ApplyAdultMaskIconTables(false);
    sAdultMaskIcons = {};
    sAdultMaskIconOwner.reset();
}
inline bool LoadAdultMaskIcons() {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    if (!mm)
        return false;
    if (sAdultMaskIconOwner == mm)
        return true;
    std::array<std::shared_ptr<Fast::Texture>, 2> loaded{};
    for (size_t i = 0; i < 2; ++i) {
        loaded[i] = std::dynamic_pointer_cast<Fast::Texture>(mm->LoadResourceProcess(sAdultMaskIconPaths[i] + 7));
        if (!loaded[i] || !loaded[i]->GetRawPointer())
            return false;
    }
    if (loaded[0]->Type != Fast::TextureType::RGBA32bpp || loaded[0]->Width != 32 || loaded[0]->Height != 32 ||
        loaded[0]->ImageDataSize != 4096 || loaded[1]->Type != Fast::TextureType::GrayscaleAlpha4bpp ||
        loaded[1]->Width != 128 || loaded[1]->Height != 16 || loaded[1]->ImageDataSize != 1024)
        return false;
    if (sAdultMaskIconOwner)
        DropAdultMaskIcons();
    sAdultMaskIconOwner = mm;
    sAdultMaskIcons = std::move(loaded);
    if (!sAdultMaskIconListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultMaskIcons);
        sAdultMaskIconListener = true;
    }
    return true;
}
} // namespace ComboAdultMM
extern "C" int Combo_MM_AdultMaskIconsReady(void) {
    return ComboAdultMM::LoadAdultMaskIcons();
}
extern "C" void Combo_MM_ApplyAdultMaskInventoryResources(int enabled) {
    if (enabled && !ComboAdultMM::LoadAdultMaskIcons())
        return;
    ComboAdultMM::ApplyAdultMaskIconTables(enabled != 0);
}

extern "C" const char* Combo_MM_AdultMaskItemName(void) {
    return ComboAdultMM::LoadAdultMaskIcons() ? ComboAdultMM::sAdultMaskIconPaths[1] : nullptr;
}
