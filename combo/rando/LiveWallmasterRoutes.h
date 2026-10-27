// The live check tracker's Full Wallmaster routes between the games (entrances/CrossWallmasters.h).
//
// The generator's oracles exchange each game's Wallmaster departures as the other game's arrivals until nothing
// changes (CrossWorldRando.h QuerySongWorlds: OracleFns::GetEntranceSourcesForAge and SetEntranceAgeArrivals). The
// live trackers do the same with their live searches: the game in front searches from where Link is, the other from
// its save in memory, and each roots the other's departures where the oracle does (OoT at RR_ROOT, fill.cpp; MM at
// RR_MAX, Logic.cpp). As in QuerySongWorlds, MM searches with one age, so every OoT departure arrives in MM, and an MM
// departure arrives in OoT at each OoT age Link can be in MM at.
//
// Masks: OoT bits name kOotWallmasterArrivalRows, MM bits kMmBaseWallmasterDestinations, and with the warps shuffled
// the bits after them the OoT warp pads and the MM owl statues (CrossWallmasters.h); OoT ages adult 0, child 1.
#pragma once
#include "entrances/CrossWallmasters.h"
#include <cstddef>
#include <cstdint>

namespace ComboRando::Entrances {
// One live OoT search: the MM destinations its reached sources route to (MM bits, any age), and the OoT ages Link can
// be in MM at after it (adult bit 0, child bit 1): by the Happy Mask Shop, a song, a Wallmaster or, while MM is in
// front, being there.
struct LiveOotSearch {
    uint32_t departures = 0;
    uint8_t mmAges = 0;
};
inline WallmasterAgeMasks LiveOotArrivals(uint32_t fromMm, uint8_t mmAges) {
    return { { (mmAges & 1) ? fromMm : 0u, (mmAges & 2) ? fromMm : 0u } };
}
// Bits only grow, and a pass that does not settle adds at least one: to the OoT arrivals, the MM arrivals or the two
// ages Link can be in MM at (17 + 17 + 2 with the warps). So every loop settles, and its last search confirms it,
// within this many passes.
inline constexpr int kLiveWallmasterPasses = static_cast<int>(kOotCrossArrivalCount + kMmCrossArrivalCount) + 2 + 1;
static_assert(kLiveWallmasterPasses >= 37, "the warps' 17 + 17 arrival bits and the two ages, and the confirming pass");
// A search with the same arrivals finds the same: the exchange has settled.
inline bool SameArrivals(const WallmasterAgeMasks& a, const WallmasterAgeMasks& b) {
    return a.masks == b.masks;
}
// The game not in front keeps its search's answer while its save, the caller's save and the arrivals stay the same
// (FNV-1a over the saves in memory): the other game does not run, and a grant changes a save.
inline uint64_t LiveStateKey(const void* save, size_t size, uint64_t seed = 14695981039346656037ull) {
    uint64_t key = seed;
    for (size_t i = 0; i < size; ++i)
        key = (key ^ static_cast<const uint8_t*>(save)[i]) * 1099511628211ull;
    return key;
}

// OoT in front. ootSearch(arrivals) runs the tracker's OoT search with those arrivals and returns a LiveOotSearch;
// mmSearch(arrivals) runs MM's live search from its save with those MM arrivals and returns its departures (OoT bits).
// The last OoT search ran with the final arrivals, so its results are the tracker's.
template <class OotSearch, class MmSearch>
inline WallmasterAgeMasks LiveWallmastersOotInFront(OotSearch ootSearch, MmSearch mmSearch) {
    uint32_t fromMm = 0;
    uint8_t mmAges = 0;
    for (int pass = 0; pass < kLiveWallmasterPasses; ++pass) {
        const LiveOotSearch found = ootSearch(LiveOotArrivals(fromMm, mmAges));
        const uint8_t nextAges = mmAges | found.mmAges;
        const uint32_t nextFromMm = nextAges ? fromMm | mmSearch(found.departures) : fromMm; // MM is entered at all
        if (SameArrivals(LiveOotArrivals(nextFromMm, nextAges), LiveOotArrivals(fromMm, mmAges)))
            break;
        fromMm = nextFromMm;
        mmAges = nextAges;
    }
    return LiveOotArrivals(fromMm, mmAges);
}

// MM in front. mmSearch(arrivals) runs the tracker's MM search with those MM arrivals and returns its departures (OoT
// bits); ootFromMm(departures) runs OoT's live search from its save with them and returns OoT's departures (MM bits).
// The last MM search ran with the final arrivals, which this returns.
template <class MmSearch, class OotFromMm>
inline uint32_t LiveWallmastersMmInFront(MmSearch mmSearch, OotFromMm ootFromMm) {
    uint32_t fromOot = 0;
    for (int pass = 0; pass < kLiveWallmasterPasses; ++pass) {
        const uint32_t next = fromOot | ootFromMm(mmSearch(fromOot));
        if (next == fromOot)
            break;
        fromOot = next;
    }
    return fromOot;
}

// OoT's side of MM in front: OoT's searches with MM's departures, until the ages Link can be in MM at settle. Link is
// in MM, so the age he has there (inMmAge) is one of them from the start. Returns the last search's departures.
template <class OotSearch>
inline uint32_t LiveOotDeparturesForMm(uint32_t fromMm, uint8_t inMmAge, OotSearch ootSearch) {
    uint8_t mmAges = inMmAge;
    LiveOotSearch found;
    for (int pass = 0; pass < kLiveWallmasterPasses; ++pass) {
        found = ootSearch(LiveOotArrivals(fromMm, mmAges));
        const uint8_t next = mmAges | found.mmAges;
        if (SameArrivals(LiveOotArrivals(fromMm, next), LiveOotArrivals(fromMm, mmAges)))
            break;
        mmAges = next;
    }
    return found.departures;
}
} // namespace ComboRando::Entrances
