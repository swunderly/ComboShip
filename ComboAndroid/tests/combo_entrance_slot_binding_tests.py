"""Execute actual slot binds and pending-seed checks with legacy, dev and shipping layouts."""
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
#include "rando/GameMode.h"
ComboGameMode sBoundGameMode = COMBO_MODE_COMBINED;
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/MmUnifiedLayout.h"
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
    layout.clear();
    ActiveMmUnified().clear();
    if (seed.contains("throwOnApply")) throw std::runtime_error("injected native failure");
    if (seed.contains("entranceLayout")) ActiveMmUnified().install(seed, ActiveCapabilities());
    const auto rows = DecodeMmWallmasterSeed(seed);
    if (rows) layout.install(*rows);
    installedSeed = seed;
    calls.push_back("entrances");
    return 1;
}
const char* dumpSeed() {
    dump = installedSeed.contains("breakRoundtrip") ? "{}" : installedSeed.dump();
    return dump.c_str();
}
static NativeFns sEntranceNative{applySeed, nullptr, nullptr, dumpSeed};
// OTRGlobals.cpp's setter: the bound slot's ER-wide runtime flags (M1-09).
std::vector<uint32_t> runtimeFlags;
void setRuntimeFlags(uint32_t flags) { runtimeFlags.push_back(flags); calls.push_back("er-flags"); }
auto SOH_SetEntranceRuntimeFlags = setRuntimeFlags;
'''
source += function('static bool InstallEntranceSeed(const nlohmann::json& seed)') + '\n'
source += function('static bool CheckPendingEntranceSeed(const nlohmann::json& seed)') + '\n'
source += function('static void Combo_OnOOTSaveLoad(int fileNum) {') + '\n'
source += r'''
Json seedFor(int slot) {
    return {{"slot", slot}, {"songTravel", {{"slot", slot}}}, {"fileType", "ComboShipRandomizer"}};
}
void expectCalls(bool loaded, bool succeeded) {
    std::vector<std::string> expected{"rules", "visited", "restore", "capture", "completion",
        "entrances", "er-flags", "song", "runtime", "oot-rando", "mm-rando", "hints"};
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
    assert(runtimeFlags.back() == (installedSeed.empty() ? 0u : ComboRando::SeedEntranceRuntimeFlags(saved)));
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
    // A unified layout this build refuses binds the rest of the slot, with no ER-wide runtime flag.
    EntranceLayout tranche;
    tranche.devBuild = true;
    tranche.required = {std::string(kMmTrancheCapability)};
    tranche.settings.erWallmasters = Mode::OwnGame;
    Overrides routes;
    i = 0;
    for (const auto source : kMmWallmasterSources)
        routes[LayoutV1::FindRow(source)->name] = LayoutV1::FindRow(kMmBaseWallmasterDestinations[i++])->name;
    tranche.overrides = routes;
    auto layoutSeed = seedFor(0);
    layoutSeed["fileType"] = ComboRando::kEntranceSeedFileType;
    layoutSeed["version"] = ComboRando::kEntranceSeedVersion;
    layoutSeed["entranceLayout"] = EncodeEntranceLayout(tranche);
    containers[0] = {{"combo", {{"rando", layoutSeed}}}};
    bind(0, true, true);
    if (ActiveCapabilities().Empty()) {
        assert(installedSeed.empty() && !ActiveMmUnified().active() && runtimeFlags.back() == 0);
    } else {
        assert(installedSeed == layoutSeed && ActiveMmUnified().active() && runtimeFlags.back() == 1);
        const auto immutable = containers[0]["combo"]["rando"];
        bind(0, false, true); // same-slot reload uses the recorded routes, never a fresh shuffle
        assert(containers[0]["combo"]["rando"] == immutable && installedSeed == layoutSeed);
        bind(2, true, true);
        assert(!ActiveMmUnified().active() && runtimeFlags.back() == 0);
    }
    // Original pending-import/finalize behavior installed its layout into another live slot.
    assert(InstallEntranceSeed(valid));
    const auto legacyLive = installedSeed;
    assert(InstallEntranceSeed(seedFor(2)) && installedSeed != legacyLive); // negative control
    assert(InstallEntranceSeed(valid));
    auto flagsBefore = runtimeFlags.size();
    assert(CheckPendingEntranceSeed(seedFor(2)) && installedSeed == legacyLive);
    assert(layout.destination(kMmWallmasterSources[0]) && runtimeFlags.size() == flagsBefore);
    auto malformed = valid;
    malformed["entranceExpansion"]["schemaVersion"] = 99;
    assert(!CheckPendingEntranceSeed(malformed) && installedSeed == legacyLive);
    auto failing = seedFor(2);
    failing["throwOnApply"] = true;
    assert(!CheckPendingEntranceSeed(failing) && installedSeed == legacyLive);
    // Failed creation restores the preceding layout; successful creation explicitly commits.
    try {
        NativeScope transaction(sEntranceNative);
        assert(InstallEntranceSeed(seedFor(2)));
        throw std::runtime_error("MM save creation failed");
    } catch (const std::runtime_error&) {}
    assert(installedSeed == legacyLive);
    {
        NativeScope transaction(sEntranceNative);
        assert(InstallEntranceSeed(seedFor(2)));
        transaction.Commit();
    }
    assert(installedSeed == seedFor(2) && !layout.destination(kMmWallmasterSources[0]));
    const auto noErLive = installedSeed;
    const bool accepted = CheckPendingEntranceSeed(layoutSeed);
    assert(accepted == !ActiveCapabilities().Empty());
    assert(installedSeed == noErLive && !ActiveMmUnified().active());
    if (!ActiveCapabilities().Empty()) {
        assert(InstallEntranceSeed(layoutSeed));
        const auto erLive = installedSeed;
        assert(CheckPendingEntranceSeed(seedFor(2)) && installedSeed == erLive);
        assert(ActiveMmUnified().active());
        auto broken = layoutSeed;
        broken["breakRoundtrip"] = true;
        assert(!CheckPendingEntranceSeed(broken) && installedSeed == erLive);
        assert(ActiveMmUnified().active() && runtimeFlags.size() == flagsBefore);
    }
    std::cout << "PASS production slot bind continues after entrance rejection; immutable seed, stale layout, "
                 "song/items/hints/rules and dormant MM residency stay slot-isolated\n";
}
'''
cpp = args.build_dir / 'entrance_slot_binding.cpp'
cpp.write_text(source, encoding='utf-8')
for dev in (False, True):
    exe = hostcxx.build(args.compiler, cpp, warnings=False, includes=[args.source / 'combo', args.json_include],
                       defines=['COMBO_ENTRANCE_DEV'] if dev else [],
                       exe=args.build_dir / ('entrance_slot_binding_dev.exe' if dev else 'entrance_slot_binding_ship.exe'))
    subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)

# Pending validation must stay temporary at both actual call sites, and creation commits only after success.
assert hostcxx.contains(function('static void Combo_FinalizeGenerate()'), 'if (!CheckPendingEntranceSeed(seed))')
assert hostcxx.contains(function('static int Combo_OnReloadRequestImpl('), 'if (!CheckPendingEntranceSeed(j))')
creation = function('static void Combo_OnOOTSaveInit(int fileNum)')
assert hostcxx.find(creation, 'std::make_unique<ComboRando::Entrances::NativeScope>') < hostcxx.find(creation, '!InstallEntranceSeed(seed)')
assert hostcxx.contains(creation, 'if (mmInitResult == 0) { g_MmSaveInMemorySlot = fileNum; entranceRestore->Commit();')
