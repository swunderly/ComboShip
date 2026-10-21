#pragma once
#include "GameMode.h"
#include "SongTravel.h"
#include "entrances/EntranceAvailability.h"
#include "entrances/EntranceLayout.h"
#include "entrances/EntranceRuntimeFlags.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace ComboRando {
inline constexpr const char* kComboSeedFileType = "ComboShipRandomizer";
inline constexpr const char* kGameModeSeedFileType = "ComboShipRandomizerGameMode";
// Older readers only check fileType and ignore version. A seed requiring the future unified
// entrance installer must therefore use a different type, even when its placements look familiar.
inline constexpr const char* kEntranceSeedFileType = "ComboShipRandomizerEntranceLayout";
inline constexpr int kEntranceSeedVersion = 2;
inline constexpr const char* kComboSeedPreflightError = "__comboSeedFormatError";

class UnsupportedComboSeedFormat : public std::invalid_argument {
  public:
    UnsupportedComboSeedFormat()
        : std::invalid_argument("This seed requires randomizer support unavailable in this build") {
    }
    explicit UnsupportedComboSeedFormat(const std::string& reason)
        : std::invalid_argument("This seed cannot be used by this build: " + reason) {
    }
};
// The private MM entrance profiles. A seed carries them or an entranceLayout, never both.
inline constexpr const char* kLegacyEntranceKeys[] = { "entranceExpansion", "ordinaryEntranceExpansion",
                                                       "grottoEntranceExpansion", "mixedEntranceExpansion" };

// Recognition is separate from capability validation: file drop must route a future combo seed
// to the combo error path, never let a native single-game reader mistake it for its own spoiler.
inline bool IsComboSeedDocument(const nlohmann::json& seed) {
    if (!seed.is_object())
        return false;
    const auto type = seed.find("fileType");
    return type != seed.end() && type->is_string() && (*type == kComboSeedFileType || *type == kEntranceSeedFileType || *type == kGameModeSeedFileType);
}
// The song facts of a seed, which its layout's capability id must carry (limits design §3): whether OoT
// logic is on (the seed's accessibility is not No Logic, so a Temple pass ran), the song travel map's logic
// flags, and the starting warp songs and Song of Soaring of both games' settings, read as the profile reads
// them (EntranceProfile.h ProfileEntranceFacts): MM's by 2Ship's spoiler names, as the seed stores them
// (MmStartingItemName). No shared item family is a song (SharedItems.h). A missing
// part counts as off, and so does a song travel map that does not parse: such a seed fails to load anyway
// (ComboShip.cpp InstallSongTravel), and with no song facts the songs ids are refused.
inline Entrances::EntranceSettingsFacts SeedEntranceFacts(const nlohmann::json& seed) {
    Entrances::EntranceSettingsFacts f;
    const auto part = [&](const nlohmann::json& object, const char* key) -> const nlohmann::json* {
        const auto it = object.find(key);
        return it != object.end() && it->is_object() ? &*it : nullptr;
    };
    if (const auto* access = part(seed, "accessibility")) {
        const auto mode = access->find("mode");
        f.logicEnabled = !(mode != access->end() && mode->is_string() && *mode == "noLogic");
    }
    SongTravelMap songs;
    try {
        songs = SongTravelMap::parse(seed.value("songTravel", nlohmann::json()));
    } catch (const std::exception&) {}
    f.songTravelLogic = songs.logicEnabled;
    f.foreignSongsLogic = songs.logicForeignSongs;
    const auto* oot = part(seed, "oot");
    if (const auto* settings = oot ? part(*oot, "settings") : nullptr)
        for (const auto& [key, name] : Entrances::kOotStartingSongSettings) {
            const auto value = settings->find(std::string("gRandoSettings.") + key);
            if (value != settings->end() && value->is_number_integer() && *value != 0)
                f.ootStartingItems.emplace_back(name);
        }
    const auto* mm = part(seed, "mm");
    if (const auto* settings = mm ? part(*mm, "settings") : nullptr)
        if (const auto items = settings->find("gRando.StartingItems"); items != settings->end() && items->is_array())
            for (const auto& item : *items)
                if (item.is_string())
                    f.mmStartingItems.push_back(Entrances::MmStartingItemName(item.get<std::string>()));
    return f;
}
// A layout's capability ids against the seed's song facts. Under logic, no id takes song destination shuffle
// or a starting Song of Soaring (the Temple gate's refusals, EntranceAvailability.h); only the songs-v2 id
// takes a starting OoT warp song, and it needs one; only the songs ids take foreign-song logic, and the songs
// id needs it; as an id must be needed (EntranceLayout.h ValidateEntranceLayoutSettings). With No Logic a seed
// has no song facts and keeps the vanilla or quests id.
inline void RequireSeedEntranceFacts(const Entrances::EntranceLayout& layout,
                                     const Entrances::EntranceSettingsFacts& f) {
    if (const auto why = Entrances::TempleSettingsRefusal(f); !why.empty())
        throw std::invalid_argument("Entrance layout seed: " + why.log);
    const auto has = [&](std::string_view id) {
        return std::find(layout.required.begin(), layout.required.end(), id) != layout.required.end();
    };
    const bool foreign = f.logicEnabled && f.foreignSongsLogic;
    const auto song = f.logicEnabled ? Entrances::StartingWarpSong(f) : std::string();
    const bool songs = has(Entrances::kSongsWallmasterCapability);
    const bool startingSongs = has(Entrances::kStartingSongsWallmasterCapability);
    if (!song.empty() && !startingSongs)
        throw std::invalid_argument("Entrance layout requires " + layout.required.front() +
                                    ", which does not admit a starting " + song);
    if (song.empty() && startingSongs)
        throw std::invalid_argument("Entrance layout requires " +
                                    std::string(Entrances::kStartingSongsWallmasterCapability) +
                                    " without a starting warp song");
    if (foreign && !songs && !startingSongs)
        throw std::invalid_argument("Entrance layout requires " + layout.required.front() +
                                    ", which does not admit foreign-song logic");
    if (!foreign && songs)
        throw std::invalid_argument("Entrance layout requires " + std::string(Entrances::kSongsWallmasterCapability) +
                                    " without foreign-song logic");
}
// A seed's entrance layout against the capabilities of this build. `settingsDocument` also
// accepts a profile settings document, whose layout has no routes (ComboRandomizerProfiles.h); it
// skips the routes and the song facts.
inline void RequireSupportedLayout(const nlohmann::json& seed, const Entrances::CapabilitySet& caps,
                                   bool settingsDocument) {
    if (!seed.is_object())
        return;
    const auto type = seed.find("fileType");
    // Presence, including null/empty/malformed data, cannot silently select vanilla routing.
    if (seed.contains("entranceLayout") || (type != seed.end() && *type == kEntranceSeedFileType)) {
        // A build with no active entrance capability rejects before decoding, as 0.4.3 and 0.4.4 do.
        if (caps.Empty())
            throw UnsupportedComboSeedFormat();
        try {
            const auto version = seed.find("version");
            if (type == seed.end() || *type != kEntranceSeedFileType || version == seed.end() ||
                !version->is_number_integer() || *version != kEntranceSeedVersion)
                throw std::invalid_argument("an entrance layout needs fileType " + std::string(kEntranceSeedFileType) +
                                            " version " + std::to_string(kEntranceSeedVersion));
            if (!seed.contains("entranceLayout"))
                throw std::invalid_argument("the seed has no entranceLayout");
            for (const char* key : kLegacyEntranceKeys)
                if (seed.contains(key))
                    throw std::invalid_argument(std::string("an entrance layout cannot be combined with ") + key);
            const auto& value = seed.at("entranceLayout");
            if (settingsDocument && value.is_object() && !value.contains("overrides"))
                Entrances::ValidateEntranceLayoutSettings(
                    Entrances::DecodeEntranceLayout(value, Entrances::LayoutPart::SettingsOnly), caps);
            else {
                const auto layout = Entrances::DecodeEntranceLayout(value);
                Entrances::Validate(layout, caps);
                if (!settingsDocument)
                    RequireSeedEntranceFacts(layout, SeedEntranceFacts(seed));
            }
        } catch (const std::exception& error) { throw UnsupportedComboSeedFormat(error.what()); }
    }
}
// Saved native-page preflight catches this typed error before loading/migrating a slot. A plain
// invalid_argument is intentionally handled differently by the legacy malformed-container path.
inline void RequireSupportedGameMode(const nlohmann::json& seed, bool settingsDocument) {
    try {
        ValidateGameModeSeed(seed, settingsDocument);
        if (!settingsDocument && seed.is_object()) {
            const bool modeFile = seed.value("fileType", std::string()) == kGameModeSeedFileType;
            if (modeFile && (!seed.contains("gameMode") || !seed.contains("version") ||
                             !seed["version"].is_number_integer() || seed["version"] != 1))
                throw std::invalid_argument("Unsupported game-mode seed version or missing mode");
            if (!CombinedMode(ReadGameMode(seed)) && !modeFile)
                throw std::invalid_argument("Single-game seed needs the game-mode file type");
        }
    } catch (const std::exception& error) {
        throw UnsupportedComboSeedFormat(std::string("Game mode: ") + error.what());
    }
}
inline void RequireSupportedSeedCapabilities(const nlohmann::json& seed, const Entrances::CapabilitySet& caps) {
    RequireSupportedGameMode(seed, false);
    RequireSupportedLayout(seed, caps, false);
}
inline void RequireSupportedSettingsCapabilities(const nlohmann::json& document, const Entrances::CapabilitySet& caps) {
    RequireSupportedGameMode(document, true);
    RequireSupportedLayout(document, caps, true);
}
inline void RequireSupportedSeedDocument(const nlohmann::json& seed, const Entrances::CapabilitySet& caps) {
    RequireSupportedSeedCapabilities(seed, caps);
    if (!IsComboSeedDocument(seed))
        throw std::invalid_argument("Not a ComboShip seed document");
    // Legacy headless writers omitted version; historical readers did not validate it. Keep that
    // contract for no-layout seeds. RequireSupportedLayout checks a layout seed's version.
}
inline void StampComboSeedFormat(nlohmann::json& seed) {
    if (!seed.is_object())
        throw std::invalid_argument("Seed document must be an object");
    if (!CombinedMode(ReadGameMode(seed))) {
        ValidateGameModeSeed(seed, false);
        seed["fileType"] = kGameModeSeedFileType;
        seed["version"] = 1;
    } else if (seed.contains("entranceLayout")) {
        seed["fileType"] = kEntranceSeedFileType;
        seed["version"] = kEntranceSeedVersion;
    } else {
        seed["fileType"] = kComboSeedFileType;
        // Preserve each existing writer's version field (including the headless writer's absence).
    }
}
inline void CheckComboSeedPreflight(const nlohmann::json& nativeSave) {
    if (nativeSave.is_object() && nativeSave.contains(kComboSeedPreflightError))
        throw UnsupportedComboSeedFormat();
}

// The ER-wide runtime flags (entrances/EntranceRuntimeFlags.h) of a seed whose layout this build already
// accepted; none without a layout. A layout that does not decode turns nothing on.
inline uint32_t SeedEntranceRuntimeFlags(const nlohmann::json& seed) {
    if (!seed.is_object() || !seed.contains("entranceLayout"))
        return 0;
    try {
        return Entrances::EntranceRuntimeFlags(Entrances::DecodeEntranceLayout(seed.at("entranceLayout")).settings);
    } catch (const std::exception&) { return 0; }
}
// The slot seed's runtime flags ride on the native OoT page the launcher hands to a save load, like
// kComboSeedPreflightError: the native loader must have them before Sram_StageOpenedSave picks the spawn,
// which runs before the OnLoadGame bind (M1 plan, critic correction C9). The key is not save data:
// taking it removes it, so the page applied is the page stored.
inline constexpr const char* kComboEntranceRuntimeFlags = "__comboEntranceRuntimeFlags";
inline uint32_t TakeComboEntranceRuntimeFlags(nlohmann::json& nativeSave) {
    if (!nativeSave.is_object())
        return 0;
    const auto flags = nativeSave.find(kComboEntranceRuntimeFlags);
    if (flags == nativeSave.end())
        return 0;
    const uint32_t value =
        flags->is_number_unsigned() ? flags->get<uint32_t>() & Entrances::kEntranceRuntimeFlagsKnown : 0u;
    nativeSave.erase(flags);
    return value;
}
} // namespace ComboRando
