# Combo Ship 0.1.23 validation

Candidate: `ComboShip-Android-0.1.23-dev-arm64.apk`, package `org.comboship.android`, version code 24, version name `0.1.23-dev-combo-94eb185`.

SHA-256: `bb1e571d673d7a0fb2aceac6e7e2d21773aad7e147febd304bfaac6c44132550`. Size: 54,229,758 bytes.

This release finishes the documented supported additions in [the current logic audit](LOGIC-AUDIT-0.1.23.md). It retains the earlier work documented in [0.1.21](VALIDATION-0.1.21.md) and [0.1.22](VALIDATION-0.1.22.md). Earlier device checks are historical evidence, not claims that every previous feature was retested in this release.

## Build and provenance

The complete Android port stack replays from pinned ComboShip revision `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. All 177 changed native files match the fresh replay after newline normalization, and the APK source markers match current source bytes. Integration patch SHA-256: `b0e063d230241f01dd35f3a16ccee6a24a6f138c0e98ae56bbd906645054eaeb`.

Native compilation, runtime staging, Gradle assembly and lint completed. Eight ARM64 libraries, 218 required callbacks and 9,036 support-file hashes were checked. The installed APK matches the candidate bytes. Signing certificate matches 0.1.22 and 16 KiB ZIP alignment passes. Lint reports zero errors and 84 non-error findings.

The public source archive contains 150 explicitly allowed files. It excludes ROMs, extracted copyrighted game data, private geometry, saves, seeds, spoilers, logs and validation binaries. The APK's two included support archives were checked separately; neither is a user's extracted ROM archive.

## Host and actual native checks

All 24 host tests pass. Additional extracted production-function checks cover 4,096 Soaring cases, 3,584 egg cases and 424,192 Skulltula cases; their public rule tests cover 524,288, 832 and 64 prerequisite combinations respectively. Fourteen source-binding assertions confirm those helpers remain attached to their actual native paths. The headless rule-writer test exercises 26 cases, including absent gameplay services, live hook refresh, option/CVar writes and fallback behavior.

The final ARM64 engines pass all 106 actual reachability/source queries: 47 for the eight new MM techniques, 17 egg cases, 19 Eyeball Frog cases, 12 age-travel cases and 11 adult Soaring cases. These include required-item negatives and enhancement-only controls. The two older MM Lens techniques are included in snapshots and generation, but are not claimed as additional isolated cases in this 106-case count. Pocket Cucco's early-trade enum is covered by the production helper tests; the native Frog inventory cases exercise six other early trade variants.

The standalone generator initially exposed a null runtime-hook service while applying saved arrow rules. The production writer now preserves its option/CVar updates and only registers gameplay hooks when their service exists. The final strict generations below reran with that fix; the host guard test alone is not used as proof of generation.

## Strict combined generation and replay

These three tests use production pool preparation, filling, native reachability, hints and spoiler generation. Input fixtures contain settings only, with no supplied placements, prices or hints. Both bosses are required. Adult trades, Skulltulas, instruments/buttons, MM pots and owls are shuffled; native dungeon-item confinement remains enabled.

| Configuration | Cross-game placements | Unreachable OoT/MM checks | Original-rule replay |
| --- | ---: | --- | --- |
| OoT start; all 23 supported logical rules and 10 MM techniques | 398 | 0 / 0 | Pass, winning sphere 17 |
| MM start; the same supported rules and techniques | 414 | 0 / 0 | Pass, winning sphere 27 |
| OoT start; runtime enhancements on, logical assumptions and techniques off | 378 | 0 / 0 | Pass, winning sphere 23 |

Each output preserves all 23 runtime/logical rule values and all ten MM technique selections. The control preserves ordinary song destinations and zero logical assumptions. Native arrow settings reflect their canonical logical values. Replays use the original authored settings rather than relaxed logic or all-trick fallbacks. Each replay reaches a 1,325-item fixed point; this includes native fixed placements and is not a claim of 1,325 shuffled checks. No junk sits on unreachable checks and no missing checks are accepted by weakening the guarantee.

## Final APK emulator observations

The final candidate ran on the isolated API-35 emulator using ARM64 native translation:

- Adult Soaring disabled: the melody does not recognize a foreign Soaring route.
- Adult Soaring enabled without owls: native recognition and Link's performance complete, followed by the native no-destination message.
- Two owls: the native confirmation lists Clock Town, left/right changes it to Woodfall, and B cancels back to gameplay.
- Native pause save and cold restart retain all 23 rules and ten MM techniques. Adult Soaring remains runtime-on and logic-off, and works again after restart. Original seed settings, song mapping and placements remain unchanged; legitimate observed hint metadata may update.
- Global Soaring search opens the shared Randomizer gameplay-rules page. Pinnacle search opens its exact entry in the common 209-technique catalogue. About displays the APK's actual version.

Screens were inspected during the live tests; logs and saved JSON corroborate the listed behaviors. The native logic and collision/source audits for the newly catalogued jump techniques are distinct from manually executing those difficult maneuvers. Successful live jump demonstrations are not claimed for this release.

## Data protection and remaining validation limits

Five game/configuration files in each of the emulator's internal and visible storage roots were restored and verified byte for byte, plus the separately captured touch preference. Only the disposable read-only emulator was stopped. Visible storage was initially absent on that boot; the app's normal migration created it before fixture testing, and shutdown discards that read-only session. The preexisting internal originals were independently preserved.

No ROMs, private seeds, saves, derived geometry or logs are copied to delivery or OneDrive. Physical-phone audio latency, display cutouts/folding, controller differences, two physical multiplayer devices and a complete playthrough remain unverified by these emulator tests. Full parity with every ROM-randomizer trick is not claimed. Fierce Deity gap jumping remains outside logic at the user's request; the documented conservative Stone Tower bridge and other unsupported movement routes remain unchanged.
