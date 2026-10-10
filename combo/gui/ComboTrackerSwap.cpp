// combo/gui/ComboTrackerSwap.cpp — see ComboTrackerSwap.h for rationale.
#include "ComboTrackerSwap.h"
#include "ComboTrackerCommon.h"
#include "ComboTrackerBridge.h" // canonical window-type defaults
#include "ComboForeground.h"
#include "ComboWidgetStyle.h"
#include <libultraship/libultraship.h>
#include <ship/resource/CrossRMRegistry.h>
#include <imgui_internal.h> // FindWindowByName / SetWindowPos / ImGuiWindow rects
#include "ComboResolve.h"   // Combo_ResolveSym (lazy MM save load for dormant draws)

namespace {

using ComboTracker::kKinds;
using ComboTracker::kTrackers;
using ComboTracker::OotActiveSlot;

// Combined Triforce progress (#136): counts live in the two game DLLs, the goal in soh's copy of the
// slot's seed. Returns false unless the loaded seed's goal is a hunt.
static int sNamedTriforceMask = -1;
bool ComboTriforceProgress(int& have, int& need) {
    sNamedTriforceMask = -1;
    static int (*sOot)(void) = nullptr;
    static int (*sMm)(void) = nullptr;
    static int (*sGoal)(int*) = nullptr;
    static int (*sType)(void) = nullptr, (*sOotNamed)(void) = nullptr, (*sMmNamed)(void) = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        sOot = (int (*)(void))Combo_ResolveSym("soh", "SOH_GetTriforcePieceCount");
        sGoal = (int (*)(int*))Combo_ResolveSym("soh", "SOH_GetComboGoal");
        sMm = (int (*)(void))Combo_ResolveSym("2ship", "MM_GetTriforcePieceCount");
        sType = (int (*)(void))Combo_ResolveSym("soh", "SOH_GetComboGoalType");
        sOotNamed = (int (*)(void))Combo_ResolveSym("soh", "SOH_GetNamedTriforceMask");
        sMmNamed = (int (*)(void))Combo_ResolveSym("2ship", "MM_GetNamedTriforceMask");
    }
    if (sType && sType() == 5 && sOotNamed && sMmNamed) {
        const unsigned mask = (sOotNamed() | sMmNamed()) & 7;
        sNamedTriforceMask = static_cast<int>(mask);
        have = (mask & 1) + ((mask >> 1) & 1) + ((mask >> 2) & 1);
        need = 3;
        return true;
    }
    if (!sOot || !sMm || !sGoal || !sGoal(&need) || need <= 0) {
        return false;
    }
    have = sOot() + sMm();
    return true;
}

// The retained overlay draws combined Triforce progress. Tracker visibility now
// belongs to ComboTrackerControls; legacy enable settings migrate at registration.
class SwapWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void Draw() override;

  protected:
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
    }
};

std::shared_ptr<SwapWindow> sSwapWindow;

void SwapWindow::Draw() {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return;
    }
    // comboui's per-module GImGui must point at the shared context (same pattern as ComboMenu).
    ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());

    // Combined Triforce progress (#136) — neither game's own tracker can show it (each counts only
    // its own pieces). Draggable overlay; ImGui persists the position.
    int have = 0, need = 0;
    if (OotActiveSlot() >= 0 && CVarGetInteger("gCombo.Rando.TriforceCounter", 1) &&
        ComboTriforceProgress(have, need)) {
        // Themed like the combo menu's frames (ComboWidgetStyle) so the overlay matches the rest of the UI.
        const ImVec4 theme = ComboRando::ComboMenu_ThemeColor();
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(theme.x, theme.y, theme.z, 0.45f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));
        if (ImGui::Begin("Combo Triforce", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
            ImGui::Text("Triforce: %d / %d", have, need);
            if (sNamedTriforceMask >= 0) {
                ImGui::Text("Power %s   Courage %s   Wisdom %s", (sNamedTriforceMask & 1) ? "+" : "-",
                            (sNamedTriforceMask & 2) ? "+" : "-", (sNamedTriforceMask & 4) ? "+" : "-");
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }
}

} // namespace

bool ComboTracker::ForegroundPaused(int fg) {
    static int (*sFn[2])(void) = {};
    static bool sTried[2] = {};
    if (fg != 0 && fg != 1) {
        return true;
    }
    if (!sTried[fg]) {
        sTried[fg] = true;
        sFn[fg] = (int (*)(void))Combo_ResolveSym(fg == 0 ? "soh" : "2ship",
                                                  fg == 0 ? "SOH_IsPausedForCombo" : "MM_IsPausedForCombo");
    }
    if (sFn[fg]) {
        return sFn[fg]() != 0;
    }
    return true;
}

bool ComboTracker::GetMasterVisible(int tracker) {
    return CVarGetInteger(kKinds[tracker].enabledCvar, 0) != 0;
}

void ComboTracker::SetMasterVisible(int tracker, bool visible) {
    CVarSetInteger(kKinds[tracker].enabledCvar, visible ? 1 : 0);
    // The reconcile window applies it next frame.
}

void ComboTracker::RegisterSwap() {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return;
    }
    // One-shot migration from the retired swap-era scheme (latched by EnabledSeeded).
    static const char* const kLegacyPrefix[2] = { "gCombo.Tracker.", "gCombo.CheckTracker." };
    for (int k = 0; k < kSwapCount; ++k) {
        const std::string pre = kLegacyPrefix[k];
        const std::string seeded = pre + "EnabledSeeded";
        if (CVarGetInteger(seeded.c_str(), 0)) {
            continue;
        }
        CVarSetInteger(seeded.c_str(), 1);
        // Recover crash-persisted forced visibility, then drop the retired CVars.
        for (int g = 0; g < 2; ++g) {
            const std::string stash = pre + (g == 0 ? "TrueIntentOot" : "TrueIntentMm");
            int v = CVarGetInteger(stash.c_str(), 0);
            if (v) {
                CVarSetInteger(kTrackers[g][kKinds[k].column].cvar, v - 1);
            }
            CVarClear(stash.c_str());
        }
        CVarClear((pre + "ShownGame").c_str());
        CVarClear((pre + "HoldMomentary").c_str());
        // Master enable seeds from the per-game CVars an existing config carries.
        bool on = CVarGetInteger(kTrackers[0][kKinds[k].column].cvar, 0) != 0 ||
                  CVarGetInteger(kTrackers[1][kKinds[k].column].cvar, 0) != 0;
        if (CVarGetInteger(kKinds[k].enabledCvar, -1) < 0) {
            const bool fresh = CVarGetInteger(kTrackers[0][kKinds[k].column].cvar, -1) < 0 &&
                               CVarGetInteger(kTrackers[1][kKinds[k].column].cvar, -1) < 0;
            CVarSetInteger(kKinds[k].enabledCvar, (on || (fresh && k == kSwapCheck)) ? 1 : 0);
        }
    }
    if (!sSwapWindow) {
        // Empty CVar name: visibility is never consulted (Draw is overridden), so don't persist
        // one. The leading space keeps it sorted before both tracker windows in the Gui map.
        sSwapWindow = std::make_shared<SwapWindow>("", true, " Combo Tracker Swap");
    }
    ctx->GetWindow()->GetGui()->AddGuiWindow(sSwapWindow);
}
