# Combo Ship 0.1.9 — movable Menu button

Install `ComboShip-Android-0.1.9-dev-arm64.apk` over the existing app. The package identity and signing key are the same, so your games, saves and settings remain in place.

The Menu button can now be moved and saved with the rest of the touch layout:

1. Pause the game, then hold **Menu** and choose **Move controls (pause the game first)**.
2. Drag the Menu button, which is labeled **Done** while editing, to the desired spot.
3. Release your finger. The editor stays open so you can adjust other buttons.
4. Tap **Done** in its new position to save and leave the editor.

The position survives restarting the app and is stored with the current screen shape and face-button layout. **Reset current screen layout** restores Menu to the top center. Menu stays fully within the screen and takes priority if it overlaps another button. Normal tap and hold actions continue to work at its new position.

## Validation

- Android build and lint passed with zero errors.
- Passed all 50 Android touch-overlay checks. The 14 new checks cover Menu dragging, tapping Done with slight finger movement, persistence, hidden controls, edge limits, overlap, cancellation, layout switching and reset. The existing 36 invisible-stick and multi-touch checks also passed.
- Installed over 0.1.8 in the Android 15 emulator. In the running app, moved Menu, confirmed releasing the drag kept the editor open, tapped Done to save, and restarted. Verified Menu stayed in the new position and both tap and hold opened their respective menus.
- All eight native libraries and the support manifest are byte-identical to 0.1.8. APK signature, runtime and ROM exclusion checks passed. The installed APK hash matches this delivery.
- Existing test saves remained byte-identical. Restored and byte-verified the original game and touch settings after testing. No crash was recorded.

Physical-phone validation remains with you. Please try moving Menu to your preferred position, restart, and check that tapping and holding it both work there.

APK size: 45,057,719 bytes.

SHA-256: `a3aaedd82a27422165835e2d7b5656effaf68df3dcf48d12c6078f1a07605fe9`
