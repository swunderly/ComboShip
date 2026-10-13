#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include "../menu/ComboCheckTrackerABI.h"

namespace ComboChecks {
enum Visibility { Always, Paused, Toggle, Hold, OnScreen };

// One edge latch is shared across both games. A held chord must never retrigger
// on a game/slot transition, after editing the binding, or while using the menu.
struct VisibilityState {
    bool open = false;
    bool down = false;
    int context = -1;
    int mode = -1;
    uint32_t binding = 0;
    bool Update(int newContext, int newMode, uint32_t mask, uint32_t buttons, bool enabled, bool loaded, bool paused,
                bool menu, bool touch = false) {
        const bool held = mask != 0 && (buttons & mask) == mask;
        if (context != newContext || mode != newMode || binding != mask) {
            down = held;
            context = newContext;
            mode = newMode;
            binding = mask;
        }
        if (enabled && loaded && !menu && newMode == Toggle && held && !down)
            open = !open;
        if (enabled && loaded && !menu && newMode == OnScreen && touch)
            open = !open;
        down = held;
        if (!enabled || !loaded)
            return false;
        if (newMode == Always)
            return true;
        if (newMode == Paused)
            return paused;
        if (newMode == Hold)
            return held && !menu;
        return open;
    }
};

// What a touch at (x, y), in screen fractions, lands on. Each tracker publishes box 0 for its window and boxes 1-4
// for its open popups (unused boxes are empty). A popup always takes the touch, and so does a window during play, as
// since 0.1.11. On the pause screen a window leaves the drawn touch buttons their touches, so the C buttons can
// assign items; the touch overlay still keeps its sticks off the tracker.
enum TrackerTouch { TouchGame, TouchTracker, TouchTrackerUnderButtons };
template <std::size_t Trackers>
inline int TrackerTouchAt(const float (&boxes)[Trackers][5][4], bool paused, float x, float y) {
    const auto inside = [x, y](const float* b) {
        return b[2] > b[0] && b[3] > b[1] && x >= b[0] && x <= b[2] && y >= b[1] && y <= b[3];
    };
    for (const auto& tracker : boxes)
        for (int i = 1; i < 5; ++i)
            if (inside(tracker[i]))
                return TouchTracker;
    for (const auto& tracker : boxes)
        if (inside(tracker[0]))
            return paused ? TouchTrackerUnderButtons : TouchTracker;
    return TouchGame;
}

inline bool IsCollected(uint32_t flags) {
    return (flags & COMBO_CHECK_COLLECTED) != 0;
}
inline bool IsSkipped(uint32_t flags) {
    return !IsCollected(flags) && (flags & COMBO_CHECK_SKIPPED) != 0;
}
inline bool OutOfLogic(uint32_t flags) {
    return !IsCollected(flags) && (flags & COMBO_CHECK_LOGIC_KNOWN) && !(flags & COMBO_CHECK_AVAILABLE);
}
inline bool PassesFilters(uint32_t flags, bool collected, bool skipped, bool onlyAvailable) {
    if (flags & COMBO_CHECK_AREA_HIDDEN)
        return !onlyAvailable;
    if (IsCollected(flags) && !collected)
        return false;
    if (IsSkipped(flags) && !skipped)
        return false;
    return !onlyAvailable || !OutOfLogic(flags);
}
struct Totals {
    int total = 0, collected = 0, skipped = 0;
    bool hiddenArea = false;
    void Add(uint32_t flags) {
        if (flags & COMBO_CHECK_AREA_HIDDEN) {
            hiddenArea = true;
            return;
        }
        ++total;
        if (IsCollected(flags))
            ++collected;
        else if (IsSkipped(flags))
            ++skipped;
    }
};
// A check may be reachable from several areas (for example MM's postboxes).
// Show it in each relevant area, but count that check only once overall.
struct UniqueTotals : Totals {
    std::unordered_set<int32_t> ids;
    void Add(int32_t id, uint32_t flags) {
        if (flags & COMBO_CHECK_AREA_HIDDEN) {
            hiddenArea = true;
            return;
        }
        if (ids.insert(id).second)
            Totals::Add(flags);
    }
};
} // namespace ComboChecks
