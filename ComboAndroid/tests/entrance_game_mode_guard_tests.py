"""Generate in every Home game mode with every entrance setting value, through the real entry points (A-08).

Both Generate buttons start in ProfileBeginGeneration: the menu's through ProfileGenerate, the file-select one
(z_file_choose.c -> SOH_TriggerComboGenerate) through Combo_OnGenerateThreaded. That applies the file's draft through
ProfileModeSetup, which turns every entrance setting off for a solo run, so the solo refusal must read the draft
itself. A solo randomizer mode with an entrance pool set stops there with the plain solo reason, before the native
snapshot, the generation attempt, any applied setting or the worker. Every other draft generates: the CVars
ProfileApplyNative writes then run the start of RunComboFill. The vanilla modes return a seed without an
entranceLayout and never read an entrance setting; combined mode and solo modes without a pool reach the attempts.
RunComboFill's own check stays as a backstop and still refuses a pool it sees in a solo run. The profile functions
(combo/ComboRandomizerProfileNative.inc) and the launcher functions (combo/ComboShip.cpp) are compiled as they are,
with the engine reads stubbed. Built in the SHIPPING and DEV configurations: a refusal is expected exactly when the
build admits Wallmaster Shuffle (Own Game).
"""
from pathlib import Path
import argparse
import re
import shutil
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
native = (a.source/'combo/ComboRandomizerProfileNative.inc').read_text(encoding='utf-8')


def body(text, signature):
    """The production function that starts at signature, through its closing brace."""
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function ' + signature)


def ordered(text, *needles):
    at = 0
    for needle in needles:
        at = hostcxx.find(text, needle, at) + 1


fill = ship[hostcxx.find(ship, 'static void RunComboFill('):]
fill = fill[:fill.index('\n}\n')]
start = hostcxx.find(fill, 'const auto gameMode = sProfileAttempt ?')
end = hostcxx.find(fill, 'nlohmann::json entranceLayout;', start)
segment = fill[start:end]
# The backstop follows the settings read and precedes the attempts, which hold the dumps, the entrance draws and the
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
# The file-select Generate (SOH_TriggerComboGenerate's callback) starts a profile generation before its worker.
threaded = body(ship, 'static void Combo_OnGenerateThreaded(const char* inputSeed) {')
ordered(threaded, 'ProfileBeginGeneration(slot, profileSeed.c_str());', 'g_ComboProgress.SetError(e.what());',
        'return;', 'g_GenerateThread = std::thread(')
assert hostcxx.contains(ship, 'sProfileTrigger = (int (*)(const char*))GetSym(sohModule, "SOH_TriggerProfileGenerate");')
trigger = body((a.source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8'),
               'extern "C" COMBO_EXPORT void SOH_TriggerComboGenerate(void) {')
assert hostcxx.contains(trigger, 'gComboGenerateRequestCallback(seed);')
assert hostcxx.contains(ship, 'SOH_SetOnComboGenerateRequestCallback(Combo_OnGenerateThreaded);')
assert hostcxx.contains((a.source/'soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c')
                        .read_text(encoding='utf-8'), 'SOH_TriggerComboGenerate();')

profile = '\n'.join(body(native, signature) for signature in (
    'static nlohmann::json ProfileNativeSnapshot() {',
    'static nlohmann::json ProfileEditableSetup(nlohmann::json setup){',
    'static nlohmann::json ProfileDraftSetup(const nlohmann::json& requested){',
    'static nlohmann::json ProfileModeSetup(const nlohmann::json& requested) {',
    'static void ProfileRefuseSoloEntrances(const nlohmann::json& requested) {',
    'static void ProfileValidateNative(const nlohmann::json& requested) {',
    'static void ProfileApplyNative(const nlohmann::json& requested) {',
    'static void ProfileRestoreGenerationSetup() {',
    'template<class F> static const char* ProfileResponse(F action) {'))
# The failed-attempt state, ProfileFinishFailedAttempt, ProfileBeginGeneration and ProfileGenerate, in order.
profile += '\n' + native[hostcxx.find(native, 'static bool sProfileFailurePending=false;'):
                         hostcxx.find(native, 'static const char* ProfileCancel(){')]
profile += '\n' + body(native, 'static void ProfileCompleteGeneration() {')
launcher = '\n'.join(body(ship, signature) for signature in (
    'static uint32_t ComboHash(const char* str) {', 'static int ProfileNativeSlot() {'))

program = r'''
#include "ComboRandomizerProfileRepository.h"
#include "gui/ComboGenProgress.h"
#include "gui/ProfileOotOptionPolicy.h"
#include "rando/GameplayRuleSnapshots.h"
#include "rando/GenerationRules.h"
#include "rando/SingleGameFill.h"
#include "rando/SongTravel.h"
#include "rando/entrances/EntranceProfile.h"
#include "rando/entrances/MmOrdinaryGenerationSettings.h"
#include "rando/entrances/UnifiedBridge.h"
#include <atomic>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>
using Json = nlohmann::json;
typedef const char* (*FnDumpData)(void);
constexpr int kSlot = 2; // the file under the file-select cursor and the menu's selected file
// soh.dll's combo CVars: applying a profile writes them, generation reads them.
static std::map<std::string, int> cvars;
static std::vector<std::string> reads;
static int applies = 0, snapshots = 0, triggers = 0, fills = 0, writes = 0, spoilers = 0;
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
static const char* DumpNativeSettings() { return "{}"; }
static const char* DumpCombo() { // SOH_DumpComboSetupCVars
    ++snapshots;
    static std::string text;
    Json values = Json::object();
    for (const auto& [key, value] : cvars)
        values[key] = value;
    text = values.dump();
    return text.c_str();
}
static int ApplyCombo(const char* raw) { // SOH_ApplyComboSetupCVars: replaces every gCombo.Rando. CVar
    ++applies;
    for (auto it = cvars.begin(); it != cvars.end();)
        it = it->first.rfind("gCombo.Rando.", 0) == 0 ? cvars.erase(it) : std::next(it);
    const auto values = Json::parse(raw);
    for (const auto& value : values.items())
        if (value.value().is_number_integer())
            cvars[value.key()] = value.value().get<int>();
    return 1;
}
static int Accept(const char*) { return 1; }
static FnDumpData SOH_DumpRandoSettings = DumpNativeSettings, MM_DumpRandoSettings = DumpNativeSettings;
static FnDumpData sProfileDumpCombo = DumpCombo, sProfileLegacySeed = nullptr;
static int (*sProfileApplyCombo)(const char*) = ApplyCombo;
static int (*sProfileApplyOot)(const char*) = Accept;
static int (*sProfileApplyMm)(const char*) = Accept;
static int (*sProfileValidateOot)(const char*) = Accept;
static int (*sProfileValidateMm)(const char*) = Accept;
static uint8_t CanGenerate() { return 1; }
static uint8_t (*sProfileCanGenerate)() = CanGenerate;
static void Combo_OnGenerateThreaded(const char* inputSeed);
// SOH_TriggerProfileGenerate: the generate-request callback, which is Combo_OnGenerateThreaded.
static int Trigger(const char* seed) {
    ++triggers;
    Combo_OnGenerateThreaded(seed);
    return 1;
}
static int (*sProfileTrigger)(const char*) = Trigger;
static int FileSelectCursor() { return kSlot; }
static int (*sFileSelectSlot)() = FileSelectCursor;
static int (*sProfileSelected)() = nullptr;
static int (*MM_CrossAgeOptionReady)() = nullptr;
static std::unique_ptr<ComboRando::Profiles::Repository> sProfiles;
static std::optional<ComboRando::Profiles::Attempt> sProfileAttempt;
static Json sProfileOriginalSetup, sProfileCatalog;
static int sProfileOriginalRuleSlot = -1, sGameplayRulesSlot = -1;
static ComboRando::SongTravelMap sSongTravel;
static void InstallSongTravel(const Json&) {}
static void RestoreGameplayRules(const Json&, int) {}
static Json SnapshotGameplayRules() { return Json::object(); }
static Json Combo_ReadSavedSeedReadonly(int) { return Json(); }
static std::atomic<bool> g_GenerateBusy{ false }, g_ComboPendingFinalize{ false };
static ComboRando::ComboGenProgress g_ComboProgress;
static std::thread g_GenerateThread;
static std::string g_ConsolidatedJson, g_FinalizeOotApply;
static uint32_t g_FinalizeMasterSeed = 0, g_FinalizeDisplaySeed = 0;
static std::filesystem::path g_FinalizeSpoilerPath;
static std::filesystem::path WriteComboSpoiler(const Json&, const std::string&) { ++spoilers; return "seed.json"; }
'''
program += launcher + r'''
struct Run {
    struct { int result = 0; } publish;
    std::string error;
    bool reachedAttempts = false;
};
static Run lastRun;
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
// The worker: the start of RunComboFill, then the busy flag its PublishResult clears.
static void RunComboFill(std::string inputSeed, ComboRando::ComboGenProgress*, bool) {
    ++fills;
    lastRun = Run{};
    Fill(inputSeed, lastRun);
    g_GenerateBusy = false;
}
'''
program += profile + '\n' + body(ship, 'static void Combo_OnGenerateThreaded(const char* inputSeed) {') + r'''
int main(int argc, char** argv) {
    using namespace ComboRando;
    assert(argc == 2);
    const auto caps = Entrances::ActiveCapabilities();
    sProfiles = std::make_unique<Profiles::Repository>(std::filesystem::path(argv[1]) / "profiles.json",
                                                       Json{ { "fileType", kComboSeedFileType } }, caps);
    Entrances::EntranceSettingsFacts solo;
    solo.combinedMode = false;
    const auto reason = EntranceGenerationRefusal(solo).player;
    assert(reason == "Wallmaster Shuffle works only in combined mode.");
    // The player's own CVars before Generate: every entrance setting off, and one more combo setting.
    std::map<std::string, int> live = { { "gCombo.Rando.StartingGame", 2 } };
    for (const auto& row : Entrances::kEntranceSettings)
        live[row.cvar] = 0;
    int refused = 0, pooled = 0, cases = 0;
    for (const auto& row : Entrances::kEntranceSettings)
        for (int value = 0; value < int(row.choices.size()); ++value) {
            const Json values = { { row.cvar, value } };
            // The value this build generates with: one it does not admit is reset (NormalizeEntranceDraft).
            const auto settings = Entrances::ReadEntranceSettings(
                [&](const char* key, int fallback) { return values.value(key, fallback); }, caps);
            const bool pools = !Entrances::MakePools(settings).empty();
            pooled += pools;
            for (int m = 0; m < 5; ++m)
                for (const bool fileSelect : { false, true }) {
                    const auto mode = static_cast<ComboGameMode>(m);
                    sProfiles->setDraft(kSlot, Json{ { "fileType", kComboSeedFileType },
                                                     { "version", 1 },
                                                     { "gameMode", kGameModeKeys[m] },
                                                     { "seed", "guard" },
                                                     { "setupCVars", values } });
                    const auto stored = sProfiles->view().json();
                    cvars = live;
                    reads.clear();
                    applies = snapshots = triggers = fills = writes = spoilers = 0;
                    g_ComboProgress.Reset();
                    g_ConsolidatedJson.clear();
                    lastRun = Run{};
                    std::string error;
                    if (fileSelect) {
                        Combo_OnGenerateThreaded("legacy"); // file select: SOH_TriggerComboGenerate's callback
                    } else {
                        const auto reply = Json::parse(ProfileGenerate(kSlot, "guard")); // the menu's Generate
                        error = reply.value("error", std::string());
                        assert(error.empty() == reply.value("success", false));
                    }
                    if (g_GenerateThread.joinable())
                        g_GenerateThread.join();
                    ++cases;
                    if (!CombinedMode(mode) && !VanillaMode(mode) && pools) {
                        // Refused from the draft: nothing read from the games, recorded, applied or run, and the
                        // setting stays as the player chose it.
                        if (fileSelect)
                            assert(g_ComboProgress.done && !g_ComboProgress.running && !g_ComboProgress.success &&
                                   g_ComboProgress.error == reason);
                        else
                            assert(error == reason);
                        assert(!sProfileAttempt && !snapshots && !applies && !triggers && !fills && !writes);
                        assert(cvars == live && sProfiles->view().json() == stored);
                        ++refused;
                        continue;
                    }
                    assert(error.empty() && sProfileAttempt && snapshots == 1 && applies == 1 && fills == 1);
                    assert(triggers == (fileSelect ? 0 : 1) && !writes);
                    // ProfileModeSetup's CVars: a solo or vanilla run has every entrance setting off, so
                    // RunComboFill's backstop sees no pool; combined mode keeps the draft's value.
                    for (const auto& other : Entrances::kEntranceSettings)
                        if (!CombinedMode(mode))
                            assert(cvars.at(other.cvar) == 0);
                        else if (&other == &row)
                            assert(cvars.at(row.cvar) == row.get(settings));
                    if (VanillaMode(mode)) {
                        assert(lastRun.publish.result == 2 && lastRun.error.empty() && !lastRun.reachedAttempts);
                        const auto seed = Json::parse(g_ConsolidatedJson);
                        assert(spoilers == 1 && ReadGameMode(seed) == mode && !seed.contains("entranceLayout"));
                        for (const auto& key : reads)
                            assert(key.rfind("gCombo.Rando.Entrances.", 0) != 0); // Ignored, never read.
                    } else {
                        assert(lastRun.publish.result == 0 && lastRun.error.empty() && lastRun.reachedAttempts);
                        assert(!spoilers);
                    }
                    // Finishing the attempt puts the player's own CVars back.
                    g_ComboProgress.success = false;
                    ProfileCompleteGeneration();
                    assert(!sProfileAttempt && cvars == live);
                }
        }
    // Each pool the build admits is refused once per solo mode and Generate button. Wallmaster Shuffle (Own Game) is
    // among them exactly when the build offers it.
    assert(refused == 2 * 2 * pooled);
    const auto* wall = Entrances::FindEntranceSetting("erWallmasters");
    assert(wall && (pooled > 0) == wall->supports(1, caps.Selected("")));
    // The backstop: a solo run whose CVars still hold a pool stops before the attempts and writes nothing.
    int backstops = 0;
    for (const auto& row : Entrances::kEntranceSettings)
        for (int value = 0; value < int(row.choices.size()); ++value)
            for (const int m : { int(COMBO_MODE_OOT_RANDO), int(COMBO_MODE_MM_RANDO) }) {
                cvars = { { row.cvar, value } };
                const auto before = cvars;
                if (Entrances::MakePools(Entrances::ReadEntranceSettings(ReadRule, caps)).empty())
                    continue;
                sProfileAttempt = Profiles::Attempt{
                    1, 7, kSlot, Json{ { "gameMode", kGameModeKeys[m] }, { "setupCVars", Json{ { row.cvar, value } } } },
                    "seed" };
                writes = 0;
                Run run;
                Fill("seed", run);
                assert(run.publish.result == 1 && run.error == reason && !run.reachedAttempts);
                assert(cvars == before && !writes);
                ++backstops;
            }
    sProfileAttempt.reset();
    assert(backstops == 2 * pooled);
    std::cout << "PASS: " << cases << " generations from both Generate buttons; " << refused
              << " solo refusals from the draft before anything is applied; " << backstops
              << " backstop refusals; vanilla ignores the entrance settings\n";
}
'''
for dev in (0, 1):
    cpp = a.build_dir/f'game_mode_guard_{dev}.cpp'
    cpp.write_text(program, encoding='utf-8', newline='\n')
    exe = hostcxx.build(a.compiler, cpp, includes=[a.source/'combo', a.json_include], warnings=False,
                        defines=(['COMBO_ENTRANCE_DEV=1'] if dev else []) + ['_CRT_SECURE_NO_WARNINGS'],
                        msvc=['/bigobj', '/Zc:preprocessor', '/utf-8'], exe=a.build_dir/f'game_mode_guard_{dev}.exe')
    sandbox = a.build_dir/f'game_mode_guard_{dev}'
    shutil.rmtree(sandbox, ignore_errors=True)
    sandbox.mkdir()
    subprocess.run([str(exe), str(sandbox.resolve())], check=True, timeout=hostcxx.RUN_TIMEOUT)
