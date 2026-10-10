# Combo Ship Android 0.2.15 validation

APK SHA-256: `8f5f197e8d859b9c8fe2dcfb0128ce17599569ce4159c205fdb2ef6ba86a2f4c`

Version 0.2.15, version code 40, package `org.comboship.android`, label `Combo Ship`. This is a narrow MM save compatibility hotfix to 0.2.14.

## Save compatibility

- 38 host tests pass, including a new test compiling the actual native MM JSON reader and complete Save/SaveContext structures.
- Legacy 83-event and 84-event arrays retain every existing entry; the current 88-event format round-trips unchanged. Only appended entries are initialized.
- Tests preserve inventory, songs, boss remains, tokens, owls, obtained/skipped/seen check data, seed settings, and pending traps.
- Truncated, malformed, future-length, and missing-inventory payloads remain rejected.
- Both pages of a private historical save pass through the native reader. No save or seed placement data is included in the public source archive.

## Installed APK regression

A disposable read-only Android API 35 emulator reproduced the 0.2.14 failure with an older 83-event save: both MM pages failed with an array index 83 error. Its stored MM section was unchanged. The same save loads on 0.2.15 without that error.

The rebuilt installed APK was checked for:

- MM inventory icons, owned songs, and owl unlocks displayed correctly while OoT is active.
- Switching foreign-song travel off and on during an existing run, leaving Include in logic unchecked and mixed destinations disabled.
- Playing Song of Soaring in OoT, completing native playback, selecting an owl on the native map, confirming travel, and arriving in MM.
- Saving at a native MM owl statue and restarting the app. Inventory, songs, owl flags, permanent flags, and collected check state remained intact. The original 83 event entries were preserved and the five appended entries were initialized to zero.
- An intentionally truncated save failed to load, kept its stored MM data unchanged, and showed the editor's unloaded-save warning with editing controls blocked.

This verifies the reproduced upgrade failure. The user's physical phone save was not inspected, and its individual recovery is not certified by the emulator result.

## Build and package

- ARM64 native build, Gradle assembleDebug, and lintDebug succeed; no lint errors or fatal issues. There are 84 existing non-error lint findings.
- Fresh production overlay replay matches all 240 patched native files. Only the MM JSON reader and MM editor change from 0.2.14.
- Installed APK bytes match the candidate hash. Signature verification passes and the certificate matches previous releases.
- Eight ARM64 libraries, 231 exported callbacks, and all 9,036 support files are verified; 16 KiB alignment passes.
- ROMs, extracted game archives, private saves, logs, credentials, and emulator state are excluded from the release source package.

This hotfix does not add new gameplay or logic features, and the checks above are not a fresh certification of every feature from prior releases.
