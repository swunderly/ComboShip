"""Execute the production save-load callback through rejected legacy entrance layouts."""
from pathlib import Path
import argparse
import subprocess

import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--json-include', type=Path, required=True)
args = parser.parse_args()
text = (args.source / 'combo/ComboShip.cpp').read_text(encoding='utf-8')
args.build_dir.mkdir(parents=True, exist_ok=True)


def function(signature):
    start = hostcxx.find(text, signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


source = r'''
#include <cassert>
#include <iostream>
#include <mutex>
#include <array>
#include <vector>
#include "rando/entrances/NativeBridge.h"
using Json = nlohmann::json;
using namespace ComboRando::Entrances;
std::array<Json, 3> containers;
std::mutex g_containerMutex;
int g_ComboContextEpoch = 0, g_MmSaveInMemorySlot = -1, completionSlot = -1;
int nativeLoads = 0, resyncs = 0, loadResult = 0;
std::vector<std::string> calls;
Json installedSeed, songTravel, ootSeed, mmSeed;
MmWallmasterLayout layout;
std::string dump;
bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
Json& LoadOrCreateContainer(int slot) { return containers.at(slot); }
Json RuleBaseline() { return Json::object(); }
namespace ComboRando {
Json GameplayRulesForRun(const Json& seed, const Json&, const Json&) {
    calls.push_back("rules"); return {{"slot", seed.at("slot")}};
}
}
void setVisited(int) { calls.push_back("visited"); }
auto SOH_SetMmVisited = setVisited;
void RestoreGameplayRules(const Json& rules, int slot) {
    assert(rules.at("slot") == slot); calls.push_back("restore");
}
void CaptureSaveRules(int) { calls.push_back("capture"); }
void LoadComboCompletion(int slot) { completionSlot = slot; calls.push_back("completion"); }
void InstallSongTravel(const Json& value) { songTravel = value; calls.push_back("song"); }
void RefreshGameplayRuntime() { calls.push_back("runtime"); }
void loadOot(const char* value) { ootSeed = Json::parse(value); calls.push_back("oot-rando"); }
void loadMm(const char* value) { mmSeed = Json::parse(value); calls.push_back("mm-rando"); }
auto SOH_LoadComboRando = loadOot;
auto MM_LoadComboRando = loadMm;
void Combo_PushHintTrackerData(int slot) {
    assert(completionSlot == slot); calls.push_back("hints");
}
int loadSave(int slot) {
    assert(g_MmSaveInMemorySlot == -1 && completionSlot == slot);
    ++nativeLoads; calls.push_back("mm-save"); return loadResult;
}
int (*MM_LoadSaveForCombo)(int) = loadSave;
void requestResync() { ++resyncs; calls.push_back("resync"); }
auto MM_Anchor_RequestResync = requestResync;
void Combo_OnTriforceProgress(int game, int slot) {
    assert(game == 0 && completionSlot == slot); calls.push_back("triforce");
}
void Combo_SharedReconcileNow() {
    assert(g_MmSaveInMemorySlot == -1 || g_MmSaveInMemorySlot == completionSlot);
    calls.push_back("reconcile");
}
void Combo_ResumeMMIfLastSavedThere(int slot) {
    assert(completionSlot == slot); calls.push_back("resume");
}
int applySeed(const char* value) {
    const auto seed = Json::parse(value);
    const auto rows = DecodeMmWallmasterSeed(seed);
    layout.clear();
    if (rows) layout.install(*rows);
    installedSeed = seed;
    calls.push_back("entrances");
    return 1;
}
const char* dumpSeed() { dump = installedSeed.dump(); return dump.c_str(); }
static NativeFns sEntranceNative{applySeed, nullptr, nullptr, dumpSeed};
'''
source += function('static bool InstallEntranceSeed(const nlohmann::json& seed)') + '\n'
source += function('static void Combo_OnOOTSaveLoad(int fileNum) {') + '\n'
source += r'''
Json seedFor(int slot) {
    return {{"slot", slot}, {"songTravel", {{"slot", slot}}}, {"fileType", "ComboShipRandomizer"}};
}
void expectCalls(bool loaded, bool succeeded) {
    std::vector<std::string> expected{"rules", "visited", "restore", "capture", "completion",
        "entrances", "song", "runtime", "oot-rando", "mm-rando", "hints"};
    if (loaded) expected.push_back("mm-save");
    if (loaded && succeeded) expected.push_back("resync");
    expected.insert(expected.end(), {"triforce", "reconcile", "resume"});
    assert(calls == expected);
}
void bind(int slot, bool loaded, bool succeeded) {
    const auto saved = containers[slot]["combo"]["rando"];
    calls.clear();
    Combo_OnOOTSaveLoad(slot);
    expectCalls(loaded, succeeded);
    assert(ootSeed == saved && mmSeed == saved);
    assert(songTravel == saved.at("songTravel"));
    assert(containers[slot]["combo"]["rando"] == saved);
    assert(containers[slot]["combo"]["gameplayRules"].at("slot") == slot);
}
int main() {
    std::vector<SavedOverride> rows;
    size_t i = 0;
    for (const auto source : kMmWallmasterSources) {
        const auto* src = Find(source);
        const auto* dst = Find(kMmBaseWallmasterDestinations[i++]);
        rows.push_back({std::string(src->name), std::string(dst->name), src->game, dst->game, src->id, dst->id});
    }
    auto valid = seedFor(0);
    valid["entranceExpansion"] = EncodeMmWallmasterExtension(rows);
    auto rejected = seedFor(1);
    rejected["entranceExpansion"] = valid["entranceExpansion"];
    rejected["entranceExpansion"]["schemaVersion"] = 99;
    containers[0] = {{"combo", {{"rando", valid}}}};
    containers[1] = {{"combo", {{"rando", rejected}}}};
    containers[2] = {{"combo", {{"rando", seedFor(2)}}}};
    bind(0, true, true);
    assert(layout.destination(kMmWallmasterSources[0]) && g_MmSaveInMemorySlot == 0);
    // A rejected saved envelope clears the preceding layout, but all remaining slot binding runs.
    bind(1, true, true);
    assert(installedSeed.empty() && !layout.destination(kMmWallmasterSources[0]));
    assert(g_MmSaveInMemorySlot == 1 && nativeLoads == 2 && resyncs == 2);
    // Same-slot rebind preserves more recent dormant progress without another disk read.
    bind(1, false, true);
    assert(nativeLoads == 2 && resyncs == 2);
    // A failed dormant read must not label the old inventory as the newly selected file.
    bind(0, true, true);
    loadResult = -1;
    bind(1, true, false);
    assert(g_MmSaveInMemorySlot == -1 && installedSeed.empty());
    loadResult = 0;
    bind(2, true, true);
    assert(g_MmSaveInMemorySlot == 2 && !layout.destination(kMmWallmasterSources[0]));
    assert(installedSeed.at("slot") == 2);
    calls.clear();
    const auto epoch = g_ComboContextEpoch;
    Combo_OnOOTSaveLoad(0xff);
    assert(calls.empty() && g_ComboContextEpoch == epoch + 1 && g_MmSaveInMemorySlot == 2);
    std::cout << "PASS production slot bind continues after entrance rejection; immutable seed, stale layout, "
                 "song/items/hints/rules and dormant MM residency stay slot-isolated\n";
}
'''
cpp = args.build_dir / 'entrance_slot_binding.cpp'
cpp.write_text(source, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, warnings=False, includes=[args.source / 'combo', args.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
