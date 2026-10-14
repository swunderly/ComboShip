// Native adaptation of OoTMM packages/logic/src/solver/entrances.ts changedWorld and
// changeWorldAssumePools (:289-385, at 413573d): one description of a candidate world, built once
// per candidate and shaped for each consumer. The opposite-age Temple probe reads it through
// TempleCandidate.h; the per-game overlays below are what the OoT and MM oracle assumptions read.
// A committed layout is a candidate with no remaining pools. The unified generator (UnifiedBridge.h)
// builds one per candidate.
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "Shuffle.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ComboRando::Entrances {
// The VOID area of each game (data/world/{oot,mm}/overworld/_system.yml:1) has no exits, so an
// exit into it is a logic dead end.
inline bool IsVoidArea(std::string_view area) {
    return area == "OOT VOID" || area == "MM VOID";
}

// An exit the candidate world adds from the source row's area, under the source row's ORIGINAL
// condition (getExpr reads the unchanged world, :165-173), to the destination row's area. It is
// ORed with any exit the area already has to that area (:213-215), never replaces it.
struct CandidateEdge {
    std::string_view src, dst;
    bool resetTimeGate = false; // AND can_reset_time: MM destination without no-sot, from either game (:210-212)
    bool toVoid = false;        // the destination row leads into its game's VOID area
    auto operator<=>(const CandidateEdge&) const = default;
};
struct CandidateWorld {
    std::vector<std::string_view> deleted;  // override and remaining-pool sources, by area pair (:352-366)
    std::vector<CandidateEdge> fixed;       // the overrides, game links aside (:368-371)
    std::vector<CandidateEdge> remaining;   // assignable small-pool pairs (:296-303); duplicates kept (OR)
    std::vector<std::string_view> assumed;  // large-pool destinations: unconditional ASSUMED exits (:307-314)
    bool assumedInstalled = true;           // `if (assumedSets)` tests an array, so it is always true (:307)
    std::vector<CandidateEdge> portalLinks; // the reserved game links; Ship's Mask Shop is portal-equivalent
};

inline bool IsReservedGameLink(std::string_view src, std::string_view dst) {
    // connectGamesDefault for games=ootmm (:888-903), which the Shuffler reserves before any pool.
    return (src == "OOT_SHOP_MASKS" && dst == "MM_CLOCK_TOWN_FROM_CLOCK_TOWER") ||
           (src == "MM_CLOCK_TOWER_FROM_CLOCK_TOWN" && dst == "OOT_MARKET_FROM_MASK_SHOP");
}
inline bool IsLargeCandidatePool(const Pool& pool) {
    return pool.src.size() * pool.dst.size() > 5000 && pool.src.size() > 50; // :293-294
}
inline CandidateEdge MakeCandidateEdge(std::string_view src, std::string_view dst) {
    const auto* d = Find(dst);
    if (!Find(src) || !d)
        throw std::invalid_argument("Unknown candidate world entrance");
    return { src, dst, d->game == Game::Mm && !d->noSongOfTime, IsVoidArea(d->to) };
}

// changedWorld(overrides, pools): the overrides include the reserved game links, as the
// Shuffler's candidates do; the pools are the ones still to place.
inline CandidateWorld BuildCandidateWorld(const Overrides& overrides, const Pools& pools, const Settings& s) {
    CandidateWorld w;
    std::set<std::string_view> deleted;
    for (const auto& [src, dst] : overrides) {
        if (IsReservedGameLink(src, dst)) {
            w.portalLinks.push_back(MakeCandidateEdge(src, dst));
            continue;
        }
        deleted.insert(src);
        w.fixed.push_back(MakeCandidateEdge(src, dst));
    }
    std::set<std::string_view> assumed;
    for (const auto& pool : pools) {
        deleted.insert(pool.src.begin(), pool.src.end());
        if (IsLargeCandidatePool(pool)) {
            assumed.insert(pool.dst.begin(), pool.dst.end()); // no source condition, gate or filter
            continue;
        }
        for (const auto src : pool.src)
            for (const auto dst : pool.dst)
                if (Assignable(*Find(src), *Find(dst), pool.ownGame, s))
                    w.remaining.push_back(MakeCandidateEdge(src, dst));
    }
    w.deleted.assign(deleted.begin(), deleted.end());
    w.assumed.assign(assumed.begin(), assumed.end());
    std::sort(w.fixed.begin(), w.fixed.end());
    std::sort(w.remaining.begin(), w.remaining.end());
    std::sort(w.portalLinks.begin(), w.portalLinks.end());
    return w;
}

// FNV-1a, the same function as CwHashName (CrossWorldRando.h), without including the generator.
inline uint32_t CandidateTokenHash(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s)
        h = (h ^ c) * 16777619u;
    return h;
}

namespace CandidateDetail {
inline nlohmann::json Edges(const std::vector<CandidateEdge>& edges) {
    auto result = nlohmann::json::array();
    for (const auto& e : edges)
        result.push_back(
            { { "src", e.src }, { "dst", e.dst }, { "resetTime", e.resetTimeGate }, { "void", e.toVoid } });
    return result;
}
inline nlohmann::json World(const CandidateWorld& w) {
    auto portals = nlohmann::json::array();
    for (const auto& e : w.portalLinks)
        portals.push_back({ { "src", e.src }, { "dst", e.dst } });
    return { { "schema", "ootmm-candidate-world-v1" },
             { "registry", kRegistrySourceSha256 },
             { "deleted", w.deleted },
             { "fixed", Edges(w.fixed) },
             { "remaining", Edges(w.remaining) },
             { "assumed", w.assumed },
             { "assumedInstalled", w.assumedInstalled },
             { "portalLinks", std::move(portals) } };
}
} // namespace CandidateDetail

// The whole candidate in both games, canonical: sorted arrays, sorted keys, so the same world
// always gives the same text whatever order the overrides and pools came in. The token is a 32-bit
// digest of that text; a memo of validation results must key on the text itself.
inline uint32_t CandidateWorldToken(const CandidateWorld& w) {
    return CandidateTokenHash(CandidateDetail::World(w).dump());
}
inline std::string EncodeCandidateWorld(const CandidateWorld& w) {
    auto world = CandidateDetail::World(w);
    world["token"] = CandidateTokenHash(world.dump());
    return world.dump();
}

// One game's part: the rows whose source area is in that game. The game links are left out, as
// both games already connect them.
struct CandidateOverlay {
    std::vector<std::string_view> deleted;
    std::vector<CandidateEdge> fixed, remaining;
};
inline CandidateOverlay SplitCandidateWorld(const CandidateWorld& w, Game game) {
    auto ours = [&](std::string_view row) { return Find(row)->game == game; };
    CandidateOverlay o;
    std::copy_if(w.deleted.begin(), w.deleted.end(), std::back_inserter(o.deleted), ours);
    std::copy_if(w.fixed.begin(), w.fixed.end(), std::back_inserter(o.fixed),
                 [&](const CandidateEdge& e) { return ours(e.src); });
    std::copy_if(w.remaining.begin(), w.remaining.end(), std::back_inserter(o.remaining),
                 [&](const CandidateEdge& e) { return ours(e.src); });
    return o;
}

// The oracle assumption input of one game: deleted native source exits (a source into VOID has no
// native exit to delete), fixed and remaining-pool edges, each with its reset gate and VOID mark,
// and the ASSUMED list. No M1 pool is large, so a non-empty ASSUMED list is refused here.
inline std::string EncodeCandidateOverlay(const CandidateWorld& w, Game game) {
    if (!w.assumed.empty())
        throw std::invalid_argument("Candidate world overlays do not support ASSUMED exits");
    const auto o = SplitCandidateWorld(w, game);
    auto deleted = nlohmann::json::array();
    for (const auto row : o.deleted)
        deleted.push_back({ { "row", row }, { "void", IsVoidArea(Find(row)->to) } });
    return nlohmann::json{
        { "profile", "ootmm-entrance-assumptions-v1" },
        { "game", game == Game::Oot ? "oot" : "mm" },
        { "registry", kRegistrySourceSha256 },
        { "world", CandidateWorldToken(w) },
        { "deleted", std::move(deleted) },
        { "fixed", CandidateDetail::Edges(o.fixed) },
        { "remaining", CandidateDetail::Edges(o.remaining) },
        { "assumed", nlohmann::json::array() }
    }.dump();
}
inline std::string EncodeOotOverlay(const CandidateWorld& w) {
    return EncodeCandidateOverlay(w, Game::Oot);
}
inline std::string EncodeMmOverlay(const CandidateWorld& w) {
    return EncodeCandidateOverlay(w, Game::Mm);
}
} // namespace ComboRando::Entrances
