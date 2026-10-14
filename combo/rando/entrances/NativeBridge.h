// Shared launcher/headless adapter for the bounded native MM Wallmaster profile.
#pragma once
#include "Format.h"
#include "rando/ComboSeedFormat.h"
#include "rando/entrances/MmGrottoLayout.h"
#include "rando/entrances/MmOrdinaryLayout.h"
#include "rando/entrances/MmMixedLayout.h"
#include "rando/CrossWorldRando.h"

namespace ComboRando::Entrances {
struct NativeFns {
    int (*applySeed)(const char*) = nullptr;
    int (*setAssumptions)(const char*) = nullptr;
    void (*clearAssumptions)() = nullptr;
    const char* (*dumpSeed)() = nullptr;
};
// A seed's unified layout, already accepted by the seed-format guard. The MM installer takes the MM
// routes and must give the same layout back. No OoT installer exists yet, so a layout with an OoT
// route is refused: it can never bind as vanilla routing.
inline void ApplyNativeLayout(const NativeFns& native, const nlohmann::json& seed, const CapabilitySet& caps) {
    const auto& value = seed.at("entranceLayout");
    if (!Validate(DecodeEntranceLayout(value), caps).oot.empty())
        throw std::runtime_error("OoT entrance layout installer unavailable");
    if (!native.applySeed || !native.dumpSeed)
        throw std::runtime_error("MM entrance layout exports unavailable");
    if (!native.applySeed(seed.dump().c_str()))
        throw std::runtime_error("MM entrance layout rejected");
    const auto* text = native.dumpSeed();
    if (!text)
        throw std::runtime_error("MM entrance layout roundtrip failed");
    const auto loaded = nlohmann::json::parse(text);
    for (const char* key : kLegacyEntranceKeys)
        if (loaded.contains(key))
            throw std::runtime_error(std::string("MM entrance layout installed beside ") + key);
    if (!loaded.contains("entranceLayout") || loaded.at("entranceLayout") != value)
        throw std::runtime_error("MM entrance layout ignored or changed");
}
inline void ApplyNativeSeed(const NativeFns& native, const nlohmann::json& seed, const CapabilitySet& caps) {
    RequireSupportedSeedCapabilities(seed, caps);
    if (seed.contains("entranceLayout")) {
        ApplyNativeLayout(native, seed, caps);
        return;
    }
    ValidateMmMixedSeed(seed);
    DecodeMmWallmasterSeed(seed); // reject unsupported data even with an older MM module
    if (seed.contains("ordinaryEntranceExpansion")) {
        MmOrdinaryLayout proof;
        proof.install(seed.at("ordinaryEntranceExpansion"));
    }
    if (seed.contains("grottoEntranceExpansion")) {
        MmGrottoLayout proof;
        proof.install(seed.at("grottoEntranceExpansion"));
    }
    if (!native.applySeed) {
        if (seed.contains("entranceExpansion") || seed.contains("ordinaryEntranceExpansion") ||
            seed.contains("grottoEntranceExpansion") || seed.contains("mixedEntranceExpansion"))
            throw std::runtime_error("MM entrance expansion export unavailable");
        return;
    }
    const bool expanded = seed.contains("entranceExpansion") || seed.contains("ordinaryEntranceExpansion") ||
                          seed.contains("grottoEntranceExpansion") || seed.contains("mixedEntranceExpansion");
    if (expanded && !native.dumpSeed)
        throw std::runtime_error("MM entrance expansion roundtrip export unavailable");
    if (!native.applySeed(seed.dump().c_str()))
        throw std::runtime_error("MM entrance expansion seed rejected");
    if (expanded) {
        const auto* text = native.dumpSeed();
        if (!text)
            throw std::runtime_error("MM entrance expansion roundtrip failed");
        const auto loaded = nlohmann::json::parse(text);
        for (const char* key :
             { "entranceExpansion", "ordinaryEntranceExpansion", "grottoEntranceExpansion", "mixedEntranceExpansion" })
            if (seed.contains(key) && (!loaded.contains(key) || loaded.at(key) != seed.at(key)))
                throw std::runtime_error(std::string("MM entrance expansion ignored or changed ") + key);
    }
}
class NativeScope {
    NativeFns native;
    std::string previous;

  public:
    explicit NativeScope(NativeFns f) : native(f) {
        if (native.applySeed && !native.dumpSeed)
            throw std::runtime_error("MM entrance snapshot export unavailable");
        if (native.dumpSeed) {
            const auto* value = native.dumpSeed();
            if (!value)
                throw std::runtime_error("MM entrance snapshot failed");
            previous = value;
            const auto snapshot = nlohmann::json::parse(previous);
            DecodeMmWallmasterSeed(snapshot);
            // An installed unified layout is restored from the same dump.
            if (snapshot.contains("entranceLayout"))
                DecodeEntranceLayout(snapshot.at("entranceLayout"));
        }
    }
    ~NativeScope() {
        if (native.clearAssumptions)
            native.clearAssumptions();
        if (native.applySeed && !previous.empty())
            native.applySeed(previous.c_str());
    }
    NativeScope(const NativeScope&) = delete;
    NativeScope& operator=(const NativeScope&) = delete;
};
class AssumptionScope {
    NativeFns native;

  public:
    AssumptionScope(NativeFns f, const Overrides& overrides, const Pools& pools) : native(f) {
        if (!native.setAssumptions || !native.clearAssumptions)
            throw std::runtime_error("MM entrance assumption exports unavailable");
        if (!native.setAssumptions(EncodeMmWallmasterAssumptions(overrides, pools).dump().c_str()))
            throw std::runtime_error("MM entrance assumption rejected");
    }
    // One unified candidate world's MM overlay (CandidateWorld.h EncodeMmOverlay).
    AssumptionScope(NativeFns f, const std::string& overlay) : native(f) {
        if (!native.setAssumptions || !native.clearAssumptions)
            throw std::runtime_error("MM entrance assumption exports unavailable");
        if (!native.setAssumptions(overlay.c_str()))
            throw std::runtime_error("MM entrance assumption rejected");
    }
    ~AssumptionScope() {
        native.clearAssumptions();
    }
    AssumptionScope(const AssumptionScope&) = delete;
    AssumptionScope& operator=(const AssumptionScope&) = delete;
};
inline std::vector<SavedOverride> MmWallmasterRows(const Overrides& overrides) {
    std::vector<SavedOverride> rows;
    for (const auto name : kMmWallmasterSources) {
        const auto it = overrides.find(name);
        if (it == overrides.end())
            throw std::runtime_error("Incomplete MM Wallmaster layout");
        const auto* src = Find(name);
        const auto* dst = Find(it->second);
        if (!src || !dst)
            throw std::runtime_error("Unknown MM Wallmaster route");
        rows.push_back({ std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id });
    }
    return rows;
}
struct AssumedWorld {
    std::vector<std::string> ootItems, mmItems;
    std::set<std::string> ootChecks, mmChecks;
    explicit AssumedWorld(const std::string& ootDump, const std::string& mmDump, const std::string& forcedOot) {
        auto collect = [](const std::string& dump, auto& items, auto& checks) {
            const auto data = nlohmann::json::parse(dump);
            if (data.contains("error") || !data.contains("pool") || !data.at("pool").is_array())
                throw std::runtime_error("Entrance validation requires a valid native item pool");
            for (const auto& row : data.at("pool"))
                items.push_back(row.at("name").template get<std::string>());
            for (const auto& row : data.value("checks", nlohmann::json::array()))
                checks.insert(row.at("name").template get<std::string>());
            for (const auto& row : data.value("fixed", nlohmann::json::array())) {
                items.push_back(row.at("item").template get<std::string>());
                checks.insert(row.at("check").template get<std::string>());
            }
            for (const auto& name : data.value("sharedStartingItems", std::vector<std::string>{}))
                items.push_back(name);
        };
        collect(ootDump, ootItems, ootChecks);
        collect(mmDump, mmItems, mmChecks);
        if (!forcedOot.empty()) {
            const auto forced = nlohmann::json::parse(forcedOot);
            for (const auto& [name, row] : forced.items()) {
                if (row.contains("item"))
                    ootItems.push_back(row.at("item").get<std::string>());
                ootChecks.insert(name);
            }
        }
    }
    bool validate(const OracleFns& oot, const OracleFns& mm, bool mmStart, const CwGoal& goal,
                  OotAccess access = OotAccess::ALL_REACHABLE) const {
        if (access == OotAccess::NO_LOGIC)
            return true;
        if (!oot.Reset || !oot.SetOwnedItems || !oot.GetReachableChecks || !oot.GetPortalOpen || !mm.Reset ||
            !mm.SetOwnedItems || !mm.GetReachableChecks)
            throw std::runtime_error("Entrance validation requires both native reachability oracles");
        auto query = [](const OracleFns& oracle, const std::vector<std::string>& owned) {
            CheckGenerationCancellation();
            oracle.Reset();
            oracle.SetOwnedItems(nlohmann::json(owned).dump().c_str());
            const auto* value = oracle.GetReachableChecks();
            if (!value)
                throw std::runtime_error("Native entrance oracle returned no checks");
            const auto data = nlohmann::json::parse(value);
            if (!data.is_array())
                throw std::runtime_error("Native entrance oracle returned malformed checks");
            std::unordered_set<std::string> result;
            for (const auto& name : data)
                result.insert(name.get<std::string>());
            return result;
        };
        bool mmOpen = mmStart;
        // An MM start roots MM from the start in the age-aware search too, as mmOpen does in the plain one.
        const auto reached = QuerySongWorlds(oot, mm, ootItems, mmItems, mmOpen, query, mmStart);
        if (access == OotAccess::ALL_REACHABLE) {
            for (const auto& name : ootChecks)
                if (!reached.first.contains(name))
                    return false;
            for (const auto& name : mmChecks)
                if (!reached.second.contains(name))
                    return false;
        }
        return CwGoalReached(goal, reached.first.contains("Ganon"), reached.second.contains("Defeat Majora"), ootItems,
                             mmItems);
    }
};
// The real combined fill still runs AFTER this assumed-inventory placement phase;
// it certifies the saved item progression under the original accessibility policy.
inline nlohmann::json GenerateNativeMmWallmasters(bool enabled, uint32_t seed, const NativeFns& native,
                                                  const std::string& ootDump, const std::string& mmDump,
                                                  const std::string& forcedOot, const OracleFns& oot,
                                                  const OracleFns& mm, bool mmStart, CwGoal goal,
                                                  OotAccess access = OotAccess::ALL_REACHABLE) {
    ApplyNativeSeed(native, nlohmann::json::object(), {});
    if (!enabled)
        return nlohmann::json(); // no pool construction or new RNG on legacy seeds
    if (!native.applySeed || !native.dumpSeed || !native.setAssumptions || !native.clearAssumptions)
        throw std::runtime_error("Native MM Wallmaster generation exports unavailable");
    AssumedWorld world(ootDump, mmDump, forcedOot);
    Settings settings;
    settings.games = Games::Mm;
    settings.erWallmasters = Mode::OwnGame;
    CwRng rng(CwHashName("entrances:mm-wallmasters-own-game-v1:" + std::to_string(seed)));
    Bindings bindings;
    bindings.logicEnabled = access != OotAccess::NO_LOGIC;
    bindings.areaExists = [](std::string_view area) {
        for (const auto& record : kRegistry)
            if ((Contains(kMmWallmasterSources, record.name) || MmBaseDestination(record, true)) &&
                (record.from == area || record.to == area))
                return true;
        return false;
    };
    bindings.sourceSupported = [](const Record& record) { return Contains(kMmWallmasterSources, record.name); };
    bindings.destinationSupported = [](const Record& record) { return MmBaseDestination(record, true); };
    bindings.chooseIndex = [&](size_t count) {
        CheckGenerationCancellation();
        return rng.below(static_cast<uint32_t>(count));
    };
    bindings.validateAssumed = [&](const Overrides& rows, const Pools& pools) {
        AssumptionScope scope(native, rows, pools);
        return world.validate(oot, mm, mmStart, goal, access);
    };
    bindings.validateComplete = [&](const Overrides& rows) {
        ApplyNativeSeed(native, { { "entranceExpansion", EncodeMmWallmasterExtension(MmWallmasterRows(rows)) } }, {});
        return world.validate(oot, mm, mmStart, goal, access);
    };
    const auto rows = Shuffler(settings, std::move(bindings)).run();
    return EncodeMmWallmasterExtension(MmWallmasterRows(rows));
}
} // namespace ComboRando::Entrances
