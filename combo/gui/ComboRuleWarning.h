#pragma once
#include "ComboCheckSnapshot.h"
#include "rando/GameplayRules.h"
#include <imgui.h>
#include <cstring>

namespace ComboRando {
inline double sRuleWarningUntil = 0;
// A slot limits the warning to edits for that file's run (next-seed drafts reach only that run).
inline void WarnAboutRuleChange(const char* name, int slot = -1) {
    if (!name)
        return;
    if (const auto* rule = FindGameplayRule(name); rule && rule->runtimeOnly)
        return;
    if (!std::strcmp(name, "gCombo.Rando.SyncCosmetics") || !std::strcmp(name, "gCombo.Rando.TriforceCounter"))
        return;
    bool rule = std::strncmp(name, "gCombo.Rando.", 13) == 0 || std::strncmp(name, "gCombo.Tricks.", 14) == 0 ||
                std::strncmp(name, "gRando.", 7) == 0 || std::strncmp(name, "gRandoSettings.", 15) == 0;
    for (const auto& entry : kGameplayRules)
        rule |= std::strcmp(name, entry.name) == 0 || LogicRuleKey(entry.name) == name;
    if (!rule)
        return;
    const auto oot = ComboChecks::Snapshot(0), mm = ComboChecks::Snapshot(1);
    ComboCheckInfo a{}, b{};
    if (oot)
        oot(&a, nullptr, nullptr);
    if (mm)
        mm(&b, nullptr, nullptr);
    if ((a.loaded && (slot < 0 || a.slot == slot)) || (b.loaded && (slot < 0 || b.slot == slot)))
        sRuleWarningUntil = ImGui::GetTime() + 8.0;
}
inline void DrawRuleChangeWarning() {
    if (ImGui::GetTime() >= sRuleWarningUntil)
        return;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, .8f, .35f, 1.0f));
    ImGui::TextWrapped("Changing settings mid-run could affect your ability to complete the run.");
    ImGui::PopStyleColor();
    ImGui::Separator();
}
} // namespace ComboRando
