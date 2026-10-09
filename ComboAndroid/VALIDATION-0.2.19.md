# Combo Ship Android 0.2.19 validation

APK SHA-256: `d134f8379f3f52e5c209d6275c92dc619e6155215f511a7f774606f7c94aa91e`

Version 0.2.19, version code 44, package `org.comboship.android`; 46,057,587 bytes. Reference: OoTMM MIT revision `41a2b7c0d7dbbb53fed928652f8271bc336c113f`. The upstream license is included in the APK and public source archive.

## Build and package

All 49 host checks pass. The ARM64 native build and a fresh replay of all 298 patched source files pass. Every packaged native library matches a copy of the current staged library stripped with the configured NDK. All 8 libraries, 236 generated callbacks and 9,036 support files are verified. Signing continuity with 0.2.18, installed-byte equality, 16 KiB alignment and Android lint pass; lint has 0 errors and 84 nonerror findings.

The final candidate was rebuilt after correcting shared MM starting Song of Time accounting. Its own generation, full-hint and emulator evidence is required; the archived earlier candidate tests are not transferred through a license-only repackage claim. The final APK and source archive include the full upstream MIT notice. Signing, installation and native provenance are checked independently.

## Native generation

All 22 feature fixtures pass generation with Every check reachable and independent replay under their original rules: six goals from each starting game, three key-ring configurations, five silver-rupee/pouch/MQ configurations and two new-shared-family configurations. These 22 disable only the three requiredness-hint surfaces to avoid repeating hundreds of hint-analysis replays per fixture; pools, progression requirements, goals and every-check guarantees are unchanged.

Four separate full-hint cases pass on the exact final candidate: Either boss, Ganon, Majora and Triforce Quest from OoT. Earlier Both-boss full-hint runs remain historical evidence and do not certify this candidate. Failed seed attempts remain in the local evidence, followed by successful generation/replay; no weaker guarantee or rule fallback is used. This is targeted full-hint coverage, not a 22-case full-hint matrix or a performance guarantee for every configuration.

## Android runtime

On a disposable ARM64 Android emulator, child OoT Soaring reached MM Great Bay using native song/map/departure/arrival; MM Requiem reached OoT Desert Colossus using native song/particles/arrival; adult fast playback with skipped departure retained the selected MM landing. Recordings were reviewed for departure effects, camera, hidden HUD and restored controls. Logs show the native scores and four wing cues executing with no unsupported sequence-dialect failures. Native audio warnings corresponding to the previously tested MM baseline remain; they are not an acoustic pass.

A legacy MM fixture was saved through the normal in-game save prompt, then its exact first serialized bytes were cold-loaded in a new process and saved again. The successful reload followed an earlier failed attempt, whose evidence was retained. Private warp fixtures were used between cases, so the tested first-save bytes were explicitly restored before the successful cold load. Inventory, songs, progress, check/reward identities, seen-item records, goal/shared rules and old option prefix were preserved; the new 71-entry option array retained the legacy 67-entry prefix. Original internal emulator game files stayed unchanged.

## Validation limits and phone checks

The private MQ availability flag exercised generation fixture metadata only; no real MQ archive or MQ gameplay was validated. The emulator has no audio output. No acoustic, physical-phone, complete-playthrough, actual silver/ring pickup or final-credits validation is claimed. Source-coupled host tests cover those implemented contracts, and native generation tests cover placement/reachability, but neither establishes every live gameplay path.

1. Listen to both cross-warp directions on phone speakers; try child/adult OoT, all six OoT warp songs from MM, and different MM forms. Check smooth pacing, correct landing and normal controls/audio afterward.
2. Repeat with fast playback and the relevant departure/cutscene skips. Selection and arrival must still work.
3. In a new silver-rupee/pouch seed, collect individual/foreign rewards, finish a puzzle, change rooms and save/restart. Check counters and opened passages persist; include a representative MQ puzzle if used.
4. Collect each MM ring after spending some keys. It must grant only never-found keys; duplicates must not refill spent keys. Check save/restart and Song of Time cycle reset.
5. Complete short seeds for the new goals and check actual victory/credits and goal-specific hints. Quest requires three different named pieces.
6. Check both-direction grants and persistence for the five shared Ocarina buttons, Song of Time and Mirror Shield.

The public source archive excludes ROMs, game-derived archives, saves, emulator state, private settings/spoilers/logs/recordings, credentials and signing keys. The remaining requested features are listed in FIXES-0.2.19.md and NEXT-PATCH-BACKLOG.md.
