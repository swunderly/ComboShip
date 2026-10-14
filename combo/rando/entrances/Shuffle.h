// Native adaptation of OoTMM packages/logic/src/solver/entrances.ts.
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "Registry.h"
#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ComboRando::Entrances {
enum class Mode : uint8_t { None = 0, OwnGame = 1, Full = 2 };
enum class Games : uint8_t { Both, Oot, Mm };
enum class Warps : uint8_t { None, OotOnly, MmOnly, OwnGame, Full };
enum class Spawns : uint8_t { None = 0, Child = 1, Adult = 2, Both = 3 };
enum class Polarity : uint8_t { In, Out, Any };

// New native profile: all entrance extensions are off until the seed enables them.
// These represent upstream field names directly; they are not menu capability claims.
struct Settings {
    Games games = Games::Both;
    Mode erBoss = Mode::None, erDungeons = Mode::None, erGrottos = Mode::None;
    Mode erRegions = Mode::None, erOverworld = Mode::None, erIndoors = Mode::None;
    Mode erMixed = Mode::None, erOneWays = Mode::None, erWallmasters = Mode::None;
    Warps erWarps = Warps::None;
    Spawns erSpawns = Spawns::None;
    bool erMajorDungeons = false, erMinorDungeons = false;
    bool erGanonCastle = false, erGanonTower = false, erSpiderHouses = false;
    bool erPirateFortress = false, erBeneathWell = false, erIkanaCastle = false;
    bool erSecretShrine = false, erMoon = false;
    bool erRegionsExtra = false, erRegionsShortcuts = false, erPiratesWorld = false;
    bool erIndoorsMajor = false, erIndoorsExtra = false, erIndoorsTelescopes = false;
    bool erIndoorsGameLinks = false;
    bool erMixedDungeons = false, erMixedGrottos = false, erMixedRegions = false;
    bool erMixedOverworld = false, erMixedIndoors = false;
    bool erOneWaysAnywhere = false, erOneWaysMajor = false, erOneWaysIkana = false;
    bool erOneWaysSongs = false, erOneWaysStatues = false, erOneWaysOwls = false;
    bool erOneWaysWoods = false, erOneWaysWaterVoids = false;
    bool erDecoupled = false, erNoPolarity = false, erSelfLoops = false;
};

using Names = std::set<std::string_view>;
using Types = std::set<Type>;
using Overrides = std::map<std::string_view, std::string_view>;
struct PoolDescription {
    std::string name;
    Types src, dst;
    bool ownGame = false, alias = false;
};
struct Pool {
    std::string name;
    Names src, dst;
    bool ownGame = false, alias = false;
};
using Descriptions = std::vector<PoolDescription>;
using Pools = std::vector<Pool>;

inline bool Active(Mode mode) {
    return mode != Mode::None;
}
inline Types DungeonTypes(const Settings& s) {
    Types t;
    if (s.erMajorDungeons)
        t.insert(Type::Dungeon);
    if (s.erMinorDungeons)
        t.insert(Type::DungeonMinor);
    if (s.erGanonCastle)
        t.insert(Type::DungeonGanon);
    if (s.erGanonTower)
        t.insert(Type::DungeonGanonTower);
    if (s.erSpiderHouses)
        t.insert(Type::DungeonSh);
    if (s.erPirateFortress)
        t.insert(Type::DungeonPf);
    if (s.erBeneathWell)
        t.insert(Type::DungeonBtw);
    if (s.erIkanaCastle)
        t.insert(Type::DungeonAcoi);
    if (s.erSecretShrine)
        t.insert(Type::DungeonSs);
    if (s.erMoon)
        t.insert(Type::DungeonCtr);
    return t;
}
inline Types OneWayTypes(const Settings& s) {
    Types t;
    if (s.erOneWaysMajor)
        t.insert(Type::OneWay);
    if (s.erOneWaysIkana)
        t.insert(Type::OneWayIkana);
    if (s.erOneWaysSongs)
        t.insert(Type::OneWaySong);
    if (s.erOneWaysStatues)
        t.insert(Type::OneWayStatue);
    if (s.erOneWaysOwls)
        t.insert(Type::OneWayOwl);
    if (s.erOneWaysWoods)
        t.insert(Type::OneWayWoods);
    if (s.erOneWaysWaterVoids)
        t.insert(Type::OneWayWaterVoid);
    return t;
}
inline PoolDescription Permutation(std::string name, Types types, Mode mode) {
    return { std::move(name), types, std::move(types), mode == Mode::OwnGame, false };
}

inline Descriptions MakeSimplePools(const Settings& s) {
    Descriptions p;
    if (Active(s.erBoss))
        p.push_back(Permutation("BOSS", { Type::Boss }, s.erBoss));
    if (Active(s.erDungeons))
        p.push_back(Permutation("DUNGEONS", DungeonTypes(s), s.erDungeons));
    if (Active(s.erGrottos))
        p.push_back(Permutation("GROTTOS", { Type::Grotto, Type::Grave }, s.erGrottos));
    if (Active(s.erRegions)) {
        Types t{ Type::Region };
        if (s.erRegionsExtra)
            t.insert(Type::RegionExtra);
        if (s.erRegionsShortcuts)
            t.insert(Type::RegionShortcut);
        p.push_back(Permutation("REGIONS", std::move(t), s.erRegions));
    }
    if (Active(s.erOverworld)) {
        Types t{ Type::Region, Type::RegionExtra, Type::RegionShortcut, Type::Overworld };
        if (s.erPiratesWorld) {
            t.insert(Type::OverworldPf);
            t.insert(Type::DungeonPf);
        }
        p.push_back(Permutation("OVERWORLD", std::move(t), s.erOverworld));
    }
    if (Active(s.erIndoors)) {
        Types t;
        if (s.erIndoorsMajor)
            t.insert(Type::Indoors);
        if (s.erIndoorsExtra)
            t.insert(Type::IndoorsExtra);
        if (s.erIndoorsExtra && s.erPiratesWorld)
            t.insert(Type::IndoorsPf);
        if (s.erIndoorsTelescopes)
            t.insert(Type::IndoorsTelescope);
        if (s.erIndoorsGameLinks)
            t.insert(Type::IndoorsLink);
        p.push_back(Permutation("INDOORS", std::move(t), s.erIndoors));
    }
    if (Active(s.erMixed)) {
        PoolDescription mixed{ "MIXED", {}, {}, s.erMixed == Mode::OwnGame, false };
        auto merge = [&](std::string_view name, bool enabled) {
            if (!enabled)
                return;
            const auto it = std::find_if(p.begin(), p.end(), [&](const auto& e) { return e.name == name; });
            if (it == p.end())
                throw std::invalid_argument("Mixed category is not enabled: " + std::string(name));
            mixed.src.insert(it->src.begin(), it->src.end());
            mixed.dst.insert(it->dst.begin(), it->dst.end());
            p.erase(it);
        };
        merge("DUNGEONS", s.erMixedDungeons);
        merge("GROTTOS", s.erMixedGrottos);
        merge("REGIONS", s.erMixedRegions);
        merge("OVERWORLD", s.erMixedOverworld);
        merge("INDOORS", s.erMixedIndoors);
        p.push_back(std::move(mixed));
    }
    if (s.erWarps != Warps::None) {
        Types t{ Type::OneWaySong, Type::OneWayStatue };
        if (s.erWarps == Warps::OotOnly || s.erOneWaysStatues)
            t.erase(Type::OneWayStatue);
        if (s.erWarps == Warps::MmOnly || s.erOneWaysSongs)
            t.erase(Type::OneWaySong);
        p.push_back({ "WARPS", t, t, s.erWarps == Warps::OwnGame, false });
    }
    if (Active(s.erOneWays) && !s.erOneWaysAnywhere)
        p.push_back(Permutation("ONE_WAYS", OneWayTypes(s), s.erOneWays));
    return p;
}

inline Descriptions MakePools(const Settings& s) {
    auto p = MakeSimplePools(s);
    Types ordinaryDestinations;
    for (const auto& pool : p)
        ordinaryDestinations.insert(pool.dst.begin(), pool.dst.end());
    if (s.erSpawns != Spawns::None) {
        Types src, dst = ordinaryDestinations;
        if (s.erSpawns == Spawns::Adult || s.erSpawns == Spawns::Both)
            src.insert(Type::SpawnAdult);
        if (s.erSpawns == Spawns::Child || s.erSpawns == Spawns::Both)
            src.insert(Type::SpawnChild);
        dst.erase(Type::Boss);
        // Upstream also adds type 'spawn', absent from this pinned registry.
        dst.insert(Type::Indoors);
        dst.insert(Type::OneWaySong);
        dst.insert(Type::Region);
        p.push_back({ "SPAWNS", std::move(src), std::move(dst), true, true });
    }
    auto extended = ordinaryDestinations;
    extended.insert(Type::Dungeon);
    extended.insert(Type::DungeonMinor);
    extended.insert(Type::DungeonSh);
    if (Active(s.erWallmasters)) {
        auto dst = extended;
        if (Active(s.erBoss))
            dst.insert(Type::Boss);
        p.push_back({ "WALLMASTERS", { Type::Wallmaster }, std::move(dst), s.erWallmasters == Mode::OwnGame, true });
    }
    if (Active(s.erOneWays) && s.erOneWaysAnywhere) {
        auto src = OneWayTypes(s), dst = extended;
        if (Active(s.erBoss))
            dst.insert(Type::Boss);
        dst.insert(src.begin(), src.end());
        p.push_back({ "ONE_WAYS", std::move(src), std::move(dst), s.erOneWays == Mode::OwnGame, true });
    }
    return p;
}

inline Polarity EntrancePolarity(const Record& e, const Settings& s) {
    if (Active(s.erOverworld) && (e.type == Type::Region || e.type == Type::RegionExtra ||
                                  e.type == Type::RegionShortcut || e.type == Type::RegionExit))
        return Polarity::Any;
    switch (e.type) {
        case Type::Boss:
        case Type::Dungeon:
        case Type::DungeonMinor:
        case Type::DungeonGanon:
        case Type::DungeonGanonTower:
        case Type::DungeonSh:
        case Type::DungeonPf:
        case Type::DungeonBtw:
        case Type::DungeonAcoi:
        case Type::DungeonSs:
        case Type::DungeonCtr:
        case Type::Wallmaster:
        case Type::Region:
        case Type::RegionExtra:
        case Type::RegionShortcut:
        case Type::Indoors:
        case Type::IndoorsPf:
        case Type::IndoorsExtra:
        case Type::IndoorsSpecial:
        case Type::Grotto:
        case Type::Grave:
            return Polarity::In;
        case Type::DungeonExit:
        case Type::IndoorsExit:
        case Type::RegionExit:
        case Type::GrottoExit:
        case Type::GraveExit:
            return Polarity::Out;
        default:
            return Polarity::Any;
    }
}
inline bool IsSelfLoop(const Record& src, const Record& dst) {
    if (src.fromMap == "NONE" || dst.toMap != src.fromMap)
        return false;
    if (src.fromMap != src.toMap || dst.fromMap != dst.toMap)
        return true;
    return src.fromSubmap == dst.toSubmap;
}
inline bool Assignable(const Record& src, const Record& dst, bool ownGame, const Settings& s) {
    if (ownGame && src.game != dst.game)
        return false;
    const auto a = EntrancePolarity(src, s), b = EntrancePolarity(dst, s);
    if (!s.erNoPolarity && a != Polarity::Any && b != Polarity::Any && a != b)
        return false;
    return s.erSelfLoops || !IsSelfLoop(src, dst);
}

class ShuffleError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

using AreaExists = std::function<bool(std::string_view)>;
// computeAllEntrances (entrances.ts:179-201): the rows of the selected games whose areas exist.
inline Names AvailableEntrances(const Settings& s, const AreaExists& areaExists) {
    Names available;
    for (const auto& e : kRegistry) {
        if ((s.games == Games::Oot && e.game != Game::Oot) || (s.games == Games::Mm && e.game != Game::Mm))
            continue;
        if (e.from != "NONE" && !areaExists(e.from))
            continue;
        if (e.to != "NONE" && !areaExists(e.to))
            continue;
        available.insert(e.name);
    }
    return available;
}
// The game links stay connected unless they are shuffled (entrances.ts:888-911).
inline Overrides ReservedEntrances(const Settings& s) {
    Overrides fixed;
    if (!s.erIndoorsGameLinks) {
        if (s.games != Games::Mm)
            fixed["OOT_SHOP_MASKS"] =
                s.games == Games::Both ? "MM_CLOCK_TOWN_FROM_CLOCK_TOWER" : "OOT_MARKET_FROM_MASK_SHOP";
        if (s.games != Games::Oot)
            fixed["MM_CLOCK_TOWER_FROM_CLOCK_TOWN"] =
                s.games == Games::Both ? "OOT_MARKET_FROM_MASK_SHOP" : "MM_CLOCK_TOWN_FROM_CLOCK_TOWER";
    }
    return fixed;
}
// entrancesForTypes (entrances.ts:601-606) over the available rows.
inline Names EntrancesForTypes(const Types& types, const Names& available) {
    Names result;
    for (const auto& name : available) {
        const auto& e = *Find(name);
        if (!types.contains(e.type))
            continue;
        result.insert(name);
        if (const auto* reverse = Reverse(e)) {
            // A dangling native region binding is rejected, not added implicitly.
            if (!available.contains(reverse->name))
                throw std::invalid_argument("Missing reverse graph area for " + std::string(name));
            result.insert(reverse->name);
        }
    }
    return result;
}
// One pool of placePools (entrances.ts:567-589, 608-626): rows the fixed overrides use and the game
// links removed. Empty when it has no source.
inline std::optional<Pool> ResolvePool(const PoolDescription& d, const Names& available, const Overrides& fixed) {
    Pool p{ d.name, EntrancesForTypes(d.src, available), EntrancesForTypes(d.dst, available), d.ownGame, d.alias };
    for (const auto& [src, dst] : fixed) {
        p.src.erase(src);
        if (!p.alias)
            p.dst.erase(dst);
    }
    p.src.erase("OOT_MARKET_FROM_MASK_SHOP");
    p.src.erase("MM_CLOCK_TOWN_FROM_CLOCK_TOWER");
    p.dst.erase("OOT_SHOP_MASKS");
    p.dst.erase("MM_CLOCK_TOWER_FROM_CLOCK_TOWN");
    if (p.src.empty()) {
        if (!p.dst.empty() && !p.alias)
            throw ShuffleError("Unbalanced pool: " + p.name);
        return std::nullopt;
    }
    if (p.dst.empty())
        throw ShuffleError("No destinations for pool: " + p.name);
    return p;
}
struct ResolvedPools {
    Names available;
    Overrides fixed; // the reserved game links
    Pools pools;
};
// The pools a world gives these settings, before any placement: no bindings, RNG or native
// support checks. Shuffler::makePools is this plus the support checks.
inline ResolvedPools ResolvePools(const Settings& s, const AreaExists& areaExists) {
    ResolvedPools result{ AvailableEntrances(s, areaExists), ReservedEntrances(s), {} };
    for (const auto& d : MakePools(s))
        if (auto pool = ResolvePool(d, result.available, result.fixed))
            result.pools.push_back(std::move(*pool));
    return result;
}

// Native graph adapters supply these predicates. Capability checks happen before
// random sampling. Area filtering matches upstream computeAllEntrances.
struct Bindings {
    AreaExists areaExists;
    std::function<bool(const Record&)> sourceSupported, destinationSupported;
    // Each candidate includes the default and prior overrides AND remaining pools.
    // Build a temporary graph, preserve source predicates, OR duplicate edges,
    // AND can_reset_time for MM destinations without no-sot, and assume
    // remaining candidates with the upstream cardinality optimization.
    std::function<bool(const Overrides&, const Pools&)> validateAssumed;
    // Pinned validateAgeTemple (entrances.ts:265-286): on the SAME changed world/remaining pools,
    // ASSUMED and SPAWN events kept, explore OOT SPAWN at both ages with STARTING items (never the
    // all items of validateAssumed); the starting age's spawn exit is false and the other a bare age
    // check. True when that age reaches Temple of Time. Only UnifiedBridge.h assigns it.
    std::function<bool(const Overrides&, const Pools&)> validateAgeTemple;
    // Supplied from the existing combined accessibility choice, not a new setting.
    bool logicEnabled = true;
    // Re-run all-item pathfinding and the existing native fill/solver contract.
    // This must apply the selected goal/accessibility policy and commit the native layout.
    std::function<bool(const Overrides&)> validateComplete;
    std::function<size_t(size_t)> chooseIndex;
};

class Shuffler {
    Settings settings;
    Bindings bindings;
    Names available;
    Overrides fixed;
    size_t backtracks = 0;
    bool anyActive = false;

    size_t choose(size_t count) {
        if (!count || !bindings.chooseIndex)
            throw std::invalid_argument("Missing entrance RNG");
        const size_t i = bindings.chooseIndex(count);
        if (i >= count)
            throw std::invalid_argument("Entrance RNG index out of bounds");
        return i;
    }
    std::string_view sample(const Names& values) {
        auto it = values.begin();
        std::advance(it, choose(values.size()));
        return *it;
    }
    const Record* coupledReverse(std::string_view name) const {
        return settings.erDecoupled ? nullptr : Reverse(*Find(name));
    }
    Pools makePools() const {
        Pools result;
        for (const auto& d : MakePools(settings)) {
            auto p = ResolvePool(d, available, fixed);
            if (!p)
                continue;
            for (const auto& name : p->src)
                if (!bindings.sourceSupported || !bindings.sourceSupported(*Find(name)))
                    throw std::invalid_argument("Unsupported native source: " + std::string(name));
            for (const auto& name : p->dst)
                if (!bindings.destinationSupported || !bindings.destinationSupported(*Find(name)))
                    throw std::invalid_argument("Unsupported native destination: " + std::string(name));
            result.push_back(std::move(*p));
        }
        return result;
    }
    bool place(Pools pools, const Overrides& overrides, Overrides& final) {
        if (pools.empty()) {
            final = overrides;
            return true;
        }
        const size_t poolIndex = choose(pools.size());
        const auto& pool = pools[poolIndex];
        const auto src = sample(pool.src);
        Names candidates;
        for (const auto dst : pool.dst)
            if (Assignable(*Find(src), *Find(dst), pool.ownGame, settings))
                candidates.insert(dst);
        while (!candidates.empty()) {
            const auto dst = sample(candidates);
            candidates.erase(dst);
            const auto* revSrc = coupledReverse(src);
            const auto* revDst = coupledReverse(dst);
            auto nextOverrides = overrides;
            nextOverrides[src] = dst;
            if (revSrc && revDst)
                nextOverrides[revDst->name] = revSrc->name;
            auto nextPools = pools;
            auto& nextPool = nextPools[poolIndex];
            nextPool.src.erase(src);
            nextPool.dst.erase(dst);
            if (revSrc && revDst) {
                nextPool.src.erase(revDst->name);
                nextPool.dst.erase(revSrc->name);
            }
            auto completeOverrides = fixed;
            completeOverrides.insert(nextOverrides.begin(), nextOverrides.end());
            if (bindings.logicEnabled) {
                if (!bindings.validateAssumed(completeOverrides, nextPools))
                    continue;
                if (settings.games != Games::Mm && !bindings.validateAgeTemple(completeOverrides, nextPools))
                    continue;
            }
            if (nextPool.src.empty()) {
                if (!nextPool.dst.empty() && !pool.alias)
                    throw ShuffleError("Unbalanced pool: " + pool.name);
                nextPools.erase(nextPools.begin() + poolIndex);
            }
            if (place(std::move(nextPools), nextOverrides, final))
                return true;
            if (++backtracks >= 10)
                throw ShuffleError("Too many entrance backtracks");
        }
        return false;
    }

  public:
    Shuffler(Settings s, Bindings b) : settings(s), bindings(std::move(b)) {
        anyActive = !MakePools(settings).empty();
        // Disabled path does not validate bindings, consume RNG, or create layout.
        if (!anyActive)
            return;
        if (!bindings.areaExists || !bindings.validateComplete || (bindings.logicEnabled && !bindings.validateAssumed))
            throw std::invalid_argument("Entrance graph validation bindings are required");
        if (bindings.logicEnabled && settings.games != Games::Mm && !bindings.validateAgeTemple)
            throw std::invalid_argument("OoT entrance validation requires the starting-item opposite-age Temple probe");
        available = AvailableEntrances(settings, bindings.areaExists);
        fixed = ReservedEntrances(settings);
    }
    Overrides run() {
        if (!anyActive)
            return {};
        const auto pools = makePools(); // capabilities checked once before RNG/retries
        for (size_t attempt = 0; attempt < 1000; ++attempt) {
            backtracks = 0;
            try {
                Overrides overrides;
                if (!place(pools, {}, overrides))
                    throw ShuffleError("Unable to place entrance pools");
                auto result = fixed;
                result.insert(overrides.begin(), overrides.end());
                if (!bindings.validateComplete(result))
                    throw ShuffleError("Complete entrance world validation failed");
                return result;
            } catch (const ShuffleError&) {
                if (attempt == 999)
                    throw;
            }
        }
        throw ShuffleError("Unreachable entrance retry state");
    }
};
// The MM-only reservation above loops the Clock Tower door back to Clock Town. In ComboShip that
// door is the native MM->OoT link, so the native MM layouts never store or model this row.
inline bool IsMmGrottoClockTowerReservation(std::string_view src, std::string_view dst) {
    return src == "MM_CLOCK_TOWER_FROM_CLOCK_TOWN" && dst == "MM_CLOCK_TOWN_FROM_CLOCK_TOWER";
}
} // namespace ComboRando::Entrances
