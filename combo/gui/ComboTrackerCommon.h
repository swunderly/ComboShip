// combo/gui/ComboTrackerCommon.h
//
// Tracker-window plumbing shared across comboui: the CVar + Gui-map levers for the per-game tracker
// windows, plus the active-slot resolve every panel needs.

#pragma once

#include <libultraship/libultraship.h>
#include "ComboResolve.h" // Combo_ResolveSym (SOH_GetActiveFileNum)

namespace ComboTracker {

// Active OOT save slot (0-2), or -1 at the title/file-select screens. The slot is combo-wide, so
// every comboui caller shares this one latched resolve (inline statics are per-DLL, not per-TU).
inline int OotActiveSlot() {
    static int (*sGetFileNum)(void) = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        sGetFileNum = (int (*)(void))Combo_ResolveSym("soh", "SOH_GetActiveFileNum");
    }
    if (sGetFileNum) {
        int slot = sGetFileNum();
        return (slot >= 0 && slot <= 2) ? slot : -1;
    }
    return -1;
}

struct Win {
    const char* cvar; // visibility CVar (source of truth; MM's CheckTracker reads it directly)
    const char* name; // registered GuiWindow name (map key in the shared Gui)
};

// [game][window]: 0 = OOT, 1 = MM. ComboTrackerControls and ComboCheckTracker draw the trackers
// and keep these native windows hidden; the item settings window follows the foreground game.
inline constexpr Win kTrackers[2][4] = {
    {
        { "gOpenWindows.ItemTracker", "Item Tracker" },
        { "gOpenWindows.ItemTrackerSettings", "Item Tracker Settings" },
        { "gOpenWindows.CheckTracker", "Check Tracker" },
        { "gOpenWindows.CheckTrackerSettings", "Check Tracker Settings" },
    },
    {
        { "gWindows.ItemTracker", "Item Tracker##MM" },
        { "gWindows.ItemTrackerSettings", "Item Tracker Settings##MM" },
        { "gWindows.CheckTracker", "Check Tracker##MM" },
        { "gWindows.CheckTrackerSettings", "Check Tracker Settings##MM" },
    },
};

inline constexpr int kItemTracker = 0;
inline constexpr int kCheckTracker = 2;

// The item and check trackers' combo enable CVars, which RegisterSwap's one-shot migration seeds
// from the per-game window CVars of older configs.
struct Kind {
    int column;              // kTrackers column of the native main tracker window
    const char* enabledCvar; // combo tracker on/off, spans both games
};

inline constexpr Kind kKinds[2] = {
    { kItemTracker, "gCombo.Tracker.Enabled" },
    { kCheckTracker, "gCombo.CheckTracker.Enabled" },
};

// Write the CVar (covers MM's CheckTracker, whose Draw() reads it directly) and mirror it onto
// the window object so mIsVisible-gated Draws react this same frame.
inline void SetTracker(const Win& w, bool visible) {
    CVarSetInteger(w.cvar, visible ? 1 : 0);
    auto ctx = Ship::Context::GetRawInstance();
    if (ctx && ctx->GetWindow() && ctx->GetWindow()->GetGui()) {
        if (auto win = ctx->GetWindow()->GetGui()->GetGuiWindow(w.name)) {
            if (visible) {
                win->Show();
            } else {
                win->Hide();
            }
        }
    }
}

} // namespace ComboTracker
