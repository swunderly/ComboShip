// Optional seed extension. Missing field always uses legacy/default entrance behavior.
#pragma once
#include "MmWallmasterGraph.h"
#include <nlohmann/json.hpp>

namespace ComboRando::Entrances {
inline constexpr std::string_view kMmWallmasterProfile = "mm-wallmasters-own-game-v1";
inline Game ReadGame(const nlohmann::json& object, const char* field) {
    const auto value = object.at(field).get<std::string>();
    if (value == "mm")
        return Game::Mm;
    if (value == "oot")
        return Game::Oot;
    throw std::invalid_argument("Unknown entrance game");
}
inline uint32_t ReadLogicalId(const nlohmann::json& object, const char* field) {
    const auto& value = object.at(field);
    if (!value.is_number_integer() || (!value.is_number_unsigned() && value.get<int64_t>() < 0))
        throw std::invalid_argument("Entrance ID must be a nonnegative integer");
    const auto id = value.get<uint64_t>();
    if (id >= 0x80000000ull)
        throw std::invalid_argument("Entrance ID exceeds logical ID range");
    return static_cast<uint32_t>(id);
}
inline void CheckEnvelope(const nlohmann::json& value) {
    if (!value.is_object() || !value.at("schemaVersion").is_number_integer() || value.at("schemaVersion") != 1 ||
        value.at("profile").get<std::string>() != kMmWallmasterProfile ||
        value.at("upstreamCommit").get<std::string>() != kUpstreamCommit ||
        value.at("registrySourceSha256").get<std::string>() != kRegistrySourceSha256)
        throw std::invalid_argument("Unsupported entrance expansion envelope");
}
inline nlohmann::json Envelope() {
    return { { "schemaVersion", 1 },
             { "profile", kMmWallmasterProfile },
             { "upstreamCommit", kUpstreamCommit },
             { "registrySourceSha256", kRegistrySourceSha256 } };
}
inline std::optional<std::vector<SavedOverride>> DecodeMmWallmasterSeed(const nlohmann::json& seed) {
    if (!seed.is_object())
        throw std::invalid_argument("Seed JSON is not an object");
    if (!seed.contains("entranceExpansion"))
        return std::nullopt;
    const auto& value = seed.at("entranceExpansion");
    CheckEnvelope(value);
    if (!value.at("overrides").is_array())
        throw std::invalid_argument("Entrance override rows must be an array");
    std::vector<SavedOverride> rows;
    for (const auto& row : value.at("overrides"))
        rows.push_back({ row.at("source").get<std::string>(), row.at("destination").get<std::string>(),
                         ReadGame(row, "sourceGame"), ReadGame(row, "destinationGame"), ReadLogicalId(row, "sourceId"),
                         ReadLogicalId(row, "destinationId") });
    MmWallmasterLayout verified;
    verified.install(rows);
    return rows;
}
inline nlohmann::json EncodeMmWallmasterExtension(std::span<const SavedOverride> rows) {
    MmWallmasterLayout verified;
    verified.install(rows);
    auto result = Envelope();
    result["overrides"] = nlohmann::json::array();
    for (const auto& row : rows)
        result["overrides"].push_back({ { "source", row.source },
                                        { "destination", row.destination },
                                        { "sourceGame", row.sourceGame == Game::Oot ? "oot" : "mm" },
                                        { "destinationGame", row.destinationGame == Game::Oot ? "oot" : "mm" },
                                        { "sourceId", row.sourceId },
                                        { "destinationId", row.destinationId } });
    return result;
}
inline nlohmann::json EncodeMmWallmasterAssumptions(const Overrides& overrides, const Pools& remaining) {
    // Validate the same candidate before emitting any native oracle input.
    BuildMmWallmasterAssumptions(overrides, remaining);
    auto result = Envelope();
    result["overrides"] = nlohmann::json::object();
    for (const auto& [src, dst] : overrides)
        result["overrides"][std::string(src)] = dst;
    result["remainingPools"] = nlohmann::json::array();
    for (const auto& pool : remaining)
        result["remainingPools"].push_back({ { "name", pool.name },
                                             { "src", pool.src },
                                             { "dst", pool.dst },
                                             { "ownGame", pool.ownGame },
                                             { "alias", pool.alias } });
    return result;
}
inline MmWallmasterAssumptions DecodeMmWallmasterAssumptions(const nlohmann::json& value) {
    CheckEnvelope(value);
    if (!value.at("overrides").is_object() || !value.at("remainingPools").is_array())
        throw std::invalid_argument("Invalid entrance assumption container");
    Overrides overrides;
    for (const auto& [srcName, dstName] : value.at("overrides").items()) {
        const auto* src = Find(srcName);
        const auto* dst = Find(dstName.get<std::string>());
        if (!src || !dst)
            throw std::invalid_argument("Unknown assumption entrance");
        if (!Contains(kMmWallmasterSources, src->name) &&
            !(src->name == "MM_CLOCK_TOWER_FROM_CLOCK_TOWN" && dst->name == "MM_CLOCK_TOWN_FROM_CLOCK_TOWER"))
            throw std::invalid_argument("Unsupported assumption source");
        overrides[src->name] = dst->name;
    }
    Pools pools;
    for (const auto& poolJson : value.at("remainingPools")) {
        if (!poolJson.at("src").is_array() || !poolJson.at("dst").is_array())
            throw std::invalid_argument("Assumption pool entries must be arrays");
        Pool pool{ poolJson.at("name").get<std::string>(),
                   {},
                   {},
                   poolJson.at("ownGame").get<bool>(),
                   poolJson.at("alias").get<bool>() };
        for (const auto& valueSrc : poolJson.at("src")) {
            const auto* src = Find(valueSrc.get<std::string>());
            if (!src || !Contains(kMmWallmasterSources, src->name))
                throw std::invalid_argument("Unsupported assumption pool source");
            if (!pool.src.insert(src->name).second)
                throw std::invalid_argument("Duplicate assumption source");
        }
        for (const auto& valueDst : poolJson.at("dst")) {
            const auto* dst = Find(valueDst.get<std::string>());
            if (!dst || !MmBaseDestination(*dst, true))
                throw std::invalid_argument("Unsupported assumption pool destination");
            if (!pool.dst.insert(dst->name).second)
                throw std::invalid_argument("Duplicate assumption destination");
        }
        pools.push_back(std::move(pool));
    }
    return BuildMmWallmasterAssumptions(overrides, pools);
}
} // namespace ComboRando::Entrances
