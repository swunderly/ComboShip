#pragma once

namespace ComboRando {
// Timeless equipping, ordinary torch transfer, and unbreakable sticks are
// distinct abilities. Only mode 2 creates its own flame, on a usable stick.
inline constexpr bool SelfLightingSticks(int mode, bool canUseSticks) {
    return mode == 2 && canUseSticks;
}
} // namespace ComboRando
