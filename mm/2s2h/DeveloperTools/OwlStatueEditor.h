#pragma once

// C-compatible Soaring test used by native Soaring and the pause owl warp. Like vanilla, any set bit opens
// the owl map: bit 15 alone is the index-warp state PauseOwlWarp handles. The 10-bit mask is for the editor.
#define MM_OWL_STATUE_COUNT 10u
#define MM_OWL_STATUE_ACTIVATION_MASK ((1u << MM_OWL_STATUE_COUNT) - 1u)
#define MM_HAS_ACTIVATED_OWL_STATUES(flags) ((flags) != 0u)

#ifdef __cplusplus
#include <cstdint>

namespace MmOwlEditor {
// Native owl IDs 0..9 are activation bits. Entrance (10) and the remaining
// upper bits are not owl destinations; editing must leave them untouched.
inline constexpr unsigned kStatueCount = MM_OWL_STATUE_COUNT;
inline constexpr uint16_t kStatueMask = MM_OWL_STATUE_ACTIVATION_MASK;

inline constexpr bool IsActivated(uint16_t flags, unsigned owlId) {
    return owlId < kStatueCount && (flags & (1u << owlId)) != 0;
}

inline constexpr uint16_t SetActivated(uint16_t flags, unsigned owlId, bool activated) {
    if (owlId >= kStatueCount) {
        return flags;
    }
    const uint16_t bit = static_cast<uint16_t>(1u << owlId);
    return activated ? static_cast<uint16_t>(flags | bit) : static_cast<uint16_t>(flags & ~bit);
}

inline constexpr uint16_t SetAllActivated(uint16_t flags, bool activated) {
    return activated ? static_cast<uint16_t>(flags | kStatueMask) : static_cast<uint16_t>(flags & ~kStatueMask);
}

inline constexpr unsigned ActivatedCount(uint16_t flags) {
    unsigned count = 0;
    for (unsigned owlId = 0; owlId < kStatueCount; ++owlId) {
        count += IsActivated(flags, owlId) ? 1u : 0u;
    }
    return count;
}
} // namespace MmOwlEditor
#endif
