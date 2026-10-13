#include "ComboNavigation.h"
#include "ComboRandomizerLayout.h"
#include "ComboTricks.h"
#include "../rando/MmTricks.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using ComboRando::MenuRoute;
int main() {
    // Searches for the shared editors must reach their common page, rather than
    // depending on either game's unrelated legacy widget names.
    using ComboRando::MenuSearchMatches;
    const char* sharedQueries[][3] = {
        { "screen fit", "media", "Screen & HUD" },
        { "automatic fitter", "media", "Screen & HUD" },
        { "HUD layout", "media", "Screen & HUD" },
        { "HUD placement", "media", "Screen & HUD" },
        { "menu scale", "settings", "Interface" },
        { "D-pad equips", "settings", "Controls" },
        { "Dpad", "settings", "Controls" },
        { "dpad equips", "settings", "Controls" },
        { "enhancement only", "gameplay", "Items & equipment" },
        { "Shadow Door Unload", "gameplay", "Restorations" },
        { "Early Eyeball Frog", "gameplay", "Restorations" },
        { "side panel", "trackers", "Check Tracker" },
        { "icon spacing", "trackers", "Item Tracker" },
        { "co-op", "tools", "Multiplayer" },
        { "placement HUD", "media", "Screen & HUD" },
        { "fit SCREEN", "media", "Screen & HUD" },
        { "pool items", "randomizer", "Items & checks" },
        // Topics whose rows moved to a shared page.
        { "climb", "gameplay", "Time savers" },
        { "push block", "gameplay", "Time savers" },
        { "owl warp", "gameplay", "Warps & travel" },
        { "accessibility", "settings", "General" },
        { "notifications", "settings", "Notifications" },
        { "input lag", "settings", "Controls" },
        { "trap effects", "gameplay", "Extra modes" },
        { "containers", "gameplay", "Randomizer extras" },
    };
    for (const auto& query : sharedQueries) {
        bool found = false;
        for (const auto& page : ComboRando::kSharedPages) {
            found |= MenuSearchMatches(query[0], page.name, page.keywords) &&
                std::string(page.scope) == query[1] && std::string(page.name) == query[2];
        }
        for (const auto& page : ComboRando::kRandomizerPages) {
            found |= MenuSearchMatches(query[0], page.name, page.keywords) &&
                std::string(query[1]) == "randomizer" && std::string(page.name) == query[2];
        }
        if (!found) std::cerr << "Missing shared page alias: " << query[0] << '\n';
        assert(found);
    }
    assert(ComboRando::NormalizeMenuSearch(" HUD\tLAYOUT\n") == "hudlayout");
    assert(ComboRando::NormalizeMenuSearch("A\xFF").size() == 2);
    assert(!MenuSearchMatches("", "Screen & HUD", "screen fit"));
    assert(!MenuSearchMatches("never-existing-setting", "Screen & HUD", "screen fit"));
    for (const auto& page : ComboRando::kSharedPages) {
        assert(MenuSearchMatches(page.name, page.name, page.keywords));
        assert(*page.scope && *page.scopeLabel && *page.keywords);
        assert(!MenuSearchMatches("completely unrelated", page.name, page.keywords));
    }
    for (const auto& trick : ComboRando::kMmTricks) {
        const auto keywords = std::string(trick.description) + " " + trick.area + " " + trick.tags + " technique trick";
        assert(MenuSearchMatches(trick.name, trick.name, keywords.c_str()));
        assert(MenuSearchMatches(trick.area, trick.name, keywords.c_str()));
        assert(MenuSearchMatches("technique", trick.name, keywords.c_str()));
        assert(!MenuSearchMatches("completely unrelated", trick.name, keywords.c_str()));
    }
    // Opening a search result clears incompatible catalogue filters once and
    // preserves subsequent user edits, including safely truncated long names.
    ComboRando::Detail::TrickCatalogueFocus focus;
    char catalogueQuery[160] = "old search";
    int world = 2, status = 1, skill = 6;
    focus.Set("Early Eyeball Frog");
    assert(focus.Apply(catalogueQuery, world, status, skill));
    assert(std::string(catalogueQuery) == "Early Eyeball Frog");
    assert(world == 0 && status == 0 && skill == 0);
    catalogueQuery[0] = 'X'; world = 1; status = 2; skill = 4;
    assert(!focus.Apply(catalogueQuery, world, status, skill));
    assert(catalogueQuery[0] == 'X' && world == 1 && status == 2 && skill == 4);
    for (const auto& trick : ComboRando::kMmTricks) {
        focus.Set(trick.name);
        assert(focus.Apply(catalogueQuery, world, status, skill));
        assert(std::string(catalogueQuery) == trick.name);
    }
    const std::string longName(240, 'a');
    focus.Set(longName.c_str());
    assert(focus.Apply(catalogueQuery, world, status, skill));
    assert(std::string(catalogueQuery) == longName.substr(0, 159));
    assert(catalogueQuery[159] == '\0');
    focus.Set(nullptr);
    assert(!focus.Apply(catalogueQuery, world, status, skill));
    // Both engines must meet at the same feature, despite their differing labels.
    const char* pairs[][3] = {
        {"Quality of Life", "Gameplay", "Quality of life"},
        {"Skips & Speed-ups", "Time Savers", "Time savers"},
        {"Items", "Items/Songs", "Items & equipment"},
        {"Difficulty", "Difficulty Options", "Difficulty"},
        {"Cosmetics Editor", "Cosmetic Editor", "Appearance"},
    };
    for (const auto& pair : pairs) {
        const auto a = MenuRoute(true, "Enhancements", pair[0]);
        const auto b = MenuRoute(false, "Enhancements", pair[1]);
        assert(a.scope == (std::string(pair[2]) == "Appearance" ? "media" : "gameplay") && b.scope == a.scope && a.page == pair[2] && b.page == a.page);
    }
    assert(MenuRoute(true, "Randomizer", "Locations").page ==
           MenuRoute(false, "Rando", "Check Exclusions").page);
    assert(MenuRoute(true, "Randomizer", "Logic/Access").page ==
           MenuRoute(false, "Rando", "Logic/Conditions").page);
    assert(MenuRoute(true, "Randomizer", "Hints/Traps").page == MenuRoute(false, "Rando", "Hints").page);
    assert(MenuRoute(true, "Randomizer", "Shuffles").page == MenuRoute(false, "Rando", "Check Pool").page);
    assert(MenuRoute(false, "Rando", "Item Pool").page == "Items & checks");
    assert(MenuRoute(false, "Rando", "Item Pool").page == MenuRoute(false, "Rando", "Check Pool").page);
    assert(MenuRoute(true, "Randomizer", "Starting Items").page == "Starting inventory");
    assert(MenuRoute(false, "Rando", "Starting Items").page == "Starting inventory");
    assert(MenuRoute(false, "Rando", "Dungeon Items").page == MenuRoute(true, "Randomizer", "Dungeons").page);
    assert(MenuRoute(false, "Rando", "Traps").page == MenuRoute(true, "Randomizer", "Hints/Traps").page);
    for (bool oot : {false, true}) {
        const auto extras = MenuRoute(oot, oot ? "Randomizer" : "Rando", "General");
        assert(extras.scope == "gameplay" && extras.page == "Randomizer extras");
    }
    // Each source belongs to one topic, preserving all columns and both engines' controls.
    using ComboRando::RandomizerWidgetGroup;
    assert(RandomizerWidgetGroup(true, "Shuffles", 0, "Shuffle Items") == 0);
    assert(RandomizerWidgetGroup(true, "Shuffles", 1, "Shuffle Shops & Merchants") == 0);
    assert(RandomizerWidgetGroup(true, "Shuffles", 2, "Additional Items") == 1);
    assert(RandomizerWidgetGroup(false, "Check Pool", 0, "") == 0);
    assert(RandomizerWidgetGroup(false, "Item Pool", 0, "") == 1);
    assert(RandomizerWidgetGroup(true, "Hints/Traps", 0, "Hints") == 0);
    assert(RandomizerWidgetGroup(true, "Hints/Traps", 0, "Traps") == 1);
    assert(RandomizerWidgetGroup(true, "Hints/Traps", 1, "Static Hints") == 0);
    assert(RandomizerWidgetGroup(false, "Hints", 0, "") == 0);
    assert(RandomizerWidgetGroup(false, "Traps", 0, "") == 1);
    assert(RandomizerWidgetGroup(false, "Future page", 0, "") == -1);
    // Native OoT's broad Logic/Access sidebar is split by setting topic in Combo Ship.
    using ComboRando::MenuGroupRoute;
    for (const char* key : {"gRandoSettings.CompleteMaskQuest", "gRandoSettings.ItemPool"}) {
        assert(MenuGroupRoute(true, "Randomizer", "Logic/Access", 0, "Logic", key).page == "Items & checks");
        assert(RandomizerWidgetGroup(true, "Logic/Access", 0, "Logic", key) == 1);
    }
    assert(MenuGroupRoute(true, "Randomizer", "Logic/Access", 0, "Logic", "gRandoSettings.FullWallets").page ==
           "Starting inventory");
    // Starting Age sits with Starting world, which greys it out; OoT's own destination shuffles sit with song travel.
    assert(MenuGroupRoute(true, "Randomizer", "Logic/Access", 0, "Logic", "gRandoSettings.StartingAge").page ==
           "Logic & access");
    for (const char* key : {"gRandoSettings.ShuffleWarpSongs", "gRandoSettings.ShuffleOwlDrops"})
        assert(MenuGroupRoute(true, "Randomizer", "Logic/Access", 2, "Entrances", key).page == "Warps & travel");
    assert(MenuGroupRoute(true, "Randomizer", "Logic/Access", 2, "Entrances",
                          "gRandoSettings.ShuffleDungeonsEntrances").page == "Logic & access");
    // Boss Souls joins the other souls on Items & checks > Items; the rest of Dungeons stays.
    assert(MenuGroupRoute(true, "Randomizer", "Dungeons", 0, "Dungeon Items", "gRandoSettings.ShuffleBossSouls").page ==
           "Items & checks");
    assert(RandomizerWidgetGroup(true, "Dungeons", 0, "Dungeon Items", "gRandoSettings.ShuffleBossSouls") == 1);
    assert(MenuGroupRoute(true, "Randomizer", "Dungeons", 0, "Dungeon Items", "gRandoSettings.Keysanity").page ==
           "Dungeons");
    {
        // OoT rows take the page's fixed headings at draw time; native separators are replaced, keys and indices stay.
        auto row = [](CwKind kind, const char* name, const char* cvar, int column) {
            CwWidget w{};
            w.kind = kind; w.name = name; w.cvar = cvar; w.column = column; w.index = column * 100 + int(std::strlen(name));
            return w;
        };
        auto names = [](const std::vector<CwWidget>& rows) {
            std::vector<std::string> out;
            for (const auto& w : rows) out.push_back(w.name);
            return out;
        };
        using List = std::vector<std::string>;
        const CwWidget logic[] = {
            row(CW_SEPARATOR_TEXT, "Logic", "", 0), row(CW_COMBOBOX, "Logic", "gRandoSettings.LogicRules", 0),
            row(CW_COMBOBOX, "Starting Age", "gRandoSettings.StartingAge", 0),
            row(CW_SEPARATOR_TEXT, "Win Condition", "", 0), row(CW_COMBOBOX, "Ganon's Boss Key", "gRandoSettings.ShuffleGanonBossKey", 0),
            row(CW_SEPARATOR_TEXT, "Area Access", "", 1), row(CW_COMBOBOX, "Forest", "gRandoSettings.ClosedForest", 1),
            row(CW_SEPARATOR_TEXT, "Entrances", "", 2), row(CW_CHECKBOX, "Dungeon Entrances", "gRandoSettings.ShuffleDungeonsEntrances", 2),
        };
        const auto arranged = ComboRando::ArrangeOotRandomizerPage("Logic & access", logic, 9);
        assert((names(arranged) == List{"Logic", "Logic", "Area access", "Starting Age", "Forest", "Final boss access",
                                        "Ganon's Boss Key", "Entrances", "Dungeon Entrances"}));
        assert(arranged[0].index == -1 && arranged[0].kind == CW_SEPARATOR_TEXT && arranged[3].index == logic[2].index);
        for (int i = 0; i < int(arranged.size()); ++i) assert(arranged[i].column == (i >= 5)); // balanced wide columns
        const CwWidget items[] = {
            row(CW_SEPARATOR_TEXT, "Item Pool Settings", "", 0), row(CW_COMBOBOX, "Item Pool", "gRandoSettings.ItemPool", 0),
            row(CW_SEPARATOR_TEXT, "Additional Items", "", 0), row(CW_CHECKBOX, "Enemy Souls (OoT)", "gRandoSettings.SoulsEnemyOot", 0),
            row(CW_CHECKBOX, "Shuffle Swim", "gRandoSettings.ShuffleSwim", 0),
            row(CW_COMBOBOX, "Shuffle Boss Souls", "gRandoSettings.ShuffleBossSouls", 0),
        };
        assert((names(ComboRando::ArrangeOotRandomizerPage("Items & checks", items, 6)) ==
                List{"Item pool", "Item Pool", "Abilities & upgrades", "Shuffle Swim", "Souls", "Enemy Souls (OoT)",
                     "Shuffle Boss Souls"}));
        const CwWidget dungeons[] = {
            row(CW_SEPARATOR_TEXT, "Dungeon Items", "", 0), row(CW_COMBOBOX, "Maps/Compasses", "gRandoSettings.StartingMapsCompasses", 0),
            row(CW_COMBOBOX, "Small Key Shuffle", "gRandoSettings.Keysanity", 0),
            row(CW_COMBOBOX, "Boss Key Shuffle", "gRandoSettings.BossKeysanity", 0),
            row(CW_COMBOBOX, "Shuffle Dungeon Rewards", "gRandoSettings.ShuffleDungeonReward", 0),
            row(CW_SEPARATOR_TEXT, "Keyrings", "", 1), row(CW_COMBOBOX, "Key Rings", "gRandoSettings.ShuffleKeyRings", 1),
            row(CW_SEPARATOR_TEXT, "Master Quest", "", 2), row(CW_COMBOBOX, "Master Quest Dungeons", "gRandoSettings.MQDungeons", 2),
        };
        assert((names(ComboRando::ArrangeOotRandomizerPage("Dungeons", dungeons, 9)) ==
                List{"Dungeon rewards", "Shuffle Dungeon Rewards", "Maps & compasses", "Maps/Compasses", "Small keys",
                     "Small Key Shuffle", "Key rings", "Key Rings", "Boss keys", "Boss Key Shuffle", "Master Quest",
                     "Master Quest Dungeons"}));
        const CwWidget warps[] = { row(CW_CHECKBOX, "Warp Songs", "gRandoSettings.ShuffleWarpSongs", 2) };
        const auto travel = ComboRando::ArrangeOotRandomizerPage("Warps & travel", warps, 1);
        assert(travel.size() == 2 && std::string(travel[0].name) == "Destination shuffles (OoT)" && travel[1].column == 0);
        // A page without a heading list keeps its rows, order and columns.
        const auto same = ComboRando::ArrangeOotRandomizerPage("Gameplay rules", logic, 9);
        assert(same.size() == 9 && same[4].column == 0 && same[5].column == 1 && same[0].kind == CW_SEPARATOR_TEXT);
        for (const auto& page : ComboRando::kRandomizerPageHeadings)
            assert(ComboRando::RandomizerPageOrder(page.page) < 100);
    }
    using ComboRando::OotItemPoolGroup;
    assert(OotItemPoolGroup("gRandoSettings.ItemPool") == 0 && OotItemPoolGroup("gRandoSettings.CompleteMaskQuest") == 0);
    assert(OotItemPoolGroup("gRandoSettings.ShuffleBossSouls") == 2 && OotItemPoolGroup("gRandoSettings.SoulsEnemyOot") == 2);
    assert(OotItemPoolGroup("gRandoSettings.ShuffleSwim") == 1 && OotItemPoolGroup("gRandoSettings.SkeletonKey") == 1);
    for (const char* key : {"gRandoSettings.BigPoeTargetCount", "gRandoSettings.SlingBowBeehives",
                           "gRandoSettings.SkipChildStealth", "gRandoSettings.EarlyGrannysShop",
                           "gRandoSettings.SkipPlantingBeans", "gRandoSettings.SkipEponaRace",
                           "gRandoSettings.SkipScarecrowsSong", "gRandoSettings.SwordlessEponaItems"})
        assert(MenuGroupRoute(true, "Randomizer", "Logic/Access", 0, "Logic", key).page == "Gameplay rules");
    for (const char* key : {"gRandoSettings.LogicRules", "gRandoSettings.ClosedForest",
                           "gRandoSettings.GanonTrial", "gRandoSettings.RainbowBridge"})
        assert(MenuGroupRoute(true, "Randomizer", "Logic/Access", 0, "Logic", key).page == "Logic & access");
    int expectedOrder = 0;
    for (const auto& page : ComboRando::kRandomizerPages) {
        assert(ComboRando::RandomizerPageOrder(page.name) == expectedOrder++);
        assert(std::string(ComboRando::RandomizerPageHelp(page.name)) == page.help);
        assert(std::string(page.keywords).size() > 0);
    }
    assert(ComboRando::RandomizerPageOrder("Future page") > expectedOrder);
    for (const auto* tracker : {"Check Tracker", "Item Tracker", "Hint Tracker"}) {
        assert(MenuRoute(true, "Randomizer", tracker).scope.empty()); // Shared implementation replaces it.
        assert(MenuRoute(false, "Rando", tracker).scope.empty());
    }
    assert(MenuRoute(true, "Randomizer", "Entrance Tracker").scope == "trackers");
    assert(MenuRoute(false, "Settings", "Overlay").scope == "media");
    assert(MenuRoute(true, "Settings", "Presets").page == MenuRoute(false, "Settings", "Presets").page);
    // Renamed pages keep their old names as search words; MM's overlay sidebar now only holds notifications.
    assert(MenuRoute(false, "Settings", "Presets").page == ComboRando::MenuPage::EnhancementPresets);
    const auto presets = ComboRando::NativeSearchPageRoute(false, "Settings", "Presets");
    assert(MenuSearchMatches("saved presets", presets.page.c_str(),
                             ComboRando::NativeSearchPageKeywords("Settings", "Presets", presets, false).c_str()));
    for (const auto& page : ComboRando::kSharedPages)
        if (std::string(page.scope) == "home")
            assert(std::string(page.name) == "Community presets" &&
                   MenuSearchMatches("presets", page.name, page.keywords));
    const auto overlay = ComboRando::NativeSearchPageRoute(false, "Settings", "Overlay");
    assert(overlay.scope == "settings" && overlay.page == "Notifications");
    // The Notifications rows of MM's overlay sidebar join OoT's Notifications page.
    assert(
        ComboRando::MenuGroupRoute(false, "Settings", "Overlay", 0, "Notifications", "gNotifications.Position").page ==
        "Notifications");
    assert(ComboRando::MenuGroupRoute(false, "Enhancements", "HUD Editor", 0, "", "gWindows.HudEditor").page ==
           "Screen & HUD");
    for (const auto* shared : {"Audio", "Graphics", "Controls", "Input Viewer"}) {
        assert(MenuRoute(true, "Settings", shared).scope == (std::string(shared) == "Audio" || std::string(shared) == "Graphics" ? "media" : "settings"));
        assert(MenuRoute(false, "Settings", shared).scope.empty());
    }
    assert(MenuRoute(false, "Dev Tools", "DL Viewer").page == MenuRoute(true, "Dev Tools", "DList Viewer").page);
    // A newly added upstream feature must not silently vanish from the app.
    assert(MenuRoute(false, "New section", "New feature").scope == "tools");
    assert(MenuRoute(false, "Enhancements", "New enhancement").scope == "gameplay");
    assert(MenuRoute(false, "Rando", "New rule").scope == "randomizer");
    assert(MenuRoute(false, "Network", "Crowd Control").scope.empty());
    assert(MenuRoute(true, "Settings", "Search").scope.empty());
    std::cout << "Feature navigation, shared replacements and fallback coverage passed\n";
}
