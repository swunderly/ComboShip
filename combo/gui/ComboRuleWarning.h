#pragma once
#include "ComboResolve.h"
#include "menu/ComboCheckTrackerABI.h"
#include "rando/GameplayRules.h"
#include <imgui.h>
#include <cstring>

namespace ComboRando {
inline double sRuleWarningUntil = 0;
inline void WarnAboutRuleChange(const char* name) {
    if (!name) return;
    if (const auto* rule = FindGameplayRule(name); rule && rule->runtimeOnly) return;
    if (!std::strcmp(name, "gCombo.Rando.SyncCosmetics") || !std::strcmp(name, "gCombo.Rando.TriforceCounter")) return;
    bool rule = std::strncmp(name, "gCombo.Rando.", 13) == 0 || std::strncmp(name, "gCombo.Tricks.", 14) == 0 ||
                std::strncmp(name, "gRando.", 7) == 0 || std::strncmp(name, "gRandoSettings.", 15) == 0;
    for (const auto& entry : kGameplayRules) rule |= std::strcmp(name, entry.name) == 0 || LogicRuleKey(entry.name)==name;
    if (!rule) return;
    static auto oot = (ComboCheckSnapshot)Combo_ResolveSym("soh", "SOH_ComboCheckSnapshot");
    static auto mm = (ComboCheckSnapshot)Combo_ResolveSym("2ship", "MM_ComboCheckSnapshot");
    ComboCheckInfo a{}, b{};
    if (oot) oot(&a, nullptr, nullptr);
    if (mm) mm(&b, nullptr, nullptr);
    if (a.loaded || b.loaded) sRuleWarningUntil = ImGui::GetTime() + 8.0;
}
inline void DrawRuleChangeWarning() {
    if (ImGui::GetTime() >= sRuleWarningUntil) return;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, .8f, .35f, 1.0f));
    ImGui::TextWrapped("Changing settings mid-run could affect your ability to complete the run.");
    ImGui::PopStyleColor();
    ImGui::Separator();
}
}
