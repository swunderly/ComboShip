#pragma once
#include "ComboResolve.h"
#include "menu/ComboCheckTrackerABI.h"

namespace ComboChecks {
// Each game's check snapshot export (0 = OoT, 1 = MM), shared by every comboui caller. Retry until
// mapped: MM can register later than comboui during startup.
inline ComboCheckSnapshot Snapshot(int game) {
    static ComboCheckSnapshot resolved[2]{};
    if (!resolved[game])
        resolved[game] = reinterpret_cast<ComboCheckSnapshot>(
            Combo_ResolveSym(game ? "2ship" : "soh", game ? "MM_ComboCheckSnapshot" : "SOH_ComboCheckSnapshot"));
    return resolved[game];
}
} // namespace ComboChecks
