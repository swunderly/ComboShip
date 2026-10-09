# Combo Ship Android 0.2.18 validation

APK SHA-256: `a5dbaa1abef3c1b92dbdd87b9f716b1d843de48e760cf2e8d5da433bd15c796a`

Version 0.2.18, version code 43, package `org.comboship.android`. The hash above was read from the current candidate APK after both departure HUD fixes and the native MM audio diagnostic instrumentation were included.

## Automated and source checks

All 42 host checks pass. The cross-warp presentation harness exercises all 16 selected departure routes, vanilla fallbacks, the playback and departure skip settings, remembered-entrance clearing, instrument-camera handoff, and both engines' HUD restoration timing. The audio harness exercises native wing cues, both games' fanfares, volume restoration, resident audio caches, and the isolated sequence dialect adapters.

The native ARM64 build completed successfully. Fresh candidate verification on the APK hash above passed: all 254 patched native files match APK provenance and the production overlay replay; all eight packaged ARM64 libraries match private copies of current staged libraries stripped with the configured NDK's `llvm-strip --strip-unneeded`; signing continuity, 16 KiB alignment, runtime assets and callbacks, and installed APK equality pass. Android lint reports no errors or fatal issues. The recorded host suite has 42 passing checks.

## Runtime evidence

Tests use disposable ARM64 Android emulator fixtures. Videos, screenshots, native logs, and original-file hashes remain local; the public source package excludes ROMs, game-derived archives, saves, credentials, recordings, and emulator state.

The current APK completed an adult OoT Song of Soaring test with Fast Ocarina Playback and Skip Soaring Cutscene enabled. It retained the selected cross-game destination, arrived in MM, and retained MM's native arrival animation, as the native skip hook does. Original internal emulator files were verified unchanged for this case.

The final child OoT-to-MM Soaring test on the hash above completed the native wings, feathers, and warp departure and reached Great Bay with MM's native owl arrival. Native logs show all three MM scores executing and all four departure cues, with zero sequence-dialect failures. The final MM-to-OoT Requiem test completed the native OoT score, particles, white flash, and camera departure and reached Desert Colossus with native arrival. Visual sheets confirm the HUD stays hidden during both departures. Original internal emulator data hashes remained unchanged in both tests.

Native MM Soaring was also used as an audio diagnostic baseline. Its null-drum warning `0x0500092c` identifies font 9, drum 44; the imported Soaring warning `0x0500262c` identifies the remapped font 38, drum 44. This establishes that the drum warning occurs in the native baseline at the corresponding drum. It does not establish audible equivalence or certify all audio output.

## Validation limits and phone checks

The emulator was started without audio output. Acoustic validation, physical-device validation, and full gameplay validation are not complete. Compilation, diagnostic score execution, and successful emulator travel do not establish sound quality or every seed, form, location, route, and settings combination.

On the user's phone, check full Song of Soaring from OoT as child and adult, listening for the song, wing and feather cues, wind, and departure sound; check an OoT warp song from MM, listening through the fanfare and departure. Confirm camera continuity, hidden departure HUD, the selected landing, native arrival, and normal control and sound after arrival. Repeat with Fast Ocarina Playback and Skip Soaring Cutscene enabled, and with Remember Save Location enabled for an MM landing. Use the existing save and archive setup.
