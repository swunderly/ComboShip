#pragma once
// The per-seed and per-save Master Quest marker (combo/mq/MqMode.h) in JSON: a seed document's "oot" section, SoH's
// spoiler and a save's randomizer section. Absence is Legacy, so every seed and save made before the marker keeps
// SoH's behaviour: vanilla, or the real oot-mq.o2r when one is installed.
//
// A seed made with the bundled data records the source and its MQ dungeons:
//   "mqDataSource": "ootmm-32.3", "masterQuestDungeons": [ids], "masterQuestDungeonNames": [names]
// The ids are OoTMM's dungeon ids (include/combo/dungeon.h MQ_*, the bits of Config_IsMq's mask), which are SoH's
// DungeonKey order, as the save's own "masterQuestDungeons" already stores them. The names are for people.
#include "MqMode.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace ComboMq {

class UnsupportedMqDataSource : public std::invalid_argument {
  public:
    explicit UnsupportedMqDataSource(const char* message = "Unsupported Master Quest data source")
        : std::invalid_argument(message) {
    }
};

inline constexpr const char* kMqDungeonsKey = "masterQuestDungeons";
// OoTMM's 12 MQ dungeons (ComboMqData.h kDungeonCount; SoH's DungeonKey DEKU_TREE..GANONS_CASTLE).
inline constexpr int kMqSeedDungeonCount = 12;
inline constexpr int kMqDungeonMaskAll = (1 << kMqSeedDungeonCount) - 1;
// The same dungeons by name, for people reading the seed: SoH's names (dungeon.cpp DungeonInfo), as SoH's spoiler
// lists its MQ dungeons (spoiler_log.cpp WriteMasterQuestDungeons) and OoTMM's lists its Master Quest Dungeons
// (packages/generator/lib/combo/spoiler.ts writeWorldFlags). The ids stay the record a replay is checked against.
inline constexpr const char* kMqDungeonNamesKey = "masterQuestDungeonNames";
inline constexpr const char* kMqDungeonNames[kMqSeedDungeonCount] = {
    "Deku Tree",          "Dodongo's Cavern", "Jabu Jabu's Belly",      "Forest Temple",
    "Fire Temple",        "Water Temple",     "Spirit Temple",          "Shadow Temple",
    "Bottom of the Well", "Ice Cavern",       "Gerudo Training Ground", "Ganon's Castle",
};

// The marker of an object, never throwing: absent is Legacy, anything but a known string is Unsupported.
inline SeedMqSource MqDataSourceOf(const nlohmann::json& object) {
    if (!object.is_object())
        return SeedMqSource::Unsupported;
    const auto it = object.find(kMqDataSourceKey);
    if (it == object.end())
        return SeedMqSource::Legacy;
    if (!it->is_string())
        return SeedMqSource::Unsupported;
    return SeedMqSourceFromMarker(it->get_ref<const std::string&>().c_str());
}
// The marker of an object; throws UnsupportedMqDataSource for one this build does not know.
inline SeedMqSource ReadMqDataSource(const nlohmann::json& object) {
    if (!object.is_object())
        throw std::invalid_argument("Master Quest data source requires an object");
    const auto source = MqDataSourceOf(object);
    if (source == SeedMqSource::Unsupported)
        throw UnsupportedMqDataSource();
    return source;
}
// Writes the marker, or removes it for Legacy, so a Legacy object is exactly what it was before the marker.
inline void WriteMqDataSource(nlohmann::json& object, SeedMqSource source) {
    if (const char* marker = MarkerForSeedMqSource(source))
        object[kMqDataSourceKey] = marker;
    else if (object.is_object())
        object.erase(kMqDataSourceKey);
}

inline nlohmann::json MqDungeonIds(int mask) {
    auto ids = nlohmann::json::array();
    for (int id = 0; id < kMqSeedDungeonCount; id++)
        if (mask & (1 << id))
            ids.push_back(id);
    return ids;
}
inline nlohmann::json MqDungeonNames(int mask) {
    auto names = nlohmann::json::array();
    for (int id = 0; id < kMqSeedDungeonCount; id++)
        if (mask & (1 << id))
            names.push_back(kMqDungeonNames[id]);
    return names;
}
// The mask of a list of dungeon ids; -1 when the list is malformed.
inline int MqDungeonMaskOf(const nlohmann::json& ids) {
    if (!ids.is_array())
        return -1;
    int mask = 0;
    for (const auto& id : ids) {
        if (!id.is_number_integer() || id.get<int>() < 0 || id.get<int>() >= kMqSeedDungeonCount ||
            (mask & (1 << id.get<int>())))
            return -1;
        mask |= 1 << id.get<int>();
    }
    return mask;
}

// Stamps a new seed document's "oot" section: a bundled seed records its source and MQ dungeons, by id and by name;
// a Legacy seed records nothing.
inline void StampSeedMq(nlohmann::json& oot, SeedMqSource source, int mqDungeonMask) {
    if (source != SeedMqSource::Bundled) {
        WriteMqDataSource(oot, SeedMqSource::Legacy);
        return;
    }
    if (mqDungeonMask < 0 || mqDungeonMask > kMqDungeonMaskAll)
        throw std::runtime_error("Master Quest dungeons of the generated seed are unavailable");
    WriteMqDataSource(oot, source);
    oot[kMqDungeonsKey] = MqDungeonIds(mqDungeonMask);
    oot[kMqDungeonNamesKey] = MqDungeonNames(mqDungeonMask);
}
// The MQ dungeons a seed document records: -1 for a Legacy seed (they are re-derived from its settings). Throws for
// an unknown marker or a malformed bundled record.
inline int RecordedMqDungeonMask(const nlohmann::json& oot) {
    if (ReadMqDataSource(oot) == SeedMqSource::Legacy)
        return -1;
    const int mask = MqDungeonMaskOf(oot.value(kMqDungeonsKey, nlohmann::json()));
    if (mask < 0)
        throw UnsupportedMqDataSource("Malformed Master Quest dungeons in the seed");
    return mask;
}
// The text a seed view shows for a bundled seed's MQ dungeons ("None" when it has none). Empty for a seed without the
// marker, which records none, and for a malformed record.
inline std::string RecordedMqDungeonsText(const nlohmann::json& oot) {
    if (MqDataSourceOf(oot) != SeedMqSource::Bundled)
        return {};
    const int mask = MqDungeonMaskOf(oot.value(kMqDungeonsKey, nlohmann::json()));
    if (mask < 0)
        return {};
    std::string text;
    for (const auto& name : MqDungeonNames(mask))
        text += (text.empty() ? "" : ", ") + name.get<std::string>();
    return text.empty() ? "None" : text;
}
// Whether this install can play a seed document's recorded MQ dungeons, before regenerating it. A Legacy seed records
// none and is decided as before, by its regenerated dungeons and SoH's file rule.
inline bool SeedMqPlayable(const nlohmann::json& oot, bool bundledAvailable) {
    const int mask = RecordedMqDungeonMask(oot);
    return mask <= 0 || bundledAvailable;
}
// Whether a replay rolled the MQ dungeons the seed recorded. Always true for a Legacy seed.
inline bool ReplayKeepsMqDungeons(const nlohmann::json& oot, int rolledMask) {
    const int mask = RecordedMqDungeonMask(oot);
    return mask < 0 || mask == rolledMask;
}

// A save's MQ needs: its randomizer section's marker and MQ dungeon count (SaveManager masterQuestDungeonCount).
struct SaveMqNeeds {
    SeedMqSource source = SeedMqSource::Legacy;
    size_t mqDungeonCount = 0;
};
inline SaveMqNeeds ReadSaveMqNeeds(const nlohmann::json& save) {
    SaveMqNeeds needs;
    const nlohmann::json::json_pointer path("/sections/randomizer/data");
    if (!save.is_object() || !save.contains(path))
        return needs;
    const auto& data = save.at(path);
    if (!data.is_object())
        return needs; // malformed: the native loader's own handling decides
    needs.source = MqDataSourceOf(data);
    const auto count = data.find("masterQuestDungeonCount");
    if (count != data.end() && count->is_number_unsigned())
        needs.mqDungeonCount = count->get<size_t>();
    return needs;
}
// SaveManager::LoadFile's refusal: a save with the marker whose MQ dungeons this install cannot play. A save without
// the marker is never refused here; file select keeps SoH's rule for it.
inline bool SaveRefusedForMq(const SaveMqNeeds& needs, bool hasArchiveMq, bool bundledAvailable) {
    return needs.source != SeedMqSource::Legacy &&
           !CanPlayMqDungeons(needs.source, needs.mqDungeonCount, hasArchiveMq, bundledAvailable);
}
// What the player is told about a save SaveRefusedForMq refuses: LoadFile's popup, which file select also shows when
// such a save is chosen (it greys the save out, so LoadFile is never reached from there).
inline constexpr const char* kSaveMqRefusalTitle = "Master Quest data unavailable";
inline const char* SaveMqRefusalText(SeedMqSource source) {
    return source == SeedMqSource::Bundled
               ? "This save uses Master Quest dungeons from OoTMM's data, which needs an OoT USA 1.0 ROM."
               : "This save uses a Master Quest data source this version does not know.";
}

// soh.dll's C exports (OTRGlobals.cpp) for the launcher and the headless generator.
using FnGetMqDataSource = int (*)();
using FnSetMqDataSource = int (*)(int);
using FnHasBundledMq = int (*)();
using FnGetMqDungeonMask = int (*)();
inline void ApplyMqDataSource(FnSetMqDataSource set, SeedMqSource source) {
    if (source == SeedMqSource::Unsupported || !set || !set(static_cast<int>(source)))
        throw std::runtime_error("Native Master Quest data source binding unavailable");
}
// Binds a seed's source for a generation or import; restores the previous one unless kept, so a failed attempt
// never leaves the bound file with another seed's source.
class MqDataSourceScope {
    FnSetMqDataSource set;
    int previous;
    bool keep = false;

  public:
    MqDataSourceScope(FnGetMqDataSource get, FnSetMqDataSource setter, SeedMqSource source)
        : set(setter), previous(get ? get() : -1) {
        if (previous != static_cast<int>(SeedMqSource::Legacy) && previous != static_cast<int>(SeedMqSource::Bundled))
            throw std::runtime_error("Native Master Quest data source context unavailable");
        ApplyMqDataSource(set, source);
    }
    MqDataSourceScope(const MqDataSourceScope&) = delete;
    MqDataSourceScope& operator=(const MqDataSourceScope&) = delete;
    void Keep() {
        keep = true;
    }
    ~MqDataSourceScope() {
        if (!keep && set)
            set(previous);
    }
};

} // namespace ComboMq
