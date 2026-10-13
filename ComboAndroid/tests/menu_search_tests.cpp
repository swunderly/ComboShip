#include "gui/ComboMenuLabels.h"
#include "gui/ComboNavigation.h"
#include "rando/GameplayRules.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

using namespace ComboRando;

static int RuleRank(const char* query, const GameplayRule& rule) {
    const auto keywords = std::string(rule.help) + " enhancement logic " + GameplayRuleSearchAliases(rule.name);
    return MenuSearchRank(query, rule.label, keywords.c_str(), MenuSearchKind::Rule);
}
int main() {
    // Use the production registry, rather than copied song labels. Ordinary
    // language must reach actual controls instead of only the Generate page.
    const auto* foreign = FindGameplayRule(kSongTravelRuleKeys[0]);
    const auto* adult = FindGameplayRule("gEnhancements.DifficultyOptions.BremenChickCount");
    const auto* destinations = FindGameplayRule(kSongTravelRuleKeys[1]);
    assert(foreign && adult && destinations);
    const char* playbackQueries[] = {"cross songs", "cross warp", "cross-game songs", "cross game travel",
        "travel cross game", "Requiem", "owl", "SOARING song", "crossgame", "warp"};
    for (const auto* query : playbackQueries) {
        assert(RuleRank(query, *foreign) > 0);
        assert(!RuleRank((std::string(query) + " impossible-term").c_str(), *foreign));
    }
    for (const auto* query : {"adult warp", "warp adult", "adult soaring", "any AGE", "owl"})
        assert(RuleRank(query, *foreign) > 0);
    for (const auto* query : {"destination shuffle", "shuffle destinations", "mixed warp", "saved map"})
        assert(RuleRank(query, *destinations) > 0);
    assert(!RuleRank("destination shuffle", *foreign));
    assert(!RuleRank("adult warp", *destinations));
    assert(!RuleRank("Requiem", *destinations));
    for (const auto& page : kRandomizerPages) {
        if (std::strcmp(page.name, "Generate")) continue;
        const int generate = MenuSearchRank("warp", page.name, page.keywords, MenuSearchKind::Page);
        assert(generate == 0);
        assert(RuleRank("warp", *foreign) > generate);
        assert(RuleRank("warp", *destinations) > generate);
    }
    assert(MenuSearchRank("Check Tracker", "Check Tracker", "check collected", MenuSearchKind::Page) >
           MenuSearchRank("Check Tracker", "Some check option", "check tracker", MenuSearchKind::Setting));
    assert(MenuSearchScore("screen fit", "Screen & HUD", "automatic screen fitter") > 0);
    assert(MenuSearchScore("fit screen", "Screen & HUD", "automatic screen fitter") > 0);
    assert(MenuSearchScore("HUD placement", "Screen & HUD", "HUD layout placement") > 0);
    assert(MenuSearchScore("co-op", "Multiplayer", "co op shared progress") > 0);
    assert(MenuSearchScore("items checks", "Items & checks", "item pool check pool") > 0);
    assert(MenuSearchScore("bow magic", "Magic arrows", "requires a bow and magic") > 0);
    assert(MenuSearchScore("bow magic missing", "Magic arrows", "requires a bow and magic") == 0);
    for (const auto* empty : {"", " \t\n", "-- / &", "the and of"})
        assert(MenuSearchScore(empty, "Warp songs", "travel") == 0);
    assert(MenuSearchScore("warp", nullptr, nullptr) == 0);
    assert(MenuSearchWords("A\xFF").size() == 1); // unsigned byte safety
    assert(MenuSearchScore("screen screen", "Screen & HUD", "") > 0);
    assert(MenuSearchScore("on-screen", "Debug Warp Screen Translation", "position screen") == 0);
    assert(MenuSearchScore("MM", "Camera", "MM camera") > 0);
    assert(MenuSearchScore("MM", "Summary", "camera") == 0);
    for (const auto* query : {"CT", "IT", "HT"}) {
        const char* target = query[0] == 'C' ? "Check Tracker" : query[0] == 'I' ? "Item Tracker" : "Hint Tracker";
        for (const auto& page : kSharedPages)
            assert((MenuSearchRank(query, page.name, page.keywords, MenuSearchKind::Page) > 0) ==
                   (std::strcmp(page.name, target) == 0));
        assert(MenuSearchRank(query, "Show collected items", "It changes the selected item", MenuSearchKind::Setting) == 0);
    }
    // The Personal notes heading is on Item Tracker; no other shared page claims the words.
    for (const auto& page : kSharedPages)
        assert((MenuSearchRank("personal notes", page.name, page.keywords, MenuSearchKind::Page) > 0) ==
               (std::strcmp(page.name, "Item Tracker") == 0));

    // Native feature names are indexed as direct shared page links, including
    // custom editors which cannot safely render as ordinary search widgets.
    for (const auto* page : {"Audio", "Graphics", "Controls"}) {
        const auto destination = NativeSearchPageRoute(true, "Settings", page);
        const auto keywords = NativeSearchPageKeywords("Settings", page, destination);
        assert(destination.scope == (std::string(page) == "Controls" ? "settings" : "media") && destination.page == page);
        assert(MenuSearchScore((std::string("settings ") + page).c_str(), page, keywords.c_str()) > 0);
        assert(NativeSearchPageTarget(true, destination, "MM audio") == -1);
    }
    const auto controls = NativeSearchPageRoute(true, "Settings", "Controls");
    const auto controlsKeywords = NativeSearchPageKeywords("Settings", "Controls", controls);
    assert(MenuSearchMatches("controller mapping", controls.page.c_str(), controlsKeywords.c_str()));
    for (bool oot : {false, true}) {
        const auto hud = NativeSearchPageRoute(oot, "Enhancements", "HUD Editor");
        assert(hud.scope == "media" && hud.page == "Screen & HUD");
        const auto hudKeywords = NativeSearchPageKeywords("Enhancements", "HUD Editor", hud, oot);
        assert(MenuSearchMatches("HUD editor", hud.page.c_str(), hudKeywords.c_str()));
        assert(NativeSearchPageRoute(oot, "Network", "Anchor").scope.empty());
        assert(NativeSearchPageRoute(oot, "Settings", "Search").scope.empty());
        assert(NativeSearchPageRoute(oot, oot ? "Randomizer" : "Rando", "Check Tracker").scope.empty());
    }
    const auto mmEditor = NativeSearchPageRoute(false, "Dev Tools", "Save Editor");
    const auto mmEditorKeywords = NativeSearchPageKeywords("Dev Tools", "Save Editor", mmEditor, false);
    const auto ootEditorKeywords = NativeSearchPageKeywords("Dev Tools", "Save Editor", mmEditor, true);
    assert(mmEditor.scope == "tools" && mmEditor.page == "Save Editor");
    for (const auto* query : {"owl statues", "activate owl", "clear statues", "MM save editor"}) {
        const auto label = NativeSearchPageLabel(false, mmEditor, query);
        assert(MenuSearchRank(query, label.c_str(), mmEditorKeywords.c_str(), MenuSearchKind::Page) > 0);
        assert(NativeSearchPageTarget(false, mmEditor, query) == 1);
    }
    assert(NativeSearchPageLabel(false, mmEditor, "owl statues") == "Owl statues");
    const auto owlLabel = NativeSearchPageLabel(false, mmEditor, "owl statues");
    const int owlRank = MenuSearchRank("owl statues", owlLabel.c_str(), mmEditorKeywords.c_str(), MenuSearchKind::Page);
    assert(owlRank > RuleRank("owl statues", *foreign) && owlRank > RuleRank("owl statues", *adult));
    assert(!MenuSearchMatches("owl statues", "Save Editor", ootEditorKeywords.c_str()));
    assert(NativeSearchPageTarget(false, mmEditor, "save editor") == -1);
    assert(NativeSearchPageTarget(true, mmEditor, "save editor") == -1);
    assert(NativeSearchPageTarget(true, mmEditor, "OoT save editor") == 0);

    // Every registered rule is discoverable and grouping is complete, unique
    // and independent of registry order. No convenience flag becomes a trick.
    std::set<std::string> grouped;
    for (const auto* key : kSongTravelRuleKeys) {
        const auto* rule = FindGameplayRule(key);
        assert(rule && !rule->trickGoverned && IsSongTravelRule(rule->name));
        assert(grouped.insert(rule->name).second);
    }
    for (const auto& rule : kGameplayRules) {
        if (IsLegacyGameplayRule(rule.name)) continue;
        assert(RuleRank(rule.label, rule) > 0);
        assert(!RuleRank("nonexistent bank platform xyzzy", rule));
        if (!rule.trickGoverned && !IsSongTravelRule(rule.name)) assert(grouped.insert(rule.name).second);
        if (rule.trickGoverned) assert(!IsSongTravelRule(rule.name));
        // A rule shows its own label unless the label table names its CVar (one name for both games).
        const auto* shared = FindMenuLabel(!std::strstr(rule.label, "(MM)"), rule.name);
        const char* expected = shared && shared->label ? shared->label : rule.label;
        if (std::strcmp(rule.name, destinations->name))
            assert(!std::strcmp(GameplayRuleMenuLabel(rule.name, rule.label, true), expected));
        assert(!std::strcmp(GameplayRuleMenuLabel(rule.name, rule.label, false), expected));
    }
    std::size_t ordinary = 0;
    for (const auto& rule : kGameplayRules) ordinary += !rule.trickGoverned && !IsLegacyGameplayRule(rule.name);
    assert(grouped.size() == ordinary);
    assert(std::string(GameplayRuleMenuLabel(destinations->name, destinations->label, true)) ==
           "Warp Songs and Soaring Spots Shuffle");
    // MM's scarecrow rule shows OoT's name for the same feature on both tabs; its own words still find it.
    const auto* scarecrow = FindGameplayRule("gEnhancements.Playback.SkipScarecrowSong");
    assert(scarecrow && std::strstr(scarecrow->label, "(MM)"));
    for (bool randomizer : { true, false })
        assert(std::string(GameplayRuleMenuLabel(scarecrow->name, scarecrow->label, randomizer)) ==
               MenuRowLabel(true, "gEnhancements.InstantScarecrow", "Skip Playing Scarecrow's Song"));
    const auto scarecrowWords = std::string(scarecrow->help) + " " + scarecrow->label;
    assert(MenuSearchRank("teaching a song", "Skip Scarecrow's Song (MM)", scarecrowWords.c_str(),
                          MenuSearchKind::Rule) > 0);
    for (const auto* query : {"shuffle song warp destinations", "soaring spots"})
        assert(RuleRank(query, *destinations) > 0); // the earlier label and OoTMM's name both find it
    assert(!IsSongTravelRule(nullptr) && !IsSongTravelRule("gCheats.Unrelated"));
    assert(std::string(GameplayRuleSearchAliases(nullptr)).empty());

    // Focus survives the wrong row, scrolls exactly once, and owns a safe copy
    // of its target. The eventual control retains its canonical setting key.
    MenuRuleFocus focus;
    {
        std::string temporary = adult->name;
        focus.Set(temporary.c_str());
    }
    assert(!focus.ConsumeScrollFor(foreign->name));
    assert(!focus.Matches(foreign->name) && focus.Matches(adult->name));
    assert(focus.ConsumeScrollFor(adult->name));
    assert(!focus.ConsumeScrollFor(adult->name) && focus.Matches(adult->name));
    focus.Set(destinations->name);
    assert(focus.ConsumeScrollFor(destinations->name));
    focus.Clear(); assert(!focus.Matches(destinations->name));
    focus.Set(nullptr); assert(!focus.ConsumeScrollFor(nullptr));

    // Returning restores the user's spelling and original feature, even when
    // the result lives in another tab. Further browsing does not replace it
    // with an empty query; explicit clearing ends the return path.
    MenuSearchReturn history;
    history.Capture(" Adult   WARP ", "settings", "settings/Screen & HUD");
    assert(history.HasResult() && history.query == " Adult   WARP ");
    assert(history.scope == "settings" && history.hub == "settings/Screen & HUD");
    history.Capture("--", "randomizer", "randomizer/Gameplay rules");
    assert(history.scope == "settings" && history.HasResult());
    history.Clear(); assert(!history.HasResult() && history.scope.empty() && history.hub.empty());

    // A setting is found by the label its page shows, and still by its old native name. Both games'
    // climb speed rows answer "climb speed" equally, and each lands on Time savers > Speed-ups.
    auto settingRank = [](const char* query, bool oot, const char* cvar, const char* name) {
        const auto label = MenuRowLabel(oot, cvar, name);
        const auto keywords = MenuRowKeywords(oot, cvar, name);
        return MenuSearchRank(query, label.c_str(), keywords.c_str(), MenuSearchKind::Setting);
    };
    const int ootClimb = settingRank("climb speed", true, "gEnhancements.ClimbSpeed", "Vine/Ladder Climb Speed +%d");
    const int mmClimb = settingRank("climb speed", false, "gEnhancements.Player.ClimbSpeed", "Climb speed");
    assert(ootClimb > 0 && ootClimb == mmClimb);
    assert(settingRank("vine ladder", true, "gEnhancements.ClimbSpeed", "Vine/Ladder Climb Speed +%d") > 0);
    assert(settingRank("trap behavior", false, "gRando.TrapItems", "Trap appearance") > 0);
    assert(settingRank("screen flash", true, "gSettings.A11yNoScreenFlashForFinishingBlow",
                       "Disable Screen Flash for Finishing Blow") ==
           settingRank("screen flash", false, "gEnhancements.A11y.NoScreenFlashForEnemyKill",
                       "Disable Screen Flash for Enemy Kills"));
    for (bool oot : { true, false }) {
        const auto* row =
            FindMenuPlacement(oot, "", "", "", oot ? "gEnhancements.ClimbSpeed" : "gEnhancements.Player.ClimbSpeed");
        assert(row && std::string(row->page) == "Time savers" && std::string(row->heading) == "Speed-ups");
    }
    // A rule result names the heading of the half it opens.
    assert(std::string(GameplayRuleHeading("gEnhancements.OpenAllHours", false)) == "Time of day & cycle");
    assert(std::string(GameplayRuleHeading("gEnhancements.OpenAllHours", true)) == "Time of day");

    // Each feature has its own guide; unrelated tabs have none.
    for (const auto& guide : kMenuGuides) {
        assert(FindMenuGuide(guide.scope) == &guide);
        assert(*guide.introduction && *guide.behavior && *guide.runWarning);
    }
    for (const auto* scope : {"home", "settings", "trackers", "tools", ""}) assert(!FindMenuGuide(scope));
    assert(std::string(FindMenuGuide("randomizer")->introduction).find("next seed") != std::string::npos);
    assert(std::string(FindMenuGuide("gameplay")->behavior).find("Randomizer") != std::string::npos);
    std::cout << "Menu search: actual rule aliases/ranking, independent word matching, complete song grouping, focus, history and guide metadata passed\n";
}
