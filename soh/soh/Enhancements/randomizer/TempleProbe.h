#pragma once

// Generation-only opposite-age Temple probe; see TempleProbe.cpp. Nothing in generation calls it
// yet: Bindings::validateAgeTemple stays unassigned, so OoT/Both entrance logic still fails closed.

#ifdef COMBO_BUILD
#include "ComboExport.h"

#include <cstdint>

// True once EnsureOracleInit (OTRGlobals.cpp) has finalized settings and built the region table,
// location pool and StartingInventory. The probe requires it and never runs it.
bool OotOracleInitialized();

extern "C" {
// Captures the prepared context (region table epoch, starting age, prices, foreign starting items).
// 0 on success, -1 unsupported, -2 contract violation, -4 internal.
COMBO_EXPORT int SOH_TempleProbe_Prepare(const char* json);
// One candidate world: 1 reached, 0 not reached, -1 unsupported, -2 contract violation,
// -3 cancelled through poll, -4 internal. agesOut gets bit0 adult and bit1 child of Temple of Time.
COMBO_EXPORT int SOH_TempleProbe_Query(const char* json, int (*poll)(void), uint8_t* agesOut);
COMBO_EXPORT const char* SOH_TempleProbe_LastError(void);
COMBO_EXPORT void SOH_TempleProbe_Release(void);
}
#endif
