#pragma once
// trade.c kMmFierceDeityAdult and item_add.c addItemFierceDeityAdult from OoTMM development commit 41a2b7c0
// (not in v32.3).
// Native constants are supplied by the MM engine. No unrelated slot is occupied.
namespace ComboAdultMaskInventory {
constexpr u8 AdultItem = 0xbc;
inline bool Grant(u8& owned, u8& slot, unsigned variant) {
    if (variant > 1 || owned > 3)
        return false;
    // Preserve a legacy native Fierce Deity item when first extending this slot.
    if (slot == ITEM_MASK_FIERCE_DEITY)
        owned |= 1;
    else if (slot == AdultItem)
        owned |= 2;
    const u8 item = variant == 0 ? ITEM_MASK_FIERCE_DEITY : AdultItem;
    if (slot == ITEM_NONE)
        slot = item;
    owned |= u8(1u << variant);
    return true;
}
inline bool HasFierceDeity(u8 owned, u8 slot) {
    return slot == ITEM_MASK_FIERCE_DEITY || (owned <= 3 && (owned & 1) != 0);
}
inline u8 Next(u8 current, u8 owned) {
    // Exact two-entry specialization of pinned comboGetNextTrade.
    if (owned > 3)
        return ITEM_NONE;
    int bit = current == ITEM_MASK_FIERCE_DEITY ? 0 : current == AdultItem ? 1 : -1;
    if (bit < 0)
        return ITEM_NONE;
    const int initial = bit;
    for (;;) {
        bit = (bit + 1) % 2;
        if ((owned & (1u << bit)) || bit == initial)
            return bit == 0 ? ITEM_MASK_FIERCE_DEITY : AdultItem;
    }
}
inline u8 NextVisible(u8 current, u8 owned, bool fierceGiven) {
    u8 probe = current;
    for (unsigned i = 0; i < 2; ++i) {
        const u8 next = Next(probe, owned);
        if (next == ITEM_NONE || next == current)
            return ITEM_NONE;
        if (!fierceGiven || next != ITEM_MASK_FIERCE_DEITY)
            return next;
        probe = next;
    }
    return ITEM_NONE;
}
inline u8 Visible(u8 current, u8 owned, bool fierceGiven) {
    return fierceGiven && current == ITEM_MASK_FIERCE_DEITY ? NextVisible(current, owned, true) : current;
}

template <class Vertex> inline void CopyExtraIconQuad(Vertex* out, const Vertex* source) {
    for (int i = 0; i < 4; ++i)
        out[i] = source[i];
    out[0].v.ob[0] += 0x10;
    out[2].v.ob[0] += 0x10;
    out[0].v.ob[1] -= 0x10;
    out[1].v.ob[1] -= 0x10;
}

} // namespace ComboAdultMaskInventory
