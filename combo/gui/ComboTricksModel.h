#pragma once
#include "menu/ComboTrickABI.h"
#include <algorithm>
#include <cctype>
#include <string>

namespace ComboTricks {
inline CwTrickInfo RestoredShadowDoorInfo(const char* label, const char* description, bool enabled) {
    return { -1000,
             label,
             description,
             "Graveyard",
             "Experimental · Glitch · restoration",
             5,
             (1u << 5) | (1u << 6),
             enabled ? 1 : 0 };
}
inline CwTrickInfo EarlyEyeballFrogInfo(const char* label, const char* description, bool enabled) {
    return { -1001,
             label,
             description,
             "Zora's Domain",
             "Experimental · Glitch · restoration",
             5,
             (1u << 5) | (1u << 6),
             enabled ? 1 : 0 };
}
inline std::string Fold(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
inline bool Matches(const CwTrickInfo& row, int game, const std::string& query, int world, int enabled,
                    int difficulty) {
    if (world && world - 1 != game)
        return false;
    if (enabled == 1 && !row.enabled)
        return false;
    if (enabled == 2 && row.enabled)
        return false;
    if (difficulty >= 0 && row.difficulty != difficulty)
        return false;
    std::string text;
    for (const char* value : { row.name, row.description, row.area, row.tags })
        if (value) {
            text += value;
            text += ' ';
        }
    return Fold(text).find(Fold(query)) != std::string::npos;
}
} // namespace ComboTricks
