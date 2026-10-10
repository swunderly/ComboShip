# Combo Ship 0.2.4 validation

APK: `ComboShip-Android-0.2.4-arm64.apk`, version code 29, 57,069,644 bytes.

SHA-256: `547a0bfed73ba1013e23ac5fc3cef5c2b45f52b7f215573fc4336eed5b2afccd`

## Build and automated checks

- ARM64 native compilation, Android assembly and lint succeeded. Lint reported zero errors and 84 non-error findings.
- All 28 host tests passed, including native song destination mapping, runtime versus logic flags, endpoint permutations, song/instrument/button requirements, owl access, seasonal arrivals, ordinary save resume, cancellation, saved rules, search, trackers and multiplayer protocol regressions.
- The source overlay reproduced all 205 patched native files from pinned upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. Their hashes match the APK's source manifest.
- Signing matches the earlier APK. Archive CRC, 16 KiB alignment, all eight ARM64 libraries, 231 callbacks and all 9,036 support-file hashes passed verification.
- Installed APK bytes match this APK. Both packaged game libraries have identical allocated runtime sections and program headers to the iteration libraries used for presentation testing. Gradle additionally removes non-runtime symbol and name tables.

## Android emulator observations

Testing used the task's disposable, read-only Android API 35 emulator at 2280×1080, with private copies of the user's games and combined save. Inventory, songs and owl unlocks were set in local fixtures to exercise presentation; these are not claims of legitimately completing those quests.

| Case | Observed result |
| --- | --- |
| Child Soaring | Full song playback opened the native Termina owl map. Cycling four destinations selected Clock Town. Confirmation, feather wings, wind and fade completed the handoff to its owl statue. |
| Adult Soaring | Full playback, owl map, confirmation and feather departure reached the Great Bay Coast statue. Adult player scale and camera were exercised on the final APK. |
| Map cancellation | B from confirmation returned to the map and retained Clock Town selection. B from the map closed it and restored gameplay. |
| No owl unlocks | Full playback showed the in-game no-owl message. A dismissed it and restored gameplay without travelling. |
| Requiem from MM | Full song playback, native colored text and confirmation preceded orange OoT particles, player departure and camera movement. Arrival used the native OoT particle sequence at Desert Colossus. This passed again on the final APK after an OoT→MM trip. |
| Minuet from MM | Playback and confirmation preceded green particles and native arrival at Sacred Forest Meadow. The iteration library's runtime sections and program headers match the final APK's MM library. |
| MM cancellation | B dismissed Minuet's confirmation and restored gameplay. Playing it again then completed travel. |
| Resource lifetime | Both game handoffs completed. Testing caught and fixed missing actor metadata, destruction after player teardown, eager loading of a foreign display list from the wrong archive, and an unaligned asset reference. No new crash occurred in the final tested routes. |
| Isolation | After discarding read-only AVD writes, a fresh boot matched all five original internal save/config files and the original touch preferences byte for byte. The temporary visible game folder was absent. The task emulator was stopped. |

The final Soaring cases and Requiem round trip were recorded and reviewed frame by frame. Earlier presentation checks are retained privately as diagnostics. An existing startup message about an unavailable input-editor window is separate from warp handling.

## Limits and distribution

This is a visual and transition update. The existing seed logic and approved menus are retained. Native visual routines are adapted across engines with their original keyframes, palettes, particle paths and timing. Audio uses the active engine's available sound bank; foreign fanfare and sound-effect banks are not imported. The emulator ran without a host audio output, so audible fidelity was not certified.

Physical-phone performance, controller behavior, every destination permutation in gameplay, all player forms and a full randomized playthrough are outside these observations. Host route tests cover the endpoint permutations independently. No claim is made that every possible bug is eliminated.

Only the APK, reviewed source ZIP, fixes, this report and SHA-256 manifest are distributed. The source ZIP contains 171 explicitly allowed files. ROMs, extracted game archives, saves, private recordings/screenshots, settings fixtures, credentials and signing keys are excluded. Players supply their own games.
