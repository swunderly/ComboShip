#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace ComboRando {
// Endpoint ordering matches each engine's native song/soaring menu. Rules/pool reference:
// OoTMM a284b8aa: settings erWarps, solver/entrances.ts poolWarps, both _system.yml.
// Destination shuffle and shared cross-game song playback are independent seed options.
inline constexpr int kSongDestinations = 16;
struct SongTravelMap {
    bool enabled = false;
    bool foreignSongs = false;
    bool logicEnabled = false;
    bool logicForeignSongs = false;
    std::array<int, kSongDestinations> destinations{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    int destination(int source) const {
        return source >= 0 && source < kSongDestinations ? (enabled ? destinations[source] : source) : -1;
    }
    uint32_t resolve(uint32_t sources) const {
        uint32_t out = 0;
        if (enabled || foreignSongs)
            for (int i = 0; i < kSongDestinations; i++)
                if (sources & (1u << i))
                    out |= 1u << destination(i);
        return out;
    }
    uint32_t resolveLogic(uint32_t sources) const {
        uint32_t out = 0;
        if (logicEnabled || logicForeignSongs)
            for (int i = 0; i < kSongDestinations; i++)
                if (sources & (1u << i))
                    out |= 1u << (logicEnabled ? destinations[i] : i);
        return out;
    }
    int logicDestination(int source) const {
        return source >= 0 && source < 16 ? (logicEnabled ? destinations[source] : source) : -1;
    }
    nlohmann::json json() const {
        return { { "version", 1 },
                 { "enabled", enabled },
                 { "foreignSongs", foreignSongs },
                 { "logicEnabled", logicEnabled },
                 { "logicForeignSongs", logicForeignSongs },
                 { "destinations", destinations } };
    }
    static SongTravelMap parse(const nlohmann::json& j) {
        SongTravelMap map;
        if (j.is_null())
            return map; // older seeds retain native warps
        if (!j.is_object() || j.value("version", 0) != 1 || !j.contains("enabled") || !j["enabled"].is_boolean() ||
            !j.contains("destinations") || !j["destinations"].is_array() || j["destinations"].size() != 16)
            throw std::invalid_argument("Invalid song travel map");
        uint32_t seen = 0;
        for (int i = 0; i < 16; i++) {
            const auto& value = j["destinations"][i];
            if (!value.is_number_integer() || value < 0 || value >= 16)
                throw std::invalid_argument("Invalid song destination");
            int d = value.get<int>();
            if (seen & (1u << d))
                throw std::invalid_argument("Duplicate song destination");
            seen |= 1u << d;
            map.destinations[i] = d;
        }
        map.enabled = j["enabled"].get<bool>();
        map.foreignSongs = j.value("foreignSongs", false);
        map.logicEnabled = j.value("logicEnabled", map.enabled);
        map.logicForeignSongs = j.value("logicForeignSongs", map.foreignSongs);
        return map;
    }
    static SongTravelMap generate(uint64_t seed, bool enabled, bool foreignSongs = false) {
        SongTravelMap map;
        map.enabled = enabled;
        map.foreignSongs = foreignSongs;
        map.logicEnabled = enabled;
        map.logicForeignSongs = foreignSongs;
        if (!enabled)
            return map;
        // Separate stream: routes are stable and never consume the item-fill RNG.
        uint64_t state = seed ^ UINT64_C(0x534f4e4754524156);
        for (int i = 15; i > 0; --i) {
            state = state * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
            const int j = int((state >> 32) % uint64_t(i + 1));
            std::swap(map.destinations[i], map.destinations[j]);
        }
        return map;
    }
    static SongTravelMap generateForRules(uint64_t seed, bool runtimeShuffle, bool runtimeForeign, bool logicalShuffle,
                                          bool logicalForeign) {
        // Destination changes can remove vanilla routes. Create a shuffled map only when
        // generation models it; enhancement-only activation reuses the seed's stored map.
        auto map = generate(seed, logicalShuffle, runtimeForeign);
        map.enabled = runtimeShuffle;
        map.logicEnabled = logicalShuffle;
        map.logicForeignSongs = logicalForeign;
        return map;
    }
};
// The launcher installs the SAME validated seed map in both engines before searching or playing.
inline SongTravelMap& ActiveSongTravel() {
    static SongTravelMap map;
    return map;
}
inline uint32_t& OotForeignKnowledge() {
    static uint32_t mask = 0;
    return mask;
}
inline uint32_t& MmForeignKnowledge() {
    static uint32_t mask = 0;
    return mask;
}
inline uint32_t& OotSongArrivals() {
    static uint32_t mask = 0;
    return mask;
}
inline uint32_t& MmSongArrivals() {
    static uint32_t mask = 0;
    return mask;
}
inline bool& OotSongOracle() {
    static bool active = false;
    return active;
}
inline bool& MmSongOracle() {
    static bool active = false;
    return active;
}
struct SongAgeArrivals {
    bool active = false;
    std::array<uint32_t, 2> masks{};
};
inline SongAgeArrivals& OotSongAgeArrivals() {
    static SongAgeArrivals value;
    return value;
}
inline SongAgeArrivals& MmSongAgeArrivals() {
    static SongAgeArrivals value;
    return value;
}

} // namespace ComboRando
