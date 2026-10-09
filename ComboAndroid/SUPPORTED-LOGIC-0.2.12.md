# Supported logic: community reference and 0.2.12

The reference is OoTMM v32.3 at `413573d22d1d53e8a81931a70927f625b73488b9`. Equivalent supported mechanics should have equivalent requirements. Native gameplay differences must be accounted for explicitly rather than replacing an upstream rule with an invented shortcut.

| Rule | Community source | APK implementation and verification |
|---|---|---|
| Paid checks | [`has_rupees`, `can_use_wallet`, `wallet_price`](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/macros/macros_oot.yml) | `GetWalletCapacity` requires the mapped renewable rupee event. Current rupees never enter this logical calculation. Tests vary balance, wallet tier and source independently, including 19/20. |
| Free checks | Same `wallet_price` macro | Price zero remains reachable without a wallet or money source. Actual native isolated-shop tests cover this. |
| Scheduled arrival | [`evalExit` restriction propagation](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/packages/logic/src/pathfind/pathfind.ts) | `FindReachableRegions` preserves each route's allowed time mask for connections and exits. Searches consider possible times rather than wall-clock/current in-game time. |
| Waiting and closing | [Clock Town doors and `stay` rules](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/mm/overworld/clock_town.yml) | Native Milk Bar and inn opening expressions are compared directly with upstream expressions for every native time slice and key/membership combination. Waiting obeys the closing boundaries. Native inn tests cover room key, roof access, no paper, day-only clocks and all clock modes. |
| Consistent schedule predicates | Same pathfinder time restrictions | A conjunction cannot combine mutually exclusive hours from different visits. Time checks/events are evaluated over individual reachable slices. |
| Newly owned clocks | Native supported clock shuffle, matching time-mask principle | Root access expands; a newly owned clock does not automatically open a previously visited locked/closed room. The next search propagates legitimate arrivals. |

`tests/fixtures/ootmm-32.3-contracts.json` records the exact upstream expressions, source URLs and hashes. The upstream MIT notice is included in `logic-comparison-2026-10-04/OOTMM-LICENSE.txt`. `oot-resource-sources.csv` retains the source links for the previous resource mapping.

Previous age-change, local instrument/button, cross-song, Scarecrow, price, technique, rule-snapshot and runtime-only separation checks are retained in the test suite. The dated 402-setting comparison is a historical source inventory, not evidence that every website setting is implemented. The APK supports its existing OoT entrance shuffle and combined song destinations; unsupported MM/mixed entrance families and unsupported website item families are outside this update.

Validation establishes the tested contracts and complete native seed/replay fixtures. It does not establish exhaustive differential parity for every inventory and setting combination or a physical-device playthrough. No claim that all website logic is identical follows from a passing seed.
