// The generation-only opposite-age Temple probe's view (soh TempleProbe.cpp) of one candidate world
// (CandidateWorld.h, OoTMM packages/logic/src/solver/entrances.ts changedWorld and
// changeWorldAssumePools, :289-385 at 413573d): the fixed and remaining-pool edges as one list.
// Used by host tests only; nothing in generation builds or sends it yet.
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

inline TempleCandidate BuildTempleCandidate(const Overrides& overrides, const Pools& pools, const Settings& s) {
    auto w = BuildCandidateWorld(overrides, pools, s);
    const auto world = CandidateWorldToken(w);
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
