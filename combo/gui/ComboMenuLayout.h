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
// One confirm dialog for every destructive menu action: its title, one line on what happens, then full-width themed
// buttons with Cancel first. Open it with ImGui::OpenPopup(title) in the same ID scope; returns true on the frame the
// action is confirmed.
inline bool MenuConfirm(const char* title, const char* consequence, const char* action) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(.5f, .5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(viewport->WorkSize.x * 0.82f, ImGui::GetFontSize() * 30.0f), 0),
                             ImGuiCond_Always);
    bool confirmed = false;
    if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("%s", consequence);
        ImGui::Spacing();
        const float height = ImGui::GetFrameHeight() * 1.25f;
        ComboMenu_PushButton(ComboMenu_ThemeColor());
        if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, height)))
            ImGui::CloseCurrentPopup();
        if (ImGui::Button(action, ImVec2(-FLT_MIN, height))) {
            confirmed = true;
            ImGui::CloseCurrentPopup();
        }
        ComboMenu_PopButton();
        ImGui::EndPopup();
    }
    return confirmed;
}
} // namespace ComboRando
