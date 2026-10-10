# Combo Ship 0.2.13 validation

APK: `ComboShip-Android-0.2.13-arm64.apk`, version code 38, 57,178,492 bytes.

SHA-256: `1abc0f7672589368399aef458aceb0f066548cda9621ea6d4f094ab69716984c`

## Community reference and scope

Reference: OoTMM v32.3, commit `413573d22d1d53e8a81931a70927f625b73488b9`. [Resource mapping](RESOURCE-PARITY-0.2.13.md) records the exact money, ammo, magic, bombchu and minigame contracts. Tests verify the pinned source hashes and expressions when that checkout is present. The earlier age, song, wallet and schedule checks remain in the suite.

This closes the identified supported resource-policy gaps. It does not certify equivalence for every possible inventory, seed or website setting, or add unsupported website features. Native prices, equipment restrictions, wallet tiers and shop-restocking behavior remain real engine boundaries.

## Build and source verification

- Both ARM64 engines compiled; Android assembly and lint passed with zero lint errors and 84 non-error findings.
- All 33 host regression tests passed. Production resource tests cover wallet capacity, source discovery, equipment, bottles, enemy souls, valid future visits, paid-source bootstrapping, vanilla versus shuffled prices, exception-safe restoration and search isolation.
- A fresh overlay replay reproduces all 219 patched native files from ComboShip `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. This incremental patch changes 24 native files. Prior overlays remain intact.
- APK CRC, signature, update certificate, 16 KiB alignment, eight ARM64 libraries, 231 callbacks, 9,036 support-file hashes and native source provenance passed verification. The installed disposable task-emulator APK matches the release byte for byte.

## Actual packaged native regression tests

All 74 cases pass against the final APK's unchanged ARM64 libraries. The private driver calls production search exports and records the exact APK and library hashes.

Retained cases cover Water Temple entrances, ordinary pedestal travel, local age changes, compatible Slingshot targets and Bow-only counterexamples, mixed-age Spirit Temple paths, cross-song prerequisites, MM Scarecrow, logical shop affordability, inn hours and clock ownership.

Added cases cover Honey & Darling's Deku-bubble day-three route, missing magic, Bow-only all-days requirements, missing valid day-three access, and the Moon trial's native bombchu license. The Moon route needs a bomb bag, Bow and reachable refill; shuffled shelves cannot supply unassigned vanilla stock. Missing equipment stays unavailable.

Two cases reproduce incorrect behavior in 0.2.12: the valid Deku-only day-three reward is rejected, and an unassigned shuffled shop is incorrectly treated as a bombchu source. Both are corrected in 0.2.13. The other 72 keep their expected results.

## Complete combined generation and replay

Three fresh fixtures generated actual placements and prices with the final APK's libraries and the production combined generator. Inputs contain no preassigned placements. Each uses strict **Every check reachable** and the combined both-bosses goal.

| Fixture | Generation | Original-rules replay | Checks | Unreachable OoT / MM |
| --- | --- | --- | ---: | --- |
| OoT start, paired rules | PASS | PASS | 1,325 | 0 / 0 |
| MM start, paired rules | PASS | PASS | 1,325 | 0 / 0 |
| OoT start, enhancements only | PASS | PASS | 1,325 | 0 / 0 |

All three preserve their requested 25-rule and ten-technique snapshots, native settings and actual shuffled purchase prices. Bremen count remains outside logical assumptions. The enhancement-only control preserves ordinary song destinations. Replay uses the original rules, with no all-tricks or No Logic fallback and no padding unreachable locations. A separate verifier rejects six deliberately corrupted receipts in memory.

Full generation caught and fixed a price-export error that focused route tests did not expose. Unchanged MM stock is preserved across generation resets, unused vanilla price metadata is ignored, and only randomized purchase prices enter the seed. Resource discovery avoids reevaluating refill predicates once that resource is known.

## Limits and privacy

Tests ran headlessly on a disposable Android API 35 AVD. They do not establish physical-phone audio/graphics behavior, animation quality, a manual playthrough or exhaustive parity. Complete fixtures do not shuffle ordinary entrances or clocks; focused native and host tests cover the existing clock modes. The dated 402-setting audit is a historical inventory; fresh source replay and APK provenance provide current source verification.

Only the APK, explicitly allowlisted source ZIP, fixes, validation and checksum manifest are distributed. ROMs, ROM-derived game archives, saves, placements, hints, private logs, screenshots, diagnostic executables, credentials and signing material are excluded. Save format and existing seed placements are unchanged.
