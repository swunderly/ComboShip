"""A seed's random OoT settings choices are drawn from the seed, recorded, and used by its reload and replay.

SoH's settings finalize (settings.cpp Context::FinalizeSettings) draws Key Rings, Ganon's Trials, a Random starting age
and, without OoTMM's bundled data, Master Quest dungeons from the shared rando stream wherever it happens to be. In the
0.4.13 RC1 corpus the Allsanity row d06 (Key Ring Selection with every dungeon Random, Starting Age Random) generated
with the oracle's finalize resolving a child start, while the headless replay's last finalize resolved an adult one
and stuck at sphere 3; an app reload of such a seed resolves its own (rando/SeedChoices.h).

This harness compiles the production lines of that finalize (the choice preamble through the starting age), SoH's
random number generator (rng.h, ShipUtils), the launcher's seed accessor and the seed-choice exports (OTRGlobals.cpp),
the context's choice state (SeedContext.h) and the record (combo/rando/SeedChoices.h), and checks:
- a Legacy finalize, every seed without the record, draws as RC1 does: from the shared stream, re-rolling;
- a Seeded finalize, a new seed's generation, draws each random choice from the seed alone: the dump's finalize and
  the oracle's agree wherever the shared stream is, and the shared stream ends where it would without those choices;
- for settings with no random choice, a Seeded finalize is a Legacy one, value for value and stream state for stream
  state, so such a seed generates byte for byte as before and records nothing;
- the record of a generation, handed to a reload or replay in another process with another shared stream, is what
  that finalize uses, and a record a seed's settings do not make random is refused;
- Allsanity's own settings (ComboPresets.h) make the Key Rings and the starting age random, with even odds, and d06's
  master seed resolves what the host build of the generator records for it;
and that the launcher (RunComboFill, the reload, Combo_FinalizeGenerate) and the headless tool (generation and
--playthrough) bind and record the choices in that order.
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
repo, out = args.source.resolve(), args.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def read(path):
    return (repo / path).read_text(encoding='utf-8-sig')


def between(text, start, end, at=0):
    first = hostcxx.find(text, start, at)
    return text[first:hostcxx.find(text, end, first + 1)]


def function(text, signature):
    first = hostcxx.find(text, signature)
    depth, at = 0, text.index('{', first)
    while True:
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            return text[first:at + 1] + '\n'
        at += 1


def ordered(text, *needles):
    at, positions = 0, []
    for needle in needles:
        try:
            at = hostcxx.find(text, needle, at)
        except ValueError:
            failures.append(f'out of order or missing: {needle}')
            return positions
        positions.append(at)
        at += 1
    return positions


settings = read('soh/soh/Enhancements/randomizer/settings.cpp')
finalize = function(settings, 'void Context::FinalizeSettings(')
preamble = between(finalize, '#ifdef COMBO_BUILD // ComboShip: the random choices below',
                   'std::array<DungeonInfo*, 12> dungeons = this->GetDungeons()->GetDungeonList();')
choices = between(finalize, 'std::array<DungeonInfo*, 12> dungeons = this->GetDungeons()->GetDungeonList();',
                  '// TODO: Random Starting Time')
check(hostcxx.contains(finalize, 'const bool bundledMq = GetMqDataSource() == ComboMq::SeedMqSource::Bundled;'),
      'FinalizeSettings: the bundled-MQ flag the choices read')
seed_context = read('soh/soh/Enhancements/randomizer/SeedContext.h')
context_methods = between(seed_context, '// ComboShip: how FinalizeSettings resolves its random choices',
                          'bool IsLocationInCurrentCatalog(')
context_methods = context_methods[:context_methods.rindex('#endif')]
context_members = between(seed_context, 'ComboRando::SeedChoices::Mode mSeedChoicesMode', '#endif')
dungeon_keys = between(read('soh/soh/Enhancements/randomizer/dungeon.h'), 'typedef enum { DEKU_TREE,', '} DungeonKey;')

rng = read('soh/soh/Enhancements/randomizer/rng.h')
rng = '\n'.join(line for line in rng.splitlines() if not line.startswith(('#pragma', '#include')))
ship_h = between(read('soh/soh/ShipUtils.h'), 'namespace ShipUtils {', '} // namespace ShipUtils') + '}\n'
ship_cpp = between(read('soh/soh/ShipUtils.cpp'), 'static bool default_init = false;', 'double ShipUtils::RandomDouble(')
otr = read('soh/soh/OTRGlobals.cpp')
seed_statics = between(otr, 'static uint64_t sComboRandoSeed = 0;', '// Reseed SoH\'s rando RNG')
seed_set = function(otr, 'extern "C" COMBO_EXPORT void SOH_SetComboRandoSeed(uint64_t seed)')
seed_get = function(otr, 'bool Combo_GetRandoSeed(uint64_t* seed)')
exports = ''.join(function(otr, signature) for signature in (
    'extern "C" COMBO_EXPORT int SOH_GetSeedChoicesMode(void)',
    'extern "C" COMBO_EXPORT const char* SOH_GetSeedChoicesRecord(void)',
    'extern "C" COMBO_EXPORT int SOH_SetSeedChoices(int mode, const char* record)',
    'extern "C" COMBO_EXPORT const char* SOH_DumpSeedChoices(void)'))
# The CVar of each option the choices read, for applying the community presets (SetAllToContext copies by key).
cvars = dict((cvar, key) for key, cvar in re.findall(
    r'OPT_(?:U8|BOOL)\((RSK_(?:KEYRINGS\w*|GERUDO_FORTRESS|GERUDO_KEYS|GANONS_TRIALS|TRIAL_COUNT|STARTING_AGE)),'
    r'[^;]*?CVAR_RANDOMIZER_SETTING\("(\w+)"\)', settings))
check(len(cvars) == 16, f'settings.cpp: expected the 16 options the choices read, found {sorted(cvars.values())}')
cvar_rows = ',\n'.join(f'    {{ "gRandoSettings.{cvar}", {key} }}' for cvar, key in sorted(cvars.items()))

harness = r'''
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#define COMBO_EXPORT
#include "randomizerEnums.h"
#include "mq/MqMode.h"
#include "rando/ComboPresets.h"
#include "rando/SeedChoices.h"
#define MAX_MQ_DUNGEON_COUNT 12

namespace ShipUtils {
''' + ship_h.split('{', 1)[1] + ship_cpp + rng + r'''
''' + seed_statics + seed_set + seed_get + r'''

typedef enum {
''' + dungeon_keys.split('{', 1)[1] + r'''
} DungeonKey;
struct OptionValue {
    uint8_t value = 0;
    bool Is(uint32_t other) const { return value == other; }
    bool IsNot(uint32_t other) const { return value != other; }
    void Set(uint32_t other) { value = static_cast<uint8_t>(other); }
    uint8_t Get() const { return value; }
    explicit operator bool() const { return value != 0; }
};
struct DungeonInfo {
    RandomizerSettingKey setting;
    bool mq = false, known = true, ring = false;
    void ClearMQ() { mq = false; }
    void SetMQ() { mq = true; }
    bool IsMQ() const { return mq; }
    void SetDungeonKnown(bool value) { known = value; }
    RandomizerSettingKey GetMQSetting() const { return setting; }
    void SetKeyRing() { ring = true; }
    void ClearKeyRing() { ring = false; }
    bool HasKeyRing() const { return ring; }
};
struct Dungeons {
    std::array<DungeonInfo, 12> list;
    std::array<DungeonInfo*, 12> GetDungeonList() {
        std::array<DungeonInfo*, 12> result{};
        for (size_t i = 0; i < list.size(); i++)
            result[i] = &list[i];
        return result;
    }
    size_t CountMQ() {
        return std::count_if(list.begin(), list.end(), [](const DungeonInfo& d) { return d.mq; });
    }
};
struct TrialInfo {
    TrialKey key = TK_LIGHT_TRIAL;
    bool required = false;
    void SetAsSkipped() { required = false; }
    void SetAsRequired() { required = true; }
    bool IsRequired() const { return required; }
    TrialKey GetTrialKey() const { return key; }
};
struct Trials {
    std::array<TrialInfo, TK_MAX> list;
    Trials() {
        for (int i = 0; i < TK_MAX; i++)
            list[i].key = static_cast<TrialKey>(i);
    }
    std::vector<TrialInfo*> GetTrialList() {
        std::vector<TrialInfo*> result;
        for (auto& trial : list)
            result.push_back(&trial);
        return result;
    }
    void SkipAll() {
        for (auto& trial : list)
            trial.SetAsSkipped();
    }
};
namespace Rando {
// settings.cpp RSK_TRIAL_COUNT: "Ganon's Trials Count", NumOpts(0, 6).
struct Settings {
    struct Count {
        size_t GetOptionCount() const { return 7; }
    };
    Count GetOption(RandomizerSettingKey) { return {}; }
    static Settings* GetInstance() {
        static Settings settings;
        return &settings;
    }
};
class Context {
  public:
    std::map<int, OptionValue> mOptions;
    Dungeons dungeonStore;
    std::shared_ptr<Trials> trials = std::make_shared<Trials>();
    bool bundledMq = false;
    Context() {
        for (int i = 0; i < 12; i++)
            dungeonStore.list[i].setting = static_cast<RandomizerSettingKey>(RSK_MQ_DEKU_TREE + i);
    }
    Dungeons* GetDungeons() { return &dungeonStore; }
    DungeonInfo* GetDungeon(size_t key) { return &dungeonStore.list[key]; }
    std::shared_ptr<Trials> GetTrials() { return trials; }
    TrialInfo* GetTrial(TrialKey key) { return &trials->list[key]; }
    static std::shared_ptr<Context> GetInstance() { return instance; }
    static std::shared_ptr<Context> instance;
''' + context_methods + r'''
    // settings.cpp Context::FinalizeSettings: the choice preamble, the MQ roll, Key Rings, Ganon's Trials, the
    // entrance pool flags and the starting age.
    void FinalizeChoices() {
''' + preamble + choices + r'''
    }
  private:
''' + context_members + r'''
};
std::shared_ptr<Context> Context::instance = std::make_shared<Context>();
} // namespace Rando
''' + exports + r'''

namespace SC = ComboRando::SeedChoices;
using Values = std::map<int, int>;
static Rando::Context& Ctx() { return *Rando::Context::instance; }
static void Require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}
static const std::map<std::string, int> kCvars = {
''' + cvar_rows + r'''
};
// What a finalize leaves that a seed's play reads: every option it touched, each dungeon's key ring and quest, the
// required trials, the choices it reported, and the shared stream.
struct Outcome {
    std::map<int, int> options;
    std::array<int, 12> dungeons{};
    int trials = 0;
    nlohmann::json resolved;
    uint64_t stream = 0;
    bool operator==(const Outcome& other) const {
        return options == other.options && dungeons == other.dungeons && trials == other.trials &&
               resolved == other.resolved && stream == other.stream;
    }
    bool SamePlay(const Outcome& other) const {
        return options == other.options && dungeons == other.dungeons && trials == other.trials &&
               resolved == other.resolved;
    }
};
static Outcome Finalize(const Values& rules, uint64_t shared) {
    for (const auto& [key, value] : rules)
        Ctx().mOptions[key].Set(value);
    rando_state = shared;
    Ctx().FinalizeChoices();
    Outcome outcome;
    for (const auto& [key, value] : Ctx().mOptions)
        outcome.options[key] = value.Get();
    for (int i = 0; i < 12; i++)
        outcome.dungeons[i] = (Ctx().dungeonStore.list[i].ring ? 1 : 0) | (Ctx().dungeonStore.list[i].mq ? 2 : 0);
    for (int i = 0; i < TK_MAX; i++)
        outcome.trials |= Ctx().trials->list[i].required ? 1 << i : 0;
    outcome.resolved = nlohmann::json::parse(SOH_DumpSeedChoices());
    outcome.stream = rando_state;
    return outcome;
}
static void Bind(SC::Mode mode, const nlohmann::json& record = nlohmann::json::object()) {
    Require(SOH_SetSeedChoices(static_cast<int>(mode), record.dump().c_str()) == 1, "the context takes the mode");
}
// The finalize's inputs: SoH's menu defaults (Key Rings off, Normal carpenters, vanilla Gerudo keys, trials Set Number
// 6, a child start), then the given values.
static Values Rules(std::initializer_list<std::pair<const int, int>> values) {
    Values rules{ { RSK_KEYRINGS, RO_KEYRINGS_OFF }, { RSK_KEYRINGS_RANDOM_COUNT, 8 },
                  { RSK_GERUDO_FORTRESS, RO_GF_CARPENTERS_NORMAL }, { RSK_GERUDO_KEYS, RO_GERUDO_KEYS_VANILLA },
                  { RSK_GANONS_TRIALS, RO_GANONS_TRIALS_SET_NUMBER }, { RSK_TRIAL_COUNT, 6 },
                  { RSK_STARTING_AGE, RO_AGE_CHILD }, { RSK_SELECTED_STARTING_AGE, RO_AGE_CHILD },
                  { RSK_MQ_DUNGEON_RANDOM, RO_MQ_DUNGEONS_NONE }, { RSK_MQ_DUNGEON_COUNT, 0 },
                  { RSK_MQ_DUNGEON_SET, 0 } };
    for (int key = RSK_KEYRINGS_GERUDO_FORTRESS; key <= RSK_KEYRINGS_GANONS_CASTLE; key++)
        rules[key] = RO_KEYRING_FOR_DUNGEON_OFF;
    for (int i = 0; i < 12; i++)
        rules[RSK_MQ_DEKU_TREE + i] = RO_MQ_SET_VANILLA;
    for (const auto& [key, value] : values)
        rules[key] = value;
    return rules;
}
static Values Preset(ComboRando::CommunityPreset preset) {
    Values rules = Rules({});
    for (const auto& option : ComboRando::CommunityPresetOptions(preset))
        if (const auto row = kCvars.find(option.key); row != kCvars.end())
            rules[row->second] = option.value;
    return rules;
}

int main() {
    try {
        static_assert(RSK_KEYRINGS_GANONS_CASTLE - RSK_KEYRINGS_GERUDO_FORTRESS == 8, "the nine key ring options");
        std::mt19937_64 garbage(413);
        const Values allsanity = Preset(ComboRando::CommunityPreset::Allsanity);
        Require(allsanity.at(RSK_KEYRINGS) == RO_KEYRINGS_SELECTION && allsanity.at(RSK_STARTING_AGE) == RO_AGE_RANDOM,
                "Allsanity: Key Ring Selection and a Random starting age (presets.ts:444, :512)");
        for (int key = RSK_KEYRINGS_GERUDO_FORTRESS; key <= RSK_KEYRINGS_GANONS_CASTLE; key++)
            Require(allsanity.at(key) == RO_KEYRING_FOR_DUNGEON_RANDOM, "Allsanity: every key ring a coin flip");
        const Values beginner = Preset(ComboRando::CommunityPreset::Beginner);
        Require(beginner.at(RSK_STARTING_AGE) == RO_AGE_RANDOM, "Beginner: a Random starting age (presets.ts:975)");

        // Legacy, every seed without the record: RC1's draws, from the shared stream. The same seed re-rolls with the
        // stream, as d06's replay did; the finalize reports what it drew.
        Bind(SC::Mode::Legacy);
        SOH_SetComboRandoSeed(419070006);
        std::set<std::string> legacyDraws;
        for (int i = 0; i < 64; i++) {
            const uint64_t shared = garbage();
            const auto outcome = Finalize(allsanity, shared);
            Require(outcome.stream != shared, "a Legacy finalize draws from the shared stream");
            Require(outcome.resolved.contains("keyRings") && outcome.resolved.contains("startingAge"),
                    "a Legacy finalize reports its random choices");
            legacyDraws.insert(outcome.resolved.dump());
        }
        Require(legacyDraws.size() > 32, "a Legacy finalize re-rolls Allsanity's choices with the shared stream");

        // Seeded, a new seed's generation: each master seed resolves one set of choices, whatever the shared stream,
        // and leaves the stream where the same finalize without those choices would.
        Bind(SC::Mode::Seeded);
        std::map<int, int> rings, adults;
        std::set<std::string> perSeed;
        const int kSeeds = 400;
        const Values fixedAllsanity = [&] {
            Values rules = allsanity;
            for (int key = RSK_KEYRINGS_GERUDO_FORTRESS; key <= RSK_KEYRINGS_GANONS_CASTLE; key++)
                rules[key] = RO_KEYRING_FOR_DUNGEON_OFF;
            rules[RSK_STARTING_AGE] = RO_AGE_CHILD;
            return rules;
        }();
        for (int s = 0; s < kSeeds; s++) {
            const uint64_t master = 419070000ull + 7919ull * s;
            SOH_SetComboRandoSeed(master);
            const uint64_t shared = garbage();
            const auto dump = Finalize(allsanity, shared);
            const auto oracle = Finalize(allsanity, garbage());
            Require(dump.SamePlay(oracle), "the oracle's finalize resolves the dump's choices");
            Bind(SC::Mode::Legacy);
            const auto without = Finalize(fixedAllsanity, shared);
            Bind(SC::Mode::Seeded);
            Require(dump.stream == without.stream, "the Seeded choices leave the shared stream as it would be");
            const auto record = SC::Decode(dump.resolved);
            Require(record.keyRings && record.adult && !record.requiredTrials && !record.masterQuest,
                    "Allsanity's random choices are its key rings and its starting age");
            Require(record.keyRings->gerudoFortress, "a Selection's Random Gerudo Fortress ring is always a ring");
            for (int d = 0; d < 12; d++)
                rings[d] += (record.keyRings->dungeons >> d) & 1;
            adults[*record.adult] += 1;
            perSeed.insert(dump.resolved.dump());

            // The seed records them; its reload or replay, in another process, at another shared stream and with
            // another seed's draw, finalizes with the record, which wins.
            nlohmann::json oot = nlohmann::json::object();
            SC::StampSeedChoices(oot, dump.resolved);
            Require(oot.at(SC::kKey) == dump.resolved, "the seed records what its generation resolved");
            SOH_SetComboRandoSeed(master ^ 0xA5A5A5A5ull);
            Bind(SC::ModeForRecord(SC::RecordedChoices(oot)), oot.at(SC::kKey));
            const auto reload = Finalize(allsanity, garbage());
            Require(reload.SamePlay(dump), "a reload with the record resolves the generation's choices");
            Require(SC::ReplayKeepsChoices(oot, reload.resolved), "the reload keeps the recorded choices");
            Require(nlohmann::json::parse(SOH_GetSeedChoicesRecord()) == oot.at(SC::kKey),
                    "the context keeps the bound record");
            Bind(SC::Mode::Seeded);
        }
        for (const auto& [dungeon, count] : rings) {
            const bool ringDungeon = (SC::kKeyRingDungeons >> dungeon) & 1;
            Require(ringDungeon ? (count > kSeeds * 3 / 10 && count < kSeeds * 7 / 10) : count == 0,
                    "dungeon " + std::to_string(dungeon) + " has a key ring in " + std::to_string(count) + " seeds");
        }
        Require(adults[0] > kSeeds * 3 / 10 && adults[1] > kSeeds * 3 / 10, "the starting age is a coin flip");
        // 256 ring sets and two ages: 400 seeds give about 277 distinct choices.
        Require(perSeed.size() > kSeeds / 2, "the choices differ from seed to seed");

        // Settings without a random choice: a Seeded finalize is a Legacy one, value for value and stream state for
        // stream state, and reports nothing, so such a seed generates exactly as before and records nothing.
        int compared = 0;
        for (int keyRings : { RO_KEYRINGS_OFF, RO_KEYRINGS_COUNT, RO_KEYRINGS_SELECTION })
            for (int count : { 0, 8, 9 })
                for (int dungeon : { RO_KEYRING_FOR_DUNGEON_OFF, RO_KEYRING_FOR_DUNGEON_ON })
                    for (int carpenters : { RO_GF_CARPENTERS_NORMAL, RO_GF_CARPENTERS_FAST })
                        for (int gerudoKeys : { RO_GERUDO_KEYS_VANILLA, RO_GERUDO_KEYS_ANYWHERE })
                            for (int trials : { RO_GANONS_TRIALS_SKIP, RO_GANONS_TRIALS_SET_NUMBER })
                                for (int trialCount : { 0, 6 })
                                    for (int age : { RO_AGE_CHILD, RO_AGE_ADULT }) {
                                        const int rings = 8 + (carpenters == RO_GF_CARPENTERS_NORMAL &&
                                                               gerudoKeys != RO_GERUDO_KEYS_VANILLA);
                                        // A count between none and all is random; the menu caps it at the
                                        // rings (settings.cpp RSK_KEYRINGS_RANDOM_COUNT's callback).
                                        if (keyRings == RO_KEYRINGS_COUNT && count != 0 && count != rings)
                                            continue;
                                        Values rules = Rules({ { RSK_KEYRINGS, keyRings },
                                                               { RSK_KEYRINGS_RANDOM_COUNT, count },
                                                               { RSK_GERUDO_FORTRESS, carpenters },
                                                               { RSK_GERUDO_KEYS, gerudoKeys },
                                                               { RSK_GANONS_TRIALS, trials },
                                                               { RSK_TRIAL_COUNT, trialCount },
                                                               { RSK_STARTING_AGE, age } });
                                        for (int key = RSK_KEYRINGS_GERUDO_FORTRESS;
                                             key <= RSK_KEYRINGS_GANONS_CASTLE; key++)
                                            rules[key] = (key - RSK_KEYRINGS_GERUDO_FORTRESS) % 2 ? dungeon
                                                                                                  : RO_KEYRING_FOR_DUNGEON_ON;
                                        const uint64_t shared = garbage();
                                        SOH_SetComboRandoSeed(compared);
                                        Bind(SC::Mode::Legacy);
                                        const auto legacy = Finalize(rules, shared);
                                        Bind(SC::Mode::Seeded);
                                        const auto seeded = Finalize(rules, shared);
                                        Require(seeded == legacy, "a finalize without a random choice is unchanged");
                                        Require(seeded.resolved == nlohmann::json::object(),
                                                "a finalize without a random choice reports none");
                                        compared++;
                                    }
        Require(compared > 300, "every finalize without a random choice compared");

        // Each random choice alone: Random and Count key rings resolve into each ring's Yes or No, a set number of
        // trials picks which, a random number picks how many, and every one keeps its record.
        const Values singles[] = {
            Rules({ { RSK_KEYRINGS, RO_KEYRINGS_RANDOM }, { RSK_GERUDO_KEYS, RO_GERUDO_KEYS_ANYWHERE } }),
            Rules({ { RSK_KEYRINGS, RO_KEYRINGS_COUNT }, { RSK_KEYRINGS_RANDOM_COUNT, 3 } }),
            Rules({ { RSK_GANONS_TRIALS, RO_GANONS_TRIALS_SET_NUMBER }, { RSK_TRIAL_COUNT, 3 } }),
            Rules({ { RSK_GANONS_TRIALS, RO_GANONS_TRIALS_RANDOM_NUMBER } }),
            Rules({ { RSK_STARTING_AGE, RO_AGE_RANDOM } }),
        };
        for (const auto& rules : singles)
            for (uint64_t s = 1; s <= 32; s++) {
                SOH_SetComboRandoSeed(s);
                Bind(SC::Mode::Seeded);
                const auto generated = Finalize(rules, garbage());
                const auto record = SC::Decode(generated.resolved);
                Require(!record.Empty() && generated.SamePlay(Finalize(rules, garbage())),
                        "each random choice resolves from the seed alone");
                if (rules.at(RSK_KEYRINGS) == RO_KEYRINGS_RANDOM || rules.at(RSK_KEYRINGS) == RO_KEYRINGS_COUNT)
                    for (int key = RSK_KEYRINGS_FOREST_TEMPLE; key <= RSK_KEYRINGS_GANONS_CASTLE; key++)
                        Require(generated.options.at(key) != RO_KEYRING_FOR_DUNGEON_RANDOM, "rings resolve to Yes or No");
                if (rules.at(RSK_KEYRINGS) == RO_KEYRINGS_COUNT)
                    Require(std::popcount(static_cast<unsigned>(record.keyRings->dungeons)) == 3 &&
                                !record.keyRings->gerudoFortress,
                            "a Count of three rings, without Gerudo Fortress's with vanilla Gerudo keys");
                if (rules.at(RSK_GANONS_TRIALS) == RO_GANONS_TRIALS_SET_NUMBER &&
                    rules.at(RSK_TRIAL_COUNT) == 3)
                    Require(std::popcount(static_cast<unsigned>(*record.requiredTrials)) == 3 &&
                                generated.options.at(RSK_TRIAL_COUNT) == 3,
                            "three trials of six");
                nlohmann::json oot = nlohmann::json::object();
                SC::StampSeedChoices(oot, generated.resolved);
                SOH_SetComboRandoSeed(s + 1000);
                Bind(SC::Mode::Seeded, oot.at(SC::kKey));
                const auto reload = Finalize(rules, garbage());
                Require(reload.SamePlay(generated) && SC::ReplayKeepsChoices(oot, reload.resolved),
                        "the record wins over another seed's draw");
            }

        // A record the seed's settings do not make random is refused: the finalize does not report that choice.
        {
            SC::Choices adultOnly;
            adultOnly.adult = true;
            nlohmann::json oot = { { SC::kKey, SC::Encode(adultOnly) } };
            Bind(SC::Mode::Seeded, oot.at(SC::kKey));
            const auto fixedChild = Finalize(Rules({}), garbage());
            Require(fixedChild.options.at(RSK_SELECTED_STARTING_AGE) == RO_AGE_CHILD &&
                        !SC::ReplayKeepsChoices(oot, fixedChild.resolved),
                    "a record for a fixed starting age is refused");
            // Nor does a Legacy context take a record.
            Require(SOH_SetSeedChoices(static_cast<int>(SC::Mode::Legacy), oot.at(SC::kKey).dump().c_str()) == 0 &&
                        SOH_GetSeedChoicesMode() == static_cast<int>(SC::Mode::Seeded),
                    "a Legacy context takes no record");
            Require(SOH_SetSeedChoices(7, "{}") == 0 && SOH_SetSeedChoices(1, "{\"future\":1}") == 0,
                    "an unknown mode or record changes nothing");
        }

        // d06, the corpus row: the host build of this generator records Forest, Fire, Water, Spirit, Shadow, Bottom
        // of the Well and Ganon's Castle rings, Gerudo Fortress's, and a child start for master seed 419070006.
        Bind(SC::Mode::Seeded);
        SOH_SetComboRandoSeed(419070006);
        const auto d06 = Finalize(allsanity, garbage());
        std::printf("d06 resolves %s\n", d06.resolved.dump().c_str());
        Require(d06.resolved == nlohmann::json::parse(
                                    R"({"version":1,"keyRings":{"dungeons":[3,4,5,6,7,8,11],"gerudoFortress":true},)"
                                    R"("startingAge":"child"})"),
                "d06's choices are the generator's");
        std::printf("PASS: %d Allsanity seeds, %d unchanged finalizes, Legacy re-rolls %zu ways\n", kSeeds, compared,
                    legacyDraws.size());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    return 0;
}
'''

if not failures:
    cpp = out / 'seed_choices_finalize.cpp'
    cpp.write_text(harness, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, std='c++20', warnings=False,
                        includes=[repo / 'soh/soh/Enhancements/randomizer', repo / 'combo', args.json_include],
                        defines=['COMBO_BUILD'], msvc=['/Zc:preprocessor', '/utf-8', '/bigobj'])
    run = subprocess.run([str(exe)], cwd=out, capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    print(run.stdout + run.stderr, end='')
    check(run.returncode == 0, 'the finalize harness failed')

# The launcher: a generation draws the choices from the seed and records them; a reload binds the seed's record before
# its finalize and refuses a seed whose finalize did not keep it; a finished generation keeps its own bound.
ship = read('combo/ComboShip.cpp')
fill = function(ship, 'static void RunComboFill(std::string inputSeed, ComboRando::ComboGenProgress* progress,')
ordered(fill, 'ComboMq::MqDataSourceScope mqDataSource(',
        'ComboRando::SeedChoices::Scope seedChoices(SOH_GetSeedChoicesMode, SOH_GetSeedChoicesRecord, '
        'SOH_SetSeedChoices, ComboRando::SeedChoices::Mode::Seeded, std::nullopt);',
        'for (uint64_t attempt = 0;', 'SOH_DumpRandoStaticData()', 'ComboMq::StampSeedMq(consolidated["oot"],',
        'ComboRando::SeedChoices::StampSeedChoices(consolidated["oot"], ResolvedSeedChoices());',
        'g_ConsolidatedJson = consolidated.dump(2);')
reload = function(ship, 'static int Combo_OnReloadRequestImpl(')
ordered(reload, 'ComboRando::SeedChoices::RecordedChoices(j.value("oot", nlohmann::json::object()))',
        'ComboRando::SeedChoices::ModeForRecord(recordedChoices)', 'SOH_RestoreRandoSettings(ootSettings.c_str());',
        'SOH_PrepRandoContext();', 'ComboMq::ReplayKeepsMqDungeons(oot,',
        'ComboRando::SeedChoices::ReplayKeepsChoices(oot, resolvedChoices)', 'return 0;',
        'sProfileApplyResolvedRules(oot["resolvedSeedRules"]', 'SOH_ShuffleEntrancesForCombo(masterSeed)',
        'CheckPendingEntranceSeed(j)', 'seedChoices.Keep();')
finish = function(ship, 'static void Combo_FinalizeGenerate()')
ordered(finish, 'ComboRando::SeedChoices::RecordedChoices(seed.value("oot", nlohmann::json::object()))',
        'std::make_unique<ComboRando::SeedChoices::Scope>(', 'seedChoices->Keep();')
for tool in ('static int RunComboGenTest(', 'static void RunComboPlaythrough('):
    body = function(ship, tool)
    check(hostcxx.contains(body, 'BindNewSeedMqSource(') == hostcxx.contains(body, 'BindNewSeedChoices('),
          f'{tool} binds a new seed\'s choices exactly where it binds its MQ source')
check(hostcxx.contains(ship, 'SOH_DumpSeedChoices = (ComboRando::SeedChoices::FnGetRecord)GetSym(sohModule, '
                             '"SOH_DumpSeedChoices");'), 'the launcher resolves the native exports')

# The headless tool: generation as RunComboFill; --playthrough binds the record before every finalize and refuses a
# replay whose finalize did not keep it, and says when an older seed's random choices are drawn again.
headless = read('combo/ComboRandoHeadless.cpp')
generate = headless[hostcxx.find(headless, '// ---- Generate + validate mode. ----'):]
ordered(generate, 'ComboRando::SeedChoices::Mode::Seeded, std::nullopt);', 'for (int attempt = 0;',
        'SOH_GenerationDump()', 'ComboMq::StampSeedMq(consolidated["oot"], mqSource,',
        'ComboRando::SeedChoices::StampSeedChoices(', 'ComboRando::StampComboSeedFormat(consolidated);')
play = headless[hostcxx.find(headless, '// ---- Playthrough mode'):hostcxx.find(headless, '// ---- Generate + validate')]
ordered(play, 'const auto recorded = ComboRando::SeedChoices::RecordedChoices(ootSeed);',
        'ComboRando::SeedChoices::ModeForRecord(recorded), recorded);', 'auto runPass =', 'SOH_Dump()',
        'SOH_PrepContext();', 'ComboMq::ReplayKeepsMqDungeons(ootSeed,',
        'ComboRando::SeedChoices::ReplayKeepsChoices(ootSeed, resolved)', 'nativePreparationFailed = true;',
        'ComboRando::SeedChoices::ReRollsChoices(ootSeed, resolved)', 'SOH_ApplyEntranceOverrides(',
        'auto r1 = runPass(false, &pt1);', 'if (choicesRerolled)', 'if (r1.beatable) {')

# A settings document and a vanilla run drop the record, a generation output.
check(hostcxx.contains(function(read('combo/ComboRandomizerProfiles.h'), 'inline Json SettingsDocument('),
                       'SeedChoices::kKey'), 'a settings document drops the recorded choices')
check(hostcxx.contains(read('combo/rando/SingleGameFill.h'), 'seed[game].erase(SeedChoices::kKey);'),
      'a vanilla run records no settings choices')

if failures:
    print('\n'.join('FAIL: ' + failure for failure in failures))
    raise SystemExit(1)
print('PASS: seed choices draw from the seed, are recorded, and bind every reload and replay')
