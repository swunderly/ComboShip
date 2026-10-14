// Home's community presets follow OoTMM v32.3's presets of the same name for every option Combo Ship supports
// (references/ootmm-stable-32.3: packages/generator/lib/combo/presets.ts, settings in packages/core/src/settings/
// data.ts). OoTMM builds each preset on DEFAULT_SETTINGS (makeSettings), so Default pins the OoTMM defaults Combo Ship
// must write because its own default differs, and Beginner and Allsanity pin their own changes, by presets.ts line.
// The junked checks are also checked against the native check tables (argument: the repository root).
#include "rando/ComboPresets.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace ComboRando;
using Values = std::map<std::string, int>;
using Expected = std::vector<std::pair<const char*, int>>;

static void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}
static const char* Name(CommunityPreset preset) {
    for (const auto& info : kCommunityPresets)
        if (info.id == preset)
            return info.name;
    return "?";
}
static Values Options(CommunityPreset preset) {
    Values values;
    for (const auto& option : CommunityPresetOptions(preset))
        Check(values.emplace(option.key, option.value).second,
              std::string(Name(preset)) + " writes " + option.key + " twice");
    return values;
}
static Values Rules(CommunityPreset preset) {
    Values values;
    for (const auto& rule : CommunityPresetRules(preset))
        Check(values.emplace(rule.key, rule.value).second,
              std::string(Name(preset)) + " sets rule " + rule.key + " twice");
    return values;
}
static void Expect(CommunityPreset preset, const Values& values, const Expected& expected) {
    for (const auto& [key, value] : expected) {
        const auto it = values.find(key);
        Check(it != values.end() && it->second == value,
              std::string(Name(preset)) + ": " + key + " should be " + std::to_string(value) + ", is " +
                  (it == values.end() ? std::string("not written") : std::to_string(it->second)));
    }
}
static void Absent(CommunityPreset preset, const Values& values, const std::vector<const char*>& keys) {
    for (const char* key : keys)
        Check(!values.count(key), std::string(Name(preset)) + " must not write " + key);
}
static std::string Read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    Check(file.good(), "cannot read " + path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

// OoTMM DEFAULT_SETTINGS that none of the three presets changes, written where Combo Ship's own default differs.
static const Expected kOotmmDefaults = {
    { "gRandoSettings.LogicRules", 0 },                   // glitchless
    { "gRando.Options.RO_LOGIC", 0 },                     //
    { "gCombo.Rando.Goal", 0 },                           // goal both: Ganon & Majora
    { "gCombo.Rando.TriforceHunt", 0 },                   //
    { "gCombo.Rando.TriforceRequired", 20 },              // triforceGoal, data.ts:91
    { "gCombo.Rando.TriforceTotal", 30 },                 // triforcePieces, data.ts:101
    { "gRandoSettings.ShuffleOcarinas", 1 },              // shuffleOcarinasOot, data.ts:984
    { "gRandoSettings.ShuffleMasterSword", 1 },           // shuffleMasterSword, data.ts:992
    { "gRandoSettings.ShuffleKokiriSword", 1 },           // an ordinary location (pool_oot.csv:13)
    { "gRandoSettings.ShuffleAdultTrade", 1 },            // ordinary NPC locations
    { "gRandoSettings.RainbowBridge", 3 },                // medallions, data.ts:1352 (also presets.ts:527,981)
    { "gRandoSettings.GanonTrial", 0 },                   // none, data.ts:1112
    { "gRando.Options.RO_CLOCK_SHUFFLE_PROGRESSIVE", 1 }, // progressiveClocks ascending, data.ts:1803
    { "gRando.Options.RO_HINTS_GOSSIP_STONES", 1 },       // hints on MM's gossip stones too
};
// Combo Ship options with no OoTMM setting, or with one OoTMM leaves off, that no preset may switch on.
static const std::vector<const char*> kNeverWritten = {
    "gRando.Options.RO_SHUFFLE_FROGS",         // only the Frog Choir HP is a location (pool_mm.csv:944)
    "gRando.Options.RO_SHUFFLE_ENEMY_DROPS",   // no enemy-drop setting
    "gRando.Options.RO_STARTING_CONSUMABLES",  // sticksNutsUpgradesMm/sharedNutsSticks off in Beginner
    "gRando.Options.RO_HINTS_BOSS_REMAINS",    // 2Ship's static hints
    "gRando.Options.RO_HINTS_TRANSFORMATIONS", //
    "gRandoSettings.ShuffleNpcSouls",          // OoT NPC, animal and misc souls are not released
    "gRandoSettings.ShuffleAnimalSouls",       //
    "gRandoSettings.ShuffleMiscSouls",         //
};

int main(int argc, char** argv) {
    Check(argc == 2, "usage: community_preset_ootmm_tests <repository root>");
    const std::string root = argv[1];
    std::set<std::string> names;
    for (const auto& info : kCommunityPresets) {
        Check(names.insert(info.name).second, "two presets share a name");
        const CommunityPreset preset = info.id;
        const auto values = Options(preset);
        const auto rules = Rules(preset);
        Absent(preset, values, kNeverWritten);
        for (const auto& [key, value] : values) {
            // Entrance settings are not supported yet (OoTMM er*), nor the hidden Shared Ocarina.
            Check(key.find("Entrance") == std::string::npos,
                  std::string(info.name) + " writes the entrance setting " + key);
            Check(key != SharedFamilyByIndex(SF_OCARINA).cvar, "a preset writes the hidden Shared Ocarina");
            Check(value >= 0, key + " is negative");
        }
        Expect(preset, values, kOotmmDefaults);
        // A shared family on in a preset must work: its MM item, ocarina buttons, masks or bombchu bag are in place
        // (SharedItems.h; OoTMM resets a shared item whose cond fails, util.ts:79-91).
        for (int i = 0; i < SF_COUNT; ++i) {
            const auto& family = SharedFamilyByIndex(i);
            if (i == SF_OCARINA || !family.publicOption)
                continue;
            Check(values.count(family.cvar), std::string(info.name) + " leaves " + family.cvar + " unset");
            if (!values.at(family.cvar))
                continue;
            if (const char* extension = SharedFamilyMmExtension(i))
                Expect(preset, values, { { (std::string("gRando.Options.") + extension).c_str(), 1 } });
            if (family.requiresButtons)
                Expect(preset, values,
                       { { "gRandoSettings.ShuffleOcarinaButtons", 1 },
                         { "gRando.Options.RO_SHUFFLE_OCARINA_BUTTONS", 1 } });
            if (family.isMask)
                Expect(preset, values, { { "gRandoSettings.CompleteMaskQuest", 2 } });
            if (i == SF_BOMBCHU_BAG)
                Check(values.count("gRandoSettings.BombchuBag") && values.at("gRandoSettings.BombchuBag") != 0,
                      std::string(info.name) + " shares Bombchu Bags without one");
        }
        for (const auto& [name, value] : rules) {
            const auto* rule = FindGameplayRule(name.c_str());
            Check(rule && rule->name == name && !rule->runtimeOnly && !rule->trickGoverned && value > 0 &&
                      value <= rule->maximum,
                  name + " is not a gameplay rule a preset can include in logic");
        }
        // The catalog export carries the same writes, rules and values.
        const auto json = CommunityPresetJson(info);
        Check(json.at("writes").size() == values.size() && json.at("rules").size() == rules.size() &&
                  json.at("values").at(SharedFamilyByIndex(SF_OCARINA).cvar) == 0,
              std::string(info.name) + "'s catalog entry differs from its lists");
        for (const auto& [key, value] : values)
            Check(json.at("writes").at(key) == value, key + " exported with another value");
        const auto& junk = json.at("values");
        Check(junk.at("gRandoSettings.ExcludedLocations").is_string() && junk.at("gRando.ExcludedChecks").is_array(),
              "junked checks are exported in the wrong form");
        if (preset != CommunityPreset::Beginner)
            Check(junk.at("gRandoSettings.ExcludedLocations") == "" && junk.at("gRando.ExcludedChecks").empty(),
                  std::string(info.name) + " junks checks; only OoTMM's Beginner has junkLocations");
    }

    // Default is OoTMM's DEFAULT_SETTINGS (presets.ts:7): nothing shared and no gameplay rules.
    const auto defaults = Options(CommunityPreset::Default);
    Expect(CommunityPreset::Default, defaults,
           {
               { "gCombo.Rando.Reachability", 0 },                // logic all locations
               { "gRandoSettings.ShuffleScrubs", 1 },             // one-time scrubs stay shuffled (fixer.ts:73-76)
               { "gRandoSettings.ShuffleGanonBossKey", 2 },       // removed, data.ts:484
               { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 1 },    // bossKeyShuffleMm ownDungeon, data.ts:350
               { "gRandoSettings.ShuffleGerudoToken", 1 },        // shuffleGerudoCard, data.ts:1000
               { "gRandoSettings.ShuffleZeldasLetter", 1 },       // an ordinary location (pool_oot.csv:123)
               { "gRandoSettings.ShuffleWeirdEgg", 1 },           // Malon's check is shuffled (pool_oot.csv:122)
               { "gRandoSettings.ShuffleMerchants", 1 },          // Bean Seller shuffled, Granny and Carpet pinned
               { "gRandoSettings.DoorOfTime", 1 },                // closed: Song of Time only, data.ts:1169
               { "gRandoSettings.ClosedForest", 2 },              // dekuTree open, data.ts:1194
               { "gRandoSettings.EnableBombchuDrops", 0 },        // bombchuBehaviorOot free, data.ts:1972
               { "gRandoSettings.IceTrapPercent", 5 },            // trapIce, trapsQuantity small
               { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 1 },   // smallKeyShuffleMm ownDungeon, data.ts:296
               { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 1 } // strayFairyChestShuffle ownDungeon, data.ts:455
           });
    Absent(CommunityPreset::Default, defaults,
           { "gRandoSettings.StartingAge", "gRandoSettings.MQDungeons", "gRandoSettings.BombchuBag",
             "gRandoSettings.CompleteMaskQuest", "gRando.Options.RO_HINTS_GOSSIP_STONE_STRENGTH" });
    for (int i = 0; i < SF_COUNT; ++i)
        if (i != SF_OCARINA)
            Expect(CommunityPreset::Default, defaults, { { SharedFamilyByIndex(i).cvar, 0 } });
    Check(Rules(CommunityPreset::Default).empty(), "Default includes a gameplay rule in logic");

    // Beginner, presets.ts:962-1072.
    const auto beginner = Options(CommunityPreset::Beginner);
    Expect(
        CommunityPreset::Beginner, beginner,
        { { "gCombo.Rando.Reachability", 0 },
          { "gRando.Options.RO_STARTING_MAPS_AND_COMPASSES", 1 }, // tingleShuffle starting, presets.ts:969
          { "gRandoSettings.StartingMapsCompasses", 0 },          // mapCompassShuffle starting, presets.ts:970
          { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 2 },        // smallKeyShuffleMm removed, presets.ts:971
          { "gRandoSettings.GerudoKeys", 0 },                     // smallKeyShuffleHideout vanilla, presets.ts:972
          { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 2 },     // strayFairyChestShuffle starting, presets.ts:973
          { "gRandoSettings.ShuffleGerudoToken", 0 },             // shuffleGerudoCard false, presets.ts:974
          { "gRandoSettings.StartingAge", 2 },                    // random, presets.ts:975
          { "gRandoSettings.DoorOfTime", 2 },                     // open, presets.ts:976
          { "gRandoSettings.ClosedForest", 1 },                   // dekuTree closed, presets.ts:977
          { "gRandoSettings.StartingZeldasLetter", 1 },           // kakarikoGate open, skipZelda, presets.ts:978,980
          { "gRandoSettings.ShuffleZeldasLetter", 0 },            // the open gate removes the letter
          { "gRandoSettings.ShuffleWeirdEgg", 2 },                // skipZelda wakes Talon
          { "gRandoSettings.FortressCarpenters", 1 },             // gerudoFortress single, presets.ts:979
          { "gRandoSettings.SkipScarecrowsSong", 1 },             // freeScarecrowOot, presets.ts:983
          { "gRandoSettings.BombchuBag", 2 },                     // bombchuBehaviorOot bagSeparate, presets.ts:987
          { "gRandoSettings.EnableBombchuDrops", 1 },             // bags add bombchu drops
          { "gRandoSettings.ShuffleMerchants", 0 },               // Bean Seller plando'd to its bean, 1056
          { "gRandoSettings.ShuffleScrubs", 1 },                  // OoTMM's default scrubs
          { "gRandoSettings.ShuffleGanonBossKey", 2 },            // ganonBossKey removed
          { "gRandoSettings.IceTrapPercent", 5 },                 //
          { "gRandoSettings.StartingOcarina", 2 }, // OOT_OCARINA and the plando'd Zelda's Letter, presets.ts:1029,1057
          { "gRandoSettings.StartingPreludeOfLight", 1 }, // plando'd Zelda's Song, presets.ts:1058
          { "gRandoSettings.StartingHylianShield", 1 },   // SHARED_SHIELD_HYLIAN, presets.ts:1027
          { "gRandoSettings.StartingDekuShield", 1 },     // presets.ts:1025
          { "gRandoSettings.StartingSticks", 1 },         // presets.ts:1026
          { "gRandoSettings.StartingNuts", 1 },           // presets.ts:1024
          { "gRandoSettings.HintDistribution", 3 },       // hints, presets.ts:1062-1072
          { "gRando.Options.RO_HINTS_GOSSIP_STONE_STRENGTH", 75 },
          { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 1 } });
    // Exactly the shared items Beginner turns on (presets.ts:990-1008) that Combo Ship has; the rest stay separate.
    const std::set<std::string> beginnerShared = {
        "gCombo.Rando.Shared.Bows",       "gCombo.Rando.Shared.BombBags",    "gCombo.Rando.Shared.BombchuBags",
        "gCombo.Rando.Shared.Magic",      "gCombo.Rando.Shared.Wallets",     "gCombo.Rando.Shared.Hookshot",
        "gCombo.Rando.Shared.FireArrows", "gCombo.Rando.Shared.IceArrows",   "gCombo.Rando.Shared.LightArrows",
        "gCombo.Rando.Shared.Lens",       "gCombo.Rando.Shared.EponasSong",  "gCombo.Rando.Shared.SongOfStorms",
        "gCombo.Rando.Shared.GoronMask",  "gCombo.Rando.Shared.ZoraMask",    "gCombo.Rando.Shared.KeatonMask",
        "gCombo.Rando.Shared.BunnyHood",  "gCombo.Rando.Shared.MaskOfTruth", "gCombo.Rando.Shared.MirrorShield",
    };
    for (int i = 0; i < SF_COUNT; ++i)
        if (i != SF_OCARINA)
            Expect(CommunityPreset::Beginner, beginner,
                   { { SharedFamilyByIndex(i).cvar, beginnerShared.count(SharedFamilyByIndex(i).cvar) ? 1 : 0 } });
    Check(Rules(CommunityPreset::Beginner) == Values({ { "gCombo.Enhancements.ForeignSongs", 1 },
                                                       { "gEnhancements.OpenAllHours", 1 }, // openMaskShop, 985
                                                       { "gEnhancements.Playback.SkipScarecrowSong", 1 } }), // 984
          "Beginner's gameplay rules differ from OoTMM's");

    // Allsanity, presets.ts:405-698.
    const auto allsanity = Options(CommunityPreset::Allsanity);
    Expect(CommunityPreset::Allsanity, allsanity,
           { { "gRandoSettings.IncludeTycoonWallet", 1 },      // colossalWallets, presets.ts:410
             { "gRando.Options.RO_SHUFFLE_TYCOON_WALLET", 1 }, //
             { "gRandoSettings.SoulsEnemyOot", 1 },            // presets.ts:413
             { "gRandoSettings.ShuffleBossSouls", 1 },         // presets.ts:415
             { "gRando.Options.RO_SHUFFLE_ENEMY_SOULS", 1 },   // presets.ts:414
             { "gRando.Options.RO_SHUFFLE_BOSS_SOULS", 1 },    // presets.ts:416
             { "gRando.Options.RO_SHUFFLE_NPC_SOULS", 1 },     // presets.ts:418
             { "gRando.Options.RO_SHUFFLE_ANIMAL_SOULS", 1 },  // presets.ts:420
             { "gRando.Options.RO_SHUFFLE_MISC_SOULS", 1 },    // presets.ts:422
             { "gRandoSettings.Shuffle100GSReward", 1 },       // presets.ts:423
             { "gCombo.Rando.Reachability", 1 },               // logic beatable, presets.ts:430
             { "gRandoSettings.ShuffleSongs", 3 },             // presets.ts:433
             { "gRandoSettings.StartingMapsCompasses", 5 },    // presets.ts:437
             { "gRandoSettings.Keysanity", 5 },                // presets.ts:438
             { "gRandoSettings.GerudoKeys", 3 },               // presets.ts:439
             { "gRandoSettings.BossKeysanity", 5 },            // presets.ts:440
             { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 0 },  // presets.ts:442
             { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 0 },   // presets.ts:443
             { "gRandoSettings.ShuffleKeyRings", 3 },          // smallKeyRingOot random, presets.ts:444
             { "gRandoSettings.ShuffleKeyRingsGanonsCastle", 1 },
             { "gRandoSettings.SilverRupeeShuffle", 2 },         // presets.ts:446
             { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 0 }, // presets.ts:449-450
             { "gRandoSettings.ShuffleGanonBossKey", 5 },        // presets.ts:451
             { "gRandoSettings.ShuffleScrubs", 2 },              // presets.ts:453
             { "gRandoSettings.ShopsanityCount", 7 },            // shopShuffleOot full, capped by logic, presets.ts:457
             { "gRandoSettings.ShuffleRocks", 1 },               // presets.ts:470
             { "gRandoSettings.ShuffleTrees", 1 },               // presets.ts:472
             { "gRandoSettings.ShuffleBushes", 1 },              // presets.ts:474
             { "gRandoSettings.ShuffleBeanFairies", 1 },         // shuffleSoilOot, presets.ts:476
             { "gRandoSettings.ShuffleButterflyFairies", 1 },    // presets.ts:485
             { "gRandoSettings.ShuffleBoulders", 3 },            // presets.ts:487-491
             { "gRandoSettings.ShuffleFrogSongRupees", 1 },      // presets.ts:492
             { "gRandoSettings.ShuffleIcicles", 1 },             // presets.ts:493
             { "gRandoSettings.ShuffleRedIce", 1 },              // presets.ts:495
             { "gRandoSettings.ShuffleGerudoToken", 1 },         // presets.ts:497
             { "gRandoSettings.ShuffleMerchants", 3 },           // presets.ts:499-500
             { "gRandoSettings.Fishsanity", 2 },                 // pondFishShuffle, presets.ts:501
             { "gRandoSettings.FishsanityPondCount", 17 },       //
             { "gRandoSettings.FishsanityAgeSplit", 1 },         //
             { "gRandoSettings.ShuffleFountainFairies", 1 },     // presets.ts:503
             { "gRandoSettings.ShuffleFairySpots", 1 },          // presets.ts:505
             { "gRandoSettings.ShuffleWeirdEgg", 1 },            // eggShuffle, presets.ts:506
             { "gRandoSettings.ShopsanityPrices", 2 },           // weighted, presets.ts:507
             { "gRandoSettings.ScrubsPrices", 2 },               // presets.ts:508
             { "gRandoSettings.StartingAge", 2 },                // presets.ts:512
             { "gRandoSettings.DoorOfTime", 1 },                 // closed, presets.ts:515
             { "gRandoSettings.ClosedForest", 1 },               // presets.ts:517
             { "gRandoSettings.SkipScarecrowsSong", 1 },         // presets.ts:529
             { "gRando.Options.RO_CROSS_GAME_FARORE", 1 },       // presets.ts:533
             { "gRandoSettings.FullWallets", 1 },                // fillWallets, presets.ts:545
             { "gRando.Options.RO_STARTING_RUPEES", 1 },         //
             { "gRando.Options.RO_SHUFFLE_SONG_SUN", 1 },        // presets.ts:554
             { "gRandoSettings.BombchuBag", 2 },                 // bagSeparate, presets.ts:557
             { "gRandoSettings.EnableBombchuDrops", 1 },         //
             { "gRando.Options.RO_MM_SPELL_FIRE", 1 },           // presets.ts:559
             { "gRando.Options.RO_MM_SPELL_WIND", 1 },           // presets.ts:560
             { "gRando.Options.RO_MM_SPELL_LOVE", 1 },           // presets.ts:561
             { "gRando.Options.RO_MM_BOOTS_IRON", 1 },           // presets.ts:562
             { "gRando.Options.RO_MM_BOOTS_HOVER", 1 },          // presets.ts:563
             { "gRando.Options.RO_MM_TUNIC_GORON", 1 },          // presets.ts:564
             { "gRando.Options.RO_MM_TUNIC_ZORA", 1 },           // presets.ts:565
             { "gRandoSettings.ShuffleOcarinaButtons", 1 },      // presets.ts:597
             { "gRando.Options.RO_SHUFFLE_OCARINA_BUTTONS", 1 }, // presets.ts:598
             { "gRandoSettings.IceTrapPercent", 3 },             // trapRupoor halves the ice traps, presets.ts:599
             { "gRandoSettings.MQDungeons", 3 },                 // mqDungeons random, presets.ts:697
             { "gRandoSettings.MQDungeonsDekuTree", 2 },
             { "gRandoSettings.MQDungeonsGanonsCastle", 2 } });
    Absent(CommunityPreset::Allsanity, allsanity, { "gRando.Options.RO_HINTS_GOSSIP_STONE_STRENGTH" });
    // Every shared item, including the MM spells, boots and tunics with their MM items (presets.ts:601-654).
    for (int i = 0; i < SF_COUNT; ++i)
        if (i != SF_OCARINA)
            Expect(CommunityPreset::Allsanity, allsanity, { { SharedFamilyByIndex(i).cvar, 1 } });
    Check(Rules(CommunityPreset::Allsanity) == Values({ { "gCombo.Enhancements.ForeignSongs", 1 },
                                                        { "gEnhancements.BlueFireArrows", 1 }, // presets.ts:406
                                                        { "gEnhancements.SunlightArrows", 1 }, // presets.ts:408
                                                        { "gEnhancements.TimeTravel", 1 },     // ageChange oot, 516
                                                        { "gEnhancements.OpenAllHours", 1 },   // openMaskShop, 531
                                                        { "gEnhancements.Playback.SkipScarecrowSong", 1 } }), // 530
          "Allsanity's gameplay rules differ from OoTMM's");

    // Beginner's junkLocations (presets.ts:1033-1052) name real checks of each game.
    const auto beginnerJunk = CommunityPresetValues(CommunityPreset::Beginner);
    const std::string checks = Read(root + "/mm/2s2h/Rando/StaticData/Checks.cpp");
    Check(beginnerJunk.at("gRando.ExcludedChecks").size() == 17, "Beginner should junk OoTMM's 17 MM checks");
    for (const auto& check : beginnerJunk.at("gRando.ExcludedChecks"))
        Check(checks.find("RC(" + check.get<std::string>() + ",") != std::string::npos,
              check.get<std::string>() + " is not an MM check");
    // The OoT list holds RandomizerCheck numbers: count the enum's items.
    const std::string enumText = Read(root + "/soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerCheck.h");
    std::vector<std::string> ootChecks;
    const std::regex item(R"(^RANDO_ENUM_ITEM\((RC_[A-Z0-9_]+)\)\s*$)");
    std::istringstream lines(enumText.substr(enumText.find("RANDO_ENUM_BEGIN(RandomizerCheck)")));
    for (std::string line; std::getline(lines, line);) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        std::smatch match;
        if (std::regex_match(line, match, item))
            ootChecks.push_back(match[1].str());
        else if (line.rfind("RANDO_ENUM_END", 0) == 0)
            break;
        else
            Check(line.empty() || line.rfind("//", 0) == 0 || line.rfind("RANDO_ENUM_BEGIN", 0) == 0,
                  "RandomizerCheck has an item this test cannot number: " + line);
    }
    std::vector<std::string> ootJunk;
    std::istringstream csv(beginnerJunk.at("gRandoSettings.ExcludedLocations").get<std::string>());
    for (std::string id; std::getline(csv, id, ',');) {
        const size_t index = std::stoul(id);
        Check(index < ootChecks.size(), "OoT check " + id + " does not exist");
        ootJunk.push_back(ootChecks[index]);
    }
    Check(ootJunk == std::vector<std::string>({ "RC_KAK_40_GOLD_SKULLTULA_REWARD", "RC_KAK_50_GOLD_SKULLTULA_REWARD" }),
          "Beginner should junk Skulltula House 40 and 50 Tokens");

    std::cout << "Community presets: OoTMM v32.3 defaults, Beginner and Allsanity values, shared families, rules and "
                 "junked checks pinned\n";
    return 0;
}
