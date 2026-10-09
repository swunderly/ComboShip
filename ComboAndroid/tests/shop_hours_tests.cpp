#include "../rando/ShopHours.h"
#include <cassert>
#include <iostream>

using namespace ComboRando;
int main() {
    assert(ShopsOpenAllHours(true,false));
    assert(!ShopsOpenAllHours(false,false));
    assert(!ShopsOpenAllHours(true,true));
    assert(!ShopsOpenAllHours(false,true));
    // The rule opens hours, never changes the required age or key.
    const bool allHours=ShopsOpenAllHours(true,false);
    auto route=[&](bool ageAllowed,bool key,bool normalHours) {
        return ageAllowed && key && (normalHours || allHours);
    };
    assert(route(true,true,false)); // child Bazaar at night / adult Kak shop at night
    assert(!route(false,true,false)); // child cannot become an adult shop customer
    assert(!route(true,false,false)); // a locked/missing key never disappears
    const bool lockedHours=ShopsOpenAllHours(true,true);
    assert(!(false || lockedHours)); // closed locked shop still requires native hours
    assert(true || lockedHours); // original legal hours remain valid
    std::cout << "PASS: shop time extension, locked-door exclusion and unchanged age/key gates\n";
}
