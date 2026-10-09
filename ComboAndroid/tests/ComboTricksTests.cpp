#include "gui/ComboTricksModel.h"
#include <cassert>

int main() {
    CwTrickInfo a{7, "Lensless chest", "Collect without Lens of Truth using the visible collision.", "Stone Tower", "Intermediate", 1, 2, 1};
    using namespace ComboTricks;
    assert(Matches(a, 1, "LENS OF TRUTH", 0, 0, -1));
    assert(Matches(a, 1, "stone tower", 2, 1, 1));
    assert(!Matches(a, 1, "", 1, 0, -1));
    assert(!Matches(a, 1, "", 0, 2, -1));
    assert(!Matches(a, 1, "", 0, 0, 0));
    assert(!Matches(a, 1, "bomb jump", 0, 0, -1));
    a.enabled = 0;
    assert(Matches(a, 1, "", 0, 2, -1));
    a.description = nullptr; a.area = nullptr; a.tags = nullptr;
    assert(Matches(a, 1, "lensless", 0, 0, -1));
    assert(!Matches(a, 1, "collision", 0, 0, -1));
    const auto shadow = RestoredShadowDoorInfo("Shadow Door Unload", "Five stair trips", false);
    const auto frog = EarlyEyeballFrogInfo("Early Eyeball Frog", "Hold R with an equipped shield", true);
    assert(shadow.id == -1000 && frog.id == -1001 && shadow.id != frog.id);
    assert(Matches(frog, 0, "shield", 1, 1, 5));
    assert(Matches(frog, 0, "zora", 0, 0, -1));
    assert(!Matches(frog, 0, "", 2, 0, -1));
    assert(!Matches(frog, 0, "", 0, 2, -1));
    assert(Matches(shadow, 0, "stair", 1, 2, 5));
}
