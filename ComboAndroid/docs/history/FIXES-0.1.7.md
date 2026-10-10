# Combo Ship 0.1.7 — ocarina icon and app name

Install `ComboShip-Android-0.1.7-dev-arm64.apk` over the existing app. The installation identity and signing key are the same, so your ROMs, saves and settings remain in place.

The app is now named **Combo Ship** in Android, the setup screen and About. A stylized blue ocarina with a silver collar and gold detail replaces the old triangle icon. The launcher uses an adaptive icon on a dark navy background; setup uses the transparent artwork. The App options shortcut also uses the new icon.

## Validation

- Android assembly and lint passed with zero errors.
- Installed over 0.1.6 in the Android 15 emulator.
- Visually checked the new icon in the Home screen, app drawer, Recents and setup. Verified the **Combo Ship** label in Android's UI and the **About Combo Ship** dialog.
- Launched from the app drawer and reached the game title sequence.
- Byte-verified that all eight native libraries and the support manifest match 0.1.6. This update changes Android branding only.
- Verified the APK signature, packaged runtime and ROM exclusion checks. The delivered APK matches the installed test APK.
- Byte-verified the existing test saves and original game settings after testing.

These checks used an emulator; physical-phone validation remains with you.

## Phone check

After updating, check that your launcher shows the blue ocarina and **Combo Ship**, then open the app normally. Long-press the icon for **App options** if you want to see the updated setup branding.

Artwork and the exact built-in imagegen prompt are documented in the source package under `ComboAndroid/branding/README.md`.

APK size: 45,054,963 bytes.

SHA-256: `e9ef00a7846bc9eace03ea7bec73d12cb0ed7ba3c6f307854c0d512234c5ef9e`
