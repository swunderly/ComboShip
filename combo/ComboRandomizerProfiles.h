#pragma once
#include <nlohmann/json.hpp>
#include "rando/KokiriSeedRevision.h"
#include "rando/ComboSeedFormat.h"
#include "mq/MqSeedMarker.h"
#include "rando/SeedChoices.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <optional>

namespace ComboRando::Profiles {
using Json = nlohmann::json;
inline void RequireSlot(int slot) {
    if (slot < 0 || slot > 2)
        throw std::invalid_argument("Settings file must be File 1, 2 or 3");
}
// Preserve all present and future rule fields. Only derived generation output is
// removed; settings, tricks, inventories, shared/goal/access/clock/ER/age/warp
// rules and unknown extensions are carried without a historical field allowlist.
inline Json SettingsDocument(const Json& seed, const Entrances::CapabilitySet& caps) {
    if (!seed.is_object())
        throw std::invalid_argument("Randomizer settings document must be an object");
    RequireSupportedSettingsCapabilities(seed, caps);
    Json result = seed;
    for (const char* key :
         { "masterSeed", "displaySeed", "file_hash", "foreign", "playthrough", "hints", "comboProfile" })
        result.erase(key);
    for (const char* game : { "oot", "mm" })
        if (result.contains(game)) {
            if (!result[game].is_object())
                throw std::invalid_argument("Malformed game settings document");
            for (const char* output : { "placements", "prices", "iceTrapModels", "resolvedSeedRules",
                                        ComboMq::kMqDungeonsKey, ComboMq::kMqDungeonNamesKey, SeedChoices::kKey })
                result[game].erase(output);
            if (result[game].contains("settings") && !result[game]["settings"].is_object())
                throw std::invalid_argument("Malformed native settings snapshot");
        }
    // Retain every entrance profile/configuration extension; discard only the
    // actual randomized route rows. New generation must solve new placements.
    for (const char* key :
         { "entranceExpansion", "ordinaryEntranceExpansion", "grottoEntranceExpansion", "mixedEntranceExpansion" })
        if (result.contains(key) && result[key].is_object())
            result[key].erase("overrides");
    if (result.contains("entranceLayout") && result["entranceLayout"].is_object())
        result["entranceLayout"].erase("overrides");
    if (result.contains("entrances") && result["entrances"].is_object() && result["entrances"].contains("oot") &&
        result["entrances"]["oot"].is_array())
        result["entrances"].erase("oot");
    return result;
}
inline bool SeedDocument(const Json& value) {
    return IsComboSeedDocument(value);
}
inline void MergeSettings(Json& destination, const Json& source) {
    for (const auto& field : source.items()) {
        if (field.value().is_object() && destination.contains(field.key()) && destination[field.key()].is_object())
            MergeSettings(destination[field.key()], field.value());
        else
            destination[field.key()] = field.value();
    }
}
// New seeds record the actual selected setup before native forced/default
// transformations. Legacy seeds retain their complete effective settings.
// Preserve later result fields too, rather than replacing the seed with a
// historical requested-fields subset.
inline Json SetupForSeed(const Json& seed, const Entrances::CapabilitySet& caps) {
    RequireSupportedSeedCapabilities(seed, caps); // a seed's layout keeps its routes
    auto setup = SettingsDocument(seed, caps);
    const auto catalog = ReadKokiriCatalogRevisions(seed.value("oot", Json::object()));
    const auto mqSource = ComboMq::ReadMqDataSource(seed.value("oot", Json::object()));
    if (seed.contains("comboProfile")) {
        const auto& profile = seed["comboProfile"];
        if (!profile.is_object() || profile.value("version", 0) != 1 || !profile.contains("settings") ||
            !profile["settings"].is_object())
            throw std::invalid_argument("Malformed requested seed settings");
        MergeSettings(setup, SettingsDocument(profile["settings"], caps));
        // The requested settings are the file's draft, a plain settings document (fileType ComboShipRandomizer
        // version 1, its entrance settings in setupCVars: NormalizeEntranceDraft), so the merge gave the seed's layout
        // the plain file type. The file type and version follow the content, as StampComboSeedFormat makes them: a
        // layout needs its own (RequireSupportedLayout), or setDraft and restore refuse this setup (0.4.13 RC2
        // emulator finding).
        if (setup.contains("entranceLayout")) {
            setup["fileType"] = kEntranceSeedFileType;
            setup["version"] = kEntranceSeedVersion;
        }
    }
    // A requested settings profile can never reinterpret an existing seed's
    // reward catalog. Only a newly generated seed selects its new catalog.
    setup["oot"][kKokiriSeedRevision] = catalog.invisibleRupees;
    setup["oot"][kKokiriGrassCatalogRevision] = catalog.grass;
    // Likewise its Master Quest data source (combo/mq/MqSeedMarker.h); a Legacy seed's setup has none.
    ComboMq::WriteMqDataSource(setup["oot"], mqSource);
    // A draft, a generation input and a copy are made from this setup and checked again only when stored; an
    // import with no draft (migrateLegacyPending) stores it unchecked, and the next restore would then refuse the
    // whole store. Refuse a setup that is not a settings document here instead.
    RequireSupportedSettingsCapabilities(setup, caps);
    return setup;
}
struct Attempt {
    uint64_t token = 0, revision = 0;
    int slot = -1;
    Json setup;
    std::string seed;
};
class Store {
    Json document;
    Entrances::CapabilitySet caps;
    std::optional<Attempt> running;
    uint64_t nextToken = 1;
    static Json EmptyRecord() {
        return { { "revision", 0 },
                 { "draft", nullptr },
                 { "generated", nullptr },
                 { "generatedInput", nullptr },
                 { "lastAttempt", nullptr } };
    }

  public:
    Store(Json legacy, Entrances::CapabilitySet capabilities) : caps(capabilities) {
        const auto settings = SettingsDocument(legacy, caps);
        document = { { "schemaVersion", 1 },
                     { "legacySetup", settings },
                     { "legacyPendingMigrationComplete", false },
                     { "files", Json::array() } };
        for (int i = 0; i < 3; ++i)
            document["files"].push_back(EmptyRecord());
    }
    const Json& json() const {
        return document;
    }
    bool busy() const {
        return running.has_value();
    }
    const std::optional<Attempt>& attempt() const {
        return running;
    }
    void restore(const Json& saved) {
        if (!saved.is_object() || saved.value("schemaVersion", 0) != 1 || !saved.contains("files") ||
            !saved["files"].is_array() || saved["files"].size() != 3 || !saved.contains("legacySetup"))
            throw std::invalid_argument("Unsupported randomizer profile store");
        SettingsDocument(saved["legacySetup"], caps);
        for (const auto& file : saved["files"]) {
            if (!file.is_object() || !file.contains("revision") ||
                (!file["revision"].is_number_unsigned() && !file["revision"].is_number_integer()) ||
                (file["revision"].is_number_integer() && !file["revision"].is_number_unsigned() &&
                 file["revision"].get<int64_t>() < 0))
                throw std::invalid_argument("Malformed settings file revision");
            for (const char* key : { "draft", "generated", "generatedInput", "lastAttempt" })
                if (!file.contains(key))
                    throw std::invalid_argument("Incomplete settings file record");
            if (!file["draft"].is_null())
                SettingsDocument(file["draft"], caps);
            if (!file["generatedInput"].is_null())
                SettingsDocument(file["generatedInput"], caps);
            if (!file["generated"].is_null() && !SeedDocument(file["generated"]))
                throw std::invalid_argument("Malformed generated seed snapshot");
            RequireSupportedSeedCapabilities(file["generated"], caps);
            if (!file["lastAttempt"].is_null() && !file["lastAttempt"].is_object())
                throw std::invalid_argument("Malformed generation status");
        }
        if (running)
            throw std::logic_error("Cannot restore profile store during generation");
        if (saved.contains("legacyPendingMigrationComplete") && !saved["legacyPendingMigrationComplete"].is_boolean())
            throw std::invalid_argument("Malformed pending-seed migration status");
        document = saved;
    }
    Json draft(int slot, const Json& savedSeed = Json()) const {
        RequireSlot(slot);
        const auto& value = document["files"][slot]["draft"];
        if (!value.is_null())
            return value;
        if (SeedDocument(savedSeed))
            return SetupForSeed(savedSeed, caps);
        return document["legacySetup"];
    }
    Json inspect(int slot, const Json& savedSeed = Json()) const {
        RequireSlot(slot);
        const auto& file = document["files"][slot];
        return { { "file", slot },
                 { "savedSeed", SeedDocument(savedSeed) ? savedSeed : Json() },
                 { "savedSettings", SeedDocument(savedSeed) ? SettingsDocument(savedSeed, caps) : Json() },
                 { "draft", draft(slot, savedSeed) },
                 { "generatedSeed", file["generated"] },
                 { "generatedSettings",
                   file["generated"].is_null() ? Json() : SettingsDocument(file["generated"], caps) },
                 { "generatedInput", file["generatedInput"] },
                 { "revision", file["revision"] },
                 { "lastAttempt", file["lastAttempt"] } };
    }
    void setDraft(int slot, const Json& input) {
        RequireSlot(slot);
        auto next = SettingsDocument(input, caps);
        if (document["files"][slot]["revision"].get<uint64_t>() == UINT64_MAX)
            throw std::overflow_error("Settings file revision exhausted");
        auto copy = document;
        auto& file = copy["files"][slot];
        file["draft"] = std::move(next);
        file["revision"] = file["revision"].get<uint64_t>() + 1;
        document = std::move(copy);
    }
    void copySaved(int source, int destination, const Json& sourceSeed) {
        RequireSlot(source);
        RequireSlot(destination);
        if (!SeedDocument(sourceSeed))
            throw std::invalid_argument("Source file has no saved randomizer seed");
        setDraft(destination, SetupForSeed(sourceSeed, caps));
    }
    Attempt begin(int slot, const Json& savedSeed, const std::string& seed) {
        RequireSlot(slot);
        if (running)
            throw std::logic_error("Randomizer generation already running");
        Attempt next{ nextToken++, document["files"][slot]["revision"].get<uint64_t>(), slot, draft(slot, savedSeed),
                      seed };
        next.setup["seed"] = seed;
        running = next;
        return next;
    }
    void finish(uint64_t token, bool success, const Json& seed = Json(), const std::string& error = "") {
        if (!running || running->token != token)
            throw std::invalid_argument("Generation transaction does not match settings file");
        if (success && !SeedDocument(seed))
            throw std::invalid_argument("Generation did not produce a complete seed document");
        if (success)
            RequireSupportedSeedCapabilities(seed, caps);
        auto copy = document;
        auto& file = copy["files"][running->slot];
        if (success) {
            file["generated"] = seed;
            file["generatedInput"] = running->setup;
        }
        file["lastAttempt"] = {
            { "success", success }, { "revision", running->revision }, { "seed", running->seed }, { "error", error }
        };
        document = std::move(copy);
        running.reset();
    }
    Json pendingSeed(int slot) const {
        RequireSlot(slot);
        return document["files"][slot]["generated"];
    }
    // Readiness without copying the slot's whole seed. Every record has "generated" (EmptyRecord, restore).
    bool hasPendingSeed(int slot) const {
        RequireSlot(slot);
        return SeedDocument(document["files"][slot]["generated"]);
    }
    void importSeed(int slot, const Json& seed, bool copyToDraft) {
        RequireSlot(slot);
        if (running)
            throw std::logic_error("Cannot import a seed during generation");
        if (!SeedDocument(seed))
            throw std::invalid_argument("Imported document is not a complete seed");
        const auto setup = SetupForSeed(seed, caps);
        auto next = *this;
        if (copyToDraft)
            next.setDraft(slot, setup);
        auto& file = next.document["files"][slot];
        file["generated"] = seed;
        file["generatedInput"] = setup;
        file["lastAttempt"] = { { "success", true },
                                { "origin", "import" },
                                { "revision", file["revision"] },
                                { "seed", seed.value("seed", std::string()) },
                                { "error", "" } };
        document = std::move(next.document);
    }
    void migrateLegacyPending(const Json& seed) {
        if (running)
            throw std::logic_error("Cannot migrate pending seed during generation");
        auto next = *this;
        if (!seed.is_null()) {
            if (!SeedDocument(seed))
                throw std::invalid_argument("Malformed legacy pending seed");
            for (int slot = 0; slot < 3; ++slot)
                if (next.pendingSeed(slot).is_null())
                    next.importSeed(slot, seed, false);
        }
        next.document["legacyPendingMigrationComplete"] = true;
        document = std::move(next.document);
    }
};
} // namespace ComboRando::Profiles
