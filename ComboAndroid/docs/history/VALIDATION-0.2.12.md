# Combo Ship 0.2.12 validation

APK: `ComboShip-Android-0.2.12-arm64.apk`, version code 37, 57,164,740 bytes.

SHA-256: `97be8837a7aa33570c4214cace866e194f22c51f3d78bbf4975d0babb0b230ca`

## Community contracts

The reference is OoTMM v32.3 at `413573d22d1d53e8a81931a70927f625b73488b9`. The [source comparison](SUPPORTED-LOGIC-0.2.12.md) identifies the exact wallet, arrival-time and scheduled-door definitions used. Money tests vary current balance independently of wallet capacity and reachable rupees. Door tests compare native expressions directly with the pinned upstream expressions over every native time slice and key/membership combination.

This is a comparison of supported mechanics, not a declaration that every website option, check or inventory combination is equivalent. Native fees and gameplay prerequisites remain those implemented by the APK.

## Build and source verification

- Both ARM64 game engines compiled. Android assembly and lint passed with zero lint errors and 84 non-error findings. Existing findings are not represented as a clean general bug audit.
- All 32 host regression tests passed. These include the actual production wallet helper, tracker cache invalidation, MM time predicate evaluation, arrival filtering, closing boundaries, alternate entry, clock ownership and exception-safe time restoration. Current rupees changing from 19 to 20 does not change logical availability or invalidate its cache.
- A fresh overlay replay from ComboShip commit `94eb185e4abcc2d568aa8241fa02c43cdd86c439` reproduces all 214 patched native files. This incremental patch changes 16 native files; prior overlays remain intact.
- APK archive CRC, signature, update certificate, 16 KiB alignment, eight ARM64 libraries, 231 exported callbacks, 9,036 support-file hashes and packaged native source hashes passed verification. The installed task-emulator APK matches the final release byte for byte.

## Actual packaged native tests

All 65 route and prerequisite cases passed against the final APK's ARM64 libraries. The private driver calls the production search exports and is bound to the APK and library hashes.

The retained cases cover shuffled Water Temple access, ordinary pedestal travel, local age change, compatible Slingshot targets and Bow-only counterexamples, mixed-age Spirit Temple paths, cross-song prerequisites, MM Scarecrow learning and isolated shop affordability. Added cases exercise the inn at night, room-key and roof alternatives, missing paper, day-only access, ascending and descending clock modes, and adult gallery equipment and payment.

Three cases reproduce incorrect availability in the preserved 0.2.11 APK: the inn toilet check after closing on nights one and two without a room key or roof access, and a paid gallery isolated from all rupee sources. All three are correctly rejected by 0.2.12. The other 62 retain their expected results.

The descending-clock fixture with no explicitly configured clock retains the native computed first clock. Its first available half-day is night three, when the inn is open. This matches the reference `clock_night3` rule using `clock_descending(0)`; the test does not incorrectly assume that an empty configured kit disables the guaranteed starting half-day.

## Complete combined generation and replay

Three fresh fixtures generated actual placements and prices using the final APK's libraries and the production combined generator. Inputs contained no preassigned placements. Each used strict **Every check reachable** and the combined both-bosses goal.

| Fixture | Generation | Original-rules replay | Generated checks | Unreachable OoT / MM |
| --- | --- | --- | ---: | --- |
| Starts in OoT, paired rules enabled | PASS | PASS | 1,325 | 0 / 0 |
| Starts in MM, paired rules enabled | PASS | PASS | 1,325 | 0 / 0 |
| Starts in OoT, enhancements enabled with logical assumptions cleared | PASS | PASS | 1,325 | 0 / 0 |

The fixtures exercise 25 rule fields and ten MM techniques, shuffled local instruments/buttons, adult trades, Skulltulas, MM pots and owl checks. Bremen count fields remain zero in the logical snapshot. Saved rules, native settings, techniques, destination flags and generated prices match the inputs. The enhancement-only control retains ordinary song destinations. Replay succeeds under the original selected rules, without all-tricks or No Logic fallback, and without padding unreachable locations.

All three fixtures pass at their first native retry index. This validates three deterministic seeds, not a promise about arbitrary settings. A separate read-only verifier rejects six deliberately corrupted receipts in memory.

## Validation limits and privacy

The final APK was installed only on the disposable Android API 35 task AVD. Production route and generation tests ran headlessly. This does not establish physical-phone audio/graphics behavior, animation quality, a manual playthrough or exhaustive community-logic parity. Clock modes are covered by focused native and host tests; the three complete generation fixtures do not shuffle clocks or ordinary entrances.

The dated 402-setting audit remains a historical inventory. Its old source hashes are not post-change evidence; this release's fresh source replay and APK verification provide that check. Earlier documented resource-accounting and unsupported-feature differences are not silently declared resolved by these tests.

Only the APK, an explicitly allowlisted source ZIP, fixes, validation and SHA-256 manifest are distributed. The ZIP includes reference expressions and the upstream MIT notice. ROMs, ROM-derived game archives, saves, generated placements/hints/prices, private logs and screenshots, diagnostic executables, credentials and signing material are excluded. Existing save data and seed placements are not migrated.
