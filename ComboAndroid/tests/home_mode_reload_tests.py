"""Execute the production early reload path, with native engine calls recorded."""
from pathlib import Path
import argparse
import subprocess
import sys

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default='cl')
p.add_argument('--json-include', type=Path, required=True)
a = p.parse_args()
sys.path.insert(0, str(a.source / 'ComboAndroid/tests'))
import hostcxx

a.build_dir.mkdir(parents=True, exist_ok=True)
text = (a.source / 'combo/ComboShip.cpp').read_text(encoding='utf-8')
start = text.index('static bool SeedNeedsUnavailableMmGear(')
end = text.index('\n}', start) + 2
gear = text[start:end]
end = text.index('        const auto catalog =', text.rindex('static int Combo_OnReloadRequestImpl('))
start = text.rindex('ComboRando::RequireSupportedSeedDocument(j,', 0, end)
reload = text[start:end]
program = r'''
#include "rando/GameMode.h"
#include "rando/GearRules.h"
#include "mq/MqSeedMarker.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
using Json = nlohmann::json;
namespace ComboRando {
namespace Entrances { int ActiveCapabilities() { return 0; } }
void RequireSupportedSeedDocument(const Json&, int) {}
}
bool allowed = true, entranceAllowed = true;
int gearQueries = 0, entranceQueries = 0, hashes = 0, settingsDumps = 0, seedCalls = 0, readyCalls = 0, writes = 0;
uint32_t lastHash = 0;
uint64_t lastSeed = 0;
std::string g_ConsolidatedJson, g_PendingMMSettingsJson, g_UserMMSettingsSnapshot;
uint32_t g_FinalizeDisplaySeed = 0;
bool g_ComboReloadRestoreUserMM = false;
int GearAllowed(int arg) { assert(arg == 0); ++gearQueries; return allowed; }
int (*MM_GearOptionsAllowed)(int) = GearAllowed;
void SetHash(uint32_t hash) { ++hashes; lastHash = hash; }
void (*SOH_SetComboSeedHash)(uint32_t) = SetHash;
const char* DumpSettings() { ++settingsDumps; return "{\"original\":true}"; }
const char* (*MM_DumpRandoSettings)() = DumpSettings;
void SetSeed(uint64_t seed) { ++seedCalls; lastSeed = seed; }
void (*MM_SetComboRandoSeed)(uint64_t) = SetSeed;
void SetReady(uint8_t value) { assert(value == 1); ++readyCalls; }
void (*SOH_SetSeedGenerated)(uint8_t) = SetReady;
bool CheckPendingEntranceSeed(const Json&) { ++entranceQueries; return entranceAllowed; }
std::string WriteComboSpoiler(const Json&, const std::string&) { ++writes; return "seed.json"; }
void RememberComboSpoiler(const std::string&) {}
bool bundledMq = true;
int mqQueries = 0;
bool BundledMqAvailable() { ++mqQueries; return bundledMq; }
'''
program += gear
program += '\nint Reload(const Json& j, bool profileSeed) {\n' + reload + '\nreturn 2;\n}\n'
program += r'''
void Reset() {
    gearQueries = entranceQueries = hashes = settingsDumps = seedCalls = readyCalls = writes = 0;
    g_ConsolidatedJson = "before"; g_PendingMMSettingsJson = "before-mm";
    g_UserMMSettingsSnapshot = "before-user"; g_FinalizeDisplaySeed = 444;
    g_ComboReloadRestoreUserMM = false; lastHash = 444; lastSeed = 0;
    allowed = entranceAllowed = true; MM_GearOptionsAllowed = GearAllowed;
    bundledMq = true; mqQueries = 0;
}
void Unchanged() {
    assert(g_ConsolidatedJson == "before" && g_PendingMMSettingsJson == "before-mm");
    assert(g_UserMMSettingsSnapshot == "before-user" && g_FinalizeDisplaySeed == 444);
    assert(!g_ComboReloadRestoreUserMM && lastHash == 444);
    assert(!hashes && !settingsDumps && !seedCalls && !readyCalls && !writes && !entranceQueries);
}
int main() {
    for (const char* mode : {"combined", "oot-randomizer", "mm-randomizer", "oot-vanilla", "mm-vanilla"}) {
        Json j = {{"gameMode", mode}, {"displaySeed", 123456u}, {"masterSeed", 987654u},
                  {"file_hash", Json::array({1,2,3,4,5})}, {"mm", {{"settings", {{"original",false}}}}}};
        for (const char* item : {"bootsIronMm", "bootsHoverMm", "tunicGoronMm", "tunicZoraMm"}) {
            // Import and pending-profile admission both reject before any native or pending-state mutation.
            j["gearRules"] = ComboRando::GearRules{}.json();
            j["gearRules"][item] = true;
            for (bool profile : {false, true}) {
                Reset(); allowed = false;
                assert(Reload(j, profile) == 0); assert(gearQueries == 1); Unchanged();
                Reset(); MM_GearOptionsAllowed = nullptr;
                assert(Reload(j, profile) == 0); Unchanged();
            }
        }
        j.erase("gearRules");
        // A seed whose MQ dungeons come from the bundled data is refused before any mutation where that data is
        // off (another ROM version); one with no MQ dungeon, and every unmarked seed, passes as before.
        for (bool profile : {false, true}) {
            Json mq = j;
            mq["oot"] = {{"mqDataSource", "ootmm-32.3"}, {"masterQuestDungeons", Json::array({3, 6})}};
            Reset(); bundledMq = false;
            assert(Reload(mq, profile) == 0); assert(mqQueries == 1); Unchanged();
            Json unknown = j;
            unknown["oot"] = {{"mqDataSource", "ootmm-40.0"}, {"masterQuestDungeons", Json::array()}};
            Reset();
            bool refused = false;
            try { Reload(unknown, profile); } catch (const ComboMq::UnsupportedMqDataSource&) { refused = true; }
            assert(refused); Unchanged(); // production's catch returns 0
            if (ComboRando::ModeRandomizesOot(ComboRando::ReadGameMode(j))) continue;
            for (const Json& oot : {Json::object(), Json{{"mqDataSource", "ootmm-32.3"},
                                                         {"masterQuestDungeons", Json::array()}}}) {
                Json playable = j;
                playable["oot"] = oot;
                Reset(); bundledMq = false;
                assert(Reload(playable, profile) == 1);
            }
        }
        if (ComboRando::ModeRandomizesOot(ComboRando::ReadGameMode(j))) continue;
        for (bool profile : {false, true}) {
            Reset();
            assert(Reload(j, profile) == 1);
            assert(hashes == 1 && lastHash == 123456u && g_FinalizeDisplaySeed == 123456u);
            assert(seedCalls == 1 && lastSeed == 987654u && readyCalls == 1);
            assert(g_ConsolidatedJson == j.dump(2) && g_ComboReloadRestoreUserMM);
            assert(Json::parse(g_PendingMMSettingsJson) == j["mm"]["settings"]);
            assert(Json::parse(g_UserMMSettingsSnapshot)["original"] == true);
            assert(writes == (profile ? 0 : 1));
        }
    }
}
'''
cpp = a.build_dir / 'home_mode_reload.cpp'
cpp.write_text(program, encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, includes=[a.source / 'combo', a.json_include], werror=True)
subprocess.run([str(exe)], cwd=a.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Production reload admits gear and bundled MQ data before mutations and restores solo/vanilla name-entry '
      'hash icons.')
