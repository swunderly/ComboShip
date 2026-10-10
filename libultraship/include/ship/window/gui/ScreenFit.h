#pragma once
#include <algorithm>
#include <cmath>

namespace Ship::ScreenFit {
struct Rect { float x = 0, y = 0, width = 0, height = 0; };
// Balanced limits extra peripheral view to 16:9; Full width permits wider views. Both
// preserve at least the original 4:3 presentation on portrait, square and docked views.
inline Rect Fit(float width, float height, int mode) {
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) return {};
    const float available = width / height;
    const float aspect = mode == 3 ? 4.0f / 3.0f :
        std::clamp(available, 4.0f / 3.0f, mode == 2 ? std::max(4.0f / 3.0f, available) : 16.0f / 9.0f);
    const float w = std::min(width, height * aspect);
    const float h = std::min(height, width / aspect);
    return {(width - w) * .5f, (height - h) * .5f, w, h};
}
}
