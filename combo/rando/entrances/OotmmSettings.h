// Hidden settings foundation for OoTMM v32.3 entrance milestones.
// No row is a public capability until every route it selects is implemented.
#pragma once
#include "Capabilities.h"
#include "Shuffle.h"
#include <algorithm>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

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
    int defaultValue = 0;
    // Capabilities.h decides which non-default values a build admits; representing
    // Full does not make it available.
    bool visible(const CapabilitySet& caps) const {
        for (size_t value = 0; value < choices.size(); ++value)
            if (static_cast<int>(value) != defaultValue && supports(static_cast<int>(value), caps))
                return true;
        return false;
    }
    bool supports(int value, const CapabilitySet& caps) const {
        return value == defaultValue ||
               (value > 0 && static_cast<size_t>(value) < choices.size() && caps.Admits(key, choices[value].value));
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

// Every capability value names a pinned key and one of its non-default values.
constexpr bool CapabilitiesNamePinnedValues() {
    for (const auto& capability : kEntranceCapabilities)
        for (const auto& admitted : capability.values) {
            bool found = false;
            for (const auto& row : kEntranceSettings)
                for (size_t value = 0; value < row.choices.size(); ++value)
                    if (admitted.key == row.key && static_cast<int>(value) != row.defaultValue &&
                        admitted.value == row.choices[value].value)
                        found = true;
            if (!found)
                return false;
        }
    return true;
}
static_assert(CapabilitiesNamePinnedValues(), "Capability values must name pinned non-default entrance values");

inline const EntranceSetting* FindEntranceSetting(std::string_view key) {
    for (const auto& row : kEntranceSettings)
        if (key == row.key)
            return &row;
    return nullptr;
}

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

// True when one capability row admits every non-default value together.
inline bool AdmitsEntranceSettings(const Settings& settings, const CapabilitySet& caps) {
    const auto admittedBy = [&](size_t capability) {
        for (const auto& row : kEntranceSettings) {
            const int value = row.get(settings);
            if (value != row.defaultValue && (value < 0 || static_cast<size_t>(value) >= row.choices.size() ||
                                              !caps.Admits(capability, row.key, row.choices[value].value)))
                return false;
        }
        return true;
    };
    bool defaults = true;
    for (const auto& row : kEntranceSettings)
        defaults = defaults && row.get(settings) == row.defaultValue;
    for (size_t capability = 0; !defaults && capability < kCapabilityCount; ++capability)
        if (admittedBy(capability))
            return true;
    return defaults;
}

// Resets, in kEntranceSettings array order, each value that no capability row
// admits together with the values kept before it, the way util.ts:79-130 resets a
// value whose cond fails. Of two values whose companion row is not active, the
// later one is reset.
inline void RestrictEntranceSettings(Settings& settings, const CapabilitySet& caps) {
    Settings kept = settings;
    for (const auto& row : kEntranceSettings)
        row.set(kept, row.defaultValue);
    for (const auto& row : kEntranceSettings) {
        row.set(kept, row.get(settings));
        if (!AdmitsEntranceSettings(kept, caps))
            row.set(kept, row.defaultValue);
    }
    settings = kept;
}

// Future profile and menu readers share this boundary (headless --er uses
// ReadEntranceArgs). Unknown/private CVar values cannot enable a pool whose
// capability has not shipped.
template <class Read> Settings ReadEntranceSettings(Read read, const CapabilitySet& caps) {
    Settings settings;
    for (const auto& row : kEntranceSettings) {
        const int value = read(row.cvar, row.defaultValue);
        row.set(settings, row.supports(value, caps) ? value : row.defaultValue);
    }
    ValidateEntranceSettings(settings);
    RestrictEntranceSettings(settings, caps);
    ValidateEntranceSettings(settings);
    return settings;
}

// "key=value" for each non-default value, in array order.
inline std::string EntranceSettingsText(const Settings& settings) {
    std::string text;
    for (const auto& row : kEntranceSettings) {
        const int value = row.get(settings);
        if (value != row.defaultValue && value >= 0 && static_cast<size_t>(value) < row.choices.size())
            text += (text.empty() ? "" : ", ") + std::string(row.key) + "=" + std::string(row.choices[value].value);
    }
    return text.empty() ? "all defaults" : text;
}

// Headless --er <key>=<value> pairs by pinned OoTMM name, and the --er-capability
// id (empty when not given). An unknown name, or a value the capabilities do not
// admit, is an error instead of being stripped; OoTMM's own resets still apply.
// Returns the error, or an empty string after setting `settings`.
inline std::string ReadEntranceArgs(std::span<const std::string> pairs, std::string_view capability,
                                    const CapabilitySet& active, Settings& settings) {
    CapabilitySet caps = active.WithoutDevOnly();
    if (!capability.empty()) {
        if (!CapabilitySet::Known(capability))
            return "unknown entrance capability '" + std::string(capability) + "'";
        caps = active.Only(capability);
        if (caps.Empty())
            return "entrance capability '" + std::string(capability) + "' is not available in this build";
    }
    Settings requested;
    std::vector<const EntranceSetting*> given;
    for (const auto& pair : pairs) {
        const size_t equals = pair.find('=');
        if (equals == std::string::npos)
            return "--er needs <key>=<value> (got '" + pair + "')";
        const std::string key = pair.substr(0, equals), value = pair.substr(equals + 1);
        const EntranceSetting* row = FindEntranceSetting(key);
        if (row == nullptr)
            return "unknown entrance setting '" + key + "'";
        if (std::find(given.begin(), given.end(), row) != given.end())
            return "--er sets " + key + " more than once";
        given.push_back(row);
        int index = -1;
        std::string names;
        for (size_t i = 0; i < row->choices.size(); ++i) {
            if (row->choices[i].value == value)
                index = static_cast<int>(i);
            names += (i == 0 ? "" : ", ") + std::string(row->choices[i].value);
        }
        if (index < 0)
            return "unknown value '" + value + "' for " + key + " (expected " + names + ")";
        if (!row->supports(index, caps)) {
            if (!capability.empty())
                return key + "=" + value + " is not admitted by " + std::string(capability);
            return key + "=" + value + " is not available in this build" +
                   (active.Admits(key, value) ? " without --er-capability" : "");
        }
        row->set(requested, index);
    }
    ValidateEntranceSettings(requested);
    Settings restricted = requested;
    RestrictEntranceSettings(restricted, caps);
    for (size_t i = 0; i < std::size(kEntranceSettings); ++i) {
        const auto& row = kEntranceSettings[i];
        if (row.get(restricted) == row.get(requested))
            continue;
        Settings before = restricted;
        for (size_t later = i; later < std::size(kEntranceSettings); ++later)
            kEntranceSettings[later].set(before, kEntranceSettings[later].defaultValue);
        return std::string(row.key) + "=" + std::string(row.choices[row.get(requested)].value) +
               " cannot be combined with " + EntranceSettingsText(before) + " in this build";
    }
    settings = requested;
    return {};
}
} // namespace ComboRando::Entrances
