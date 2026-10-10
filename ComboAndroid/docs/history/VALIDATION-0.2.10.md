# Combo Ship 0.2.10 validation

APK: `ComboShip-Android-0.2.10-arm64.apk`, version code 35, 45,676,083 bytes.

SHA-256: `1e8858b429eed21c3b67fb7c41629e8a0971ce9e084f0070be5e36dab019070d`

## Build and reproducibility

- ARM64 native compilation, Android assembly and lint succeeded. Lint has zero errors and 84 non-error findings. Existing compiler/lint findings are not represented as a clean general bug audit.
- All 30 host regression tests passed. New tests compile the actual OoT rule reader, MM tracker context, Scarecrow and affordability macros, OoT arrow predicates and price estimator. They cover independent runtime/generation values for all 25 rules, nested tracker-context restoration, 40 Scarecrow prerequisite assertions, 40,808 MM price/capacity boundaries, both range orders in all three OoT price families, and native-seed versus enhancement arrow abilities.
- The age-propagation test compiles the production search block across every native region, all five modes, both origin ages and day/night states. All 166 permitted regions also receive separately missing song, instrument, button, Ocarina-of-Time and sword cases. Unsafe water, boss, minigame and unaudited MQ regions remain excluded; the fixed-point termination and original context are checked.
- A fresh overlay replay from `94eb185e4abcc2d568aa8241fa02c43cdd86c439` reproduces all 209 patched native files. This incremental component changes 11 native files. Existing overlays remain intact.
- APK signature verification and comparison with the earlier update certificate passed. Archive CRC, 16 KiB alignment, eight ARM64 libraries, 231 exported callbacks, every one of the 9,036 support-file hashes and every packaged native source hash passed verification. The final installed emulator APK matches this release byte for byte.

## Actual packaged native route tests

All 45 route/prerequisite cases passed against the final APK's real ARM64 OoT and MM libraries. The private test executable calls the production reachability exports; it does not implement a replacement solver. Its receipt is bound to the APK, library and driver hashes.

The retained Water Temple fixtures cover the child-only Deku Tree entrance swap, Hookshot rather than Longshot, age change, ordinary pedestal travel and separately missing requirements. Forest Temple/Training Ground fixtures retain compatible Slingshot eye/switch routes and Bow-only counterexamples. The added Spirit cases verify both child-crawl/adult-Hookshot climb chests, missing instrument/song/button/key rejection, an ordinary child Slingshot route, enhancement-only generation isolation, minimum-key uncertainty and the boss-key restriction. MM cases retain a reachable Scarecrow teacher or the free rule while separately removing the instrument, Hookshot or second note.

The same two Spirit climb cases fail against the preserved previous working 0.2.9 APK and pass with 0.2.10. This is a concrete before/after native comparison. It is not a claim that all 45 routes were different or that the two engines now match every website predicate.

## Complete native generation and replay

Three new fixtures generated item placements and prices with the final APK's native libraries and the production combined generator. They contain no preassigned placements. Each uses the strict **Every check reachable** setting and the combined both-bosses goal.

| Fixture | Generation | Original-rules replay | Generated checks | Unreachable OoT / MM |
| --- | --- | --- | ---: | --- |
| Starts in OoT, paired logic rules enabled | PASS | PASS | 1,325 | 0 / 0 |
| Starts in MM, paired logic rules enabled | PASS | PASS | 1,325 | 0 / 0 |
| Starts in OoT, enhancements enabled with logical assumptions cleared | PASS | PASS | 1,325 | 0 / 0 |

The fixtures exercise all 25 rule fields and ten MM techniques, with shuffled local instruments/buttons, adult trades, Skulltulas, MM pots and owl checks. Runtime-only Bremen fields remain zero in the logical snapshot. Saved settings, rule snapshots, selected techniques, song-map flags and generated prices match the inputs. The enhancement-only control keeps ordinary song destinations. Replay passes with the original selected rules; it does not need a diagnostic all-tricks retry or No Logic fallback.

An earlier trial master seed rejected two confined-fill candidates before producing a valid seed; those failure logs are retained privately. The three final fixtures use the previously established deterministic test masters and pass their first native retry index. This is successful validation of three concrete seeds, not evidence that arbitrary settings always generate on their first attempt. The read-only receipt verifier also rejects six deliberately corrupted receipts in memory.

## Validation limits and privacy

The final APK was installed on the disposable read-only Android API 35 AVD. Its setup screen launched without a crash, and the installed APK hash was verified. Native generation and route tests run headlessly without a ROM. This pass does not claim a physical-phone test, an in-game animation check, a full manual playthrough, or visual/gameplay verification in each of the 90 additional rooms. The seed guarantees apply to the native model; the [comparison and remaining differences](FIXES-0.2.10.md) still matter.

The dated 402-setting source comparison remains unchanged. Its verification describes the pre-change audit and must not be treated as a post-change source hash check. This release's fresh source replay and APK verification provide that check.

Only the APK, an explicitly allowlisted source ZIP, fixes, validation and SHA-256 manifest are distributed. The source ZIP includes the comparison tables, their source references and the upstream license. ROMs, ROM-derived game archives, saves, generated placements/hints/prices, private screenshots and logs, diagnostic executables, credentials and signing material are excluded. No existing seed placement or user save is migrated by this update.
