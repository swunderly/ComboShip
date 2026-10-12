// Combo side of the generation-only opposite-age Temple probe (soh TempleProbe.cpp): the capability
// gate that must run before any Shuffler is built or any random number drawn, the foreign starting
// items, and the mapping of the native return codes for Bindings::validateAgeTemple. Only host tests
// use it: generation does not assign validateAgeTemple, so OoT/Both entrance logic fails closed.
#pragma once
#include "OotEdgeBindingNames.h"
#include "TempleCandidate.h"
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
struct TempleFacts {
    TempleStartingAge startingAge = TempleStartingAge::Child; // the resolved RSK_SELECTED_STARTING_AGE
    bool glitchless = true;                                   // OoT logic rules
    bool shipwrightEntrances = false;                         // any native Ship entrance shuffle
    bool songTravelLogic = false, foreignSongsLogic = false;  // ActiveSongTravel() logic flags
    bool dungeonBeaten = false;                               // OoTMM regionState dungeonBeaten
    std::vector<std::string> ootStartingItems, mmStartingItems, foreignStartingItems;
    std::string_view registry = kRegistrySourceSha256;
};

// Supported envelope T1: own-game one-ways whose OoT rows all have audited native bindings, no large
// pool, glitchless logic, no song travel and no starting warp songs or Soaring. Everything else throws
// here, before any random draw.
inline void CheckTempleCapability(const Settings& s, const TempleFacts& f, const Descriptions& pools) {
    auto reject = [](const std::string& why) { throw TempleProbeUnsupported("Temple probe: " + why); };
    if (f.registry != kRegistrySourceSha256 || kOotEdgeBindingRegistry != kRegistrySourceSha256)
        reject("entrance registry identity differs from the audited bindings");
    if (s.games != Games::Both)
        reject("only the combined (games=Both) path is supported");
    if (f.startingAge == TempleStartingAge::Random)
        reject("the starting age must be resolved first");
    if (!f.glitchless)
        reject("only glitchless OoT logic is supported");
    if (f.shipwrightEntrances)
        reject("Ship's own entrance shuffle must be off");
    if (f.songTravelLogic || f.foreignSongsLogic)
        reject("song travel logic needs GLOBAL song sources (deferred)");
    for (const Mode mode : { s.erBoss, s.erDungeons, s.erGrottos, s.erRegions, s.erOverworld, s.erIndoors, s.erMixed,
                             s.erOneWays, s.erWallmasters })
        if (mode == Mode::Full)
            reject("Full (cross-game) pools need the reset gate natively (deferred)");
    if (Active(s.erBoss) || f.dungeonBeaten)
        reject("boss pools and dungeonBeaten events at SPAWN are deferred");
    if (s.erSpawns != Spawns::None || s.erIndoorsGameLinks || Active(s.erMixed) || s.erWarps != Warps::None)
        reject("spawn, game-link, mixed and warp pools are deferred");
    if (Active(s.erWallmasters))
        reject("OoT Wallmasters wait for the M1 native edge audit");
    const std::set<std::string_view> bound(kOotTempleCandidateRows.begin(), kOotTempleCandidateRows.end());
    for (const auto& pool : pools) {
        if (pool.name != "ONE_WAYS" || !pool.ownGame)
            reject("pool " + pool.name + " has no native representation yet");
        size_t sources = 0, destinations = 0;
        for (const auto& r : kRegistry) {
            const bool src = pool.src.contains(r.type), dst = pool.dst.contains(r.type);
            sources += src;
            destinations += dst;
            if ((src || dst) && r.game == Game::Oot && !bound.contains(r.name))
                reject("OoT row " + std::string(r.name) + " has no audited native binding");
        }
        if (sources * destinations > 5000 && sources > 50) // entrances.ts:293-294, before area filtering
            reject("pool " + pool.name + " would need native ASSUMED exits");
    }
    static const std::set<std::string> songs{ "Song of Soaring",   "Minuet of Forest",   "Bolero of Fire",
                                              "Serenade of Water", "Nocturne of Shadow", "Requiem of Spirit",
                                              "Prelude of Light" };
    for (const auto* list : { &f.ootStartingItems, &f.mmStartingItems, &f.foreignStartingItems })
        for (const auto& item : *list)
            if (songs.contains(item))
                reject("starting " + item + " needs GLOBAL song sources (deferred)");
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

inline std::function<bool(const Overrides&, const Pools&)> MakeTempleValidator(const TempleProbeFns& fns,
                                                                               const Settings& s) {
    return [fns, s](const Overrides& overrides, const Pools& pools) {
        const auto query = EncodeTempleQuery(BuildTempleCandidate(overrides, pools, s));
        uint8_t ages = 0;
        return MapTempleResult(fns.query(query.c_str(), fns.poll, &ages), fns);
    };
}
} // namespace ComboRando::Entrances
