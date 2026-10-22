// MM half of the unified OoTMM entrance layout (schema ootmm-entrance-layout-v1, EntranceLayout.h):
// the routes a seed installs, the candidate-world assumptions the MM oracle reads during generation
// (profile ootmm-entrance-assumptions-v1, CandidateWorld.h), and the graph the MM logic reads from
// either. Adapted from OoTMM v32.3 (413573d) packages/logic/src/solver/entrances.ts placeSingleExpr
// (:203-217) and changedWorld (:352-371); see LICENSE.OoTMM. The private MM profiles (Format.h,
// MmWallmasterGraph.h) are separate and unchanged; a seed carries one or the other, never both.
#pragma once
#include "rando/ComboSeedFormat.h"
#include "rando/entrances/CandidateWorld.h"
#include "rando/entrances/CrossWallmasters.h"
#include "rando/entrances/EntranceRuntimeFlags.h"
#include "rando/entrances/MmWallmasterGraph.h"
#include <initializer_list>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ComboRando::Entrances {
inline constexpr std::string_view kCandidateAssumptionsProfile = "ootmm-entrance-assumptions-v1";

// The MM rows the unified MM logic can route today: the four Wallmaster sources and the seven MM
// arrivals an own-game Wallmaster pool offers them while one-ways are off (tools/ootmm/m1-contract.json).
// A layout or candidate with any other MM row is refused, never routed as vanilla; the one-way and
// water-void rows join here once their exits and arrivals are bound.
inline bool MmUnifiedSourceBound(std::string_view row) {
    return Contains(kMmWallmasterSources, row);
}
inline bool MmUnifiedDestinationBound(std::string_view row) {
    return Contains(kMmBaseWallmasterDestinations, row);
}
// Under the Full capability an MM Wallmaster may also route to an OoT dungeon of the Full pool (the eleven
// arrivals of OotWallmasterArrivalNames.h). Such a route is never traversed by the MM graph: the MM oracle records
// it (CrossWallmasters.h) and the capture departs to OoT (ComboEntrancesMM.cpp).
inline bool MmForeignDestinationBound(const Record& dst) {
    return dst.game == Game::Oot && OotWallmasterArrivalIndex(dst.name) >= 0 && !IsVoidArea(dst.to);
}

// One edge from an MM source, to a destination named by game and id, with the rules its arrival keeps.
struct MmUnifiedRoute {
    Key destination;
    bool resetTime = false; // AND can_reset_time: an MM destination without no-sot (:210-212)
    bool toVoid = false;    // the destination leads into MM VOID, which has no exits: no arrival
    friend bool operator==(const MmUnifiedRoute&, const MmUnifiedRoute&) = default;
};
// By MM source id: a Wallmaster's synthetic id now, a native exit id for the later one-way rows.
using MmUnifiedRoutes = std::map<uint32_t, std::vector<MmUnifiedRoute>>;
inline MmUnifiedRoute MakeMmUnifiedRoute(const Record& destination) {
    return { { destination.game, destination.id },
             destination.game == Game::Mm && !destination.noSongOfTime,
             IsVoidArea(destination.to) };
}

// The installed layout: the whole envelope, for the dump, and its MM routes, one per source.
class MmUnifiedLayout {
    std::optional<EntranceLayout> layout;
    MmUnifiedRoutes table;

  public:
    void clear() {
        layout.reset();
        table.clear();
    }
    bool active() const {
        return layout.has_value();
    }
    // The envelope as installed; installing it again gives the same layout.
    nlohmann::json json() const {
        return layout ? EncodeEntranceLayout(*layout) : nlohmann::json();
    }
    const MmUnifiedRoutes& routes() const {
        return table;
    }
    // The installed layout's er* settings, from which the ER-wide behaviour follows
    // (EntranceRuntimeFlags.h); null when no layout is installed.
    const Settings* settings() const {
        return layout ? &layout->settings : nullptr;
    }
    std::vector<MmUnifiedRoute> routesFrom(uint32_t source) const {
        const auto it = table.find(source);
        return it == table.end() ? std::vector<MmUnifiedRoute>{} : it->second;
    }
    // A seed's entranceLayout, never beside a private MM profile: decoded, validated against this
    // build's capabilities, and its MM routes bound. Nothing changes unless all of it is accepted.
    void install(const nlohmann::json& seed, const CapabilitySet& caps) {
        if (!seed.is_object() || !seed.contains("entranceLayout"))
            throw std::invalid_argument("Seed has no entrance layout");
        for (const char* key : kLegacyEntranceKeys)
            if (seed.contains(key))
                throw std::invalid_argument(std::string("An entrance layout cannot be combined with ") + key);
        auto decoded = DecodeEntranceLayout(seed.at("entranceLayout"));
        const auto validated = Validate(decoded, caps);
        const bool full = decoded.required == std::vector<std::string>{ std::string(kFullWallmasterCapability) };
        if (!validated.mmToOot.empty() && !full)
            throw std::invalid_argument("MM Wallmaster routes into OoT require the Full capability");
        auto routes = validated.mm;
        routes.insert(routes.end(), validated.mmToOot.begin(), validated.mmToOot.end());
        MmUnifiedRoutes next;
        for (const auto& route : routes) {
            const auto& src = *route.source;
            const auto& dst = *route.destination;
            if (!MmUnifiedSourceBound(src.name) ||
                !(dst.game == Game::Mm ? MmUnifiedDestinationBound(dst.name) : MmForeignDestinationBound(dst)))
                throw std::invalid_argument("MM route " + std::string(src.name) + " to " + std::string(dst.name) +
                                            " has no unified MM binding yet");
            // The logic and runtime read the live registry by id; it must name the same rows.
            const auto *liveSrc = Find(src.name), *liveDst = Find(dst.name);
            if (!liveSrc || !liveDst || liveSrc->game != src.game || liveSrc->id != src.id ||
                liveDst->game != dst.game || liveDst->id != dst.id)
                throw std::invalid_argument("Entrance route metadata mismatch");
            next[src.id].push_back(MakeMmUnifiedRoute(dst));
        }
        layout = std::move(decoded);
        table = std::move(next);
    }
};
inline MmUnifiedLayout& ActiveMmUnified() {
    static MmUnifiedLayout value;
    return value;
}

// One candidate world's MM part, as EncodeMmOverlay writes it: every fixed and remaining edge from an
// MM source (several per source are ORed), and the candidate world token both games' oracles share.
struct MmUnifiedAssumptions {
    MmUnifiedRoutes routes;
    uint32_t world = 0;
};
// Scoped to the native oracle's thread, independent of the installed layout.
inline std::optional<MmUnifiedAssumptions>& ActiveMmUnifiedAssumptions() {
    static thread_local std::optional<MmUnifiedAssumptions> value;
    return value;
}

// The er* settings of the layout being generated (UnifiedBridge.h), which the MM logic's er_enabled reads
// while candidate worlds are checked and no layout is installed yet. Scoped to the native oracle's
// thread, like the assumptions; set and cleared only by the generation, never by a bind.
inline std::optional<Settings>& ActiveMmGenerationSettings() {
    static thread_local std::optional<Settings> value;
    return value;
}
// er_enabled (macros_mm.yml:305) for the MM logic: the generation's settings while it runs, else the
// installed unified layout's. A private MM profile never counts: its seeds keep the logic they had.
inline bool MmUnifiedErEnabled() {
    if (const auto& generation = ActiveMmGenerationSettings())
        return ErEnabled(*generation);
    const auto* installed = ActiveMmUnified().settings();
    return installed && ErEnabled(*installed);
}
// isEntranceShuffle (helpers.ts:6-55) for the MM runtime (PG_ER): only the installed layout counts.
inline bool MmUnifiedEntranceShuffle() {
    const auto* installed = ActiveMmUnified().settings();
    return installed && IsEntranceShuffle(*installed);
}

namespace MmUnifiedDetail {
inline void RequireKeys(const nlohmann::json& value, std::initializer_list<std::string_view> keys) {
    if (!value.is_object() || value.size() != keys.size())
        throw std::invalid_argument("Malformed MM entrance assumptions");
    for (const auto key : keys)
        if (!value.contains(key))
            throw std::invalid_argument("MM entrance assumptions miss " + std::string(key));
}
inline std::string Text(const nlohmann::json& value) {
    return value.is_string() ? value.get<std::string>() : std::string();
}
inline const Record& MmRow(const nlohmann::json& value) {
    const auto* row = Find(Text(value));
    if (!row || row->game != Game::Mm)
        throw std::invalid_argument("MM entrance assumptions name an unknown MM row");
    return *row;
}
inline bool Flag(const nlohmann::json& value) {
    if (!value.is_boolean())
        throw std::invalid_argument("MM entrance assumption flags must be booleans");
    return value.get<bool>();
}
} // namespace MmUnifiedDetail

// The MM overlay of one candidate world (CandidateWorld.h). A deleted Wallmaster source has no native
// exit to delete; deleting a native MM exit (a one-way) or an ASSUMED exit is not supported yet. An edge
// into an OoT dungeon is admitted only while `caps` has the Full capability.
inline MmUnifiedAssumptions DecodeMmUnifiedAssumptions(const nlohmann::json& value,
                                                       const CapabilitySet& caps = ActiveCapabilities()) {
    using namespace MmUnifiedDetail;
    RequireKeys(value, { "profile", "game", "registry", "world", "deleted", "fixed", "remaining", "assumed" });
    if (Text(value.at("profile")) != kCandidateAssumptionsProfile || Text(value.at("game")) != "mm" ||
        Text(value.at("registry")) != kRegistrySourceSha256)
        throw std::invalid_argument("Unsupported MM entrance assumptions");
    const auto& world = value.at("world");
    if (!world.is_number_unsigned() || world.get<uint64_t>() > 0xffffffffull)
        throw std::invalid_argument("MM entrance assumptions need a candidate world token");
    if (!value.at("assumed").is_array() || !value.at("assumed").empty())
        throw std::invalid_argument("MM entrance assumptions do not support ASSUMED exits");
    if (!value.at("deleted").is_array() || !value.at("fixed").is_array() || !value.at("remaining").is_array())
        throw std::invalid_argument("Malformed MM entrance assumptions");
    MmUnifiedAssumptions result;
    result.world = static_cast<uint32_t>(world.get<uint64_t>());
    std::set<std::string_view> deleted;
    for (const auto& row : value.at("deleted")) {
        RequireKeys(row, { "row", "void" });
        const auto& src = MmRow(row.at("row"));
        if (Flag(row.at("void")) != IsVoidArea(src.to))
            throw std::invalid_argument("MM entrance assumptions mark " + std::string(src.name) + " wrongly");
        if (!MmUnifiedSourceBound(src.name))
            throw std::invalid_argument("MM row " + std::string(src.name) + " has no unified MM binding yet");
        if (!deleted.insert(src.name).second)
            throw std::invalid_argument("MM entrance assumptions delete " + std::string(src.name) + " twice");
    }
    for (const char* part : { "fixed", "remaining" })
        for (const auto& edge : value.at(part)) {
            RequireKeys(edge, { "src", "dst", "resetTime", "void" });
            const auto& src = MmRow(edge.at("src"));
            const auto* dst = Find(Text(edge.at("dst")));
            if (!dst)
                throw std::invalid_argument("MM entrance assumptions name an unknown destination");
            // Every edge leaves a source the candidate deleted (changedWorld, :352-371).
            if (!deleted.contains(src.name))
                throw std::invalid_argument("MM entrance assumptions route " + std::string(src.name) +
                                            " without deleting it");
            const bool foreign = dst->game == Game::Oot;
            if (foreign ? !caps.Active(kFullWallmasterCapability) || !MmForeignDestinationBound(*dst)
                        : !MmUnifiedDestinationBound(dst->name))
                throw std::invalid_argument("MM route " + std::string(src.name) + " to " + std::string(dst->name) +
                                            " has no unified MM binding yet");
            const auto route = MakeMmUnifiedRoute(*dst);
            if (Flag(edge.at("resetTime")) != route.resetTime || Flag(edge.at("void")) != route.toVoid)
                throw std::invalid_argument("MM entrance assumptions mark " + std::string(dst->name) + " wrongly");
            result.routes[src.id].push_back(route);
        }
    return result;
}

// Which MM Wallmaster graph the logic reads. Candidate assumptions of either kind come before an
// installed layout of either kind, as MmWallmasterGraphDestinations has them. Every bind clears both
// kinds of layout and assumptions, and every assumption call both kinds of assumptions, so at most
// one layout and one assumption set are ever active.
inline bool MmUnifiedGraphActive() {
    if (ActiveMmUnifiedAssumptions())
        return true;
    const auto& legacy = ActiveMmWallmasterAssumptions();
    return !(legacy && legacy->enabled) && ActiveMmUnified().active();
}
inline std::vector<MmUnifiedRoute> MmUnifiedGraphRoutes(uint32_t source) {
    if (const auto& assumed = ActiveMmUnifiedAssumptions()) {
        const auto it = assumed->routes.find(source);
        return it == assumed->routes.end() ? std::vector<MmUnifiedRoute>{} : it->second;
    }
    return ActiveMmUnified().routesFrom(source);
}
} // namespace ComboRando::Entrances
