"""Exercise seed compatibility and production import/save preflight with engine stubs."""
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
Json legacy(bool version) {
    Json seed = {{"fileType", kComboSeedFileType}, {"oot", {{"settings", Json::object()}}},
                 {"mm", {{"settings", Json::object()}}}, {"gearRules", {{"boots", true}}},
                 {"faroreRules", {{"crossAge", true}}}};
    if (version) seed["version"] = 1;
    return seed;
}
int main() {
    for (bool version : {false, true}) {
        auto seed = legacy(version);
        const auto bytes = seed.dump(2);
        StampComboSeedFormat(seed);
        assert(seed.dump(2) == bytes); // Both existing writer contracts remain byte-identical.
        RequireSupportedSeedDocument(seed);
        assert(IsComboSeedDocument(seed));
        write("seed.json", seed);
        Json imported;
        assert(TryLoadComboSeedFile("seed.json", imported) && imported == seed);
        Profiles::Store profiles(seed);
        profiles.importSeed(0, seed, true);
        assert(profiles.pendingSeed(0) == seed);
    }
    RequireSupportedSeedCapabilities(Json{{"settings", {{"native-option", 1}}}});
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
        rejects([&] { RequireSupportedSeedDocument(bad); });
        rejects([&] { Profiles::SettingsDocument(bad); });
        rejects([&] { Entrances::ApplyNativeSeed(sEntranceNative, bad); });
    }
    for (const Json& version : {Json(), Json(1), Json(2), Json(99), Json("2")}) {
        auto bad = future;
        bad["version"] = version;
        rejects([&] { RequireSupportedSeedDocument(bad); });
    }
    auto absent = future;
    absent.erase("entranceLayout");
    rejects([&] { RequireSupportedSeedDocument(absent); });
    write("future.json", future);
    Json output = {{"untouched", true}};
    assert(!TryLoadComboSeedFile("future.json", output) && output.at("untouched") == true);
    char futurePath[] = "future.json";
    assert(Rando_HandleSpoilerDrop(futurePath) && dropped == "ComboDroppedFile" && mutations == 0);
    Profiles::Store profiles(legacy(false));
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
    for (const Json& seed : {future, mislabeled}) for (bool cached : {false, true}) {
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
    rejects([&] { Profiles::SeedFromContainer(stringContainer); });
    // Supported no-layout seed still binds normally, proving the stubbed native path is usable.
    g_containerCache.clear();
    write(ComboContainerPath(1), {{"oot", nativePage}, {"combo", {{"rando", legacy(false)}}}});
    SaveManager manager;
    mutations = containerLoads = popups = 0;
    assert(manager.LoadFile(1, true));
    assert(gSaveContext.fileNum == 1 && mutations > 0 && containerLoads == 1 && popups == 0);
    assert(manager.comboLoadState == ComboLoadState::Ready);
    std::cout << "PASS legacy bytes/imports, future discriminator, capability rejection and production native "
                 "save preflight before context/slot mutation\n";
}
'''
cpp = args.build_dir / 'seed_format.cpp'
cpp.write_text(source, encoding='utf-8')
exe = hostcxx.build(args.compiler, cpp, warnings=False, includes=[args.source / 'combo', args.json_include])
subprocess.run([str(exe)], cwd=args.build_dir, check=True, timeout=hostcxx.RUN_TIMEOUT)

# Full headless/main/UI dependencies are unavailable to this small harness. Verify their actual
# boundary calls and writer ordering in addition to executing the shared policies above.
headless = (args.source / 'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(headless, 'spoiler = nlohmann::json::parse(ReadFile(playthroughFile)); ComboRando::RequireSupportedSeedDocument(spoiler);')
assert hostcxx.find(headless, 'ComboRando::RequireSupportedSeedCapabilities(sj);') < hostcxx.find(headless, 'const auto& oot = sj.contains("oot") ? sj["oot"] : sj;')
assert hostcxx.contains(headless, 'ComboRando::StampComboSeedFormat(consolidated); f << consolidated.dump(2);')
assert hostcxx.contains(launcher, 'ComboRando::StampComboSeedFormat(consolidated); g_ConsolidatedJson = consolidated.dump(2);')
assert hostcxx.contains(launcher, 'j = *profileSeed; ComboRando::RequireSupportedSeedDocument(j);')
menu = (args.source / 'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')
assert hostcxx.find(menu, 'ComboRando::RequireSupportedSeedDocument(j);') < hostcxx.find(menu, 'sPlando.sohDump = sSohDump ? sSohDump() : "";')
