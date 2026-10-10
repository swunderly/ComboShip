// combo/gui/ComboTrackerSwap.h
//
// Legacy enable migration and combined Triforce overlay. Main tracker visibility
// and Android touch access are managed by ComboTrackerControls.
#pragma once

namespace ComboTracker {

// Tracker kinds (indices into ComboTrackerCommon.h kKinds).
enum SwapTrackerId { kSwapItem = 0, kSwapCheck = 1, kSwapCount = 2 };

// Clear the retired swap-era CVars, seed the combo enables from the per-game CVars an existing
// config may carry (once), and register the combined Triforce overlay window.
void RegisterSwap();

// The FOREGROUND game's pause state, from its DLL (the dormant game's own is stale). Fails open
// (paused) so a missing module/export degrades to always-show. Shared with the combo Hint Tracker.
bool ForegroundPaused(int fg);

} // namespace ComboTracker
