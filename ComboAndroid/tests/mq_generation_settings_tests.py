"""The generator offers Master Quest dungeons with OoTMM's bundled MQ data and rolls them from the seed (MQ port U7).

OoTMM picks each seed's MQ dungeons once, from the seed's own random (mqDungeons, packages/logic/src/world/flags.ts:
45-75; Allsanity's mqDungeons random is a coin flip per dungeon, presets.ts:697), and plays them from the vanilla ROM.
This harness compiles the production lines of SoH's settings finalization (settings.cpp Context::FinalizeSettings: the
MQ-only and no-MQ gates and the MQ roll), SoH's random number generator (rng.h, ShipUtils), the launcher's seed
accessor (OTRGlobals.cpp) and the seed marker (combo/mq/MqSeedMarker.h), and checks:
- the finalize gate in every state: without oot-mq.o2r every dungeon stays vanilla, unless the seed records the
  bundled data; a seed without the marker (every seed made before it) is finalized exactly as before;
- a bundled seed rolls its MQ dungeons from its master seed on a stream of their own: the generation, the oracle's
  finalize and a reload in another process roll the same dungeons wherever the shared stream is, the shared stream is
  left as it was, and the dungeons differ from seed to seed with OoTMM's even odds;
- Allsanity's 13 Master Quest writes (ComboPresets.h) now give a coin flip per dungeon, and none without the data;
- the seed records the rolled dungeons (StampSeedMq) and a reload's roll matches them (ReplayKeepsMqDungeons);
- the launcher and the headless generator set the master seed before every finalize, and the headless replay checks
  the recorded MQ dungeons as the in-game reload does;
- the native options, the profile catalogue (mqPlayable, mqSource), the profile editor's rows and its Wallmaster
  refusal agree in every install state;
- the seed lists its MQ dungeons by SoH's names, as SoH's and OoTMM's spoilers do, and the seed views show them;
- Allsanity's help says its MQ dungeons are OoTMM's coin flips and what they need;
- the check tracker keeps SoH's spoiler behaviour (the owner's default): a dungeon whose quest was rolled stays hidden
  until visited, and no Combo row turns MQ spoilers on.
"""
from pathlib import Path
import argparse
import re
import subprocess
import sys
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
    """text from `start` up to (not including) `end`, ignoring whitespace differences."""
    first = hostcxx.find(text, start, at)
    return text[first:hostcxx.find(text, end, first + 1)]


def function(text, signature):
    """The definition that begins with `signature`, through its closing brace."""
    first = hostcxx.find(text, signature)
    depth, at = 0, text.index('{', first)
    while True:
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            return text[first:at + 1] + '\n'
        at += 1


def fnv1a(text):
    value = 2166136261
    for byte in text.encode():
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


settings = read('soh/soh/Enhancements/randomizer/settings.cpp')
finalize = function(settings, 'void Context::FinalizeSettings(')
gate = between(finalize, '// If we only have MQ, set all dungeons to MQ',
               '// If any of the individual shuffle settings are on')
roll = between(finalize, 'std::array<DungeonInfo*, 12> dungeons = this->GetDungeons()->GetDungeonList();',
               '// Set key ring for each dungeon')
check(hostcxx.find(finalize, '// If we only have MQ') < hostcxx.find(finalize, 'std::array<DungeonInfo*, 12> dungeons'),
      'FinalizeSettings: the MQ gates must come before the MQ roll')
check(re.search(r'#define MAX_MQ_DUNGEON_COUNT 12\b', read('soh/soh/Enhancements/randomizer/randomizerTypes.h')),
      'randomizerTypes.h: MAX_MQ_DUNGEON_COUNT is 12')
# The CVar of each MQ option (SetAllToContext copies them into the context by key).
cvars = dict((cvar, key) for key, cvar in re.findall(
    r'OPT_(?:U8|BOOL)\((RSK_MQ_\w+),[^;]*?CVAR_RANDOMIZER_SETTING\("(\w+)"\)', settings))
check(len(cvars) == 15, f'settings.cpp: expected the 15 MQ options, found {sorted(cvars)}')

rng = read('soh/soh/Enhancements/randomizer/rng.h')
rng = '\n'.join(line for line in rng.splitlines() if not line.startswith(('#pragma', '#include')))
ship_h = read('soh/soh/ShipUtils.h')
ship_h = between(ship_h, 'namespace ShipUtils {', '} // namespace ShipUtils') + '}\n'
ship_cpp = read('soh/soh/ShipUtils.cpp')
ship_cpp = between(ship_cpp, 'static bool default_init = false;', 'double ShipUtils::RandomDouble(')

otr = read('soh/soh/OTRGlobals.cpp')
seed_statics = between(otr, 'static uint64_t sComboRandoSeed = 0;', '// Reseed SoH\'s rando RNG')
seed_set = function(otr, 'extern "C" COMBO_EXPORT void SOH_SetComboRandoSeed(uint64_t seed)')
seed_get = function(otr, 'bool Combo_GetRandoSeed(uint64_t* seed)')
mask_export = function(otr, 'extern "C" COMBO_EXPORT int SOH_GetMqDungeonMask(void)')

seeds = [0, 1, 7, 412070000, 4294967295, 18446744073709551615]
pins = '\n'.join(f'    static_assert(ComboMq::MqRollSeed({s}ull) == {fnv1a("mqDungeons:ootmm-32.3:" + str(s))}u);'
                 for s in seeds)
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
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#define COMBO_EXPORT
#include "randomizerEnums.h"
#include "mq/MqSeedMarker.h"
#include "rando/ComboPresets.h"
#define MAX_MQ_DUNGEON_COUNT 12

namespace ShipUtils {
''' + ship_h.split('{', 1)[1] + ship_cpp + rng + r'''

// OTRGlobals: which O2R files are loaded and whether the bundled MQ data plays here (OTRGlobals.cpp Initialize).
struct OTRGlobals {
    static OTRGlobals* Instance;
    bool original = true, archive = false, bundled = false;
    bool HasOriginal() { return original; }
    bool HasMasterQuest() { return archive; }
    bool HasBundledMasterQuest() { return bundled; }
};
OTRGlobals* OTRGlobals::Instance = nullptr;
''' + seed_statics + seed_set + seed_get + r'''

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
    bool mq = false, known = true;
    void ClearMQ() { mq = false; }
    void SetMQ() { mq = true; }
    bool IsMQ() const { return mq; }
    void SetDungeonKnown(bool value) { known = value; }
    RandomizerSettingKey GetMQSetting() const { return setting; }
};
struct Dungeons {
    std::array<DungeonInfo, 12> list;
    std::array<DungeonInfo*, 12> GetDungeonList() {
        std::array<DungeonInfo*, 12> result{};
        for (size_t i = 0; i < list.size(); i++)
            result[i] = &list[i];
        return result;
    }
};
namespace Rando {
class Context {
  public:
    std::map<int, OptionValue> mOptions;
    Dungeons dungeonStore;
    ComboMq::SeedMqSource source = ComboMq::SeedMqSource::Legacy;
    Context() {
        for (int i = 0; i < 12; i++)
            dungeonStore.list[i].setting = static_cast<RandomizerSettingKey>(RSK_MQ_DEKU_TREE + i);
    }
    ComboMq::SeedMqSource GetMqDataSource() const { return source; }
    Dungeons* GetDungeons() { return &dungeonStore; }
    static std::shared_ptr<Context> GetInstance() { return instance; }
    static std::shared_ptr<Context> instance;
    // settings.cpp Context::FinalizeSettings: the MQ gates, then the MQ roll.
    void FinalizeMq() {
''' + gate + roll + r'''
    }
};
std::shared_ptr<Context> Context::instance = std::make_shared<Context>();
} // namespace Rando
''' + mask_export + r'''

static const std::map<std::string, int> kMqCvars = {
''' + cvar_rows + r'''
};
using Values = std::map<int, int>;
// A profile's MQ CVars as SetAllToContext copies them; the menu defaults elsewhere (settings.cpp OPT_* defaults:
// None, a count of 12, Set Dungeon Quests off, every dungeon Vanilla).
static Values Rules(std::initializer_list<std::pair<const int, int>> values) {
    Values rules{ { RSK_MQ_DUNGEON_RANDOM, RO_MQ_DUNGEONS_NONE }, { RSK_MQ_DUNGEON_COUNT, 12 },
                  { RSK_MQ_DUNGEON_SET, 0 } };
    for (int i = 0; i < 12; i++)
        rules[RSK_MQ_DEKU_TREE + i] = RO_MQ_SET_VANILLA;
    for (const auto& [key, value] : values)
        rules[key] = value;
    return rules;
}
static Values PresetRules(ComboRando::CommunityPreset preset) {
    Values rules = Rules({});
    int written = 0;
    for (const auto& option : ComboRando::CommunityPresetOptions(preset))
        if (const auto row = kMqCvars.find(option.key); row != kMqCvars.end()) {
            rules[row->second] = option.value;
            written++;
        }
    if (preset == ComboRando::CommunityPreset::Allsanity && written != 13)
        throw std::runtime_error("Allsanity no longer makes its 13 Master Quest writes");
    return rules;
}
static Rando::Context& Ctx() { return *Rando::Context::instance; }
static int Finalize(const Values& rules, ComboMq::SeedMqSource source) {
    for (const auto& [key, value] : rules)
        Ctx().mOptions[key].Set(value);
    Ctx().source = source;
    Ctx().FinalizeMq();
    return SOH_GetMqDungeonMask();
}
static int Count(int mask) { return std::popcount(static_cast<unsigned>(mask)); }
static void Require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}
static const auto kLegacy = ComboMq::SeedMqSource::Legacy;
static const auto kBundled = ComboMq::SeedMqSource::Bundled;

int main() {
    try {
''' + pins + r'''
        OTRGlobals globals;
        OTRGlobals::Instance = &globals;
        const Values allsanity = PresetRules(ComboRando::CommunityPreset::Allsanity);
        Require(allsanity.at(RSK_MQ_DUNGEON_RANDOM) == RO_MQ_DUNGEONS_SELECTION, "Allsanity: Selection Only");
        for (int i = 0; i < 12; i++)
            Require(allsanity.at(RSK_MQ_DEKU_TREE + i) == RO_MQ_SET_RANDOM, "Allsanity: every dungeon Random");
        Require(PresetRules(ComboRando::CommunityPreset::Default) == Rules({}), "Default writes no MQ option");

        // Before any combo seed (no generation yet), a bundled finalize keeps SoH's shared stream.
        rando_state = 0x1234;
        Finalize(allsanity, kBundled);
        Require(rando_state != 0x1234, "without a combo seed the roll stays on the shared stream");

        // The finalize gate in every state. SoH's rule for a seed without the marker: oot.o2r without oot-mq.o2r
        // keeps every dungeon vanilla, oot-mq.o2r alone makes every dungeon MQ.
        SOH_SetComboRandoSeed(41207);
        for (int state = 0; state < 16; state++) {
            globals.original = state & 1;
            globals.archive = (state & 2) != 0;
            globals.bundled = (state & 4) != 0 && globals.original; // the bundled data needs oot.o2r
            const auto source = (state & 8) ? kBundled : kLegacy;
            const bool legacyNone = globals.original && !globals.archive;
            const bool forcedNone = legacyNone && source != kBundled;
            const bool allMq = globals.archive && !globals.original;
            for (const auto& rules : { allsanity, Rules({ { RSK_MQ_DUNGEON_RANDOM, RO_MQ_DUNGEONS_SET_NUMBER },
                                                          { RSK_MQ_DUNGEON_COUNT, 12 } }) }) {
                const int mask = Finalize(rules, source);
                const std::string where = "state " + std::to_string(state) + ": ";
                if (allMq)
                    Require(mask == 0xfff, where + "oot-mq.o2r alone makes every dungeon MQ");
                else if (forcedNone) {
                    Require(mask == 0 && Ctx().mOptions[RSK_MQ_DUNGEON_RANDOM].Is(RO_MQ_DUNGEONS_NONE) &&
                                Ctx().mOptions[RSK_MQ_DUNGEON_COUNT].Get() == 0,
                            where + "without MQ data every dungeon stays vanilla");
                    for (int i = 0; i < 12; i++)
                        Require(Ctx().mOptions[RSK_MQ_DEKU_TREE + i].Is(RO_MQ_SET_VANILLA), where + "vanilla quests");
                } else if (rules == allsanity)
                    Require(Ctx().mOptions[RSK_MQ_DUNGEON_RANDOM].Is(RO_MQ_DUNGEONS_SELECTION) &&
                                Ctx().mOptions[RSK_MQ_DUNGEON_COUNT].Get() == Count(mask),
                            where + "the MQ selection takes effect");
                else
                    Require(mask == 0xfff, where + "a Set Number of 12 makes every dungeon MQ");
            }
        }

        // A bundled seed on an install with the data: each master seed rolls one set of MQ dungeons.
        globals.original = true;
        globals.archive = false;
        globals.bundled = true;
        std::mt19937_64 garbage(99);
        std::map<int, int> perDungeon;
        std::set<int> masks;
        const int kSeeds = 400;
        for (int s = 0; s < kSeeds; s++) {
            const uint64_t master = 412070000ull + 7919ull * s;
            // Generation: the attempt's master seed, then the dump's finalize.
            SOH_SetComboRandoSeed(master);
            const uint64_t shared = garbage();
            rando_state = shared;
            const int generated = Finalize(allsanity, kBundled);
            Require(rando_state == shared, "the MQ roll leaves the shared stream as it was");
            // The oracle's first reset finalizes again later in the same generation.
            rando_state = garbage();
            Require(Finalize(allsanity, kBundled) == generated, "the oracle's finalize rolls the same MQ dungeons");
            // The seed document records them.
            nlohmann::json oot = nlohmann::json::object();
            ComboMq::StampSeedMq(oot, kBundled, generated);
            Require(ComboMq::RecordedMqDungeonMask(oot) == generated && ComboMq::MqDataSourceOf(oot) == kBundled,
                    "the seed records its source and MQ dungeons");
            // A reload in another process: another seed first, an unrelated shared stream, then the seed's own.
            SOH_SetComboRandoSeed(master ^ 0x5555);
            rando_state = garbage();
            Finalize(allsanity, kBundled);
            SOH_SetComboRandoSeed(master);
            rando_state = garbage();
            const int replayed = Finalize(allsanity, kBundled);
            Require(ComboMq::ReplayKeepsMqDungeons(oot, replayed), "a reload rolls the MQ dungeons the seed records");
            masks.insert(generated);
            for (int i = 0; i < 12; i++)
                perDungeon[i] += (generated >> i) & 1;
        }
        // OoTMM's odds: a coin flip for each dungeon (flags.ts:61, random.next() & 0x1000).
        for (const auto& [dungeon, count] : perDungeon)
            Require(count > kSeeds * 3 / 10 && count < kSeeds * 7 / 10,
                    "dungeon " + std::to_string(dungeon) + " is MQ in " + std::to_string(count) + " of 400 seeds");
        Require(masks.size() > kSeeds * 3 / 4, "the MQ dungeons differ from seed to seed");
        // The roll changes with the master seed, so a replay of a different seed is refused.
        int differing = 0;
        for (uint64_t s = 1; s <= 64; s++) {
            SOH_SetComboRandoSeed(s);
            const int a = Finalize(allsanity, kBundled);
            SOH_SetComboRandoSeed(s + 1);
            nlohmann::json oot = nlohmann::json::object();
            ComboMq::StampSeedMq(oot, kBundled, a);
            differing += !ComboMq::ReplayKeepsMqDungeons(oot, Finalize(allsanity, kBundled));
        }
        Require(differing > 48, "another master seed rolls other MQ dungeons");

        // SoH's other MQ modes keep their meaning on the seed's stream.
        for (uint64_t s = 0; s < 64; s++) {
            SOH_SetComboRandoSeed(s);
            const auto setNumber = Rules({ { RSK_MQ_DUNGEON_RANDOM, RO_MQ_DUNGEONS_SET_NUMBER },
                                           { RSK_MQ_DUNGEON_COUNT, 5 } });
            const int five = Finalize(setNumber, kBundled);
            rando_state = s * 31;
            Require(Count(five) == 5 && Finalize(setNumber, kBundled) == five, "Set Number 5: five MQ dungeons");
            const auto randomNumber = Rules({ { RSK_MQ_DUNGEON_RANDOM, RO_MQ_DUNGEONS_RANDOM_NUMBER } });
            const int any = Finalize(randomNumber, kBundled);
            rando_state = s * 37;
            Require(Finalize(randomNumber, kBundled) == any, "Random Number: reproducible");
            const auto selection = Rules({ { RSK_MQ_DUNGEON_RANDOM, RO_MQ_DUNGEONS_SELECTION },
                                           { RSK_MQ_DEKU_TREE, RO_MQ_SET_MQ }, { RSK_MQ_FIRE_TEMPLE, RO_MQ_SET_VANILLA },
                                           { RSK_MQ_SPIRIT_TEMPLE, RO_MQ_SET_RANDOM } });
            const int chosen = Finalize(selection, kBundled);
            Require((chosen & ~(1 | (1 << 6))) == 0 && (chosen & 1), "Selection: fixed quests stay fixed");
        }

        // A seed without the marker on the same install: every dungeon vanilla, and no random number is drawn, so
        // the seed finalizes exactly as before.
        for (const auto& rules : { allsanity, Rules({ { RSK_MQ_DUNGEON_RANDOM, RO_MQ_DUNGEONS_RANDOM_NUMBER } }) }) {
            SOH_SetComboRandoSeed(412070000);
            rando_state = 777;
            Require(Finalize(rules, kLegacy) == 0 && rando_state == 777, "a Legacy seed stays all vanilla");
        }
        // A Legacy seed with a real oot-mq.o2r keeps SoH's own roll on the shared stream.
        globals.archive = true;
        rando_state = 777;
        Finalize(allsanity, kLegacy);
        Require(rando_state != 777, "a Legacy seed with oot-mq.o2r rolls on the shared stream, as before");
        std::puts("PASS: bundled seeds roll and keep their MQ dungeons; Legacy seeds finalize as before");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    return 0;
}
'''

if not failures:
    cpp = out / 'mq_generation_settings.cpp'
    cpp.write_text(harness, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, std='c++20', warnings=False,
                        includes=[repo / 'soh/soh/Enhancements/randomizer', repo / 'combo', args.json_include],
                        defines=['COMBO_BUILD'], msvc=['/Zc:preprocessor', '/utf-8', '/bigobj'])
    run = subprocess.run([str(exe)], cwd=out, capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    print(run.stdout + run.stderr, end='')
    check(run.returncode == 0, 'the finalize harness failed')

# The menus, in every install state: SoH's native options (settings.cpp CreateOptions), the profile catalogue
# (OTRGlobals.cpp SOH_DumpProfileCatalog), the profile editor's copy of the native gate (ProfileOotOptionPolicy.h) and
# its Wallmaster refusal (EntranceProfile.h), all from the production lines.
options_gate = between(settings, '// Don\'t show any MQ options if both quests aren\'t available', '// clang-format on',
                       hostcxx.find(settings, 'void Settings::CreateOptions()'))
can_play = function(otr, 'bool OTRGlobals::CanPlayMasterQuest()')
catalogue = between(otr, 'data["hasOriginal"] = OTRGlobals::Instance->HasOriginal();',
                    'auto settings = Rando::Settings::GetInstance();',
                    hostcxx.find(otr, 'extern "C" COMBO_EXPORT const char* SOH_DumpProfileCatalog(void)'))
menus = r'''
#include <cstdint>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include "ProfileOotOptionPolicy.h"
#include "rando/entrances/EntranceProfile.h"
#include "mq/MqMode.h"
#include "randomizerEnums.h"

// OTRGlobals: the loaded archives (Initialize) and the bundled data's gate (SetBundledMasterQuest).
struct OTRGlobals {
    static OTRGlobals* Instance;
    bool hasOriginal = true, hasMasterQuest = false, bundledMasterQuest = false;
    bool HasOriginal() { return hasOriginal; }
    bool HasMasterQuest() { return hasMasterQuest; }
    bool HasBundledMasterQuest() { return bundledMasterQuest; }
    bool CanPlayMasterQuest();
};
OTRGlobals* OTRGlobals::Instance = nullptr;
''' + can_play + r'''
struct NativeOption {
    bool disabled = false;
    std::string disabledText;
    void Disable(std::string text) { disabled = true; disabledText = std::move(text); }
    void Enable() { disabled = false; disabledText.clear(); }
};
// settings.cpp Settings::CreateOptions: the Master Quest gate.
struct Settings {
    std::map<int, NativeOption> mOptions;
    void CreateMqOptions() {
''' + options_gate + r'''
    }
};
static nlohmann::json Catalogue() {
    nlohmann::json data = { { "options", nlohmann::json::object() } };
''' + catalogue + r'''
    return data;
}
static void Require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}
static const int kMqOptions[] = { RSK_MQ_DUNGEON_RANDOM, RSK_MQ_DUNGEON_COUNT, RSK_MQ_DUNGEON_SET, RSK_MQ_DEKU_TREE,
                                  RSK_MQ_DODONGOS_CAVERN, RSK_MQ_JABU_JABU, RSK_MQ_FOREST_TEMPLE, RSK_MQ_FIRE_TEMPLE,
                                  RSK_MQ_WATER_TEMPLE, RSK_MQ_SPIRIT_TEMPLE, RSK_MQ_SHADOW_TEMPLE,
                                  RSK_MQ_BOTTOM_OF_THE_WELL, RSK_MQ_ICE_CAVERN, RSK_MQ_GTG, RSK_MQ_GANONS_CASTLE };
static const char* kNative = "This option has been disabled because only one type of O2R has been loaded";

int main() {
    try {
        OTRGlobals globals;
        OTRGlobals::Instance = &globals;
        int states = 0;
        for (int state = 0; state < 8; state++) {
            globals.hasOriginal = state & 1;
            globals.hasMasterQuest = (state & 2) != 0;
            globals.bundledMasterQuest = (state & 4) != 0;
            if (globals.bundledMasterQuest && !globals.hasOriginal)
                continue; // the bundled data plays only over a vanilla oot.o2r (OTRGlobals::SetBundledMasterQuest)
            states++;
            const std::string where = "state " + std::to_string(state) + ": ";
            const bool available = (globals.hasMasterQuest || globals.bundledMasterQuest) && globals.hasOriginal;
            // Native options: disabled with SoH's own text exactly when MQ cannot be played beside the vanilla game.
            Settings native;
            native.CreateMqOptions();
            for (int key : kMqOptions) {
                const auto& option = native.mOptions.at(key);
                Require(option.disabled == !available && option.disabledText == (available ? "" : kNative),
                        where + "native MQ option " + std::to_string(key));
            }
            // SoH's own gate is unchanged without the bundled data.
            if (!globals.bundledMasterQuest)
                Require(available == (globals.hasMasterQuest && globals.hasOriginal), where + "SoH's gate without bundled data");
            // The catalogue says the same, and which data a new seed's MQ dungeons use.
            const auto catalogue = Catalogue();
            Require(catalogue.at("mqPlayable") == available, where + "catalogue mqPlayable");
            Require(catalogue.at("hasMasterQuest") == globals.hasMasterQuest && catalogue.at("hasOriginal") == globals.hasOriginal,
                    where + "hasMasterQuest keeps meaning a real oot-mq.o2r");
            Require(catalogue.at("mqSource") == (globals.bundledMasterQuest   ? ComboMq::kMqDataSourceBundled
                                                 : globals.hasMasterQuest ? "oot-mq.o2r"
                                                                   : "none"),
                    where + "catalogue mqSource");
            // The profile editor disables the same rows with the same text.
            for (int mqMode = 0; mqMode < 4; mqMode++) {
                ComboRando::ProfileOotPolicy::View view(catalogue, [&](const char* key, int fallback) {
                    return std::string(key) == "gRandoSettings.MQDungeons" ? mqMode : fallback;
                });
                for (int key : kMqOptions) {
                    const auto& row = view.mOptions.at(key);
                    Require(row.disabled == !available && row.disabledText == (available ? "" : kNative),
                            where + "profile MQ row " + std::to_string(key));
                }
                // Wallmaster Shuffle stays refused with MQ selected wherever MQ can be played: the player sees the
                // plain Master Quest sentence, and the log keeps the developer text (EntranceAvailability.h).
                nlohmann::json draft = { { "mm", { { "settings", nlohmann::json::object() } } } };
                const auto refusal = ComboRando::Entrances::ProfileWallmasterRefusal(
                    draft, catalogue, [&](const char* key, int fallback) {
                        return std::string(key) == "gRandoSettings.MQDungeons" ? mqMode : fallback;
                    });
                const bool refused = (globals.hasMasterQuest && !globals.hasOriginal) || (available && mqMode != 0);
                Require(refusal.player ==
                                (refused ? "Wallmaster Shuffle does not support Master Quest dungeons yet." : "") &&
                            refusal.log == (refused ? "Wallmaster Shuffle currently supports vanilla dungeons only" : ""),
                        where + "Wallmaster refusal with MQ mode " + std::to_string(mqMode));
            }
        }
        Require(states == 6, "six install states");
        // A catalogue without mqPlayable (never written by this build) keeps the rows unavailable.
        auto old = Catalogue();
        old.erase("mqPlayable");
        ComboRando::ProfileOotPolicy::View view(old, [](const char*, int fallback) { return fallback; });
        Require(view.mOptions.at(RSK_MQ_DUNGEON_RANDOM).disabled, "a catalogue without mqPlayable");
        std::puts("PASS: the native options, catalogue, profile rows and Wallmaster refusal agree in every state");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
    return 0;
}
'''
if not failures:
    cpp = out / 'mq_menu_gates.cpp'
    cpp.write_text(menus, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, std='c++20', warnings=False,
                        includes=[repo / 'combo/gui', repo / 'combo', repo / 'soh/soh/Enhancements/randomizer',
                                  args.json_include],
                        defines=['COMBO_BUILD'], msvc=['/Zc:preprocessor', '/utf-8', '/bigobj'])
    run = subprocess.run([str(exe)], cwd=out, capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
    print(run.stdout + run.stderr, end='')
    check(run.returncode == 0, 'the menu harness failed')


def body(text, signature):
    start = text.index('{', hostcxx.find(text, signature))
    depth = 0
    for at in range(start, len(text)):
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            return text[start:at + 1]
    raise AssertionError(f'unbalanced body after {signature}')


def order(text, names, what):
    at = 0
    for name in names:
        try:
            at = hostcxx.find(text, name, at) + 1
        except ValueError:
            check(False, f'{what}: {name} missing or out of order')
            return


# Every finalize of a seed has its master seed: the oracle and the dump finalize, and the launcher and the headless
# generator set the master seed first.
check(hostcxx.contains(body(otr, 'extern "C" COMBO_EXPORT void SOH_PrepRandoContext(void)'),
                       'ctx->FinalizeSettings(Combo_ParseExcludedLocations(), Combo_ParseEnabledTricks());') and
      hostcxx.contains(body(otr, 'static void EnsureOracleInit() {'),
                       'ctx->FinalizeSettings(Combo_ParseExcludedLocations(), Combo_ParseEnabledTricks());'),
      'the dump and the oracle finalize through FinalizeSettings')
dump = body(otr, 'static const char* Combo_DumpRandoStaticData(bool prepareEntrances)')
check(hostcxx.find(dump, 'SOH_PrepRandoContext();') < hostcxx.find(dump, 'EnsureOracleInit();'),
      'the dump finalizes before the oracle')
ship = read('combo/ComboShip.cpp')
fill = body(ship, 'static void RunComboFill(')
order(fill, ['masterSeed = baseSeed + uint32_t(attempt) * 0x9E3779B9u;', 'SOH_SetComboRandoSeed(masterSeed);',
             'SOH_DumpRandoStaticData()', 'consolidated["masterSeed"] = masterSeed;',
             'ComboMq::StampSeedMq(consolidated["oot"], mqSource,'], 'generation')
reload = ship[hostcxx.find(ship, 'if (SOH_RestoreRandoSettings)\n            SOH_RestoreRandoSettings(ootSettings.c_str());'):]
order(reload, ['SOH_SetComboRandoSeed(masterSeed);', 'SOH_PrepRandoContext();',
               'if (!ComboMq::ReplayKeepsMqDungeons(oot, SOH_GetMqDungeonMask ? SOH_GetMqDungeonMask() : -1))'],
      'reload')
headless = read('combo/ComboRandoHeadless.cpp')
generate = headless[hostcxx.find(headless, '// ---- Generate + validate mode. ----'):]
order(generate, ['SOH_SetSeed(masterSeed);', 'SOH_GenerationDump()', 'consolidated["masterSeed"] = masterSeed;',
                 'ComboMq::StampSeedMq(consolidated["oot"], mqSource,'], 'headless generation')
replay = headless[hostcxx.find(headless, 'auto runPass = [&](bool allTricks, nlohmann::json* ptOut) {'):]
order(replay, ['SOH_SetSeed(masterSeed);', 'SOH_Dump()', 'SOH_PrepContext();',
               'if (!ComboMq::ReplayKeepsMqDungeons(ootSeed, SOH_GetMqDungeonMask ? SOH_GetMqDungeonMask() : -1)) {',
               'nativePreparationFailed = true;'], 'headless replay')
check(hostcxx.contains(headless, 'ComboMq::RecordedMqDungeonMask(spoiler.value("oot", nlohmann::json::object()));'),
      'the headless replay must refuse a malformed MQ record before it runs')

# The spoiler lists a bundled seed's MQ dungeons by name, as SoH's spoiler (spoiler_log.cpp WriteMasterQuestDungeons:
# DungeonInfo::GetName of each MQ dungeon) and OoTMM's (spoiler.ts writeWorldFlags) do: the names are SoH's, in the
# DungeonKey order of the ids, and the profile's seed views show them.
dungeon_rows = re.findall(r'DungeonInfo\(\s*"([^"]+)".*?(RSK_MQ_\w+)', read('soh/soh/Enhancements/randomizer/dungeon.cpp'),
                          re.S)
marker = read('combo/mq/MqSeedMarker.h')
table = re.search(r'kMqDungeonNames\[kMqSeedDungeonCount\] = \{(.*?)\};', marker, re.S)
names = re.findall(r'"([^"]+)"', table.group(1)) if table else []
order_keys = re.findall(r'RANDO_ENUM_ITEM\((RSK_MQ_\w+)\)',
                        read('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h'))[3:]
check([name for name, _ in dungeon_rows] == names and [key for _, key in dungeon_rows] == order_keys,
      f'MqSeedMarker.h kMqDungeonNames must be dungeon.cpp\'s names in DungeonKey order: {names} != {dungeon_rows}')
check(re.search(r'WriteMasterQuestDungeons\(\) \{.*?jsonData\["masterQuestDungeons"\]\.push_back\(dungeon->GetName\(\)\);',
                read('soh/soh/Enhancements/randomizer/3drando/spoiler_log.cpp'), re.S),
      'SoH\'s spoiler lists MQ dungeons by DungeonInfo name')
stamp = body(marker, 'inline void StampSeedMq(')
check('oot[kMqDungeonsKey] = MqDungeonIds(mqDungeonMask);' in stamp and
      'oot[kMqDungeonNamesKey] = MqDungeonNames(mqDungeonMask);' in stamp and
      stamp.index('WriteMqDataSource(oot, SeedMqSource::Legacy);') < stamp.index('oot[kMqDungeonsKey]'),
      'a bundled seed records its MQ dungeons by id and name; a Legacy seed records neither')
profile_ui = read('combo/gui/ComboRandomizerProfiles.cpp')
check(re.search(r'if \(view != 0 && !std::strcmp\(key, "gRandoSettings\.MQDungeons"\)\) \{.*?'
                r'ComboMq::RecordedMqDungeonsText\(seed\.at\("oot"\)\); !text\.empty\(\)\)\s*'
                r'ImGui::TextWrapped\("Master Quest dungeons: %s", text\.c_str\(\)\);', profile_ui, re.S),
      'the saved and generated seed views show a bundled seed\'s MQ dungeons under the MQ setting')
check('ComboMq::kMqDungeonNamesKey' in body(read('combo/ComboRandomizerProfiles.h'), 'inline Json SettingsDocument('),
      'a settings document drops the seed\'s MQ dungeon names, a generation output')

# Allsanity's help no longer says its MQ dungeons need data setup does not import: they take effect, as in OoTMM.
presets = read('combo/rando/ComboPresets.h')
allsanity_help = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', between(presets, '{ CommunityPreset::Allsanity,', '},')))
check('setup does not import' not in presets and 'settings.cpp:2896' not in presets,
      'ComboPresets.h still says Master Quest is unavailable')
check('Master Quest on a coin flip, as in OoTMM' in allsanity_help and 'NTSC-U 1.0' in allsanity_help,
      f'the Allsanity help must say its MQ dungeons are OoTMM\'s coin flips and what they need: {allsanity_help}')

# The check tracker keeps SoH's spoiler behaviour (owner default): a dungeon whose quest was rolled stays hidden until
# visited, a dungeon of fixed quest is shown, and no Combo row turns MQ spoilers on. Rows follow the seed's quests.
snapshot = read('combo/menu/ComboCheckTrackerOOT.inc')
check(hostcxx.contains(snapshot, 'if (!IsAreaSpoiled(area) && !CVarGetInteger(CVAR_TRACKER_CHECK("MQSpoilers"), 0)) {')
      and hostcxx.contains(snapshot, 'COMBO_CHECK_AREA_HIDDEN'),
      'the Combo tracker hides an unspoiled dungeon unless MQ spoilers are on')
combo_mq_spoilers = sorted(p.relative_to(repo).as_posix() for p in (repo / 'combo').rglob('*')
                           if p.suffix in ('.h', '.cpp', '.inc') and 'MQSpoilers' in p.read_text(encoding='utf-8',
                                                                                                 errors='replace'))
check(combo_mq_spoilers == ['combo/menu/ComboCheckTrackerOOT.inc'],
      f'no Combo menu row may set the MQ spoiler option: {combo_mq_spoilers}')
tracker = read('soh/soh/Enhancements/randomizer/randomizer_check_tracker.cpp')
check(re.search(r'RSK_MQ_DUNGEON_RANDOM\) == RO_MQ_DUNGEONS_NONE \|\|.*?RO_MQ_SET_RANDOM\).*?'
                r'SetAreaSpoiled\(static_cast<RandomizerCheckArea>\(i\)\);', tracker, re.S),
      'SoH spoils a dungeon area at load only when its quest is not random')
check(hostcxx.contains(read('soh/soh/Enhancements/randomizer/SeedContext.cpp'),
                       'loc->GetQuest() == RCQUEST_MQ && mDungeons->GetDungeonFromScene(loc->GetScene())->IsMQ()'),
      'tracker rows follow the seed\'s own dungeon quests')

if failures:
    print('FAIL:\n  ' + '\n  '.join(failures))
    sys.exit(1)
print('PASS: the generator rolls a bundled seed\'s MQ dungeons from its master seed; old seeds are unchanged')
