# Combo Ship 0.2.11 validation

APK: `ComboShip-Android-0.2.11-arm64.apk`, version code 36, 45,393,684 bytes.

SHA-256: `36cd3338a14934427cd16dbebc1cacaa1c00abd1457df26046f0804b67c48cf3`

## Build and source verification

- ARM64 native compilation, Android assembly and lint succeeded. Lint reports zero errors and 84 non-error findings. Existing compiler and lint findings are not represented as a clean general bug audit.
- All 31 host regression tests passed. The added resource test compiles the complete production item-use, Bombchu-refill, wallet-capacity and source-event functions with explicit external input bindings. It checks renewable versus finite supplies, ownership, age and item restrictions, pot/grass prerequisites, fairy instrument and notes, Bombchu-drop conditions, search reset, wallet limits and free checks. The tracker regression compiles the actual availability-settings updater and verifies invalidation when ammo, magic or rupees change. These are predicate tests, not a replacement world solver.
- Every one of the 107 source mappings was checked against its exact file, line and condition at OoTMM commit `413573d22d1d53e8a81931a70927f625b73488b9`, and against the corresponding native expression. The table covers 42 named native regions and five generic-grotto event types. These counts describe the mapped subset, not complete website resource coverage.
- A fresh overlay replay from ComboShip commit `94eb185e4abcc2d568aa8241fa02c43cdd86c439` reproduces all 211 patched native files. This incremental patch changes six native files. Prior overlays remain intact.
- APK signature and update-certificate comparison passed. Archive CRC, 16 KiB alignment, eight ARM64 libraries, 231 exported callbacks, all 9,036 support-file hashes and all packaged native source hashes passed verification. The installed emulator APK matches the final release byte for byte.

## Actual packaged native tests

All 51 route and prerequisite cases passed against the final APK's ARM64 game libraries. The private test driver calls the production search exports and is bound to the APK and library hashes.

The retained 45 cases cover shuffled Water Temple access, ordinary pedestal travel, local age change, compatible Slingshot routes and Bow-only counterexamples, mixed-age Spirit Temple paths, cross-song prerequisites, and MM Scarecrow learning. Six additional cases use an actual decoupled native shop entrance to isolate renewable rupees: a wallet without a money source cannot pay; a free check remains reachable with or without a wallet; restoring the ordinary exit makes a reachable money source usable; a source without a wallet cannot pay; and a missing shop-access item still blocks entry.

The isolated paid-shop case incorrectly passes against the preserved 0.2.10 APK and is correctly rejected by 0.2.11. The other 50 cases retain their expected results. This establishes a concrete before/after native correction, not complete website equivalence.

## Complete combined generation and replay

Three new fixtures generated actual item placements and prices using the final APK's libraries and production combined generator. Inputs contained no preassigned placements. Each used strict **Every check reachable** and the combined both-bosses goal.

| Fixture | Generation | Original-rules replay | Generated checks | Unreachable OoT / MM |
| --- | --- | --- | ---: | --- |
| Starts in OoT, paired rules enabled | PASS | PASS | 1,325 | 0 / 0 |
| Starts in MM, paired rules enabled | PASS | PASS | 1,325 | 0 / 0 |
| Starts in OoT, enhancements enabled with logical assumptions cleared | PASS | PASS | 1,325 | 0 / 0 |

The fixtures exercise 25 rule fields and ten MM techniques, shuffled local instruments/buttons, adult trades, Skulltulas, MM pots and owl checks. Runtime-only Bremen fields remain zero in the logical snapshot. Saved rules, native settings, selected techniques, destination-map flags and generated prices match the inputs. The enhancement-only control keeps ordinary song destinations. Replay succeeds with the original selected rules, without diagnostic all-tricks or No Logic fallback.

All three final fixtures pass at their first native retry index. This is validation of three concrete deterministic seeds, not a guarantee that arbitrary settings will generate on the first attempt. A separate read-only verifier rejects six deliberately corrupted receipts in memory.

## Validation limits and privacy

The final APK was installed on the disposable read-only Android API 35 AVD. Its setup activity launched without an Android runtime crash, and its installed hash was checked. Production route and generation tests ran headlessly. This pass does not establish physical-phone audio or graphics behavior, native animation quality, a manual playthrough, or every option combination. Seed guarantees remain guarantees of the native model; the [remaining differences](FIXES-0.2.11.md) still apply.

The dated 402-setting comparison remains an immutable baseline. Its old source hashes are not post-change evidence; this release's fresh source replay and APK verification provide that check.

Only the APK, an explicitly allowlisted source ZIP, fixes, validation and SHA-256 manifest are distributed. The ZIP includes the resource mapping and upstream license. ROMs, ROM-derived game archives, saves, generated placements/hints/prices, private logs and screenshots, diagnostic executables, credentials and signing material are excluded. Existing save data and seed placements are not migrated.
