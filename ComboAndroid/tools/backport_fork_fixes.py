# Adapted from swunderly/2ship2harkinian-Android, Save-Editor-Touch-Fix @ 9031e2d.
from pathlib import Path

path = Path("mm/2s2h/DeveloperTools/SaveEditor.cpp")
text = path.read_text(encoding='utf-8')

# ------------------------------------------------------------
# Android-only sizing for touch use.
# Keep desktop dimensions completely unchanged.
# ------------------------------------------------------------
old_constants = """constexpr float INV_GRID_WIDTH = 46.0f;
constexpr float INV_GRID_HEIGHT = 58.0f;
constexpr float INV_GRID_ICON_SIZE = 40.0f;
constexpr float INV_GRID_PADDING = 10.0f;
constexpr float INV_GRID_TOP_MARGIN = 20.0f;
"""

new_constants = """#if defined(__ANDROID__)
// Large touch targets for the Save Editor on phones/tablets.
// Row height deliberately reserves space BELOW an ammo-bearing icon so the
// numeric input can never overlap the icon in the following row.
constexpr float INV_GRID_WIDTH = 136.0f;
constexpr float INV_GRID_HEIGHT = 180.0f;
constexpr float INV_GRID_ICON_SIZE = 120.0f;
constexpr float INV_GRID_PADDING = 16.0f;
constexpr float INV_GRID_TOP_MARGIN = 28.0f;
constexpr float INV_GRID_AMMO_WIDTH = 60.0f;
constexpr float INV_GRID_AMMO_PADDING = 8.0f;
constexpr int INV_PICKER_COLUMNS = 5;
#else
constexpr float INV_GRID_WIDTH = 46.0f;
constexpr float INV_GRID_HEIGHT = 58.0f;
constexpr float INV_GRID_ICON_SIZE = 40.0f;
constexpr float INV_GRID_PADDING = 10.0f;
constexpr float INV_GRID_TOP_MARGIN = 20.0f;
constexpr float INV_GRID_AMMO_WIDTH = 24.0f;
constexpr float INV_GRID_AMMO_PADDING = 4.0f;
constexpr int INV_PICKER_COLUMNS = 8;
#endif
"""

if new_constants in text:
    pass
elif old_constants in text:
    text = text.replace(old_constants, new_constants, 1)
elif "INV_GRID_AMMO_WIDTH" not in text:
    raise SystemExit("ERROR: Could not locate Save Editor grid constants")

# ------------------------------------------------------------
# Put ammo quantities in a dedicated area below their icons and center them.
# ------------------------------------------------------------
old_ammo = """    ImGui::SetCursorPos(
        ImVec2(x * INV_GRID_WIDTH + INV_GRID_PADDING + 7.0f,
               y * INV_GRID_HEIGHT + INV_GRID_TOP_MARGIN + INV_GRID_PADDING + (INV_GRID_ICON_SIZE - 2.0f)));
    ImGui::PushItemWidth(24.0f);
"""

new_ammo = """    const float ammoXOffset = (INV_GRID_ICON_SIZE - INV_GRID_AMMO_WIDTH) * 0.5f;
    const float ammoYOffset = INV_GRID_ICON_SIZE + 6.0f;
    ImGui::SetCursorPos(
        ImVec2(x * INV_GRID_WIDTH + INV_GRID_PADDING + ammoXOffset,
               y * INV_GRID_HEIGHT + INV_GRID_TOP_MARGIN + INV_GRID_PADDING + ammoYOffset));
    ImGui::PushItemWidth(INV_GRID_AMMO_WIDTH);
"""

if new_ammo in text:
    pass
elif old_ammo in text:
    text = text.replace(old_ammo, new_ammo, 1)
elif "ammoYOffset" not in text:
    raise SystemExit("ERROR: Could not locate Save Editor ammo input layout")

old_ammo_padding = """    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
"""
new_ammo_padding = """    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(INV_GRID_AMMO_PADDING, INV_GRID_AMMO_PADDING));
"""

if new_ammo_padding in text:
    pass
elif old_ammo_padding in text:
    text = text.replace(old_ammo_padding, new_ammo_padding, 1)
elif "INV_GRID_AMMO_PADDING" not in text:
    raise SystemExit("ERROR: Could not locate ammo input frame padding")

# Fewer picker columns because each selection is now much larger.
old_wrap = """            if (((pickerIndex + 1) % 8) != 0) {
"""
new_wrap = """            if (((pickerIndex + 1) % INV_PICKER_COLUMNS) != 0) {
"""

if new_wrap in text:
    pass
elif old_wrap in text:
    text = text.replace(old_wrap, new_wrap, 1)
elif "INV_PICKER_COLUMNS" not in text:
    raise SystemExit("ERROR: Could not locate inventory picker column layout")

# ------------------------------------------------------------
# Android layout: give Items/Masks the FULL editor width. The desktop Save
# Editor places the controls panel beside the grids, which squeezes both areas
# on a touch screen. On Android, reserve the upper 75% for a scrollable full-
# width grid region and put Give All / Reset / Safe Mode / rando controls below.
# ------------------------------------------------------------
old_left_child = """    ImGui::BeginChild("leftSide##items", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX);
"""
new_left_child = """#if defined(__ANDROID__)
    const float gridPanelHeight = ImGui::GetContentRegionAvail().y * 0.75f;
    ImGui::BeginChild("leftSide##items", ImVec2(0, gridPanelHeight), ImGuiChildFlags_Border);
#else
    ImGui::BeginChild("leftSide##items", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX);
#endif
"""

if new_left_child in text:
    pass
elif old_left_child in text:
    text = text.replace(old_left_child, new_left_child, 1)
elif "gridPanelHeight" not in text:
    raise SystemExit("ERROR: Could not locate Items/Masks grid container")

old_panel_transition = """    ImGui::EndGroup();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("equipsBox", ImVec2(0, 0), true);
"""
new_panel_transition = """    ImGui::EndGroup();
    ImGui::EndChild();

#if defined(__ANDROID__)
    ImGui::Spacing();
#else
    ImGui::SameLine();
#endif
    ImGui::BeginChild("equipsBox", ImVec2(0, 0), true);
"""

if new_panel_transition in text:
    pass
elif old_panel_transition in text:
    text = text.replace(old_panel_transition, new_panel_transition, 1)
elif "#if defined(__ANDROID__)\n    ImGui::Spacing();" not in text:
    raise SystemExit("ERROR: Could not locate controls-panel transition")

path.write_text(text, encoding='utf-8', newline='\n')
print("Applied extra-large Android Save Editor touch layout.")


# User fork Grandma-Fix: convert u10.2 dimensions before multiplication.
path = Path("libultraship/src/fast/interpreter.cpp")
text = path.read_text(encoding="utf-8")
start = text.index("void Interpreter::Gfxs2dexBg1cyc(F3DuObjBg* bg)")
end = text.find("\nvoid Interpreter::", start + 1)
if end == -1: end = len(text)
function = text[start:end]
old = "GfxDpLoadBlock(G_TX_LOADTILE, 0, 0, (bg->b.imageW * bg->b.imageH >> 4) - 1, 0);"
new = "GfxDpLoadBlock(G_TX_LOADTILE, 0, 0, ((bg->b.imageW >> 2) * (bg->b.imageH >> 2)) - 1, 0);"
if old in function:
    if function.count(old) != 1: raise RuntimeError("Ambiguous background texture patch")
    path.write_text(text[:start] + function.replace(old, new) + text[end:], encoding="utf-8", newline="\n")
elif new not in function:
    raise RuntimeError("Background texture patch anchor changed")
print("Applied user fork background texture fix.")
