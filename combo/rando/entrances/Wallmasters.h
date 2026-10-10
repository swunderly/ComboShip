// Source routing adapted from OoTMM mm/actors/En/En_Wallmas.c.
// See LICENSE.OoTMM. Unsupported contexts preserve the vanilla action.
#pragma once
#include "Context.h"
#include "Shuffle.h"
#include <array>
#include <optional>
#include <span>

namespace ComboRando::Entrances {
enum class SceneContext : uint8_t {
    Unknown,
    MmDampe,
    MmBeneathWell,
};
inline const Record* WallmasterSource(SceneContext scene, uint8_t room) {
    std::string_view name;
    if (scene == SceneContext::MmDampe)
        name = "MM_WALLMASTER_DAMPE";
    if (scene == SceneContext::MmBeneathWell)
        switch (room) {
            case 1:
                name = "MM_WALLMASTER_BTW_ENTRANCE";
                break;
            case 5:
                name = "MM_WALLMASTER_BTW_FOUNTAIN";
                break;
            case 7:
                name = "MM_WALLMASTER_BTW_EXIT";
                break;
            default:
                break;
        }
    return Find(name);
}

inline constexpr std::array<std::string_view, 4> kMmWallmasterSources = {
    "MM_WALLMASTER_DAMPE",
    "MM_WALLMASTER_BTW_ENTRANCE",
    "MM_WALLMASTER_BTW_FOUNTAIN",
    "MM_WALLMASTER_BTW_EXIT",
};
inline constexpr std::array<std::string_view, 7> kMmBaseWallmasterDestinations = {
    "MM_TEMPLE_WOODFALL",    "MM_TEMPLE_SNOWHEAD",    "MM_TEMPLE_GREAT_BAY",   "MM_TEMPLE_STONE_TOWER_INVERTED",
    "MM_TEMPLE_STONE_TOWER", "MM_SPIDER_HOUSE_SWAMP", "MM_SPIDER_HOUSE_OCEAN",
};
inline bool Contains(std::span<const std::string_view> list, std::string_view value) {
    return std::find(list.begin(), list.end(), value) != list.end();
}
inline bool MmBaseDestination(const Record& e, bool includeReverse = false) {
    if (Contains(kMmBaseWallmasterDestinations, e.name))
        return true;
    if (includeReverse && !e.reverse.empty())
        return Contains(kMmBaseWallmasterDestinations, e.reverse);
    return false;
}
struct SavedOverride {
    std::string source, destination;
    Game sourceGame = Game::Mm, destinationGame = Game::Mm;
    uint32_t sourceId = 0, destinationId = 0;
};

// Complete bounded profile: default MM-only own-game Wallmaster alias pool.
// The complete requested settings have wider pools and are rejected by this
// runtime profile until their native arrival/graph adapters are available.
class MmWallmasterLayout {
    std::array<uint32_t, 4> destinationIds{};
    bool enabled = false;

  public:
    void clear() {
        destinationIds.fill(0);
        enabled = false;
    }
    bool active() const {
        return enabled;
    }
    void install(std::span<const SavedOverride> rows) {
        // Validate into a local candidate, then publish all routes together.
        if (rows.size() != kMmWallmasterSources.size())
            throw std::invalid_argument("MM Wallmaster layout must contain exactly four source routes");
        std::array<uint32_t, 4> candidate{};
        std::set<std::string_view> seenSrc, seenDst;
        for (const auto& row : rows) {
            const auto* src = Find(row.source);
            const auto* dst = Find(row.destination);
            if (!src || !dst || !Contains(kMmWallmasterSources, src->name) || !MmBaseDestination(*dst))
                throw std::invalid_argument("Unsupported MM Wallmaster route");
            if (row.sourceGame != src->game || row.destinationGame != dst->game || row.sourceId != src->id ||
                row.destinationId != dst->id)
                throw std::invalid_argument("Entrance route metadata mismatch");
            if (!seenSrc.insert(src->name).second || !seenDst.insert(dst->name).second)
                throw std::invalid_argument("Duplicate MM Wallmaster source/destination");
            const auto index = std::find(kMmWallmasterSources.begin(), kMmWallmasterSources.end(), src->name) -
                               kMmWallmasterSources.begin();
            if (!dst->noSongOfTime || dst->game != Game::Mm || dst->id > 0xffffu)
                throw std::invalid_argument("MM Wallmaster profile requires native no-sot arrivals");
            candidate[index] = dst->id;
        }
        destinationIds = candidate;
        enabled = true;
    }
    std::optional<uint16_t> destination(std::string_view source) const {
        if (!enabled)
            return std::nullopt;
        const auto it = std::find(kMmWallmasterSources.begin(), kMmWallmasterSources.end(), source);
        if (it == kMmWallmasterSources.end())
            return std::nullopt;
        return static_cast<uint16_t>(destinationIds[it - kMmWallmasterSources.begin()]);
    }
    // The caller clears the real dungeon ingress when this returns a route.
    std::optional<uint16_t> capture(SceneContext scene, uint8_t room) const {
        const auto* src = WallmasterSource(scene, room);
        if (!src)
            return std::nullopt;
        return destination(src->name);
    }
};
inline MmWallmasterLayout& ActiveMmWallmasters() {
    static MmWallmasterLayout layout;
    return layout;
}
} // namespace ComboRando::Entrances
