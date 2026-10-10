# Combo Ship 0.2.11

This update addresses renewable supplies in finding L06 of the [OoTMM source comparison](LOGIC-COMPARISON-2026-10-04.md). It follows the pinned OoTMM 32.3 world events and resource prerequisites, adapted to the native game's existing region graph. The approved menus and existing seed placements remain intact.

## Changes

- **Reachable ammo and magic supplies.** OoT generation and tracker searches now track renewable seeds, arrows, bombs, Bombchu-compatible drops and magic. Owning a weapon or magic upgrade alone no longer supplies ammunition. The normal age, equipment and item-use restrictions still apply. Native shop-refill routes remain valid alternatives.
- **Source prerequisites.** The port maps 107 reference events into 42 named native regions and five events inherited by generic grottos. Pots and grass retain the native ways of breaking or cutting them. Great Fairy magic requires playable Zelda's Lullaby. Bombchu-compatible bowling rewards require access to the minigame, the native Bombchu rule and sufficient wallet capacity. The [mapping table](../../oot-resource-sources.csv) records the reference file, exact line, condition and native adaptation for every mapped event.
- **Money for paid checks.** OoT's shared shop, scrub and merchant affordability helper now requires reachable renewable rupees for seed generation. Wallet capacity alone no longer makes a paid check reachable. Free checks remain free; sources do not grant a missing wallet. Bowling also retains its actual 30-rupee entry requirement.
- **Held supplies in the live tracker.** During play, available ammunition, magic and rupees can satisfy the corresponding checks even without a renewable source. Spending the last shot, exhausting magic or changing the rupee balance refreshes OoT availability. These finite live supplies never become generation assumptions.

The reference is [OoTMM commit 413573d](https://github.com/OoTMM/OoTMM/tree/413573d22d1d53e8a81931a70927f625b73488b9), including its [OoT resource macros](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/macros/macros_oot.yml) and the world files identified in the mapping table. No new traversal trick or replacement solver was introduced.

Existing runs receive tracker corrections without reshuffling their items. New seeds use the new source events. Paired logic rules, enhancement-only choices, local age-change behavior, ordinary pedestal travel and all prior route corrections are retained.

## Remaining differences

L06 is improved, not closed. OoTMM also tracks reusable randomized refill placements; this port retains native shop-refill flags and does not yet reproduce that entire accounting model. One-time ammo chests are not treated as renewable. The coarser native region graph does not establish individual-pot or complete MQ equivalence. Full MM resource, time/cycle and cross-game entrance models, additional item families and remaining dungeon/key differences still need work.

Native shared-item options share upgrades, not ammo, magic or rupee balances. Foreign-game refills are therefore not imported as native sources. This update does not simulate a budget for every shot or magic use, and some paid minigames still use their existing wallet-capacity predicates. It is not a claim that every randomizer setting or website route now has exact parity.

See [validation](VALIDATION-0.2.11.md) for the production tests and their limits.
