# ComboShip Android 0.1.6 — remove the presets pop-up

Install `ComboShip-Android-0.1.6-dev-arm64.apk` over the existing app to keep your ROMs, saves and settings.

Starting a new randomizer file now goes directly to name entry without the **Take a look at our presets!** pop-up. Presets remain available through the menu; this update does not automatically apply one or change your gameplay settings.

This build also includes the earlier removal of the unusable desktop settings-import prompt on Android.

## Validation

- Native build, Android assembly and lint passed with zero errors.
- Installed over 0.1.5 in the Android 15 emulator.
- Set the pop-up's previously-dismissed flag to zero, selected an empty file, chose **Start Randomizer**, and entered a name character without the pop-up appearing. The flag remained zero.
- Byte-verified that the existing test saves were unchanged and restored the original emulator settings after the test.
- Verified patch replay, APK signature, native libraries and entry points, support hashes, 16 KiB ELF alignment and ROM exclusion checks.
- Confirmed that the installed test APK and delivered APK have the same SHA-256 hash.

This was an emulator test of the affected new-file flow; the update has not been tested on a physical phone.

## Phone check

Open an empty file and choose **Start Randomizer**. Name entry should appear directly, without the presets pop-up.

APK size: 44,193,714 bytes.

SHA-256: `830cbd0ade5c398b188a0d773e3fd8d965b30230668a7d689a7baf285586d988`
