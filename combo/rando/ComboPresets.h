#pragma once
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>
#include "GameplayRules.h"
#include "SharedItems.h"

namespace ComboRando {
enum class CommunityPreset { Default, Beginner, Allsanity };
struct CommunityPresetInfo {
    CommunityPreset id;
    const char* name;
    const char* summary;
    const char* help;
};
inline constexpr CommunityPresetInfo kCommunityPresets[] = {
    { CommunityPreset::Default, "Default", "Classic combined randomizer",
      "Based on OoTMM's Default preset, using Combo Ship's supported options. Glitchless logic. The Rainbow Bridge "
      "needs all six medallions, Ganon's trials are off and you start with Ganon's Boss Key. Swords, ocarinas, the "
      "Gerudo card and the trade items are shuffled, MM keys stay in their dungeons, a few ice traps hide among both "
      "games' items and MM starts with no items." },
    { CommunityPreset::Beginner, "Beginner", "Easier access, strong hints, shared upgrades",
      "Adapted from OoTMM's Beginner preset. Start with both ocarinas, Zelda's Letter, Prelude of Light, the Hylian "
      "Shield, Soaring and supplies. Share the upgrades, masks and songs OoTMM's Beginner shares where Combo Ship has "
      "them. The Door of Time and Kakariko gate are open, one carpenter needs rescuing, the Scarecrow needs no song, "
      "shops open at any hour and the Bean Seller sells beans. Maps and compasses start collected and OoTMM's junk "
      "checks hold only junk. Combo Ship does not pre-complete dungeons." },
    { CommunityPreset::Allsanity, "Allsanity", "Explore a much larger check pool",
      "Adapted from OoTMM's Allsanity preset. Beatable-only logic. Keys, souls, shops, tokens, NPC rewards, fairies, "
      "freestanding items and scenery drops are shuffled in both worlds, plus OoT's pond fish, with random Master "
      "Quest dungeons and OoT key rings. Adds the MM spells, boots and tunics and the ocarina buttons, all shared "
      "across games. MM starts with no items. Entrance shuffle, Skip Child Zelda, Cross-Games Age and OoTMM item "
      "families Combo Ship lacks are left out." },
};
struct PresetOption {
    const char* key;
    int value;
};
// Adds each value; a key already listed takes the new value, so a preset's own choice replaces the base one.
inline void SetPresetOptions(std::vector<PresetOption>& options, std::initializer_list<PresetOption> values) {
    for (const auto& value : values) {
        auto it = std::find_if(options.begin(), options.end(),
                               [&](const PresetOption& option) { return !std::strcmp(option.key, value.key); });
        if (it == options.end())
            options.push_back(value);
        else
            it->value = value.value;
    }
}
// The shared items OoTMM's Beginner turns on (presets.ts:990-1008) that Combo Ship has; sharedShields is the Mirror
// Shield family. Song of Time, the ocarina buttons, the spells, boots and tunics stay separate (data.ts:2672,
// 2968-3048). Allsanity shares every family (presets.ts:601-654) and Default none.
inline bool BeginnerSharesFamily(int family) {
    switch (family) {
        case SF_BOW:
        case SF_BOMB_BAG:
        case SF_BOMBCHU_BAG:
        case SF_MAGIC:
        case SF_WALLET:
        case SF_HOOKSHOT:
        case SF_FIRE_ARROWS:
        case SF_ICE_ARROWS:
        case SF_LIGHT_ARROWS:
        case SF_LENS:
        case SF_EPONAS_SONG:
        case SF_SONG_OF_STORMS:
        case SF_GORON_MASK:
        case SF_ZORA_MASK:
        case SF_KEATON_MASK:
        case SF_BUNNY_HOOD:
        case SF_MASK_OF_TRUTH:
        case SF_MIRROR_SHIELD:
            return true;
        default:
            return false;
    }
}
// Corresponding enums/values are validated against each native menu by the preset regression.
// OoTMM builds its presets on DEFAULT_SETTINGS (makeSettings; presets.ts:7). The base list holds OoTMM's defaults
// (data.ts) wherever Combo Ship's own default differs, and each preset then sets what its OoTMM preset changes. A
// setting whose Combo Ship default differs from OoTMM's belongs in the base list; only one whose default matches may
// go unnamed. Entrance settings are not supported yet.
// The three keep OoTMM's default goal, Ganon & Majora (Goal 0): the profile derives TriforceHunt from Goal.
inline std::vector<PresetOption> CommunityPresetOptions(CommunityPreset preset) {
    std::vector<PresetOption> options = {
        { "gRandoSettings.LogicRules", 0 },
        { "gCombo.Rando.Reachability", 0 },
        { "gCombo.Rando.StartingGame", 0 },
        { "gCombo.Rando.Goal", 0 },
        { "gCombo.Rando.TriforceHunt", 0 },
        { "gCombo.Rando.TriforceRequired", 20 }, // triforceGoal, data.ts:91
        { "gCombo.Rando.TriforceTotal", 30 },    // triforcePieces, data.ts:101
        { "gRando.Options.RO_LOGIC", 0 },
        { "gRandoSettings.GossipStoneHints", 1 },
        { "gRandoSettings.HintClarity", 2 },
        { "gRandoSettings.HintDistribution", 1 },
        { "gRando.Options.RO_HINTS_GOSSIP_STONES", 1 }, // OoTMM's hints are on MM's stones too (hints.ts:20-28)
        // OoTMM shuffles these checks and its fixer pins none of them by default (fixer.ts:30-160).
        { "gRandoSettings.ShuffleKokiriSword", 1 },
        { "gRandoSettings.ShuffleOcarinas", 1 },    // shuffleOcarinasOot, data.ts:984
        { "gRandoSettings.ShuffleMasterSword", 1 }, // shuffleMasterSword, data.ts:992
        { "gRandoSettings.ShuffleGerudoToken", 1 }, // shuffleGerudoCard, data.ts:1000
        { "gRandoSettings.ShuffleZeldasLetter", 1 },
        { "gRandoSettings.ShuffleAdultTrade", 1 },
        { "gRandoSettings.ShuffleWeirdEgg", 1 },     // Malon's check is shuffled (pool_oot.csv:122)
        { "gRandoSettings.ShuffleMerchants", 1 },    // the Bean Seller is shuffled (fixer.ts:43)
        { "gRandoSettings.ShuffleScrubs", 1 },       // the three one-time scrubs are shuffled (fixer.ts:73-76)
        { "gRandoSettings.DoorOfTime", 1 },          // closed: only the Song of Time, data.ts:1169
        { "gRandoSettings.ClosedForest", 2 },        // dekuTree open, data.ts:1194
        { "gRandoSettings.RainbowBridge", 3 },       // six medallions, data.ts:1352
        { "gRandoSettings.GanonTrial", 0 },          // no trials, data.ts:1112
        { "gRandoSettings.ShuffleGanonBossKey", 2 }, // removed: we start with it, data.ts:484
        { "gRandoSettings.EnableBombchuDrops", 0 },  // bombchuBehaviorOot free, data.ts:1972
        { "gRandoSettings.IceTrapPercent", 5 },      // trapIce, trapsQuantity small: 5 per 100 junk, data.ts:2495,2551
        // OoTMM adds its traps to the combined pool, so they land on MM checks too (transform.ts:693-716). MM takes a
        // count, not a share: its pool holds about 45 of OoTMM's junk items (groups.ts:337), 5 per 100 of which is 2.
        { "gRando.Options.RO_SHUFFLE_TRAPS", 1 },
        { "gRando.Options.RO_TRAP_AMOUNT", 2 },
        { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 1 },      // own dungeon, data.ts:296
        { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 1 },       // own dungeon, data.ts:350
        { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 1 },   // chest fairies own dungeon, data.ts:455
        { "gRando.Options.RO_CLOCK_SHUFFLE_PROGRESSIVE", 1 }, // ascending, data.ts:1803
    };
    for (int i = 0; i < SF_COUNT; ++i) {
        const bool shared =
            preset == CommunityPreset::Allsanity || (preset == CommunityPreset::Beginner && BeginnerSharesFamily(i));
        if (i != SF_OCARINA && SharedFamilyByIndex(i).publicOption)
            options.push_back({ SharedFamilyByIndex(i).cvar, shared ? 1 : 0 });
    }
    if (preset != CommunityPreset::Default)
        SetPresetOptions(options, {
                                      { "gRandoSettings.CompleteMaskQuest", 2 },  // shared masks need OoT's in the pool
                                      { "gRandoSettings.BombchuBag", 2 },         // bagSeparate, presets.ts:557,987
                                      { "gRandoSettings.EnableBombchuDrops", 1 }, // bags add drops, data.ts:1980
                                  });
    if (preset == CommunityPreset::Beginner)
        SetPresetOptions(options,
                         {
                             { "gRando.Options.RO_STARTING_MAPS_AND_COMPASSES", 1 }, // and Tingle's, presets.ts:969
                             { "gRandoSettings.StartingMapsCompasses", 0 },          // presets.ts:970
                             { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 2 },        // removed, presets.ts:971
                             { "gRandoSettings.GerudoKeys", 0 },                     // presets.ts:972
                             { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 2 },     // presets.ts:973
                             { "gRandoSettings.ShuffleGerudoToken", 0 },             // presets.ts:974
                             { "gRandoSettings.StartingAge", 2 },                    // presets.ts:975
                             { "gRandoSettings.DoorOfTime", 2 },                     // presets.ts:976
                             { "gRandoSettings.ClosedForest", 1 },                   // presets.ts:977
                             { "gRandoSettings.FortressCarpenters", 1 },             // presets.ts:979
                             // kakarikoGate open and skipZelda (presets.ts:978,980): the letter is out of
                             // the pool and Talon is awake. Zelda's Letter check holds the letter
                             // (item_pool.cpp:344-346); Impa's song check, which OoTMM plandos to Prelude of
                             // Light (presets.ts:1058), gives one random song at the start.
                             { "gRandoSettings.StartingZeldasLetter", 1 },
                             { "gRandoSettings.ShuffleZeldasLetter", 0 },
                             { "gRandoSettings.ShuffleWeirdEgg", 2 },
                             { "gRandoSettings.SkipScarecrowsSong", 1 }, // presets.ts:983
                             // startingItems and the Zelda plando (presets.ts:1023-1032,1057-1058)
                             { "gRandoSettings.StartingOcarina", 2 },
                             { "gRandoSettings.StartingPreludeOfLight", 1 },
                             { "gRandoSettings.StartingHylianShield", 1 },
                             { "gRandoSettings.StartingDekuShield", 1 },
                             { "gRandoSettings.StartingSticks", 1 },
                             { "gRandoSettings.StartingNuts", 1 },
                             { "gRandoSettings.ShuffleMerchants", 0 }, // Bean Seller plando'd to its Magic Bean, 1056
                             // The custom hint list (presets.ts:1062-1072) gives 8 foolish hints and 3 item
                             // hints where OoTMM's default (hints.ts:20-28) gives 5 and 1.
                             { "gRandoSettings.HintDistribution", 3 },
                             { "gRando.Options.RO_HINTS_GOSSIP_STONE_STRENGTH", 75 },
                         });
    if (preset == CommunityPreset::Allsanity)
        SetPresetOptions(options,
                         {
                             { "gRandoSettings.IncludeTycoonWallet", 1 }, // colossalWallets, presets.ts:410
                             { "gRando.Options.RO_SHUFFLE_TYCOON_WALLET", 1 },
                             { "gRandoSettings.SoulsEnemyOot", 1 },          // presets.ts:413
                             { "gRandoSettings.ShuffleBossSouls", 1 },       // presets.ts:415
                             { "gRando.Options.RO_SHUFFLE_ENEMY_SOULS", 1 }, // presets.ts:414-422
                             { "gRando.Options.RO_SHUFFLE_BOSS_SOULS", 1 },
                             { "gRando.Options.RO_SHUFFLE_NPC_SOULS", 1 },
                             { "gRando.Options.RO_SHUFFLE_ANIMAL_SOULS", 1 },
                             { "gRando.Options.RO_SHUFFLE_MISC_SOULS", 1 },
                             { "gRandoSettings.Shuffle100GSReward", 1 }, // presets.ts:423
                             { "gCombo.Rando.Reachability", 1 },         // beatable, presets.ts:430
                             { "gRandoSettings.ShuffleSongs", 3 },       // presets.ts:433
                             { "gRandoSettings.ShuffleTokens", 3 },      // presets.ts:434-435
                             { "gRando.Options.RO_SHUFFLE_GOLD_SKULLTULAS", 1 },
                             { "gRandoSettings.StartingMapsCompasses", 5 },   // anywhere, presets.ts:437
                             { "gRandoSettings.Keysanity", 5 },               // presets.ts:438
                             { "gRandoSettings.GerudoKeys", 3 },              // presets.ts:439
                             { "gRandoSettings.BossKeysanity", 5 },           // presets.ts:440
                             { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 0 }, // presets.ts:442
                             { "gRando.Options.RO_PLACEMENT_BOSS_KEYS", 0 },  // presets.ts:443
                             // smallKeyRingOot random: a coin flip for each ring (presets.ts:444)
                             { "gRandoSettings.ShuffleKeyRings", 3 },
                             { "gRandoSettings.ShuffleKeyRingsGerudoFortress", 1 },
                             { "gRandoSettings.ShuffleKeyRingsForestTemple", 1 },
                             { "gRandoSettings.ShuffleKeyRingsFireTemple", 1 },
                             { "gRandoSettings.ShuffleKeyRingsWaterTemple", 1 },
                             { "gRandoSettings.ShuffleKeyRingsSpiritTemple", 1 },
                             { "gRandoSettings.ShuffleKeyRingsShadowTemple", 1 },
                             { "gRandoSettings.ShuffleKeyRingsBottomOfTheWell", 1 },
                             { "gRandoSettings.ShuffleKeyRingsGTG", 1 },
                             { "gRandoSettings.ShuffleKeyRingsGanonsCastle", 1 },
                             { "gRandoSettings.SilverRupeeShuffle", 2 },         // presets.ts:446
                             { "gRando.Options.RO_PLACEMENT_STRAY_FAIRIES", 0 }, // presets.ts:449-450
                             { "gRandoSettings.ShuffleGanonBossKey", 5 },        // presets.ts:451
                             { "gRandoSettings.ShuffleScrubs", 2 },              // presets.ts:453
                             { "gRandoSettings.ShuffleCows", 1 },                // presets.ts:455-456
                             { "gRando.Options.RO_SHUFFLE_COWS", 1 },
                             // shopShuffle full (presets.ts:457-458); logic caps the count at 7
                             { "gRandoSettings.Shopsanity", 1 },
                             { "gRandoSettings.ShopsanityCount", 7 },
                             { "gRando.Options.RO_SHUFFLE_SHOPS", 1 },
                             { "gRando.Options.RO_SHUFFLE_TINGLE_SHOPS", 1 },
                             { "gRando.Options.RO_SHUFFLE_OWL_STATUES", 1 }, // presets.ts:459
                             { "gRandoSettings.ShufflePots", 3 },            // presets.ts:460-461
                             { "gRando.Options.RO_SHUFFLE_POT_DROPS", 1 },
                             { "gRandoSettings.ShuffleCrates", 3 }, // presets.ts:462-464
                             { "gRando.Options.RO_SHUFFLE_CRATE_DROPS", 1 },
                             { "gRando.Options.RO_SHUFFLE_BARREL_DROPS", 1 },
                             { "gRandoSettings.ShuffleBeehives", 1 }, // presets.ts:465-466
                             { "gRando.Options.RO_SHUFFLE_HIVE_DROPS", 1 },
                             { "gRandoSettings.ShuffleGrass", 3 }, // presets.ts:467-469
                             { "gRando.Options.RO_SHUFFLE_GRASS_DROPS", 1 },
                             { "gRandoSettings.ShuffleRocks", 1 }, // presets.ts:470
                             { "gRandoSettings.ShuffleTrees", 1 }, // presets.ts:472-473
                             { "gRando.Options.RO_SHUFFLE_TREE_DROPS", 1 },
                             { "gRandoSettings.ShuffleBushes", 1 },       // presets.ts:474
                             { "gRandoSettings.ShuffleBeanFairies", 1 },  // shuffleSoilOot, presets.ts:476
                             { "gRandoSettings.ShuffleFreestanding", 3 }, // presets.ts:478-481
                             { "gRando.Options.RO_SHUFFLE_FREESTANDING_ITEMS", 1 },
                             { "gRandoSettings.ShuffleWonderItems", 3 }, // presets.ts:482-483
                             { "gRando.Options.RO_SHUFFLE_WONDER_ITEMS", 1 },
                             { "gRando.Options.RO_SHUFFLE_SNOWBALL_DROPS", 1 }, // presets.ts:484
                             { "gRandoSettings.ShuffleButterflyFairies", 1 },   // presets.ts:485-486
                             { "gRando.Options.RO_SHUFFLE_BUTTERFLIES", 1 },
                             { "gRandoSettings.ShuffleBoulders", 3 },       // all three kinds, presets.ts:487-491
                             { "gRandoSettings.ShuffleFrogSongRupees", 1 }, // presets.ts:492
                             { "gRandoSettings.ShuffleIcicles", 1 },        // presets.ts:493
                             { "gRandoSettings.ShuffleRedIce", 1 },         // presets.ts:495
                             { "gRandoSettings.ShuffleMerchants", 3 },      // presets.ts:499-500
                             // pondFishShuffle: the pond's fish only, adult fish apart (presets.ts:501)
                             { "gRandoSettings.Fishsanity", 2 },
                             { "gRandoSettings.FishsanityPondCount", 17 },
                             { "gRandoSettings.FishsanityAgeSplit", 1 },
                             { "gRandoSettings.ShuffleFountainFairies", 1 }, // presets.ts:503
                             { "gRandoSettings.ShuffleFairySpots", 1 },      // presets.ts:505
                             { "gRandoSettings.ShopsanityPrices", 2 },       // weighted, presets.ts:507-508
                             { "gRandoSettings.ScrubsPrices", 2 },
                             { "gRandoSettings.StartingAge", 2 },  // presets.ts:512
                             { "gRandoSettings.ClosedForest", 1 }, // presets.ts:517
                             // skipZelda (presets.ts:526) is left out: with a shuffled letter, Start with Zelda's
                             // Letter does not hand out Zelda's two checks (savefile.cpp:633-646) and opens the
                             // Kakariko gate, which this preset keeps vanilla (presets.ts:523).
                             { "gRandoSettings.SkipScarecrowsSong", 1 },   // presets.ts:529
                             { "gRando.Options.RO_CROSS_GAME_FARORE", 1 }, // presets.ts:533
                             { "gRandoSettings.FullWallets", 1 },          // fillWallets, presets.ts:545
                             { "gRando.Options.RO_STARTING_RUPEES", 1 },
                             { "gRando.Options.RO_SHUFFLE_SONG_SUN", 1 }, // presets.ts:554
                             { "gRando.Options.RO_MM_SPELL_FIRE", 1 },    // presets.ts:559-565
                             { "gRando.Options.RO_MM_SPELL_WIND", 1 },
                             { "gRando.Options.RO_MM_SPELL_LOVE", 1 },
                             { "gRando.Options.RO_MM_BOOTS_IRON", 1 },
                             { "gRando.Options.RO_MM_BOOTS_HOVER", 1 },
                             { "gRando.Options.RO_MM_TUNIC_GORON", 1 },
                             { "gRando.Options.RO_MM_TUNIC_ZORA", 1 },
                             { "gRandoSettings.ShuffleOcarinaButtons", 1 }, // presets.ts:597-598
                             { "gRando.Options.RO_SHUFFLE_OCARINA_BUTTONS", 1 },
                             { "gRandoSettings.IceTrapPercent", 3 }, // Rupoors take half the traps, presets.ts:599
                             // MM: 2.5 traps per 100 of its about 2,100 junk items (transform.ts:714-715).
                             { "gRando.Options.RO_TRAP_AMOUNT", 50 },
                             // mqDungeons random: a coin flip for each dungeon (presets.ts:697)
                             { "gRandoSettings.MQDungeons", 3 },
                             { "gRandoSettings.MQDungeonsDekuTree", 2 },
                             { "gRandoSettings.MQDungeonsDodongosCavern", 2 },
                             { "gRandoSettings.MQDungeonsJabuJabu", 2 },
                             { "gRandoSettings.MQDungeonsForestTemple", 2 },
                             { "gRandoSettings.MQDungeonsFireTemple", 2 },
                             { "gRandoSettings.MQDungeonsWaterTemple", 2 },
                             { "gRandoSettings.MQDungeonsSpiritTemple", 2 },
                             { "gRandoSettings.MQDungeonsShadowTemple", 2 },
                             { "gRandoSettings.MQDungeonsBottomOfTheWell", 2 },
                             { "gRandoSettings.MQDungeonsIceCavern", 2 },
                             { "gRandoSettings.MQDungeonsGTG", 2 },
                             { "gRandoSettings.MQDungeonsGanonsCastle", 2 },
                         });
    return options;
}
// The gameplay rules a preset turns on in play and in logic (SetLogicRule), by their in-game names. Applying a preset
// first takes every rule out of logic and keeps the player's own enhancements.
inline std::vector<PresetOption> CommunityPresetRules(CommunityPreset preset) {
    if (preset == CommunityPreset::Default)
        return {};
    std::vector<PresetOption> rules = {
        { "gCombo.Enhancements.ForeignSongs", 1 }, // songs*Mm and songSoaringOot (presets.ts:579-589,989,1010-1015)
        // openMaskShop (presets.ts:531,985): the Mask Shop opens at night; this also opens OoT's other shops.
        { "gEnhancements.OpenAllHours", 1 },
        { "gEnhancements.Playback.SkipScarecrowSong", 1 }, // freeScarecrowMm, presets.ts:530,984
    };
    if (preset == CommunityPreset::Allsanity)
        SetPresetOptions(rules,
                         {
                             { "gEnhancements.BlueFireArrows", 1 }, // presets.ts:406
                             { "gEnhancements.SunlightArrows", 1 }, // presets.ts:408
                             { "gEnhancements.TimeTravel", 1 }, // ageChange oot: the Ocarina of Time, presets.ts:516
                         });
    return rules;
}
// Values a preset writes as they are, beside its menu options: the checks it fills with junk (OoTMM junkLocations,
// OoT check numbers and MM check names), and the hidden Shared Ocarina, which OoTMM leaves off (sharedOcarina,
// data.ts:2704) and only an old file's import can turn on.
inline nlohmann::json CommunityPresetValues(CommunityPreset preset) {
    std::string oot;
    auto mm = nlohmann::json::array();
    if (preset == CommunityPreset::Beginner) {
        oot = "149,150"; // RC_KAK_40_GOLD_SKULLTULA_REWARD, RC_KAK_50_GOLD_SKULLTULA_REWARD (presets.ts:1051-1052)
        // OoTMM's MM junkLocations in its order (presets.ts:1034-1050), each with its OoTMM name.
        for (const char* check : {
                 "RC_BENEATH_THE_GRAVEYARD_DAMPE_CHEST",              // Beneath The Graveyard Dampe Chest
                 "RC_DEKU_PLAYGROUND_ALL_DAYS",                       // Deku Playground Reward All Days
                 "RC_GORON_RACETRACK_GOLD_DUST",                      // Goron Race Reward
                 "RC_GREAT_BAY_GREAT_FAIRY",                          // Great Bay Great Fairy
                 "RC_CLOCK_TOWN_EAST_HONEY_DARLING_ALL_DAYS",         // Honey & Darling Reward All Days
                 "RC_IKANA_GREAT_FAIRY",                              // Ikana Great Fairy
                 "RC_GREAT_BAY_COAST_NEW_WAVE_BOSSA_NOVA",            // Laboratory Zora Song
                 "RC_MOON_FIERCE_DEITY_MASK",                         // Moon Fierce Deity Mask
                 "RC_MOUNTAIN_VILLAGE_FROG_CHOIR",                    // Mountain Village Frog Choir HP
                 "RC_OCEAN_SPIDER_HOUSE_WALLET",                      // Ocean Spider House Wallet
                 "RC_PINNACLE_ROCK_REUNITE_SEAHORSE",                 // Pinnacle Rock HP
                 "RC_SNOWHEAD_GREAT_FAIRY",                           // Snowhead Great Fairy
                 "RC_STOCK_POT_INN_COUPLES_MASK",                     // Stock Pot Inn Couple's Mask
                 "RC_SWAMP_SPIDER_HOUSE_MASK_OF_TRUTH",               // Swamp Spider House Mask of Truth
                 "RC_CLOCK_TOWN_EAST_SHOOTING_GALLERY_PERFECT_SCORE", // Town Archery Reward 2
                 "RC_WATERFALL_RAPIDS_BEAVER_RACE_02",                // Waterfall Rapids Beaver Race 2
                 "RC_WOODFALL_GREAT_FAIRY",                           // Woodfall Great Fairy
             })
            mm.push_back(check);
    }
    return { { "gRandoSettings.ExcludedLocations", oot },
             { "gRando.ExcludedChecks", mm },
             { SharedFamilyByIndex(SF_OCARINA).cvar, 0 } };
}
// A preset as ProfileComboCatalog exports it to the menu, whose ApplyCommunityPreset applies it.
inline nlohmann::json CommunityPresetJson(const CommunityPresetInfo& preset) {
    auto writes = nlohmann::json::object(), rules = nlohmann::json::object();
    for (const auto& option : CommunityPresetOptions(preset.id))
        writes[option.key] = option.value;
    for (const auto& rule : CommunityPresetRules(preset.id))
        rules[rule.key] = rule.value;
    return { { "id", int(preset.id) },
             { "label", preset.name },
             { "summary", preset.summary },
             { "help", preset.help },
             { "writes", writes },
             { "rules", rules },
             { "values", CommunityPresetValues(preset.id) } };
}
} // namespace ComboRando
