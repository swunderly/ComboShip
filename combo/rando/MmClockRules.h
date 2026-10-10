#pragma once
#include <algorithm>
#include <cstdint>

// Adapted from OoTMM 41a2b7c0d7dbbb53fed928652f8271bc336c113f,
// packages/generator/src/common/time.c and src/mm/time_selector.c (MIT).
// Chronological bits: D1, N1, D2, N2, D3, N3. These helpers never use live
// wall-clock state for tracker reachability.
namespace ComboRando::MmClockRules {
constexpr uint32_t HalfTicks = 0x8000u;
constexpr uint32_t MaxSelectorTicks = 0x2c000u;
constexpr uint16_t Dawn = 0x4000u;
constexpr uint16_t Dusk = 0xc000u;

constexpr uint32_t Linear(int day, uint16_t time) {
    return static_cast<uint32_t>(day) * 0x10000u + static_cast<uint16_t>(time - Dawn);
}
constexpr uint32_t FromHalfDay(int half) { return 0x10000u + half * HalfTicks; }
constexpr uint32_t MoonCrash(uint8_t mask) {
    int end = 0;
    for (int i=0; i<6; ++i) if (mask & (1u << i)) end = i+1;
    return FromHalfDay(end);
}
constexpr uint32_t NewCycle(uint8_t mask) {
    for (int i=0; i<6; ++i) if (mask & (1u << i)) return FromHalfDay(i);
    return FromHalfDay(6);
}
constexpr bool IsMoonCrash(uint8_t mask, int day, uint16_t time) {
    const uint32_t linear = Linear(day, time);
    return linear >= MoonCrash(mask) && linear < 0x40000u;
}
constexpr uint32_t ResumeGrace(uint8_t mask, int day, uint16_t time) {
    const uint32_t linear = Linear(day, time);
    const uint32_t crash = MoonCrash(mask);
    constexpr uint32_t grace = 0x1e0;
    if (IsMoonCrash(mask, day, time) || (linear < crash && linear + grace >= crash))
        return crash - grace;
    return linear;
}
constexpr bool OwnsTime(uint8_t mask, int day, uint16_t time) {
    if (day < 1 || day > 3) return false;
    const int half = (day-1)*2 + ((time < Dawn || time >= Dusk) ? 1 : 0);
    return (mask & (1u << half)) != 0;
}
constexpr uint32_t MinutesToTicks(uint32_t minutes) { return ((minutes << 16) + 1439u) / 1440u; }
constexpr uint32_t TicksToMinutes(uint32_t ticks) { return (ticks * 1440u) >> 16; }
constexpr int SelectorAvailabilityHalf(uint32_t ticks) {
    uint32_t half = ticks/HalfTicks;
    if (ticks && !(ticks%HalfTicks)) --half;
    return half >= 6 ? 5 : static_cast<int>(half);
}
constexpr uint32_t FinalSelectorTarget(uint32_t ticks) {
    return ticks == MaxSelectorTicks ? MinutesToTicks(TicksToMinutes(ticks)-1) : ticks;
}
constexpr bool OwnsSelectorTarget(uint8_t mask, uint32_t ticks) {
    // A boundary can be the end of the owned half-day, or a jump to the start
    // of a later owned one (DtMoveTarget uses both). The time actor processes
    // any resulting boundary crossing immediately after the song.
    const int actual = std::min<int>(ticks/HalfTicks, 5);
    return (mask & (1u<<actual)) || (mask & (1u<<SelectorAvailabilityHalf(ticks)));
}
inline uint32_t SelectOwnedTime(uint8_t mask, uint32_t original, uint32_t current,
                                uint32_t proposed, bool forward) {
    // Source selector refuses to move beyond midnight on the final night.
    proposed = std::max(original, std::min(proposed, std::max(original, MaxSelectorTicks)));
    const int half = SelectorAvailabilityHalf(proposed);
    if (mask & (1u << half)) return proposed;
    if (forward) {
        for (int next=static_cast<int>(proposed/HalfTicks)+1; next<6; ++next)
            if (mask & (1u << next)) return std::min<uint32_t>(next*HalfTicks, MaxSelectorTicks);
        return current;
    }
    for (int previous=half-1; previous>=0; --previous) {
        if (!(mask & (1u << previous))) continue;
        const uint32_t minutes = TicksToMinutes((previous+1)*HalfTicks);
        return std::max(original, MinutesToTicks(minutes-30));
    }
    return original;
}
}
