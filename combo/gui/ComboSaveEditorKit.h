#pragma once

#include "ComboTouchSlider.h"
#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Shared presentation for both Save Editors (OoT soh/soh/Enhancements/debugger/debugSaveEditor.cpp and MM
// mm/2s2h/DeveloperTools/SaveEditor.cpp) so the two read as mirrors: the same tabs, sizes, cards, coloured resource
// bars, tap rule and wording. Pure ImGui; each editor keeps its own data and its own give/remove code.
namespace ComboSaveEditorKit {

// Sizes. Android uses MM's large touch targets; desktop keeps MM's desktop values.
#if defined(__ANDROID__)
// The row height reserves room below an icon for its ammo count, so the count never overlaps the next row.
inline constexpr float kIconSize = 120.0f;
inline constexpr float kGridWidth = 136.0f;
inline constexpr float kGridHeight = 180.0f;
inline constexpr float kGridPadding = 16.0f;
inline constexpr float kGridTopMargin = 28.0f;
inline constexpr float kAmmoWidth = 60.0f;
inline constexpr float kAmmoPadding = 8.0f;
inline constexpr int kPickerColumns = 5;
#else
inline constexpr float kIconSize = 40.0f;
inline constexpr float kGridWidth = 46.0f;
inline constexpr float kGridHeight = 58.0f;
inline constexpr float kGridPadding = 10.0f;
inline constexpr float kGridTopMargin = 20.0f;
inline constexpr float kAmmoWidth = 24.0f;
inline constexpr float kAmmoPadding = 4.0f;
inline constexpr int kPickerColumns = 8;
#endif
inline constexpr float kSongIconWidth = kIconSize * 2.0f / 3.0f;
inline constexpr float kCardMinWidth = 420.0f;
inline constexpr int kAmmoMax = 99; // more draws garbage digits, less than 0 crashes

// Colours: the values both games' UIWidgets::ColorValues give these names (the two tables are identical).
inline constexpr ImVec4 kRed(0.55f, 0.0f, 0.0f, 1.0f);
inline constexpr ImVec4 kDarkRed(0.3f, 0.0f, 0.0f, 1.0f);
inline constexpr ImVec4 kGreen(0.0f, 0.55f, 0.0f, 1.0f);
inline constexpr ImVec4 kDarkGreen(0.0f, 0.3f, 0.0f, 1.0f);
// What each colour means. Everything else uses the menu theme colour.
inline constexpr ImVec4 kHealth = kDarkRed;
inline constexpr ImVec4 kHealthMax = kRed;
inline constexpr ImVec4 kMagic = kDarkGreen;
inline constexpr ImVec4 kRupees = kGreen;
inline constexpr ImVec4 kHeartPieces = kRed;
inline constexpr ImVec4 kGiveAll = kGreen;
inline constexpr ImVec4 kReset = kRed;

// Tabs, in the order both editors draw them. MM then adds kTabRandomizer and, last, kTabRegEditor.
inline constexpr const char* kTabGeneral = "General";
inline constexpr const char* kTabInventory = "Inventory";
inline constexpr const char* kTabEquipment = "Equipment";
inline constexpr const char* kTabQuestStatus = "Quest Status";
inline constexpr const char* kTabDungeonItems = "Dungeon Items";
inline constexpr const char* kTabFlags = "Flags";
inline constexpr const char* kTabPlayer = "Player";
inline constexpr const char* kTabRandomizer = "Randomizer";
inline constexpr const char* kTabRegEditor = "Reg Editor";

// Wording shared by both editors.
inline constexpr const char* kNoOotFile = "Load an Ocarina of Time file to edit it.";
inline constexpr const char* kNoMmFile = "Load a Majora's Mask file to edit it.";
inline constexpr const char* kNeedsPlay = "Available while playing.";
inline constexpr const char* kInventoryHelp =
    "Faded icons are not owned. Tap to add or remove; slots with more than one item open a list.";
inline constexpr const char* kRestrictLabel = "Restrict to valid items";
inline constexpr const char* kRestrictTooltip =
    "Offer only the items and amounts the game can give, in the slots that hold them.";
inline constexpr const char* kMagicLevelNames[] = { "No magic", "Single magic", "Double magic" };
inline constexpr const char* kWalletNames[] = { "Child Wallet", "Adult Wallet", "Giant Wallet", "Tycoon Wallet" };

// The tap rule (Sam's example 1): when only one item can go in a slot, a tap adds or removes it; when several can,
// a tap opens a picker with None first. With the restriction off every slot opens the picker.
enum class Tap { Toggle, Pick };
constexpr Tap ToggleOrPick(bool restrictToValid, size_t candidates) {
    return restrictToValid && candidates == 1 ? Tap::Toggle : Tap::Pick;
}
constexpr const char* TapHint(Tap tap) {
    return tap == Tap::Toggle ? "Tap to add or remove." : "Tap to choose.";
}

// Time of day in both games: 0x0000 is midnight, 0x4000 6:00 am, 0x8000 noon. "6:00 am", or "06:00" in 24-hour mode.
inline std::string ClockText(uint16_t time, bool use24Hour = false) {
    const int minutes = static_cast<int>((static_cast<uint32_t>(time) * 24u * 60u) >> 16);
    const int hours = minutes / 60;
    char text[16];
    if (use24Hour) {
        std::snprintf(text, sizeof(text), "%02d:%02d", hours, minutes % 60);
    } else {
        std::snprintf(text, sizeof(text), "%d:%02d %s", hours % 12 ? hours % 12 : 12, minutes % 60,
                      hours >= 12 ? "pm" : "am");
    }
    return text;
}

// Buttons: one colour, MM's rounding and padding.
inline void PushButtonStyle(const ImVec4& colour) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(colour.x, colour.y, colour.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(colour.x, colour.y, colour.z, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(colour.x, colour.y, colour.z, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
}
inline void PopButtonStyle() {
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}
inline bool Button(const char* label, const ImVec4& colour, const ImVec2& size = ImVec2(0.0f, 0.0f)) {
    PushButtonStyle(colour);
    const bool pressed = ImGui::Button(label, size);
    PopButtonStyle();
    return pressed;
}
inline void Tooltip(const char* text) {
    if (text && *text && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", text);
    }
}

// Bars: a full-width coloured slider with the value written inside and no keyboard entry (MM's General tab).
inline void PushBarStyle(const ImVec4& colour) {
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(colour.x, colour.y, colour.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(colour.x, colour.y, colour.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(colour.x, colour.y, colour.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(colour.x, colour.y, colour.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
}
inline void PopBarStyle() {
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(6);
}
// `format` holds the label and value, e.g. "Health: %d"; a format without %d shows as written (level names).
template <typename T>
inline bool ResourceBar(const char* id, T* value, int minimum, int maximum, const char* format, const ImVec4& colour) {
    PushBarStyle(colour);
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool changed = ComboTouch::Count(id, value, minimum, maximum, format);
    PopBarStyle();
    return changed;
}

// Every tab draws inside MM's bordered, rounded panel with 8 px spacing.
inline void BeginPanel(const char* id) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
}
inline void EndPanel() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

// Masonry cards, the same as MM's UIWidgets::BeginCardLayout/BeginCard/EndCard/EndCardLayout: each card goes to the
// shortest column, and a narrow screen gets one column. Layouts do not nest.
struct CardLayoutState {
    std::vector<float> x, height;
    float width = 0.0f, startY = 0.0f, spacing = 8.0f;
    int column = 0;
};
inline CardLayoutState& CurrentCardLayout() {
    static CardLayoutState state;
    return state;
}
inline void BeginCardLayout(int columnsPerRow = 2, float minColumnWidth = kCardMinWidth) {
    auto& state = CurrentCardLayout();
    const float avail = ImGui::GetContentRegionAvail().x;
    int columns = static_cast<int>((avail + state.spacing) / (minColumnWidth + state.spacing));
    columns = std::clamp(columns, 1, std::max(columnsPerRow, 1));
    state.width = (avail - state.spacing * (columns - 1)) / columns;
    state.x.assign(columns, 0.0f);
    state.height.assign(columns, 0.0f);
    for (int i = 0; i < columns; i++) {
        state.x[i] = ImGui::GetCursorPosX() + i * (state.width + state.spacing);
    }
    state.startY = ImGui::GetCursorPosY();
    state.column = 0;
}
inline void BeginCard(const char* id) {
    auto& state = CurrentCardLayout();
    state.column = static_cast<int>(std::min_element(state.height.begin(), state.height.end()) - state.height.begin());
    ImGui::SetCursorPosX(state.x[state.column]);
    ImGui::SetCursorPosY(state.startY + state.height[state.column]);
    ImGui::BeginChild(id, ImVec2(state.width, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
    ImGui::PushItemWidth(-FLT_MIN);
}
inline void EndCard() {
    auto& state = CurrentCardLayout();
    ImGui::PopItemWidth();
    ImGui::EndChild();
    state.height[state.column] += ImGui::GetItemRectSize().y + state.spacing;
}
inline void EndCardLayout() {
    auto& state = CurrentCardLayout();
    const float tallest = *std::max_element(state.height.begin(), state.height.end());
    if (tallest > 0.0f) {
        ImGui::SetCursorPosY(state.startY + tallest - state.spacing);
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
    }
}

// An item icon on a transparent button, faded when the item is not owned (MM's grid style).
inline bool IconButton(const char* id, ImTextureID texture, bool owned,
                       const ImVec2& size = ImVec2(kIconSize, kIconSize)) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    const bool pressed =
        ImGui::ImageButton(id, texture, size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                           ImVec4(1.0f, 1.0f, 1.0f, owned ? 1.0f : 0.4f));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    return pressed;
}
// The same button for an empty slot or the picker's None.
inline bool EmptyIconButton(const char* id, const ImVec2& size = ImVec2(kIconSize, kIconSize)) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    const bool pressed = ImGui::Button(id, size);
    ImGui::PopStyleColor(3);
    return pressed;
}

// Item grids: six icons a row, each cell tall enough for its ammo count under the icon (MM's Items and Masks boxes).
inline ImVec2 GridBoxSize(int rows = 4) {
    return ImVec2(kGridWidth * 6 + kGridPadding * 2, kGridHeight * rows + kGridPadding * 2 + kGridTopMargin);
}
inline ImVec2 GridCell(int index) {
    return ImVec2((index % 6) * kGridWidth + kGridPadding, (index / 6) * kGridHeight + kGridTopMargin + kGridPadding);
}
inline ImVec2 GridAmmo(int index) {
    const ImVec2 cell = GridCell(index);
    return ImVec2(cell.x + (kIconSize - kAmmoWidth) * 0.5f, cell.y + kIconSize + 6.0f);
}
// An ammo count under its icon, 0 to 99, no keyboard entry.
template <typename T> inline bool AmmoCount(const char* id, T* value, const ImVec4& colour) {
    ImGui::PushItemWidth(kAmmoWidth);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, colour);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(kAmmoPadding, kAmmoPadding));
    const bool changed = ComboTouch::Count(id, value, 0, kAmmoMax);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();
    return changed;
}

// One picker entry, in rows of kPickerColumns with None first (index 0): the icon with its name under it, since a
// touch screen shows no hover tooltips. A null texture draws an empty button (None).
inline bool PickerEntry(int index, const char* id, ImTextureID texture, const char* name) {
    if (index % kPickerColumns != 0) {
        ImGui::SameLine();
    }
    ImGui::BeginGroup();
    const bool pressed = texture ? IconButton(id, texture, true) : EmptyIconButton(id);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + kIconSize);
    ImGui::TextWrapped("%s", name);
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();
    return pressed;
}

// No texture: PickerEntry and LevelPicker draw an empty button. ImTextureID is a macro for void* in the game builds,
// so a constant one is written "ImTextureID const" (a const pointer), never "const ImTextureID" (a pointer to const).
inline ImTextureID const kNoTexture = (ImTextureID)0;

// An icon that only shows state (a dungeon's small keys or stray fairies), faded when there are none.
inline void Icon(ImTextureID texture, bool owned, const ImVec2& size = ImVec2(kIconSize, kIconSize)) {
    ImGui::ImageWithBg(texture, size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
                       ImVec4(1.0f, 1.0f, 1.0f, owned ? 1.0f : 0.4f));
}

// Something with several levels (an upgrade, MM's sword or shield): the current level's icon with `label` under it,
// faded at level 0, and a tap opens a picker of every level with its name. levels[0] is None (no texture). Returns
// the level picked this frame, or -1.
struct Level {
    ImTextureID texture;
    const char* name;
};
inline int LevelPicker(const char* label, int level, const std::vector<Level>& levels) {
    ImGui::PushID(label);
    const bool owned = level > 0 && level < static_cast<int>(levels.size());
    ImTextureID const shown = owned ? levels[level].texture : levels.size() > 1 ? levels[1].texture : kNoTexture;
    ImGui::BeginGroup();
    const bool pressed = shown ? IconButton("##level", shown, owned) : EmptyIconButton("##level");
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + kIconSize);
    ImGui::TextWrapped("%s", label);
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();
    const std::string tooltip = std::string(owned ? levels[level].name : "None") + "\n" + TapHint(Tap::Pick);
    Tooltip(tooltip.c_str());
    if (pressed) {
        ImGui::OpenPopup("levelPicker");
    }
    int picked = -1;
    if (ImGui::BeginPopup("levelPicker")) {
        for (size_t i = 0; i < levels.size(); i++) {
            ImGui::PushID(static_cast<int>(i));
            if (PickerEntry(static_cast<int>(i), "##choice", i == 0 ? kNoTexture : levels[i].texture,
                            levels[i].name)) {
                picked = static_cast<int>(i);
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return picked;
}

} // namespace ComboSaveEditorKit
