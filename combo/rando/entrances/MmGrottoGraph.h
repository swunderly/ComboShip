// Full source-qualified MM grotto graph and pinned assumption arcs.
#pragma once
#include "rando/entrances/LegacyEnvelope.h"
#include "rando/entrances/MmGrottoLayout.h"
#include "rando/entrances/Shuffle.h"
namespace ComboRando::Entrances {
inline uint32_t MmGrottoLogicArrival(uint32_t destination) {
    for (const auto& row : kMmGrottoGraphBindings)
        if (row.logicalId == destination)
            return row.arrivalProxy;
    throw std::invalid_argument("Unbound MM typed grotto graph arrival");
}
struct MmGrottoAssumptions {
    std::map<uint32_t, std::set<uint32_t>> routes;
};
inline std::optional<MmGrottoAssumptions>& ActiveMmGrottoAssumptions() {
    static thread_local std::optional<MmGrottoAssumptions> value;
    return value;
}
inline MmGrottoAssumptions BuildMmGrottoAssumptions(const Overrides& rows, const Pools& pools,
                                                    const Settings& settings) {
    MmGrottoAssumptions result;
    auto bind = [&](std::string_view src, std::string_view dst) {
        const auto* a = Find(src);
        const auto* b = Find(dst);
        if (!a || !b || !MmGrottoBound(src) || !MmGrottoBound(dst))
            throw std::invalid_argument("Unbound MM grotto assumption");
        result.routes[a->id].insert(b->id);
    };
    for (const auto& [src, dst] : rows) {
        if (IsMmGrottoClockTowerReservation(src, dst))
            continue;
        bind(src, dst);
    }
    for (const auto& pool : pools) {
        // The complete default-US pool has58 records: cardinality3364. It never
        // invokes the pinned >5000 synthetic-ASSUMED optimization. Reject other
        // scopes explicitly rather than replacing their source/time/flag gates.
        if (pool.src.size() * pool.dst.size() > 5000 && pool.src.size() > 50)
            throw std::invalid_argument("MM grotto synthetic scope unsupported");
        for (const auto src : pool.src) {
            const auto* a = Find(src);
            if (!a || !MmGrottoBound(src))
                throw std::invalid_argument("Unbound MM grotto assumed source");
            result.routes.try_emplace(a->id);
            for (const auto dst : pool.dst) {
                const auto* b = Find(dst);
                if (!b || !MmGrottoBound(dst))
                    throw std::invalid_argument("Unbound MM grotto assumed destination");
                if (Assignable(*a, *b, pool.ownGame, settings))
                    bind(src, dst);
            }
        }
    }
    return result;
}
inline bool MmGrottoGraphOverrides(Key source) {
    if (const auto& assumed = ActiveMmGrottoAssumptions())
        if (assumed->routes.contains(source.id))
            return true;
    return ActiveMmGrottos().containsSource(source);
}
inline std::vector<uint32_t> MmGrottoGraphDestinations(Key source) {
    if (const auto& assumed = ActiveMmGrottoAssumptions()) {
        auto i = assumed->routes.find(source.id);
        if (i != assumed->routes.end())
            return { i->second.begin(), i->second.end() };
    }
    return { ActiveMmGrottos().destination(source).id };
}
inline nlohmann::json EncodeMmGrottoAssumptions(const Overrides& rows, const Pools& pools, const Settings& settings) {
    BuildMmGrottoAssumptions(rows, pools, settings);
    nlohmann::json value = { { "schemaVersion", 1 },
                             { "profile", "mm-grotto-coupled-assumptions-v1" },
                             { "upstreamCommit", kLegacyUpstreamCommit },
                             { "registrySha256", kLegacyRegistrySourceSha256 },
                             { "noPolarity", settings.erNoPolarity },
                             { "selfLoops", settings.erSelfLoops },
                             { "overrides", nlohmann::json::object() },
                             { "remainingPools", nlohmann::json::array() } };
    for (const auto& [src, dst] : rows)
        value["overrides"][std::string(src)] = dst;
    for (const auto& pool : pools)
        value["remainingPools"].push_back({ { "name", pool.name },
                                            { "src", pool.src },
                                            { "dst", pool.dst },
                                            { "ownGame", pool.ownGame },
                                            { "alias", pool.alias } });
    return value;
}
inline MmGrottoAssumptions DecodeMmGrottoAssumptions(const nlohmann::json& value) {
    if (!value.is_object() || value.value("schemaVersion", 0) != 1 ||
        value.value("profile", std::string()) != "mm-grotto-coupled-assumptions-v1" ||
        value.value("upstreamCommit", std::string()) != kLegacyUpstreamCommit ||
        value.value("registrySha256", std::string()) != kLegacyRegistrySourceSha256 ||
        !value.at("noPolarity").is_boolean() || !value.at("selfLoops").is_boolean() ||
        !value.at("overrides").is_object() || !value.at("remainingPools").is_array())
        throw std::invalid_argument("Invalid MM grotto assumptions");
    Settings settings;
    settings.games = Games::Mm;
    settings.erNoPolarity = value["noPolarity"];
    settings.erSelfLoops = value["selfLoops"];
    Overrides rows;
    Pools pools;
    for (const auto& [src, dst] : value["overrides"].items()) {
        if (!dst.is_string())
            throw std::invalid_argument("Invalid MM grotto assumed target");
        const auto* a = Find(src);
        const auto* b = Find(dst.get<std::string>());
        if (!a || !b)
            throw std::invalid_argument("Unknown MM grotto assumed record");
        rows[a->name] = b->name;
    }
    for (const auto& p : value["remainingPools"]) {
        Pool pool;
        pool.name = p.at("name").get<std::string>();
        pool.ownGame = p.at("ownGame").get<bool>();
        pool.alias = p.at("alias").get<bool>();
        for (const auto& v : p.at("src")) {
            const auto* r = Find(v.get<std::string>());
            if (!r)
                throw std::invalid_argument("Unknown assumed source");
            pool.src.insert(r->name);
        }
        for (const auto& v : p.at("dst")) {
            const auto* r = Find(v.get<std::string>());
            if (!r)
                throw std::invalid_argument("Unknown assumed destination");
            pool.dst.insert(r->name);
        }
        pools.push_back(std::move(pool));
    }
    return BuildMmGrottoAssumptions(rows, pools, settings);
}
} // namespace ComboRando::Entrances
