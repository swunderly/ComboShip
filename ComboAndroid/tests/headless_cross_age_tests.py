"""Run the headless oracle bindings and rule writer through their real consumers."""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
headless = (source/'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
launcher = (source/'combo/ComboShip.cpp').read_text(encoding='utf-8')
# Execute production assignments, including their actual export names. Missing bindings
# leave zeroed members and must fail the behavioral test, rather than the text extraction.
bindings = headless.split('    mmO.SetExternalBeanRenewal =', 1)[1].split(
    '\n    if (!SOH_InitRandoHeadless', 1)[0].split(';', 1)[1]


def writer(text):
    return '\n'.join(re.findall(
        r'consolidated\["(?:faroreRules|spellRules|gearRules)"\]\s*=[^;]+;', text))


prelude = r'''
#define NOMINMAX
#include <cassert>
#include <iostream>
#include <type_traits>
#include "rando/CrossWorldRando.h"
#include "rando/FaroreRules.h"
#include "rando/SpellRules.h"
#include "rando/GearRules.h"
using namespace ComboRando;
using Json = nlohmann::json;
bool crossAge = false;
uint8_t portalAges = 0, entryAges = 0;
int entryCalls = 0, mmAgeArrivalCalls = 0;
std::array<uint32_t, 2> ootSources{}, mmSources{}, mmArrivals{}, ootArrivals{};
void SetEntry(uint8_t ages) {entryAges = ages; ++entryCalls;}
uint8_t DefaultEntry() {return 2;}
uint8_t PortalAges() {return portalAges;}
uint8_t StartAges() {return 2;}
bool UseCrossAge() {return crossAge;}
uint32_t OotSources(int age) {return ootSources.at(age);}
uint32_t MmSources(int age) {return (entryAges & (1 << age)) ? mmSources.at(age) : 0;}
void OotArrivals(uint32_t adult, uint32_t child) {ootArrivals = {adult, child};}
void MmArrivals(uint32_t adult, uint32_t child) {mmArrivals = {adult, child}; ++mmAgeArrivalCalls;}
template<class T, class F> T Export(F fn) {
    if constexpr (std::is_same_v<T, F>) return fn;
    assert(false && "wrong symbol ABI"); return nullptr;
}
template<class T> T Sym(int module, const char* symbol) {
    const std::string key(symbol);
#define BIND(M, NAME, FN) if (key == NAME) {assert(module == M); return Export<T>(&FN);}
    BIND(1, "SOH_GetOraclePortalAges", PortalAges)
    BIND(1, "SOH_GetOracleSongSourcesForAge", OotSources)
    BIND(1, "SOH_SetOracleSongAgeArrivals", OotArrivals)
    BIND(1, "SOH_GetOracleStartAges", StartAges)
    BIND(2, "MM_SetOracleEntryAges", SetEntry)
    BIND(2, "MM_GetOracleDefaultEntryAges", DefaultEntry)
    BIND(2, "MM_GetOracleSongSourcesForAge", MmSources)
    BIND(2, "MM_SetOracleSongAgeArrivals", MmArrivals)
    BIND(2, "MM_UseCrossAgeOracle", UseCrossAge)
#undef BIND
    assert(false && "unexpected export name"); return nullptr;
}
void Bind(OracleFns& oot, OracleFns& mmO) {
    const int soh = 1, mm = 2;
'''
prelude += bindings + '\n}\n'
prelude += 'nlohmann::json MmSettingsForPlaythrough(' + headless.split(
    'nlohmann::json MmSettingsForPlaythrough(', 1)[1].split('\n}', 1)[0] + '\n}\n'
assert hostcxx.contains(headless, 'mmSettings = MmSettingsForPlaythrough(spoiler);')
assert hostcxx.contains(headless, 'nlohmann::json ms = mmSettings;')
assert hostcxx.contains(headless, 'MM_RestoreSettings(ms.dump().c_str());')
for name, text in [('Headless', headless), ('Launcher', launcher)]:
    prelude += ('Json Write' + name + '(const std::string& mmDump) {\n'
                'Json consolidated = Json::object();\n' + writer(text) + '\nreturn consolidated;\n}\n')
tests = r'''
void TestOracle() {
    OracleFns oot{}, mm{};
    Bind(oot, mm);
    // The rest of the existing loader is unrelated; supply its song/portal interfaces.
    oot.SetSongArrivals = +[](uint32_t) {};
    mm.SetSongArrivals = +[](uint32_t) {};
    oot.GetPortalOpen = +[]() -> uint8_t {return portalAges != 0;};
    oot.GetSongSources = +[]() -> uint32_t {return ootSources[0] | ootSources[1];};
    mm.GetSongSources = +[]() -> uint32_t {return mmSources[0] | mmSources[1];};
    oot.SongDestination = mm.SongDestination = +[](int i) {return i;};
    using Checks = std::unordered_set<std::string>;
    auto query = [&](const OracleFns& oracle, const std::vector<std::string>&) -> Checks {
        if (&oracle == &oot) return {"oot"};
        if (!crossAge) return {"legacy"};
        Checks checks;
        if (entryAges & 1) checks.insert("adult");
        if (entryAges & 2) checks.insert("child");
        return checks;
    };
    // Both starting games, option on/off, closed/child/adult/both portal ages.
    for (bool cross : {false, true}) for (bool mmStart : {false, true})
        for (uint8_t portal : {0, 1, 2, 3}) {
            crossAge = cross; portalAges = portal;
            entryAges = 3; entryCalls = mmAgeArrivalCalls = 0;
            ootSources = mmSources = {}; ootArrivals = mmArrivals = {63, 63};
            bool open = mmStart;
            auto result = QuerySongWorlds(oot, mm, {}, {}, open, query, mmStart);
            const uint8_t expected = portal | (mmStart ? 2 : 0);
            if (cross) {
                assert(entryCalls > 0 && mmAgeArrivalCalls > 0);
                assert(entryAges == expected);
                assert(result.second.contains("adult") == bool(expected & 1));
                assert(result.second.contains("child") == bool(expected & 2));
                assert(mmArrivals[0] == 0 && mmArrivals[1] == 0);
            } else {
                assert(entryCalls == 0 && mmAgeArrivalCalls == 0);
                assert(result.second.contains("legacy") == bool(expected));
            }
            assert(open == bool(expected));
        }
    // A song can open MM independently of the portal, and must retain the departure age.
    crossAge = true; portalAges = 0;
    for (int age : {0, 1}) {
        ootSources = {}; ootSources[age] = 1u << 6;
        mmSources = {}; mmSources[age] = 1u;
        bool open = false;
        auto result = QuerySongWorlds(oot, mm, {}, {}, open, query);
        assert(open && entryAges == (1 << age));
        assert(result.second.size() == 1);
        assert(result.second.contains(age == 0 ? "adult" : "child"));
        assert(mmArrivals[age] == (1u << 6) && mmArrivals[1-age] == 0);
        assert(ootArrivals[age] == 1 && ootArrivals[1-age] == 0);
    }
    // A later probe without those items must clear the previously reached age and landings.
    ootSources = mmSources = {}; bool open = false;
    auto result = QuerySongWorlds(oot, mm, {}, {}, open, query);
    assert(!open && result.second.empty() && entryAges == 0);
    assert(mmArrivals[0] == 0 && mmArrivals[1] == 0);
}
void TestWriter() {
    const char* options[] = {"RO_MM_SPELL_WIND", "RO_CROSS_GAME_FARORE", "RO_MM_CROSS_AGE",
        "RO_MM_SPELL_FIRE", "RO_MM_SPELL_LOVE", "RO_MM_BOOTS_IRON", "RO_MM_BOOTS_HOVER",
        "RO_MM_TUNIC_GORON", "RO_MM_TUNIC_ZORA"};
    // Exhaustive distinct flags catch cross-wired fields that all-on/all-off cases miss.
    for (int mask = 0; mask < 512; ++mask) {
        Json dump = {{"options", Json::object()}};
        auto enabled = [&](int bit) {return bool(mask & (1 << bit));};
        for (int bit = 0; bit < 9; ++bit) dump["options"][options[bit]] = int(enabled(bit));
        const auto written = WriteHeadless(dump.dump());
        assert(written == WriteLauncher(dump.dump()));
        assert(!written.contains("version")); // No top-level legacy-format migration.
        const auto restored = Json::parse(written.dump());
        FaroreRules farore; SpellRules spell; GearRules gear;
        assert(ReadFaroreRules(restored.value("faroreRules", Json()), farore));
        assert(ReadSpellRules(restored.value("spellRules", Json()), spell));
        assert(ReadGearRules(restored.value("gearRules", Json()), gear));
        assert(farore.spellWindMm == enabled(0));
        assert(farore.crossGameFw == (enabled(0) && enabled(1)));
        assert(farore.crossAge == enabled(2));
        assert(spell.spellFireMm == enabled(3) && spell.spellLoveMm == enabled(4));
        assert(gear.bootsIronMm == enabled(5) && gear.bootsHoverMm == enabled(6));
        assert(gear.tunicGoronMm == enabled(7) && gear.tunicZoraMm == enabled(8));
        // Replay must use the same immutable flags, even if native settings disagree.
        Json seed = restored;
        for (int bit = 0; bit < 9; ++bit)
            seed["mm"]["settings"][std::string("gRando.Options.") + options[bit]] = int(!enabled(bit));
        seed["mm"]["settings"]["unrelated"] = 17;
        const auto original = seed;
        const auto replay = MmSettingsForPlaythrough(seed);
        assert(seed == original && replay["unrelated"] == 17);
        for (int bit = 0; bit < 9; ++bit)
            assert(replay[std::string("gRando.Options.") + options[bit]] ==
                   int(bit == 1 ? enabled(0) && enabled(1) : enabled(bit)));
        for (const char* envelope : {"faroreRules", "spellRules", "gearRules"}) {
            auto invalid = seed; invalid[envelope]["version"] = 99;
            bool rejected = false;
            try {MmSettingsForPlaythrough(invalid);} catch (const std::runtime_error&) {rejected = true;}
            assert(rejected);
        }
    }
    // Historical seeds without these envelopes keep the existing off defaults.
    FaroreRules farore{true, true, true}; SpellRules spell{true, true}; GearRules gear{true, true, true, true};
    assert(ReadFaroreRules(Json(), farore) && !farore.crossAge && !farore.spellWindMm && !farore.crossGameFw);
    assert(ReadSpellRules(Json(), spell) && !spell.spellFireMm && !spell.spellLoveMm);
    assert(ReadGearRules(Json(), gear) && !gear.bootsIronMm && !gear.bootsHoverMm && !gear.tunicGoronMm && !gear.tunicZoraMm);
    Json legacy;
    for (const char* option : options) legacy["mm"]["settings"][std::string("gRando.Options.") + option] = 1;
    const auto original = legacy;
    const auto replay = MmSettingsForPlaythrough(legacy);
    assert(legacy == original);
    for (const char* option : options) assert(replay[std::string("gRando.Options.") + option] == 0);
}
int main(int argc, char**) {
    if (argc > 1) TestWriter(); else TestOracle();
    std::cout << "headless age oracle and immutable rule snapshots passed\n";
}
'''
cpp = work/'headless_cross_age.cpp'
cpp.write_text(prelude + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
subprocess.run([str(exe), 'writer'], check=True, timeout=hostcxx.RUN_TIMEOUT)
