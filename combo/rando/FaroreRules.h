#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
namespace ComboRando {
// Native startup sets this only after the complete bridge and owning typed resources
// are ready. Teardown invalidates it; saved seed rules still control actual use.
inline bool ComboFaroreIntegrationReady = false;
struct FaroreRules {
    bool spellWindMm = false, crossGameFw = false, crossAge = false;
    nlohmann::json json() const {
        return {
            { "version", 1 }, { "spellWindMm", spellWindMm }, { "crossGameFw", crossGameFw }, { "crossAge", crossAge }
        };
    }
};
inline bool ReadFaroreRules(const nlohmann::json& j, FaroreRules& out) {
    out = {};
    if (j.is_null())
        return true;
    if (!j.is_object() || !j.contains("version") || !j["version"].is_number_integer() || j["version"] != 1)
        return false;
    auto flag = [&](const char* key, bool& value) {
        if (!j.contains(key))
            return false;
        const auto& v = j[key];
        if (v.is_boolean()) {
            value = v.get<bool>();
            return true;
        }
        if (!v.is_number_integer() || v < 0 || v > 1)
            return false;
        value = v == 1;
        return true;
    };
    FaroreRules r;
    if (!flag("spellWindMm", r.spellWindMm) || !flag("crossGameFw", r.crossGameFw) || !flag("crossAge", r.crossAge))
        return false;
    if (r.crossGameFw && !r.spellWindMm)
        return false;
    out = r;
    return true;
}
inline FaroreRules FaroreRulesFromMmDump(const nlohmann::json& dump) {
    FaroreRules r;
    const auto options = dump.value("options", nlohmann::json::object());
    auto yes = [&](const char* key) {
        auto it = options.find(key);
        return it != options.end() && it->is_number_integer() && *it == 1;
    };
    r.spellWindMm = yes("RO_MM_SPELL_WIND");
    r.crossGameFw = r.spellWindMm && yes("RO_CROSS_GAME_FARORE");
    r.crossAge = yes("RO_MM_CROSS_AGE");
    return r;
}
} // namespace ComboRando
