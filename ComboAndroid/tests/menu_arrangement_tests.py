"""Both world views of a page show the same headings in the same order (T8 menu spec, sections 1-3).

Compiles ComboMenu.cpp's production ArrangeByPlacement with the real placement catalogue and lays
out every catalogue page from each game's rows, as the page body build does.
"""
import argparse
from pathlib import Path
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
text = (source / 'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')


def block(signature, opener='{'):
    start = hostcxx.find(text, signature)
    end = text.index(opener, start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    if text[end:end + 1] == ';':
        end += 1
    return text[start:end] + '\n'


production = block('struct RowOrigin {') + text[hostcxx.find(text, 'using RowOrigins'):].split('\n', 1)[0] + '\n'
production += block('bool IsStaleIntroText(') + block('uint32_t ArrangeByPlacement(')

code = r'''
#include "ComboMenuABI.h"
#include "ComboMenuPlacement.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
namespace ComboRando {
''' + production + r'''
}
using namespace ComboRando;

struct Laid {
    std::vector<std::string> headings;            // in drawing order
    std::map<std::string, std::string> headingOf; // CVar -> heading it is drawn under
    std::map<std::string, int> columnOf;          // heading -> column
    std::vector<std::string> order;               // CVars in drawing order
};

Laid Lay(const MenuPageLayout& layout, bool oot, bool withNativeExtras) {
    std::vector<CwWidget> widgets;
    RowOrigins origins;
    int index = 0;
    auto add = [&](CwKind kind, const char* name, const char* cvar, const char* nativeHeading) {
        CwWidget widget{};
        widget.index = index++;
        widget.kind = kind;
        widget.name = name;
        widget.cvar = cvar;
        widgets.push_back(widget);
        origins[widget.index] = { cvar && *cvar ? FindMenuPlacement(oot, "", "", "", cvar) : nullptr, nativeHeading };
    };
    if (withNativeExtras) {
        add(CW_SEPARATOR_TEXT, "Native heading", "", "Native heading");
        add(CW_TEXT, "Be sure to explore the Presets and Enhancements Menus for various Speedups and Quality of Life changes!",
            "", "Native heading");
    }
    for (const auto& row : kMenuPlacements) {
        const std::string key = row.key;
        if (!MenuPlacementForGame(row, oot) || std::strcmp(row.scope, layout.scope) || std::strcmp(row.page, layout.page) ||
            key.find(':') != std::string::npos || key.back() == '*')
            continue;
        add(CW_CHECKBOX, row.key, row.key, "Native heading");
    }
    if (withNativeExtras)
        add(CW_CHECKBOX, "Future upstream row", "gEnhancements.Future.Row", "Upstream heading");
    const uint32_t columns = ArrangeByPlacement(layout, widgets, origins, 1);
    assert(columns == (layout.split < layout.headingCount ? 2u : 1u));
    Laid laid;
    std::string heading;
    for (const auto& widget : widgets) {
        if (widget.kind == CW_SEPARATOR_TEXT) {
            assert(widget.index == -1); // native separators give way to the page's own headings
            heading = widget.name;
            laid.headings.push_back(heading);
            laid.columnOf[heading] = widget.column;
            continue;
        }
        assert(widget.kind != CW_TEXT); // the stale intro line is gone
        assert(widget.column == laid.columnOf[heading]);
        laid.headingOf[widget.cvar] = heading;
        laid.order.push_back(widget.cvar);
    }
    return laid;
}

int main() {
    int pages = 0;
    for (const auto& layout : kMenuPages) {
        if (layout.headingCount < 2)
            continue;
        ++pages;
        const Laid views[] = { Lay(layout, true, true), Lay(layout, false, true) };
        for (const auto& view : views) {
            // The fixed headings in their fixed order, then rows the catalogue does not place under
            // their native heading.
            int last = -1;
            for (const auto& heading : view.headings) {
                const int index = MenuHeadingIndex(layout, heading.c_str());
                if (index < 0) {
                    assert(heading == "Upstream heading" && &heading == &view.headings.back());
                    continue;
                }
                assert(index > last);
                last = index;
            }
            assert(view.headingOf.at("gEnhancements.Future.Row") == "Upstream heading");
        }
        // A heading sits in the same column in both views.
        for (const auto& [heading, column] : views[0].columnOf)
            if (views[1].columnOf.count(heading))
                assert(views[1].columnOf.at(heading) == column);
        // A feature that works the same in both games is drawn under the same heading.
        for (const auto& row : kMenuPlacements) {
            if (!row.mirrored || std::strcmp(row.page, layout.page) || !std::strchr(row.key, '.') ||
                std::strchr(row.key, ':'))
                continue;
            for (const auto& other : kMenuPlacements)
                if (other.mirrored && row.feature && other.feature && !std::strcmp(row.feature, other.feature) &&
                    MenuPlacementForGame(row, true) && MenuPlacementForGame(other, false) && !std::strchr(other.key, ':'))
                    assert(views[0].headingOf.at(row.key) == views[1].headingOf.at(other.key));
        }
        // An anchored row is drawn below its anchor.
        for (bool oot : { true, false }) {
            const auto& view = views[oot ? 0 : 1];
            for (const auto& row : kMenuPlacements) {
                if (!row.after || !MenuPlacementForGame(row, oot) || std::strcmp(row.page, layout.page) ||
                    std::strchr(row.key, ':') || std::strchr(row.key, '*'))
                    continue;
                const auto anchor = std::find(view.order.begin(), view.order.end(), row.after);
                const auto anchored = std::find(view.order.begin(), view.order.end(), row.key);
                assert(anchored != view.order.end());
                if (anchor != view.order.end())
                    assert(anchor < anchored && view.headingOf.at(row.after) == view.headingOf.at(row.key));
            }
        }
    }
    assert(pages >= 15);
    // Sam's example: climb speed under Time savers > Speed-ups in both views.
    const auto* timeSavers = FindMenuPage("gameplay", MenuPage::TimeSavers);
    assert(Lay(*timeSavers, true, false).headingOf.at("gEnhancements.ClimbSpeed") == "Speed-ups");
    assert(Lay(*timeSavers, false, false).headingOf.at("gEnhancements.Player.ClimbSpeed") == "Speed-ups");
    // A page without fixed headings keeps native order and drops only a heading repeating its title.
    MenuPageLayout notifications = *FindMenuPage("settings", MenuPage::Notifications);
    std::vector<CwWidget> rows(3);
    rows[0].index = 1, rows[0].kind = CW_SEPARATOR_TEXT, rows[0].name = "Notifications";
    rows[1].index = 2, rows[1].kind = CW_CHECKBOX, rows[1].name = "Position", rows[1].cvar = "gNotifications.Position";
    rows[2].index = 3, rows[2].kind = CW_SEPARATOR_TEXT, rows[2].name = "Sounds";
    assert(ArrangeByPlacement(notifications, rows, {}, 2) == 2u && rows.size() == 2 && !std::strcmp(rows[1].name, "Sounds"));
    std::cout << "PASS: " << pages << " catalogue pages show the same headings in the same order in both worlds\n";
}
'''
cpp = out / 'menu_arrangement.cpp'
cpp.write_text(code, encoding='utf-8')
exe = hostcxx.build(a.compiler, cpp, std='c++17', werror=True, includes=[source / 'combo/gui', source / 'combo/menu'])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
