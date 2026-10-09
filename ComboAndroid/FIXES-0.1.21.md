# Combo Ship 0.1.21 — shared rules, editors and native song playback

This patch continues making Combo Ship behave as one application. The menus now
share progression controls, a techniques catalogue, numeric controls and HUD
editing. Cross-game songs use the games' successful song-performance sequences.

## Abilities and seed logic

The same **Gameplay rules** page appears under **Randomizer** and **Enhancements**.
Selecting an option in Randomizer enables its actual ability and its supported
logic. Enhancements can enable the ability for your own use without adding a
randomizer assumption; **Include in logic** makes that choice explicit. Changing
an enhancement to a different mode clears an incompatible logic selection.

Eighteen audited rules are included:

- OoT: Blue Fire Arrows, Sunlight Arrows, shops open at any hour, pulling ordinary
  graves during the day, protective tunics, Deku stick behavior and Song of Time
  age change.
- MM: Gibdo trade modes, frog choir count, dog race results, unlimited Deku water
  hops, Fierce Deity anywhere, Bremen's one-chick quest, Goron rolling without
  magic, Blast Mask powder-keg explosions and a shorter Ballad of the Wind Fish.
- Combined: playing warp songs across games and using the seed's mixed warp-song
  and owl destinations.

The descriptions explain actual prerequisites and coverage. Fierce Deity counts
its sword, measured shallow-water traversal and magic beams against Big Octos;
it does not assume every larger jump or shortcut. Infinite hopping models the
verified surface returns to Great Bay's low beach, not platform ascent or diving.
General movement cheats and unverified shortcuts remain available for your own
use, and generation does not require those routes.

Randomizer creates a shuffled song-destination map when that shuffle is included
in logic. The enhancement reuses the map saved with the seed. Turning it on for
a seed generated without shuffling keeps ordinary destinations; turning it on
during a run never invents a new permutation outside the saved seed's logic.

Original generated rules, both games' native settings and enabled techniques are
kept separately from intentional run overrides. Loading another slot restores
that slot's choices, including older seeds. Mid-run changes remain allowed and
show a small warning that they could affect completion; there is no validity
check, blocking dialog or automatic regeneration.

## Shared generation guarantee and techniques

Combined seed options offer **Every check reachable** and **Only completion
guaranteed** for both games. Every-check validation includes junk, confined and
fixed rewards. Completion validation uses the selected combined victory
condition and actual Majora defeat rather than merely entering the lair. The
native OoT confined-fill and entrance policy follows the combined choice.
Selecting **No Logic** explicitly removes both guarantees.

**Randomizer → Tricks & glitches** is one searchable catalogue with game,
difficulty and enabled-state filters. It uses 197 native OoT techniques
and two verified MM techniques: fewer Lens requirements and climbing
the hidden wall to Darmani without the Lens. The descriptions retain the Lens
requirements for following/healing Darmani and seeing Shiro. Technique choices
persist with the seed and with run overrides.

The generator still retries failed layouts, supports Cancel and offers its
five-minute reminder without weakening the selected guarantee. A failed native
confined placement now rejects that attempt before missing progression items can
be replaced with padded junk. Retryable placement failures request another
attempt; malformed native results or fatal configuration errors stop with an
explanation.

## Song presentation, hints and tracker knowledge

- OoT Soaring recognizes a learned, locally playable song even with no activated
  MM owls. It performs the song, then explains in a native textbox that there
  are no destinations.
- MM foreign warp songs use the successful performance sequence and the
  corresponding OoT melody before the native confirmation. The erroneous
  invalid-song/bonk path is removed from recognized foreign songs.
- Instrument, playable buttons, song ownership, owl unlocks and supported
  age/form restrictions still apply. Holding the final note cannot accept the
  later confirmation. Cancellation retains the current game and entrance.
- Boss/event path hints are derived from the combined seed's actual inventory
  traversal. A path hint names an area and target, not the hidden reward.
- The shared tracker distinguishes **Hinted**, **Seen**, **Known** and
  **Collected**. Dialogue revealing a reward is Hinted; an actual visual
  observation is Seen. Ambiguous legacy records are Known. Entering an area or
  reading a path hint does not reveal unseen items.
- Global settings search finds the shared song controls using terms such as
  `warp`, `song` and `soaring`, rather than exposing retired per-game copies.

Existing reward knowledge is preserved. The corrected observation and hint
hooks govern new reveals; an older record explicitly saved as Seen is not
erased or reassigned without evidence of its original source.

## Screen, HUD and numeric editing

**Settings → Screen & HUD** provides the same HUD editor for both games. Choose
an element, set its anchor/visibility, drag the placement guide or use horizontal
and vertical sliders, and reset one element or the layout. It maps to the
existing native placement settings and keeps manual screen fitting available.
Touch-controller button layout remains separate and is edited by holding Menu.

Both save editors and normal numeric settings use bounded sliders without
opening a numeric keyboard. This includes inventory amounts, keys, rupees,
health/magic, timers, scores and resolution dimensions. Precise network ports
and advanced raw debugger IDs, coordinates and physics fields retain text entry.

## Validation status

Production-model tests passed for rule and trick persistence, migration and slot
isolation, malformed saved values, enhancement-only modes, frog minimums,
runtime/logic song-map agreement, native guarantee modes and retired controls.
Native pool-error tests cover retryable failures from either game, fatal mixed
failures and malformed results before downstream pool processing.
HUD geometry, native enhancement predicates, catalogue filters and combined
oracle model tests have separate regression coverage. The full port patch stack
replays from the pinned checkout and matches the working native source after
newline normalization.

The [versioned validation report](VALIDATION-0.1.21.md) identifies the exact APK,
16 passing host tests, seven native generation verdicts and live emulator
results, including 58 touch-overlay checks. Physical-phone testing and a
complete playthrough are not established by those tests.

## Phone checks

1. Play Soaring as Child Link in OoT with no activated MM owls. Confirm the full
   performance, melody and native no-destination message. Repeat with an owl,
   cancel, replay and confirm the expected destination.
2. Play Requiem and a second OoT warp song in MM. Check the melody, animation,
   text, B cancellation and arrival. Hold the final note through recognition.
3. Try a missing instrument or song button and check that ordinary native songs
   still work. Listen through the phone speakers for the reported bonk issue.
4. Enable a Gameplay rule in Randomizer and check its Enhancements value. Then
   use an enhancement without Include in logic, save, switch slots and reload.
5. Test both generation guarantees and cancel a generation once. With No Logic
   selected, check that the interface explains there is no guarantee.
6. Verify MM hints say Hinted, and that an unseen freestanding reward remains
   unknown until it is actually visible. A path hint should reveal no reward.
7. Move a HUD element and reset it in each game. Check phone rotation and supported
   fold changes, the tracker split view and the full HUD on your phone. Gameplay
   uses landscape orientation.
8. Adjust rupees, ammunition, timers and resolution using touch. Search for
   `warp` and confirm it leads to the combined Gameplay rules page.
