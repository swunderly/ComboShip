// Home's community presets follow OoTMM v32.3's presets of the same name for every option Combo Ship supports
// (references/ootmm-stable-32.3: packages/generator/lib/combo/presets.ts, settings in packages/core/src/settings/
// data.ts). OoTMM builds each preset on DEFAULT_SETTINGS (makeSettings), so Default pins the OoTMM defaults Combo Ship
// must write because its own default differs, and Beginner and Allsanity pin their own changes, by presets.ts line.
// Every value a preset writes must be pinned. A pinned value with more than two choices also pins what it means: the
// choice's label in SoH's settings.cpp or the enum name of 2Ship's option in Types.h, so a wrong choice number fails
// here. The junked checks are also checked against the native check tables (argument: the repository root).
#include "rando/ComboPresets.h"
#include <algorithm>
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
using Labels = std::vector<std::string>;
struct Pin {
    const char* key;
    int value;
    const char* meaning = nullptr; // the choice's label or enum name; required when the option has 3+ choices
};
using Expected = std::vector<Pin>;

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
static std::string Read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    Check(file.good(), "cannot read " + path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

// Each SoH option's choice labels by CVar, from its OPT_U8 or OPT_BOOL declaration in settings.cpp (a bool without a
// list is Off/On). Sliders (NumOpts) have no labels.
static std::map<std::string, Labels> SohChoices(const std::string& text) {
    std::map<std::string, Labels> choices;
    const std::string marker = "CVAR_RANDOMIZER_SETTING(\"";
    for (size_t at = text.find(marker); at != std::string::npos; at = text.find(marker, at + 1)) {
        size_t start = std::string::npos;
        for (const char* macro : { "OPT_U8(", "OPT_BOOL(" }) {
            const size_t found = text.rfind(macro, at);
            if (found != std::string::npos && (start == std::string::npos || found > start))
                start = found;
        }
        // A CVar read in a callback is not a declaration: the last OPT_ statement ended before it.
        if (start == std::string::npos || text.find(';', start) < at)
            continue;
        const size_t nameStart = at + marker.size();
        const std::string key = "gRandoSettings." + text.substr(nameStart, text.find('"', nameStart) - nameStart);
        const std::string head = text.substr(start, at - start);
        Labels labels;
        const size_t open = head.find('{');
        if (open == std::string::npos) {
            if (head.rfind("OPT_BOOL(", 0) == 0)
                labels = { "Off", "On" };
        } else if (head.find("NumOpts", open) == std::string::npos) {
            const size_t close = head.find('}', open);
            for (size_t quote = head.find('"', open); quote < close; quote = head.find('"', quote + 1)) {
                const size_t end = head.find('"', quote + 1);
                labels.push_back(head.substr(quote + 1, end - quote - 1));
                quote = end;
            }
        }
        if (labels.empty())
            continue;
        const auto [it, added] = choices.emplace(key, labels);
        Check(added || it->second == labels, key + " is declared twice with different choices");
    }
    return choices;
}
// Each 2Ship option's choices by CVar: the enum (Types.h) that holds its default value (Options.cpp). Options whose
// default is a number, and enums that assign values, have none.
static std::map<std::string, Labels> MmChoices(const std::string& types, const std::string& options) {
    std::vector<Labels> enums;
    const std::string marker = "typedef enum {";
    for (size_t at = types.find(marker); at != std::string::npos; at = types.find(marker, at + 1)) {
        std::string body = types.substr(at + marker.size(), types.find('}', at) - at - marker.size());
        body = std::regex_replace(body, std::regex(R"(//[^\n]*)"), "");
        Labels names;
        bool assigned = false;
        std::istringstream items(body);
        for (std::string item; std::getline(items, item, ',');) {
            item = std::regex_replace(item, std::regex(R"(\s+)"), "");
            assigned |= item.find('=') != std::string::npos;
            if (!item.empty())
                names.push_back(item);
        }
        if (!assigned)
            enums.push_back(names);
    }
    std::map<std::string, Labels> choices;
    const std::regex option(R"(^\s*RO\((RO_[A-Z0-9_]+),\s*([A-Z][A-Z0-9_]*)\),)");
    std::istringstream lines(options);
    for (std::string line; std::getline(lines, line);) {
        std::smatch match;
        if (!std::regex_search(line, match, option))
            continue;
        for (const auto& names : enums)
            if (std::find(names.begin(), names.end(), match[2].str()) != names.end())
                choices["gRando.Options." + match[1].str()] = names;
    }
    return choices;
}
static std::map<std::string, Labels> gChoices;
static std::map<CommunityPreset, std::set<std::string>> gMeant;  // keys with 3+ choices whose meaning was pinned
static std::map<CommunityPreset, std::set<std::string>> gPinned; // every key whose value was pinned

static void Expect(CommunityPreset preset, const Values& values, const Expected& expected) {
    for (const auto& pin : expected) {
        gPinned[preset].insert(pin.key);
        const auto it = values.find(pin.key);
        Check(it != values.end() && it->second == pin.value,
              std::string(Name(preset)) + ": " + pin.key + " should be " + std::to_string(pin.value) + ", is " +
                  (it == values.end() ? std::string("not written") : std::to_string(it->second)));
        const auto labels = gChoices.find(pin.key);
        const bool known = labels != gChoices.end();
        if (pin.meaning) {
            Check(known, std::string(pin.key) + "'s choices could not be read from the native settings");
            const auto& names = labels->second;
            const std::string actual =
                pin.value >= 0 && size_t(pin.value) < names.size() ? names[pin.value] : "out of range";
            Check(actual == pin.meaning, std::string(Name(preset)) + ": " + pin.key + " " + std::to_string(pin.value) +
                                             " means '" + actual + "', not '" + pin.meaning + "'");
            gMeant[preset].insert(pin.key);
        } else {
            Check(!known || labels->second.size() <= 2, std::string(pin.key) + " has " +
                                                            std::to_string(known ? labels->second.size() : 0) +
                                                            " choices: pin what its value means too");
        }
    }
}
static void Absent(CommunityPreset preset, const Values& values, const std::vector<const char*>& keys) {
    for (const char* key : keys)
        Check(!values.count(key), std::string(Name(preset)) + " must not write " + key);
}
// The per-dungeon CVars whose keys start with prefix and whose choices are exactly these labels.
static std::vector<std::string> DungeonKeys(const std::string& prefix, const Labels& labels) {
    std::vector<std::string> keys;
    for (const auto& [key, names] : gChoices)
        if (key.rfind(prefix, 0) == 0 && key != prefix && names == labels)
            keys.push_back(key);
    return keys;
}

// OoTMM DEFAULT_SETTINGS that none of the three presets changes, written where Combo Ship's own default differs.
static const Expected kOotmmDefaults = {
    { "gRandoSettings.LogicRules", 0 },                       // glitchless
    { "gRando.Options.RO_LOGIC", 0, "RO_LOGIC_GLITCHLESS" },  //
    { "gCombo.Rando.StartingGame", 0 },                       // OoT: OoTMM has no setting and starts there
    { "gCombo.Rando.Goal", 0 },                               // goal both: Ganon & Majora
    { "gCombo.Rando.TriforceHunt", 0 },                       //
    { "gCombo.Rando.TriforceRequired", 20 },                  // triforceGoal, data.ts:91
    { "gCombo.Rando.TriforceTotal", 30 },                     // triforcePieces, data.ts:101
    { "gRandoSettings.GossipStoneHints", 1, "Need Nothing" }, // every stone hints
    { "gRandoSettings.HintClarity", 2, "Clear" },             //
    { "gRandoSettings.ShuffleOcarinas", 1 },                  // shuffleOcarinasOot, data.ts:984
    { "gRandoSettings.ShuffleMasterSword", 1 },               // shuffleMasterSword, data.ts:992
    { "gRandoSettings.ShuffleKokiriSword", 1 },               // an ordinary location (pool_oot.csv:13)
    { "gRandoSettings.ShuffleAdultTrade", 1 },                // ordinary NPC locations
    { "gRandoSettings.RainbowBridge", 3, "Medallions" },      // data.ts:1352 (presets.ts:527,981)
    { "gRandoSettings.GanonTrial", 0, "Skip" },               // none, data.ts:1112
    { "gRando.Options.RO_CLOCK_SHUFFLE_PROGRESSIVE", 1, "RO_CLOCK_SHUFFLE_ASCENDING" }, // data.ts:1803
    { "gRando.Options.RO_HINTS_GOSSIP_STONES", 1 }, // hints on MM's gossip stones too
    { "gRando.Options.RO_SHUFFLE_TRAPS", 1 },       // trapIce: OoTMM's traps reach MM
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
    gChoices = SohChoices(Read(root + "/soh/soh/Enhancements/randomizer/settings.cpp"));
    for (auto& [key, names] :
         MmChoices(Read(root + "/mm/2s2h/Rando/Types.h"), Read(root + "/mm/2s2h/Rando/StaticData/Options.cpp")))
        gChoices[key] = names;
    // The readers found the choices this test depends on.
    Check(gChoices["gRandoSettings.ShuffleMerchants"] ==
              Labels({ "Off", "Bean Merchant Only", "All But Beans", "All" }),
          "settings.cpp's choice labels could not be read");
    Check(gChoices["gRando.Options.RO_PLACEMENT_SMALL_KEYS"] ==
              Labels({ "RO_DUNGEON_ITEM_ANYWHERE", "RO_DUNGEON_ITEM_OWN_DUNGEON", "RO_DUNGEON_ITEM_START_WITH" }),
          "2Ship's option enums could not be read");

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
                Expect(preset, values, { { "gRandoSettings.CompleteMaskQuest", 2, "Shuffle" } });
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
               { "gCombo.Rando.Reachability", 0 },                                            // logic all locations
               { "gRandoSettings.HintDistribution", 1, "Balanced" },                          // OoTMM's default hints
               { "gRandoSettings.ShuffleScrubs", 1, "One-Time Only" },                        // fixer.ts:73-76
               { "gRandoSettings.ShuffleGanonBossKey", 2, "Start With" },                     // removed, data.ts:484
               { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 1, "RO_DUNGEON_ITEM_OWN_DUNGEON" }, // data.ts:350
               { "gRandoSettings.ShuffleGerudoToken", 1 },                     // shuffleGerudoCard, data.ts:1000
               { "gRandoSettings.ShuffleZeldasLetter", 1 },                    // an ordinary location
               { "gRandoSettings.ShuffleWeirdEgg", 1, "Shuffled" },            // Malon's check (pool_oot.csv:122)
               { "gRandoSettings.ShuffleMerchants", 1, "Bean Merchant Only" }, // Granny, Carpet and Medigoron vanilla
               { "gRandoSettings.DoorOfTime", 1, "Song only" },                // closed, data.ts:1169
               { "gRandoSettings.ClosedForest", 2, "Off" },                    // dekuTree open, data.ts:1194
               { "gRandoSettings.EnableBombchuDrops", 0 },                     // bombchuBehaviorOot free
               { "gRandoSettings.IceTrapPercent", 5 },                         // trapIce, trapsQuantity small
               { "gRando.Options.RO_TRAP_AMOUNT", 2 },                         // 5 per 100 of MM's ~45 junk
               { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 1, "RO_DUNGEON_ITEM_OWN_DUNGEON" },   // data.ts:296
               { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 1, "RO_DUNGEON_ITEM_OWN_DUNGEON" } // data.ts:455
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
    Expect(CommunityPreset::Beginner, beginner,
           { { "gCombo.Rando.Reachability", 0 },
             { "gRando.Options.RO_STARTING_MAPS_AND_COMPASSES", 1 },      // tingleShuffle starting, 969
             { "gRandoSettings.StartingMapsCompasses", 0, "Start With" }, // presets.ts:970
             { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 2, "RO_DUNGEON_ITEM_START_WITH" },    // removed, 971
             { "gRandoSettings.GerudoKeys", 0, "Vanilla" },                                    // presets.ts:972
             { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 2, "RO_DUNGEON_ITEM_START_WITH" }, // 973
             { "gRandoSettings.ShuffleGerudoToken", 0 },                                       // presets.ts:974
             { "gRandoSettings.StartingAge", 2, "Random" },                                    // presets.ts:975
             { "gRandoSettings.DoorOfTime", 2, "Open" },                                       // presets.ts:976
             { "gRandoSettings.ClosedForest", 1, "Deku Only" },                                // dekuTree closed, 977
             { "gRandoSettings.StartingZeldasLetter", 1 },                 // kakarikoGate, skipZelda, 978,980
             { "gRandoSettings.ShuffleZeldasLetter", 0 },                  // the letter stays at Zelda
             { "gRandoSettings.ShuffleWeirdEgg", 2, "Skip Waking Talon" }, // skipZelda wakes Talon
             { "gRandoSettings.FortressCarpenters", 1, "Fast" },           // gerudoFortress single, 979
             { "gRandoSettings.SkipScarecrowsSong", 1 },                   // freeScarecrowOot, 983
             { "gRandoSettings.CompleteMaskQuest", 2, "Shuffle" },         // shared masks
             { "gRandoSettings.BombchuBag", 2, "Progressive Bags" },       // bagSeparate, 987
             { "gRandoSettings.EnableBombchuDrops", 1 },                   // bags add bombchu drops
             { "gRandoSettings.ShuffleMerchants", 0, "Off" },              // Bean Seller plando, 1056
             { "gRandoSettings.ShuffleScrubs", 1, "One-Time Only" },       // OoTMM's default scrubs
             { "gRandoSettings.ShuffleGanonBossKey", 2, "Start With" },    // ganonBossKey removed
             { "gRandoSettings.IceTrapPercent", 5 },                       //
             { "gRando.Options.RO_TRAP_AMOUNT", 2 },                       //
             { "gRandoSettings.StartingOcarina", 2, "Ocarina of Time" },   // OOT_OCARINA + Zelda plando
             { "gRandoSettings.StartingPreludeOfLight", 1 },               // plando'd Zelda's Song, 1058
             { "gRandoSettings.StartingHylianShield", 1 },                 // SHARED_SHIELD_HYLIAN, 1027
             { "gRandoSettings.StartingDekuShield", 1 },                   // presets.ts:1025
             { "gRandoSettings.StartingSticks", 1 },                       // presets.ts:1026
             { "gRandoSettings.StartingNuts", 1 },                         // presets.ts:1024
             { "gRandoSettings.HintDistribution", 3, "Very Strong" },      // hints, presets.ts:1062-1072
             { "gRando.Options.RO_HINTS_GOSSIP_STONE_STRENGTH", 75 },      //
             { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 1, "RO_DUNGEON_ITEM_OWN_DUNGEON" } });
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
           { { "gRandoSettings.IncludeTycoonWallet", 1 },               // colossalWallets, presets.ts:410
             { "gRando.Options.RO_SHUFFLE_TYCOON_WALLET", 1 },          //
             { "gRandoSettings.SoulsEnemyOot", 1 },                     // presets.ts:413
             { "gRandoSettings.ShuffleBossSouls", 1 },                  // presets.ts:415
             { "gRando.Options.RO_SHUFFLE_ENEMY_SOULS", 1 },            // presets.ts:414
             { "gRando.Options.RO_SHUFFLE_BOSS_SOULS", 1 },             // presets.ts:416
             { "gRando.Options.RO_SHUFFLE_NPC_SOULS", 1 },              // presets.ts:418
             { "gRando.Options.RO_SHUFFLE_ANIMAL_SOULS", 1 },           // presets.ts:420
             { "gRando.Options.RO_SHUFFLE_MISC_SOULS", 1 },             // presets.ts:422
             { "gRandoSettings.Shuffle100GSReward", 1 },                // presets.ts:423
             { "gCombo.Rando.Reachability", 1 },                        // logic beatable, presets.ts:430
             { "gRandoSettings.HintDistribution", 1, "Balanced" },      // the default hint list
             { "gRandoSettings.ShuffleSongs", 3, "Anywhere" },          // presets.ts:433
             { "gRandoSettings.ShuffleTokens", 3, "All Tokens" },       // presets.ts:434-435
             { "gRando.Options.RO_SHUFFLE_GOLD_SKULLTULAS", 1 },        //
             { "gRandoSettings.StartingMapsCompasses", 5, "Anywhere" }, // presets.ts:437
             { "gRandoSettings.Keysanity", 5, "Anywhere" },             // presets.ts:438
             { "gRandoSettings.GerudoKeys", 3, "Anywhere" },            // presets.ts:439
             { "gRandoSettings.BossKeysanity", 5, "Anywhere" },         // presets.ts:440
             { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 0, "RO_DUNGEON_ITEM_ANYWHERE" }, // presets.ts:442
             { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 0, "RO_DUNGEON_ITEM_ANYWHERE" },  // presets.ts:443
             { "gRandoSettings.ShuffleKeyRings", 3, "Selection" },                        // smallKeyRingOot random, 444
             { "gRandoSettings.SilverRupeeShuffle", 2, "Anywhere" },                      // presets.ts:446
             { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 0, "RO_DUNGEON_ITEM_ANYWHERE" }, // presets.ts:449-450
             { "gRandoSettings.ShuffleGanonBossKey", 5, "Anywhere" },                        // presets.ts:451
             { "gRandoSettings.ShuffleScrubs", 2, "All" },                                   // presets.ts:453
             { "gRandoSettings.ShuffleCows", 1 },                                            // presets.ts:455
             { "gRando.Options.RO_SHUFFLE_COWS", 1 },                                        // presets.ts:456
             { "gRandoSettings.Shopsanity", 1, "Specific Count" },                           // shopShuffleOot full, 457
             { "gRandoSettings.ShopsanityCount", 7 },                    // logic caps the count at 7
             { "gRando.Options.RO_SHUFFLE_SHOPS", 1 },                   // shopShuffleMm full, presets.ts:458
             { "gRando.Options.RO_SHUFFLE_TINGLE_SHOPS", 1 },            //
             { "gRando.Options.RO_SHUFFLE_OWL_STATUES", 1 },             // presets.ts:459
             { "gRandoSettings.ShufflePots", 3, "All Pots" },            // presets.ts:460-461
             { "gRando.Options.RO_SHUFFLE_POT_DROPS", 1 },               //
             { "gRandoSettings.ShuffleCrates", 3, "All Crates" },        // presets.ts:462-464
             { "gRando.Options.RO_SHUFFLE_CRATE_DROPS", 1 },             //
             { "gRando.Options.RO_SHUFFLE_BARREL_DROPS", 1 },            // shuffleBarrelsMm, presets.ts:464
             { "gRandoSettings.ShuffleBeehives", 1 },                    // presets.ts:465
             { "gRando.Options.RO_SHUFFLE_HIVE_DROPS", 1 },              // presets.ts:466
             { "gRandoSettings.ShuffleGrass", 3, "All Grass" },          // presets.ts:467-469
             { "gRando.Options.RO_SHUFFLE_GRASS_DROPS", 1 },             //
             { "gRandoSettings.ShuffleRocks", 1 },                       // presets.ts:470
             { "gRandoSettings.ShuffleTrees", 1 },                       // presets.ts:472
             { "gRando.Options.RO_SHUFFLE_TREE_DROPS", 1 },              // presets.ts:473
             { "gRandoSettings.ShuffleBushes", 1 },                      // presets.ts:474
             { "gRandoSettings.ShuffleBeanFairies", 1 },                 // shuffleSoilOot, presets.ts:476
             { "gRandoSettings.ShuffleFreestanding", 3, "All Items" },   // presets.ts:478-481
             { "gRando.Options.RO_SHUFFLE_FREESTANDING_ITEMS", 1 },      //
             { "gRandoSettings.ShuffleWonderItems", 3, "All Items" },    // presets.ts:482-483
             { "gRando.Options.RO_SHUFFLE_WONDER_ITEMS", 1 },            //
             { "gRando.Options.RO_SHUFFLE_SNOWBALL_DROPS", 1 },          // presets.ts:484
             { "gRandoSettings.ShuffleButterflyFairies", 1 },            // presets.ts:485
             { "gRando.Options.RO_SHUFFLE_BUTTERFLIES", 1 },             // presets.ts:486
             { "gRandoSettings.ShuffleBoulders", 3, "All Boulders" },    // presets.ts:487-491
             { "gRandoSettings.ShuffleFrogSongRupees", 1 },              // presets.ts:492
             { "gRandoSettings.ShuffleIcicles", 1 },                     // presets.ts:493
             { "gRandoSettings.ShuffleRedIce", 1 },                      // presets.ts:495
             { "gRandoSettings.ShuffleGerudoToken", 1 },                 // presets.ts:497
             { "gRandoSettings.ShuffleMerchants", 3, "All" },            // presets.ts:499-500
             { "gRandoSettings.Fishsanity", 2, "Shuffle Fishing Pond" }, // pondFishShuffle, presets.ts:501
             { "gRandoSettings.FishsanityPondCount", 17 },               //
             { "gRandoSettings.FishsanityAgeSplit", 1 },                 //
             { "gRandoSettings.ShuffleFountainFairies", 1 },             // presets.ts:503
             { "gRandoSettings.ShuffleFairySpots", 1 },                  // presets.ts:505
             { "gRandoSettings.ShuffleWeirdEgg", 1, "Shuffled" },        // the base list: Malon's check
             { "gRandoSettings.ShuffleZeldasLetter", 1 },                // the base list; skipZelda is left out
             { "gRandoSettings.ShopsanityPrices", 2, "Balanced" },       // weighted, presets.ts:507
             { "gRandoSettings.ScrubsPrices", 2, "Balanced" },           // presets.ts:508
             { "gRandoSettings.StartingAge", 2, "Random" },              // presets.ts:512
             { "gRandoSettings.DoorOfTime", 1, "Song only" },            // closed, presets.ts:515
             { "gRandoSettings.ClosedForest", 1, "Deku Only" },          // presets.ts:517
             { "gRandoSettings.SkipScarecrowsSong", 1 },                 // presets.ts:529
             { "gRando.Options.RO_CROSS_GAME_FARORE", 1 },               // presets.ts:533
             { "gRandoSettings.FullWallets", 1 },                        // fillWallets, presets.ts:545
             { "gRando.Options.RO_STARTING_RUPEES", 1 },                 //
             { "gRando.Options.RO_SHUFFLE_SONG_SUN", 1 },                // presets.ts:554
             { "gRandoSettings.CompleteMaskQuest", 2, "Shuffle" },       // shared masks
             { "gRandoSettings.BombchuBag", 2, "Progressive Bags" },     // bagSeparate, presets.ts:557
             { "gRandoSettings.EnableBombchuDrops", 1 },                 //
             { "gRando.Options.RO_MM_SPELL_FIRE", 1 },                   // presets.ts:559
             { "gRando.Options.RO_MM_SPELL_WIND", 1 },                   // presets.ts:560
             { "gRando.Options.RO_MM_SPELL_LOVE", 1 },                   // presets.ts:561
             { "gRando.Options.RO_MM_BOOTS_IRON", 1 },                   // presets.ts:562
             { "gRando.Options.RO_MM_BOOTS_HOVER", 1 },                  // presets.ts:563
             { "gRando.Options.RO_MM_TUNIC_GORON", 1 },                  // presets.ts:564
             { "gRando.Options.RO_MM_TUNIC_ZORA", 1 },                   // presets.ts:565
             { "gRandoSettings.ShuffleOcarinaButtons", 1 },              // presets.ts:597
             { "gRando.Options.RO_SHUFFLE_OCARINA_BUTTONS", 1 },         // presets.ts:598
             { "gRandoSettings.IceTrapPercent", 3 },                     // trapRupoor halves the ice traps, 599
             { "gRando.Options.RO_TRAP_AMOUNT", 50 },                    // 2.5 per 100 of MM's ~2,100 junk
             { "gRandoSettings.MQDungeons", 3, "Selection Only" } });    // mqDungeons random, presets.ts:697
    Absent(CommunityPreset::Allsanity, allsanity, { "gRando.Options.RO_HINTS_GOSSIP_STONE_STRENGTH" });
    // Each OoT key ring and each dungeon's quest is a coin flip (smallKeyRingOot and mqDungeons random).
    const auto keyRings = DungeonKeys("gRandoSettings.ShuffleKeyRings", { "No", "Random", "Yes" });
    const auto quests = DungeonKeys("gRandoSettings.MQDungeons", { "Vanilla", "Master Quest", "Random" });
    Check(keyRings.size() == 9 && quests.size() == 12, "expected 9 OoT key-ring and 12 dungeon-quest settings, found " +
                                                           std::to_string(keyRings.size()) + " and " +
                                                           std::to_string(quests.size()));
    for (const auto& key : keyRings)
        Expect(CommunityPreset::Allsanity, allsanity, { { key.c_str(), 1, "Random" } });
    for (const auto& key : quests)
        Expect(CommunityPreset::Allsanity, allsanity, { { key.c_str(), 2, "Random" } });
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

    // Every value a preset writes was pinned above, and one with more than two choices had its meaning pinned too.
    for (const auto& info : kCommunityPresets)
        for (const auto& [key, value] : Options(info.id)) {
            Check(gPinned[info.id].count(key),
                  std::string(info.name) + " writes " + key + " = " + std::to_string(value) + " without pinning it");
            const auto labels = gChoices.find(key);
            if (labels != gChoices.end() && labels->second.size() > 2)
                Check(gMeant[info.id].count(key), std::string(info.name) + " writes " + key + " = " +
                                                      std::to_string(value) + " without pinning what it means");
        }

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

    std::cout << "Community presets: OoTMM v32.3 defaults, Beginner and Allsanity values and their meanings, shared "
                 "families, rules and junked checks pinned\n";
    return 0;
}
