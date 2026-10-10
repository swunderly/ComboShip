#pragma once
// Exact supported native MM projection of pinned common/item/progressive.c comboRenewable.
// Foreign ownership must resolve through its owning engine, never a local invented refill.
namespace Rando {
inline RandoItemId RenewableStockItem(RandoItemId item) {
    switch (item) {
        case RI_RECOVERY_HEART:
        case RI_DEKU_STICK:
        case RI_DEKU_NUT:
        case RI_DEKU_NUTS_5:
        case RI_DEKU_NUTS_10:
        case RI_BOMBS_5:
        case RI_BOMBS_10:
        case RI_BOMBCHU:
        case RI_BOMBCHU_5:
        case RI_BOMBCHU_10:
        case RI_ARROWS_10:
        case RI_ARROWS_30:
        case RI_ARROWS_50:
        case RI_SHIELD_HERO:
        case RI_MILK_REFILL:
        case RI_CHATEAU_ROMANI_REFILL:
        case RI_RED_POTION_REFILL:
        case RI_GREEN_POTION_REFILL:
        case RI_BLUE_POTION_REFILL:
        case RI_FAIRY_REFILL:
        case RI_MAGIC_BEAN:
        case RI_MAGIC_JAR_SMALL:
        case RI_MAGIC_JAR_BIG:
            return item;
        case RI_BOTTLE_RED_POTION:
            return RI_RED_POTION_REFILL;
        case RI_BOTTLE_MILK:
            return RI_MILK_REFILL;
        case RI_BOTTLE_CHATEAU_ROMANI:
            return RI_CHATEAU_ROMANI_REFILL;
        case RI_COMBO_FOREIGN:
            return RI_UNKNOWN;
        default:
            return RI_RECOVERY_HEART;
    }
}
} // namespace Rando
