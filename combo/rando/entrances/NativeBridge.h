// Shared launcher/headless adapter for native entrance layouts and the frozen private MM profiles.
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
    // The unified generation's er* settings document, or null to clear it (MmUnifiedLayout.h).
    int (*setGenerationSettings)(const char*) = nullptr;
    int (*applyOotLayout)(const char*) = nullptr;
    int (*setOotAssumptions)(const char*) = nullptr;
    void (*clearOotAssumptions)() = nullptr;
    const char* (*dumpOotLayout)() = nullptr;
    const char* (*ootAreaContext)() = nullptr;
};
class NativeLayoutScope {
    NativeFns native;
    std::string previous, previousOot;

  public:
    explicit NativeLayoutScope(NativeFns f) : native(f) {
        if (native.applySeed && !native.dumpSeed)
            throw std::runtime_error("MM entrance snapshot export unavailable");
        if (bool(native.applyOotLayout) != bool(native.dumpOotLayout))
            throw std::runtime_error("OoT entrance snapshot exports incomplete");
        if (native.dumpSeed) {
            const auto* value = native.dumpSeed();
            if (!value)
                throw std::runtime_error("MM entrance snapshot failed");
            previous = value;
            const auto snapshot = nlohmann::json::parse(previous);
            DecodeMmWallmasterSeed(snapshot);
            if (snapshot.contains("entranceLayout"))
                DecodeEntranceLayout(snapshot.at("entranceLayout"));
        }
        if (native.dumpOotLayout) {
            const auto* value = native.dumpOotLayout();
            if (!value)
                throw std::runtime_error("OoT entrance snapshot failed");
            previousOot = value;
            const auto snapshot = nlohmann::json::parse(previousOot);
            if (!snapshot.is_object())
                throw std::runtime_error("OoT entrance snapshot malformed");
            if (snapshot.contains("entranceLayout"))
                DecodeEntranceLayout(snapshot.at("entranceLayout"));
        }
    }
    ~NativeLayoutScope() {
        if (native.clearAssumptions)
            native.clearAssumptions();
        if (native.clearOotAssumptions)
            native.clearOotAssumptions();
        if (native.applySeed && !previous.empty())
            native.applySeed(previous.c_str());
        if (native.applyOotLayout && !previousOot.empty())
            native.applyOotLayout(previousOot.c_str());
    }
    void Commit() {
        previous.clear();
        previousOot.clear();
    }
    NativeLayoutScope(const NativeLayoutScope&) = delete;
    NativeLayoutScope& operator=(const NativeLayoutScope&) = delete;
};
// Install both halves transactionally and require the complete envelope to roundtrip unchanged.
inline void ApplyNativeLayout(const NativeFns& native, const nlohmann::json& seed, const CapabilitySet& caps) {
    const auto& value = seed.at("entranceLayout");
    const auto validated = Validate(DecodeEntranceLayout(value), caps);
    if (!validated.oot.empty() && (!native.applyOotLayout || !native.dumpOotLayout))
        throw std::runtime_error("OoT entrance layout installer unavailable");
    if (!native.applySeed || !native.dumpSeed)
        throw std::runtime_error("MM entrance layout exports unavailable");
    NativeLayoutScope restore(native);
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
    if (native.applyOotLayout) {
        if (!native.applyOotLayout(seed.dump().c_str()))
            throw std::runtime_error("OoT entrance layout rejected");
        const auto* ootText = native.dumpOotLayout();
        if (!ootText)
            throw std::runtime_error("OoT entrance layout roundtrip failed");
        const auto ootLoaded = nlohmann::json::parse(ootText);
        for (const char* key : kLegacyEntranceKeys)
            if (ootLoaded.contains(key))
                throw std::runtime_error("OoT entrance layout installed beside a private MM profile");
        if (!ootLoaded.contains("entranceLayout") || ootLoaded.at("entranceLayout") != value)
            throw std::runtime_error("OoT entrance layout ignored or changed");
    }
    restore.Commit();
}
inline void ApplyNativeSeed(const NativeFns& native, const nlohmann::json& seed, const CapabilitySet& caps) {
    RequireSupportedSeedCapabilities(seed, caps);
    if (seed.contains("entranceLayout")) {
        ApplyNativeLayout(native, seed, caps);
        return;
    }
    if (native.applyOotLayout) {
        // A normal/legacy MM seed also clears the previous slot's OoT overlay, as one transaction.
        NativeLayoutScope restore(native);
        if (!native.applyOotLayout("{}"))
            throw std::runtime_error("OoT entrance layout clear rejected");
        const auto* clearedOot = native.dumpOotLayout();
        if (!clearedOot || nlohmann::json::parse(clearedOot) != nlohmann::json::object())
            throw std::runtime_error("OoT entrance layout clear ignored or changed");
        NativeFns mmOnly = native;
        mmOnly.applyOotLayout = nullptr;
        mmOnly.setOotAssumptions = nullptr;
        mmOnly.clearOotAssumptions = nullptr;
        mmOnly.dumpOotLayout = nullptr;
        mmOnly.ootAreaContext = nullptr;
        ApplyNativeSeed(mmOnly, seed, caps);
        restore.Commit();
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
using NativeScope = NativeLayoutScope;
class AssumptionScope {
    NativeFns native;

  public:
    AssumptionScope(NativeFns f, const Overrides& overrides, const Pools& pools) : native(f) {
        if (!native.setAssumptions || !native.clearAssumptions)
            throw std::runtime_error("MM entrance assumption exports unavailable");
        if (native.clearOotAssumptions)
            native.clearOotAssumptions();
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
    AssumptionScope(NativeFns f, const std::string& mmOverlay, const std::string& ootOverlay) : native(f) {
        const auto oot = nlohmann::json::parse(ootOverlay);
        const bool needsOot = !oot.at("deleted").empty() || !oot.at("fixed").empty() || !oot.at("remaining").empty();
        if (!native.setAssumptions || !native.clearAssumptions ||
            (needsOot && (!native.setOotAssumptions || !native.clearOotAssumptions)) ||
            (bool(native.setOotAssumptions) != bool(native.clearOotAssumptions)))
            throw std::runtime_error("Entrance assumption exports unavailable");
        try {
            if (native.setOotAssumptions && !native.setOotAssumptions(ootOverlay.c_str()))
                throw std::runtime_error("OoT entrance assumption rejected");
            if (!native.setAssumptions(mmOverlay.c_str()))
                throw std::runtime_error("MM entrance assumption rejected");
        } catch (...) {
            native.clearAssumptions();
            if (native.clearOotAssumptions)
                native.clearOotAssumptions();
            throw;
        }
    }
    ~AssumptionScope() {
        native.clearAssumptions();
        if (native.clearOotAssumptions)
            native.clearOotAssumptions();
    }
    AssumptionScope(const AssumptionScope&) = delete;
    AssumptionScope& operator=(const AssumptionScope&) = delete;
};
// The er* settings of one unified generation, for the MM logic's er_enabled while its candidate worlds
// are checked (macros_mm.yml:305), cleared on every exit. A settings document of the layout being made
// (EntranceLayout.h, LayoutPart::SettingsOnly), which the MM module checks against its own capabilities.
class GenerationSettingsScope {
    NativeFns native;

  public:
    GenerationSettingsScope(NativeFns f, const std::string& settings) : native(f) {
        if (!native.setGenerationSettings)
            throw std::runtime_error("MM entrance generation settings export unavailable");
        if (!native.setGenerationSettings(settings.c_str())) {
            native.setGenerationSettings(nullptr);
            throw std::runtime_error("MM entrance generation settings rejected");
        }
    }
    ~GenerationSettingsScope() {
        native.setGenerationSettings(nullptr);
    }
    GenerationSettingsScope(const GenerationSettingsScope&) = delete;
    GenerationSettingsScope& operator=(const GenerationSettingsScope&) = delete;
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
