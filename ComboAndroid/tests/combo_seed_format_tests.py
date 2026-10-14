"""Exercise seed compatibility and production import/save preflight with engine stubs.

The build's capability set is the shipping one, which in Phase 0 admits nothing, so every
entrance layout is refused before decoding exactly as in 0.4.4. A test-only set (the whole
catalog) exercises the acceptance path the M1-03 validator adds behind it.
"""
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
args.build_dir.mkdir(parents=True, exist_ok=True)
launcher = (args.source / 'combo/ComboShip.cpp').read_text(encoding='utf-8')
native = (args.source / 'soh/soh/SaveManager.cpp').read_text(encoding='utf-8')
drop = (args.source / 'soh/soh/Enhancements/randomizer/randomizer.cpp').read_text(encoding='utf-8')


def function(text, signature):
    start = hostcxx.find(text, signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


source = r'''
#define COMBO_BUILD 1
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
#define CVAR_GENERAL(name) name
#include <cassert>
#include <atomic>
#include <iostream>
#include <mutex>
#include <map>
#include <memory>
#include <functional>
#include "ComboRandomizerSavedSeed.h"
#include "rando/entrances/NativeBridge.h"
using Json = nlohmann::json;
using namespace ComboRando;
std::mutex g_containerMutex;
std::map<int, Json> g_containerCache;
int containerLoads = 0, mutations = 0, drains = 0, popups = 0;
std::string popupTitle, dropped;
std::filesystem::path ComboContainerPath(int slot) { return "slot" + std::to_string(slot) + ".combosav"; }
bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
Json& LoadOrCreateContainer(int slot) {
    ++containerLoads;
    if (!g_containerCache.contains(slot))
        g_containerCache[slot] = Profiles::ReadSavedContainerFile(ComboContainerPath(slot));
    return g_containerCache.at(slot);
}
namespace SohGui {
void RegisterPopup(const std::string& title, const std::string&) { ++popups; popupTitle = title; }
}
namespace SohUtils { bool IsStringEmpty(const char* value) { return !value || !value[0]; } }
void CVarSetString(const char* key, const char*) { dropped = key; }
void CVarSetInteger(const char*, int) { ++mutations; }
static Entrances::NativeFns sEntranceNative;
'''
source += function(launcher, 'static const char* Combo_ReadGameSave(int game, int fileNum) {') + '\n'
source += function(launcher, 'static bool TryLoadComboSeedFile(const std::filesystem::path& p, nlohmann::json& out) {') + '\n'
source += function(drop, 'bool Rando_HandleSpoilerDrop(char* filePath) {') + '\n'
source += r'''
auto gComboReadGameSave = Combo_ReadGameSave;
struct SaveContext {
    int fileNum = 2;
    struct { struct { int id = 0; } quest; } ship;
} gSaveContext;
void SaveContext_Init() { ++mutations; }
namespace Rando {
struct Logic {
    void SetContext(void*) { ++mutations; }
    void SetSaveContext(SaveContext*) { ++mutations; }
};
struct Context {
    Logic logic;
    Logic* GetLogic() { return &logic; }
    void AddExcludedOptions() { ++mutations; }
    static std::shared_ptr<Context> CreateInstance() { ++mutations; return std::make_shared<Context>(); }
};
struct Settings {
    static Settings* GetInstance() { static Settings result; return &result; }
    void ClearContext() { ++mutations; }
    void AssignContext(const std::shared_ptr<Context>&) { ++mutations; }
};
}
struct OTRGlobals {
    std::shared_ptr<Rando::Context> gRandoContext = std::make_shared<Rando::Context>();
    static OTRGlobals* Instance;
};
OTRGlobals globals;
OTRGlobals* OTRGlobals::Instance = &globals;
struct GameInteractor {
    struct OnLoadFile {};
    static GameInteractor* Instance;
    template<class Event> void ExecuteHooks(int) { ++mutations; }
};
GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
namespace Ship { struct Context { static std::string GetPathRelativeToAppDirectory(const char*) { return "."; } }; }
int GetUnixTimestamp() { return 0; }
void Randomizer_RepairLegacyMaskInventory() { ++mutations; }
constexpr int QUEST_RANDOMIZER = 1;
constexpr const char* FILE_TYPE_SAVE_RANDO = "RandoSave";
enum class ComboLoadState { Blocked, Loading, Ready };
class SaveManager {
  public:
    static constexpr int MaxFiles = 3;
    std::atomic<ComboLoadState> comboLoadState{ComboLoadState::Ready};
    std::mutex saveMtx;
    Json saveBlock = {{"untouched", true}};
    Json* currentJsonContext = nullptr;
    using SectionLoadHandler = std::map<int, std::function<void()>>;
    std::map<std::string, SectionLoadHandler> sectionLoadHandlers;
    void ThreadPoolWait() { ++drains; }
    std::filesystem::path GetFileName(int slot) { return "native" + std::to_string(slot) + ".sav"; }
    void InitFile(bool) { ++mutations; }
    void InitMeta(int) { ++mutations; }
    bool LoadFile(int fileNum, bool initializeFullContext);
};
'''
source += function(native, 'bool SaveManager::LoadFile(int fileNum, bool initializeFullContext) {') + '\n'
source += r'''
void write(const std::filesystem::path& path, const Json& value) {
    std::ofstream output(path); output << value.dump(2);
}
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path); return {std::istreambuf_iterator<char>(input), {}};
}
template<class F> void rejects(F action) {
    bool rejected = false;
    try { action(); } catch (const UnsupportedComboSeedFormat&) { rejected = true; }
    assert(rejected);
}
// The 0.4.4 refusal, raised before the layout is decoded.
const std::string kBeforeDecode = UnsupportedComboSeedFormat().what();
template<class F> void rejectsBeforeDecode(F action) {
    std::string message;
    try { action(); } catch (const UnsupportedComboSeedFormat& error) { message = error.what(); }
    assert(message == kBeforeDecode);
}
// A layout the build's capabilities reach, refused by the validator with its reason.
template<class F> void rejectsLayout(F action) {
    std::string message;
    try { action(); } catch (const UnsupportedComboSeedFormat& error) { message = error.what(); }
    assert(!message.empty() && message != kBeforeDecode);
}
Json legacy(bool version) {
    Json seed = {{"fileType", kComboSeedFileType}, {"oot", {{"settings", Json::object()}}},
                 {"mm", {{"settings", Json::object()}}}, {"gearRules", {{"boots", true}}},
                 {"faroreRules", {{"crossAge", true}}}};
    if (version) seed["version"] = 1;
    return seed;
}
int main() {
    const auto shipping = Entrances::ActiveCapabilities();
    assert(shipping.Empty() && !shipping.devBuild); // Phase 0: no value is open in any build
    for (bool version : {false, true}) {
        auto seed = legacy(version);
        const auto bytes = seed.dump(2);
        StampComboSeedFormat(seed);
        assert(seed.dump(2) == bytes); // Both existing writer contracts remain byte-identical.
        RequireSupportedSeedDocument(seed, shipping);
        assert(IsComboSeedDocument(seed));
        write("seed.json", seed);
        Json imported;
        assert(TryLoadComboSeedFile("seed.json", imported) && imported == seed);
        Profiles::Store profiles(seed, shipping);
        profiles.importSeed(0, seed, true);
        assert(profiles.pendingSeed(0) == seed);
    }
    RequireSupportedSeedCapabilities(Json{{"settings", {{"native-option", 1}}}}, shipping);
    assert(!IsComboSeedDocument(Json::array()) && !IsComboSeedDocument(Json{{"fileType", 1}}));
    auto future = legacy(true);
    future["entranceLayout"] = {{"schemaVersion", 1}, {"overrides", Json::array()}};
    StampComboSeedFormat(future);
    assert(future.at("fileType") == kEntranceSeedFileType && future.at("version") == kEntranceSeedVersion);
    // Old exact-type readers fail closed even though they ignore numeric versions.
    assert(future.at("fileType") != kComboSeedFileType && IsComboSeedDocument(future));
    for (const Json& layout : {Json(), Json::object(), Json::array(), Json(1)}) {
        auto bad = legacy(false);
        bad["entranceLayout"] = layout;
        rejectsBeforeDecode([&] { RequireSupportedSeedDocument(bad, shipping); });
        rejectsBeforeDecode([&] { Profiles::SettingsDocument(bad, shipping); });
        rejectsBeforeDecode([&] { Entrances::ApplyNativeSeed(sEntranceNative, bad, shipping); });
    }
    for (const Json& version : {Json(), Json(1), Json(2), Json(99), Json("2")}) {
        auto bad = future;
        bad["version"] = version;
        rejectsBeforeDecode([&] { RequireSupportedSeedDocument(bad, shipping); });
    }
    auto absent = future;
    absent.erase("entranceLayout");
    rejectsBeforeDecode([&] { RequireSupportedSeedDocument(absent, shipping); });
    // A complete, valid layout (the dev-only MM tranche) is refused just the same.
    Entrances::EntranceLayout tranche;
    tranche.required = {"wallmasters-own-game-mm-tranche"};
    tranche.settings.erWallmasters = Entrances::Mode::OwnGame;
    tranche.overrides = Entrances::Overrides{{"MM_WALLMASTER_BTW_ENTRANCE", "MM_TEMPLE_WOODFALL"},
                                             {"MM_WALLMASTER_BTW_EXIT", "MM_TEMPLE_SNOWHEAD"},
                                             {"MM_WALLMASTER_BTW_FOUNTAIN", "MM_TEMPLE_GREAT_BAY"},
                                             {"MM_WALLMASTER_DAMPE", "MM_SPIDER_HOUSE_SWAMP"}};
    auto layoutSeed = legacy(true);
    layoutSeed["entranceLayout"] = Entrances::EncodeEntranceLayout(tranche);
    StampComboSeedFormat(layoutSeed);
    rejectsBeforeDecode([&] { RequireSupportedSeedDocument(layoutSeed, shipping); });
    rejectsBeforeDecode([&] { RequireSupportedSettingsCapabilities(layoutSeed, shipping); });
    rejectsBeforeDecode([&] { Profiles::SettingsDocument(layoutSeed, shipping); });
    rejectsBeforeDecode([&] { Profiles::SetupForSeed(layoutSeed, shipping); });
    rejectsBeforeDecode([&] { Entrances::ApplyNativeSeed(sEntranceNative, layoutSeed, shipping); });
    rejectsBeforeDecode([&] { Profiles::SeedFromContainer({{"combo", {{"rando", layoutSeed}}}}, shipping); });

    // The acceptance path, with every catalog value open (tests only; no build has this set).
    auto dev = Entrances::CapabilitySet::AtLeast(Entrances::Stage::Hidden);
    dev.devBuild = true;
    RequireSupportedSeedDocument(layoutSeed, dev);
    RequireSupportedSettingsCapabilities(layoutSeed, dev);
    // Settings documents drop the routes and stay valid settings documents, never seeds.
    const auto settingsDocument = Profiles::SettingsDocument(layoutSeed, dev);
    auto routeless = layoutSeed["entranceLayout"];
    routeless.erase("overrides");
    assert(settingsDocument.at("entranceLayout") == routeless);
    assert(Profiles::SettingsDocument(settingsDocument, dev) == settingsDocument);
    rejectsLayout([&] { RequireSupportedSeedDocument(settingsDocument, dev); });
    rejectsLayout([&] { Profiles::SetupForSeed(settingsDocument, dev); });
    Profiles::Store devStore(legacy(false), dev);
    devStore.importSeed(0, layoutSeed, true);
    assert(devStore.pendingSeed(0) == layoutSeed);
    assert(devStore.draft(0).at("entranceLayout") == routeless);
    rejectsLayout([&] { devStore.importSeed(1, settingsDocument, true); });
    Profiles::Store reread(legacy(false), dev);
    reread.restore(devStore.json());
    assert(reread.json() == devStore.json());
    rejectsBeforeDecode([&] { Profiles::Store(legacy(false), shipping).restore(devStore.json()); });
    assert(Profiles::SeedFromContainer({{"combo", {{"rando", layoutSeed.dump()}}}}, dev) == layoutSeed);
    // Without the MM installer's exports an accepted layout still never binds as vanilla routing.
    std::string installer;
    try { Entrances::ApplyNativeSeed(sEntranceNative, layoutSeed, dev); }
    catch (const std::runtime_error& error) { installer = error.what(); }
    assert(installer == "MM entrance layout exports unavailable");
    // Seed-level refusals behind the shipping one.
    auto oldType = layoutSeed;
    oldType["fileType"] = kComboSeedFileType;
    rejectsLayout([&] { RequireSupportedSeedDocument(oldType, dev); });
    auto noLayout = layoutSeed;
    noLayout.erase("entranceLayout");
    rejectsLayout([&] { RequireSupportedSeedDocument(noLayout, dev); });
    for (const Json& version : {Json(), Json(1), Json(99), Json("2"), Json(2.5)}) {
        auto bad = layoutSeed;
        bad["version"] = version;
        rejectsLayout([&] { RequireSupportedSeedDocument(bad, dev); });
    }
    auto unversioned = layoutSeed;
    unversioned.erase("version");
    rejectsLayout([&] { RequireSupportedSeedDocument(unversioned, dev); });
    for (const char* key : kLegacyEntranceKeys) {
        auto both = layoutSeed;
        both[key] = Json::object();
        rejectsLayout([&] { RequireSupportedSeedDocument(both, dev); });
        rejectsLayout([&] { Profiles::SettingsDocument(both, dev); });
    }
    for (const Json& layout : {Json(), Json::object(), Json::array(), Json(1)}) {
        auto bad = layoutSeed;
        bad["entranceLayout"] = layout;
        rejectsLayout([&] { RequireSupportedSeedDocument(bad, dev); });
        rejectsLayout([&] { RequireSupportedSettingsCapabilities(bad, dev); });
    }
    auto devLayout = tranche;
    devLayout.devBuild = true;
    auto devSeed = layoutSeed;
    devSeed["entranceLayout"] = Entrances::EncodeEntranceLayout(devLayout);
    RequireSupportedSeedDocument(devSeed, dev);
    auto releaseSet = dev;
    releaseSet.devBuild = false;
    rejectsLayout([&] { RequireSupportedSeedDocument(devSeed, releaseSet); });

    write("future.json", future);
    Json output = {{"untouched", true}};
    assert(!TryLoadComboSeedFile("future.json", output) && output.at("untouched") == true);
    write("layout.json", layoutSeed);
    assert(!TryLoadComboSeedFile("layout.json", output) && output.at("untouched") == true);
    char futurePath[] = "future.json";
    assert(Rando_HandleSpoilerDrop(futurePath) && dropped == "ComboDroppedFile" && mutations == 0);
    Profiles::Store profiles(legacy(false), shipping);
    const auto before = profiles.json();
    rejects([&] { profiles.importSeed(0, future, true); });
    assert(profiles.json() == before);
    auto savedProfiles = before;
    savedProfiles["files"][0]["generated"] = future;
    rejects([&] { profiles.restore(savedProfiles); });
    assert(profiles.json() == before);

    const Json nativePage = {{"version", 1}, {"sections", Json::object()}};
    // Run real launcher callback + real native LoadFile. Unsupported seeds, including a legacy
    // type carrying the new field, cannot change native context, fileNum, cache or on-disk bytes.
    auto mislabeled = future;
    mislabeled["fileType"] = kComboSeedFileType;
    for (const Json& seed : {future, mislabeled, layoutSeed}) for (bool cached : {false, true}) {
        const Json container = {{"oot", nativePage}, {"combo", {{"rando", seed}}}};
        write(ComboContainerPath(0), container);
        g_containerCache.clear();
        if (cached) g_containerCache[0] = container;
        const auto cacheBefore = g_containerCache;
        const auto diskBefore = read(ComboContainerPath(0));
        write("native0.sav", nativePage); // A compatibility failure must never rename this either.
        SaveManager manager;
        auto* context = globals.gRandoContext.get();
        gSaveContext.fileNum = 2;
        containerLoads = mutations = popups = 0;
        assert(!manager.LoadFile(0, true));
        assert(manager.comboLoadState == ComboLoadState::Blocked);
        assert(mutations == 0 && containerLoads == 0 && gSaveContext.fileNum == 2);
        assert(globals.gRandoContext.get() == context && manager.saveBlock.at("untouched") == true);
        assert(popups == 1 && popupTitle == "Unsupported seed format");
        assert(g_containerCache == cacheBefore && read(ComboContainerPath(0)) == diskBefore);
        assert(std::filesystem::exists("native0.sav"));
    }
    // Serialized legacy rando pages use the same capability gate.
    Json stringContainer = {{"oot", nativePage}, {"combo", {{"rando", future.dump()}}}};
    rejects([&] { Profiles::SeedFromContainer(stringContainer, shipping); });
    // Supported no-layout seed still binds normally, proving the stubbed native path is usable.
    g_containerCache.clear();
    write(ComboContainerPath(1), {{"oot", nativePage}, {"combo", {{"rando", legacy(false)}}}});
    SaveManager manager;
    mutations = containerLoads = popups = 0;
    assert(manager.LoadFile(1, true));
    assert(gSaveContext.fileNum == 1 && mutations > 0 && containerLoads == 1 && popups == 0);
    assert(manager.comboLoadState == ComboLoadState::Ready);
    std::cout << "PASS legacy bytes/imports, future discriminator, capability rejection and production native "
                 "save preflight before context/slot mutation; the layout acceptance path behind them\n";
}
'''
cpp = args.build_dir / 'seed_format.cpp'
cpp.write_text(source, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, warnings=False, includes=[args.source / 'combo', args.json_include])
subprocess.run([str(exe)], cwd=args.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)

# Full headless/main/UI dependencies are unavailable to this small harness. Verify their actual
# boundary calls and writer ordering in addition to executing the shared policies above.
headless = (args.source / 'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
caps = 'ComboRando::Entrances::ActiveCapabilities()'
assert hostcxx.contains(headless, 'spoiler = nlohmann::json::parse(ReadFile(playthroughFile)); '
                                  f'ComboRando::RequireSupportedSeedDocument(spoiler, {caps});')
assert hostcxx.find(headless, f'ComboRando::RequireSupportedSettingsCapabilities(sj, {caps});') < \
    hostcxx.find(headless, 'const auto& oot = sj.contains("oot") ? sj["oot"] : sj;')
assert hostcxx.contains(headless, 'ComboRando::StampComboSeedFormat(consolidated); f << consolidated.dump(2);')
assert hostcxx.contains(launcher, 'ComboRando::StampComboSeedFormat(consolidated); g_ConsolidatedJson = consolidated.dump(2);')
assert hostcxx.contains(launcher, f'j = *profileSeed; ComboRando::RequireSupportedSeedDocument(j, {caps});')
assert hostcxx.contains(launcher, f'ComboRando::Profiles::Repository>(profilePath, ProfileNativeSnapshot(), {caps});')
menu = (args.source / 'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')
assert hostcxx.find(menu, f'ComboRando::RequireSupportedSeedDocument(j, {caps});') < \
    hostcxx.find(menu, 'sPlando.sohDump = sSohDump ? sSohDump() : "";')
