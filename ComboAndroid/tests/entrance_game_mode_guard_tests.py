"""Run RunComboFill's production start for every Home game mode and every entrance setting value (A-08).

The code from the game mode read to the generation attempts is compiled as it is in combo/ComboShip.cpp, with
the engine reads stubbed. A solo randomizer mode with an entrance pool set fails there with the plain solo
reason, before the attempts that hold every dump, entrance draw and fill, and no setting is written. The vanilla
modes return a seed without an entranceLayout and never read an entrance setting. Combined mode reaches the
attempts unchanged. Built in the SHIPPING and DEV configurations: a refusal is expected exactly when the build
admits Wallmaster Shuffle (Own Game).
"""
from pathlib import Path
import argparse
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
a.build_dir.mkdir(parents=True, exist_ok=True)

ship = (a.source/'combo/ComboShip.cpp').read_text(encoding='utf-8')
fill = ship[hostcxx.find(ship, 'static void RunComboFill('):]
fill = fill[:fill.index('\n}\n')]
start = hostcxx.find(fill, 'const auto gameMode = sProfileAttempt ?')
end = hostcxx.find(fill, 'nlohmann::json entranceLayout;', start)
segment = fill[start:end]


def ordered(text, *needles):
    at = 0
    for needle in needles:
        at = hostcxx.find(text, needle, at) + 1


# The guard follows the settings read and precedes the attempts, which hold the dumps, the entrance draws and the
# fill. The compiled start writes no setting, dumps no pool and seeds no RNG.
ordered(fill, 'const auto gameMode = sProfileAttempt ?', 'if (ComboRando::VanillaMode(gameMode)) {',
        'ComboRando::PrepareVanillaRun(', 'publish.result = 2;',
        'ReadEntranceSettings(readSetting, entranceCaps);',
        'if (!combinedMode && !ComboRando::Entrances::MakePools(entranceSettings).empty()) {',
        'fail(ComboRando::EntranceGenerationRefusal(modeFacts).player.c_str());', 'return;',
        'nlohmann::json entranceLayout;', 'for (uint64_t attempt = 0;', 'SOH_DumpRandoStaticData()',
        'GenerateEntranceLayout(unified, sEntranceNative, sTempleProbe)', 'CrossWorldCombinedFill(')
for needle in ('sWriteGameplayRule', 'DumpRandoStaticData', 'InactiveGameDump', 'GenerateEntranceLayout',
               'CrossWorldCombinedFill', 'SetComboRandoSeed', 'CwRng', 'ApplyNativeSeed', 'CVarSet'):
    assert needle not in segment, needle

hash_start = hostcxx.find(ship, 'static uint32_t ComboHash(const char* str) {')
combo_hash = ship[hash_start:ship.index('\n}\n', hash_start) + 3]

program = r'''
#include "ComboRandomizerProfiles.h"
#include "rando/GenerationRules.h"
#include "rando/SingleGameFill.h"
#include "rando/entrances/MmOrdinaryGenerationSettings.h"
#include "rando/entrances/UnifiedBridge.h"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>
using Json = nlohmann::json;
static std::optional<ComboRando::Profiles::Attempt> sProfileAttempt;
static std::map<std::string, int> cvars;
static std::vector<std::string> reads;
static int writes = 0, spoilers = 0;
static int32_t ReadRule(const char* key, int32_t fallback) {
    reads.emplace_back(key);
    const auto it = cvars.find(key);
    return it == cvars.end() ? fallback : it->second;
}
static void WriteRule(const char*, int32_t) { ++writes; }
static int32_t (*sReadGameplayRule)(const char*, int32_t) = ReadRule;
static void (*sWriteGameplayRule)(const char*, int32_t) = WriteRule;
static int (*SOH_ReadComboGoalCVars)(int*, int*) = nullptr;
static int (*SOH_ReadComboGoalTypeCVars)(int*, int*) = nullptr;
static int (*SOH_ReadComboStartingGameCVar)(void) = nullptr;
static uint32_t (*SOH_ReadComboSharedCVars)(void) = nullptr;
static std::string g_ConsolidatedJson, g_FinalizeOotApply;
static uint32_t g_FinalizeMasterSeed = 0, g_FinalizeDisplaySeed = 0;
static std::filesystem::path g_FinalizeSpoilerPath;
static Json SnapshotGameplayRules() { return Json::object(); }
static std::filesystem::path WriteComboSpoiler(const Json&, const std::string&) { ++spoilers; return "seed.json"; }
'''
program += combo_hash + r'''
struct Run {
    struct { int result = 0; } publish;
    std::string error;
    bool reachedAttempts = false;
};
static void Fill(std::string inputSeed, Run& run) {
    auto& publish = run.publish;
    const auto fail = [&](const char* msg) {
        run.error = msg;
        publish.result = 1;
    };
'''
program += segment + r'''
    run.reachedAttempts = true;
}
static Run StartRun(std::optional<ComboGameMode> mode, const Json& setupCVars) {
    sProfileAttempt.reset();
    if (mode)
        sProfileAttempt = ComboRando::Profiles::Attempt{
            1, 7, 2, Json{ { "gameMode", ComboRando::kGameModeKeys[*mode] }, { "setupCVars", setupCVars } }, "seed" };
    reads.clear();
    writes = spoilers = 0;
    g_ConsolidatedJson.clear();
    const auto before = cvars;
    Run run;
    Fill("seed", run);
    assert(cvars == before && writes == 0); // The setting stays as the player chose it.
    return run;
}
int main() {
    using namespace ComboRando;
    const auto caps = Entrances::ActiveCapabilities();
    Entrances::EntranceSettingsFacts solo;
    solo.combinedMode = false;
    const auto reason = EntranceGenerationRefusal(solo).player;
    assert(reason == "Wallmaster Shuffle works only in combined mode.");
    int refused = 0, pooled = 0, cases = 0;
    for (const auto& row : Entrances::kEntranceSettings)
        for (int value = 0; value < int(row.choices.size()); ++value) {
            cvars = { { row.cvar, value } };
            const auto settings = Entrances::ReadEntranceSettings(ReadRule, caps);
            const bool pools = !Entrances::MakePools(settings).empty();
            pooled += pools;
            for (int m = -1; m < 5; ++m) {
                const auto mode = m < 0 ? std::nullopt : std::optional(static_cast<ComboGameMode>(m));
                const auto run = StartRun(mode, Json{ { row.cvar, value } });
                ++cases;
                if (mode && VanillaMode(*mode)) {
                    assert(run.publish.result == 2 && run.error.empty() && !run.reachedAttempts && spoilers == 1);
                    const auto seed = Json::parse(g_ConsolidatedJson);
                    assert(ReadGameMode(seed) == *mode && !seed.contains("entranceLayout"));
                    for (const auto& key : reads)
                        assert(key.rfind("gCombo.Rando.Entrances.", 0) != 0); // Ignored, never read.
                } else if (mode && !CombinedMode(*mode) && pools) {
                    assert(run.publish.result == 1 && run.error == reason && !run.reachedAttempts && !spoilers);
                    ++refused;
                } else {
                    assert(run.publish.result == 0 && run.error.empty() && run.reachedAttempts && !spoilers);
                }
            }
        }
    // Each pool the build admits is refused once per solo mode. Wallmaster Shuffle (Own Game) is among them exactly
    // when the build offers it.
    assert(refused == 2 * pooled);
    const auto* wall = Entrances::FindEntranceSetting("erWallmasters");
    assert(wall && (pooled > 0) == wall->supports(1, caps.Selected("")));
    std::cout << "PASS: " << cases << " game mode starts; " << refused
              << " solo refusals before any dump, entrance draw or fill; vanilla ignores the entrance settings\n";
}
'''
for dev in (0, 1):
    cpp = a.build_dir/f'game_mode_guard_{dev}.cpp'
    cpp.write_text(program, encoding='utf-8', newline='\n')
    exe = hostcxx.build(a.compiler, cpp, includes=[a.source/'combo', a.json_include], warnings=False,
                        defines=(['COMBO_ENTRANCE_DEV=1'] if dev else []) + ['_CRT_SECURE_NO_WARNINGS'],
                        msvc=['/bigobj'], exe=a.build_dir/f'game_mode_guard_{dev}.exe')
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
