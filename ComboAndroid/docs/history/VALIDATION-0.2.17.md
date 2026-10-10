# Combo Ship Android 0.2.17 validation

APK SHA-256: `dfea30598de053d33aae6ec07138a9b09fa9c86c8cb673ca26ef8984baa5d683`

Version 0.2.17, version code 42, package `org.comboship.android`, label `Combo Ship`. This release fixes permanent push-block state in the Android OoT engine.

## Reproduced failure and fix

The released 0.2.16 APK, running in a disposable read-only ARM64 Android API 35 emulator, let Link push the Gerudo Training Ground silver-gauntlet block into the pit. Its blockstop switch (11) was not recorded. A normal native save retained switch value zero; after restarting and returning to the room, the block appeared back in its original position.

The new APK records switch 11 after the same push. The native save records `0x800`. After a full app restart and returning to the block room, the block remains seated in the pit. Entering the four-chest room (room 10), then returning to the block room (room 3), reconstructs the room with the block still seated and the corridor clear. A subsequent normal native save retains the completion bit.

The fixture preclears combat. Farore's Wind provides transport to the block room. The independent eye-switch door is preopened for the room-transition check; the block's completion flag is earned by gameplay and is never injected. Original emulator files were checked byte-for-byte unchanged. No user phone or user save was modified.

## Regression checks

- All 40 host tests pass.
- The new regression compiles the production collision, blockstop, switch, and scene-save functions. At the actual Training Ground pit height (-41), fused arithmetic misses both huge-block variants; the corrected arithmetic records both.
- Saved switch restoration, room reconstruction, both huge-block variants, ordinary-block sound behavior, missing blocks, and unrelated actors are covered.
- The Android OoT target uses `-fno-fast-math -ffp-contract=off`, matching the arithmetic correction accepted upstream in Shipwright PR 6811: https://github.com/HarbourMasters/Shipwright/pull/6811.

## Build and package

- ARM64 native build, Gradle assembleDebug, and lintDebug succeed. No lint errors or fatal issues; 84 existing non-error findings remain.
- Fresh production overlay replay matches all 240 patched native files. Only `soh/CMakeLists.txt` changes from 0.2.16.
- Only `libsoh.so` changes; the other seven native libraries, including MM, are byte-identical to 0.2.16.
- Installed APK bytes match the candidate hash. Signature verification passes with the previous signing certificate; 16 KiB alignment passes.
- Eight ARM64 libraries, 231 callbacks, and 9,036 support files are verified.
- The public source archive excludes ROMs, extracted game archives, private saves, logs, credentials, and emulator state.

The save format, inventory, item placements, and progression logic are unchanged. Existing saves remain supported. This validates the reported block-persistence regression in an emulator, not every gameplay feature or the user's physical phone.
