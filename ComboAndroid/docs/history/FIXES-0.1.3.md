# ComboShip Android 0.1.3 — simpler setup

Install `ComboShip-Android-0.1.3-dev-arm64.apk` over the current version. Existing ROMs and saves are kept; do not uninstall to try the new interface. This update includes the text and quest-screen fixes from 0.1.2.

## First run

1. Choose your Ocarina of Time ROM.
2. Choose your Majora’s Mask ROM.
3. The game opens automatically and prepares its game data as needed.

The launcher now has a focused layout with two progress rows and one main action. On subsequent visits, or when updating an existing installation, the main action is **Play**. Support-file preparation happens automatically. Mods, backups, storage, repair and ROM management are under **App options**, with detailed startup errors under **About and help**.

## Validation

- Built and linted the Android app: zero errors. The APK verifier passed native libraries, entry points, support-file hashes, 16 KiB ELF alignment and ROM exclusion checks. The game libraries are unchanged from 0.1.2.
- Installed over 0.1.2 on the Android 15 emulator. Existing ROMs were recognized and the launcher showed **Play** without re-importing.
- Exercised the new setup with locally supplied ROMs: cancelling the picker retained step 1; a non-ROM file was rejected; importing OoT advanced to step 2; importing MM launched the combined game and reached its title screen.
- Rotated the setup screen in both directions and checked that the current step was preserved and the main action remained accessible by scrolling on a short landscape screen.
- Simulated Android reclaiming the background setup process while the MM document picker was open. Selecting the ROM restored setup, waited for support preparation, imported the document and launched successfully.
- Checked that maintenance tools are available in App options. Both re-imported ROM hashes matched the original emulator copies; test copies were kept local.

The flow was tested with existing extracted game archives; native extraction is unchanged and was verified in the prior integration tests. This is emulator validation, not confirmation on a physical phone.

## Phone check

An existing installation should show the new **Ready to play** screen with **Play** and **App options**. Confirm that your saves still appear after launching. Fresh installations should ask for OoT, then MM, and start automatically. Keep testing the previously reported text corruption too; the 0.1.2 fix is included.

APK size: 44,193,099 bytes.

SHA-256: `ea8a478e61e49f499728419ae71835f35462e225f669942d8fd5d84dd9dd7073`
