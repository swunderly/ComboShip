# Combo Ship 0.2.10

This update improves the progression and tracker gaps identified in the [OoTMM source comparison](LOGIC-COMPARISON-2026-10-04.md). The comparison remains a dated baseline, with its original 402 website settings, 361 native options, 24 paired rules and 33 findings. This release has 25 paired rules. The approved menu organization remains in place.

## Changes

- **Broader age-change planning.** The generator and OoT tracker now permit local Song of Time age change in 166 named regions, up from 76. The 90 added regions are dry vanilla Spirit Temple, Forest Temple and Gerudo Training Ground rooms. A route can enter using one age and use the other age's equipment after arriving. This follows the mixed-age approach in [OoTMM's Spirit Temple graph](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/oot/dungeons/spirit_temple.yml), adapted to the native game's collision and item rules. The selected instrument, Song of Time and its playable buttons remain necessary. Normal pedestal travel, the existing dry Water Temple entrance route, key requirements, projectile compatibility and boss requirements remain separate.
- **Spirit Temple certainty.** When native key analysis guarantees access to a permitted room as one age, local age change shares that access with the other age. This fixes the tested child-crawl/adult-Hookshot North and East climb chests. It retains the native analysis of competing key doors; minimum-key paths that remain ambiguous are still conservative.
- **Active abilities in both trackers.** OoT and MM availability searches use the corresponding active gameplay enhancements. Generation continues to use the independently selected randomizer assumptions. Changing a paired OoT ability invalidates tracker availability, extending the existing age-change refresh to the complete rule catalogue. OoT Blue Fire and Sunlight Arrows also retain the native seed-option alternative used by their actual actor hooks.
- **MM Scarecrow prerequisites.** Ordinary Scarecrow traversal requires a reachable teacher or a song already taught in the current cycle. The instrument, Hookshot and two distinct playable notes remain necessary. A new paired option, **Scarecrow without teaching a song**, appears under Randomizer / Gameplay rules / Majora's Mask and Enhancements / Time savers / Majora's Mask. The enhancement-only choice can leave generation conservative. [OoTMM's corresponding macro](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/macros/macros_mm.yml) supplied the taught-or-free distinction; the native game supplied its local note and actor requirements.
- **Price correctness.** OoT's unknown range-price estimate now reads both endpoints for shops, scrubs and merchants. MM affordability now uses finite native wallet limits instead of treating the Giant Wallet as unlimited: 99, 200, 500 and, when enabled and owned, the native 5,000-rupee Tycoon Wallet.

These changes do not reshuffle an existing seed or rewrite its item placements. Existing runs benefit from the tracker corrections; new generation uses the expanded routes. Bremen chick count remains a gameplay difficulty preference, and Fierce Deity gap jumps remain excluded as requested.

## Comparison status after this update

| Finding | Status |
| --- | --- |
| L02 / L04: age change and mixed-age dungeon routes | Improved for the supported dry rooms, including the two concrete Spirit routes. Full dungeon/MQ graph equivalence is not established. Native immediate age change intentionally retains its existing equipment contract rather than requiring the website's prior Temple-of-Time event. |
| L20: MM Scarecrow learning and buttons | Learned-or-free requirement added with teacher events, a paired option, negative prerequisites and active tracker context. Clock Town savewarp normally makes the teacher reachable, so this is not a claim that every older MM seed was invalid. |
| L27: unknown OoT range-price estimate | Corrected and tested for both endpoint orders across all three price families. |
| L26: prices and wallets | Finite MM capacities corrected against the native runtime. Website price distributions and wallet extensions still differ. |
| L33: live abilities versus seed assumptions | Central readers now distinguish tracker and generation context; rule isolation and actual predicate behavior are tested. Native seed flags remain valid runtime ability sources. |

The largest remaining differences are renewable ammo/magic/rupee source modeling, complete MM and cross-game entrance shuffle, the website's foreign/ageless/soul item extensions and shared tiers, MM time/cycle route semantics, and remaining conservative dungeon/key routes. Many are missing feature families or intentional native policies, not confirmed defects in an ordinary seed. The comparison inventories retain exact source references and distinguish those cases. Full website-equivalence is not claimed.

See [validation](VALIDATION-0.2.10.md) for the tests and their limits.
