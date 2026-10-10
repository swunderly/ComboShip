#pragma once
#include "rando/AdultGearDrawRecipe.h"
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sGearGiOwner;
static std::array<std::shared_ptr<Fast::DisplayList>, 11> sGearGiResources{};
static bool sGearGiListener = false;
inline void DropGearGi() {
    sGearGiResources = {};
    sGearGiOwner.reset();
}
inline bool LoadGearGi() {
    auto oot = Ship::CrossRMRegistry::Get("oot");
    if (!oot)
        return false;
    if (sGearGiOwner == oot)
        return true;
    std::array<std::shared_ptr<Fast::DisplayList>, 11> loaded{};
    size_t index = 0;
    for (int kind = 0; kind < 4; ++kind) {
        auto recipe = ComboRando::AdultGearRecipe(kind);
        for (int i = 0; i < recipe->count; ++i) {
            loaded[index] = std::dynamic_pointer_cast<Fast::DisplayList>(
                oot->LoadResourceProcess(recipe->paths[i] + sizeof("__OTR__@oot:") - 1));
            if (!loaded[index] || loaded[index]->Instructions.empty())
                return false;
            ++index;
        }
    }
    if (sGearGiOwner)
        DropGearGi();
    sGearGiOwner = oot;
    sGearGiResources = std::move(loaded);
    if (!sGearGiListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(DropGearGi);
        sGearGiListener = true;
    }
    return true;
}
} // namespace ComboAdultMM
extern "C" int Combo_MM_AdultGearGiReady(void) {
    return ComboAdultMM::LoadGearGi();
}
