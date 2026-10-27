// The generation-only opposite-age Temple probe's view (soh TempleProbe.cpp) of one candidate world
// (CandidateWorld.h, OoTMM packages/logic/src/solver/entrances.ts changedWorld and
// changeWorldAssumePools, :289-385 at 413573d): the fixed and remaining-pool edges as one list.
// The unified generator (UnifiedBridge.h) sends it through TempleNative.h.
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "CandidateWorld.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ComboRando::Entrances {
using TempleEdge = CandidateEdge;
struct TempleCandidate {
    std::vector<std::string_view> deleted; // override and remaining-pool sources, by area pair (:351-365)
    std::vector<TempleEdge> edges;         // overrides plus assignable small-pool pairs; duplicates kept (OR)
    std::vector<std::string_view> assumed; // large-pool destinations: unconditional ASSUMED exits (:307-314)
    bool assumedInstalled = true;          // `if (assumedSets)` tests an array, so it is always true (:307)
    std::vector<TempleEdge> portalLinks;   // the reserved game links; Ship's Mask Shop is portal-equivalent
    uint32_t world = 0;                    // CandidateWorldToken of the world it was built from
};

inline bool IsLargeTemplePool(const Pool& pool) {
    return IsLargeCandidatePool(pool);
}
inline TempleEdge MakeTempleEdge(std::string_view src, std::string_view dst) {
    return MakeCandidateEdge(src, dst);
}

// The WARPS pool's rows (Warp Songs and Soaring Spots Shuffle, limits design Stage 4) are left out: their sources are
// the warp songs and the Song of Soaring, which no Temple check plays, as it has none of them among its starting items
// (the Temple gate refuses a starting warp song or Soaring with the warps, EntranceAvailability.h), so every edge
// from them is false there (lemma S1) and deleting their native exits changes nothing. A Wallmaster's edge to a warp
// pad or an owl statue is a Wallmaster edge and stays. `world` is still the token of the whole candidate.
inline TempleCandidate BuildTempleCandidate(const Overrides& overrides, const Pools& pools, const Settings& s) {
    auto w = BuildCandidateWorld(overrides, pools, s);
    const auto world = CandidateWorldToken(w);
    const auto warp = [](const CandidateEdge& e) { return IsWarpRow(e.src); };
    w.deleted.erase(
        std::remove_if(w.deleted.begin(), w.deleted.end(), [](std::string_view row) { return IsWarpRow(row); }),
        w.deleted.end());
    w.fixed.erase(std::remove_if(w.fixed.begin(), w.fixed.end(), warp), w.fixed.end());
    w.remaining.erase(std::remove_if(w.remaining.begin(), w.remaining.end(), warp), w.remaining.end());
    TempleCandidate c{ std::move(w.deleted), std::move(w.fixed),       std::move(w.assumed),
                       w.assumedInstalled,   std::move(w.portalLinks), world };
    c.edges.insert(c.edges.end(), w.remaining.begin(), w.remaining.end());
    std::sort(c.edges.begin(), c.edges.end());
    return c;
}

// The query's own digest, the same function as the candidate world token. The candidate itself is
// named by `world`, the token both oracle overlays carry (EncodeCandidateOverlay).
inline uint32_t TempleTokenHash(const std::string& s) {
    return CandidateTokenHash(s);
}

// Canonical request for the native probe: sorted arrays, sorted keys, the candidate world token
// (`world`, which the native ignores) and a token over everything else. OoT rows are probed;
// MM-only rows are listed after their game is checked, because no MM change can reach the OoT
// target in the supported envelope (pinned lemma L4).
inline std::string EncodeTempleQuery(const TempleCandidate& c) {
    using Json = nlohmann::json;
    auto game = [](std::string_view name) { return Find(name)->game; };
    Json ootDeleted = Json::array(), mmDeleted = Json::array(), ootEdges = Json::array(), mmEdges = Json::array();
    for (const auto name : c.deleted)
        (game(name) == Game::Oot ? ootDeleted : mmDeleted).push_back(name);
    for (const auto& e : c.edges) {
        if (game(e.src) == Game::Mm && game(e.dst) == Game::Mm)
            mmEdges.push_back({ { "src", e.src }, { "dst", e.dst }, { "resetTime", e.resetTimeGate } });
        else
            ootEdges.push_back({ { "src", e.src }, { "dst", e.dst } });
    }
    Json portals = Json::array(), assumed = Json::array();
    for (const auto& e : c.portalLinks)
        portals.push_back({ { "src", e.src }, { "dst", e.dst } });
    for (const auto name : c.assumed)
        assumed.push_back(name);
    Json query = { { "schema", "oot-temple-probe-query-v1" },
                   { "registry", kRegistrySourceSha256 },
                   { "deleted", ootDeleted },
                   { "edges", ootEdges },
                   { "portalLinks", portals },
                   { "assumed", assumed },
                   { "assumedInstalled", c.assumedInstalled },
                   { "mm", { { "deleted", mmDeleted }, { "edges", mmEdges } } },
                   { "world", c.world } };
    query["token"] = TempleTokenHash(query.dump());
    return query.dump();
}
} // namespace ComboRando::Entrances
