#include "rando/MmEnhancementLogic.h"
#include <cassert>

int main() {
    using namespace ComboRando;
    for (int mode = 0; mode <= 2; ++mode) {
        assert(!CanPassGibdoDoor(mode, false, false));
        assert(!CanPassGibdoDoor(mode, false, true));
        assert(CanPassGibdoDoor(mode, true, true));
        assert(CanPassGibdoDoor(mode, true, false) == (mode == 2));
        assert(!CanSupplyGibdoPotion(mode, false, true, true));
        assert(CanSupplyGibdoPotion(mode, true, true, false));
        assert(CanSupplyGibdoPotion(mode, true, false, true) == (mode == 1));
    }
    assert(!CanPassGibdoDoor(3, true, false)); // malformed modes do not open doors
    assert(!CanSupplyGibdoPotion(3, true, false, true));
    assert(!CanWinDoggyRace(0, false));
    assert(!CanWinDoggyRace(1, false));
    assert(CanWinDoggyRace(2, false));
    assert(CanWinDoggyRace(0, true));
    assert(CanWinDoggyRace(1, true));
    assert(CanWinDoggyRace(2, true));
    assert(!CanWinDoggyRace(3, false));

    for (int frogs = 0; frogs < 16; ++frogs) {
        const bool cyan = frogs & 1;
        const bool pink = frogs & 2;
        const bool blue = frogs & 4;
        const bool white = frogs & 8;
        const int count = 1 + int(cyan) + int(pink) + int(blue) + int(white);
        for (int threshold = 1; threshold <= 5; ++threshold)
            assert(HasEnoughChoirFrogs(threshold, cyan, pink, blue, white) == (count >= threshold));
        assert(HasEnoughChoirFrogs(0, cyan, pink, blue, white) == (frogs == 15));
        assert(HasEnoughChoirFrogs(-1, cyan, pink, blue, white) == (frogs == 15));
        assert(HasEnoughChoirFrogs(99, cyan, pink, blue, white) == (frogs == 15));
    }
    assert(CanCrossOpenWaterSurface(true, false, false));
    assert(!CanCrossOpenWaterSurface(false, true, false));
    assert(!CanCrossOpenWaterSurface(false, false, true));
    assert(CanCrossOpenWaterSurface(false, true, true));
    return 0;
}
