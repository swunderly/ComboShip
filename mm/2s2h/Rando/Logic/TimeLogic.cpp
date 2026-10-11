#include "Logic.h"
#include <algorithm>
#include <spdlog/spdlog.h>

extern "C" {
#include "ShipUtils.h"
}

namespace Rando {
namespace Logic {
namespace TimeLogic {

uint64_t EvaluateConditionTimes(const std::function<bool()>& condition, uint64_t timeSlices) {
    struct RestoreTime {
        uint64_t previous = gCurrentRegionTime;
        ~RestoreTime() {
            gCurrentRegionTime = previous;
            // An enclosing evaluation cannot tell which times this one read.
            gCurrentRegionTimeRead = true;
        }
    } restore;
    struct Result {
        uint64_t allowed = 0;
#ifdef COMBO_BUILD
        bool anyDomain = false;
        ComboRando::Entrances::MmFlagDomain domain = ComboRando::Entrances::mmFlagSource;
#endif
    } result;
    const auto evaluate = [&](uint64_t times) {
        gCurrentRegionTime = times;
#ifdef COMBO_BUILD
        if (NativeMmErContextsEnabled()) {
            const auto evaluated = ComboRando::Entrances::EvaluateMmFlags(condition);
            if (evaluated.allowed) {
                result.allowed |= times;
                result.domain =
                    result.anyDomain ? ComboRando::Entrances::Merge(result.domain, evaluated.domain) : evaluated.domain;
                result.anyDomain = true;
            }
        } else
#endif
            if (condition())
            result.allowed |= times;
    };
    // Most predicates never read the time: one evaluation with every slice then
    // stands for each slice. Only a predicate that read it is evaluated per slice.
    gCurrentRegionTimeRead = false;
    if (timeSlices & TIME_ALL_SLICES)
        evaluate(timeSlices & TIME_ALL_SLICES);
    if (gCurrentRegionTimeRead) {
        result = {};
        for (int slice = 0; slice < TIME_SLICE_COUNT; ++slice) {
            const uint64_t bit = TIME_BIT_ONE << slice;
            if (timeSlices & bit)
                evaluate(bit);
        }
    }
#ifdef COMBO_BUILD
    ComboRando::Entrances::mmFlagResult = result.anyDomain ? result.domain : ComboRando::Entrances::mmFlagSource;
#endif
    return result.allowed;
}

// Core expansion function - expands accessible time forward with stay restrictions
// Implements sequential expansion: if a stay restriction fails, expansion stops permanently
// For unrestricted regions without Clock Shuffle: fast bitwise fill across all time slices (O(1))
// For Clock Shuffle or restricted regions: sequential expansion respecting boundaries
uint64_t ExpandTimeForward(uint64_t timeSlices, const RandoRegion& region) {
    const uint64_t owned = GetOwnedTimeSlices();
    uint64_t expanded = timeSlices & owned;
    bool canWait = false;
    for (int slice = 0; slice < TIME_SLICE_COUNT; ++slice) {
        const uint64_t bit = TIME_BIT_ONE << slice;
        if (timeSlices & bit) {
            canWait = true;
        } else if (canWait) {
            // OoTMM pathfind.ts propagates future time; macros_mm.yml filters
            // check/exit predicates by the clocks owned. A missing clock is a
            // skipped interval, so its closed-door stay restriction cannot
            // prevent waiting into a later owned clock.
            if (!(owned & bit))
                continue;
            const auto restriction = region.timeStayRestrictions.find(static_cast<TimeSlice>(slice));
            if (restriction != region.timeStayRestrictions.end() && !EvaluateConditionTimes(restriction->second, bit)) {
                canWait = false;
            } else {
                expanded |= bit;
            }
        }
    }
    return expanded & owned;
}

// Owned time calculation - aggregates all owned half-day time slices
// Minutes after Day 1 06:00 at which each TimeSlice begins, in enum order.
static constexpr uint16_t kSliceStartMinutes[TIME_SLICE_COUNT] = {
    0,    60,   120,  240,  465,  540,  600,              // Day 1
    720,  840,  900,  960,  1020, 1080, 1230, 1320, 1380, // Night 1
    1440, 1500, 1560, 1680, 1770, 1920, 2040,             // Day 2
    2160, 2280, 2340, 2400, 2460, 2520, 2760, 2820, 2850, // Night 2
    2880, 2940, 3000, 3120, 3210, 3300,                   // Day 3
    3600, 3720, 3780, 3840, 3900, 3960, 4200, 4260,       // Night 3
};
static_assert(kSliceStartMinutes[TIME_NIGHT3_AM_05_00] == 4260, "one start time per TimeSlice");

uint64_t CurrentTimeSlice() {
    // MM's day counter advances at dawn, so after-midnight night times still belong to that day.
    const int day = std::clamp<int>(gSaveContext.save.day, 1, 3);
    const int clockMinutes = gSaveContext.save.time * (24 * 60) / 0x10000;
    const int now = (day - 1) * 24 * 60 + (clockMinutes - 6 * 60 + 24 * 60) % (24 * 60);
    int current = 0;
    for (int slice = 0; slice < TIME_SLICE_COUNT; ++slice)
        if (kSliceStartMinutes[slice] <= now)
            current = slice;
    return TIME_BIT_ONE << current;
}

uint64_t GetOwnedTimeSlices() {
    if (!RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE]) {
        return TIME_ALL_SLICES;
    }

    uint64_t timeSlices = 0;
    for (int halfDayIndex = 0; halfDayIndex < 6; ++halfDayIndex) {
        if (OwnsClockHalfDay(halfDayIndex)) {
            timeSlices |= GetHalfDayTimeMask(halfDayIndex);
        }
    }

    // No invented Day 1 capability. Generation grants one initial clock;
    // a malformed/empty clock inventory has no reachable half-day.
    return timeSlices;
}

// Validation helper for clock ownership during logic generation
void ValidateRegionTimeOwnership(RandoRegionId regionId, RandoCheckId checkId, uint64_t regionTime,
                                 const char* context) {
    if (!SettingClocks())
        return;

    if (!HasAnyOwnedTime(regionTime)) {
        auto& region = Regions[regionId];
        SPDLOG_ERROR("CLOCK SHUFFLE VALIDATION FAILED ({})!", context);
        SPDLOG_ERROR("Check: {}", Rando::StaticData::Checks[checkId].name);
        SPDLOG_ERROR("Region: {} - {}", Ship_GetSceneName(region.sceneId), region.name);
        SPDLOG_ERROR("Region time mask: 0x{:X}", regionTime);
        SPDLOG_ERROR("Owned clocks: D1={} N1={} D2={} N2={} D3={} N3={}", OwnsClockHalfDay(0), OwnsClockHalfDay(1),
                     OwnsClockHalfDay(2), OwnsClockHalfDay(3), OwnsClockHalfDay(4), OwnsClockHalfDay(5));
        assert(false && "Check placed in unowned time period during Clock Shuffle!");
    }
}

} // namespace TimeLogic
} // namespace Logic
} // namespace Rando
