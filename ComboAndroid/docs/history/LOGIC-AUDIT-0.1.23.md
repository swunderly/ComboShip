# Logic additions in 0.1.23

This release extends the explicit supported rule registry. It retains the previously delivered combined generation guarantees, native song performances, screen/HUD fitting, sliders, unified trackers, path hints and warning-only run overrides. The original generated rules remain separate from saved run overrides. Randomizer selection enables the corresponding ability and its logic; enhancement-only selection does not require the new route in generation.

## Additional progression rules

| Option | Default | Modeled effect and retained requirements |
| --- | --- | --- |
| Song of Soaring at any age (OoT) | Off | Adult Link can use the existing cross-game Soaring routes. Cross-game songs, learned Soaring, the local usable instrument, D/L/U buttons and an activated source owl still apply. No owls permits song recognition/performance and the native no-destination message, but supplies no route. |
| Zora eggs required for Bossa Nova (MM) | Seven | Logical counts 1–7 use that many distinct reachable egg events. A reusable bottle, Zora form and MM's local instrument remain necessary. The song and its note buttons are not required to learn the randomized lesson reward. Logical zero means vanilla seven. |
| Night Skulltulas during the day (OoT) | Off | Thirteen audited check/age pairs at the Graveyard wall, Fountain log, Fortress roof/range, Kakariko and Ranch can be collected during the day without Sun's Song. Location, combat, collection equipment and age requirements remain. Other checks retain their ordinary time rules. |
| Early Eyeball Frog | Restoration and trick off | After thawing King Zora, Adult Link can use a usable equipped shield while holding R with an earlier adult-trade item. The normal randomized Prescription/Eyeball Frog check is awarded. Blue Fire/thawing, adult age, actual trade inventory and speaking to Zora remain necessary. Enable its assumption only in **Randomizer → Tricks & glitches**; the restoration alone never selects the trick. |

Early adult-trade inventory includes Pocket Egg/Pocket Cucco, Cojiro, Odd Mushroom, Odd Potion, Poacher's Saw and Broken Sword. Empty slots, Eyeball Frog, Eyedrops and Claim Check do not qualify. The ordinary Prescription route remains available independently. Its predicate also now respects the native shuffled Speak Zora requirement.

## Song of Time locations

The existing equipment modes now propagate age access in twenty audited ordinary house/shop/laboratory regions and Lon Lon Ranch, in addition to the earlier overworld allowlist. Native collision headers were inspected locally across age/time setups, and the native switch preserves room, position and static-background camera. Each area keeps its normal entry, item-use, check and exit gates. The dry-well route remains supported.

Underwater laboratory positions, Dampe's House, grottos with alternate setups, minigames, bosses and arbitrary dungeon shortcuts remain outside this region-level assumption. The helper covers sixty permitted native region enums; this is deliberately narrower than unrestricted runtime age changing.

## MM techniques and reference comparison

Eight optional techniques extend the two existing Lens techniques. Their descriptions, scope and source links are in [MM tricks reference](MM-TRICKS-0123-REFERENCE.md). They use one common catalogue, metadata search and saved rule block. The Stone Tower one-mask option covers climbing to top/owl routes; this implementation retains the ordinary temple bridge requirements.

The comparison uses the [official OoTMM trick definitions at the audited revision](https://github.com/OoTMM/OoTMM/blob/a284b8aab4c707ceb35278fbc66a2271002a20f0/packages/core/src/settings/tricks.ts) and its native world/macro definitions. It verifies the listed route alternatives and requirements against the actual ports; it does not claim complete parity with the ROM randomizer's different equipment and technique catalogue. The Early Eyeball Frog and daytime Skulltula options are native Ship extensions with their own audited requirements.

Fierce Deity continues to provide the previously verified sword, shallow-water and Big Octo capabilities. Extra gap jumps remain outside logic at the user's request. Arbitrary speed, Moon Jump, unrestricted item use, Hookshot Anywhere, unrestricted flower glide and Get Item Manipulation are not modeled as required routes. Ordinary quantity/time-saving settings that preserve the same repeatable source, inventory and entrance requirements do not create a new route alternative.

## Validation boundary

[Versioned validation](VALIDATION-0.1.23.md) distinguishes production predicate tests, actual native engine queries, seed generation and emulator observations. Local ROMs, geometry, private test saves and logs are excluded from the APK/source delivery. Physical-phone cutouts, folding, acoustic latency and a full playthrough remain device testing limits.
