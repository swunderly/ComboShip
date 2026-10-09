# Supported logic comparison, Combo Ship 0.2.14

Reference: [OoTMM v32.3](https://github.com/OoTMM/OoTMM/tree/413573d22d1d53e8a81931a70927f625b73488b9), commit `413573d22d1d53e8a81931a70927f625b73488b9`. This supersedes the historical gap descriptions where the corresponding 0.2.12–0.2.14 corrections apply.

## Method

The private comparator sends equivalent inventories, setting values, techniques and actual fixed stock/prices to the unmodified upstream world builder and Pathfinder, and to the APK's production native search exports. No native graph is substituted. The reference normalizer rejects unknown keys, enum values and tricks. Previously solved reference cases are reused only when their complete request inputs remain equal.

The matrix covers missing-item counterexamples; normal and Master Quest layouts; deterministic sparse inventories; paired setting boundaries; supported techniques; mixed-age targets; combined routes and shared equipment tiers; renewable resources; and the native extension settings. Cross-song source permissions have a separate matrix for both ages, instruments, buttons, songs, owl unlocks and enhancement-only activation.

Locations are matched by explicit identity or the actual native actor/scene/flag protocol. Numbered scenery names alone are not treated as proof that two locations are the same. Native separate seasonal/form checks, extra ceiling icicles, native-only drops and root/end-goal markers do not always have a one-to-one upstream reward location. These are not silently described as compared locations.

The checked-in importer translates 49 valid upstream MM combat expressions with source hashes and the full MIT license. Native souls remain separate prerequisites. Context-specific Moon encounters use the actual [Moon YAML](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/mm/dungeons/moon.yml), whose fight requirements differ from generic enemy macros. Foreign items that the native MM player cannot use do not gain invented combat abilities.

## Actual game and setting differences

| Difference | Native behavior and evidence |
| --- | --- |
| Moon trial access | The native 20-mask option requires cumulative counts 2/6/12/20. `mm/2s2h/Rando/ActorBehavior/EnJs.cpp` changes the entrance dialogue and `Logic/Regions/Moon.cpp` matches it. OoTMM's last trial uses its own four-mask requirement. |
| Fierce Deity reward | Native `EnJs.cpp` makes the gift available at 20 owned masks, without requiring all four completed trials. OoTMM's gift depends on trial-completion events. |
| Adult start | Native `soh/soh/Enhancements/randomizer/savefile.cpp` gives and marks the Master Sword-location reward at initialization when starting adult. The comparator does not pretend that already-credited reward is a new visit. |
| Open Gerudo Fortress | The native card location is processed as a native reward, while the website's open-fortress transformation removes or automatically assigns the corresponding location. |
| Gorman milk price | The native randomized purchase uses its actual rolled price (`Logic/Regions/MilkRoad.cpp` and the actor's rando price). The website location retains its fixed milk fee. Wallet and renewable-money rules still apply. |
| Powder Keg trial | Native `ActorBehavior/EnGo.cpp` supplies the temporary keg for Medigoron's trial even before finding the randomized permanent Keg item. The native license/trial route can therefore open the race without owning that item. |
| Gold Dust refill | Native `ActorBehavior/EnGk.cpp` still fills an empty bottle with Gold Dust at the race while queuing its randomized reward. The website separates its randomized reward from this resource. |
| Kotake potion | Native `ActorBehavior/EnTrt.cpp` still fills an existing empty bottle with red potion while queuing the shuffled Kotake reward. This can save Koume without another potion source. |

Exceptions are restricted to the exact tested inventory/setting case, location and direction. A differing result in another case is a failure, even at one of these locations. These differences describe actual engine behavior; changing only their logic to the website would misrepresent the APK.

## Seed generation and tracker

Both corrected native graphs serve generation and tracker planning. OoT's tracker calls `InternalRecalculateAvailableChecks` and the production `ReachabilitySearch`; MM's `RefreshChecksInLogic` uses the native region/event fixed point under `LiveTrackerRuleScope` and the shared resource scope. The combined tracker refreshes these results as inventory, game, settings and context change.

Reachable future times, repeatable sources and wallet capacity are used. Current rupees, ammo or temporary magic balance do not replace progression logic. Randomizer rules enable their runtime enhancements; gameplay-only activation does not change serialized seed assumptions. Existing placements are not regenerated during an update.

## Limits of the result

The release closes the identified gaps and verifies the supported-feature matrix. It does not certify every mathematical combination of inventories, all randomized entrance permutations, every native-only scenery check, or a physical-device playthrough. Unsupported website entrance/time-cycle modes, foreign equipment, counters and pool features are not added by the comparison. Seed numbers and placements need not match across different generation algorithms.

See VALIDATION-0.2.14.md for the final APK-bound counts and receipts.
