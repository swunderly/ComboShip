# Combo Ship 0.2.14 validation

APK: `ComboShip-Android-0.2.14-arm64.apk`, version code 39, 54,408,424 bytes.

SHA-256: `77b1e32495b005ac7493d86a7d2c392f57402d5ef099d81723c7359e4aae6a95`

## Community reference and comparison

Reference: [OoTMM v32.3](https://github.com/OoTMM/OoTMM/tree/413573d22d1d53e8a81931a70927f625b73488b9), commit `413573d22d1d53e8a81931a70927f625b73488b9`. The official world builder, expression parser and Pathfinder are unchanged. Native results come from the final APK's eight ARM64 libraries, whose hashes are frozen before and verified after each run. No native graph or library is replaced during the release comparison.

- 1,591 equivalent inventory/settings cases produce **892,984 matched-check comparisons** with **zero unexplained differences**.
- The 121 differing case/check results fall into eight verified native-engine or setting categories documented in [LOGIC-PARITY-0.2.14.md](LOGIC-PARITY-0.2.14.md). Exceptions match the exact case, location and direction, rather than exempting a whole location.
- Across the tested pools, 2,227 of 2,303 eligible OoT checks and 2,284 of 2,381 eligible MM checks have reference counterparts. The remaining 76 OoT and 97 MM checks include native-only rewards, separate seasonal/form locations, extra scenery and internal markers; they are not counted as differential-tested counterparts.
- Matrices cover normal and Master Quest dungeons, both ages, supported techniques, paired setting boundaries, shared equipment tiers, sparse inventories, scenery, resource routes and combined rules. Actual fixed stock and prices are passed to the reference. All 1,591 cases retain identical fixed/price/eligibility metadata across the final native rebuild.
- A separate 270-case song matrix verifies **8,640 source/endpoint results**, with zero mismatches, for songs, local instruments/buttons, both ages, unlocked owls and enhancement-only activation.
- The importer records upstream hashes and the full MIT license for 49 valid MM combat expressions. Host fixtures exercise all 49 expressions across 111 inventory vectors (5,439 evaluations), while native regressions check the context-specific Moon encounters.

## Build and source verification

- Both ARM64 engines compiled; Android assembly and lint passed with zero lint errors and 84 non-error findings.
- All **37 host regression tests** passed.
- A fresh overlay replay reproduces all 240 patched native files from ComboShip `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. This release changes 48 native files through `patches/full-parity.patch`; all prior overlays remain applied.
- APK CRC, signature, update certificate, 16 KiB alignment, eight libraries, 231 callbacks, 9,036 support-file hashes and native source provenance passed verification. The installed task-emulator APK matches the release byte for byte.

## Packaged native regressions and tracker connection

All **84 focused cases** pass against the final APK's unchanged production libraries. These include earlier age-change, entrance, shop, resource and cross-song regressions plus ten Moon combat cases that distinguish valid attacks from unsupported alternatives.

Source verification confirms both trackers use the corrected production searches: OoT calls `InternalRecalculateAvailableChecks` and `ReachabilitySearch`; MM uses `RefreshChecksInLogic` under the live seed-rule and resource scopes. The combined tracker refreshes changed inventory/settings/context. Future reachable times, renewable resources and wallet capacity remain progression assumptions, rather than current rupee/ammo balances. Gameplay-only enhancements remain separate from serialized randomizer rules.

## Complete combined generation and replay

Three new seeds were generated with the final APK libraries and production combined generator, without preassigned placements. Each uses strict **Every check reachable** and the combined both-bosses goal.

| Fixture | Generation | Original-rules replay | Checks | Unreachable OoT / MM |
| --- | --- | --- | ---: | --- |
| OoT start, paired rules | PASS | PASS | 1,325 | 0 / 0 |
| MM start, paired rules | PASS | PASS | 1,325 | 0 / 0 |
| OoT start, enhancements only | PASS | PASS | 1,325 | 0 / 0 |

All three retain their requested 25-rule and 13-technique snapshots, native settings and actual shuffled prices. Bremen chick count stays outside logical assumptions. Enhancement-only activation preserves ordinary song destinations. Replay uses original rules, without all-tricks or No Logic fallback and without padding unreachable checks. The evidence verifier rejects six deliberately corrupted receipts in memory.

## Limits and privacy

Validation ran headlessly on a disposable Android API 35 emulator. It does not certify every possible inventory/settings combination, all shuffled entrance permutations, every native-only scenery check, a manual tracker playthrough, physical-phone rendering/audio or cross-warp animation quality. Complete generation fixtures do not shuffle ordinary entrances or clocks; focused tests cover supported age, entrance and clock scenarios. Unsupported website features are outside the comparison.

Only the APK, allowlisted source ZIP, release notes, validation, comparison document and checksums are distributed. ROMs, ROM-derived archives, saves, placements, hints, private logs, diagnostic binaries, credentials and signing material are excluded. Existing save format and seed placements are preserved.
