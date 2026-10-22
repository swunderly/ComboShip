// Combo side of the generation-only opposite-age Temple probe (soh TempleProbe.cpp): the capability
// gate that must run before any Shuffler is built or any random number drawn, the foreign starting
// items, and the mapping of the native return codes for Bindings::validateAgeTemple. Production code
// reaches it only through TempleNative.h, which only UnifiedBridge.h may use.
#pragma once
#include "OotEdgeBindingNames.h"
#include "OotWallmasterSourceNames.h"
#include "OotWallmasterArrivalNames.h"
#include "TempleCandidate.h"
#include "EntranceAvailability.h"
#include "gui/ComboGenProgress.h"
#include "rando/SharedItems.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ComboRando::Entrances {
// Not a ShuffleError: Shuffler::run retries those, and an unsupported world must stop generation.
class TempleProbeUnsupported : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

enum class TempleStartingAge : uint8_t { Child, Adult, Random };
// Seed facts the gate needs besides the entrance settings, read once before the Shuffler exists.
struct TempleFacts : EntranceSettingsFacts {
    TempleStartingAge startingAge = TempleStartingAge::Child; // the resolved RSK_SELECTED_STARTING_AGE
    bool dungeonBeaten = false; // OoTMM regionState dungeonBeaten: read only with a BOSS pool
    std::string_view registry = kRegistrySourceSha256;
};

// Whether a Shuffler run can call validateAgeTemple at all: never for logic none, which isValidShuffle
// accepts before building any world (entrances.ts:388-391), nor without OoT (:411), nor when no pool
// resolves to a source. placePools drops such pools (:581-589) and placePoolsRecursive validates
// nothing once none is left (:500-502), as Shuffle.h's place() never calls the hook with no pool. The
// pools are the ones the Shuffler will place, resolved with the same areaExists; with no pool in the
// settings at all, areaExists is never called.
inline bool NeedsTemplePass(const Settings& s, const TempleFacts& f, const AreaExists& areaExists) {
    return f.logicEnabled && s.games != Games::Mm && !MakePools(s).empty() &&
           !ResolvePools(s, areaExists).pools.empty();
}

// Supported envelope: own-game one-ways and the own-game alias Wallmaster pool, as far as every OoT
// row a candidate can touch has an audited native binding; no large pool, glitchless logic, no song
// travel and no starting Soaring. Foreign-song logic is inside it: without a starting song every OoTMM
// song exit is false here (lemma S1). So are starting OoT warp songs, which the probe plays from its
// GLOBAL node as OoTMM does (lemma L3), and through MM no path reaches the target first (the Mask Shop
// lemma). Which ids take them is the generator's rule (UnifiedBridge.h UnifiedSongRequirement,
// UnifiedWarpSongRequirement), not the probe's. Full's Wallmaster pool (the same alias pool with ownGame false)
// is inside it too: the probe searches OoT alone, so it answers only a candidate without a cross-game edge, which
// the base world of the monotone shortcut is (UnifiedBridge.h); the validator refuses any other (Stage 5 adds the
// Temple check through MM). Everything else throws here, before any random draw. The pools are the ones the
// Shuffler will place, resolved with the same areaExists.
inline void CheckTempleCapability(const Settings& s, const TempleFacts& f, const AreaExists& areaExists) {
    if (!f.logicEnabled)
        return; // logic none never runs the Temple pass (entrances.ts:388-391)
    auto reject = [](const std::string& why) { throw TempleProbeUnsupported("Temple probe: " + why); };
    if (f.registry != kRegistrySourceSha256 || kOotEdgeBindingRegistry != kRegistrySourceSha256 ||
        kOotWallmasterArrivalRegistry != kRegistrySourceSha256)
        reject("entrance registry identity differs from the audited bindings");
    if (s.games != Games::Both)
        reject("only the combined (games=Both) path is supported");
    if (f.startingAge == TempleStartingAge::Random)
        reject("the starting age must be resolved first");
    if (const auto why = TempleSettingsRefusal(f); !why.empty())
        reject(why.log);
    for (const Mode mode :
         { s.erBoss, s.erDungeons, s.erGrottos, s.erRegions, s.erOverworld, s.erIndoors, s.erMixed, s.erOneWays })
        if (mode == Mode::Full)
            reject("Full (cross-game) pools need the reset gate natively (deferred)");
    const bool fullWallmasters = s.erWallmasters == Mode::Full;
    if (fullWallmasters && MakePools(s).size() != 1)
        reject("Full Wallmasters with another pool need a Temple check through MM (deferred)");
    // regionState dungeonBeaten adds boss events at SPAWN only through a BOSS pool (entrances.ts:241-246,
    // :334-346), so dungeonBeaten alone changes no candidate world.
    if (Active(s.erBoss))
        reject("boss pools and their dungeonBeaten events at SPAWN are deferred");
    if (s.erSpawns != Spawns::None || s.erIndoorsGameLinks || Active(s.erMixed) || s.erWarps != Warps::None)
        reject("spawn, game-link, mixed and warp pools are deferred");
    if (Active(s.erOneWays) && s.erOneWaysAnywhere)
        reject("one-ways Anywhere (extended destinations) is deferred");
    std::set<std::string_view> bound(kOotTempleCandidateRows.begin(), kOotTempleCandidateRows.end());
    bound.insert(kOotWallmasterSources.begin(), kOotWallmasterSources.end());
    bound.insert(kOotWallmasterArrivalRows.begin(), kOotWallmasterArrivalRows.end());
    auto requireBound = [&](std::string_view name) {
        if (Find(name)->game == Game::Oot && !bound.contains(name))
            reject("OoT row " + std::string(name) + " has no audited native binding");
    };
    for (const auto& pool : ResolvePools(s, areaExists).pools) {
        const bool fullPool = fullWallmasters && pool.name == "WALLMASTERS" && pool.alias && !pool.ownGame;
        if ((!pool.ownGame && !fullPool) || (pool.name != "ONE_WAYS" && !(pool.name == "WALLMASTERS" && pool.alias)))
            reject("pool " + pool.name + " has no native representation yet");
        if (IsLargeCandidatePool(pool)) // entrances.ts:293-294
            reject("pool " + pool.name + " would need native ASSUMED exits");
        // The rows a candidate world deletes or connects (CandidateWorld.h): every source, and every
        // destination assignable from one; no Wallmaster or one-way row has a reverse. Own-game MM rows
        // only meet MM rows, which the probe lists without probing them, because no MM change reaches
        // the OoT target here (lemma L4). The thirteen Wallmaster sources (nine vanilla, four only an
        // MQ dungeon has) and eleven dungeon arrivals share M1-14's audited table; the pools resolve
        // with the quest context's areaExists, so a context offers only its own sources.
        // Wallmaster-to-one-way pairs stay unbound. A Full pair between the games is never probed (the
        // validator refuses it), and its OoT end is one of the same audited sources and arrivals.
        for (const auto src : pool.src) {
            requireBound(src);
            for (const auto dst : pool.dst)
                if (Assignable(*Find(src), *Find(dst), pool.ownGame, s)) {
                    requireBound(dst);
                    if (Find(src)->game == Game::Oot && Find(dst)->game == Game::Oot &&
                        Find(src)->type == Type::Wallmaster &&
                        std::find(kOotWallmasterArrivalRows.begin(), kOotWallmasterArrivalRows.end(), dst) ==
                            kOotWallmasterArrivalRows.end())
                        reject("OoT Wallmaster arrival " + std::string(dst) + " is deferred");
                }
        }
    }
}

// Foreign starting items come only from the shared-family starters (SharedItems.h): the OoT dump's own
// sharedStartingItems list is the whole native StartingInventory, which the probe already applies.
inline std::vector<std::string> NormalizeForeignStartingItems(uint32_t sharedMask, const std::string& ootDump,
                                                              const std::string& mmDump) {
    std::vector<std::string> result;
    for (const auto& item : SharedStartingInventories(sharedMask, ootDump, mmDump).first)
        if (std::find(result.begin(), result.end(), item) == result.end())
            result.push_back(item);
    return result;
}

inline std::string EncodeTemplePrepare(const TempleFacts& f) {
    if (f.startingAge == TempleStartingAge::Random)
        throw TempleProbeUnsupported("Temple probe: the starting age must be resolved first");
    return nlohmann::json{
        { "schema", "oot-temple-probe-prepare-v1" },
        { "registry", f.registry },
        { "startingAge", f.startingAge == TempleStartingAge::Child ? "child" : "adult" },
        { "foreignStartingItems", f.foreignStartingItems }
    }.dump();
}

// The native probe's entry points, resolved by the caller.
struct TempleProbeFns {
    int (*prepare)(const char* json) = nullptr;
    int (*query)(const char* json, int (*poll)(void), uint8_t* agesOut) = nullptr;
    const char* (*lastError)(void) = nullptr;
    void (*release)(void) = nullptr;
    int (*poll)(void) = nullptr; // non-zero cancels the running query
};

// 1 reached, 0 not reached, -3 cancelled; anything else stops generation as unsupported.
inline bool MapTempleResult(int code, const TempleProbeFns& fns) {
    switch (code) {
        case 1:
            return true;
        case 0:
            return false;
        case -3:
            throw GenerationCancelled();
        default: {
            const char* error = fns.lastError ? fns.lastError() : nullptr;
            throw TempleProbeUnsupported("Temple probe returned " + std::to_string(code) + ": " +
                                         (error ? error : "no detail"));
        }
    }
}

// Prepares the native probe for one generation and releases it on every exit path.
class TempleProbeSession {
  public:
    TempleProbeSession(const TempleProbeFns& fns_, const TempleFacts& facts) : fns(fns_) {
        if (!fns.prepare || !fns.query || !fns.release)
            throw TempleProbeUnsupported("Temple probe: native entry points are missing");
        const int code = fns.prepare(EncodeTemplePrepare(facts).c_str());
        if (code != 0) {
            fns.release();
            MapTempleResult(code < 0 ? code : -4, fns);
        }
    }
    ~TempleProbeSession() {
        fns.release();
    }
    TempleProbeSession(const TempleProbeSession&) = delete;
    TempleProbeSession& operator=(const TempleProbeSession&) = delete;

  private:
    TempleProbeFns fns;
};

// A candidate world with an edge between the games (Full Wallmasters) needs the Temple check through MM, which the
// probe does not have (limits design, Stage 5): refused, never answered from OoT alone. The monotone shortcut probes
// only the base world, which has no Wallmaster edge at all.
inline bool CrossGameTempleCandidate(const TempleCandidate& c) {
    return std::any_of(c.edges.begin(), c.edges.end(),
                       [](const TempleEdge& e) { return Find(e.src)->game != Find(e.dst)->game; });
}
inline std::function<bool(const Overrides&, const Pools&)> MakeTempleValidator(const TempleProbeFns& fns,
                                                                               const Settings& s) {
    return [fns, s](const Overrides& overrides, const Pools& pools) {
        const auto candidate = BuildTempleCandidate(overrides, pools, s);
        if (CrossGameTempleCandidate(candidate))
            throw TempleProbeUnsupported("Temple probe: a cross-game Wallmaster candidate needs a Temple check "
                                         "through MM (deferred)");
        const auto query = EncodeTempleQuery(candidate);
        uint8_t ages = 0;
        return MapTempleResult(fns.query(query.c_str(), fns.poll, &ages), fns);
    };
}
} // namespace ComboRando::Entrances
