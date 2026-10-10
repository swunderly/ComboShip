#pragma once

// OoTMM v32.3 En_Gb2.S changes all three 0x30 health comparisons
// from strict to inclusive, allowing the ordinary three-heart starting kit.
enum { COMBO_MM_GHOST_HUT_MIN_HEARTS = 3 };

static inline int ComboMmGhostHutHasHealth(int health, int comboRandomizer) {
    return comboRandomizer ? health >= COMBO_MM_GHOST_HUT_MIN_HEARTS * 16 : health > 0x30;
}
