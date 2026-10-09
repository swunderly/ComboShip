# Combo Ship 0.1.12 - one app, feature-based menus

Install `ComboShip-Android-0.1.12-dev-arm64.apk` over the existing app. Saves and preferences are retained.

## What changed

- A Combo Ship home screen replaces the two port-specific menu tabs. Navigation is now **Home, Gameplay, Randomizer, Trackers, Settings and Tools**.
- Related pages meet under one feature: quality of life, time savers, items and songs, difficulty, appearance, logic/access, check pools and hints. Advanced pages follow the game you are playing. **Options for** lets you configure either game explicitly.
- **Gameplay > Everyday** offers five controls that apply to both games: D-pad item equips, unequipping items, quick magic-arrow equip, arrow cycling and autosave. Existing differing preferences remain intact until you change one here; differing values are shown explicitly. Each game retains its native autosave timing and rules.
- **Settings > Interface** controls menu theme, opacity, scale and controller navigation for the whole app. Game-specific startup, language and accessibility options are under Gameplay. Desktop folder/cursor controls and obsolete native search preferences are removed from that page.
- OoT and MM menu controls now use the same font. Narrow content areas reflow into a readable single column. Each feature retains its own scroll position and the main sections remember their selected page during the session.
- Search results respect the menu text size, use compact wrapped location labels, and omit obsolete desktop setup controls.
- **Randomizer > Generate** opens directly to the seed field, Generate button and progress. Detailed combined options are folded below, with the starting world and goal summarized above them. Game-specific randomizer settings remain available by feature. Generation restrictions and save behavior are unchanged.
- All trackers and timers are together under **Trackers**. The shared compact check tracker remains unchanged. Extra item-tracker settings are tucked into a single expandable section.
- **Tools > Session** contains restart and exit, with confirmation to prevent accidental loss of unsaved progress. **Return to game** stays at the top. Mods, presets, editors, multiplayer and developer tools remain reachable.
- **Settings > About Combo Ship** gives the app its own identity and credits the upstream projects.

## Please test

1. Browse each top-level section in both games. Try **Playing now** and the explicit game selector on a feature page. Confirm the page is readable at your usual menu scale.
2. Under **Gameplay > Everyday**, enable and disable an option; check its behavior in both games. Set different values through the individual feature pages, then confirm Everyday reports the difference.
3. At file select, open **Randomizer > Generate**. Try a blank seed and a named seed, wait for the success message, and start a separate new test file. Confirm both worlds randomize. Existing files should retain their seeds.
4. Open the check tracker from its new **Trackers** location; test the shortcut and side panel in both games. Check that controller mapping, music volume and your preferred enhancements still work.
5. Try search, Return to game, and cancelling the restart/exit dialogs. Restart the app and verify saves and settings are retained.

## Scope

This is a substantial menu unification pass. Several advanced editors, presets and game-specific rules still use their original implementation inside the shared navigation. Their behaviors are not interchangeable, and this update does not claim complete feature parity or a full seed playthrough.

ROMs, extracted game archives, private test data and saves are excluded from the APK and source package.

## Verification

Native and Android builds, lint (zero errors), the navigation regression test, APK signature and packaging checks passed. Emulator checks covered both saved games, menu organization, shared preference values, search, restart/cancel flows and the tracker toggle. Original test saves and settings were restored afterward. No new full seed playthrough or physical-device test is claimed. Detailed evidence is in VALIDATION.md.

APK: 54,025,566 bytes. SHA-256: `5bc055ef4a48c3fd505eb715c18aeef39861b6e08d38a0a8f263cec5219937af`.
