#include "rando/SingleGameFill.h"
#include "ComboRandomizerProfiles.h"
#include <cassert>
#include <iostream>
using namespace ComboRando;
using Json = nlohmann::json;
static std::vector<std::string> owned;
static bool blocked = false;
static void Reset() { owned.clear(); }
static void Own(const char* raw) { owned = Json::parse(raw).get<std::vector<std::string>>(); }
static const char* ReachOot() {
    return blocked ? "[\"Start\"]" : "[\"Start\",\"Second\",\"Ganon\"]";
}
static const char* ReachMm() {
    return blocked ? "[\"Start\"]" : "[\"Start\",\"Second\",\"Defeat Majora\"]";
}
static void Place(const char*, const char*) {}
static uint8_t ShutPortal() { assert(false && "A solo fill queried the native cross-game portal"); return 0; }
template<class F> static void Refuses(F f) { bool refused = false; try { f(); } catch (const std::exception&) { refused = true; } assert(refused); }
static Json Seed(ComboGameMode mode) {
    Json seed = {{"gameMode", kGameModeKeys[mode]}, {"startingGame", ModeStartsMm(mode) ? "MM" : "OOT"},
                 {"foreign", Json::array()}, {"sharedItems", Json::array()},
                 {"oot", {{"placements", Json::object()}}}, {"mm", {{"placements", Json::object()}}}};
    seed["goal"] = CwGoalJson(SingleGameGoal(mode));
    StampComboSeedFormat(seed);
    return seed;
}
int main() {
    assert(ReadGameMode(Json::object()) == COMBO_MODE_COMBINED);
    Refuses([] { ReadGameMode(Json{{"gameMode", nullptr}}); });
    Refuses([] { ReadGameMode(Json{{"gameMode", "future-mode"}}); });
    for (int i=0;i<5;++i) assert(ReadGameMode(Json{{"gameMode", kGameModeKeys[i]}}) == i);
    for (auto mode : {COMBO_MODE_OOT_RANDO, COMBO_MODE_MM_RANDO, COMBO_MODE_OOT_VANILLA, COMBO_MODE_MM_VANILLA}) {
        auto seed = Seed(mode);
        assert(seed["fileType"] == kGameModeSeedFileType && IsComboSeedDocument(seed));
        RequireSupportedSeedDocument(seed, Entrances::CapabilitySet{});
        auto wrong = seed; wrong["startingGame"] = ModeStartsMm(mode) ? "OOT" : "MM";
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        wrong = seed; wrong["goal"]["type"] = "bosses";
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        wrong = seed; wrong["version"] = 2;
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        wrong = seed; wrong["foreign"].push_back({{"check", "Foreign"}});
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        wrong = seed; wrong["sharedItems"].push_back("bows");
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        wrong = seed; wrong[ModeStartsMm(mode) ? "oot" : "mm"]["placements"]["Inactive"] = "Item";
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        wrong = seed; wrong["entranceLayout"] = nullptr;
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        wrong = seed; wrong["fileType"] = kComboSeedFileType;
        Refuses([&] { RequireSupportedSeedDocument(wrong, Entrances::CapabilitySet{}); });
        auto draft = Profiles::SettingsDocument(seed, Entrances::CapabilitySet{});
        assert(ReadGameMode(draft) == mode && !draft["oot"].contains("placements"));
        Profiles::Store profiles(Json{{"fileType", kComboSeedFileType}}, Entrances::CapabilitySet{});
        profiles.setDraft(1, draft);
        assert(ReadGameMode(profiles.draft(0)) == COMBO_MODE_COMBINED);
        auto attempt = profiles.begin(1, Json(), "same-seed");
        assert(ReadGameMode(attempt.setup) == mode);
        profiles.setDraft(1, Json{{"gameMode", "combined"}});
        assert(ReadGameMode(attempt.setup) == mode); // Immutable in-flight run despite later draft edits.
    }
    for (auto mode : {COMBO_MODE_OOT_VANILLA, COMBO_MODE_MM_VANILLA}) {
        Json setup = {{"gameMode", kGameModeKeys[mode]}, {"opaqueFutureSetting", 17}, {"seed", "old"}};
        const auto original = setup;
        auto seed = PrepareVanillaRun(setup, Json::object(), "new", 42, 123456789, 2);
        assert(setup == original && seed["opaqueFutureSetting"] == 17);
        assert(seed["oot"]["placements"].empty() && seed["mm"]["placements"].empty());
        assert(seed["foreign"].empty() && seed["sharedItems"].empty());
        assert(seed["file_hash"] == Json::array({1,23,45,67,89}));
        assert(ReadGameMode(seed["comboProfile"]["settings"]) == mode);
        RequireSupportedSeedDocument(seed, Entrances::CapabilitySet{});
        // A draft made from a bundled Master Quest seed keeps its data source
        // (SetupForSeed). A vanilla game rolls no MQ dungeons, so its seed
        // records none, and the reload's MQ check admits it.
        Json bundled = Seed(COMBO_MODE_OOT_RANDO);
        ComboMq::StampSeedMq(bundled["oot"], ComboMq::SeedMqSource::Bundled,
                             1 << 3);
        auto draft =
            Profiles::SetupForSeed(bundled, Entrances::CapabilitySet{});
        assert(ComboMq::MqDataSourceOf(draft["oot"]) ==
               ComboMq::SeedMqSource::Bundled);
        draft["gameMode"] = kGameModeKeys[mode];
        auto fromBundled = PrepareVanillaRun(
            Profiles::SettingsDocument(draft, Entrances::CapabilitySet{}),
            Json::object(), "new", 42, 123456789, 2);
        assert(ComboMq::MqDataSourceOf(fromBundled["oot"]) ==
               ComboMq::SeedMqSource::Legacy);
        assert(ComboMq::RecordedMqDungeonMask(fromBundled["oot"]) == -1);
        assert(ComboMq::SeedMqPlayable(fromBundled["oot"], false));
        RequireSupportedSeedDocument(fromBundled, Entrances::CapabilitySet{});
    }
    Refuses([] { PrepareVanillaRun(Json::object(), Json::object(), "", 0, 0, 0); });
    Json dump = {{"checks", {{{"name", "Start"}}, {{"name", "Second"}}}},
                 {"pool", {{{"name", "Filler A"}, {"advancement", false}, {"category", "junk"}},
                           {{"name", "Filler B"}, {"advancement", false}, {"category", "junk"}}}}};
    for (auto mode : {COMBO_MODE_OOT_RANDO, COMBO_MODE_MM_RANDO}) {
        OracleFns native{Reset, Own, mode == COMBO_MODE_OOT_RANDO ? ReachOot : ReachMm, Place, ShutPortal};
        const auto active = SingleGameOracle(native), inactive = InactiveGameOracle();
        const bool mm = ModeStartsMm(mode);
        for (int seed=1;seed<=30;++seed) {
            auto fill = CrossWorldCombinedFill((mm ? InactiveGameDump() : dump).dump(),
                (mm ? dump : InactiveGameDump()).dump(), seed, mm ? inactive : active, mm ? active : inactive,
                nullptr, "", OotAccess::ALL_REACHABLE, SingleGameGoal(mode), mm ? GAME_MM : GAME_OOT);
            assert(fill.success);
            auto result = Json::parse(fill.spoilerJson);
            assert(result["foreign"].empty());
            assert(result[mm ? "oot" : "mm"].empty());
            assert(result[mm ? "mm" : "oot"].size() == 2);
        }
        blocked = true;
        auto fill = CrossWorldCombinedFill((mm ? InactiveGameDump() : dump).dump(),
            (mm ? dump : InactiveGameDump()).dump(), 1, mm ? inactive : active, mm ? active : inactive,
            nullptr, "", OotAccess::ALL_REACHABLE, SingleGameGoal(mode), mm ? GAME_MM : GAME_OOT);
        assert(!fill.success); // Missing own boss or own optional check cannot be certified by the other world.
        blocked = false;
    }
    std::cout << "Game modes: profile isolation, compatibility refusals, 60 solo fills and two negative fixed points passed\n";
}
