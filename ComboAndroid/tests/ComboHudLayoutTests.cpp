#include "gui/ComboHudLayoutModel.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace ComboHud;
    assert(NativeMode(0, Anchor::Left) == 1);
    assert(NativeMode(1, Anchor::Left) == 3);
    assert(NativeMode(0, Anchor::Hidden) == 4);
    assert(NativeMode(1, Anchor::Hidden) == 1);
    for (int game = 0; game < 2; ++game) {
        for (int a = 0; a < 5; ++a) {
            const auto anchor = static_cast<Anchor>(a);
            assert(ReadMode(game, NativeMode(game, anchor)) == anchor);
        }
    }
    for (float aspect : {.5f, 4.0f / 3, 16.0f / 9, 21.0f / 9}) {
        const float width = LogicalWidth(aspect);
        for (auto anchor : {Anchor::Center, Anchor::Left, Anchor::Right}) {
            for (float x : {0.0f, 30.0f, 167.0f, 295.0f}) {
                const float rendered = CanvasX(x, anchor, width);
                assert(std::fabs(NativeX(rendered, anchor, width) - x) < .001f);
            }
        }
    }
    const float wide = LogicalWidth(16.0f / 9);
    assert(std::fabs(CanvasX(30, Anchor::Left, wide) - 30) < .001f);
    assert(std::fabs((wide - CanvasX(295, Anchor::Right, wide)) - 25) < .001f);
    assert(std::fabs(CanvasX(160, Anchor::Center, wide) - wide / 2) < .001f);
    assert(LogicalWidth(std::numeric_limits<float>::quiet_NaN()) == 320);
    assert(LogicalWidth(0) == 320);
    assert(LogicalWidth(100) == 2400);
    assert(Pixel(1.51f) == 2);
    assert(Pixel(-1.51f) == -2);
}
