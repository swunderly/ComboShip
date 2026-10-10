#pragma once
#include "GameplayRules.h"
#include "SongTravel.h"
#include <utility>

namespace ComboRando {
// Build a complete runtime snapshot without consulting the active slot's CVars. The original
// seed remains immutable; malformed/missing entries cannot inherit another slot's abilities.
inline bool IsGameplayRuleBlock(const nlohmann::json& block) {
    return block.is_object() && block.contains("version") && block["version"].is_number_integer() &&
           (block["version"] == 1 || block["version"] == 2) && block.contains("values") && block["values"].is_object();
}
inline bool HasSavedRuleValue(const nlohmann::json& block, const GameplayRule& rule) {
    if (!IsGameplayRuleBlock(block))
        return false;
    const auto value = block["values"].find(rule.name);
    return value != block["values"].end() && value->is_number_integer() && *value >= rule.minimum &&
           *value <= rule.maximum;
}
inline nlohmann::json NormalizeGameplayRuleSnapshot(const nlohmann::json& input) {
    const auto block = UpgradeGameplayRules(input);
    auto result = CaptureGameplayRules([](const char*, int fallback) { return fallback; });
    result["ootTricks"] = "";
    if (!IsGameplayRuleBlock(block))
        return result;
    for (const auto& rule : kGameplayRules) {
        if (!HasSavedRuleValue(block, rule))
            continue;
        const int runtime = block["values"][rule.name].get<int>();
        result["values"][rule.name] = runtime;
        if (rule.runtimeOnly)
            continue; // preserve old quest counts without a generation assumption
        if (block["version"] == 1) {
            if (!rule.trickGoverned)
                result["logicValues"][rule.name] = runtime;
        } else if (block.contains("logicValues") && block["logicValues"].is_object()) {
            const auto logical = block["logicValues"].find(rule.name);
            if (logical != block["logicValues"].end() && logical->is_number_integer() && *logical >= 0 &&
                *logical <= rule.maximum)
                result["logicValues"][rule.name] = logical->get<int>();
        }
    }
    if (block.contains("mmTricks") && block["mmTricks"].is_object()) {
        for (const auto& trick : kMmTricks) {
            const auto value = block["mmTricks"].find(trick.key);
            if (trick.publicOption && value != block["mmTricks"].end() && value->is_number_integer() &&
                (*value == 0 || *value == 1))
                result["mmTricks"][trick.key] = value->get<int>();
        }
    }
    if (block.contains("ootTricks") && block["ootTricks"].is_string())
        result["ootTricks"] = block["ootTricks"];
    return result;
}

// baseline (optional): the player's own settings, captured before any slot restored its rules this
// session. A vanilla file, or a save from before rules were stored with the seed, adopts it once in
// place of the fallbacks, so loading it does not switch the player's enhancements back to their
// defaults. What the seed itself records (song map, arrow settings, trick list) still wins.
inline nlohmann::json GameplayRulesForSeed(const nlohmann::json& seed,
                                           const nlohmann::json& baseline = nlohmann::json()) {
    const auto saved = seed.value("gameplayRules", nlohmann::json());
    const bool captured = IsGameplayRuleBlock(saved);
    const bool adopt = !captured && IsGameplayRuleBlock(baseline);
    if (adopt && seed.empty())
        return NormalizeGameplayRuleSnapshot(baseline); // nothing else to take rules from
    auto rules = captured ? saved
                 : adopt  ? UpgradeGameplayRules(baseline)
                          : CaptureGameplayRules([](const char*, int fallback) { return fallback; });
    if (adopt)
        rules.erase("ootTricks"); // the seed's own trick list, below
    if (!captured || rules["version"] == 1) {
        const auto map = SongTravelMap::parse(seed.value("songTravel", nlohmann::json()));
        for (const auto& pair : { std::pair{ "gCombo.Enhancements.ForeignSongs", map.foreignSongs },
                                  std::pair{ "gCombo.Enhancements.SongTravel", map.enabled } }) {
            const auto& rule = *FindGameplayRule(pair.first);
            if (!captured || !HasSavedRuleValue(saved, rule))
                rules["values"][rule.name] = int(pair.second);
        }
        if (rules["version"] == 2) {
            rules["logicValues"]["gCombo.Enhancements.ForeignSongs"] = int(map.logicForeignSongs);
            rules["logicValues"]["gCombo.Enhancements.SongTravel"] = int(map.logicEnabled);
        }
    }
    const auto oot = seed.value("oot", nlohmann::json::object());
    const auto settings = oot.value("settings", nlohmann::json::object());
    for (const char* runtime : { "gEnhancements.BlueFireArrows", "gEnhancements.SunlightArrows" }) {
        const auto& rule = *FindGameplayRule(runtime);
        if (captured && HasSavedRuleValue(saved, rule))
            continue;
        const auto key = LogicRuleKey(runtime);
        const auto value = settings.find(key);
        const int enabled = value != settings.end() && ((value->is_boolean() && value->get<bool>()) ||
                                                        (value->is_number_integer() && *value == 1));
        rules["values"][runtime] = enabled;
        if (rules["version"] == 2)
            rules["logicValues"][runtime] = enabled;
    }
    if (!rules.contains("ootTricks") || !rules["ootTricks"].is_string()) {
        std::string csv;
        const auto names = oot.value("enabledTricks", nlohmann::json());
        if (names.is_array()) {
            for (const auto& name : names) {
                if (!name.is_string())
                    continue;
                if (!csv.empty())
                    csv += ",";
                csv += name.get<std::string>();
            }
        } else if (settings.contains("gRandoSettings.EnabledTricks") &&
                   settings["gRandoSettings.EnabledTricks"].is_string())
            csv = settings["gRandoSettings.EnabledTricks"].get<std::string>();
        rules["ootTricks"] = csv;
    }
    return NormalizeGameplayRuleSnapshot(rules);
}

inline nlohmann::json GameplayRulesForRun(const nlohmann::json& seed, const nlohmann::json& savedRun,
                                          const nlohmann::json& baseline = nlohmann::json()) {
    const auto original = GameplayRulesForSeed(seed, baseline);
    if (!IsGameplayRuleBlock(savedRun))
        return original;
    auto rules = NormalizeGameplayRuleSnapshot(savedRun);
    for (const auto& rule : kGameplayRules) {
        if (HasSavedRuleValue(UpgradeGameplayRules(savedRun), rule))
            continue;
        rules["values"][rule.name] = original["values"][rule.name];
        rules["logicValues"][rule.name] = original["logicValues"][rule.name];
    }
    if (!savedRun.contains("mmTricks"))
        rules["mmTricks"] = original["mmTricks"];
    if (!savedRun.contains("ootTricks") || !savedRun["ootTricks"].is_string())
        rules["ootTricks"] = original["ootTricks"];
    return rules;
}
} // namespace ComboRando
