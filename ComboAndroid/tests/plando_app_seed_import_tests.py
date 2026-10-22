"""Randomizer > Plandomizer: Save for File N imports seeds made in the app (0.4.13 RC2 emulator finding).

A seed made in the app also records the settings it was requested with (comboProfile, ComboShip.cpp RunComboFill):
the file's draft, a plain settings document of fileType ComboShipRandomizer version 1. Saving such a seed for a file
imports it into that file's settings profile, which merged that document over the seed's own settings, file type
included (ComboRandomizerProfiles.h SetupForSeed). A seed with an entrance layout then kept its layout under the plain
file type, the profile refused it ("an entrance layout needs fileType ComboShipRandomizerEntranceLayout version 2") and
the Plandomizer said "Reload rejected (be on the file-select screen; no generation running)." 0.4.12 did the same. The
headless corpus seeds carry no comboProfile, so plando_seed_roundtrip_tests.py and the 0.4.12 co-op check (a corpus
seed) never met it.

Runs the real PlandoLoad and PlandoSavePlay (combo/gui/ComboMenu.cpp), the real import wrapper Combo_OnReloadRequest
and ProfilePrepareSave (combo/ComboRandomizerProfileNative.inc) and the real profile Repository over every seed of the
0.4.13 release corpus (fixtures/wallmasters-0413: each Wallmaster capability id, Master Quest dungeons, recorded
settings choices, Allsanity, No Logic) and three 0.4.12 rows (fixtures/wallmasters-vanilla-v1, among them the co-op
check's seed), each given the version and comboProfile the app writes, and over a composite of the emulator's seed A
(Allsanity settings, a Wallmaster layout, Master Quest dungeons, a recorded starting age and key rings). Only the native
seed apply, the module exports and the file-select queries are stubbed. Each seed must save for File 2 byte for byte as
the app wrote it, become File 2's pending seed without touching File 3's, prepare File 2's save, survive a restart of
the profile store and leave File 2 a next-seed draft the settings menu reads. A layout seed requested from a copied
Wallmaster file's raw draft, copying a saved seed's settings, a file with no draft and the one-time pending-seed
migration use the same setup. A refused import says so in a plain sentence.
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
fixtures = source/'ComboAndroid/tests/fixtures'


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
    'int PlandoImportTarget()', 'void PlandoBuildItems()', 'void PlandoLoad()', 'void PlandoSavePlay()'))
profile_functions = extract('static int ProfileNativeSlot()', launcher) + '\n' + '\n'.join(
    extract(name, profiles) for name in ('static int ProfilePrepareSave(', 'static int Combo_OnReloadRequest('))
support = r'''
#include <algorithm>
#include <cassert>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "rando/ComboPlaythrough.h"
#include "rando/PlandoSeed.h"
#include "rando/ComboSeedFormat.h"
#include "rando/entrances/EntranceProfile.h"
#include "ComboRandomizerProfileRepository.h"
namespace fs = std::filesystem;
namespace CE = ComboRando::Entrances;
namespace Profiles = ComboRando::Profiles;
using Json = nlohmann::json;
void BindGameMode(const Json& seed) { assert(ComboRando::CombinedMode(ComboRando::ReadGameMode(seed))); }
using FnDump = const char* (*)(void);
using FnRequestReload = int (*)(const char*);
using FnPlandoFileSelectSlot = int (*)(void);
FnDump sSohDump = nullptr, sMmDump = nullptr;
FnRequestReload sRequestReload = nullptr;
FnPlandoFileSelectSlot sPlandoFileSelectSlot = nullptr;
void ResolvePlandoSyms() {}
namespace ComboTracker { int OotActiveSlot() { return -1; } }
const char* CVarGetString(const char*, const char*) { return ""; }
// The file-select queries: the highlighted file and SOH_CanGenerateComboSeed, which the menu (sIsOnFileSelect) and
// the launcher (sProfileCanGenerate) both resolve.
int nativeSlot = 1, profileSelected = 2;
bool onFileSelectMenu = true, nativeRefuses = false;
int NativeSlot() { return nativeSlot; }
int SelectedSlot() { return profileSelected; }
unsigned char CanGenerate() { return onFileSelectMenu ? 1 : 0; }
unsigned char (*sIsOnFileSelect)() = CanGenerate;
uint8_t (*sProfileCanGenerate)() = CanGenerate;
int (*sFileSelectSlot)() = NativeSlot;
int (*sProfileSelected)() = SelectedSlot;
std::string ootDump, mmDump;
const char* OotDump() { return ootDump.c_str(); }
const char* MmDump() { return mmDump.c_str(); }
std::string Read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary); assert(in.is_open());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
// As PlandoLoad and the app read a seed file: text mode, so the Windows host's line ends read as the device's.
std::string ReadText(const fs::path& path) {
    std::ifstream in(path); assert(in.is_open());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path()); std::ofstream out(path, std::ios::binary);
    assert(out.is_open()); out << text;
}
std::unique_ptr<Profiles::Repository> sProfiles;
std::atomic<bool> g_GenerateBusy{false}, g_ComboPendingFinalize{false};
std::atomic<uint64_t> g_ComboContextEpoch{0};
bool sProfileAttempt = false, sProfileLegacyPendingMigration = false;
int sProfilePreparedSlot = -1;
uint64_t sProfilePreparedEpoch = 0;
Json sProfilePreparedSeed, appliedSeed;
const char* (*SOH_GetComboSpoilerPath)() = nullptr;
int seedGeneratedClears = 0;
void SetSeedGenerated(uint8_t value) { assert(value == 0); ++seedGeneratedClears; }
void (*SOH_SetSeedGenerated)(uint8_t) = SetSeedGenerated;
fs::path ResolveComboSeedPath(const char* path) { return path; }
bool TryLoadComboSeedFile(const fs::path& path, Json& seed) {
    seed = Json::parse(Read(path));
    ComboRando::RequireSupportedSeedDocument(seed, CE::ActiveCapabilities());
    return true;
}
void FindNewestComboSeed(Json&) { assert(false && "unexpected legacy migration"); }
// The device's own reload of the seed (ComboShip.cpp Combo_OnReloadRequestImpl); its own refusals are its business.
int nativeApplies = 0;
int Combo_OnReloadRequestImpl(const char* path, const Json* supplied = nullptr) {
    const auto seed = supplied ? *supplied : Json::parse(Read(path));
    ComboRando::RequireSupportedSeedDocument(seed, CE::ActiveCapabilities());
    if (nativeRefuses) return 0;
    appliedSeed = seed; ++nativeApplies; ++g_ComboContextEpoch;
    return 1;
}
static int Combo_OnReloadRequest(const char* path);
int reloadCalls = 0;
std::string reloadedPath;
int Reload(const char* path) { ++reloadCalls; reloadedPath = path; return Combo_OnReloadRequest(path); }
'''
tests = r'''
const CE::CapabilitySet kCaps = CE::ActiveCapabilities();
// The legacy setup a new profile store captures (ComboRandomizerProfileNative.inc ProfileNativeSnapshot).
Json Legacy() {
    return {{"fileType", ComboRando::kComboSeedFileType}, {"version", 1}, {"seed", ""},
            {"oot", {{"settings", Json::object()}}}, {"mm", {{"settings", Json::object()}}},
            {"setupCVars", Json::object()}};
}
// File 3's own seed, which nothing here may touch.
Json OtherSeed() {
    return {{"fileType", ComboRando::kComboSeedFileType}, {"version", 1}, {"seed", "file-3"},
            {"file_hash", {9, 9, 9, 9, 9}}, {"sharedItems", Json::array()}, {"goal", {{"type", "ganonMajora"}}},
            {"startingGame", "OOT"},
            {"oot", {{"settings", {{"keep", 3}}}, {"placements", {{"OOT Check", "Kokiri Sword"}}}}},
            {"mm", {{"settings", {{"keep", 4}}}, {"placements", {{"MM Check", "Deku Mask"}}}}},
            {"foreign", Json::array()}};
}
void Flatten(const std::string& prefix, const Json& value, Json& out) {
    if (!value.is_object()) { out[prefix] = value; return; }
    for (const auto& field : value.items()) Flatten(prefix + "." + field.key(), field.value(), out);
}
// The settings a generation records (comboProfile.settings): the file's draft as the settings menu stores it
// (ProfileNativeSnapshot's fields, the entrance rows in setupCVars and never an entranceLayout, as
// NormalizeEntranceDraft leaves it), so a plain settings document of fileType ComboShipRandomizer version 1. The setup
// CVars are the corpus input's gCombo.Rando values and the entrance rows of the seed's layout.
Json RequestedSettings(const Json& seed, const Json& input, const Json& ootFrom, const Json& mmFrom) {
    Json combo = Json::object();
    const auto cvars = input.value("CVars", Json::object()).value("gCombo", Json::object());
    Flatten("gCombo.Rando", cvars.value("Rando", Json::object()), combo);
    CE::Settings entrances;
    if (seed.contains("entranceLayout"))
        entrances = CE::DecodeEntranceLayout(seed.at("entranceLayout")).settings;
    for (const auto& row : CE::kEntranceSettings) combo[row.cvar] = row.get(entrances);
    combo["gCombo.Rando.StartingGame"] = seed.at("startingGame") == "MM" ? 1 : 0;
    Json settings = {{"fileType", ComboRando::kComboSeedFileType}, {"version", 1},
                     {"seed", seed.value("seed", std::string())},
                     {"oot", {{"settings", ootFrom.at("oot").at("settings")}}},
                     {"mm", {{"settings", mmFrom.at("mm").at("settings")}}}, {"setupCVars", combo}};
    for (const char* key : {"gameplayRules", "songTravel", "goal", "startingGame", "sharedItems"})
        if (seed.contains(key)) settings[key] = seed.at(key);
    return settings;
}
// RunComboFill's additions to what the headless generator writes (ComboShip.cpp): version 1 on a seed without a layout
// (kept by StampComboSeedFormat; a layout seed has version 2), displaySeed, the five file_hash icons, the playthrough
// and comboProfile.
Json AppSeed(Json seed, const Json& requested) {
    if (!seed.contains("version")) seed["version"] = 1;
    const uint32_t identity = seed.value("displaySeed", seed.at("masterSeed").get<uint32_t>() * 2654435761u);
    seed["displaySeed"] = identity;
    if (!seed.contains("file_hash")) {
        Json icons = Json::array();
        for (int i = 0; i < 5; ++i) icons.push_back((identity >> (i * 6)) % 100);
        seed["file_hash"] = icons;
    }
    if (!seed.contains("playthrough")) seed["playthrough"] = Json::array();
    seed["comboProfile"] = {{"version", 1}, {"file", 2}, {"revision", 3}, {"settings", requested}};
    ComboRando::RequireSupportedSeedDocument(seed, kCaps);
    return seed;
}
// Every leaf of `subset` has the same value in `document`, except the top-level format fields.
void RequireLeaves(const Json& subset, const Json& document, bool top = true) {
    for (const auto& field : subset.items()) {
        if (top && (field.key() == "fileType" || field.key() == "version")) continue;
        assert(document.contains(field.key()));
        if (field.value().is_object() && document.at(field.key()).is_object())
            RequireLeaves(field.value(), document.at(field.key()), false);
        else
            assert(document.at(field.key()) == field.value());
    }
}
// A next-seed draft made from an app seed: a settings document setDraft and a restart accept, of the file type its
// content needs (a routeless layout keeps the layout type), with every requested setting, and one the settings menu
// shows (ProfileInspect runs NormalizeEntranceDraft) with the seed's entrance rows.
void CheckDraft(const Json& draft, const Json& seed) {
    const auto& requested = seed.at("comboProfile").at("settings");
    ComboRando::RequireSupportedSettingsCapabilities(draft, kCaps);
    assert(Profiles::SettingsDocument(draft, kCaps) == draft);
    if (draft.contains("entranceLayout")) {
        assert(draft.at("fileType") == ComboRando::kEntranceSeedFileType &&
               draft.at("version") == ComboRando::kEntranceSeedVersion);
        if (seed.contains("entranceLayout")) {
            auto routeless = seed.at("entranceLayout");
            routeless.erase("overrides");
            assert(draft.at("entranceLayout") == routeless);
        }
    } else
        assert(draft.at("fileType") == requested.at("fileType") && draft.at("version") == requested.at("version"));
    RequireLeaves(requested, draft);
    auto shown = draft;
    CE::NormalizeEntranceDraft(shown, kCaps);
    assert(!shown.contains("entranceLayout") && shown.at("fileType") == ComboRando::kComboSeedFileType);
    CE::Settings entrances;
    if (seed.contains("entranceLayout"))
        entrances = CE::DecodeEntranceLayout(seed.at("entranceLayout")).settings;
    for (const auto& row : CE::kEntranceSettings) assert(shown.at("setupCVars").at(row.cvar) == row.get(entrances));
}
// Item tables like the games' dumps: every item name the seed uses, in both tables (plando_seed_roundtrip_tests.py).
void DumpsFor(const Json& seed) {
    std::set<std::string> names;
    auto bare = [](std::string s) { return ComboRando::StripGameSuffix(std::move(s)); };
    for (const char* game : {"oot", "mm"})
        for (const auto& item : seed.at(game).at("placements")) names.insert(bare(item.get<std::string>()));
    for (const auto& fm : seed.at("foreign")) names.insert(bare(fm.at("itemName").get<std::string>()));
    Json items = Json::array();
    for (const auto& n : names) items.push_back({{"name", n}, {"advancement", true}});
    ootDump = Json{{"items", items}, {"pool", items}}.dump();
    mmDump = ootDump;
}
// A fresh profile store for the case, File 3 holding its own seed.
void FreshProfiles() {
    sProfiles = std::make_unique<Profiles::Repository>("Randomizer/settings-profiles.json", Legacy(), kCaps);
    sProfiles->importSeed(2, OtherSeed(), true);
    sProfilePreparedSlot = -1; sProfilePreparedSeed = nullptr; appliedSeed = nullptr;
}
// Randomizer > Plandomizer: pick the seed's file in the dropdown, Load, then Save for the highlighted File 2.
void LoadAndSave(const fs::path& path) {
    sPlando = {};
    sPlando.spoilerPaths = {path.string()};
    sPlando.spoilerNames = {path.stem().string()};
    sPlando.spoilerSel = 0;
    PlandoLoad();
    assert(sPlando.loaded && !sPlando.statusError);
    nativeSlot = 1;
    const int before = reloadCalls;
    PlandoSavePlay();
    assert(reloadCalls == before + 1);
}

// The whole path for one app seed.
void SeedCase(const fs::path& dir, const std::string& id, const Json& seed) {
    fs::create_directories(dir);
    fs::current_path(dir);
    const std::string text = seed.dump(2); // g_ConsolidatedJson, written as is (WriteComboSpoiler)
    const fs::path input = fs::path("Randomizer") / (id + ".json");
    Write(input, text);
    FreshProfiles();
    const auto other = OtherSeed();
    const auto otherDraft = sProfiles->view().draft(2);
    DumpsFor(seed);
    LoadAndSave(input);
    if (sPlando.statusError || sPlando.status != "Saved for File 2. Start that file to play it.") {
        std::cerr << id << ": " << sPlando.status << "\n";
        std::exit(1);
    }
    assert(sPlando.rows.size() == seed["oot"]["placements"].size() + seed["mm"]["placements"].size());
    // The saved file is the seed exactly as the app wrote it, beside the untouched original.
    const auto output = ComboRando::ComboSpoilerPath(seed.at("file_hash"), "Combo-Plando");
    assert(fs::path(reloadedPath) == output && ReadText(output) == text && Json::parse(Read(output)) == seed);
    assert(Read(input) == text && sPlando.loadedJson == text);
    // File 2 has the seed and its draft; File 3 keeps its own.
    const auto& files = sProfiles->view().json().at("files");
    assert(sProfiles->view().pendingSeed(1) == seed && sProfiles->view().pendingSeed(2) == other);
    assert(files[1].at("lastAttempt").at("origin") == "import" && files[1].at("lastAttempt").at("success") == true);
    assert(files[1].at("generatedInput") == files[1].at("draft"));
    CheckDraft(files[1].at("draft"), seed);
    assert(sProfiles->view().draft(2) == otherDraft && files[0].at("generated").is_null());
    // Start Randomizer on File 2 prepares this seed, on File 3 its own.
    assert(ProfilePrepareSave(1) && appliedSeed == seed);
    assert(ProfilePrepareSave(2) && appliedSeed == other);
    // A restart reads the store back; restore checks every draft and generation input.
    const auto stored = sProfiles->view().json();
    sProfiles = std::make_unique<Profiles::Repository>("Randomizer/settings-profiles.json", Legacy(), kCaps);
    assert(sProfiles->view().json() == stored);
    // Saved seed view > Copy settings > To File 1 next seed reads the same setup.
    sProfiles->copySaved(1, 0, seed);
    assert(sProfiles->view().draft(0) == stored.at("files")[1].at("draft"));
    // A file with no draft of its own shows its saved seed's setup.
    Profiles::Store bare(Legacy(), kCaps);
    CheckDraft(bare.draft(1, seed), seed);
    // The one-time pending-seed migration imports the newest seed into every empty file without a draft; the store
    // it writes must load on the next start.
    {
        Profiles::Repository migrating("Randomizer/migrated.json", Legacy(), kCaps);
        migrating.migrateLegacyPending(seed);
    }
    Profiles::Repository migrated("Randomizer/migrated.json", Legacy(), kCaps);
    for (int slot = 0; slot < 3; ++slot) {
        assert(migrated.view().pendingSeed(slot) == seed);
        CheckDraft(migrated.view().json().at("files")[slot].at("generatedInput"), seed);
    }
    sProfiles.reset();
}
// A layout seed whose generation ran from a raw draft copied from a Wallmaster file (copySaved without an edit): its
// requested settings already carry the routeless layout and the layout file type.
void CopiedDraftCase(const Json& headless) {
    auto requested = Profiles::SetupForSeed(headless, kCaps);
    requested["seed"] = "copied";
    const auto seed = AppSeed(headless, requested);
    Profiles::Store store(Legacy(), kCaps);
    store.importSeed(1, seed, true);
    assert(store.pendingSeed(1) == seed);
    CheckDraft(store.json().at("files")[1].at("draft"), seed);
    Profiles::Store reread(Legacy(), kCaps);
    reread.restore(store.json());
}

// The sentences Save shows when the seed does not become the file's (not "Reload rejected ...").
void RefusalCases(const fs::path& dir, Json seed) {
    fs::create_directories(dir);
    fs::current_path(dir);
    const std::string refused = "This seed could not be saved for File 2, so File 2 is unchanged. native.log says why.";
    // The device takes the seed, but File 2's profile refuses its requested settings (an unknown record version).
    seed["comboProfile"]["version"] = 2;
    const fs::path input = "Randomizer/refused.json";
    Write(input, seed.dump(2));
    FreshProfiles();
    DumpsFor(seed);
    const auto before = sProfiles->view().json();
    const int clears = seedGeneratedClears;
    LoadAndSave(input);
    assert(sPlando.statusError && sPlando.status == refused);
    assert(sProfiles->view().json() == before && seedGeneratedClears == clears + 1);
    // The device itself refuses the seed (Combo_OnReloadRequestImpl; native.log says why).
    nativeRefuses = true;
    PlandoSavePlay();
    assert(sPlando.statusError && sPlando.status == refused && sProfiles->view().json() == before);
    nativeRefuses = false;
    // Off the main file-select menu (name entry, copy, erase) the launcher refuses before reading the file.
    onFileSelectMenu = false;
    PlandoSavePlay();
    assert(sPlando.statusError && sPlando.status == "Go back to the file-select menu, then save again.");
    assert(sProfiles->view().json() == before);
    onFileSelectMenu = true;
    sProfiles.reset();
    std::puts("A refused import or seed says so plainly; off the file-select menu Save says to go back to it");
}

Json Load(const fs::path& path) { return Json::parse(Read(path)); }

int Run(int argc, char** argv) {
    assert(argc == 3);
    const fs::path root = fs::absolute(argv[1]), fixtures = fs::absolute(argv[2]);
    assert(!fs::exists(root));
    sSohDump = OotDump; sMmDump = MmDump; sRequestReload = Reload; sPlandoFileSelectSlot = NativeSlot;
    std::vector<std::pair<std::string, Json>> seeds; // id, app seed
    std::vector<Json> layoutSeeds;
    std::set<std::string> capabilities;
    int masterQuest = 0, choices = 0, allsanity = 0;
    const auto corpus = fixtures / "wallmasters-0413";
    const auto manifest = Load(corpus / "manifest.json");
    for (const auto& row : manifest.at("seeds")) {
        const auto headless = Load(corpus / row.at("seed").get<std::string>());
        assert(!headless.contains("comboProfile"));
        const auto input = Load(corpus / row.at("config").get<std::string>());
        const auto& facts = row.at("facts");
        if (facts.at("capability").is_string()) capabilities.insert(facts.at("capability").get<std::string>());
        masterQuest += facts.at("masterQuestDungeons").empty() ? 0 : 1;
        choices += headless.at("oot").contains("resolvedChoices") ? 1 : 0;
        allsanity += row.at("id").get<std::string>().find("allsanity") != std::string::npos ? 1 : 0;
        if (headless.contains("entranceLayout")) layoutSeeds.push_back(headless);
        seeds.emplace_back(row.at("id").get<std::string>(),
                           AppSeed(headless, RequestedSettings(headless, input, headless, headless)));
    }
    // Every Wallmaster capability id the release makes, Master Quest seeds, recorded choices and Allsanity.
    assert(capabilities == std::set<std::string>({"wallmasters-own-game-vanilla-v1", "wallmasters-own-game-quests-v1",
                                                  "wallmasters-own-game-songs-v1", "wallmasters-own-game-songs-v2"}));
    assert(masterQuest > 0 && choices >= 2 && allsanity == 1 && layoutSeeds.size() >= 30);
    // Three 0.4.12 rows, among them the co-op check's seed S (oot-child-everyCheck-souls0-00).
    const auto older = fixtures / "wallmasters-vanilla-v1";
    const auto olderManifest = Load(older / "manifest.json");
    for (const auto& row : olderManifest.at("seeds")) {
        const auto id = row.at("id").get<std::string>();
        if (id != "oot-child-everyCheck-souls0-00" && id != "mm-everyCheck-souls0-00" &&
            id != "oot-adult-noLogic-souls0")
            continue;
        const auto headless = Load(older / row.at("seed").get<std::string>());
        layoutSeeds.push_back(headless);
        const auto input = Load(older / row.at("config").get<std::string>());
        seeds.emplace_back("0412-" + id, AppSeed(headless, RequestedSettings(headless, input, headless, headless)));
    }
    assert(seeds.size() == manifest.at("seeds").size() + 3);
    // The emulator's seed A in one seed: Allsanity's requested settings (d06's), a Wallmaster layout with Master Quest
    // dungeons (c04's) and a recorded starting age and key rings (d06's record; the import never reads it).
    {
        const auto allsanity06 = Load(corpus / "seeds/d06-allsanity-oot.json");
        auto composite = Load(corpus / "seeds/c04-mq-randomnumber-oot-child.json");
        assert(composite.contains("entranceLayout") && !composite["oot"].at("masterQuestDungeons").empty());
        const auto& record = allsanity06.at("oot").at("resolvedChoices");
        assert(record.contains("keyRings") && record.contains("startingAge"));
        composite["oot"]["resolvedChoices"] = record;
        const auto input = Load(corpus / "inputs/comboship-d06-allsanity-oot.json");
        seeds.emplace_back("seed-a-shape",
                           AppSeed(composite, RequestedSettings(composite, input, allsanity06, allsanity06)));
    }
    // The shapes named in the finding: Wallmaster (each id), Master Quest, recorded choices, Allsanity and seed A.
    for (const char* id : {"e-oot-child-everyCheck-souls0-00", "0412-oot-child-everyCheck-souls0-00",
                           "c01-mq12-oot-child", "a01-songs-oot-child", "b01-beginner-oot-child-1",
                           "a07-songs-oot-randomage-ageless", "d06-allsanity-oot", "seed-a-shape"})
        assert(std::count_if(seeds.begin(), seeds.end(), [&](const auto& seed) { return seed.first == id; }) == 1);
    fs::create_directories(root);
    // Short case directories: Windows paths stay under MAX_PATH below a deep ctest build directory.
    for (size_t i = 0; i < seeds.size(); ++i)
        SeedCase(root / ("s" + std::to_string(i)), seeds[i].first, seeds[i].second);
    for (const auto& headless : layoutSeeds) CopiedDraftCase(headless);
    RefusalCases(root / "r", seeds.front().second);
    std::printf("Plandomizer Save imported %zu app-made seeds into File 2 exactly as made (%zu with a layout, also "
                "from a copied draft)\n", seeds.size(), layoutSeeds.size());
    return 0;
}
int main(int argc, char** argv) {
    try {
        return Run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "unexpected exception: " << error.what() << "\n";
        return 1;
    }
}
'''
for name in ('wallmasters-0413', 'wallmasters-vanilla-v1'):
    assert (fixtures/name/'manifest.json').exists(), f'the committed corpus {name} is missing'
cpp = work/'plando_app_seed_import.cpp'
cpp.write_text(support + profile_functions + state + '\n' + functions + '\n' + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, includes=[source/'combo', args.json_include],
                    defines=['_CRT_SECURE_NO_WARNINGS'], werror=True, msvc=['/bigobj'],
                    gnu=['-Wno-missing-field-initializers'])
subprocess.run([str(exe), str(work/('run-' + uuid.uuid4().hex)), str(fixtures)], check=True,
               timeout=hostcxx.RUN_TIMEOUT)
print('Plandomizer Save for a file imports every app-made seed shape exactly as made, and a refusal says so plainly.')
