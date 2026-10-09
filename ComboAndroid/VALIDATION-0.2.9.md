# Combo Ship 0.2.9 validation

APK: `ComboShip-Android-0.2.9-arm64.apk`, version code 34, 45,327,575 bytes.

SHA-256: `bc0037201df07a0b6a221b59f8eafce6854974098e508a7b2e5de65021926cdd`

## Build and source checks

- ARM64 native compilation, Android assembly and lint succeeded. Lint reported zero errors and 84 non-error findings.
- All 29 host regression tests passed. The age test compiles the production region-search block and distinguishes active tracker abilities from seed-generation assumptions across the native area list, equipment modes and age/time states. It fails against the previous implementation and passes with this change. The tracker test also compiles the actual settings-refresh helper, checks enable/disable/re-enable behavior and ensures unchanged settings do not continually recalculate.
- A fresh overlay replay from upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439` reproduces all 207 patched native files. Two native files changed from 0.2.8: the region search and the OoT tracker drawing helper. The existing transition and inventory refresh remains in place.
- Signing matches previous releases. Archive CRC, 16 KiB alignment, eight ARM64 libraries, 231 callbacks and 9,036 support-file hashes passed verification. The installed emulator APK matches the release APK byte for byte.

## Native route checks

All 19 prerequisite and route cases passed using the actual packaged ARM64 libraries. The shuffled fixture uses a known coupled Deku Tree/Water Temple entrance swap, Deku-only forest access, no child sword, Hookshot rather than Longshot, Iron Boots, Zora Tunic, songs, an instrument, playable buttons, swimming and six Water Temple keys.

| Case | Result |
| --- | --- |
| Shuffled child entrance with age changing in seed logic | Map and Compass are reachable, including with Hookshot as the fixture's only adult weapon. |
| Enhancement-only age changing during generation search | Does not silently add routes to the seed's separately selected logic assumptions. |
| Missing prerequisites | Disabling the age rule or removing Song of Time, an instrument, a required button, swimming or Hookshot blocks the corresponding tested route. |
| Individual equipment requirements | Central Bow Target remains unavailable without the needed equipment; Morpha remains unavailable without the boss key. |
| Standard Temple of Time travel, age-anywhere disabled | From Child Link, three stones, the Ocarina of Time, playable Song of Time, Master Sword and the dungeon equipment make Map and Compass reachable through Water Temple's normal entrance. |
| Standard travel missing requirements | A missing stone, Song of Time, instrument or required button prevents that tested adult route when the Door of Time is closed. |
| Water Temple behind the child-only Deku Tree entrance | Normal pedestal travel alone does not provide an age change inside that dungeon. The search retains the shuffled entrance's age restriction. |

## Android tracker checks

Private fixtures ran on a disposable read-only Android API 35 emulator. The captured tracker screens were visually inspected.

- 0.2.8 reproduces grey Water Temple checks while Child Link has the active age-change enhancement but the seed does not assume it.
- 0.2.9 shows reachable Water Temple checks while still Child Link, both inside the dry entrance and outside in Kokiri Forest. The outside test uses the discovered shuffled entrance and does not enter the dungeon or change age first.
- Disabling and re-enabling the enhancement through the actual menu refreshes the same tracker immediately. Native logs show one recalculation per change with no intervening scene transition or save reload.
- Removing Song of Time leaves the tested shuffled adult checks unavailable.
- With age change disabled, the three stones and normal Temple of Time requirements allow adult Water Temple checks to appear available while Child Link remains outside in Kokiri Forest. Checks requiring additional equipment remain grey.
- After discarding the read-only emulator writes, a fresh boot matched all five original internal save/config files and touch preferences byte for byte. The temporary visible test folder was absent; the task emulator was stopped.

These are focused regression checks, not a full seed playthrough or physical-phone test. The user's exact phone save was not tested. Existing startup diagnostics and non-error build findings were not treated as a clean general bug audit.

The update does not migrate or rewrite existing seeds or saves. Only the APK, 187-file source ZIP, fixes, this report and SHA-256 manifest are distributed. ROMs, extracted game archives, saves, private screenshots, fixtures, logs, diagnostic binaries, credentials and signing keys are excluded.
