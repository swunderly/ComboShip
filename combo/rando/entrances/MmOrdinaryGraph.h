#pragma once
#include "MmOrdinaryLayout.h"
#include "rando/entrances/Shuffle.h"
#include <set>
namespace ComboRando::Entrances {
struct MmOrdinaryAssumptions {
    std::map<uint32_t, std::set<uint32_t>> routes;
    std::set<uint32_t> syntheticDestinations;
};
inline std::optional<MmOrdinaryAssumptions>& ActiveMmOrdinaryAssumptions() {
    static thread_local std::optional<MmOrdinaryAssumptions> value;
    return value;
}
inline MmOrdinaryAssumptions BuildMmOrdinaryAssumptions(const Overrides& overrides, const Pools& pools,
                                                        const Settings& settings) {
    MmOrdinaryAssumptions result;
    auto bind = [&](std::string_view source, std::string_view destination) {
        auto a = Find(source), b = Find(destination);
        if (!a || !b || !MmOrdinaryBound(source) || !MmOrdinaryBound(destination) || a->game != Game::Mm ||
            b->game != Game::Mm)
            throw std::invalid_argument("Unbound ordinary MM assumption");
        for (const auto& row : kMmOrdinaryBindings)
            if (row.name == source)
                result.routes[row.physicalId].insert(b->id);
    };
    for (const auto& [src, dst] : overrides)
        bind(src, dst);
    for (const auto& pool : pools) {
        // Pin solver/entrances.ts:279 and pathfind.ts:849: synthetic ASSUMED starts
        // unconditionally at OOT SPAWN day1; its null stay expands all future MM times.
        if (pool.src.size() * pool.dst.size() > 5000 && pool.src.size() > 50) {
            for (auto source : pool.src) {
                if (!MmOrdinaryBound(source))
                    throw std::invalid_argument("Unbound MM synthetic source");
                for (const auto& row : kMmOrdinaryBindings)
                    if (row.name == source)
                        result.routes.try_emplace(row.physicalId);
            }
            for (auto destination : pool.dst) {
                auto record = Find(destination);
                if (!record || !MmOrdinaryBound(destination))
                    throw std::invalid_argument("Unbound MM synthetic arrival");
                result.syntheticDestinations.insert(record->id);
            }
            continue;
        }
        for (auto source : pool.src) {
            if (!MmOrdinaryBound(source))
                throw std::invalid_argument("Unbound ordinary MM source pool");
            for (const auto& row : kMmOrdinaryBindings)
                if (row.name == source)
                    result.routes.try_emplace(row.physicalId);
            for (auto destination : pool.dst) {
                auto a = Find(source), b = Find(destination);
                if (!a || !b || !MmOrdinaryBound(destination))
                    throw std::invalid_argument("Unbound ordinary MM destination pool");
                if (Assignable(*a, *b, pool.ownGame, settings))
                    bind(source, destination);
            }
        }
    }
    return result;
}
inline bool MmOrdinaryGraphOverrides(uint32_t source) {
    if (const auto& assumed = ActiveMmOrdinaryAssumptions())
        if (assumed->routes.contains(CanonicalMmOrdinarySource(source)))
            return true;
    return ActiveMmOrdinary().active() && ActiveMmOrdinary().destination(source) != source;
}
inline std::vector<uint32_t> MmOrdinaryGraphDestinations(uint32_t source) {
    if (const auto& assumed = ActiveMmOrdinaryAssumptions()) {
        auto i = assumed->routes.find(CanonicalMmOrdinarySource(source));
        if (i != assumed->routes.end())
            return { i->second.begin(), i->second.end() };
    }
    return { ActiveMmOrdinary().destination(source) };
}
inline nlohmann::json EncodeMmOrdinaryAssumptions(const Overrides& overrides, const Pools& pools,
                                                  const Settings& settings) {
    BuildMmOrdinaryAssumptions(overrides, pools, settings);
    nlohmann::json value = { { "schemaVersion", 1 },
                             { "profile", "mm-ordinary-coupled-assumptions-v1" },
                             { "upstreamCommit", kUpstreamCommit },
                             { "registrySha256", kRegistrySourceSha256 },
                             { "noPolarity", settings.erNoPolarity },
                             { "selfLoops", settings.erSelfLoops },
                             { "overrides", nlohmann::json::object() },
                             { "remainingPools", nlohmann::json::array() } };
    for (const auto& [src, dst] : overrides)
        value["overrides"][std::string(src)] = dst;
    for (const auto& pool : pools)
        value["remainingPools"].push_back({ { "name", pool.name },
                                            { "src", pool.src },
                                            { "dst", pool.dst },
                                            { "ownGame", pool.ownGame },
                                            { "alias", pool.alias } });
    return value;
}
inline MmOrdinaryAssumptions DecodeMmOrdinaryAssumptions(const nlohmann::json& value) {
    if (!value.is_object() || value.value("schemaVersion", 0) != 1 ||
        value.value("profile", std::string()) != "mm-ordinary-coupled-assumptions-v1" ||
        value.value("upstreamCommit", std::string()) != kUpstreamCommit ||
        value.value("registrySha256", std::string()) != kRegistrySourceSha256 || !value.at("noPolarity").is_boolean() ||
        !value.at("selfLoops").is_boolean())
        throw std::invalid_argument("Invalid ordinary MM assumptions");
    Settings settings;
    settings.games = Games::Mm;
    settings.erNoPolarity = value["noPolarity"];
    settings.erSelfLoops = value["selfLoops"];
    Overrides overrides;
    Pools pools;
    for (const auto& [src, dst] : value.at("overrides").items()) {
        if (!dst.is_string())
            throw std::invalid_argument("Invalid ordinary MM assumed target");
        auto a = Find(src), b = Find(dst.get<std::string>());
        if (!a || !b)
            throw std::invalid_argument("Unknown ordinary MM assumed record");
        overrides[a->name] = b->name;
    }
    for (const auto& p : value.at("remainingPools")) {
        Pool pool;
        pool.name = p.at("name").get<std::string>();
        pool.ownGame = p.at("ownGame").get<bool>();
        pool.alias = p.at("alias").get<bool>();
        for (const auto& v : p.at("src")) {
            auto a = Find(v.get<std::string>());
            if (!a)
                throw std::invalid_argument("Unknown assumed source");
            pool.src.insert(a->name);
        }
        for (const auto& v : p.at("dst")) {
            auto a = Find(v.get<std::string>());
            if (!a)
                throw std::invalid_argument("Unknown assumed destination");
            pool.dst.insert(a->name);
        }
        pools.push_back(std::move(pool));
    }
    return BuildMmOrdinaryAssumptions(overrides, pools, settings);
}
} // namespace ComboRando::Entrances
