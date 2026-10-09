# Combo Ship 0.2.0 validation

The final APK, source replay, host/native tests and emulator checks below passed. The [completion audit](RELEASE-AUDIT-0.2.0.md) reconciles the accepted backlog and later requests with finished implementation and earlier versioned test records. This is a reviewed release stopping point, not a guarantee that the app contains no bugs.

## Final APK and source provenance

Version **0.2.0**, version code **25**, **45,578,895 bytes**. APK SHA-256:

`e2eea7bbdf074424765d14eb879d3449917b2b0a584aa5fcb78071a6666625b4`

The installed APK hash matches the candidate. Verification confirms the previous release's signing certificate, eight ARM64 libraries, 218 required native callbacks, 9,036 support files, 178 native source markers and 16-KiB alignment. Lint reports **zero errors** and 84 non-error findings; this is not a claim of warning-free lint. About displays the actual 0.2.0 version.

The complete overlay replays onto pinned upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. All 178 marker hashes and normalized source contents match the production checkout. The final integration-patch SHA-256 is `c1347ded989fdd91ce4b1c455a2d05f89b56e17225172e7544e1250d2cc1c3ea`.

## Host and production-block tests

All **26 host tests pass** with assertions enabled. The suite covers shared navigation/search, gameplay rules and slot snapshots, song prerequisites, native age propagation, tracker knowledge, editor flags, logical techniques, generation controls and retained protocol/input checks. Passing a protocol test is not a new live multiplayer test.

The song mapping test compiles the actual OoT/MM routing blocks, native endpoint tables, native entrance encoding and oracle/runtime exports. It passes:

- **6,656 native routing cases**, including all sixteen source-to-destination rotations, independent runtime/logical flags, unrelated exits and saved-map round trips.
- **16,384 oracle/runtime export queries**, preserving independent ability and logic layers.
- **40 native seasonal/form arrival cases**, including cleared Swamp and spring Mountain Village, plus the no-arrival negative case.

Separate negative regressions restore each original OoT search, MM search and legacy export defect in turn. **All three mutations are detected** by failed assertions. The combined launcher already resolves its oracle to the explicit logical export; the legacy export correction prevents a stale ABI from disagreeing with it.

Menu tests use the actual rule registry and production search/focus/history helpers. They verify multi-word and reordered queries, song aliases/ranking, canonical page discovery, MM owl-editor targeting, complete song grouping, one-shot focus, return history and distinct guide acknowledgement keys. Independent source review confirms that page links and guides do not edit gameplay values. Native setting widgets retain their original indices, callbacks and disabled predicates.

An additional test compiles the actual wrapped-label checkbox helper with host ImGui and supplies input events. It passes wrapped-label bounds, toggle on release, exactly one toggle per tap, cancelled drag, disabled input and the ordinary checkbox path. Its source hash matches the final `ComboMenu.cpp`. These are host ImGui events, not Android touch instrumentation.

## Actual ARM64 native oracle checks

The final native driver runs against the final APK's actual library bytes. **All 1,194 cases pass, with zero failures**: 1,088 song-map cases and 106 retained native progression cases.

Song cases exercise separate logical/runtime flags and destination exports, native OoT Minuet-to-Colossus reachability, native MM Clock Town-to-Great Bay reachability, foreign Requiem sources and Adult OoT Soaring sources. The OoT reachability target is the actual pooled Sheik-at-Colossus check. Required-item and enhancement-only negatives remain part of the retained tests.

The retained cases cover the eight newer MM techniques, Bossa Nova egg thresholds, Early Eyeball Frog, supported age-change propagation and Ageless Soaring. Oracle success verifies route predicates; it does not prove a player manually performed every difficult technique. The binding receipt records all eight final APK library hashes, the APK hash above and driver SHA-256 `d95a58061cff3f2899941f8a7ec6c87a94da69968b3d12127b0f1e51ce7fa811`.

The three strict seed generations and original-rule replays documented in [0.1.23 validation](VALIDATION-0.1.23.md) remain historical evidence. Those generations were not repeated for 0.2.0.

## Final APK menu and guide checks

Interactive inspection used the final APK on an API-35 emulator executing the ARM64 libraries through the native bridge. It verified:

- Shared Home, Settings, Audio, Controls, Screen & HUD and About pages.
- `adult warp` returns the canonical rule, opens the exact Randomizer or Enhancements row and highlights it. Back restores the original query and search scope; Clear exits result mode.
- `owl statues` ranks the Save Editor destination first, opens Tools with MM selected and shows the actual owl editor. Editing is correctly unavailable before loading a file.
- Both first-open guides have readable explanations and visible actions. Dismissal survives cold restart; Guide reopens each explanation. Browse from the guide reaches the corresponding rule page.
- Guide acknowledgement changes only the two help acknowledgement settings. All 23 runtime and 23 logical rule values remain unchanged.
- At both the normal wide viewport and a 1440-by-1080 landscape viewport, shared navigation, wrapped explanations and actions fit and scroll. The original emulator display size was restored before travel tests.

## Final APK native song travel, cancellation and save/reload

Controlled test saves supplied learned songs, the current game's instrument/buttons and explicit rule choices. Travel itself used ordinary native song inputs and confirmations; no world transition or physics callback was substituted.

| Check | Observed result |
| --- | --- |
| Adult OoT Soaring, ordinary destinations | Native staff/performance and OoT confirmation, then actual MM Clock Town owl arrival as human Link. |
| MM Requiem return | Native MM staff/performance and confirmation, then actual OoT Desert Colossus warp-platform arrival with Adult Link restored. |
| Saved mixed destinations | OoT Soaring from the unlocked Clock Town source routes to actual Mountain Village; MM Serenade routes back to actual Sacred Forest Meadow as Adult Link. The destination owl need not independently be unlocked. |
| Cancel native confirmation | Link remains in the current OoT world; no accepted travel or world switch is logged. Playing again works and completes the shuffled route. |
| Explicit native save | Pause, save prompt and confirmation produce the native save/finish record. |
| Cold restart and reload | The saved container is byte-identical after reload. Soaring again shows the saved Mountain Village destination and reaches that actual MM landing. |

The full authored randomizer data—placements, native settings, hints, original rules and song map—remains identical across the captured trips, explicit save and cold reload. All 23 runtime rules, 23 logical rules and ten MM techniques retain their intentional saved overrides. A read-only checker verifies these comparisons and rejects six deliberately corrupted in-memory cases. MM Serenade's return was checked before reload; after reload, the OoT-to-MM leg was repeated.

The captured Android crash buffer is empty for this session. Native performance phases and visual song presentation were checked; the headless emulator does not provide an audible-melody or phone-speaker latency measurement.

## Restoration and public package checks

The test emulator's five original internal files and five migrated visible-storage files were restored and verified byte for byte. The original touch preferences were separately restored and verified. No baseline file was deleted. The app was stopped, the disposable emulator shut down and its device serial was confirmed absent.

The public source archive uses the previous 150 allowlisted members plus six explicit additions, for **156 members**. Current file bytes, unique member names, CRCs, safe paths and the replayed production patch are checked before delivery. The native GUI headers are included through the integration patch. ROMs, private extracted game data, saves, seeds, spoilers, fixtures, test logs, derived geometry, signing keys and generated build directories are excluded.

Release gates bind native/runtime verdicts to the final APK hash and recheck its identity, certificate, alignment, callbacks, support files and source markers. Delivery consists of exactly five public artifacts: APK, source ZIP, FIXES, VALIDATION and SHA256SUMS. Their copied bytes are checked in the delivery folder and authorized local OneDrive folder. A local copy is not by itself proof of completed cloud synchronization.

## Validation boundaries

The completion audit distinguishes completed supported behavior from complete ROM-randomizer parity. Fierce Deity gap jumping remains outside logic at the user's request. Unselected research suggestions and unaudited movement routes are not silently added as generation assumptions.

There is no completed physical-phone or complete-playthrough test. Phone-speaker acoustic latency, cutouts/folding, controller differences, Internet/two-phone multiplayer and every upstream multiplayer mode remain unverified by this release's tests. Earlier audio and local co-op results retain only their specifically recorded scope. Emulator/native oracle proof is not physical-device certification or complete gameplay parity.
