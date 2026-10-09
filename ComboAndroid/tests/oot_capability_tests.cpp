#include "../rando/OotCapabilityRules.h"
#include <cassert>
#include <iostream>

using namespace ComboRando;
int main() {
    assert(!SelfLightingSticks(0,true)); // normal sticks need an external flame
    assert(!SelfLightingSticks(1,true)); // unbreakable does not self-ignite
    assert(SelfLightingSticks(2,true));
    assert(!SelfLightingSticks(2,false)); // no sticks, no ammo source, wrong age or restricted use
    assert(!SelfLightingSticks(-1,true));
    assert(!SelfLightingSticks(3,true));
    // Lighting a reachable ground torch does not bypass access or opening its reward.
    const bool selfFlame=SelfLightingSticks(2,true);
    auto reward=[&](bool accessible,bool canOpenChest) {
        return accessible && selfFlame && canOpenChest;
    };
    assert(reward(true,true));
    assert(!reward(false,true));
    assert(!reward(true,false));
    std::cout << "PASS: self-lighting mode versus unbreakable sticks and retained item/access requirements\n";
}
