#include "ComboRandomizerProfiles.h"
#include "ComboTrackerControls.h"
// combo/gui/ComboMenu.cpp
#include "ComboMenu.h"
#include "ComboExport.h"
#include "ComboMenuModel.h"
#include "ComboNavigation.h"
#include "ComboMenuNavigation.h"
#include "ComboMenuLayout.h"
#include "rando/ComboPresets.h"
#include "ComboWidgetRender.h"
#include "ComboWidgetStyle.h"
#include "ComboForeground.h" // dormant-game gate for inline tracker popout buttons
#include "ComboTrackerBridge.h"
#include "ComboTrackerCommon.h" // kKinds (HideBackground CVars for the tracker panels)
#include "ComboTrackerSwap.h"
#include "ComboAnchorRoomWindow.h"  // combo-native floating Anchor room window
#include "ComboNotesWindow.h"       // combo-owned cross-game Personal Notes window
#include "ComboHintTracker.h"       // combo-owned unified Hint Tracker (#164)
#include "ComboCheckTracker.h"
#include "rando/GameplayRules.h"
#include "rando/GenerationRules.h"
#include "ComboRuleWarning.h"
#include "ComboSongTravel.h"
#include "ComboHudLayout.h"
#include "ComboTricks.h"
#include "menu/ComboTrickABI.h"
#include "ComboTimersWindow.h"      // combo-owned overlay timers (#173)
#include "rando/ComboPlaythrough.h" // plando: ParseSpoilerPlacements + Suffix/BuildForeignArray + slot paths
#include <imgui.h>
#include <libultraship/libultraship.h>         // CVar bridge (CVarGet/Set* incl. color) + color.h (Color_RGBA8)
#include <ship/window/gui/IconsFontAwesome4.h> // ICON_FA_* for the header action buttons
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <unordered_set>
#include "ComboResolve.h"

namespace {
// soh.dll exports: trigger combo generation (non-blocking; spawns the launcher worker), read the
// shared progress, and gate generation to the file-select screen.
typedef void (*FnTriggerGenerate)(void);
typedef const ComboRando::ComboGenProgress* (*FnGetProgress)(void);
typedef unsigned char (*FnIsOnFileSelect)(void);
// (#135) re-applies the Starting Age / Mask Shop exclusion disables after the dropdown changes.
typedef void (*FnRefreshStartingGameUI)(void);
FnTriggerGenerate sTrigger = nullptr;
FnGetProgress sGetProgress = nullptr;
FnIsOnFileSelect sIsOnFileSelect = nullptr;
FnRefreshStartingGameUI sRefreshStartingGameUI = nullptr;
void ResolveComboGenSyms() {
    if (!sTrigger)
        sTrigger = (FnTriggerGenerate)Combo_ResolveSym("soh", "SOH_TriggerComboGenerate");
    if (!sGetProgress)
        sGetProgress = (FnGetProgress)Combo_ResolveSym("soh", "SOH_GetComboGenProgress");
    if (!sIsOnFileSelect)
        sIsOnFileSelect = (FnIsOnFileSelect)Combo_ResolveSym("soh", "SOH_CanGenerateComboSeed");
    if (!sRefreshStartingGameUI)
        sRefreshStartingGameUI = (FnRefreshStartingGameUI)Combo_ResolveSym("soh", "SOH_RefreshComboStartingGameUI");
}

// Bug 2: Anchor resync exports, one per game DLL â€” resolved the same way as the combo-gen syms
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

// Shared Anchor config CVar keys (process-global libultraship store; both game DLLs read these â€” see
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
FnDump sSohDump = nullptr;
FnDump sMmDump = nullptr;
FnRequestReload sRequestReload = nullptr;
void ResolvePlandoSyms() {
    if (!sSohDump)
        sSohDump = (FnDump)Combo_ResolveSym("soh", "SOH_DumpRandoStaticData");
    if (!sRequestReload)
        sRequestReload = (FnRequestReload)Combo_ResolveSym("soh", "SOH_RequestComboReload");
    if (!sMmDump)
        sMmDump = (FnDump)Combo_ResolveSym("2ship", "MM_DumpRandoStaticData");
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

// OoT exports its restoration controls in the third Fixes column (index 2). Route that
// group into the shared Restorations page while retaining every native widget.
static MenuDestination FeatureMenuRoute(bool oot, const std::string& section, const std::string& sidebar, int column) {
    if (oot && section == "Enhancements" && sidebar == "Fixes" && column == 2)
        return { "gameplay", "Restorations" };
    return MenuRoute(oot, section, sidebar);
}

// Search normalization shared by the query and every widget haystack: lowercase + strip spaces.
static std::string NormalizeSearch(std::string s) {
    return NormalizeMenuSearch(std::move(s));
}
static const ImVec4 kBreadcrumbColor(0.6f, 0.6f, 0.6f, 1.0f); // muted gray for the result origin line

void ComboMenu::Draw() {
    if (auto ctx = Ship::Context::GetRawInstance(); ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui())
        ImGui::SetCurrentContext(ctx->GetWindow()->GetGui()->GetImGuiContext());
    DrawSongTravel();
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
    // Mirror Ship::Menu::Draw â€” skip GuiWindow::Draw's normal Begin/End wrapper so DrawElement
    // can open its own fullscreen overlay window instead of a floating one.
    if (!IsVisible()) {
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
    // context before any ImGui call here (same pattern soh.dll/2ship.dll use) â€” otherwise
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
                          ImVec4(0.035f, 0.045f, 0.08f, CVarGetFloat("gSettings.Menu.BackgroundOpacity", 0.94f)));
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
                mSearchReturn.Clear();
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
        if (hasSearch) {
            ImGui::SameLine();
            if (ImGui::Button("Clear##ComboSearch")) {
                mSearchBuf[0] = 0;
                mRuleFocus.Clear();
                mSearchReturn.Clear();
            }
        }
        if (!mSearchBuf[0] && mSearchReturn.HasResult()) {
            if (ImGui::Button("Back to search results", ImVec2(0, ImGui::GetFrameHeight() * 1.25f))) {
                mScope = mSearchReturn.scope;
                mHubActive = mSearchReturn.hub;
                std::snprintf(mSearchBuf, sizeof(mSearchBuf), "%s", mSearchReturn.query.c_str());
                mSearchReturn.Clear();
                mRuleFocus.Clear();
                mResetContentScroll = true;
                mLastNavSig = mScope + "|" + mHubActive;
            }
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

void ComboMenu::OpenSearchDestination(const std::string& scope, const std::string& page, const char* rule,
                                      const char* trick, int targetGame) {
    mSearchReturn.Capture(mSearchQuery, mScope, mHubActive);
    mSectionSelection[mScope] = mHubActive;
    mScope = scope;
    mHubActive = scope + "/" + page;
    mSectionSelection[mScope] = mHubActive;
    mRuleFocus.Set(rule);
    if (trick)
        FocusUnifiedTrick(trick);
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
    auto dismiss = [&]() {
        CVarSetInteger(guide->seenKey, 1);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    };
    bool open = true;
    const float width = std::min(ImGui::GetMainViewport()->WorkSize.x * 0.82f, ImGui::GetFontSize() * 37.0f);
    ImGui::SetNextWindowSize(ImVec2(width, 0), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Combo Ship guide", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", guide->title);
        ImGui::Separator();
        ImGui::Spacing();
        // Keep both actions visible on a phone; longer copy scrolls separately.
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
        if (ImGui::Button("Got it", ImVec2(-FLT_MIN, height))) {
            dismiss();
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button("View travel settings", ImVec2(-FLT_MIN, height))) {
            dismiss();
            OpenSearchDestination("randomizer", "Warps & travel");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (!open)
        dismiss();
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
    };
    std::vector<Hit> hits;
    auto addLink = [&](MenuSearchKind kind, const char* name, const char* keywords, const char* scope, const char* page,
                       const char* origin = "", int canonicalBonus = 0, int targetGame = -1, int targetSubpage = -1) {
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
                         targetSubpage });
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
    addLink(MenuSearchKind::Setting, "Victory condition",
            "triforce quest hunt ganon majora goal required total pieces win victory", "randomizer", "Logic & access",
            "Randomizer");
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
            auto keywords = row.value("tooltip", std::string()) + " " + row.value("heading", std::string()) +
                            " MM Majora Mask randomizer";
            if (name == "Clocks as Items (MM)")
                keywords += " clock shuffle time day night half days six cycle progressive ascending descending";
            if (name == "Farore's Wind (MM)")
                keywords += " farore fw spell magic warp cross game items extension";
            addLink(MenuSearchKind::Setting, name.c_str(), keywords.c_str(), "randomizer",
                    MenuRoute(false, "Rando", page).page.c_str(), "MM", 0, 1,
                    RandomizerWidgetGroup(false, page, 0, ""));
        }
    addLink(MenuSearchKind::Setting, "Cross-Games Farore's Wind",
            "farore fw spell warp travel between oot mm cross game world", "randomizer", "Logic & access",
            "Randomizer");
    addLink(MenuSearchKind::Setting, "Reachability", "guarantee beatable all checks reachable", "randomizer",
            "Logic & access", "Randomizer");
    for (const auto& rule : kGameplayRules) {
        if (IsLegacyGameplayRule(rule.name))
            continue;
        const auto keywords = std::string(rule.help) +
                              (rule.runtimeOnly ? " enhancement quality of life " : " randomizer logic ") +
                              GameplayRuleSearchAliases(rule.name);
        const auto* label = GameplayRuleMenuLabel(rule.name, rule.label, !rule.runtimeOnly);
        const int rank = MenuSearchRank(query, label, keywords.c_str(), MenuSearchKind::Rule);
        if (rank)
            hits.push_back({ MenuSearchKind::Rule, rank, label, rule.runtimeOnly ? "gameplay" : "randomizer",
                             rule.trickGoverned ? "Tricks & glitches" : GameplayRulePage(rule.name, !rule.runtimeOnly),
                             "", &rule });
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
                    // Non-interactive / opted-out kinds never appear in search.
                    if (wd.kind == CW_SEPARATOR || wd.kind == CW_SEPARATOR_TEXT || wd.kind == CW_TEXT ||
                        wd.kind == CW_CUSTOM || wd.hideInSearch)
                        continue;
                    const auto keywords = std::string(wd.tooltip ? wd.tooltip : "") + " " + src.label +
                                          (src.game == &model.Oot() ? " OoT" : " MM");
                    const int rank = MenuSearchRank(query, wd.name, keywords.c_str(), MenuSearchKind::Setting);
                    if (!rank)
                        continue;
                    // Dedupe shared settings (same backing CVar) across both games. Widgets with no
                    // CVar (buttons/window toggles) drive distinct callbacks, so scope their key per
                    // game â€” otherwise two same-named buttons would collapse and one would vanish.
                    std::string cvar = wd.cvar ? wd.cvar : "";
                    std::string key = cvar.empty() ? (std::string(src.label) + "|" + (wd.name ? wd.name : ""))
                                                   : NormalizeSearch(cvar);
                    if (!seen.insert(NormalizeSearch(key)).second)
                        continue; // a shared setting already shown from OOT

                    hits.push_back({ MenuSearchKind::Setting, rank, name, route.scope, route.page, src.label, nullptr,
                                     &game, &wd });
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
    const ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings;
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
                if (hit.widget) {
                    ImGui::PushID(hit.origin.c_str());
                    if (hit.scope == "randomizer")
                        ProfileUI::DrawWidget(*hit.widget, *hit.game);
                    else
                        RenderWidget(*hit.widget, *hit.game, 90 / cols);
                    ImGui::PopID();
                } else
                    ImGui::TextWrapped("%s", hit.label.c_str());
                ImGui::PushTextWrapPos(0.0f);
                if (hit.rule) {
                    if (hit.rule->runtimeOnly) {
                        ImGui::TextColored(kBreadcrumbColor, "Enhancements / %s", hit.page.c_str());
                        if (ImGui::Button("Open Enhancements", ImVec2(0, buttonHeight))) {
                            OpenSearchDestination("gameplay", hit.page, hit.rule->name, nullptr,
                                                  std::strstr(hit.rule->label, "(MM)") ? 1 : 0);
                            navigate = true;
                        }
                    } else if (hit.rule->trickGoverned) {
                        ImGui::TextColored(kBreadcrumbColor, "Randomizer / Tricks & glitches");
                        if (ImGui::Button("Open technique", ImVec2(-FLT_MIN, buttonHeight))) {
                            OpenSearchDestination(hit.scope, hit.page, nullptr, hit.rule->label);
                            navigate = true;
                        }
                    } else {
                        ImGui::TextColored(kBreadcrumbColor, "Randomizer / %s", hit.page.c_str());
                        const float pairWidth = ImGui::CalcTextSize("Open Randomizer").x +
                                                ImGui::CalcTextSize("Open Enhancements").x +
                                                ImGui::GetStyle().FramePadding.x * 4 + ImGui::GetStyle().ItemSpacing.x;
                        const bool pairFits = ImGui::GetContentRegionAvail().x >= pairWidth;
                        if (ImGui::Button("Open Randomizer", ImVec2(0, buttonHeight))) {
                            OpenSearchDestination("randomizer", GameplayRulePage(hit.rule->name, true), hit.rule->name,
                                                  nullptr,
                                                  IsSongTravelRule(hit.rule->name)       ? -1
                                                  : std::strstr(hit.rule->label, "(MM)") ? 1
                                                                                         : 0);
                            navigate = true;
                        }
                        if (pairFits)
                            ImGui::SameLine();
                        if (ImGui::Button("Open Enhancements", ImVec2(0, buttonHeight))) {
                            OpenSearchDestination("gameplay", GameplayRulePage(hit.rule->name, false), hit.rule->name,
                                                  nullptr,
                                                  IsSongTravelRule(hit.rule->name)       ? -1
                                                  : std::strstr(hit.rule->label, "(MM)") ? 1
                                                                                         : 0);
                            navigate = true;
                        }
                    }
                } else {
                    ImGui::TextColored(kBreadcrumbColor, "%s / %s", hit.origin.c_str(), hit.page.c_str());
                    if (!hit.widget && ImGui::Button("Open", ImVec2(-FLT_MIN, buttonHeight))) {
                        if (hit.targetSubpage >= 0)
                            (hit.page == "Hints & traps" ? mHintSection : mPoolSection) = hit.targetSubpage;
                        OpenSearchDestination(hit.scope, hit.page, nullptr,
                                              hit.kind == MenuSearchKind::Technique ? hit.label.c_str() : nullptr,
                                              hit.targetGame);
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

    // Widgets in the first cell of a 2-col stretch table, so they're half-width like the other panels.
    const ImGuiTableFlags anchorTableFlags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings;
    if (!ImGui::BeginTable("##anchorcols", 2, anchorTableFlags)) {
        return;
    }
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

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
    if (ImGui::Button("Restore Defaults", ImVec2(ImGui::GetContentRegionAvail().x / 2, 0))) {
        CVarSetString(AnchorCVar::Host, "anchor.hm64.org");
        CVarSetInteger(AnchorCVar::Port, 43383);
        CVarSetString(AnchorCVar::TeamId, "default");
        CVarSetString(AnchorCVar::RoomId, "");
        CVarSetString(AnchorCVar::Name, "");
        AnchorSaveCVars();
    }
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

    // --- Room settings (owner-only, non-global) â€” second column ---
    ImGui::TableSetColumnIndex(1);
    // ownerInfo bit0=isOwner, bit1=isGlobalRoom; 0 unless connected. Mirrors soh's AnchorAdminMenu gate.
    int ownerInfo = sSohAnchorGetOwnerInfo ? sSohAnchorGetOwnerInfo() : 0;
    bool isOwner = (ownerInfo & 1) != 0;
    bool isGlobalRoom = (ownerInfo & 2) != 0;
    if (connected && isOwner && !isGlobalRoom) {
        ImGui::SeparatorText("Room Settings (Admin Only)");

        ComboRando::ComboMenu_PushButton(theme);
        if (ImGui::Button("Clear All Team State", ImVec2(-1.0f, 0.0f))) {
            if (sSohAnchorClearTeamState)
                sSohAnchorClearTeamState();
        }
        ComboRando::ComboMenu_PopButton();

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

// Load the current pending/slot consolidated spoiler into the editable model (the #73 fix â€” the
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
    if (j.value("fileType", std::string()) != "ComboShipRandomizer") {
        sPlando.status = "Not a ComboShip seed (regenerate with this build).";
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
    // Shared Items: the loaded seed's own effective mask â€” plando doesn't change which families are
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
        sPlando.status = "Saved. Start a file to play it.";
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
        const bool canPlay = sRequestReload && onFileSelect;
        if (!canPlay)
            ImGui::BeginDisabled();
        ComboRando::ComboMenu_PushButton(theme);
        if (ImGui::Button("Save"))
            PlandoSavePlay();
        ComboRando::ComboMenu_PopButton();
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
void DrawGameplayRules(bool randomizer, const std::string& page, int world,
                       ComboRando::MenuRuleFocus* focus = nullptr) {
    auto& model = ComboMenuModel::Get();
    auto apply = [&](const char* name, int value) {
        const auto* rule = FindGameplayRule(name);
        if (randomizer || (ProfileUI::Active() && rule && !rule->runtimeOnly)) {
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
    auto read = [randomizer](const char* name, int fallback) {
        const auto* rule = FindGameplayRule(name);
        return randomizer || (ProfileUI::Active() && rule && !rule->runtimeOnly)
                   ? ProfileUI::ReadInteger(name, fallback)
                   : CVarGetInteger(name, fallback);
    };
    std::vector<const GameplayRule*> rules;
    for (const auto& rule : kGameplayRules) {
        if (rule.trickGoverned || (randomizer && rule.runtimeOnly) || IsLegacyGameplayRule(rule.name) ||
            GameplayRulePage(rule.name, randomizer) != page)
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
        const auto& rule = *item;
        ImGui::TableNextColumn();
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
        ImGui::BeginDisabled(required ||
                             (!randomizer && !rule.runtimeOnly && ProfileUI::Active() && !ProfileUI::Editable()));
        bool changed = false;
        if (rule.maximum == 1) {
            bool enabled = value != 0;
            changed = DrawRuleCheckbox(label.c_str(), &enabled);
            value = enabled;
        } else if (rule.minimum > 0) {
            if (value < rule.minimum)
                value = rule.fallback;
            ImGui::TextWrapped("%s", label.c_str());
            ImGui::SetNextItemWidth(-FLT_MIN);
            changed =
                ImGui::SliderInt("##RequiredCount", &value, rule.minimum, rule.maximum, "%d", ImGuiSliderFlags_NoInput);
        } else {
            const char* gibdo[] = { "Vanilla", "MM3D", "No trade" };
            const char* dog[] = { "Off", "With Mask of Truth", "Always" };
            const char* sticks[] = { "Normal", "Unbreakable", "Always burning" };
            const char* age[] = { "Off", "Ocarina of Time", "Ocarina of Time + Master Sword", "Either ocarina",
                                  "Either ocarina + Master Sword" };
            const char** modes = rule.maximum == 4                              ? age
                                 : !std::strcmp(rule.name, "gCheats.DekuStick") ? sticks
                                 : !std::strcmp(rule.name, "gEnhancements.DifficultyOptions.GibdoTradeSequence") ? gibdo
                                                                                                                 : dog;
            ImGui::TextWrapped("%s", label.c_str());
            ImGui::SetNextItemWidth(-FLT_MIN);
            changed = ImGui::Combo("##Ability", &value, modes, rule.maximum + 1);
        }
        if (focused)
            ImGui::PopStyleColor();
        ImGui::EndGroup();
        ImGui::EndDisabled();
        if (focus && focus->ConsumeScrollFor(rule.name))
            ImGui::SetScrollHereY(.15f);
        MenuTooltip(rule.help);
        if (changed) {
            if (randomizer)
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
    ImGui::EndTable();
    ImGui::Spacing();
}

void DrawPairedPreferences(const std::string& page) {
    struct Pair {
        const char* label;
        const char* oot;
        const char* mm;
        const char* help;
        const char* page;
    };
    static const Pair pairs[] = {
        { "Equip items with the D-pad", "gEnhancements.DpadEquips", "gEnhancements.Dpad.DpadEquips",
          "Assign items to the D-pad in the pause menu.", "Controls" },
        { "Allow unequipping items", "gEnhancements.ItemUnequip", "gEnhancements.Equipment.ItemUnequip",
          "Press an item's assigned button again in the pause menu to unequip it.", "Items & equipment" },
        { "Quick magic-arrow equip", "gEnhancements.SkipArrowAnimation", "gEnhancements.Equipment.MagicArrowEquipSpeed",
          "Skip the long animation when equipping magic arrows.", "Items & equipment" },
        { "Cycle arrow types", "gEnhancements.BowArrowCycle", "gEnhancements.PlayerActions.ArrowCycle",
          "Switch arrow types while aiming. Each game retains its native button behavior.", "Items & equipment" },
        { "Autosave", "gEnhancements.Autosave", "gEnhancements.Saving.Autosave",
          "OoT saves on its three-minute interval. MM uses its configured interval and persistent owl saves. Save "
          "manually before exiting.",
          "Saving" }
    };
    auto& model = ComboMenuModel::Get();
    for (const auto& pair : pairs) {
        if (page != pair.page)
            continue;
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
        MenuTooltip(pair.help);
    }
}
void DrawCombinedGoals() {
    auto save = [] {};
    const char* guarantees[] = { "All checks reachable", "Beatable seed" };
    int guarantee = std::clamp(ProfileUI::ReadInteger(kCombinedGuarantee, 0), 0, 1);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::Combo("##Reachability", &guarantee, guarantees, 2)) {
        ProfileUI::WriteInteger(kCombinedGuarantee, guarantee);
        SynchronizeGenerationGuarantee([](const char* k, int v) { return ProfileUI::ReadInteger(k, v); },
                                       [](const char* k, int v) { ProfileUI::WriteInteger(k, v); });
        save();
    }
    MenuTooltip("All checks reachable includes every enabled, non-excluded check. Beatable seed requires the selected "
                "victory goal. No Logic disables these guarantees.");
    const char* worlds[] = { "Start in Ocarina of Time", "Start in Majora's Mask", "Random starting world" };
    int world = std::clamp(ProfileUI::ReadInteger("gCombo.Rando.StartingGame", 0), 0, 2);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::Combo("##StartingWorld", &world, worlds, 3)) {
        ProfileUI::WriteInteger("gCombo.Rando.StartingGame", world);

        save();
    }
    MenuTooltip("Choose the world where the new file starts. Both worlds remain part of the seed.");
    const char* goals[] = { "Both final bosses", "Triforce Hunt", "Either final boss",
                            "Ganon only",        "Majora only",   "Triforce Quest" };
    int goal = std::clamp(
        ProfileUI::ReadInteger("gCombo.Rando.Goal", ProfileUI::ReadInteger("gCombo.Rando.TriforceHunt", 0) ? 1 : 0), 0,
        5);
    ImGui::TextUnformatted("Victory condition");
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::Combo("##VictoryCondition", &goal, goals, 6)) {
        ProfileUI::WriteInteger("gCombo.Rando.Goal", goal);
        ProfileUI::WriteInteger("gCombo.Rando.TriforceHunt", goal == 1);
        save();
    }
    MenuTooltip("Choose the goal for new seeds. Triforce Quest requires Power, Courage and Wisdom, placed in later "
                "progression spheres in different regions.");
    const bool hunt = goal == 1;
    if (hunt) {
        int required =
            std::clamp(ProfileUI::ReadInteger("gCombo.Rando.TriforceRequired", 15), 1, kMaxComboTriforcePieces);
        int total =
            std::clamp(ProfileUI::ReadInteger("gCombo.Rando.TriforceTotal", 15), required, kMaxComboTriforcePieces);
        ImGui::TextUnformatted("Required pieces");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##RequiredPieces", &required, 1, kMaxComboTriforcePieces, "%d",
                             ImGuiSliderFlags_NoInput)) {
            ProfileUI::WriteInteger("gCombo.Rando.TriforceRequired", required);
            if (total < required) {
                total = required;
                ProfileUI::WriteInteger("gCombo.Rando.TriforceTotal", total);
            }
            save();
        }
        ImGui::TextUnformatted("Pieces in the pool");
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::SliderInt("##TotalPieces", &total, required, kMaxComboTriforcePieces, "%d",
                             ImGuiSliderFlags_NoInput)) {
            ProfileUI::WriteInteger("gCombo.Rando.TriforceTotal", total);
            save();
        }
    }
    ImGui::SeparatorText("World access");
    // OoTMM main.cross crossGameFw: available only with spellWindMm.
    const bool windMm = ProfileUI::ReadInteger("gRando.Options.RO_MM_SPELL_WIND", 0) != 0;
    bool crossFw = windMm && ProfileUI::ReadInteger("gRando.Options.RO_CROSS_GAME_FARORE", 0) != 0;
    ImGui::BeginDisabled(!windMm);
    if (DrawRuleCheckbox("Cross-Games Farore's Wind", &crossFw)) {
        ProfileUI::WriteInteger("gRando.Options.RO_CROSS_GAME_FARORE", crossFw);
        save();
    }
    ImGui::EndDisabled();
    MenuTooltip(windMm ? "Controls whether you can use Farore's Wind to warp between OoT and MM."
                       : "Controls whether you can use Farore's Wind to warp between OoT and MM.\n\nRequires Farore's "
                         "Wind (MM) under Items & checks > Majora's Mask.");
}
void DrawSharedItems() {
    const int columns = MenuColumns();
    if (!ImGui::BeginTable("##SharedItems", columns,
                           ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings))
        return;
    for (int i = 0; i < SF_COUNT; ++i) {
        ImGui::TableNextColumn();
        if (i == SF_OCARINA || !SharedFamilyByIndex(i).publicOption)
            continue; // private native features await runtime validation
        const auto& family = SharedFamilyByIndex(i);
        bool enabled = ProfileUI::ReadInteger(family.cvar, 0) != 0;
        ImGui::PushID(family.cvar);
        const bool unavailable =
            (family.requiresButtons && (!ProfileUI::ReadInteger("gRandoSettings.ShuffleOcarinaButtons", 0) ||
                                        !ProfileUI::ReadInteger("gRando.Options.RO_SHUFFLE_OCARINA_BUTTONS", 0))) ||
            (i == SF_SPELL_WIND &&
             !ProfileUI::ReadInteger("gRando.Options.RO_MM_SPELL_WIND", 0)); // OoTMM: requires spellWindMm
        ImGui::BeginDisabled(unavailable);
        if (DrawRuleCheckbox(family.label, &enabled)) {
            ProfileUI::WriteInteger(family.cvar, enabled);
        }
        ImGui::EndDisabled();
        MenuTooltip(family.tooltip);
        ImGui::PopID();
    }
    ImGui::EndTable();
}
void DrawCosmeticSync() {
    bool linked = CVarGetInteger("gCombo.Rando.SyncCosmetics", 0) != 0;
    if (DrawRuleCheckbox("Match cosmetics between games", &linked)) {
        CVarSetInteger("gCombo.Rando.SyncCosmetics", linked);
        AnchorSaveCVars();
    }
    MenuTooltip("Use matching randomized colors and trails in both games when generating a new seed. This changes "
                "appearance only.");
}
void DrawCommunityPresets() {
    static std::string applied;
    ResolveComboGenSyms();
    static auto native = (void (*)(int))Combo_ResolveSym("2ship", "MM_ApplyComboCommunityPreset");
    const bool ready = sIsOnFileSelect && sIsOnFileSelect() && sTrigger && native;
    const auto* progress = sGetProgress ? sGetProgress() : nullptr;
    const bool running = progress && progress->running.load();
    const int columns = ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 60.0f ? 3 : MenuColumns(20.0f);
    if (ImGui::BeginTable("##CommunityPresets", columns,
                          ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings)) {
        for (const auto& preset : kCommunityPresets) {
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(preset.id));
            ImGui::SeparatorText(preset.name);
            ImGui::TextWrapped("%s", preset.summary);
            ImGui::BeginDisabled(!ready || running);
            if (ImGui::Button("Apply preset", ImVec2(-FLT_MIN, ImGui::GetFrameHeight() * 1.25f))) {
                // Presets configure a new seed only. Clearing these precise namespaces
                // also prevents options from a previously chosen preset leaking into it.
                CVarClearBlock("gRandoSettings");
                CVarClearBlock("gRando.Options");
                CVarClearBlock("gCombo.Tricks");
                for (const auto& rule : kGameplayRules)
                    SetLogicRule(rule, 0, [](const char* k, int v) { CVarSetInteger(k, v); });
                for (const auto& option : CommunityPresetOptions(preset.id))
                    CVarSetInteger(option.key, option.value);
                const auto& songs = *FindGameplayRule("gCombo.Enhancements.ForeignSongs");
                SetLogicRule(songs, preset.id != CommunityPreset::Default,
                             [](const char* k, int v) { CVarSetInteger(k, v); });
                native(static_cast<int>(preset.id));
                if (sRefreshStartingGameUI)
                    sRefreshStartingGameUI();
                SynchronizeGenerationGuarantee([](const char* k, int v) { return CVarGetInteger(k, v); },
                                               [](const char* k, int v) { CVarSetInteger(k, v); });
                applied = preset.name;
                AnchorSaveCVars();
            }
            ImGui::EndDisabled();
            MenuTooltip(preset.help);
            ImGui::Spacing();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::Spacing();
    ImGui::TextDisabled("Adapted from OoTMM community presets");
    MenuTooltip("These are supported Combo Ship adaptations. Hover, focus or touch each Apply button for its details. "
                "Source: OoTMM/OoTMM packages/generator/lib/combo/presets.ts.");
    if (!ready)
        ImGui::TextDisabled("Return to file select to configure a new seed.");
    if (!applied.empty())
        ImGui::Text("%s applied. Fine-tune under Randomizer, then Generate.", applied.c_str());
}

void DrawScreenAndHud() {
#ifdef __ANDROID__
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
    static int game = ComboUI::GetForegroundGame();
    MenuWorldButtons(game);
    DrawHudLayout(game);
}

void AppendFeatureWidgets(const HubEntry::Source& source, const std::string& page, int subpage,
                          std::vector<CwWidget>& output, uint32_t& columns) {
    CwSidebar side = *source.sidebar;
    std::vector<CwWidget> grouped;
    std::string currentHeading;
    int sourceColumn = -1;
    for (int i = 0; i < side.widgetCount; ++i) {
        const auto& widget = side.widgets[i];
        if (sourceColumn != widget.column) {
            sourceColumn = widget.column;
            currentHeading.clear();
        }
        if (widget.kind == CW_SEPARATOR_TEXT)
            currentHeading = widget.name ? widget.name : "";
        if (MenuGroupRoute(source.oot, source.section, side.sidebarName, sourceColumn, currentHeading,
                           widget.cvar ? widget.cvar : "")
                .page == page)
            grouped.push_back(widget);
    }
    side.widgets = grouped.data();
    side.widgetCount = (int)grouped.size();
    std::vector<CwWidget> filtered;
    if (source.oot && side.sidebarName && std::strcmp(side.sidebarName, "Fixes") == 0 &&
        (page == "Fixes" || page == "Restorations")) {
        for (int i = 0; i < side.widgetCount; ++i) {
            const auto& w = side.widgets[i];
            if (FeatureMenuRoute(true, "Enhancements", side.sidebarName, w.column).page != page)
                continue;
            filtered.push_back(w); // preserve native index, CVar and callbacks
            if (page == "Restorations")
                filtered.back().column = 0;
        }
        side.widgets = filtered.data();
        side.widgetCount = (int)filtered.size();
        side.columnCount = page == "Restorations" ? 1 : 2;
    }
    if (page == "General") {
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
    for (int i = 0; i < side.widgetCount; ++i)
        if (!IsRetiredNativeRuleControl(side.widgets[i].cvar))
            output.push_back(side.widgets[i]);
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
                mSearchReturn.Clear();
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
        ImGui::SeparatorText(active->kind == HubEntry::COMBO_HOME ? "Community presets" : active->label.c_str());
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
                                if (name == "Menu Theme" || name == "Menu Controller Navigation" ||
                                    name == "Menu Background Opacity" || name == "ImGui Menu Scaling")
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
            case HubEntry::COMBO_SESSION:
                ImGui::TextWrapped("Save in-game before restarting or exiting.");
                if (ImGui::Button("Restart game"))
                    ImGui::OpenPopup("Restart game?");
                if (ImGui::Button("Exit Combo Ship"))
                    ImGui::OpenPopup("Exit Combo Ship?");
                for (int i = 0; i < 2; ++i) {
                    if (ImGui::BeginPopupModal(i == 0 ? "Restart game?" : "Exit Combo Ship?", nullptr,
                                               ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::TextUnformatted("Unsaved progress will be lost.");
                        if (ImGui::Button("Cancel"))
                            ImGui::CloseCurrentPopup();
                        ImGui::SameLine();
                        if (ImGui::Button(i == 0 ? "Restart" : "Exit")) {
                            ImGui::CloseCurrentPopup();
                            if (i == 1)
                                Ship::Context::GetRawInstance()->GetWindow()->Close();
                            else if (auto console = std::static_pointer_cast<Ship::ConsoleWindow>(
                                         Ship::Context::GetRawInstance()->GetWindow()->GetGui()->GetGuiWindow(
                                             "Console"))) {
                                console->Dispatch("reset");
                                Hide();
                            }
                        }
                        ImGui::EndPopup();
                    }
                }
                break;
            default: {
                if (active->label == "Tricks & glitches") {
                    ProfileUI::DrawTricks();
                    break;
                }
                if (active->label == "Screen & HUD") {
                    DrawScreenAndHud();
                    if (ImGui::Button("Advanced graphics")) {
                        mScope = "media";
                        mHubActive = "media/Graphics";
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
                } else if (active->label == "Warps & travel") {
                    selectedGame = -1;
                } else if (active->label == "Gameplay rules" || active->label == "Difficulty" ||
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
                DrawGameplayRules(seedOptions, active->label, selectedGame, &mRuleFocus);
                ImGui::EndDisabled();
                if (!seedOptions)
                    DrawPairedPreferences(active->label);
                if (mScope == "media" && active->label == "Appearance")
                    DrawCosmeticSync();
                const auto bodyKey = std::make_pair(selectedGame, subpage);
                auto body = active->bodies.find(bodyKey);
                if (body == active->bodies.end()) {
                    HubEntry::Body built;
                    for (const auto& source : active->sources) {
                        if (source.oot != (selectedGame == 0))
                            continue;
                        built.game = source.game;
                        if (!source.oot && subpage >= 0 &&
                            RandomizerWidgetGroup(false, source.sidebar->sidebarName, 0, "") != subpage)
                            continue;
                        AppendFeatureWidgets(source, active->label, subpage, built.widgets, built.columns);
                    }
                    if (active->label == "Controls" || active->label == "Saving") {
                        built.columns = 1;
                        for (auto& widget : built.widgets)
                            widget.column = 0;
                        std::stable_partition(built.widgets.begin(), built.widgets.end(),
                                              [](const CwWidget& widget) { return widget.kind != CW_CUSTOM; });
                    }
                    body = active->bodies.emplace(bodyKey, std::move(built)).first;
                }
                auto& content = body->second;
                if (content.game) {
                    const CwSidebar feature = { active->label.c_str(), content.columns, content.widgets.data(),
                                                (int)content.widgets.size() };
                    if (seedOptions)
                        ProfileUI::DrawSidebar(feature, *content.game, selectedGame);
                    else
                        RenderSidebarWidgets(feature, *content.game);
                }
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

void ComboMenu::DrawComboPanel() {
    ResolveComboGenSyms();
    // Seed field -> shared CVar the generator reads (same source the native file-select
    // "Generate a new seed" option uses).
    // Keep restored custom seeds visible too; a blank-looking field must never hide a fixed seed.
    std::strncpy(mSeedBuf, CVarGetString("gGeneral.ComboSeed", ""), sizeof(mSeedBuf) - 1);
    mSeedBuf[sizeof(mSeedBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(std::min(500.0f, ImGui::GetContentRegionAvail().x * 0.8f));
    if (ImGui::InputTextWithHint("Seed", "(blank = random)", mSeedBuf, sizeof(mSeedBuf))) {
        CVarSetString("gGeneral.ComboSeed", mSeedBuf);
    }
    MenuTooltip("Leave this blank for a new random seed. The same text and settings repeat a seed. Generate before "
                "starting a new randomizer file; existing saves keep their original seed.");

    const ComboRando::ComboGenProgress* p = sGetProgress ? sGetProgress() : nullptr;
    const bool running = p && p->running.load();
    // Generation may only run at the OOT file-select screen, so the worker can't race a live game
    // tick (the prior off-thread crash class).
    const bool onFileSelect = sIsOnFileSelect && sIsOnFileSelect();
    const bool canGenerate = sTrigger && onFileSelect && !running;

    if (!canGenerate)
        ImGui::BeginDisabled();
    ComboMenu_PushButton(ComboMenu_ThemeColor());
    if (ImGui::Button("Generate combined seed", ImVec2(-FLT_MIN, 0))) {
        CVarSetString("gGeneral.ComboSeed", mSeedBuf);
        sTrigger(); // non-blocking: launcher spawns the worker; the main loop keeps running
    }
    ComboMenu_PopButton();
    if (!canGenerate)
        ImGui::EndDisabled();

    if (!sTrigger) {
        ImGui::TextUnformatted("Generate unavailable (soh.dll export not found).");
    } else if (!onFileSelect && !running) {
        ImGui::TextWrapped("Generate from the file list or an empty file's randomizer screen. "
                           "Finish or cancel name entry first.");
    }

    // Live progress + result, polled from the launcher-owned progress each frame.
    if (p) {
        if (running) {
            int placed = p->placed.load();
            int total = p->total.load();
            float frac = total > 0 ? (float)placed / (float)total : 0.0f;
            ImGui::TextUnformatted(ComboGenProgress::PhaseLabel(p->phase.load()));
            if (p->phase.load() == 3)
                ImGui::TextWrapped("Items are placed. Building hints and preparing both games; please wait.");
            else
                ImGui::ProgressBar(frac, ImVec2(360.0f, 0.0f));
            ImGui::Text("%d / %d", placed, total);
        } else if (p->done.load()) {
            if (p->success.load()) {
                ImGui::TextUnformatted("Seed ready for OoT + MM. Start Randomizer on a new file.");
                // Show the reproducible seed token (the input string, not the internal masterSeed
                // hash). Paste it into the Seed field + same settings to reproduce â€” shareable
                // without sharing a save file.
                ImGui::Text("Seed: %s", p->seedStr);
                ImGui::SameLine();
                if (ImGui::SmallButton("Copy")) {
                    ImGui::SetClipboardText(p->seedStr);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Reuse")) {
                    std::strncpy(mSeedBuf, p->seedStr, sizeof(mSeedBuf) - 1);
                    mSeedBuf[sizeof(mSeedBuf) - 1] = '\0';
                    CVarSetString("gGeneral.ComboSeed", mSeedBuf);
                }
                ImGui::Text("OOT checks: %d", p->ootCheckCount.load());
                ImGui::Text("MM checks: %d", p->mmCheckCount.load());
                ImGui::Text("Cross-game placements: %d", p->foreignCount.load());
            } else if (p->CancelRequested()) {
                ImGui::TextUnformatted("Generation cancelled. Change settings or try again when ready.");
            } else {
                ImGui::TextWrapped("Generation stopped: %s", p->error);
            }
        }
    }
    ImGui::Spacing();
    static const char* goalNames[] = { "Both final bosses", "Triforce Hunt", "Either final boss",
                                       "Ganon only",        "Majora only",   "Triforce Quest" };
    static const char* worldNames[] = { "Ocarina of Time", "Majora's Mask", "Random" };
    ImGui::TextWrapped(
        "Starting world: %s. Goal: %s.", worldNames[std::clamp(CVarGetInteger("gCombo.Rando.StartingGame", 0), 0, 2)],
        goalNames[std::clamp(
            CVarGetInteger("gCombo.Rando.Goal", CVarGetInteger("gCombo.Rando.TriforceHunt", 0) ? 1 : 0), 0, 5)]);
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

    // Item Tracker: shared appearance + game-swap manager (see ComboTrackerSwap.h).
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
    // hidden, so force it shut here â€” a config that persisted gOpenWindows.HintTracker=1 would
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
