"""Execute the real plando Load -> edit -> Save & Play path on v2 and legacy seeds.

Only native game apply, module exports, slot queries and the remembered-path CVar are stubbed. The production
JSON parser, placement/foreign serializers, capability guard, file picker, and filesystem writes run.
The real profile import wrapper, repository and save preparation prove native-cursor slot ownership.
Sibling text is deliberately misleading; it must not become an item source or modify saved routes.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess
import uuid
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
menu = (source/'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')
launcher = (source/'combo/ComboShip.cpp').read_text(encoding='utf-8')
profiles = (source/'combo/ComboRandomizerProfileNative.inc').read_text(encoding='utf-8')


def extract(signature, text=menu):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production function')


state = menu[menu.index('struct PlandoPickItem {'):menu.index('static PlandoState sPlando;') + len('static PlandoState sPlando;')]
functions = '\n'.join(extract(name) for name in (
    'int PlandoImportTarget()', 'void PlandoRefreshSpoilerList()', 'void PlandoBuildItems()', 'void PlandoLoad()', 'void PlandoSavePlay()'))
profile_functions = extract('static int ProfileNativeSlot()', launcher) + '\n' + '\n'.join(
    extract(name, profiles) for name in ('static int ProfilePrepareSave(', 'static int Combo_OnReloadRequest('))
support = r'''
#include <cassert>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include "rando/ComboPlaythrough.h"
#include "rando/ComboSeedFormat.h"
#include "ComboRandomizerProfileRepository.h"
namespace fs = std::filesystem;
namespace CE = ComboRando::Entrances;
using Json = nlohmann::json;
void BindGameMode(const Json& seed) { assert(ComboRando::ReadGameMode(seed) == COMBO_MODE_COMBINED); }
using FnDump = const char* (*)(void);
using FnRequestReload = int (*)(const char*);
using FnPlandoFileSelectSlot = int (*)(void);
FnDump sSohDump = nullptr, sMmDump = nullptr;
FnRequestReload sRequestReload = nullptr;
FnPlandoFileSelectSlot sPlandoFileSelectSlot = nullptr;
void ResolvePlandoSyms() {}
int activeSlot = -1, dumpCalls = 0, reloadCalls = 0;
int nativeSlot = 0, profileSelected = 2, nativeApplies = 0;
int NativeSlot() { return nativeSlot; }
int SelectedSlot() { return profileSelected; }
namespace ComboTracker { int OotActiveSlot() { return activeSlot; } }
std::string rememberedPath, reloadedPath;
const char* CVarGetString(const char* key, const char*) {
    assert(std::string(key) == "gGeneral.ComboSpoiler"); return rememberedPath.c_str();
}
std::string Read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary); assert(in.is_open());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path()); std::ofstream out(path, std::ios::binary);
    assert(out.is_open()); out << text;
}
std::unique_ptr<ComboRando::Profiles::Repository> sProfiles;
std::atomic<bool> g_GenerateBusy{false}, g_ComboPendingFinalize{false};
std::atomic<uint64_t> g_ComboContextEpoch{0};
bool sProfileAttempt = false, sProfileLegacyPendingMigration = false;
int sProfilePreparedSlot = -1;
uint64_t sProfilePreparedEpoch = 0;
Json sProfilePreparedSeed, appliedSeed;
int (*sFileSelectSlot)() = NativeSlot;
int (*sProfileSelected)() = SelectedSlot;
uint8_t CanGenerate() { return 1; }
uint8_t (*sProfileCanGenerate)() = CanGenerate;
const char* (*SOH_GetComboSpoilerPath)() = nullptr;
void (*SOH_SetSeedGenerated)(uint8_t) = nullptr;
fs::path ResolveComboSeedPath(const char* path) { return path; }
bool TryLoadComboSeedFile(const fs::path& path, Json& seed) {
    seed = Json::parse(Read(path)); return true;
}
void FindNewestComboSeed(Json&) { assert(false && "unexpected legacy migration"); }
int Combo_OnReloadRequestImpl(const char* path, const Json* supplied = nullptr) {
    const auto seed = supplied ? *supplied : Json::parse(Read(path));
    ComboRando::RequireSupportedSeedDocument(seed, CE::ActiveCapabilities());
    appliedSeed = seed; ++nativeApplies; ++g_ComboContextEpoch;
    return 1;
}
static int Combo_OnReloadRequest(const char* path);
int Reload(const char* path) { ++reloadCalls; reloadedPath = path; return Combo_OnReloadRequest(path); }
const char* OotDump() { ++dumpCalls; return R"({"items":[{"name":"Kokiri Sword"}],"pool":[{"name":"Kokiri Sword","advancement":true}]})"; }
const char* MmDump() { ++dumpCalls; return R"({"items":[{"name":"Deku Mask"}],"pool":[{"name":"Deku Mask","advancement":true}]})"; }
'''
tests = r'''
// Kinds: a legacy seed; a layout seed this build makes (the dev tranche, or the vanilla contract a
// shipping build admits, P1-05); a dev build's layout seed, which a shipping build refuses.
enum SeedKind { kLegacy, kOwnLayout, kDevLayout };
Json BaseSeed(SeedKind kind) {
    Json seed = {
        {"fileType", "ComboShipRandomizer"}, {"seed", "m1-plando"}, {"file_hash", {1,2,3,4,5}},
        {"sharedItems", Json::array()}, {"faroreRules", {{"crossAge", true}}},
        {"goal", {{"type", "ganonMajora"}}}, {"startingGame", "MM"},
        {"oot", {{"settings", {{"preserve", 17}}}, {"placements", {{"OOT Native Check", "Kokiri Sword"}, {"OOT Foreign Check", "Deku Mask"}}}}},
        {"mm", {{"settings", {{"preserve", 23}}}, {"placements", {{"MM Native Check", "Deku Mask"}, {"MM Foreign Check", "Kokiri Sword"}}}}},
        {"foreign", Json::array({
            {{"checkGame", "oot"}, {"checkName", "OOT Foreign Check"}, {"itemGame", "mm"}, {"itemName", "Deku Mask"}, {"advancement", true}},
            {{"checkGame", "mm"}, {"checkName", "MM Foreign Check"}, {"itemGame", "oot"}, {"itemName", "Kokiri Sword"}, {"advancement", true}}
        })}
    };
    const bool tranche = kind == kDevLayout || (kind == kOwnLayout && CE::ActiveCapabilities().devBuild);
    if (kind != kLegacy) {
        CE::EntranceLayout layout;
        layout.settings.erWallmasters = CE::Mode::OwnGame;
        if (tranche) {
            layout.required = {std::string(CE::kMmTrancheCapability)};
            layout.devBuild = true;
            layout.overrides = CE::Overrides{
                {"MM_WALLMASTER_DAMPE", "MM_TEMPLE_WOODFALL"},
                {"MM_WALLMASTER_BTW_ENTRANCE", "MM_TEMPLE_SNOWHEAD"},
                {"MM_WALLMASTER_BTW_FOUNTAIN", "MM_TEMPLE_GREAT_BAY"},
                {"MM_WALLMASTER_BTW_EXIT", "MM_TEMPLE_STONE_TOWER"}};
        } else {
            layout.required = {std::string(CE::kVanillaWallmasterCapability)};
            layout.overrides = CE::Overrides{@VANILLA_ROUTES@};
        }
        seed["entranceLayout"] = CE::EncodeEntranceLayout(layout);
        ComboRando::StampComboSeedFormat(seed);
    }
    return seed;
}
Json ExceptPlacements(Json seed) {
    seed["oot"].erase("placements"); seed["mm"].erase("placements"); seed.erase("foreign"); return seed;
}
void VerifyRows(const std::vector<ComboRando::CwPlacedItem>& rows, bool edited) {
    assert(rows.size() == 4);
    for (const auto& row : rows) {
        if (row.check == "OOT Native Check") {
            assert(row.checkGame == ComboRando::GAME_OOT);
            assert(row.itemGame == (edited ? ComboRando::GAME_MM : ComboRando::GAME_OOT));
            assert(row.item == (edited ? "Deku Mask" : "Kokiri Sword"));
        } else if (row.check == "OOT Foreign Check") {
            assert(row.checkGame == ComboRando::GAME_OOT && row.itemGame == ComboRando::GAME_MM && row.item == "Deku Mask");
        } else if (row.check == "MM Foreign Check") {
            assert(row.checkGame == ComboRando::GAME_MM && row.itemGame == (edited ? ComboRando::GAME_MM : ComboRando::GAME_OOT));
            assert(row.item == (edited ? "Deku Mask" : "Kokiri Sword"));
        } else {
            assert(row.check == "MM Native Check" && row.checkGame == ComboRando::GAME_MM && row.itemGame == ComboRando::GAME_MM && row.item == "Deku Mask");
        }
    }
}
void Case(const fs::path& root, SeedKind kind, int loadMode) {
    fs::create_directories(root); fs::current_path(root);
    const bool unified = kind != kLegacy;
    const auto seed = BaseSeed(kind);
    const std::string original = seed.dump(2);
    const fs::path input = ComboRando::ComboSpoilerPath(seed.at("file_hash"));
    auto sibling = input; sibling.replace_extension(".txt");
    const std::string misleading = R"({"oot":{"placements":{"TEXT ONLY CHECK":"TEXT ONLY ITEM"}},"entranceLayout":{"overrides":[]}})";
    Write(input, original); Write(sibling, misleading);
    sProfiles = std::make_unique<ComboRando::Profiles::Repository>("Randomizer/settings-profiles.json", Json::object(), CE::ActiveCapabilities());
    auto otherSeed = BaseSeed(kLegacy); otherSeed["seed"] = "other-file"; otherSeed["startingGame"] = "OOT";
    sProfiles->importSeed(2, otherSeed, true);
    nativeSlot = 0; profileSelected = 2; sProfilePreparedSlot = -1; sProfilePreparedSeed = nullptr;
    appliedSeed = nullptr; nativeApplies = 0;
    sPlando = {}; activeSlot = -1; rememberedPath = input.string(); dumpCalls = reloadCalls = 0; reloadedPath.clear();
    PlandoRefreshSpoilerList();
    assert(sPlando.spoilerPaths.size() == 1 && sPlando.spoilerPaths.front() == input.string());
    if (loadMode != 0) sPlando.spoilerSel = -1;
    if (loadMode == 1) {
        activeSlot = 2;
        Write("Save/file3.combosav", Json{{"combo", {{"rando", seed}}}}.dump());
    }
    PlandoLoad();
    if (kind == kDevLayout) {
        assert(!sPlando.loaded && sPlando.statusError && sPlando.rows.empty());
        assert(dumpCalls == 0 && reloadCalls == 0 && reloadedPath.empty());
        assert(!fs::exists(ComboRando::ComboSpoilerPath(seed.at("file_hash"), "Combo-Plando")));
        assert(Read(input) == original && Read(sibling) == misleading);
        return;
    }
    assert(sPlando.loaded && !sPlando.statusError && dumpCalls == 2);
    assert(Json::parse(sPlando.loadedJson) == seed);
    VerifyRows(sPlando.rows, false);
    for (auto& row : sPlando.rows) {
        if (row.check == "OOT Native Check" || row.check == "MM Foreign Check") {
            row.item = "Deku Mask"; row.itemGame = ComboRando::GAME_MM;
        }
    }
    PlandoSavePlay();
    assert(reloadCalls == 1 && !sPlando.statusError);
    assert(sPlando.status == "Saved for File 1. Start that file to play it.");
    const auto output = ComboRando::ComboSpoilerPath(seed.at("file_hash"), "Combo-Plando");
    assert(fs::path(reloadedPath) == output && output != input);
    const auto saved = Json::parse(Read(output));
    assert(saved != seed && ExceptPlacements(saved) == ExceptPlacements(seed));
    assert(saved.at("fileType") == seed.at("fileType"));
    if (unified) {
        assert(saved.at("fileType") == ComboRando::kEntranceSeedFileType && saved.at("version") == 2);
        assert(saved.at("entranceLayout") == seed.at("entranceLayout"));
    } else assert(!saved.contains("entranceLayout") && !saved.contains("version"));
    assert(Read(input) == original && Read(sibling) == misleading);
    assert(Json::parse(sPlando.loadedJson) == saved);
    // Menu settings say File3, but the native file-select cursor is File1. Import and
    // acknowledgement must agree, and starting File3 must retain its distinct pending seed.
    assert(sProfiles->view().pendingSeed(0) == saved);
    assert(sProfiles->view().pendingSeed(2) == otherSeed);
    assert(ProfilePrepareSave(2) && appliedSeed == otherSeed);
    assert(appliedSeed.at("startingGame") == "OOT");
    assert(ProfilePrepareSave(0) && appliedSeed == saved);
    assert(appliedSeed.at("startingGame") == "MM");
    if (unified) assert(appliedSeed.at("entranceLayout") == seed.at("entranceLayout"));
    const int beforeCached = nativeApplies;
    assert(ProfilePrepareSave(0) && nativeApplies == beforeCached);
    sPlando.spoilerPaths = {output.string()}; sPlando.spoilerSel = 0;
    PlandoLoad(); assert(sPlando.loaded && !sPlando.statusError); VerifyRows(sPlando.rows, true);
    PlandoSavePlay(); assert(reloadCalls == 2 && !sPlando.statusError);
    assert(Json::parse(Read(output)) == saved && Read(sibling) == misleading);
    // Now import explicitly under File3's native cursor; its status and preparation agree.
    nativeSlot = 2;
    PlandoSavePlay(); assert(reloadCalls == 3 && !sPlando.statusError);
    assert(sPlando.status == "Saved for File 3. Start that file to play it.");
    assert(ProfilePrepareSave(2) && appliedSeed == saved);
    const auto profileBefore = sProfiles->view().json();
    const auto outputBefore = Read(output);
    nativeSlot = -1;
    PlandoSavePlay(); assert(sPlando.statusError && reloadCalls == 3);
    assert(sProfiles->view().json() == profileBefore && Read(output) == outputBefore);
    sProfiles.reset();
}
int main(int argc, char** argv) {
    assert(argc == 2); const fs::path root = fs::absolute(argv[1]); assert(!fs::exists(root));
    sSohDump = OotDump; sMmDump = MmDump; sRequestReload = Reload; sPlandoFileSelectSlot = NativeSlot;
    std::vector<SeedKind> kinds{kLegacy, kOwnLayout};
    if (!CE::ActiveCapabilities().devBuild)
        kinds.push_back(kDevLayout);
    for (const auto kind : kinds)
        for (int mode = 0; mode < 3; ++mode)
            Case(root / (std::string(kind == kLegacy ? "legacy-" : kind == kOwnLayout ? "unified-" : "dev-") +
                         std::to_string(mode)), kind, mode);
    std::puts("Actual plando dropdown/slot/pending Load and repeated Save & Play preserve metadata/routes; sibling text ignored");
}
'''
# The pinned sample layout of the vanilla Wallmaster pool (tools/ootmm/m1-contract.json).
contract = json.loads((source/'tools/ootmm/m1-contract.json').read_text(encoding='utf-8'))
sample = next(c for c in contract['combinations'] if c['id'] == 'erWallmasters=ownGame;erOneWays=none')['sampleLayout']
assert len(sample) == 13
tests = tests.replace('@VANILLA_ROUTES@', ', '.join('{"%s", "%s"}' % (src, dst) for src, dst in sample))
for dev in (False, True):
    variant = 'dev' if dev else 'shipping'
    cpp = work/('entrance_plando_' + variant + '.cpp')
    cpp.write_text(support + profile_functions + state + '\n' + functions + '\n' + tests, encoding='utf-8')
    exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include],
                       defines=['_CRT_SECURE_NO_WARNINGS'] + (['COMBO_ENTRANCE_DEV=1'] if dev else []),
                       werror=True, msvc=['/bigobj'], gnu=['-Wno-missing-field-initializers'])
    subprocess.run([str(exe), str(work/(variant + '-' + uuid.uuid4().hex))], check=True, timeout=hostcxx.RUN_TIMEOUT)
print('Plando v2/legacy round trips, the shipping vanilla layout and the shipping dev-seed refusal passed.')
