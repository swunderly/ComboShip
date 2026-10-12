# Device validation still required

## Quick checks for Cross-Games Age (unreleased)

Checks 4 to 10 matter most: adult Link in MM had never been played before this build. He holds MM's own child-size swords, shields, bow and hookshot, as decided; say if they look wrong in his hands. Note the frame rate as adult Link and while riding Epona.

1. Install over the current build without uninstalling. Load an existing save in OoT and in MM: MM Link is child and nothing has changed. The MM check tracker shows the same checks as before, and a collected rupee or heart rises to the same height as before.
2. Randomizer > Logic & access > World access: **Cross-Games Age** is directly below Cross-Games Farore's Wind, with the tooltip "When you enter MM as Adult Link, you will be Adult Link in MM.", and it can be ticked. If it is greyed out with "Disabled because this device cannot run Adult Link in MM yet", the app could not prepare OoT's adult Link or Epona; report it. Tick Swim under Items & checks > Majora's Mask > Items > Abilities & Upgrades: the row turns greyed out and unticked, saying it is disabled while Swim is shuffled; untick Swim and it comes back as it was. Searching "Cross-Games Age" or "adult age" finds it. The Allsanity preset leaves it unticked.
3. Generate a seed with Cross-Games Age off and start it: the spoiler shows Cross-Games Age off, and both games play as before.
4. Tick Cross-Games Age, generate a seed and start a new file. Enter MM while child in OoT: Link is child. Become adult in OoT, then enter MM by Soaring: Link is adult, with OoT's adult body and face, and the MM check tracker updates right away. Owl-save as adult and resume: still adult. Return to OoT, become child, and enter MM through the Mask Shop: Link is child again, and nothing looks scaled.
5. As adult Link in MM: walk, roll, climb a ladder and a ledge, open a door from each side and a chest, and march with the Bremen Mask. Swing each MM sword you have (Kokiri, Razor, Gilded, Great Fairy's Sword), and hold the Hero's and Mirror Shield. With every sword drawn or sheathed and either shield in hand or on his back, his tunic stays Kokiri green (red or blue with the Goron or Zora Tunic (MM)) and never turns dark navy or black. Play the Ocarina of Time, hold a bottle and a Deku Stick.
6. Aim the bow and the hookshot in first person: the hand and the item are visible. Say whether part of the arm between shoulder and hand is missing.
7. Wear the Bunny Hood, Keaton Mask and Mask of Scents as adult Link: they sit on his larger head. Other masks look as usual. Transform with the Deku, Goron and Zora Masks and back to human.
8. Charge and release a spin attack: the charge ring and the spin trail are adult-sized. Watch Link blink and look left and right: his eyes move the right way.
9. As adult Link, play Epona's Song in Termina Field: Epona looks like OoT's adult Epona, blinks, walks, gallops and jumps a fence, and Link sits on the saddle. Then go to OoT and ride Epona there: she looks and moves as usual. On a child save, MM's Epona looks as before.
10. Go between OoT and MM a few times as adult and as child: Link's age always follows OoT, and nothing looks out of place.
11. Optional, with the item tracker's logic view: as adult in MM, the Astral Observatory passage chest (with explosives), the Snowhead Temple icicle room chest (with explosives) and the way on from Snowhead Temple's dual switches room count as reachable.

If the release-switch commit is reverted: the row is greyed out with the "cannot run" tooltip, new seeds leave Cross-Games Age off, and a save made with it still loads with Link drawn as child.

## Quick checks for Iron Boots, Hover Boots, Goron Tunic and Zora Tunic (MM) (unreleased)

Checks 2 to 8 matter most: the boots and tunics had never been played before this build. The child boots use OoTMM's boot shape with OoT's textures (docs/deviations/rando.md, "Child boot geometry"); if they look wrong on Link's feet, say so.

1. Randomizer > Items & checks > Majora's Mask > Items > Cross-Game Items lists **Iron Boots (MM)**, **Hover Boots (MM)**, **Goron Tunic (MM)** and **Zora Tunic (MM)** after Nayru's Love (MM), each with OoTMM's tooltip, and all four can be ticked (a greyed-out row saying the device cannot run the MM boots and tunics yet means it could not prepare them; report it). Tick Swim under Abilities & Upgrades: the four rows turn greyed out and unticked, saying they are disabled while Swim is shuffled; untick Swim and they come back as they were. Under Randomizer > Shared items, the four Shared rows are greyed out and unticked until their MM option is ticked, and again whenever that MM row is greyed out (for example with Swim ticked). Randomizer > Tricks & glitches lists "Skip playing Goron Lullaby by using Iron Boots", "Fewer Tunic Requirements (MM)", "Stone Tower Updrafts without Deku Mask" and the other new boots tricks with OoTMM's tooltips, and search finds them.
2. Generate a seed with Iron Boots (MM) and Zora Tunic (MM) on and shared off, and start a new file. Find both in MM. As human Link, put the Iron Boots on from a C button: Link clanks, sinks in Zora Cape water, is not pushed by the Snowhead blizzard and presses a heavy floor switch. Press the button again: they come off.
3. Generate a seed with all four MM boots and tunics, Farore's Wind (MM) and Nayru's Love (MM) on, and find them. In the pause menu's Items page, the Iron Boots and Goron Tunic share the Room Key / Letter to Mama slot with Farore's Wind, and the Hover Boots and Zora Tunic share the Letter to Kafei / Pendant slot with Nayru's Love. On each slot, select each boot and tunic the way you select the spells (A, then left/right when A opens a choice), assign it to a C button and put it on. The slot shows the small icons under it and no extra icon in its corner, the cursor stays its usual colour, and L or C-up do not change the item. Repeat once with the Room Key or a letter in the slot as well.
4. Same seed: wear the Zora Tunic with the Iron Boots and walk on the bottom of Zora Cape or Great Bay water for well over a minute. There is no drowning timer, Link's tunic turns blue, and a bottle or the Hookshot works from a C button on the bottom.
5. Generate a seed with Hover Boots (MM) on. Run off a ledge: a glowing circle shows and Link hovers for a moment. Stand on a Southern Swamp lily pad as human Link: it does not sink with the boots on, and does without them. The floor feels slippery.
6. Generate a seed with Goron Tunic (MM) on: on child Link, wearing it turns his tunic red, or the OoT Goron Tunic colour if one is set in Cosmetics. Take it off: the MM tunic colour comes back, including a custom MM tunic colour. On adult Link (Cross-Games Age) it is always OoTMM's red, and green when taken off, whatever the colour settings.
7. Wearing boots and a tunic, put on the Deku, Goron or Zora Mask and change back to human: the boots and tunic are off. Pressing the gear button mid sword swing or while shielding does nothing.
8. With **Shared Iron Boots** on: get the Iron Boots in OoT, go through a portal to MM, and check MM's Iron Boots are owned. Repeat once starting with the MM copy.
9. Add the **Trade Items** group to the item tracker: the gear rows show only on gear seeds and light up after pickup. A save made before this version loads with no new rows and its Link colours unchanged.
10. On a seed without the gear: lily pads sink as before, Link's tunic and sword and shield colours are unchanged, and the MM tunic colour setting still works. A seed with your usual settings generates, and existing seeds and saves load.
11. Optional: with "Stone Tower Updrafts without Deku Mask" on and no gear, the seed may expect the Inverted Stone Tower Temple east middle chest and the two updraft bridge pots without the Deku Mask.

If the release-switch commit is reverted: the four MM rows are greyed out with the "cannot run" tooltip, the four Shared rows are greyed out and unticked even where an MM option was ticked before, and generated seeds contain no MM boots or tunics. Dropping a seed that adds them (made by a build with the switch on) plays the error sound and does not load it.

## Quick checks for Din's Fire (MM) and Nayru's Love (MM) (0.4.1)

Checks 2, 3, 4 and 7 matter most: casting the MM spells had not been played before 0.4.1.

1. Randomizer > Items & checks > Majora's Mask > Items > Cross-Game Items lists **Din's Fire (MM)**, **Farore's Wind (MM)** and **Nayru's Love (MM)** in that order, each with OoTMM's tooltip, and all three can be ticked (a greyed-out Din's Fire or Nayru's Love row means this device could not prepare the spell; report it). Under Randomizer > Shared items, **Shared Din's Fire** and **Shared Nayru's Love** are greyed out and unticked until their MM option is ticked. Search finds "Din's Fire (MM)".
2. Generate a seed with both MM spells on and their shared options off, and start a new file. Find both spells in MM. Din's Fire should light a torch and melt ice or a large snowball; Nayru's Love should block damage and last through a scene change. Both use magic, and only human Link can cast them. Use the trade-slot cycling to switch between the Deed and Couple items and the spells.
3. In the Southern Swamp with Swim, cast Nayru's Love and swim through the poisoned water: you should take no damage while it lasts.
4. With **Shared Din's Fire** on: pick up Din's Fire in OoT, then go to MM: MM's Deed slot holds it. Repeat once starting with the MM copy.
5. An MM spell placed in an OoT chest shows the spinning OoT spell model, not a blue rupee.
6. Add the **Trade Items** group to the item tracker: the spell rows show only on this seed and light up after pickup. In the default **Inventory** group, the Deed and Couple slots show the Din's Fire and Nayru's Love icons once you have only the spell there, not the Moon's Tear or a letter. A save from 0.4.0 or earlier loads with no spell rows and no new items.
7. With the trick **Burn Igos' curtain with Din's Fire** on, stand at the top of the stairs next to Igos' throne and cast Din's Fire: the left curtain should burn.

## Quick checks for OoT Enemy Souls (0.4.0)

1. Items & checks > OoT > Item pool shows **Enemy Souls (OoT)** with OoTMM's tooltip, and "Complete OoT Soul Compatibility" appears nowhere. Tick it, then turn on Enhancements > Enemy Randomizer: the option unticks and greys out with "Disabled because Enemy Randomizer is on." Turn Enemy Randomizer off again.
2. Generate a seed with Enemy Souls (OoT) on and start a new file. Repeat once with a Plentiful item pool.
3. Without the Lizalfos soul, the Lizalfos room in Dodongo's Cavern should be empty and its doors should stay barred. Once you have the soul, re-enter: they appear and the room clears normally.
4. In a room with two kinds of enemy where you own only one soul, defeat the ones that appear: the door should stay barred.
5. Without Dark Link's soul, his room in the Water Temple should never clear.
6. In the Forest Temple room where Stalfos drop from the ceiling, with the Stalfos soul the room should play as normal.
7. Pick up a soul: it shows a white skull with a red flame and "You found the Soul of ...!". The **OoT Enemy Souls** tracker group marks it, and a hint that names a soul shows its name, not "No Hint".
8. An OoT soul found at an MM check should look like the same white and red token. After collecting it, back in OoT, its enemy appears.
9. Save, quit and reload: owned souls stay owned. A save from 0.3.10 or earlier loads with every enemy present, and so does a new file started from a seed generated before this version.
10. With Boss Souls also on, bosses behave exactly as with Boss Souls alone.
11. In a souls save, turn on Enhancements > Enemy Randomizer and enter a room whose soul you lack: enemies appear and the room clears. Turn it off again.
12. Master Quest Spirit Temple, child side: without the Keese soul, defeat the Torch Slugs in the room just past the crawlspace; its doors open.

## Quick checks for 0.3.10

1. Install over 0.3.9 without uninstalling. The first launch may take a few extra seconds while support files refresh. Load an existing save in each game; it should load as before.
2. At the file select, move to File 2 and choose Generate. Start Randomizer on File 2 should become available, and File 1's pending seed should be unchanged.
3. Randomizer menu: set OoT Mask Quest to Shuffle, close and reopen the menu; it should still say Shuffle. Drag a slider; the game should not stutter.
4. In a seed with silver rupees shuffled, collect one silver rupee check: you should get one item, not two. A silver rupee floating in the air should be collectable.
5. Rotate the phone (or fold/unfold) during play: the game should keep running.
6. Hold the app icon and choose App options while a game is running: it should warn before closing.

The development APK has passed local ROM extraction, combined seed generation, gameplay, normal portals in both directions, one OoT-to-MM item delivery, and MM pause-save/cold-reload checks in an Android 15 emulator. These are targeted integration checks, not a full playthrough. See [VALIDATION.md](docs/history/VALIDATION.md) for exact scope and evidence. Repeat the checks below on a physical device; audio, hardware controllers, SD storage, mods, cycle resets and complete QoL parity still need coverage.

Record phone/tablet model, Android version, GPU, controller, app version and APK checksum. Use a separate test save and export a backup before updating builds.

1. Install on Android 9 or later with ARM64 and OpenGL ES 3.0. Confirm the launcher shows the ocarina icon and **Combo Ship** name. A new installation should ask only for OoT, then MM. With both ROMs already present, launch should go directly to the game with only one Recents card. Reopen from Home and check that the current session resumes.
2. Import supported OoT and MM ROMs through the document picker; test `.z64`, `.v64` and `.n64` byte ordering. Cancel and retry a picker; rotate the screen; leave and reopen partway through setup. Confirm failed imports keep the current step and preserve the previous file.
3. After the second import, confirm the game launches automatically and completes native ROM validation/extraction. There should be no **ComboShip Settings Import** prompt or desktop Browse buttons. Relaunch without repeating extraction. After saving, hold **Menu → App options** and check a clean return to maintenance. Also test the **App options** shortcut by holding the Android app icon.
4. Generate a combined seed using ComboShip's own generator. Wait through finalization for **Seed ready for OoT + MM**, then start an empty save. Name entry should open directly without a **Take a look at our presets!** pop-up. Check randomized rewards in both games. A blank Seed field rolls a new seed; the same text and settings repeat it. Existing saves keep their original seeds. Generation should be unavailable during name entry.
5. Exercise OoT → MM → OoT transitions, cross-game item delivery, linked inventory, save/reload, and cycle reset. Play through a representative combined seed before calling the build stable.
6. Test graphics/audio in both games, higher frame rates, free camera, widescreen, frame interpolation, background/resume and screen rotation. Check Anju's grandmother's background and the enlarged MM save editor.
7. With the default invisible sticks, drag from empty spots anywhere on either half. Test both sticks together, movement plus multiple buttons, all C buttons, camera with your usual settings, D-pad, menu tap/hold, Android Back, controller Select, keyboard input and controller reconnection. Lift one stick while holding the other, cross the center, and background/resume; verify no stuck inputs. Confirm native menus receive touches on both halves with the overlay suppressed.
8. Test Nintendo/Xbox/GameCube layouts, move/resize/opacity settings, menu scaling, screen aspect changes and preference persistence. Under **Hold Menu → Control sticks**, test invisible, visible fixed and visible floating-left modes. Invisible sticks should remain hidden even while held, and saved button positions should survive mode changes and relaunch. In **Move controls**, drag the Menu button (labeled **Done** while editing); releasing the drag should keep the editor open. Tap Done to save, then confirm both tap and hold work at the new position after relaunch. Test **Reset current screen layout** to restore its default position.
9. Import one compatible mod per game. Test a launch without mods, then a normal launch; mod files and enable preferences should remain intact.
10. Copy data to SD storage, verify saves after relaunch, return to the preserved original location, and test an unavailable SD card. Confirm an active game prevents imports, storage copying and backup export.
11. Export saves/settings/logs; inspect the ZIP and verify it omits ROMs, game archives, mods and bundled support data. Backups currently require manual file restoration; there is no in-app restore importer.
12. Check native/Java logs and Android exit reports after a failed launch. Android exit history requires API 30+, and native trace availability depends on Android/OEM support.

13. Under **Trackers → Check Tracker**, test the compact side panel in both games: Button toggle, Button hold, While paused, custom button combinations, expandable areas, popup details and skip/unskip. Closing it should restore the game view; divider width should survive toggling and a cold restart. Try **Only show current area** and **Show search and filter toolbar**. Collect a check, save/reload, and travel between games. Verify counts and layout persist correctly. Check that touches inside the tracker operate it while touches beside it still control the game. Use an uncovered touch button for the shortcut, or a hardware controller. See [0.1.11 checklist](docs/history/FIXES-0.1.11.md).

14. For 0.1.12, test **Gameplay → Everyday** preferences in both games, **Playing now** versus explicit game selection, search at your preferred text scale, **Settings → Interface**, **Randomizer → Generate**, and the restart/exit confirmations under **Tools → Session**. See [0.1.12 focused checklist](docs/history/FIXES-0.1.12.md).

15. For 0.1.13, test **Items & checks > Locations / Extra items**, the world buttons, **Hints & traps**, relocated MM dungeon options and search using old pool names. Check option persistence across app restarts, readability at your preferred scale, and combined seed generation in a separate new file. See [0.1.13 focused checklist](docs/history/FIXES-0.1.13.md).

16. For 0.1.14, update every participant and use the same combined seed, room and team under **Tools > Multiplayer**. Check live rewards in both games, cross-game delivery while players occupy different worlds, reconnect recovery, a join while the other player is offline, and save/reload without duplicate rewards. Include an MM fourth heart piece, shared check markers and a cycle reset. Shared-progress co-op is the supported repair; MM PvP remains incomplete upstream. See [0.1.14 focused checklist](docs/history/FIXES-0.1.14.md).

Report failures with the exported diagnostic ZIP. ROMs are not needed in a bug report. Web OoTMM-generated seeds are a separate format and are not certified compatible with this runtime.

## Version 0.1.22 editor and restoration checks

- Tools > Save Editor > Ocarina of Time > Inventory: absent items should appear dimmed and remain selectable. Add/remove a basic item, try bottle contents and trade items, save normally and reload. Owned items should be bright.
- Tools > Save Editor > Majora's Mask > General: expand Owl Statues / Warp Destinations. Toggle named destinations, play Soaring with the required song/instrument, save normally and reload. Try Activate all and Clear all.
- Enhancements > Restorations > Shadow Door Unload: with it enabled, cross the actual Graveyard stair loading boundary five complete round trips in one visit. Return to the torch room and enter the temple without opening the door normally. Repeat with the option off; the door should remain. Turning it off should bring the door back on its next room reload.
- Randomizer > Tricks & glitches > Shadow Door Unload: verify checking the trick enables both its restoration and logic assumption. Unchecking the trick leaves the restoration available but removes the assumption. Enabling the restoration alone must not check the trick. Disabling the restoration clears both.

See [0.1.22 fixes](docs/history/FIXES-0.1.22.md) and [validation scope](docs/history/VALIDATION-0.1.22.md).
