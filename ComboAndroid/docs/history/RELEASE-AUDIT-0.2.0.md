# Combo Ship 0.2.0 completion audit

This reconciles the accepted list and later requests with completed implementation and recorded validation. It is not a claim of a complete seed playthrough, physical-device certification, every upstream multiplayer mode, or complete feature parity with the ROM randomizer. Historical release notes retain their original scope; this audit supersedes their lists of future work.

## Earlier interface and reliability requests

| Request | Completed behavior | Recorded evidence |
| --- | --- | --- |
| Readable OoT text and meaningful new-profile selection | Android tagged resource pointers are supported; the one-option quest carousel is bypassed. | [0.1.2](FIXES-0.1.2.md), pointer regression and emulator text/profile checks. |
| Simple two-ROM setup and one app in Recents | Choose OoT, choose MM, start; prepared installations launch the game directly. | [0.1.3](FIXES-0.1.3.md) and [0.1.4](FIXES-0.1.4.md), setup, update and generation checks. |
| Remove startup settings-import/preset prompts | Broken desktop import and unsolicited preset offer are skipped on Android. | [0.1.5](FIXES-0.1.5.md) and [0.1.6](FIXES-0.1.6.md), clean-settings and new-profile checks. |
| Combo Ship name and ocarina icon | Both launcher and setup use the requested branding. | [0.1.7](FIXES-0.1.7.md), launcher, Recents and About inspection. |
| Invisible floating sticks and movable Menu | Each screen half supplies its stick; Menu moves and persists with the other controls. | [0.1.8](FIXES-0.1.8.md) and [0.1.9](FIXES-0.1.9.md), real multi-touch events, movement, persistence and reset. |
| One compact tracker with a shortcut | Shared 2S2H-style side panel, visibility modes, configurable button/chord, filters and details. | [0.1.10](FIXES-0.1.10.md) and [0.1.11](FIXES-0.1.11.md), both engines, toggle, docking and restart. |
| Feature-based menus and consistent pools | Common Home/Enhancements/Randomizer/Trackers/Settings/Tools navigation; matching Locations and Extra items categories. | [0.1.12](FIXES-0.1.12.md) and [0.1.13](FIXES-0.1.13.md), both-world navigation and settings checks. |
| Functional combined multiplayer | Co-op synchronizes both inventories/checks, caches progress, rejects other seeds and suppresses duplicate rewards. | [0.1.14](FIXES-0.1.14.md), two local Android clients, both transfer directions, reconnect/server restart and save/reload. Physical Internet/phone tests and upstream MM PvP remain outside that proof. |
| MM knows actually seen rewards | Rendered-item observation and camera/collision gate; explicit hints have separate provenance. | [0.1.15](FIXES-0.1.15.md), [0.1.17](FIXES-0.1.17.md), and [0.1.21](VALIDATION-0.1.21.md), hidden Palace/laboratory negatives, rendered positive and cold reload. Earlier false observations are not erased automatically. |
| Lower Android audio delay | Shared queue/output blocks and native-rate conversion corrected. | [0.1.16](FIXES-0.1.16.md), production queue/converter and Android checks; phone speaker acoustic latency remains unmeasured. |

## Accepted larger patch list and later additions

| Request | Completed behavior | Recorded evidence or deliberate boundary |
| --- | --- | --- |
| Bremen quality of life | One-chick alternative, including seed/run rules. | Live quest/reward checks in [0.1.18](FIXES-0.1.18.md). Faster marching was the unchosen alternative. |
| Cross-game transportation and logic | Native songs, local instrument/buttons, learned-song ownership, source owl unlocks, optional saved mixed destinations, generation and tracker routes. | [0.1.21](VALIDATION-0.1.21.md), [0.1.23](VALIDATION-0.1.23.md) and the additional final-source checks in [0.2.0 validation](VALIDATION-0.2.0.md). Requiem is always required for its route; the playable instrument belongs to the currently played game. |
| Adult OoT Soaring | Opt-in any-age Soaring with independent runtime/logic selection. | Actual native search and native performance/no-owl/cancel/save checks in 0.1.23; final transport coverage listed in 0.2.0 validation. Cross-game playback must also be enabled. |
| Uniform bounded numeric controls | Sliders for ordinary quantities; shared editors/HUD presentation. | [0.1.21](VALIDATION-0.1.21.md), both live rupee editors and HUD placement. Raw IDs/physics and exact network ports intentionally retain precise text input. |
| Well without Storms through age change | Adult descent followed by valid child transformation and real Well entry. | Exact live route and production prerequisites in 0.1.21, without a drained flag or coordinate edits. |
| Logic-affecting enhancements | Explicit shared registry: Randomizer enables ability and logic; Enhancements can use an ability alone, with an explicit Include in logic choice. | [Enhancement audit](ENHANCEMENT-LOGIC-AUDIT-0.1.21.md) and [0.1.23 audit](LOGIC-AUDIT-0.1.23.md). Unsupported shortcuts are not generation assumptions. |
| First-boot screen fit and obvious HUD movement | Automatic balanced fitting, safe bounds, manual overrides and shared Screen & HUD editor with drag/reset. | Two emulator aspect ratios and both editors in 0.1.21; physical folds/cutouts remain device checks. |
| Rules persisted with every seed/run | Immutable authored rules/map/placements plus saved intentional run overrides and warning-only edits. | Cross-slot, migration and cold-reload tests; three strict 0.1.23 native generations/replays preserve all 23 rules and ten MM techniques. |
| Explain logic effects | Rule descriptions identify supported prerequisites and limits; 0.2.0 adds first-open section guidance and reusable help. | Rule registry/predicate review and the 0.2.0 menu checks. No mid-run validity gate or forced regeneration. |
| Common tricks catalogue | 197 OoT techniques, two restorations and ten verified MM techniques, searchable with descriptions. | Public/native predicates and actual engine route tests in 0.1.21–0.1.23. This is not every OoTMM technique. |
| Same combined completion guarantee | Every check reachable or Only completion guaranteed; strict coverage includes both games' junk/fixed/confined checks and actual selected victory. | Native generation/replay cases in 0.1.21 and three strict 0.1.23 generations with zero unreachable checks. No Logic explicitly has no guarantee. |
| Boss/event paths and knowledge states | Cross-game path analysis, distinct Hinted/Seen/Known/Collected and saved reward knowledge. | Counterfactual path/provenance tests and hidden/visible live observations in 0.1.21. |
| OoT absent inventory icons and MM owls | Faded clickable absent items; ten named owl destinations, activate/clear all and counts. | [0.1.22](VALIDATION-0.1.22.md), editor/save/reload and native Soaring checks. |
| Shadow door unload restoration | Five full stair trips unload the door safely; logic uses it only when its normal trick is selected. | Source/production predicate review and final APK door/trick checks in 0.1.22. |
| Remaining supported logic options | Bossa Nova egg count, audited daytime Skulltulas, conservative Song of Time interior propagation, Early Eyeball Frog and eight additional MM routes. | [0.1.23](VALIDATION-0.1.23.md), extracted production cases, 106 actual native oracle cases and strict generation/replays. |
| Search for warp and improve discoverability | Ranked multi-word search, aliases, focused rule navigation, results return and clear; song settings grouped together. | New public tests and final APK navigation checks listed in 0.2.0 validation. |

Fierce Deity's verified sword, shallow-water and Big Octo effects remain supported; extra gap jumping is excluded by the user's explicit instruction. The three unselected research suggestions—missing-requirement explanations, shortened adventures and expanded shared equipment—were not added to this patch. No accepted list item is left listed as future implementation work within the supported behavior above. Remaining physical-device and full-playthrough validation limits are preserved rather than relabeled as completed tests.
