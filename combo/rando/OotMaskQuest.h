#pragma once

namespace ComboRando {

// OoTMM supplies all eight OoT masks as items; its Mask Shop is the game portal.
// Preserve serialized mode IDs, but resolve legacy Vanilla requests before fill.
// Existing seeds keep their settings and placements. Legacy Vanilla inventories
// are backfilled as starting masks because their pool contains no mask items.
constexpr int NormalizeOotMaskQuestForGeneration(int mode) {
    return mode == 0 ? 2 : mode;
}

constexpr bool OotMaskQuestHasStartingMasks(int mode) {
    return mode == 0 || mode == 1;
}

// The mode the Randomizer menu shows for a stored one; the stored value is kept. Vanilla is no longer offered: the
// next seed generates it as Shuffle, and a seed already made with it starts with the masks, as Completed does.
constexpr int OotMaskQuestShownMode(int mode, bool nextSeed) {
    return mode != 0 ? mode : nextSeed ? NormalizeOotMaskQuestForGeneration(mode) : 1;
}

} // namespace ComboRando
