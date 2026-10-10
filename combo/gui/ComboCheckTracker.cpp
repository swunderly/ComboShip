#include "ComboCheckTracker.h"
#include "ComboCheckTrackerModel.h"
#include "ComboTrackerControls.h"
#include "ComboForeground.h"
#include "ComboTrackerCommon.h"
#include "ComboTrackerSwap.h"
#include "ComboWidgetStyle.h"
#include "ComboMenuLayout.h"
#include "ComboExport.h"
#include <libultraship/controller/controldeck/ControlDeck.h>
#include <ship/window/gui/GuiWindow.h>
#include <ship/window/gui/IconsFontAwesome4.h>
#include <imgui_internal.h>
#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace ComboRando {
namespace {
using namespace ComboChecks;
constexpr const char* kMode = "gCombo.CheckTracker.Visibility";
constexpr const char* kShortcut = "gCombo.CheckTracker.Shortcut";
bool dirty = true;
double lastRefresh = -1;
int lastContext = -1, lastArea = -1, expandAll = -1, selectedCheck = -1;
bool scrollToArea = true;
ImGuiTextFilter search;
ComboCheckInfo info{ 0, -1, -1 };
struct Row {
    int id;
    uint32_t flags;
    std::string name, type, reward, logic, searchable;
};
struct Area {
    int id;
    std::string name;
    std::vector<Row> rows;
    Totals totals;
};
std::vector<Area> areas;
UniqueTotals totals;

struct GameAPI {
    ComboCheckSnapshot snapshot = nullptr;
    ComboCheckSetSkipped skip = nullptr;
};
const GameAPI& API(int game) {
    static GameAPI api[2];
    // Retry until mapped: MM can register later than comboui during startup.
    auto& a = api[game];
    if (!a.snapshot)
        a.snapshot = reinterpret_cast<ComboCheckSnapshot>(
            Combo_ResolveSym(game ? "2ship" : "soh", game ? "MM_ComboCheckSnapshot" : "SOH_ComboCheckSnapshot"));
    if (!a.skip)
        a.skip = reinterpret_cast<ComboCheckSetSkipped>(
            Combo_ResolveSym(game ? "2ship" : "soh", game ? "MM_ComboCheckSetSkipped" : "SOH_ComboCheckSetSkipped"));
    return a;
}
void Save() {
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    dirty = true;
}
bool Check(const char* label, const char* cvar, int def) {
    bool value = CVarGetInteger(cvar, def) != 0;
    if (!ImGui::Checkbox(label, &value))
        return false;
    CVarSetInteger(cvar, value);
    Save();
    return true;
}
void Collect(const ComboCheckRow* row, void*) {
    if (!row)
        return;
    auto group = std::find_if(areas.begin(), areas.end(), [&](const Area& a) { return a.id == row->area; });
    if (group == areas.end()) {
        areas.push_back({ row->area, row->areaName ? row->areaName : "Other", {}, {} });
        group = areas.end() - 1;
    }
    Row r{ row->id,
           row->flags,
           row->name ? row->name : "",
           row->type ? row->type : "",
           row->reward ? row->reward : "",
           row->logic ? row->logic : "",
           "" };
    r.searchable = group->name + " " + r.name + " " + r.type + " " + r.reward;
    group->totals.Add(r.flags);
    totals.Add(r.id, r.flags);
    group->rows.push_back(std::move(r));
}
void Migrate() {
    // The compact 2S2H-style side panel is the new presentation default.
    // Existing visibility modes, shortcuts, filters and save flags are retained.
    if (!CVarGetInteger("gCombo.CheckTracker.CompactVersion", 0)) {
        CVarSetInteger("gCombo.CheckTracker.CompactVersion", 1);
        CVarSetInteger("gCombo.CheckTracker.WindowType", 2);
        Save();
    }
    if (CVarGetInteger("gCombo.CheckTracker.UnifiedVersion", 0))
        return;
    CVarSetInteger("gCombo.CheckTracker.UnifiedVersion", 1);
    // Keep the user's MM toggle/chord, as it already supports both tap and hold.
    CVarSetInteger(kMode, std::clamp(CVarGetInteger("gRando.CheckTracker.VisibilityMode", Paused), 0, 3));
    uint32_t mask = CVarGetInteger("gRando.CheckTracker.VisibilityBtn", 0x20) & 0xffff;
    CVarSetInteger(kShortcut, mask ? mask : 0x20);
    CVarSetFloat("gCombo.CheckTracker.Scale", std::clamp(CVarGetFloat("gRando.CheckTracker.Scale", 1), 0.7f, 2.5f));
    CVarSetFloat("gCombo.CheckTracker.Opacity",
                 std::clamp(CVarGetFloat("gRando.CheckTracker.Opacity", .75f), 0.1f, 1.0f));
    CVarSetInteger("gCombo.CheckTracker.Collected", CVarGetInteger("gRando.CheckTracker.CollectedMode", 2) != 2);
    CVarSetInteger("gCombo.CheckTracker.Skipped", CVarGetInteger("gRando.CheckTracker.SkippedMode", 2) != 2);
    CVarSetInteger("gCombo.CheckTracker.CurrentArea", CVarGetInteger("gRando.CheckTracker.ShowCurrentScene", 0));
    CVarSetInteger("gCombo.CheckTracker.AutoScroll", CVarGetInteger("gRando.CheckTracker.ScrollToCurrentScene", 1));
    CVarSetInteger("gCombo.CheckTracker.ShowLogic", CVarGetInteger("gRando.CheckTracker.OutOfLogicMode", 1) != 0);
    CVarSetInteger("gCombo.CheckTracker.AvailableOnly", CVarGetInteger("gRando.CheckTracker.OutOfLogicMode", 1) == 2);
    Save();
}
void SuppressOldWindows() {
    for (int g = 0; g < 2; ++g)
        for (int i : { 2, 3 }) {
            const auto& w = ComboTracker::kTrackers[g][i];
            auto win = Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGuiWindow(w.name);
            if (CVarGetInteger(w.cvar, 0) || (win && win->IsVisible()))
                ComboTracker::SetTracker(w, false);
        }
}
const char* Status(const Row& row) {
    if (IsCollected(row.flags))
        return "Collected";
    if (IsSkipped(row.flags))
        return "Skipped";
    if (row.flags & COMBO_CHECK_SCUMMED)
        return "Unsaved";
    if (row.flags & COMBO_CHECK_SEEN)
        return "Seen";
    if (row.flags & COMBO_CHECK_HINTED)
        return "Hinted";
    if (row.flags & COMBO_CHECK_KNOWN)
        return "Known";
    if (row.flags & COMBO_CHECK_LOGIC_KNOWN)
        return OutOfLogic(row.flags) ? "Unavailable" : "Available";
    return "Unchecked";
}
ImVec4 Color(const Row& row) {
    if (IsCollected(row.flags))
        return { .45f, .9f, .6f, 1 };
    if (IsSkipped(row.flags))
        return { 1.0f, .4f, 1.0f, 1 };
    if (row.flags & (COMBO_CHECK_SEEN | COMBO_CHECK_SCUMMED | COMBO_CHECK_HINTED | COMBO_CHECK_KNOWN))
        return { .55f, .8f, 1, 1 };
    if (OutOfLogic(row.flags))
        return { .6f, .63f, .68f, 1 };
    return { .95f, .95f, .97f, 1 };
}

const char* TypeIcon(const Row& row) {
    if (row.type == "Heart")
        return ICON_FA_HEART;
    if (row.type == "Song")
        return ICON_FA_MUSIC;
    if (row.type == "Shop")
        return ICON_FA_SHOPPING_BAG;
    if (row.type == "Chest" || row.name.find("Chest") != std::string::npos)
        return ICON_FA_ARCHIVE;
    if (row.type == "Crate" || row.type == "Barrel" || row.type == "Pot")
        return ICON_FA_CUBE;
    if (row.type == "Skulltula" || row.type == "Beehive")
        return ICON_FA_BUG;
    if (row.type == "Fairy" || row.type == "Wonder item")
        return ICON_FA_MAGIC;
    if (row.type == "Grass" || row.type == "Tree")
        return ICON_FA_LEAF;
    if (row.type == "Cow" || row.type == "Frog" || row.type == "Fish")
        return ICON_FA_PAW;
    if (row.type == "Dungeon reward")
        return ICON_FA_STAR;
    if (row.type == "Freestanding")
        return ICON_FA_DIAMOND;
    return ICON_FA_GIFT;
}

std::string RowLabel(const Row& row) {
    std::string label = row.name;
    if (IsSkipped(row.flags))
        label += " (Skipped)";
    else if (IsCollected(row.flags))
        label += " (" + (row.reward.empty() ? std::string("Collected") : row.reward) + ")";
    else if (row.flags & COMBO_CHECK_SCUMMED)
        label += " (Unsaved)";
    else if (row.flags & (COMBO_CHECK_SEEN | COMBO_CHECK_HINTED | COMBO_CHECK_KNOWN)) {
        const std::string state = Status(row);
        label += row.reward.empty() ? " (" + state + ")" : " (" + state + ": " + row.reward + ")";
    }
    return label;
}

class Window final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void Draw() override {
        auto ctx = Ship::Context::GetRawInstance();
        if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui())
            return;
        ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
        SuppressOldWindows();
        const int game = ComboUI::GetForegroundGame() == 1 ? 1 : 0;
        const auto& api = API(game);
        info = { 0, -1, -1 };
        if (api.snapshot)
            api.snapshot(&info, nullptr, nullptr);
        const int context = info.loaded ? game * 4 + info.slot : -1;
        if (lastContext != context) {
            areas.clear();
            totals = {};
            search.Clear();
            selectedCheck = -1;
            dirty = true;
            scrollToArea = true;
            lastContext = context;
            lastArea = -1;
        }
        if (lastArea != info.currentArea) {
            lastArea = info.currentArea;
            scrollToArea = true;
            dirty = true;
        }
        if (!ComboTrackerUI::Visible(ComboTrackerUI::Check))
            return;
        if (dirty || ImGui::GetTime() - lastRefresh >= .3) {
            areas.clear();
            totals = {};
            api.snapshot(&info, Collect, nullptr);
            for (auto& area : areas)
                std::stable_sort(area.rows.begin(), area.rows.end(), [](const Row& a, const Row& b) {
                    if (IsCollected(a.flags) != IsCollected(b.flags))
                        return !IsCollected(a.flags);
                    return a.name < b.name;
                });
            dirty = false;
            lastRefresh = ImGui::GetTime();
        }
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float unit = std::max(1.0f, ImGui::GetFontSize() / 16);
        if (ComboTrackerUI::Begin(ComboTrackerUI::Check)) {
            const bool showTools = CVarGetInteger("gCombo.CheckTracker.ShowTools", 0);
            if (showTools) {
                ImGui::TextUnformatted(game ? "Majora's Mask" : "Ocarina of Time");
                ImGui::Text("%d / %d collected  |  %d skipped%s", totals.collected, totals.total, totals.skipped,
                            totals.hiddenArea ? "  (known areas)" : "");
                ComboMenu_PushInput(ComboMenu_ThemeColor());
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputTextWithHint("##search", "Search checks, areas, types or known rewards",
                                             search.InputBuf, sizeof(search.InputBuf)))
                    search.Build();
                ComboMenu_PopInput();
                ComboMenu_PushCheckbox(ComboMenu_ThemeColor());
                Check("Collected", "gCombo.CheckTracker.Collected", 0);
                ImGui::SameLine();
                Check("Skipped", "gCombo.CheckTracker.Skipped", 0);
                ImGui::SameLine();
                Check("Current area", "gCombo.CheckTracker.CurrentArea", 0);
                ComboMenu_PopCheckbox();
                ComboMenu_PushButton(ComboMenu_ThemeColor());
                if (ImGui::Button("Expand all"))
                    expandAll = 1;
                ImGui::SameLine();
                if (ImGui::Button("Collapse all"))
                    expandAll = 0;
                ImGui::SameLine();
                if (ImGui::Button("Find current")) {
                    scrollToArea = true;
                    forceScroll = true;
                }
                ComboMenu_PopButton();
                ImGui::Separator();
            } else if (search.IsActive()) {
                search.Clear();
            }
            if (ImGui::BeginChild("Checks", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
                DrawRows(game);
            ImGui::EndChild();
            DrawDetails(api, display, unit);
        }
        ComboTrackerUI::End(ComboTrackerUI::Check);
    }

  protected:
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
    }

  private:
    bool forceScroll = false;
    bool openDetails = false;
    void DrawRows(int game) {
        const bool showCollected = CVarGetInteger("gCombo.CheckTracker.Collected", 0);
        const bool showSkipped = CVarGetInteger("gCombo.CheckTracker.Skipped", 0);
        const bool currentOnly = CVarGetInteger("gCombo.CheckTracker.CurrentArea", 0);
        const bool availableOnly = CVarGetInteger("gCombo.CheckTracker.ShowLogic", 1) &&
                                   CVarGetInteger("gCombo.CheckTracker.AvailableOnly", 0);
        const bool autoScroll = CVarGetInteger("gCombo.CheckTracker.AutoScroll", 1) || forceScroll;
        int shown = 0;
        ImGui::PushID(game);
        for (const auto& area : areas) {
            if (currentOnly && area.id != info.currentArea)
                continue;
            std::vector<const Row*> rows;
            for (const auto& row : area.rows)
                if (PassesFilters(row.flags, showCollected, showSkipped, availableOnly) &&
                    search.PassFilter(row.searchable.c_str()))
                    rows.push_back(&row);
            if (rows.empty())
                continue;
            ++shown;
            ImGui::PushID(area.id);
            ImGui::Separator();
            const bool target = scrollToArea && autoScroll && area.id == info.currentArea && !search.IsActive();
            if (expandAll >= 0)
                ImGui::SetNextItemOpen(expandAll != 0);
            else if (target)
                ImGui::SetNextItemOpen(true);
            else
                ImGui::SetNextItemOpen(area.id == info.currentArea, ImGuiCond_Once);
            std::string heading = area.name +
                                  (area.totals.hiddenArea ? " (?)"
                                                          : " (" + std::to_string(area.totals.collected) + "/" +
                                                                std::to_string(area.totals.total) + ")") +
                                  "###area";
            const bool complete = !area.totals.hiddenArea && area.totals.collected == area.totals.total;
            ImGui::PushStyleColor(ImGuiCol_Text, complete ? ImVec4(.4f, 1, .4f, 1) : ImVec4(1, 1, 1, 1));
            bool expanded = ImGui::CollapsingHeader(heading.c_str());
            ImGui::PopStyleColor();
            if (target) {
                ImGui::SetScrollHereY(0);
                scrollToArea = false;
                forceScroll = false;
            }
            if (expanded)
                ImGui::Indent(ImGui::GetFontSize() * .65f);
            if (expanded)
                for (const Row* row : rows) {
                    if (row->flags & COMBO_CHECK_AREA_HIDDEN) {
                        ImGui::TextWrapped("%s", row->name.c_str());
                        continue;
                    }
                    ImGui::PushID(row->id);
                    ImGui::PushStyleColor(ImGuiCol_Text, Color(*row));
                    ImGui::TextUnformatted(TypeIcon(*row));
                    ImGui::SameLine();
                    const std::string label = RowLabel(*row) + "##check";
                    if (ImGui::Selectable(label.c_str(), false, 0, ImVec2(0, ImGui::GetTextLineHeight() * 1.15f))) {
                        selectedCheck = row->id;
                        openDetails = true;
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
            if (expanded)
                ImGui::Unindent(ImGui::GetFontSize() * .65f);
            ImGui::PopID();
        }
        ImGui::PopID();
        expandAll = -1;
        if (!shown)
            ImGui::TextWrapped("No checks match these filters. Try showing collected or skipped checks, clearing "
                               "search, or turning off Current area.");
    }

    void DrawDetails(const GameAPI& api, ImVec2 display, float unit) {
        if (openDetails) {
            ImGui::OpenPopup("Check details");
            openDetails = false;
        }
        ImGui::SetNextWindowSize(ImVec2(std::min(420 * unit, display.x * .8f), 0), ImGuiCond_Appearing);
        if (ImGui::BeginPopup("Check details")) {
            const Row* selected = nullptr;
            for (const auto& area : areas)
                for (const auto& row : area.rows)
                    if (row.id == selectedCheck)
                        selected = &row;
            if (!selected)
                ImGui::CloseCurrentPopup();
            else {
                ImGui::TextWrapped("%s", selected->name.c_str());
                ImGui::TextColored(Color(*selected), "%s | %s", Status(*selected), selected->type.c_str());
                if (!selected->reward.empty())
                    ImGui::TextWrapped("Reward: %s", selected->reward.c_str());
                if (!selected->logic.empty() && ImGui::TreeNode("Logic details")) {
                    ImGui::TextWrapped("%s", selected->logic.c_str());
                    ImGui::TreePop();
                }
                ComboMenu_PushButton(ComboMenu_ThemeColor());
                if (!IsCollected(selected->flags) && api.skip) {
                    if (ImGui::Button(IsSkipped(selected->flags) ? "Unskip check" : "Skip check")) {
                        dirty = api.skip(info.slot, selected->id, !IsSkipped(selected->flags)) != 0;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                }
                if (ImGui::Button("Close"))
                    ImGui::CloseCurrentPopup();
                ComboMenu_PopButton();
            }
            ImGui::EndPopup();
        }
    }
};
std::shared_ptr<Window> window;
} // namespace

void RegisterCheckTrackerWindow() {
    Migrate();
    ComboTrackerUI::Migrate(ComboTrackerUI::Check);
    if (!window)
        window = std::make_shared<Window>("", true, " Combo Check Tracker");
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->AddGuiWindow(window);
}

void DrawUnifiedCheckTrackerSettings() {
    const ImVec4 theme = ComboMenu_ThemeColor();
    ComboTrackerUI::DrawEnabled(ComboTrackerUI::Check);
    ImGui::Spacing();
    if (ImGui::BeginTable("##shared-check-settings", 2,
                          ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableNextColumn();
        ComboTrackerUI::DrawOpenControls(ComboTrackerUI::Check);
        ImGui::TableNextColumn();
        ComboTrackerUI::DrawAppearance(ComboTrackerUI::Check);
        ComboMenu_PushCheckbox(theme);
        Check("Show search and filter toolbar", "gCombo.CheckTracker.ShowTools", 0);
        Check("Show collected checks", "gCombo.CheckTracker.Collected", 0);
        Check("Show skipped checks", "gCombo.CheckTracker.Skipped", 0);
        Check("Only show current area", "gCombo.CheckTracker.CurrentArea", 0);
        Check("Follow current area", "gCombo.CheckTracker.AutoScroll", 1);
        Check("Show availability and logic", "gCombo.CheckTracker.ShowLogic", 1);
        if (CVarGetInteger("gCombo.CheckTracker.ShowLogic", 1))
            Check("Hide unavailable checks", "gCombo.CheckTracker.AvailableOnly", 0);
        ComboMenu_PopCheckbox();
        ImGui::EndTable();
    }
}
} // namespace ComboRando
