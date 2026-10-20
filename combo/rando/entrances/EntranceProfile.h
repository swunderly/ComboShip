#pragma once
#include "EntranceLayout.h"
#include "EntranceAvailability.h"
#include "rando/ComboSeedFormat.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <limits>
#include <vector>

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
// What the native Master Quest roll can make of one dungeon (settings.cpp Context::FinalizeSettings).
enum class MqRoll : uint8_t { Vanilla, MasterQuest, Either };
// The per-dungeon CVars of the MQ Dungeon Setting rows, in kOotMqDungeons order (settings.cpp RSK_MQ_*).
inline constexpr const char* kMqDungeonQuestSettings[] = {
    "gRandoSettings.MQDungeonsDekuTree",
    "gRandoSettings.MQDungeonsDodongosCavern",
    "gRandoSettings.MQDungeonsJabuJabu",
    "gRandoSettings.MQDungeonsForestTemple",
    "gRandoSettings.MQDungeonsFireTemple",
    "gRandoSettings.MQDungeonsWaterTemple",
    "gRandoSettings.MQDungeonsSpiritTemple",
    "gRandoSettings.MQDungeonsShadowTemple",
    "gRandoSettings.MQDungeonsBottomOfTheWell",
    "gRandoSettings.MQDungeonsIceCavern",
    "gRandoSettings.MQDungeonsGTG",
    "gRandoSettings.MQDungeonsGanonsCastle",
};
static_assert(std::size(kMqDungeonQuestSettings) == kOotMqDungeons.size());
// Each dungeon's possible quests under the native roll where MQ can be played: MQ data alone makes all twelve
// MQ; MQ Dungeon Setting (None 0, Set Number 1, Random 2, Selection Only 3) with Set Dungeon Quests picks
// presets (Vanilla 0, Master Quest 1, Random 2) and Set Number's count from the rest.
template <class Read> std::array<MqRoll, kOotMqDungeons.size()> MqRollRange(Read read, bool mqOnly) {
    std::array<MqRoll, kOotMqDungeons.size()> range{};
    range.fill(mqOnly ? MqRoll::MasterQuest : MqRoll::Vanilla);
    const int mode = mqOnly ? 0 : read("gRandoSettings.MQDungeons", 0);
    if (mode == 3) { // Selection Only: each Random dungeon is a coin flip
        for (size_t i = 0; i < range.size(); ++i) {
            const int preset = read(kMqDungeonQuestSettings[i], 0);
            range[i] = preset == 1 ? MqRoll::MasterQuest : preset == 2 ? MqRoll::Either : MqRoll::Vanilla;
        }
    } else if (mode == 1 || mode == 2) {
        const bool presets = read("gRandoSettings.MQDungeonsSelection", 0) != 0;
        std::vector<size_t> candidates;
        int set = 0;
        for (size_t i = 0; i < range.size(); ++i) {
            const int preset = presets ? read(kMqDungeonQuestSettings[i], 0) : 2;
            if (preset == 1) {
                range[i] = MqRoll::MasterQuest;
                ++set;
            } else if (preset == 2) {
                candidates.push_back(i);
            }
        }
        // Set Number picks count - presets of the candidates (none or all fix them); Random any number.
        const int count = mode == 1 ? read("gRandoSettings.MQDungeonCount", static_cast<int>(range.size())) : -1;
        const int picks = mode == 1 ? std::clamp(count - set, 0, static_cast<int>(candidates.size())) : -1;
        for (const auto i : candidates)
            range[i] = picks == 0                                     ? MqRoll::Vanilla
                       : picks == static_cast<int>(candidates.size()) ? MqRoll::MasterQuest
                                                                      : MqRoll::Either;
    }
    return range;
}
// Every roll of these settings is a quest context with more OoT Wallmasters than dungeon entrances: Spirit Temple
// and Bottom of the Well always Master Quest, Shadow Temple and Ganon's Castle always vanilla
// (UnifiedBridge.h RequireWallmasterCapacity). Such a selection can never generate, in OoTMM either.
template <class Read> bool MqWallmasterShortfallForced(Read read, bool mqOnly) {
    const auto range = MqRollRange(read, mqOnly);
    const auto is = [&](std::string_view dungeon, MqRoll roll) { return range[OotMqIndex(dungeon)] == roll; };
    return is("Spirit", MqRoll::MasterQuest) && is("BotW", MqRoll::MasterQuest) && is("Shadow", MqRoll::Vanilla) &&
           is("Ganon", MqRoll::Vanilla);
}
// Asset facts come from the native profile catalogue, not an unapplied MQ CVar. `caps` are the build's.
template <class Read>
EntranceRefusal ProfileWallmasterRefusal(const nlohmann::json& draft, const nlohmann::json& ootCatalog, Read read,
                                         const CapabilitySet& caps = ActiveCapabilities()) {
    auto why = WallmasterSettingsRefusal(ProfileEntranceFacts(draft, read));
    if (!why.empty())
        return why;
    if (!ootCatalog.contains("hasMasterQuest") || !ootCatalog.contains("hasOriginal") ||
        !ootCatalog.contains("mqPlayable"))
        return { kWallmasterDungeonDataText, "Wallmaster Shuffle cannot verify the available dungeon data" };
    // Only Master Quest is loaded, or MQ dungeons are selected where they can be played (mqPlayable: a real
    // oot-mq.o2r or OoTMM's bundled MQ data). Without MQ data the native settings keep every dungeon vanilla.
    const bool mqOnly = ootCatalog.at("hasMasterQuest").get<bool>() && !ootCatalog.at("hasOriginal").get<bool>();
    if (mqOnly || (ootCatalog.at("mqPlayable").get<bool>() && read("gRandoSettings.MQDungeons", 0) != 0)) {
        // The frozen vanilla id takes no Master Quest dungeon; the quests id takes every context it can place.
        if (!caps.Active(kQuestsWallmasterCapability))
            return { kWallmasterMasterQuestText, "Wallmaster Shuffle currently supports vanilla dungeons only" };
        if (MqWallmasterShortfallForced(read, mqOnly))
            return WallmasterCapacityRefusal();
    }
    return {};
}
} // namespace ComboRando::Entrances
