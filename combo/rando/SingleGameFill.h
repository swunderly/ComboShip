#pragma once
#include "CrossWorldRando.h"
#include "GameMode.h"
#include "ComboSeedFormat.h"

namespace ComboRando {
inline OracleFns InactiveGameOracle() {
    return { +[] {}, +[](const char*) {}, +[]() -> const char* { return "[]"; },
             +[](const char*, const char*) {}, +[]() -> uint8_t { return 1; } };
}
inline OracleFns SingleGameOracle(OracleFns native) {
    // No portal prerequisite, foreign songs, bean renewal or cross-age entrances. The selected
    // world's native inventory/search/place callbacks still determine its complete fixed point.
    return { native.Reset, native.SetOwnedItems, native.GetReachableChecks, native.PlaceItem,
             +[]() -> uint8_t { return 1; } };
}
inline CwGoal SingleGameGoal(ComboGameMode mode) {
    CwGoal goal;
    goal.type = ModeStartsMm(mode) ? CwGoal::MAJORA : CwGoal::GANON;
    goal.total = 0;
    return goal;
}
// Vanilla preparation is metadata only: no native pool, placements, randomizer inventory or RNG.
inline nlohmann::json PrepareVanillaRun(nlohmann::json setup, const nlohmann::json& rules,
                                       const std::string& inputSeed, uint32_t masterSeed, uint32_t displaySeed,
                                       int slot) {
    const auto mode = ReadGameMode(setup);
    if (!VanillaMode(mode) || slot < 0 || slot > 2)
        throw std::invalid_argument("Vanilla preparation needs a vanilla game and a valid file");
    auto seed = setup;
    seed["seed"] = inputSeed;
    seed["masterSeed"] = masterSeed;
    seed["displaySeed"] = displaySeed;
    std::string digits = std::to_string(displaySeed);
    digits.insert(0, 10 - digits.size(), '0');
    seed["file_hash"] = nlohmann::json::array();
    for (int i = 0; i < 5; ++i)
        seed["file_hash"].push_back(std::stoi(digits.substr(i * 2, 2)));
    seed["goal"] = CwGoalJson(SingleGameGoal(mode));
    seed["startingGame"] = ModeStartsMm(mode) ? "MM" : "OOT";
    seed["foreign"] = seed["sharedItems"] = seed["playthrough"] = nlohmann::json::array();
    seed["hints"] = nlohmann::json::object();
    seed["songTravel"] = SongTravelMap{}.json();
    for (const auto* key : {"faroreRules", "spellRules", "gearRules"}) seed.erase(key);
    seed["gameplayRules"] = rules;
    for (const char* game : { "oot", "mm" }) {
        seed[game]["placements"] = nlohmann::json::object();
        seed[game]["prices"] = nlohmann::json::object();
        seed[game].erase("resolvedSeedRules");
    }
    seed["comboProfile"] = { { "version", 1 }, { "file", slot }, { "settings", setup } };
    StampComboSeedFormat(seed);
    return seed;
}
} // namespace ComboRando
