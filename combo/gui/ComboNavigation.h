#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include "ComboMenuPlacement.h"
#include "ComboMenuSearch.h"

// Feature destinations, independent of engine ordering. Unknown pages stay reachable in Tools.
namespace ComboRando {
struct MenuDestination {
    std::string scope, page;
};
inline std::string NormalizeMenuSearch(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c) != 0; }),
               text.end());
    return text;
}
inline bool MenuSearchMatches(const std::string& query, const char* name, const char* keywords) {
    return MenuSearchScore(query, name, keywords) != 0;
}
// Combo-owned pages have no native widget metadata. Index them explicitly so search
// leads to the common editor or canonical setter instead of a legacy engine control.
struct SharedPageInfo {
    const char* scope;
    const char* scopeLabel;
    const char* name;
    const char* keywords;
};
inline constexpr SharedPageInfo kSharedPages[] = {
    { "media", "Video & Audio", MenuPage::ScreenHud,
      "screen fit automatic fitter screen size aspect ratio widescreen borders safe insets HUD layout HUD editor edit "
      "HUD HUD placement hearts magic rupees minimap position guide reset" },
    { "media", "Video & Audio", MenuPage::Camera,
      "camera aiming first person third person free look freelook gyro sensitivity invert distance" },
    { "settings", "Settings", MenuPage::Interface,
      "menu scale menu size text size theme background opacity navigation" },
    { "home", "Home", MenuPage::CommunityPresets, "presets community OoTMM beginner default allsanity" },
    { "randomizer", "Randomizer", "Shared items",
      "shared bows bomb bags magic wallet hookshot arrows masks songs shared items pool" },
    { "randomizer", "Randomizer", "Warps & travel",
      "warp cross game songs Requiem Soaring adult child destination shuffle" },
    { "gameplay", "Enhancements", MenuPage::WarpsTravel,
      "cross game songs warp Soaring adult child travel quality of life owl warp pause menu warp Farore" },
    { "randomizer", "Randomizer", "Gameplay rules", "required counts frogs eggs dog race Gibdo trades" },
    { "gameplay", "Enhancements", MenuPage::Difficulty, "minigame targets required counts chicks Bremen" },
    { "gameplay", "Enhancements", MenuPage::ItemsEquipment,
      "enhancement only quality of life unequip magic arrow equip arrow cycle masks" },
    { "gameplay", "Enhancements", MenuPage::TimeSavers,
      "song playback animations cutscene skip speed-ups climb climbing ladder vine push pull block" },
    { "gameplay", "Enhancements", MenuPage::Saving, "autosave automatic saves saving interval" },
    { "settings", "Settings", MenuPage::Controls,
      "D-pad dpad equips controller mapping input buttons invisible floating sticks touch layout menu position "
      "ocarina input lag pause buffer timing manual jump" },
    { "settings", "Settings", MenuPage::General,
      "accessibility screen flash quakes crit wiggle boot file select reset shortcut languages" },
    { "settings", "Settings", MenuPage::Notifications, "notifications notification position duration opacity size" },
    { "gameplay", "Enhancements", MenuPage::ExtraModes,
      "extra modes traps trap effects mirrored world enemy randomizer challenges" },
    { "gameplay", "Enhancements", MenuPage::RandomizerExtras,
      "containers match contents chests junk items trap appearance mystery models fanfares" },
    { "gameplay", "Enhancements", MenuPage::Restorations,
      "restoration restored behavior Shadow Door Unload stair shortcut Early Eyeball Frog trade sequence shield delay "
      "gameplay only" },
    { "trackers", "Trackers", MenuPage::CheckTracker,
      "check tracker on-screen CT touch visibility shortcut button toggle hold collected skipped search current area "
      "logic appearance text size background side panel dock divider" },
    { "trackers", "Trackers", MenuPage::ItemTracker,
      "item tracker equipment inventory icons icon size icon spacing layout visibility while paused button toggle hold "
      "shortcut modifier IT on-screen touch side panel" },
    { "trackers", "Trackers", MenuPage::HintTracker,
      "hint tracker hinted seen known path boss event notes visibility while paused button toggle hold shortcut "
      "modifier HT on-screen touch side panel" },
    { "trackers", "Trackers", MenuPage::Timers, "game timer combined timer play time clock splits" },
    { "tools", "Tools", MenuPage::Multiplayer,
      "multiplayer co-op shared progress Anchor network host server room team port reconnect" },
};
struct MenuGuideInfo {
    const char* scope;
    const char* title;
    const char* introduction;
    const char* behavior;
    const char* runWarning;
};
inline constexpr MenuGuideInfo kMenuGuides[] = {
    { "randomizer", "Randomizer", "These settings define the items and logic of your next seed.",
      "These rules also control the corresponding in-game abilities.", "Existing item placements stay unchanged." },
    { "gameplay", "Enhancements", "These settings change how you play.", "Logic choices are under Randomizer.",
      "Your seed's item placements stay unchanged." }
};
inline const MenuGuideInfo* FindMenuGuide(const std::string& scope) {
    for (const auto& guide : kMenuGuides)
        if (scope == guide.scope)
            return &guide;
    return nullptr;
}
inline MenuDestination MenuRoute(bool oot, const std::string& section, const std::string& page) {
    if (page == "Search")
        return {};
    if (section == "Network")
        return {}; // Anchor is shared; unsupported integrations stay hidden.
    if (section == "Randomizer" || section == "Rando") {
        if (page == "Item Tracker" || page == "Check Tracker" || page == "Hint Tracker" || page == "Plandomizer")
            return {};
        if (page.find("Tracker") != std::string::npos)
            return { "trackers", page };
        if (page == "General")
            return { "gameplay", MenuPage::RandomizerExtras };
        if (page == "Locations" || page == "Check Exclusions")
            return { "randomizer", "Excluded checks" };
        if (page == "Logic/Access" || page == "Logic/Conditions")
            return { "randomizer", "Logic & access" };
        if (page == "Tricks/Glitches")
            return { "randomizer", "Tricks & glitches" };
        if (page == "Hints/Traps" || page == "Hints" || page == "Traps")
            return { "randomizer", "Hints & traps" };
        if (page == "Shuffles" || page == "Check Pool" || page == "Item Pool")
            return { "randomizer", "Items & checks" };
        if (page == "Starting Items")
            return { "randomizer", "Starting inventory" };
        if (page == "Dungeon Items")
            return { "randomizer", "Dungeons" };
        return { "randomizer", page };
    }
    if (section == "Settings") {
        if (page == "Camera")
            return { "media", MenuPage::Camera };
        if (page == "General")
            return { "settings", MenuPage::General };
        if (page == "Presets")
            return { "tools", MenuPage::EnhancementPresets };
        if (page == "Mod Menu")
            return { "tools", MenuPage::Mods };
        if (page == "Overlay")
            return { "media", MenuPage::HudOverlays };
        // The shared audio bridge and controller backend already drive both engines.
        if (!oot && (page == "Audio" || page == "Graphics" || page == "Controls" || page == "Input Viewer"))
            return {};
        if (page == "Audio" || page == "Graphics")
            return { "media", page };
        return { "settings", page };
    }
    if (section == "Enhancements") {
        if (page == "Timers")
            return {}; // Replaced by the combined timer.
        if (page == "Quality of Life" || page == "Gameplay")
            return { "gameplay", MenuPage::QualityOfLife };
        if (page == "Skips & Speed-ups" || page == "Time Savers")
            return { "gameplay", MenuPage::TimeSavers };
        if (page == "Items" || page == "Items/Songs")
            return { "gameplay", MenuPage::ItemsEquipment };
        if (page == "Difficulty" || page == "Difficulty Options" || (oot && page == "Minigames"))
            return { "gameplay", MenuPage::Difficulty };
        if (page == "Camera")
            return { "media", MenuPage::Camera };
        if (page == "Cosmetics Editor" || page == "Cosmetic Editor")
            return { "media", MenuPage::Appearance };
        if (page == "HUD Editor")
            return { "media", MenuPage::HudOverlays };
        if (page == "Graphics")
            return { "media", MenuPage::VisualEnhancements };
        if (page == "Audio Editor")
            return { "media", MenuPage::MusicSounds };
        if (page == "Gameplay Stats" || page == "Time Splits")
            return { "trackers", page };
        return { "gameplay", page };
    }
    if (section == "Dev Tools") {
        if (page == "General")
            return { "tools", MenuPage::DeveloperOptions };
        if (page == "DList Viewer" || page == "DL Viewer")
            return { "tools", MenuPage::DisplayLists };
        return { "tools", page };
    }
    return { "tools", section + " / " + page };
}
inline const char* MenuScopeLabel(const std::string& scope) {
    if (scope == "randomizer")
        return "Randomizer";
    if (scope == "gameplay")
        return "Enhancements";
    if (scope == "trackers")
        return "Trackers";
    if (scope == "settings")
        return "Settings";
    if (scope == "media")
        return "Video & Audio";
    if (scope == "home")
        return "Home";
    return "Tools";
}
inline MenuDestination MenuGroupRoute(bool oot, const std::string& section, const std::string& page, int column,
                                      const std::string& heading, const std::string& key = {}) {
    auto route = MenuRoute(oot, section, page);
    // The placement catalogue decides first. Its Randomizer rows only order the seed pages; the
    // clauses below still route those, and every row the catalogue does not hold.
    if (page != "Search" && section != "Network")
        if (const auto* placement = FindMenuPlacement(oot, section, page, heading, key);
            placement && std::strcmp(placement->scope, "randomizer") != 0)
            return { placement->scope, placement->page };
    if (oot && section == "Randomizer" && page == "Logic/Access") {
        if (key == "gRandoSettings.CompleteMaskQuest" || key == "gRandoSettings.ItemPool" ||
            heading == "Item Pool Settings")
            return { "randomizer", "Items & checks" };
        if (key == "gRandoSettings.FullWallets")
            return { "randomizer", "Starting inventory" };
        // OoTMM keeps one warp-song and soaring-spot destination shuffle; OoT's own two sit beside the combined one.
        if (key == "gRandoSettings.ShuffleWarpSongs" || key == "gRandoSettings.ShuffleOwlDrops")
            return { "randomizer", "Warps & travel" };
        for (const char* rule : { "BigPoeTargetCount", "SlingBowBeehives", "SwordlessEponaItems", "SkipChildStealth",
                                  "EarlyGrannysShop", "SkipPlantingBeans", "SkipEponaRace", "SkipScarecrowsSong" })
            if (key == std::string("gRandoSettings.") + rule)
                return { "randomizer", "Gameplay rules" };
    }
    // Boss souls sit with the other souls, as OoTMM keeps them and as MM shows them.
    if (oot && section == "Randomizer" && page == "Dungeons" && key == "gRandoSettings.ShuffleBossSouls")
        return { "randomizer", "Items & checks" };
    if (section == "Enhancements" && (page == "Quality of Life" || page == "Gameplay")) {
        if (heading == "Saving")
            return { "gameplay", MenuPage::Saving };
        if (heading == "Controls")
            return { "settings", MenuPage::Controls };
    }
    if (oot && section == "Enhancements" && page == "Fixes" && column == 2)
        return { "gameplay", MenuPage::Restorations };
    return route;
}
inline int MenuPageOrder(const std::string& scope, const std::string& page) {
    using namespace MenuPage;
    const char* gameplay[] = { ItemsEquipment, WarpsTravel,  TimeSavers, Saving, QualityOfLife,   Difficulty,
                               Fixes,          Restorations, ExtraModes, Cheats, RandomizerExtras };
    const char* media[] = { ScreenHud, Camera, Graphics, VisualEnhancements, Appearance, Audio, MusicSounds };
    const char* settings[] = { General, Controls, InputViewer, Interface, Notifications, About };
    const char* trackers[] = { CheckTracker, ItemTracker, HintTracker,  EntranceTracker,
                               Timers,       TimeSplits,  GameplayStats };
    const char* tools[] = { SaveEditor,    EnhancementPresets, Mods,
                            Multiplayer,   DeveloperOptions,   Stats,
                            Console,       HookDebugger,       CollisionViewer,
                            ActorViewer,   DisplayLists,       ValueViewer,
                            MessageViewer, GfxDebugger,        EventLog,
                            Session };
    const char* const* pages = scope == "gameplay"   ? gameplay
                               : scope == "media"    ? media
                               : scope == "tools"    ? tools
                               : scope == "trackers" ? trackers
                                                     : settings;
    const int count = scope == "gameplay"   ? int(sizeof(gameplay) / sizeof(*gameplay))
                      : scope == "media"    ? int(sizeof(media) / sizeof(*media))
                      : scope == "tools"    ? int(sizeof(tools) / sizeof(*tools))
                      : scope == "trackers" ? int(sizeof(trackers) / sizeof(*trackers))
                                            : int(sizeof(settings) / sizeof(*settings));
    for (int i = 0; i < count; ++i)
        if (page == pages[i])
            return i;
    return 100;
}
inline bool IsDeveloperToolPage(const std::string& page) {
    using namespace MenuPage;
    for (const char* name : { DeveloperOptions, Stats, Console, HookDebugger, CollisionViewer, ActorViewer,
                              DisplayLists, ValueViewer, MessageViewer, GfxDebugger, EventLog })
        if (page == name)
            return true;
    return false;
}
inline MenuDestination NativeSearchPageRoute(bool oot, const std::string& section, const std::string& page) {
    // Discovery of HUD editing goes to the common editor, where MM's own editor is a folded block.
    // MM's overlay sidebar holds only its notification settings, which share OoT's page.
    if (section == "Enhancements" && page == "HUD Editor")
        return { "media", MenuPage::ScreenHud };
    if (section == "Settings" && page == "Overlay")
        return { "settings", MenuPage::Notifications };
    return MenuRoute(oot, section, page);
}
inline std::string NativeSearchPageKeywords(const std::string& section, const std::string& sidebar,
                                            const MenuDestination& destination, bool oot = true) {
    auto keywords = section + " " + sidebar + " " + MenuScopeLabel(destination.scope) + " " + destination.page;
    // Topic aliases describe this page, not every widget elsewhere in the game.
    if (destination.scope == "settings" || destination.scope == "media") {
        if (destination.page == MenuPage::Audio)
            keywords += " sound music volume speakers both games OoT MM";
        else if (destination.page == MenuPage::Graphics)
            keywords += " display renderer performance frame FPS resolution both games OoT MM";
        else if (destination.page == MenuPage::Controls)
            keywords += " controller input mapping gamepad button bind keyboard both games OoT MM";
    }
    if (destination.scope == "tools" && destination.page == MenuPage::EnhancementPresets)
        keywords += " saved presets"; // the page's old name
    if (destination.scope == "tools" && destination.page == MenuPage::SaveEditor) {
        keywords += " save inventory equipment resources cheats";
        keywords +=
            oot ? " OoT Ocarina Time"
                : " MM Majora Mask owl statues owl activation activate unlock clear destinations Soaring save flags";
    }
    return keywords;
}
inline bool IsOwlEditorSearch(bool oot, const MenuDestination& destination, const std::string& query) {
    if (oot || destination.scope != "tools" || destination.page != MenuPage::SaveEditor)
        return false;
    for (const auto& word : MenuSearchWords(query))
        if (word == "owl" || word == "statue")
            return true;
    return false;
}
inline std::string NativeSearchPageLabel(bool oot, const MenuDestination& destination, const std::string& query) {
    return IsOwlEditorSearch(oot, destination, query) ? "Owl statues" : destination.page;
}
inline int NativeSearchPageTarget(bool oot, const MenuDestination& destination, const std::string& query) {
    if (IsOwlEditorSearch(oot, destination, query))
        return 1;
    if (destination.scope != "tools" || destination.page != MenuPage::SaveEditor)
        return -1;
    for (const auto& word : MenuSearchWords(query))
        if ((oot && (word == "oot" || word == "ocarina")) || (!oot && (word == "mm" || word == "majora")))
            return oot ? 0 : 1;
    return -1; // unqualified feature links retain the user's current world selection
}
// One catalog defines ordering, page explanations and search aliases for both engines.
struct RandomizerPageInfo {
    const char* name;
    const char* help;
    const char* keywords;
};
inline constexpr RandomizerPageInfo kRandomizerPages[] = {
    { "Generate", "Create a seed using your chosen settings.", "seed generate creation progress retry cancel" },
    { "Logic & access", "Choose the goal, reachability and world access rules.",
      "goal Triforce hunt starting world reachability guarantee access bosses" },
    { "Items & checks", "Choose randomized locations and extra items.",
      "check pool item pool shuffles shops merchants scenery drops abilities upgrades souls songs plentiful" },
    { "Shared items", "One item can count for both games.",
      "shared items bows bombs magic arrows wallets hookshot masks songs" },
    { "Starting inventory", "Choose your starting items and resources.",
      "starting items equipment hearts consumables" },
    { "Warps & travel", "Choose cross-game song travel and shuffled destinations.",
      "warp crossgame cross game songs transport transportation adult child Soaring owl Requiem" },
    { "Gameplay rules", "Choose abilities and quest requirements the seed logic can require.",
      "fierce deity time travel age blue fire sunlight tunics water hopping rolling required count frogs eggs Gibdo "
      "dog race" },
    { "Dungeons", "Choose dungeon item placement and dungeon rules.",
      "small keys boss keys keyrings maps compass fairies master quest" },
    { "Hints & traps", "Choose hints and disguised traps.", "gossip stone hints ice traps junk" },
    { "Excluded checks", "Choose checks that should not hold progression.",
      "locations check exclusions excluded checks" },
    { "Tricks & glitches", "Choose optional techniques the seed may require.", "tricks glitches difficulty" },
    { "Plandomizer", "Place specific items in a seed.", "planned placements plando" },
};
inline const char* GameplayRulePage(const char* name, bool randomizer) {
    if (IsSongTravelRule(name))
        return MenuPage::WarpsTravel; // the same page name on both tabs
    if (randomizer)
        return "Gameplay rules";
    // The in-game half sits in its feature's heading; a rule the catalogue lacks keeps the old pages.
    for (bool oot : { true, false })
        if (const auto* row = FindRulePlacement(name, oot, false))
            return row->page;
    const std::string key = name ? name : "";
    if (key.find("DifficultyOptions") != std::string::npos || key.find("ZoraEggCount") != std::string::npos ||
        key.find("AlwaysWinDoggyRace") != std::string::npos)
        return MenuPage::Difficulty;
    if (key.find("TimeTravel") != std::string::npos || key.find("OpenAllHours") != std::string::npos ||
        key.find("DayGravePull") != std::string::npos || key.find("NightGSAlwaysSpawn") != std::string::npos ||
        key.find("SkipBalladOfWindfish") != std::string::npos || key.find("SkipScarecrowSong") != std::string::npos)
        return MenuPage::TimeSavers;
    return MenuPage::ItemsEquipment;
}
// The heading a rule's row sits under on that page ("" when the catalogue has no row for it).
inline const char* GameplayRuleHeading(const char* name, bool randomizer) {
    for (bool oot : { true, false })
        if (const auto* row = FindRulePlacement(name, oot, randomizer))
            return row->heading ? row->heading : "";
    return "";
}
inline int RandomizerPageOrder(const std::string& page) {
    for (int i = 0; i < int(sizeof(kRandomizerPages) / sizeof(kRandomizerPages[0])); ++i)
        if (page == kRandomizerPages[i].name)
            return i;
    return 100; // Retain unknown upstream pages after the known workflow.
}
inline const char* RandomizerPageHelp(const std::string& page) {
    for (const auto& info : kRandomizerPages)
        if (page == info.name)
            return info.help;
    return "Configure this world's options for the next combined seed.";
}
// A widget belongs to exactly one subpage. Indices/callbacks remain those of the native menu.
inline int RandomizerWidgetGroup(bool oot, const std::string& sidebar, int column, const std::string& heading,
                                 const std::string& key = {}) {
    if (oot && sidebar == "Logic/Access" &&
        (key == "gRandoSettings.CompleteMaskQuest" || key == "gRandoSettings.ItemPool" ||
         heading == "Item Pool Settings"))
        return 1;
    if (oot && sidebar == "Dungeons" && key == "gRandoSettings.ShuffleBossSouls")
        return 1;
    if (oot && sidebar == "Shuffles")
        return column < 2 ? 0 : 1;
    if (!oot && sidebar == "Check Pool")
        return 0;
    if (!oot && sidebar == "Item Pool")
        return 1;
    if (oot && sidebar == "Hints/Traps")
        return heading == "Traps" ? 1 : 0;
    if (!oot && sidebar == "Hints")
        return 0;
    if (!oot && sidebar == "Traps")
        return 1;
    return -1;
}
// Keep native widget callbacks while regrouping the OoT location pool like MM's cards.
inline int OotLocationPoolGroup(const std::string& key, int originalColumn) {
    if (originalColumn == 1)
        return 3; // shops, prices and merchants belong together
    for (const char* part : { "ShuffleFreestanding", "ShuffleWonderItems", "ShufflePots", "ShuffleGrass",
                              "ShuffleCrates", "ShuffleBoulders", "ShuffleRocks", "ShuffleTrees", "ShuffleBushes",
                              "ShuffleIcicles", "ShuffleRedIce", "ShuffleSigns" })
        if (key.find(part) != std::string::npos)
            return 2;
    for (const char* part : { "ShuffleTokens", "Fishsanity", "ShuffleBeehives", "Fairies" })
        if (key.find(part) != std::string::npos)
            return 1;
    return 0;
}
// OoT's item pool under MM's Items headings: 0 Item pool (pool size first, as MM), 1 Abilities & upgrades, 2 Souls.
inline int OotItemPoolGroup(const std::string& key) {
    if (key == "gRandoSettings.ItemPool" || key == "gRandoSettings.CompleteMaskQuest")
        return 0;
    for (const char* soul :
         { "gRandoSettings.SoulsEnemyOot", "gRandoSettings.ShuffleBeanSouls", "gRandoSettings.ShuffleBossSouls" })
        if (key == soul)
            return 2;
    return 1;
}
} // namespace ComboRando
