# Resource logic comparison, 0.2.13

Reference: OoTMM v32.3, commit `413573d22d1d53e8a81931a70927f625b73488b9`. This extends the supported-setting audit and 0.2.12 wallet/time corrections. It does not add unsupported website features or certify every possible inventory and setting combination.

## Source definitions

| Behavior | Pinned community definition | Native adaptation |
| --- | --- | --- |
| Money | [OoT macros](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/macros/macros_oot.yml), `has_rupees`, `can_use_wallet`, `wallet_price`; corresponding MM macros | Wallet capacity plus an actual reachable renewable money source; free checks require no money source. Current rupees are not read. |
| Magic and arrows | [MM macros](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/macros/macros_mm.yml), `has_magic`, `has_arrows`; corresponding OoT macros | Equipment/capacity plus reachable refills. Owned magic-capacity upgrades count even when current magic is zero. |
| MM bombchus | MM macros, `bombchu_license`, `bombchu_source`, `has_bombchu` | Native MM uses the bomb-bag license. Its unchanged Bomb Shop shelf can create the bombchu inventory slot on purchase; finite bombchu ownership is not required first. Shuffled shelves supply only their actual assigned refill. |
| Free MM sources | [Termina Field](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/mm/overworld/termina_field.yml), [Swamp Front](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/mm/overworld/southern_swamp.yml), [Clock Town](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/mm/overworld/clock_town.yml), [Woodfall](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/mm/overworld/woodfall.yml) | Their money/ammo/magic/stick/nut events map to the corresponding native regions. Baba and Chuchu sources retain enemy-soul and usable-equipment conditions. These outdoor sources cover the ordinary connected MM world; ordinary MM entrances are not shuffled by this app. |
| Refillable shelves | MM macros' renewable item predicates and actual native `EnGirlA` restocking | A shelf must be reachable at a valid time, payable and assigned a refill. Bottled magic refills require a bottle. A paid refill cannot supply its own starting money. Native vanilla refill fees fit the base wallet except Chateau, which needs the 200-rupee wallet. Unused rolled prices on unchanged shelves are ignored; genuine shuffled prices remain wallet gates. |
| Honey & Darling | Clock Town's `HD_REWARD_3` and `Honey & Darling Reward All Days` | Day three accepts Bow or Deku bubbles. The native minigame supplies ammo and refills acquired magic; it does not grant missing equipment or magic capacity. All-days retains Bow, money and all three events. |

The hashed reference contracts are checked in `tests/fixtures/ootmm-32.3-contracts.json`. When the pinned reference checkout is present, tests verify source hashes and exact expressions. `oot-resource-sources.csv` retains the earlier OoT source/event mapping.

## Consistent searches

MM's combined generation oracle, native placement search and check tracker use the same search-local resource state. Each new search starts empty, discovers sources monotonically through reachable regions and valid times, and restores the previous state when it ends. Failed placement trials clear their temporary sources. No resource state is added to persistent saves.

OoT uses its existing reachable resource events. Temporary held ammo and magic no longer override those events for the tracker. Native infinite quiver, bullet bag, bomb bag, sticks, nuts, magic, bombchus and wallet upgrades read their actual saved ownership flags.

## Actual engine boundaries

- Native MM wallet capacities are 99, 200, 500 and 5,000. Website-only wallet tiers are not invented in the APK.
- Native OoT randomized replacement shop rewards are single purchases. Their repeatable vanilla buy-item stock remains a valid refill source; a single-purchase randomized reward is not made renewable merely because a different engine treats a shelf differently.
- The APK does not have MM's website bombchu bag modes or arbitrary mixed ordinary entrance shuffling. The resource adapter follows the implemented native bomb-bag behavior and connected MM map.
- Existing differences explicitly accepted or tied to native behavior remain documented in the supported-setting audit. This release does not replace native room graphs with unsupported website routes, silently enable tricks, or weaken strict generation after a failure.

See `SUPPORTED-LOGIC-0.2.12.md` for the preceding schedule and wallet correction and `LOGIC-COMPARISON-2026-10-04.md` for the historical setting inventory.
