#pragma once
#include "entrances/EntranceLayout.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace ComboRando {
inline constexpr const char* kComboSeedFileType = "ComboShipRandomizer";
// Older readers only check fileType and ignore version. A seed requiring the future unified
// entrance installer must therefore use a different type, even when its placements look familiar.
inline constexpr const char* kEntranceSeedFileType = "ComboShipRandomizerEntranceLayout";
inline constexpr int kEntranceSeedVersion = 2;
inline constexpr const char* kComboSeedPreflightError = "__comboSeedFormatError";

class UnsupportedComboSeedFormat : public std::invalid_argument {
  public:
    UnsupportedComboSeedFormat()
        : std::invalid_argument("This seed requires entrance randomization support unavailable in this build") {
    }
    explicit UnsupportedComboSeedFormat(const std::string& reason)
        : std::invalid_argument("This seed's entrance layout cannot be used by this build: " + reason) {
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
    return type != seed.end() && type->is_string() && (*type == kComboSeedFileType || *type == kEntranceSeedFileType);
}
// A seed's entrance layout against the capabilities of this build. `settingsDocument` also
// accepts a profile settings document, whose layout has no routes (ComboRandomizerProfiles.h).
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
            else
                Entrances::Validate(Entrances::DecodeEntranceLayout(value), caps);
        } catch (const std::exception& error) { throw UnsupportedComboSeedFormat(error.what()); }
    }
}
inline void RequireSupportedSeedCapabilities(const nlohmann::json& seed, const Entrances::CapabilitySet& caps) {
    RequireSupportedLayout(seed, caps, false);
}
inline void RequireSupportedSettingsCapabilities(const nlohmann::json& document, const Entrances::CapabilitySet& caps) {
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
    if (seed.contains("entranceLayout")) {
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
} // namespace ComboRando
