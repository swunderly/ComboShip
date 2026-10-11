#include "rando/OotMaskQuest.h"
// combo/ComboRandoHeadless.cpp
// ComboShip: standalone HEADLESS cross-world seed generator/validator. Loads soh.dll + 2ship.dll and
// drives the rando-only headless init (SOH_InitRandoHeadless / MM_InitRandoHeadless) + the cross-world
// fill directly — no game window, ResourceManager, audio, or ComboShip.exe boot.
//
// Modes (run from the game dir, next to soh.dll/2ship.dll):
//   comborando.exe [--seed <s>] [--count <n>]      generate+validate seed(s); also writes a consolidated
//                                                  spoiler to saves/combo/comborando.spoiler.json (count 1)
//   comborando.exe --playthrough <spoiler.json>    forward-traverse a finished seed to judge beatability
//
// --starting-game <oot|mm|random> overrides the menu's Starting Game CVar for generate mode (#135).
//
// Exact-seed repro (generate mode): --settings <consolidated.json> restores that seed's OOT/MM settings
// and tricks, and --master-seed <n> uses n directly instead of hashing --seed. Together they reproduce a
// reported seed's generation bit-for-bit, which plain --seed cannot.
//
// --playthrough runs two passes, ignoring the seed's No-Logic/Glitchless flag (it always evaluates real
// gates; permissiveness comes only from tricks): Pass 1 uses the seed's own tricks ("can this player beat
// it?"); if that sticks, Pass 2 enables every trick to tell "needs more tricks than configured" apart from
// "factually impossible" (e.g. a required item behind an unbreakable possession lock).
//
// Exit codes: 0 = beatable as configured; 3 = beatable only with all tricks; 1 = not beatable / seed
// failed; 2 = setup error.
#ifdef _WIN32
#define NOMINMAX // windows.h min/max macros clash with std::min/std::max in CrossWorldRando.h
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "rando/CrossWorldRando.h"
#include "rando/ComboPlaythrough.h"
#include "rando/ComboSeedFormat.h"
#include "rando/GameplayRules.h"
#include "rando/GenerationRules.h"
#include "rando/KokiriSeedRevision.h"
#include "rando/entrances/MmOrdinaryNativeBridge.h"
#include "rando/entrances/MmOrdinaryGenerationSettings.h"
#include "rando/entrances/MmMixedNativeBridge.h"
#include "rando/entrances/MmGrottoNativeBridge.h"
#include "rando/GameplayRuleSnapshots.h"
#include "rando/FaroreRules.h"
#include "rando/SpellRules.h"
#include "rando/GearRules.h"
#include "rando/SongTravel.h"
#include "rando/CrossForeign.h" // BuildForeignArray (foreign enrichment parity with RunComboFill)
#include "rando/CrossHints.h"

namespace {

typedef void (*FnVoidV)(void);
typedef const char* (*FnDump)(void);
typedef void (*FnSetSeed)(uint64_t);
typedef const char* (*FnGetForced)(uint32_t);
typedef void (*FnTakeStr)(const char*);
typedef void (*FnOracleVoid)(void);
typedef void (*FnOracleSetItems)(const char*);
typedef const char* (*FnOracleGetChecks)(void);
typedef void (*FnOraclePlaceItem)(const char*, const char*);
typedef uint8_t (*FnOracleGetPortalOpen)(void); // OOT->MM portal gate (see CrossWorldRando.h)

// Must match ComboShip.cpp's ComboHash (FNV-1a 32-bit) so headless seeds match in-game seeds.
uint32_t ComboHash(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s)
        h = (h ^ c) * 16777619u;
    return h;
}

// #135: resolve the starting-game config (0 = OOT, 1 = MM, 2 = Random) for one attempt. Must stay
// byte-identical to ComboShip.cpp's ResolveStartingGameMM, derivation string included.
bool ResolveStartingGameMM(int cfg, uint32_t masterSeed) {
    if (cfg == 2)
        return (ComboHash("startingGame:" + std::to_string(masterSeed)) & 1u) != 0;
    return cfg == 1;
}

// Runtime MM_InitRandoSaveFile installs these immutable envelopes after native settings.
// Match that order in replay, including all-off defaults for historical missing envelopes.
nlohmann::json MmSettingsForPlaythrough(const nlohmann::json& spoiler) {
    ComboRando::FaroreRules farore;
    ComboRando::SpellRules spell;
    ComboRando::GearRules gear;
    if (!ComboRando::ReadFaroreRules(spoiler.value("faroreRules", nlohmann::json()), farore) ||
        !ComboRando::ReadSpellRules(spoiler.value("spellRules", nlohmann::json()), spell) ||
        !ComboRando::ReadGearRules(spoiler.value("gearRules", nlohmann::json()), gear))
        throw std::runtime_error("Invalid immutable MM feature rules");
    auto settings = spoiler.value("mm", nlohmann::json::object()).value("settings", nlohmann::json::object());
    settings["gRando.Options.RO_MM_SPELL_WIND"] = int(farore.spellWindMm);
    settings["gRando.Options.RO_CROSS_GAME_FARORE"] = int(farore.crossGameFw);
    settings["gRando.Options.RO_MM_CROSS_AGE"] = int(farore.crossAge);
    settings["gRando.Options.RO_MM_SPELL_FIRE"] = int(spell.spellFireMm);
    settings["gRando.Options.RO_MM_SPELL_LOVE"] = int(spell.spellLoveMm);
    settings["gRando.Options.RO_MM_BOOTS_IRON"] = int(gear.bootsIronMm);
    settings["gRando.Options.RO_MM_BOOTS_HOVER"] = int(gear.bootsHoverMm);
    settings["gRando.Options.RO_MM_TUNIC_GORON"] = int(gear.tunicGoronMm);
    settings["gRando.Options.RO_MM_TUNIC_ZORA"] = int(gear.tunicZoraMm);
    return settings;
}

// Same module-loading seam as ComboShip.cpp's LoadDll/GetSym (see there for the RTLD notes).
#ifdef _WIN32
typedef HMODULE DllHandle;
DllHandle LoadGameModule(const char* name) {
    return LoadLibraryA(name);
}
template <typename T> T Sym(DllHandle m, const char* name) {
    return reinterpret_cast<T>(GetProcAddress(m, name));
}
#else
typedef void* DllHandle;
DllHandle LoadGameModule(const char* name) {
    // Bare names search the system library paths; the tool runs from the game dir, so anchor there.
    return dlopen((std::string("./") + name).c_str(), RTLD_NOW | RTLD_GLOBAL);
}
template <typename T> T Sym(DllHandle m, const char* name) {
    return reinterpret_cast<T>(dlsym(m, name));
}
#endif

std::string ReadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Render the spoiler's "seed" (string or number) as a label.
std::string SeedLabel(const nlohmann::json& spoiler) {
    if (spoiler.contains("seed")) {
        const auto& s = spoiler["seed"];
        if (s.is_string())
            return s.get<std::string>();
        if (s.is_number())
            return std::to_string(s.get<int64_t>());
    }
    return "?";
}

} // namespace

int main(int argc, char** argv) {
    std::string seed = "1";
    int count = 1;
    std::string playthroughFile;
    std::string settingsFile; // consolidated spoiler to restore settings/tricks from (exact-seed repro)
    bool haveMasterSeed = false;
    uint32_t masterSeedArg = 0;
    // #136: combined goal. Absent => read the menu CVars (matches in-game); 0 => force bosses.
    int triforceRequiredArg = -1;
    std::string goalArg;
    // #136: combined pool total. Absent (-1) => the menu CVar, or `required` when that is overridden.
    int triforceTotalArg = -1;
    // #135: starting game. Absent (-1) => read the menu CVar, same as in-game.
    int startingGameArg = -1;
    // Shared Items: "all", "none", or a comma-separated key list (see SharedItems.h). Empty => menu CVars.
    std::string sharedItemsArg;
    bool nativeMmOrdinaryIndoors = false, nativeMmOrdinaryTelescopes = false;
    bool nativeMmOrdinaryOverworld = false, nativeMmOrdinaryDungeons = false;
    nlohmann::json savedOrdinaryEntrances, savedGrottoEntrances, savedMixedEntrances;
    bool nativeMmMixed = false;
    bool nativeMmGrottos = false;     // private exact-generation validation; no public menu claim
    bool nativeMmWallmasters = false; // internal validation flag; no menu capability claim
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--mm-ordinary-indoors")
            nativeMmOrdinaryIndoors = true;
        if (a == "--mm-ordinary-telescopes") {
            nativeMmOrdinaryIndoors = true;
            nativeMmOrdinaryTelescopes = true;
        }
        if (a == "--mm-ordinary-overworld")
            nativeMmOrdinaryOverworld = true;
        if (a == "--mm-ordinary-dungeons")
            nativeMmOrdinaryDungeons = true;
        else if (a == "--mm-mixed")
            nativeMmMixed = true;
        else if (a == "--mm-grottos")
            nativeMmGrottos = true;
        else if (a == "--mm-wallmasters")
            nativeMmWallmasters = true;
        else if (a == "--goal" && i + 1 < argc)
            goalArg = argv[++i];
        else if (a == "--triforce-required" && i + 1 < argc)
            triforceRequiredArg = std::atoi(argv[++i]);
        else if (a == "--triforce-total" && i + 1 < argc)
            triforceTotalArg = std::atoi(argv[++i]);
        else if (a == "--starting-game" && i + 1 < argc) {
            std::string v = argv[++i];
            startingGameArg = v == "oot" ? 0 : v == "mm" ? 1 : v == "random" ? 2 : -2;
            if (startingGameArg == -2) {
                std::cerr << "[comborando] --starting-game must be oot, mm or random (got '" << v << "')\n";
                return 2;
            }
        } else if (a == "--shared-items" && i + 1 < argc) {
            sharedItemsArg = argv[++i];
        } else if (a == "--seed" && i + 1 < argc)
            seed = argv[++i];
        else if (a == "--count" && i + 1 < argc)
            count = std::max(1, std::atoi(argv[++i]));
        else if (a == "--playthrough" && i + 1 < argc)
            playthroughFile = argv[++i];
        else if (a == "--settings" && i + 1 < argc)
            settingsFile = argv[++i];
        else if (a == "--master-seed" && i + 1 < argc) {
            masterSeedArg = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            haveMasterSeed = true;
        }
    }

#ifdef _WIN32
    DllHandle soh = LoadGameModule("soh.dll");
    DllHandle mm = LoadGameModule("2ship.dll");
#elif defined(__APPLE__)
    DllHandle soh = LoadGameModule("libsoh.dylib");
    DllHandle mm = LoadGameModule("lib2ship.dylib");
#else
    DllHandle soh = LoadGameModule("libsoh.so");
    DllHandle mm = LoadGameModule("lib2ship.so");
#endif
    if (!soh || !mm) {
        std::cerr << "[comborando] failed to load the soh / 2ship game modules — run from the game directory\n";
        return 2;
    }

    auto SOH_InitRandoHeadless = Sym<FnVoidV>(soh, "SOH_InitRandoHeadless");
    auto MM_InitRandoHeadless = Sym<FnVoidV>(mm, "MM_InitRandoHeadless");
    auto SOH_Dump = Sym<FnDump>(soh, "SOH_DumpRandoStaticData");
    auto MM_Dump = Sym<FnDump>(mm, "MM_DumpRandoStaticData");
    auto SOH_DumpSettings = Sym<FnDump>(soh, "SOH_DumpRandoSettings");
    auto MM_DumpSettings = Sym<FnDump>(mm, "MM_DumpRandoSettings");
    auto SOH_RestoreSettings = Sym<FnTakeStr>(soh, "SOH_RestoreRandoSettings");
    auto SOH_SetKokiriRevision = Sym<ComboRando::FnSetKokiriSeedRevision>(soh, "SOH_SetKokiriSeedRevision");
    auto SOH_SetKokiriGrassRevision =
        Sym<ComboRando::FnSetKokiriSeedRevision>(soh, "SOH_SetKokiriGrassCatalogRevision");
    auto SOH_PrepContext = Sym<FnVoidV>(soh, "SOH_PrepRandoContext");
    auto MM_RestoreSettings = Sym<FnTakeStr>(mm, "MM_RestoreRandoSettings");
    auto SOH_SetSeed = Sym<FnSetSeed>(soh, "SOH_SetComboRandoSeed");
    auto MM_SetSeed = Sym<FnSetSeed>(mm, "MM_SetComboRandoSeed");
    auto SOH_GetForced = Sym<FnGetForced>(soh, "SOH_GetForcedPlacements");
    // OOT entrance shuffle (#90) — without this the validator checks a vanilla entrance graph.
    auto SOH_ShuffleEntrances = Sym<int (*)(uint64_t)>(soh, "SOH_ShuffleEntrancesForCombo");
    auto SOH_DumpEntranceOverrides = Sym<FnDump>(soh, "SOH_DumpEntranceOverrides");
    auto SOH_ApplyEntranceOverrides = Sym<int (*)(const char*)>(soh, "SOH_ApplyEntranceOverridesForCombo");
    auto MM_Restore = Sym<FnOracleVoid>(mm, "Combo_MM_Rando_Restore");
    auto SOH_DumpEnabledTricks = Sym<FnDump>(soh, "SOH_DumpEnabledTricks");
    auto readRule = Sym<int32_t (*)(const char*, int32_t)>(soh, "SOH_ReadGameplayRule");
    auto writeRule = Sym<void (*)(const char*, int32_t)>(soh, "SOH_WriteGameplayRule");
    auto setTricksCsv = Sym<FnTakeStr>(soh, "SOH_SetComboTricksCsv");
    auto restoreRules = [&](const nlohmann::json& rules) {
        ComboRando::ApplyGameplayRules(rules, [&](const char* key, int value) {
            if (writeRule)
                writeRule(key, value);
        });
        if (setTricksCsv && rules.contains("ootTricks") && rules["ootTricks"].is_string())
            setTricksCsv(rules["ootTricks"].get<std::string>().c_str());
    };

    auto SOH_DumpRandoHintData = Sym<FnDump>(soh, "SOH_DumpRandoHintData"); // cross-hint verification
    auto SOH_SetEnabledTricks = Sym<FnTakeStr>(soh, "SOH_SetEnabledTricks");
    auto SOH_SetAllTricks = Sym<FnVoidV>(soh, "SOH_SetAllTricks");
    auto SOH_SetCheckPrices = Sym<FnTakeStr>(soh, "SOH_SetCheckPrices");
    auto MM_SetCheckPrices = Sym<FnTakeStr>(mm, "MM_SetCheckPrices");
    // #136: combined goal — must be pushed before every dump (it shapes both games' pools).
    auto SOH_SetComboGoal = Sym<void (*)(int, int, int)>(soh, "SOH_SetComboGoal");
    auto MM_SetComboGoal = Sym<void (*)(int, int, int)>(mm, "MM_SetComboGoal");
    auto SOH_SetComboGoalType = Sym<void (*)(int, int, int)>(soh, "SOH_SetComboGoalType");
    auto MM_SetComboGoalType = Sym<void (*)(int, int, int)>(mm, "MM_SetComboGoalType");
    auto SOH_ReadComboGoalTypeCVars = Sym<int (*)(int*, int*)>(soh, "SOH_ReadComboGoalTypeCVars");
    auto SOH_ReadComboGoalCVars = Sym<int (*)(int*, int*)>(soh, "SOH_ReadComboGoalCVars");
    // #135: starting game — also pushed before every dump (it forces OOT's age/forest/exclusions).
    auto SOH_SetComboStartingGame = Sym<void (*)(int)>(soh, "SOH_SetComboStartingGame");
    auto SOH_ReadComboStartingGameCVar = Sym<int (*)(void)>(soh, "SOH_ReadComboStartingGameCVar");
    // Shared Items: also pushed before every dump (shapes OOT's wallet force + MM's trimmed pool).
    auto SOH_SetComboSharedItems = Sym<void (*)(uint32_t)>(soh, "SOH_SetComboSharedItems");
    auto MM_SetComboSharedItems = Sym<void (*)(uint32_t)>(mm, "MM_SetComboSharedItems");
    auto SOH_ReadComboSharedCVars = Sym<uint32_t (*)(void)>(soh, "SOH_ReadComboSharedCVars");
    // Ganon's trials resolved at generation (absent on spoilers made before they were recorded).
    auto SOH_ApplyResolvedSeedRules = Sym<int (*)(const char*)>(soh, "SOH_ApplyProfileResolvedSeedRules");

    ComboRando::OracleFns oot{ Sym<FnOracleVoid>(soh, "Combo_SOH_Rando_Reset"),
                               Sym<FnOracleSetItems>(soh, "Combo_SOH_Rando_SetOwnedItems"),
                               Sym<FnOracleGetChecks>(soh, "Combo_SOH_Rando_GetReachableChecks"),
                               Sym<FnOraclePlaceItem>(soh, "Combo_SOH_Rando_PlaceItem"),
                               Sym<FnOracleGetPortalOpen>(soh, "Combo_SOH_Rando_GetPortalOpen"),
                               Sym<void (*)(uint32_t)>(soh, "SOH_SetSongArrivals"),
                               Sym<uint32_t (*)()>(soh, "SOH_GetSongSources"),
                               Sym<int (*)(int)>(soh, "SOH_SongLogicDestination"),
                               Sym<uint32_t (*)()>(soh, "SOH_GetSongKnowledge"),
                               Sym<void (*)(uint32_t)>(soh, "SOH_SetForeignSongKnowledge") };
    ComboRando::OracleFns mmO{ Sym<FnOracleVoid>(mm, "Combo_MM_Rando_Reset"),
                               Sym<FnOracleSetItems>(mm, "Combo_MM_Rando_SetOwnedItems"),
                               Sym<FnOracleGetChecks>(mm, "Combo_MM_Rando_GetReachableChecks"),
                               Sym<FnOraclePlaceItem>(mm, "Combo_MM_Rando_PlaceItem"),
                               nullptr,
                               Sym<void (*)(uint32_t)>(mm, "MM_SetSongArrivals"),
                               Sym<uint32_t (*)()>(mm, "MM_GetSongSources"),
                               Sym<int (*)(int)>(soh, "SOH_SongLogicDestination"),
                               Sym<uint32_t (*)()>(mm, "MM_GetSongKnowledge"),
                               Sym<void (*)(uint32_t)>(mm, "MM_SetForeignSongKnowledge") };
    mmO.SetExternalBeanRenewal = Sym<int (*)(uint8_t)>(mm, "MM_SetOracleExternalBeanRenewal");
    // Match the app's AttachAdultAgeOracle for both generation and saved-seed playthroughs.
    oot.GetPortalAges = Sym<uint8_t (*)()>(soh, "SOH_GetOraclePortalAges");
    oot.GetSongSourcesForAge = Sym<uint32_t (*)(int)>(soh, "SOH_GetOracleSongSourcesForAge");
    oot.SetSongAgeArrivals = Sym<void (*)(uint32_t, uint32_t)>(soh, "SOH_SetOracleSongAgeArrivals");
    oot.GetStartAges = Sym<uint8_t (*)()>(soh, "SOH_GetOracleStartAges");
    mmO.SetEntryAges = Sym<void (*)(uint8_t)>(mm, "MM_SetOracleEntryAges");
    mmO.GetDefaultEntryAges = Sym<uint8_t (*)()>(mm, "MM_GetOracleDefaultEntryAges");
    mmO.GetSongSourcesForAge = Sym<uint32_t (*)(int)>(mm, "MM_GetOracleSongSourcesForAge");
    mmO.SetSongAgeArrivals = Sym<void (*)(uint32_t, uint32_t)>(mm, "MM_SetOracleSongAgeArrivals");
    mmO.UseCrossAgeOracle = Sym<bool (*)()>(mm, "MM_UseCrossAgeOracle");

    if (!SOH_InitRandoHeadless || !MM_InitRandoHeadless || !SOH_Dump || !MM_Dump || !oot.Reset || !oot.SetOwnedItems ||
        !oot.GetReachableChecks || !oot.PlaceItem || !oot.GetPortalOpen || !mmO.Reset || !mmO.SetOwnedItems ||
        !mmO.GetReachableChecks || !mmO.PlaceItem) {
        std::cerr << "[comborando] missing required DLL exports — rebuild soh.dll / 2ship.dll\n";
        return 2;
    }

    SOH_InitRandoHeadless();
    MM_InitRandoHeadless();
    ComboRando::Entrances::NativeFns entranceNative{
        Sym<int (*)(const char*)>(mm, "MM_ApplyEntranceExpansionForCombo"),
        Sym<int (*)(const char*)>(mm, "MM_SetEntranceExpansionAssumptionsForCombo"),
        Sym<void (*)()>(mm, "MM_ClearEntranceExpansionAssumptionsForCombo"),
        Sym<const char* (*)()>(mm, "MM_DumpEntranceExpansionForCombo")
    };
    auto setOotSongs = Sym<void (*)(const char*)>(soh, "SOH_SetSongTravel");
    auto setMmSongs = Sym<void (*)(const char*)>(mm, "MM_SetSongTravel");
    ComboRando::SongTravelMap generatedSongs;
    auto installSongs = [&](const ComboRando::SongTravelMap& map) {
        auto text = map.json().dump();
        if (setOotSongs)
            setOotSongs(text.c_str());
        if (setMmSongs)
            setMmSongs(text.c_str());
    };

    // ---- Playthrough mode: replay a finished seed and judge beatability. ----
    if (!playthroughFile.empty()) {
        if (!SOH_RestoreSettings || !SOH_PrepContext || !MM_RestoreSettings) {
            std::cerr << "[playthrough] settings-restore exports unavailable — rebuild soh.dll / 2ship.dll\n";
            return 2;
        }
        nlohmann::json spoiler, mmSettings;
        try {
            spoiler = nlohmann::json::parse(ReadFile(playthroughFile));
            ComboRando::RequireSupportedSeedDocument(spoiler);
            mmSettings = MmSettingsForPlaythrough(spoiler);
        } catch (const std::exception& e) {
            std::cerr << "[playthrough] could not read/parse '" << playthroughFile << "': " << e.what() << "\n";
            return 2;
        }
        try {
            const auto catalog = ComboRando::ReadKokiriCatalogRevisions(spoiler.value("oot", nlohmann::json::object()));
            ComboRando::ApplyKokiriSeedRevision(SOH_SetKokiriRevision, catalog.invisibleRupees);
            ComboRando::ApplyKokiriSeedRevision(SOH_SetKokiriGrassRevision, catalog.grass);
        } catch (const std::exception& error) {
            std::cerr << "[playthrough] " << error.what() << "\n";
            return 2;
        }
        try {
            ComboRando::Entrances::ApplyNativeSeed(entranceNative, spoiler);
        } catch (const std::exception& error) {
            std::cerr << "[playthrough] " << error.what() << "\n";
            return 2;
        }
        const auto originalRules = ComboRando::GameplayRulesForSeed(spoiler);
        auto ootSettings = spoiler.value("oot", nlohmann::json::object()).value("settings", nlohmann::json::object());
        // Flat placement map that RunPlaythrough consumes.
        nlohmann::json flat;
        flat["oot"] = spoiler.value("oot", nlohmann::json::object()).value("placements", nlohmann::json::object());
        flat["mm"] = spoiler.value("mm", nlohmann::json::object()).value("placements", nlohmann::json::object());
        flat["foreign"] = spoiler.value("foreign", nlohmann::json::array());
        std::string label = SeedLabel(spoiler);
        uint32_t masterSeed = spoiler.value("masterSeed", 0u);
        const auto songRoutes = spoiler.value("songTravel", nlohmann::json()).dump();
        auto setOotSongs = Sym<void (*)(const char*)>(soh, "SOH_SetSongTravel");
        auto setMmSongs = Sym<void (*)(const char*)>(mm, "MM_SetSongTravel");
        if (setOotSongs && setMmSongs) {
            setOotSongs(songRoutes.c_str());
            setMmSongs(songRoutes.c_str());
        } else if (spoiler.contains("songTravel")) {
            std::cerr << "Missing song-route exports\n";
            return 2;
        }
        // #136: the seed's own goal drives both the traversal win test and the pool shaping below.
        ComboRando::CwGoal goal;
        {
            auto g = spoiler.value("goal", nlohmann::json::object());
            goal = ComboRando::CwGoalFromJson(g); // absent on seeds made before the combined total
        }
        // #135: the seed's starting game. Absent on old spoilers, which all started in OOT.
        const bool mmStart = spoiler.value("startingGame", std::string("OOT")) == "MM";
        // Shared Items: absent on old spoilers -> mask 0 -> feature off.
        const uint32_t sharedMask =
            ComboRando::SharedMaskFromKeys(spoiler.value("sharedItems", nlohmann::json::array()));

        auto ootEntrances = spoiler.value("entrances", nlohmann::json::object()).value("oot", nlohmann::json::array());
        const auto ootSeed = spoiler.value("oot", nlohmann::json::object());
        const bool hasResolvedTrials = ootSeed.contains("resolvedSeedRules");

        // Prices are part of the seed: an unknown price can never be treated as buyable, so a spoiler
        // without them can't be validated honestly. Hard-fail rather than emit an optimistic verdict.
        auto ootPrices = spoiler.value("oot", nlohmann::json::object()).value("prices", nlohmann::json::object());
        auto mmPrices = spoiler.value("mm", nlohmann::json::object()).value("prices", nlohmann::json::object());
        if (ootPrices.empty() || mmPrices.empty() || !SOH_SetCheckPrices || !MM_SetCheckPrices) {
            std::cerr << "[playthrough] spoiler has no shop prices (predates price export) — regenerate the seed "
                         "to validate it\n";
            return 2;
        }

        bool entranceRestoreFailed = false; // set inside runPass; a wrong graph makes any verdict meaningless
        bool nativePreparationFailed = false; // setup failure cannot classify the saved seed
        // One traversal pass. Forces real-logic evaluation (ignore the seed's No-Logic/Glitchless flag).
        // Canonical seed rules are authoritative after native settings restoration. The
        // all-tricks pass is an explicitly reported diagnostic, not a weaker generation policy.
        auto applyPlaythroughRules = [&](bool allTricks) {
            restoreRules(originalRules);
            if (allTricks) {
                if (SOH_SetAllTricks)
                    SOH_SetAllTricks();
                if (writeRule)
                    for (const auto& trick : ComboRando::kMmTricks)
                        writeRule(trick.key, 1);
            }
        };
        auto runPass = [&](bool allTricks, nlohmann::json* ptOut) {
            // Restore the seed's ORIGINAL settings for the dump so the shuffled-shop-slot set matches
            // generation (a No-Logic seed shuffles 8 slots/shop; glitchless computes 7 — GAP-8).
            SOH_RestoreSettings(ootSettings.dump().c_str());
            nlohmann::json ms = mmSettings;
            for (auto it = ms.begin(); it != ms.end(); ++it)
                if (it.key().find("RO_LOGIC") != std::string::npos)
                    it.value() = 0; // MM glitchless — pool shape is logic-independent
            MM_RestoreSettings(ms.dump().c_str());
            applyPlaythroughRules(allTricks);
            if (SOH_SetSeed)
                SOH_SetSeed(masterSeed);
            if (MM_SetSeed)
                MM_SetSeed(masterSeed);
            if (SOH_SetComboGoal)
                SOH_SetComboGoal(goal.hunt ? 1 : 0, goal.required, ComboRando::CwOotPieces(goal.total));
            if (MM_SetComboGoal)
                MM_SetComboGoal(goal.hunt ? 1 : 0, goal.required, ComboRando::CwMmPieces(goal.total));
            if (SOH_SetComboGoalType)
                SOH_SetComboGoalType(ComboRando::CwGoalType(goal), goal.required, ComboRando::CwOotPieces(goal.total));
            if (MM_SetComboGoalType)
                MM_SetComboGoalType(ComboRando::CwGoalType(goal), goal.required, ComboRando::CwMmPieces(goal.total));
            if (SOH_SetComboStartingGame)
                SOH_SetComboStartingGame(mmStart ? 1 : 0);
            if (SOH_SetComboSharedItems)
                SOH_SetComboSharedItems(sharedMask);
            if (MM_SetComboSharedItems)
                MM_SetComboSharedItems(sharedMask);
            // A real in-game save's placement map lists only SHUFFLED checks; non-shuffled items the
            // oracle still gates on (OOT vanilla shop items, MM boss remains when not shuffled) live in
            // the dump's fixed[]. Merge them in so those gates (e.g. MM RemainsCount for the Moon) resolve.
            std::string sohDump = SOH_Dump(), mmDump = MM_Dump();
            if (const auto failure = ComboRando::ParseNativePoolFailure(sohDump, mmDump)) {
                std::cerr << "[playthrough] native preparation failed: " << failure->error << " — no verdict\n";
                nativePreparationFailed = true;
                return ComboRando::PlaythroughResult{};
            }
            // NOW force glitchless for traversal (real gates); the dump above froze the seed's slot
            // set in the DLL-side cache, so this re-prep can't shrink it.
            nlohmann::json os = ootSettings;
            for (auto it = os.begin(); it != os.end(); ++it)
                if (it.key().find("LogicRules") != std::string::npos)
                    it.value() = 0;
            SOH_RestoreSettings(os.dump().c_str());
            applyPlaythroughRules(allTricks);
            SOH_PrepContext();
            // The player's trials are the ones generation resolved, as the in-game reload applies them.
            if (hasResolvedTrials && (!SOH_ApplyResolvedSeedRules ||
                                      !SOH_ApplyResolvedSeedRules(ootSeed.at("resolvedSeedRules").dump().c_str()))) {
                std::cerr << "[playthrough] could not apply the seed's resolved Ganon's trials — no verdict\n";
                nativePreparationFailed = true;
                return ComboRando::PlaythroughResult{};
            }
            // The PrepContext above reset the OOT region graph to VANILLA wiring — not the graph the
            // seed was filled against. Install the seed's recorded entrance layout (empty array =
            // vanilla, still correct). Re-deriving via SOH_ShuffleEntrances is only a fallback for
            // old spoilers without the array: its logic-validated accepts depend on the trick set, so
            // the all-tricks pass could re-derive a DIFFERENT layout than generation.
            if (SOH_ApplyEntranceOverrides) {
                if (!SOH_ApplyEntranceOverrides(ootEntrances.dump().c_str())) {
                    std::cerr << "[playthrough] entrance apply failed — verdict unreliable\n";
                    entranceRestoreFailed = true;
                }
            } else if (!ootEntrances.empty()) {
                std::cerr << "[playthrough] spoiler has entrances but soh.dll lacks the apply export — "
                             "rebuild soh.dll; re-deriving (all-tricks pass may diverge)\n";
                if (!SOH_ShuffleEntrances || !SOH_ShuffleEntrances(masterSeed))
                    entranceRestoreFailed = true;
            }
            try {
                ComboRando::Entrances::ApplyNativeSeed(entranceNative, spoiler);
            } catch (const std::exception& error) {
                std::cerr << "[playthrough] " << error.what() << "\n";
                entranceRestoreFailed = true;
            }
            // Spoiler prices are the seed's truth — they override the dumps' re-rolls in every oracle
            // reset (SOH) / query (MM), so wallet gates evaluate against what the player actually pays.
            SOH_SetCheckPrices(ootPrices.dump().c_str());
            MM_SetCheckPrices(mmPrices.dump().c_str());
            nlohmann::json passFlat = flat;
            auto mergeFixed = [&](const std::string& dump, const char* game) {
                try {
                    for (auto& f : nlohmann::json::parse(dump).value("fixed", nlohmann::json::array())) {
                        std::string chk = f.value("check", ""), item = f.value("item", "");
                        if (!chk.empty() && !item.empty() && !passFlat[game].contains(chk))
                            passFlat[game][chk] = item;
                    }
                } catch (...) {}
            };
            mergeFixed(sohDump, "oot");
            mergeFixed(mmDump, "mm");
            return ComboRando::RunPlaythrough(passFlat.dump(), oot, mmO, label, MM_Restore, ptOut, sohDump, mmDump,
                                              ComboRando::OotAccessFromDump(sohDump) != ComboRando::OotAccess::NO_LOGIC,
                                              /*progressionOnly*/ false, goal, mmStart, sharedMask);
        };

        // Affordability canary: re-check every priced purchase in the walk against the wallets held
        // at that sphere. A violation means the oracle's price wiring regressed to "shops are free".
        // With Child Wallet not shuffled, logic starts at the 99-capacity tier (logic.cpp Reset).
        const int ootBaseWalletTier = ootSettings.value("gRandoSettings.ShuffleChildWallet", 0) == 0 ? 1 : 0;
        auto affordabilityViolations = [&](const nlohmann::json& pt) {
            int ow = 0, mw = 0, bad = 0;
            for (const auto& sph : pt) {
                const int owS = ow, mwS = mw; // wallets from earlier spheres only
                for (const auto& st : sph.value("steps", nlohmann::json::array())) {
                    std::string game = st.value("game", ""), chk = st.value("check", ""), item = st.value("item", "");
                    // Junk is never required, so an unaffordable junk slot must not fail a beatable seed.
                    if (!st.value("advancement", true))
                        continue;
                    if (game == "oot" && ootPrices.contains(chk)) {
                        static const int caps[5] = { 0, 99, 200, 500, 999 };
                        int price = ootPrices[chk].get<int>();
                        if (price > caps[std::min(ootBaseWalletTier + owS, 4)]) {
                            ++bad;
                            std::cerr << "[playthrough] AFFORDABILITY: [OOT] " << chk << " price=" << price << " with "
                                      << owS << " wallet(s) at sphere " << sph.value("sphere", -1) << "\n";
                        }
                    } else if (game == "mm" && mmPrices.contains(chk) &&
                               // ComboShip: friendly (prettified) forms of the MM purchase check names.
                               (chk.find("Shop Item") != std::string::npos ||
                                chk.find("Tingle Map") != std::string::npos ||
                                chk.find("Gorman Milk") != std::string::npos ||
                                chk.find("Milk Bar") != std::string::npos ||
                                chk.find("Curiosity") != std::string::npos)) {
                        int p = mmPrices[chk].get<int>();
                        if (!(p < 100 || (p <= 200 && mwS >= 1) || mwS >= 2)) {
                            ++bad;
                            std::cerr << "[playthrough] AFFORDABILITY: [MM] " << chk << " price=" << p << " with "
                                      << mwS << " wallet(s) at sphere " << sph.value("sphere", -1) << "\n";
                        }
                    }
                    // Both games' wallet normalizes to "Progressive Wallet"; attribute to the home game
                    // unless shared, where a pickup raises both (see Combo_SharedReconcileNow).
                    if (item == "Progressive Wallet") {
                        if (sharedMask & (1u << ComboRando::SF_WALLET)) {
                            ++ow;
                            ++mw;
                        } else {
                            bool fgn = st.value("foreign", false);
                            std::string homeGame = fgn ? (game == "oot" ? "mm" : "oot") : game;
                            if (homeGame == "oot")
                                ++ow;
                            else
                                ++mw;
                        }
                    }
                }
            }
            return bad;
        };
        // Names the blocking win side from reachability at the collected fixed point, so "stuck" is exact.
        // Only a stuck pass asks, and there the goal is not met with everything reached collected.
        auto blocked = [&goal](const ComboRando::PlaythroughResult& r) -> const char* {
            if (r.hunt)
                return "not enough Triforce Pieces are reachable";
            if (ComboRando::CwGoalType(goal) == ComboRando::CwGoal::QUEST)
                return "the named Triforce Pieces are not all reachable";
            return (!r.ganonReachable && !r.majoraReachable) ? "neither Ganon nor Majora is reachable"
                   : !r.ganonReachable                       ? "Ganon (OOT) is unreachable"
                                                             : "Majora (MM) is unreachable";
        };

        // No Logic seeds may be OOT-unbeatable by design; the verdict below tolerates that as long as
        // MM stays fully reachable + Majora beatable.
        bool ootNoLogic = false;
        for (auto it = ootSettings.begin(); it != ootSettings.end(); ++it)
            if (it.key().find("LogicRules") != std::string::npos && it.value().is_number() &&
                it.value().get<int>() == 1)
                ootNoLogic = true;

        // Older spoilers did not record which trials generation picked. When only some (or a random
        // number) are required, the replay re-derives them and may judge a different set.
        const int trialMode = ootSettings.value("gRandoSettings.GanonTrial", 1);
        const int trialCount = ootSettings.value("gRandoSettings.GanonTrialCount", 6);
        if (!hasResolvedTrials && (trialMode == 2 || (trialMode == 1 && trialCount > 0 && trialCount < 6)))
            std::cout << "[playthrough] note: this spoiler predates recorded Ganon's trials; the replay re-derives "
                         "them, so the verdict is approximate\n";

        // Pass 1 — the seed's own settings + enabled tricks: "can this player beat it?"
        const auto trickCsv = originalRules["ootTricks"].get<std::string>();
        const auto ootTrickCount = trickCsv.empty() ? 0 : 1 + std::count(trickCsv.begin(), trickCsv.end(), ',');
        int mmTrickCount = 0;
        for (const auto& trick : ComboRando::kMmTricks)
            mmTrickCount += originalRules["mmTricks"].value(trick.key, 0) != 0;
        std::cout << "[playthrough] seed '" << label << "' — Pass 1 (" << ootTrickCount << " OoT / " << mmTrickCount
                  << " MM enabled trick(s), starts in " << (mmStart ? "MM" : "OOT") << ")\n";
        nlohmann::json pt1 = nlohmann::json::array();
        auto r1 = runPass(false, &pt1);
        if (nativePreparationFailed)
            return 2;
        if (entranceRestoreFailed) {
            std::cerr << "[playthrough] could not restore the seed's entrance layout — no verdict\n";
            return 2;
        }
        if (r1.beatable) {
            if (int bad = affordabilityViolations(pt1)) {
                std::cout << "[playthrough] RESULT: FAIL — beatable but " << bad
                          << " purchase(s) exceed the wallet held at their sphere (price wiring bug).\n";
                return 1;
            }
            std::cout << "[playthrough] RESULT: PASS — seed passed validation, should be beatable (sphere "
                      << r1.beatableSphere << ").\n";
            return 0;
        }
        // Pass 2 — all tricks: separates "needs more tricks than configured" from "impossible".
        std::cout << "[playthrough] Pass 1 stuck (" << blocked(r1) << ") — retrying with ALL tricks\n";
        nlohmann::json pt2 = nlohmann::json::array();
        auto r2 = runPass(true, &pt2);
        if (nativePreparationFailed)
            return 2;
        if (entranceRestoreFailed) {
            std::cerr << "[playthrough] could not restore the seed's entrance layout — no verdict\n";
            return 2;
        }
        if (r2.beatable) {
            std::cout << "[playthrough] RESULT: beatable ONLY with tricks beyond the seed's settings (sphere "
                      << r2.beatableSphere << "). With the seed's tricks, Pass 1 got stuck: " << blocked(r1) << ".\n";
            return 3;
        }
        // No Logic: RunPlaythrough walks to the collected fixed point, so with both bosses as the goal, Ganon
        // and Majora both reachable there already passed as beatable above. Majora-only => tolerate OOT/Ganon
        // (still a valid No-Logic seed, MM win chain intact; MM-advancement reachability is guaranteed by the
        // fill, which retries on mmAdvUnreachable>0 — not gated on unreachableMm==0, ~1 MM junk check is always
        // under-modeled since the oracle evaluates MM with zeroed save options).
        if (ootNoLogic && ComboRando::CwGoalType(goal) == ComboRando::CwGoal::BOTH && r2.majoraReachable) {
            std::cout << "[playthrough] RESULT: PASS (No Logic) — Majora beatable (MM reachable); Ganon (OOT) "
                         "unreachable even with all tricks, tolerated.\n";
            return 0;
        }
        std::cout << "[playthrough] RESULT: FAIL — not beatable even with all tricks; stuck: " << blocked(r2)
                  << " (OOT unreachable=" << r2.unreachableOot << ", MM unreachable=" << r2.unreachableMm
                  << "; see saves/combo/slot0.playthrough.txt).\n";
        return 1;
    }

    // ---- Generate + validate mode. ----
    int kokiriRevision = ComboRando::kKokiriModernRevision;
    int grassRevision = ComboRando::kKokiriGrassModernRevision;
    nlohmann::json savedGoal;
    int savedStartingGame = -1;
    bool hasSavedSharedItems = false;
    uint32_t savedSharedMask = 0;
    // Restore a reported seed's settings so generation matches it; otherwise live CVar defaults apply.
    if (!settingsFile.empty()) {
        if (!SOH_RestoreSettings || !MM_RestoreSettings) {
            std::cerr << "[comborando] --settings needs the settings-restore exports — rebuild soh.dll / 2ship.dll\n";
            return 2;
        }
        try {
            auto sj = nlohmann::json::parse(ReadFile(settingsFile));
            // Standalone per-game settings remain valid; unsupported combo capabilities do not.
            ComboRando::RequireSupportedSeedCapabilities(sj);
            const auto& oot = sj.contains("oot") ? sj["oot"] : sj;
            const auto& mm = sj.contains("mm") ? sj["mm"] : sj;
            // Combined metadata is outside the native per-game settings snapshots.
            if (sj.contains("goal")) {
                const auto& saved = sj.at("goal");
                for (const char* key : { "requiredPieces", "totalPieces" })
                    if (saved.contains(key) && !saved.at(key).is_number_integer())
                        throw std::runtime_error("Saved goal piece counts must be integers");
                const auto parsed = ComboRando::CwGoalFromJson(saved);
                if ((parsed.hunt && (parsed.required < 1 || parsed.required > ComboRando::kMaxComboTriforcePieces)) ||
                    parsed.total < -1 || parsed.total > ComboRando::kMaxComboTriforcePieces ||
                    (parsed.total >= 0 && parsed.total < parsed.required))
                    throw std::runtime_error("Saved goal piece counts are out of range");
                savedGoal = ComboRando::CwGoalJson(parsed);
            }
            if (sj.contains("startingGame")) {
                const auto game = sj.at("startingGame").get<std::string>();
                if (game != "OOT" && game != "MM")
                    throw std::runtime_error("Saved startingGame must be OOT or MM");
                savedStartingGame = game == "MM" ? 1 : 0;
            }
            if (sj.contains("sharedItems")) {
                savedSharedMask = ComboRando::SharedMaskFromKeys(sj.at("sharedItems"));
                hasSavedSharedItems = true;
            }
            // Exact settings reproduction retains the original family (absence = legacy).
            const auto catalog = ComboRando::ReadKokiriCatalogRevisions(oot);
            kokiriRevision = catalog.invisibleRupees;
            grassRevision = catalog.grass;
            if (oot.contains("settings"))
                SOH_RestoreSettings(oot["settings"].dump().c_str());
            if (mm.contains("settings"))
                MM_RestoreSettings(mm["settings"].dump().c_str());
            if (SOH_SetEnabledTricks && oot.contains("enabledTricks"))
                SOH_SetEnabledTricks(oot["enabledTricks"].dump().c_str());
            if (sj.contains("gameplayRules"))
                restoreRules(ComboRando::GameplayRulesForSeed(sj));
            if (sj.contains("entranceExpansion")) {
                ComboRando::Entrances::DecodeMmWallmasterSeed(sj);
                nativeMmWallmasters = true;
            }
            ComboRando::Entrances::ValidateMmMixedSeed(sj);
            if (sj.contains("mixedEntranceExpansion"))
                savedMixedEntrances = sj.at("mixedEntranceExpansion");
            if (sj.contains("grottoEntranceExpansion")) {
                ComboRando::Entrances::MmGrottoLayout proof;
                proof.install(sj.at("grottoEntranceExpansion"));
                savedGrottoEntrances = sj.at("grottoEntranceExpansion");
            }
            if (sj.contains("ordinaryEntranceExpansion")) {
                ComboRando::Entrances::MmOrdinaryLayout proof;
                proof.install(sj.at("ordinaryEntranceExpansion"));
                savedOrdinaryEntrances = sj.at("ordinaryEntranceExpansion");
            }
            if (sj.contains("accessibility") && writeRule)
                writeRule("gCombo.Rando.Reachability",
                          sj["accessibility"].value("mode", std::string()) == "completion");
            std::cout << "[comborando] restored settings from '" << settingsFile << "'\n";
        } catch (const std::exception& e) {
            std::cerr << "[comborando] could not read/parse '" << settingsFile << "': " << e.what() << "\n";
            return 2;
        }
    }
    try {
        ComboRando::ApplyKokiriSeedRevision(SOH_SetKokiriRevision, kokiriRevision);
        ComboRando::ApplyKokiriSeedRevision(SOH_SetKokiriGrassRevision, grassRevision);
    } catch (const std::exception& error) {
        std::cerr << "[comborando] " << error.what() << "\n";
        return 2;
    }
    // #136: goal resolution. Explicit flags win, then saved combined metadata, then menu CVars.
    ComboRando::CwGoal goal;
    if (!savedGoal.is_null()) {
        goal = ComboRando::CwGoalFromJson(savedGoal);
    } else if (triforceRequiredArg < 0 && SOH_ReadComboGoalCVars) {
        int required = 0, total = -1;
        goal.hunt = SOH_ReadComboGoalCVars(&required, &total) != 0;
        goal.type = SOH_ReadComboGoalTypeCVars
                        ? static_cast<ComboRando::CwGoal::Type>(SOH_ReadComboGoalTypeCVars(&required, &total))
                        : (goal.hunt ? ComboRando::CwGoal::HUNT : ComboRando::CwGoal::BOTH);
        goal.hunt = goal.type == ComboRando::CwGoal::HUNT;
        goal.required = goal.hunt ? required : 0;
        goal.total = total;
    }
    if (triforceRequiredArg >= 0) {
        goal.hunt = triforceRequiredArg > 0;
        goal.type = goal.hunt ? ComboRando::CwGoal::HUNT : ComboRando::CwGoal::BOTH;
        goal.required = std::min(triforceRequiredArg, ComboRando::kMaxComboTriforcePieces);
        // Keep a saved pool total when changing only the required count. Without a snapshot,
        // retain the historical CLI default of exactly the required number of pieces.
        if (savedGoal.is_null())
            goal.total = goal.required;
    }
    if (triforceTotalArg >= 0)
        goal.total = std::max(triforceTotalArg, goal.required);
    if (!goalArg.empty()) {
        try {
            auto override = ComboRando::CwGoalFromJson(
                { { "type", goalArg }, { "requiredPieces", goal.required }, { "totalPieces", goal.total } });
            if (override.hunt && override.required < 1)
                override.required = 15;
            if (override.hunt && override.total < override.required)
                override.total = override.required;
            goal = override;
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
            return 2;
        }
    }
    // Same cap the DLLs apply per game, so the recorded totalPieces can't disagree with the pools.
    goal.total = std::clamp(goal.total, goal.required, ComboRando::kMaxComboTriforcePieces);
    // #135: a seed records its resolved starting game, not the menu's Random preference.
    const int startCfg = startingGameArg >= 0            ? startingGameArg
                         : savedStartingGame >= 0        ? savedStartingGame
                         : SOH_ReadComboStartingGameCVar ? SOH_ReadComboStartingGameCVar()
                                                         : 0;
    // Shared Items: --shared-items wins ("all"/"none"/comma-separated keys), then saved keys, then CVars.
    uint32_t sharedMask = 0;
    if (sharedItemsArg.empty() && hasSavedSharedItems) {
        sharedMask = savedSharedMask;
    } else if (sharedItemsArg == "all") {
        sharedMask = ComboRando::SharedMaskAll();
    } else if (sharedItemsArg == "none" || sharedItemsArg.empty()) {
        sharedMask = (sharedItemsArg.empty() && SOH_ReadComboSharedCVars) ? SOH_ReadComboSharedCVars() : 0;
    } else {
        nlohmann::json keys = nlohmann::json::array();
        std::stringstream ss(sharedItemsArg);
        std::string tok;
        while (std::getline(ss, tok, ','))
            keys.push_back(tok);
        sharedMask = ComboRando::SharedMaskFromKeys(keys);
    }
    std::cout << "[comborando] validating " << count << " seed(s) from "
              << (haveMasterSeed ? "masterSeed " + std::to_string(masterSeedArg) : "'" + seed + "'")
              << (goal.hunt ? " (Triforce Hunt, " + std::to_string(goal.required) + " of " +
                                  std::to_string(goal.total) + " pieces)"
                            : std::string(" (both bosses)"))
              << (startCfg == 1   ? ", starting game MM"
                  : startCfg == 2 ? ", starting game random"
                                  : "")
              << "\n";
    int failures = 0;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < count; ++i) {
        const uint32_t base = (haveMasterSeed ? masterSeedArg : ComboHash(seed)) + static_cast<uint32_t>(i);
        uint32_t masterSeed = base;
        std::string sohDump, mmDump, sohHintDump;
        ComboRando::CombinedFillResult r{};
        nlohmann::json entranceExpansion, ordinaryEntranceExpansion, grottoEntranceExpansion, mixedEntranceExpansion;
        ComboRando::RequirednessResult pareDownResult; // cross-hint verification (headless mirror of RunComboFill)
        // Whole-fill retries with re-derived seeds, identical to RunComboFill (GAP-4) so a headless
        // seed reproduces the in-game one even when early attempts fail (shared budget, same header).
        const int kFillAttempts = ComboRando::kFillAttempts;
        bool pinStartOot = false, resolvedMmStart = false; // #135: Random falls back to OOT after a failure
        for (int attempt = 0; attempt < kFillAttempts; ++attempt) {
            masterSeed = base + attempt * 0x9E3779B9u;
            try {
                ComboRando::Entrances::ApplyNativeSeed(entranceNative, nlohmann::json::object());
            } catch (const std::exception& error) {
                std::cerr << error.what() << "\n";
                return 2;
            }
            auto read = [&](const char* name) { return readRule && readRule(name, 0) != 0; };
            generatedSongs = ComboRando::SongTravelMap::generateForRules(
                masterSeed, read("gCombo.Enhancements.SongTravel"), read("gCombo.Enhancements.ForeignSongs"),
                read("gCombo.Rando.SongTravel"), read("gCombo.Rando.ForeignSongs"));
            installSongs(generatedSongs);
            const bool mmStart = !pinStartOot && !(startCfg == 2 && attempt + 1 == kFillAttempts) &&
                                 ResolveStartingGameMM(startCfg, masterSeed);
            if (SOH_SetSeed)
                SOH_SetSeed(masterSeed);
            if (MM_SetSeed)
                MM_SetSeed(masterSeed);
            // Before the dumps: the goal shapes OOT's wincon placement and MM's hunt pool.
            if (SOH_SetComboGoal)
                SOH_SetComboGoal(goal.hunt ? 1 : 0, goal.required, ComboRando::CwOotPieces(goal.total));
            if (MM_SetComboGoal)
                MM_SetComboGoal(goal.hunt ? 1 : 0, goal.required, ComboRando::CwMmPieces(goal.total));
            if (SOH_SetComboGoalType)
                SOH_SetComboGoalType(ComboRando::CwGoalType(goal), goal.required, ComboRando::CwOotPieces(goal.total));
            if (MM_SetComboGoalType)
                MM_SetComboGoalType(ComboRando::CwGoalType(goal), goal.required, ComboRando::CwMmPieces(goal.total));
            // Same reason (#135): an MM start forces OOT's age/forest/exclusions.
            if (SOH_SetComboStartingGame)
                SOH_SetComboStartingGame(mmStart ? 1 : 0);
            // Shared Items: shapes OOT's wallet-force (settings.cpp reads gComboSharedMask) and MM's shared
            // ocarina/spell pool (GeneratePools reads gMMComboSharedMask) before the dumps, as RunComboFill.
            if (SOH_SetComboSharedItems)
                SOH_SetComboSharedItems(sharedMask);
            if (MM_SetComboSharedItems)
                MM_SetComboSharedItems(sharedMask);
            ComboRando::SynchronizeGenerationGuarantee(
                [&](const char* key, int fallback) { return readRule ? readRule(key, fallback) : fallback; },
                [&](const char* key, int value) {
                    if (writeRule)
                        writeRule(key, value);
                });
            if (readRule && writeRule)
                writeRule("gRandoSettings.CompleteMaskQuest", ComboRando::NormalizeOotMaskQuestForGeneration(
                                                                  readRule("gRandoSettings.CompleteMaskQuest", 2)));
            if (readRule && writeRule)
                ComboRando::ApplyOotEnemySoulsAvailability(readRule, writeRule);
            sohDump = SOH_Dump();
            mmDump = MM_Dump();
            if (const auto failure = ComboRando::ParseNativePoolFailure(sohDump, mmDump)) {
                std::cerr << "[comborando] seed " << masterSeed << ": " << failure->error << "\n";
                if (!failure->retryable)
                    return 2;
                if (mmStart && startCfg == 2)
                    pinStartOot = true;
                continue;
            }
            if (goal.hunt) {
                const int pieces = ComboRando::CountPoolTriforcePieces(sohDump, mmDump);
                if (pieces < goal.required) {
                    std::cerr << "[comborando] Triforce Hunt needs " << goal.required << " pieces but only " << pieces
                              << " are in the combined pool — raise --triforce-total / the Combo pool total\n";
                    return 2;
                }
            }
            std::string forced = SOH_GetForced ? SOH_GetForced(masterSeed) : "";
            // Same placement as RunComboFill: after the dump/forced read, before the fill, so logic
            // validates the shuffled world. No-op when the entrance options are off.
            if (SOH_ShuffleEntrances && !SOH_ShuffleEntrances(masterSeed)) {
                std::cerr << "[comborando] seed " << masterSeed << ": entrance shuffle found no valid layout\n";
                if (mmStart && startCfg == 2)
                    pinStartOot = true;
                continue;
            }
            ComboRando::OotAccess ootAccess = ComboRando::OotAccessFromDump(sohDump);
            if (ootAccess != ComboRando::OotAccess::NO_LOGIC && readRule)
                ootAccess = readRule("gCombo.Rando.Reachability", 0) ? ComboRando::OotAccess::BEATABLE_ONLY
                                                                     : ComboRando::OotAccess::ALL_REACHABLE;
            try {
                entranceExpansion = ComboRando::Entrances::GenerateNativeMmWallmasters(
                    nativeMmWallmasters, masterSeed, entranceNative, sohDump, mmDump, forced, oot, mmO, mmStart, goal);
            } catch (const std::exception& error) {
                std::cerr << "[comborando] MM Wallmaster generation: " << error.what() << "\n";
                return 2;
            }
            if (nativeMmMixed || !savedMixedEntrances.is_null()) {
                try {
                    mixedEntranceExpansion = savedMixedEntrances;
                    if (mixedEntranceExpansion.is_null())
                        mixedEntranceExpansion = ComboRando::Entrances::GenerateNativeMmMixed(
                            ComboRando::Entrances::MmMixedGenerationSettings(
                                ComboRando::Entrances::MmOrdinaryGenerationSettings(
                                    nativeMmOrdinaryIndoors, nativeMmOrdinaryTelescopes, nativeMmOrdinaryOverworld,
                                    nativeMmOrdinaryDungeons),
                                nativeMmGrottos),
                            masterSeed, entranceNative, sohDump, mmDump, forced, oot, mmO, mmStart, goal);
                    ComboRando::Entrances::ApplyNativeMmMixed(entranceNative, mixedEntranceExpansion);
                } catch (const std::exception& error) {
                    std::cerr << "[comborando] MM mixed generation: " << error.what() << "\n";
                    return 2;
                }
            } else {
                try {
                    ordinaryEntranceExpansion = savedOrdinaryEntrances;
                    if (ordinaryEntranceExpansion.is_null() &&
                        (nativeMmOrdinaryIndoors || nativeMmOrdinaryOverworld || nativeMmOrdinaryDungeons)) {
                        const auto settings = ComboRando::Entrances::MmOrdinaryGenerationSettings(
                            nativeMmOrdinaryIndoors, nativeMmOrdinaryTelescopes, nativeMmOrdinaryOverworld,
                            nativeMmOrdinaryDungeons);
                        ordinaryEntranceExpansion = ComboRando::Entrances::GenerateNativeMmOrdinary(
                            settings, masterSeed, entranceNative, sohDump, mmDump, forced, oot, mmO, mmStart, goal);
                    }
                    if (!ordinaryEntranceExpansion.is_null())
                        ComboRando::Entrances::ApplyNativeMmOrdinary(entranceNative, ordinaryEntranceExpansion);
                } catch (const std::exception& error) {
                    std::cerr << "[comborando] MM ordinary generation: " << error.what() << "\n";
                    return 2;
                }
                try {
                    grottoEntranceExpansion = savedGrottoEntrances;
                    if (grottoEntranceExpansion.is_null() && nativeMmGrottos) {
                        ComboRando::Entrances::Settings settings;
                        settings.games = ComboRando::Entrances::Games::Mm;
                        settings.erGrottos = ComboRando::Entrances::Mode::OwnGame;
                        grottoEntranceExpansion = ComboRando::Entrances::GenerateNativeMmGrottos(
                            settings, masterSeed, entranceNative, sohDump, mmDump, forced, oot, mmO, mmStart, goal);
                    }
                    if (!grottoEntranceExpansion.is_null())
                        ComboRando::Entrances::ApplyNativeMmGrottos(entranceNative, grottoEntranceExpansion);
                } catch (const std::exception& error) {
                    std::cerr << "[comborando] MM grotto generation: " << error.what() << "\n";
                    return 2;
                }
            }
            r = ComboRando::CrossWorldCombinedFill(sohDump, mmDump, masterSeed, oot, mmO, nullptr, forced, ootAccess,
                                                   goal, mmStart ? ComboRando::GAME_MM : ComboRando::GAME_OOT,
                                                   sharedMask);
            if (r.success) {
                resolvedMmStart = mmStart;
                // Cross-hint data (Phase 2/3 mirror of RunComboFill, incl. the same area maps so the
                // WotH/Foolish rollup matches in-game): needs this attempt's still-live oracle session.
                sohHintDump = SOH_DumpRandoHintData ? SOH_DumpRandoHintData() : "";
                std::unordered_map<std::string, std::string> ootAreas, mmAreas;
                try {
                    auto hd = nlohmann::json::parse(sohHintDump.empty() ? "{}" : sohHintDump);
                    for (auto& c : hd.value("checks", nlohmann::json::array())) {
                        std::string name = c.value("name", ""), area = c.value("area", "");
                        if (!name.empty() && !area.empty())
                            ootAreas.emplace(std::move(name), std::move(area));
                    }
                    auto d = nlohmann::json::parse(mmDump);
                    const auto locHints = d.value("locationHints", nlohmann::json::object());
                    for (auto& [chk, region] : locHints.items())
                        mmAreas.emplace(chk, region.get<std::string>());
                } catch (...) {}
                const bool noLogic = ootAccess == ComboRando::OotAccess::NO_LOGIC;
                // A hunt under NO_LOGIC has no meaningful requiredness (see RunComboFill) — skip it.
                if (goal.hunt && noLogic)
                    std::cout << "[comborando]   pare-down skipped (No Logic + Triforce Hunt)\n";
                else if (ComboRando::NeedsRequirednessPareDown(sohHintDump, mmDump))
                    // NO_LOGIC: gate requiredness on MM only (OOT may be unbeatable by design).
                    pareDownResult = ComboRando::PareDownPlaythrough(
                        r.spoilerJson, oot, mmO, nullptr, sohDump, mmDump, ootAreas, mmAreas,
                        (noLogic && ComboRando::CwGoalType(goal) == ComboRando::CwGoal::BOTH)
                            ? ComboRando::GoalPredicate(ComboRando::MmOnlyMajoraGoal)
                            : ComboRando::MakeCombinedGoal(goal),
                        !noLogic, mmStart, nullptr, sharedMask);
                else
                    std::cout << "[comborando]   pare-down skipped (no enabled hint surface needs requiredness)\n";
            }
            if (MM_Restore)
                MM_Restore();
            if (r.success)
                break;
            if (mmStart && startCfg == 2)
                pinStartOot = true;
            std::cerr << "[comborando]   attempt " << (attempt + 1) << "/" << kFillAttempts << " failed: " << r.error
                      << "\n";
        }
        std::string tag = "'" + seed + "'" + (count > 1 ? ("+" + std::to_string(i)) : "");
        if (r.success) {
            size_t foreign = 0;
            try {
                foreign = nlohmann::json::parse(r.spoilerJson).value("foreign", nlohmann::json::array()).size();
            } catch (...) {}
            std::cout << "[comborando]   seed " << tag << " (" << masterSeed << ") PASS (" << foreign
                      << " cross-game placements)\n";
            // For a single-seed run, emit a consolidated spoiler (settings + placements) so it can be fed
            // straight back to --playthrough. Mirrors ComboShip.cpp's consolidated shape.
            if (count == 1 && SOH_DumpSettings && MM_DumpSettings) {
                try {
                    auto fillSpoiler = nlohmann::json::parse(r.spoilerJson);
                    auto pricesOf = [](const std::string& dump) {
                        return nlohmann::json::parse(dump).value("prices", nlohmann::json::object());
                    };
                    // OOT's curated ice-trap disguise set (parity with RunComboFill's writer).
                    auto iceTrapModelsOf = [](const std::string& dump) {
                        return nlohmann::json::parse(dump).value("iceTrapModels", nlohmann::json::array());
                    };
                    nlohmann::json consolidated;
                    consolidated["seed"] = seed;
                    consolidated["masterSeed"] = masterSeed;
                    consolidated["gameplayRules"] = ComboRando::CaptureGameplayRules(
                        [&](const char* key, int fallback) { return readRule ? readRule(key, fallback) : fallback; });
                    auto getCsv = Sym<FnDump>(soh, "SOH_GetComboTricksCsv");
                    consolidated["gameplayRules"]["ootTricks"] = getCsv ? getCsv() : "";
                    consolidated["songTravel"] = generatedSongs.json();
                    consolidated["faroreRules"] =
                        ComboRando::FaroreRulesFromMmDump(nlohmann::json::parse(mmDump)).json();
                    consolidated["spellRules"] = ComboRando::SpellRulesFromMmDump(nlohmann::json::parse(mmDump)).json();
                    consolidated["gearRules"] = ComboRando::GearRulesFromMmDump(nlohmann::json::parse(mmDump)).json();
                    if (!entranceExpansion.is_null())
                        consolidated["entranceExpansion"] = entranceExpansion;
                    if (!ordinaryEntranceExpansion.is_null())
                        consolidated["ordinaryEntranceExpansion"] = ordinaryEntranceExpansion;
                    if (!grottoEntranceExpansion.is_null())
                        consolidated["grottoEntranceExpansion"] = grottoEntranceExpansion;
                    if (!mixedEntranceExpansion.is_null())
                        consolidated["mixedEntranceExpansion"] = mixedEntranceExpansion;
                    consolidated["accessibility"] = {
                        { "version", 1 },
                        { "mode",
                          ComboRando::OotAccessFromDump(sohDump) == ComboRando::OotAccess::NO_LOGIC
                              ? "noLogic"
                              : (readRule && readRule("gCombo.Rando.Reachability", 0) ? "completion" : "everyCheck") }
                    };
                    consolidated["goal"] = ComboRando::CwGoalJson(goal);
                    consolidated["startingGame"] = resolvedMmStart ? "MM" : "OOT"; // #135
                    // Shared Items: the EFFECTIVE mask (families actually trimmed this fill), not the
                    // requested one — a family with no OOT copies was left alone (see CrossWorldRando.h).
                    nlohmann::json sharedItemsJson = fillSpoiler.value("sharedItems", nlohmann::json::array());
                    const uint32_t effectiveSharedMask = ComboRando::SharedMaskFromKeys(sharedItemsJson);
                    consolidated["sharedItems"] = sharedItemsJson;
                    // ComboShip: suffix cross-game item-name collisions in the placements (parity with
                    // RunComboFill's consolidated writer) so the headless spoiler shows "(OOT)"/"(MM)".
                    nlohmann::json ootPl = fillSpoiler.value("oot", nlohmann::json::object());
                    nlohmann::json mmPl = fillSpoiler.value("mm", nlohmann::json::object());
                    ComboRando::SuffixCrossGameItems(ootPl, mmPl, fillSpoiler.value("foreign", nlohmann::json::array()),
                                                     sohDump, mmDump,
                                                     ComboRando::SharedUntaggedNames(effectiveSharedMask));
                    consolidated["oot"] = { { "settings", nlohmann::json::parse(SOH_DumpSettings()) },
                                            { "enabledTricks", SOH_DumpEnabledTricks
                                                                   ? nlohmann::json::parse(SOH_DumpEnabledTricks())
                                                                   : nlohmann::json::array() },
                                            { "placements", ootPl },
                                            { "prices", pricesOf(sohDump) },
                                            { "iceTrapModels", iceTrapModelsOf(sohDump) } };
                    consolidated["oot"][ComboRando::kKokiriSeedRevision] = kokiriRevision;
                    consolidated["oot"][ComboRando::kKokiriGrassCatalogRevision] = grassRevision;
                    consolidated["mm"] = { { "settings", nlohmann::json::parse(MM_DumpSettings()) },
                                           { "placements", mmPl },
                                           { "prices", pricesOf(mmDump) } };
                    // ComboShip: enrich the foreign array exactly like RunComboFill (displayName game
                    // suffix) so the headless consolidated file matches the in-game one.
                    auto foreignArr = fillSpoiler.value("foreign", nlohmann::json::array());
                    ComboRando::AssignTrapDisguises(foreignArr, fillSpoiler.value("oot", nlohmann::json::object()),
                                                    fillSpoiler.value("mm", nlohmann::json::object()), sohDump, mmDump,
                                                    masterSeed);
                    consolidated["foreign"] = ComboRando::BuildForeignArray(foreignArr, effectiveSharedMask);
                    // OOT entrance layout (parity with ComboShip.cpp's consolidated writer) — this is
                    // what --playthrough installs into the region graph before walking.
                    {
                        nlohmann::json ootEnt = nlohmann::json::array();
                        if (SOH_DumpEntranceOverrides) {
                            try {
                                ootEnt = nlohmann::json::parse(SOH_DumpEntranceOverrides());
                            } catch (...) {}
                        }
                        consolidated["entrances"] = { { "oot", std::move(ootEnt) } };
                    }
                    // Cross-hint generation (mirrors RunComboFill) — enables the headless determinism check.
                    consolidated["hints"] =
                        ComboRando::Generate(masterSeed, sohDump, sohHintDump, mmDump, consolidated["foreign"],
                                             r.spoilerJson, pareDownResult);
                    std::error_code ec;
                    std::filesystem::create_directories("saves/combo", ec);
                    std::ofstream f("saves/combo/comborando.spoiler.json", std::ios::trunc);
                    ComboRando::StampComboSeedFormat(consolidated);
                    f << consolidated.dump(2);
                    std::cout << "[comborando]   consolidated spoiler -> saves/combo/comborando.spoiler.json\n";
                } catch (const std::exception& e) {
                    std::cerr << "[comborando]   spoiler write failed: " << e.what() << "\n";
                }
            }
        } else {
            std::cerr << "[comborando]   seed " << tag << " (" << masterSeed << ") FAIL: " << r.error << "\n";
            ++failures;
        }
    }
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    std::cout << "[comborando] RESULT: " << (failures ? "FAIL" : "PASS") << " — " << (count - failures) << "/" << count
              << " completable, " << ms << " ms\n";
    return failures == 0 ? 0 : 1;
}
