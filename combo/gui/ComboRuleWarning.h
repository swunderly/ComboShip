#pragma once
#include "rando/GameplayRules.h"
#include <imgui.h>
#include <cstring>

namespace ComboRando {
// The file slot of the run the foreground game has loaded, or -1 on the title and file screens.
int LoadedRunSlot();
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
    const int loaded = LoadedRunSlot();
    if (loaded >= 0 && (slot < 0 || loaded == slot))
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
