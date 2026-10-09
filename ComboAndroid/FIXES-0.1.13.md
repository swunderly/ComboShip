# Combo Ship 0.1.13 - a consistent randomizer menu

Install `ComboShip-Android-0.1.13-dev-arm64.apk` over the existing app. Saves and preferences are retained.

## What changed

- **Randomizer > Items & checks** brings together the former Shuffles, Check Pool and Item Pool pages. Both worlds now have the same **Locations** and **Extra items** categories. Locations chooses which checks give randomized rewards; Extra items chooses additional abilities, equipment and other items that can enter the pool.
- **Dungeons** now includes MM's dungeon items, alongside the corresponding OoT page. MM small keys, boss keys and stray fairy placement no longer hide in Item Pool.
- **Hints & traps** has matching **Hints** and **Traps** categories in both worlds. MM traps have moved here from Item Pool. Its hint controls use the available page width, and its logic cards reflow when space is tight.
- Randomizer options have visible **Ocarina of Time** and **Majora's Mask** buttons. Choose a world once and move between topics; the selected world and pool/hint category stay selected during that menu session. These choices are independent of which game is currently running.
- Pages follow one workflow: **Generate, Items & checks, Starting inventory, Logic & access, Dungeons, Hints & traps, Excluded checks, Tricks & glitches, Plandomizer**. Short explanations clarify each topic; world and category buttons remain above the scrolling options.
- Search includes page shortcuts and recognizes old names such as **check pool**, **item pool** and **starting items**.
- The former **World rules** page contained presentation and convenience settings. These now live under **Gameplay > Randomizer extras**, apart from seed-generation rules.

The controls still use each game's existing settings and generator callbacks. Changing an OoT seed option does not silently change the MM option. Game-specific rules remain available under the common categories. MM's pool balance explicitly reports the MM pool, not a combined total.

## Please test

1. Open **Randomizer > Items & checks**. Switch between both worlds and both categories; confirm options are readable at your usual menu scale. Scroll a long page and check that the world/category buttons remain accessible.
2. Change one option in each world, then visit another topic and return. Close and reopen the app and verify the actual option values were retained. Navigation choices are session preferences; they do not need to survive a cold restart.
3. Find MM key/fairy placement under **Dungeons** and MM traps under **Hints & traps > Traps**. Check that the corresponding OoT options are easy to find in the same pages.
4. Search for **item pool**, **check pool** and **starting items**. Open a page shortcut, then clear search to browse the normal menu.
5. On a separate new test file, generate a combined seed using your desired options and confirm rewards randomize in both worlds. Existing files retain the seed they were created with.

ROMs, extracted game archives, saves and private test data are excluded from the APK and source package.

## Verification

Native and Android builds, lint (zero errors), the expanded navigation regression test, APK signature and packaging checks passed. Emulator testing covered both worlds' reorganized menus, option changes and cold-restart persistence, search navigation, and the corrected MM hint/logic layouts. Original test saves and settings were restored afterward. A fresh seed and physical-device testing remain on the checklist above. Details are in VALIDATION.md.

APK: 54,034,142 bytes. SHA-256: `72db3a57820c4ff3d46c7fd062f01e911d26b8a808a5fbdf0418eb6d64fbd9cb`.
