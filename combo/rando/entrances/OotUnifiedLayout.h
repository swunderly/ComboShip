// OoT half of the unified layout and candidate-world overlay. Source bindings are generated from the
// pinned actor/anchor audit. The frozen vanilla capability rejects MQ, never converting it into a smaller
// source pool; the quests and both songs capabilities bind the quest context the layout records.
#pragma once
#include "rando/ComboSeedFormat.h"
#include "rando/entrances/CandidateWorld.h"
#include "rando/entrances/OotWallmasterArrivalNames.h"
#include "rando/entrances/OotWallmasterSourceNames.h"
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace ComboRando::Entrances {
inline bool AnyMasterQuest(const OotQuests& quests) {
    return std::any_of(quests.begin(), quests.end(), [](bool mq) { return mq; });
}
inline void RequireVanillaOotQuests(const OotQuests& quests) {
    if (AnyMasterQuest(quests))
        throw std::invalid_argument("Wallmaster Shuffle currently supports vanilla OoT dungeons only");
}
// Before a layout exists (generation, candidate assumptions, the Temple probe), a Master Quest dungeon is
// admitted only while this build has the quests or a songs capability.
inline void RequireAdmittedOotQuests(const OotQuests& quests, const CapabilitySet& caps) {
    if (!caps.Active(kQuestsWallmasterCapability) && !caps.Active(kSongsWallmasterCapability) &&
        !caps.Active(kStartingSongsWallmasterCapability))
        RequireVanillaOotQuests(quests);
}
inline OotQuests DecodeOotAreaContext(const nlohmann::json& value) {
    if (!value.is_object() || value.size() != 3 ||
        value.value("profile", std::string{}) != "ootmm-oot-area-context-v1" ||
        value.value("registry", std::string{}) != kRegistrySourceSha256 || !value.contains("ootMq"))
        throw std::invalid_argument("Unsupported native OoT quest context");
    const auto& mq = value.at("ootMq");
    if (!mq.is_object() || mq.size() != kOotMqDungeons.size())
        throw std::invalid_argument("Native OoT quest context must name all twelve dungeons");
    OotQuests result{};
    for (size_t i = 0; i < result.size(); ++i) {
        const auto it = mq.find(std::string(kOotMqDungeons[i]));
        if (it == mq.end() || !it->is_boolean())
            throw std::invalid_argument("Native OoT quest context needs explicit quest booleans");
        result[i] = it->get<bool>();
    }
    return result;
}
inline bool OotUnifiedSourceBound(std::string_view row) {
    return std::find(kOotWallmasterSources.begin(), kOotWallmasterSources.end(), row) != kOotWallmasterSources.end();
}
// A bound source that exists in these quests: its area is in the quest's dungeon file (LayoutV1::AreaExists).
inline bool OotUnifiedSourceInQuests(const Record& row, const OotQuests& quests) {
    return row.game == Game::Oot && OotUnifiedSourceBound(row.name) && LayoutV1::AreaExists(row.from, quests, false);
}
// The audited edge applies to these quests: its dungeon has the edge's quest (kOotWallmasterEdgeQuests). An edge of
// the other quest is anchored in regions that quest's dungeon never reaches.
inline bool OotWallmasterEdgeInQuests(std::string_view auditId, const OotQuests& quests) {
    for (const auto& edge : kOotWallmasterEdgeQuests)
        if (edge.auditId == auditId)
            return quests[OotMqIndex(edge.dungeon)] == edge.mq;
    throw std::invalid_argument("OoT Wallmaster edge " + std::string(auditId) + " has no audited quest");
}
inline bool OotUnifiedDestinationBound(std::string_view row) {
    return std::find(kOotWallmasterArrivalRows.begin(), kOotWallmasterArrivalRows.end(), row) !=
           kOotWallmasterArrivalRows.end();
}
struct OotUnifiedRoute {
    uint32_t destination = 0;
    friend bool operator==(const OotUnifiedRoute&, const OotUnifiedRoute&) = default;
};
using OotUnifiedRoutes = std::map<uint32_t, std::vector<OotUnifiedRoute>>;

class OotUnifiedLayout {
    std::optional<EntranceLayout> layout;
    OotUnifiedRoutes table;

  public:
    void clear() {
        layout.reset();
        table.clear();
    }
    bool active() const {
        return layout.has_value();
    }
    bool hasOotRoutes() const {
        return !table.empty();
    }
    nlohmann::json json() const {
        return layout ? EncodeEntranceLayout(*layout) : nlohmann::json();
    }
    const OotUnifiedRoutes& routes() const {
        return table;
    }
    std::vector<OotUnifiedRoute> routesFrom(uint32_t source) const {
        const auto it = table.find(source);
        return it == table.end() ? std::vector<OotUnifiedRoute>{} : it->second;
    }
    const OotQuests* quests() const {
        return layout ? &layout->ootMq : nullptr;
    }
    void install(const nlohmann::json& seed, const CapabilitySet& caps) {
        if (!seed.is_object() || !seed.contains("entranceLayout"))
            throw std::invalid_argument("Seed has no entrance layout");
        for (const char* key : kLegacyEntranceKeys)
            if (seed.contains(key))
                throw std::invalid_argument("An OoT entrance layout cannot be combined with a private MM profile");
        auto decoded = DecodeEntranceLayout(seed.at("entranceLayout"));
        const auto validated = Validate(decoded, caps);
        OotUnifiedRoutes next;
        if (!validated.oot.empty()) {
            // The frozen vanilla id binds all-vanilla dungeons; the quests id a context with a Master Quest dungeon,
            // which Validate requires of it; the songs ids either.
            if (decoded.required == std::vector<std::string>{ std::string(kVanillaWallmasterCapability) })
                RequireVanillaOotQuests(decoded.ootMq);
            else if (decoded.required != std::vector<std::string>{ std::string(kQuestsWallmasterCapability) } &&
                     decoded.required != std::vector<std::string>{ std::string(kSongsWallmasterCapability) } &&
                     decoded.required != std::vector<std::string>{ std::string(kStartingSongsWallmasterCapability) })
                throw std::invalid_argument(
                    "OoT Wallmaster routes require the vanilla, Master Quest or a songs capability");
        }
        for (const auto& route : validated.oot) {
            const auto& src = *route.source;
            const auto& dst = *route.destination;
            if (!OotUnifiedSourceInQuests(src, decoded.ootMq) || !OotUnifiedDestinationBound(dst.name) ||
                src.game != Game::Oot || dst.game != Game::Oot)
                throw std::invalid_argument("OoT entrance route has no verified binding");
            const auto* liveSrc = Find(src.name);
            const auto* liveDst = Find(dst.name);
            if (!liveSrc || !liveDst || liveSrc->id != src.id || liveDst->id != dst.id)
                throw std::invalid_argument("OoT entrance route metadata mismatch");
            next[src.id].push_back({ dst.id });
        }
        layout = std::move(decoded);
        table = std::move(next);
    }
};
inline OotUnifiedLayout& ActiveOotUnified() {
    static OotUnifiedLayout value;
    return value;
}
struct OotUnifiedAssumptions {
    OotUnifiedRoutes routes;
    uint32_t world = 0;
};
inline std::optional<OotUnifiedAssumptions>& ActiveOotUnifiedAssumptions() {
    static thread_local std::optional<OotUnifiedAssumptions> value;
    return value;
}
namespace OotUnifiedDetail {
inline void RequireKeys(const nlohmann::json& value, std::initializer_list<std::string_view> keys) {
    if (!value.is_object() || value.size() != keys.size())
        throw std::invalid_argument("Malformed OoT entrance assumptions");
    for (const auto key : keys)
        if (!value.contains(std::string(key)))
            throw std::invalid_argument("Missing OoT entrance assumption field");
}
inline std::string Text(const nlohmann::json& value) {
    if (!value.is_string())
        throw std::invalid_argument("OoT entrance assumption names must be strings");
    return value.get<std::string>();
}
inline bool Flag(const nlohmann::json& value) {
    if (!value.is_boolean())
        throw std::invalid_argument("OoT entrance assumption flags must be booleans");
    return value.get<bool>();
}
inline const Record& Source(const nlohmann::json& value, const OotQuests& quests) {
    const auto* row = Find(Text(value));
    if (!row || !OotUnifiedSourceInQuests(*row, quests))
        throw std::invalid_argument("OoT entrance assumptions need a verified Wallmaster source of these dungeons");
    return *row;
}
} // namespace OotUnifiedDetail

// Each candidate's OoT portion uses the original source condition. Multiple destinations are ORed;
// no arrival grants ACCESS. Synthetic Wallmaster sources have no native exit to delete. A Master Quest
// context is admitted only while `caps` has the quests capability, and every source must exist in it.
inline OotUnifiedAssumptions DecodeOotUnifiedAssumptions(const nlohmann::json& value, const OotQuests& quests,
                                                         const CapabilitySet& caps = ActiveCapabilities()) {
    using namespace OotUnifiedDetail;
    RequireKeys(value, { "profile", "game", "registry", "world", "deleted", "fixed", "remaining", "assumed" });
    if (Text(value.at("profile")) != "ootmm-entrance-assumptions-v1" || Text(value.at("game")) != "oot" ||
        Text(value.at("registry")) != kRegistrySourceSha256)
        throw std::invalid_argument("Unsupported OoT entrance assumptions");
    const auto& world = value.at("world");
    if (!world.is_number_unsigned() || world.get<uint64_t>() > 0xffffffffull)
        throw std::invalid_argument("OoT entrance assumptions need a candidate world token");
    if (!value.at("assumed").is_array() || !value.at("assumed").empty())
        throw std::invalid_argument("OoT entrance assumptions do not support ASSUMED exits");
    for (const char* key : { "deleted", "fixed", "remaining" })
        if (!value.at(key).is_array())
            throw std::invalid_argument("Malformed OoT entrance assumption arrays");
    if (!value.at("deleted").empty() || !value.at("fixed").empty() || !value.at("remaining").empty())
        RequireAdmittedOotQuests(quests, caps);
    OotUnifiedAssumptions result;
    result.world = static_cast<uint32_t>(world.get<uint64_t>());
    std::set<std::string_view> deleted;
    for (const auto& row : value.at("deleted")) {
        RequireKeys(row, { "row", "void" });
        const auto& src = Source(row.at("row"), quests);
        if (Flag(row.at("void")) != IsVoidArea(src.to) || !deleted.insert(src.name).second)
            throw std::invalid_argument("Invalid or duplicate OoT deleted source");
    }
    for (const char* part : { "fixed", "remaining" })
        for (const auto& edge : value.at(part)) {
            RequireKeys(edge, { "src", "dst", "resetTime", "void" });
            const auto& src = Source(edge.at("src"), quests);
            const auto* dst = Find(Text(edge.at("dst")));
            if (!deleted.contains(src.name) || !dst || dst->game != Game::Oot ||
                !OotUnifiedDestinationBound(dst->name) || Flag(edge.at("resetTime")) || Flag(edge.at("void")) ||
                !LayoutV1::RowAssignable(src, *dst, true))
                throw std::invalid_argument("OoT entrance assumption route has no verified binding");
            result.routes[src.id].push_back({ dst->id });
        }
    return result;
}
inline std::vector<OotUnifiedRoute> OotUnifiedGraphRoutes(uint32_t source) {
    if (const auto& assumed = ActiveOotUnifiedAssumptions()) {
        const auto it = assumed->routes.find(source);
        return it == assumed->routes.end() ? std::vector<OotUnifiedRoute>{} : it->second;
    }
    return ActiveOotUnified().routesFrom(source);
}
} // namespace ComboRando::Entrances
