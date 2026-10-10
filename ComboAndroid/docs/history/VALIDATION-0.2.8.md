# Combo Ship 0.2.8 validation

APK: `ComboShip-Android-0.2.8-arm64.apk`, version code 33, 45,327,467 bytes.

SHA-256: `4d152f68b2631a2fe4e8f9b3cb0d30f7130c9a4f6404a38826bd8a1d5b262f54`

## Build and source checks

- ARM64 native compilation, Android assembly and lint succeeded. Lint reported zero errors and 84 non-error findings.
- All 29 host regression tests passed. The new tracker test compiles the actual production transition and deferred-refresh functions; it failed against the old implementation and passes with this fix. The age-change area test includes the three dry Water Temple entry regions and rejects underwater/main-room shortcuts.
- A fresh overlay replay from upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439` reproduces all 207 patched native files. Three native files changed from 0.2.7: tracker transition handling, the safe age-change area list, and its settings tooltip.
- Signing matches previous releases. Archive CRC, 16 KiB alignment, eight ARM64 libraries, 231 callbacks and 9,036 support-file hashes passed verification. The installed emulator APK matches the distributed APK byte for byte.

## Native logic and Android tracker checks

The route fixture uses a coupled Deku Tree/Water Temple entrance swap, Deku-only forest access, no child sword, Hookshot rather than Longshot, Iron Boots, Zora Tunic, songs, an instrument, playable buttons, swimming and six Water Temple keys. Tests call the real reachability search in the packaged ARM64 libraries. The same driver reproduces three missing-route failures against 0.2.7 and passes all 12 cases against 0.2.8.

| Case | Result |
| --- | --- |
| Shuffled child entrance with age changing in seed logic | Map and Compass are reachable, including when Hookshot is the only adult weapon in the fixture. |
| Enhancement-only age changing | Does not silently add age-changing routes to the seed's original logic assumptions. |
| Missing prerequisites | Disabling the age rule or removing Song of Time, its instrument, a required button, swimming or Hookshot prevents the corresponding tested route. |
| Individual Water Temple requirements | Central Bow Target stays unavailable without its needed equipment; Morpha stays unavailable without the boss key. |
| Old Android tracker | An actual child-to-adult change on Water Temple's dry entrance ledge leaves the checks stale and grey in 0.2.7. |
| Updated Android tracker | The same age change refreshes Map, Compass, Central Pillar, Longshot and Torches to available. Changing back to Child Link refreshes them to unavailable. This live-location test uses enhancement-only age changing. |
| Data isolation | After discarding the read-only emulator writes, a fresh boot matched all five original internal save/config files and touch preferences byte for byte. The test folder was absent and the task emulator was stopped. |

The tracker uses the current entrance and current age after a transition; it does not grant every check merely because the player is inside a dungeon. Equipment, song and key requirements still apply. These checks used private test fixtures on an Android emulator, not the user's exact phone save. Physical-phone validation and a full seed playthrough were not repeated.

The update does not migrate or rewrite existing seeds or saves. Only the APK, 184-file source ZIP, fixes, this report and SHA-256 manifest are distributed. ROMs, extracted game archives, saves, private screenshots, fixtures, logs, diagnostic binaries, credentials and signing keys are excluded.
