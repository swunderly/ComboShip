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

} // namespace ComboRando
