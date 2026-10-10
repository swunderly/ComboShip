# Combo Ship 0.1.11 - compact 2S2H-style tracker

Install `ComboShip-Android-0.1.11-dev-arm64.apk` over your existing installation.

## What changed

- Both games now use a compact tracker inspired by your 2S2H screenshot: a black side panel, collapsible area headings with collected/total counts, small icons and single-line check names.
- The game fits beside the panel. Toggle the tracker off to restore the full game view. Drag the divider to change the panel width; the layout is saved between launches.
- Skipped checks have a purple `(Skipped)` label. Available checks are white, unavailable checks are gray, and collected checks are green. Skipping does not increase the collected count.
- Tap a check to open its details and skip/unskip actions in a small popup.
- Search, totals and filter buttons are hidden by default to keep the list compact. Enable **Show search and filter toolbar** when you want them.
- **Settings → Check Tracker** remains one shared page for both games. **Side panel** is the new default style; **Floating window** and **Overlay** remain available. Your existing shortcut, visibility mode and filters are retained.

## Quick setup

Open **Settings → Check Tracker** and enable the tracker. Choose **Button toggle** and **Side panel**. The default shortcut is **L**: press once to open, and again to close. Change it under **Change shortcut**. If you move the panel over a touch button, use a shortcut whose touch button remains outside the panel, or use your hardware controller.

To reproduce the minimal current-area view in your screenshot, enable **Only show current area**, hide collected checks and show skipped checks. Leave **Show search and filter toolbar** off.

## Please test on your phone

1. Open and close the panel repeatedly in both games. The game should reclaim the space when the panel closes.
2. Resize the divider, close and reopen the tracker, then restart the app. Check that its width is retained.
3. Expand an area, tap a check, skip/unskip it, and verify the purple label and unchanged collected count.
4. Try **Only show current area**, then the optional toolbar. Confirm the list and shortcut behave the same in both games.

## Validation

Native ARM64 build, Android build/lint, APK signature, library/export checks, 16 KiB alignment and support-file hashes pass. The compact panel, L toggle, details and skip/unskip were inspected in both engines in the local Android 15 emulator. Dock resizing, close-button behavior, optional toolbar and switching between floating and docked styles were also exercised. The restart test led to a fix that restores the saved dock rather than creating another split.

These are targeted emulator checks. Physical-phone and hardware-controller testing are still needed. The tracking data, game hooks and save formats are unchanged. Save normally before exiting, especially after changing skipped checks in MM. ROMs, extracted game archives and private test data are excluded from the APK and source package.

APK SHA-256: `7137450e3f8e28f7a076a6511c0e8f4f22989ef9c045be7c8716573ee222ed04`
