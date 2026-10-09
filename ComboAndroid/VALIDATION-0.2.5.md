# Combo Ship 0.2.5 validation

APK: `ComboShip-Android-0.2.5-arm64.apk`, version code 30, 45,330,923 bytes.

SHA-256: `72923aa8a04c0ec4e9b083411f5ff2619415299dc521fc75e1fb5ee661246035`

## Build checks

- ARM64 native compilation, Android assembly and lint succeeded. Lint reported zero errors and 84 non-error findings.
- All 28 existing host regression tests passed, including tracker knowledge, visibility guards, provenance and persistence, plus menu, progression, song-route and multiplayer protocol checks.
- The overlay reproduced all 205 patched native files from pinned upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. Their hashes match the APK source manifest. The only changed native source is the OoT randomized actor hook file.
- APK signing matches earlier releases. Archive CRC, 16 KiB alignment, eight ARM64 libraries, 231 callbacks and 9,036 support-file hashes passed verification. Installed emulator APK bytes match the delivered APK.
- The seven libraries other than `libsoh.so` are byte-identical to 0.2.4.

## Focused Android emulator checks

Testing used a disposable, read-only API 35 emulator and private Forest Temple save fixtures. Native Gold Skulltula reward actors (`En_Si`) were placed using console commands, with controlled randomized Piece of Heart placements. Screenshots were reviewed and saved tracker data verified. This exercised the real randomized reward drawing and observation path, without requiring a full seed playthrough or claiming a natural Skulltula combat test.

| Case | Result |
| --- | --- |
| 0.2.4 reproduction | A visible Piece of Heart from the native Skulltula reward actor remained unknown in the tracker. |
| 0.2.5 visible reward | The same actor and camera setup immediately recorded `Seen: Piece of Heart` for its exact check. |
| Restart | Force-stopping and relaunching retained the recorded item name even though the test actor was gone. |
| Hidden reward | A reward placed behind the camera remained unknown. A second reward placed in view was recorded for its own check; the hidden check remained unknown. |
| Mysterious Shuffle | The visible reward rendered as a question mark and did not reveal the underlying item. |
| Collection | Skulltula collection flags, token count and heart capacity remained unchanged by observation. The seen reward stayed uncollected. |
| Data isolation | After discarding the read-only emulator writes, a fresh boot matched all five original internal save/config files and touch preferences byte for byte. The test folder was absent and the task emulator was stopped. |

The existing projection, model scale, transition, pause and collision visibility checks are retained. Physical-phone behavior, every Skulltula location, other reward types and a full randomized playthrough were not newly tested for this small change. Earlier approved menus and warp presentation remain in place.

Only the APK, 174-file source ZIP, fixes, this report and SHA-256 manifest are distributed. Game ROMs, extracted game archives, saves, private screenshots, fixtures, logs, credentials and signing keys are excluded.
