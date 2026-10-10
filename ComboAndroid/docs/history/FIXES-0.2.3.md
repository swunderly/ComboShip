# Combo Ship 0.2.3

Check, Item and Hint trackers now share their access controls, appearance settings and panel behavior in both games.

- **Default access:** fresh tracker settings use **While paused**. Your saved L toggle and existing shortcuts are retained. Item and Hint trackers remain optional; enable them under **Trackers**.
- **Five visibility modes:** Always, While paused, Button toggle, Button hold and On-screen button. Each tracker has its own choice and shortcut. Multiple selected controller inputs form one button combination.
- **Touch buttons:** On-screen button adds **CT**, **IT** or **HT** after a save is loaded. Each toggles only its tracker. The buttons remain available with the regular touch gamepad hidden. Hold **Menu → Move controls** to drag them; **Menu → Tracker button sizes** provides three separate size sliders. Positions and sizes persist by screen shape and controller layout.
- **Common panels:** choose Overlay, Floating window or Side panel. Trackers share one side panel as tabs, so opening another does not repeatedly shrink the game view. The divider can be resized. Tracker bodies and detail popups do not activate the invisible control sticks.
- **Item tracker:** compact, grouped icon grids follow the active game. OoT and MM use the same icon spacing and common controls; missing items stay faded. Equipment, masks, songs, quest items and dungeon items retain their native ownership and counts.
- **Hint tracker:** grouped hints from both games use the same access and appearance controls. Tap a hint for its detail popup. Unrevealed hints do not expose their reward text unless you explicitly enable the spoiler option.
- **Controller notes:** the shortcut picker explains the actual input mapping in tooltips. Default L/L1 maps to L, ZL/L2 to Z and ZR/R2 to R. R/R1 is spare in the default mapping. Modifier 1 and Modifier 2 are extra virtual inputs; assign a physical button to them in **Settings → Controls**, then select that modifier in the tracker shortcut.
- **Search:** CT, IT and HT lead directly to their respective tracker pages. Tracker visibility, touch controls and modifier terms are indexed alongside the other settings.

Install over the previous Combo Ship APK to retain app data.

## Phone checks

1. Confirm your saved L toggle still opens and closes the Check Tracker. Try While paused, then restore your preferred mode.
2. Enable On-screen button for each tracker. Toggle CT, IT and HT independently, including with the touch gamepad hidden.
3. Move one tracker button, change its size and restart. Check that the other buttons retain their positions and sizes.
4. Compare the Item Tracker in both games. Open a revealed hint and an unrevealed hint; only the revealed one should show its text.
5. With your controller connected, assign its spare shoulder button to Modifier 1 and use it as a tracker shortcut.

Build, automated and emulator evidence is recorded in [VALIDATION-0.2.3.md](VALIDATION-0.2.3.md). Physical-controller and phone checks remain for the user.
