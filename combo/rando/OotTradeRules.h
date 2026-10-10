#pragma once

namespace ComboRando {
// The native restoration selects the Prescription check while holding a shield
// with an earlier adult trade item. It never thaws King Zora or supplies an item.
inline constexpr bool CanClaimKingZoraTrade(bool adult, bool thawed, bool canSpeakZora, bool prescription,
                                            bool earlyFrogLogic, bool canShield, bool earlierTradeItem) {
    return adult && thawed && canSpeakZora && (prescription || (earlyFrogLogic && canShield && earlierTradeItem));
}
} // namespace ComboRando
