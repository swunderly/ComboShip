# Code review, October 10, 2026

An independent review read every line the earlier AI agent added (about 70,000 lines in 16 areas). Two reviewers with different focuses read each area, and every finding was then checked against the code by separate verifiers trying to refute it. 200 findings were confirmed and 15 refuted. 187 are fixed in 0.3.10, 10 are deferred and 3 low-value cleanups were skipped.

Severity: high = crash, corruption or a broken feature in a realistic case; medium = wrong behaviour in a narrower case; low = minor wrong behaviour; cleanup = dead code, leftovers or misleading comments.

| ID | Severity | Area | Finding | Outcome |
| --- | --- | --- | --- | --- |
| F5 | high | combo-core | File-select Generate/import/Start use comboui's selected settings file, not the file-select slot | fixed |
| F22 | high | combo-gui-menu | OoT Mask Quest dropdown in the Randomizer editor stores the wrong values (Shuffle saves Completed, Completed saves Vanilla) | fixed |
| F61 | high | combo-rando-rules | Range-for over items() of a temporary JSON in parsePool: use-after-free on every generation | fixed |
| F75 | high | oot-silver-rupees | Shuffled silver rupees in midair or on fences cannot be collected (pickup needs Link standing on the ground) | fixed |
| F76 | high | oot-silver-rupees | Anchor team-state resync resets received silver-rupee counts to zero (save progress loss) | fixed |
| F77 | high | oot-silver-rupees | Every physical silver rupee check gives its item twice (direct GI offer, then the RC queue) | fixed |
| F84 | high | oot-souls-enums | Anchor team-state sync zeroes received silver rupee counts and OoT soul ownership | fixed |
| F110 | high | oot-runtime | Anchor team-state resync zeroes received silver rupees and OoT soul ownership | fixed |
| F113 | high | oot-runtime | Save Editor inventory tab throws std::out_of_range on Roc's Feather | fixed |
| F128 | high | mm-rando-runtime | Moon field crashes when NPC souls are shuffled and the Moon Children soul is not owned | fixed |
| F6 | medium | combo-core | Editing the next seed's settings for the file being played rewrites that run's live rules, including ones its logic needs | fixed |
| F18 | medium | combo-gui-menu | Global search crashes when the randomizer catalog was never loaded (first profile inspection failed) | fixed |
| F23 | medium | combo-gui-menu | Enhancements-page logic rules edit the profile selector's file draft, not the running game | fixed |
| F24 | medium | combo-gui-menu | The mid-run rule-change warning can no longer fire for gameplay rules while profiles are active | fixed |
| F35 | medium | combo-gui-trackers | Profile drafts made from old (pre-0.3.9) seeds have no gameplayRules version, so every gameplay-rule edit is silently dropped, permanently for that file | fixed |
| F36 | medium | combo-gui-trackers | Blue Fire / Sunlight Arrows logic choice is overwritten at generation by a stale value in oot.settings | fixed |
| F37 | medium | combo-gui-trackers | New button-combination control has no Modifier 1/2 entries, so default modifier bits can't be seen or removed | fixed |
| F43 | medium | combo-gui-trackers | Shared button-combination editor omits Modifier 1/2, so native default combos are invisible and become hidden chords | fixed |
| F50 | medium | entrances | Mixed-mode grotto exit keeps nextEntrance=0xFFFF into the transition: out-of-bounds sSceneEntranceTable read | fixed |
| F51 | medium | entrances | Ordinary MM ER layouts always include the MM-only clock-tower reservation, which breaks the MM->OoT portal | fixed |
| F64 | medium | combo-rando-rules | Path hints include optional bosses and promote non-required areas into the WotH pool (OoTMM only paths goal-required targets) | fixed |
| F65 | medium | combo-rando-rules | Fill treats MM Magic Beans at OoT shops, cows, scrubs and fairies as renewable, but SoH gives those items only once | fixed |
| F78 | medium | oot-silver-rupees | Silver rupee items and locations use RHT_NONE, so gossip stones say "No Hint" | fixed |
| F82 | medium | oot-souls-enums | Thieves' Hideout pots/crates/wonder items reclassified as dungeon checks with no seed revision, so old seeds lose placed items | fixed |
| F90 | medium | oot-rando-logic-trackers | Thieves' Hideout pots/crates/wonder items switched to 'dungeon' at runtime without a seed revision: old Overworld-shuffle saves lose those items | fixed |
| F91 | medium | oot-rando-logic-trackers | Non-oracle generation searches (OoT entrance validation) add live/stale player song state to the region graph | fixed |
| F93 | medium | oot-rando-logic-trackers | COMBO logic rewrites drop Ship's Shuffle Grab / Shuffle Climb requirements | fixed |
| F100 | medium | oot-crosswarp | Stale cross-game FW sentinel (0xFFFD) falls through to native OoT return when cross FW is inactive | fixed |
| F102 | medium | oot-crosswarp | MM soaring sequence ids can alias Audio Editor's reserved instrument ids (130-135) | fixed |
| F111 | medium | oot-runtime | New save inherits silver rupees, named triforces and souls from the previously loaded file | fixed |
| F122 | medium | mm-rando-logic | Blacksmith access lost its Day 1/2 limit, but the native smithy refuses on Day 3 | deferred |
| F123 | medium | mm-rando-logic | Ranch alien defense no longer requires the Alien soul | fixed |
| F132 | medium | mm-rando-runtime | Tingle map checks have no Tingle-soul prerequisite, but Tingle (EN_BAL) is deleted without his soul | fixed |
| F133 | medium | mm-rando-runtime | Romani Ranch field cow checks miss the cow-soul prerequisite although EN_COW is soul-gated | fixed |
| F134 | medium | mm-rando-runtime | Farore's Wind readiness depends on hidden Din/Nayru resources and exact native texture sizes; HD packs silently disable it while seeds still place and require it | fixed |
| F141 | medium | mm-2s2h-rest | Team-state resync overwrites new per-player save fields (Farore's Wind point and ownership, spell and ocarina bits, adultMm) with the teammate's values | fixed |
| F156 | medium | mm-adult-link-code | Fairy Ocarina hand hook branches the renderer into a resource-name string | fixed |
| F180 | medium | android-engine-tools | Native boot crash causes an auto-relaunch crash loop; the .running marker is written but never read | fixed |
| F189 | medium | android-engine-tools | Missing SD card blocks every switch to another data location, including the recovery the error message suggests | fixed |
| F190 | medium | android-engine-tools | Native crash leaves no failure marker, so the launcher auto-restarts the crashing game; the '.running' marker is write-only | deferred |
| F191 | medium | android-engine-tools | Bundled support files are only refreshed when versionCode changes; a same-version rebuild keeps stale archives | fixed |
| F0 | low | combo-core | OoT save worker reads CVars (raw string pointer) while main thread rewrites them on every save load | fixed |
| F1 | low | combo-core | Generation worker publishes finalize before its RAII scopes restore DLL state | fixed |
| F2 | low | combo-core | Loading a pre-gameplayRules save resets about 25 enhancement CVars and all MM tricks to defaults | deferred |
| F3 | low | combo-core | Private MM entrance-shuffle generators can be enabled through profile/seed setupCVars | fixed |
| F4 | low | combo-core | Save creation ignores rejection of the seed's MM settings and builds the MM save from live CVars | fixed |
| F7 | low | combo-core | Rejected profile generation leaves the old success flag set, so the file select plays the success fanfare | fixed |
| F8 | low | combo-core | displaySeed now hashes runtime-only and enhancement-only rule values | fixed |
| F9 | low | combo-core | Headless generator says it matches in-game generation but does not | fixed |
| F10 | low | combo-core | Combo_OnFinalBossDefeated can undo an earlier hunt or quest win | fixed |
| F15 | low | combo-core | Per-frame settings-file rewrite and full inspect while dragging a profile slider | fixed |
| F19 | low | combo-gui-menu | Search 'Open technique' focus is sent to a window that is never drawn | fixed |
| F20 | low | combo-gui-menu | ComboTouch::Count lets values wrap to the field type, and the MM save editor passes ±32768 for an s8 field | fixed |
| F25 | low | combo-gui-menu | Profile option policy does not grey out the Mask Shop exclusions for a Majora's Mask start | fixed |
| F26 | low | combo-gui-menu | "Search all settings" does not index most MM randomizer options | fixed |
| F27 | low | combo-gui-menu | Shared items grid leaves a blank cell before "Shared Farore's Wind" | fixed |
| F38 | low | combo-gui-trackers | Named-Triforce overlay reads MM's dormant save without the launcher's slot-residency check | fixed |
| F39 | low | combo-gui-trackers | Every profile edit rewrites the whole settings-profiles.json and re-inspects the full snapshot, including on every slider-drag frame | fixed |
| F40 | low | combo-gui-trackers | DrawUnifiedTricks is never called; FocusUnifiedTrick's pending focus is never consumed | fixed |
| F44 | low | combo-gui-trackers | Search 'Open technique' focus is never applied: it targets a dead tricks page | fixed |
| F45 | low | combo-gui-trackers | Live tricks page search is case-sensitive for catalog techniques | fixed |
| F46 | low | combo-gui-trackers | Saved/Generated seed page deep-copies the whole seed JSON and lays out every placement row each frame | fixed |
| F52 | low | entrances | Mixed-mode Door_Ana grottos are never redirected: the marker is cleared before the fall transition starts | fixed |
| F53 | low | entrances | Generation worker restores the MM entrance layout after publishing completion, racing the main-thread finalize | fixed |
| F56 | low | entrances | MM Wallmaster transport clears a dead copy of the dungeon ingress, leaving the real dungeon respawn stale | fixed |
| F62 | low | combo-rando-rules | Requiredness replays re-parse the full MM dump on every sphere (and both dumps per replay) | fixed |
| F63 | low | combo-rando-rules | Hidden MM tricks are applied from seed/profile JSON into live CVars that the menu cannot clear | fixed |
| F66 | low | combo-rando-rules | Loading a vanilla or pre-0.3.9 save resets the player's global enhancement CVars to defaults | fixed |
| F67 | low | combo-rando-rules | 'Zelda's escape' path target uses the wrong OoT check name and can never be reached | fixed |
| F68 | low | combo-rando-rules | RunPlaythrough now walks to the collected fixed point, which changes spoiler output and kills the headless No-Logic verdict branch | fixed |
| F69 | low | combo-rando-rules | SetLogicRule(rule, 0) also switches off the standalone enhancement; 'Apply preset' resets every player enhancement | fixed |
| F71 | low | combo-rando-rules | Playthrough and pare-down omit mmRootAtStart in the cross-age song search (dormant) | fixed |
| F85 | low | oot-souls-enums | Soul spawn block on Bongo Bongo/Morpha omits OoTMM's compensating floor/water handling | fixed |
| F86 | low | oot-souls-enums | EventPrerequisite gates by event id, adding soul requirements OoTMM does not have | fixed |
| F87 | low | oot-souls-enums | River-sound soul gate blocks a spawn that OoTMM never actually blocks | fixed |
| F88 | low | oot-souls-enums | New items use wrong hint keys: souls/silver rupees read 'No Hint', named Triforces read 'a Piece of the Triforce' | fixed |
| F92 | low | oot-rando-logic-trackers | Item tracker tooltip shows mojibake "DampÃ©'s Hut Key" | fixed |
| F94 | low | oot-rando-logic-trackers | Enemy-soul gate in CanKillEnemy makes absent (soulless) enemies block CanPassEnemy/CanAvoidEnemy | deferred |
| F95 | low | oot-rando-logic-trackers | Gameplay-rule and soul lookups added to hot logic predicates (linear, allocating, per call) | deferred |
| F101 | low | oot-crosswarp | MM soaring audio RM freed by MM_Deinit teardown listener while OoT audio thread still runs | fixed |
| F103 | low | oot-crosswarp | Owl departure actor kills itself on the warp frame; Link, camera, HUD and old BGM come back during an OoT-bound fade | fixed |
| F104 | low | oot-crosswarp | Soaring map plays the cursor sound even when the cursor cannot move | fixed |
| F115 | low | oot-runtime | File select deep-copies the pending seed several times per frame | fixed |
| F120 | low | mm-rando-logic | Per-slice predicate evaluation multiplies all MM logic cost by ~46 (unmeasured on device) | deferred |
| F121 | low | mm-rando-logic | Stone Tower pinned graph is chosen at ShipInit time from whatever save is in memory | fixed |
| F124 | low | mm-rando-logic | Gilded Sword uses a one-time Gold Dust bottle that this engine empties on cycle reset | fixed |
| F129 | low | mm-rando-runtime | Hidden spell, complete-soul and adult options can be turned on through profile or CVar settings | fixed |
| F130 | low | mm-rando-runtime | Granting Fairy Ocarina after Ocarina of Time replaces it in the inventory slot | fixed |
| F135 | low | mm-rando-runtime | Business scrub 'cannot buy' result shows stale or contradictory text, unlike OoTMM; the COMBO bean branch in IsEligible is dead | fixed |
| F142 | low | mm-2s2h-rest | Grotto previous-scene context is never serialized and is cleared from live RAM by every pause save and autosave | fixed |
| F143 | low | mm-2s2h-rest | Hidden Adult-MM options can still enter a new save, and such a save is then refused on every load (-6) | fixed |
| F145 | low | mm-2s2h-rest | Generated profile layout ships an unlabelled Traps slider and junk tooltips | skipped (cleanup) |
| F146 | low | mm-2s2h-rest | Owl-mask change disables vanilla 'bit 15 only' Soaring and makes PauseOwlWarp's special case unreachable | fixed |
| F147 | low | mm-2s2h-rest | Clock Shuffle silently forces Better Song of Double Time but the enhancement checkbox is not checked or greyed | fixed |
| F148 | low | mm-2s2h-rest | criticalRenewable export omits Hero's Shield and Magic Bean from OoTMM's CRITICAL_RENEWABLES | fixed |
| F149 | low | mm-2s2h-rest | grottoPreviousScene/grottoPreviousSceneValid are never serialized; SaveManager's owl-blob preservation of them is dead | fixed |
| F150 | low | mm-2s2h-rest | Foreign OoT warp songs require Ocarina of Time even when MM Fairy Ocarina is a playable instrument | fixed |
| F155 | low | mm-adult-link-code | Hidden adult/gear rando options create seeds whose MM save the loader always refuses | fixed |
| F157 | low | mm-adult-link-code | Child Iron/Hover boot display lists reference texture names that do not exist | fixed |
| F159 | low | mm-adult-link-code | Hand-written child Iron/Hover boot DLs reference nonexistent textures and unmarked segment addresses (latent crash) | fixed |
| F160 | low | mm-adult-link-code | AdultMask_RefreshButtonIcons memsets 4 KB into the iconItemSegment pointer array (heap corruption) | fixed |
| F161 | low | mm-adult-link-code | Hover-boots circle DL: unmarked segment 0x09 call and unaligned texture paths | fixed |
| F162 | low | mm-adult-link-code | Rando Fierce Deity mask grant now bypasses Item_Give and the OnItemGive hooks | fixed |
| F165 | low | mm-adult-link-code | Double-encoded UTF-8 (mojibake) in modified comments | fixed |
| F169 | low | mm-overlays | Showing Farore's Wind / Din's / Nayru's to an NPC indexes sGetItemTable[-1] | fixed |
| F170 | low | mm-overlays | Pause equip animation treats Din's/Nayru's (0xB8/0xB9), Adult Mask (0xBC) and adult gear (0xC4-0xC7) as magic arrows (OOB color table) | fixed |
| F174 | low | mm-overlays | Goron Tunic check put in the lava-damage function instead of burn duration; tunic gives full lava immunity | fixed |
| F181 | low | android-engine-tools | Copying data rewrites comboship.json through Android org.json, which turns whole-number float settings into integers | fixed |
| F182 | low | android-engine-tools | An interrupted migration or storage copy leaks a hidden full-size .comboship-copy-* folder in the root of internal storage | fixed |
| F183 | low | android-engine-tools | The 'App options' launcher shortcut closes a running game without the save warning | fixed |
| F184 | low | android-engine-tools | GameActivity does not handle all configuration changes, so a recreate tears down the native session inside the same process | fixed |
| F186 | low | android-engine-tools | Mis-encoded (double-UTF-8) text in AI-edited files, including one user-visible item name | fixed |
| F192 | low | android-engine-tools | N64 Mode (and other low-res modes) silently do nothing on Android while Screen fit is Automatic (the fresh-install default) | fixed |
| F193 | low | android-engine-tools | Fresh install cannot use 'Private app storage' / SD locations; selecting one shows a misleading SD-card error | fixed |
| F194 | low | android-engine-tools | Old copy left after migration can never be removed from inside the app | fixed |
| F195 | low | android-engine-tools | Touch 'Change control size'/'Change opacity' cycles never return to the defaults | fixed |
| F11 | cleanup | combo-core | Legacy setupCVars branch in ProfileApplyNative is unreachable and differs from the live version | fixed |
| F12 | cleanup | combo-core | Profile catalog 'combo.options' is built and sent on every inspect but never read | fixed |
| F13 | cleanup | combo-core | MM entrance generation (ordinary/grotto/mixed/Wallmaster) cannot be turned on from any menu | deferred |
| F14 | cleanup | combo-core | Diagnostic env-var probe and debug export check left in production main() | fixed |
| F16 | cleanup | combo-core | Tricks CSV applied twice on save load | fixed |
| F17 | cleanup | combo-core | Retry comments and log refer to a bounded attempt budget that interactive generation no longer has | fixed |
| F21 | cleanup | combo-gui-menu | Dormant and dead menu code: DrawCommunityPresets, DrawComboPanel, write-only guide CVars, no-op DrawSongTravel | fixed |
| F28 | cleanup | combo-gui-menu | ComboMenu::DrawComboPanel is dead but was still edited | fixed |
| F29 | cleanup | combo-gui-menu | DrawSongTravel is an empty function still called every frame | fixed |
| F30 | cleanup | combo-gui-menu | Guide "seen" CVars are written but never read, and the guide has a hard-coded travel button | fixed |
| F31 | cleanup | combo-gui-menu | No-op save lambda called six times in DrawCombinedGoals | fixed |
| F32 | cleanup | combo-gui-menu | ProfileOotOptionPolicy.h is a hand-copied duplicate of SoH's option callbacks with no parity test | fixed |
| F33 | cleanup | combo-gui-menu | FeatureMenuRoute and the Fixes re-filter repeat MenuGroupRoute's column-2 rule | fixed |
| F34 | cleanup | combo-gui-menu | Em-dashes in comments are garbled (UTF-8 mis-decoded as Windows-1252) | fixed |
| F41 | cleanup | combo-gui-trackers | Tracker-swap leftovers: unused master-visibility API, unused Kind fields and stale comments | fixed |
| F42 | cleanup | combo-gui-trackers | Re-encoded comments in ComboWidgetRender.cpp (em dashes became 'â€”') | fixed |
| F47 | cleanup | combo-gui-trackers | Retired swap/peek tracker plumbing left behind as dead code | fixed |
| F48 | cleanup | combo-gui-trackers | Stale comments describe the removed per-frame reconcile, HideBackground peek and 0/1 window type | fixed |
| F49 | cleanup | combo-gui-trackers | Duplicated tracker helpers and repeated per-widget predicate evaluation | fixed |
| F54 | cleanup | entrances | Cross-game entrance flight is dormant: no caller of SOH_/MM_RequestEntranceDeparture | deferred |
| F55 | cleanup | entrances | grottoPreviousScene/Valid are never serialized, yet owl-save code and comments treat them as persisted | fixed |
| F57 | cleanup | entrances | Entrance registry comes from OoTMM 41a2b7c, its generator is not in the repo, and its hash is pinned into every saved seed | fixed |
| F58 | cleanup | entrances | The whole MM entrance-randomizer stack is reachable only through hidden CVars with no menu | deferred |
| F59 | cleanup | entrances | Dead entrance plumbing: wire/arrival types, unused exports and helpers | fixed |
| F60 | cleanup | entrances | Duplicated entrance tables and fields that must be kept in sync by hand | skipped (cleanup) |
| F70 | cleanup | combo-rando-rules | Triforce Quest piece placement deviates from the OoTMM solve.ts it claims to copy | fixed |
| F72 | cleanup | combo-rando-rules | Unreferenced helpers in new rule headers (Farore hot-water and GBT routes, MM arrival adapter, misc) | fixed |
| F73 | cleanup | combo-rando-rules | QueryReachable/QueryReachableMemo/ReachResult/OraclePortalOpen left orphaned in ComboPlaythrough.h | fixed |
| F74 | cleanup | combo-rando-rules | BEATABLE_ONLY relaxation comment says MM items always retry, but the code now relaxes MM items too | fixed |
| F79 | cleanup | oot-silver-rupees | The check and item tables are written out twice; the second copy plus extra state API exist only for tests | fixed |
| F80 | cleanup | oot-silver-rupees | About 2,800 lines of test-only JSON fixtures sit in the production source directory | fixed |
| F81 | cleanup | oot-silver-rupees | SilverRoutes.inc has an event nothing reads and five exits that can never be taken | fixed |
| F83 | cleanup | oot-souls-enums | In-progress OoT souls are switched on only by CVars with no menu widget, which profile apply still accepts | fixed |
| F89 | cleanup | oot-souls-enums | Soul family boundaries and index list duplicated as magic numbers in several places | fixed |
| F96 | cleanup | oot-rando-logic-trackers | SpiritShared re-implements Logic::CanUseSongAgeChange inline | fixed |
| F97 | cleanup | oot-rando-logic-trackers | Tracker settings 'Always Show GS' / 'Hide Unshuffled Shop Checks' are dead in COMBO but still read every snapshot | fixed |
| F98 | cleanup | oot-rando-logic-trackers | Non-COMBO branch of the check tracker no longer compiles (default argument only in COMBO forward declaration) | fixed |
| F99 | cleanup | oot-rando-logic-trackers | Age-change allowlist comment says underwater positions are excluded, but Ice Cavern's vanilla underwater region is allowed | fixed |
| F105 | cleanup | oot-crosswarp | Local DEG_TO_BINANG drops MM's (s32) step, so float-to-s16 conversion is out of range | fixed |
| F106 | cleanup | oot-crosswarp | Shared-dungeon rules can never be enabled; dungeon respawn only works with Cross-Games FW on | fixed |
| F107 | cleanup | oot-crosswarp | Dead and leftover code in the CrossWarp port | fixed |
| F108 | cleanup | oot-crosswarp | Diagnostic logging and a per-command trace hook left in the production audio path | fixed |
| F109 | cleanup | oot-crosswarp | Symbol lookup and resource-manager lookup repeated inside per-frame draw loops | fixed |
| F112 | cleanup | oot-runtime | Developer actor-exhaustion scan and log left in the En_Holl room-load path | fixed |
| F114 | cleanup | oot-runtime | Profile seed is fully reloaded twice per new file | fixed |
| F116 | cleanup | oot-runtime | Song-travel exports with no runtime caller, one with a misleading comment | fixed |
| F117 | cleanup | oot-runtime | Duplicated tables and logic in the new OTRGlobals/profile code | fixed |
| F118 | cleanup | oot-runtime | Combo-build leftovers: unused load wrappers, stale comments, repeated magic values | fixed |
| F119 | cleanup | mm-rando-logic | Junk-replacement trials always 'succeed' because resetting resource sources is counted as progress | fixed |
| F125 | cleanup | mm-rando-logic | Unused MM logic helpers: moon reset macro/function, Goron Tunic macro, IsTimeSliceOwned | fixed |
| F126 | cleanup | mm-rando-logic | Snowhead pillar: unused Farore event and a stale 'Din unsupported' comment | fixed |
| F127 | cleanup | mm-rando-logic | Leftover comment about the removed East Clock Town stray fairy check | fixed |
| F131 | cleanup | mm-rando-runtime | Every drawn randomizer model runs a collision line test each frame before the cheap eligibility checks | fixed |
| F136 | cleanup | mm-rando-runtime | 'Missing MM souls' family (Igos boss, Bio Baba, Floormaster, Flying Pot, Real Bombchu, Skullwalltula) is dormant: RO_MM_COMPLETE_SOULS is never set | fixed |
| F137 | cleanup | mm-rando-runtime | Unshuffled-soul starting items are no-ops; readiness flags are permanently true; some soul helpers are unused | fixed |
| F138 | cleanup | mm-rando-runtime | ExpandedSoulsLogic.h hard-codes about 150 soul indices as static_cast<ExpandedSoul>(N) | fixed |
| F139 | cleanup | mm-rando-runtime | Tracker visibility test duplicates the shared TrackerItemInView helper and records progressive items twice per frame | fixed |
| F140 | cleanup | mm-rando-runtime | Misleading comments: Farore's Wind called 'dormant' though exposed; code cites an OoTMM revision other than the pinned v32.3 | fixed |
| F144 | cleanup | mm-2s2h-rest | No compile-time guard that sizeof(Save) fits SAVE_BUFFER_SIZE_HALF after the RandoSaveCheck growth | fixed |
| F151 | cleanup | mm-2s2h-rest | Dead foreign-song exports and an unreachable multi-choice selector in the native song dialogue | fixed |
| F152 | cleanup | mm-2s2h-rest | Unreachable key-ring case labels in ItemTracker switch statements | fixed |
| F153 | cleanup | mm-2s2h-rest | Hidden debug/test hooks left in production paths | fixed |
| F154 | cleanup | mm-2s2h-rest | Misleading or stale comments and magic values in the port layer | fixed |
| F158 | cleanup | mm-adult-link-code | Adult readiness check runs on the audio thread and mutates shared state without locks | deferred |
| F163 | cleanup | mm-adult-link-code | Unreferenced adult helpers duplicate live logic | fixed |
| F164 | cleanup | mm-adult-link-code | 'Pinned OoTMM' claims for Adult Mask and startingAgeMm point at code that is not in pinned v32.3 | fixed |
| F166 | cleanup | mm-adult-link-code | Foreign-song debug diagnostic calls left in native ocarina/message paths | fixed |
| F167 | cleanup | mm-adult-link-code | Stale or contentless COMBO comments in z_actor.c and z_play.c | fixed |
| F168 | cleanup | mm-adult-link-code | Song-arrival swamp/mountain remap in title_setup duplicates Play_Init's native 0xFFEF remap | fixed |
| F171 | cleanup | mm-overlays | Adult gear items 0xC4-0xC7 missing from Player_ItemToItemAction; grant path is not gated by readiness | fixed |
| F172 | cleanup | mm-overlays | Astral Observatory telescope ER branch runs after the vanilla audio mute and sequence disable | fixed |
| F173 | cleanup | mm-overlays | Debug 'Soaring baseline' logging left in EnTest7; dead wind macro/function | fixed |
| F175 | cleanup | mm-overlays | Two OoTMM boots hooks not ported: iron-boots environment wind and hover-boots sound; wind helper left unused | fixed |
| F176 | cleanup | mm-overlays | Mask page overwrites the blue "cycle available" cursor on the next line | fixed |
| F177 | cleanup | mm-overlays | Spell casting state machine duplicated; Farore's Wind branches in the shared copy cannot run | skipped (cleanup) |
| F178 | cleanup | mm-overlays | Unused Adult Mask helpers and an empty stub called from player code | fixed |
| F179 | cleanup | mm-overlays | Stale or misleading comments in the new Adult and CrossWarp code | fixed |
| F185 | cleanup | android-engine-tools | Build scripts only build pristine upstream plus patch files; the fork's own source edits never reach the APK | fixed |
| F187 | cleanup | android-engine-tools | Committed comparison data links to the author's local PC paths | fixed |
| F188 | cleanup | android-engine-tools | Production-dead helper RuntimePaths::libraryPath | fixed |
| F196 | cleanup | android-engine-tools | User-facing 'Import PC-prepared Adult resources' requires an archive no tool in the repo can produce | fixed |
| F197 | cleanup | android-engine-tools | import_mm_combat.py is a stale generator; running it would regress MmCombatRules.h and break the build | fixed |
| F198 | cleanup | android-engine-tools | Patch-series build tools and docs cannot run against the fork; duplicated hard-coded path lists | fixed |
| F199 | cleanup | android-engine-tools | Encoding-corrupted (mojibake) comments introduced into upstream libultraship files | fixed |
