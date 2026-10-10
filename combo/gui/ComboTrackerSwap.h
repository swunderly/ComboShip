// combo/gui/ComboTrackerSwap.h
//
// Legacy enable migration and combined Triforce overlay. Main tracker visibility
// and Android touch access are managed by ComboTrackerControls.
#pragma once

namespace ComboTracker {

// Tracker kinds (indices into ComboTrackerCommon.h kKinds).
enum SwapTrackerId { kSwapItem = 0, kSwapCheck = 1, kSwapCount = 2 };

// Register the logic-only reconcile window into the shared Gui, clear the retired swap-era CVars,
// and seed the master enables from the per-game CVars an existing config may carry.
void RegisterSwap();

// Master "is this tracker on" switch (both games) for the combo menu panels.
bool GetMasterVisible(int tracker);
void SetMasterVisible(int tracker, bool visible);

// The FOREGROUND game's pause state, from its DLL (the dormant game's own is stale). Fails open
// (paused) so a missing module/export degrades to always-show. Shared with the combo Hint Tracker.
bool ForegroundPaused(int fg);

} // namespace ComboTracker
