#pragma once
#include "ComboAdultMM.h"
// Save-only seed bootstrap; raw ages 0 adult/1 child. OoTMM v32.3 Cross-Games Age (crossAge) only stores an age: Link
// is child until an MM entry copies OoT's age in (util.c:74-81), and MM never changes it or swaps loadouts
// (builder.ts:319). The Adult Mask and MM starting age of OoTMM development commit 41a2b7c0 (not in v32.3) also swap
// loadouts, so their runtime caller must preflight full resources before calling.
namespace ComboAdultMM {
inline bool ConfigureSeedAge(Save& save, int startingAge, bool maskEnabled, bool crossAge) {
    if (startingAge < 0 || startingAge > 1)
        return false;
    auto& state = save.shipSaveInfo.adultMm;
    const bool required = maskEnabled || crossAge || startingAge == 0;
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
    // Normalize legacy MM's raw0/child semantics before the 41a2b7c0 equipment swap.
    save.linkAge = 1;
    state.enabled = 1;
    state.maskEnabled = maskEnabled;
    if (!maskEnabled && startingAge == 1)
        return true; // Cross-Games Age alone: child, loadouts untouched
    state.boots = state.tunic = state.customMask = 0;
    InitializeLoadouts(state);
    return SwapHumanLoadout(save, (u8)startingAge);
}
// OoTMM's MM gear settings bootsIronMm, bootsHoverMm, tunicGoronMm and tunicZoraMm (data.ts:2024-2054). The gear is
// independent of the adult state: in OoTMM child Link wears it (Player.c:616-687, 1171-1207).
inline bool SaveHasGear(const Save& save) {
    const auto& options = save.shipSaveInfo.rando.randoSaveOptions;
    return save.shipSaveInfo.saveType == SAVETYPE_RANDO && (options[RO_MM_BOOTS_IRON] || options[RO_MM_BOOTS_HOVER] ||
                                                            options[RO_MM_TUNIC_GORON] || options[RO_MM_TUNIC_ZORA]);
}
// A load is never refused for the adult or gear state. A ready adult runtime binds the seed's age once; otherwise the
// save loads as it is, and a feature that is not active draws Link as child without gear.
inline void PrepareLoadedSave(Save& save, bool adultRuntimeReady) {
    const auto& options = save.shipSaveInfo.rando.randoSaveOptions;
    const bool mask = options[RO_MM_ADULT_MASK] != 0, cross = options[RO_MM_CROSS_AGE] != 0;
    const int age = options[RO_MM_STARTING_AGE] ? 0 : 1;
    if (adultRuntimeReady && !save.shipSaveInfo.adultMm.enabled && (mask || cross || age == 0))
        ConfigureSeedAge(save, age, mask, cross);
}
} // namespace ComboAdultMM
