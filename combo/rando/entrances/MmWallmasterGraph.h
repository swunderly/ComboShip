#pragma once
#include "Wallmasters.h"
#include <array>
#include <vector>

namespace ComboRando::Entrances {
struct MmWallmasterAssumptions {
    std::array<std::vector<uint16_t>, 4> routes;
    bool enabled = false;
};
// Scoped to the native oracle's thread, independent of runtime loaded layout.
inline std::optional<MmWallmasterAssumptions>& ActiveMmWallmasterAssumptions() {
    static thread_local std::optional<MmWallmasterAssumptions> state;
    return state;
}
inline MmWallmasterAssumptions BuildMmWallmasterAssumptions(const Overrides& overrides,
                                                           const Pools& remaining) {
    Settings s;
    s.games = Games::Mm; s.erWallmasters = Mode::OwnGame;
    MmWallmasterAssumptions result;
    result.enabled = true;
    for (size_t i = 0; i < kMmWallmasterSources.size(); ++i) {
        const auto src = kMmWallmasterSources[i];
        std::set<uint16_t> destinations;
        const auto fixed = overrides.find(src);
        if (fixed != overrides.end()) {
            const auto* dst = Find(fixed->second);
            if (!dst || !MmBaseDestination(*dst)) throw std::invalid_argument("Unsupported assumed MM Wallmaster destination");
            destinations.insert(static_cast<uint16_t>(dst->id));
        } else {
            for (const auto& pool : remaining) {
                if (!pool.src.contains(src)) continue;
                if (pool.name != "WALLMASTERS" || !pool.alias || !pool.ownGame)
                    throw std::invalid_argument("Unsupported MM Wallmaster assumption pool");
                for (const auto name : pool.dst) {
                    const auto* dst = Find(name);
                    if (!dst || !MmBaseDestination(*dst, true))
                        throw std::invalid_argument("Unsupported assumed MM Wallmaster pool destination");
                    if (Assignable(*Find(src), *dst, true, s)) {
                        if (!dst->noSongOfTime || dst->id > 0xffffu)
                            throw std::invalid_argument("Unsupported assumed MM Wallmaster arrival context");
                        destinations.insert(static_cast<uint16_t>(dst->id));
                    }
                }
            }
        }
        result.routes[i].assign(destinations.begin(), destinations.end());
    }
    return result;
}
class MmWallmasterAssumptionScope {
    std::optional<MmWallmasterAssumptions> previous;
public:
    explicit MmWallmasterAssumptionScope(MmWallmasterAssumptions candidate)
        : previous(std::move(ActiveMmWallmasterAssumptions())) {
        ActiveMmWallmasterAssumptions() = std::move(candidate);
    }
    ~MmWallmasterAssumptionScope() { ActiveMmWallmasterAssumptions() = std::move(previous); }
    MmWallmasterAssumptionScope(const MmWallmasterAssumptionScope&) = delete;
    MmWallmasterAssumptionScope& operator=(const MmWallmasterAssumptionScope&) = delete;
};
inline std::vector<uint16_t> MmWallmasterGraphDestinations(std::string_view source) {
    const auto i = std::find(kMmWallmasterSources.begin(), kMmWallmasterSources.end(), source);
    if (i == kMmWallmasterSources.end()) return {};
    const auto& assumptions = ActiveMmWallmasterAssumptions();
    if (assumptions && assumptions->enabled) return assumptions->routes[i - kMmWallmasterSources.begin()];
    const auto dst = ActiveMmWallmasters().destination(source);
    return dst ? std::vector<uint16_t>{*dst} : std::vector<uint16_t>{};
}
} // namespace ComboRando::Entrances
