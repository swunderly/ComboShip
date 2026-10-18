#include "ComboRandomizerProfiles.h"
#include "ComboTrackerControls.h"
// combo/gui/ComboMenu.cpp
#include "ComboMenu.h"
#include "ComboExport.h"
#include "ComboMenuModel.h"
#include "ComboNavigation.h"
#include "ComboMenuNavigation.h"
#include "ComboMenuLayout.h"
#include "ComboMenuLabels.h"
#include "ComboRandomizerLayout.h"
#include "rando/SharedItems.h"
#include "rando/ComboSeedFormat.h"
#include "rando/entrances/OotmmSettings.h"
#include "ComboWidgetRender.h"
#include "ComboWidgetStyle.h"
#include "ComboForeground.h" // dormant-game gate for inline tracker popout buttons
#include "ComboTrackerBridge.h"
#include "ComboTrackerCommon.h" // OotActiveSlot
#include "ComboTrackerSwap.h"
#include "ComboAnchorRoomWindow.h" // combo-native floating Anchor room window
#include "ComboNotesWindow.h"      // combo-owned cross-game Personal Notes window
#include "ComboHintTracker.h"      // combo-owned unified Hint Tracker (#164)
#include "ComboCheckTracker.h"
#include "ComboCheckSnapshot.h"
#include "rando/GameplayRules.h"
#include "rando/GenerationRules.h"
#include "ComboRuleWarning.h"
#include "ComboHudLayout.h"
#include "ComboTricks.h"
#include "menu/ComboTrickABI.h"
#include "ComboTimersWindow.h"      // combo-owned overlay timers (#173)
#include "rando/ComboPlaythrough.h" // plando: ParseSpoilerPlacements + Suffix/BuildForeignArray + slot paths
#include <imgui.h>
#include <imgui_internal.h>                    // ClosePopupToLevel: Back closes the top popup
#include <libultraship/libultraship.h>         // CVar bridge (CVarGet/Set* incl. color) + color.h (Color_RGBA8)
#include <ship/window/gui/IconsFontAwesome4.h> // ICON_FA_* for the header action buttons
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <functional>
#include <unordered_map>
#include <cmath>
#include <unordered_set>
#include "ComboResolve.h"

namespace {
// soh.dll exports: read the shared generation progress, and tell whether the file-select screen
// (the only place the save state may be rewritten) is showing.
typedef const ComboRando::ComboGenProgress* (*FnGetProgress)(void);
typedef unsigned char (*FnIsOnFileSelect)(void);
FnGetProgress sGetProgress = nullptr;
FnIsOnFileSelect sIsOnFileSelect = nullptr;
void ResolveComboGenSyms() {
    if (!sGetProgress)
        sGetProgress = (FnGetProgress)Combo_ResolveSym("soh", "SOH_GetComboGenProgress");
    if (!sIsOnFileSelect)
        sIsOnFileSelect = (FnIsOnFileSelect)Combo_ResolveSym("soh", "SOH_CanGenerateComboSeed");
}

// Bug 2: Anchor resync exports, one per game DLL — resolved the same way as the combo-gen syms
// above. The button calls both so a resync pulls the peer's OOT AND MM team-state.
typedef void (*FnRequestResync)(void);
FnRequestResync sSohRequestResync = nullptr;
FnRequestResync sMmRequestResync = nullptr;
// Anchor connection panel: soh drives Enable/Disable + reports connection state (the socket
// is launcher-owned, but enable/status stays in soh's Anchor object). Resolved from soh.dll.
typedef void (*FnSetEnabled)(int);
typedef int (*FnGetConnState)(void);
FnSetEnabled sSohAnchorSetEnabled = nullptr;
FnGetConnState sSohAnchorGetConnState = nullptr;
// Room-admin: owner-gating + room-state broadcast + clear-team-state, all in soh's Anchor object.
typedef int (*FnGetOwnerInfo)(void);
typedef void (*FnSendRoomState)(void);
typedef void (*FnClearTeamState)(void);
FnGetOwnerInfo sSohAnchorGetOwnerInfo = nullptr;
FnSendRoomState sSohAnchorSendRoomState = nullptr;
FnClearTeamState sSohAnchorClearTeamState = nullptr;
void ResolveAnchorResyncSyms() {
    if (!sSohRequestResync)
        sSohRequestResync = (FnRequestResync)Combo_ResolveSym("soh", "SOH_Anchor_RequestResync");
    if (!sSohAnchorSetEnabled)
        sSohAnchorSetEnabled = (FnSetEnabled)Combo_ResolveSym("soh", "SOH_Anchor_SetEnabled");
    if (!sSohAnchorGetConnState)
        sSohAnchorGetConnState = (FnGetConnState)Combo_ResolveSym("soh", "SOH_Anchor_GetConnectionState");
    if (!sSohAnchorGetOwnerInfo)
        sSohAnchorGetOwnerInfo = (FnGetOwnerInfo)Combo_ResolveSym("soh", "SOH_Anchor_GetOwnerInfo");
    if (!sSohAnchorSendRoomState)
        sSohAnchorSendRoomState = (FnSendRoomState)Combo_ResolveSym("soh", "SOH_Anchor_SendRoomState");
    if (!sSohAnchorClearTeamState)
        sSohAnchorClearTeamState = (FnClearTeamState)Combo_ResolveSym("soh", "SOH_Anchor_ClearTeamState");
    if (!sMmRequestResync)
        sMmRequestResync = (FnRequestResync)Combo_ResolveSym("2ship", "MM_Anchor_RequestResync");
}

// Shared Anchor config CVar keys (process-global libultraship store; both game DLLs read these — see
// MMAnchor.cpp). comboui has no access to soh's CVAR_REMOTE_ANCHOR macro, so the literals live here.
namespace AnchorCVar {
constexpr const char* Host = "gRemote.Anchor.Host";
constexpr const char* Port = "gRemote.Anchor.Port";
constexpr const char* Name = "gRemote.Anchor.Name";
constexpr const char* RoomId = "gRemote.Anchor.RoomId";
constexpr const char* TeamId = "gRemote.Anchor.TeamId";
constexpr const char* ColorValue = "gRemote.Anchor.Color.Value";
constexpr const char* ShowOnMinimap = "gRemote.Anchor.ShowOtherPlayersOnMinimap";
// Room-admin settings: shared store both games read; changes broadcast via SendRoomState.
constexpr const char* PvpMode = "gRemote.Anchor.RoomSettings.PvpMode";
constexpr const char* ShowLocationsMode = "gRemote.Anchor.RoomSettings.ShowLocationsMode";
constexpr const char* TeleportMode = "gRemote.Anchor.RoomSettings.TeleportMode";
constexpr const char* SyncItemsAndFlags = "gRemote.Anchor.RoomSettings.SyncItemsAndFlags";
} // namespace AnchorCVar

// Combo plandomizer exports: each game's static-data dump (friendly item names) + the reload trigger
// that plays an edited consolidated spoiler back verbatim. Resolved like the combo-gen syms above.
typedef const char* (*FnDump)(void);
typedef int (*FnRequestReload)(const char*);
typedef int (*FnPlandoFileSelectSlot)(void);
FnDump sSohDump = nullptr;
FnDump sMmDump = nullptr;
FnRequestReload sRequestReload = nullptr;
FnPlandoFileSelectSlot sPlandoFileSelectSlot = nullptr;
void ResolvePlandoSyms() {
    if (!sSohDump)
        sSohDump = (FnDump)Combo_ResolveSym("soh", "SOH_DumpRandoStaticData");
    if (!sRequestReload)
        sRequestReload = (FnRequestReload)Combo_ResolveSym("soh", "SOH_RequestComboReload");
    if (!sPlandoFileSelectSlot)
        sPlandoFileSelectSlot = (FnPlandoFileSelectSlot)Combo_ResolveSym("soh", "SOH_GetFileSelectSlot");
    if (!sMmDump)
        sMmDump = (FnDump)Combo_ResolveSym("2ship", "MM_DumpRandoStaticData");
}

int PlandoImportTarget() {
    const int slot = sPlandoFileSelectSlot ? sPlandoFileSelectSlot() : -1;
    return slot >= 0 && slot < 3 ? slot : -1;
}

// Combo plandomizer editable state (single ComboMenu instance -> file-static). rows is the edited
// placement model; loadedJson is the original consolidated file, preserved on write-back so only
// placements/foreign change. items is the combined per-game picker list (bare name + game + display).
struct PlandoPickItem {
    std::string name; // bare friendly name (item's own game namespace)
    ComboRando::GameId game;
    bool advancement;
    std::string display; // "name (OOT)" / "name (MM)"
};
struct PlandoState {
    bool loaded = false;
    bool statusError = false;
    std::string status;
    std::string loadedJson; // original consolidated file text (write-back base)
    std::string sohDump, mmDump;
    std::vector<ComboRando::CwPlacedItem> rows;
    std::vector<PlandoPickItem> items;
    std::vector<std::string> spoilerNames; // selectable spoilers in the Randomizer folder (display stems)
    std::vector<std::string> spoilerPaths; // parallel full paths
    int spoilerSel = -1;
    char filter[128] = { 0 };
    char pickerFilter[128] = { 0 };
};
static PlandoState sPlando;

// List every *.json in the Randomizer folder as a loadable spoiler; default the selection to the
// remembered (most recently generated) one when it's among them.
void PlandoRefreshSpoilerList() {
    sPlando.spoilerNames.clear();
    sPlando.spoilerPaths.clear();
    std::error_code ec;
    auto dir = ComboRando::ConsolidatedDir();
    if (std::filesystem::exists(dir, ec)) {
        for (const auto& e : std::filesystem::directory_iterator(dir, ec)) {
            if (e.is_regular_file() && e.path().extension() == ".json" &&
                e.path().filename() != "settings-profiles.json") {
                sPlando.spoilerPaths.push_back(e.path().string());
                sPlando.spoilerNames.push_back(e.path().stem().string());
            }
        }
    }
    sPlando.spoilerSel = sPlando.spoilerPaths.empty() ? -1 : 0;
    std::string remembered = std::filesystem::path(CVarGetString("gGeneral.ComboSpoiler", "")).stem().string();
    if (remembered.empty()) {
        return;
    }
    for (int i = 0; i < (int)sPlando.spoilerNames.size(); i++) {
        if (sPlando.spoilerNames[i] == remembered) {
            sPlando.spoilerSel = i;
            break;
        }
    }
}
} // namespace

namespace ComboRando {

static std::shared_ptr<ComboMenu> sComboMenu;

// Search normalization shared by the query and every widget haystack: lowercase + strip spaces.
static std::string NormalizeSearch(std::string s) {
    return NormalizeMenuSearch(std::move(s));
}
static const ImVec4 kBreadcrumbColor(0.6f, 0.6f, 0.6f, 1.0f); // muted gray for the result origin line

void ComboMenu::Draw() {
    if (auto ctx = Ship::Context::GetRawInstance(); ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui())
        ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
    ResolveComboGenSyms();
    const auto* generation = sGetProgress ? sGetProgress() : nullptr;
    if (generation && generation->running && !ImGui::IsPopupOpen("Generating combined seed"))
        ImGui::OpenPopup("Generating combined seed");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(.5f, .5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(540.0f, ImGui::GetMainViewport()->Size.x * .9f), 0), ImGuiCond_Always);
    if (ImGui::BeginPopupModal("Generating combined seed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!generation || !generation->running)
            ImGui::CloseCurrentPopup();
        else {
            const auto elapsed = generation->ElapsedSeconds();
            ImGui::Text("Attempt %llu - %llu:%02llu", (unsigned long long)generation->attempt.load(),
                        (unsigned long long)(elapsed / 60), (unsigned long long)(elapsed % 60));
            ImGui::TextUnformatted(ComboGenProgress::PhaseLabel(generation->phase));
            const int total = generation->total.load();
            if (total > 0)
                ImGui::ProgressBar(float(generation->placed.load()) / total, ImVec2(-1, 0));
            ImGui::TextWrapped("Invalid layouts are retried automatically. Your existing saves keep their seeds.");
            if (generation->CancelRequested())
                ImGui::TextUnformatted("Cancelling after the current step...");
            else if (!generation->CanCancel())
                ImGui::TextUnformatted("Saving the completed seed...");
            else if (generation->WarningDue(elapsed)) {
                ImGui::Separator();
                ImGui::TextWrapped(
                    "Your seed may be too difficult to generate. Try changing some settings and trying again.");
                if (ImGui::Button("Keep trying", ImVec2(-1, 0)))
                    generation->KeepTrying(elapsed);
                if (ImGui::Button("Stop and change settings", ImVec2(-1, 0)))
                    generation->RequestCancel();
            } else if (ImGui::Button("Cancel generation", ImVec2(-1, 0)))
                generation->RequestCancel();
        }
        ImGui::EndPopup();
    }
    // Mirror Ship::Menu::Draw — skip GuiWindow::Draw's normal Begin/End wrapper so DrawElement
    // can open its own fullscreen overlay window instead of a floating one.
    if (!IsVisible()) {
        ProfileUI::Flush();
        return;
    }
    DrawElement();
    SyncVisibilityConsoleVariable();
}

void ComboMenu::OpenAtRandomizer() {
    mScope = "randomizer";
    mHubActive = "randomizer/Generate";
    mSearchBuf[0] = 0;
    mSearchQuery.clear();
    mSearchReturn.Clear();
    mRuleFocus.Clear();
    mResetContentScroll = true;
    Show();
}

void ComboMenu::DrawElement() {
    // ImGui's GImGui (current-context) is a per-module global; comboui.dll has its own,
    // separate from libultraship.dll where the context actually lives. Point it at the shared
    // context before any ImGui call here (same pattern soh.dll/2ship.dll use) — otherwise
    // ImGui::BeginTabBar dereferences a null context and crashes.
    auto ctx = Ship::Context::GetRawInstance();
    if (ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui()) {
        ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
    }

    // Fullscreen overlay covering the viewport work area, matching the old port menu
    // (NoDecoration/NoMove, sized to the viewport, with a translucent backdrop).
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(vp->WorkSize, ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,
                          ImVec4(0.035f, 0.045f, 0.08f, CVarGetFloat("gSettings.Menu.BackgroundOpacity", 0.85f)));
    bool open = ImGui::Begin("Combo Menu", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (open) {
        // Center an inset menu block over the full-screen backdrop (mirrors SoH DrawElement: 90% /
        // 16:9 cap, centered) instead of filling the viewport edge-to-edge.
        ImGuiViewport* mvp = ImGui::GetMainViewport();
        const float ww = mvp->WorkSize.x, wh = mvp->WorkSize.y;
        ImVec2 menuSize(ww, wh);
        if (ww > 1280.0f) {
            menuSize.x = std::min(ww * 0.9f, wh * 1.77f);
        }
        if (wh > 800.0f) {
            menuSize.y = wh * 0.9f;
        }
        ImGui::SetCursorScreenPos(
            ImVec2(mvp->WorkPos.x + (ww - menuSize.x) * 0.5f, mvp->WorkPos.y + (wh - menuSize.y) * 0.5f));
        ImGui::BeginChild("##ComboMenuBlock", menuSize, 0, ImGuiWindowFlags_NoScrollbar);

        // A single app identity and a readable action row, separate from navigation.
        const ImVec4 accent(0.57f, 0.70f, 0.96f, 1.0f);
        ImGui::TextColored(accent, "COMBO SHIP");
        const float returnW = ImGui::CalcTextSize("Return to game").x + 32.0f;
        ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - returnW));
        ComboMenu_PushButton(ComboMenu_ThemeColor());
        if (ImGui::Button("Return to game", ImVec2(returnW, 0)))
            Hide();
        ComboMenu_PopButton();
        ProfileUI::BeginFrame();
        // Only these tabs read the selected file's settings (Enhancements: its next seed while no run is loaded).
        if (mScope == "home" || mScope == "randomizer" || mScope == "gameplay")
            ProfileUI::DrawSelector();
        ImGui::Spacing();
        struct Scope {
            const char* id;
            const char* label;
        };
        static const Scope scopes[] = {
            { "home", "Home" },           { "gameplay", "Enhancements" }, { "randomizer", "Randomizer" },
            { "media", "Video & Audio" }, { "trackers", "Trackers" },     { "settings", "Settings" },
            { "tools", "Tools" }
        };
        if (mScope.empty())
            mScope = "home";
        ImGui::PushID("ComboSections");
        bool first = true;
        for (const auto& scope : scopes) {
            const float need = ImGui::CalcTextSize(scope.label).x + 32.0f;
            if (!first &&
                ImGui::GetItemRectMax().x + need + 12.0f < ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x)
                ImGui::SameLine();
            first = false;
            if (ComboMenu_StyledHeaderEntry(scope.label, mScope == scope.id)) {
                mSectionSelection[mScope] = mHubActive;
                mScope = scope.id;
                mHubActive = mSectionSelection[mScope];
                mRuleFocus.Clear();
                ClearSearch(); // even on the tab already shown
            }
        }
        if (FindMenuGuide(mScope)) {
            const float need = ImGui::CalcTextSize("Guide").x + 32.0f;
            if (ImGui::GetItemRectMax().x + need + 12.0f < ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x)
                ImGui::SameLine();
            if (ImGui::Button("Guide"))
                mHelpRequested = true;
        }
        ImGui::PopID();
        ImGui::Spacing();
        const float clearWidth = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        const bool hasSearch = mSearchBuf[0] != 0;
        ImGui::SetNextItemWidth(
            hasSearch ? std::max(1.0f, ImGui::GetContentRegionAvail().x - clearWidth - ImGui::GetStyle().ItemSpacing.x)
                      : -FLT_MIN);
        if (ImGui::InputTextWithHint("##ComboSearch", "Search all settings...", mSearchBuf, sizeof(mSearchBuf)))
            mSearchReturn.Clear();
        mSearchField = ImGui::GetItemID();
        if (hasSearch) {
            ImGui::SameLine();
            if (ImGui::Button("Clear##ComboSearch")) {
                ClearSearch();
                mRuleFocus.Clear();
            }
        }
        if (!mSearchBuf[0] && mSearchReturn.HasResult()) {
            if (ImGui::Button("Back to search results", ImVec2(0, ImGui::GetFrameHeight() * 1.25f)))
                ReturnToSearchResults();
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        mSearchQuery = MenuSearchWords(mSearchBuf).empty() ? "" : mSearchBuf;
        DrawSharedPanel();
        DrawScopeHelp();
        const std::string navSig = mScope + "|" + mHubActive;
        if (navSig != mLastNavSig) {
            if (!mLastNavSig.empty()) {
                mSearchBuf[0] = 0;
                mSearchQuery.clear();
            }
            mLastNavSig = navSig;
        }
        ImGui::EndChild();
    }
    ProfileUI::EndFrame();
    ImGui::End();
}

// Ends the search: its text, its results and the way back to them.
void ComboMenu::ClearSearch() {
    mSearchBuf[0] = 0;
    mSearchQuery.clear();
    mSearchReturn.Clear();
}

void ComboMenu::ReturnToSearchResults() {
    mScope = mSearchReturn.scope;
    mHubActive = mSearchReturn.hub;
    std::snprintf(mSearchBuf, sizeof(mSearchBuf), "%s", mSearchReturn.query.c_str());
    mSearchReturn.Clear();
    mRuleFocus.Clear();
    mResetContentScroll = true;
    mLastNavSig = mScope + "|" + mHubActive;
}

bool ComboMenu::HandleBack() {
    if (!IsVisible())
        return false;
    if (auto ctx = Ship::Context::GetRawInstance(); ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui())
        ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
    if (const int popups = ImGui::GetCurrentContext()->OpenPopupStack.Size; popups > 0) {
        ImGui::ClosePopupToLevel(popups - 1, true);
        return true;
    }
    // Back ends a text edit whose keyboard already closed. The search field drops its text first: ImGui writes an
    // edited field's text back into its buffer when the edit ends, which would undo clearing the search.
    if (auto* edit = ImGui::GetInputTextState(ImGui::GetActiveID())) {
        if (edit->ID == mSearchField)
            edit->ClearText();
        ImGui::ClearActiveID();
    }
    if (mSearchBuf[0]) {
        ClearSearch();
        mRuleFocus.Clear();
        return true;
    }
    if (mSearchReturn.HasResult()) {
        ReturnToSearchResults();
        return true;
    }
    return false;
}

void ComboMenu::OpenSearchDestination(const std::string& scope, const std::string& page, const char* rule,
                                      const char* trick, int targetGame) {
    mSearchReturn.Capture(mSearchQuery, mScope, mHubActive);
    mSectionSelection[mScope] = mHubActive;
    mScope = scope;
    mHubActive = scope + "/" + page;
    mSectionSelection[mScope] = mHubActive;
    mRuleFocus.Set(rule);
    if (trick)
        FocusTrick(trick);
    if (targetGame >= 0 && targetGame <= 1) {
        if (scope == "randomizer")
            mRandomizerGame = targetGame;
        else
            mPreferenceGame = targetGame + 1;
    }
    mSearchBuf[0] = 0;
    mSearchQuery.clear();
    mResetContentScroll = true;
    mResetOptionsScroll = true;
}

void ComboMenu::DrawScopeHelp() {
    if (const auto* guide = FindMenuGuide(mScope); guide && mHelpRequested) {
        mGuideScope = mScope;
        ImGui::OpenPopup("Combo Ship guide");
        mHelpRequested = false;
    }
    const auto* guide = FindMenuGuide(mGuideScope);
    if (!guide)
        return;
    bool open = true;
    const float width = std::min(ImGui::GetMainViewport()->WorkSize.x * 0.82f, ImGui::GetFontSize() * 37.0f);
    ImGui::SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Combo Ship guide", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", guide->title);
        ImGui::Separator();
        ImGui::Spacing();
        // Keep the action visible on a phone; longer copy scrolls separately.
        const float copyHeight =
            std::max(ImGui::GetFontSize() * 3.0f,
                     std::min(ImGui::GetFontSize() * 7.0f,
                              ImGui::GetMainViewport()->WorkSize.y * 0.85f - ImGui::GetFontSize() * 7.0f));
        ImGui::BeginChild("##GuideCopy", ImVec2(0, copyHeight));
        ImGui::TextWrapped("%s", guide->introduction);
        ImGui::Spacing();
        ImGui::TextWrapped("%s", guide->behavior);
        ImGui::Spacing();
        ImGui::TextWrapped("%s", guide->runWarning);
        ImGui::EndChild();
        ImGui::Spacing();
        const float height = ImGui::GetFrameHeight() * 1.25f;
        if (ImGui::Button("Got it", ImVec2(-FLT_MIN, height)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// The profile catalog stays null until a file inspection first succeeds, and
// search runs before the profile pages' availability check.
static const nlohmann::json* CatalogueList(const char* game, const char* list) {
    const auto& catalog = ProfileUI::Catalogue();
    if (!catalog.is_object())
        return nullptr;
    const auto section = catalog.find(game);
    if (section == catalog.end() || !section->is_object())
        return nullptr;
    const auto rows = section->find(list);
    return rows != section->end() && rows->is_array() ? &*rows : nullptr;
}

// Canonical rules and techniques navigate to their owning editor. Native
// settings retain their real callbacks; shared CVars surface once (OoT wins).
void ComboMenu::DrawSearchResults(const std::string& query) {
    auto& model = ComboMenuModel::Get();
    model.EnsureLoaded();
    struct Source {
        const char* label;
        const GameMenu* game;
    };
    const Source sources[] = { { "Ocarina of Time", &model.Oot() }, { "Majora's Mask", &model.Mm() } };
    struct Hit {
        MenuSearchKind kind;
        int rank;
        std::string label, scope, page, origin;
        const GameplayRule* rule = nullptr;
        const GameMenu* game = nullptr;
        const CwWidget* widget = nullptr;
        int targetGame = -1;
        int targetSubpage = -1;
        std::string heading;    // the heading a setting or rule sits under: <Tab> / <Page> / <Heading> - OoT|MM
        std::string catalogKey; // an MM Randomizer option drawn from its catalog row (ProfileUI::DrawCatalogSetting)
    };
    std::vector<Hit> hits;
    auto addLink = [&](MenuSearchKind kind, const char* name, const char* keywords, const char* scope, const char* page,
                       const char* origin = "", int canonicalBonus = 0, int targetGame = -1, int targetSubpage = -1,
                       const char* heading = "") {
        int rank = MenuSearchRank(query, name, keywords, kind);
        if (!rank)
            return;
        rank += canonicalBonus;
        if (kind == MenuSearchKind::Page) {
            for (auto& hit : hits)
                if (hit.kind == kind && hit.scope == scope && hit.page == page) {
                    if (rank > hit.rank) {
                        hit.rank = rank;
                        hit.label = name;
                        hit.origin = origin;
                        hit.targetGame = targetGame;
                    }
                    return; // one link per shared feature, even with several native aliases
                }
        }
        hits.push_back({ kind, rank, name ? name : "", scope, page, origin, nullptr, nullptr, nullptr, targetGame,
                         targetSubpage, heading ? heading : "" });
    };
    for (const auto& page : kSharedPages) {
        addLink(MenuSearchKind::Page, page.name, page.keywords, page.scope, page.name, page.scopeLabel, 40);
    }
    for (int i = 0; i < SF_COUNT; ++i) {
        if (i == SF_OCARINA || !SharedFamilyByIndex(i).publicOption)
            continue; // private native features await runtime validation
        const auto& family = SharedFamilyByIndex(i);
        addLink(MenuSearchKind::Setting, family.label, family.tooltip, "randomizer", "Shared items", "Randomizer");
    }
    // OoTMM names; the earlier labels stay aliases.
    addLink(MenuSearchKind::Setting, "Goal",
            "victory condition triforce quest hunt ganon majora both final bosses either any final boss required total "
            "pieces triforce goal triforce pieces win",
            "randomizer", "Logic & access", "Randomizer");
    addLink(MenuSearchKind::Setting, "Starting world", "starting game OoT MM random world", "randomizer",
            "Logic & access", "Randomizer");
    // MM randomizer pages are native custom widgets; index their catalog rows instead.
    if (const auto* layout = CatalogueList("mm", "layout"))
        for (const auto& row : *layout) {
            if (!row.is_object())
                continue;
            const auto name = row.value("label", std::string()), page = row.value("page", std::string());
            if (name.empty() || name.rfind("##", 0) == 0)
                continue;
            auto keywords = row.value("tooltip", std::string()) + " " + row.value("heading", std::string()) + " " +
                            row.value("aliases", std::string()) + " MM Majora Mask randomizer";
            if (name == "Clocks as Items (MM)")
                keywords += " clock shuffle time day night half days six cycle progressive ascending descending";
            if (name == "Farore's Wind (MM)")
                keywords += " farore fw spell magic warp cross game items extension";
            if (name == "Din's Fire (MM)")
                keywords += " din dins spell magic fire cross game items extension";
            if (name == "Nayru's Love (MM)")
                keywords += " nayru nayrus spell magic barrier cross game items extension";
            if (name == "Iron Boots (MM)" || name == "Hover Boots (MM)" || name == "Goron Tunic (MM)" ||
                name == "Zora Tunic (MM)")
                keywords += " boots tunic gear equipment cross game items extension";
            const size_t before = hits.size();
            addLink(MenuSearchKind::Setting, name.c_str(), keywords.c_str(), "randomizer",
                    MenuRoute(false, "Rando", page).page.c_str(), "MM", 0, 1, RandomizerWidgetGroup(false, page, 0, ""),
                    row.value("heading", std::string()).c_str());
            if (hits.size() > before)
                hits.back().catalogKey = row.value("key", std::string());
        }
    for (const auto& row : Entrances::kEntranceSettings) {
        if (!row.visible(Entrances::ActiveCapabilities().Selected("")))
            continue;
        const size_t before = hits.size();
        addLink(MenuSearchKind::Setting, row.name, row.description, "randomizer", "Logic & access", "Randomizer", 0, -1,
                -1, "Entrances");
        if (hits.size() > before)
            hits.back().catalogKey = row.cvar;
    }
    addLink(MenuSearchKind::Setting, "Cross-Games Farore's Wind",
            "farore fw spell warp travel between oot mm cross game world", "randomizer", "Logic & access",
            "Randomizer");
    addLink(MenuSearchKind::Setting, "Cross-Games Age",
            "adult link age enter mm as adult majora mask between oot mm cross game world", "randomizer",
            "Logic & access", "Randomizer");
    addLink(MenuSearchKind::Setting, "Logic",
            "reachability guarantee all locations beatable only all checks reachable beatable seed", "randomizer",
            "Logic & access", "Randomizer");
    for (const auto& rule : kGameplayRules) {
        if (IsLegacyGameplayRule(rule.name))
            continue;
        // The rule's own label stays a search word when its pages show a shared one; the result keeps the game.
        const auto keywords = std::string(rule.help) +
                              (rule.runtimeOnly ? " enhancement quality of life " : " randomizer logic ") +
                              GameplayRuleSearchAliases(rule.name) + " " + rule.label;
        std::string label = GameplayRuleMenuLabel(rule.name, rule.label, !rule.runtimeOnly);
        for (const char* suffix : { " (OoT)", " (MM)" })
            if (std::strstr(rule.label, suffix) && label.find(suffix) == std::string::npos)
                label += suffix;
        const int rank = MenuSearchRank(query, label.c_str(), keywords.c_str(), MenuSearchKind::Rule);
        if (rank)
            hits.push_back({ MenuSearchKind::Rule, rank, label, rule.runtimeOnly ? "gameplay" : "randomizer",
                             rule.trickGoverned ? "Tricks & glitches" : GameplayRulePage(rule.name, !rule.runtimeOnly),
                             "", &rule, nullptr, nullptr, -1, -1,
                             rule.trickGoverned ? "" : GameplayRuleHeading(rule.name, !rule.runtimeOnly) });
    }
    for (const auto& trick : kMmTricks) {
        if (!trick.publicOption)
            continue;
        const auto keywords =
            std::string(trick.description) + " " + trick.area + " " + trick.tags + " technique trick MM Majora Mask";
        addLink(MenuSearchKind::Technique, trick.name, keywords.c_str(), "randomizer", "Tricks & glitches", "MM");
    }
    if (const auto* tricks = CatalogueList("oot", "tricks"))
        for (const auto& row : *tricks) {
            if (!row.is_object())
                continue;
            const auto name = row.value("label", std::string());
            const auto keywords = row.value("description", std::string()) + " " + row.value("area", std::string()) +
                                  " " + row.value("tags", std::string()) + " technique trick OoT";
            addLink(MenuSearchKind::Technique, name.c_str(), keywords.c_str(), "randomizer", "Tricks & glitches",
                    "OoT");
        }
    for (const auto& info : kRandomizerPages) {
        addLink(MenuSearchKind::Page, info.name, info.keywords, "randomizer", info.name, "Randomizer", 40);
    }
    std::unordered_set<std::string> seen;
    for (const auto& src : sources) {
        const GameMenu& game = *src.game;
        if (!game.loaded || !game.menu)
            continue;
        const CwMenu* m = game.menu;
        for (int s = 0; s < m->sectionCount; ++s) {
            const CwSection& sec = m->sections[s];
            for (int sb = 0; sb < sec.sidebarCount; ++sb) {
                const CwSidebar& side = sec.sidebars[sb];
                const auto pageRoute =
                    NativeSearchPageRoute(src.game == &model.Oot(), sec.sectionLabel ? sec.sectionLabel : "",
                                          side.sidebarName ? side.sidebarName : "");
                if (!pageRoute.scope.empty()) {
                    const bool oot = src.game == &model.Oot();
                    const auto keywords =
                        NativeSearchPageKeywords(sec.sectionLabel ? sec.sectionLabel : "",
                                                 side.sidebarName ? side.sidebarName : "", pageRoute, oot);
                    const auto label = NativeSearchPageLabel(oot, pageRoute, query);
                    const int target = NativeSearchPageTarget(oot, pageRoute, query);
                    const auto origin =
                        std::string(MenuScopeLabel(pageRoute.scope)) + (target == 1   ? " / Majora's Mask"
                                                                        : target == 0 ? " / Ocarina of Time"
                                                                                      : "");
                    addLink(MenuSearchKind::Page, label.c_str(), keywords.c_str(), pageRoute.scope.c_str(),
                            pageRoute.page.c_str(), origin.c_str(), 0, target);
                }
                const auto sidebarRoute = MenuRoute(src.game == &model.Oot(), sec.sectionLabel ? sec.sectionLabel : "",
                                                    side.sidebarName ? side.sidebarName : "");
                if (sidebarRoute.scope.empty())
                    continue;
                if (side.sidebarName && std::strcmp(side.sidebarName, "Check Tracker") == 0)
                    continue;
                std::string heading;
                int column = -1;
                for (int w = 0; w < side.widgetCount; ++w) {
                    const CwWidget& wd = side.widgets[w];
                    if (column != wd.column) {
                        column = wd.column;
                        heading.clear();
                    }
                    if (wd.kind == CW_SEPARATOR_TEXT)
                        heading = wd.name ? wd.name : "";
                    const auto route = MenuGroupRoute(
                        src.game == &model.Oot(), sec.sectionLabel ? sec.sectionLabel : "",
                        side.sidebarName ? side.sidebarName : "", wd.column, heading, wd.cvar ? wd.cvar : "");
                    if (route.scope.empty())
                        continue;
                    if (wd.cvar && (std::strstr(wd.cvar, "CheckTracker") || IsRetiredNativeRuleControl(wd.cvar)))
                        continue; // retired per-game controls
                    const std::string name = wd.name ? wd.name : "";
                    if (name == "Open App Files Folder" || name == "Cursor Always Visible" ||
                        name == "Allow background inputs" || name == "Search In Sidebar" ||
                        name == "Search Input Autofocus" || name == "Import Settings")
                        continue;
                    // Non-interactive / opted-out kinds never appear in search, nor pop-out buttons, whose pages
                    // are indexed themselves.
                    if (wd.kind == CW_SEPARATOR || wd.kind == CW_SEPARATOR_TEXT || wd.kind == CW_TEXT ||
                        wd.kind == CW_CUSTOM || wd.kind == CW_WINDOW_BUTTON || wd.hideInSearch)
                        continue;
                    const bool oot = src.game == &model.Oot();
                    // The seed row of Skip Child Stealth shows only while the selected file has it on (DrawSidebar).
                    if (route.scope == "randomizer" && wd.cvar &&
                        !std::strcmp(wd.cvar, "gRandoSettings.SkipChildStealth") && !ProfileUI::ReadInteger(wd.cvar, 0))
                        continue;
#ifdef __ANDROID__
                    if (name == "ImGui Menu Scaling")
                        continue; // Settings > Interface > Menu size replaces it on Android
#endif
                    // A row ranks by the label its page shows (OoTMM's name on the Randomizer tab, the shared
                    // label elsewhere); MenuRowKeywords keeps the native name a keyword.
                    const char* shown = route.scope == "randomizer" ? ProfileUI::OptionDisplayLabel(wd.cvar) : nullptr;
                    const auto label = shown ? std::string(shown) : MenuRowLabel(oot, wd.cvar, wd.name);
                    const auto keywords = std::string(wd.tooltip ? wd.tooltip : "") + " " + src.label +
                                          (oot ? " OoT " : " MM ") + MenuRowKeywords(oot, wd.cvar, wd.name);
                    const int rank = MenuSearchRank(query, label.c_str(), keywords.c_str(), MenuSearchKind::Setting);
                    if (!rank)
                        continue;
                    // Dedupe shared settings (same backing CVar) across both games. Widgets with no
                    // CVar (buttons/window toggles) drive distinct callbacks, so scope their key per
                    // game — otherwise two same-named buttons would collapse and one would vanish.
                    std::string cvar = wd.cvar ? wd.cvar : "";
                    std::string key = cvar.empty() ? (std::string(src.label) + "|" + (wd.name ? wd.name : ""))
                                                   : NormalizeSearch(cvar);
                    if (!seen.insert(NormalizeSearch(key)).second)
                        continue; // a shared setting already shown from OOT

                    const auto* placement =
                        FindMenuPlacement(oot, sec.sectionLabel ? sec.sectionLabel : "",
                                          side.sidebarName ? side.sidebarName : "", heading, cvar, name);
                    // OoT Randomizer rows sit under the page's fixed headings (ComboRandomizerLayout.h).
                    const char* fixed =
                        oot && route.scope == "randomizer" ? OotRandomizerHeading(route.page, cvar, heading) : nullptr;
                    hits.push_back({ MenuSearchKind::Setting, rank, label, route.scope, route.page, src.label, nullptr,
                                     &game, &wd, oot ? 0 : 1,
                                     route.scope == "randomizer"
                                         ? RandomizerWidgetGroup(oot, side.sidebarName ? side.sidebarName : "",
                                                                 wd.column, heading, cvar)
                                         : -1,
                                     fixed                             ? fixed
                                     : placement && placement->heading ? placement->heading
                                                                       : heading });
                }
            }
        }
    }
    if (hits.empty()) {
        ImGui::TextDisabled("No matching settings.");
        ImGui::TextWrapped("Try fewer words, a setting name, or a topic such as warp, screen fit or check tracker.");
        return;
    }
    std::stable_sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
        if (a.rank != b.rank)
            return a.rank > b.rank;
        const auto an = NormalizeMenuSearch(a.label), bn = NormalizeMenuSearch(b.label);
        return an != bn ? an < bn : a.origin < b.origin;
    });
    struct Group {
        MenuSearchKind kind;
        const char* title;
        int best = 0;
        int count = 0;
    };
    std::vector<Group> groups = { { MenuSearchKind::Rule, "Gameplay rules" },
                                  { MenuSearchKind::Setting, "Settings" },
                                  { MenuSearchKind::Technique, "Tricks & glitches" },
                                  { MenuSearchKind::Page, "Pages" } };
    for (auto& group : groups)
        for (const auto& hit : hits)
            if (hit.kind == group.kind) {
                group.best = std::max(group.best, hit.rank);
                ++group.count;
            }
    std::stable_sort(groups.begin(), groups.end(), [](const Group& a, const Group& b) { return a.best > b.best; });
    ImGui::TextWrapped("%d result%s for \"%s\"", static_cast<int>(hits.size()), hits.size() == 1 ? "" : "s",
                       query.c_str());
    const int cols = ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 60.0f ? 2 : 1;
    const float buttonHeight = ImGui::GetFrameHeight() * 1.25f;
    // Every Open button has the width of the longest label, so a result's actions line up.
    float openWidth = 0;
    for (const char* label : { "Open page", "Open technique", "Open Randomizer", "Open Enhancements" })
        openWidth = std::max(openWidth, ImGui::CalcTextSize(label).x);
    openWidth += ImGui::GetStyle().FramePadding.x * 4;
    const ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings;
    // One breadcrumb format: <Tab> / <Page> / <Heading> - OoT|MM.
    auto breadcrumb = [](const char* scope, const std::string& page, const std::string& heading, int game) {
        std::string text = std::string(MenuScopeLabel(scope)) + " / " + page;
        if (!heading.empty() && heading != page)
            text += " / " + heading;
        if (game == 0 || game == 1)
            text += game == 0 ? " - OoT" : " - MM";
        ImGui::TextColored(kBreadcrumbColor, "%s", text.c_str());
    };
    auto openButton = [&](const char* label) { return ImGui::Button(label, ImVec2(openWidth, buttonHeight)); };
    for (const auto& group : groups) {
        if (!group.count)
            continue;
        ImGui::PushID(static_cast<int>(group.kind));
        ImGui::Spacing();
        ImGui::SeparatorText((std::string(group.title) + " (" + std::to_string(group.count) + ")").c_str());
        bool navigate = false;
        if (ImGui::BeginTable("##RankedResults", group.kind == MenuSearchKind::Rule ? 1 : cols, flags)) {
            int id = 0;
            for (const auto& hit : hits) {
                if (hit.kind != group.kind)
                    continue;
                ImGui::TableNextColumn();
                ImGui::PushID(id++);
                // Every setting shows its control, one its page hides greyed out, so a result always names it.
                if (hit.widget) {
                    ImGui::PushID(hit.origin.c_str());
                    if (hit.scope == "randomizer")
                        ProfileUI::DrawWidget(*hit.widget, *hit.game, true);
                    else
                        RenderWidget(*hit.widget, *hit.game, 90 / cols, true);
                    ImGui::PopID();
                } else if (hit.catalogKey.empty() || !ProfileUI::DrawCatalogSetting(hit.catalogKey.c_str()))
                    ImGui::TextWrapped("%s", hit.label.c_str());
                ImGui::PushTextWrapPos(0.0f);
                if (hit.rule) {
                    const int ruleGame = IsSongTravelRule(hit.rule->name)       ? -1
                                         : std::strstr(hit.rule->label, "(MM)") ? 1
                                                                                : 0;
                    if (hit.rule->runtimeOnly) {
                        breadcrumb("gameplay", hit.page, hit.heading, ruleGame);
                        if (openButton("Open Enhancements")) {
                            OpenSearchDestination("gameplay", hit.page, hit.rule->name, nullptr,
                                                  std::strstr(hit.rule->label, "(MM)") ? 1 : 0);
                            navigate = true;
                        }
                    } else if (hit.rule->trickGoverned) {
                        breadcrumb("randomizer", "Tricks & glitches", "", -1);
                        if (openButton("Open technique")) {
                            OpenSearchDestination(hit.scope, hit.page, nullptr, hit.rule->label);
                            navigate = true;
                        }
                    } else {
                        breadcrumb("randomizer", hit.page, hit.heading, ruleGame);
                        const bool pairFits =
                            ImGui::GetContentRegionAvail().x >= openWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
                        if (openButton("Open Randomizer")) {
                            OpenSearchDestination("randomizer", GameplayRulePage(hit.rule->name, true), hit.rule->name,
                                                  nullptr, ruleGame);
                            navigate = true;
                        }
                        if (pairFits)
                            ImGui::SameLine();
                        if (openButton("Open Enhancements")) {
                            OpenSearchDestination("gameplay", GameplayRulePage(hit.rule->name, false), hit.rule->name,
                                                  nullptr, ruleGame);
                            navigate = true;
                        }
                    }
                } else {
                    // Techniques and catalog links name their game in the origin; native pages in the target.
                    const int game = hit.origin == "OoT" ? 0 : hit.origin == "MM" ? 1 : hit.targetGame;
                    breadcrumb(hit.scope.c_str(), hit.page, hit.heading, game);
                    // A setting is edited in place here; "Open page" shows it among its neighbours, in its world.
                    if (openButton(hit.kind == MenuSearchKind::Technique ? "Open technique" : "Open page")) {
                        if (hit.targetSubpage >= 0)
                            (hit.page == "Hints & traps" ? mHintSection : mPoolSection) = hit.targetSubpage;
                        OpenSearchDestination(hit.scope, hit.page, nullptr,
                                              hit.kind == MenuSearchKind::Technique ? hit.label.c_str() : nullptr,
                                              game);
                        navigate = true;
                    }
                }
                ImGui::PopTextWrapPos();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::PopID();
                if (navigate)
                    break;
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
        if (navigate)
            return;
    }
}

// A feature page can draw one or both engine sources without duplicating navigation.
namespace {

void DrawTrackerSharedPanel() {
    ComboTrackerUI::DrawItemSettings();
    ImGui::SeparatorText("Personal notes");
    bool notes = ComboNotes::WindowShown();
    ComboRando::ComboMenu_PushCheckbox(ComboRando::ComboMenu_ThemeColor());
    if (ImGui::Checkbox("Show Personal Notes window", &notes))
        ComboNotes::SetWindowShown(notes);
    ComboRando::ComboMenu_PopCheckbox();
}

// One check tracker and one settings model for both games.
void DrawCheckTrackerSharedPanel() {
    ComboRando::DrawUnifiedCheckTrackerSettings();
}

// Helper: seed a fixed char buffer from a string CVar for ImGui::InputText.
static void AnchorLoadStr(char* buf, size_t n, const char* cvar, const char* dflt) {
    const char* cur = CVarGetString(cvar, dflt);
    std::strncpy(buf, cur ? cur : "", n - 1);
    buf[n - 1] = '\0';
}

// Persist CVar writes to disk next frame (mirrors soh's Anchor menu, which saves after every edit).
static void AnchorSaveCVars() {
    if (auto ctx = Ship::Context::GetRawInstance(); ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui()) {
        ctx->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
}

// Combo Anchor panel: connection settings, owner-only room admin, and a toggle for the room window.
// Writes the shared gRemote.Anchor.* CVars and drives the games via the SOH_/MM_Anchor_* exports.
void DrawNetworkSharedPanel() {
    const ImVec4 theme = ComboRando::ComboMenu_ThemeColor();
    ResolveAnchorResyncSyms();

    int state = sSohAnchorGetConnState ? sSohAnchorGetConnState() : 0;
    bool enabled = (state & 1) != 0;
    bool connected = (state & 2) != 0;

    // Connection in the first cell and room admin in the second: side by side where they fit, stacked on a phone.
    const ImGuiTableFlags anchorTableFlags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings;
    if (!ImGui::BeginTable("##anchorcols", MenuColumns(), anchorTableFlags)) {
        return;
    }
    ImGui::TableNextColumn();

    // --- Connection settings (disabled while enabled, like soh) ---
    ImGui::SeparatorText("Connection Settings");
    MenuTooltip("Set a host, room and name, then Enable. Shared progress requires Combo Ship 0.1.15 or newer, the same "
                "seed and the same team for every player.");
    ImGui::BeginDisabled(enabled);

    char host[256], name[128], roomId[128], teamId[128];
    AnchorLoadStr(host, sizeof(host), AnchorCVar::Host, "anchor.hm64.org");
    AnchorLoadStr(name, sizeof(name), AnchorCVar::Name, "");
    AnchorLoadStr(roomId, sizeof(roomId), AnchorCVar::RoomId, "");
    AnchorLoadStr(teamId, sizeof(teamId), AnchorCVar::TeamId, "default");
    int port = CVarGetInteger(AnchorCVar::Port, 43383);

    ImGui::Text("Host");
    ComboRando::ComboMenu_PushInput(theme);
    if (ImGui::InputText("##AnchorHost", host, sizeof(host))) {
        CVarSetString(AnchorCVar::Host, host);
        AnchorSaveCVars();
    }
    ImGui::Text("Port");
    if (ImGui::InputInt("##AnchorPort", &port)) {
        if (port < 1025)
            port = 1025;
        if (port > 65534)
            port = 65534;
        CVarSetInteger(AnchorCVar::Port, port);
        AnchorSaveCVars();
    }
    ImGui::Text("Name");
    if (ImGui::InputText("##AnchorName", name, sizeof(name))) {
        CVarSetString(AnchorCVar::Name, name);
        AnchorSaveCVars();
    }
    ComboRando::ComboMenu_PopInput();

    // Color: shared gRemote.Anchor.Color.Value (Color type; MM reads it as Color24). RGB only, matching
    // soh's picker default {100,255,100}.
    Color_RGBA8 col = CVarGetColor(AnchorCVar::ColorValue, Color_RGBA8{ 100, 255, 100, 255 });
    float rgb[3] = { col.r / 255.0f, col.g / 255.0f, col.b / 255.0f };
    ImGui::Text("Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit3("##AnchorColor", rgb, ImGuiColorEditFlags_NoInputs)) {
        col.r = (uint8_t)(rgb[0] * 255.0f + 0.5f);
        col.g = (uint8_t)(rgb[1] * 255.0f + 0.5f);
        col.b = (uint8_t)(rgb[2] * 255.0f + 0.5f);
        col.a = 255;
        CVarSetColor(AnchorCVar::ColorValue, col);
        AnchorSaveCVars();
    }

    ComboRando::ComboMenu_PushInput(theme);
    ImGui::Text("Room ID");
    if (ImGui::InputText("##AnchorRoomId", roomId, sizeof(roomId))) {
        CVarSetString(AnchorCVar::RoomId, roomId);
        AnchorSaveCVars();
    }
    ImGui::Text("Team ID (Items & Flags Shared)");
    if (ImGui::InputText("##AnchorTeamId", teamId, sizeof(teamId))) {
        CVarSetString(AnchorCVar::TeamId, teamId);
        AnchorSaveCVars();
    }
    ComboRando::ComboMenu_PopInput();

    ImGui::Spacing();
    ComboRando::ComboMenu_PushButton(theme);
    if (ImGui::Button("Restore Defaults", ImVec2(ImGui::GetContentRegionAvail().x / 2, 0)))
        ImGui::OpenPopup("Restore Anchor defaults?");
    ImGui::SameLine();
    if (ImGui::Button("Global Room", ImVec2(-1.0f, 0.0f))) {
        CVarSetString(AnchorCVar::Host, "anchor.hm64.org");
        CVarSetInteger(AnchorCVar::Port, 43383);
        CVarSetString(AnchorCVar::TeamId, "default");
        CVarSetString(AnchorCVar::RoomId, "soh-global");
        AnchorSaveCVars();
    }
    ComboRando::ComboMenu_PopButton();
    ImGui::SetItemTooltip("Always-online public room so you don't have to experience Hyrule alone. "
                          "PVP and syncing are disabled.");
    ImGui::EndDisabled();
    if (MenuConfirm("Restore Anchor defaults?", "Host, port, team, room and name return to their defaults.",
                    "Restore defaults")) {
        CVarSetString(AnchorCVar::Host, "anchor.hm64.org");
        CVarSetInteger(AnchorCVar::Port, 43383);
        CVarSetString(AnchorCVar::TeamId, "default");
        CVarSetString(AnchorCVar::RoomId, "");
        CVarSetString(AnchorCVar::Name, "");
        AnchorSaveCVars();
    }

    // --- Enable/Disable (gated by form validity, mirroring soh's isFormValid) ---
    ImGui::Spacing();
    bool formValid = host[0] != '\0' && port > 1024 && port < 65535 && roomId[0] != '\0' && name[0] != '\0';
    ImGui::BeginDisabled(!formValid || sSohAnchorSetEnabled == nullptr);
    const ImVec4 red = { 0.66f, 0.13f, 0.13f, 1.0f };
    const ImVec4 green = { 0.15f, 0.55f, 0.20f, 1.0f };
    ComboRando::ComboMenu_PushButton(enabled ? red : green);
    if (ImGui::Button(enabled ? "Disable" : "Enable", ImVec2(-1.0f, 0.0f))) {
        if (sSohAnchorSetEnabled)
            sSohAnchorSetEnabled(enabled ? 0 : 1);
    }
    ComboRando::ComboMenu_PopButton();
    ImGui::EndDisabled();

    // --- Status line ---
    if (enabled) {
        if (connected) {
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Connected");
        } else {
            ImGui::TextDisabled("Connecting...");
        }
    }

    // --- Team sync + minimap (only once Anchor is enabled) ---
    if (enabled) {
        ImGui::SeparatorText("Team Sync");
        ComboRando::ComboMenu_PushButton(theme);
        if (ImGui::Button("Resync team state", ImVec2(-1.0f, 0.0f))) {
            if (sSohRequestResync)
                sSohRequestResync();
            if (sMmRequestResync)
                sMmRequestResync();
        }
        ComboRando::ComboMenu_PopButton();
        MenuTooltip(
            "Pull the latest progress for both games if you are missing items or flags a teammate has collected.");

        bool showMinimap = CVarGetInteger(AnchorCVar::ShowOnMinimap, 1) != 0;
        ComboRando::ComboMenu_PushCheckbox(theme);
        if (ImGui::Checkbox("Show Other Players on Minimap", &showMinimap)) {
            CVarSetInteger(AnchorCVar::ShowOnMinimap, showMinimap ? 1 : 0);
            AnchorSaveCVars();
        }
        ComboRando::ComboMenu_PopCheckbox();
    }

    // --- Room window: toggle the combo-native floating roster window (only once Anchor is enabled) ---
    if (enabled) {
        ImGui::SeparatorText("Room");
        bool roomOpen = CVarGetInteger("gCombo.Anchor.RoomWindow", 0) != 0;
        ComboRando::ComboMenu_PushButton(theme);
        if (ImGui::Button(roomOpen ? "Hide Room Window" : "Show Room Window", ImVec2(-1.0f, 0.0f))) {
            roomOpen = !roomOpen;
            CVarSetInteger("gCombo.Anchor.RoomWindow", roomOpen ? 1 : 0);
            AnchorSaveCVars();
            if (auto ctx = Ship::Context::GetRawInstance(); ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui()) {
                if (auto win = ctx->GetWindow()->GetGui()->GetGuiWindow("Anchor Room")) {
                    if (roomOpen)
                        win->Show();
                    else
                        win->Hide();
                }
            }
        }
        ComboRando::ComboMenu_PopButton();
        MenuTooltip("Shows every teammate's game and area, with same-game teleport.");
    }

    // --- Room settings (owner-only, non-global) — second cell ---
    ImGui::TableNextColumn();
    // ownerInfo bit0=isOwner, bit1=isGlobalRoom; 0 unless connected. Mirrors soh's AnchorAdminMenu gate.
    int ownerInfo = sSohAnchorGetOwnerInfo ? sSohAnchorGetOwnerInfo() : 0;
    bool isOwner = (ownerInfo & 1) != 0;
    bool isGlobalRoom = (ownerInfo & 2) != 0;
    if (connected && isOwner && !isGlobalRoom) {
        ImGui::SeparatorText("Room Settings (Admin Only)");

        ComboRando::ComboMenu_PushButton(theme);
        if (ImGui::Button("Clear All Team State", ImVec2(-1.0f, 0.0f)))
            ImGui::OpenPopup("Clear all team state?");
        ComboRando::ComboMenu_PopButton();
        if (MenuConfirm("Clear all team state?",
                        "Every teammate's shared items and flags in this room are cleared. This cannot be undone.",
                        "Clear team state") &&
            sSohAnchorClearTeamState)
            sSohAnchorClearTeamState();

        auto sendRoomState = [&]() {
            if (sSohAnchorSendRoomState)
                sSohAnchorSendRoomState();
        };

        const char* kPvpModes[] = { "Off", "On", "On + Friendly Fire" };
        const char* kShowLocationsModes[] = { "None", "Team Only", "All" };
        const char* kTeleportModes[] = { "None", "Team Only", "All" };

        ComboRando::ComboMenu_PushCombobox(theme);
        int pvp = CVarGetInteger(AnchorCVar::PvpMode, 1);
        if (ImGui::Combo("PvP Mode", &pvp, kPvpModes, 3)) {
            CVarSetInteger(AnchorCVar::PvpMode, pvp);
            AnchorSaveCVars();
            sendRoomState();
        }
        int showLoc = CVarGetInteger(AnchorCVar::ShowLocationsMode, 1);
        if (ImGui::Combo("Show Locations For", &showLoc, kShowLocationsModes, 3)) {
            CVarSetInteger(AnchorCVar::ShowLocationsMode, showLoc);
            AnchorSaveCVars();
            sendRoomState();
        }
        int teleport = CVarGetInteger(AnchorCVar::TeleportMode, 1);
        if (ImGui::Combo("Allow Teleporting To", &teleport, kTeleportModes, 3)) {
            CVarSetInteger(AnchorCVar::TeleportMode, teleport);
            AnchorSaveCVars();
            sendRoomState();
        }
        ComboRando::ComboMenu_PopCombobox();

        bool syncItems = CVarGetInteger(AnchorCVar::SyncItemsAndFlags, 1) != 0;
        ComboRando::ComboMenu_PushCheckbox(theme);
        if (ImGui::Checkbox("Sync Items & Flags", &syncItems)) {
            CVarSetInteger(AnchorCVar::SyncItemsAndFlags, syncItems ? 1 : 0);
            AnchorSaveCVars();
            sendRoomState();
        }
        ComboRando::ComboMenu_PopCheckbox();
    }

    ImGui::EndTable();
}

// Build the combined item picker list from both games' static-data dumps (items[] = full item table
// with friendly name + advancement). Each game's items get its own "(OOT)"/"(MM)" display tag.
void PlandoBuildItems() {
    sPlando.items.clear();
    auto add = [&](const std::string& dump, ComboRando::GameId g, const char* suf) {
        try {
            auto d = nlohmann::json::parse(dump);
            std::unordered_set<std::string> seen;
            for (auto& it : d.value("items", nlohmann::json::array())) {
                std::string n = it.value("name", std::string{});
                if (n.empty() || !seen.insert(n).second)
                    continue;
                if (g == ComboRando::GAME_OOT && n == "Triforce")
                    continue; // OOT's win item: placing it would roll credits outside the combo goal
                sPlando.items.push_back({ n, g, it.value("advancement", true), n + suf });
            }
        } catch (...) {}
    };
    add(sPlando.sohDump, ComboRando::GAME_OOT, " (OOT)");
    add(sPlando.mmDump, ComboRando::GAME_MM, " (MM)");
}

// Load the current pending/slot consolidated spoiler into the editable model (the #73 fix — the
// vendored OOT plando can't parse the consolidated schema). Flattens oot/mm.placements + foreign[]
// into the shape ParseSpoilerPlacements expects, resolving foreign checks to their real cross-game item.
void PlandoLoad() {
    ResolvePlandoSyms();
    sPlando.loaded = false;
    sPlando.rows.clear();
    // Load the spoiler picked in the dropdown; else the active slot's baked rando (combo.rando in its
    // .combosav container), else the pending generated seed.
    sPlando.loadedJson.clear();
    std::string srcLabel;
    auto readFile = [](const std::filesystem::path& p) -> std::string {
        std::ifstream in(p);
        if (!in.is_open())
            return {};
        std::stringstream ss;
        ss << in.rdbuf();
        return ss.str();
    };
    if (sPlando.spoilerSel >= 0 && sPlando.spoilerSel < (int)sPlando.spoilerPaths.size()) {
        sPlando.loadedJson = readFile(sPlando.spoilerPaths[sPlando.spoilerSel]);
        srcLabel = std::filesystem::path(sPlando.spoilerPaths[sPlando.spoilerSel]).filename().string();
    } else {
        int slot = ComboTracker::OotActiveSlot();
        if (slot >= 0) {
            try {
                auto cj = nlohmann::json::parse(
                    readFile(std::filesystem::path("Save") / ("file" + std::to_string(slot + 1) + ".combosav")));
                auto r = cj.value("combo", nlohmann::json::object()).value("rando", nlohmann::json());
                if (!r.is_null()) {
                    sPlando.loadedJson = r.dump();
                    srcLabel = "file" + std::to_string(slot + 1) + ".combosav";
                }
            } catch (...) {}
        }
        if (sPlando.loadedJson.empty()) {
            // The launcher remembers the newest generated spoiler here (soh owns the config).
            std::filesystem::path remembered = CVarGetString("gGeneral.ComboSpoiler", "");
            if (!remembered.empty()) {
                sPlando.loadedJson = readFile(remembered);
                srcLabel = remembered.filename().string();
            }
        }
    }
    if (sPlando.loadedJson.empty()) {
        sPlando.status = "No generated seed found to load.";
        sPlando.statusError = true;
        return;
    }
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(sPlando.loadedJson);
    } catch (...) {
        sPlando.status = "Failed to parse the spoiler file.";
        sPlando.statusError = true;
        return;
    }
    try {
        ComboRando::RequireSupportedSeedDocument(j, ComboRando::Entrances::ActiveCapabilities());
    } catch (const std::exception& error) {
        sPlando.status = error.what();
        sPlando.statusError = true;
        return;
    }
    sPlando.sohDump = sSohDump ? sSohDump() : "";
    sPlando.mmDump = sMmDump ? sMmDump() : "";
    PlandoBuildItems();

    nlohmann::json flat;
    flat["oot"] = j.value("oot", nlohmann::json::object()).value("placements", nlohmann::json::object());
    flat["mm"] = j.value("mm", nlohmann::json::object()).value("placements", nlohmann::json::object());
    flat["foreign"] = j.value("foreign", nlohmann::json::array());
    sPlando.rows = ComboRando::ParseSpoilerPlacements(flat.dump(), sPlando.sohDump, sPlando.mmDump);
    std::sort(sPlando.rows.begin(), sPlando.rows.end(), [](const auto& a, const auto& b) {
        if (a.checkGame != b.checkGame)
            return a.checkGame < b.checkGame;
        return a.check < b.check;
    });
    sPlando.loaded = true;
    sPlando.statusError = false;
    sPlando.status = "Loaded " + std::to_string(sPlando.rows.size()) + " placements from " + srcLabel;
}

// Serialize the edited model back to the consolidated schema and play it verbatim. Preserves the
// original file and replaces only oot/mm.placements + foreign[], mirroring the generator's emit
// (SuffixCrossGameItems on native collisions + BuildForeignArray for cross-game entries).
void PlandoSavePlay() {
    ResolvePlandoSyms();
    const int targetSlot = PlandoImportTarget();
    if (targetSlot < 0) {
        sPlando.status = "Highlight File 1, 2 or 3 on the file-select screen before saving.";
        sPlando.statusError = true;
        return;
    }
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(sPlando.loadedJson);
    } catch (...) {
        sPlando.status = "Internal parse error.";
        sPlando.statusError = true;
        return;
    }

    nlohmann::json ootPl = nlohmann::json::object();
    nlohmann::json mmPl = nlohmann::json::object();
    nlohmann::json foreignRaw = nlohmann::json::array();
    // Trap disguises and item categories can't be recomputed here; carry them over for rows whose
    // item is unchanged.
    std::unordered_map<std::string, nlohmann::json> priorForeign;
    for (const auto& fm : j.value("foreign", nlohmann::json::array())) {
        priorForeign[fm.value("checkGame", "") + "|" + fm.value("checkName", "")] = fm;
    }
    for (const auto& r : sPlando.rows) {
        // The check's own game stores the REAL foreign item's bare name; the sentinel is injected at
        // apply time by Combo_OnReloadRequest, never written here.
        (r.checkGame == ComboRando::GAME_OOT ? ootPl : mmPl)[r.check] = r.item;
        if (r.checkGame != r.itemGame) {
            const std::string cg = r.checkGame == ComboRando::GAME_OOT ? "oot" : "mm";
            nlohmann::json marker = { { "checkGame", cg },
                                      { "checkName", r.check },
                                      { "itemGame", r.itemGame == ComboRando::GAME_OOT ? "oot" : "mm" },
                                      { "itemName", r.item },
                                      { "displayName", r.item }, // BuildForeignArray appends the game suffix
                                      { "advancement", r.advancement } };
            auto pf = priorForeign.find(cg + "|" + r.check);
            if (pf != priorForeign.end() && pf->second.value("itemName", "") == r.item) {
                for (const char* k : { "fakeItemName", "fakeDisplayName", "fakeTrickName", "category" })
                    if (pf->second.contains(k))
                        marker[k] = pf->second[k];
            }
            foreignRaw.push_back(std::move(marker));
        }
    }
    // Shared Items: the loaded seed's own effective mask — plando doesn't change which families are
    // shared, only where items land.
    const uint32_t plandoSharedMask = ComboRando::SharedMaskFromKeys(j.value("sharedItems", nlohmann::json::array()));
    // Native cross-game name collisions get their own-game suffix; foreign checks are skipped (their
    // real item travels in foreign[]). Exactly the generator's write path.
    ComboRando::SuffixCrossGameItems(ootPl, mmPl, foreignRaw, sPlando.sohDump, sPlando.mmDump,
                                     ComboRando::SharedUntaggedNames(plandoSharedMask));
    nlohmann::json foreign = ComboRando::BuildForeignArray(foreignRaw, plandoSharedMask);

    j["oot"]["placements"] = ootPl;
    j["mm"]["placements"] = mmPl;
    j["foreign"] = foreign;

    std::error_code ec;
    std::filesystem::create_directories(ComboRando::ConsolidatedDir(), ec);
    // Its own file: writing the source seed's name would overwrite that seed with the edited copy.
    auto path = j.contains("file_hash") ? ComboRando::ComboSpoilerPath(j["file_hash"], "Combo-Plando")
                                        : ComboRando::ConsolidatedDir() / "Combo-Plando.json";
    {
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) {
            sPlando.status = "Failed to write the plandomizer file.";
            sPlando.statusError = true;
            return;
        }
        out << j.dump(2);
    }
    sPlando.loadedJson = j.dump(2); // keep in sync for a follow-up edit
    if (!sRequestReload) {
        sPlando.status = "Reload export unavailable (soh.dll not loaded).";
        sPlando.statusError = true;
        return;
    }
    int ok = sRequestReload(path.string().c_str());
    if (ok) {
        sPlando.status = "Saved for File " + std::to_string(targetSlot + 1) + ". Start that file to play it.";
        sPlando.statusError = false;
    } else {
        sPlando.status = "Reload rejected (be on the file-select screen; no generation running).";
        sPlando.statusError = true;
    }
}

// Combo > Plandomizer: load a generated combo seed, edit item placements (native OR cross-game),
// then Save & Play. Placements-only MVP; prices/settings/tricks/hints are preserved read-only.
void DrawComboPlandoPanel() {
    const ImVec4 theme = ComboRando::ComboMenu_ThemeColor();
    ResolvePlandoSyms();
    ResolveComboGenSyms(); // sIsOnFileSelect gates Save & Play (reload mutates save state)

    if (sPlando.spoilerPaths.empty())
        PlandoRefreshSpoilerList(); // lazy first fill; the Refresh button re-scans on demand

    // Spoiler picker: every *.json in the Randomizer folder is loadable, not just Last-Generated.
    const char* preview = (sPlando.spoilerSel >= 0 && sPlando.spoilerSel < (int)sPlando.spoilerNames.size())
                              ? sPlando.spoilerNames[sPlando.spoilerSel].c_str()
                              : "(no spoilers found)";
    ImGui::SetNextItemWidth(360.0f);
    ComboRando::ComboMenu_PushCombobox(theme);
    if (ImGui::BeginCombo("Seed", preview)) {
        for (int i = 0; i < (int)sPlando.spoilerNames.size(); i++) {
            if (ImGui::Selectable(sPlando.spoilerNames[i].c_str(), i == sPlando.spoilerSel))
                sPlando.spoilerSel = i;
        }
        ImGui::EndCombo();
    }
    ComboRando::ComboMenu_PopCombobox();
    MenuTooltip("Load a generated spoiler from the Randomizer folder, edit rewards in either game, then Save. Use Save "
                "& Play at file select.");
    ImGui::SameLine();
    ComboRando::ComboMenu_PushButton(theme);
    if (ImGui::Button("Refresh"))
        PlandoRefreshSpoilerList();
    ImGui::SameLine();
    if (ImGui::Button("Load"))
        PlandoLoad();
    ComboRando::ComboMenu_PopButton();

    if (sPlando.loaded) {
        ImGui::SameLine();
        const bool onFileSelect = sIsOnFileSelect && sIsOnFileSelect();
        const int targetSlot = PlandoImportTarget();
        const bool canPlay = sRequestReload && onFileSelect && targetSlot >= 0;
        const std::string saveLabel = targetSlot >= 0 ? "Save for File " + std::to_string(targetSlot + 1) : "Save";
        if (!canPlay)
            ImGui::BeginDisabled();
        ComboRando::ComboMenu_PushButton(theme);
        if (ImGui::Button(saveLabel.c_str()))
            PlandoSavePlay();
        ComboRando::ComboMenu_PopButton();
        ComboRando::MenuTooltip("Imports into the file highlighted on the file-select screen.");
        if (!canPlay)
            ImGui::EndDisabled();
        if (sRequestReload && !onFileSelect) {
            ImGui::SameLine();
            ImGui::TextDisabled("(available on the file-select screen)");
        }
    }

    if (!sPlando.status.empty()) {
        ImVec4 c = sPlando.statusError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
        ImGui::TextColored(c, "%s", sPlando.status.c_str());
    }
    if (!sPlando.loaded)
        return;

    ImGui::Separator();
    ImGui::SetNextItemWidth(320.0f);
    ComboRando::ComboMenu_PushInput(theme);
    ImGui::InputTextWithHint("##plandofilter", "Filter by check or item name...", sPlando.filter,
                             sizeof(sPlando.filter));
    ComboRando::ComboMenu_PopInput();

    const std::string q = NormalizeSearch(sPlando.filter);

    ImGui::BeginChild("##plandotable", ImVec2(0, 0));
    const ImGuiTableFlags tf = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY |
                               ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##plandorows", 3, tf)) {
        ImGui::TableSetupColumn("Game", ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("Check", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (int i = 0; i < (int)sPlando.rows.size(); ++i) {
            auto& r = sPlando.rows[i];
            if (!q.empty() && NormalizeSearch(r.check + r.item).find(q) == std::string::npos)
                continue;
            const bool cross = r.checkGame != r.itemGame;
            const char* itag = r.itemGame == ComboRando::GAME_OOT ? " (OOT)" : " (MM)";
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(r.checkGame == ComboRando::GAME_OOT ? "OOT" : "MM");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(r.check.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::PushID(i);
            std::string label = r.item + itag;
            if (cross) // highlight cross-game placements (the harness's whole point)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.4f, 1.0f));
            ComboRando::ComboMenu_PushButton(theme);
            if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 0.0f))) {
                sPlando.pickerFilter[0] = '\0';
                ImGui::OpenPopup("##itempicker");
            }
            ComboRando::ComboMenu_PopButton();
            if (cross)
                ImGui::PopStyleColor();
            if (ImGui::BeginPopup("##itempicker")) {
                ImGui::SetNextItemWidth(300.0f);
                ImGui::InputTextWithHint("##pickfilter", "Search items...", sPlando.pickerFilter,
                                         sizeof(sPlando.pickerFilter));
                const std::string pq = NormalizeSearch(sPlando.pickerFilter);
                ImGui::BeginChild("##pickerlist", ImVec2(340.0f, 380.0f));
                for (const auto& pit : sPlando.items) {
                    if (!pq.empty() && NormalizeSearch(pit.name).find(pq) == std::string::npos)
                        continue;
                    if (ImGui::Selectable(pit.display.c_str())) {
                        r.item = pit.name;
                        r.itemGame = pit.game;
                        r.advancement = pit.advancement;
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}
} // namespace

// MM is foreground only inside a loaded file, but its snapshot reports randomizer runs only, so
// any other MM run takes the combo-wide save slot.
int LoadedRunSlot() {
    const int game = ComboUI::GetForegroundGame() == 1 ? 1 : 0;
    ComboCheckInfo info{ 0, -1, -1 };
    if (const auto snapshot = ComboChecks::Snapshot(game))
        snapshot(&info, nullptr, nullptr);
    return info.loaded ? info.slot : game ? ComboTracker::OotActiveSlot() : -1;
}

// The MM spells where the device cannot cast them, and the MM boots and tunics where it cannot run them or while Swim
// is shuffled (RO_SHUFFLE_SWIM = RO_GENERIC_YES, 1), with 2S2H's own disabled tooltips.
const char* MmPoolOptionUnavailable(const char* option, bool swimShuffled) {
    static int (*spellReady)(int) = nullptr;
    static int (*gearAllowed)(int) = nullptr;
    if (!spellReady)
        spellReady = (int (*)(int))Combo_ResolveSym("2ship", "MM_SpellOptionReady");
    if (!gearAllowed)
        gearAllowed = (int (*)(int))Combo_ResolveSym("2ship", "MM_GearOptionsAllowed");
    const std::string name = option ? option : "";
    if (name == "RO_MM_SPELL_FIRE" || name == "RO_MM_SPELL_LOVE") // Spells.cpp: 2 Din's Fire, 1 Nayru's Love
        return spellReady && spellReady(name == "RO_MM_SPELL_FIRE" ? 2 : 1)
                   ? nullptr
                   : "Disabled because this device could not prepare the spell. New seeds leave it out.";
    if (name != "RO_MM_BOOTS_IRON" && name != "RO_MM_BOOTS_HOVER" && name != "RO_MM_TUNIC_GORON" &&
        name != "RO_MM_TUNIC_ZORA")
        return nullptr;
    if (gearAllowed && gearAllowed(swimShuffled))
        return nullptr;
    return gearAllowed && gearAllowed(0)
               ? "Disabled while Swim is shuffled. New seeds leave the MM boots and tunics out."
               : "Disabled because this device cannot run the MM boots and tunics yet. New seeds leave them out.";
}

namespace {
const CwWidget* FindPreference(const GameMenu& game, const char* cvar) {
    if (!game.loaded || !game.menu)
        return nullptr;
    for (int s = 0; s < game.menu->sectionCount; ++s) {
        const auto& section = game.menu->sections[s];
        for (int b = 0; b < section.sidebarCount; ++b) {
            const auto& side = section.sidebars[b];
            for (int w = 0; w < side.widgetCount; ++w)
                if (side.widgets[w].cvar && std::strcmp(side.widgets[w].cvar, cvar) == 0)
                    return &side.widgets[w];
        }
    }
    return nullptr;
}
bool DrawRuleCheckbox(const char* label, bool* enabled) {
    const float need = ImGui::CalcTextSize(label).x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
    if (need <= ImGui::GetContentRegionAvail().x)
        return ImGui::Checkbox(label, enabled);
    bool changed = ImGui::Checkbox("##RuleToggle", enabled);
    ImGui::SameLine();
    ImGui::TextWrapped("%s", label);
    const ImVec2 labelStart = ImGui::GetItemRectMin(), labelEnd = ImGui::GetItemRectMax();
    const ImVec2 afterLabel = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(labelStart);
    // Native button behavior toggles on release and cancels a drag released
    // elsewhere. A disabled seed-generation control also blocks this target.
    if (ImGui::InvisibleButton("##RuleLabel", ImVec2(labelEnd.x - labelStart.x, labelEnd.y - labelStart.y))) {
        *enabled = !*enabled;
        changed = true;
    }
    ImGui::SetCursorScreenPos(afterLabel);
    return changed;
}
// One gameplay rule: its logic half on Randomizer (the selected file's next seed), its in-game half
// on Enhancements, locked while the seed requires it. Drawn inside its catalogue heading or by
// DrawGameplayRules.
void DrawGameplayRuleRow(const GameplayRule& rule, bool randomizer, ComboRando::MenuRuleFocus* focus) {
    auto& model = ComboMenuModel::Get();
    // Randomizer edits the selected file's next seed. During a run, Enhancements change the running
    // game: its live rules, which the save keeps per run, under the logic locks restored from its
    // seed. With no run loaded, Enhancements configure the selected file's next seed instead.
    const bool nextSeed = !randomizer && LoadedRunSlot() < 0 && ProfileUI::Active();
    auto apply = [&](const char* name, int value) {
        const auto* rule = FindGameplayRule(name);
        if (randomizer || (nextSeed && rule && !rule->runtimeOnly)) {
            ProfileUI::WriteInteger(name, value);
            return;
        }
        CVarSetInteger(name, value);
        static auto write = (void (*)(const char*, int32_t))Combo_ResolveSym("soh", "SOH_WriteGameplayRule");
        if (write)
            write(name, value);
        WarnAboutRuleChange(name);
        if (model.Oot().applyCVarChange)
            model.Oot().applyCVarChange(name);
        if (model.Mm().applyCVarChange)
            model.Mm().applyCVarChange(name);
    };
    auto read = [randomizer, nextSeed](const char* name, int fallback) {
        const auto* rule = FindGameplayRule(name);
        return randomizer || (nextSeed && rule && !rule->runtimeOnly) ? ProfileUI::ReadInteger(name, fallback)
                                                                      : CVarGetInteger(name, fallback);
    };
    const auto theme = ComboMenu_ThemeColor();
    ImGui::PushID(rule.name);
    ImGui::BeginGroup();
    const bool focused = focus && focus->Matches(rule.name);
    if (focused)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.65f, .8f, 1.0f, 1.0f));
    std::string label = GameplayRuleMenuLabel(rule.name, rule.label, randomizer);
    for (const auto* suffix : { " (OoT)", " (MM)" }) {
        const auto pos = label.find(suffix);
        if (pos != std::string::npos)
            label.erase(pos);
    }
    int value = ReadGameplayRule(rule, randomizer, read);
    const int required = !randomizer && !rule.runtimeOnly ? ReadGameplayRule(rule, true, read) : 0;
    if (required)
        value = required;
    ImGui::BeginDisabled(required || (nextSeed && !rule.runtimeOnly && !ProfileUI::Editable()));
    bool changed = false;
    if (rule.maximum == 1) {
        bool enabled = value != 0;
        ComboMenu_PushCheckbox(theme);
        changed = DrawRuleCheckbox(label.c_str(), &enabled);
        ComboMenu_PopCheckbox();
        value = enabled;
    } else if (rule.minimum > 0) {
        if (value < rule.minimum)
            value = rule.fallback;
        changed = ComboMenu_SliderRowInt(label.c_str(), &value, rule.minimum, rule.maximum, 1, rule.fallback, "%d");
    } else {
        const char* gibdo[] = { "Vanilla", "MM3D", "No trade" };
        const char* dog[] = { "Off", "With Mask of Truth", "Always" };
        const char* sticks[] = { "Normal", "Unbreakable", "Always burning" };
        const char* age[] = { "Off", "Ocarina of Time", "Ocarina of Time + Master Sword", "Either ocarina",
                              "Either ocarina + Master Sword" };
        const char** modes = rule.maximum == 4                                                               ? age
                             : !std::strcmp(rule.name, "gCheats.DekuStick")                                  ? sticks
                             : !std::strcmp(rule.name, "gEnhancements.DifficultyOptions.GibdoTradeSequence") ? gibdo
                                                                                                             : dog;
        ImGui::TextWrapped("%s", label.c_str());
        ImGui::SetNextItemWidth(-FLT_MIN);
        ComboMenu_PushCombobox(theme);
        changed = ImGui::Combo("##Ability", &value, modes, rule.maximum + 1);
        ComboMenu_PopCombobox();
    }
    if (focused)
        ImGui::PopStyleColor();
    ImGui::EndGroup();
    ImGui::EndDisabled();
    if (focus && focus->ConsumeScrollFor(rule.name))
        ImGui::SetScrollHereY(.15f);
    // The in-game half says where its logic half is.
    std::string help = MenuRowTooltip(std::strstr(rule.label, "(MM)") == nullptr, rule.name, rule.help);
    if (!randomizer && !rule.runtimeOnly)
        help += IsSongTravelRule(rule.name) ? "\n\nLogic: Randomizer > Warps & travel."
                                            : "\n\nLogic: Randomizer > Gameplay rules.";
    MenuTooltip(help.c_str());
    if (changed) {
        if (randomizer && value == 0)
            ClearLogicRule(rule, apply); // the standalone enhancement stays as the player set it
        else if (randomizer)
            SetLogicRule(rule, value, apply);
        else
            SetEnhancementRule(rule, value, read, apply);
    }
    if (changed && !randomizer) {
        static auto refresh = (void (*)())Combo_ResolveSym("soh", "SOH_RefreshComboRules");
        if (refresh)
            refresh();
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    ImGui::Spacing();
    ImGui::PopID();
}
// The rules of a page that has no catalogue layout, and the cross-game song rules that sit above the
// world buttons of Enhancements > Warps & travel. Rules on catalogue pages are rows of their heading.
void DrawGameplayRules(bool randomizer, const std::string& page, int world,
                       ComboRando::MenuRuleFocus* focus = nullptr) {
    const bool laidOut = FindMenuPage(randomizer ? "randomizer" : "gameplay", page) != nullptr;
    std::vector<const GameplayRule*> rules;
    for (const auto& rule : kGameplayRules) {
        if (rule.trickGoverned || (randomizer && rule.runtimeOnly) || IsLegacyGameplayRule(rule.name) ||
            GameplayRulePage(rule.name, randomizer) != page)
            continue;
        if (laidOut && !(IsSongTravelRule(rule.name) && !randomizer))
            continue;
        if (!IsSongTravelRule(rule.name) && (std::strstr(rule.label, "(MM)") != nullptr) != (world == 1))
            continue;
        rules.push_back(&rule);
    }
    if (rules.empty())
        return;
    const int columns = MenuColumns();
    if (!ImGui::BeginTable("##GameplayAbilities", columns,
                           ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        return;
    for (const auto* item : rules) {
        ImGui::TableNextColumn();
        DrawGameplayRuleRow(*item, randomizer, focus);
    }
    ImGui::EndTable();
    ImGui::Spacing();
}

// Checkboxes that set one CVar in each game, so both games behave alike.
struct PairedPreference {
    const char* label;
    const char* oot;
    const char* mm;
    const char* help;
    const char* page;
};
constexpr PairedPreference kPairedPreferences[] = {
    { "Equip items with the D-pad", "gEnhancements.DpadEquips", "gEnhancements.Dpad.DpadEquips",
      "Assign items to the D-pad in the pause menu.", MenuPage::Controls },
    { "Allow unequipping items", "gEnhancements.ItemUnequip", "gEnhancements.Equipment.ItemUnequip",
      "Press an item's assigned button again in the pause menu to unequip it.", MenuPage::ItemsEquipment },
    { "Quick magic-arrow equip", "gEnhancements.SkipArrowAnimation", "gEnhancements.Equipment.MagicArrowEquipSpeed",
      "Skip the long animation when equipping magic arrows.", MenuPage::ItemsEquipment },
    { "Cycle arrow types", "gEnhancements.BowArrowCycle", "gEnhancements.PlayerActions.ArrowCycle",
      "Switch arrow types while aiming. Each game retains its native button behavior.", MenuPage::ItemsEquipment },
    { "Autosave", "gEnhancements.Autosave", "gEnhancements.Saving.Autosave",
      "OoT saves on its three-minute interval. MM uses its configured interval and persistent owl saves. Save "
      "manually before exiting.",
      MenuPage::Saving }
};
void DrawPairedPreferenceRow(const PairedPreference& pair) {
    auto& model = ComboMenuModel::Get();
    const GameMenu* games[] = { &model.Oot(), &model.Mm() };
    const CwWidget* widgets[] = { FindPreference(*games[0], pair.oot), FindPreference(*games[1], pair.mm) };
    bool disabled = !widgets[0] || !widgets[1];
    bool values[2] = {};
    for (int i = 0; i < 2; ++i) {
        if (!widgets[i])
            continue;
        values[i] = CVarGetInteger(widgets[i]->cvar, widgets[i]->bDefault) != 0;
        disabled |= widgets[i]->kind != CW_CHECKBOX || (widgets[i]->hasPreFunc && games[i]->evalDisabled &&
                                                        games[i]->evalDisabled(widgets[i]->index, nullptr));
    }
    ImGui::PushID(pair.label);
    ImGui::Spacing();
    ImGui::BeginDisabled(disabled);
    bool enabled = values[0] && values[1];
    ComboMenu_PushCheckbox(ComboMenu_ThemeColor());
    if (ImGui::Checkbox(pair.label, &enabled)) {
        for (int i = 0; i < 2; ++i) {
            CVarSetInteger(widgets[i]->cvar, enabled ? 1 : 0);
            if (widgets[i]->hasCallback && games[i]->invokeCallback)
                games[i]->invokeCallback(widgets[i]->index);
            if (games[i]->applyCVarChange)
                games[i]->applyCVarChange(widgets[i]->cvar);
        }
        AnchorSaveCVars();
    }
    ComboMenu_PopCheckbox();
    ImGui::EndDisabled();
    MenuTooltip((std::string(pair.help) + "\n\nChanges both games.").c_str());
    ImGui::PopID();
}
// The pairs of a page that has no catalogue layout; on catalogue pages they are rows of their heading.
void DrawPairedPreferences(const std::string& scope, const std::string& page) {
    if (FindMenuPage(scope, page))
        return;
    for (const auto& pair : kPairedPreferences)
        if (page == pair.page)
            DrawPairedPreferenceRow(pair);
}

// Rule and pair rows inside their catalogue heading are pseudo-widgets: CW_CUSTOM rows whose index
// is -1000 - rule or -2000 - pair, with the rule's (or this world's) CVar so anchors find them.
constexpr int32_t kRuleRowBase = -1000, kPairRowBase = -2000, kEntranceRowBase = -3000;
ComboRando::MenuRuleFocus* sRuleRowFocus = nullptr; // the menu's search focus while a page draws

bool MakePseudoRow(const char* cvar, bool oot, const std::string& scope, const std::string& page, CwWidget& row,
                   const MenuPlacement*& placement) {
    if (!cvar)
        return false;
    row = CwWidget{};
    row.kind = CW_CUSTOM;
    for (int i = 0; i < int(sizeof(kPairedPreferences) / sizeof(*kPairedPreferences)); ++i) {
        const auto& pair = kPairedPreferences[i];
        if (std::strcmp(cvar, oot ? pair.oot : pair.mm))
            continue;
        placement = Detail::FindMenuPlacementKey(oot, std::string("pair:") + pair.label);
        if (!placement || scope != placement->scope || page != placement->page)
            return false;
        row.index = kPairRowBase - i;
        row.name = pair.label;
        row.cvar = oot ? pair.oot : pair.mm;
        return true;
    }
    const auto caps = Entrances::ActiveCapabilities().Selected("");
    for (int i = 0; i < int(std::size(Entrances::kEntranceSettings)); ++i) {
        const auto& setting = Entrances::kEntranceSettings[i];
        if (std::strcmp(cvar, setting.cvar) || !setting.visible(caps))
            continue;
        placement = Detail::FindMenuPlacementKey(oot, std::string("entrance:") + setting.key);
        if (!placement || scope != placement->scope || page != placement->page)
            return false;
        row.index = kEntranceRowBase - i;
        row.name = setting.name;
        row.cvar = setting.cvar;
        row.tooltip = setting.description;
        return true;
    }
    const auto* rule = FindGameplayRule(cvar);
    if (!rule || std::strcmp(rule->name, cvar) || rule->trickGoverned || IsLegacyGameplayRule(rule->name) ||
        IsSongTravelRule(rule->name) || (std::strstr(rule->label, "(MM)") != nullptr) == oot)
        return false;
    const bool randomizer = scope == "randomizer";
    if (randomizer && rule->runtimeOnly)
        return false;
    placement = FindRulePlacement(rule->name, oot, randomizer);
    if (!placement || scope != placement->scope || page != placement->page)
        return false;
    row.index = kRuleRowBase - int32_t(rule - kGameplayRules);
    row.name = GameplayRuleMenuLabel(rule->name, rule->label, randomizer);
    row.cvar = rule->name;
    return true;
}
} // namespace

bool DrawMenuPseudoRow(const CwWidget& widget, bool randomizer) {
    if (widget.index <= kEntranceRowBase && widget.index > kEntranceRowBase - 1000) {
        if (randomizer)
            ProfileUI::DrawEntranceSetting(kEntranceRowBase - widget.index);
        return true;
    }
    if (widget.index <= kPairRowBase && widget.index > kPairRowBase - 1000) {
        const int pair = kPairRowBase - widget.index;
        if (pair < int(sizeof(kPairedPreferences) / sizeof(*kPairedPreferences)))
            DrawPairedPreferenceRow(kPairedPreferences[pair]);
        return true;
    }
    if (widget.index <= kRuleRowBase && widget.index > kRuleRowBase - 1000) {
        const int rule = kRuleRowBase - widget.index;
        if (rule < int(sizeof(kGameplayRules) / sizeof(*kGameplayRules))) {
            ImGui::BeginDisabled(randomizer && !ProfileUI::Editable());
            DrawGameplayRuleRow(kGameplayRules[rule], randomizer, sRuleRowFocus);
            ImGui::EndDisabled();
        }
        return true;
    }
    return false;
}

namespace {
void DrawCombinedGoals() {
    // One block drives both worlds, under OoTMM v32.3's names (data.ts goal, triforceGoal, triforcePieces, logic).
    // Stored values and keys are unchanged; the old names stay search aliases (DrawSearchResults).
    const auto theme = ComboMenu_ThemeColor();
    ImGui::SeparatorText("Goal & logic");
    // OoTMM's order; each entry keeps the value it was stored with.
    const char* goals[] = { "Ganon & Majora", "Any Final Boss", "Ganon", "Majora", "Triforce Hunt", "Triforce Quest" };
    const int goalValues[] = { 0, 2, 3, 4, 1, 5 };
    const int goal = std::clamp(
        ProfileUI::ReadInteger("gCombo.Rando.Goal", ProfileUI::ReadInteger("gCombo.Rando.TriforceHunt", 0) ? 1 : 0), 0,
        5);
    int goalChoice = int(std::find(std::begin(goalValues), std::end(goalValues), goal) - std::begin(goalValues));
    ImGui::TextUnformatted("Goal");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ComboMenu_PushCombobox(theme);
    if (ImGui::Combo("##Goal", &goalChoice, goals, 6)) {
        ProfileUI::WriteInteger("gCombo.Rando.Goal", goalValues[goalChoice]);
        ProfileUI::WriteInteger("gCombo.Rando.TriforceHunt", goalValues[goalChoice] == 1);
    }
    ComboMenu_PopCombobox();
    MenuTooltip("The objective of the seed. Triforce Quest requires Power, Courage and Wisdom, placed in later "
                "progression spheres in different regions.");
    const bool hunt = goalValues[goalChoice] == 1;
    if (hunt) {
        int required =
            std::clamp(ProfileUI::ReadInteger("gCombo.Rando.TriforceRequired", 15), 1, kMaxComboTriforcePieces);
        int total =
            std::clamp(ProfileUI::ReadInteger("gCombo.Rando.TriforceTotal", 15), required, kMaxComboTriforcePieces);
        // The one slider row; Default returns to the 15 pieces a new file starts with.
        if (ComboMenu_SliderRowInt("Triforce Goal", &required, 1, kMaxComboTriforcePieces, 1, 15, "%d")) {
            ProfileUI::WriteInteger("gCombo.Rando.TriforceRequired", required);
            if (total < required) {
                total = required;
                ProfileUI::WriteInteger("gCombo.Rando.TriforceTotal", total);
            }
        }
        MenuTooltip("The amount of Triforce Pieces that are required to win.");
        if (ComboMenu_SliderRowInt("Triforce Pieces", &total, required, kMaxComboTriforcePieces, 1,
                                   std::max(required, 15), "%d")) {
            ProfileUI::WriteInteger("gCombo.Rando.TriforceTotal", total);
        }
        MenuTooltip("The total amount of Triforce Pieces in the item pool.");
    }
    const char* guarantees[] = { "All Locations", "Beatable Only" };
    int guarantee = std::clamp(ProfileUI::ReadInteger(kCombinedGuarantee, 0), 0, 1);
    ImGui::TextUnformatted("Logic");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ComboMenu_PushCombobox(theme);
    if (ImGui::Combo("##Reachability", &guarantee, guarantees, 2)) {
        ProfileUI::WriteInteger(kCombinedGuarantee, guarantee);
        SynchronizeGenerationGuarantee([](const char* k, int v) { return ProfileUI::ReadInteger(k, v); },
                                       [](const char* k, int v) { ProfileUI::WriteInteger(k, v); });
    }
    ComboMenu_PopCombobox();
    MenuTooltip("All Locations: every enabled, non-excluded check can be reached. Beatable Only: the goal can be "
                "reached; some checks may not be. Each world's own No Logic turns these guarantees off.");
    const char* worlds[] = { "Start in Ocarina of Time", "Start in Majora's Mask", "Random starting world" };
    int world = std::clamp(ProfileUI::ReadInteger("gCombo.Rando.StartingGame", 0), 0, 2);
    ImGui::TextUnformatted("Starting world");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ComboMenu_PushCombobox(theme);
    if (ImGui::Combo("##StartingWorld", &world, worlds, 3)) {
        ProfileUI::WriteInteger("gCombo.Rando.StartingGame", world);
    }
    ComboMenu_PopCombobox();
    MenuTooltip("Choose the world where the new file starts. Both worlds remain part of the seed.");
    ComboMenu_PushCheckbox(theme);
    // OoTMM main.cross crossGameFw: available only with spellWindMm.
    const bool windMm = ProfileUI::ReadInteger("gRando.Options.RO_MM_SPELL_WIND", 0) != 0;
    bool crossFw = windMm && ProfileUI::ReadInteger("gRando.Options.RO_CROSS_GAME_FARORE", 0) != 0;
    ImGui::BeginDisabled(!windMm);
    if (DrawRuleCheckbox("Cross-Games Farore's Wind", &crossFw)) {
        ProfileUI::WriteInteger("gRando.Options.RO_CROSS_GAME_FARORE", crossFw);
    }
    ImGui::EndDisabled();
    MenuTooltip(windMm ? "Controls whether you can use Farore's Wind to warp between OoT and MM."
                       : "Controls whether you can use Farore's Wind to warp between OoT and MM.\n\nRequires Farore's "
                         "Wind (MM) under Items & checks > Items > Cross-game items (Majora's Mask).");
    // OoTMM main.cross crossAge (data.ts:1504-1511). Where this device cannot run adult Link in MM, or while the draft
    // has Swim shuffled (it switches on the MM gear's pinned graphs, BenPort.cpp Combo_MM_CrossAgeOptionsAllowed), the
    // row is greyed out and unticked without overwriting the stored preference, and new seeds leave it off (BenPort.cpp
    // Combo_MM_ClearUnavailableOptions). A seed's own settings show as saved. It is in no preset and has no standalone
    // rule (critique X9).
    static int (*crossAgeReady)() = nullptr;
    static int (*crossAgeAllowed)(int) = nullptr;
    if (!crossAgeReady)
        crossAgeReady = (int (*)())Combo_ResolveSym("2ship", "MM_CrossAgeOptionReady");
    if (!crossAgeAllowed)
        crossAgeAllowed = (int (*)(int))Combo_ResolveSym("2ship", "MM_CrossAgeOptionsAllowed");
    const bool ageReady = crossAgeReady && crossAgeReady();
    const bool swimShuffled = ProfileUI::Editable() && ProfileUI::ReadInteger("gRando.Options.RO_SHUFFLE_SWIM", 0) == 1;
    const bool ageAllowed = crossAgeAllowed && crossAgeAllowed(swimShuffled);
    bool crossAge = ageAllowed && ProfileUI::ReadInteger("gRando.Options.RO_MM_CROSS_AGE", 0) != 0;
    ImGui::BeginDisabled(!ageAllowed);
    if (DrawRuleCheckbox("Cross-Games Age", &crossAge)) {
        ProfileUI::WriteInteger("gRando.Options.RO_MM_CROSS_AGE", crossAge);
    }
    ImGui::EndDisabled();
    ComboMenu_PopCheckbox();
    MenuTooltip(ageAllowed ? "When you enter MM as Adult Link, you will be Adult Link in MM."
                : ageReady ? "When you enter MM as Adult Link, you will be Adult Link in MM.\n\nDisabled while Swim is "
                             "shuffled. New seeds leave it off."
                           : "When you enter MM as Adult Link, you will be Adult Link in MM.\n\nDisabled because this "
                             "device cannot run Adult Link in MM yet. New seeds leave it off.");
}
// Whether MM offers the family's MM option: its MM row is greyed out and unticked where the device cannot run the item,
// and for the boots and tunics while Swim is shuffled (MmPoolOptionUnavailable).
bool SharedMmExtensionOffered(int family) {
    const char* option = SharedFamilyMmExtension(family);
    return !option ||
           !MmPoolOptionUnavailable(option, ProfileUI::ReadInteger("gRando.Options.RO_SHUFFLE_SWIM", 0) == 1);
}
void DrawSharedItems() {
    const int columns = MenuColumns();
    if (!ImGui::BeginTable("##SharedItems", columns,
                           ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        return;
    ComboMenu_PushCheckbox(ComboMenu_ThemeColor());
    for (int i = 0; i < SF_COUNT; ++i) {
        if (i == SF_OCARINA || !SharedFamilyByIndex(i).publicOption)
            continue; // private native features await runtime validation
        ImGui::TableNextColumn();
        const auto& family = SharedFamilyByIndex(i);
        bool enabled = ProfileUI::ReadInteger(family.cvar, 0) != 0;
        ImGui::PushID(family.cvar);
        // OoTMM data.ts:3000-3054 cond `hasOoTMM && <x>Mm`: without its MM option the shared item is off
        // (util.ts:80-93 resets it). Show it unchecked but keep the stored preference; generation ignores it. An MM
        // option MM does not offer here counts as off, as generation gathers it (BenPort.cpp
        // Combo_MM_ClearUnavailableOptions).
        const char* extension = SharedFamilyMmExtension(i);
        const bool extensionOff =
            extension && (!ProfileUI::ReadInteger((std::string("gRando.Options.") + extension).c_str(), 0) ||
                          !SharedMmExtensionOffered(i));
        if (extensionOff)
            enabled = false;
        const bool unavailable =
            (family.requiresButtons && (!ProfileUI::ReadInteger("gRandoSettings.ShuffleOcarinaButtons", 0) ||
                                        !ProfileUI::ReadInteger("gRando.Options.RO_SHUFFLE_OCARINA_BUTTONS", 0))) ||
            extensionOff;
        ImGui::BeginDisabled(unavailable);
        if (DrawRuleCheckbox(family.label, &enabled)) {
            ProfileUI::WriteInteger(family.cvar, enabled);
        }
        ImGui::EndDisabled();
        MenuTooltip(family.tooltip);
        ImGui::PopID();
    }
    ComboMenu_PopCheckbox();
    ImGui::EndTable();
}
#ifdef __ANDROID__
// Android sizes the menu from gSettings.Menu.AndroidScale every frame (Mobile::ApplyMenuScale), so the desktop ImGui
// scale does nothing there. Offer the touch dialog's sizes (GameActivity.java menuSize) instead.
void DrawAndroidMenuSize() {
    static constexpr float scales[] = { 1.f, 1.25f, 1.45f, 1.75f, 2.f, 2.5f, 3.f };
    const char* labels[] = { "100%", "125%", "145% (default)", "175%", "200%", "250%", "300%" };
    const float current = CVarGetFloat("gSettings.Menu.AndroidScale", 1.45f);
    int choice = 0;
    for (int i = 1; i < IM_ARRAYSIZE(scales); ++i)
        if (std::fabs(scales[i] - current) < std::fabs(scales[choice] - current))
            choice = i;
    ImGui::TextUnformatted("Menu size");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ComboMenu_PushCombobox(ComboMenu_ThemeColor());
    if (ImGui::Combo("##MenuSize", &choice, labels, IM_ARRAYSIZE(labels))) {
        CVarSetFloat("gSettings.Menu.AndroidScale", scales[choice]);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    ComboMenu_PopCombobox();
    MenuTooltip("Size of the menu text and controls. The touch controls' Menu size option sets the same value.");
}
#endif
void DrawCosmeticSync() {
    bool linked = CVarGetInteger("gCombo.Rando.SyncCosmetics", 0) != 0;
    if (DrawRuleCheckbox("Match cosmetics between games", &linked)) {
        CVarSetInteger("gCombo.Rando.SyncCosmetics", linked);
        AnchorSaveCVars();
    }
    MenuTooltip("Use matching randomized colors and trails in both games when generating a new seed. This changes "
                "appearance only.");
}
// One HUD layout for both games; MM's own HUD editor stays reachable as a folded block in its view.
void DrawScreenAndHud(int& game, const HubEntry::Body* advanced) {
#ifdef __ANDROID__
    ImGui::SeparatorText("Screen fit");
    int mode = CVarGetInteger("gCombo.Display.AutoFit", 1);
    const char* modes[] = { "Manual (advanced graphics)", "Automatic - balanced", "Automatic - full width",
                            "Original 4:3" };
    ImGui::SetNextItemWidth(std::min(400.0f, ImGui::GetContentRegionAvail().x));
    if (ImGui::Combo("Screen fit", &mode, modes, IM_ARRAYSIZE(modes))) {
        CVarSetInteger("gCombo.Display.AutoFit", mode);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    MenuTooltip("Fit the complete original 4:3 view and HUD. Balanced limits extra side visibility to 16:9; Full width "
                "fills wider displays. Manual keeps your advanced graphics settings.");
#endif
    ImGui::SeparatorText("HUD layout");
    MenuTooltip("Move and resize hearts, buttons, magic, rupees and other game information. To move touch controls, "
                "hold Menu and choose Move controls.");
    MenuWorldButtons(game);
    DrawHudLayout(game);
    if (game == 1 && advanced && advanced->game && !advanced->widgets.empty()) {
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Advanced HUD editor (MM)")) {
            const CwSidebar side = { MenuPage::ScreenHud, advanced->columns, advanced->widgets.data(),
                                     (int)advanced->widgets.size() };
            RenderSidebarWidgets(side, *advanced->game);
        }
    }
}

// Where the catalogue puts a native row, and the native heading it sat under (game-owned storage).
struct RowOrigin {
    const MenuPlacement* placement = nullptr;
    const char* heading = nullptr;
};
using RowOrigins = std::unordered_map<int32_t, RowOrigin>;

void AppendFeatureWidgets(const HubEntry::Source& source, const std::string& scope, const std::string& page,
                          int subpage, std::vector<CwWidget>& output, uint32_t& columns,
                          RowOrigins* origins = nullptr) {
    CwSidebar side = *source.sidebar;
    std::vector<CwWidget> grouped;
    std::string currentHeading;
    const char* nativeHeading = nullptr;
    int sourceColumn = -1;
    for (int i = 0; i < side.widgetCount; ++i) {
        const auto& widget = side.widgets[i];
        if (sourceColumn != widget.column) {
            sourceColumn = widget.column;
            currentHeading.clear();
            nativeHeading = nullptr;
        }
        if (widget.kind == CW_SEPARATOR_TEXT) {
            currentHeading = widget.name ? widget.name : "";
            nativeHeading = widget.name;
        }
        const auto route = MenuGroupRoute(source.oot, source.section, side.sidebarName, sourceColumn, currentHeading,
                                          widget.cvar ? widget.cvar : "");
        if (route.scope != scope || route.page != page)
            continue;
        grouped.push_back(widget);
        if (origins)
            (*origins)[widget.index] = { FindMenuPlacement(source.oot, source.section, side.sidebarName, currentHeading,
                                                           widget.cvar ? widget.cvar : "",
                                                           widget.name ? widget.name : ""),
                                         nativeHeading };
    }
    side.widgets = grouped.data();
    side.widgetCount = (int)grouped.size();
    std::vector<CwWidget> filtered;
    if (page == MenuPage::General && !std::strcmp(source.section, "Settings") &&
        !std::strcmp(source.sidebar->sidebarName, "General")) {
        // Drop desktop-only actions, obsolete native-menu preferences and engine About columns.
        for (int i = 0; i < side.widgetCount; ++i) {
            const auto& w = side.widgets[i];
            const std::string name = w.name ? w.name : "";
            const std::string cv = w.cvar ? w.cvar : "";
            if (FindGameplayRule(w.cvar))
                continue;
            if (w.column > 0 || name == "Open App Files Folder" || name == "Cursor Always Visible" ||
                name == "Allow background inputs" || name == "Search In Sidebar" || name == "Search Input Autofocus" ||
                name == "Import Settings" || name == "Menu Theme" || name == "Menu Controller Navigation" ||
                name == "Menu Background Opacity" || name == "ImGui Menu Scaling" || name == "Menu Settings" ||
                name == "General Settings" || name == "EXPERIMENTAL" || cv == "gSettings.Menu.SidebarSearch")
                continue;
            filtered.push_back(w);
        }
        side.widgets = filtered.data();
        side.widgetCount = (int)filtered.size();
        side.columnCount = 1;
    }
    if (subpage >= 0 && source.oot) {
        filtered.clear();
        std::string heading;
        int column = -1;
        for (int i = 0; i < side.widgetCount; ++i) {
            const auto& w = side.widgets[i];
            if (column != w.column) {
                column = w.column;
                heading.clear();
            }
            if (w.kind == CW_SEPARATOR_TEXT)
                heading = w.name ? w.name : "";
            if (RandomizerWidgetGroup(true, source.sidebar->sidebarName, w.column, heading, w.cvar ? w.cvar : "") !=
                subpage)
                continue;
            filtered.push_back(w);
        }
        side.widgets = filtered.data();
        side.widgetCount = (int)filtered.size();
        const bool pools = std::strcmp(source.sidebar->sidebarName, "Shuffles") == 0;
        side.columnCount = pools ? 2 : source.sidebar->columnCount;
        if (pools)
            for (auto& widget : filtered)
                widget.column = widget.column % 2;
    }
    if (source.oot && subpage == 0 && std::strcmp(source.sidebar->sidebarName, "Shuffles") == 0) {
        constexpr const char* headings[] = { "World & NPCs", "Creatures & Collectibles", "Scenery Drops",
                                             "Shops & Merchants" };
        std::vector<CwWidget> arranged;
        for (int group = 0; group < 4; ++group) {
            CwWidget heading{};
            heading.index = -1;
            heading.kind = CW_SEPARATOR_TEXT;
            heading.name = headings[group];
            heading.column = group == 1 || group == 2 ? 1 : 0;
            arranged.push_back(heading);
            for (int i = 0; i < side.widgetCount; ++i) {
                const auto& widget = side.widgets[i];
                if (widget.kind == CW_SEPARATOR_TEXT || widget.kind == CW_SEPARATOR)
                    continue;
                if (OotLocationPoolGroup(widget.cvar ? widget.cvar : "", widget.column) != group)
                    continue;
                arranged.push_back(widget);
                arranged.back().column = heading.column;
            }
        }
        filtered = std::move(arranged);
        side.widgets = filtered.data();
        side.widgetCount = (int)filtered.size();
        side.columnCount = 2;
    }
    columns = std::max(columns, side.columnCount);
    for (int i = 0; i < side.widgetCount; ++i) {
        const auto& widget = side.widgets[i];
        if (!IsRetiredNativeRuleControl(widget.cvar)) {
            output.push_back(widget);
            continue;
        }
        CwWidget row;
        const MenuPlacement* placement = nullptr;
        if (origins && MakePseudoRow(widget.cvar, source.oot, scope, page, row, placement) &&
            !origins->count(row.index)) {
            output.push_back(row);
            (*origins)[row.index] = { placement, nullptr };
        }
    }
}
// The rule and pair rows of a catalogue page that no native control of this world stood in for
// (Randomizer > Gameplay rules, a count with no native widget), after the native rows.
void AppendEntranceRows(bool oot, const std::string& scope, const std::string& page, std::vector<CwWidget>& output,
                        RowOrigins& origins, bool addHeading) {
    CwWidget row;
    const MenuPlacement* placement = nullptr;
    bool headingAdded = false;
    for (const auto& setting : Entrances::kEntranceSettings)
        if (MakePseudoRow(setting.cvar, oot, scope, page, row, placement) && !origins.count(row.index)) {
            if (addHeading && !headingAdded) {
                CwWidget heading{};
                heading.index = -1;
                heading.kind = CW_SEPARATOR_TEXT;
                heading.name = placement->heading;
                heading.column = 1;
                output.push_back(heading);
                headingAdded = true;
            }
            row.column = addHeading ? 1 : 0;
            output.push_back(row);
            origins[row.index] = { placement, nullptr };
        }
}
void AppendPseudoRows(bool oot, const std::string& scope, const std::string& page, std::vector<CwWidget>& output,
                      RowOrigins& origins) {
    CwWidget row;
    const MenuPlacement* placement = nullptr;
    for (const auto& pair : kPairedPreferences)
        if (MakePseudoRow(oot ? pair.oot : pair.mm, oot, scope, page, row, placement) && !origins.count(row.index)) {
            output.push_back(row);
            origins[row.index] = { placement, nullptr };
        }
    AppendEntranceRows(oot, scope, page, output, origins, false);
    for (const auto& rule : kGameplayRules)
        if (MakePseudoRow(rule.name, oot, scope, page, row, placement) && !origins.count(row.index)) {
            output.push_back(row);
            origins[row.index] = { placement, nullptr };
        }
}

// Native intro lines that describe the old per-game menus, not this page.
bool IsStaleIntroText(const CwWidget& widget) {
    static constexpr const char* stale[] = {
        "Be sure to explore the Presets and Enhancements Menus",
        "These enhancements are only useful in the Randomizer mode",
    };
    if (widget.kind != CW_TEXT || !widget.name)
        return false;
    for (const char* text : stale)
        if (!std::strncmp(widget.name, text, std::strlen(text)))
            return true;
    return false;
}

// Lay a page out from the catalogue: its fixed headings in order (a heading with no rows in this
// world is not drawn), rows by rank and then native order, each anchored row kept under its
// anchor, and the same column split in both worlds. Native separators give way to the page's own
// headings; a row the catalogue does not place keeps its native heading after the fixed ones.
// Returns the column count; RenderSidebarWidgets keeps one column on a phone.
uint32_t ArrangeByPlacement(const MenuPageLayout& layout, std::vector<CwWidget>& widgets, const RowOrigins& origins,
                            uint32_t columns) {
    if (layout.headingCount == 0) {
        // No fixed headings: keep native order, minus a heading that only repeats the page title.
        widgets.erase(std::remove_if(widgets.begin(), widgets.end(),
                                     [&](const CwWidget& widget) {
                                         return IsStaleIntroText(widget) ||
                                                (widget.kind == CW_SEPARATOR_TEXT && widget.name &&
                                                 !std::strcmp(widget.name, layout.page));
                                     }),
                      widgets.end());
        return columns;
    }
    struct Row {
        CwWidget widget;
        int heading, rank, order;
        int anchor = -1;       // native order of the row this one stays under
        bool sameLine = false; // drawn on its anchor's line
    };
    std::vector<const char*> extraHeadings;
    std::vector<Row> rows;
    for (const auto& widget : widgets) {
        if (widget.index >= 0 && (widget.kind == CW_SEPARATOR || widget.kind == CW_SEPARATOR_TEXT))
            continue;
        if (IsStaleIntroText(widget))
            continue;
        const auto found = origins.find(widget.index);
        const RowOrigin origin = found != origins.end() ? found->second : RowOrigin{};
        int heading =
            origin.placement && origin.placement->heading ? MenuHeadingIndex(layout, origin.placement->heading) : -1;
        if (heading < 0) {
            const char* name = origin.heading ? origin.heading : "";
            auto extra = std::find_if(extraHeadings.begin(), extraHeadings.end(),
                                      [&](const char* other) { return !std::strcmp(other, name); });
            if (extra == extraHeadings.end())
                extra = extraHeadings.insert(extraHeadings.end(), name);
            heading = layout.headingCount + int(extra - extraHeadings.begin());
        }
        rows.push_back({ widget, heading, origin.placement ? origin.placement->rank : 100000, int(rows.size()) });
    }
    // Anchors use native order: a sameLine row follows its native predecessor; an "after" row
    // follows the row with its anchor CVar.
    for (auto& row : rows) {
        if (row.widget.sameLine && row.order > 0) {
            row.anchor = row.order - 1;
            row.sameLine = true;
            continue;
        }
        const auto found = origins.find(row.widget.index);
        const char* after =
            found != origins.end() && found->second.placement ? found->second.placement->after : nullptr;
        if (!after)
            continue;
        for (const auto& other : rows)
            if (other.widget.cvar && !std::strcmp(other.widget.cvar, after))
                row.anchor = other.order;
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.heading != b.heading ? a.heading < b.heading : a.rank != b.rank ? a.rank < b.rank : a.order < b.order;
    });
    std::vector<int> position(rows.size());
    for (int i = 0; i < int(rows.size()); ++i)
        position[rows[i].order] = i;
    // Emit in sorted order; a row sorted above its anchor (or sharing its line) waits for it.
    std::vector<bool> emitted(rows.size());
    std::vector<Row> arranged;
    const auto waits = [&](int i) {
        const auto& row = rows[i];
        return row.anchor >= 0 && row.anchor != row.order &&
               (row.sameLine || position[row.order] < position[row.anchor]);
    };
    std::function<void(int)> emit = [&](int i) {
        emitted[i] = true;
        arranged.push_back(rows[i]);
        for (int j = 0; j < int(rows.size()); ++j)
            if (!emitted[j] && waits(j) && rows[j].anchor == rows[i].order) {
                rows[j].heading = rows[i].heading;
                emit(j);
            }
    };
    for (int i = 0; i < int(rows.size()); ++i)
        if (!emitted[i] && (!waits(i) || emitted[position[rows[i].anchor]]))
            emit(i);
    for (int i = 0; i < int(rows.size()); ++i)
        if (!emitted[i]) // an anchor chain that loops: keep the row rather than drop it
            emit(i);
    const bool split = layout.split < layout.headingCount;
    widgets.clear();
    int heading = -1;
    for (const auto& row : arranged) {
        const int column = split && row.heading >= layout.split ? 1 : 0;
        if (row.heading != heading) {
            heading = row.heading;
            const char* name =
                heading < layout.headingCount ? layout.headings[heading] : extraHeadings[heading - layout.headingCount];
            if (*name && (layout.headingCount > 1 || heading >= layout.headingCount)) {
                CwWidget title{};
                title.index = -1;
                title.kind = CW_SEPARATOR_TEXT;
                title.name = name;
                title.column = column;
                widgets.push_back(title);
            }
        }
        widgets.push_back(row.widget);
        widgets.back().column = column;
        widgets.back().sameLine = row.sameLine;
    }
    return split ? 2 : 1;
}
} // namespace

void ComboMenu::DrawSharedPanel() {
    DrawRuleChangeWarning();
    auto& model = ComboMenuModel::Get();
    model.EnsureLoaded();
    static MenuNavigationCache navigation;
    auto& entries = navigation.Get(mScope, model);
    if (entries.empty()) {
        ImGui::TextUnformatted("Loading settings...");
        return;
    }
    auto active =
        std::find_if(entries.begin(), entries.end(), [&](const HubEntry& e) { return e.key() == mHubActive; });
    if (active == entries.end()) {
        active = entries.begin();
        mHubActive = active->key();
    }
    if (mScope != "home") {
        float width = 0;
        for (const auto& e : entries)
            width = std::max(width, ImGui::CalcTextSize(e.label.c_str()).x + ImGui::GetStyle().ScrollbarSize + 40.0f);
        width = std::min(width, ImGui::GetContentRegionAvail().x * 0.32f);
        ImGui::BeginChild("##FeatureSidebar", ImVec2(width, 0));
        auto drawEntry = [&](auto it) {
            ImGui::PushID(it->key().c_str());
            if (ComboMenu_StyledSidebarEntry(it->label.c_str(), it == active, ImGui::GetContentRegionAvail().x)) {
                active = it;
                mHubActive = it->key();
                mRuleFocus.Clear();
                ClearSearch(); // even on the page already shown
            }
            ImGui::PopID();
        };
        for (auto it = entries.begin(); it != entries.end(); ++it)
            if (mScope != "tools" || !IsDeveloperToolPage(it->label))
                drawEntry(it);
        if (mScope == "tools" && std::any_of(entries.begin(), entries.end(),
                                             [](const HubEntry& e) { return IsDeveloperToolPage(e.label); })) {
            ImGui::Spacing();
            if (IsDeveloperToolPage(active->label))
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            if (ImGui::CollapsingHeader("Developer tools"))
                for (auto it = entries.begin(); it != entries.end(); ++it)
                    if (IsDeveloperToolPage(it->label))
                        drawEntry(it);
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }
    ImGui::PushID(active->key().c_str());
    ImGui::BeginChild("##FeatureContent", ImVec2(0, 0));
    if (mResetContentScroll || mSearchQuery != mLastSearchQuery) {
        ImGui::SetScrollY(0.0f);
        mResetContentScroll = false;
    }
    mLastSearchQuery = mSearchQuery;
    ResolveComboGenSyms();
    const auto* generation = sGetProgress ? sGetProgress() : nullptr;
    const bool generating = generation && generation->running.load();
    // Changing rules during the worker's reachability queries could create inconsistent seeds.
    ImGui::BeginDisabled(generating && mScope != "randomizer" && active->kind != HubEntry::COMBO_GEN);
    if (!mSearchQuery.empty()) {
        DrawSearchResults(mSearchQuery);
    } else if ((mScope == "randomizer" || mScope == "home") && ProfileUI::DrawUnavailableSettings()) {
        // Never present fallback defaults as a file's saved/generated settings.
    } else {
        ImGui::SeparatorText(active->label.c_str());
        if (mScope == "randomizer")
            MenuTooltip(RandomizerPageHelp(active->label));
        switch (active->kind) {
            case HubEntry::COMBO_HOME:
                ProfileUI::DrawHome();
                break;
            case HubEntry::COMBO_SHARED_ITEMS:
                ImGui::BeginDisabled(!ProfileUI::Editable());
                DrawSharedItems();
                ImGui::EndDisabled();
                break;
            case HubEntry::COMBO_INTERFACE: {
                const auto& game = model.Oot();
                if (game.loaded && game.menu) {
                    for (int s = 0; s < game.menu->sectionCount; ++s) {
                        const auto& section = game.menu->sections[s];
                        if (std::strcmp(section.sectionLabel, "Settings") != 0)
                            continue;
                        for (int b = 0; b < section.sidebarCount; ++b) {
                            const auto& side = section.sidebars[b];
                            if (std::strcmp(side.sidebarName, "General") != 0)
                                continue;
                            for (int w = 0; w < side.widgetCount; ++w) {
                                const auto& widget = side.widgets[w];
                                const std::string name = widget.name ? widget.name : "";
                                if (name != "Menu Theme" && name != "Menu Controller Navigation" &&
                                    name != "Menu Background Opacity" && name != "ImGui Menu Scaling")
                                    continue;
#ifdef __ANDROID__
                                if (name == "ImGui Menu Scaling") {
                                    DrawAndroidMenuSize();
                                    continue;
                                }
#endif
                                RenderWidget(widget, game);
                            }
                        }
                    }
                }
                break;
            }
            case HubEntry::COMBO_GEN:
                ProfileUI::DrawGeneration();
                break;
            case HubEntry::COMBO_PLANDO:
                if (ProfileUI::Editable())
                    DrawComboPlandoPanel();
                else
                    ProfileUI::DrawSavedPlacements();
                break;
            case HubEntry::COMBO_TRACKER:
                DrawTrackerSharedPanel();
                break;
            case HubEntry::COMBO_CHECK_TRACKER:
                DrawCheckTrackerSharedPanel();
                break;
            case HubEntry::COMBO_HINT_TRACKER:
                DrawHintTrackerSharedPanel();
                break;
            case HubEntry::COMBO_TIMERS:
                DrawTimersSharedPanel();
                break;
            case HubEntry::COMBO_NETWORK:
                DrawNetworkSharedPanel();
                break;
            case HubEntry::COMBO_ABOUT:
#ifdef COMBO_ANDROID_APP_VERSION
                ImGui::TextWrapped("Combo Ship for Android - %s", COMBO_ANDROID_APP_VERSION);
#else
                ImGui::TextUnformatted("Combo Ship for Android");
#endif
                ImGui::TextWrapped("A combined Ocarina of Time and Majora's Mask randomizer with shared menus, "
                                   "trackers and touch controls.");
                ImGui::Spacing();
                ImGui::SeparatorText("Built on community work");
                ImGui::TextWrapped(
                    "ComboShip, Ship of Harkinian, 2 Ship 2 Harkinian, libultraship and their Android ports.");
                ImGui::TextWrapped("Your ROMs, extracted game data and saves stay on your device.");
                break;
            case HubEntry::COMBO_SESSION: {
                ImGui::TextWrapped("Save in-game before restarting or exiting.");
                const float height = ImGui::GetFrameHeight() * 1.25f;
                ComboMenu_PushButton(ComboMenu_ThemeColor());
                if (ImGui::Button("Restart game", ImVec2(-FLT_MIN, height)))
                    ImGui::OpenPopup("Restart game?");
                if (ImGui::Button("Exit game", ImVec2(-FLT_MIN, height)))
                    ImGui::OpenPopup("Exit game?");
                ComboMenu_PopButton();
                if (MenuConfirm("Restart game?", "Unsaved progress will be lost.", "Restart game")) {
                    if (auto console = std::static_pointer_cast<Ship::ConsoleWindow>(
                            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGuiWindow("Console"))) {
                        console->Dispatch("reset");
                        Hide();
                    }
                }
                if (MenuConfirm("Exit game?", "Unsaved progress will be lost.", "Exit game"))
                    Ship::Context::GetRawInstance()->GetWindow()->Close();
                break;
            }
            default: {
                if (active->label == "Tricks & glitches") {
                    ProfileUI::DrawTricks();
                    break;
                }
                if (active->label == MenuPage::ScreenHud) {
                    const auto key = std::make_pair(1, -1);
                    if (!active->bodies.count(key)) {
                        HubEntry::Body built;
                        for (const auto& source : active->sources)
                            if (!source.oot) {
                                built.game = source.game;
                                AppendFeatureWidgets(source, mScope, active->label, -1, built.widgets, built.columns);
                            }
                        active->bodies.emplace(key, std::move(built));
                    }
                    int game = mPreferenceGame ? mPreferenceGame - 1 : ComboUI::GetForegroundGame();
                    DrawScreenAndHud(game, &active->bodies.at(key));
                    mPreferenceGame = game + 1;
                    if (ImGui::Button("Advanced graphics")) {
                        mScope = "media";
                        mHubActive = std::string("media/") + MenuPage::Graphics;
                    }
                    break;
                }
                bool hasOot = false, hasMm = false;
                for (const auto& source : active->sources) {
                    hasOot |= source.oot;
                    hasMm |= !source.oot;
                }
                int selectedGame = ComboUI::GetForegroundGame();
                const bool seedOptions = mScope == "randomizer";
                auto choiceRow = [&](const char* id, const char* first, const char* second, int& choice) {
                    ImGui::PushID(id);
                    if (ComboMenu_StyledHeaderEntry(first, choice == 0))
                        choice = 0;
                    const float need = ImGui::CalcTextSize(second).x + 32.0f;
                    if (ImGui::GetItemRectMax().x + need + 12.0f <
                        ImGui::GetWindowPos().x + ImGui::GetContentRegionMax().x)
                        ImGui::SameLine();
                    if (ComboMenu_StyledHeaderEntry(second, choice == 1))
                        choice = 1;
                    ImGui::PopID();
                };
                const auto* layout = FindMenuPage(mScope, active->label);
                if (!seedOptions) {
                    const char* scopeLine = layout && *layout->scopeLine ? layout->scopeLine
                                            : (hasOot && hasMm) || active->label == MenuPage::WarpsTravel ? ""
                                            : hasOot ? "Ocarina of Time only."
                                            : hasMm  ? "Majora's Mask only."
                                                     : "";
                    if (*scopeLine)
                        ImGui::TextColored(kBreadcrumbColor, "%s", scopeLine);
                }
                if (!seedOptions && active->label == MenuPage::WarpsTravel) {
                    ImGui::SeparatorText(GameplayRuleHeading(kSongTravelRuleKeys[0], false));
                    DrawGameplayRules(false, active->label, -1, &mRuleFocus);
                }
                if (seedOptions && active->label == "Logic & access") {
                    ImGui::BeginDisabled(!ProfileUI::Editable());
                    DrawCombinedGoals();
                    ImGui::EndDisabled();
                }
                if (hasOot && hasMm) {
                    if (seedOptions) {
                        choiceRow("SeedWorld", "Ocarina of Time", "Majora's Mask", mRandomizerGame);
                        selectedGame = mRandomizerGame;
                    } else {
                        if (mPreferenceGame)
                            selectedGame = mPreferenceGame - 1;
                        MenuWorldButtons(selectedGame);
                        mPreferenceGame = selectedGame + 1;
                    }
                } else if (active->label == MenuPage::WarpsTravel) {
                    // Song travel drives both games; OoT's own destination shuffles follow it on the Randomizer tab.
                    selectedGame = hasOot ? 0 : hasMm ? 1 : -1;
                } else if (active->label == "Gameplay rules" || active->label == MenuPage::Difficulty ||
                           active->label == "Logic & access") {
                    if (seedOptions) {
                        MenuWorldButtons(mRandomizerGame);
                        selectedGame = mRandomizerGame;
                    } else {
                        if (mPreferenceGame)
                            selectedGame = mPreferenceGame - 1;
                        MenuWorldButtons(selectedGame);
                        mPreferenceGame = selectedGame + 1;
                    }
                } else if (hasOot || hasMm) {
                    selectedGame = hasOot ? 0 : 1;
                }
                int subpage = -1;
                if (active->label == "Items & checks") {
                    choiceRow("PoolSection", "Checks", "Items", mPoolSection);
                    subpage = mPoolSection;
                } else if (active->label == "Hints & traps") {
                    choiceRow("HintSection", "Hints", "Traps", mHintSection);
                    subpage = mHintSection;
                }
                // Keep world/topic selectors visible while each independent body scrolls.
                ImGui::PushID(selectedGame);
                ImGui::PushID(subpage);
                ImGui::BeginChild("##FeatureOptions", ImVec2(0, 0));
                if (mResetOptionsScroll) {
                    ImGui::SetScrollY(0.0f);
                    mResetOptionsScroll = false;
                }
                ImGui::BeginDisabled(seedOptions && !ProfileUI::Editable());
                if (seedOptions || active->label != MenuPage::WarpsTravel)
                    DrawGameplayRules(seedOptions, active->label, selectedGame, &mRuleFocus);
                ImGui::EndDisabled();
                if (!seedOptions)
                    DrawPairedPreferences(mScope, active->label);
                sRuleRowFocus = &mRuleFocus;
                if (mScope == "media" && active->label == MenuPage::Appearance)
                    DrawCosmeticSync();
                const auto bodyKey = std::make_pair(selectedGame, subpage);
                auto body = active->bodies.find(bodyKey);
                if (body == active->bodies.end()) {
                    HubEntry::Body built;
                    RowOrigins origins;
                    for (const auto& source : active->sources) {
                        if (source.oot != (selectedGame == 0))
                            continue;
                        built.game = source.game;
                        if (!source.oot && subpage >= 0 &&
                            RandomizerWidgetGroup(false, source.sidebar->sidebarName, 0, "") != subpage)
                            continue;
                        AppendFeatureWidgets(source, mScope, active->label, subpage, built.widgets, built.columns,
                                             &origins);
                    }
                    if (layout && selectedGame >= 0) {
                        AppendPseudoRows(selectedGame == 0, mScope, active->label, built.widgets, origins);
                        if (!built.game && !built.widgets.empty())
                            built.game = selectedGame == 0 ? &model.Oot() : &model.Mm();
                        built.columns = ArrangeByPlacement(*layout, built.widgets, origins, built.columns);
                    } else if (seedOptions && active->label == "Logic & access" && selectedGame >= 0) {
                        AppendEntranceRows(selectedGame == 0, mScope, active->label, built.widgets, origins, true);
                        if (!built.game && !built.widgets.empty())
                            built.game = selectedGame == 0 ? &model.Oot() : &model.Mm();
                    }
                    body = active->bodies.emplace(bodyKey, std::move(built)).first;
                }
                // OoT rows that drive both games, shown in the MM view too: Controls' Controller mapping and
                // Audio's Volume (the volume sliders mirror into MM). A page's first heading goes above MM's
                // rows, a later one below them, so both views keep the page's heading order.
                const char* sharedHeading = !layout || seedOptions || selectedGame != 1 ? nullptr
                                            : mScope == "settings" && active->label == MenuPage::Controls
                                                ? "Controller mapping"
                                            : mScope == "media" && active->label == MenuPage::Audio ? "Volume"
                                                                                                    : nullptr;
                const bool sharedFirst = sharedHeading && MenuHeadingIndex(*layout, sharedHeading) == 0;
                auto drawShared = [&] {
                    const auto mappingKey = std::make_pair(0, -2);
                    auto mapping = active->bodies.find(mappingKey);
                    if (mapping == active->bodies.end()) {
                        HubEntry::Body built;
                        RowOrigins origins;
                        for (const auto& source : active->sources)
                            if (source.oot) {
                                built.game = source.game;
                                AppendFeatureWidgets(source, mScope, active->label, -1, built.widgets, built.columns,
                                                     &origins);
                            }
                        built.columns = ArrangeByPlacement(*layout, built.widgets, origins, built.columns);
                        // Keep the shared heading and its rows.
                        const auto first =
                            std::find_if(built.widgets.begin(), built.widgets.end(), [&](const CwWidget& w) {
                                return w.index == -1 && w.kind == CW_SEPARATOR_TEXT && w.name &&
                                       !std::strcmp(w.name, sharedHeading);
                            });
                        const auto last = std::find_if(
                            first == built.widgets.end() ? first : first + 1, built.widgets.end(),
                            [](const CwWidget& w) { return w.index == -1 && w.kind == CW_SEPARATOR_TEXT; });
                        built.widgets = std::vector<CwWidget>(first, last);
                        for (auto& widget : built.widgets)
                            widget.column = 0;
                        built.columns = 1;
                        mapping = active->bodies.emplace(mappingKey, std::move(built)).first;
                    }
                    if (mapping->second.game && !mapping->second.widgets.empty()) {
                        const CwSidebar shared = { active->label.c_str(), 1, mapping->second.widgets.data(),
                                                   (int)mapping->second.widgets.size() };
                        ImGui::PushID("SharedOot"); // OoT and MM widget indexes overlap
                        RenderSidebarWidgets(shared, *mapping->second.game);
                        ImGui::PopID();
                    }
                };
                if (sharedFirst)
                    drawShared();
                auto& content = body->second;
                if (content.game) {
                    const CwSidebar feature = { active->label.c_str(), content.columns, content.widgets.data(),
                                                (int)content.widgets.size() };
                    if (seedOptions)
                        ProfileUI::DrawSidebar(feature, *content.game, selectedGame);
                    else
                        RenderSidebarWidgets(feature, *content.game);
                }
                if (sharedHeading && !sharedFirst)
                    drawShared();
                ImGui::EndChild();
                ImGui::PopID();
                ImGui::PopID();
                break;
            }
        }
    }
    ImGui::EndDisabled();
    ImGui::EndChild();
    ImGui::PopID();
}

} // namespace ComboRando

// ComboShip: open the combo menu on the Randomizer tab (file-select "Open Randomizer Settings").
extern "C" COMBO_EXPORT void ComboUI_OpenRandomizerSettings(void) {
    if (ComboRando::sComboMenu)
        ComboRando::sComboMenu->OpenAtRandomizer();
}

extern "C" COMBO_EXPORT void ComboUI_Register(void) {
    auto ctx = Ship::Context::GetRawInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return; // GUI not ready
    }
    auto gui = ctx->GetWindow()->GetGui();
    // Match the existing menu-visibility CVar so the in-game menu hotkey toggles us.
    ComboRando::sComboMenu = std::make_shared<ComboRando::ComboMenu>("gOpenWindows.Menu", "Combo Menu");
    gui->SetMenu(ComboRando::sComboMenu);
    gui->SetMenuBackHandler([] { return ComboRando::sComboMenu && ComboRando::sComboMenu->HandleBack(); });

    // Trackers: legacy setting migration, shared appearance, Triforce overlay and the combo windows.
    ComboTrackerUI::Migrate(ComboTrackerUI::Item);
    ComboTrackerUI::Migrate(ComboTrackerUI::Hint);
    ComboTracker::SyncAppearance();
    ComboTracker::RegisterSwap();
    ComboRando::RegisterCheckTrackerWindow();
    ComboTrackerUI::RegisterItemWindow();

    // Combo-native floating Anchor room window (toggled from the Anchor panel).
    ComboRando::RegisterAnchorRoomWindow();

    // Combo-owned cross-game Personal Notes window (toggled from the Shared item tracker panel).
    ComboNotes::RegisterWindow();

    // Combo-owned unified Hint Tracker (#164). OOT's native one is unreachable now that its sidebar is
    // hidden, so force it shut here — a config that persisted gOpenWindows.HintTracker=1 would
    // otherwise keep showing a window with no settings and no combo hint content.
    ComboRando::RegisterHintTrackerWindow();
    for (const auto& [cvar, name] : { std::pair{ "gOpenWindows.HintTracker", "Hint Tracker" },
                                      std::pair{ "gOpenWindows.HintTrackerSettings", "Hint Tracker Settings" } }) {
        CVarSetInteger(cvar, 0);
        if (auto win = gui->GetGuiWindow(name))
            win->Hide();
    }

    // Combo-owned overlay timers (#173). Both games' overlays are retired. MM's window is not
    // registered yet, so only its CVar can be cleared here; the combo window re-asserts each frame.
    ComboRando::RegisterTimersWindow();
    for (const auto& [cvar, name] : { std::pair{ "gOpenWindows.TimeDisplayEnabled", "Additional Timers" },
                                      std::pair{ "gWindows.DisplayOverlay", "Display Overlay" } }) {
        CVarSetInteger(cvar, 0);
        if (auto win = gui->GetGuiWindow(name))
            win->Hide();
    }
}
