// Source routing adapted from OoTMM En/En_Wallmas.c in each game.
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
    OotForest,
    OotShadow,
    OotSpirit,
    OotBottomWell,
    OotGtg,
    OotGanonCastle,
    MmDampe,
    MmBeneathWell,
};
inline const Record* WallmasterSource(Game game, SceneContext scene, uint8_t room) {
    std::string_view name;
    if (game == Game::Mm) {
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
    } else {
        switch (scene) {
            case SceneContext::OotForest:
                name = room == 0x14 ? "OOT_WALLMASTER_FOREST_CORRIDOR_EAST" : "OOT_WALLMASTER_FOREST_CORRIDOR_WEST";
                break;
            case SceneContext::OotShadow:
                name = "OOT_WALLMASTER_SHADOW";
                break;
            case SceneContext::OotSpirit:
                switch (room) {
                    case 0x02:
                        name = "OOT_WALLMASTER_SPIRIT_CHILD_RUPEES";
                        break;
                    case 0x05:
                        name = "OOT_WALLMASTER_SPIRIT_STATUE";
                        break;
                    case 0x08:
                        name = "OOT_WALLMASTER_SPIRIT_CHILD_SUN";
                        break;
                    case 0x0f:
                        name = "OOT_WALLMASTER_SPIRIT_ADULT_CLIMB";
                        break;
                    default:
                        break;
                }
                break;
            case SceneContext::OotBottomWell:
                switch (room) {
                    case 0:
                        name = "OOT_WALLMASTER_BOTW_MAIN";
                        break;
                    case 1:
                        name = "OOT_WALLMASTER_BOTW_BASEMENT";
                        break;
                    case 3:
                        name = "OOT_WALLMASTER_BOTW_PIT";
                        break;
                    default:
                        break;
                }
                break;
            case SceneContext::OotGtg:
                name = "OOT_WALLMASTER_GTG";
                break;
            case SceneContext::OotGanonCastle:
                name = room == 0x0a ? "OOT_WALLMASTER_GANON_LIGHT" : "OOT_WALLMASTER_GANON_SPIRIT";
                break;
            default:
                break;
        }
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
    std::optional<Arrival> capture(SceneContext scene, uint8_t room, OotAge age, MmForm form,
                                   SaveContexts& saves) const {
        const auto* src = WallmasterSource(Game::Mm, scene, room);
        if (!src)
            return std::nullopt;
        const auto dst = destination(src->name);
        if (!dst)
            return std::nullopt;
        // Upstream clears only the custom dungeon ingress; per-age Farore
        // context remains available. MM capture keeps current native form.
        saves.clearDungeonIngress();
        return Arrival{ { Game::Mm, *dst }, ArrivalReason::Entrance, age, form, {}, {}, true };
    }
};
inline MmWallmasterLayout& ActiveMmWallmasters() {
    static MmWallmasterLayout layout;
    return layout;
}
} // namespace ComboRando::Entrances
