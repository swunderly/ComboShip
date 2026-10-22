// Full Wallmaster Shuffle's cross-game channel in the cross-world fixpoint (CrossWorldRando.h QuerySongWorlds and
// QueryAgeSongWorlds; limits design, Stage 3; F4 step 10, Wallmaster rows only), with fake oracles.
//
// OoTMM's pathfinder keeps Link's age on every exit (packages/logic/src/pathfind/pathfind.ts exploreArea) and an
// OoT-to-MM exit ORs MM's day 1 (:596-599). So: an OoT source reached at an age lands in MM at that age; without
// Cross-Games Age MM's search has one age and an MM source lands in OoT at every OoT age that can be in MM; with it
// MM searches each entry age and an MM source lands at the age it was reached. MM opens through a Wallmaster
// arrival alone. Without a cross-game route the channel never sets a bit, so every earlier seed queries the oracles
// exactly as before, pass for pass. Each fixpoint converges within its bound, and the bound is reached only by a
// chain that adds one state bit per pass.
#include "rando/CrossWorldRando.h"
#include "rando/entrances/CrossWallmasters.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace ComboRando;
namespace CE = ComboRando::Entrances;
using Checks = std::unordered_set<std::string>;

namespace {
// Engine state, as the native exports keep it.
uint32_t ootArrivals[2], mmArrivals[2], ootDepartures[2], mmDepartures[2];
uint8_t portalAges = 0, startAges = 2, mmEntryAges = 0;
bool crossAge = false;
int ootQueries = 0, mmQueries = 0;

void Reset() {
    for (auto* a : { ootArrivals, mmArrivals, ootDepartures, mmDepartures })
        a[0] = a[1] = 0;
    portalAges = 0;
    startAges = 2;
    mmEntryAges = 0;
    crossAge = false;
    ootQueries = mmQueries = 0;
}

constexpr int Oot(std::string_view row) {
    for (size_t i = 0; i < CE::kOotWallmasterArrivalRows.size(); ++i)
        if (CE::kOotWallmasterArrivalRows[i] == row)
            return static_cast<int>(i);
    return -1;
}
constexpr int Mm(std::string_view row) {
    for (size_t i = 0; i < CE::kMmBaseWallmasterDestinations.size(); ++i)
        if (CE::kMmBaseWallmasterDestinations[i] == row)
            return static_cast<int>(i);
    return -1;
}
constexpr int kWoodfall = Mm("MM_TEMPLE_WOODFALL"), kOceanSh = Mm("MM_SPIDER_HOUSE_OCEAN");
constexpr int kDekuTree = Oot("OOT_DEKU_TREE"), kForest = Oot("OOT_TEMPLE_FOREST");
static_assert(kWoodfall >= 0 && kOceanSh >= 0 && kDekuTree >= 0 && kForest >= 0);

OracleFns MakeOot(bool entrances) {
    OracleFns o{};
    o.Reset = +[] {};
    o.SetOwnedItems = +[](const char*) {};
    o.GetPortalOpen = +[]() -> uint8_t { return portalAges != 0; };
    o.SetSongArrivals = +[](uint32_t) {};
    o.GetPortalAges = +[]() -> uint8_t { return portalAges; };
    o.GetStartAges = +[]() -> uint8_t { return startAges; };
    o.GetSongSourcesForAge = +[](int) -> uint32_t { return 0; };
    o.SetSongAgeArrivals = +[](uint32_t, uint32_t) {};
    o.SongDestination = +[](int source) { return source; };
    if (entrances) {
        o.GetEntranceSourcesForAge = +[](int age) { return ootDepartures[age]; };
        o.SetEntranceAgeArrivals = +[](uint32_t adult, uint32_t child) {
            ootArrivals[0] = adult;
            ootArrivals[1] = child;
        };
    }
    return o;
}
OracleFns MakeMm(bool entrances, bool ageOracle) {
    OracleFns o{};
    o.Reset = +[] {};
    o.SetOwnedItems = +[](const char*) {};
    o.SetSongArrivals = +[](uint32_t) {};
    o.SongDestination = +[](int source) { return source; };
    if (ageOracle) {
        o.UseCrossAgeOracle = +[] { return crossAge; };
        o.SetEntryAges = +[](uint8_t ages) { mmEntryAges = ages; };
        o.GetDefaultEntryAges = +[]() -> uint8_t { return 2; };
        o.GetSongSourcesForAge = +[](int) -> uint32_t { return 0; };
        o.SetSongAgeArrivals = +[](uint32_t, uint32_t) {};
    }
    if (entrances) {
        o.GetEntranceSourcesForAge = +[](int age) { return mmDepartures[age]; };
        o.SetEntranceAgeArrivals = +[](uint32_t adult, uint32_t child) {
            mmArrivals[0] = adult;
            mmArrivals[1] = child;
        };
    }
    return o;
}

// A small two-game world. OoT: the adult reaches the Forest Temple's Wallmaster, which routes to Woodfall Temple
// (`ootRoute`); an arrival at the Deku Tree gives "Deku <age>". MM: Woodfall Temple, at an age, gives "Woodfall" and
// reaches Dampe's Wallmaster, routed to the Deku Tree (`mmRoute`). Departures are written per age, as the native
// searches record them: OoT at each age the source is reached, MM at the age it searched (its link age without
// Cross-Games Age, child, or each entry age with it).
struct World {
    bool ootRoute = true, mmRoute = true, adultReachesForest = true, childReachesForest = false;
    bool dampeFromStart = false; // Dampe's Wallmaster is reached from MM's start, without Woodfall
};
World world;
template <class Fixpoint> std::pair<Checks, Checks> Run(const OracleFns& oot, const OracleFns& mm, Fixpoint fixpoint,
                                                        bool& mmOpen) {
    auto query = [&](const OracleFns& o, const std::vector<std::string>&) {
        Checks result;
        if (&o == &oot) {
            ++ootQueries;
            ootDepartures[0] = world.ootRoute && world.adultReachesForest ? 1u << kWoodfall : 0;
            ootDepartures[1] = world.ootRoute && world.childReachesForest ? 1u << kWoodfall : 0;
            if (ootArrivals[0] & (1u << kDekuTree))
                result.insert("Deku adult");
            if (ootArrivals[1] & (1u << kDekuTree))
                result.insert("Deku child");
            return result;
        }
        ++mmQueries;
        mmDepartures[0] = mmDepartures[1] = 0;
        const bool perAge = crossAge && mmEntryAges;
        for (int age = 0; age < 2; ++age) {
            if (perAge ? !(mmEntryAges & (1u << age)) : age != 1)
                continue; // searched ages only
            if (mmArrivals[age] & (1u << kWoodfall))
                result.insert(perAge ? (age ? "Woodfall child" : "Woodfall adult") : "Woodfall");
            if (world.mmRoute && ((mmArrivals[age] & (1u << kWoodfall)) || world.dampeFromStart))
                mmDepartures[age] |= 1u << kDekuTree;
        }
        return result;
    };
    return fixpoint(oot, mm, query, mmOpen);
}
auto Plain = [](const OracleFns& oot, const OracleFns& mm, auto query, bool& mmOpen) {
    return QuerySongWorlds(oot, mm, {}, {}, mmOpen, query);
};
auto MmStart = [](const OracleFns& oot, const OracleFns& mm, auto query, bool& mmOpen) {
    return QuerySongWorlds(oot, mm, {}, {}, mmOpen, query, true);
};

void WithoutCrossAge() {
    // MM opens through the OoT capture alone, at the adult age that reached it; Dampe's Wallmaster then lands the
    // adult in the Deku Tree, never the child, as MM keeps OoT's age.
    Reset();
    world = {};
    const auto oot = MakeOot(true);
    const auto mm = MakeMm(true, true);
    bool open = false;
    auto reached = Run(oot, mm, Plain, open);
    assert(open && reached.second.count("Woodfall"));
    assert(reached.first.count("Deku adult") && !reached.first.count("Deku child"));
    // A child source instead: the child lands in MM and back in the Deku Tree as a child.
    Reset();
    world.adultReachesForest = false;
    world.childReachesForest = true;
    open = false;
    reached = Run(oot, mm, Plain, open);
    assert(open && reached.first.count("Deku child") && !reached.first.count("Deku adult"));
    // The adult capture and the child Mask Shop portal put both ages in MM: the MM source then lands both ages.
    Reset();
    world = {};
    portalAges = 2;
    open = false;
    reached = Run(oot, mm, Plain, open);
    assert(reached.first.count("Deku adult") && reached.first.count("Deku child"));
    // No OoT route into MM and the portal shut: MM stays closed and nothing lands.
    Reset();
    world.ootRoute = false;
    open = false;
    reached = Run(oot, mm, Plain, open);
    assert(!open && reached.second.empty() && reached.first.empty() && mmQueries == 0);
    // An MM start roots MM at the OoT starting age, with no OoT route: Dampe's Wallmaster lands that age in the Deku
    // Tree.
    for (const uint8_t start : { uint8_t(1), uint8_t(2) }) {
        Reset();
        startAges = start;
        world.ootRoute = false;
        world.dampeFromStart = true;
        open = true;
        reached = Run(oot, mm, MmStart, open);
        assert(reached.first.count("Deku child") == (start == 2) && reached.first.count("Deku adult") == (start == 1));
    }
}

void WithCrossAge() {
    // Cross-Games Age: MM is searched at each age it is entered at. The adult's capture searches MM as an adult only,
    // and the MM source lands the adult; the child's, the child.
    Reset();
    crossAge = true;
    world = {};
    const auto oot = MakeOot(true);
    const auto mm = MakeMm(true, true);
    bool open = false;
    auto reached = Run(oot, mm, Plain, open);
    assert(open && reached.second.count("Woodfall adult") && !reached.second.count("Woodfall child"));
    assert(reached.first.count("Deku adult") && !reached.first.count("Deku child"));
    Reset();
    crossAge = true;
    world.adultReachesForest = false;
    world.childReachesForest = true;
    open = false;
    reached = Run(oot, mm, Plain, open);
    assert(reached.second.count("Woodfall child") && reached.first.count("Deku child") &&
           !reached.first.count("Deku adult"));
    // A child portal adds the child to MM; Woodfall still needs the Wallmaster arrival, which only the adult has.
    Reset();
    crossAge = true;
    world = {};
    portalAges = 2;
    open = false;
    reached = Run(oot, mm, Plain, open);
    assert(reached.second.count("Woodfall adult") && !reached.second.count("Woodfall child"));
    assert(reached.first.count("Deku adult") && !reached.first.count("Deku child"));
}

// Without any cross-game route the channel is inert: the same reachability and the same number of oracle queries as
// an engine without the functions at all, in both fixpoints, with the portal open or shut and the songs' own ages.
void InertWithoutRoutes() {
    for (const bool cross : { false, true })
        for (const uint8_t portal : { uint8_t(0), uint8_t(1), uint8_t(2), uint8_t(3) })
            for (const bool mmStart : { false, true }) {
                std::pair<Checks, Checks> results[2];
                int queries[2][2];
                for (const bool entrances : { false, true }) {
                    Reset();
                    crossAge = cross;
                    portalAges = portal;
                    world = {};
                    world.ootRoute = world.mmRoute = false;
                    const auto oot = MakeOot(entrances);
                    const auto mm = MakeMm(entrances, true);
                    bool open = false;
                    results[entrances] = mmStart ? Run(oot, mm, MmStart, open) : Run(oot, mm, Plain, open);
                    queries[entrances][0] = ootQueries;
                    queries[entrances][1] = mmQueries;
                    assert(ootArrivals[0] == 0 && ootArrivals[1] == 0 && mmArrivals[0] == 0 && mmArrivals[1] == 0);
                }
                assert(results[0] == results[1]);
                assert(queries[0][0] == queries[1][0] && queries[0][1] == queries[1][1]);
            }
}

// Every query leaves both engines' arrivals empty, so a direct oracle query afterwards (the combined fill's OoT-only
// fixpoint) never inherits a Wallmaster arrival.
void ArrivalsClearedOnExit() {
    for (const bool cross : { false, true }) {
        Reset();
        crossAge = cross;
        world = {};
        const auto oot = MakeOot(true);
        const auto mm = MakeMm(true, true);
        bool open = false;
        const auto reached = Run(oot, mm, Plain, open);
        assert(reached.first.count("Deku adult"));
        assert(ootArrivals[0] == 0 && ootArrivals[1] == 0 && mmArrivals[0] == 0 && mmArrivals[1] == 0);
    }
}

// The longest chain: each pass adds one arrival bit, alternating games, through every OoT and MM destination at both
// ages; the fixpoint converges, and its pass count is the chain's length plus the stable pass, within the bound.
void LongestChain() {
    for (const bool cross : { false, true }) {
        Reset();
        crossAge = cross;
        startAges = 2;
        // OoT: an arrival at OoT bit i at an age reaches the source routed to MM bit i at that age. MM: an arrival at
        // MM bit i reaches the source routed to OoT bit i+1. The first OoT source is reached at both ages.
        const auto oot = MakeOot(true);
        const auto mm = MakeMm(true, true);
        const int ootBits = int(CE::kOotWallmasterArrivalRows.size()), mmBits = int(CE::kMmBaseWallmasterDestinations.size());
        auto query = [&](const OracleFns& o, const std::vector<std::string>&) {
            Checks result;
            if (&o == &oot) {
                ++ootQueries;
                for (int age = 0; age < 2; ++age) {
                    ootDepartures[age] = 1u; // MM bit 0 from the first source
                    for (int i = 1; i < ootBits && i < mmBits; ++i)
                        if (ootArrivals[age] & (1u << i))
                            ootDepartures[age] |= 1u << i;
                    for (int i = 0; i < ootBits; ++i)
                        if (ootArrivals[age] & (1u << i))
                            result.insert("oot " + std::to_string(i) + (age ? " child" : " adult"));
                }
                return result;
            }
            ++mmQueries;
            mmDepartures[0] = mmDepartures[1] = 0;
            const bool perAge = crossAge && mmEntryAges;
            for (int age = 0; age < 2; ++age) {
                if (perAge ? !(mmEntryAges & (1u << age)) : age != 1)
                    continue;
                for (int i = 0; i < mmBits; ++i)
                    if ((mmArrivals[age] & (1u << i)) && i + 1 < ootBits)
                        mmDepartures[age] |= 1u << (i + 1);
            }
            return result;
        };
        bool open = false;
        const auto reached = QuerySongWorlds(oot, mm, {}, {}, open, query);
        // Every OoT arrival up to the last MM destination's, at both ages.
        for (int i = 1; i <= mmBits && i < ootBits; ++i)
            for (const char* age : { " adult", " child" })
                assert(reached.first.count("oot " + std::to_string(i) + age));
        assert(ootQueries == mmQueries && ootQueries <= (cross ? 70 + 36 : 31 + 29));
        assert(ootQueries >= mmBits + 1);
    }
}

// Bit order: the two frozen destination tables of the Full pool (EntranceLayout.h LayoutV2), 11 OoT and 7 MM, all of
// MM's no-sot (entrances.yml:292-298).
void BitOrder() {
    assert(CE::kOotWallmasterArrivalBits == 0x7ffu && CE::kMmWallmasterArrivalBits == 0x7fu);
    for (size_t i = 0; i < CE::kMmBaseWallmasterDestinations.size(); ++i) {
        const auto* row = CE::Find(CE::kMmBaseWallmasterDestinations[i]);
        assert(row && row->game == CE::Game::Mm && row->noSongOfTime && CE::MmWallmasterArrivalIndex(row->name) == int(i));
    }
    for (size_t i = 0; i < CE::kOotWallmasterArrivalRows.size(); ++i) {
        const auto* row = CE::Find(CE::kOotWallmasterArrivalRows[i]);
        assert(row && row->game == CE::Game::Oot && CE::OotWallmasterArrivalIndex(row->name) == int(i));
    }
    assert(CE::OotWallmasterArrivalIndex("MM_TEMPLE_WOODFALL") < 0 && CE::MmWallmasterArrivalIndex("OOT_DEKU_TREE") < 0);
}
} // namespace

int main() {
    BitOrder();
    WithoutCrossAge();
    WithCrossAge();
    InertWithoutRoutes();
    ArrivalsClearedOnExit();
    LongestChain();
    std::cout << "Full Wallmaster arrivals keep the age, open MM, stay inert without a route and converge within the "
                 "pass bounds\n";
}
