#pragma once
// TU glue for real native OoT GetItemId/RandomizerGet enums. Pinned common/item/progressive.c.
inline bool Combo_OOTRenewableGi(int gi, bool output = false) {
    switch (gi) {
        case GI_HEART: case GI_STICKS_1: case GI_STICKS_5: case GI_STICKS_10:
        case GI_NUTS_5: case GI_NUTS_5_2: case GI_NUTS_10:
        case GI_BOMBS_1: case GI_BOMBS_5: case GI_BOMBS_10: case GI_BOMBS_20: case GI_BOMBS_30:
        case GI_ARROWS_SMALL: case GI_ARROWS_MEDIUM: case GI_ARROWS_LARGE: case GI_SEEDS_5: case GI_SEEDS_30:
        case GI_BOMBCHUS_5: case GI_BOMBCHUS_10: case GI_BOMBCHUS_20:
        case GI_SHIELD_DEKU: case GI_SHIELD_HYLIAN:
        case GI_MILK: case GI_POTION_RED: case GI_POTION_GREEN: case GI_POTION_BLUE:
        case GI_FAIRY: case GI_FISH: case GI_BUGS: case GI_BLUE_FIRE: case GI_POE:
        case GI_MAGIC_SMALL: case GI_MAGIC_LARGE: return true;
        // Pinned filled Big Poe converts to Big Poe; an already-unbottled Big Poe defaults to finder heart.
        case GI_BIG_POE: return output;
        default: return false;
    }
}
inline int Combo_OOTFilledBottleRenewable(RandomizerGet rg) {
    switch (rg) {
        case RG_BOTTLE_WITH_MILK: return GI_MILK;
        case RG_BOTTLE_WITH_RED_POTION: return GI_POTION_RED;
        case RG_BOTTLE_WITH_GREEN_POTION: return GI_POTION_GREEN;
        case RG_BOTTLE_WITH_BLUE_POTION: return GI_POTION_BLUE;
        case RG_BOTTLE_WITH_FAIRY: return GI_FAIRY;
        case RG_BOTTLE_WITH_BLUE_FIRE: return GI_BLUE_FIRE;
        case RG_BOTTLE_WITH_POE: return GI_POE;
        case RG_BOTTLE_WITH_BIG_POE: return GI_BIG_POE;
        default: return 0;
    }
}
