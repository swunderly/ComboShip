# Next big patch — implementation authorized

## October 8, 2026 — current limit and phone feedback

Continue the remaining authorized additions, stopping at 30% weekly usage remaining (70% used) if reached. The user's later October 8 instruction extends the earlier 50% cutoff. The user tested the uploaded 0.2.20 TEST candidate (SHA-256 `9fa47f1bac8f7102fe9ad88d5bedb2db3432f9e2a782530e712e15771230dac4`) and confirmed the three reported regressions appear fixed: MM-to-OoT handoff hitch/model reappearance, Fire Temple seated block resetting, and Silver Rupee checks shown when not randomized. Keep those resolved rather than returning them to the upcoming list. The other requested additions remain in progress; the test candidate is not their completion report.

## October 7, 2026 — next feature expansion

The user authorized finishing cross-warp native animation/audio and honoring the matching native cutscene-skip settings first. Then implement the following items from the newer missing-feature list (these numbers are independent of the historical list below):

1. Silver rupee shuffle and puzzle pouches.
2. MM dungeon key rings.
5. Full MM and cross-game entrance randomization.
6. Expanded one-way entrance pools, including Wallmaster destinations.
7. Cross-game Farore's Wind.
8. Adult Link in MM, including actual gameplay and logic support.
9. Cross-game equipment and additional shared item families.
10. Expanded enemy, NPC, animal and miscellaneous soul shuffle.
12. Additional victory conditions: either final boss, Ganon only, Majora only, and the three distinct-piece Triforce Quest.

Additional request: MM's six day/night time segments as shuffled unlock items, following OoTMM's clock-item functionality, including its time-flow and logic behavior. Verify the current native support first; an isolated draft does not count as an available setting.

Use OoTMM's existing implementation and rules as the reference, with adapters for the native engines. No completion is implied by this authorization. The user removed the earlier 50% remaining cutoff on October 7: continue until all work is complete or the available usage is exhausted. Provide the tested APK and an explicit completed/remaining list, plus focused physical-device checks where the user's testing is more useful. Preserve old seeds, saves and settings; new seed-only features must not reinterpret old records.

Cross-warp native animation/audio and matching skip behavior are completed for 0.2.18. The exact APK passed 42 host checks, clean source replay, package verification, and targeted Android travel tests in both directions. Phone sound quality and animation pacing remain user checks in [the validation report](VALIDATION-0.2.18.md).

The tested 0.2.19 release implements items 1, 2 and 12, plus seven shared families from item 9: five Ocarina buttons, Song of Time and nonprogressive Mirror Shield. All 49 host checks, the ARM64 build, 298-file production replay, 22 original-rule generation/replay fixtures, four exact-candidate new-goal full-hint cases, both full warp directions, adult fast/skipped travel and legacy MM native save/cold-reload checks pass within the scope stated in [the validation report](VALIDATION-0.2.19.md). Item 9 remains partial; items 5, 6, 7, 8 and 10 are not included. Ocarina, expanded-soul and Farore work remains in isolated incomplete drafts, with no partial options exposed. Physical-phone sound/pacing, new-item pickup/puzzle/cycle behavior and actual victory/credits remain user checks. MQ availability was fixture metadata only, with no real MQ archive/gameplay validation. This is not a claim that the entire requested list is complete.

## Historical completed work

**Current status: 0.2.3.** The accepted feature list is covered by the [0.2.0 completion audit](RELEASE-AUDIT-0.2.0.md). The subsequent ten menu-cleanup requests are implemented in the [0.2.1 request audit](RELEASE-AUDIT-0.2.1.md). The five further inventory/navigation refinements are covered by the [0.2.2 request audit](RELEASE-AUDIT-0.2.2.md): matching inventory grids, randomizer difficulty merged into Gameplay rules, Bremen count only in Enhancements Difficulty, OoT minigames in Difficulty, and Camera under Video & Audio. Those latest destinations and Bremen's runtime-only classification supersede the historical descriptions below. Physical-device and full-playthrough validation limits remain explicit.

The October 3 tracker requests are completed in [the 0.2.3 audit](RELEASE-AUDIT-0.2.3.md): pause access for fresh preferences, retained saved L toggle, explained controller modifiers, independent movable/resizable CT/IT/HT buttons, and common Item/Hint tracker controls and panels.

Recorded September 27, 2026; updated October 3, 2026. The user explicitly resumed work after their reset and requested completion and testing. Preserve incomplete items for subsequent work. Mid-run changes are allowed with a small warning that they could affect the ability to complete the run; do not require a validity check, block the change, or regenerate the seed.

1. Bremen Mask quality of life: faster marching, or a difficulty option requiring only one baby chick. Determine any logic implications before implementation.
2. Cross-game song travel: randomizer options for travelling between OoT and MM through Song of Soaring or warp songs, including optional shuffling of warp-song and soaring destinations together across both games, as in OoTMM. Treat these as progression routes that the seed may require, with the same routes and prerequisites used by generation, gameplay and tracker logic. The user's desired example is having to enter MM to use a song route to reach Spirit Temple / its Desert Colossus approach because no OoT-side route is available with the current inventory. Model actual song, instrument, destination-unlock, age/form and other applicable prerequisites, plus escape or return access where needed to avoid stranding progression. Store the chosen rules and destination mapping with the seed. This must support meaningful cross-game routing rather than only a convenience switch between games.
3. Uniform touch-friendly settings and editors: continue combining inconsistent controls. Prefer MM-style sliders for numeric fields such as rupees instead of requiring keyboard entry wherever practical.
4. Bottom of the Well access: when age transformation anywhere is enabled, verify and model Adult Link's descent into the dry shaft followed by transformation to Child Link, without Song of Storms or a drained-well flag. Require every actual prerequisite of that route.
5. General logic-affecting setting rule: audit gameplay settings that change access, requirements or progression. Expose supported options in Randomizer settings and use them during seed generation. October 2 clarification supersedes the original single-value requirement: selecting an option in Randomizer enables its actual enhancement and logic, while Enhancements may enable the ability alone without changing logic. An explicit Include in logic choice links an enhancement to generation. Song of Time age transformation and its allowed locations are a specific example. Fierce Deity Anywhere must account for the mask's usable sword and verified movement/size advantages when enabled, checking mask ownership, actual form restrictions and route-specific requirements; do not assume it replaces every movement item. Audit both enabling and disabling effects. Clearly handle changes after seed generation so the player knows when logic assumptions would change. Items 7-10 expand this rule.
6. Automatic screen fitting on first boot: choose an initial layout from the usable display size, aspect ratio and safe insets, aiming to fill the screen while preserving the information visible in the native 4:3 presentation. Keep manual screen-size and framing adjustments available. Fix the reported UI clipping: HUD, menus, text and controls must remain fully visible within the usable screen bounds. Evaluate UI fitting separately from the third-person camera view, balancing any scene cropping against added side visibility/FOV so very wide views do not create excessive apparent motion or distortion at the edges. Validate the default in both games and across screen shapes, including orientation/fold changes where supported; preserve the user's manual overrides. Make HUD repositioning easy to discover through a clearly named, shared HUD Layout / Edit HUD entry alongside the screen-fitting controls. Both games already have HUD placement controls, but access is split between OoT's Appearance / HUD Placement and MM's HUD & overlays editor. Provide consistent, touch-friendly controls for moving HUD elements, with visible feedback and a reset option; keep the distinction between game HUD elements and touch-controller buttons clear.

7. Complete rules saved with each seed (accepted research suggestion 1): save all progression-affecting settings, enabled tricks and relevant enhancement values with the seed. Generation, both game runtimes and the tracker must use that same configuration. Loading a save restores its rules; changes intended for a new seed must not silently alter an existing one. Allow mid-run changes with a small warning that they could affect the ability to complete the run; no validity check or blocking. Preserve the original generated rules separately from intentional run overrides. Validate agreement for settings enabled and disabled, including meaningful combinations.
8. Explain each setting's logic effects (accepted research suggestion 2): show which abilities or requirements a setting changes and clearly identify incomplete logic support. Under the October 2 clarification, Randomizer activation includes the supported ability and its logic. Enhancements can activate an ability alone, with an explicit Include in logic checkbox. Never silently require unimplemented or unverified routes. Distinguish seed rules from purely visual or interface preferences.
9. Consistent tricks menu for both games (accepted research suggestion 3): provide one coherent catalogue with understandable descriptions of techniques, prerequisites and the checks/routes they affect, plus optional demonstrations where available. Use OoTMM's documented techniques as research references, verifying each in the actual ports before including it in logic. Straightforward abilities follow the relevant setting; difficult or specialized techniques require explicit trick selection. Save these selections with the seed and use them in tracker reachability too.
10. Same completion guarantee across both games (accepted research suggestion 4): provide combined Every check reachable and Only completion guaranteed choices with clear meanings, enforced against the selected victory condition and seed rules. Audit and close MM reachability gaps currently exempted from part of the all-check validation; an all-check guarantee must cover both games, including checks holding junk. Do not silently fall back to weaker guarantees after a failed generation or validation.
11. More useful hints and distinct knowledge states (accepted research suggestion 6): extend the existing cross-game hint system with paths toward particular bosses and major events, informed by OoTMM's path analysis and the seed's actual rules. Distinguish Hinted / revealed by dialogue from Seen on-screen and Collected in the unified tracker. Preserve known rewards and their source without revealing unknown rewards. At the time of the report, MM exact-hint handling called RecordSeenItem; the New Wave Bossa Nova screenshot may have reflected that path, but its specific cause was not confirmed from the user's save.

September 30 prioritization: the user selected research suggestions 1, 2, 3, 4 and 6, recorded as items 7-11 above. Suggestions 5 (tracker explanations of missing requirements), 7 (shorter adventures / pre-completed dungeons) and 8 (expanded cross-game equipment) are not priorities for this patch and have not been added to its active scope. Existing requested items 1-6 remain on the list.

Implementation authorized September 30. The completion report for each delivered APK must identify finished, partial, and remaining work; authorization does not make unfinished features complete.

## First delivery: 0.1.18, September 30

The original requirements above remain authoritative. Status is scoped to this APK, with implementation and test limits in [FIXES-0.1.18.md](FIXES-0.1.18.md) and [VALIDATION.md](VALIDATION.md).

| Item | Status | Delivered and remaining |
| --- | --- | --- |
| 1. Bremen quest | Implemented using the one-chick alternative | Live emulator quest and randomized reward collection passed. Faster marching is not included. |
| 2. Cross-game song routes | Remaining | No new song transportation, destination shuffle or associated logic in this APK. |
| 3. Uniform sliders/editors | Partial | OoT health capacity, rupees and tokens, plus shared hunt counts use sliders. Remaining numeric fields and full editor consistency still need work. |
| 4. Well age-change route | Logic implemented | Shared OoT search models Adult well access followed by valid Song of Time age change. Production-code prerequisite tests passed; the exact route still needs a live gameplay check. |
| 5. Progression settings audit | Partial | Shared controls and logic support for the well route and Fierce Deity sword; one-chick option included. Full settings audit, other age routes and Fierce Deity gap/movement routes remain. |
| 6. Screen fit and HUD placement | Partial | Automatic balanced/full-width/4:3 fitting, safe insets and one Screen & HUD entry implemented. Resized emulator views and both editor entry points checked. Physical fold/cutout testing and identical touch-friendly HUD editors remain. |
| 7. Seed/run rules | Partial | Three enhancement rules now persist with seeds and run overrides; warning-only mid-run changes implemented. Live generation, save/reload and override isolation passed. Complete enhancement/trick coverage remains. |
| 8. Logic explanations | Partial | The three new shared controls explain supported effects and missing route coverage. Other settings still need the same review. |
| 9. Unified tricks | Remaining | No new catalogue in this APK. |
| 10. Combined guarantees | Remaining | No stronger all-check guarantee claimed; outstanding MM reachability gaps remain. |
| 11. Hints and knowledge | Partial | MM Seen/Hinted provenance is saved and synchronized; ambiguous legacy/OoT records use Known. Boss/event path hints and OoT source-specific observation hooks remain. |

## Second delivery: 0.1.19, September 30

Item 2 now includes cross-game song playback, optional mixed destinations, combined generation/playthrough/tracker route search and per-seed persistence. The user's specific Colossus route requires learned Requiem plus an instrument and its playable buttons in the currently played game. It does not waive Requiem ownership and does not require destination shuffling. OoT Soaring is Child-only and uses activated MM owls. See [FIXES-0.1.19.md](FIXES-0.1.19.md) and the source-backed [OoTMM comparison](OOTMM-LOGIC-AUDIT.md) for precise coverage and remaining differences.

Items 7 and 8 also extend to the new song options: options/mappings persist with the seed and the menu explains their prerequisites. The broader progression-setting audit, full combined reachability guarantee, Fierce Deity movement routes, unified tricks, boss/event hints and remaining numeric/HUD-editor consistency still remain. Physical-device and full-playthrough checks are not replaced by regression tests.

Additional request implemented in 0.1.19: interactive generation retries invalid layouts continuously, supports cooperative Cancel, and offers Keep trying / Stop and change settings after five minutes. The same validation rules remain enforced. Unrecoverable configuration or runtime errors still stop with an explanation; automated headless test runs retain their bounded attempt budget.

At the 0.1.19 delivery, the next priority was the progression-rule audit and combined reachability validation. The October 2 status below supersedes that historical priority. Keep warning-only mid-run changes and do not list completed fixes as unimplemented work.

## Settings search discovery — resumed October 2

Reported September 30 after 0.1.20 and held until the user's October 2 authorization. Shared Gameplay rules now participate in global search and lead to Randomizer → Gameplay rules. Retired per-game controls are suppressed rather than letting search bypass the shared runtime/logic behavior. Live search verification passed on the emulator candidate; its functional menu code is unchanged in the final APK.

## Cross-warp presentation bugs — resumed October 2

The user stopped implementation September 30 with 1% weekly usage remaining, then explicitly resumed October 2. The following reports define the regression cases for the new native-performance patch. Their initial investigation only read source and applied no fixes.

- **OoT Soaring with no activated owls:** the user owns and can play Soaring in MM, but playing it in OoT does nothing. No activated owls was a suspected condition, not confirmed from the user's save. At the report time, the old source gated song recognition on a nonempty destination mask. Separate learned/playable song recognition from destination availability: play the normal song-performance sequence, then show a native-styled no-destination message when no owls are available, matching MM's behavior. Preserve instrument, buttons, age and seed-rule requirements. Test zero and multiple unlocked destinations and unlearned/unplayable songs.
- **MM playback of OoT warp songs:** the destination transition works, but the normal Link song-performance sequence is missing and the user hears a bonk/error sound. Integrate foreign-song recognition with the native successful-song playback sequence before confirmation. Identify and remove the erroneous failure sound without suppressing legitimate invalid-song feedback. Verify animation, melody, timing, confirmation and cancellation together, including audible testing. Prior destination/dialogue tests did not establish this presentation/audio behavior; do not describe the feature as fully working on that basis.

## October 2 implementation status — 0.1.21

This table describes implemented coverage and its deliberate limits. Final APK, native generation and emulator results are recorded in [the versioned validation report](VALIDATION-0.1.21.md); no physical-phone or complete-playthrough claim is made here.

| Item | Implemented behavior | Validation or deliberate limit |
| --- | --- | --- |
| 1. Bremen quest | One-chick alternative retained and included in seed/run rules. | Previous live quest test exists; faster marching is not part of the chosen alternative. |
| 2. Cross-game songs | Native successful song playback, no-owl Soaring message, shared song controls and modeled saved destination maps. | Final APK native-performance, confirmation, B cancellation, MM-to-Colossus arrival and OoT no-owl-message spot checks passed. Earlier unchanged-engine cases cover ordinary native songs, held notes and six prerequisite failures. Local instrument/buttons and song ownership still apply. |
| 3. Uniform controls | Bounded numeric controls and both save editors use sliders; common HUD editor and searchable techniques catalogue. | Emulator HUD drag/reset passed in both games; OoT and MM rupee sliders changed live HUD values without opening a keyboard. Raw advanced IDs/physics and exact network ports intentionally keep text. Physical-phone testing remains necessary. |
| 4. Well age change | Safe adult descent/child transformation route retained with equipment and song requirements. | Exact live emulator route passed through normal walking, falling, Song of Time and entry into the real Well, without Song of Storms or a drained-well flag. No position edits. Additional unsafe interiors and arbitrary dungeon age shortcuts are not assumed. |
| 5. Progression options | Eighteen audited shared rules, separate runtime/logic state, retired native duplicates and explicit unsupported-route descriptions. | FD gap shortcuts and arbitrary movement/glitch routes remain unmodeled; generation does not require them. |
| 6. Screen/HUD | Automatic fitting retained; one native-CVar HUD placement editor for both games, placement guide and resets. | Final APK balanced fitting, tracker docking and menus passed at 2280×1080 and 1440×1080. Manual 4:3 settings remained unchanged across ten captures and restored through the UI. Both games' native HUD drag/reset passed on the functionally identical predecessor. Device cutouts, fold behavior and physical touch/HUD positioning still require phone checks. |
| 7. Seed/run persistence | Original settings, rules and techniques are immutable seed data; per-run overrides restore by slot with warning-only edits. Legacy migration is deterministic. | Production helper regression passed for cross-slot state, original aliases, missing technique fields and malformed values. Native OoT save and cold reload retained runtime-only and linked rules while the exact original seed hash stayed unchanged. |
| 8. Logic descriptions | Shared rules explain audited effects and prerequisites; enhancements identify generation's coverage boundaries. | Unverified shortcuts are not represented as required routes. |
| 9. Tricks catalogue | 197 native OoT entries and two verified MM Lens techniques share search, filters and persisted selection. | Live catalogue search, MM filter and descriptions passed on the emulator. This is the actually supported catalogue, not every OoTMM technique copied without engine verification. |
| 10. Combined guarantees | Both games' junk/confined/fixed rewards participate in all-check validation; completion uses the selected combined goal and actual Majora defeat. Native OoT generation policy follows the combined choice. | All seven final-source native generation/replay verdicts passed, including mixed rules, failure/retry integrity, completion hunt and No Logic. Cancellation preserves existing seed files and the saved slot byte-for-byte. No Logic explicitly has no guarantee. |
| 11. Hints/knowledge | Counterfactual boss/event paths in both games; Hinted/Seen/Known/Collected provenance and shared tracker presentation. | Final APK hidden Palace and laboratory entries remain unknown; an actually rendered Palace reward becomes Seen and persists after restart while uncollected. The positive renderer fixture used private player relocation and actor freeze, not a legitimate Palace route. Exact-hint provenance and reward-free path hints have production helper coverage. |

Host tests and clean pinned patch replay are distinguished from native and live emulator validation. See [changes and phone checks](FIXES-0.1.21.md), [release validation](VALIDATION-0.1.21.md), [coverage audit](ENHANCEMENT-LOGIC-AUDIT-0.1.21.md) and [logic verification notes](LOGIC-0121-VALIDATION.md).

## October 2 additional requests completed - 0.1.22

- OoT inventory editing now shows faded, clickable icons for all absent item slots, matching the requested MM behavior. Normal add/remove and save/reload checks passed.
- MM Save Editor now exposes ten named Owl Statues / Warp Destinations, with Activate all, Clear all and an active count. Native Soaring, saved destination flags and reserved-bit handling passed targeted checks.
- Shadow Door Unload is available on the shared Enhancements > Restorations page. Five complete Graveyard stair trips recreate the shortcut without exhausting memory or setting a permanent door switch. The generator may require it only through the normal Randomizer > Tricks & glitches selection; restoration-only use remains outside logic.

The final APK's menu and door behavior were tested directly, with editor/Soaring tests retained through byte-identical game engines. See [0.1.22 changes and phone checks](FIXES-0.1.22.md) and [validation](VALIDATION-0.1.22.md). Physical-phone and complete-playthrough limitations remain explicit above.

## Remaining supported additions - 0.1.23

This status supersedes earlier release tables; completed features are not upcoming tasks. Fierce Deity gap jumping is excluded by the user's latest instruction.

- Adult Soaring now has a shared runtime option and separate generation assumption. Local song/instrument/buttons and source owl activation remain required.
- The Bossa Nova egg threshold and audited daytime Skulltula spawns now have matching logic rules. Seahorse acquisition also retains its native bottle requirement.
- Song of Time age propagation now includes twenty audited ordinary interior regions and Lon Lon Ranch. Arbitrary dungeon age shortcuts remain outside the supported area list.
- Early Eyeball Frog is included in the normal combined Tricks & glitches catalogue; its restoration can also be used without a logic assumption.
- Eight additional verified MM route alternatives bring its common technique catalogue to ten entries. Both games' techniques are globally searchable, with focused navigation to the selected result.
- Shared tracker, screen/HUD, multiplayer and gameplay controls have explicit discovery aliases. About shows the actual Android package version; both ordinary save-editor interfaces retain sliders and faded absent slots.

The eleven accepted list items are covered by 0.1.21-0.1.23 together. This means the stated supported behavior, not every technique or equipment-sharing feature of the ROM randomizer. Source references and coverage limits are in [the current audit](LOGIC-AUDIT-0.1.23.md), [technique reference](MM-TRICKS-0123-REFERENCE.md) and [release validation](VALIDATION-0.1.23.md). Physical-phone, fold/cutout, acoustic latency and full-playthrough testing remain explicitly unverified.
