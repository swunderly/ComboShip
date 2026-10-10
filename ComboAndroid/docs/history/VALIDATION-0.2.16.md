# Combo Ship Android 0.2.16 validation

APK SHA-256: `16756362a0cdfcbbc5df4fb695e24a3f3fede7298b941249ee3043e4bfa956c1`

Version 0.2.16, version code 41, package `org.comboship.android`, label `Combo Ship`. This release fixes OoT tracker knowledge persistence.

## Reproduced failure

A disposable read-only Android API 35 emulator running the released 0.2.15 APK loaded eight previously observed shop rewards. Native Requiem travel from MM to OoT returned to Desert Colossus without revisiting those shops. The subsequent tracker save retained their check statuses but erased all eight knowledge sources, item IDs, and names.

The return path loads OoT before firing its exit hook. That hook cleared the newly loaded knowledge. Return to Spawn uses the same load-before-exit ordering. The fix removes that reset from exit cleanup while retaining the resets used when initializing or loading a save.

## Regression checks

- All 39 host tests pass. The new regression compiles the production tracker lifecycle functions and native JSON serializer; it fails with the old cleanup and passes with the fix.
- Full and partial saves, load-exit-save ordering, and cold reload preserve knowledge for shops, Skulltula rewards, freestanding rewards, and MM items displayed in OoT.
- Hint provenance and physical sightings remain distinct; trap appearances, collected/saved/scummed/skipped states, unseen checks, and profile isolation are covered.
- In the installed 0.2.16 APK, the same eight rewards retain their sources, item IDs, names, and check statuses after native MM-to-OoT travel, a normal in-game save, and a full app restart away from the shops.
- Selecting Return to Spawn in the native save menu also preserves those eight records.
- Freshly rendered shop rewards are still learned and written to the save. Previously unseen shops remain unknown when loading away from their models.

The tests use private fixtures only in a disposable emulator. Original emulator files were checked byte-for-byte, and no user phone save was modified or inspected. Names already erased and saved by an older build are not recoverable from this fix alone; visible rewards can be learned again by revisiting them.

## Build and package

- ARM64 native build, Gradle assembleDebug, and lintDebug succeed. No lint errors or fatal issues; 84 existing non-error findings remain.
- A fresh production overlay replay matches all 240 patched native files. Only the OoT tracker source changes from 0.2.15.
- Only `libsoh.so` changes; the other seven native libraries are byte-identical to 0.2.15, including the MM save compatibility fix.
- Installed APK bytes match the candidate hash. Signature verification passes with the previous signing certificate; 16 KiB alignment passes.
- Eight ARM64 libraries, 231 callbacks, and 9,036 support files are verified.
- The public source archive excludes ROMs, extracted game archives, private saves, logs, credentials, and emulator state.

The save schema, item placements, inventory, and progression logic are unchanged. This is a focused persistence regression check, not a new certification of every gameplay feature or the user's physical device.
