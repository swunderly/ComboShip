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
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

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
// FNV-1a over memory, eight bytes a step and continued from seed: the keys of the answers the game not in front keeps
// (below). Each step maps the key one to one, so a change to any one word always changes the key. MM's save is 73 KB.
inline constexpr uint64_t kLiveKeySeed = 14695981039346656037ull;
inline uint64_t LiveStateKey(const void* bytes, size_t size, uint64_t seed = kLiveKeySeed) {
    const auto* at = static_cast<const uint8_t*>(bytes);
    uint64_t key = seed;
    for (; size >= sizeof(uint64_t); at += sizeof(uint64_t), size -= sizeof(uint64_t)) {
        uint64_t word;
        std::memcpy(&word, at, sizeof(word));
        key = (key ^ word) * 1099511628211ull;
    }
    for (; size; ++at, --size)
        key = (key ^ *at) * 1099511628211ull;
    return key;
}
// The same over a save but one part of it: MM's logic events, which every MM search clears first and rewrites.
inline uint64_t LiveStateKeySkipping(const void* save, size_t size, size_t skipFrom, size_t skipSize,
                                     uint64_t seed = kLiveKeySeed) {
    const auto* bytes = static_cast<const uint8_t*>(save);
    return LiveStateKey(bytes + skipFrom + skipSize, size - skipFrom - skipSize, LiveStateKey(bytes, skipFrom, seed));
}
// What the other game's search reads of the game in front: its live song knowledge and song sources (OoT's search
// reads MM's in SOH_LiveIncomingSongAges and SOH_LiveMmEntryAges, MM's reads OoT's in MM_LiveIncomingSongs and
// MmLiveSongSources). The game in front passes this, not its save, which changes every frame it runs.
inline uint64_t LiveSongExportsKey(uint32_t knowledge, uint32_t sources) {
    const uint32_t exports[] = { knowledge, sources };
    return LiveStateKey(exports, sizeof(exports));
}

// The game not in front answers the searches the game in front asks of it (SOH_/MM_LiveWallmasterDepartures). Each
// answer follows from the arrivals it is given and the rest of what its search reads, the inputs: its own save in
// memory, what it reads of the game in front (LiveSongExportsKey) and the rules, song map and layout both games run
// under (LiveTrackerRules.h LiveRulesKey). None of them changes while the other game runs unless something happens (an
// item, a song, a changed option); the save of the game in front, which changes every frame, is not among them. One
// exchange asks once per pass, each time with more arrivals, so the answers for the same inputs are kept per arrivals:
// a refresh that changes none of them searches the game not in front not at all. A search that throws keeps nothing.
class LiveDepartureMemo {
    uint64_t inputs = 0;
    size_t count = 0, next = 0;
    std::array<std::pair<uint32_t, uint32_t>, static_cast<size_t>(kLiveWallmasterPasses)> answers{};

  public:
    // search() runs the search with these arrivals and returns its departures.
    template <class Search> uint32_t Answer(uint64_t key, uint32_t arrivals, Search search) {
        if (key != inputs) {
            inputs = key;
            count = next = 0;
        }
        for (size_t i = 0; i < count; ++i)
            if (answers[i].first == arrivals)
                return answers[i].second;
        const uint32_t departures = search();
        answers[next] = { arrivals, departures };
        next = (next + 1) % answers.size();
        count = std::min(count + 1, answers.size());
        return departures;
    }
};

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
