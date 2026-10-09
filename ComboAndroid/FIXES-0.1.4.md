# ComboShip Android 0.1.4 — launch and seed fixes

Install `ComboShip-Android-0.1.4-dev-arm64.apk` over your current version. Keep the existing installation so your ROMs, settings and saves remain available.

## What changed

- After ROM setup, opening the app goes directly to the game. An update may briefly prepare support files. Setup and gameplay now share one Android task, and the update removes the old extra game card from Recents.
- Save first, then hold **Menu → App options** to return to ROM, backup and recovery tools. Holding the Android app icon also offers an **App options** shortcut. Returning to these tools now shuts down the two game engines cleanly; an Android exit crash was fixed.
- Seed generation shows progress for both games and explains the final hint-building stage. Success explicitly says **Seed ready for OoT + MM**. Failures no longer use a previous seed's success flag.
- Generation is blocked during name entry, where an unfinished finalization was reproduced. The file list also prevents loading another file while generation is running.
- The Seed field displays any saved custom seed. Previously it could look empty while the generator reused that saved value. Leave it blank for a new random roll; the same text and settings repeat a seed.
- The two-ROM setup and earlier text/quest-logo fixes are included.

## Starting a new seed

Generate from the file list or an empty file's randomizer settings. Wait for the ready message, then choose **Start Randomizer** and create a new save. Existing saves keep their original seeds. Some checks remain unchanged unless their shuffle option is enabled; seeing one normal reward does not establish that the whole game is unrandomized.

## Verified locally

Tests used an Android 15 emulator with locally supplied ROMs:

- Installed over the previous build, launched directly, removed the legacy duplicate Recents card, and resumed the same game process after Home/reopen.
- Returned from gameplay to App options without a native crash, then launched the game again. Verified the registered Android shortcut and its management intent.
- Reproduced the name-entry generation freeze before the fix. In this build, generation completed from the supported screen and was unavailable during name entry.
- Generated a new combined seed, created a new randomizer save, and opened it in OoT gameplay. The save's embedded seed exactly matched the generator's output: 442 OoT placement entries, 288 MM entries, and 220 cross-game placements. These counts include fixed entries.
- Confirmed that the OoT and MM save sections had the same seed, that MM stored 284 shuffled checks, and that all four saved Mido chest rewards matched the output, including two MM rewards.
- Native compilation, Android assembly and lint completed with zero errors. APK checks passed native entry points, support hashes, ARM64 libraries, 16 KiB ELF alignment, ROM exclusion and signing verification. The source patch replay matched the compiled sources.

The reported sound loss recovered on the phone before this update. No audio-driver change is included, and the cause remains unconfirmed. The emulator showed an active audio track after resume, but its host audio was disabled, so audibility and Bluetooth were not tested. This update has not had a full seed playthrough or physical-device validation.

## Please test on your phone

1. Open the app after updating, then use Home and reopen. It should enter/resume the game and show one ComboShip card in Recents.
2. Generate with the Seed field blank. Wait for the ready message, create an empty **Randomizer** save, and check rewards in both games.
3. Check sound before and after Home/resume and switching games. If it disappears, report the last action and whether you were using speakers or Bluetooth.
4. Save, hold **Menu → App options**, then press **Play**. Confirm your save still loads.

APK size: 55,889,973 bytes.

SHA-256: `a5acc61e55b648934cc763710c4f57c0251dea21073a183b7b2e1da80c4e230a`
