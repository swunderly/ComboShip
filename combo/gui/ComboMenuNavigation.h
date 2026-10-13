#pragma once
#include "ComboMenuModel.h"
#include "ComboNavigation.h"
#include <map>
#include <utility>
#include <vector>

namespace ComboRando {
struct HubEntry {
    std::string label; // shown in the left panel
    std::string group; // owning group label (for the unique key)
    enum Kind {
        ENGINE,
        COMBO_GEN,
        COMBO_PLANDO,
        COMBO_TRACKER,
        COMBO_CHECK_TRACKER,
        COMBO_HINT_TRACKER,
        COMBO_TIMERS,
        COMBO_NETWORK,
        COMBO_HOME,
        COMBO_SHARED_ITEMS,
        COMBO_ABOUT,
        COMBO_SESSION,
        COMBO_INTERFACE
    } kind;
    struct Source {
        const ComboRando::GameMenu* game;
        const CwSidebar* sidebar;
        bool oot;
        const char* section;
    };
    std::vector<Source> sources;
    struct Body {
        const GameMenu* game = nullptr;
        std::vector<CwWidget> widgets;
        uint32_t columns = 1;
    };
    std::map<std::pair<int, int>, Body> bodies;
    std::string key() const {
        return group + "/" + label;
    }
};

inline MenuDestination NavigationSourceRoute(bool oot, const std::string& section, const std::string& sidebar,
                                             int column) {
    return MenuGroupRoute(oot, section, sidebar, column, "");
}

inline std::vector<HubEntry> BuildFeatureNavigation(const std::string& scope, const ComboMenuModel& model) {
    std::vector<HubEntry> entries;
    auto add = [&](const char* destinationScope, const char* label, HubEntry::Kind kind) {
        if (scope != destinationScope)
            return;
        HubEntry entry;
        entry.group = destinationScope;
        entry.label = label;
        entry.kind = kind;
        entries.push_back(std::move(entry));
    };
    add("home", MenuPage::CommunityPresets, HubEntry::COMBO_HOME);
    add("randomizer", "Shared items", HubEntry::COMBO_SHARED_ITEMS);
    add("randomizer", "Warps & travel", HubEntry::ENGINE);
    add("gameplay", MenuPage::WarpsTravel, HubEntry::ENGINE);
    add("randomizer", "Logic & access", HubEntry::ENGINE);
    add("gameplay", MenuPage::Difficulty, HubEntry::ENGINE);
    add("gameplay", MenuPage::Saving, HubEntry::ENGINE);
    add("gameplay", MenuPage::ItemsEquipment, HubEntry::ENGINE);
    add("gameplay", MenuPage::TimeSavers, HubEntry::ENGINE);
    add("randomizer", "Generate", HubEntry::COMBO_GEN);
    add("randomizer", "Gameplay rules", HubEntry::ENGINE);
    add("randomizer", "Plandomizer", HubEntry::COMBO_PLANDO);
    add("trackers", MenuPage::CheckTracker, HubEntry::COMBO_CHECK_TRACKER);
    add("trackers", MenuPage::ItemTracker, HubEntry::COMBO_TRACKER);
    add("trackers", MenuPage::HintTracker, HubEntry::COMBO_HINT_TRACKER);
    add("trackers", MenuPage::Timers, HubEntry::COMBO_TIMERS);
    add("settings", MenuPage::Interface, HubEntry::COMBO_INTERFACE);
    add("media", MenuPage::ScreenHud, HubEntry::ENGINE);
    add("tools", MenuPage::Session, HubEntry::COMBO_SESSION);
    add("tools", MenuPage::Multiplayer, HubEntry::COMBO_NETWORK);
    const GameMenu* games[] = { &model.Oot(), &model.Mm() };
    for (int g = 0; g < 2; ++g) {
        const auto& game = *games[g];
        if (!game.loaded || !game.menu)
            continue;
        for (int s = 0; s < game.menu->sectionCount; ++s) {
            const auto& section = game.menu->sections[s];
            for (int b = 0; b < section.sidebarCount; ++b) {
                const auto& sidebar = section.sidebars[b];
                const auto dest = NavigationSourceRoute(g == 0, section.sectionLabel ? section.sectionLabel : "",
                                                        sidebar.sidebarName ? sidebar.sidebarName : "", 0);
                auto addSource = [&](const MenuDestination& route) {
                    if (route.scope != scope)
                        return;
                    auto it = std::find_if(entries.begin(), entries.end(),
                                           [&](const HubEntry& e) { return e.label == route.page; });
                    if (it == entries.end()) {
                        HubEntry entry;
                        entry.group = route.scope;
                        entry.label = route.page;
                        entry.kind = HubEntry::ENGINE;
                        entries.push_back(std::move(entry));
                        it = entries.end() - 1;
                    }
                    if (std::none_of(it->sources.begin(), it->sources.end(), [&](const HubEntry::Source& source) {
                            return source.game == &game && source.sidebar == &sidebar;
                        }))
                        it->sources.push_back({ &game, &sidebar, g == 0, section.sectionLabel });
                };
                // A sidebar's own destination names a page only when it has no rows to route; otherwise
                // the page list follows its rows, so a sidebar whose rows all moved leaves no empty page.
                if (sidebar.widgetCount == 0)
                    addSource(dest);
                std::string heading;
                int column = -1;
                for (int w = 0; w < sidebar.widgetCount; ++w) {
                    const auto& widget = sidebar.widgets[w];
                    if (widget.column != column) {
                        column = widget.column;
                        heading.clear();
                    }
                    if (widget.kind == CW_SEPARATOR_TEXT)
                        heading = widget.name ? widget.name : "";
                    addSource(MenuGroupRoute(g == 0, section.sectionLabel, sidebar.sidebarName, column, heading,
                                             widget.cvar ? widget.cvar : ""));
                }
            }
        }
    }
    if (scope == "randomizer") {
        std::stable_sort(entries.begin(), entries.end(), [](const HubEntry& a, const HubEntry& b) {
            return RandomizerPageOrder(a.label) < RandomizerPageOrder(b.label);
        });
    }
    add("settings", MenuPage::About, HubEntry::COMBO_ABOUT);
    if (scope == "gameplay" || scope == "media" || scope == "settings" || scope == "tools" || scope == "trackers")
        std::stable_sort(entries.begin(), entries.end(), [&](const HubEntry& a, const HubEntry& b) {
            return MenuPageOrder(scope, a.label) < MenuPageOrder(scope, b.label);
        });
    return entries;
}

// Native menu exports own immutable metadata for the process lifetime. Keep the
// page index and grouped widget arrays, but still evaluate live values/callbacks
// when rendering. A late-loading or replaced game export invalidates both caches.
class MenuNavigationCache {
  public:
    std::vector<HubEntry>& Get(const std::string& scope, const ComboMenuModel& model) {
        const CwMenu* current[] = { model.Oot().loaded ? model.Oot().menu : nullptr,
                                    model.Mm().loaded ? model.Mm().menu : nullptr };
        if (menus[0] != current[0] || menus[1] != current[1]) {
            pages.clear();
            menus[0] = current[0];
            menus[1] = current[1];
        }
        auto found = pages.find(scope);
        if (found == pages.end())
            found = pages.emplace(scope, BuildFeatureNavigation(scope, model)).first;
        return found->second;
    }

  private:
    const CwMenu* menus[2]{};
    std::map<std::string, std::vector<HubEntry>> pages;
};
} // namespace ComboRando
