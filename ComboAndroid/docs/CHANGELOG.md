# Combo Ship for Android: changelog

This is a short, player-facing summary of every Combo Ship Android build, newest first. OoT means Ocarina of Time and MM means Majora's Mask.

The detailed notes written for each release (changes, phone checklists, test evidence and logic audits) are in [history/](history/). See [README.md](README.md) for what each kind of file there contains. Notes for the 0.2.20 test builds and for 0.3.0 to 0.3.9 were delivered next to the APKs and are not in this repository; their sections below are summaries of those notes.

## How these builds were tested

Builds were checked by the developer with automated tests, and most also on an Android emulator. Unless a section says otherwise, a change was not tested on a physical phone, and no build has had a complete seed playthrough. The fixes confirmed on a phone so far are the three regressions reported against the first 0.2.20 test build and the MM menu hitch (gone in 0.3.7). "Tested" lines below repeat the limits stated in the original notes.

Install each update over the existing app, so your ROMs, saves and settings are kept. Do not uninstall first or clear app data. Existing saves keep the seed they were created with. New seed options only apply to newly generated seeds.

---

## Unreleased

- **New options: Iron Boots (MM), Hover Boots (MM), Goron Tunic (MM) and Zora Tunic (MM)**, under Randomizer > Items & checks > Majora's Mask > Items > Cross-Game Items, after Nayru's Love (MM), with OoTMM v32.3's names and descriptions. Each adds that OoT item to MM's item pool, and child Link wears it, as in OoTMM. All four are off by default and only affect newly generated seeds. If the app cannot prepare them on your device, the options are greyed out and new seeds leave them out. They are also greyed out while Swim is shuffled, because their OoTMM logic does not yet know about the Swim item. A seed made elsewhere that adds them is not loaded on a device that cannot run them, so no file loses items its seed relies on. They have not been played on a phone yet.
  - Put one on or take it off from a C button in human form. Transforming with a mask takes the boots and tunic off.
  - Iron Boots make Link heavy: he sinks in water, is not pushed by wind and can press heavy switches. With the Zora Tunic as well, he can walk on the bottom without drowning. Hover Boots let Link hover for a moment past a ledge and keep lily pads from sinking. The Goron Tunic delays damage from hot floors.
  - Wearing the Goron or Zora Tunic recolours Link's tunic, using your OoT Goron and Zora tunic colour settings. Your MM tunic colour setting still applies when neither is worn.
  - **Shared Iron Boots**, **Shared Hover Boots**, **Shared Goron Tunic** and **Shared Zora Tunic** (Randomizer > Shared items) make one copy count in both games. As in OoTMM, each stays greyed out and unticked until its MM option is on.
  - MM logic counts the boots and tunics where OoTMM does, for example in Great Bay Temple, Pirates' Fortress, Zora Cape, Stone Tower and Woodfall Temple. Seeds without them are unchanged.
  - New tricks with OoTMM's names, off by default, in Randomizer > Tricks & glitches: "Fewer Tunic Requirements (MM)", "Fight Gyorg as Human using Iron Boots and Hookshot", "Great Bay Temple Center Underwater Pot using only Iron Boots", "Stone Tower Updrafts without Deku Mask", and Hover Boots tricks for Woodfall, Woodfall Temple, the Soaring tablet, the Path to Snowhead, Great Bay Temple and Stone Tower Temple. "Stone Tower Updrafts without Deku Mask" also works on seeds without the boots.
  - The Snowhead Iron Boots trick now appears in the Tricks list under OoTMM's name, "Skip playing Goron Lullaby by using Iron Boots". A value you saved for it is kept.
  - The item tracker's Trade Items group shows the boots and tunics on seeds that add them.
- An MM save is no longer refused at load because of its Adult Link or boots-and-tunics data. Anything the app cannot run on your device is simply left off.

Tested:
- 75 automated host tests pass (3 are skipped on Windows). They check the four options, their shared items and the new tricks against OoTMM v32.3, every changed logic line against OoTMM's, that MM logic is unchanged with the boots and tunics off, that the options are off wherever the device cannot run them, that a save keeps its seed's choice, and that saves from earlier versions load. The boots and tunics have not been built into an APK, generated or played on the emulator, or tried on a phone yet.

## 0.4.1 - 2026-10-11

- **New options: Din's Fire (MM) and Nayru's Love (MM)**, under Randomizer > Items & checks > Majora's Mask > Items > Cross-Game Items, next to Farore's Wind (MM), with OoTMM v32.3's names and descriptions. Each adds the OoT spell to MM's item pool; it is cast from its trade slot like Farore's Wind. Both are off by default. If the app cannot prepare a spell on your device, its option is greyed out and new seeds leave the spell out, so no seed needs a spell you cannot cast.
  - **Shared Din's Fire** and **Shared Nayru's Love** (Randomizer > Shared items) make one spell count in both games. As in OoTMM, each stays greyed out and unticked until its MM option is on. Every shared item now shows OoTMM's description.
  - MM logic counts the spells where OoTMM does. Din's Fire works as short-range fire (Snowhead Temple, the Goron Village keg trial and crate, Ikana Castle, the Beneath the Well Mirror Shield, the Termina Field icicles and more), breaks large snowballs and can defeat the swamp's Big Octo. Nayru's Love lets you cross the poisoned swamp water with Swim, and opens OoTMM's routes to the Deku Shrine, Deku Palace, Woodfall and in Ikana Castle.
  - With Nayru's Love (MM) on, a seed uses OoTMM's own layout for Deku Palace, the Southern Swamp and Woodfall, so a few other requirements there follow OoTMM too: some swamp spots (the Deku Shrine, the Deku Palace bean route, the frog and the beehive among them) no longer accept wading as Goron when Swim is shuffled and not found, or as Fierce Deity with its logic rule on. OoTMM counts neither there.
  - Ikana Castle keeps its usual logic with Nayru's Love on and only gains OoTMM's Nayru's Love route.
  - New tricks with OoTMM's names, off by default: "Burn Igos' curtain with Din's Fire" and "Light the Zora Hall Stage Lights using Din's Fire, Bow, and Hookshot".
  - The item tracker's Trade Items group shows the MM spells, on seeds that add them. Its default Inventory group now shows a spell's own icon when the spell sits in a trade slot; until now Farore's Wind (MM) showed there as a lit Room Key.
- **MM logic follows OoTMM v32.3 in more places**, whatever the options:
  - The Oceanside Spider House library, the Swamp Spider House entrance and the door from the Woodfall Temple dark room to the maze need arrows or Deku Sticks, so plain arrows now count. Din's Fire does not count there, as in OoTMM.
  - The Snowhead Temple Pillars Room chest needs you to defeat its Freezard (and its soul, with enemy souls on); the pots there are free.
  - Fire Arrows also reach the Woodfall Temple main room's upper ledge from below, and open the Snowhead Temple icicle room chest.
  - Once Snowhead is cleared, its blizzard no longer blocks the way to the temple.
  - The logic behind the hidden Cross-Games Age and MM boots and tunic options also follows v32.3 now; current seeds cannot turn those on.

Tested:
- 74 automated host tests pass (2 are skipped on Windows). They check the MM spell settings, shared items, tricks and every spell logic site against OoTMM v32.3, including that Din's Fire counts nowhere OoTMM leaves it out. They compare, for every combination of items, what a Nayru's Love seed can reach in the swamp, Deku Palace and Woodfall against the previous logic, and check that Ikana Castle only gains OoTMM's Nayru's Love route. They also check the tracker icons and that the spell options are off wherever the device cannot cast them.
- Seed generation on the emulator with this release's native build, one seed each, every one generated on the first try and passed its playthrough check: defaults (OoT start and MM start); both MM spells (OoT start); both MM spells shared (MM start); both MM spells with a Minimal pool; the Allsanity preset. The spoilers have exactly one Din's Fire (MM) and one Nayru's Love (MM), or one shared copy of each when shared.
- Not tried on a phone: casting the MM spells has not been played yet.

## 0.4.0 - 2026-10-11

- **New option: Enemy Souls (OoT)**, under Items & checks > OoT > Item pool, copied from OoTMM v32.3 with its description. It adds a soul for each of the 43 kinds of OoT enemy to the item pool, and an enemy does not appear until you find its soul. A Plentiful item pool adds second copies of souls as far as there are free checks, the way SoH treats its other Plentiful extras (OoTMM always adds a second copy of each). It is off by default and not in any preset yet.
  - A room that is missing an enemy because of its soul stays barred, as in OoTMM. That includes Dark Link's room.
  - Logic now asks for the same enemy souls as OoTMM in parts of Dodongo's Cavern, Ganon's Castle, Gerudo Training Ground and the Master Quest dungeons. This only matters with the option on.
  - Souls use OoTMM's names: the item text reads "the Soul of ...", hints name the soul instead of "No Hint", the tracker group is "OoT Enemy Souls", and a soul found in MM looks like OoTMM's white token with a red flame.
  - The option is unavailable (unticked and greyed out, and off in new seeds) while Enhancements > Enemy Randomizer is on. OoTMM has no enemy randomizer to copy. If you turn Enemy Randomizer on during a souls save, souls stop keeping enemies away and rooms clear normally; turn it off again to play with souls.
  - Two Keese in the Master Quest Spirit Temple's Torch Slug room never appear in the original game either (their room does not load them). As in OoTMM, they do not count, so that room clears without the Keese soul.
- **Tracker fixes** from a dedicated review of both games' trackers:
  - MM: New Wave Bossa Nova can show as available (the tracker counted the seven Zora eggs as one). Saria's Song hint uses the same search as the tracker, so it no longer names a check you can't reach at the current time.
  - MM: a junk stand-in shown for a temporarily unobtainable item is no longer recorded as Seen. In co-op, a teammate's Seen now upgrades your Hinted record.
  - Both games follow "Play warp songs across games" when it is on only as a gameplay option.
  - Seed logic: an MM warp song that lands in OoT now counts only for the ages Link can actually be in MM.
  - Combo gossip stones and always-hints that name an exact OoT check now mark it Hinted (newly generated seeds).
  - With Shopsanity on, OoT refill stock is no longer listed in the check tracker, since it could never show Collected.
  - The OoT item tracker shows shuffled abilities and the Fishing Pole, and hides Ocarina buttons and Overworld keys when the seed doesn't shuffle them.
- **Removed**: the hidden "Complete OoT Soul Compatibility" switch is no longer a setting. NPC, animal and misc OoT souls stay hidden and off.
- **Renamed**: "Soul of Wolfos (OoT)" is now "Soul of Wolfoses (OoT)" in spoilers and the tracker, as in OoTMM.
- Existing saves play as before, with every OoT enemy present, even if their stored settings carry the old hidden soul switches. Seeds generated before this version also keep souls off when reloaded, whatever the new option says. The option has a new setting name, so an old value of the hidden switch in your settings or a profile does not turn it on.

Tested:
- 72 automated host tests pass (2 are skipped on Windows), including one that checks OoT enemy souls against OoTMM v32.3, broken actors, 0.3.10 saves and the Enemy Randomizer rules.
- Seed generation on the emulator with this release's native build, one seed each, every one generated on the first try and passed its playthrough check: defaults (OoT start and MM start); the Allsanity preset; Enemy Souls; Enemy Souls with Boss Souls, all 12 dungeons Master Quest and a Plentiful pool; Enemy Souls with a Minimal pool; cross-game song travel from an MM start; silver rupees anywhere. The Master Quest seed checks logic only: there is no Master Quest game data on the test emulator.
- Not tried on a phone: the room-clear behaviour, Dark Link's room and a soul found in MM still need a check in game.

## 0.3.11 - 2026-10-11

- Tingle's map menu now shows a bought slot as **SOLD OUT** (in red, no price) when its item cannot be given again, as OoTMM does. Refillable items stay for sale. Picking a sold-out slot plays Tingle's own reply.
- Check tracker: entering an area later in the cycle no longer puts its earlier-time checks in logic. For example, walking into Romani Ranch on Day 3 (after the boulder is gone) no longer shows Epona's Song, the aliens or Cremia's escort as available; those still need the boulder broken on Day 1.

Tested: automated host tests and APK checks; not tested on a phone.

## 0.3.10 - 2026-10-10

A cleanup release: no new features. An independent review of all code the earlier AI agent wrote confirmed 200 problems; 187 are fixed here, 10 are deferred and 3 minor cleanups were skipped (list in [history/CODE-REVIEW-2026-10-10.md](history/CODE-REVIEW-2026-10-10.md)).

- **Crashes fixed**:
  - entering the Moon with NPC souls shuffled and the Moon Children soul missing;
  - showing Farore's Wind to an MM character;
  - the Save Editor's inventory tab;
  - the settings search when the randomizer catalog had not loaded;
  - a possible crash after cross-game Soaring;
  - seed generation was reading freed memory on every seed.
- **Save safety**:
  - A new save no longer inherits silver rupees, named Triforce pieces or souls from the file loaded before it.
  - Multiplayer (Anchor) resyncs no longer wipe received silver rupees, OoT soul ownership or MM Farore's Wind and spell data.
  - A crash at startup no longer turns into an endless relaunch loop.
- **Silver rupees**:
  - Each silver rupee check now gives its item once instead of twice.
  - Silver rupees in midair or on fences can be collected.
  - Gossip stones no longer say "No Hint" for them.
- **Randomizer menu**:
  - The OoT Mask Quest dropdown now saves the value you pick.
  - Generate and seed import at the file select now apply to the file you are on, not always File 1.
  - Editing the next seed's settings no longer changes the rules of the run you are playing.
  - Apply preset and turning a logic rule off leave your own enhancements alone.
  - Clock Shuffle now checks and greys out Better Song of Double Time.
  - Search covers MM randomizer options.
  - Sliders no longer stutter.
- **Logic now matches OoTMM in more places**:
  - Path hints only name bosses the goal needs.
  - MM beans from OoT shops, cows and scrubs are not treated as renewable.
  - Shuffle Grab/Climb requirements are restored.
  - Ranch alien defense, Tingle's maps and the Romani Ranch cows need their souls when those souls are shuffled.
  - Foreign warp songs work with the MM Fairy Ocarina.
  - Getting the Fairy Ocarina no longer replaces the Ocarina of Time.
- **Unfinished, hidden options** (MM entrance shuffle, adult Link and gear in MM, MM Din's Fire and Nayru's Love, hidden MM tricks) can no longer be switched on through profile or seed files.
- **Cross-game travel**: an MM Farore's Wind placed in OoT shows the spell's orb instead of a blue rupee. Link, the camera and the HUD no longer reappear during an OoT-bound owl departure.
- **Android**:
  - The game survives screen and window changes.
  - The App options shortcut warns before closing a running game.
  - Storage location choices and leftover copies work as described.
  - Touch size and opacity cycles return to the defaults.
  - N64 Mode works with automatic screen fit.
- **First launch after updating** takes a few seconds longer while the app refreshes its support files. Saves are not touched.

Tested:
- 68 automated host tests pass and the APK checks pass.
- Seed generation on the emulator: see the release report.
- Not tested on a phone.
- Still open, needs the owner's decision: a save made before per-save rules existed (0.1.18) still falls back to default enhancement values on load.

## 0.3.9 - 2026-10-09

- Adds OoTMM's three Farore's Wind settings, all off by default: **Farore's Wind (MM)**, **Shared Farore's Wind** and **Cross-Games Farore's Wind**.
- Farore's Wind (MM) is under Randomizer > Items & checks > Majora's Mask > Items > Cross-Game Items. The other two are greyed out unless it is on.
- Cross-Games Farore's Wind (Randomizer > Logic & access > World access) lets you warp between OoT and MM with the spell.
- MM logic uses Farore's Wind the way OoTMM does, for example for Snowhead hot-water routes and dungeon time resets.
- Known issue: right after loading a save in MM, the camera can sit very close behind Link until you move. The notes say this is not caused by Farore's Wind.

Tested: build and APK checks only. Setting and returning to a point inside MM was tested on the emulator earlier. Seed generation with these options and cross-game returns were not tested before release.

## 0.3.8 - 2026-10-09

- Adds three MM soul options under Randomizer > Items & checks > Majora's Mask: **NPC Souls** (53 characters, including shopkeepers and quest characters), **Animal Souls** (butterflies, cows, cuccos, dogs) and **Misc. Souls** (business scrubs and Gold Skulltulas).
- With a soul group on, those characters or animals stay absent until you find their soul. Generation and the tracker use the same rule.
- All 59 souls can be chosen as starting items. A starting soul is removed from the randomized pool.
- These are MM-only options. Shared souls and the separate enemy-soul expansion are not included.

Tested: automated comparisons against OoTMM's rules and generated seeds; no in-game playthrough.

## 0.3.7 - 2026-10-09

- The menu reuses its page layout instead of rebuilding it every frame, to reduce hitches. Setting values still update live.
- Empty "Saved seed" and "Generated seed" views now say the file has no settings, instead of showing defaults as if they were saved.
- File-profile tabs explain their purpose. **Copy settings** is clearer and is disabled when the current view cannot be copied.
- Silver rupee pouch choices sit under a collapsible heading and are disabled when silver rupees are not shuffled. Your stored choices are kept.
- Tools: Save Editor is near the top; developer viewers are grouped under **Developer tools**.
- Checks and Items sub-tabs; the inner page scrolls back to the top when you open a search result.

Tested: automated checks before release; the user later confirmed on the phone that the MM menu hitch is gone.

## 0.3.6 - 2026-10-09

- Reopening the menu no longer re-reads the whole settings catalog and seed each time. This targets the hitch and music interruption when opening the menu in MM.
- All shuffled OoT shop slots, including refill stock, now appear in the check tracker. With Shopsanity off they stay hidden.
- Shop stock was already part of generation; this only fixes what the tracker shows.

Tested: automated regression checks; phone performance not tested.

## 0.3.5 - 2026-10-09

- MM shop goods you can see on the shelf are marked **Seen** without talking to the shopkeeper.
- Looking at or selecting displayed stock records it as Seen; remote stock announcements still count as Hinted.
- Fixed a check-ID overlap that falsely marked the Curiosity Shop special item as Seen when you picked up an Ocean Gold Skulltula token. A false record saved by an older build may remain.
- The menu no longer reloads and parses the full settings catalog on every idle frame.

Tested: automated tests of the actual handlers; gameplay and audio performance on a phone not tested.

## 0.3.4 - 2026-10-09

- Starting-song pickers for both games follow the in-game quest menu: colored songs on the first row, white songs on the second.
- MM uses its native colors, including the Goron Lullaby tier picker. Optional Saria's Song and Sun's Song use their native slots; Double Time and Inverted Time are on an extra row.
- Display only: availability, starting inventory and logic are unchanged.

Tested: compared against both native quest screens; not checked on a phone.

## 0.3.3 - 2026-10-09

- Turning on a rule in Randomizer now also turns on its matching Enhancements control and greys it out. Turn the rule off to use the enhancement on its own, without a logic assumption.
- The **Include in logic** checkboxes are removed from Enhancements. Logic choices are made only in Randomizer.
- Paired settings follow the selected file. Editing another file's draft does not change the run you are playing or an existing seed's logic.
- Shorter Ballad of the Wind Fish is now only an enhancement (Enhancements > Time savers) and no longer gives the generator a logic shortcut.
- Search results lead to the same destinations for both sides of a paired setting.

Tested: automated profile and navigation checks; the look of disabled controls was not checked on a phone.

## 0.3.2 - 2026-10-09

- Fixed a menu crash when choosing **Single Bag** or **Progressive Bags** for Bombchus.
- Fairy Spot Shuffle uses OoTMM's exact description, including the Song of Storms / Sun's Song guidance.
- Menu moves: OoT Mask Quest and Item Pool to Items & checks > Extra items; Starting Age and Full Wallets to Starting inventory; quest skips, Big Poe count and equipment rules to Gameplay rules. Search follows the new locations.

Tested: the crash was reproduced and fixed under a memory checker; no phone gameplay test.

## 0.3.1 - 2026-10-09

- Hides internal `##` identifiers that were showing in randomizer setting captions.
- Shared Reachability, Starting world and Victory condition controls now sit above the OoT/MM tabs; the tabs only apply to the settings below them.
- About shows the same version as the installed app.

Tested: build checks only; the new menu layout was not viewed on a phone.

## 0.3.0 - 2026-10-09

First numbered release after the 0.2.20 test builds below, and builds on them.

- New OoT seeds use OoTMM's mask model: masks are shuffled, or you start with them (**Completed**). The Happy Mask Shop stays the MM portal.
- The Deku Theater Skull Mask and Mask of Truth checks need the actual OoT mask.
- Older saves made with the "Vanilla" mask mode get the missing starting masks on load. Their placements are unchanged.
- MM one-time rewards stay collected across a three-day reset. Collecting a foreign reward or progressive ocarina again gives junk instead of a duplicate.
- MM logic keeps shuffled renewable shop stock after each logic reset; critical potion and Bombchu refills get a renewable source.

Tested: focused automated tests; no full playthrough or full generation stress run, and not tested on a phone.

## 0.2.20 test builds - 2026-10-08

These were test candidates, not releases. Some were uploaded for testing on the user's phone. All were replaced by 0.3.0.

**0.2.20 TEST** (first candidate)

- MM-to-OoT warp: Link's model stays hidden until the handoff, removing the final hitch and model flash.
- Fire Temple: a seated push block keeps its position instead of resetting.
- The tracker shows silver rupee checks only when silver rupees are shuffled in the seed.
- Logic corrections, including MM shop identity and coordinated adult-start Master Sword rules. Existing save ownership and collected checks are not cleared.
- Phone result: the user installed this build and reported that the three issues above (warp hitch/model flash, Fire Temple block, silver rupee checks in an unshuffled seed) were fixed. Cutscene skips, sound timing and new item/cycle behavior were still unconfirmed.

**TEST12 and TEST13** have no release notes.

**0.2.20 TEST14**

- Per-file settings selector: view a file's saved seed settings separately from the next-seed settings, and copy settings into another file's next-seed draft.
- Silver rupee tracker visibility follows the saved seed, including Own Dungeon mode.
- Ganon's Castle availability uses the seed's required trials and OoTMM's Lens and trick rules.
- MM gets the green targetable Hookshot dot, shared with OoT: Video & Audio > Camera > Majora's Mask > First Person > Targetable Hookshot Reticle.
- Farore's, Din's and Nayru's expansion options stay unavailable in this build.
- Tested: automated checks and emulator only. A full logic comparison was stopped partway at the user's request and is not a complete pass.

**0.2.20 TEST15**

- Happy Mask Shop logic fix: new seeds offer **Shuffle** (masks in the item pool) or **Completed** (start with the masks). Old "Vanilla" requests become Shuffle.
- Deku Theater checks require owning the Skull Mask or Mask of Truth.
- Known issue at the time: three generation test cases reached every OoT check, but all-checks validation rejected several MM checks (for example Tourist Information Archery and Road to Ikana Stone Mask). Full-seed certification was not complete.

## 0.2.19 - 2026-10-07

- New seed option: OoT silver rupees and puzzle pouches, placed Vanilla, Own Dungeon or Anywhere, for normal and Master Quest dungeons.
- New seed option: MM key rings for Woodfall, Snowhead, Great Bay and Stone Tower. A ring gives only the keys you have not already found.
- More victory conditions: both final bosses, either final boss, Ganon only, Majora only, Triforce Hunt and Triforce Quest (Triforce Quest needs distinct Power, Courage and Wisdom pieces).
- Seven more shared item families: the five Ocarina buttons, Song of Time and the non-progressive Mirror Shield. Button sharing needs button shuffle in both games.
- Older MM saves stay compatible. Cross-game warps keep the 0.2.18 presentation.
- Not included: full MM and cross-game entrance shuffle, expanded one-way pools, cross-game Farore's Wind, Adult Link in MM, expanded souls.

Tested: emulator without audio. Master Quest was only checked as generation data, not with real MQ gameplay. Sound, pacing and new pickups were not tested on a phone.

Details: [FIXES-0.2.19](history/FIXES-0.2.19.md), [VALIDATION-0.2.19](history/VALIDATION-0.2.19.md)

## 0.2.18 - 2026-10-07

- Cross-game warp songs use the originating game's departure animation and audio. Soaring from OoT uses MM's wings, feathers, wind, camera and Soaring music; an OoT warp song in MM uses OoT's sparkles, flash and fanfare.
- Music and samples come from your own extracted game data.
- Fast Ocarina Playback and Skip Soaring Cutscene now apply to cross-game Soaring. Skipping keeps the chosen destination and the destination game's arrival.
- Fixed an MM arrival that could land you at a remembered pause-save point instead of the chosen destination.

Tested: emulator with audio disabled. Sound quality and pacing on a phone were left as user checks.

Details: [FIXES-0.2.18](history/FIXES-0.2.18.md), [VALIDATION-0.2.18](history/VALIDATION-0.2.18.md)

## 0.2.17 - 2026-10-05

- Fixed the Gerudo Training Ground silver-gauntlet block falling into its pit without being recorded. On reload it reappeared in the corridor and could trap Link.
- The fix matches Shipwright's upstream floating-point change.
- A push that an older build failed to record is not restored. Push the block again after updating, then save.

Tested: emulator reproduction and fix; not on a phone.

Details: [FIXES-0.2.17](history/FIXES-0.2.17.md), [VALIDATION-0.2.17](history/VALIDATION-0.2.17.md)

## 0.2.16 - 2026-10-05

- Fixed OoT forgetting the names of seen items after returning from MM or using Return to Spawn. This affected shop items, Skulltula rewards, freestanding items and MM rewards seen in OoT.
- Names already lost under an older build cannot be rebuilt. Look at the item again to record it.

Tested: emulator only; no user phone save was inspected.

Details: [FIXES-0.2.16](history/FIXES-0.2.16.md), [VALIDATION-0.2.16](history/VALIDATION-0.2.16.md)

## 0.2.15 - 2026-10-05

- Hotfix: older MM saves failed to load after 0.2.14. MM stayed unloaded, the save editor showed empty progress, and OoT did not see that you owned Song of Soaring.
- Older saves now load with inventory, songs, owl statues, collected checks and settings intact.
- The MM save editor now refuses to edit when no valid MM save is loaded.
- Copy your ComboShip/Save folder before updating.

Tested: an older save on the emulator. Recovery of any particular phone save was not confirmed.

Details: [FIXES-0.2.15](history/FIXES-0.2.15.md), [VALIDATION-0.2.15](history/VALIDATION-0.2.15.md)

## 0.2.14 - 2026-10-05

- Logic corrections for both games, using OoTMM v32.3 as the reference, in seed generation and the check tracker.
- Adds 49 MM combat requirements from OoTMM.
- MM fixes include Moon trials, Song of Time returns, Ghost Hut, owl activation with sticks, and Snowhead, Great Bay and Stone Tower routes.
- Fixed OoT trick settings being lost during seed finalization; corrected Master Quest and normal dungeon requirements and several OoT and MM scenery checks.
- Existing seed placements are kept. Menus and warp visuals are unchanged.

Tested: headless and emulator tests only.

Details: [FIXES-0.2.14](history/FIXES-0.2.14.md), [VALIDATION-0.2.14](history/VALIDATION-0.2.14.md), [LOGIC-PARITY-0.2.14](history/LOGIC-PARITY-0.2.14.md)

## 0.2.13 - 2026-10-04

- MM logic tracks renewable money, arrows, bombs, magic, sticks and nuts. Owning equipment alone no longer supplies refills.
- MM shops supply refills only when they are reachable, open and affordable.
- MM Bombchus need a bomb bag and a reachable Bombchu refill.
- The OoT tracker no longer opens routes just because you currently hold ammo or magic; it uses the same renewable-source rules as generation (this replaces the 0.2.11 behavior).
- Fixed Honey & Darling's day-three reward to allow Deku bubbles.

Tested: headless tests on an emulator.

Details: [FIXES-0.2.13](history/FIXES-0.2.13.md), [VALIDATION-0.2.13](history/VALIDATION-0.2.13.md), [RESOURCE-PARITY-0.2.13](history/RESOURCE-PARITY-0.2.13.md)

## 0.2.12 - 2026-10-04

- Paid checks depend on wallet size and a reachable source of rupees, not your current balance.
- OoT paid activities (galleries, fishing, Dampe, Talon and others) need a reachable money source.
- MM routes respect time of day: a room you opened by day no longer counts as open at midnight after it closes.
- Finding a shuffled clock updates which areas are reachable.

Tested: headless tests on an emulator.

Details: [FIXES-0.2.12](history/FIXES-0.2.12.md), [VALIDATION-0.2.12](history/VALIDATION-0.2.12.md), [SUPPORTED-LOGIC-0.2.12](history/SUPPORTED-LOGIC-0.2.12.md)

## 0.2.11 - 2026-10-04

- OoT logic tracks renewable seeds, arrows, bombs, Bombchu drops and magic. Owning a weapon no longer counts as having its ammo.
- Paid OoT checks need a reachable source of rupees for generation, not just a big enough wallet.
- The live tracker could use ammo, magic and rupees you currently held (changed again in 0.2.13).
- Bowling keeps its 30-rupee entry fee.

Tested: headless route and generation tests; the emulator check only confirmed the app starts.

Details: [FIXES-0.2.11](history/FIXES-0.2.11.md), [VALIDATION-0.2.11](history/VALIDATION-0.2.11.md)

## 0.2.10 - 2026-10-04

- Song of Time age change is allowed in 166 named regions instead of 76, adding dry Spirit Temple, Forest Temple and Gerudo Training Ground rooms.
- Fixes Spirit Temple routes that need child crawling and adult Hookshot.
- Both trackers use your active enhancements; generation still uses the seed's own rules.
- MM Scarecrow routes need a reachable teacher or a song already taught this cycle. New option: **Scarecrow without teaching a song**.
- Fixed OoT price estimates and MM wallet limits (99, 200, 500, and 5,000 with the Tycoon Wallet).

Tested: headless route and generation tests; the emulator check only confirmed the app starts.

Details: [FIXES-0.2.10](history/FIXES-0.2.10.md), [VALIDATION-0.2.10](history/VALIDATION-0.2.10.md), [LOGIC-COMPARISON-2026-10-04](history/LOGIC-COMPARISON-2026-10-04.md)

## 0.2.9 - 2026-10-04

- The OoT tracker can plan age-change routes with the active Song of Time enhancement, even if the seed was generated without that rule.
- Checks behind a known shuffled entrance show as reachable without entering the dungeon or changing age first.
- Availability refreshes as soon as the age-change option changes.

Tested: focused emulator regression checks; not on a phone.

Details: [FIXES-0.2.9](history/FIXES-0.2.9.md), [VALIDATION-0.2.9](history/VALIDATION-0.2.9.md)

## 0.2.8 - 2026-10-04

- Check availability refreshes when you enter an area or change age, including reloading the same area.
- Logic allows Song of Time age change on the Water Temple's dry entrance ledge, so a child-only shuffled entrance can still lead to adult routes.

Tested: emulator checks with private test files.

Details: [FIXES-0.2.8](history/FIXES-0.2.8.md), [VALIDATION-0.2.8](history/VALIDATION-0.2.8.md)

## 0.2.7 - 2026-10-03

- Cross-game departures use the original games' animation code and timing.
- MM-to-OoT particles now update every frame, fixing the choppy effect.
- The owl map uses MM's exact positions and timing, and no longer freezes a white overlay behind it.
- Assets load from your extracted games. OoT's Soaring sound is still a fallback.

Tested: emulator; not pixel-identical to the originals and not checked for phone frame pacing.

Details: [FIXES-0.2.7](history/FIXES-0.2.7.md), [VALIDATION-0.2.7](history/VALIDATION-0.2.7.md)

## 0.2.6 - 2026-10-03

- The OoT tracker records the names of visible shop items, including refills, as Seen. Reading an offer also records it.
- Quantities are kept, for example "Buy Arrows (30)".
- Old records without a name are no longer shown as known rewards.

Tested: focused emulator checks.

Details: [FIXES-0.2.6](history/FIXES-0.2.6.md), [VALIDATION-0.2.6](history/VALIDATION-0.2.6.md)

## 0.2.5 - 2026-10-03

- The OoT tracker records the reward shown by a killed Gold Skulltula as Seen before you collect it.
- Off-screen, hidden and Mysterious Shuffle rewards stay unknown.

Tested: focused emulator checks.

Details: [FIXES-0.2.5](history/FIXES-0.2.5.md), [VALIDATION-0.2.5](history/VALIDATION-0.2.5.md)

## 0.2.4 - 2026-10-03

- OoT to MM: Song of Soaring plays, then MM's owl map opens. Pick an activated statue, confirm or cancel. Child and adult Link can both soar.
- MM to OoT: OoT warp songs show the native song-colored confirmation, departure effects and warp-pad arrival.
- MM arrivals use the statue's arrival sequence; Swamp and Mountain Village respect cleared temples.
- Soaring with no activated owls shows an in-game message.
- Sound uses the current game's sound bank only.

Tested: emulator.

Details: [FIXES-0.2.4](history/FIXES-0.2.4.md), [VALIDATION-0.2.4](history/VALIDATION-0.2.4.md)

## 0.2.3 - 2026-10-03

- Check, Item and Hint trackers share access, appearance and panel controls in both games.
- New default for fresh settings is **While paused**; your saved L toggle is kept.
- New **On-screen button** mode adds CT, IT and HT touch buttons. Move them with Hold Menu > Move controls and size them under Tracker button sizes.
- Trackers share one side panel as tabs. The Item tracker uses compact icon grids; the Hint tracker hides unrevealed hint text by default.
- Shortcut help explains the default mapping (L/L1 to L, ZL/L2 to Z, ZR/R2 to R) and the Modifier 1/2 inputs.

Tested: emulator at two screen sizes; controllers and phone touch not tested.

Details: [FIXES-0.2.3](history/FIXES-0.2.3.md), [VALIDATION-0.2.3](history/VALIDATION-0.2.3.md), [RELEASE-AUDIT-0.2.3](history/RELEASE-AUDIT-0.2.3.md)

## 0.2.2 - 2026-10-03

- MM starting inventory uses OoT-style icon groups plus a Masks group, with faded unselected icons and progressive tier pickers. The whole page scrolls together.
- Gibdo trades, frog choir count, dog race results and Zora egg count move to Randomizer > Gameplay rules. The separate Randomizer Difficulty page is gone.
- The Bremen chick count is only under Enhancements > Difficulty > Majora's Mask (1 to 10). It changes quest length and is not a logic setting.
- OoT minigame settings move to Enhancements > Difficulty. Both games' camera settings are under Video & Audio > Camera.

Tested: emulator.

Details: [FIXES-0.2.2](history/FIXES-0.2.2.md), [VALIDATION-0.2.2](history/VALIDATION-0.2.2.md), [RELEASE-AUDIT-0.2.2](history/RELEASE-AUDIT-0.2.2.md)

## 0.2.1 - 2026-10-03

- Setting descriptions are tooltips (hover, focus or touch). The short Guide is available on request instead of popping up.
- Randomizer holds seed choices; gameplay-only, cosmetic and tracker display settings move to their matching pages.
- One cross-game song setting covers Child and Adult Link.
- Home offers Default, Beginner and Allsanity presets adapted from OoTMM. They reset seed choices but keep audio, screen and control preferences.
- New Shared items page and Video & Audio tab. The Everyday page is removed; its settings moved to Controls, Items & equipment and Saving.
- Bremen Mask chick count is a 1 to 10 slider.
- Fixed endless generation retries after switching from a larger preset.

Tested: emulator.

Details: [FIXES-0.2.1](history/FIXES-0.2.1.md), [VALIDATION-0.2.1](history/VALIDATION-0.2.1.md), [RELEASE-AUDIT-0.2.1](history/RELEASE-AUDIT-0.2.1.md)

## 0.2.0 - 2026-10-02

- Search accepts several words in any order and familiar names (cross-game travel, adult warp, Requiem, owl statues, HUD). Results are ranked, grouped and open the matching control.
- **Songs & travel** groups cross-game playback, mixed destinations and any-age OoT Soaring.
- First-open guides for Randomizer and Enhancements (made on-request in 0.2.1).
- Same-game song logic uses the seed's saved destinations, independent of gameplay-only toggles.
- Long rule labels wrap and stay tappable.

Tested: emulator; the notes record no physical-phone test or full playthrough.

Details: [FIXES-0.2.0](history/FIXES-0.2.0.md), [VALIDATION-0.2.0](history/VALIDATION-0.2.0.md), [RELEASE-AUDIT-0.2.0](history/RELEASE-AUDIT-0.2.0.md)

## 0.1.23 - 2026-10-02

- New **Song of Soaring at any age** option, with its own logic choice.
- Zora egg count affects Bossa Nova logic; new logic option for daytime Skulltula spawns.
- Song of Time age-change logic covers audited houses, shops, the laboratory and Lon Lon Ranch.
- Early Eyeball Frog has its own Tricks & glitches entry.
- Eight more optional MM techniques (ten in total).
- More search aliases; About shows the actual app version.

Tested: emulator and automated tests.

Details: [FIXES-0.1.23](history/FIXES-0.1.23.md), [VALIDATION-0.1.23](history/VALIDATION-0.1.23.md), [LOGIC-AUDIT-0.1.23](history/LOGIC-AUDIT-0.1.23.md), [MM-TRICKS-0123-REFERENCE](history/MM-TRICKS-0123-REFERENCE.md)

## 0.1.22 - 2026-10-02

- OoT save editor shows all 24 inventory slots; empty slots are faded but clickable.
- MM save editor has named Owl Statue checkboxes with Activate all and Clear all.
- New **Shadow Door Unload** restoration: five full trips over the Graveyard stair loading boundary remove the Shadow Temple door.
- Logic only expects the Shadow door trick if you select it under Tricks & glitches.

Tested: emulator.

Details: [FIXES-0.1.22](history/FIXES-0.1.22.md), [VALIDATION-0.1.22](history/VALIDATION-0.1.22.md)

## 0.1.21 - 2026-10-02

- One Gameplay rules page under both Randomizer and Enhancements. Randomizer turns on the ability and its logic; Enhancements can turn on the ability for play only.
- 18 audited rules across both games, for example Blue Fire and Sunlight Arrows, Gibdo trade modes, Goron rolling without magic and cross-game warp songs.
- New seed guarantees for both games: **Every check reachable** or **Only completion guaranteed**. No Logic removes both.
- Tricks & glitches is one searchable catalogue (197 OoT and 2 MM techniques).
- Cross-game songs play the full song performance. The error "bonk" sound in MM is removed, and OoT Soaring with no owls plays the song and then explains there are no destinations.
- Boss and event path hints. The tracker shows Hinted, Seen, Known and Collected separately.
- One HUD editor for both games. Sliders replace number entry in the save editors and most numeric settings.

Tested: host tests, native generation checks and emulator; no phone.

Details: [FIXES-0.1.21](history/FIXES-0.1.21.md), [VALIDATION-0.1.21](history/VALIDATION-0.1.21.md), [ENHANCEMENT-LOGIC-AUDIT-0.1.21](history/ENHANCEMENT-LOGIC-AUDIT-0.1.21.md), [UNIFORM-EDITORS-0.1.21](history/UNIFORM-EDITORS-0.1.21.md)

## 0.1.20 - 2026-09-30

- The cross-game song prompt uses each game's own text box instead of a grey overlay window.
- Confirm with A, cancel with B. For OoT Soaring, move left or right to choose a destination.
- The last song note must be released before the prompt opens, so it cannot confirm the warp by accident.

Tested: emulator.

Details: [FIXES-0.1.20](history/FIXES-0.1.20.md), [VALIDATION](history/VALIDATION.md)

## 0.1.19 - 2026-09-30

- New seed option **Play warp songs across games**. Song knowledge is shared; the instrument and buttons come from the game you are playing. For example, Requiem played in MM can take you to Desert Colossus.
- Child Link can play Song of Soaring in OoT to reach activated MM owl statues.
- Optional destination shuffle mixing OoT's six warp pads and MM's ten owl statues.
- Generation, playthrough checks and the tracker consider these routes. The route map is saved with the seed.
- Generation retries automatically until it succeeds or you cancel, with an attempt counter and a reminder after five minutes.

Tested: emulator and host tests.

Details: [FIXES-0.1.19](history/FIXES-0.1.19.md), [VALIDATION](history/VALIDATION.md), [OOTMM-LOGIC-AUDIT](history/OOTMM-LOGIC-AUDIT.md)

## 0.1.18 - 2026-09-30

- Shared Gameplay rules page: Song of Time age change, Fierce Deity Anywhere and a one-chick Bremen Mask quest.
- These rules are saved with new seeds and each run. Changing them mid-run is allowed with a short warning.
- Logic: Adult Link can go down the dry well shaft and change to Child with Song of Time to reach Bottom of the Well without Song of Storms. Fierce Deity's sword only counts when Anywhere is on.
- Settings > Screen & HUD: automatic Balanced fitting, Full width, Original 4:3 or Manual, respecting screen cutouts. Both games' HUD editors are reachable from one page.
- Sliders for OoT max health, rupees, Gold Skulltula tokens and Triforce hunt counts.
- The MM tracker separates Seen (you saw it) from Hinted (dialogue told you); older records show Known.

Tested: host tests and emulator.

Details: [FIXES-0.1.18](history/FIXES-0.1.18.md), [VALIDATION](history/VALIDATION.md), [NEXT-PATCH-BACKLOG](history/NEXT-PATCH-BACKLOG.md)

## 0.1.17 - 2026-09-29

- MM items hidden behind walls (for example in the Deku Palace maze) no longer become Seen.
- The check now considers the camera view, hidden models, transitions, pause screens and solid collision between the camera and the item.
- False Seen records saved by older builds are not erased.

Tested: host tests and emulator.

Details: [FIXES-0.1.17](history/FIXES-0.1.17.md), [VALIDATION](history/VALIDATION.md)

## 0.1.16 - 2026-09-27

- Lower audio delay on Android: a smaller shared audio queue (target 48 ms instead of 128 ms) and conversion to the phone's output rate.
- Changing audio settings during play can no longer free the audio converter while it is in use.
- Game data moves to a visible `Internal storage/ComboShip` folder. Older data is copied and verified first, and the original is kept.

Tested: host audio tests and emulator logs. Actual delay on a phone was not measured.

Details: [FIXES-0.1.16](history/FIXES-0.1.16.md), [VALIDATION](history/VALIDATION.md)

## 0.1.15 - 2026-09-26

- The MM check tracker remembers rewards you have seen (item models, shop items) or that dialogue names, shown as "Seen: item".
- Seen checks stay uncollected and do not raise the collected count. Traps keep their disguise.
- Hidden rewards, vague hints and off-camera items do not reveal anything.
- Stored in the normal MM save and shared through multiplayer sync. Items seen before this update need to be looked at again.

Tested: emulator.

Details: [FIXES-0.1.15](history/FIXES-0.1.15.md), [VALIDATION](history/VALIDATION.md)

## 0.1.14 - 2026-09-26

- Multiplayer teammates receive both OoT and MM progress in one sync. Rejoining recovers both games' progress.
- Repeated or cached messages no longer give extra items.
- Progress from a different combined seed is rejected.
- Every player needs this build; older Combo Ship builds and standalone Ship/2S2H clients are not compatible.
- This covers shared-progress co-op. MM PvP is still incomplete upstream.

Tested: two Android emulator clients against a local server; not over the Internet or on phones.

Details: [FIXES-0.1.14](history/FIXES-0.1.14.md), [VALIDATION](history/VALIDATION.md)

## 0.1.13 - 2026-09-26

- Randomizer > Items & checks combines the old Shuffles, Check Pool and Item Pool pages, with Locations and Extra items for both games.
- MM dungeon items move to Dungeons; MM traps move to Hints & traps.
- OoT and MM buttons pick the world; pages follow a fixed order from Generate to Plandomizer.
- Search understands old names such as "item pool" and "starting items".

Tested: emulator.

Details: [FIXES-0.1.13](history/FIXES-0.1.13.md), [VALIDATION](history/VALIDATION.md)

## 0.1.12 - 2026-09-26

- New menu layout: Home, Gameplay, Randomizer, Trackers, Settings and Tools, grouped by feature. "Options for" chooses which game a page applies to.
- Gameplay > Everyday has five controls shared by both games (removed again in 0.2.1).
- Settings > Interface sets theme, opacity, scale and controller navigation. Both games' menus use the same font.
- Randomizer > Generate opens straight to the seed field and Generate button.
- Tools > Session holds restart and exit, with confirmation.

Tested: emulator.

Details: [FIXES-0.1.12](history/FIXES-0.1.12.md), [VALIDATION](history/VALIDATION.md)

## 0.1.11 - 2026-09-25

- New compact side-panel tracker in the style of 2S2H: collapsible areas with counts, small icons and one line per check. The game resizes beside it.
- Drag the divider to change its width; the width is saved.
- Colors: available white, unavailable gray, collected green; skipped checks get a purple label.
- Search and filters are hidden by default; **Only show current area** is available.

Tested: emulator.

Details: [FIXES-0.1.11](history/FIXES-0.1.11.md), [VALIDATION](history/VALIDATION.md)

## 0.1.10 - 2026-09-25

- One shared check tracker and settings page for both games.
- Modes: Always, While paused, Button toggle and Button hold. The default shortcut is L.
- Tap a check for its status, known reward and skip/unskip. Skipped checks are counted separately.
- Search and filters work the same in both games. Touches on the tracker no longer move the invisible sticks.

Tested: emulator and 58 automated touch checks.

Details: [FIXES-0.1.10](history/FIXES-0.1.10.md), [VALIDATION](history/VALIDATION.md)

## 0.1.9 - 2026-09-25

- The Menu button can be moved: pause, hold Menu > Move controls, drag it, then tap Done.
- Its position is saved per screen shape and button layout. Reset current screen layout puts it back at the top center.

Tested: emulator and automated touch checks.

Details: [FIXES-0.1.9](history/FIXES-0.1.9.md)

## 0.1.8 - 2026-09-25

- Both control sticks are invisible by default. Drag from an empty spot on the left half to move, or the right half for the right stick.
- Each stick starts where your finger lands. Both work together with other buttons, and buttons take priority.
- Hold Menu > Control sticks to choose invisible sticks, visible fixed sticks or a visible floating left stick.

Tested: emulator and automated touch checks; how it feels on a phone was not tested.

Details: [FIXES-0.1.8](history/FIXES-0.1.8.md)

## 0.1.7 - 2026-09-25

- The app is named **Combo Ship** and has a new blue ocarina icon.

Tested: emulator.

Details: [FIXES-0.1.7](history/FIXES-0.1.7.md)

## 0.1.6 - 2026-09-25

- Starting a new randomizer file goes straight to name entry without the "Take a look at our presets!" pop-up. Presets are still in the menu.

Tested: emulator.

Details: [FIXES-0.1.6](history/FIXES-0.1.6.md)

## 0.1.5 - 2026-09-25

- Removed the ComboShip Settings Import screen at startup. Its desktop file picker did not work on Android. Importing another Ship app's settings is not supported.

Tested: emulator startup only.

Details: [FIXES-0.1.5](history/FIXES-0.1.5.md)

## 0.1.4 - 2026-09-25

- After setup, opening the app goes straight into the game, with a single card in Recents.
- Save, then hold Menu > App options (or long-press the app icon) to reach ROM, backup and recovery tools. Fixed a crash when returning there.
- Seed generation shows progress for both games and ends with "Seed ready for OoT + MM". A failed run no longer reports an old success.
- Generation is blocked during name entry, which fixes a freeze.
- The Seed field shows a saved custom seed. Leave it blank for a new random seed.

Tested: emulator only.

Details: [FIXES-0.1.4](history/FIXES-0.1.4.md)

## 0.1.3 - 2026-09-25

- Simpler first run: choose your OoT ROM, then your MM ROM, and the game starts.
- Existing installations show a Ready to play screen with Play and App options.
- Mods, backups, storage, repair and ROM management move to App options.

Tested: emulator, not a phone.

Details: [FIXES-0.1.3](history/FIXES-0.1.3.md)

## 0.1.2 - 2026-09-25

- Fixed a likely cause of garbled text in name entry, saved names and dialogue on ARM64 phones (tagged memory pointers).
- New empty files open randomizer setup directly, without the one-option quest screen.

Tested: a synthetic pointer test and the emulator. The emulator cannot reproduce the phone's behavior, so a phone retest was still needed.

Details: [FIXES-0.1.2](history/FIXES-0.1.2.md)

## 0.1.1 - 2026-09-25

- ROM extraction works on Android (it previously failed because of its temporary folder).
- The Menu button no longer covers the native menu tabs; Android Back opens the menu; the keyboard keeps existing text.
- New linked MM saves are valid, so MM pause and owl saves are no longer discarded.
- OoT player names carry over into MM correctly.
- Much faster seed generation (diagnostic timers no longer run when disabled).
- Backups exclude ZIP files and the bundled controller database.

Tested: Android 15 emulator, including extraction, generation, portals in both directions and one OoT-to-MM item. No physical device was connected.

Details: [VALIDATION](history/VALIDATION.md)

## 0.1.0 - 2026-09-25

- First development build: ComboShip's two game engines and combined randomizer packaged as one ARM64 Android app.
- You supply your own OoT and MM ROMs; none are included.
- No separate release notes were kept for this build.
