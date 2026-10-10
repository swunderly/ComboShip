#include "ComboHudLayout.h"
#include "ComboHudLayoutModel.h"
#include "ComboWidgetStyle.h"
#include "ComboMenuLayout.h"

#include <libultraship/libultraship.h>
#include <fast/Fast3dWindow.h>
#include <fast/interpreter.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace ComboRando {
namespace {
struct Element {
    const char* name;
    const char* prefix;
    int x;
    int y;
    float scale;
    bool resizable;
    // OoT's heart/minimap/timer renderer applies an extra offset; the position
    // guide subtracts it when converting a drag to the engine's saved values.
    float offsetX = 0;
    float offsetY = 0;
};

const Element oot[] = {
    {"Hearts", "gCosmetics.HUD.HeartsCount", -70, -10, .7f, true, 100, 36.5f},
    {"Magic", "gCosmetics.HUD.MagicBar", 18, 34, 1, false},
    {"Rupees", "gCosmetics.HUD.Rupees", 26, 206, 1, false},
    {"Keys", "gCosmetics.HUD.SmallKey", 26, 190, 1, false},
    {"A button", "gCosmetics.HUD.AButton", 186, 18, 1, false},
    {"B button", "gCosmetics.HUD.BButton", 160, 18, .95f, true},
    {"C up", "gCosmetics.HUD.CUpButton", 254, 16, .5f, true},
    {"C left", "gCosmetics.HUD.CLeftButton", 227, 18, .87f, true},
    {"C down", "gCosmetics.HUD.CDownButton", 249, 34, .87f, true},
    {"C right", "gCosmetics.HUD.CRightButton", 271, 18, .87f, true},
    {"D-pad items", "gCosmetics.HUD.Dpad", 271, 55, 1, false},
    {"Minimap", "gCosmetics.HUD.Minimap", 250, 0, 1, false, 0, 180},
    {"Start button", "gCosmetics.HUD.StartButton", 136, 26, .75f, true, 0, -9.75f},
    {"Horse carrots", "gCosmetics.HUD.Carrots", 160, 64, 1, false},
    {"Timers", "gCosmetics.HUD.Timers", 26, 46, 1, false},
    {"Archery score", "gCosmetics.HUD.ArcheryScore", 20, 67, 1, false},
    {"Map title", "gCosmetics.HUD.TitleCard.Map", 0, 0, 1, false},
    {"Boss title", "gCosmetics.HUD.TitleCard.Boss", 0, 0, 1, false},
    {"Gameplay timer", "gCosmetics.HUD.IGT", 0, 0, 1, false},
    {"Stone of Agony", "gCosmetics.HUD.VisualSoA", 270, 50, 1, false},
};
const Element mm[] = {
    {"Hearts", "gHudEditor.Hearts", 30, 26, 1, true},
    {"Magic", "gHudEditor.Magic", 18, 34, 1, true},
    {"Rupees", "gHudEditor.Rupees", 26, 206, 1, true},
    {"Keys", "gHudEditor.Keys", 26, 190, 1, true},
    {"A button", "gHudEditor.A", 191, 18, 1, true},
    {"B button", "gHudEditor.B", 167, 17, 1, true},
    {"C up", "gHudEditor.CUp", 254, 16, 1, true},
    {"C left", "gHudEditor.CLeft", 227, 18, 1, true},
    {"C down", "gHudEditor.CDown", 249, 34, 1, true},
    {"C right", "gHudEditor.CRight", 271, 18, 1, true},
    {"D-pad items", "gHudEditor.DPad", 271, 55, 1, true},
    {"Minimap", "gHudEditor.Minimap", 295, 220, 1, true},
    {"Start button", "gHudEditor.Start", 136, 17, 1, true},
    {"Horse carrots", "gHudEditor.Carrots", 160, 64, 1, true},
    {"Timers", "gHudEditor.Timers", 26, 46, 1, true},
    {"Three-day clock", "gHudEditor.Clock", 160, 206, 1, true},
    {"Skull Kid timer", "gHudEditor.SkullKidTimer", 115, 200, 1, true},
    {"Minigame counter", "gHudEditor.Minigames", 20, 67, 1, true},
    {"Skulltulas", "gHudEditor.Skulltulas", 26, 190, 1, true},
};

void Save() {
    auto ctx = Ship::Context::GetRawInstance();
    if (ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui())
        ctx->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

float CurrentAspect() {
    auto ctx = Ship::Context::GetRawInstance();
    auto fast = ctx && ctx->GetWindow() ? std::dynamic_pointer_cast<Fast::Fast3dWindow>(ctx->GetWindow()) : nullptr;
    auto interp = fast ? fast->GetInterpreterWeak().lock() : nullptr;
    if (interp && interp->mCurDimensions.height)
        return static_cast<float>(interp->mCurDimensions.width) / interp->mCurDimensions.height;
    return 4.0f / 3.0f;
}

std::string Key(const Element& e, int game, const char* field) {
    if (game == 0) {
        if (std::string(field) == "Mode") return std::string(e.prefix) + ".PosType";
        if (std::string(field) == "X") return std::string(e.prefix) + ".PosX";
        if (std::string(field) == "Y") return std::string(e.prefix) + ".PosY";
    } else if (std::string(field) == "X" || std::string(field) == "Y") {
        return std::string(e.prefix) + ".Position." + field;
    }
    return std::string(e.prefix) + "." + field;
}

void Reset(const Element& e, int game) {
    for (const char* field : {"X", "Y", "Scale", "Mode"}) CVarClear(Key(e, game, field).c_str());
    if (game == 0) {
        CVarClear((std::string(e.prefix) + ".UseMargins").c_str());
        if (std::string(e.prefix) == "gCosmetics.HUD.HeartsCount") CVarClear("gCosmetics.HUD.Hearts.UseMargins");
    }
}

ComboHud::Anchor OriginalAnchor(const Element& e, int game) {
    if (game != 0) return ComboHud::Anchor::Center;
    const std::string prefix = e.prefix;
    if (prefix == "gCosmetics.HUD.HeartsCount" || prefix == "gCosmetics.HUD.MagicBar" ||
        prefix == "gCosmetics.HUD.Rupees" || prefix == "gCosmetics.HUD.SmallKey" ||
        prefix == "gCosmetics.HUD.Timers") return ComboHud::Anchor::Left;
    if (prefix.find("Button") != std::string::npos || prefix == "gCosmetics.HUD.Dpad" ||
        prefix == "gCosmetics.HUD.Minimap") return ComboHud::Anchor::Right;
    return ComboHud::Anchor::Center;
}

float ExtraX(const Element& e, int game, ComboHud::Anchor mode) {
    // OoT's unanchored timer draw adds 204 pixels after reading PosX.
    if (game == 0 && std::string(e.prefix) == "gCosmetics.HUD.Timers" && mode == ComboHud::Anchor::Center)
        return 204;
    return e.offsetX;
}

void InitializeCustom(const Element& e, int game, ComboHud::Anchor mode) {
    CVarSetInteger(Key(e, game, "Mode").c_str(), ComboHud::NativeMode(game, mode));
    CVarSetInteger(Key(e, game, "X").c_str(), ComboHud::Pixel(e.x + e.offsetX - ExtraX(e, game, mode)));
    CVarSetInteger(Key(e, game, "Y").c_str(), e.y);
    if (game == 0) {
        CVarSetInteger((std::string(e.prefix) + ".UseMargins").c_str(), 0);
        if (std::string(e.prefix) == "gCosmetics.HUD.HeartsCount") CVarSetInteger("gCosmetics.HUD.Hearts.UseMargins", 0);
    }
}

void Guide(const Element& e, int game, ComboHud::Anchor mode, int& x, int& y, float scale) {
    const float logicalWidth = ComboHud::LogicalWidth(CurrentAspect());
    const float width = std::min(ImGui::GetContentRegionAvail().x, 620.0f);
    const float height = std::clamp(width * 240.0f / logicalWidth, 95.0f, 200.0f);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##HudPositionGuide", ImVec2(width, height));
    auto draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(12, 16, 20, 230), 5);
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(80, 100, 110, 255), 5);
    // A centered 4:3 reference helps players keep information away from edges.
    const float pad = (logicalWidth - 320.0f) / 2.0f;
    if (pad > 0) {
        const float left = origin.x + pad / logicalWidth * width;
        const float right = origin.x + (pad + 320.0f) / logicalWidth * width;
        draw->AddLine(ImVec2(left, origin.y), ImVec2(left, origin.y + height), IM_COL32(45, 55, 65, 255));
        draw->AddLine(ImVec2(right, origin.y), ImVec2(right, origin.y + height), IM_COL32(45, 55, 65, 255));
    }
    const bool custom = mode != ComboHud::Anchor::Original && mode != ComboHud::Anchor::Hidden;
    const float actualX = custom ? x : e.x;
    const float actualY = custom ? y : e.y;
    const float extraY = game == 0 && std::string(e.prefix) == "gCosmetics.HUD.HeartsCount" ? 26 + scale * 15 : e.offsetY;
    const auto guideAnchor = custom ? mode : OriginalAnchor(e, game);
    const float px = ComboHud::CanvasX(actualX + ExtraX(e, game, custom ? mode : ComboHud::Anchor::Original), guideAnchor, logicalWidth);
    const float py = actualY + extraY;
    const ImVec2 textSize = ImGui::CalcTextSize(e.name);
    const ImVec2 center(origin.x + std::clamp(px / logicalWidth, 0.0f, 1.0f) * width,
                        origin.y + std::clamp(py / 240.0f, 0.0f, 1.0f) * height);
    const ImVec2 label(std::clamp(center.x, origin.x + 4, std::max(origin.x + 4, origin.x + width - textSize.x - 4)),
                       std::clamp(center.y, origin.y + 4, std::max(origin.y + 4, origin.y + height - textSize.y - 4)));
    draw->AddCircleFilled(center, 4, mode == ComboHud::Anchor::Hidden ? IM_COL32(100, 100, 100, 255) : IM_COL32(240, 208, 108, 255));
    draw->AddText(label, IM_COL32(230, 230, 230, 255), e.name);
    if (mode != ComboHud::Anchor::Hidden && ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const float targetX = std::clamp((mouse.x - origin.x) / width, 0.0f, 1.0f) * logicalWidth;
        const float targetY = std::clamp((mouse.y - origin.y) / height, 0.0f, 1.0f) * 240.0f;
        const auto activeMode = custom ? mode : ComboHud::Anchor::Center;
        if (!custom) InitializeCustom(e, game, activeMode);
        x = ComboHud::Pixel(ComboHud::NativeX(targetX, activeMode, logicalWidth) - ExtraX(e, game, activeMode));
        y = ComboHud::Pixel(targetY - extraY);
        CVarSetInteger(Key(e, game, "X").c_str(), x);
        CVarSetInteger(Key(e, game, "Y").c_str(), y);
        Save();
    }
}
} // namespace

void DrawHudLayout(int game) {
    game = game == 0 ? 0 : 1;
    const Element* elements = game == 0 ? oot : mm;
    const int count = game == 0 ? IM_ARRAYSIZE(oot) : IM_ARRAYSIZE(mm);
    static int selected[] = {0, 0};
    ImGui::PushID(game);
    const ImVec4 theme = ComboMenu_ThemeColor();
    ComboMenu_PushCombobox(theme);
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 420.0f));
    if (ImGui::BeginCombo("HUD element", elements[selected[game]].name)) {
        for (int i = 0; i < count; ++i)
            if (ImGui::Selectable(elements[i].name, selected[game] == i)) selected[game] = i;
        ImGui::EndCombo();
    }
    MenuTooltip("Choose an element, then drag its position guide or use the sliders. Changes apply immediately. Elements appear when the game normally shows them; the minimap varies by area. Hold Menu to move touch-controller buttons.");
    const Element& e = elements[selected[game]];
    ImGui::PushID(e.prefix);
    const std::string modeKey = Key(e, game, "Mode");
    auto anchor = ComboHud::ReadMode(game, CVarGetInteger(modeKey.c_str(), 0));
    int mode = static_cast<int>(anchor);
    const char* modes[] = {"Original", "Screen center", "Left edge", "Right edge", "Hidden"};
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 420.0f));
    if (ImGui::Combo("Position mode", &mode, modes, IM_ARRAYSIZE(modes))) {
        const auto next = static_cast<ComboHud::Anchor>(mode);
        if (anchor == ComboHud::Anchor::Original && next != ComboHud::Anchor::Hidden) InitializeCustom(e, game, next);
        CVarSetInteger(modeKey.c_str(), ComboHud::NativeMode(game, next));
        anchor = next;
        Save();
    }
    ComboMenu_PopCombobox();
    int x = CVarGetInteger(Key(e, game, "X").c_str(), e.x);
    int y = CVarGetInteger(Key(e, game, "Y").c_str(), e.y);
    float scale = CVarGetFloat(Key(e, game, "Scale").c_str(), e.scale);
    ImGui::TextUnformatted("Position guide: drag to move");
    Guide(e, game, anchor, x, y, scale);
    ImGui::BeginDisabled(anchor == ComboHud::Anchor::Original || anchor == ComboHud::Anchor::Hidden);
    ComboMenu_PushSlider(theme);
    const int limit = ComboHud::Pixel(ComboHud::LogicalWidth(CurrentAspect()));
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 620.0f));
    if (ImGui::SliderInt("Horizontal", &x, -limit, limit + 320, "%d", ImGuiSliderFlags_NoInput)) {
        CVarSetInteger(Key(e, game, "X").c_str(), x); Save();
    }
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 620.0f));
    if (ImGui::SliderInt("Vertical", &y, -240, 240, "%d", ImGuiSliderFlags_NoInput)) {
        CVarSetInteger(Key(e, game, "Y").c_str(), y); Save();
    }
    ImGui::BeginDisabled(!e.resizable);
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 620.0f));
    if (ImGui::SliderFloat("Size", &scale, e.scale * .5f, e.scale * 2.5f, "%.2f", ImGuiSliderFlags_NoInput)) {
        CVarSetFloat(Key(e, game, "Scale").c_str(), scale); Save();
    }
    ImGui::EndDisabled();
    ComboMenu_PopSlider();
    ImGui::EndDisabled();
    if (!e.resizable) ImGui::TextDisabled("This element uses a fixed size.");
    ComboMenu_PushButton(theme);
    if (ImGui::Button("Reset this element")) { Reset(e, game); Save(); }
    if (ImGui::GetContentRegionAvail().x > ImGui::CalcTextSize("Reset all HUD positions").x + 40) ImGui::SameLine();
    if (ImGui::Button("Reset all HUD positions")) {
        for (int i = 0; i < count; ++i) Reset(elements[i], game);
        if (game == 0) {
            CVarClear("gCosmetics.HUD.Hearts.UseMargins");
            for (const char* edge : {"T", "L", "R", "B"}) CVarClear((std::string("gCosmetics.HUD.Margin.") + edge).c_str());
        }
        Save();
    }
    ComboMenu_PopButton();
    ImGui::PopID();
    ImGui::PopID();
}
} // namespace ComboRando
