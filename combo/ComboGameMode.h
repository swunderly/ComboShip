#pragma once
// C ABI shared by the launcher, engines and save creation. Never a live menu CVar.
enum ComboGameMode {
    COMBO_MODE_COMBINED = 0,
    COMBO_MODE_OOT_RANDO = 1,
    COMBO_MODE_MM_RANDO = 2,
    COMBO_MODE_OOT_VANILLA = 3,
    COMBO_MODE_MM_VANILLA = 4
};
