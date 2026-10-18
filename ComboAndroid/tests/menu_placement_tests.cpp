// One feature, one place: every native menu row of both games sits on a page and heading from
// combo/gui/ComboMenuPlacement.h, the rows a feature shares land together in both world views, and no CVar a menu
// offered before the T8 audit lost its widget. The menus are read from the games' own registration sources, the same
// COMBO_BUILD branches the Android build compiles.
#include "ComboMenuLabels.h"
#include "ComboNavigation.h"
#include "ComboRandomizerLayout.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef COMBO_REPO_DIR
#error "COMBO_REPO_DIR must name the repository checkout"
#endif

using namespace ComboRando;

namespace {
std::string Read(const std::string& relative) {
    std::ifstream in(std::string(COMBO_REPO_DIR) + "/" + relative, std::ios::binary);
    if (!in) {
        std::cerr << "Missing menu source " << relative << '\n';
        assert(false);
    }
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}
// The text from start up to end, or through the brace block that follows start when end is empty.
std::string Region(const std::string& text, const char* start, const char* end = nullptr) {
    const auto first = text.find(start);
    assert(first != std::string::npos);
    if (end) {
        const auto last = text.find(end, first);
        assert(last != std::string::npos);
        return text.substr(first, last - first);
    }
    auto at = text.find('{', first) + 1;
    for (int depth = 1; depth; ++at)
        depth += (text[at] == '{') - (text[at] == '}');
    return text.substr(first, at - first);
}
// Lines the Android build compiles: COMBO_BUILD branches; other conditions count as compiled.
std::string Active(const std::string& text) {
    std::istringstream in(text);
    std::string line, out;
    std::vector<int> stack; // 1 compiled, 0 skipped, -1 other condition (both branches kept)
    while (std::getline(in, line)) {
        const auto start = line.find_first_not_of(" \t");
        if (start != std::string::npos && line[start] == '#') {
            auto directive = line.substr(line.find_first_not_of(" \t#", start));
            if (directive.rfind("ifdef COMBO_BUILD", 0) == 0) {
                stack.push_back(1);
                continue;
            }
            if (directive.rfind("ifndef COMBO_BUILD", 0) == 0) {
                stack.push_back(0);
                continue;
            }
            if (directive.rfind("if", 0) == 0) {
                stack.push_back(-1);
                continue;
            }
            if (directive.rfind("el", 0) == 0) {
                if (stack.back() >= 0)
                    stack.back() = 1 - stack.back();
                continue;
            }
            if (directive.rfind("endif", 0) == 0) {
                stack.pop_back();
                continue;
            }
        }
        bool compiled = true;
        for (int state : stack)
            compiled &= state != 0;
        if (compiled)
            out += line + '\n';
    }
    return out;
}
// The CVar of every .CVar(...) call, with the games' prefix macros expanded. An expression that is not a literal (a
// loop over a table) is returned in angle brackets.
std::vector<std::string> CVars(const std::string& text) {
    static const std::map<std::string, std::string> prefixes = {
        { "CVAR_ENHANCEMENT", "gEnhancements" },
        { "CVAR_CHEAT", "gCheats" },
        { "CVAR_RANDOMIZER_ENHANCEMENT", "gRandoEnhancements" },
        { "CVAR_RANDOMIZER_SETTING", "gRandoSettings" },
        { "CVAR_SETTING", "gSettings" },
        { "CVAR_WINDOW", "gOpenWindows" },
        { "CVAR_TIME_DISPLAY", "gTimeDisplay" },
        { "CVAR_GENERAL", "gGeneral" },
        { "CVAR_GAMEPLAY_STATS", "gGameplayStats" },
        { "CVAR_AUDIO", "gAudioEditor" },
        { "CVAR_COSMETIC", "gCosmetics" },
        { "CVAR_DEVELOPER_TOOLS", "gDeveloperTools" },
        { "CVAR_TRACKER", "gTrackers" },
        { "CVAR_REMOTE", "gRemote" },
    };
    // CMake/lus-cvars.cmake
    static const std::map<std::string, std::string> macros = {
        { "CVAR_LOW_RES_MODE", "gSettings.LowResMode" },
        { "CVAR_SIMULATED_INPUT_LAG", "gSettings.SimulatedInputLag" },
        { "CVAR_VSYNC_ENABLED", "gSettings.VsyncEnabled" },
        { "CVAR_Z_FIGHTING_MODE", "gSettings.ZFightingMode" },
        { "CVAR_INTERNAL_RESOLUTION", "gSettings.InternalResolution" },
        { "CVAR_MSAA_VALUE", "gSettings.MSAAValue" },
        { "CVAR_SDL_WINDOWED_FULLSCREEN", "gSettings.SdlWindowedFullscreen" },
        { "CVAR_TEXTURE_FILTER", "gSettings.TextureFilter" },
        { "CVAR_IMGUI_CONTROLLER_NAV", "gSettings.ControlNav" },
        { "CVAR_ENABLE_MULTI_VIEWPORTS", "gSettings.EnableMultiViewports" },
        { "CVAR_ALLOW_BACKGROUND_INPUTS", "gSettings.AllowBackgroundInputs" },
    };
    std::vector<std::string> found;
    for (auto at = text.find(".CVar("); at != std::string::npos; at = text.find(".CVar(", at)) {
        auto end = at + std::strlen(".CVar(");
        const auto start = end;
        for (int depth = 1; depth; ++end)
            depth += (text[end] == '(') - (text[end] == ')');
        std::string expression = text.substr(start, end - 1 - start);
        at = end;
        expression.erase(0, expression.find_first_not_of(" \t\r\n"));
        expression.erase(expression.find_last_not_of(" \t\r\n") + 1);
        const auto quote = expression.find('"');
        if (quote == 0 && expression.find('"', 1) == expression.size() - 1) {
            found.push_back(expression.substr(1, expression.size() - 2));
        } else if (quote != std::string::npos && prefixes.count(expression.substr(0, expression.find('(')))) {
            const auto close = expression.find('"', quote + 1);
            found.push_back(prefixes.at(expression.substr(0, expression.find('('))) + "." +
                            expression.substr(quote + 1, close - quote - 1));
        } else if (macros.count(expression)) {
            found.push_back(macros.at(expression));
        } else {
            found.push_back("<" + expression + ">");
        }
    }
    return found;
}
// The tooltip of the menu widget named name: the string literals inside the .Tooltip(...) of the statement that adds
// it, joined as the compiler joins them.
std::string WidgetTooltip(const std::string& menus, const std::string& name) {
    const auto at = menus.find("AddWidget(path, \"" + name + "\",");
    assert(at != std::string::npos);
    const auto open = menus.find(".Tooltip(", at);
    assert(open != std::string::npos && open < menus.find("AddWidget(", at + 1));
    std::string text;
    bool quoted = false;
    for (auto i = open + std::strlen(".Tooltip("); quoted || menus[i] != ')'; ++i) {
        if (menus[i] == '"') {
            quoted = !quoted;
        } else if (quoted) {
            if (menus[i] == '\\')
                text += menus[i++];
            text += menus[i];
        }
    }
    assert(!text.empty());
    return text;
}
std::string Joined(const std::vector<const char*>& files) {
    std::string text;
    for (const auto* file : files)
        text += Active(Read(file));
    return text;
}
const std::vector<const char*> kOotMenuFiles = {
    "soh/soh/SohGui/SohMenu.cpp",
    "soh/soh/SohGui/SohMenuDevTools.cpp",
    "soh/soh/SohGui/SohMenuEnhancements.cpp",
    "soh/soh/SohGui/SohMenuNetwork.cpp",
    "soh/soh/SohGui/SohMenuRandomizer.cpp",
    "soh/soh/SohGui/SohMenuSettings.cpp",
    "soh/soh/SohGui/ResolutionEditor.cpp",
    "soh/soh/Enhancements/ExtraModes/EnemyRandomizer.cpp",
    "soh/soh/Enhancements/Presets/Presets.cpp",
};
const std::vector<const char*> kMmMenuFiles = { "mm/2s2h/BenGui/BenMenu.cpp", "mm/2s2h/BenGui/ResolutionEditor.cpp",
                                                "mm/2s2h/Rando/Menu.cpp" };
bool IsCVarKey(const char* key) {
  for (const char *kind :
       {"widget:", "heading:", "sidebar:", "rule:", "pair:", "entrance:"})
    if (!std::strncmp(key, kind, std::strlen(kind)))
      return false;
  return true;
}
const MenuPlacement* CVarPlacement(bool oot, const std::string& cvar) {
    if (cvar.find("randomizedEnemySpawnTable") != std::string::npos)
        return FindMenuPlacement(oot, "Enhancements", "Extra Modes", "", "gEnhancements.RandomizedEnemyList.Armos");
    return FindMenuPlacement(oot, "", "", "", cvar);
}
} // namespace

int main() {
    const std::string ootMenus = Joined(kOotMenuFiles), mmMenus = Joined(kMmMenuFiles);
    // OoT's seed options register their widgets from the randomizer's option table.
    const std::string ootSeedOptions = Read("soh/soh/Enhancements/randomizer/settings.cpp");
    std::set<std::string> ootWidgets, mmWidgets;
    for (const auto& cvar : CVars(ootMenus))
        ootWidgets.insert(cvar);
    for (const auto& cvar : CVars(mmMenus))
        mmWidgets.insert(cvar);

    // The page catalogue: each heading list is complete and its column split is inside it.
    std::set<std::string> pageKeys;
    for (const auto& page : kMenuPages) {
        assert(pageKeys.insert(std::string(page.scope) + "/" + page.page).second);
        assert(page.headingCount >= 0 && page.headingCount <= 10);
        assert(page.split >= 0 && page.split <= page.headingCount);
        for (int i = 0; i < page.headingCount; ++i) {
            assert(page.headings[i] && *page.headings[i]);
            assert(MenuHeadingIndex(page, page.headings[i]) == i); // no heading twice
        }
    }

    // Every row names a real page and one of its headings, and every CVar row names a widget the game's menu registers,
    // so a typo cannot hide a setting.
    std::map<std::string, int> keysPerGame;
    for (const auto& row : kMenuPlacements) {
        assert(row.games >= kMenuOot && row.games <= kMenuBoth);
        const auto* page = FindMenuPage(row.scope, row.page);
        if (!page)
            std::cerr << "No page for " << row.key << ": " << row.scope << "/" << row.page << '\n';
        if (!page && std::string(row.scope) == "randomizer") {
          const auto *randomizerPage = FindRandomizerPageHeadings(row.page);
          assert(randomizerPage &&
                 RandomizerHeadingIndex(*randomizerPage, row.heading) >= 0);
          assert(std::string(row.key).rfind("entrance:", 0) == 0);
          continue;
        }
        assert(page);
        if (row.heading && MenuHeadingIndex(*page, row.heading) < 0)
            std::cerr << "No heading " << row.heading << " on " << row.page << " for " << row.key << '\n';
        assert(!row.heading || MenuHeadingIndex(*page, row.heading) >= 0);
        for (bool oot : { true, false }) {
            if (!MenuPlacementForGame(row, oot))
                continue;
            // One row per game, scope and key.
            assert(++keysPerGame[std::string(oot ? "OoT " : "MM ") + row.scope + " " + row.key] == 1);
            if (!std::strncmp(row.key, "widget:",
                              7)) { // the named widget is in that game's menu
                // Sidebar and widget names may hold '/', so try the name after each one.
                const std::string key = row.key;
                bool named = false;
                for (auto slash = key.find('/'); slash != std::string::npos && !named; slash = key.find('/', slash + 1))
                    named = (oot ? ootMenus : mmMenus).find('"' + key.substr(slash + 1) + '"') != std::string::npos;
                assert(named);
            }
            if (!IsCVarKey(row.key))
                continue;
            if (!std::strcmp(row.scope, "randomizer")) {
                const std::string prefix = "gRandoSettings.", key = row.key;
                assert(oot && key.rfind(prefix, 0) == 0);
                assert(ootSeedOptions.find("CVAR_RANDOMIZER_SETTING(\"" + key.substr(prefix.size()) + "\")") !=
                       std::string::npos);
                continue;
            }
            const auto& widgets = oot ? ootWidgets : mmWidgets;
            std::string key = row.key;
            bool exists = widgets.count(key) != 0;
            if (key.back() == '*') { // a table-driven list: the CVars its loop registers
                key.pop_back();
                const auto next = widgets.lower_bound(key);
                exists = next != widgets.end() && next->rfind(key, 0) == 0;
            }
            if (!exists)
                std::cerr << "Placement for a CVar no " << (oot ? "OoT" : "MM") << " menu widget has: " << key << '\n';
            assert(exists);
        }
        // An anchored row stays in its anchor's heading.
        if (row.after) {
            for (bool oot : { true, false }) {
                if (!MenuPlacementForGame(row, oot))
                    continue;
                const auto* anchor = FindMenuPlacement(oot, "", "", "", row.after);
                assert(anchor && !std::strcmp(anchor->page, row.page));
                assert(!row.heading || !anchor->heading || !std::strcmp(anchor->heading, row.heading));
            }
        }
    }

    // Coverage: every CVar widget on both games' Enhancements sections and their Randomizer > General sidebars has a
    // row, so a new upstream setting fails here instead of landing anywhere.
    struct ScanRegion {
        bool oot;
        const char* file;
        const char* start;
        const char* end;
    };
    const ScanRegion regions[] = {
        { true, "soh/soh/SohGui/SohMenuEnhancements.cpp", "void SohMenu::AddMenuEnhancements()", nullptr },
        { true, "soh/soh/Enhancements/ExtraModes/EnemyRandomizer.cpp", "void RegisterEnemyRandomizerWidgets()",
          nullptr },
        { true, "soh/soh/SohGui/SohMenuRandomizer.cpp", "WidgetPath path = { \"Randomizer\", \"General\"",
          "auto randoSettings =" },
        { false, "mm/2s2h/BenGui/BenMenu.cpp", "void BenMenu::AddEnhancements()", nullptr },
        { false, "mm/2s2h/Rando/Menu.cpp", "mBenMenu->AddSidebarEntry(\"Rando\", \"General\", 1);",
          "mBenMenu->AddSidebarEntry(\"Rando\", \"Logic/Conditions\", 1);" },
    };
    int covered = 0;
    for (const auto& region : regions) {
        for (const auto& cvar : CVars(Active(Region(Read(region.file), region.start, region.end)))) {
            const auto* row = CVarPlacement(region.oot, cvar);
            if (!row || !IsCVarKey(row->key))
                std::cerr << "Unplaced " << (region.oot ? "OoT" : "MM") << " menu CVar " << cvar << " in "
                          << region.file << '\n';
            assert(row && IsCVarKey(row->key));
            ++covered;
        }
    }
    assert(covered > 500);

    // Rows of one feature that works the same way in both games land on one tab, page and heading.
    std::map<std::string, const MenuPlacement*> features;
    for (const auto& row : kMenuPlacements) {
        if (!row.mirrored || !row.feature)
            continue;
        auto [it, first] = features.emplace(row.feature, &row);
        if (first)
            continue;
        const auto& other = *it->second;
        if (std::strcmp(other.scope, row.scope) || std::strcmp(other.page, row.page) ||
            (other.heading && row.heading && std::strcmp(other.heading, row.heading)))
            std::cerr << "Feature " << row.feature << " split between " << other.page << " and " << row.page << '\n';
        assert(!std::strcmp(other.scope, row.scope) && !std::strcmp(other.page, row.page));
        assert(!other.heading || !row.heading || !std::strcmp(other.heading, row.heading));
    }
    // ... and has a row in each game, so neither view lacks it (OoT's ocarina input rows were once a note).
    for (const auto& feature : features) {
        bool games[2] = {};
        for (const auto& row : kMenuPlacements)
            if (row.mirrored && row.feature && feature.first == row.feature)
                for (bool oot : { true, false })
                    if (MenuPlacementForGame(row, oot))
                        games[oot] = true;
        if (!games[0] || !games[1])
            std::cerr << "Feature " << feature.first << " has rows in one game only\n";
        assert(games[0] && games[1]);
    }

    // Settings > Controls > Ocarina input: the same two switches with the same names in both views, each game's on
    // its own CVars. OoT's are widgets of its menu, searchable under their earlier names, and registered once: the
    // bindings editor no longer draws them, and keeps Customize Ocarina Controls with the note bindings.
    const char* ocarinaInput[][2] = { { "gSettings.CustomOcarina.Dpad", "gEnhancements.Playback.DpadOcarina" },
                                      { "gSettings.CustomOcarina.RightStick",
                                        "gEnhancements.Playback.RightStickOcarina" } };
    const char* ocarinaLabels[] = { "D-pad ocarina", "Right stick ocarina" };
    for (bool oot : { true, false }) {
        std::vector<std::string> rows;
        for (const auto& row : kMenuPlacements)
            if (MenuPlacementForGame(row, oot) && !std::strcmp(row.scope, "settings") &&
                !std::strcmp(row.page, MenuPage::Controls) && row.heading && !std::strcmp(row.heading, "Ocarina input"))
                rows.push_back(row.key);
        assert(rows.size() == 2);
        for (int i = 0; i < 2; ++i) {
            const char* cvar = ocarinaInput[i][oot ? 0 : 1];
            assert(std::count(rows.begin(), rows.end(), cvar) == 1);
            // Without its own row, an OoT switch would fall back to its sidebar's row: Controller mapping.
            const auto* row = FindMenuPlacement(oot, "Settings", "Controls", "", cvar);
            assert(row && !std::strcmp(row->heading, "Ocarina input") && row->mirrored);
            assert(row->feature && !std::strcmp(row->feature, "dpad_ocarina"));
            assert(MenuRowLabel(oot, cvar, nullptr) == ocarinaLabels[i]);
        }
    }
    const std::string inputEditor = Active(Read("soh/soh/Enhancements/controls/SohInputEditorWindow.cpp"));
    const auto ootRegistered = CVars(ootMenus + inputEditor);
    for (const auto& cvars : ocarinaInput)
        assert(std::count(ootRegistered.begin(), ootRegistered.end(), cvars[0]) == 1);
    for (const char* name : { "Dpad Ocarina Playback", "Right Stick Ocarina Playback" }) {
        const auto at = ootMenus.find(std::string("AddWidget(path, \"") + name + "\", WIDGET_CVAR_CHECKBOX)");
        assert(at != std::string::npos);
        assert(ootMenus.substr(at, ootMenus.find(';', at) - at).find("HideInSearch") == std::string::npos);
    }
    // Each OoT switch shows the same tooltip as its MM twin.
    const char* ocarinaNames[][2] = { { "Dpad Ocarina Playback", "D-pad Ocarina" },
                                      { "Right Stick Ocarina Playback", "Right Stick Ocarina" } };
    for (int i = 0; i < 2; ++i) {
        const auto oot = MenuRowTooltip(true, ocarinaInput[i][0], WidgetTooltip(ootMenus, ocarinaNames[i][0]).c_str());
        const auto mm = MenuRowTooltip(false, ocarinaInput[i][1], WidgetTooltip(mmMenus, ocarinaNames[i][1]).c_str());
        if (oot != mm)
            std::cerr << "Ocarina input tooltips differ: \"" << oot << "\" in OoT, \"" << mm << "\" in MM\n";
        assert(oot == mm);
    }
    const std::string ocarinaPanel = Region(inputEditor, "void SohInputEditorWindow::DrawOcarinaControlPanel()");
    assert(ocarinaPanel.find("dpadOcarina") == std::string::npos);
    assert(ocarinaPanel.find("rightStickOcarina") == std::string::npos);
    assert(ocarinaPanel.find("CVAR_SETTING(\"CustomOcarina.Enabled\")") != std::string::npos);
    assert(Region(inputEditor, "void SohInputEditorWindow::DrawLinkTab()").find("DrawOcarinaControlPanel();") !=
           std::string::npos);

    // Sam's example: climb speed sits on Time savers > Speed-ups in both games, through routing.
    const auto ootClimb =
        MenuGroupRoute(true, "Enhancements", "Skips & Speed-ups", 1, "Animations", "gEnhancements.ClimbSpeed");
    const auto mmClimb =
        MenuGroupRoute(false, "Enhancements", "Gameplay", 0, "Player", "gEnhancements.Player.ClimbSpeed");
    assert(ootClimb.scope == "gameplay" && ootClimb.page == MenuPage::TimeSavers);
    assert(mmClimb.scope == ootClimb.scope && mmClimb.page == ootClimb.page);
    for (bool oot : { true, false }) {
        const auto* row =
            FindMenuPlacement(oot, "", "", "", oot ? "gEnhancements.ClimbSpeed" : "gEnhancements.Player.ClimbSpeed");
        assert(row && !std::strcmp(row->heading, "Speed-ups"));
    }
    // A row without a CVar follows its name, heading or sidebar; an unknown upstream row keeps its sidebar's
    // destination.
    assert(!std::strcmp(FindMenuPlacement(true, "Enhancements", "Skips & Speed-ups", "Cutscenes", "", "All")->heading,
                        "Cutscenes"));
    // A native name with an ImGui id suffix finds its row: OoT's Cutscenes All/None buttons stay under the page's
    // Cutscenes heading instead of a second Cutscenes heading after the fixed ones.
    for (const char* name : { "All##Skips", "None##Skips" }) {
        assert(ootMenus.find(std::string("AddWidget(path, \"") + name + '"') != std::string::npos);
        const auto* row = FindMenuPlacement(true, "Enhancements", "Skips & Speed-ups", "Cutscenes", "", name);
        assert(row && !std::strcmp(row->page, MenuPage::TimeSavers) && !std::strcmp(row->heading, "Cutscenes"));
    }
    assert(!std::strcmp(
        FindMenuPlacement(true, "Enhancements", "Extra Modes", "Trap Options", "", "Tier 2 Traps:")->heading, "Traps"));
    assert(!std::strcmp(FindMenuPlacement(true, "Enhancements", "Extra Modes", "", "", "Unknown text")->page,
                        MenuPage::ExtraModes));
    assert(!FindMenuPlacement(false, "Enhancements", "Gameplay", "Player", "gEnhancements.Future.Setting"));
    assert(MenuGroupRoute(false, "Enhancements", "Gameplay", 0, "Player", "gEnhancements.Future.Setting").page ==
           MenuPage::QualityOfLife);
    // Search and network sidebars stay hidden whatever their rows say.
    assert(MenuGroupRoute(true, "Settings", "Search", 0, "", "gEnhancements.ClimbSpeed").scope.empty());

    // Page order: no page that is never produced, Tools opens on the Save Editor and ends with Session.
    assert(MenuPageOrder("gameplay", "Minigames") == 100);
    assert(MenuPageOrder("gameplay", MenuPage::ExtraModes) < MenuPageOrder("gameplay", MenuPage::Cheats));
    assert(MenuPageOrder("media", MenuPage::HudOverlays) == 100);
    assert(MenuPageOrder("tools", MenuPage::SaveEditor) == 0);
    for (const char* page : { MenuPage::EnhancementPresets, MenuPage::Mods, MenuPage::Multiplayer, MenuPage::EventLog })
        assert(MenuPageOrder("tools", page) < MenuPageOrder("tools", MenuPage::Session));
    const char* trackers[] = { MenuPage::CheckTracker,    MenuPage::ItemTracker, MenuPage::HintTracker,
                               MenuPage::EntranceTracker, MenuPage::Timers,      MenuPage::TimeSplits,
                               MenuPage::GameplayStats };
    for (int i = 0; i < int(sizeof(trackers) / sizeof(*trackers)); ++i)
        assert(MenuPageOrder("trackers", trackers[i]) == i);

    // Every relabelled row is a widget of that game's menu (or a combo rule row), and a renamed feature
    // keeps one label in both games.
    std::map<std::string, std::string> labelOf;
    for (const auto& row : kMenuLabels)
        for (bool oot : { true, false }) {
            if (!(row.games & (oot ? kMenuOot : kMenuMm)))
                continue;
            const bool rule = Detail::FindMenuPlacementKey(oot, std::string("rule:") + row.cvar) != nullptr;
            if (!rule && !(oot ? ootWidgets : mmWidgets).count(row.cvar))
                std::cerr << "Label for a CVar no " << (oot ? "OoT" : "MM") << " menu widget has: " << row.cvar << '\n';
            assert(rule || (oot ? ootWidgets : mmWidgets).count(row.cvar));
            assert(FindMenuLabel(oot, row.cvar) == &row); // one row per game and CVar
        }
    const char* climbLabels[] = { "gEnhancements.ClimbSpeed", "gEnhancements.Player.ClimbSpeed" };
    assert(MenuRowLabel(true, climbLabels[0], "Vine/Ladder Climb Speed +%d") == "Climb speed");
    assert(MenuRowLabel(false, climbLabels[1], "Climb speed") == "Climb speed");
    assert(std::string(FindMenuLabel(true, climbLabels[0])->format) == "+%d");
    assert(std::string(FindMenuLabel(false, climbLabels[1])->format) == "%dx");
    assert(MenuRowLabel(true, "gEnhancements.CrawlSpeed", "Crawl Speed %dx") == "Crawl Speed");
    assert(MenuRowLabel(false, "gCheats.SpeedModifier.Value", "Multiplier:") == "Multiplier");
    assert(MenuRowKeywords(true, climbLabels[0], "Vine/Ladder Climb Speed +%d").find("Vine/Ladder") !=
           std::string::npos);
    assert(MenuRowTooltip(false, "gEnhancements.Minigames.CuccoShackCuccoCount", "Count").find("lower number wins") !=
           std::string::npos);

    // No setting lost its home: every widget CVar the menus registered before the audit is still a widget of the same
    // game's menu.
    std::ifstream before(std::string(COMBO_REPO_DIR) + "/ComboAndroid/tests/fixtures/menu-cvars-before-t8.txt");
    assert(before);
    int checked = 0;
    for (std::string line; std::getline(before, line);) {
        if (line.empty() || line[0] == '#')
            continue;
        const bool oot = line.rfind("OoT ", 0) == 0;
        const auto cvar = line.substr(line.find(' ') + 1);
        if (!(oot ? ootWidgets : mmWidgets).count(cvar))
            std::cerr << "A menu CVar lost its widget: " << line << '\n';
        assert((oot ? ootWidgets : mmWidgets).count(cvar));
        ++checked;
    }
    assert(checked > 600);
    std::cout << "Menu placement: " << sizeof(kMenuPlacements) / sizeof(*kMenuPlacements) << " rows, " << covered
              << " Enhancements CVars placed, " << features.size() << " shared features together, " << checked
              << " earlier CVars kept\n";
}
