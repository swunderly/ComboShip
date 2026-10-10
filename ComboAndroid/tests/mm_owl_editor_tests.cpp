#include "OwlStatueEditor.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

int main() {
    using namespace MmOwlEditor;
    static_assert(kStatueMask == 0x03FF);
    const uint16_t unrelated = 0xA400;
    // The old editor wrote 0xFFFF. Clearing the real statues must leave its
    // reserved bits intact. Like vanilla, any stored bit opens the owl map;
    // bit 15 alone is the index-warp state PauseOwlWarp handles.
    const auto clearedLegacyAll = SetAllActivated(0xFFFF, false);
    assert(clearedLegacyAll == 0xFC00);
    assert(ActivatedCount(clearedLegacyAll) == 0);
    assert(MM_HAS_ACTIVATED_OWL_STATUES(clearedLegacyAll));
    assert(MM_HAS_ACTIVATED_OWL_STATUES(1u << 15));
    assert(!MM_HAS_ACTIVATED_OWL_STATUES(0u));
    for (unsigned id = 0; id < kStatueCount; ++id) {
        const auto activated = SetActivated(unrelated, id, true);
        assert(MM_HAS_ACTIVATED_OWL_STATUES(activated));
        assert(activated == (unrelated | (1u << id)));
        assert(ActivatedCount(activated) == 1);
        for (unsigned other = 0; other < kStatueCount; ++other) {
            assert(IsActivated(activated, other) == (id == other));
        }
        assert(SetActivated(activated, id, false) == unrelated);
    }

    // Cover every stored flag value, including unknown future/non-statue bits.
    // All/clear and individual toggles must never turn an entrance/reserved bit
    // into a destination or remove one that already belonged to the save.
    for (unsigned value = 0; value <= std::numeric_limits<uint16_t>::max(); ++value) {
        const auto flags = static_cast<uint16_t>(value);
        const auto all = SetAllActivated(flags, true);
        const auto none = SetAllActivated(flags, false);
        assert((all & kStatueMask) == kStatueMask);
        assert((none & kStatueMask) == 0);
        assert((all & ~kStatueMask) == (flags & ~kStatueMask));
        assert((none & ~kStatueMask) == (flags & ~kStatueMask));
        assert(ActivatedCount(all) == 10);
        assert(ActivatedCount(none) == 0);
        assert(MM_HAS_ACTIVATED_OWL_STATUES(all));
        assert(MM_HAS_ACTIVATED_OWL_STATUES(none) == (none != 0));
        assert(MM_HAS_ACTIVATED_OWL_STATUES(flags) == (flags != 0));
        assert(SetAllActivated(all, true) == all);
        assert(SetAllActivated(none, false) == none);
        for (unsigned id = 0; id < kStatueCount; ++id) {
            const auto changed = SetActivated(flags, id, !IsActivated(flags, id));
            assert(static_cast<unsigned>(changed ^ flags) == (1u << id));
        }
        for (unsigned invalid : {10u, 15u, 16u, 31u, 32u, std::numeric_limits<unsigned>::max()}) {
            assert(!IsActivated(flags, invalid));
            assert(SetActivated(flags, invalid, true) == flags);
            assert(SetActivated(flags, invalid, false) == flags);
        }
    }
    std::cout << "All ten statue bits, all 65536 flag values, invalid IDs, and legacy upper-only flags passed.\n";
}
