#include "ComboTrackerControls.h"
#include "ComboCheckSnapshot.h"
#include "ComboForeground.h"
#include "ComboTrackerSwap.h"
#include "ComboTrackerCommon.h"
#include "ComboTrackerBridge.h"
#include "ComboWidgetStyle.h"
#include "ComboMenuLayout.h"
#include "ComboExport.h"
#include <libultraship/controller/controldeck/ControlDeck.h>
#include <ship/window/gui/GuiWindow.h>
#include <imgui_internal.h>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>

namespace ComboTrackerUI {
namespace {
using namespace ComboChecks;
constexpr const char* prefixes[] = { "gCombo.CheckTracker.", "gCombo.Tracker.", "gCombo.HintTracker." };
constexpr const char* titles[] = { "Check Tracker##Combo", "Item Tracker##Combo", "Hint Tracker##Combo" };
constexpr const char* labels[] = { "check", "item", "hint" };
constexpr const char* touchLabels[] = { "CT", "IT", "HT" };
VisibilityState states[Count];
bool focus[Count]{}, open[Count]{};
int styles[Count] = { -1, -1, -1 };
ImGuiID dock = 0;
std::atomic<unsigned> pending{ 0 }, touchButtons{ 0 };
std::mutex boundsMutex;
float bounds[Count][5][4]{};
std::string Key(Kind k, const char* suffix) {
    return std::string(prefixes[k]) + suffix;
}
int Int(Kind k, const char* suffix, int def) {
    return CVarGetInteger(Key(k, suffix).c_str(), def);
}
void Set(Kind k, const char* suffix, int v) {
    CVarSetInteger(Key(k, suffix).c_str(), v);
}
float Float(Kind k, const char* suffix, float def) {
    return CVarGetFloat(Key(k, suffix).c_str(), def);
}
void Publish(Kind kind, bool shown) {
    std::lock_guard<std::mutex> lock(boundsMutex);
    for (auto& b : bounds[kind])
        std::fill(b, b + 4, 0.0f);
    auto* b = bounds[kind][0];
    if (!shown)
        return;
    const auto p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize(), d = ImGui::GetIO().DisplaySize;
    b[0] = p.x / d.x;
    b[1] = p.y / d.y;
    b[2] = (p.x + s.x) / d.x;
    b[3] = (p.y + s.y) / d.y;
}
void DockBesideGame(Kind kind) {
    // All three trackers share one dock, so enabling another never splits the game again.
    if (!dock || !ImGui::DockBuilderGetNode(dock)) {
        dock = 0;
        for (auto title : titles) {
            auto* w = ImGui::FindWindowByName(title);
            auto* saved = ImGui::FindWindowSettingsByID(ImHashStr(title));
            const ImGuiID candidate = w ? w->DockId : (saved ? saved->DockId : 0);
            if (candidate && ImGui::DockBuilderGetNode(candidate)) {
                dock = candidate;
                break;
            }
        }
    }
    if (!dock) {
        auto* main = ImGui::FindWindowByName("Main Game");
        auto* node = main ? ImGui::DockBuilderGetNode(main->DockId) : nullptr;
        if (node && !node->IsSplitNode()) {
            const ImGuiID root = node->ID;
            ImGuiID gameDock = 0;
            dock = ImGui::DockBuilderSplitNode(root, ImGuiDir_Right, .5f, nullptr, &gameDock);
            ImGui::DockBuilderDockWindow("Main Game", gameDock);
            ImGui::DockBuilderDockWindow(titles[kind], dock);
            if (auto* panel = ImGui::DockBuilderGetNode(dock))
                panel->LocalFlags &= ~(ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_HiddenTabBar);
            ImGui::DockBuilderFinish(root);
        }
    }
    if (dock)
        ImGui::SetNextWindowDockID(dock);
}
struct Button {
    const char* name;
    uint32_t mask;
};
constexpr Button buttons[] = { { "L", 0x20 },     { "R", 0x10 },       { "Z", 0x2000 },        { "Start", 0x1000 },
                               { "A", 0x8000 },   { "B", 0x4000 },     { "Modifier 1", 0x40 }, { "Modifier 2", 0x80 },
                               { "D-Up", 0x800 }, { "D-Down", 0x400 }, { "D-Left", 0x200 },    { "D-Right", 0x100 },
                               { "C-Up", 8 },     { "C-Down", 4 },     { "C-Left", 2 },        { "C-Right", 1 } };
class ItemWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void Draw() override {
        auto ctx = Ship::Context::GetRawInstance();
        if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui())
            return;
        ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
        for (int game = 0; game < 2; ++game) {
            const auto& w = ComboTracker::kTrackers[game][0];
            auto old = ctx->GetWindow()->GetGui()->GetGuiWindow(w.name);
            if (CVarGetInteger(w.cvar, 0) || (old && old->IsVisible()))
                ComboTracker::SetTracker(w, false);
        }
        if (!Visible(Item))
            return;
        if (Begin(Item)) {
            const int game = ComboUI::GetForegroundGame() == 1 ? 1 : 0;
            static void (*draw[2])(){};
            if (!draw[game])
                draw[game] = reinterpret_cast<void (*)()>(
                    Combo_ResolveSym(game ? "2ship" : "soh",
                                     game ? "MM_ComboItemTrackerDrawContents" : "SOH_ComboItemTrackerDrawContents"));
            if (draw[game])
                draw[game]();
        }
        End(Item);
    }

  protected:
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
    }
};
} // namespace
void Save() {
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}
bool Checkbox(const char* label, const char* cvar, int fallback) {
    bool value = CVarGetInteger(cvar, fallback) != 0;
    if (!ImGui::Checkbox(label, &value))
        return false;
    CVarSetInteger(cvar, value);
    Save();
    return true;
}
void Migrate(Kind kind) {
    if (Int(kind, "ControlsVersion", 0))
        return;
    Set(kind, "ControlsVersion", 1);
    if (Int(kind, "Visibility", -1) < 0) {
        // Retain explicit legacy access choices; fresh installs use pause access.
        int mode = Paused;
        if (kind == Item) {
            const int old = CVarGetInteger("gSettings.ItemTracker.VisibilityMode", -1);
            if (old >= 0)
                mode = std::clamp(old, 0, 3);
            if (Int(kind, "OnlyPaused", 0))
                mode = Paused;
        } else if (kind == Hint && Int(kind, "Enabled", 0) && !Int(kind, "OnlyPaused", 0))
            mode = Always;
        Set(kind, "Visibility", mode);
    }
    if (Int(kind, "Shortcut", -1) < 0)
        Set(kind, "Shortcut", kind == Item ? CVarGetInteger("gSettings.ItemTracker.VisibilityBtn", 0x20) : 0x20);
    if (Int(kind, "WindowType", -1) < 0)
        Set(kind, "WindowType", 2);
    Save();
}
bool Visible(Kind kind) {
    const int game = ComboUI::GetForegroundGame() == 1 ? 1 : 0;
    ComboCheckInfo info{ 0, -1, -1 };
    if (const auto snapshot = Snapshot(game))
        snapshot(&info, nullptr, nullptr);
    auto deck = std::dynamic_pointer_cast<LUS::ControlDeck>(Ship::Context::GetRawInstance()->GetControlDeck());
    const auto* pads = deck ? deck->GetPads() : nullptr;
    const bool menu = CVarGetInteger("gOpenWindows.Menu", 0) || ImGui::GetIO().WantTextInput;
    const int mode = std::clamp(Int(kind, "Visibility", Paused), 0, 4);
    const bool enabled = Int(kind, "Enabled", kind == Check ? 1 : 0) != 0;
    const unsigned bit = 1u << kind;
    const bool touch = (pending.fetch_and(~bit) & bit) != 0;
    const bool available = enabled && info.loaded && mode == OnScreen;
    if (available)
        touchButtons.fetch_or(bit);
    else
        touchButtons.fetch_and(~bit);
    const bool was = states[kind].open;
    const bool shown = states[kind].Update(info.loaded ? game * 4 + info.slot : -1, mode, Int(kind, "Shortcut", 0x20),
                                           pads ? pads[0].button : 0, enabled, info.loaded,
                                           ComboTracker::ForegroundPaused(game), menu, touch);
    if (!was && states[kind].open && shown)
        focus[kind] = true;
    if (!shown)
        Publish(kind, false);
    return shown;
}
bool Begin(Kind kind) {
    const ImVec2 d = ImGui::GetIO().DisplaySize;
    const float unit = std::max(1.0f, ImGui::GetFontSize() / 16);
    const int style = std::clamp(Int(kind, "WindowType", 2), 0, 2);
    if (style == 2)
        DockBesideGame(kind);
    else {
        ImGui::SetNextWindowDockID(0);
        const ImVec2 size(std::min(d.x * .7f, 485 * unit), std::min(d.y * .8f, 500 * unit));
        const auto cond = styles[kind] != style ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
        ImGui::SetNextWindowSize(size, cond);
        ImGui::SetNextWindowPos(ImVec2(d.x - size.x - 20, 50), cond);
    }
    styles[kind] = style;
    ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(220 * unit, d.x * .4f), std::min(160 * unit, d.y * .5f)), d);
    ImGui::SetNextWindowBgAlpha(style == 2 ? 1 : std::clamp(Float(kind, "Opacity", .75f), 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(.3f, .35f, .4f, .35f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(.3f, .35f, .4f, .5f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    if (focus[kind]) {
        ImGui::SetNextWindowFocus();
        focus[kind] = false;
    }
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing;
    if (style != 2)
        flags |= ImGuiWindowFlags_NoDocking;
    if (style == 0)
        flags |= ImGuiWindowFlags_NoTitleBar;
    open[kind] = true;
    const bool expanded = ImGui::Begin(titles[kind], &open[kind], flags);
    // An inactive dock tab has no interactive body. A collapsed window still owns its title.
    Publish(kind, !CVarGetInteger("gOpenWindows.Menu", 0) && !ImGui::GetCurrentWindow()->Hidden);
    if (expanded) {
        ImGui::SetWindowFontScale(std::clamp(Float(kind, "Scale", 1), .7f, 2.5f));
        if (style == 0) {
            ImGui::TextUnformatted(kind == Check ? "Check Tracker" : kind == Item ? "Item Tracker" : "Hint Tracker");
            ImGui::SameLine();
            if (ImGui::SmallButton("Close"))
                open[kind] = false;
        }
    }
    return expanded;
}
void End(Kind kind) {
    // Details popups can extend over the game. They must also keep touches out
    // of the invisible stick zones, not just the tracker window's rectangle.
    if (!CVarGetInteger("gOpenWindows.Menu", 0)) {
        std::lock_guard<std::mutex> lock(boundsMutex);
        const auto d = ImGui::GetIO().DisplaySize;
        int index = 1;
        for (const auto& popup : ImGui::GetCurrentContext()->OpenPopupStack) {
            auto* w = popup.Window;
            if (!w || w->Hidden || index >= 5)
                continue;
            auto* owner = w->ParentWindow;
            while (owner && owner != ImGui::GetCurrentWindow())
                owner = owner->ParentWindow;
            if (!owner)
                continue;
            auto* b = bounds[kind][index++];
            b[0] = w->Pos.x / d.x;
            b[1] = w->Pos.y / d.y;
            b[2] = (w->Pos.x + w->Size.x) / d.x;
            b[3] = (w->Pos.y + w->Size.y) / d.y;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    if (!open[kind]) {
        const int mode = Int(kind, "Visibility", Paused);
        if (mode == Toggle || mode == OnScreen)
            states[kind].open = false;
        else {
            Set(kind, "Enabled", 0);
            Save();
        }
        Publish(kind, false);
    }
}
void DrawEnabled(Kind kind) {
    const std::string label = std::string("Enable ") + labels[kind] + " tracker";
    ComboRando::ComboMenu_PushCheckbox(ComboRando::ComboMenu_ThemeColor());
    Checkbox(label.c_str(), Key(kind, "Enabled").c_str(), kind == Check ? 1 : 0);
    ComboRando::ComboMenu_PopCheckbox();
    ComboRando::MenuTooltip("Visibility, shortcut and appearance apply in both games. The item and check trackers "
                            "follow the active game; hints include both games.");
}
void DrawOpenControls(Kind kind) {
    const auto theme = ComboRando::ComboMenu_ThemeColor();
    ImGui::SeparatorText("Open and close");
    int mode = std::clamp(Int(kind, "Visibility", Paused), 0, 4);
    const char* modes[] = { "Always", "While paused", "Button toggle", "Button hold", "On-screen button" };
    ComboRando::ComboMenu_PushCombobox(theme);
    if (ImGui::Combo("Visibility", &mode, modes, 5)) {
        Set(kind, "Visibility", mode);
        Save();
    }
    ComboRando::ComboMenu_PopCombobox();
    ComboRando::MenuTooltip("While paused is the default. Button toggle opens and closes with a press; button hold "
                            "shows the tracker while held. On-screen button adds a separate touch control.");
    if (mode == Toggle || mode == Hold) {
        uint32_t mask = Int(kind, "Shortcut", 0x20);
        std::string shortcut;
        for (auto b : buttons)
            if (mask & b.mask) {
                if (!shortcut.empty())
                    shortcut += " + ";
                shortcut += b.name;
            }
        ImGui::Text("Shortcut: %s", shortcut.empty() ? "None selected" : shortcut.c_str());
        if (ImGui::CollapsingHeader("Change shortcut")) {
            ComboRando::ComboMenu_PushCheckbox(theme);
            if (ImGui::BeginTable("##buttons", 2)) {
                for (auto b : buttons) {
                    ImGui::TableNextColumn();
                    bool selected = (mask & b.mask) != 0;
                    if (ImGui::Checkbox(b.name, &selected)) {
                        mask = selected ? mask | b.mask : mask & ~b.mask;
                        Set(kind, "Shortcut", mask);
                        Save();
                    }
                    ComboRando::MenuTooltip(
                        b.mask == 0x40 || b.mask == 0x80
                            ? "Modifier 1 and 2 are extra virtual buttons. Assign a physical button to them in "
                              "Settings > Controls (controller editor). They are not automatically ZL or ZR. Select "
                              "several buttons here to require them together."
                            : "These are mapped N64 inputs. Default: L/L1 = L, ZL/L2 = Z, ZR/R2 = R. Use the "
                              "controller editor to assign a spare button to Modifier 1 or 2. All selected inputs form "
                              "one chord.");
                }
                ImGui::EndTable();
            }
            ComboRando::ComboMenu_PopCheckbox();
        }
    } else if (mode == OnScreen) {
        const std::string help = std::string(touchLabels[kind]) +
                                 " toggles only this tracker. Hold Menu > Move controls to move it; Tracker button "
                                 "sizes changes its size. It stays available when the gamepad is hidden.";
        ImGui::Text("Touch button: %s", touchLabels[kind]);
        ComboRando::MenuTooltip(help.c_str());
    }
    if (mode == Toggle || mode == OnScreen) {
        ComboRando::ComboMenu_PushButton(theme);
        if (ImGui::Button(states[kind].open ? "Hide tracker now" : "Open tracker now")) {
            states[kind].open = !states[kind].open;
            focus[kind] = states[kind].open;
            if (states[kind].open)
                Set(kind, "Enabled", 1);
            Save();
        }
        ComboRando::ComboMenu_PopButton();
    }
}
void DrawAppearance(Kind kind, bool textSize) {
    const auto theme = ComboRando::ComboMenu_ThemeColor();
    ImGui::SeparatorText("Appearance");
    int type = std::clamp(Int(kind, "WindowType", 2), 0, 2);
    const char* types[] = { "Overlay", "Floating window", "Side panel" };
    ComboRando::ComboMenu_PushCombobox(theme);
    if (ImGui::Combo("Style", &type, types, 3)) {
        Set(kind, "WindowType", type);
        Save();
    }
    ComboRando::ComboMenu_PopCombobox();
    ComboRando::MenuTooltip("Side panel reserves room beside the game. Trackers share the panel as tabs. Drag the "
                            "divider to resize it; floating windows can be moved and resized.");
    ComboRando::ComboMenu_PushSlider(theme);
    float scale = Float(kind, "Scale", 1), opacity = 100 * Float(kind, "Opacity", .75f);
    if (textSize && ImGui::SliderFloat("Text size", &scale, .7f, 2.5f, "%.1fx", ImGuiSliderFlags_NoInput)) {
        CVarSetFloat(Key(kind, "Scale").c_str(), scale);
        Save();
    }
    if (type != 2 && ImGui::SliderFloat("Background", &opacity, 0, 100, "%.0f%%", ImGuiSliderFlags_NoInput)) {
        CVarSetFloat(Key(kind, "Opacity").c_str(), opacity / 100);
        Save();
    }
    ComboRando::ComboMenu_PopSlider();
}
void RegisterItemWindow() {
    Migrate(Item);
    static auto window = std::make_shared<ItemWindow>("", true, " Combo Item Tracker");
    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->AddGuiWindow(window);
}
void DrawItemSettings() {
    DrawEnabled(Item);
    if (ImGui::BeginTable("##item-settings", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableNextColumn();
        DrawOpenControls(Item);
        ImGui::TableNextColumn();
        DrawAppearance(Item, false);
        ComboRando::ComboMenu_PushSlider(ComboRando::ComboMenu_ThemeColor());
        int size = Int(Item, "IconSize", 36), spacing = Int(Item, "IconSpacing", 12);
        if (ImGui::SliderInt("Icon size", &size, 16, 128, "%d px", ImGuiSliderFlags_NoInput)) {
            Set(Item, "IconSize", size);
            ComboTracker::SyncAppearance();
            Save();
        }
        if (ImGui::SliderInt("Icon spacing", &spacing, 0, 50, "%d px", ImGuiSliderFlags_NoInput)) {
            Set(Item, "IconSpacing", spacing);
            ComboTracker::SyncAppearance();
            Save();
        }
        ComboRando::ComboMenu_PopSlider();
        ImGui::SeparatorText("Groups");
        ComboRando::ComboMenu_PushCheckbox(ComboRando::ComboMenu_ThemeColor());
        Checkbox("Items", "gCombo.Tracker.Inventory", 1);
        Checkbox("Equipment and masks", "gCombo.Tracker.Equipment", 1);
        Checkbox("Songs", "gCombo.Tracker.Songs", 1);
        Checkbox("Quest items", "gCombo.Tracker.Quest", 1);
        Checkbox("Dungeon items", "gCombo.Tracker.Dungeons", 1);
        Checkbox("Show combined Triforce count", "gCombo.Rando.TriforceCounter", 1);
        ComboRando::ComboMenu_PopCheckbox();
        ImGui::EndTable();
    }
}
} // namespace ComboTrackerUI
extern "C" COMBO_EXPORT int ComboUI_TrackerTouchButtons() {
    return ComboTrackerUI::touchButtons.load();
}
extern "C" COMBO_EXPORT void ComboUI_ToggleTracker(int kind) {
    if (kind >= 0 && kind < ComboTrackerUI::Count)
        ComboTrackerUI::pending.fetch_or(1u << kind);
}
extern "C" COMBO_EXPORT int ComboUI_TrackerHitTest(float x, float y) {
    std::lock_guard<std::mutex> lock(ComboTrackerUI::boundsMutex);
    for (const auto& boxes : ComboTrackerUI::bounds)
        for (const auto& b : boxes)
            if (b[2] > b[0] && b[3] > b[1] && x >= b[0] && x <= b[2] && y >= b[1] && y <= b[3])
                return 1;
    return 0;
}
