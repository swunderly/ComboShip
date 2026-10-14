# Combo Ship for Android: Ocarina of Time + Majora's Mask combo randomizer (OoTMM-style)

**Ocarina of Time and Majora's Mask in one combined randomizer, running natively on Android.**

Combo Ship for Android is an unofficial Android port of [ComboShip](https://github.com/Varuuna/ComboShip). ComboShip is a cross-game randomizer inspired by [OoTMM](https://ootmm.com/). It shuffles items across both games at once, so a chest in Ocarina of Time (OoT) can hold a Majora's Mask (MM) item. One seed covers both games. It is built on [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) (SoH) and [2 Ship 2 Harkinian](https://github.com/2ship2harkinian/2ship2harkinian) (2Ship).

This fork turns ComboShip into one Android app (APK) for phones and tablets. Both games run as native code, not in an N64 emulator. No PC is needed: ROM import, game data setup and seed generation all happen on the device. The fork also adds touch controls, phone-sized menus, and many settings and logic rules copied from OoTMM.

> **Unofficial fan project.** It is not made by or affiliated with Nintendo, Harbour Masters, the 2 Ship 2 Harkinian team, the OoTMM team or upstream ComboShip. **No ROMs are included, and no game archives extracted from ROMs.** You need your own legally obtained OoT and MM ROMs. Like upstream ComboShip, this project uses AI in its development. These are early builds; see [Status and testing](#status-and-testing).

## Download

**[Get the latest APK from Releases](https://github.com/swunderly/ComboShip/releases/latest)**

Current release: **Combo Ship for Android 0.4.9** (October 14, 2026). File: `ComboShip-Android-0.4.9-arm64.apk`, about 45 MB (XXSIZEXX bytes). [Direct download](https://github.com/swunderly/ComboShip/releases/download/v0.4.9-android/ComboShip-Android-0.4.9-arm64.apk).

The app is not on Google Play. You install the APK yourself. It does not check for updates, so look here for new versions. Combo Ship for Android's version numbers (0.1.0 to 0.4.9) are this fork's own. They are not upstream ComboShip's version numbers.

## What you need

- Android 9 or newer.
- A 64-bit ARM device (arm64-v8a). There is no 32-bit ARM or x86 build.
- OpenGL ES 3.0.
- Free space for the app and for the game data it extracts from your ROMs.
- Your own OoT and MM ROMs, as raw `.z64`, `.v64` or `.n64` files. ZIP files are not accepted.

| Game | Accepted ROMs | Tested on Android |
| --- | --- | --- |
| Ocarina of Time | The versions Ship of Harkinian accepts. Use a normal ROM, not Master Quest. | N64 USA 1.0, SHA-1 `ad69c91157f6705e8ab06c79fe08aad47bb57ba7` |
| Majora's Mask | N64 USA 1.0 or GameCube USA only. PAL and Japanese ROMs are not accepted. | N64 USA 1.0, SHA-1 `d6133ace5afaa0882cf214cf88daba39e266c078` |

You can check a ROM at [ship.equipment](https://ship.equipment/) (OoT) and [2ship.equipment](https://2ship.equipment/) (MM).

Touch controls and game controllers both work. The owner plays on a phone with both.

## Install and first run

1. Copy your two ROM files to the device first, for example to Download. If a ROM is in a `.zip` file, unzip it first.
2. On the device, open the [latest release](https://github.com/swunderly/ComboShip/releases/latest). Under **Assets**, tap `ComboShip-Android-0.4.9-arm64.apk`, not "Source code". If your browser warns about the file, choose **Download anyway**.
3. Open the downloaded APK. If Android asks, allow your browser or file manager to install unknown apps. The app is not from Google Play and is signed with a development key, so Google Play Protect may warn you. You can still install it, usually with **More details**, then **Install anyway**.
4. Open **Combo Ship** and tap **Allow file access**. On Android 11 and newer, Android opens an **All files access** screen: turn on the switch for Combo Ship, then go back. Combo Ship uses this to keep your saves, seeds and backups in a visible `Internal storage/ComboShip` folder that you can copy.
5. Tap **Choose OoT ROM** and pick your OoT ROM. Then tap **Choose MM ROM** and pick your MM ROM. The app copies them into its folder. Your original files are not changed.
6. The game then starts by itself. On the **ComboShip ROM Setup** panel, choose **Extract** and wait. This only happens once. Later launches go straight to the game. If the panel says a ROM is not valid, choose **Quit**, pick a supported copy under **App options → Manage ROMs**, then open the game again.
7. At the file select screen, tap **Menu** at the top of the screen. On **Home**, tap **Apply preset** under Default, Beginner or Allsanity. Change any settings you like. Then open **Randomizer → Generate** and tap **Generate combined seed**. Settings and seeds belong to one save file, chosen at the top of the menu (File 1 unless you change it).
8. When the seed is done, tap **Return to game**. Use the on-screen stick and **A** to pick that empty file. The Randomizer options open and show **Seed ready for OoT + MM**. Choose **Start Randomizer** and enter a name. (You can also choose **Generate New Randomizer Seed** there.)

**Touch controls.** Drag on the left half of the screen to move. Drag on the right half for the right stick. Both sticks are invisible by default. Tap **Menu**, Android Back or controller Select to open the menu. Hold **Menu** to move, resize, fade or hide the touch buttons. Save in game before you exit.

**App options.** Save first. Then hold **Menu → App options**, or long-press the Combo Ship icon and choose **App options**. Here you can change ROMs, add mods, make backups, move your data, repair support files, or start with **Play without mods**.

**Updating.** Install the new APK over the old one. Do not uninstall first, and do not clear app data. Your ROMs, saves and settings are kept. Saves keep the seed they were made with. New options only affect new seeds.

## Main differences from upstream ComboShip

**The big one: this runs on Android.** Upstream ComboShip is a desktop app for Windows and Linux. Upstream merged macOS support on October 12, 2026. It has no Android version. This fork makes no desktop builds. On a PC, use [upstream ComboShip](https://github.com/Varuuna/ComboShip).

| | Upstream ComboShip | Combo Ship for Android (this fork) |
| --- | --- | --- |
| Runs on | Windows and Linux (macOS support merged October 12, 2026) | Android 9+, 64-bit ARM |
| Download | Windows releases, plus nightly Windows and Linux builds | APK on this repo's Releases page |
| Controls | Keyboard and controllers | Touch controls, plus controllers |
| Randomizer | Combined item shuffle, cross-game hints, starting-world choice, Triforce Hunt, 17 shared items, Anchor multiplayer. Since October also SoH 9.3 silver rupees and cross-game teleport songs. | Upstream's features as of September 17, 2026, plus many more OoTMM settings and logic rules (see below) |
| Upstream code | Updated often | Based on upstream ComboShip as of September 17, 2026 |

### Android features

- **One native app.** Both game engines and the combined randomizer are built for ARM64 and packed into one APK. It is not two separate apps and not an emulator.
- **Everything on the device.** ROM import, game data extraction and seed generation all run on the phone.
- **Touch controls.** A multi-touch gamepad with invisible floating sticks, visible fixed sticks, or a visible floating left stick. It has C buttons, D-pad, L, Z, R and Start, with Nintendo, Xbox or GameCube face-button layouts. Buttons can be moved, and their size and opacity can be changed. Layouts are saved per screen shape.
- **Controllers.** Supported through SDL, and used on the owner's phone alongside touch. The default mapping is L/L1 → L, ZL/L2 → Z and ZR/R2 → R. Remap in **Settings → Controls**. Hide the touch buttons with **Hold Menu → Hide touch controls**.
- **Phone-sized menus.** Tabs for Home, Enhancements, Randomizer, Video & Audio, Trackers, Settings and Tools. Adjustable menu size, tap for descriptions, multi-word search, sliders for most number settings, and a movable Menu button. The OoT and MM menus match: each setting has one place and one name in both games, and the two Save Editors share the same tabs in the same order (MM adds Randomizer and Reg Editor).
- **Your files, visible.** Saves, seeds, backups and mods live in `Internal storage/ComboShip`. You can move them to private storage or an SD card. Data is copied and checked before a move, and the old copy is kept.
- **Recovery tools.** Play without mods, startup and crash logs, and a backup ZIP of saves, settings and logs. Restoring a backup is manual.
- **Lower audio delay.** A smaller audio buffer (48 ms target instead of 128 ms). The real delay on a phone has not been measured.
- **MM fixes** from the owner's [2 Ship 2 Harkinian Android fork](https://github.com/swunderly/2ship2harkinian-Android): cutscene, interpolation, clock and pictograph fixes, a background texture fix, and bigger save-editor touch targets.
- **Removed:** the first-launch screen that imports settings from an existing SoH or 2Ship install. It is not supported on Android.

### Randomizer additions (mostly from OoTMM)

Upstream ComboShip already shuffles items across both games. It has cross-game hints, a starting-world choice, a combined Triforce Hunt, 17 shared items and Anchor multiplayer. All of that is kept. This fork adds many OoTMM settings on top. Newer ones use OoTMM v32.3's names and descriptions; some older ones still use the fork's own wording. The few additions that are this fork's own, not OoTMM's, are marked.

- **Presets.** Default, Beginner and Allsanity on the Home tab. They adapt OoTMM's presets to the options this app supports and are not identical.
- **Goal.** Ganon & Majora, Any Final Boss, Ganon, Majora, Triforce Hunt, and Triforce Quest (separate Power, Courage and Wisdom pieces).
- **Shared items.** 31 instead of 17. Added: the five Ocarina buttons, Song of Time, Mirror Shield, Farore's Wind, Din's Fire, Nayru's Love, Iron Boots, Hover Boots, Goron Tunic and Zora Tunic.
- **OoT spells and gear in MM.** Farore's Wind (MM), Din's Fire (MM), Nayru's Love (MM), Iron Boots (MM), Hover Boots (MM), Goron Tunic (MM) and Zora Tunic (MM), each with a shared version. Child Link wears the boots and tunics, as in OoTMM. Cross-Games Farore's Wind lets you warp between the games.
- **Adult Link in MM.** With **Cross-Games Age** on, Link enters MM at the age he has in OoT, as in OoTMM. Adult Link in MM is OoT's own adult Link, made from the OoT game data the app extracts from your ROM, and he rides OoT's adult Epona. To get to MM as adult, play Song of Soaring in OoT (see Cross-game songs below).
- **Souls.** Enemy Souls (OoT), covering 43 kinds of OoT enemy. NPC Souls, Animal Souls and Misc. Souls for MM. These add to the Boss Souls and MM enemy souls that SoH and 2Ship already have.
- **Dungeon items.** OoT silver rupees with puzzle pouches, placed Vanilla, Own Dungeon or Anywhere. MM key rings for the four temples. (Upstream has since added SoH 9.3's own, different silver rupee option. That is not in this build.)
- **Masks.** OoT uses OoTMM's mask model: masks are shuffled, or you start with them.
- **Cross-game songs (this fork's own option).** With **Play warp songs across games**, OoT warp songs work in MM and Song of Soaring works in OoT with MM's owl statues. Each uses its own game's animation and sound. Song of Soaring works in OoT at either age, so adult Link can use it to reach MM once an MM owl statue is activated. There is also an option for mixing OoT warp pads and MM owls as shuffled destinations. (Upstream added its own, separate version of this on October 11, 2026. That version is not in this build.)
- **Path hints.** OoTMM-style path hints for bosses and the final goal, plus Path of Power, Courage and Wisdom for Triforce Quest.
- **Tricks and rules.** One searchable Tricks & glitches list, including 29 MM tricks based on OoTMM's. The newest use OoTMM's exact names; older ones keep the fork's own wording. A Gameplay rules page (this fork's own) links enhancements to logic, such as Song of Time age change and Fierce Deity Anywhere. Turning a rule on in Randomizer also turns on the matching enhancement.
- **Logic.** OoT and MM logic follow OoTMM v32.3 in many places. Examples: MM combat requirements, renewable ammo, money and magic, MM time of day, and one-time MM rewards that stay collected across a three-day reset. A seed can guarantee **All Locations** or just **Beatable Only**. This is not full OoTMM parity. A dated comparison (October 4, 2026) of 402 OoTMM settings records where it differed then.

### Trackers and multiplayer (this fork's own changes)

- **Trackers.** Check, Item and Hint trackers share one set of access modes: while paused, a button, on-screen CT/IT/HT buttons, or a side panel next to the game. The MM check tracker keeps items you have **Seen** apart from items a hint told you about (**Hinted**).
- **Multiplayer.** Anchor co-op from upstream is kept (**Tools → Multiplayer**). This fork makes one sync carry both OoT and MM progress, so rejoining recovers both games. It blocks duplicate items and rejects progress from a different seed. Every player needs the same Combo Ship build and the same seed.

## FAQ

**Is this OoTMM for Android?** No. OoTMM is a ROM randomizer: its website makes a patched ROM that you play in an N64 emulator or on N64 hardware. Combo Ship is a separate program built on ComboShip, Ship of Harkinian and 2 Ship 2 Harkinian. It copies many OoTMM settings and logic rules, but seeds and patches from ootmm.com do not work in it. It has its own seed generator.

**Do I need an emulator or a PC?** No. The games run as native Android code. Setup and seed generation happen on the device. You only need your ROM files on the device.

**Can I play co-op?** Yes, through Anchor, if everyone has the same build and seed. So far it has only been tested with two emulators on a local server.

**Does it work on Windows, Linux or Mac?** Not this fork. Use [upstream ComboShip](https://github.com/Varuuna/ComboShip).

**Does it share saves with the 2 Ship 2 Harkinian Android app?** No. Combo Ship is a separate app (`org.comboship.android`) with its own saves. It does not touch 2S2H Android data.

## Status and testing

These are early builds. Expect bugs, and keep backups of your saves.

- Most testing is automated tests plus an Android 15 emulator. The emulator runs the ARM64 code through translation with a software GPU, usually with audio off.
- The owner plays the releases on a phone, with touch controls and a game controller. Only a few specific fixes have been confirmed on a phone so far.
- No build has had a complete seed playthrough.
- For 0.4.9 (more pickup counts): it builds; it had no host test or emulator run. For 0.4.8 (a silver rupee count in the pickup message): its host tests pass; it was not run on the emulator. For 0.4.7 (an MM soul room fix): its host test passes; it was not run on the emulator. For 0.4.6 (a silver rupee fix): 128 automated tests pass and the fix had an independent code review; it was not run on the emulator. For 0.4.5: 129 automated tests, run on the developer's Windows PC, pass, and each 0.4.5 change was checked on the emulator, including a seed from the Allsanity and Default presets that passed the generator's automatic playthrough check. For 0.4.4: 128 automated tests pass. 2 tests that cannot run on Windows were skipped. On the emulator, adult Link in MM, the MM boots and tunics, casting the MM spells, the menus and the tracker fixes were tried in the app. The automatic playthrough check confirms in logic that a seed can be finished. Nobody played them through.
- 0.4.3 to 0.4.9 were not tested on a phone before release; the 0.4.6 and 0.4.7 fixes were then confirmed on a phone. Installing it over 0.4.1 was not tried on the emulator either; only automated tests cover older saves.
- Not yet checked on a real device: audio delay and quality, keyboards, SD card storage, mods, MM three-day cycle resets, multiplayer over the Internet, and tablets.
- The SoH and 2Ship enhancements carried over from upstream are not all tested on Android.
- Only OoT USA 1.0 and MM USA 1.0 ROMs have been used in testing. Master Quest has only been checked as seed data, never in play.
- This port was written largely by AI coding agents, with code reviews between releases (for example, a full review before 0.3.10 found and fixed 187 problems).

## Known limits

- No Windows, Linux or macOS builds from this fork.
- Not on Google Play. The APK is signed with a development (debug) key. An APK built by someone else has a different signature and cannot install over it.
- 64-bit ARM only.
- Seeds and patches from the OoTMM website do not work.
- MM accepts USA ROMs only (N64 1.0 or GameCube).
- Master Quest dungeon options stay greyed out. They need both normal and Master Quest OoT data, and setup imports one OoT ROM.
- Importing settings from another SoH or 2Ship install is not supported.
- Restoring a backup is manual: copy the files back into the ComboShip folder.
- Upstream changes after September 17, 2026 are not included. That means no SoH 9.3 update, no macOS support, and no upstream cross-game teleport songs.
- MM PvP in multiplayer was unfinished upstream when this fork branched and is not supported here.
- OoT entrance shuffle (from Ship of Harkinian) is available. On Android only a few focused logic tests cover it so far. MM and cross-game entrance shuffle are not available yet.
- Known issues in the current release are listed in its [release notes](https://github.com/swunderly/ComboShip/releases/latest) and in the [changelog](ComboAndroid/docs/CHANGELOG.md).

**In development, not in any release yet:** MM and cross-game entrance randomization (OoTMM-style). The `android-source` branch may already contain unfinished work on it. To play, use the APKs from Releases.

## Reporting problems

Please report Android problems in this repository's [Issues](https://github.com/swunderly/ComboShip/issues). Do not send them to upstream ComboShip, Harbour Masters or OoTMM.

Please include:

- the app version (**App options → About and help**),
- your phone or tablet model and Android version,
- your controller, if you use one,
- what you did and what happened.

Logs are in the ComboShip folder: `native.log`, `last-startup-error.txt`, `java-crash.txt`, and on Android 11+ `android-last-exit.txt` when Android provides one. **App options → Back up saves and settings** makes a ZIP with your saves, settings and logs. It never contains ROMs. It does contain your `Randomizer/` folder, which holds spoiler logs. If you do not want to share spoilers, attach only the log files listed above instead of the ZIP.

## Building from source

Full steps are in [ComboAndroid/BUILDING.md](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/ComboAndroid/BUILDING.md).

```sh
git clone -b android-source https://github.com/swunderly/ComboShip.git
```

To build exactly the 0.4.9 release, use `-b v0.4.9-android` instead. The `android-source` branch can contain unfinished work.

You need Git, Python 3.10+, Java 17 or 21, Android SDK platform 35, build-tools 35.0.0, NDK 28.2.13676358, CMake 3.31.5, Ninja and vcpkg at a pinned commit. Windows also needs Visual Studio's C++ desktop workload. Linux also needs the packages listed in the CI workflow, and ccache. The build uses no ROM. It makes a debug-signed APK. Your own APK has a different signature from the release APK, so it cannot update a release install. Back up your saves before you switch.

Host tests, after a full build (see BUILDING.md): `python ComboAndroid/tests/run_tests.py`.

Branches and docs:

- `android-source` is the Android port's main branch. Releases are tagged `v<version>-android`.
- `develop` is an older, unmodified copy of upstream ComboShip's `develop` (last synced October 9, 2026). For current upstream code, use [Varuuna/ComboShip](https://github.com/Varuuna/ComboShip).
- Any other branches are old or temporary. Use Releases for builds.
- For 0.4.9: [changelog](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/ComboAndroid/docs/CHANGELOG.md), [roadmap](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/ComboAndroid/docs/ROADMAP.md), [device test checklist](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/ComboAndroid/DEVICE_TESTS.md), and [notes on changes to the vendored SoH and 2Ship code](https://github.com/swunderly/ComboShip/tree/v0.4.9-android/docs/deviations) (started upstream, extended here).
- Upstream ComboShip's own README is at [Varuuna/ComboShip](https://github.com/Varuuna/ComboShip#readme).

## Credits

This fork stands on the work of many projects:

- [ComboShip](https://github.com/Varuuna/ComboShip) by Varuuna and contributors. This fork branched from upstream commit `94eb185e` (September 17, 2026).
- [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) by Harbour Masters.
- [2 Ship 2 Harkinian](https://github.com/2ship2harkinian/2ship2harkinian) by the 2 Ship 2 Harkinian team.
- [libultraship](https://github.com/Kenix3/libultraship) by Kenix3, including the Fast3D renderer by Emill and MaikelChan.
- [ZAPDTR](https://github.com/HarbourMasters/ZAPDTR) (Harbour Masters' version of ZAPD by the Zelda Reverse Engineering Team) and [OTRExporter](https://github.com/HarbourMasters/OTRExporter) (Harbour Masters), which also extract game data on the device.
- [OoT3D Randomizer](https://github.com/gamestabled/OoT3D_Randomizer) and [OoT-Randomizer](https://github.com/OoTRandomizer/OoT-Randomizer), which Ship of Harkinian's randomizer code is partly derived from.
- The [zeldaret](https://github.com/zeldaret) decompilation projects for OoT and MM.
- [OoTMM](https://github.com/OoTMM/OoTMM) by the OoTMM Team. OoTMM v32.3 is the reference for this fork's logic, setting names, descriptions and behaviour. Much of the added code and logic data is adapted from OoTMM, including the MM spells, boots and tunics, adult Link's rules in MM, enemy souls, Triforce Quest placement and many logic rules.
- The Android ports this one used as references: [linkzenic/Shipwright-Android](https://github.com/linkzenic/Shipwright-Android) for Ship of Harkinian, and the owner's fork [swunderly/2ship2harkinian-Android](https://github.com/swunderly/2ship2harkinian-Android) of [linkzenic/2ship2harkinian-Android](https://github.com/linkzenic/2ship2harkinian-Android) for 2 Ship 2 Harkinian. Thanks also to Waterdish for the Android ports of both games: [Waterdish/Shipwright-Android](https://github.com/Waterdish/Shipwright-Android) and [Waterdish/2ship2harkinian-Android](https://github.com/Waterdish/2ship2harkinian-Android), which linkzenic's 2 Ship 2 Harkinian port was forked from.
- SDL2, SDL_net and the other libraries whose license notices ship inside the APK under `assets/licenses`.

## Licenses

There is no single license for this repository. Each part keeps its own terms.

- Code written for this port in `ComboAndroid/` is under the MIT license ([ComboAndroid/LICENSE](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/ComboAndroid/LICENSE)). Third-party files in that folder, such as the Gradle wrapper and data taken from OoTMM, keep their own licenses (see [ComboAndroid/licenses/](https://github.com/swunderly/ComboShip/tree/v0.4.9-android/ComboAndroid/licenses)). This does not relicense the games, engines or other dependencies.
- Ship of Harkinian and upstream ComboShip do not publish a license file, so their code keeps its authors' terms. This fork's own changes outside `ComboAndroid/` are not covered by ComboAndroid/LICENSE. This fork does not relicense any upstream code.
- 2 Ship 2 Harkinian is CC0-1.0 ([LICENSE](https://github.com/2ship2harkinian/2ship2harkinian/blob/develop/LICENSE)).
- libultraship ([LICENSE](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/libultraship/LICENSE)), Fast3D ([LICENSE](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/libultraship/src/fast/LICENSE.txt)), ZAPDTR ([LICENSE](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/ZAPDTR/LICENSE)), OTRExporter ([LICENSE](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/OTRExporter/LICENSE)) and the OoT-Randomizer code ([LICENSE](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/soh/soh/Enhancements/randomizer/3drando/LICENSE.md)) are MIT.
- OoTMM is MIT. A copy of its license is at [ComboAndroid/licenses/OoTMM-MIT-LICENSE.txt](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/ComboAndroid/licenses/OoTMM-MIT-LICENSE.txt) and ships in the APK. Some folders with adapted code also have their own copy, for example [combo/rando/entrances/LICENSE.OoTMM](https://github.com/swunderly/ComboShip/blob/v0.4.9-android/combo/rando/entrances/LICENSE.OoTMM).
- The APK carries dependency notices under `assets/licenses`. In 0.4.9 the APK does not yet include the Fast3D, ZAPDTR, OTRExporter and OoT-Randomizer texts. They are attached to the [0.4.9 release](https://github.com/swunderly/ComboShip/releases/tag/v0.4.9-android) as `THIRD-PARTY-NOTICES.txt`.

## Disclaimer

Combo Ship for Android is an unofficial fan project. It is not affiliated with or endorsed by Nintendo, Harbour Masters, the 2 Ship 2 Harkinian team, the OoTMM team or upstream ComboShip. Nintendo, Nintendo 64, GameCube, The Legend of Zelda, Ocarina of Time, Majora's Mask, the Triforce and related names and symbols are trademarks of Nintendo. They are used here only to describe what the software works with.

This repository and the APK contain no ROMs and none of the game archives that are extracted from ROMs. Like Ship of Harkinian and 2 Ship 2 Harkinian, the app extracts the games' assets on your device from ROMs you supply. Please use your own legally obtained copies. This project will not help you find ROMs.
