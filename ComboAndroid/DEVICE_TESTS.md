# Device validation still required

## Home game modes (0.4.10)

Use an empty file for each new run; leave existing progress alone. In Home, select the file's Next seed, choose Game mode, and generate or prepare it. Start the prepared file at file select.

- OoT randomizer: pickups belong to OoT, the Happy Mask Shop stays in OoT, and Ganon is the goal.
- MM randomizer: the run starts in MM, pickups belong to MM, leaving the Clock Tower stays in MM, and Majora is the goal.
- Vanilla OoT: the original opening and ordinary unshuffled items; the Happy Mask Shop stays in OoT.
- Vanilla MM: the original forest opening and first cycle; no free combined-mode ocarina, Deku Mask or songs. The Clock Tower stays in MM. After completing the first cycle, save/reload and check the normal clock start; also check an owl-save resume.
- Combined: both games and cross-game pickups still work. An old combined file still plays as before.
- Save/reload each mode, then switch between files with different modes. The Home selector changes only next-run settings; it must not change an existing run's mode. In MM, reset and owl-save quit should return to the shared selector.

Still due from earlier builds: adult Epona fence jump, adult chest, Goron Tunic hot floor, OoT D-pad ocarina, and pickup counts for silver rupees, keys and MM tokens/hearts. The silver rupee reset and MM soul room reset were already confirmed on Sam's phone. Also check an MM room held shut by a missing soul before and after obtaining that soul, including its temporary clear door.

## Start here: phone checks for 0.4.3

Nothing in 0.4.3 has been tried on a phone yet. These eight checks matter most; the sections below have the details.

1. Install over 0.4.1 without uninstalling. Load your saves in OoT and in MM: they load as before, MM Link is still child, and your settings show the same values. The emulator could not test this.
2. On your 0.4.1 save whose MM spells came from the Save Editor's Give All, cast Din's Fire, Nayru's Love and Farore's Wind from a C button as human Link, in Termina Field and in Woodfall Temple. None plays the "can't use" sound while the magic meter is full. This needs a seed made with those spell options on; a spell the seed was made without still cannot be cast. Details: casting the MM spells, checks 2 and 5.
3. Home > pick a spare file > Default > Apply preset (one tap on the confirm button applies it), then Randomizer > Generate: the seed generates, and Goal reads Ganon & Majora even if the file had Triforce Hunt.
4. On a spare file, generate a seed with Cross-Games Age and Randomizer > Warps & travel > Play warp songs across games on. Become adult in OoT, make sure an MM owl statue is activated and Song of Soaring is learned, then play Soaring in OoT to go to MM: Link is OoT's adult Link. Walk, climb, swing a sword, ride Epona, then play the Fairy Ocarina (brown) and the Ocarina of Time (blue). Note the frame rate and anything drawn wrong. Details: Cross-Games Age, checks 4 to 9.
5. On a spare file, generate a seed with Iron Boots (MM) and Zora Tunic (MM) on. Put both on from C buttons in Zora Cape: Link sinks and walks on the bottom with no drowning timer. Say how the boots look on child Link's feet. Details: Iron Boots, Hover Boots, Goron Tunic and Zora Tunic, checks 2 to 4.
6. On the Fold's inner display, open Enhancements > Time savers and Randomizer > Items & checks in the OoT and MM views: the same headings in the same columns. On the outer display: one column. Back closes a list, then clears a search, then closes the menu. Details: the menus, checks 4, 9 and 12.
7. In the OoT check tracker, the underwater item in Hyrule Field's Tektite grotto and the Gold Skulltula token behind the cow grotto's web turn Seen once they are on screen. Details: OoT tracker sightings, checks 1 and 2.
8. In MM, pause and press B: the save prompt has no title text. Then Tools > Session > Restart game: the OoT title screen shows, and neither game was saved. Details: the MM pause menu and Restart game.

## Quick checks for ocarina input (unreleased)

Each game's view has its own two switches: turning D-pad ocarina on in the MM view does not turn it on in OoT. Check 2 matters most: it is the case Sam reported, D-pad ocarina working in MM but not in OoT.

1. Install over 0.4.4 without uninstalling. Settings > Controls > Ocarina input shows "D-pad ocarina" then "Right stick ocarina" in the OoT view, and the same two rows in the MM view. A switch you had turned on before is still on.
2. Turn on D-pad ocarina in the OoT view. In OoT, take out the ocarina and play with a controller's D-pad: up, left, right and down play the C-up, C-left, C-right and C-down notes. A song you have learned that uses only C notes, played on the D-pad, is recognised. Then in MM, with the MM view's switch on, the D-pad plays notes as before.
3. Turn on Right stick ocarina in the OoT view: in OoT the right stick plays the same four notes.
4. Settings > Controls > Controller mapping > Ocarina Controls has Customize Ocarina Controls and its buttons, but no D-pad or right-stick boxes. Its tooltip says it is Ocarina of Time only and names what it replaces: A and the C buttons, L, and R and Z.

## Quick checks for casting the MM spells (0.4.3)

On 0.4.1, Din's Fire, Nayru's Love and Farore's Wind (MM) given with the Save Editor's Give All played the "can't use" sound everywhere. Use a randomizer seed with all three MM spell options on, and human Link.

1. Save Editor > Inventory > Give all, and Save Editor > General > Max Magic. In the pause menu the three trade slots show Din's Fire, Farore's Wind and Nayru's Love, and the ocarina slot shows the Ocarina of Time. Press A on each trade slot: it offers every trade item, including both the Land and Ocean Title Deeds, and the MM boots and tunics on a seed that adds them. If a trade slot shows an item with a Japanese name right after Give all, say which slot.
2. In Termina Field, cast each spell from a C button: Din's Fire, Nayru's Love, and Farore's Wind twice (the first cast sets a warp point, the second opens Return / Dispel / Exit). Repeat inside Woodfall Temple. None plays the "can't use" sound while the magic meter is full.
3. On a new file, get each spell from its check, or with Save Editor > Majora's Mask > Randomizer > Give randomizer item > Give Din's Fire, Give Nayru's Love and Give Farore's Wind. Cast each one as in check 2.
4. On the Clock Tower rooftop or in the Astral Observatory, where Song of Soaring cannot be played, Farore's Wind's C button is greyed out. It lights up again once you leave.
5. A 0.4.1 save whose spells came from Give All now casts them, with nothing else changed, if its seed was made with those spell options on. If one still plays the "can't use" sound there, try a new seed on a spare file with all three spell options on before you report it. On such a save, A on a spell's trade slot now switches to another item you own for that slot (at once if you own only one, through the list if you own more). The spell you switch away from does not come back until you give it again with Save Editor > Majora's Mask > Randomizer > Give randomizer item, so only try this if you are ready to give it again.
6. Save Editor > Inventory > Reset. The trade and ocarina slots are empty, and pressing A on them in the pause menu brings nothing back. Link takes off any MM boots or tunic he was wearing.
7. If a spell still plays the "can't use" sound, report three things: whether Din's Fire (MM) and Nayru's Love (MM) are greyed out under Randomizer > Items & checks > Majora's Mask > Items > Cross-game items, how the spell was given (found, Give all or Give randomizer item), and whether the magic meter was full.
8. On a seed without Farore's Wind (MM), Save Editor > Majora's Mask > Randomizer > Give randomizer item > Farore's Wind gives junk, like Din's Fire and Nayru's Love on seeds without theirs. On a seed with Farore's Wind (MM) it gives the real spell, which casts.

## Quick checks for Cross-Games Age (0.4.3)

Checks 4 to 10 matter most: adult Link in MM has only been played on the emulator so far. He holds MM's own child-size swords, shields, bow and hookshot, as decided; say if they look wrong in his hands. Note the frame rate as adult Link and while riding Epona.

1. Install over the current build without uninstalling. Load an existing save in OoT and in MM: MM Link is child and nothing has changed. The MM check tracker shows the same checks as before, and a collected rupee or heart rises to the same height as before.
2. Randomizer > Logic & access, at the top: **Cross-Games Age** is directly below Cross-Games Farore's Wind, with the tooltip "When you enter MM as Adult Link, you will be Adult Link in MM.", and it can be ticked. If it is greyed out with "Disabled because this device cannot run Adult Link in MM yet", the app could not prepare OoT's adult Link or Epona; report it. Tick Swim under Items & checks > Majora's Mask > Items > Abilities & upgrades: the row turns greyed out and unticked, saying it is disabled while Swim is shuffled; untick Swim and it comes back as it was. Searching "Cross-Games Age" or "adult age" finds it. The Allsanity preset leaves it unticked.
3. Generate a seed with Cross-Games Age off and start it: the spoiler shows Cross-Games Age off, and both games play as before.
4. Tick Cross-Games Age and Randomizer > Warps & travel > Play warp songs across games, generate a seed and start a new file. Enter MM while child in OoT: Link is child. Activate an MM owl statue and learn Song of Soaring. Become adult in OoT, then play Soaring in OoT to enter MM: Link is adult, with OoT's adult body and face, and the MM check tracker updates right away. Owl-save as adult and resume: still adult. Return to OoT, become child, and enter MM through the Mask Shop: Link is child again, and nothing looks scaled.
5. As adult Link in MM: walk, roll, climb a ladder and a ledge, open a door from each side and a chest, and march with the Bremen Mask. Swing each MM sword you have (Kokiri, Razor, Gilded, Great Fairy's Sword), and hold the Hero's and Mirror Shield. With every sword drawn or sheathed and either shield in hand or on his back, his tunic stays Kokiri green (red or blue with the Goron or Zora Tunic (MM)) and never turns dark navy or black. Play the Ocarina of Time (it is blue), hold a bottle and a Deku Stick. Then play the Fairy Ocarina: he holds it at his mouth the same way, and it is brown like the Fairy Ocarina, not blue (child Link's Fairy Ocarina looks as before). In OoT it looks the same: adult Link with only the Fairy Ocarina plays it with the same brown ocarina (before this build it was blue), with the Ocarina of Time it is blue, and child Link's Fairy Ocarina looks as before.
6. Aim the bow and the hookshot in first person: the hand and the item are visible. Say whether part of the arm between shoulder and hand is missing.
7. Wear the Bunny Hood, Keaton Mask and Mask of Scents as adult Link: they sit on his larger head. Other masks look as usual. Wearing the Keaton Mask, find Kafei wearing his (South Clock Town early on Day 1, or the Laundry Pool): his mask is its normal size. Transform with the Deku, Goron and Zora Masks and back to human, then turn Use Alternate Assets (Video & Audio > Visual enhancements, Majora's Mask) on and off: Link keeps his adult body.
8. Charge and release a spin attack: the charge ring and the spin trail are adult-sized. Watch Link blink and look left and right: his eyes move the right way.
9. As adult Link, play Epona's Song in Termina Field: Epona looks like OoT's adult Epona, blinks, walks, gallops and jumps a fence, and Link sits on the saddle. Then go to OoT and ride Epona there: she looks and moves as usual. On a child save, MM's Epona looks as before.
10. Go between OoT and MM a few times as adult and as child: Link's age always follows OoT, and nothing looks out of place.
11. Optional, with the MM check tracker showing availability and logic: as adult in MM, the Astral Observatory passage chest (with explosives), the Snowhead Temple icicle room chest (with explosives) and the way on from Snowhead Temple's dual switches room count as reachable.
12. As adult Link in MM, play an ocarina: the camera is MM's usual ocarina camera, the same as for child Link, and it goes back to normal after the song.

## Quick checks for Iron Boots, Hover Boots, Goron Tunic and Zora Tunic (MM) (0.4.3)

Checks 2 to 8 matter most: the boots and tunics have only been played on the emulator so far. The child boots use OoTMM's boot shape with OoT's textures (docs/deviations/rando.md, "Child boot geometry"); if they look wrong on Link's feet, say so.

1. Randomizer > Items & checks > Majora's Mask > Items > Cross-game items lists **Iron Boots (MM)**, **Hover Boots (MM)**, **Goron Tunic (MM)** and **Zora Tunic (MM)** after Nayru's Love (MM), each with OoTMM's tooltip, and all four can be ticked (a greyed-out row saying the device cannot run the MM boots and tunics yet means it could not prepare them; report it). Tick Swim under Abilities & upgrades: the four rows turn greyed out and unticked, saying they are disabled while Swim is shuffled; untick Swim and they come back as they were. Under Randomizer > Shared items, the four Shared rows are greyed out and unticked until their MM option is ticked, and again whenever that MM row is greyed out (for example with Swim ticked). Randomizer > Tricks & glitches lists "Skip playing Goron Lullaby by using Iron Boots", "Fewer Tunic Requirements (MM)", "Stone Tower Updrafts without Deku Mask" and the other new boots tricks with OoTMM's tooltips, and search finds them.
2. Generate a seed with Iron Boots (MM) and Zora Tunic (MM) on and shared off, and start a new file. Find both in MM. As human Link, put the Iron Boots on from a C button: Link clanks, sinks in Zora Cape water, is not pushed by the Snowhead blizzard and presses a heavy floor switch. Press the button again: they come off.
3. Generate a seed with all four MM boots and tunics, Farore's Wind (MM) and Nayru's Love (MM) on, and find them. In the pause menu's Items page, the Iron Boots and Goron Tunic share the Room Key / Letter to Mama slot with Farore's Wind, and the Hover Boots and Zora Tunic share the Letter to Kafei / Pendant slot with Nayru's Love. On each slot, select each boot and tunic the way you select the spells (A, then left/right when A opens a choice), assign it to a C button and put it on. The slot shows the small icons under it and no extra icon in its corner, the cursor stays its usual colour, and L or C-up do not change the item. Repeat once with the Room Key or a letter in the slot as well.
4. Same seed: wear the Zora Tunic with the Iron Boots and walk on the bottom of Zora Cape or Great Bay water for well over a minute. There is no drowning timer, Link's tunic turns blue, and a bottle or the Hookshot works from a C button on the bottom. Optional: touch an item dropped on the bottom that you do not own yet (for example Deku Nuts when you have none): Link plays the get-item animation, gets it and can move on.
5. Generate a seed with Hover Boots (MM) on. Run off a ledge: a glowing circle shows and Link hovers for a moment. Stand on a Southern Swamp lily pad as human Link: it does not sink with the boots on, and does without them. The floor feels slippery.
6. Generate a seed with Goron Tunic (MM) on: on child Link, wearing it turns his tunic red, or the OoT Goron Tunic colour if one is set in Cosmetics. Take it off: the MM tunic colour comes back, including a custom MM tunic colour. On adult Link (Cross-Games Age) it is always OoTMM's red, and green when taken off, whatever the colour settings.
7. Wearing boots and a tunic, put on the Deku, Goron or Zora Mask and change back to human: the boots and tunic are off. Pressing the gear button mid sword swing or while shielding does nothing.
8. With **Shared Iron Boots** on: get the Iron Boots in OoT, go through a portal to MM, and check MM's Iron Boots are owned. Repeat once starting with the MM copy.
9. A save made before this version loads with no new items and its Link colours unchanged. (The item tracker's Trade items (MM) group, under Trackers > Item Tracker > Groups, shows the gear rows only on gear seeds.)
10. On a seed without the gear: lily pads sink as before, Link's tunic and sword and shield colours are unchanged, and the MM tunic colour setting still works. A seed with your usual settings generates, and existing seeds and saves load.
11. Optional: with "Stone Tower Updrafts without Deku Mask" on and no gear, the seed may expect the Inverted Stone Tower Temple east middle chest and the two updraft bridge pots without the Deku Mask.

## Quick checks for the menus (0.4.3)

Checks 2 to 5 are the examples Sam reported; they matter most. For each page, look at the OoT view, then tap the MM world button and compare.

1. Install over the current build without uninstalling. Your settings, enhancement presets and each file's randomizer settings show the same values as before.
2. Tools opens on the Save Editor. In OoT and in MM the tabs are General, Inventory, Equipment, Quest Status, Dungeon Items, Flags and Player, in that order; MM then adds Randomizer and Reg Editor. OoT's General looks like MM's: a dark red health bar, a green magic bar and green rupee and wallet bars with Max and Reset buttons, and the file number reads "File N" and cannot be changed.
3. Save Editor > Inventory in OoT: tap Boomerang and it turns on and off with no list; tap Hookshot and a list offers None, Hookshot and Longshot with names. In MM: tap the Bow and it turns on and off; tap a bottle and a list opens in which every entry has a name (Empty Bottle, Red Potion ... Fish, Bug, Blue Fire, Poe, Big Poe, Spring Water, Zora Egg, Sea Horse, Chateau Romani, Hyrule Loach), with no "Unknown" and no Obaba's Drink. Pick Fish: the slot holds a fish and its tooltip reads Fish.
4. Enhancements > Time savers in both views: the headings are Cutscenes, Text & dialogue, Speed-ups and Quests & errands, each shown once. In the OoT view, All and None are the last rows under Cutscenes (All ticks the cutscene skips, None clears them), with no second Cutscenes heading. Climb speed is under Speed-ups in both (it reads +N in OoT and Nx in MM). Open two or three other Enhancements pages: both views show the same headings in the same order.
5. Change a slider on an Enhancements or Settings page and on a Randomizer page in each view (for example Climb speed, Big Poes required for reward in the OoT view of Randomizer > Gameplay rules, and Trap Count in the MM view of Randomizer > Hints & traps > Traps): the name is above it, the value and unit are inside the bar, - and + step it, and a Default button appears. Change a checkbox too, restart the app, and both changes are kept. Tracker, timer, HUD position and advanced resolution sliders keep their compact style. In the MM view of Video & Audio > Visual enhancements > Motion blur, with the mode on Dynamic, On/Off turns motion blur on and off and Strength changes it.
6. Randomizer > Hints & traps > Traps in the MM view shows only Traps and Trap Count, then a line pointing to the trap settings. Enhancements > Extra modes > Traps (MM view) has the nine trap effects, and Enhancements > Randomizer extras has Trap appearance, Junk items and Containers match contents.
7. Randomizer > Gameplay rules in the OoT view: the headings are Abilities, Time of day, Quest requirements, Minigames and Seed settings (no logic effect). The last heading shows once, with Swordless Epona Items; Skip Child Stealth appears there only on a file that has it on. The rules still tick and grey out as before. Under Quest requirements both views read Skip Scarecrow's Song, as Enhancements > Time savers > Quests & errands does. Randomizer > Items & checks > Items: Souls reads Boss Souls and Enemy Souls in both views.
8. Randomizer > Logic & access: Goal, Logic and Starting world are at the top, then Cross-Games Farore's Wind and Cross-Games Age. In the OoT view Starting Age is first under Area access. On the page list, Dungeons comes right after Items & checks.
9. Back: open a dropdown and press Back (the dropdown closes). Search for something and press Back (the search clears). Search again, close the keyboard with Back, tap Open page on a result, then press Back once (the results come back), Back (the search clears) and Back (the menu closes). Type in the search field, close the keyboard with Back, then press Back once: the search clears. While the keyboard is open, Back only closes the keyboard. If an Android 9 or 10 phone is at hand, repeat this check on it: the app finds the keyboard a different way there.
10. Search "climb" and "trap": each setting result reads like "Enhancements / Time savers / Speed-ups - OoT", and Open page goes to that page in the right world. Search "personal notes": it finds the Item Tracker page. In the "trap" results every card shows a setting name and a control: OoT's Freeze, Burn, Shock and other trap rows are greyed out while Enhancements > Extra modes > Additional Traps is off and can be changed when it is on, and the MM Traps card has its checkbox and Trap Count its slider (greyed out while Traps is off). A change made in the results shows on the setting's page.
11. Home > Apply preset and Tools > Session > Restart game ask first, with Cancel first. Settings > Interface > Menu size resizes the menu at once and is kept after a restart.
12. On a Fold, the inner display shows two columns split at the same heading in both views; the outer display shows one column. Randomizer > Items & checks, both views: under Checks, the right column starts at Scenery drops (with All On/Off), then Shops & merchants; under Items, it starts at Souls (then Clocks and Cross-game items in MM). Logic & access, Dungeons and Hints & traps split as before. Then set Settings > Interface > Menu size to 200% and open Logic & access, Items & checks and Dungeons in both views, on the inner display and in landscape on the outer one: on each page and screen the OoT and MM views show the same number of columns (one or two), and a page in two columns splits at the same heading in both.
13. Enhancements > Difficulty > Minigames in the OoT view: each Customize Behavior and Instant Win row starts with its minigame's name, and the Lost Woods ocarina game rows sit together. Video & Audio > Audio in the MM view shows the volume sliders above Ambience.
14. Randomizer > Starting inventory > Other in the OoT view: Link's Pocket and Link's Pocket Reward Type are full-width lists with readable labels. Gold Skulltula Tokens and Starting Hearts are full-width slider rows like MM's Starting Hearts (name above, value in the bar, - and +, and a Default button after a change). Changes are kept after reopening the menu.
15. On a file whose randomizer settings were made by 0.4.1 or earlier, Randomizer > Items & checks > Items in the OoT view: Mask Quest reads Shuffle in Next seed, not "Unknown saved value", and Completed in the Saved and Generated seed views of a generated file. Picking Completed and then Shuffle saves normally. Generating without touching the row gives Shuffle, as before.
16. Save Editor > General in OoT: Player Name is a text field showing the file's name, also on a file made with a US ROM. Rename it (for example to Zelda), save and check the name on File Select. A character the font lacks, such as "!", becomes a space. Use only a disposable file.
17. Home > choose a file > Default > Apply preset > confirm, then Randomizer > Generate: the seed generates, with no "Invalid native settings profile" or "Generation failed". Repeat with Beginner and Allsanity. On a file whose randomizer settings were made by an older build, the shown values are unchanged and Generate still works.
18. Randomizer counts follow the file's own settings: set Goal to Triforce Hunt with 30 pieces and the Rainbow Bridge to Triforce Pieces; the bridge count goes up to 15. Set it to 12 and generate: it succeeds. Set Key rings to Count with Gerudo Fortress keys Any Dungeon and Fortress carpenters Normal: the count goes up to 9; set 9, then set the Fortress keys to Vanilla: the count reads 8, and Generate succeeds. Afterwards set Goal back yourself: Apply preset does not change Goal yet (known issue).

## Quick checks for OoT tracker sightings (0.4.3)

Use a randomizer save with Gold Skulltula tokens shuffled. A check you skipped shows as Skipped even after you see its item: unskip it first in the tracker's details popup.

1. In Hyrule Field's Tektite grotto, stand at the pool edge where the item at the bottom is hidden under the ledge: "Tektite Grotto Freestanding PoH" is not Seen yet. Dive until the item is on screen: the row turns Seen. If it reads "Seen: Blue Rupee", the real item is one you cannot get yet.
2. In Hyrule Field's cow grotto, defeat the Gold Skulltula and look at its token through the web without burning it: "GS Cow Grotto" turns Seen.
3. Save, reload and open the tracker: both rows are still Seen. Collect one: its row turns Collected.
4. A check whose item is hidden, such as a closed chest or an item behind a wall, stays unmarked until you can see the item.
5. Optional: Bombchu Bowling prizes on the counter, shuffled fairies, the Ocarina of Time in the castle moat and fishsanity fish turn Seen once on screen. An MM item at an OoT check shows its MM name.
6. Optional: in the Treasure Chest Game's last room, turn on the Lens of Truth with the closed prize chest in the middle of the lens circle: "Treasure Chest Game Reward" turns Seen. With the Lens off, or with the chest at the edge of the screen outside the circle, it stays unmarked.
7. In Impa's House in Kakariko, look from the front room at the Piece of Heart behind the cow-pen bars, or stand in the pen (back entrance): "Impas House Freestanding PoH" turns Seen.
8. An item behind a solid wall or a closed door still stays unmarked, and a token behind a web (check 2) still turns Seen. Optional: an item seen across Kakariko's wooden fences or Lake Hylia's bridge rails turns Seen.
9. Optional, in the Bottom of the Well or the Shadow Temple: with the Lens of Truth off, an item behind a fake wall or under a fake floor stays unmarked. With the Lens on, look at it where the wall has vanished: it turns Seen.

## Quick checks for the MM pause menu (0.4.3)

1. In MM gameplay, pause and press B: the "Would you like to save?" page's title strip is blank, with no Japanese "セーブ", and looks like MM's own game-over save page.
2. Save Editor > Majora's Mask > Quest Status: give Sun's Song and Saria's Song, with Oath to Order and Song of Storms not learned. On the pause menu's Quest Status page, column 5: the upper song reads "Sun's Song" and the lower "Saria's Song" in English, the cursor turns blue, and the song's notes play back on the staff.
3. Give Oath to Order and Song of Storms as well: column 5 names those two. Pressing right moves to Saria's or Sun's Song with its English name; right again goes on to the Quiver or the sword, and left from there comes back to Oath to Order or Song of Storms.

## Quick checks for Restart game (0.4.3)

1. While playing in MM, open Tools > Session > Restart game and confirm: the game returns to OoT's title screen, as in 0.4.1, not straight into OoT gameplay on the save.
2. Turn Autosave on in both games. In MM, change something visible (for example rupees with the Save Editor), then Restart game: the OoT title shows, and reloading the file does not show the change. Repeat from OoT: Restart game does not save OoT either.
3. With Autosave still on, go OoT → MM → OoT through a portal: the hand-off still saves.

## Quick checks for Din's Fire (MM) and Nayru's Love (MM) (0.4.1)

Checks 2, 3, 4 and 7 matter most: casting the MM spells had not been played before 0.4.1.

1. Randomizer > Items & checks > Majora's Mask > Items > Cross-Game Items lists **Din's Fire (MM)**, **Farore's Wind (MM)** and **Nayru's Love (MM)** in that order, each with OoTMM's tooltip, and all three can be ticked (a greyed-out Din's Fire or Nayru's Love row means this device could not prepare the spell; report it). Under Randomizer > Shared items, **Shared Din's Fire** and **Shared Nayru's Love** are greyed out and unticked until their MM option is ticked. Search finds "Din's Fire (MM)".
2. Generate a seed with both MM spells on and their shared options off, and start a new file. Find both spells in MM. Din's Fire should light a torch and melt ice or a large snowball; Nayru's Love should block damage and last through a scene change. Both use magic, and only human Link can cast them. Use the trade-slot cycling to switch between the Deed and Couple items and the spells.
3. In the Southern Swamp with Swim, cast Nayru's Love and swim through the poisoned water: you should take no damage while it lasts.
4. With **Shared Din's Fire** on: pick up Din's Fire in OoT, then go to MM: MM's Deed slot holds it. Repeat once starting with the MM copy.
5. An MM spell placed in an OoT chest shows the spinning OoT spell model, not a blue rupee.
6. Turn on Trackers > Item Tracker > Groups > Trade items (MM): the spell rows show on this seed and light up after pickup. In the default **Inventory** group, the Deed and Couple slots show the Din's Fire and Nayru's Love icons once you have only the spell there, not the Moon's Tear or a letter. A save from 0.4.0 or earlier loads with no spell rows and no new items.
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
