#pragma once
#include "ComboAdultMM.h"
// Save-only seed bootstrap. Runtime caller must preflight full resources before calling.
// Pinned startingAgeMm is independent of adultMaskMm; raw ages0adult/1child.
namespace ComboAdultMM {
inline bool ConfigureSeedAge(Save& save, int startingAge, bool maskEnabled, bool crossAge, bool gearEnabled = false) {
    if (startingAge < 0 || startingAge > 1)
        return false;
    auto& state = save.shipSaveInfo.adultMm;
    const bool required = maskEnabled || crossAge || startingAge == 0 || gearEnabled;
    if (!required && !state.enabled)
        return true; // legacy native child linkAge0 stays untouched
    if (!required && state.enabled)
        return false; // never silently discard a bound enabled seed's saved age
    if (state.enabled) {
        if (save.linkAge > 1)
            return false;
        state.maskEnabled = maskEnabled;
        return true;
    }
    // Normalize legacy MM's raw0/child semantics before pinned equipment swap.
    save.linkAge = 1;
    state.enabled = 1;
    state.maskEnabled = maskEnabled;
    state.boots = state.tunic = state.customMask = 0;
    InitializeLoadouts(state);
    return SwapHumanLoadout(save, (u8)startingAge);
}
} // namespace ComboAdultMM
