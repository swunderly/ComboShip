#pragma once
#include "GameplayRules.h"
#include <optional>

namespace ComboRando {
// Native RandoOptionLogic order is Glitchless=0, No Logic=1 (RandomizerOptions.h).
// The host regression compiles that enum and checks this contract directly.
inline constexpr int kNativeNoLogicOption = 1;
inline constexpr const char* kNativeLogicRule = "gRandoSettings.LogicRules";
inline constexpr const char* kNativeAllLocations = "gRandoSettings.AllLocationsReachable";
inline constexpr const char* kCombinedGuarantee = "gCombo.Rando.Reachability";
inline constexpr bool IsNativeNoLogic(int nativeLogic) {
    return nativeLogic == kNativeNoLogicOption;
}
inline constexpr int NativeAllLocationsForGuarantee(int combinedGuarantee, int nativeLogic) {
    return !IsNativeNoLogic(nativeLogic) && combinedGuarantee == 0 ? 1 : 0;
}
template <class Read, class Write> void SynchronizeGenerationGuarantee(Read read, Write write) {
    write(kNativeAllLocations, NativeAllLocationsForGuarantee(read(kCombinedGuarantee, 0), read(kNativeLogicRule, 0)));
}
inline bool IsRetiredNativeRuleControl(const char* key) {
    if (!key)
        return false;
    if (!std::strcmp(key, kNativeAllLocations))
        return true;
    for (const char* paired :
         { "gEnhancements.DpadEquips", "gEnhancements.Dpad.DpadEquips", "gEnhancements.ItemUnequip",
           "gEnhancements.Equipment.ItemUnequip", "gEnhancements.SkipArrowAnimation",
           "gEnhancements.Equipment.MagicArrowEquipSpeed", "gEnhancements.BowArrowCycle",
           "gEnhancements.PlayerActions.ArrowCycle", "gEnhancements.Autosave", "gEnhancements.Saving.Autosave" })
        if (!std::strcmp(key, paired))
            return true;
    const auto* rule = FindGameplayRule(key);
    // Trick-governed restorations keep their native runtime checkbox. The
    // logical control belongs exclusively to the normal Tricks & glitches UI.
    return rule && !(rule->trickGoverned && !std::strcmp(key, rule->name));
}
struct NativePoolFailure {
    std::string error;
    bool retryable;
};
// Inspect both engine results before any pool counting, forced-placement or entrance work.
// A corrupt ABI result is a fatal error, even if the other engine asks for another attempt.
inline std::optional<NativePoolFailure> ParseNativePoolFailure(const std::string& sohDump, const std::string& mmDump) {
    std::optional<NativePoolFailure> result;
    const auto append = [&](const char* game, const std::string& error, bool retryable) {
        const std::string labelled = std::string(game) + ": " + error;
        if (result) {
            result->error += "; " + labelled;
            result->retryable &= retryable;
        } else
            result = NativePoolFailure{ labelled, retryable };
    };
    const auto inspect = [&](const char* game, const std::string& raw) {
        try {
            const auto dump = nlohmann::json::parse(raw);
            if (!dump.is_object()) {
                append(game, "Malformed native pool result: expected an object", false);
                return;
            }
            if (dump.contains("error")) {
                if (!dump["error"].is_string() || dump["error"].get_ref<const std::string&>().empty() ||
                    (dump.contains("retryable") && !dump["retryable"].is_boolean())) {
                    append(game, "Malformed native pool failure", false);
                    return;
                }
                append(game, dump["error"].get<std::string>(), dump.value("retryable", false));
                return;
            }
            if (dump.contains("retryable") || !dump.contains("checks") || !dump["checks"].is_array() ||
                (dump.contains("pool") && !dump["pool"].is_array()))
                append(game, "Malformed native pool result", false);
        } catch (const nlohmann::json::exception&) { append(game, "Malformed native pool JSON", false); }
    };
    inspect("OoT", sohDump);
    inspect("MM", mmDump);
    return result;
}
} // namespace ComboRando
