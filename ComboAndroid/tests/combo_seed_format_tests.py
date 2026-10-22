"""Exercise seed compatibility and production import/save preflight with engine stubs.

The build's capability set is the shipping one, which admits only the vanilla Wallmaster
contract (P1-05): a vanilla layout seed is accepted, and every other layout is decoded and
refused with its reason. A build with no capability, as 0.4.4, refuses before decoding. A
test-only set (the whole catalog) exercises the acceptance path the M1-03 validator adds.
"""
from pathlib import Path
import argparse
import json
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
#include <tuple>
#include <vector>
#include "ComboRandomizerSavedSeed.h"
#include "rando/entrances/NativeBridge.h"
using Json = nlohmann::json;
using namespace ComboRando;
std::mutex g_containerMutex;
std::map<int, Json> g_containerCache;
int containerLoads = 0, mutations = 0, drains = 0, popups = 0;
std::string popupTitle, popupMessage, dropped;
std::filesystem::path ComboContainerPath(int slot) { return "slot" + std::to_string(slot) + ".combosav"; }
bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
Json& LoadOrCreateContainer(int slot) {
    ++containerLoads;
    if (!g_containerCache.contains(slot))
        g_containerCache[slot] = Profiles::ReadSavedContainerFile(ComboContainerPath(slot));
    return g_containerCache.at(slot);
}
namespace SohGui {
void RegisterPopup(const std::string& title, const std::string& message) {
    ++popups;
    popupTitle = title;
    popupMessage = message;
}
}
namespace SohUtils { bool IsStringEmpty(const char* value) { return !value || !value[0]; } }
void CVarSetString(const char* key, const char*) { dropped = key; }
void CVarSetInteger(const char*, int) { ++mutations; }
static Entrances::NativeFns sEntranceNative;
// OTRGlobals.cpp's setter, which SaveManager::LoadFile calls with the flags it takes off the page (M1-09).
std::vector<uint32_t> runtimeFlags;
void SOH_SetEntranceRuntimeFlags(uint32_t flags) { runtimeFlags.push_back(flags); }
std::string flaggedPage;
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
    bool archiveMq = false, bundledMq = false;
    bool HasMasterQuest() { return archiveMq; }
    bool HasBundledMasterQuest() { return bundledMq; }
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
// The SaveFileMetaInfo fields file select's MQ explanation reads (SaveManager.h).
struct SaveFileMetaInfo {
    unsigned requiresMasterQuest = 0;
    unsigned char mqDataSource = 0;
};
class SaveManager {
  public:
    static constexpr int MaxFiles = 3;
    static SaveManager* Instance;
    SaveFileMetaInfo fileMetaInfo[MaxFiles];
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
SaveManager* SaveManager::Instance = nullptr;
'''
source += function(native, 'bool SaveManager::LoadFile(int fileNum, bool initializeFullContext) {') + '\n'
source += function(native, 'extern "C" void Save_ExplainMqRefusal(int fileNum) {') + '\n'
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
// The 0.4.4 refusal, raised before the layout is decoded by a build with no capability.
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
    assert(!shipping.devBuild && shipping == Entrances::CapabilitySet::AtLeast(Entrances::Stage::Public) &&
           shipping.Active(Entrances::kVanillaWallmasterCapability)); // P1-05: only the vanilla contract
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
        rejectsLayout([&] { RequireSupportedSeedDocument(bad, shipping); });
        rejectsLayout([&] { Profiles::SettingsDocument(bad, shipping); });
        rejectsLayout([&] { Entrances::ApplyNativeSeed(sEntranceNative, bad, shipping); });
    }
    for (const Json& version : {Json(), Json(1), Json(2), Json(99), Json("2")}) {
        auto bad = future;
        bad["version"] = version;
        rejectsLayout([&] { RequireSupportedSeedDocument(bad, shipping); });
    }
    auto absent = future;
    absent.erase("entranceLayout");
    rejectsLayout([&] { RequireSupportedSeedDocument(absent, shipping); });
    // A complete, valid layout of another contract (the dev-only MM tranche) is refused too.
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
    rejectsLayout([&] { RequireSupportedSeedDocument(layoutSeed, shipping); });
    rejectsLayout([&] { RequireSupportedSettingsCapabilities(layoutSeed, shipping); });
    rejectsLayout([&] { Profiles::SettingsDocument(layoutSeed, shipping); });
    rejectsLayout([&] { Profiles::SetupForSeed(layoutSeed, shipping); });
    rejectsLayout([&] { Entrances::ApplyNativeSeed(sEntranceNative, layoutSeed, shipping); });
    rejectsLayout([&] { Profiles::SeedFromContainer({{"combo", {{"rando", layoutSeed}}}}, shipping); });
    // A build with no active capability still refuses before decoding, as 0.4.3 and 0.4.4 do.
    rejectsBeforeDecode([&] { RequireSupportedSeedDocument(layoutSeed, Entrances::CapabilitySet{}); });
    rejectsBeforeDecode([&] { RequireSupportedSeedDocument(future, Entrances::CapabilitySet{}); });

    // The shipping set's own contract (P1-05): a vanilla Wallmaster seed passes the seed guard, the
    // profile view and the saved-slot reader. A dev build's vanilla seed, a Master Quest dungeon and a
    // private MM profile beside it are decoded and refused.
    Entrances::EntranceLayout vanilla;
    vanilla.required = {std::string(Entrances::kVanillaWallmasterCapability)};
    vanilla.settings.erWallmasters = Entrances::Mode::OwnGame;
    vanilla.overrides = Entrances::Overrides{@VANILLA_ROUTES@};
    auto vanillaSeed = legacy(true);
    vanillaSeed["entranceLayout"] = Entrances::EncodeEntranceLayout(vanilla);
    StampComboSeedFormat(vanillaSeed);
    RequireSupportedSeedDocument(vanillaSeed, shipping);
    RequireSupportedSettingsCapabilities(vanillaSeed, shipping);
    auto vanillaRouteless = vanillaSeed["entranceLayout"];
    vanillaRouteless.erase("overrides");
    assert(Profiles::SettingsDocument(vanillaSeed, shipping).at("entranceLayout") == vanillaRouteless);
    Profiles::Store shippingStore(legacy(false), shipping);
    shippingStore.importSeed(0, vanillaSeed, true);
    assert(shippingStore.pendingSeed(0) == vanillaSeed);
    assert(shippingStore.draft(0).at("entranceLayout") == vanillaRouteless);
    Profiles::Store shippingReread(legacy(false), shipping);
    shippingReread.restore(shippingStore.json());
    assert(shippingReread.json() == shippingStore.json());
    // A seed made in the app also records the draft it was requested with (comboProfile): a plain settings document,
    // fileType ComboShipRandomizer version 1. Merging it keeps the layout's file type, so the import, a copy of its
    // settings, a file without a draft and the pending-seed migration all store a valid setup (0.4.13 RC2 emulator
    // finding; plando_app_seed_import_tests.py runs the whole Plandomizer path).
    auto appSeed = vanillaSeed;
    appSeed["comboProfile"] = {{"version", 1}, {"file", 2}, {"revision", 1},
                               {"settings", {{"fileType", kComboSeedFileType}, {"version", 1}, {"seed", "app"},
                                             {"setupCVars", {{"gCombo.Rando.Goal", 0}}}}}};
    Profiles::Store appStore(legacy(false), shipping);
    appStore.importSeed(1, appSeed, true);
    assert(appStore.pendingSeed(1) == appSeed);
    const auto appDraft = appStore.draft(1);
    assert(appDraft.at("fileType") == kEntranceSeedFileType && appDraft.at("version") == kEntranceSeedVersion);
    assert(appDraft.at("entranceLayout") == vanillaRouteless && appDraft.at("seed") == "app");
    appStore.copySaved(1, 0, appSeed);
    assert(appStore.draft(0) == appDraft && Profiles::Store(legacy(false), shipping).draft(2, appSeed) == appDraft);
    Profiles::Store appMigrated(legacy(false), shipping);
    appMigrated.migrateLegacyPending(appSeed);
    Profiles::Store(legacy(false), shipping).restore(appMigrated.json());
    // Without a layout the requested file type and version stay.
    auto plainApp = legacy(false);
    plainApp["comboProfile"] = appSeed["comboProfile"];
    const auto plainSetup = Profiles::SetupForSeed(plainApp, shipping);
    assert(plainSetup.at("fileType") == kComboSeedFileType && plainSetup.at("version") == 1);
    assert(Profiles::SeedFromContainer({{"combo", {{"rando", vanillaSeed.dump()}}}}, shipping) == vanillaSeed);
    auto devVanilla = vanilla;
    devVanilla.devBuild = true;
    auto mqVanilla = vanilla;
    mqVanilla.ootMq[Entrances::OotMqIndex("Forest")] = true;
    for (const auto& layout : {devVanilla, mqVanilla}) {
        auto refused = vanillaSeed;
        refused["entranceLayout"] = Entrances::EncodeEntranceLayout(layout);
        rejectsLayout([&] { RequireSupportedSeedDocument(refused, shipping); });
        rejectsLayout([&] { Profiles::SettingsDocument(refused, shipping); });
        rejectsLayout([&] { Profiles::SeedFromContainer({{"combo", {{"rando", refused}}}}, shipping); });
    }
    for (const char* key : kLegacyEntranceKeys) {
        auto both = vanillaSeed;
        both[key] = Json::object();
        rejectsLayout([&] { RequireSupportedSeedDocument(both, shipping); });
    }

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
    rejectsLayout([&] { Profiles::Store(legacy(false), shipping).restore(devStore.json()); });
    assert(Profiles::SeedFromContainer({{"combo", {{"rando", layoutSeed.dump()}}}}, dev) == layoutSeed);
    // The song facts a layout's id carries (limits design, Stage 1): "Play warp songs across games" in logic takes
    // wallmasters-own-game-songs-v1 in either quest context; the vanilla and quests ids refuse it and the songs id
    // needs it. No id takes song destination shuffle or a starting Soaring under logic, and no earlier id a starting
    // warp song (the songs-v2 id below). No Logic has no song facts. A profile settings document carries none, and
    // neither does a song map that does not parse.
    {
        const auto all = Entrances::CapabilitySet::AtLeast(Entrances::Stage::Hidden);
        SongTravelMap foreignMap, shuffleMap;
        foreignMap.foreignSongs = foreignMap.logicForeignSongs = true;
        shuffleMap.enabled = shuffleMap.logicEnabled = true;
        auto mqLayout = vanilla;
        mqLayout.ootMq[Entrances::OotMqIndex("Water")] = true;
        const auto seedOf = [&](const Entrances::EntranceLayout& base, std::string_view id, const Json& songs,
                                bool noLogic) {
            auto layout = base;
            layout.required = {std::string(id)};
            auto seed = vanillaSeed;
            seed["entranceLayout"] = Entrances::EncodeEntranceLayout(layout);
            if (!songs.is_null())
                seed["songTravel"] = songs;
            seed["accessibility"] = {{"version", 1}, {"mode", noLogic ? "noLogic" : "everyCheck"}};
            return seed;
        };
        const auto refusedFor = [&](const Json& seed, const std::string& reason) {
            std::string message;
            try { RequireSupportedSeedDocument(seed, all); }
            catch (const UnsupportedComboSeedFormat& error) { message = error.what(); }
            assert(message.find(reason) != std::string::npos);
            rejectsLayout([&] { Profiles::SeedFromContainer({{"combo", {{"rando", seed}}}}, all); });
        };
        const auto songsId = Entrances::kSongsWallmasterCapability;
        const std::string_view quests = Entrances::kQuestsWallmasterCapability,
                               plain = Entrances::kVanillaWallmasterCapability;
        for (const auto* base : {&vanilla, &mqLayout}) {
            const auto other = base == &vanilla ? plain : quests;
            const auto songs = seedOf(*base, songsId, foreignMap.json(), false);
            RequireSupportedSeedDocument(songs, all);
            RequireSupportedSettingsCapabilities(songs, all);
            assert(Profiles::SeedFromContainer({{"combo", {{"rando", songs.dump()}}}}, all) == songs);
            refusedFor(seedOf(*base, other, foreignMap.json(), false), "which does not admit foreign-song logic");
            RequireSupportedSeedDocument(seedOf(*base, other, foreignMap.json(), true), all);
            RequireSupportedSeedDocument(seedOf(*base, other, SongTravelMap{}.json(), false), all);
            refusedFor(seedOf(*base, songsId, Json(), false), "without foreign-song logic");
            refusedFor(seedOf(*base, songsId, SongTravelMap{}.json(), false), "without foreign-song logic");
            refusedFor(seedOf(*base, songsId, foreignMap.json(), true), "without foreign-song logic");
            // A profile settings document drops the routes and the song facts with them.
            const auto document = Profiles::SettingsDocument(seedOf(*base, songsId, Json(), false), all);
            RequireSupportedSettingsCapabilities(document, all);
            // Destination shuffle and starting songs, refused under every earlier id.
            for (const auto id : {std::string_view(songsId), other}) {
                const auto songsMap = id == songsId ? foreignMap.json() : SongTravelMap{}.json();
                auto shuffled = seedOf(*base, id, shuffleMap.json(), false);
                if (id == songsId)
                    shuffled["songTravel"]["logicForeignSongs"] = true;
                refusedFor(shuffled, "song travel logic");
                auto prelude = seedOf(*base, id, songsMap, false);
                prelude["oot"]["settings"]["gRandoSettings.StartingPreludeOfLight"] = 1;
                refusedFor(prelude, "starting Prelude of Light");
                // MM's starting items as a seed stores them: 2Ship's spoiler names (BenPort.cpp).
                auto soaring = seedOf(*base, id, songsMap, false);
                soaring["mm"]["settings"]["gRando.StartingItems"] = {"RI_SONG_TIME", "RI_SONG_SOARING"};
                refusedFor(soaring, "starting Song of Soaring");
                soaring["mm"]["settings"]["gRando.StartingItems"] = {"Song of Soaring"};
                refusedFor(soaring, "starting Song of Soaring");
                prelude["accessibility"]["mode"] = "noLogic";
                if (id != songsId)
                    RequireSupportedSeedDocument(prelude, all);
            }
        }
        // A song map that does not parse has no song facts here (loading it fails on its own): the old id passes this
        // guard and the songs id is refused.
        auto malformed = seedOf(vanilla, plain, foreignMap.json(), false);
        malformed["songTravel"]["destinations"] = Json::array();
        RequireSupportedSeedDocument(malformed, all);
        malformed = seedOf(vanilla, songsId, foreignMap.json(), false);
        malformed["songTravel"]["destinations"] = Json::array();
        refusedFor(malformed, "without foreign-song logic");
        // This build's shipping set takes the songs id (Public from 0.4.13).
        const auto songs = seedOf(vanilla, songsId, foreignMap.json(), false);
        assert(shipping.Active(songsId));
        RequireSupportedSeedDocument(songs, shipping);
        assert(Profiles::SeedFromContainer({{"combo", {{"rando", songs}}}}, shipping) == songs);
        refusedFor(seedOf(vanilla, plain, foreignMap.json(), false), "which does not admit foreign-song logic");
        // A starting OoT warp song under logic (limits design, Stage 2) takes wallmasters-own-game-songs-v2, in either
        // quest context, with foreign-song logic or without, read from the OoT settings (MM's starting items hold
        // none); every earlier id refuses it (above), and the songs-v2 id needs one. A starting Soaring
        // and destination shuffle stay refused under it. With No Logic there is no song, so the songs-v2 id is refused.
        const auto v2 = Entrances::kStartingSongsWallmasterCapability;
        for (const auto* base : {&vanilla, &mqLayout})
            for (const bool foreign : {false, true}) {
                const auto map = foreign ? foreignMap.json() : SongTravelMap{}.json();
                for (const auto& [key, name] : Entrances::kOotStartingSongSettings) {
                    auto seed = seedOf(*base, v2, map, false);
                    seed["oot"]["settings"][std::string("gRandoSettings.") + key] = 1;
                    RequireSupportedSeedDocument(seed, all);
                    assert(Profiles::SeedFromContainer({{"combo", {{"rando", seed.dump()}}}}, all) == seed);
                    seed["accessibility"]["mode"] = "noLogic";
                    refusedFor(seed, "without a starting warp song");
                    auto old = seedOf(*base, foreign ? std::string_view(songsId) : base == &vanilla ? plain : quests,
                                      map, false);
                    old["oot"]["settings"][std::string("gRandoSettings.") + key] = 1;
                    refusedFor(old, std::string("which does not admit a starting ") + name);
                }
                refusedFor(seedOf(*base, v2, map, false), "without a starting warp song");
                // MM's starting items, as a seed stores them (2Ship's spoiler names), never hold an OoT warp song:
                // 2Ship has no such item (entrance_temple_song_lemma_tests.py), so they never make one.
                auto mmSide = seedOf(*base, v2, map, false);
                mmSide["mm"]["settings"]["gRando.StartingItems"] = {"RI_SONG_TIME", "RI_OCARINA"};
                refusedFor(mmSide, "without a starting warp song");
                auto soaring = seedOf(*base, v2, map, false);
                soaring["oot"]["settings"]["gRandoSettings.StartingPreludeOfLight"] = 1;
                soaring["mm"]["settings"]["gRando.StartingItems"] = {"Song of Time", "Song of Soaring"};
                refusedFor(soaring, "starting Song of Soaring");
                auto shuffled = seedOf(*base, v2, shuffleMap.json(), false);
                shuffled["songTravel"]["logicForeignSongs"] = foreign;
                shuffled["oot"]["settings"]["gRandoSettings.StartingPreludeOfLight"] = 1;
                refusedFor(shuffled, "song travel logic");
            }
        // This build's shipping set takes the songs-v2 id (Public from 0.4.13).
        auto startingSong = seedOf(vanilla, v2, SongTravelMap{}.json(), false);
        startingSong["oot"]["settings"]["gRandoSettings.StartingPreludeOfLight"] = 1;
        assert(shipping.Active(v2));
        RequireSupportedSeedDocument(startingSong, shipping);
        assert(Profiles::SeedFromContainer({{"combo", {{"rando", startingSong}}}}, shipping) == startingSong);
    }
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
    Json solo = {{"fileType", kGameModeSeedFileType}, {"version", 1}, {"gameMode", "mm-vanilla"},
        {"startingGame", "MM"}, {"goal", {{"type", "majora"}}}, {"foreign", Json::array()},
        {"sharedItems", Json::array()}, {"oot", {{"placements", Json::object()}}},
        {"mm", {{"placements", Json::object()}}}};
    RequireSupportedSeedDocument(solo, shipping);
    Json unknownMode = solo; unknownMode["gameMode"] = "future-game-mode";
    Json unsupportedVersion = solo; unsupportedVersion["version"] = 2;
    Json wrongGoal = solo; wrongGoal["goal"]["type"] = "bosses";
    Json wrongStart = solo; wrongStart["startingGame"] = "OOT";
    Json missingMode = solo; missingMode.erase("gameMode");
    Json foreignSolo = solo; foreignSolo["foreign"].push_back({{"check", "foreign"}});
    for (const Json& seed : {future, mislabeled, layoutSeed, unknownMode, unsupportedVersion, wrongGoal,
                            wrongStart, missingMode, foreignSolo}) for (bool cached : {false, true}) {
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
        runtimeFlags.clear();
        assert(!manager.LoadFile(0, true));
        assert(runtimeFlags.empty()); // refused before the runtime flags are taken
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
    runtimeFlags.clear();
    assert(manager.LoadFile(1, true));
    assert(gSaveContext.fileNum == 1 && mutations > 0 && containerLoads == 1 && popups == 0);
    assert(manager.comboLoadState == ComboLoadState::Ready);
    assert(runtimeFlags == std::vector<uint32_t>{0}); // no layout: no ER-wide runtime flag
    // Master Quest data source (combo/mq/MqSeedMarker.h): a save whose seed records the bundled data and has MQ
    // dungeons is refused before any context or slot change where that data is off; an unmarked save is not, so
    // file select keeps SoH's own rule for it.
    auto mqPage = [&](const Json& data) {
        Json page = nativePage;
        page["sections"]["randomizer"] = {{"version", 1}, {"data", data}};
        return page;
    };
    const Json marked = mqPage({{"mqDataSource", "ootmm-32.3"}, {"masterQuestDungeonCount", 2}});
    const std::string kBundledRefusal =
        "This save uses Master Quest dungeons from OoTMM's data, which needs an OoT USA 1.0 ROM.";
    const std::string kUnknownRefusal = "This save uses a Master Quest data source this version does not know.";
    const Json unknown = mqPage({{"mqDataSource", "ootmm-40.0"}, {"masterQuestDungeonCount", 0}});
    const Json unmarked = mqPage({{"masterQuestDungeonCount", 2}});
    for (const auto& [page, archive, bundled, loads] :
         {std::tuple{marked, true, false, false}, std::tuple{unknown, true, true, false},
          std::tuple{marked, false, true, true}, std::tuple{unmarked, false, false, true}}) {
        globals.archiveMq = archive;
        globals.bundledMq = bundled;
        g_containerCache.clear();
        write(ComboContainerPath(1), {{"oot", page}, {"combo", {{"rando", legacy(false)}}}});
        SaveManager mqManager;
        auto* context = globals.gRandoContext.get();
        gSaveContext.fileNum = 2;
        mutations = popups = 0;
        runtimeFlags.clear();
        assert(mqManager.LoadFile(1, true) == loads);
        if (!loads) {
            assert(mutations == 0 && popups == 1 && popupTitle == "Master Quest data unavailable");
            assert(popupMessage == (page == marked ? kBundledRefusal : kUnknownRefusal));
            assert(gSaveContext.fileNum == 2 && globals.gRandoContext.get() == context && runtimeFlags.empty());
            assert(mqManager.comboLoadState == ComboLoadState::Blocked);
        } else {
            assert(popups == 0 && gSaveContext.fileNum == 1);
        }
    }
    // File select greys out a save it cannot open and only plays an error sound when it is chosen. For a save whose
    // MQ data source is the reason, Save_ExplainMqRefusal shows LoadFile's popup; any other save gets none.
    SaveManager selectManager;
    SaveManager::Instance = &selectManager;
    for (const auto& [source, needsMq, archive, bundled, message] :
         {std::tuple{1, 1u, true, false, kBundledRefusal}, std::tuple{1, 1u, false, true, std::string()},
          std::tuple{1, 0u, false, false, std::string()}, std::tuple{2, 0u, true, true, kUnknownRefusal},
          std::tuple{0, 1u, false, true, std::string()}}) {
        globals.archiveMq = archive;
        globals.bundledMq = bundled;
        selectManager.fileMetaInfo[2] = {needsMq, static_cast<unsigned char>(source)};
        popups = 0;
        popupMessage.clear();
        Save_ExplainMqRefusal(2);
        assert(popups == (message.empty() ? 0 : 1) && popupMessage == message);
        assert(message.empty() || popupTitle == "Master Quest data unavailable");
    }
    popups = 0;
    Save_ExplainMqRefusal(-1);
    Save_ExplainMqRefusal(SaveManager::MaxFiles);
    assert(popups == 0);
    SaveManager::Instance = nullptr;
    globals.archiveMq = globals.bundledMq = false;
    // A page with the launcher's runtime flags (Combo_ReadGameSave adds them for a layout this build accepts):
    // the loader takes them before any save context change and applies the page as stored.
    Json flagged = nativePage;
    flagged[kComboEntranceRuntimeFlags] = Entrances::kEntranceRuntimeErAny;
    flaggedPage = flagged.dump();
    gComboReadGameSave = [](int, int) -> const char* { return flaggedPage.c_str(); };
    SaveManager flaggedManager;
    runtimeFlags.clear();
    assert(flaggedManager.LoadFile(1, true));
    assert(runtimeFlags == std::vector<uint32_t>{Entrances::kEntranceRuntimeErAny});
    assert(flaggedManager.saveBlock == nativePage);
    gComboReadGameSave = Combo_ReadGameSave;
    std::cout << "PASS legacy bytes/imports, future discriminator, capability rejection and production native "
                 "save preflight before context/slot mutation; the layout acceptance path behind them\n";
}
'''
# The pinned sample layout of the vanilla Wallmaster pool (tools/ootmm/m1-contract.json).
contract = json.loads((args.source / 'tools/ootmm/m1-contract.json').read_text(encoding='utf-8'))
sample = next(c for c in contract['combinations'] if c['id'] == 'erWallmasters=ownGame;erOneWays=none')['sampleLayout']
assert len(sample) == 13
source = source.replace('@VANILLA_ROUTES@', ', '.join('{"%s", "%s"}' % (src, dst) for src, dst in sample))
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
stamp = hostcxx.find(headless, 'ComboRando::StampComboSeedFormat(consolidated);')
preflight = hostcxx.find(headless, f'ComboRando::RequireSupportedSeedDocument(consolidated, {caps});', stamp)
opened = hostcxx.find(headless, 'std::ofstream f("saves/combo/comborando.spoiler.json", std::ios::trunc);', preflight)
written = hostcxx.find(headless, 'f << consolidated.dump(2);', opened)
closed = hostcxx.find(headless, 'f.close();', written)
assert stamp < preflight < opened < written < closed < hostcxx.find(headless, 'if (!f)', closed) < \
    hostcxx.find(headless, 'WriteEntranceSpoilerFile(', closed)
assert hostcxx.contains(launcher, 'ComboRando::StampComboSeedFormat(consolidated); g_ConsolidatedJson = consolidated.dump(2);')
assert hostcxx.contains(launcher, f'j = *profileSeed; ComboRando::RequireSupportedSeedDocument(j, {caps});')
assert hostcxx.contains(launcher, f'ComboRando::Profiles::Repository>(profilePath, ProfileNativeSnapshot(), {caps});')
menu = (args.source / 'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')
assert hostcxx.find(menu, f'ComboRando::RequireSupportedSeedDocument(j, {caps});') < \
    hostcxx.find(menu, 'sPlando.sohDump = sSohDump ? sSohDump() : "";')
