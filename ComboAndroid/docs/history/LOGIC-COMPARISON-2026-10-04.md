# Combo Ship / OoTMM logic comparison

Source audit dated **2026-10-04**. **Our logic is not equivalent to the OoTMM website's logic.** The two projects share many setting families, but their item pools, traversal predicates and supported combinations differ. The recent tracker/age-change work closes particular native routes; it does not establish website-wide parity.

This audit inventories **all 402 stable website setting keys**, **all 361 native option registrations** (295 OoT and 66 MM), the **24 combined gameplay-rule entries**, all **149 reference / 197 native OoT / 10 native MM techniques**, and eight structured setting inputs. Every scalar setting is represented exactly once in its catalogue. Internal, derived and retired native registrations remain visible in the reverse inventory; their presence does not prove a visible or functioning UI option.

**No production logic or menu code was changed for this comparison.** The catalogue covers every setting declaration; the reviewed findings below compare consequential implementation families. It is not a proof that every individual check, technique or combination behaves identically. A row marked **Analogue; parity unverified** identifies a related native feature, not a passing equivalence test. **No mapped seed counterpart** means no same configurable policy was established in the reviewed registries/tables; a related fixed rule or runtime preference may still exist.

## Sources and reproducibility

- Primary reference: the [OoTMM website](https://ootmm.com/), stable **v32.3**, pinned commit `413573d22d1d53e8a81931a70927f625b73488b9`. The report uses its actual [setting definitions](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/packages/core/src/settings/data.ts), world YAML, item-pool transformations and solver/macros, rather than relying on labels or recalling wiki descriptions.
- Development reference: pinned commit `41a2b7c0d7dbbb53fed928652f8271bc336c113f`, with **433** keys. [development-delta.csv](../../logic-comparison-2026-10-04/development-delta.csv) lists added/removed registry keys only. Development features are not counted as stable website features; changed behavior under an unchanged development key is not fully audited here.
- Native source: base commit `227a76addad54e583d2c3184be9f3027e5e0242f` plus the existing dirty Android overlays and 0.2.9 tracker/age-change work. A base commit alone cannot reproduce that snapshot. [coverage.json](../../logic-comparison-2026-10-04/coverage.json) and [reviewed-source-hashes.csv](../../logic-comparison-2026-10-04/reviewed-source-hashes.csv) record the inspected content hashes.
- Setting consumers are indexed from literal source references. Dynamic lookups, resolved flags, generated item tables and indirect helper calls can escape that index. A blank consumer cell is not proof that an option is unused.
- Catalogue defaults are declaration defaults; the native combined menu, presets, imports and compatibility normalization can override them. They are not a claim about a particular player's current settings.
- Source metadata is attributed to the OoTMM Team under its MIT license; [reference license](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/LICENSE) and the bundled `OOTMM-LICENSE.txt` apply to copied reference metadata.

## Full inventories

| File | What it contains |
| --- | --- |
| [website-settings.csv](../../logic-comparison-2026-10-04/website-settings.csv) | All 402 stable keys, defaults, value sets/bounds/conditions, native mappings, differences and source pointers. |
| [native-settings.csv](../../logic-comparison-2026-10-04/native-settings.csv) | All 361 native option registrations, defaults/value expressions, consumers and reverse reference associations. |
| [gameplay-rules.csv](../../logic-comparison-2026-10-04/gameplay-rules.csv) | All 24 combined runtime/logic rules, including legacy, runtime-only and trick-governed entries. |
| [tricks.csv](../../logic-comparison-2026-10-04/tricks.csv) | All 356 reference/native technique entries; 34 reviewed aliases. Unmapped techniques are not silently declared absent. |
| [structured-settings.csv](../../logic-comparison-2026-10-04/structured-settings.csv) | Starting items, exclusions, tricks, custom counters, three plando fields and structured hints. |
| [findings.csv](../../logic-comparison-2026-10-04/findings.csv) | 33 reviewed findings, with practical implications, certainty boundaries, sources and suggested next steps. |
| [development-delta.csv](../../logic-comparison-2026-10-04/development-delta.csv) | Added/removed development keys, kept separate from stable comparison. |

These are catalogue sizes, not numbers of bugs or a percentage of logic coverage. Many reference toggles describe features that Combo Ship does not offer; other differences are intentional runtime changes.

## Coverage by setting family

| Stable category | Keys | Comparison scope and conclusion |
| --- | ---: | --- |
| main | 13 | Games, multiplayer, goals, pool, guarantees and spoiler output; native combined generator is a different solver. |
| hints | 9 | Nine scalar selectors plus the structured hint list; different algorithms/output surfaces. |
| main.shuffle | 77 | Pool membership and placement/confinement; many related native switches, with grouped or narrower options. |
| main.prices | 5 | Five price families; OoT native policies differ and MM lacks the same separate selectors. |
| main.events | 36 | Door/dungeon/world states, age change, custom gates, Scarecrow, pre-completion and starting unlocks. |
| main.cross | 2 | Cross-game age and Farore Wind; no equivalent combined native policy. |
| main.camc | 5 | Chest appearance and knowledge; presentation compatibility is separate from progression. |
| main.misc | 7 | Clock/time/damage/movement extras; source distinguishes runtime preferences and logical abilities. |
| main.nologic | 8 | Explicit reference non-logical choices; native FD and OoT damage policies intentionally differ. |
| items.progressive | 7 | Sword/shield/lullaby tiers; native fixed item policies and toggles differ. |
| items.extensions | 93 | Foreign abilities, wallets, buttons, souls, coins, clocks and trap types; many website extensions lack a mapped native seed control. |
| items.shared | 69 | 69 reference selectors versus 17 native table families, with tier/direction restrictions. |
| items.ageless | 14 | Fourteen ageless selectors; runtime cheats and changing age do not establish equivalent per-check predicates. |
| entrances | 44 | 44 selectors; native OoT entrance categories plus mixed song travel are narrower than the combined reference. |
| main.world | 2 | MQ and Japanese layouts; native MQ exists, exact graph translation is unproved; no mapped JP selector. |
| main.qol | 11 | Cycle retention, masks, shop hours, spikes, shadows/lightblocks and speed; runtime analogues are not all paired seed rules. |

The structured fields matter as much as the scalar settings. In particular, the website's five custom gates can combine **19 counter families**; native local OoT counters and MM mask/remains thresholds are not that expression system. The website's generation-time plando and weighted seven-type hint lists also cannot be represented by the native finished-seed editor and hint presets without a translation layer.

## Highest-impact progression differences

### Age change is broader in one respect and narrower in another

The website first requires normal Temple of Time time travel before enabling at-will Song of Time age change. It then allows age transitions through most OoT graph areas, with explicit exclusions. Ours instead checks local song/equipment immediately and restricts generation to **76 named regions**: 40 overworld regions, 20 ordinary interiors, three Water Temple entrance/ledge regions and 13 Forest Temple/Training Ground regions. The website's graph includes explicit mixed-age shortcuts in additional dungeons, including Spirit. ([Reference age event](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/oot/overworld/temple_of_time.yml#L10); [native equipment contract](C:/Users/swunderly/Documents/Codex/2026-09-25/i-want-you-to-create-an/native-source/combo/rando/GameplayRules.h:182); [Spirit routes](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/oot/dungeons/spirit_temple.yml#L32); [native area allowlist](C:/Users/swunderly/Documents/Codex/2026-09-25/i-want-you-to-create-an/native-source/soh/soh/Enhancements/randomizer/age_change_areas.h:11).)

That difference can affect where the generator places progression. It also means the recent Water/Forest/Training Ground changes are a subset of the requested global age-change behavior. The correct solution is to port usable room transitions and target predicates together. An eye that accepts a child Slingshot can support a switch-age route; a Poe painting or another Bow-only actor cannot be loosened merely because the player can change age.

There is a separate pedestal difference. Website Closed Door of Time uses the song event and normally makes Master Sword ownership a pedestal-travel requirement. Native Closed requires three stones and Ocarina of Time too; native Song Only is the nearer door analogue, while its pedestal age-propagation function has no explicit Master Sword ownership test. These contracts should be compared explicitly rather than conflated with the Song of Time enhancement. ([Website Temple of Time](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/world/oot/overworld/temple_of_time.yml#L8); [native door condition](C:/Users/swunderly/Documents/Codex/2026-09-25/i-want-you-to-create-an/native-source/soh/soh/Enhancements/randomizer/location_access/overworld/temple_of_time.cpp:41). Findings L02-L04.)

### Resource availability differs from item ownership

The website checks renewable arrows, seeds, bombs, magic and rupees where its macros require them. The reviewed native rules usually treat owned Bow, Slingshot, Bomb Bag or magic capacity as sufficient; sticks, nuts and Bombchus retain some separate source checks. Native MM purchase logic primarily checks wallet tier, without the website RUPEES-source event. This is a model difference affecting many settings at once, especially shuffled drops/shops, item scarcity and restricted entrances. It is not proof that an ordinary native seed becomes impossible: the Ship runtime's available refill sources must also be evaluated. ([Website resource macros](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/macros/macros_oot.yml#L78); [native item-use predicates](C:/Users/swunderly/Documents/Codex/2026-09-25/i-want-you-to-create-an/native-source/soh/soh/Enhancements/randomizer/logic.cpp:585). L06.)

### Cross-song playback is not the same as foreign/shared item pools

The useful common contract is the one requested: **Requiem knowledge plus the instrument and playable buttons in the game where it is being played**. Native foreign-song reachability exchanges learned warp-song knowledge, retains local instruments/buttons, and requires an activated source owl for Soaring routes. The website independently controls foreign song copies and shared item families. Our single cross-song switch does not implement all those item-pool choices. Mixed warp destinations are additionally a native 16-destination permutation; the website offers separate own-game/full and one-way pool policies. ([Reference song requirement](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/data/macros/macros_oot.yml#L52); [native foreign-song rules](C:/Users/swunderly/Documents/Codex/2026-09-25/i-want-you-to-create-an/native-source/combo/rando/ForeignSongs.h:1); [reference warp options](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/packages/core/src/settings/data.ts#L3557). L07-L09.)

Ordinary entrance shuffle is another system. Native generation shuffles OoT entrances and exports no corresponding MM ordinary-entrance layout. Neither the Mask Shop portal nor mixed songs supplies the website's MM/full mixed dungeon/interior/region/one-way entrance randomization. L10 is a capability gap, not an already implemented option with a small predicate bug.

### Pool, confinement and shared tiers must agree before comparing routes

Related settings often describe different transformations:

- The website's song policy is combined and has Notes mode; native OoT policy is separate and MM songs are currently always shuffled.
- Native MM fairies have one dungeon placement mode, while the website separates chest, other and Town Fairy policies. Native Town Fairy is always shuffled.
- Freestanding hearts and rupees are independently selectable on the website but grouped in native controls.
- Website Removed Keys changes doors; native Start With Keys grants inventory. Those must not be treated as synonyms.
- Native shared support has **17** families versus **69** reference shared selectors. Bombchu Bag sharing is one-way; MM shared Hookshot has one native tier; shared-wallet mode forces the OoT child-wallet option off.
- OoT native Swim really is a Bronze Scale progression tier. The MM Swim ability is independently modeled and does not establish the website's optional MM-scale progression.

The default song/pool settings can therefore produce different item/check sets even when two menu labels appear similar. Findings L11-L14 and L16 describe these transformations and their sources. The exhaustive tables retain the exact declared enum expressions instead of pretending every value has a matching checkbox.

## Other consequential comparisons

| Family | Result and practical distinction | Findings |
| --- | --- | --- |
| World/dungeon openings | OoT native Jabu/forest/fountain policies and global MM dungeon access modes are narrower/different from website per-dungeon sets. Skip Child Stealth is not the complete website Skip Zelda sequence. | Catalogue `main.events` |
| Clocks and cycle behavior | Both have owned half-day concepts. Native sequential time expansion, starting roots and cutoffs differ from website stay/time/reset predicates; no equivalence claim for every timed route. Runtime cycle-retention and MoonCrashSave are separate. | L17 |
| Goals and gate counters | Native combined both-bosses/Triforce Hunt covers a subset of website goals. Website Triforce defaults are 20 required/30 available; native combined defaults are 15/15. Local thresholds are not general combined custom counters. | L18 |
| Moon trials and FD reward | Website needs four trial completion events; native reward needs 20 masks and its actor hook intentionally does the same. Copying only website reward logic would over-restrict the simplified native quest. | L19 |
| Scarecrow and shuffled buttons | Native OoT free Scarecrow still checks two buttons. Native MM traversal macro omits the learned event. Practical MM false-positive needs a gameplay fixture before being called a bug. | L20 |
| Prices/wallets | Website Affordable means 10, while native weighted/range/fixed modes are different. MM shuffled shop/Tingle prices are 0..<200; no matching independent price policy. Native infinite upgrades differ from bottomless 9999 wallets. | L14, L26 |
| Price estimate error | OoT unknown Range-price tracker estimate reads endpoint one twice. Confirmed source error; no generation-price failure demonstrated. | L27 |
| Damage and tunics | Website explicitly does not adjust logic for damage multiplier; native EffectiveHealth/TakeDamage do. Heat/water timers are separate heart/tunic predicates, not evidence of multiplier-based timer logic. | L28 |
| Foreign/ageless items and souls | Native subset lacks many website locally usable foreign items, ageless settings and NPC/animal/misc soul predicates. Runtime surface/item-use cheats do not automatically become native seed rules. | L25 |
| Saria song in MM | Native consumable priority-item hint is a different item purpose from the website reusable foreign Saria song/song-event extension. | L15 |
| Goron lenient spikes | Website aerial charge/retention/no-minimum-speed differs from native accelerated charge. | L29 |
| Tricks/restorations | Both catalogue and prerequisites differ. Only 34 aliases have a reviewed pairing. Shadow Door Unload and Early Eyeball Frog require explicit native trick permission, independently from the restoration. | L21 |
| Hints/appearance/Seen | These are knowledge/output systems. A reachable check is not evidence the player has visually seen its reward; no general website hint or chest-appearance parity is established. | L31 |
| Plando/exclusions | Native finished-seed editing is not website generation-time constrained placement. Excluded checks still depend on the native guarantee/model. | L30 |
| Multiplayer | Anchor same-seed co-op is not distinct-world, per-player website multiworld generation. | L32 |

## Intentional choices and native additions

These should not be labeled bugs merely because the website differs:

1. **Adult and child cross songs use one native setting**, as requested. The legacy adult-Soaring field is retained for old imports; it is not a request to restore a second UI toggle.
2. **Bremen required-chick count is runtime-only difficulty.** It changes quest length, not the Bremen Mask/Cucco Shack inventory requirement. The website has no corresponding stable count slider.
3. **Fierce Deity gap jumps remain excluded**, as approved. Importantly, v32.3 itself says its FD-anywhere setting has **no logical applications**. It does not provide the gap-jumping logic originally assumed to exist. Native optional sword/shallow-water/Big-Octo beam benefits are a native logical extension. ([Stable FD description](https://github.com/OoTMM/OoTMM/blob/413573d22d1d53e8a81931a70927f625b73488b9/packages/core/src/settings/data.ts#L1597); [native ability predicates](C:/Users/swunderly/Documents/Codex/2026-09-25/i-want-you-to-create-an/native-source/combo/rando/MmAbilities.h:1).)
4. **Adult descent, child entry to the Well** is included through the native age-change rule. The website labels this a separate technique. That is an explicit policy difference.
5. Native **egg/frog count, Gibdo trade, dog-race, keg/rolling and specific Deku-hop rules** do not all have matching stable website controls. Common prerequisites should be reused, but the custom native runtime still determines which graph edges are valid. Egg logic retains distinct accessible eggs, bottle, Zora form and the local lesson instrument; Frogs retain returned events, Don Gero and spring.
6. Native shuffles of abilities such as **Climb, Crawl, Grab, Speak/Open Chest**, Fishing Pole, Roc's Feather, and infinite upgrades add their own prerequisite/pool families. The reverse catalogue lists them. They cannot be made equivalent merely by copying reference predicates that assume those abilities are available by default.

The shared tables also reserve the native Mask Shop location for the cross-game portal. That removes/replaces some native Mask Shop reward/hint behavior, while the website's linked-world entrance scheme is independently defined.

## Runtime settings, seed assumptions and tracker planning

The native versioned rule table distinguishes **runtime values** from **generation assumptions**. Enabling a paired rule in Randomizer enables the ability too; an enhancement-only change can leave generation conservative. Trick-governed restorations and runtime-only counts are separate. This preserves the user-requested behavior; blanket equality of those two values would undo it.

The remaining consistency question is the tracker: the recent OoT age-change path reads the active runtime ability when planning available checks, while several other helpers still read the saved logical assumption. That mixed read policy is visible in source, but this audit has not reproduced a failure for every other enhancement. It should be tested rule by rule, rather than reported as either universally fixed or universally broken. L33 records the boundary and suggested matrix.

## Recommended implementation order after this comparison

1. **Age-change contract and room coverage**, with matching generator and tracker route predicates; include Spirit/mixed-age targets and the pedestal policy explicitly.
2. **Resource and affordability policy**, then pool/confinement and shared-tier translation. These affect many settings at once and should precede blanket parity claims.
3. **Scarecrow/button and price-estimate regressions**, with concrete native gameplay/oracle fixtures. L27 is a confirmed code defect; L20 needs runtime confirmation.
4. **Time/reset, gate and goal translation**, including restricted clocks and custom counters.
5. **Technique mapping and supported extension coverage**, followed by richer entrance, plando, hint and multiworld features only if those capabilities are desired.

The appropriate oracle is a common inventory/settings/entrance fixture translated into each engine, comparing reachable checks **and their reasons**, then testing combinations (age change + shuffled entrances + buttons + scarce resources, for example). Exact random seed numbers need not produce identical placements because the engines use different generation algorithms. Unsupported settings should be identified explicitly rather than silently translated to a loosely related option.

## Verification boundary

- Completeness checks verify unique registry keys, all stable/native registrations, all active technique entries and all combined rules.
- Evidence links are anchored to actual source text and recorded hashes. The reference repositories are pinned; the native dirty snapshot is identified by hashes.
- This audit performs source/inventory verification, **not a new in-game test run**, a complete playthrough or an exhaustive differential reachability test. Prior 0.2.9 native/Android tests validate particular native routes only; they are not reused as proof of website parity.
- No app rebuild, production patch or ROM/save upload is part of this audit.

Verification details are recorded in `logic-comparison-2026-10-04/verification.json` after checking the generated artifacts.
