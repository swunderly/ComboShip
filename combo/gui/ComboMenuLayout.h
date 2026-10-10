#pragma once
#include <imgui.h>
#include "ComboWidgetStyle.h"
#include <algorithm>

namespace ComboRando {
inline void MenuTooltip(const char* text) {
    if (!text || !*text ||
        !(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemFocused() || ImGui::IsItemActive()))
        return;
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 36.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}
inline void MenuWorldButtons(int& selected) {
    selected = std::clamp(selected, 0, 1);
    if (ComboMenu_StyledHeaderEntry("Ocarina of Time", selected == 0))
        selected = 0;
    const float need = ImGui::CalcTextSize("Majora's Mask").x + 32.0f;
    if (ImGui::GetItemRectMax().x + need + 12.0f < ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x)
        ImGui::SameLine();
    if (ComboMenu_StyledHeaderEntry("Majora's Mask", selected == 1))
        selected = 1;
    ImGui::Spacing();
}
inline int MenuColumns(float minWidth = 20.0f) {
    return ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * minWidth * 2 ? 2 : 1;
}
} // namespace ComboRando
