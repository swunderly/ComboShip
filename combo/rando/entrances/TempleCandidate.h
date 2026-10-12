// Native adaptation of OoTMM packages/logic/src/solver/entrances.ts changedWorld and
// changeWorldAssumePools (:289-385, at 413573d) as a pure description of one candidate world for
// the generation-only opposite-age Temple probe (soh TempleProbe.cpp). Used by host tests only;
// nothing in generation builds or sends it yet.
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "Shuffle.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ComboRando::Entrances {
// An exit the candidate world adds from the source row's area, under the source row's ORIGINAL
// condition (getExpr reads the unchanged world, :165-173), to the destination row's area.
struct TempleEdge {
    std::string_view src, dst;
    bool resetTimeGate = false; // AND can_reset_time: MM destination without no-sot (:203-218)
    auto operator<=>(const TempleEdge&) const = default;
};
struct TempleCandidate {
    std::vector<std::string_view> deleted; // override and remaining-pool sources, by area pair (:351-365)
    std::vector<TempleEdge> edges;         // overrides plus assignable small-pool pairs; duplicates kept (OR)
    std::vector<std::string_view> assumed; // large-pool destinations: unconditional ASSUMED exits (:307-314)
    bool assumedInstalled = true;          // `if (assumedSets)` tests an array, so it is always true (:307)
    std::vector<TempleEdge> portalLinks;   // the reserved game links; Ship's Mask Shop is portal-equivalent
};

inline bool IsReservedGameLink(std::string_view src, std::string_view dst) {
    // connectGamesDefault for games=ootmm (:888-903), which the Shuffler reserves before any pool.
    return (src == "OOT_SHOP_MASKS" && dst == "MM_CLOCK_TOWN_FROM_CLOCK_TOWER") ||
           (src == "MM_CLOCK_TOWER_FROM_CLOCK_TOWN" && dst == "OOT_MARKET_FROM_MASK_SHOP");
}
inline bool IsLargeTemplePool(const Pool& pool) {
    return pool.src.size() * pool.dst.size() > 5000 && pool.src.size() > 50; // :293-294
}
inline TempleEdge MakeTempleEdge(std::string_view src, std::string_view dst) {
    const auto* d = Find(dst);
    if (!Find(src) || !d)
        throw std::invalid_argument("Unknown Temple candidate entrance");
    return { src, dst, d->game == Game::Mm && !d->noSongOfTime };
}

inline TempleCandidate BuildTempleCandidate(const Overrides& overrides, const Pools& pools, const Settings& s) {
    TempleCandidate c;
    std::set<std::string_view> deleted;
    for (const auto& [src, dst] : overrides) {
        if (IsReservedGameLink(src, dst)) {
            c.portalLinks.push_back(MakeTempleEdge(src, dst));
            continue;
        }
        deleted.insert(src);
        c.edges.push_back(MakeTempleEdge(src, dst));
    }
    std::set<std::string_view> assumed;
    for (const auto& pool : pools) {
        deleted.insert(pool.src.begin(), pool.src.end());
        if (IsLargeTemplePool(pool)) {
            assumed.insert(pool.dst.begin(), pool.dst.end()); // no source condition, gate or filter
            continue;
        }
        for (const auto src : pool.src)
            for (const auto dst : pool.dst)
                if (Assignable(*Find(src), *Find(dst), pool.ownGame, s))
                    c.edges.push_back(MakeTempleEdge(src, dst));
    }
    c.deleted.assign(deleted.begin(), deleted.end());
    c.assumed.assign(assumed.begin(), assumed.end());
    std::sort(c.edges.begin(), c.edges.end());
    std::sort(c.portalLinks.begin(), c.portalLinks.end());
    return c;
}

// FNV-1a, the same function as CwHashName (CrossWorldRando.h), so the all-items pass can later carry
// the identical candidate token without including the generator here.
inline uint32_t TempleTokenHash(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s)
        h = (h ^ c) * 16777619u;
    return h;
}

// Canonical request for the native probe: sorted arrays, sorted keys, and a token over everything
// else. OoT rows are probed; MM-only rows are listed after their game is checked, because no MM
// change can reach the OoT target in the supported envelope (pinned lemma L4).
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
                   { "mm", { { "deleted", mmDeleted }, { "edges", mmEdges } } } };
    query["token"] = TempleTokenHash(query.dump());
    return query.dump();
}
} // namespace ComboRando::Entrances
