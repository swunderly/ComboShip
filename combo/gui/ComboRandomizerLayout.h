#pragma once
// Randomizer pages: one fixed heading list per page, shared by both worlds. The MM catalog cards
// (mm/2s2h/ProfileMmLayout.inc) use the same heading text, and OoT's native rows are regrouped under it when the page
// is drawn (ProfileUI::DrawSidebar). Placement only: every row keeps its native index, CVar and callbacks, and routing
// (ComboNavigation.h) still decides which page a row is on.
#include "ComboMenuABI.h"
#include "ComboNavigation.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace ComboRando {
struct RandomizerPageHeadings {
    const char* page;
    const char* headings[12];
    const char* secondColumn[6]; // the headings drawn in the second column on wide screens, in both worlds
};
// A heading with no rows in the selected world is not drawn. Items & checks lists its Checks headings, then its Items
// headings; each half shows only on its own subpage, and each half has its own second column. A subpage whose
// headings all sit in one column is drawn in one column.
inline constexpr RandomizerPageHeadings kRandomizerPageHeadings[] = {
    { "Logic & access",
      { "Logic", "Area access", "Final boss access", "Entrances" },
      { "Final boss access", "Entrances" } },
    { "Items & checks",
      { "World & NPCs", "Creatures & collectibles", "Scenery drops", "Shops & merchants", "Item pool",
        "Abilities & upgrades", "Extra songs", "Souls", "Clocks (MM)", "Cross-game items" },
      { "Scenery drops", "Shops & merchants", "Souls", "Clocks (MM)", "Cross-game items" } },
    { "Dungeons",
      { "Dungeon rewards", "Maps & compasses", "Small keys", "Key rings", "Boss keys", "Stray fairies",
        "Master Quest" },
      { "Boss keys", "Stray fairies", "Master Quest" } },
    { "Warps & travel", { "Destination shuffles (OoT)" }, {} },
    { "Hints & traps", { "Hints", "Static hints", "Traps" }, { "Static hints" } },
    // Item icons first; their last group, Other, holds the rest of the page.
    { "Starting inventory", {}, {} },
};
inline const RandomizerPageHeadings* FindRandomizerPageHeadings(const std::string& page) {
    for (const auto& entry : kRandomizerPageHeadings)
        if (page == entry.page)
            return &entry;
    return nullptr;
}
inline int RandomizerHeadingIndex(const RandomizerPageHeadings& page, const char* heading) {
    for (int i = 0; heading && i < int(sizeof(page.headings) / sizeof(*page.headings)) && page.headings[i]; ++i)
        if (!std::strcmp(page.headings[i], heading))
            return i;
    return -1;
}
// The wide-screen column of a heading on its page: 1 for the page's second-column headings, else 0.
inline int RandomizerHeadingColumn(const RandomizerPageHeadings& page, const char* heading) {
    for (const char* second : page.secondColumn)
        if (heading && second && !std::strcmp(second, heading))
            return 1;
    return 0;
}
// The narrowest wide-screen column of a Randomizer page, in both worlds. Logic & access holds the widest rows.
inline float RandomizerColumnMinimum(const std::string& page, float fontSize) {
    return page == "Logic & access" ? fontSize * 28 : std::max(400.f, fontSize * 20);
}
// The column count of a Randomizer page: two where the page splits and both columns fit, else one. OoT's rows and
// MM's cards both use it, so the two worlds switch to two columns at the same width.
inline int RandomizerColumnCount(const std::string& page, float fontSize, float width, bool split) {
    return split && width >= 2 * RandomizerColumnMinimum(page, fontSize) ? 2 : 1;
}
// The OoT row's heading on a Randomizer page, from its key or its native heading; nullptr keeps it above the headings.
inline const char* OotRandomizerHeading(const std::string& page, const std::string& key,
                                        const std::string& nativeHeading) {
    const std::string option = key.rfind("gRandoSettings.", 0) == 0 ? key.substr(15) : key;
    if (page == "Logic & access") {
        if (option == "StartingAge")
            return "Area access"; // follows Starting world, which greys it out
        for (const auto& [native, heading] :
             { std::pair{ "Logic", "Logic" }, std::pair{ "Area Access", "Area access" },
               std::pair{ "Win Condition", "Final boss access" }, std::pair{ "Entrances", "Entrances" } })
            if (nativeHeading == native)
                return heading;
        return nullptr;
    }
    if (page == "Items & checks") {
        // Checks arrive grouped under AppendFeatureWidgets' headings; Items are grouped by key.
        for (const auto& [native, heading] :
             { std::pair{ "World & NPCs", "World & NPCs" },
               std::pair{ "Creatures & Collectibles", "Creatures & collectibles" },
               std::pair{ "Scenery Drops", "Scenery drops" }, std::pair{ "Shops & Merchants", "Shops & merchants" } })
            if (nativeHeading == native)
                return heading;
        if (nativeHeading.empty() || nativeHeading == "Item Pool Settings" || nativeHeading == "Additional Items" ||
            nativeHeading == "Dungeon Items") {
            const char* items[] = { "Item pool", "Abilities & upgrades", "Souls" };
            return items[OotItemPoolGroup(key)];
        }
        return nullptr;
    }
    if (page == "Dungeons") {
        if (option == "ShuffleDungeonReward")
            return "Dungeon rewards";
        if (option == "StartingMapsCompasses")
            return "Maps & compasses";
        if (option == "Keysanity" || option == "GerudoKeys")
            return "Small keys";
        if (option == "BossKeysanity")
            return "Boss keys";
        if (nativeHeading == "Keyrings")
            return "Key rings";
        if (nativeHeading == "Master Quest")
            return "Master Quest";
        return nullptr;
    }
    if (page == "Warps & travel")
        return option == "ShuffleWarpSongs" || option == "ShuffleOwlDrops" ? "Destination shuffles (OoT)" : nullptr;
    if (page == "Hints & traps") {
        for (const auto& [native, heading] :
             { std::pair{ "Hints", "Hints" }, std::pair{ "Static Hints", "Static hints" },
               std::pair{ "Traps", "Traps" } })
            if (nativeHeading == native)
                return heading;
        return nullptr;
    }
    return nullptr;
}
// Regroups an OoT Randomizer body under its page's headings: custom panels first, then rows without a heading, then
// each heading in the page's order with its rows in native order (a row that leads its heading first). Native
// separators are replaced by the page's headings. Wide screens draw the page's second-column headings in the second
// column, as MM's view does; a page without a heading list keeps its rows and columns.
inline std::vector<CwWidget> ArrangeOotRandomizerPage(const std::string& page, const CwWidget* widgets, int count) {
    std::vector<CwWidget> out(widgets, widgets + std::max(count, 0));
    const auto* layout = FindRandomizerPageHeadings(page);
    if (!layout)
        return out;
    struct Row {
        int group, lead, order;
        CwWidget widget;
    };
    std::vector<Row> rows;
    std::string heading;
    int column = -1;
    for (int i = 0; i < count; ++i) {
        const auto& widget = widgets[i];
        if (widget.column != column) {
            column = widget.column;
            heading.clear();
        }
        if (widget.kind == CW_SEPARATOR_TEXT) {
            heading = widget.name ? widget.name : "";
            continue;
        }
        if (widget.kind == CW_SEPARATOR)
            continue;
        const std::string key = widget.cvar ? widget.cvar : "";
        const int group =
            widget.kind == CW_CUSTOM ? -2 : RandomizerHeadingIndex(*layout, OotRandomizerHeading(page, key, heading));
        rows.push_back({ group, key == "gRandoSettings.StartingAge" ? 0 : 1, i, widget });
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.group != b.group ? a.group < b.group : a.lead < b.lead;
    });
    out.clear();
    int drawn = -3;
    for (const auto& row : rows) {
        const int target = row.group >= 0 ? RandomizerHeadingColumn(*layout, layout->headings[row.group]) : 0;
        if (row.group >= 0 && row.group != drawn) {
            CwWidget title{};
            title.index = -1;
            title.kind = CW_SEPARATOR_TEXT;
            title.name = layout->headings[row.group];
            title.cvar = "";
            title.tooltip = "";
            title.windowName = "";
            title.column = target;
            out.push_back(title);
        }
        drawn = row.group;
        out.push_back(row.widget);
        out.back().column = target;
    }
    return out;
}
} // namespace ComboRando
