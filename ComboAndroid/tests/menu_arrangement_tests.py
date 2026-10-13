"""Both world views of a page show the same headings in the same order (T8 menu spec, sections 1-3).

Compiles ComboMenu.cpp's production ArrangeByPlacement, with its gameplay rule and both-games rows,
against the real placement catalogue, and lays out every catalogue page from each game's rows as
the page body build does.
"""
import argparse
from pathlib import Path
import subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--json-include', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
source, out = a.source.resolve(), a.build_dir.resolve()
out.mkdir(parents=True, exist_ok=True)
text = (source / 'combo/gui/ComboMenu.cpp').read_text(encoding='utf-8')
render = (source / 'combo/gui/ComboWidgetRender.cpp').read_text(encoding='utf-8')


def block(signature, text=text):
    start = hostcxx.find(text, signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    if text[end:end + 1] == ';':
        end += 1
    return text[start:end] + '\n'


def line(signature):
    start = hostcxx.find(text, signature)
    return text[start:text.index(';', start) + 1] + '\n'


production = block('struct RowOrigin {') + line('using RowOrigins')
production += block('struct PairedPreference {') + block('constexpr PairedPreference kPairedPreferences[]')
production += line('constexpr int32_t kRuleRowBase') + block('bool MakePseudoRow(') + block('void AppendPseudoRows(')
production += block('bool IsStaleIntroText(') + block('uint32_t ArrangeByPlacement(')
# The slider readout of the one widget style (ComboWidgetRender.cpp).
production += block('struct SliderReadout {', render) + block('static bool ReadoutFits(', render)
production += block('static SliderReadout ReadoutOf(', render)
# Which widgets the owning game draws (ComboWidgetRender.cpp).
production += block('static bool DelegatesToGame(', render) + block('static bool HoldsValuePointer(', render)
production += block('static bool DrawnByGame(', render) + block('static bool GameDrawsWidget(', render)
# A value-pointer control never reaches the CVar switch of RenderWidget.
render_widget = block('void RenderWidget(', render)
assert 'game.drawWidget && GameDrawsWidget(w)' in render_widget, 'RenderWidget no longer asks GameDrawsWidget'
guard = render_widget.find('if (HoldsValuePointer(w))')
assert 0 <= guard < render_widget.find('switch (w.kind)'), 'a value-pointer control can reach the CVar path'

code = r'''
#include "ComboMenuABI.h"
#include "ComboMenuLabels.h"
#include "ComboNavigation.h"
#include "rando/GameplayRules.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace ComboRando {
''' + production + r'''
}
using namespace ComboRando;

struct Laid {
    std::vector<std::string> headings;            // in drawing order
    std::map<std::string, std::string> headingOf; // CVar (or rule/pair label) -> heading it is drawn under
    std::map<std::string, int> columnOf;          // heading -> column
    std::vector<std::string> order;               // CVars in drawing order
};

bool ReplacedByComboRow(const char* cvar) {
    for (const auto& pair : kPairedPreferences)
        if (!std::strcmp(cvar, pair.oot) || !std::strcmp(cvar, pair.mm))
            return true;
    const auto* rule = FindGameplayRule(cvar);
    return rule && !rule->trickGoverned;
}

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
            key.find(':') != std::string::npos || key.back() == '*' || ReplacedByComboRow(row.key))
            continue;
        add(CW_CHECKBOX, row.key, row.key, "Native heading");
    }
    if (withNativeExtras)
        add(CW_CHECKBOX, "Future upstream row", "gEnhancements.Future.Row", "Upstream heading");
    AppendPseudoRows(oot, layout.scope, layout.page, widgets, origins);
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
        // A rule or pair row carries the rule's (or this world's) CVar and its label.
        laid.headingOf[widget.cvar] = heading;
        if (widget.index <= -1000)
            laid.headingOf[widget.name] = heading;
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
            if (!row.mirrored || std::strcmp(row.scope, layout.scope) || std::strcmp(row.page, layout.page) ||
                std::strchr(row.key, ':') || ReplacedByComboRow(row.key))
                continue;
            for (const auto& other : kMenuPlacements)
                if (other.mirrored && row.feature && other.feature && !std::strcmp(row.feature, other.feature) &&
                    MenuPlacementForGame(row, true) && MenuPlacementForGame(other, false) &&
                    !std::strchr(other.key, ':') && !ReplacedByComboRow(other.key))
                    assert(views[0].headingOf.at(row.key) == views[1].headingOf.at(other.key));
        }
        // A both-games checkbox sits under the same heading in both views.
        for (const auto& pair : kPairedPreferences)
            if (views[0].headingOf.count(pair.label) || views[1].headingOf.count(pair.label))
                assert(views[0].headingOf.at(pair.label) == views[1].headingOf.at(pair.label));
        // An anchored row is drawn below its anchor.
        for (bool oot : { true, false }) {
            const auto& view = views[oot ? 0 : 1];
            for (const auto& row : kMenuPlacements) {
                if (!row.after || !MenuPlacementForGame(row, oot) || std::strcmp(row.scope, layout.scope) ||
                    std::strcmp(row.page, layout.page) || std::strchr(row.key, ':') || std::strchr(row.key, '*'))
                    continue;
                const auto anchor = std::find(view.order.begin(), view.order.end(), row.after);
                const auto anchored = std::find(view.order.begin(), view.order.end(), row.key);
                assert(anchored != view.order.end());
                if (anchor != view.order.end())
                    assert(anchor < anchored && view.headingOf.at(row.after) == view.headingOf.at(row.key));
            }
        }
    }
    assert(pages >= 16);
    // Sam's example: climb speed under Time savers > Speed-ups in both views.
    const auto* timeSavers = FindMenuPage("gameplay", MenuPage::TimeSavers);
    assert(Lay(*timeSavers, true, false).headingOf.at("gEnhancements.ClimbSpeed") == "Speed-ups");
    assert(Lay(*timeSavers, false, false).headingOf.at("gEnhancements.Player.ClimbSpeed") == "Speed-ups");
    // Rules sit inside their feature's heading: Blue Fire Arrows with the bow, the Bremen count with the
    // other quest counts, both arrow checkboxes under Bow & arrows in both worlds.
    const auto* items = FindMenuPage("gameplay", MenuPage::ItemsEquipment);
    const auto ootItems = Lay(*items, true, false), mmItems = Lay(*items, false, false);
    assert(ootItems.headingOf.at("gEnhancements.BlueFireArrows") == "Bow & arrows");
    assert(ootItems.headingOf.at("Quick magic-arrow equip") == "Bow & arrows");
    assert(mmItems.headingOf.at("Quick magic-arrow equip") == "Bow & arrows");
    assert(mmItems.headingOf.at("gEnhancements.Masks.FierceDeitysAnywhere") == "Masks & forms");
    const auto mmDifficulty = Lay(*FindMenuPage("gameplay", MenuPage::Difficulty), false, false);
    assert(mmDifficulty.headingOf.at("gEnhancements.DifficultyOptions.BremenChickCount") == "Quest requirements");
    const auto ootQuality = Lay(*FindMenuPage("gameplay", MenuPage::QualityOfLife), true, false);
    for (const char* rule : { "gEnhancements.OpenAllHours", "gEnhancements.DayGravePull", "gEnhancements.NightGSAlwaysSpawn" })
        assert(ootQuality.headingOf.at(rule) == "Time of day & cycle");
    // Autosave's notification stays under the both-games Autosave checkbox.
    const auto ootSaving = Lay(*FindMenuPage("gameplay", MenuPage::Saving), true, false);
    const auto autosave = std::find(ootSaving.order.begin(), ootSaving.order.end(), "gEnhancements.Autosave");
    assert(autosave != ootSaving.order.end() && *(autosave + 1) == "gEnhancements.AutosaveNotification");
    // The logic halves: one heading list for both worlds, no runtime-only count among them.
    const auto* rules = FindMenuPage("randomizer", "Gameplay rules");
    const auto ootRules = Lay(*rules, true, false), mmRules = Lay(*rules, false, false);
    assert(ootRules.headingOf.at("gEnhancements.TimeTravel") == "Abilities");
    assert(ootRules.headingOf.at("gRandoSettings.SkipScarecrowsSong") == "Quest requirements");
    assert(mmRules.headingOf.at("gEnhancements.Playback.SkipScarecrowSong") == "Quest requirements");
    assert(ootRules.headingOf.at("gRandoSettings.SwordlessEponaItems") == "Seed settings (no logic effect)");
    assert(mmRules.headingOf.at("gEnhancements.Minigames.AlwaysWinDoggyRace") == "Minigames");
    assert(!mmRules.headingOf.count("gEnhancements.DifficultyOptions.BremenChickCount"));
    // Slider readouts: the label table's unit shows the stored value; the game's own format keeps its
    // percentage; a printf token in the native name splits off the label.
    auto readout = [](bool oot, const char* name, const char* cvar, const char* format, bool percentage, bool isFloat) {
        CwWidget w{};
        w.name = name;
        w.cvar = cvar;
        w.format = format;
        w.isPercentage = percentage;
        return ReadoutOf(w, FindMenuLabel(oot, cvar), isFloat);
    };
    auto same = [](const SliderReadout& r, const char* format, bool percentage) {
        return r.format == format && r.percentage == percentage;
    };
    assert(same(readout(true, "Vine/Ladder Climb Speed +%d", "gEnhancements.ClimbSpeed", "+%d", false, false), "+%d", false));
    assert(same(readout(false, "Climb speed", "gEnhancements.Player.ClimbSpeed", "%d", false, false), "%dx", false));
    assert(MenuRowLabel(true, "gEnhancements.ClimbSpeed", "Vine/Ladder Climb Speed +%d") ==
           MenuRowLabel(false, "gEnhancements.Player.ClimbSpeed", "Climb speed"));
    assert(same(readout(true, "Crawl Speed %dx", "gEnhancements.CrawlSpeed", "%dx", false, false), "%dx", false));
    assert(same(readout(false, "Master Volume: %.0f%%", "gSettings.Audio.MasterVolume", "%.0f%%", true, true), "%.0f%%", true));
    assert(same(readout(false, "Internal Resolution: %.0f%%", "gSettings.InternalResolution", "", true, true), "%.0f%%", true));
    assert(same(readout(true, "Multiplier:", "gCheats.SpeedModifier.Value", "%.0f%%", true, true), "%.2fx", false));
    assert(same(readout(false, "Multiplier:", "gCheats.SpeedModifier.Value", "%.1fx", false, true), "%.2fx", false));
    assert(same(readout(true, "Ingame Text Spacing: %d", "gEnhancements.TextSpacing", "%d", false, false), "%d", false));
    assert(same(readout(true, "Duration (seconds):", "gSettings.Notifications.Duration", "%.1f", false, true),
                "%.1f seconds", false));
    assert(same(readout(true, "Strength", "", nullptr, false, false), "%d", false));
    assert(same(readout(true, "Scale", "", "%f", false, true), "%.2f", false));
    assert(same(readout(true, "Odd: %d", "", "%f", false, true), "%.2f", false)); // an int token on a float bar
    // A checkbox or slider without a CVar (a value pointer: MM motion blur, Frame Advance) is drawn by its game,
    // never through a CVar named ""; CVar checkboxes and sliders take the one combo style.
    for (CwKind kind : { CW_CHECKBOX, CW_SLIDER_INT, CW_SLIDER_FLOAT }) {
        CwWidget w{};
        w.kind = kind;
        w.name = "On/Off";
        for (const char* cvar : { "", static_cast<const char*>(nullptr) }) {
            w.cvar = cvar;
            assert(HoldsValuePointer(w) && GameDrawsWidget(w));
        }
        w.cvar = "gEnhancements.ClimbSpeed";
        assert(!HoldsValuePointer(w) && !GameDrawsWidget(w));
    }
    CwWidget choice{};
    choice.kind = CW_COMBOBOX;
    choice.cvar = "";
    assert(GameDrawsWidget(choice) && !HoldsValuePointer(choice));

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
exe = hostcxx.build(a.compiler, cpp, std='c++17', werror=True,
                    includes=[source / 'combo', source / 'combo/gui', source / 'combo/menu', a.json_include])
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
