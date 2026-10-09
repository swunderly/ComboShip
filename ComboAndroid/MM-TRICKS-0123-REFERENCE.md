# MM technique assumptions

The ten MM catalogue rows are optional, default off, and selected in **Randomizer → Tricks & glitches**. The two previously shipped Lens techniques retain IDs 0 and 1. IDs 2–9 add the eight routes below. Selecting a technique permits the generator to expect the player to perform it; these options do not automate a jump, win a minigame, or grant equipment.

The reference is the locally pinned [OoTMM source](https://github.com/OoTMM/OoTMM), specifically `data/world/mm`, `data/macros/macros_mm.yml`, and `packages/core/src/settings/tricks.ts`. The linked demonstrations are those cited by that catalogue. Native prerequisites and boundaries below deliberately exclude routes needing foreign equipment or unverified movement capabilities.

| Catalogue row | Reference technique and demonstration | Retained native requirements and boundary |
|---|---|---|
| Pinnacle Rock without the Seahorse | `MM_NO_SEAHORSE`; [demonstration](https://www.youtube.com/watch?v=Ye7lu1kSxpQ) | Zora form and swimming ability. Only entrance-to-inner navigation loses the guide requirement. Reuniting the seahorses still requires the actual Seahorse and killing the Deep Pythons. |
| Post Office timing without Bunny Hood | `MM_POST_OFFICE_GAME`; [demonstration](https://www.youtube.com/watch?v=bAlGLPkp8go) | Day-one afternoon/evening, or day-two evening after mailing the Letter to Kafei. The native ten-second timing challenge still has to be won; the Bunny Hood normally supplies its visible timer. |
| Guess Oceanside Spider House code | `MM_CAPTAIN_SKIP`; no demonstration linked by the source catalogue | Bow and normal access to the colored-skull room. Only learning the sequence from Stalchildren loses its Captain's Hat requirement. Arrow switches still require the correct sequence. |
| South Clock Town lower chest with a precise jump | `MM_SCT_NOTHING`; [demonstration](https://www.youtube.com/watch?v=8_tjO86vRsY) | South Clock Town access and the human roof jump. Only the lower chest changes; the upper carnival chest retains its normal route and final-day condition. |
| Doggy Racetrack chest with a fence jump | `MM_DOG_RACE_CHEST_NOTHING`; [demonstration](https://www.youtube.com/watch?v=DlpBZq579Ds) | Racetrack access, its usual hours, and the human fence jump. The dog-race prize and its separate requirements do not change. |
| Skip planting beans in Deku Palace | `MM_PALACE_BEAN_SKIP`; [demonstration](https://www.youtube.com/watch?v=sL30HLF9tAQ) | Normal lower-palace access. The west garden doorway backflip and angled roll jump remove beans/watering only for climbing to the upper cell-side ledge. Deku flight and Mad Scrub combat still gate travel across the upper palace. |
| Climb Ikana Canyon without Ice Arrows | `MM_ICELESS_IKANA`; [demonstration](https://www.youtube.com/watch?v=VtMx8XIE0tA) | Lower-canyon access and Hookshot. The precise riverside aim replaces the ice platform only; it does not supply the Hookshot or open lower-canyon entry. |
| Climb Stone Tower with one transformation mask | `MM_ONE_MASK_STONE_TOWER`; [demonstration](https://www.youtube.com/watch?v=I3xLE1X9Iiw) | Hookshot, playable Elegy including instrument/buttons, and Goron **or** Zora. Deku alone never suffices. This implementation permits climbing to the tower's top/owl routes; the temple bridge retains the port's ordinary two-mask condition pending a separate verified route. |

Native-source checks:

- `En_Mm3` checks the stopped timer for its reward. The Bunny Hood controls help/visibility rather than supplying an otherwise unavailable minigame.
- `Obj_Switch` accepts the arrow hits; `En_Hint_Skb` supplies the Captain's Hat dialogue clues. Guessing does not bypass the Bow or puzzle state.
- `En_Box` snaps ordinary chests to their collision floor on update. Scene spawn height is not the actual platform height used for jump validation.
- `Bg_F40_Block` follows its native scene path with dynamic collision. `Bg_F40_Switch` reads real pressure, retains a short release delay and holds switches during a cutscene. Human, Goron and Zora can press switches; Deku cannot. No block physics or switch behavior is modified by the technique.
- Jump/Hookshot route collision and actors were inspected using the user's private local ROM. Derived geometry and test saves are excluded from public artifacts. Collision compatibility is distinct from a successful native-input maneuver; release validation must record those live observations separately.

Regression coverage compiles the eight production `Logic.h` macro definitions, varies missing items/forms/abilities/time conditions, checks actual region bindings and protected neighboring gates, and exercises all ten rows through the real seed/run snapshot migration helpers. Original seed settings and placements remain immutable when a saved run changes its techniques.
