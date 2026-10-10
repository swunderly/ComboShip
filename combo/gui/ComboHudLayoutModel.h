#pragma once

#include <algorithm>
#include <cmath>

namespace ComboHud {
enum class Anchor { Original, Center, Left, Right, Hidden };

inline int NativeMode(int game, Anchor anchor) {
    constexpr int oot[] = { 0, 3, 1, 2, 4 };
    constexpr int mm[] = { 0, 2, 3, 4, 1 };
    const int index = std::clamp(static_cast<int>(anchor), 0, 4);
    return game == 0 ? oot[index] : mm[index];
}

inline Anchor ReadMode(int game, int mode) {
    for (int i = 0; i < 5; ++i)
        if (NativeMode(game, static_cast<Anchor>(i)) == mode)
            return static_cast<Anchor>(i);
    return Anchor::Original;
}

inline float LogicalWidth(float aspect) {
    if (!std::isfinite(aspect) || aspect <= 0)
        return 320;
    return std::clamp(aspect * 240.0f, 80.0f, 2400.0f);
}

// The engines keep a 320 x 240 logical canvas and extend it symmetrically on
// widescreen. Side anchors follow its left/right edge; centered values do not.
inline float CanvasX(float value, Anchor anchor, float width) {
    const float padding = (width - 320.0f) / 2.0f;
    if (anchor == Anchor::Left)
        return value;
    if (anchor == Anchor::Right)
        return value + 2.0f * padding;
    return value + padding;
}

inline float NativeX(float canvas, Anchor anchor, float width) {
    const float padding = (width - 320.0f) / 2.0f;
    if (anchor == Anchor::Left)
        return canvas;
    if (anchor == Anchor::Right)
        return canvas - 2.0f * padding;
    return canvas - padding;
}

inline int Pixel(float value) {
    return static_cast<int>(std::lround(value));
}
} // namespace ComboHud
