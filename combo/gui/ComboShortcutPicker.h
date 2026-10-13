#pragma once
// One button-shortcut picker for every shortcut in the menu (Settings button combinations through CW_BTN_SELECTOR,
// the tracker shortcuts): the same N64 names in one order, a summary line, the buttons, and a Reset button. Values are
// the native controller button masks the games already store; only the drawing is shared.
#include "ComboMenuLayout.h"
#include "ComboWidgetStyle.h"
#include <imgui.h>
#include <algorithm>
#include <cstdint>
#include <string>

namespace ComboRando {
struct ShortcutButton {
    const char* name;
    uint32_t mask;
};
inline constexpr ShortcutButton kShortcutButtons[] = {
    { "A", 0x8000 },      { "B", 0x4000 },       { "Z", 0x2000 },          { "Start", 0x1000 },
    { "L", 0x0020 },      { "R", 0x0010 },       { "C-Up", 0x0008 },       { "C-Down", 0x0004 },
    { "C-Left", 0x0002 }, { "C-Right", 0x0001 }, { "D-Up", 0x0800 },       { "D-Down", 0x0400 },
    { "D-Left", 0x0200 }, { "D-Right", 0x0100 }, { "Modifier 1", 0x0040 }, { "Modifier 2", 0x0080 },
};
// "L + R", in the picker's order.
inline std::string ShortcutSummary(uint32_t mask) {
    std::string text;
    for (const auto& button : kShortcutButtons)
        if (mask & button.mask) {
            if (!text.empty())
                text += " + ";
            text += button.name;
        }
    return text.empty() ? "None selected" : text;
}
// Draws the summary and, under "Change shortcut", the buttons and Reset. Returns true when `mask` changed.
inline bool DrawShortcutPicker(const char* id, uint32_t& mask, uint32_t defaultMask) {
    bool changed = false;
    ImGui::PushID(id);
    ImGui::Text("Shortcut: %s", ShortcutSummary(mask).c_str());
    if (ImGui::CollapsingHeader("Change shortcut")) {
        const auto theme = ComboMenu_ThemeColor();
        const int columns = std::clamp(int(ImGui::GetContentRegionAvail().x / (ImGui::GetFontSize() * 7.0f)), 1, 4);
        ComboMenu_PushCheckbox(theme);
        if (ImGui::BeginTable("##ShortcutButtons", columns,
                              ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings)) {
            for (const auto& button : kShortcutButtons) {
                ImGui::TableNextColumn();
                bool selected = (mask & button.mask) != 0;
                if (ImGui::Checkbox(button.name, &selected)) {
                    mask = selected ? mask | button.mask : mask & ~button.mask;
                    changed = true;
                }
                MenuTooltip(button.mask == 0x40 || button.mask == 0x80
                                ? "Modifier 1 and 2 are extra virtual buttons. Assign a physical button to them in "
                                  "Settings > Controls (controller editor). They are not automatically ZL or ZR."
                                : "These are mapped N64 inputs. Default: L/L1 = L, ZL/L2 = Z, ZR/R2 = R. All selected "
                                  "inputs form one chord.");
            }
            ImGui::EndTable();
        }
        ComboMenu_PopCheckbox();
        ComboMenu_PushButton(theme);
        if (ImGui::Button("Reset shortcut")) {
            mask = defaultMask;
            changed = true;
        }
        ComboMenu_PopButton();
    }
    ImGui::PopID();
    return changed;
}
} // namespace ComboRando
