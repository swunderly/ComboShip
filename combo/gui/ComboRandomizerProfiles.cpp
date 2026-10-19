#include "ProfileOotOptionPolicy.h"
#include <memory>
#include "ComboRandomizerProfiles.h"
#include "ComboMenuModel.h"
#include "ComboExport.h"
#include "ComboWidgetStyle.h"
#include "ComboMenuLayout.h"
#include "ComboRandomizerLayout.h"
#include "ComboTricks.h"
#include "ComboTricksModel.h"
#include "ComboRuleWarning.h"
#include "ComboWidgetRender.h"
#include "rando/GameMode.h"
#include "rando/GameplayRules.h"
#include "rando/GameplayRuleSnapshots.h"
#include "rando/GenerationRules.h"
#include "rando/entrances/EntranceProfile.h"
#include "rando/OotMaskQuest.h"
#include <libultraship/libultraship.h>
#include <fast/Fast3dGui.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <set>
#include <vector>
namespace ComboRando::ProfileUI {
using Json = nlohmann::json;
static ComboRandomizerProfileCallbacks callbacks{};
static int selected = 0, view = 0;
static bool active = false, dirty = false;
static std::string error;
static Json snapshot, document, catalog;
// Inspection serializes the complete native catalog and both seed placement tables.
// Keep it out of ordinary render frames; edits and generation still refresh the view.
static bool refreshPending = true, snapshotValid = false;
static int snapshotSlot = -1, snapshotView = -1, lastMenuFrame = -2;
static double lastInspection = -1;
static uint64_t snapshotRevision = 0, snapshotSerial = 0; // serial: changes with every inspection
static std::shared_ptr<ProfileOotPolicy::View> optionView;
static std::set<std::string> mmBaseAvailable;
static bool deferPoolViews = false; // ApplyCommunityPreset refreshes the MM pool view once, after all its writes
static void RefreshOptionPolicy(bool apply);
bool Ready() {
    return (callbacks.version == 1 || callbacks.version == 2) && callbacks.inspect && callbacks.setDraft &&
           callbacks.copySaved && callbacks.copyDraft && callbacks.generate && callbacks.cancel;
}
const Json& Catalogue() {
    return catalog;
}
int SelectedSlot() {
    return selected;
}
bool Active() {
    return active;
}
bool Editable() {
    return active && document.is_object() && view == 0;
}
static bool Response(const char* raw) {
    auto reply = Json::parse(raw ? raw : "null");
    error = reply.value("error", std::string());
    return error.empty();
}
// Give the document the complete version 2 rule block that runtime application and generation read
// (ProfileGameplayRules), the same one the launcher derives for an unedited document.
static Json& RuleBlock() {
    auto& block = document["gameplayRules"];
    if (!IsGameplayRuleBlock(block) || block["version"] != 2)
        block = ProfileGameplayRules(document);
    return block;
}
static Json* Field(const char* key, bool create) {
    if (!document.is_object())
        return nullptr;
    for (const auto& rule : kGameplayRules) {
        const bool logic = LogicRuleKey(rule.name) == key;
        if (!logic && std::strcmp(key, rule.name))
            continue;
        auto& values = RuleBlock()[logic ? "logicValues" : "values"];
        if (create && !values.is_object())
            values = Json::object();
        return create || values.contains(rule.name) ? &values[rule.name] : nullptr;
    }
    const bool oot = std::strncmp(key, "gRandoSettings.", 15) == 0,
               mm = std::strncmp(key, "gRando.", 7) == 0 || std::strncmp(key, "gCombo.Tricks.MM.", 17) == 0;
    auto& values = oot ? document["oot"]["settings"] : mm ? document["mm"]["settings"] : document["setupCVars"];
    if (create && !values.is_object())
        values = Json::object();
    return create || values.contains(key) ? &values[key] : nullptr;
}
int ReadInteger(const char* key, int fallback) {
    if (view != 0 && std::strncmp(key, "gCombo.Rando.Entrances.", 23) == 0)
        return Entrances::SavedEntranceValue(document, key, fallback);
    const auto* value = Field(key, false);
    return value && value->is_number_integer() ? value->get<int>() : fallback;
}
static void RefreshPoolViews() {
    mmBaseAvailable.clear();
    const auto mm = catalog.value("mm", Json::object());
    const auto conditions = mm.value("checkRequirements", Json::object());
    const auto* exclusions = Field("gRando.ExcludedChecks", false);
    for (const auto& check : conditions.items()) {
        const auto& row = check.value();
        if (row.contains("enabledKey") && !ReadInteger(row["enabledKey"].get<std::string>().c_str(), 0))
            continue;
        if (row.contains("countKey") && ReadInteger(row["countKey"].get<std::string>().c_str(), 30) <= 0)
            continue;
        if (ReadInteger("gRando.Options.RO_LOGIC", 0) != 3 && row.value("junk", false) && exclusions &&
            exclusions->is_array() &&
            std::find(exclusions->begin(), exclusions->end(), Json(check.key())) != exclusions->end())
            continue;
        mmBaseAvailable.insert(row.at("vanillaItem").get<std::string>());
    }
}
void WriteInteger(const char* key, int value) {
    if (Editable()) {
        // ProfileSetDraft also applies a draft's rules to the run loaded from the same file.
        if (const auto* rule = FindGameplayRule(key); rule && !rule->runtimeOnly && !rule->trickGoverned)
            if (const auto* old = Field(key, false); !old || *old != value)
                WarnAboutRuleChange(key, selected);
        *Field(key, true) = value;
        dirty = true;
        // Two logic keys are also native OoT options, which generation applies after the rules.
        for (const auto& rule : kGameplayRules)
            if (const auto native = LogicRuleKey(rule.name); native.rfind("gRandoSettings.", 0) == 0)
                if (const auto* logic = Field(native.c_str(), false); logic && logic->is_number_integer())
                    document["oot"]["settings"][native] = *logic;
        if (!std::strcmp(key, "gRando.Options.RO_LOGIC") && value == 3)
            for (const auto& clear : catalog.at("mm").at("vanillaIncompatible"))
                *Field(clear.get<std::string>().c_str(), true) = 0;
        if (!std::strcmp(key, "gRando.Options.RO_STRAY_FAIRIES_MAX")) {
            const char* required = "gRando.Options.RO_STRAY_FAIRIES_REQUIRED";
            if (ReadInteger(required, 15) > value)
                *Field(required, true) = value;
        }
        if (ReadInteger("gRandoSettings.LogicRules", 0) != ProfileOotPolicy::RO_LOGIC_NO_LOGIC &&
            ReadInteger("gRandoSettings.ShopsanityCount", 0) > 7)
            *Field("gRandoSettings.ShopsanityCount", true) = 7;
        RefreshOptionPolicy(true);
        Entrances::NormalizeEntranceDraft(document, Entrances::ActiveCapabilities());
        if (!deferPoolViews)
            RefreshPoolViews();
    }
}
static void RefreshOptionPolicy(bool apply) {
    optionView = std::make_shared<ProfileOotPolicy::View>(catalog.at("oot"), ReadInteger);
    if (apply && Editable())
        for (const auto& value : optionView->Adjustments())
            *Field(value.first.c_str(), true) = value.second;
}
static std::string ReadString(const char* key) {
    const auto* value = Field(key, false);
    return value && value->is_string() ? value->get<std::string>() : "";
}
static void RefreshSnapshot(double now, uint64_t revision) {
    refreshPending = false;
    snapshotValid = false;
    snapshotSlot = selected;
    snapshotView = view;
    lastInspection = now;
    auto next = Json::parse(callbacks.inspect(selected));
    if (next.contains("error"))
        throw std::runtime_error(next["error"].get<std::string>());
    snapshot = std::move(next);
    ++snapshotSerial;
    catalog = snapshot.at("catalog");
    document = snapshot.value(view == 0 ? "draft" : view == 1 ? "savedSettings" : "generatedSettings", Json());
    RefreshOptionPolicy(false);
    RefreshPoolViews();
    snapshotRevision = revision;
    snapshotValid = true;
}
// Each commit rewrites the whole profile store and refreshes the snapshot, so EndFrame waits while a
// slider or text field is in use. The document belongs to the inspected file and view.
static void CommitDraft() {
    if (!dirty)
        return;
    dirty = false;
    if (snapshotView != 0 || !document.is_object())
        return;
    refreshPending = true;
    try {
        Response(callbacks.setDraft(snapshotSlot, document.dump().c_str()));
    } catch (const std::exception& e) { error = e.what(); }
}
void BeginFrame() {
    active = false;
    try {
        if (!Ready())
            return;
        if (selected != snapshotSlot || view != snapshotView)
            CommitDraft();
        const int frame = ImGui::GetFrameCount();
        const double now = ImGui::GetTime();
        const bool versioned = callbacks.version >= 2 && callbacks.revision;
        const uint64_t revision = versioned ? callbacks.revision() : 0;
        const bool running =
            snapshot.is_object() && snapshot.value("generation", Json::object()).value("running", false);
        // An uncommitted edit lives only in the document; refresh after it is committed.
        const bool refresh = !dirty && (refreshPending || !snapshotValid || (!versioned && frame > lastMenuFrame + 1) ||
                                        (versioned && revision != snapshotRevision) || selected != snapshotSlot ||
                                        view != snapshotView || (running && now - lastInspection >= .25));
        lastMenuFrame = frame;
        if (refresh)
            RefreshSnapshot(now, revision);
        active = snapshotValid;
    } catch (const std::exception& e) { error = e.what(); }
}
void EndFrame() {
    if (active && !ImGui::IsAnyItemActive())
        CommitDraft();
    active = false;
}
// A hidden menu never releases its slider or text field, so commit the edit still waiting for it.
void Flush() {
    CommitDraft();
}
bool DrawUnavailableSettings() {
    if (active && document.is_object())
        return false;
    if (!active) {
        ImGui::TextWrapped("Settings are unavailable. Reopen the menu to try again.");
        return true;
    }
    ImGui::TextWrapped(view == 1   ? "File %d has no saved seed settings."
                       : view == 2 ? "File %d has no generated seed settings."
                                   : "File %d has no next-seed settings.",
                       selected + 1);
    if (view != 0 && ImGui::Button("Configure next seed")) {
        view = 0;
        BeginFrame();
    }
    return true;
}
void DrawSelector() {
    static const char* files[] = { "File 1", "File 2", "File 3" };
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
    if (ImGui::Combo("##Settings file", &selected, files, 3))
        BeginFrame();
    MenuTooltip("Choose the file whose saved rules you inspect or whose next seed you configure. This does not change "
                "gameplay.");
    const char* modes[] = { "Next seed", "Saved seed", "Generated seed" };
    const char* tips[] = { "Editable settings for this file's next seed.",
                           "Read-only settings of the run saved in this file.",
                           "Read-only settings of the last seed generated for this file." };
    for (int i = 0; i < 3; ++i) {
        if (ImGui::GetItemRectMax().x + ImGui::CalcTextSize(modes[i]).x + 44.f <
            ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x)
            ImGui::SameLine();
        if (ComboMenu_StyledHeaderEntry(modes[i], view == i)) {
            view = i;
            BeginFrame();
        }
        MenuTooltip(tips[i]);
    }
    if (ImGui::GetItemRectMax().x + ImGui::CalcTextSize("Copy settings").x + 44.f <
        ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x)
        ImGui::SameLine();
    ImGui::BeginDisabled(!active || view == 2 || !document.is_object());
    if (ImGui::Button("Copy settings"))
        ImGui::OpenPopup("Copy settings");
    ImGui::EndDisabled();
    MenuTooltip(view == 2
                    ? "Select Next seed or Saved seed to copy settings."
                    : "Copy these settings to a file's next-seed setup. This does not copy or replace saved progress.");
    static int copyTarget = -1;
    bool confirmCopy = false;
    if (ImGui::BeginPopup("Copy settings")) {
        for (int destination = 0; destination < 3; ++destination) {
            const std::string label = "To File " + std::to_string(destination + 1) + " next seed";
            ImGui::BeginDisabled(view == 2 || document.is_null());
            if (ImGui::Selectable(label.c_str())) {
                copyTarget = destination;
                confirmCopy = true;
            }
            ImGui::EndDisabled();
        }
        ImGui::EndPopup();
    }
    if (confirmCopy)
        ImGui::OpenPopup("Copy settings?");
    const std::string copyText =
        "File " + std::to_string(copyTarget + 1) + "'s next-seed settings are replaced by these. Saved progress stays.";
    if (MenuConfirm("Copy settings?", copyText.c_str(), "Copy settings") && copyTarget >= 0 && view != 2 &&
        !document.is_null()) {
        refreshPending = true;
        try {
            Response(view == 0 ? callbacks.copyDraft(selected, copyTarget) : callbacks.copySaved(selected, copyTarget));
        } catch (const std::exception& e) { error = e.what(); }
    }
    if (!error.empty())
        ImGui::TextWrapped("%s", error.c_str());
    static std::set<std::pair<int, uint64_t>> shownEntranceNotices;
    const auto notice = std::pair{ selected, snapshotRevision };
    if (view == 0 && snapshot.value("entranceSettingsStripped", false) && !shownEntranceNotices.count(notice)) {
        ImGui::OpenPopup("Entrance settings updated");
        if (MenuConfirm(
                "Entrance settings updated",
                "Unsupported entrance values were reset to their defaults for this build. Saved seeds stay unchanged.",
                "OK")) {
            shownEntranceNotices.insert(notice);
            dirty = true;
        }
    }
}
static void Tooltip(const char* help) {
    if (help && help[0])
        MenuTooltip(help);
}
static Entrances::Settings MenuEntranceSettings() {
    return Entrances::ReadEntranceSettings(ReadInteger, Entrances::ActiveCapabilities());
}
static std::string EntranceUnavailable() {
    return Entrances::ProfileWallmasterRefusal(
        document, catalog.value("oot", Json::object()), [](const char* key, int fallback) {
            return !std::strcmp(key, kOotEnemyRandomizer) ? CVarGetInteger(key, fallback) : ReadInteger(key, fallback);
        });
}
void DrawEntranceSetting(int index) {
    if (!active || !document.is_object() || index < 0 || index >= int(std::size(Entrances::kEntranceSettings)))
        return;
    const auto& row = Entrances::kEntranceSettings[index];
    const auto caps = Entrances::ActiveCapabilities().Selected("");
    if (!row.visible(caps))
        return;
    const auto settings = MenuEntranceSettings();
    if (!row.condition(settings))
        return;
    const int value = row.get(settings);
    const auto unavailable = EntranceUnavailable();
    ImGui::PushID(row.cvar);
    ImGui::BeginDisabled(!Editable());
    ImGui::TextWrapped("%s", row.name);
    ComboMenu_PushCombobox(ComboMenu_ThemeColor());
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##Entrance", row.choices[value].name.data())) {
        for (int i = 0; i < int(row.choices.size()); ++i) {
            if (!row.supports(i, caps))
                continue; // Full and all unadmitted rows remain hidden.
            const bool blocked = i != row.defaultValue && (!row.condition(settings) || !unavailable.empty());
            ImGui::BeginDisabled(blocked);
            if (ImGui::Selectable(row.choices[i].name.data(), i == value))
                WriteInteger(row.cvar, i);
            ImGui::EndDisabled();
            if (blocked)
                Tooltip(unavailable.empty() ? "This combination is not available in this build." : unavailable.c_str());
        }
        ImGui::EndCombo();
    }
    ComboMenu_PopCombobox();
    Tooltip(row.description);
    ImGui::EndDisabled();
    if (!unavailable.empty())
        ImGui::TextWrapped("%s", unavailable.c_str());
    ImGui::PopID();
}
// Display text the profile pages give a native OoT option without renaming it: settings.cpp keeps its names for saved
// settings, presets and search, where they stay aliases. Labels follow OoTMM v32.3
// (packages/core/src/settings/data.ts), whose "(OoT)" suffix is dropped inside a world view.
struct OptionDisplay {
    const char* key;
    const char* label; // nullptr keeps the native label
    const char* note;  // appended to the tooltip
    bool visibleNote;  // also shown under the row
};
static constexpr OptionDisplay kOptionDisplay[] = {
    { "gRandoSettings.SwordlessEponaItems", nullptr, "No logic effect. Fixed when the seed is made.", false },
    { "gRandoSettings.SkipChildStealth", nullptr,
      "No logic effect. Use Enhancements > Time savers > Quests & errands > Skip Child Stealth instead.", true },
    { "gRandoSettings.ShuffleWarpSongs", nullptr,
      "Ocarina of Time destinations only. Warp Songs and Soaring Spots Shuffle above mixes both games.", false },
    { "gRandoSettings.ShuffleOwlDrops", nullptr,
      "Ocarina of Time destinations only. Warp Songs and Soaring Spots Shuffle above mixes both games.", false },
    { "gRandoSettings.FullWallets", "Fill Wallets", nullptr, false },
    // Gameplay rules count rows read "<thing> required for <reward>", as MM's frog and Zora egg rules.
    { "gRandoSettings.BigPoeTargetCount", "Big Poes required for reward", nullptr, false },
    // Items & checks: one name for an option both worlds list under the same heading, as MM's rows read
    // (OoTMM's Boss Souls, Enemy Souls and Ocarina Buttons without the game suffix).
    { "gRandoSettings.ShuffleBossSouls", "Boss Souls", nullptr, false },
    { "gRandoSettings.SoulsEnemyOot", "Enemy Souls", nullptr, false },
    { "gRandoSettings.ShuffleFreestanding", "Freestanding Items", nullptr, false },
    { "gRandoSettings.ShuffleSwim", "Swim", nullptr, false },
    { "gRandoSettings.ShuffleOcarinaButtons", "Ocarina Buttons", nullptr, false },
    { "gRandoSettings.IncludeTycoonWallet", "Tycoon's Wallet", nullptr, false },
    { "gRandoSettings.ShuffleSongs", "Song Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleTokens", "Gold Skulltula Tokens Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleCows", "Cow Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleScrubs", "Scrub Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleMerchants", "Merchants Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleWonderItems", "Wonder Items Shuffle", nullptr, false },
    { "gRandoSettings.ShufflePots", "Pots Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleCrates", "Crates Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleBeehives", "Hives Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleGrass", "Grass Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleRocks", "Rock Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleTrees", "Tree Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleBushes", "Bush Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleBoulders", "Boulder Drops Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleIcicles", "Icicles Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleRedIce", "Red Ice Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleFountainFairies", "Fairy Fountain Fairy Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleFairySpots", "Fairy Spot Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleFrogSongRupees", "Frogs Rupees Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleOcarinas", "Ocarina Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleMasterSword", "Master Sword Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleGerudoToken", "Gerudo Card Shuffle", nullptr, false },
    { "gRandoSettings.Shuffle100GSReward", "Skulltula Final Reward Shuffle", nullptr, false },
    { "gRandoSettings.ShuffleDungeonReward", "Dungeon Reward Shuffle", nullptr, false },
    { "gRandoSettings.StartingMapsCompasses", "Map / Compass Shuffle", nullptr, false },
};
static const OptionDisplay* FindOptionDisplay(const char* key) {
    for (const auto& row : kOptionDisplay)
        if (key && !std::strcmp(key, row.key))
            return &row;
    return nullptr;
}
const char* OptionDisplayLabel(const char* key) {
    const auto* display = FindOptionDisplay(key);
    return display && display->label ? display->label : nullptr;
}
// Borrow the inspected seed: a copy would duplicate both placement tables every frame.
static const Json& InspectedSeed() {
    static const Json none;
    const auto seed = snapshot.find(view == 1 ? "savedSeed" : "generatedSeed");
    return seed != snapshot.end() ? *seed : none;
}
void DrawWidget(const CwWidget& w, const GameMenu& game, bool showHidden) {
    if (!active || !document.is_object()) {
        ImGui::TextDisabled("No settings for this file and view.");
        return;
    }
    // A selection refreshes optionView; retain the current choices until this widget finishes drawing.
    const auto policyView = optionView;
    const auto* policy = policyView && w.cvar ? policyView->Find(w.cvar) : nullptr;
    if (policy && policy->hidden && !showHidden)
        return;
    (void)game;
    ImGui::PushID(w.index);
    const char* key = w.cvar ? w.cvar : "";
    const auto* display = FindOptionDisplay(key);
    const char* label = display && display->label ? display->label : w.name ? w.name : "";
    const auto theme = ComboMenu_ThemeColor();
    std::string caption = label;
    if (const auto suffix = caption.find("##"); suffix != std::string::npos)
        caption.resize(suffix);
    const bool blockedByEntrances =
        Entrances::IsShipEntranceSetting(key) && Entrances::Active(MenuEntranceSettings().erWallmasters);
    ImGui::BeginDisabled(!Editable() || blockedByEntrances || (policy && (policy->disabled || policy->hidden)));
    switch (w.kind) {
        case CW_SEPARATOR:
            ImGui::Separator();
            break;
        case CW_SEPARATOR_TEXT:
            ImGui::SeparatorText(caption.c_str());
            break;
        case CW_TEXT:
            ImGui::TextWrapped("%s", caption.c_str());
            break;
        case CW_CHECKBOX: {
            // The row style of the other tabs and the gameplay rule rows: a long label wraps beside the box.
            bool value = ReadInteger(key, w.bDefault) != 0;
            if (ComboMenu_CheckboxRow(caption.c_str(), &value))
                WriteInteger(key, value);
            break;
        }
        case CW_SLIDER_INT: {
            // The one slider row: the label above, the choice's words in the bar, - and +, and Default.
            int value = ReadInteger(key, w.iDefault);
            const int maximum =
                policy ? (!std::strcmp(key, "gRandoSettings.ShopsanityCount") &&
                                  ReadInteger("gRandoSettings.LogicRules", 0) != ProfileOotPolicy::RO_LOGIC_NO_LOGIC
                              ? std::min(7, int(policy->choices.size()) - 1)
                              : int(policy->choices.size()) - 1)
                       : w.iMax;
            const char* format = policy && !policy->choices.empty()
                                     ? policy->choices[std::clamp(value, 0, int(policy->choices.size()) - 1)].c_str()
                                     : "%d";
            if (ComboMenu_SliderRowInt(caption.c_str(), &value, w.iMin, maximum, w.iStep, w.iDefault, format))
                WriteInteger(key, value);
            break;
        }
        case CW_COMBOBOX: {
            int value = ReadInteger(key, w.iDefault);
            // Mask Quest's Vanilla, still stored by older files, has no choice: show the mode it plays as.
            if (!std::strcmp(key, "gRandoSettings.CompleteMaskQuest"))
                value = OotMaskQuestShownMode(value, view == 0);
            std::vector<CwChoice> choices;
            for (int i = 0; policy && i < int(policy->choices.size()); ++i)
                choices.push_back({ policy->choices[i].c_str(), policy->choiceValues[i] });
            const CwChoice* list = policy ? choices.data() : w.choices;
            const int count = policy ? int(choices.size()) : w.choiceCount;
            const char* preview = "Unknown saved value";
            for (int i = 0; i < count; ++i)
                if (list[i].value == value)
                    preview = list[i].label;
            ImGui::TextWrapped("%s", caption.c_str());
            ComboMenu_PushCombobox(theme);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##Choice", preview)) {
                for (int i = 0; i < count; ++i)
                    if (ImGui::Selectable(list[i].label, list[i].value == value))
                        WriteInteger(key, list[i].value);
                ImGui::EndCombo();
            }
            ComboMenu_PopCombobox();
            break;
        }
        case CW_INPUT_TEXT: {
            std::string value = ReadString(key);
            if (ImGui::InputText(label, &value) && Editable()) {
                *Field(key, true) = value;
                dirty = true;
            }
            break;
        }
        case CW_CUSTOM:
            break;
        default:
            ImGui::TextDisabled("%s", caption.c_str());
            break;
    }
    ImGui::EndDisabled();
    if (blockedByEntrances)
        Tooltip("Turn Wallmaster Shuffle off before enabling Ship's own entrance shuffle.");
    else if (policy && policy->disabled)
        Tooltip(policy->disabledText.c_str());
    else if (display && display->note) {
        const std::string help =
            w.tooltip && w.tooltip[0] ? std::string(w.tooltip) + "\n\n" + display->note : std::string(display->note);
        Tooltip(help.c_str());
    } else
        Tooltip(w.tooltip);
    if (display && display->note && display->visibleNote) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("%s", display->note);
        ImGui::PopTextWrapPos();
    }
    if (view != 0 && !std::strcmp(key, "gRandoSettings.GanonTrial")) {
        const auto& seed = InspectedSeed();
        if (seed.is_object() && seed.contains("oot") && seed["oot"].contains("resolvedSeedRules")) {
            const auto trials = seed["oot"]["resolvedSeedRules"].value("requiredTrials", Json::array());
            std::string text;
            for (const auto& id : trials) {
                if (!text.empty())
                    text += ", ";
                text += catalog.at("oot")
                            .at("trialNames")
                            .value(std::to_string(id.get<int>()), std::string("Unknown trial"));
            }
            ImGui::TextWrapped("Required trials: %s", text.empty() ? "None" : text.c_str());
        }
    }
    ImGui::PopID();
}
// All On / All Off under OoT's Scenery drops heading, as MM's card has: every shown, enabled row goes to its widest
// (last) or first choice.
static void SceneryButtons(const CwWidget* rows, int count) {
    ImGui::BeginDisabled(!Editable());
    for (int on = 1; on >= 0; --on) {
        if (on == 0)
            ImGui::SameLine();
        if (!ImGui::Button(on ? "All On" : "All Off"))
            continue;
        for (int i = 0; i < count; ++i) {
            const auto& row = rows[i];
            if (!row.cvar || !row.cvar[0])
                continue;
            int value = on;
            if (const auto* policy = optionView ? optionView->Find(row.cvar) : nullptr) {
                if (policy->hidden || policy->disabled)
                    continue;
                if (!policy->choiceValues.empty())
                    value = on ? policy->choiceValues.back() : policy->choiceValues.front();
            } else if (row.kind == CW_COMBOBOX && row.choiceCount > 0)
                value = row.choices[on ? row.choiceCount - 1 : 0].value;
            WriteInteger(row.cvar, value); // refreshes optionView: no policy is kept across it
        }
    }
    ImGui::EndDisabled();
}
void DrawSidebar(const CwSidebar& page, const GameMenu& game, int world) {
    // OoT's rows take the page's fixed headings (ComboRandomizerLayout.h); MM's catalog cards already carry them.
    const auto rows = world == 0 && page.sidebarName
                          ? ArrangeOotRandomizerPage(page.sidebarName, page.widgets, page.widgetCount)
                          : std::vector<CwWidget>(page.widgets, page.widgets + std::max(page.widgetCount, 0));
    const CwSidebar side{ page.sidebarName, page.columnCount, rows.data(), int(rows.size()) };
    // A body routed out of a wider sidebar may use one column only; then wide screens draw one column too.
    const bool oneColumn = std::all_of(rows.begin(), rows.end(), [&](const CwWidget& w) {
        return std::min(w.column, 1) == std::min(rows.front().column, 1);
    });
    const unsigned desired = oneColumn ? 1 : std::min<unsigned>(2, side.columnCount ? side.columnCount : 1);
    const unsigned columns = RandomizerColumnCount(side.sidebarName ? side.sidebarName : "", ImGui::GetFontSize(),
                                                   ImGui::GetContentRegionAvail().x, desired > 1);
    bool pouchesDrawn = false;
    const auto isPouch = [](const CwWidget& w) {
        constexpr char prefix[] = "gRandoSettings.SilverPouch";
        return w.cvar && std::strncmp(w.cvar, prefix, sizeof(prefix) - 1) == 0;
    };
    // Seed options no logic reads sit under the seed-settings heading of Gameplay rules (ComboMenuPlacement.h).
    // Skip Child Stealth has a live twin in Enhancements, so its seed row shows only while the viewed settings have it
    // on: decided here, because routing never depends on values.
    const auto hiddenSeedRow = [](const CwWidget& w) {
        return w.cvar && !std::strcmp(w.cvar, "gRandoSettings.SkipChildStealth") && !ReadInteger(w.cvar, 0);
    };
    auto draw = [&](int index) {
        const auto& widget = side.widgets[index];
        if (widget.index <= -1000 && DrawMenuPseudoRow(widget, true))
            return; // a gameplay rule row inside its heading
        if (hiddenSeedRow(widget))
            return;
        if (world == 0 && isPouch(widget)) {
            if (pouchesDrawn)
                return;
            pouchesDrawn = true;
            ImGui::BeginDisabled(!ReadInteger("gRandoSettings.SilverRupeeShuffle", 0));
            if (ImGui::CollapsingHeader("Silver rupee pouches"))
                for (int i = 0; i < side.widgetCount; ++i) {
                    const auto& pouch = side.widgets[i];
                    if (isPouch(pouch))
                        DrawWidget(pouch, game);
                }
            ImGui::EndDisabled();
            return;
        }
        if (widget.kind == CW_CUSTOM)
            DrawCustom(widget.name, world);
        else
            DrawWidget(widget, game);
        if (world == 0 && widget.kind == CW_SEPARATOR_TEXT && widget.name &&
            !std::strcmp(widget.name, "Scenery drops")) {
            int count = 0;
            while (index + 1 + count < side.widgetCount && side.widgets[index + 1 + count].kind != CW_SEPARATOR_TEXT)
                ++count;
            SceneryButtons(side.widgets + index + 1, count);
        }
    };
    if (columns == 1) {
        for (int i = 0; i < side.widgetCount; ++i)
            draw(i);
        return;
    }
    if (!ImGui::BeginTable("##NativeOptions", columns,
                           ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        return;
    ImGui::TableNextRow();
    for (unsigned column = 0; column < columns; ++column) {
        ImGui::TableSetColumnIndex(column);
        ImGui::PushID(int(column));
        ImGui::BeginGroup();
        for (int i = 0; i < side.widgetCount; ++i)
            if (unsigned(std::clamp(side.widgets[i].column, 0, int(columns) - 1)) == column)
                draw(i);
        ImGui::EndGroup();
        ImGui::PopID();
    }
    ImGui::EndTable();
}
static void CatalogRow(const Json& row, bool showHidden = false) {
    const auto key = row.at("key").get<std::string>(), label = row.at("label").get<std::string>(),
               kind = row.at("kind").get<std::string>();
    CwWidget widget{};
    widget.name = label.c_str();
    widget.cvar = key.c_str();
    widget.index = 0;
    std::string help = row.value("tooltip", std::string());
    widget.tooltip = help.c_str();
    std::vector<CwChoice> choices;
    std::vector<std::string> labels;
    if (kind == "checkbox")
        widget.kind = CW_CHECKBOX;
    else if (kind == "slider") {
        widget.kind = CW_SLIDER_INT;
        widget.iMin = row.at("minimum");
        widget.iMax = row.at("maximum");
        if (row.contains("maximumKey"))
            widget.iMax = ReadInteger(row["maximumKey"].get<std::string>().c_str(), widget.iMax);
        // The slider row's Default button returns to MM's own default for the option.
        const auto& mm = catalog.at("mm");
        const int fallback = mm.contains("defaults") ? mm.at("defaults").value(key, widget.iMin) : widget.iMin;
        widget.iDefault = std::max(widget.iMin, std::min(widget.iMax, fallback));
    } else {
        widget.kind = CW_COMBOBOX;
        for (const auto& c : row.at("choices"))
            labels.push_back(c.at("label"));
        int i = 0;
        for (const auto& c : row.at("choices"))
            choices.push_back({ labels[i++].c_str(), c.at("value") });
        widget.choices = choices.data();
        widget.choiceCount = int(choices.size());
    }
    bool visible = true, disabled = false;
    const auto get = [](const char* suffix, int fallback = 0) {
        const std::string key = std::string("gRando.Options.") + suffix;
        return ReadInteger(key.c_str(), fallback);
    };
    if (key == "gRando.Options.RO_SKULLTULA_TOKENS_REQUIRED" || key == "gRando.Options.RO_SKULLTULA_SHUFFLED")
        visible = get("RO_SHUFFLE_GOLD_SKULLTULAS") != 0;
    if (key == "gRando.Options.RO_CLOCK_SHUFFLE_PROGRESSIVE")
        visible = get("RO_CLOCK_SHUFFLE") != 0;
    if (key == "gRando.Options.RO_STRAY_FAIRIES_REQUIRED" || key == "gRando.Options.RO_STRAY_FAIRIES_MAX")
        visible = get("RO_PLACEMENT_STRAY_FAIRIES") != 2;
    if (key == "gRando.Options.RO_TRAP_AMOUNT")
        visible = get("RO_SHUFFLE_TRAPS") != 0;
    if (key == "gRando.Options.RO_SHUFFLE_SKELETON_KEY")
        disabled = get("RO_PLACEMENT_SMALL_KEYS") == 2;
    // Match the native SpellPoolCheckbox and GearPoolCheckbox: where this device cannot run an MM spell or the MM boots
    // and tunics, or while Swim is shuffled for the boots and tunics, new seeds leave it out. Keep the draft
    // preference so it returns once the row is offered again, and display old seed snapshots as saved.
    static const std::string kOptionPrefix = "gRando.Options.";
    const char* unavailable =
        Editable() && key.rfind(kOptionPrefix, 0) == 0
            ? MmPoolOptionUnavailable(key.c_str() + kOptionPrefix.size(), get("RO_SHUFFLE_SWIM") == 1)
            : nullptr;
    if (get("RO_LOGIC") == 3)
        for (const auto& value : catalog.at("mm").at("disabledInVanilla"))
            disabled |= value == key;
    if (!visible && !showHidden)
        return;
    ImGui::PushID(key.c_str());
    ImGui::BeginDisabled(disabled || unavailable || !visible);
    if (unavailable) {
        bool off = false;
        ComboMenu_CheckboxRow(label.c_str(), &off);
        Tooltip(unavailable);
    } else {
        DrawWidget(widget, ComboMenuModel::Get().Mm());
    }
    ImGui::EndDisabled();
    ImGui::PopID();
}
bool DrawCatalogSetting(const char* key) {
    for (int i = 0; i < int(std::size(Entrances::kEntranceSettings)); ++i)
        if (key && !std::strcmp(key, Entrances::kEntranceSettings[i].cvar) &&
            Entrances::kEntranceSettings[i].visible(Entrances::ActiveCapabilities().Selected(""))) {
            DrawEntranceSetting(i);
            return true;
        }
    const auto mm = catalog.is_object() ? catalog.find("mm") : catalog.end();
    if (!key || mm == catalog.end() || !mm->is_object() || !mm->contains("layout"))
        return false;
    for (const auto& row : mm->at("layout"))
        if (row.value("key", std::string()) == key) {
            CatalogRow(row, true);
            return true;
        }
    return false;
}
static void CheckList(int world) {
    const char* game = world == 0 ? "oot" : "mm";
    const char* key = world == 0 ? "gRandoSettings.ExcludedLocations" : "gRando.ExcludedChecks";
    static ImGuiTextFilter query;
    static int group = 0;
    // MM's list shows only checks its pool can hold; SoH does not export that per check, so OoT lists them all.
    if (world == 0)
        ImGui::TextWrapped("Lists every Ocarina of Time check, including ones these settings leave out of the pool.");
    query.Draw("Search checks");
    std::vector<std::string> selectedValues;
    if (world == 0) {
        std::istringstream text(ReadString(key));
        std::string item;
        while (std::getline(text, item, ','))
            if (!item.empty())
                selectedValues.push_back(item);
    } else if (auto* value = Field(key, false); value && value->is_array())
        selectedValues = value->get<std::vector<std::string>>();
    const auto& data = catalog.at(game);
    const Json groups = data.value("checkGroups", Json::array());
    if (world == 1 && !groups.empty()) {
        group = std::clamp(group, 0, int(groups.size()) - 1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ComboMenu_PushCombobox(ComboMenu_ThemeColor());
        if (ImGui::BeginCombo("##CheckGroup", groups[group].at("label").get<std::string>().c_str())) {
            for (int i = 0; i < int(groups.size()); ++i) {
                if (ImGui::Selectable(groups[i].at("label").get<std::string>().c_str(), group == i))
                    group = i;
                Tooltip(groups[i].value("tooltip", std::string()).c_str());
            }
            ImGui::EndCombo();
        }
        ComboMenu_PopCombobox();
    }
    std::vector<Json> shown;
    for (const auto& row : data.at("checks")) {
        const std::string id = row.at("value"), label = row.at("label");
        if (!query.PassFilter(label.c_str()))
            continue;
        if (world == 1) {
            const bool excluded = std::find(selectedValues.begin(), selectedValues.end(), id) != selectedValues.end();
            const auto& requirements = data.at("checkRequirements");
            bool eligible = requirements.contains(id);
            if (eligible) {
                const auto& rule = requirements.at(id);
                if (rule.contains("enabledKey"))
                    eligible = ReadInteger(rule["enabledKey"].get<std::string>().c_str(), 0) != 0;
                if (rule.contains("countKey"))
                    eligible &= ReadInteger(rule["countKey"].get<std::string>().c_str(), 30) > 0;
            }
            if (!eligible && !excluded)
                continue;
            if (!groups.empty() && !groups[group]["checks"].empty() &&
                std::find(groups[group]["checks"].begin(), groups[group]["checks"].end(), Json(id)) ==
                    groups[group]["checks"].end())
                continue;
        }
        shown.push_back(row);
    }
    bool changed = false;
    ImGui::BeginDisabled(!Editable());
    if (ImGui::Button("Exclude shown")) {
        for (const auto& row : shown) {
            const std::string id = row.at("value");
            if (std::find(selectedValues.begin(), selectedValues.end(), id) == selectedValues.end())
                selectedValues.push_back(id);
        }
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Include shown")) {
        for (const auto& row : shown) {
            const std::string id = row.at("value");
            selectedValues.erase(std::remove(selectedValues.begin(), selectedValues.end(), id), selectedValues.end());
        }
        changed = true;
    }
    ImGui::TextDisabled("%d checks shown", int(shown.size()));
    ImGui::BeginChild("##CheckRows", ImVec2(0, ImGui::GetFontSize() * 20));
    ImGuiListClipper clipper;
    clipper.Begin(int(shown.size()));
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            const auto& row = shown[i];
            const std::string label = row.at("label"), id = row.at("value");
            auto found = std::find(selectedValues.begin(), selectedValues.end(), id);
            bool checked = found != selectedValues.end();
            ImGui::PushID(id.c_str());
            if (ImGui::Checkbox(label.c_str(), &checked)) {
                if (checked)
                    selectedValues.push_back(id);
                else
                    selectedValues.erase(found);
                changed = true;
            }
            ImGui::PopID();
        }
    ImGui::EndChild();
    ImGui::EndDisabled();
    if (changed && Editable()) {
        if (world == 0) {
            std::string csv;
            for (const auto& v : selectedValues) {
                if (!csv.empty())
                    csv += ',';
                csv += v;
            }
            *Field(key, true) = csv;
        } else
            *Field(key, true) = selectedValues;
        dirty = true;
    }
}
static bool MmStartingAvailable(const Json& row) {
    const std::string item = row.at("value");
    const auto* starting = Field("gRando.StartingItems", false);
    if (starting && starting->is_array() &&
        std::find(starting->begin(), starting->end(), Json(item)) != starting->end())
        return true;
    if (row.value("excludeHunt", false) && ReadInteger("gCombo.Rando.TriforceHunt", 0))
        return false;
    if (row.contains("progressiveClock") &&
        row["progressiveClock"].get<bool>() != (ReadInteger("gRando.Options.RO_CLOCK_SHUFFLE_PROGRESSIVE", 0) != 0))
        return false;
    if (row.value("alwaysPool", false))
        return true;
    if (row.contains("poolKey"))
        return ReadInteger(row["poolKey"].get<std::string>().c_str(), 0) != 0;
    return mmBaseAvailable.count(item) != 0;
}
// Returns the last group heading drawn, so the page's other rows can continue under it.
static std::string StartingIcons(int world) {
    std::string last;
    const auto& rows = catalog.at(world == 0 ? "oot" : "mm").at(world == 0 ? "startingLayout" : "startingItems");
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    for (const char* group : { "Equipment", "Items", "Masks", "Songs", "Clocks (MM)", "Animal Souls", "Misc. Souls",
                               "NPC Souls", "Other" }) {
        // OoT's counts and choices (Link's Pocket, Starting Hearts) are full-width rows after the icons, as MM's
        // Starting Hearts is: an icon cell would squeeze their labels to a letter per line.
        std::vector<const Json*> groupRows, wideRows;
        for (const auto& row : rows) {
            if (row.value("category", std::string()) != group || (world == 1 && !MmStartingAvailable(row)))
                continue;
            if (row.contains("visibleKey") &&
                ReadInteger(row["visibleKey"].get<std::string>().c_str(), 0) != row["visibleValue"].get<int>())
                continue;
            const bool wide =
                world == 0 && (row.at("kind") == "StartingItemCount" || row.value("tiers", Json::array()).empty());
            (wide ? wideRows : groupRows).push_back(&row);
        }
        if (groupRows.empty() && wideRows.empty())
            continue;
        ImGui::SeparatorText(group);
        last = group;
        const auto unavailableRow = [](const Json& row) {
            bool unavailable = false;
            if (row.contains("disabledBy"))
                unavailable = ReadInteger(row["disabledBy"].get<std::string>().c_str(), 0) != 0;
            if (row.contains("requiresKey"))
                unavailable |=
                    ReadInteger(row["requiresKey"].get<std::string>().c_str(), 0) != row["requiresValue"].get<int>();
            if (row.contains("modeKey"))
                unavailable |= ReadInteger(row["modeKey"].get<std::string>().c_str(), 0) == row["noneMode"].get<int>();
            return unavailable;
        };
        const auto drawWideRows = [&] {
            for (const auto* entry : wideRows) {
                const auto& row = *entry;
                const std::string key = row.at("key"), label = row.at("label");
                const auto& option = catalog.at("oot").at("options").at(key);
                ImGui::PushID(key.c_str());
                ImGui::BeginDisabled(unavailableRow(row));
                if (row.at("kind") == "StartingItemCount") {
                    // The one slider row: the name above, the count in the bar, - and +, and Default.
                    CwWidget widget{};
                    widget.kind = CW_SLIDER_INT;
                    widget.name = label.c_str();
                    widget.cvar = key.c_str();
                    widget.iMax = int(option.at("choices").size()) - 1;
                    widget.iStep = 1;
                    widget.iDefault = catalog.at("oot").at("defaults").value(key, 0);
                    DrawWidget(widget, ComboMenuModel::Get().Oot());
                } else {
                    CatalogRow({ { "key", key },
                                 { "label", label },
                                 { "kind", "combo" },
                                 { "choices", option.at("choices") } });
                }
                ImGui::EndDisabled();
                ImGui::PopID();
            }
        };
        if (groupRows.empty()) {
            drawWideRows();
            continue;
        }
        const bool songGroup = std::strcmp(group, "Songs") == 0;
        if (songGroup)
            std::stable_sort(groupRows.begin(), groupRows.end(), [](const Json* a, const Json* b) {
                return a->value("songSlot", 100) < b->value("songSlot", 100);
            });
        const int columns = std::clamp(
            int(ImGui::GetContentRegionAvail().x /
                ((songGroup ? 32 : 48) + ImGui::GetStyle().FramePadding.x * 2 + ImGui::GetStyle().ItemSpacing.x)),
            1, 6);
        if (!ImGui::BeginTable(group, columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings)) {
            drawWideRows();
            continue;
        }
        int songRow = -1;
        for (const auto* entry : groupRows) {
            const auto& row = *entry;
            if (songGroup) {
                const int slot = row.value("songSlot", 100),
                          line = (slot / 6) * ((6 + columns - 1) / columns) + (slot % 6) / columns;
                if (line != songRow) {
                    ImGui::TableNextRow();
                    songRow = line;
                }
                ImGui::TableSetColumnIndex((slot % 6) % columns);
            } else
                ImGui::TableNextColumn();
            const std::string key = row.at(world == 0 ? "key" : "value");
            ImGui::PushID(key.c_str());
            int count = world == 0 ? ReadInteger(key.c_str(), 0) : 0;
            const char* arrayKey = "gRando.StartingItems";
            if (world == 1) {
                auto* values = Field(arrayKey, false);
                if (values && values->is_array())
                    for (const auto& v : *values)
                        if (v == key)
                            ++count;
            }
            auto setCount = [&](int value) {
                if (!Editable())
                    return;
                if (world == 0)
                    WriteInteger(key.c_str(), value);
                else {
                    auto* values = Field(arrayKey, true);
                    if (!values->is_array())
                        *values = Json::array();
                    auto next = Json::array();
                    for (const auto& v : *values)
                        if (v != key)
                            next.push_back(v);
                    for (int i = 0; i < value; ++i)
                        next.push_back(key);
                    *values = next;
                    dirty = true;
                }
            };
            ImGui::BeginDisabled(unavailableRow(row));
            const auto tiers = row.value("tiers", Json::array());
            int maximum = world == 0 ? int(tiers.size()) : row.at("maximum").get<int>();
            if (world == 1 && key == "RI_PROGRESSIVE_WALLET" &&
                ReadInteger("gRando.Options.RO_SHUFFLE_TYCOON_WALLET", 0))
                maximum = 3;
            if (row.contains("modeKey") &&
                ReadInteger(row["modeKey"].get<std::string>().c_str(), 0) == row["singleMode"].get<int>())
                maximum = 1;
            const bool song = row.value("song", false);
            const ImVec2 size(song ? 32.f : 48.f, 48.f);
            const auto& display = tiers.at(std::clamp(count - 1, 0, int(tiers.size()) - 1));
            const auto texture =
                display.at(count || !display.contains("faded") ? "texture" : "faded").get<std::string>();
            ImVec4 baseTint(1, 1, 1, 1);
            if (row.contains("tint")) {
                const auto& color = row["tint"];
                baseTint =
                    ImVec4(color[0].get<float>(), color[1].get<float>(), color[2].get<float>(), color[3].get<float>());
            }
            ImVec4 tint = baseTint;
            if (row.contains("nightClock"))
                tint = row["nightClock"].get<bool>() ? ImVec4(.3f, .5f, 1, 1) : ImVec4(1, .85f, .3f, 1);
            if (!count) {
                tint.x *= .35f;
                tint.y *= .35f;
                tint.z *= .35f;
            }
            ImGui::BeginDisabled(!Editable());
            ComboMenu_PushButton(ImVec4(.15f, .15f, .15f, 1));
            if (texture.empty() ? ImGui::Button(row.at("label").get<std::string>().c_str())
                                : ImGui::ImageButton("##Item", gui ? gui->GetTextureByName(texture) : nullptr, size,
                                                     ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint)) {
                if (maximum > 1)
                    ImGui::OpenPopup("##Tier");
                else
                    setCount(count ? 0 : 1);
            }
            ComboMenu_PopButton();
            ImGui::EndDisabled();
            Tooltip(row.at("label").get<std::string>().c_str());
            if (count && maximum > 1) {
                const auto badge = std::to_string(count);
                ImGui::GetWindowDrawList()->AddText(ImGui::GetItemRectMax() - ImGui::CalcTextSize(badge.c_str()) -
                                                        ImVec2(3, 2),
                                                    IM_COL32_WHITE, badge.c_str());
            }
            if (ImGui::BeginPopup("##Tier")) {
                ImGui::BeginDisabled(!Editable());
                if (ImGui::Button("None")) {
                    setCount(0);
                    ImGui::CloseCurrentPopup();
                }
                for (int tier = 1; tier <= maximum; ++tier) {
                    ImGui::SameLine();
                    ImGui::PushID(tier);
                    const auto& entry = tiers.at(std::min(tier - 1, int(tiers.size()) - 1));
                    const auto path = entry.at("texture").get<std::string>();
                    if (path.empty() ? ImGui::Button(entry.value("label", row.at("label").get<std::string>()).c_str())
                                     : ImGui::ImageButton("##Choice", gui ? gui->GetTextureByName(path) : nullptr, size,
                                                          ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), baseTint)) {
                        setCount(tier);
                        ImGui::CloseCurrentPopup();
                    }
                    Tooltip(entry.value("label", row.at("label").get<std::string>()).c_str());
                    ImGui::PopID();
                }
                ImGui::EndDisabled();
                ImGui::EndPopup();
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
        drawWideRows();
    }
    return last;
}
static void SariaPriority() {
    if (!ReadInteger("gRando.Options.RO_SHUFFLE_SONG_SARIA", 0))
        return;
    ImGui::SeparatorText("Saria's Song Priority Items");
    auto* field = Field("gRando.SariaPriorityItems", false);
    Json list = field && field->is_array() ? *field : Json::array();
    bool changed = false;
    const auto& choices = catalog.at("mm").at("sariaCandidates");
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());
    ImGui::BeginDisabled(!Editable());
    if (ImGui::Button("Reset to default")) {
        list = catalog.at("mm").at("defaults").at("gRando.SariaPriorityItems");
        changed = true;
    }
    ImGui::TextDisabled("%d / 16 items", int(list.size()));
    if (ImGui::BeginTable("##SariaPriority", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings)) {
        for (int i = 0; i < int(list.size()); ++i) {
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            auto found =
                std::find_if(choices.begin(), choices.end(), [&](const Json& row) { return row["value"] == list[i]; });
            const std::string label =
                found == choices.end() ? list[i].get<std::string>() : found->at("label").get<std::string>();
            if (found != choices.end() && !found->at("texture").get<std::string>().empty())
                ImGui::Image(gui ? gui->GetTextureByName(found->at("texture").get<std::string>()) : nullptr,
                             ImVec2(24, 24));
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", label.c_str());
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(i == 0);
            if (ImGui::Button("Up")) {
                std::swap(list[i], list[i - 1]);
                changed = true;
            }
            ImGui::EndDisabled();
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(i + 1 == int(list.size()));
            if (ImGui::Button("Down")) {
                std::swap(list[i], list[i + 1]);
                changed = true;
            }
            ImGui::EndDisabled();
            ImGui::TableNextColumn();
            if (ImGui::Button("Remove")) {
                list.erase(list.begin() + i);
                changed = true;
                --i;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::BeginDisabled(list.size() >= 16);
    if (ImGui::BeginCombo("Add item", "Choose an item")) {
        for (const auto& row : choices) {
            if (std::find(list.begin(), list.end(), row["value"]) != list.end() || !MmStartingAvailable(row))
                continue;
            if (ImGui::Selectable(row.at("label").get<std::string>().c_str())) {
                list.push_back(row["value"]);
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (changed && Editable()) {
        *Field("gRando.SariaPriorityItems", true) = list;
        dirty = true;
    }
}
void DrawCustom(const char* sidebar, int world) {
    const std::string name = sidebar ? sidebar : "";
    std::string iconHeading;
    if (name == "Starting Items")
        iconHeading = StartingIcons(world);
    if (name == "Locations" || name == "Excluded Locations" || name == "Check Exclusions") {
        CheckList(world);
        return;
    }
    if (world == 0)
        return;
    // Each card sits in the column of its first heading on the page (ComboRandomizerLayout.h), as OoT's rows do.
    const std::string page = MenuRoute(false, "Rando", name).page;
    const auto* layout = FindRandomizerPageHeadings(page);
    std::vector<std::string> cards;
    std::vector<int> cardColumns;
    for (const auto& row : catalog.at("mm").at("layout"))
        if (row.at("page") == name) {
            const std::string card = row.at("card");
            if (std::find(cards.begin(), cards.end(), card) == cards.end()) {
                cards.push_back(card);
                const auto heading = row.at("heading").get<std::string>();
                cardColumns.push_back(layout ? RandomizerHeadingColumn(*layout, heading.c_str()) : 0);
            }
        }
    // A page whose cards all sit in one column uses the whole width; OoT's rows split at the same width.
    const auto second = std::count(cardColumns.begin(), cardColumns.end(), 1);
    const int columns = RandomizerColumnCount(page, ImGui::GetFontSize(), ImGui::GetContentRegionAvail().x,
                                              second > 0 && second < std::ptrdiff_t(cards.size()));
    if (cards.empty() || !ImGui::BeginTable("##NativeMmCards", columns,
                                            ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        return;
    const auto drawCard = [&](const std::string& card) {
        ImGui::PushID(card.c_str());
        // A card can hold several headings; rows after the item icons continue under the icons' last group.
        std::string lastHeading = iconHeading;
        iconHeading.clear();
        bool sceneryButtons = card == "checkPoolScenery";
        for (const auto& row : catalog.at("mm").at("layout")) {
            if (row.at("page") != name || row.at("card") != card)
                continue;
            const auto heading = row.at("heading").get<std::string>();
            if (heading != lastHeading) {
                if (!heading.empty())
                    ImGui::SeparatorText(heading.c_str());
                lastHeading = heading;
            }
            if (sceneryButtons) {
                sceneryButtons = false;
                ImGui::BeginDisabled(!Editable());
                for (int value = 1; value >= 0; --value) {
                    if (value == 0)
                        ImGui::SameLine();
                    if (ImGui::Button(value ? "All On" : "All Off"))
                        for (const auto& scenery : catalog.at("mm").at("layout"))
                            if (scenery.at("card") == card)
                                WriteInteger(scenery.at("key").get<std::string>().c_str(), value);
                }
                ImGui::EndDisabled();
            }
            CatalogRow(row);
        }
        ImGui::PopID();
    };
    ImGui::TableNextRow();
    for (int column = 0; column < columns; ++column) {
        ImGui::TableSetColumnIndex(column);
        ImGui::BeginGroup();
        for (size_t i = 0; i < cards.size(); ++i)
            if (columns == 1 || cardColumns[i] == column)
                drawCard(cards[i]);
        ImGui::EndGroup();
    }
    ImGui::EndTable();
    if (name == "Item Pool")
        SariaPriority();
    // Trap disguises and effects are chosen at pickup, not by the seed, so they are live settings.
    if (name == "Traps")
        ImGui::TextWrapped(
            "Trap look and effects: Enhancements > Extra modes > Traps and Randomizer extras > Item appearance.");
}
void DrawGeneration() {
    if (!active || document.is_null()) {
        ImGui::TextUnformatted("No settings are available for this file.");
        return;
    }
    const auto generation = snapshot.value("generation", Json::object());
    const bool running = generation.value("running", false);
    std::string seed = document.value("seed", std::string());
    const auto mode = ReadGameMode(document);
    // Every randomizer mode refuses the setting here as generation does, solo modes with their own reason. Vanilla
    // preparation ignores it, so only the Entrances row shows the reason there.
    const auto entranceRefusal =
        !VanillaMode(mode) && Entrances::Active(MenuEntranceSettings().erWallmasters) ? EntranceUnavailable() : "";
    ImGui::BeginDisabled(!Editable() || running || !entranceRefusal.empty());
    if (ImGui::InputTextWithHint("Seed", "(blank = random)", &seed)) {
        document["seed"] = seed;
        dirty = true;
    }
    const char* action = VanillaMode(mode) ? "Prepare vanilla game" : CombinedMode(mode) ? "Generate combined seed" : "Generate single-game seed";
    if (ImGui::Button(action)) {
        refreshPending = true;
        try {
            bool committed = true;
            if (dirty) {
                committed = Response(callbacks.setDraft(selected, document.dump().c_str()));
                if (committed)
                    dirty = false;
            }
            if (committed)
                Response(callbacks.generate(selected, seed.c_str()));
        } catch (const std::exception& e) { error = e.what(); }
    }
    ImGui::EndDisabled();
    if (!entranceRefusal.empty())
        ImGui::TextWrapped("%s", entranceRefusal.c_str());
    if (running) {
        ImGui::Text("File %d: %s (%d / %d)", generation.value("file", selected) + 1,
                    generation.value("phase", std::string()).c_str(), generation.value("placed", 0),
                    generation.value("total", 0));
        ImGui::BeginDisabled(!generation.value("canCancel", false));
        if (ImGui::Button("Cancel")) {
            refreshPending = true;
            Response(callbacks.cancel());
        }
        ImGui::EndDisabled();
    }
    DrawAdvanced();
}
// Each write refreshes the MM pool view, a walk over every MM check: the hundred-odd writes of a preset froze the menu
// for seconds on the confirming frame, which kept showing the pressed button, so the view is refreshed once at the end.
static void ApplyCommunityPreset(const Json& preset) {
    deferPoolViews = true;
    document["oot"]["settings"] = Json::object();
    for (auto it = document["mm"]["settings"].begin(); it != document["mm"]["settings"].end();) {
        if (it.key().rfind("gRando.Options.", 0) == 0 || it.key().rfind("gCombo.Tricks.", 0) == 0)
            it = document["mm"]["settings"].erase(it);
        else
            ++it;
    }
    document["oot"]["settings"]["gRandoSettings.EnabledTricks"] = "";
    // Drop every logic assumption; the player's own enhancements stay, except the preset's rules turned on below.
    for (const auto& rule : kGameplayRules)
        ClearLogicRule(rule, WriteInteger);
    for (const char* game : { "oot", "mm" })
        for (const auto& value : catalog.at(game).at("defaults").items())
            document[game]["settings"][value.key()] = value.value();
    for (const auto& write : preset.at("writes").items())
        WriteInteger(write.key().c_str(), write.value());
    // Junked checks and the hidden Shared Ocarina (ComboPresets.h CommunityPresetValues), stored as they are.
    for (const auto& value : preset.at("values").items())
        *Field(value.key().c_str(), true) = value.value();
    document["mm"]["settings"]["gRando.StartingItems"] =
        catalog.at("mm").at("presetStartingItems").at(std::to_string(preset.at("id").get<int>()));
    // The rules the preset includes in logic also turn on in play; the loop above cleared the others' logic.
    for (const auto& rule : preset.at("rules").items())
        if (const auto* found = FindGameplayRule(rule.key().c_str()))
            SetLogicRule(*found, rule.value().get<int>(), WriteInteger);
    SynchronizeGenerationGuarantee(ReadInteger, WriteInteger);
    deferPoolViews = false;
    RefreshOptionPolicy(true);
    RefreshPoolViews();
    dirty = true;
}
void DrawHome() {
    if (!active)
        return;
    ImGui::SeparatorText("Game mode");
    int mode = static_cast<int>(ReadGameMode(document));
    const bool running = snapshot.value("generation", Json::object()).value("running", false);
    ImGui::BeginDisabled(!Editable() || running);
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 23);
    if (ImGui::Combo("##GameMode", &mode, kGameModeLabels, 5)) {
        document["gameMode"] = kGameModeKeys[mode];
        if (mode != COMBO_MODE_COMBINED)
            for (const char* key : {"entranceLayout", "entranceExpansion", "ordinaryEntranceExpansion", "grottoEntranceExpansion", "mixedEntranceExpansion"})
                document.erase(key);
        dirty = true;
    }
    ImGui::EndDisabled();
    const auto chosen = static_cast<ComboGameMode>(mode);
    ImGui::TextWrapped(CombinedMode(chosen)
        ? "Items can move between both games. Finish the selected combined goal."
        : VanillaMode(chosen)
        ? "Play the original game without shuffled items. Your enhancement settings still apply."
        : "Only the selected game's checks and items are shuffled. Defeat its final boss. Cross-game portals, shared items and combined goals are off for this run.");
    ImGui::TextWrapped("This changes File %d's next run. Existing saves keep their own game mode. Prepare or generate from an empty file, then start it at file select.", selected + 1);
    DrawGeneration();
    if (VanillaMode(chosen))
        return;
    ImGui::SeparatorText("Presets");
    const int columns = ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 60 ? 3 : MenuColumns(20.f);
    if (!ImGui::BeginTable("##CommunityPresets", columns,
                           ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        return;
    for (const auto& preset : catalog.at("combo").at("presets")) {
        ImGui::TableNextColumn();
        ImGui::PushID(preset.at("id").get<int>());
        ImGui::SeparatorText(preset.at("label").get<std::string>().c_str());
        ImGui::TextWrapped("%s", preset.at("summary").get<std::string>().c_str());
        ImGui::BeginDisabled(!Editable());
        if (ImGui::Button("Apply preset", ImVec2(-FLT_MIN, ImGui::GetFrameHeight() * 1.25f)))
            ImGui::OpenPopup("Apply preset?");
        ImGui::EndDisabled();
        Tooltip(preset.at("help").get<std::string>().c_str());
        // ApplyCommunityPreset turns the preset's gameplay rules on in play too (SetLogicRule).
        const std::string presetText =
            "File " + std::to_string(selected + 1) + "'s next-seed settings become " +
            preset.at("label").get<std::string>() +
            (preset.at("rules").empty() ? ". Your enhancements and saved progress stay."
                                        : ". Gameplay rules the preset uses are switched on; your other enhancements "
                                          "and saved progress stay.");
        if (MenuConfirm("Apply preset?", presetText.c_str(), "Apply preset") && Editable())
            ApplyCommunityPreset(preset);
        ImGui::Spacing();
        ImGui::PopID();
    }
    ImGui::EndTable();
}
void DrawSavedPlacements() {
    const auto& seed = InspectedSeed();
    if (!seed.is_object()) {
        ImGui::TextUnformatted("No seed placements are available for this file.");
        return;
    }
    // A seed holds thousands of placements: list them once per inspection and draw only visible rows.
    static std::vector<std::pair<std::string, std::string>> rows[2];
    static uint64_t listed = 0;
    static int listedView = -1;
    if (listed != snapshotSerial || listedView != view) {
        listed = snapshotSerial;
        listedView = view;
        for (int i = 0; i < 2; ++i) {
            rows[i].clear();
            const Json* placements = nullptr;
            if (const auto game = seed.find(i == 0 ? "oot" : "mm"); game != seed.end() && game->is_object())
                if (const auto found = game->find("placements"); found != game->end() && found->is_object())
                    placements = &*found;
            if (!placements)
                continue;
            for (const auto& row : placements->items())
                rows[i].emplace_back(row.key(),
                                     row.value().is_string() ? row.value().get<std::string>() : row.value().dump());
        }
    }
    for (int i = 0; i < 2; ++i) {
        ImGui::SeparatorText(i == 0 ? "Ocarina of Time" : "Majora's Mask");
        if (rows[i].empty())
            continue;
        ImGui::PushID(i);
        ImGui::BeginChild("##Placements", ImVec2(0, ImGui::GetFontSize() * 20));
        if (ImGui::BeginTable("##Rows", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Check");
            ImGui::TableSetupColumn("Item");
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin(int(rows[i].size()));
            while (clipper.Step())
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                    const auto& [check, item] = rows[i][row];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(check.c_str());
                    Tooltip(check.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item.c_str());
                    Tooltip(item.c_str());
                }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::PopID();
    }
}
void DrawAdvanced() {
    if (ImGui::CollapsingHeader("Advanced seed inspection")) {
        static std::string raw;
        static uint64_t dumped = 0;
        static int dumpedView = -1;
        if (dumped != snapshotSerial || dumpedView != view) {
            dumped = snapshotSerial;
            dumpedView = view;
            raw = InspectedSeed().dump(2);
        }
        // Unwrapped text lets ImGui skip the lines outside the window; long lines scroll sideways.
        ImGui::BeginChild("##SeedDump", ImVec2(0, ImGui::GetFontSize() * 20), 0, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(raw.c_str(), raw.c_str() + raw.size());
        ImGui::EndChild();
    }
}
void DrawTricks() {
    static char query[160] = {};
    static int world = 0, state = 0, skill = 0;
    ApplyTrickFocus(query, world, state, skill);
    ImGui::InputTextWithHint("##TechniqueSearch", "Search techniques", query, sizeof(query));
    const char* names[] = { "Both games", "Ocarina of Time", "Majora's Mask" };
    for (int i = 0; i < 3; ++i) {
        if (i)
            ImGui::SameLine();
        if (ComboMenu_StyledHeaderEntry(names[i], world == i))
            world = i;
    }
    const char* modes[] = { "All techniques", "Enabled only", "Disabled only" };
    ImGui::Combo("Show", &state, modes, 3);
    const char* levels[] = { "All difficulties", "Novice",  "Intermediate", "Advanced",
                             "Expert",           "Extreme", "Experimental" };
    ImGui::Combo("Difficulty", &skill, levels, 7);
    if (world != 2)
        for (const auto* rule : { FindGameplayRule("gEnhancements.RestoreShadowDoorUnload"),
                                  FindGameplayRule("gEnhancements.EarlyEyeballFrog") }) {
            const bool original = ReadInteger(LogicRuleKey(rule->name).c_str(), 0) != 0;
            const auto row = std::strcmp(rule->name, "gEnhancements.RestoreShadowDoorUnload") == 0
                                 ? ComboTricks::RestoredShadowDoorInfo(rule->label, rule->help, original)
                                 : ComboTricks::EarlyEyeballFrogInfo(rule->label, rule->help, original);
            if (!ComboTricks::Matches(row, 0, query, world, state, skill - 1))
                continue;
            ImGui::PushID(rule->name);
            bool enabled = original;
            ImGui::BeginDisabled(!Editable());
            ComboMenu_PushCheckbox(ComboMenu_ThemeColor());
            if (ImGui::Checkbox(rule->label, &enabled))
                SetTrickGovernedRule(*rule, enabled, WriteInteger);
            ComboMenu_PopCheckbox();
            ImGui::EndDisabled();
            Tooltip(rule->help);
            ImGui::TextDisabled("%s · %s", row.area, row.tags);
            if (ImGui::TreeNode("Details")) {
                ImGui::TextWrapped("%s", rule->help);
                ImGui::TreePop();
            }
            ImGui::Separator();
            ImGui::PopID();
        }
    const auto folded = ComboTricks::Fold(query);
    for (int game = 0; game < 2; ++game) {
        if (world && world != game + 1)
            continue;
        const char* key = game == 0 ? "gRandoSettings.EnabledTricks" : "";
        std::vector<std::string> csv;
        if (game == 0) {
            std::istringstream stream(ReadString(key));
            std::string tag;
            while (std::getline(stream, tag, ','))
                if (!tag.empty())
                    csv.push_back(tag);
        }
        for (const auto& row : catalog.at(game == 0 ? "oot" : "mm").at("tricks")) {
            const std::string label = row.at("label"), id = row.at("value"),
                              help = row.value("description", std::string());
            bool on = game == 0 ? std::find(csv.begin(), csv.end(), id) != csv.end() : ReadInteger(id.c_str(), 0) != 0;
            if ((state == 1 && !on) || (state == 2 && on) || (skill && row.value("difficulty", 0) != skill - 1) ||
                (query[0] && ComboTricks::Fold(label + ' ' + help + ' ' + row.value("area", std::string()) + ' ' +
                                               row.value("tags", std::string()))
                                     .find(folded) == std::string::npos))
                continue;
            ImGui::PushID(id.c_str());
            ImGui::BeginDisabled(!Editable());
            ComboMenu_PushCheckbox(ComboMenu_ThemeColor());
            if (ImGui::Checkbox(label.c_str(), &on)) {
                if (game == 1)
                    WriteInteger(id.c_str(), on);
                else {
                    auto found = std::find(csv.begin(), csv.end(), id);
                    if (on)
                        csv.push_back(id);
                    else if (found != csv.end())
                        csv.erase(found);
                    std::string text;
                    for (const auto& tag : csv) {
                        if (!text.empty())
                            text += ',';
                        text += tag;
                    }
                    *Field(key, true) = text;
                    dirty = true;
                }
            }
            ComboMenu_PopCheckbox();
            ImGui::EndDisabled();
            Tooltip(help.c_str());
            ImGui::TextDisabled("%s · %s · %s", game == 0 ? "OoT" : "MM",
                                row.value("area", std::string("General")).c_str(),
                                row.value("tags", std::string()).c_str());
            if (ImGui::TreeNode("Details")) {
                ImGui::TextWrapped("%s", help.c_str());
                ImGui::TreePop();
            }
            ImGui::Separator();
            ImGui::PopID();
        }
    }
}
} // namespace ComboRando::ProfileUI
extern "C" COMBO_EXPORT void ComboUI_SetRandomizerProfileStore(const ComboRandomizerProfileCallbacks* source) {
    using namespace ComboRando::ProfileUI;
    callbacks = {};
    refreshPending = true;
    snapshotValid = false;
    dirty = false; // an edit still waiting for EndFrame belongs to the replaced store
    if (!source || (source->version != 1 && source->version != 2))
        return;
    // Copy the version-1 prefix without reading beyond an older caller's struct.
    callbacks = { source->version,  source->inspect,
                  source->setDraft, source->copySaved,
                  source->generate, source->copyDraft,
                  source->cancel,   source->version >= 2 ? source->revision : nullptr };
    // Prime at runtime initialization, before gameplay/menu opening. This helper
    // uses no ImGui calls and never applies settings or writes a gameplay save.
    if (Ready() && callbacks.version >= 2 && callbacks.revision)
        try {
            RefreshSnapshot(0, callbacks.revision());
        } catch (const std::exception& e) { error = e.what(); }
}
extern "C" COMBO_EXPORT int ComboUI_GetSelectedSettingsFile() {
    return ComboRando::ProfileUI::SelectedSlot();
}
