#pragma once
#include "../ComboGameMode.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace ComboRando {
inline constexpr const char* kGameModeKeys[] = { "combined", "oot-randomizer", "mm-randomizer", "oot-vanilla",
                                                "mm-vanilla" };
inline constexpr const char* kGameModeLabels[] = { "Combined randomizer", "OoT randomizer", "MM randomizer",
                                                  "Vanilla Ocarina of Time", "Vanilla Majora's Mask" };
inline ComboGameMode ReadGameMode(const nlohmann::json& document) {
    if (!document.is_object() || !document.contains("gameMode"))
        return COMBO_MODE_COMBINED; // All pre-feature saves keep precisely their existing behavior.
    const auto& mode = document.at("gameMode");
    if (mode.is_string())
        for (int i = 0; i < 5; ++i)
            if (mode == kGameModeKeys[i])
                return static_cast<ComboGameMode>(i);
    throw std::invalid_argument("Unknown game mode; this file needs a compatible ComboShip build");
}
inline bool CombinedMode(ComboGameMode mode) { return mode == COMBO_MODE_COMBINED; }
inline bool VanillaMode(ComboGameMode mode) { return mode == COMBO_MODE_OOT_VANILLA || mode == COMBO_MODE_MM_VANILLA; }
inline bool ModeStartsMm(ComboGameMode mode) { return mode == COMBO_MODE_MM_RANDO || mode == COMBO_MODE_MM_VANILLA; }
inline bool ModeRandomizesOot(ComboGameMode mode) { return mode == COMBO_MODE_COMBINED || mode == COMBO_MODE_OOT_RANDO; }
inline bool ModeRandomizesMm(ComboGameMode mode) { return mode == COMBO_MODE_COMBINED || mode == COMBO_MODE_MM_RANDO; }
// Inactive world: no checks, inventory, prices, options or native preparation. This is an empty
// participant in the shared fill, not the other game's vanilla items mixed into a solo seed.
inline nlohmann::json InactiveGameDump() {
    return { { "checks", nlohmann::json::array() }, { "pool", nlohmann::json::array() },
             { "fixed", nlohmann::json::array() }, { "items", nlohmann::json::array() },
             { "prices", nlohmann::json::object() } };
}
inline void ValidateGameModeSeed(const nlohmann::json& seed, bool settingsDocument) {
    const auto mode = ReadGameMode(seed);
    if (CombinedMode(mode))
        return;
    // Single-game layouts require a separately audited logic/runtime adapter.
    for (const auto* field : { "entranceLayout", "entranceExpansion", "ordinaryEntranceExpansion",
                                "grottoEntranceExpansion", "mixedEntranceExpansion" })
        if (seed.contains(field))
            throw std::invalid_argument("Unified entrance shuffle is available in combined mode only");
    if (settingsDocument)
        return;
    if (seed.value("startingGame", std::string()) != (ModeStartsMm(mode) ? "MM" : "OOT"))
        throw std::invalid_argument("Game mode and starting game disagree");
    const auto goal = seed.value("goal", nlohmann::json::object());
    if (!goal.is_object() || goal.value("type", std::string()) != (ModeStartsMm(mode) ? "majora" : "ganon"))
        throw std::invalid_argument("A single-game run must finish its own final boss");
    for (const auto* field : { "foreign", "sharedItems" })
        if (!seed.contains(field) || !seed[field].is_array() || !seed[field].empty())
            throw std::invalid_argument("A single-game run cannot contain cross-game items");
    for (const auto* game : { "oot", "mm" }) {
        const bool randomized = std::string(game) == "oot" ? ModeRandomizesOot(mode) : ModeRandomizesMm(mode);
        const auto& placements = seed.at(game).at("placements");
        if (!placements.is_object() || (!randomized && !placements.empty()))
            throw std::invalid_argument("Placements do not match the selected game mode");
    }
}
} // namespace ComboRando
