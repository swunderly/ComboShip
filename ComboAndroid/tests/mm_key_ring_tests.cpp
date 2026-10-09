#include "Rando/KeyRings.h"
#include <array>
#include <cassert>
#include <iostream>
using namespace Rando;
static std::vector<RandoItemId> VanillaKeys() {
    std::vector<RandoItemId> pool;
    for (const auto& r : keyRings) pool.insert(pool.end(), r.maximum, r.key);
    return pool;
}
int main() {
    static_assert(RI_WOODFALL_KEY_RING == RI_MAX_TRAP + 1);
    static_assert(RI_STONE_TOWER_KEY_RING == RI_WOODFALL_KEY_RING + 3);
    static_assert(RO_KEY_RING_STONE_TOWER == RO_KEY_RING_WOODFALL + 3);
    for (const auto& r : keyRings) {
        std::array<int8_t,4> inventory{{-1,-1,-1,-1}}, found{{-1,-1,-1,-1}};
        assert(GrantKeyRingCounters(inventory[r.dungeon], found[r.dungeon], r.maximum) == r.maximum);
        assert(inventory[r.dungeon] == r.maximum && found[r.dungeon] == r.maximum);
        for (int other = 0; other < 4; ++other) if (other != r.dungeon) assert(inventory[other] == -1 && found[other] == -1);
        inventory[r.dungeon] = 0;
        assert(GrantKeyRingCounters(inventory[r.dungeon], found[r.dungeon], r.maximum) == 0);
        assert(inventory[r.dungeon] == 0 && found[r.dungeon] == r.maximum);
    }
    int8_t spent = 0, collected = 2;
    assert(GrantKeyRingCounters(spent, collected, 3) == 1 && spent == 1 && collected == 3);
    int8_t inventory[4] = {0,0,0,0}, found[4] = {1,2,0,2};
    KeyRingTrialSnapshot retry;
    retry.Capture(inventory, found);
    GrantKeyRingCounters(inventory[1], found[1], 3);
    retry.Restore(inventory, found);
    assert(inventory[1] == 0 && found[1] == 2);
    GrantKeyRingCounters(inventory[3], found[3], 4);
    assert(inventory[3] == 2 && found[3] == 4 && found[1] == 2);
    std::array<unsigned,RO_MAX> options{};
    auto vanilla = VanillaKeys(), pool = vanilla;
    assert(vanilla.size() == 9);
    ReplaceKeyRingPool(options, {}, pool);
    assert(pool == vanilla);
    for (const auto& r : keyRings) options[r.option] = RO_GENERIC_YES;
    pool = vanilla;
    ReplaceKeyRingPool(options, {}, pool);
    assert(pool.size() == 4);
    for (const auto& r : keyRings) assert(std::count(pool.begin(), pool.end(), r.ring) == 1);
    // Excluded checks retain their vanilla advancement item in native pool;
    // replacement consumes those keys too. No source keys means no new ring.
    pool = {RI_RUPEE_BLUE};
    ReplaceKeyRingPool(options, {}, pool);
    assert(pool == std::vector<RandoItemId>{RI_RUPEE_BLUE});
    options.fill(0); options[RO_KEY_RING_SNOWHEAD] = RO_GENERIC_YES;
    pool = vanilla;
    ReplaceKeyRingPool(options, {}, pool);
    assert(pool.size() == 7 && std::count(pool.begin(),pool.end(),RI_SNOWHEAD_SMALL_KEY) == 0);
    assert(std::count(pool.begin(),pool.end(),RI_SNOWHEAD_KEY_RING) == 1);
    assert(std::count(pool.begin(),pool.end(),RI_STONE_TOWER_SMALL_KEY) == 4);
    // Explicit starting ring works even with its replacement toggle disabled.
    pool = vanilla;
    ReplaceKeyRingPool(options, {RI_STONE_TOWER_KEY_RING}, pool);
    assert(std::count(pool.begin(),pool.end(),RI_STONE_TOWER_SMALL_KEY) == 0);
    assert(std::count(pool.begin(),pool.end(),RI_STONE_TOWER_KEY_RING) == 0);
    // Start With all selected rings removes every original key.
    options.fill(0); for (const auto& r : keyRings) options[r.option] = RO_GENERIC_YES;
    pool = vanilla;
    ReplaceKeyRingPool(options, {RI_WOODFALL_KEY_RING,RI_SNOWHEAD_KEY_RING,RI_GREAT_BAY_KEY_RING,RI_STONE_TOWER_KEY_RING}, pool);
    assert(pool.empty());
    std::cout << "Four capped ring grants, spent/duplicate keys, fill rollback and mixed/starting pool accounting passed.\n";
}
