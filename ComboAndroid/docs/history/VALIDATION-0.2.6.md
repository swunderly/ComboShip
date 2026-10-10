# Combo Ship 0.2.6 validation

APK: `ComboShip-Android-0.2.6-arm64.apk`, version code 31, 45,331,663 bytes.

SHA-256: `98fea43c006fd192392325c9f9d8487ee59e4f049547429ac9b8334857b74550`

## Build checks

- ARM64 native compilation, clean Android assembly and lint succeeded. Lint reported zero errors and 84 non-error findings.
- All 28 existing host regression tests passed, covering tracker knowledge, visibility and persistence, plus the existing menu, progression, song-route and multiplayer protocol checks.
- The overlay reproduced all 205 patched native files from pinned upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. Their hashes match the APK source manifest. This release changes only the OoT tracker adapter, tracker implementation and shop actor drawing.
- Signing matches earlier releases. Archive CRC, 16 KiB alignment, eight ARM64 libraries, 231 callbacks and 9,036 support-file hashes passed verification. The installed emulator APK matches the delivered APK byte for byte.
- The seven libraries other than `libsoh.so` are byte-identical to 0.2.5.

## Focused Android emulator checks

Testing used a disposable, read-only API 35 emulator with a private Kokiri Shop save fixture. Seven ordinary refill offers and a randomized Piece of Heart occupied the eight native shop slots. Slot 7 started with the old identified status and no reward knowledge. Screenshots were reviewed and the actual saved tracker records checked.

| Case | Result |
| --- | --- |
| 0.2.5 reproduction | Visible shop models were not recorded; slot 7 appeared as `Known` without an item name. |
| 0.2.6 visible rewards | All eight visible shelf rewards were recorded for their exact checks. Slot 7 displayed `Seen: Buy Arrows (30)` and retained its price status. |
| Restart | All eight names and observation sources survived force-stop and reload. Mysterious Shuffle was enabled before reload to prevent fresh model observation; previously learned rewards stayed known. |
| Ordinary descriptions | Browsing the native 30-arrow offer repaired the old blank record, even with the tracker window closed and model observation disabled. |
| Mystery protection | The randomized slot rendered as a question mark and stayed unknown. Reading its `Mysterious Item` description and price did not record the underlying reward or treat the price as an item name. |
| Legacy unknown record | Outside the shop, the old slot retained its original status without a fabricated item name or misleading `Known` label. |
| Collection | All tested shop rewards stayed uncollected. Inventory, Skulltula collection flags, rupees and heart capacity were unchanged by observation and description browsing. |
| Data isolation | After discarding the read-only emulator writes, a fresh boot matched all five original internal save/config files and touch preferences byte for byte. The test folder was absent and the task emulator was stopped. |

Shop shelf previews retain camera-frustum, model-scale, transition and pause checks; they bypass invisible shelf interaction collision. Ordinary world rewards retain their collision visibility checks. Physical-phone behavior, every shop, all merchant dialogues and a full seed playthrough were not newly tested for this focused fix.

Only the APK, 177-file source ZIP, fixes, this report and SHA-256 manifest are distributed. ROMs, extracted game archives, saves, private screenshots, fixtures, logs, credentials and signing keys are excluded.
