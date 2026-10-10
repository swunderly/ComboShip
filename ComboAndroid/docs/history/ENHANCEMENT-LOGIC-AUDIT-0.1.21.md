# Enhancement and logic audit

The supported progression rules are explicit entries in `combo/rando/GameplayRules.h`. Selecting a rule in Randomizer enables its actual runtime ability. Selecting an enhancement alone may leave the randomizer assumption off. The original generated rules and later per-run choices are stored separately. OoT's complete enabled-trick CSV and the two verified MM techniques are included as well.

## Additional MM rules

`mm-enhancement-logic.patch` follows `fd-native-traversal.patch` and adds these engine-backed predicates:

| Setting | Runtime default | Logic effect |
| --- | --- | --- |
| `gEnhancements.DifficultyOptions.GibdoTradeSequence` | 0, Vanilla | Mode 1 accepts a bottled red or blue potion at the first well door. Mode 2 bypasses all requested resources at Gibdo doors. All modes still require the Gibdo Mask; flame and sunlight requirements remain. |
| `gEnhancements.DifficultyOptions.FrogChoirCount` | 5 | The yellow frog counts as one, plus the four returned-frog flags. Logic uses the selected count while retaining Don Gero and spring. Logical zero means vanilla five. |
| `gEnhancements.Minigames.AlwaysWinDoggyRace` | 0, Off | Mode 2 supplies a winning dog without the Mask of Truth. Mode 1 keeps the mask requirement. Ranch access and opening times remain. |
| `gEnhancements.Player.InfiniteDekuHopping` | 0, Off | An owned Deku Mask and enabled logic allow the three existing downward/surface returns from Great Bay's lab exterior, cliffside and pirate ledge to the low beach. This does not unlock platform ascent, swimming, diving, Mikau, Pinnacle Rock or the fortress. |

The Gibdo audit used the actual `EnTalkGibud_PassiveIdle`, mask check and `GibdoTradeSequenceOptions.cpp` hooks. The frog audit used `SavedFrogs()` and the native `FrogChoirCount` hook. The dog audit used `AlwaysWinDoggyRace`'s forced-speed and mask tests. The hopping audit used the native hop-budget reset and player water-hop action; raised-platform ascent was excluded because the Deku ledge-climb handler has separate limits.

The OoTMM reference's Beneath-the-Well resource and final-lighting paths are consistent with the native vanilla model. Its open-well setting is broader than this port's No Trade enhancement: native No Trade still needs the Gibdo Mask, so that requirement deliberately remains. OoTMM has no directly equivalent unlimited-hop port option; its marine-platform swimming rule was not copied into a hypothetical Deku ascent.

## Existing paired OoT options

Blue Fire Arrows and Sunlight Arrows already have native `gRandoSettings.*` counterparts and matching context options. The corresponding enhancement and randomizer controls must use those native logical keys, not an additional shadow key. Ice Arrows melt red ice; Light Arrows activate sun switches. They still require the bow, magic and usable arrows for the current age. Runtime-only activation can stay outside seed logic.

## Access prerequisites unchanged by ordinary QoL

- Pause-map soaring still requires the Song of Soaring, instrument, playable buttons, activated owls and a permitted scene.
- Skip Scarecrow Song still requires the instrument and at least two playable buttons; the logic already uses those requirements.
- Fast Marine Lab Fish reduces repeated feeding; the same bottle and repeatable fish source are needed.
- Extra Powder Kegs changes carrying capacity rather than the form, keg and barrier prerequisites.
- Lower Bank Thresholds, reduced minigame scores, shorter races and faster digging change repeated quantities or execution difficulty. They do not add a new item or entrance alternative to the current access model.
- The Oceanside Spider House squatter fix is already forced on in randomizer mode.

## Deliberate coverage limits

General movement cheats (manual jumping, arbitrary speed multipliers, unlimited flower glide, Climb Anywhere, Hookshot Anywhere and Moon Jump), unrestricted item use and restored Get Item Manipulation are not asserted as equivalent to OoTMM's route catalogue. Their actual shortcut coverage depends on native collision, item restrictions or glitch execution. They may remain convenience-only; a seed should not require an unaudited route.

OoT's Bunny Hood enhancement grants real speed and optional jump multiplication but explicitly has no upstream randomizer route model. Early Eyeball Frog can alter which King Zora reward is obtained using shield collection delay; its interaction with shuffled trade inventory needs a dedicated route test before adding a logical bypass. These are coverage limits, not tested progression features.

## Verification

`MmEnhancementLogicTests.cpp` covers every Gibdo mode, missing masks and bottles, red-versus-blue substitution, invalid values, every combination of returned frogs and all valid frog thresholds, the dog-race mask modes, and independent surface-hopping prerequisites. The standalone MSVC run passed with assertions enabled and `/W4 /WX`. Native engine compilation, emulator integration and physical-device gameplay are separate validation stages.
