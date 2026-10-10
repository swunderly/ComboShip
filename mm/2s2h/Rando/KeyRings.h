#pragma once
/* MM key rings adapted from OoTMM transform.ts / item_add.c,
 * revision 41a2b7c0d7dbbb53fed928652f8271bc336c113f.
MIT License

Copyright (c) 2020-2022 OoTMM Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#include "Types.h"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace Rando {
struct KeyRingDefinition {
    RandoItemId ring;
    RandoItemId key;
    RandoOptionId option;
    int dungeon;
    int maximum;
};
inline constexpr KeyRingDefinition keyRings[] = {
    { RI_WOODFALL_KEY_RING, RI_WOODFALL_SMALL_KEY, RO_KEY_RING_WOODFALL, 0, 1 },
    { RI_SNOWHEAD_KEY_RING, RI_SNOWHEAD_SMALL_KEY, RO_KEY_RING_SNOWHEAD, 1, 3 },
    { RI_GREAT_BAY_KEY_RING, RI_GREAT_BAY_SMALL_KEY, RO_KEY_RING_GREAT_BAY, 2, 1 },
    { RI_STONE_TOWER_KEY_RING, RI_STONE_TOWER_SMALL_KEY, RO_KEY_RING_STONE_TOWER, 3, 4 },
};
inline const KeyRingDefinition* FindKeyRing(RandoItemId item) {
    for (const auto& ring : keyRings)
        if (ring.ring == item)
            return &ring;
    return nullptr;
}
// Found keys represent lifetime collection, including keys already spent on doors.
// A repeated ring must not refill those spent keys.
inline int GrantKeyRingCounters(int8_t& inventory, int8_t& found, int maximum) {
    const int collected = std::max(0, static_cast<int>(found));
    const int delta = std::max(0, maximum - collected);
    if (delta) {
        inventory = static_cast<int8_t>(std::max(0, static_cast<int>(inventory)) + delta);
        found = static_cast<int8_t>(collected + delta);
    }
    return delta;
}
// Owned by the native fill invocation, never by runtime item delivery.
struct KeyRingTrialSnapshot {
    int8_t inventory[4] = {};
    int8_t found[4] = {};
    bool active = false;
    void Capture(const int8_t* currentInventory, const int8_t* currentFound) {
        std::copy_n(currentInventory, 4, inventory);
        std::copy_n(currentFound, 4, found);
        active = true;
    }
    void Restore(int8_t* currentInventory, int8_t* currentFound) const {
        std::copy_n(inventory, 4, currentInventory);
        std::copy_n(found, 4, currentFound);
    }
};
// Replace only pool entries; every physical check remains available for filler.
// Starting rings remove all keys even if their dungeon's ring toggle is disabled.
template <typename Options>
inline void ReplaceKeyRingPool(const Options& options, const std::vector<RandoItemId>& starting,
                               std::vector<RandoItemId>& pool) {
    for (const auto& ring : keyRings) {
        const bool startsWithRing = std::find(starting.begin(), starting.end(), ring.ring) != starting.end();
        if (!options[ring.option] && !startsWithRing)
            continue;
        const auto oldSize = pool.size();
        pool.erase(std::remove(pool.begin(), pool.end(), ring.key), pool.end());
        if (!startsWithRing && pool.size() != oldSize)
            pool.push_back(ring.ring);
    }
}
} // namespace Rando
