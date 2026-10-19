#pragma once
#include "EntranceLayout.h"
#include "EntranceAvailability.h"
#include "rando/ComboSeedFormat.h"
#include <nlohmann/json.hpp>
#include <limits>

namespace ComboRando::Entrances {
// Read-only saved/generated views show the committed layout, even if a seed's
// requested setup CVars differ. Next-seed drafts use their editable CVars.
inline int SavedEntranceValue(const nlohmann::json& document, const char* key, int fallback) {
    if (!document.contains("entranceLayout"))
        return fallback;
    for (const auto& row : kEntranceSettings)
        if (std::string_view(key) == row.cvar)
            return row.get(DecodeEntranceLayout(document.at("entranceLayout"), LayoutPart::SettingsOnly).settings);
    return fallback;
}
// Operates only on a detached next-seed draft, never on a saved/generated seed.
// Transfer the old envelope's requested values once, then discard its derived
// capability/quest data. A new generation must resolve those itself.
inline bool NormalizeEntranceDraft(nlohmann::json& draft, const CapabilitySet& active) {
    RequireSupportedSettingsCapabilities(draft, active);
    auto& values = draft["setupCVars"];
    if (values.is_null())
        values = nlohmann::json::object();
    if (!values.is_object())
        throw std::invalid_argument("Malformed profile entrance settings");
    if (draft.contains("entranceLayout")) {
        const auto layout = DecodeEntranceLayout(draft.at("entranceLayout"), LayoutPart::SettingsOnly);
        for (const auto& row : kEntranceSettings)
            if (!values.contains(row.cvar))
                values[row.cvar] = row.get(layout.settings);
        draft.erase("entranceLayout");
        draft["fileType"] = kComboSeedFileType;
        draft["version"] = 1;
    }
    const auto normalized = ReadEntranceSettings(
        [&](const char* key, int fallback) {
            const auto it = values.find(key);
            return it != values.end() && it->is_number_integer() && *it >= 0 && *it <= std::numeric_limits<int>::max()
                       ? it->get<int>()
                       : fallback;
        },
        active);
    bool stripped = false;
    for (const auto& row : kEntranceSettings)
        if (const auto it = values.find(row.cvar); it != values.end()) {
            const int value = row.get(normalized);
            stripped |= !it->is_number_integer() || *it != value;
            *it = value;
        }
    return stripped;
}

inline nlohmann::json EntranceProfileCatalog(const CapabilitySet& active) {
    auto rows = nlohmann::json::array();
    const auto caps = active.Selected("");
    for (const auto& row : kEntranceSettings) {
        if (!row.visible(caps))
            continue;
        auto choices = nlohmann::json::array();
        for (int i = 0; i < int(row.choices.size()); ++i)
            if (row.supports(i, caps))
                choices.push_back({ { "value", i },
                                    { "label", row.choices[i].name },
                                    { "description", row.choices[i].description } });
        rows.push_back({ { "key", row.key },
                         { "cvar", row.cvar },
                         { "label", row.name },
                         { "description", row.description },
                         { "choices", choices },
                         { "page", "Logic & access" },
                         { "heading", "Entrances" } });
    }
    return rows;
}

template <class Read> EntranceSettingsFacts ProfileEntranceFacts(const nlohmann::json& draft, Read read) {
    EntranceSettingsFacts f;
    f.combinedMode = CombinedMode(ReadGameMode(draft));
    const int logic = read("gRandoSettings.LogicRules", 0);
    f.logicEnabled = logic != 1;
    f.glitchless = logic == 0;
    f.shipwrightEntrances = ShipEntrancesActive(read);
    f.enemyRandomizer = read("gEnhancements.RandomizedEnemies", 0) != 0;
    f.songTravelLogic = read("gCombo.Rando.SongTravel", 0) != 0;
    f.foreignSongsLogic = read("gCombo.Rando.ForeignSongs", 0) != 0;
    for (const auto& [key, name] : { std::pair{ "StartingMinuetOfForest", "Minuet of Forest" },
                                     std::pair{ "StartingBoleroOfFire", "Bolero of Fire" },
                                     std::pair{ "StartingSerenadeOfWater", "Serenade of Water" },
                                     std::pair{ "StartingRequiemOfSpirit", "Requiem of Spirit" },
                                     std::pair{ "StartingNocturneOfShadow", "Nocturne of Shadow" },
                                     std::pair{ "StartingPreludeOfLight", "Prelude of Light" } })
        if (read((std::string("gRandoSettings.") + key).c_str(), 0))
            f.ootStartingItems.emplace_back(name);
    const auto mm = draft.value("mm", nlohmann::json::object()).value("settings", nlohmann::json::object());
    if (const auto it = mm.find("gRando.StartingItems"); it != mm.end() && it->is_array())
        for (const auto& item : *it)
            if (item.is_string())
                f.mmStartingItems.push_back(item.get<std::string>());
    return f;
}
// Asset facts come from the native profile catalogue, not an unapplied MQ CVar.
template <class Read>
EntranceRefusal ProfileWallmasterRefusal(const nlohmann::json& draft, const nlohmann::json& ootCatalog, Read read) {
    auto why = WallmasterSettingsRefusal(ProfileEntranceFacts(draft, read));
    if (!why.empty())
        return why;
    if (!ootCatalog.contains("hasMasterQuest") || !ootCatalog.contains("hasOriginal") ||
        !ootCatalog.contains("mqPlayable"))
        return { kWallmasterDungeonDataText, "Wallmaster Shuffle cannot verify the available dungeon data" };
    // Only Master Quest is loaded, or MQ dungeons are selected where they can be played (mqPlayable: a real
    // oot-mq.o2r or OoTMM's bundled MQ data). Without MQ data the native settings keep every dungeon vanilla.
    if ((ootCatalog.at("hasMasterQuest").get<bool>() && !ootCatalog.at("hasOriginal").get<bool>()) ||
        (ootCatalog.at("mqPlayable").get<bool>() && read("gRandoSettings.MQDungeons", 0) != 0))
        return { kWallmasterMasterQuestText, "Wallmaster Shuffle currently supports vanilla dungeons only" };
    return {};
}
} // namespace ComboRando::Entrances
