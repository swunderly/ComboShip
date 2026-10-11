#pragma once
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
};

// Recognition is separate from capability validation: file drop must route a future combo seed
// to the combo error path, never let a native single-game reader mistake it for its own spoiler.
inline bool IsComboSeedDocument(const nlohmann::json& seed) {
    if (!seed.is_object())
        return false;
    const auto type = seed.find("fileType");
    return type != seed.end() && type->is_string() && (*type == kComboSeedFileType || *type == kEntranceSeedFileType);
}
inline void RequireSupportedSeedCapabilities(const nlohmann::json& seed) {
    if (!seed.is_object())
        return;
    const auto type = seed.find("fileType");
    // Presence, including null/empty/malformed data, cannot silently select vanilla routing.
    // Remove this rejection only when the unified layout validator AND installer are ready.
    if (seed.contains("entranceLayout") || (type != seed.end() && *type == kEntranceSeedFileType))
        throw UnsupportedComboSeedFormat();
}
inline void RequireSupportedSeedDocument(const nlohmann::json& seed) {
    RequireSupportedSeedCapabilities(seed);
    if (!IsComboSeedDocument(seed))
        throw std::invalid_argument("Not a ComboShip seed document");
    // Legacy headless writers omitted version; historical readers did not validate it. Keep that
    // contract for no-layout seeds. Future layout schema versions must be checked by their installer.
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
