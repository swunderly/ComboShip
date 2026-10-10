#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

// Anchor caches one opaque state per team. Keep both games inside that state so a
// second game's reply cannot consume the first game's waiters or replace its save.
namespace ComboAnchorProtocol {
using Json = nlohmann::json;
inline constexpr int Version = 1;

inline std::string GamePrefix(int game) {
    return game == 1 ? "mm:" : "oot:";
}

inline bool AcceptEvent(Json& counters, const Json& p) {
    const std::string stream = p.value("comboStream", "");
    const uint64_t sequence = p.value("comboSequence", uint64_t{ 0 });
    if (stream.empty() || !sequence)
        return false;
    const int game = p.value("type", "") == "COMBO_CROSS_ITEM" ? p.value("targetGame", -1)
                                                               : (p.value("originGame", "oot") == "mm" ? 1 : 0);
    if (game < 0 || game > 1)
        return false;
    const std::string key = GamePrefix(game) + stream;
    const uint64_t previous = counters.value(key, uint64_t{ 0 });
    if (sequence <= previous)
        return false;
    counters[key] = sequence;
    return true;
}

// A background game's save must not commit duplicate markers for unsaved items
// in the foreground game. Commit each journal alongside its own inventory.
inline void CommitGameCounters(Json& saved, const Json& live, int game) {
    if (!saved.is_object())
        saved = Json::object();
    const auto prefix = GamePrefix(game);
    for (auto it = live.begin(); it != live.end(); ++it)
        if (it.key().compare(0, prefix.size(), prefix) == 0)
            saved[it.key()] = it.value();
}

inline void MergeCounters(Json& counters, const Json& incoming) {
    if (!incoming.is_object())
        return;
    for (auto it = incoming.begin(); it != incoming.end(); ++it) {
        if (it.value().is_number_unsigned())
            counters[it.key()] = std::max(counters.value(it.key(), uint64_t{ 0 }), it.value().get<uint64_t>());
    }
}

inline Json Combine(const Json& oot, const Json& mm, uint32_t seed) {
    if (!oot.contains("state") || !mm.contains("state") || !oot["state"].contains("healthCapacity") ||
        !mm["state"].contains("shipSaveInfo") || oot.value("targetTeamId", "") != mm.value("targetTeamId", "")) {
        throw std::runtime_error("Both loaded game saves are required for combined synchronization");
    }
    return { { "type", "UPDATE_TEAM_STATE" },
             { "clientId", oot.value("clientId", 0u) },
             { "targetTeamId", oot.at("targetTeamId") },
             { "originGame", "combo" },
             { "state",
               { { "comboProtocol", Version },
                 { "comboSeed", seed },
                 { "oot", oot.at("state") },
                 { "mm", mm.at("state") } } },
             { "queue", Json::array() } };
}

inline bool IsProgress(const Json& p) {
    const auto t = p.value("type", "");
    return t == "UPDATE_TEAM_STATE" || t == "GIVE_ITEM" || t == "COMBO_CROSS_ITEM" || t == "SET_CHECK_STATUS" ||
           t == "SET_FLAG" || t == "UNSET_FLAG" || t == "UPDATE_DUNGEON_ITEMS" || t == "UPDATE_BEANS_COUNT" ||
           t == "ENTRANCE_DISCOVERED";
}

// The public server drops envelope fields when replying from its cache. Protocol
// and seed belong inside state; queued packets retain their own routing tags.
inline std::vector<Json> Expand(const Json& packet, uint32_t seed) {
    if (packet.value("type", "") != "UPDATE_TEAM_STATE")
        return { packet };
    std::vector<Json> result;
    if (packet.contains("state") && !packet["state"].empty()) {
        const auto& state = packet["state"];
        if (state.value("comboProtocol", 0) != Version || state.value("comboSeed", 0u) != seed ||
            !state.contains("oot") || !state.contains("mm") || !state["oot"].contains("healthCapacity") ||
            !state["mm"].contains("shipSaveInfo")) {
            return {}; // incompatible/stale cached saves must never touch this seed
        }
        for (const char* game : { "oot", "mm" }) {
            auto p = packet;
            p.erase("queue");
            p["originGame"] = game;
            p["state"] = state.at(game);
            // Both inventories are present; per-game foreign backfill would grant
            // an additive item a second time after its target inventory was merged.
            p["comboSnapshot"] = true;
            result.push_back(std::move(p));
        }
    }
    if (packet.contains("queue") && packet["queue"].is_array()) {
        for (const auto& item : packet["queue"]) {
            if (!item.is_string())
                continue;
            auto p = Json::parse(item.get<std::string>(), nullptr, false);
            if (p.is_discarded() || !p.is_object() || !IsProgress(p) || p.value("type", "") == "UPDATE_TEAM_STATE" ||
                p.value("comboProtocol", 0) != Version || p.value("comboSeed", 0u) != seed)
                continue;
            result.push_back(std::move(p));
        }
    }
    return result;
}
} // namespace ComboAnchorProtocol
