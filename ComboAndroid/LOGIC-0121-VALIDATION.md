# Combined logic validation, 0.1.21 work

This is source and automated-test evidence. It is not a full physical-device playthrough or a claim of complete OoTMM feature parity.

## Accessibility and goals

- Every check reachable validates the collected-inventory fixed point across **both games**, including junk and confined checks. There is no longer an MM-junk exemption.
- Only completion guaranteed tests the selected combined victory condition and permits optional unreachable checks symmetrically in either game.
- Majora is represented by an explicit defeat marker, requiring a viable first-phase and finishing weapon, plus the boss soul when shuffled. Entering the lair or reaching one of its pots does not prove victory.
- Winning is latched at its first sphere. Traversal continues to the collected fixed point; unreachable placed items are never assumed owned.
- An MM oracle reset must start in the human form. Zeroing the save alone selects Fierce Deity (enum value zero), which previously granted an unowned sword in logic.

## Progression capabilities

Logic reads the generator's rule keys, separately from runtime convenience overrides. Native audited additions include magic-free Goron spike rolling, Blast Mask powder-keg behavior, and skipping the Windfish performance with the native prerequisite masks still owned. The same rule registry also covers the audited Gibdo trade modes, frog-choir count, dog-race behavior, surface-hop returns, ordinary day-grave access, blue-fire/sunlight arrows, shop hours, hazard tunics, and selected burning-stick targets. Each keeps its documented equipment and route prerequisites; settings are not treated as generic access bypasses. Song age travel propagates only the actual reachable day/night states and enables the corresponding savewarp root.

Age propagation currently covers safe overworld regions and Bottom of the Well. It deliberately does not presume an arbitrary age switch preserves access in every dungeon room. Engine relocation and unsafe geometry need separate verification before expanding those paths.

Fierce Deity outside-boss sword checks require the mask and the logical anywhere rule. Native measured movement support includes the shallow-water routes already usable as Goron: the FD water-depth threshold is 84 units versus Goron's 56 (`mm/src/overlays/actors/ovl_player_actor/z_player.c`, `sPlayerAgeProperties`). This does not grant swimming, diving, Deku hopping or flight. With magic, native FDAnywhere sword-beam hooks explicitly allow Big Octo damage, so its swamp kill event recognizes that capability.

The official pinned OoTMM source classifies FDAnywhere as nonlogical. `data/macros/macros_mm.yml` explicitly leaves adding FD to `is_tall` as a TODO. No specific gap shortcut is imported from that TODO or inferred solely from player size.

## Saved MM techniques

The initial native MM catalogue contains two actual techniques, both default off:

- `gCombo.Tricks.MM.Lensless`: remembered invisible platforms and invisible chests. The Lens still gates following/healing Darmani and seeing Shiro.
- `gCombo.Tricks.MM.DarmaniWall`: climbing the hidden Mountain Village wall to the graveyard. The reward still needs its actual Lens and song prerequisites.

These correspond to official `MM_LENS` and `MM_DARMANI_WALL`. Native `ObjVisiblock_Init` loads platform collision unconditionally; `EnBox_WaitOpen` interacts with invisible chests without a Lens test. Lens visibility remains a rendering concern. These are logic assumptions, not toggles that alter the player's collision or reveal items.

## Boss/event path hints

Counterfactual replays score eight OoT boss reward checks, Ganon, the four MM boss warp checks, the actual Majora defeat marker, and the Zelda escape reward event. Each target must first be reachable in the baseline collected-inventory traversal. Removing a placement must actually remove target access before it is considered on that path.

The seed's selected victory condition remains the WotH requiredness predicate. An item may be necessary for an optional boss without being necessary for a Triforce-hunt goal. Shared item mirrors are applied in these replays, as in validation.

Both games receive area-only path hints and common `pathTarget` / `pathArea` metadata. The unified hint tracker uses the same revealed path labels in either game. Unread hints retain generic labels, and path hints neither name the hidden reward nor set any check's Seen state.

## Evidence and patch order

The logic components have this relative order. The complete port replay order,
including the other 0.1.21 components, is recorded in `tools/apply_port.py`:

1. `combined-logic-guarantees.patch`
2. `boss-event-path-hints.patch`
3. `mm-verified-tricks.patch`
4. `fd-native-traversal.patch`

Independent replay: `tooling/verify-logic-patch-stack-0121.py`; log `tooling/logic-patch-stack-0121.log`. Pure-helper/fake-oracle suite: 16/16 tests passed in `tooling/native-pool-error-0121-tests.log`, including strict MM junk/confined checks, combined goals, post-win collection, circular locks, optional cross-game boss paths, deterministic hint output without reward leakage, Lensless prerequisites, and FD mask-plus-logical-rule ownership.

Reference checkout: `references/ootmm-research`, revision `a284b8aab4c707ceb35278fbc66a2271002a20f0`, especially `packages/core/src/settings/tricks.ts`, `data/macros/macros_mm.yml`, `data/world/mm/dungeons/moon.yml`, and the matching Lens checks in MM region files. Native boss damage tables in `ovl_Boss_07/z_boss_07.c` were checked against Majora's phase requirements.

The native evidence below uses the actual rebuilt Android engine libraries; it is separate from fixture-based tests.

## Native oracle integration diagnostics

The final Android ARM64 engines run under the emulator's native bridge through an isolated headless executable. Its only platform adaptation supplies a private storage path for the SDL Android storage getters, which otherwise require a Java activity. An absolute library RUNPATH permits the emulator's ARM64 native bridge to resolve dependencies; the wrapper flushes output and exits before the standalone process's SDL-context teardown. The executable, its storage shim, one-attempt test fault, and longer missing-check diagnostic are private test artifacts; the APK does not use that shim. No ROM data are used or uploaded by this generator harness.

An initial strict seed run exposed 64 vanilla OoT shop slots and three MM checks absent from the collected-inventory traversal. The MM checks were Mountain Village Frog Choir, Swamp Spider House Mask Of Truth, and Ocean Spider House Wallet. Native GeneratePools omits unshuffled frogs and fully unshuffled tokens from the free pool. The dump had already emitted vanilla boss remains as fixed placements, but omitted those two counter-item categories. They must be emitted as non-hintable fixed placements and credited only when each original check is reachable; granting their totals at reset would incorrectly skip their access requirements.

The counter regression verifies that reachable vanilla fixed frogs/tokens unlock a reward, while blocking one token makes strict coverage fail. The combined CTest suite currently passes 16/16 (`tooling/native-pool-error-0121-tests.log`). The initial native failure and its full missing MM names are recorded in `tooling/headless-strict-mm-names-0121.log`. Two consecutive strict seeds subsequently passed on the rebuilt native oracles (`tooling/headless-native-strict-final-0121.log`), and saved default replay passed with 795 collected placements (`tooling/headless-native-replay-0121.log`). These early successes preceded the additional owl and confinement fixes below; final-source runs are recorded separately.


## Native preparation and retry integrity

Two further omitted vanilla progression categories were found through actual native generation. OoT's unshuffled Master Sword is excluded from its free location pool; it now appears as a fixed placement and is credited only when the original adult pedestal check is reachable. MM's unshuffled owl statues are also excluded from its free pool. They now appear as fixed, non-hintable placements so each real reachable activation unlocks the corresponding Soaring source. This matters when a mixed destination map sends an MM owl source to an OoT landing.

The forced-retry test also found a native confined-fill failure losing 71 progression items (dungeon keys/maps/compasses, songs, and rewards). Returning that damaged dump allowed generic pool padding to replace the missing items with junk. Each confined preparation now resets its own failure/check-search state; a failed placement returns an explicit retryable native error. Both launcher and headless read that error before counting hunt pieces or accepting a pool. They do not silently fall back to a different location universe.

Canonical saved MM techniques are restored after native MM settings. A contradictory older native settings copy can therefore no longer overwrite the combined snapshot's technique assumptions. Replay also restores these rules after its diagnostic logic-mode re-preparation.

Earlier real native variants include Only completion guaranteed with an 8-of-20 Triforce hunt (`tooling/headless-native-completion-hunt-0121.log`, saved replay `tooling/headless-native-completion-replay-0121.log`) and a No Logic generation (`tooling/headless-native-no-logic-0121.log`). No Logic is intentionally a weaker contract; generating it is not evidence of a glitchless OoT completion guarantee.


## Final-source native receipts

The final rebuilt ARM64 libraries passed the MM-start profile with logical mixed song destinations, foreign songs, Song-of-Time age mode 3, logical FDAnywhere, and both MM techniques. The first master (`1675750772`) was rejected as an explicit retryable native confined-fill error. The next (`35219245`) passed strict coverage with zero missing checks in either game and no pool trimming or padding. Its final universe contains 443 OoT and 362 MM placements, including all ten vanilla owl activations. See `tooling/headless-native-mm-rules-final-known-0121.log`.

The saved result was then replayed on the original assumptions: zero enabled OoT tricks and two enabled MM tricks, starting in MM. Both canonical rule keys and the native MM settings export contain those two enabled techniques, despite the test input deliberately carrying disabled keys in its older native settings copy. Replay reached both selected boss goals at winning sphere 23 and collected all 805 placements at the fixed point (`tooling/headless-native-mm-rules-replay-final-0121.log`). This verifies the saved assumption boundary, rather than treating an all-tricks diagnostic as the player's result.

The harness is run sequentially in private directories. The original emulator became unresponsive during a first attempt to run two native processes simultaneously; those interrupted runs produced no verdict. Those interrupted runs were discarded; recovery uses an isolated 8 GB emulator and one native harness process at a time. The interruption is recorded as a test-environment failure, not a successful validation receipt or proof of a game bug; its precise resource failure was not diagnosed.


Two consecutive default strict seeds passed on the final sources (`313221031` and `313221032`), each with zero missing OoT/MM checks and no pool padding or trimming (`tooling/headless-native-strict-final-current-0121.log`).

A private one-attempt fault then hid MM's actual reachability response for master `313221032`. The resulting combined fill failed after its real inner passes. The fault was removed before master `2967656801`; the original native MM oracle then passed strict coverage with zero missing checks and no padding/trimming (`tooling/headless-native-forced-retry-final-0121.log`). The two accepted preparation dumps retain the entire original check universe and configured counts: OoT has 173 free and 269 fixed checks before the forced pocket placement; MM has 284 free and 78 fixed checks. MM's advancement multiset is exactly equal. OoT's category-normalized advancement multiset is equal, allowing its normal seeded bottle contents and the particular dungeon reward reserved for the forced pocket to change (`tooling/native-retry-integrity-final-0121.json`).

The final Only completion guaranteed test generated 8-of-20 Triforce Hunt master `491482061`. All 20 pieces appear in its placements; the native added-piece pool trims eight MM junk items and no progression items. Saved replay achieved the selected goal at winning sphere 4 and ultimately collected 805 placements (`tooling/headless-native-completion-final-0121.log`, `tooling/headless-native-completion-replay-final-0121.log`). The optional-check relaxation is separately covered by the production-helper fixture where strict coverage fails but the chosen goal is reachable; this native seed happened to leave zero optional checks unreachable.

A final No Logic hunt generated master `527484916`, retained the saved native No Logic enum and canonical MM Lensless technique, and captured that same enabled technique in native MM settings despite the older native input containing zero (`tooling/headless-native-no-logic-final-0121.log`). This is a weaker No Logic contract and is not labeled a glitchless OoT completion guarantee.

The local sanitized summary `tooling/native-verdicts-0121.json` contains verdicts, counts and relative log references only, without private placements or ROM contents. Its seven verdicts are also summarized in the delivered [versioned validation report](VALIDATION-0.1.21.md); the local harness logs are not included in the source archive. Winning sphere and total fixed-point item count are reported separately: the existing console text incorrectly uses winning sphere as its displayed sphere total. That diagnostic wording was left unchanged after the final source freeze; it does not stop traversal at the winning sphere.
