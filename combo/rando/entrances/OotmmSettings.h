// Hidden settings foundation for OoTMM v32.3 entrance milestones.
// No row is a public capability until every route it selects is implemented.
#pragma once
#include "Shuffle.h"
#include <span>
#include <type_traits>

namespace ComboRando::Entrances {
struct EntranceChoice {
    std::string_view value, name, description;
};
inline constexpr EntranceChoice kBooleanChoices[] = { { "false", "", "" }, { "true", "", "" } };
struct EntranceSetting {
    const char *key, *cvar, *name, *description;
    std::span<const EntranceChoice> choices;
    int (*get)(const Settings&);
    void (*set)(Settings&, int);
    bool (*condition)(const Settings&);
    // Bit n admits value n. Default value zero is always allowed. All non-default
    // bits remain zero in M0; representing Full does not make it available.
    uint32_t capabilityValues = 0;
    int defaultValue = 0;
    bool visible() const {
        return capabilityValues != 0;
    }
    bool supports(int value) const {
        return value == defaultValue || (value > 0 && static_cast<size_t>(value) < choices.size() &&
                                         (capabilityValues & (uint32_t(1) << value)) != 0);
    }
};

template <auto Member, size_t N>
constexpr EntranceSetting MakeEntranceSetting(const char* key, const char* cvar, const char* name,
                                              const char* description, const EntranceChoice (&choices)[N],
                                              bool (*condition)(const Settings&)) {
    return { key,
             cvar,
             name,
             description,
             choices,
             [](const Settings& s) { return static_cast<int>(s.*Member); },
             [](Settings& s, int value) {
                 using Value = std::remove_cvref_t<decltype(s.*Member)>;
                 s.*Member = static_cast<Value>(value >= 0 && value < static_cast<int>(N) ? value : 0);
             },
             condition };
}

#include "OotmmSettingsData.inc"

// Match the reference's repeated, array-ordered validation. Each pass can only
// reset a value, so at most one pass per setting plus the stable pass is needed.
inline void ValidateEntranceSettings(Settings& settings) {
    settings.games = Games::Both;
    settings.erWarps = Warps::None;
    settings.erOneWaysSongs = settings.erOneWaysStatues = settings.erOneWaysWoods = false;
    for (size_t pass = 0; pass <= std::size(kEntranceSettings); ++pass) {
        bool changed = false;
        for (const auto& row : kEntranceSettings) {
            const int before = row.get(settings);
            if (before < 0 || static_cast<size_t>(before) >= row.choices.size() || !row.condition(settings))
                row.set(settings, row.defaultValue);
            changed |= before != row.get(settings);
        }
        if (!changed)
            return;
    }
    throw std::logic_error("Entrance settings did not converge");
}

// Future profile, headless and menu readers share this boundary. Unknown/private
// CVar values cannot enable a pool whose capability has not shipped.
template <class Read> Settings ReadEntranceSettings(Read read) {
    Settings settings;
    for (const auto& row : kEntranceSettings) {
        const int value = read(row.cvar, row.defaultValue);
        row.set(settings, row.supports(value) ? value : row.defaultValue);
    }
    ValidateEntranceSettings(settings);
    return settings;
}
} // namespace ComboRando::Entrances
