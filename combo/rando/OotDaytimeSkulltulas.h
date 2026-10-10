#pragma once

namespace ComboRando {
// The caller supplies whether this exact check has a verified daytime actor
// for the current age. Ordinary night-time/Sun's Song policy remains intact
// for every unverified spawn and for enhancement-only activation.
inline constexpr bool CanCollectNightSkulltula(bool day, bool night, bool canPlaySunsSong, bool expectSunsSong,
                                               bool logicalAlwaysSpawn, bool verifiedDayActor) {
    return (logicalAlwaysSpawn && verifiedDayActor && (day || night)) ||
           (night && (canPlaySunsSong || !expectSunsSong));
}
} // namespace ComboRando
