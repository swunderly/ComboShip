#pragma once

namespace ComboRando {
// Matches OpenAllHours.cpp: timed shop doors may become unrestricted only
// when the randomizer has not replaced overworld doors with key locks.
inline constexpr bool ShopsOpenAllHours(bool logicEnabled, bool lockedOverworldDoors) {
    return logicEnabled && !lockedOverworldDoors;
}
} // namespace ComboRando
