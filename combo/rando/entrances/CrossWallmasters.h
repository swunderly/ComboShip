// Full Wallmaster Shuffle's cross-game arrivals (limits design, Stage 3; F4 step 10, Wallmaster rows only): the
// frozen bit order of both games' Wallmaster destinations and the per-age masks the two reachability oracles
// exchange in the cross-world fixpoint (CrossWorldRando.h QueryAgeSongWorlds and QuerySongWorlds,
// OracleFns::GetEntranceSourcesForAge and SetEntranceAgeArrivals).
//
// OoTMM v32.3 places a route by adding an exit from the source's area to the destination's (entrances.ts:203-217);
// its pathfinder keeps Link's age on every exit and ORs MM's day 1 into an OoT-to-MM one
// (packages/logic/src/pathfind/pathfind.ts:596-599). Here a foreign source is an anchor region and the audited
// predicate of its edge (combo_entrance_overlay.cpp, ComboEntrancesMM.cpp): the oracle that owns the source records
// the destination it reaches, at each age, instead of traversing it, and the other oracle roots it at its start
// region, as the cross-game songs are: OoT at RR_ROOT (fill.cpp), MM at RR_MAX with that region's start time, the
// port's day 1 (Logic.cpp). Every Full destination of MM is no-sot (entrances.yml:292-298), so no arrival is gated on
// can_reset_time. Each engine has its own copy of these masks, as SongTravel.h's song masks are.
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "OotWallmasterArrivalNames.h"
#include "Wallmasters.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace ComboRando::Entrances {
// Bit i of an OoT mask names kOotWallmasterArrivalRows[i] (the 11 OoT dungeons), of an MM mask
// kMmBaseWallmasterDestinations[i] (the 7 MM ones).
inline constexpr uint32_t kOotWallmasterArrivalBits = (1u << kOotWallmasterArrivalRows.size()) - 1;
inline constexpr uint32_t kMmWallmasterArrivalBits = (1u << kMmBaseWallmasterDestinations.size()) - 1;
inline int OotWallmasterArrivalIndex(std::string_view row) {
    for (size_t i = 0; i < kOotWallmasterArrivalRows.size(); ++i)
        if (kOotWallmasterArrivalRows[i] == row)
            return static_cast<int>(i);
    return -1;
}
inline int MmWallmasterArrivalIndex(std::string_view row) {
    for (size_t i = 0; i < kMmBaseWallmasterDestinations.size(); ++i)
        if (kMmBaseWallmasterDestinations[i] == row)
            return static_cast<int>(i);
    return -1;
}

// One mask per OoT age, adult 0 and child 1 (the order of the song age masks): the ages Link has in the game the
// mask belongs to. Without Cross-Games Age MM's own search has one age, and the fixpoint gives every MM entry age
// the same masks.
struct WallmasterAgeMasks {
    std::array<uint32_t, 2> masks{};
    bool any() const {
        return (masks[0] | masks[1]) != 0;
    }
};
// The OoT engine: arrivals from MM's sources (OoT bits), rooted at RR_ROOT during an oracle search; and departures,
// the MM destinations (MM bits) of the OoT sources the last oracle search reached.
inline WallmasterAgeMasks& OotWallmasterArrivals() {
    static WallmasterAgeMasks value;
    return value;
}
inline WallmasterAgeMasks& OotWallmasterDepartures() {
    static WallmasterAgeMasks value;
    return value;
}
// The MM engine: arrivals from OoT's sources (MM bits), rooted at RR_MAX; departures, the OoT destinations (OoT bits)
// of the MM sources the last oracle search reached, at the age it searched.
inline WallmasterAgeMasks& MmWallmasterArrivals() {
    static WallmasterAgeMasks value;
    return value;
}
inline WallmasterAgeMasks& MmWallmasterDepartures() {
    static WallmasterAgeMasks value;
    return value;
}
} // namespace ComboRando::Entrances
