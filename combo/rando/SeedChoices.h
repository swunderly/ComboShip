#pragma once
// The random choices of a seed's OoT settings finalization (soh settings.cpp Context::FinalizeSettings) that SoH
// draws from its shared rando stream (rng.h rando_state), and the record a seed keeps of them:
// - Key Rings: a Random or Count key ring setting, or a Selection with a dungeon set to Random;
// - Ganon's Trials: a random number of trials, or a set number other than none or all (which trials);
// - the starting age, when it is Random;
// - the Master Quest dungeons of a seed without OoTMM's bundled MQ data. A bundled seed rolls its own from the seed
//   and records them already (combo/mq/MqMode.h MqRollSeed, MqSeedMarker.h).
// SoH draws them wherever the shared stream happens to be: a finalize after another draw, the reachability oracle's
// own first finalize (OTRGlobals.cpp EnsureOracleInit), an app reload or a headless replay drew them again,
// differently, while each reload of an older seed still does.
//
// A seed generated from now on draws each one that its settings make random from a stream of its own, named for the
// choice and the combo master seed (Mode::Seeded), so every finalize of the seed resolves it the same, and records
// them in its "oot" section under kKey. Its reload and its replay finalize with that record, which wins over any draw.
// A seed whose settings make none of them random records nothing and resolves exactly as before: it stays byte for
// byte the seed it was. A seed without the record, made before it or without a random choice, finalizes as SoH always
// has (Mode::Legacy), so an older seed with a random choice re-rolls it on each load exactly as it did; nothing
// migrates it.
//
// Pure C++17 with nlohmann::json and no game headers, so host tests check it. The option values below are SoH's
// (randomizerEnums/RandomizerOptions.h, checked against them in settings.cpp).
#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace ComboRando::SeedChoices {

inline constexpr const char* kKey = "resolvedChoices";
inline constexpr int kVersion = 1;

// How a finalize resolves its random choices.
enum class Mode : uint8_t {
    Legacy = 0, // SoH's rule: the shared rando stream. A seed without the record, and every finalize outside one.
    Seeded = 1, // each random choice from a stream of its own (RollSeed), and a record, when there is one, wins.
};

// The choices, and the stream each is drawn from in a Seeded finalize.
enum class Choice : uint8_t { KeyRings, Trials, StartingAge };
inline constexpr const char* ChoiceStream(Choice choice) {
    return choice == Choice::KeyRings ? "keyRings:soh-finalize-v1:"
           : choice == Choice::Trials ? "trials:soh-finalize-v1:"
                                      : "startingAge:soh-finalize-v1:";
}
// FNV-1a of "<stream><combo master seed in decimal>", as ComboMq::MqRollSeed names the Master Quest stream. A Legacy
// seed's random Master Quest dungeons are drawn from that MQ stream, as a bundled seed's are.
constexpr uint32_t RollSeed(Choice choice, uint64_t comboSeed) {
    uint32_t hash = 2166136261u;
    for (const char* c = ChoiceStream(choice); *c != '\0'; c++)
        hash = (hash ^ static_cast<unsigned char>(*c)) * 16777619u;
    char digits[20] = {};
    int count = 0;
    do {
        digits[count++] = static_cast<char>('0' + comboSeed % 10);
        comboSeed /= 10;
    } while (comboSeed != 0);
    while (count > 0)
        hash = (hash ^ static_cast<unsigned char>(digits[--count])) * 16777619u;
    return hash;
}

// Whether a finalize draws the choice, from the option values it reaches the choice with (after its own forcing).
// Key Rings: RSK_KEYRINGS (0 off, 1 random, 2 count, 3 selection), the count, how many rings a random or count
// setting draws among (8 dungeons, 9 with Gerudo Fortress) and whether a Selection sets one of the 8 dungeons to Random
// (a Selection's Gerudo Fortress Random is never drawn: item_pool.cpp reads any value but No as a ring). A count of
// none or all draws a shuffle but resolves the same rings every time, so it is no random choice.
constexpr bool KeyRingsRandom(int mode, int count, int rings, bool anyDungeonRandom) {
    return mode == 1 || (mode == 2 && count > 0 && count < rings) || (mode == 3 && anyDungeonRandom);
}
// Trials: RSK_GANONS_TRIALS (0 skip, 1 set number, 2 random number), RSK_TRIAL_COUNT and the number of trials. SoH
// shuffles the trials on every finalize; only a count other than none or all makes which ones a choice.
constexpr bool TrialsRandom(int mode, int count, int trials) {
    return mode == 2 || (mode == 1 && count > 0 && count < trials);
}
// RSK_STARTING_AGE: 2 is Random.
constexpr bool StartingAgeRandom(int age) {
    return age == 2;
}
// Master Quest dungeons, for a seed without the bundled data: RSK_MQ_DUNGEON_RANDOM (0 none, 1 set number, 2 random
// number, 3 selection) as the finalize reaches its roll, RSK_MQ_DUNGEON_COUNT, RSK_MQ_DUNGEON_SET, and the 12 dungeons'
// own settings: how many are Random (2) and how many MQ (1). Mirrors the roll: a set number of all or none of the
// dungeons it draws among resolves the same dungeons every time.
constexpr bool MasterQuestRandom(int mode, int count, bool set, int randomDungeons, int mqDungeons) {
    if (mode == 3)
        return randomDungeons > 0;
    if (mode != 1 && mode != 2)
        return false;
    const int candidates = set ? randomDungeons : 12, fixed = set ? mqDungeons : 0;
    if (candidates == 0)
        return false;
    if (mode == 2)
        return true;
    const int toSet = count > fixed ? (count - fixed < candidates ? count - fixed : candidates) : 0;
    return toSet > 0 && toSet < candidates;
}

// What a finalize resolved, one field for each choice its settings made random.
struct Choices {
    struct KeyRings {
        uint16_t dungeons = 0;       // DungeonKey bits (dungeon.h; Forest 3 .. Ganon's Castle 11) with a key ring
        bool gerudoFortress = false; // RSK_KEYRINGS_GERUDO_FORTRESS ends on Yes
        bool operator==(const KeyRings& other) const {
            return dungeons == other.dungeons && gerudoFortress == other.gerudoFortress;
        }
    };
    std::optional<KeyRings> keyRings;
    std::optional<uint8_t> requiredTrials; // TrialKey bits (Light 0 .. Shadow 5)
    std::optional<bool> adult;             // RSK_SELECTED_STARTING_AGE
    std::optional<uint16_t> masterQuest;   // DungeonKey bits; only a seed without the bundled MQ record
    bool Empty() const {
        return !keyRings && !requiredTrials && !adult && !masterQuest;
    }
    bool operator==(const Choices& other) const {
        return keyRings == other.keyRings && requiredTrials == other.requiredTrials && adult == other.adult &&
               masterQuest == other.masterQuest;
    }
    bool operator!=(const Choices& other) const {
        return !(*this == other);
    }
};

inline constexpr uint16_t kKeyRingDungeons = 0xDF8; // Forest, Fire, Water, Spirit, Shadow, BotW, GTG, Ganon's Castle
inline constexpr int kTrialCount = 6;
inline constexpr int kDungeonCount = 12;

inline nlohmann::json Bits(uint32_t bits, int count) {
    auto ids = nlohmann::json::array();
    for (int id = 0; id < count; id++)
        if (bits & (1u << id))
            ids.push_back(id);
    return ids;
}
inline uint32_t BitsOf(const nlohmann::json& ids, int count, const char* what) {
    if (!ids.is_array())
        throw std::invalid_argument(std::string("Malformed resolved ") + what);
    uint32_t bits = 0;
    for (const auto& id : ids) {
        if (!id.is_number_integer() || id.is_boolean() || id.get<int64_t>() < 0 || id.get<int64_t>() >= count)
            throw std::invalid_argument(std::string("Unknown resolved ") + what);
        const uint32_t bit = 1u << id.get<int>();
        if (bits & bit)
            throw std::invalid_argument(std::string("Duplicate resolved ") + what);
        bits |= bit;
    }
    return bits;
}

// The record of `choices`; an empty object when the settings made nothing random (a seed then records nothing).
inline nlohmann::json Encode(const Choices& choices) {
    nlohmann::json record = nlohmann::json::object();
    if (choices.Empty())
        return record;
    record["version"] = kVersion;
    if (choices.keyRings)
        record["keyRings"] = { { "dungeons", Bits(choices.keyRings->dungeons, kDungeonCount) },
                               { "gerudoFortress", choices.keyRings->gerudoFortress } };
    if (choices.requiredTrials)
        record["requiredTrials"] = Bits(*choices.requiredTrials, kTrialCount);
    if (choices.adult)
        record["startingAge"] = *choices.adult ? "adult" : "child";
    if (choices.masterQuest)
        record["masterQuestDungeons"] = Bits(*choices.masterQuest, kDungeonCount);
    return record;
}
// The choices of a record; an empty object is none. Throws std::invalid_argument for anything else this build does
// not know, so a load never drops a choice it cannot read.
inline Choices Decode(const nlohmann::json& record) {
    if (!record.is_object())
        throw std::invalid_argument("Malformed resolved choices");
    Choices choices;
    if (record.empty())
        return choices;
    for (const auto& item : record.items())
        if (item.key() != "version" && item.key() != "keyRings" && item.key() != "requiredTrials" &&
            item.key() != "startingAge" && item.key() != "masterQuestDungeons")
            throw std::invalid_argument("Unknown resolved choice " + item.key());
    const auto version = record.find("version");
    if (version == record.end() || !version->is_number_integer() || version->is_boolean() || *version != kVersion)
        throw std::invalid_argument("Unsupported resolved choices version");
    if (const auto it = record.find("keyRings"); it != record.end()) {
        if (!it->is_object() || it->size() != 2 || !it->contains("gerudoFortress") ||
            !it->at("gerudoFortress").is_boolean())
            throw std::invalid_argument("Malformed resolved key rings");
        Choices::KeyRings rings;
        rings.dungeons =
            static_cast<uint16_t>(BitsOf(it->value("dungeons", nlohmann::json()), kDungeonCount, "key ring"));
        if (rings.dungeons & ~kKeyRingDungeons)
            throw std::invalid_argument("Unknown resolved key ring");
        rings.gerudoFortress = it->at("gerudoFortress").get<bool>();
        choices.keyRings = rings;
    }
    if (const auto it = record.find("requiredTrials"); it != record.end())
        choices.requiredTrials = static_cast<uint8_t>(BitsOf(*it, kTrialCount, "trial"));
    if (const auto it = record.find("startingAge"); it != record.end()) {
        if (!it->is_string() || (*it != "child" && *it != "adult"))
            throw std::invalid_argument("Malformed resolved starting age");
        choices.adult = *it == "adult";
    }
    if (const auto it = record.find("masterQuestDungeons"); it != record.end())
        choices.masterQuest = static_cast<uint16_t>(BitsOf(*it, kDungeonCount, "Master Quest dungeon"));
    if (choices.Empty())
        throw std::invalid_argument("Malformed resolved choices");
    return choices;
}

// The record a seed document's "oot" section keeps, or none for a seed without one (Legacy).
inline std::optional<Choices> RecordedChoices(const nlohmann::json& oot) {
    if (!oot.is_object())
        throw std::invalid_argument("Resolved choices require an object");
    const auto it = oot.find(kKey);
    if (it == oot.end())
        return std::nullopt;
    const auto choices = Decode(*it);
    if (choices.Empty())
        throw std::invalid_argument("Malformed resolved choices");
    return choices;
}
inline Mode ModeForRecord(const std::optional<Choices>& record) {
    return record ? Mode::Seeded : Mode::Legacy;
}
// Stamps a new seed document's "oot" section with what its generation resolved; nothing when none was random.
inline void StampSeedChoices(nlohmann::json& oot, const nlohmann::json& resolved) {
    const auto choices = Decode(resolved);
    if (choices.Empty())
        oot.erase(kKey);
    else
        oot[kKey] = Encode(choices);
}
// Whether a reload or replay resolved the choices its seed records. Always true for a seed without the record.
inline bool ReplayKeepsChoices(const nlohmann::json& oot, const nlohmann::json& resolved) {
    const auto record = RecordedChoices(oot);
    return !record || Decode(resolved) == *record;
}
// Whether a seed without the record has settings that make a choice random (`resolved`, what its Legacy finalize drew
// from the shared stream): its reload and its replay re-roll that choice, as they always have.
inline bool ReRollsChoices(const nlohmann::json& oot, const nlohmann::json& resolved) {
    return !RecordedChoices(oot) && !Decode(resolved).Empty();
}

// The native context's mode and record for a launcher scope (soh OTRGlobals.cpp SOH_GetSeedChoicesMode,
// SOH_GetSeedChoicesRecord, SOH_SetSeedChoices).
using FnGetMode = int (*)();
using FnGetRecord = const char* (*)();
using FnSetChoices = int (*)(int mode, const char* record);
inline void ApplySeedChoices(FnSetChoices set, Mode mode, const std::optional<Choices>& record) {
    const auto text = record ? Encode(*record).dump() : std::string("{}");
    if (!set || !set(static_cast<int>(mode), text.c_str()))
        throw std::runtime_error("Native resolved choices context unavailable");
}
// Sets a mode and record for one generation, reload or replay and puts the previous ones back, unless kept.
class Scope {
    FnSetChoices set;
    int previousMode;
    std::string previousRecord;
    bool keep = false;

  public:
    Scope(FnGetMode getMode, FnGetRecord getRecord, FnSetChoices setter, Mode mode,
          const std::optional<Choices>& record)
        : set(setter), previousMode(getMode ? getMode() : -1) {
        const char* text = getRecord ? getRecord() : nullptr;
        if ((previousMode != static_cast<int>(Mode::Legacy) && previousMode != static_cast<int>(Mode::Seeded)) || !text)
            throw std::runtime_error("Native resolved choices context unavailable");
        previousRecord = text;
        ApplySeedChoices(set, mode, record);
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    void Keep() {
        keep = true;
    }
    ~Scope() {
        if (!keep && set)
            set(previousMode, previousRecord.c_str());
    }
};

} // namespace ComboRando::SeedChoices
