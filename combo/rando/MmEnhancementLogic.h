#pragma once

namespace ComboRando {
// EnTalkGibud only offers the peaceful conversation while wearing the Gibdo
// Mask. NoTrade bypasses the requested item, not that conversation requirement.
inline constexpr bool CanPassGibdoDoor(int logicalMode, bool ownsGibdoMask, bool hasRequestedResource) {
    return ownsGibdoMask && (logicalMode == 2 || hasRequestedResource);
}
inline constexpr bool CanSupplyGibdoPotion(int logicalMode, bool ownsBottle, bool blueRefill, bool redRefill) {
    return ownsBottle && (blueRefill || (logicalMode == 1 && redRefill));
}

// Always mode forces the chosen dog's race speed without checking Mask of
// Truth. The conditional mode still checks ownership of that mask.
inline constexpr bool CanWinDoggyRace(int logicalMode, bool ownsMaskOfTruth) {
    return ownsMaskOfTruth || logicalMode == 2;
}

// The yellow frog always counts as one. Zero is the logic-disabled/vanilla
// value; all four returned-frog flags are still required in that mode.
inline bool HasEnoughChoirFrogs(int logicalCount, bool cyan, bool pink, bool blue, bool white) {
    const int required = logicalCount >= 1 && logicalCount <= 5 ? logicalCount : 5;
    const int saved = 1 + int(cyan) + int(pink) + int(blue) + int(white);
    return saved >= required;
}

// Infinite Deku hops renew the surface-hop budget. They do not grant diving,
// Zora swimming, carrying Mikau, or underwater doorway access.
inline constexpr bool CanCrossOpenWaterSurface(bool ownsSwim, bool canBeDeku, bool logicalInfiniteHops) {
    return ownsSwim || (canBeDeku && logicalInfiniteHops);
}
} // namespace ComboRando
