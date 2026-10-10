#include "ComboTricks.h"
#include "ComboTricksModel.h"
#include "ComboWidgetStyle.h"
#include "ComboRuleWarning.h"
#include "ComboMenuModel.h"
#include "ComboMenuLayout.h"
#include "rando/GameplayRules.h"
#include "ComboResolve.h"
#include <libultraship/libultraship.h>
#include <imgui.h>
#include <array>
#include <string>

namespace ComboRando {
namespace {
Detail::TrickCatalogueFocus sPendingFocus;
struct Source {
    Fn_ComboTricks get = nullptr;
    Fn_ComboSetTrick set = nullptr;
};
std::array<Source, 2> Sources() {
    const char* modules[] = { "soh", "2ship" };
    const char* getters[] = { "SOH_GetComboTricks", "MM_GetComboTricks" };
    const char* setters[] = { "SOH_SetComboTrick", "MM_SetComboTrick" };
    std::array<Source, 2> result;
    for (int game = 0; game < 2; ++game) {
        result[game].get = reinterpret_cast<Fn_ComboTricks>(Combo_ResolveSym(modules[game], getters[game]));
        result[game].set = reinterpret_cast<Fn_ComboSetTrick>(Combo_ResolveSym(modules[game], setters[game]));
    }
    return result;
}
} // namespace

void FocusUnifiedTrick(const char* name) {
    sPendingFocus.Set(name);
}

void DrawUnifiedTricks() {
    static char query[160] = {};
    static int world = 0, status = 0, skill = 0;
    sPendingFocus.Apply(query, world, status, skill);
    const char* worlds[] = { "Both games", "Ocarina of Time", "Majora's Mask" };
    const char* states[] = { "All techniques", "Enabled only", "Disabled only" };
    const char* levels[] = { "All difficulties", "Novice",  "Intermediate", "Advanced",
                             "Expert",           "Extreme", "Experimental" };
    const auto theme = ComboMenu_ThemeColor();
    ComboMenu_PushInput(theme);
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 620.0f));
    ImGui::InputTextWithHint("##TechniqueSearch", "Search techniques, areas or prerequisites", query, sizeof(query));
    ComboMenu_PopInput();
    ComboMenu_PushCombobox(theme);
    for (int i = 0; i < IM_ARRAYSIZE(worlds); ++i) {
        if (i && ImGui::GetItemRectMax().x + ImGui::CalcTextSize(worlds[i]).x + 44.0f <
                     ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x)
            ImGui::SameLine();
        if (ComboMenu_StyledHeaderEntry(worlds[i], world == i))
            world = i;
    }
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 420.0f));
    ImGui::Combo("Show", &status, states, IM_ARRAYSIZE(states));
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 420.0f));
    ImGui::Combo("Difficulty", &skill, levels, IM_ARRAYSIZE(levels));
    ComboMenu_PopCombobox();
    const auto sources = Sources();
    int totals[] = { 0, 0 }, enabled = 0, visible = 0;
    const std::array<const GameplayRule*, 2> restoredRules = {
        FindGameplayRule("gEnhancements.RestoreShadowDoorUnload"), FindGameplayRule("gEnhancements.EarlyEyeballFrog")
    };
    const std::array<CwTrickInfo, 2> restored = {
        ComboTricks::RestoredShadowDoorInfo(restoredRules[0]->label, restoredRules[0]->help,
                                            CVarGetInteger(LogicRuleKey(restoredRules[0]->name).c_str(), 0) != 0),
        ComboTricks::EarlyEyeballFrogInfo(restoredRules[1]->label, restoredRules[1]->help,
                                          CVarGetInteger(LogicRuleKey(restoredRules[1]->name).c_str(), 0) != 0)
    };
    for (const auto& row : restored) {
        enabled += row.enabled ? 1 : 0;
        if (ComboTricks::Matches(row, 0, query, world, status, skill - 1))
            ++visible;
    }
    const CwTrickInfo* rows[] = { nullptr, nullptr };
    for (int game = 0; game < 2; ++game) {
        if (sources[game].get)
            rows[game] = sources[game].get(&totals[game]);
        if (!rows[game])
            totals[game] = 0;
        for (int i = 0; i < totals[game]; ++i) {
            enabled += rows[game][i].enabled ? 1 : 0;
            if (ComboTricks::Matches(rows[game][i], game, query, world, status, skill - 1))
                ++visible;
        }
    }
    ImGui::Text("%d enabled / %d techniques; %d match", enabled,
                totals[0] + totals[1] + static_cast<int>(restored.size()), visible);
    ImGui::Separator();
    const bool busy = CVarGetInteger("gGeneral.RandoGenerating", 0) || CVarGetInteger("gSettings.DisableChanges", 0);
    ImGui::BeginDisabled(busy);
    for (int game = 0; game < 2; ++game) {
        if (world && world - 1 != game)
            continue;
        if (game == 0)
            for (size_t i = 0; i < restored.size(); ++i) {
                const auto& row = restored[i];
                const auto& rule = *restoredRules[i];
                if (!ComboTricks::Matches(row, 0, query, world, status, skill - 1))
                    continue;
                ImGui::PushID(row.id);
                bool on = row.enabled != 0;
                ComboMenu_PushCheckbox(theme);
                if (ImGui::Checkbox(row.name, &on)) {
                    SetTrickGovernedRule(rule, on, [](const char* name, int value) {
                        CVarSetInteger(name, value);
                        static auto write = reinterpret_cast<void (*)(const char*, int32_t)>(
                            Combo_ResolveSym("soh", "SOH_WriteGameplayRule"));
                        if (write)
                            write(name, value);
                        auto& model = ComboMenuModel::Get();
                        if (model.Oot().applyCVarChange)
                            model.Oot().applyCVarChange(name);
                        WarnAboutRuleChange(name);
                    });
                    static auto refresh =
                        reinterpret_cast<void (*)()>(Combo_ResolveSym("soh", "SOH_RefreshComboRules"));
                    if (refresh)
                        refresh();
                    Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
                }
                ComboMenu_PopCheckbox();
                MenuTooltip(row.description);
                ImGui::TextDisabled("OoT · %s · %s", row.area, row.tags);
                if (ImGui::TreeNode("Details")) {
                    ImGui::TextWrapped("%s", row.description);
                    ImGui::TreePop();
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        if (!totals[game]) {
            ImGui::TextWrapped("%s: no additional technique assumptions are available from this engine yet.",
                               game == 0 ? "Ocarina of Time" : "Majora's Mask");
            continue;
        }
        for (int i = 0; i < totals[game]; ++i) {
            const auto& row = rows[game][i];
            if (!ComboTricks::Matches(row, game, query, world, status, skill - 1))
                continue;
            ImGui::PushID(game);
            ImGui::PushID(row.id);
            bool on = row.enabled != 0;
            ComboMenu_PushCheckbox(theme);
            ImGui::BeginDisabled(!sources[game].set);
            if (ImGui::Checkbox(row.name, &on) && sources[game].set(row.id, on ? 1 : 0)) {
                WarnAboutRuleChange(game == 0 ? "gRandoSettings.EnabledTricks" : "gCombo.Tricks.MM");
                static auto refresh = reinterpret_cast<void (*)()>(Combo_ResolveSym("soh", "SOH_RefreshComboRules"));
                if (refresh)
                    refresh();
            }
            ImGui::EndDisabled();
            ComboMenu_PopCheckbox();
            MenuTooltip(row.description);
            ImGui::TextDisabled("%s · %s · %s", game == 0 ? "OoT" : "MM", row.area ? row.area : "General",
                                row.tags ? row.tags : "");
            if (ImGui::TreeNode("Details")) {
                ImGui::TextWrapped("%s", row.description && *row.description
                                             ? row.description
                                             : "No additional description is supplied by this engine.");
                ImGui::TreePop();
            }
            ImGui::Separator();
            ImGui::PopID();
            ImGui::PopID();
        }
    }
    ImGui::EndDisabled();
    if (!visible)
        ImGui::TextUnformatted("No techniques match these filters.");
}
} // namespace ComboRando
