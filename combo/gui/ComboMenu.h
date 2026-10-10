// combo/gui/ComboMenu.h
// ComboShip: combo-owned unified settings menu, hosted in comboui.dll.
#pragma once

#include <libultraship/libultraship.h>
#include "ComboExport.h"
#include <string>
#include <map>
#include <utility>
#include <vector>
#include "gui/ComboGenProgress.h"
#include "ComboMenuSearch.h"

namespace ComboRando {

class ComboMenu final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~ComboMenu() override {
    }

    // Override GuiWindow::Draw so we render as a fullscreen overlay (like the port menu)
    // instead of a normal floating window. DrawElement opens its own window.
    void Draw() override;

    // Open the menu with the Randomizer tab active (file-select "Open Randomizer Settings").
    void OpenAtRandomizer();

  protected:
    void InitElement() override {
    }
    void DrawElement() override; // fullscreen window: scope-tab strip + active panel
    void UpdateElement() override {
    }

  private:
    void DrawSharedPanel();                  // hub for the Settings + Randomizer tabs (by mScope)
    void DrawComboPanel();                   // cross-world Generate (seed + button + progress)
    // Global menu search: ranks canonical rules, techniques, widgets and pages;
    // native controls remain editable through their existing callbacks.
    // Drawn in the active scope's content area, so the left navigation panel stays visible.
    void DrawSearchResults(const std::string& query);
    void OpenSearchDestination(const std::string& scope, const std::string& page, const char* rule = nullptr,
                               const char* trick = nullptr, int targetGame = -1);
    void DrawScopeHelp();

    char mSearchBuf[256] = { 0 }; // raw global-search input; normalized into mSearchQuery each frame
    std::string mSearchQuery;     // raw multi-word query this frame ("" = not searching)
    MenuRuleFocus mRuleFocus;
    MenuSearchReturn mSearchReturn;
    std::string mLastSearchQuery;
    std::string mGuideScope;
    bool mHelpRequested = false;
    bool mResetContentScroll = false;
    bool mResetOptionsScroll = false;
    char mSeedBuf[128] = { 0 };
    std::string mHubActive;        // active hub entry key ("group/label"); persists across frames
    std::string mScope;            // active top-level tab: feature scope
    std::string mLastNavSig;       // last frame's nav location; a change clears the search box
    std::map<std::string, std::string> mSectionSelection;
    int mPreferenceGame = 0; // 0 follows foreground; 1 OoT; 2 MM
    int mRandomizerGame = 0; // Seed configuration stays on the selected world, independent of gameplay.
    int mPoolSection = 0;    // Shared Checks / Items selection.
    int mHintSection = 0;    // Shared Hints / Traps selection.
};

} // namespace ComboRando

extern "C" COMBO_EXPORT void ComboUI_Register(void);
