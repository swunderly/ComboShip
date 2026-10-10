# ComboShip Android 0.1.5 — remove the broken startup import prompt

Install `ComboShip-Android-0.1.5-dev-arm64.apk` over the existing app to keep your ROMs, saves and settings.

The **ComboShip Settings Import** screen used a desktop file picker and disabled its Browse buttons on Android. Android startup now skips this optional screen. After the two ROMs are supplied and prepared, the game opens. Existing installations continue to launch directly into the game.

Importing another Ship app's settings through those buttons is not supported on Android. This update removes the unusable startup offer; it does not add settings migration. The existing in-game settings remain available.

## Validation

- Native build, Android assembly and lint passed with zero errors.
- Installed over 0.1.4 in the Android 15 emulator.
- Started with no `comboship.json`, retaining the test ROMs and extracted archives. The app created fresh settings and reached the OoT title sequence without showing or interacting with the settings-import dialog.
- Restored and byte-verified the original test saves and settings, then confirmed a normal cold launch went directly into the game.
- Verified the patch replay, APK signature, native libraries and entry points, support hashes, 16 KiB ELF alignment and ROM exclusion checks.

This is a targeted emulator startup test. ROM extraction and seed generation were not retested for this change. Generation code is unchanged from 0.1.4.

## Phone check

Install the update, open ComboShip, and confirm that the settings-import prompt no longer appears and your saves remain listed. If the old prompt is currently open, **Skip** should let the older build continue.

APK size: 44,193,966 bytes.

SHA-256: `f89cc0d8a458882ecaf8340c928cb4e12a8a74a317743d3ed4e9b30acf9080eb`
