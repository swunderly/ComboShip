#pragma once
#include <nlohmann/json.hpp>
namespace ComboRando {
// The seed's MM gear settings (OoTMM bootsIronMm, bootsHoverMm, tunicGoronMm, tunicZoraMm, data.ts:2024-2054), so a
// save always uses its seed's values rather than the live options.
struct GearRules {
    bool bootsIronMm = false, bootsHoverMm = false, tunicGoronMm = false, tunicZoraMm = false;
    nlohmann::json json() const {
        return { { "version", 1 },
                 { "bootsIronMm", bootsIronMm },
                 { "bootsHoverMm", bootsHoverMm },
                 { "tunicGoronMm", tunicGoronMm },
                 { "tunicZoraMm", tunicZoraMm } };
    }
};
inline bool ReadGearRules(const nlohmann::json& j, GearRules& out) {
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
    GearRules r;
    if (!flag("bootsIronMm", r.bootsIronMm) || !flag("bootsHoverMm", r.bootsHoverMm) ||
        !flag("tunicGoronMm", r.tunicGoronMm) || !flag("tunicZoraMm", r.tunicZoraMm))
        return false;
    out = r;
    return true;
}
inline GearRules GearRulesFromMmDump(const nlohmann::json& dump) {
    const auto options = dump.value("options", nlohmann::json::object());
    auto yes = [&](const char* key) {
        auto i = options.find(key);
        return i != options.end() && i->is_number_integer() && *i == 1;
    };
    return { yes("RO_MM_BOOTS_IRON"), yes("RO_MM_BOOTS_HOVER"), yes("RO_MM_TUNIC_GORON"), yes("RO_MM_TUNIC_ZORA") };
}
} // namespace ComboRando
