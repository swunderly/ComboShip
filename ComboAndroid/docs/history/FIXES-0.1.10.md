# Combo Ship 0.1.10 - shared check tracker

Install `ComboShip-Android-0.1.10-dev-arm64.apk` over your existing installation.

## What changed

- **Settings → Check Tracker** is one compact page, replacing the separate OoT/MM settings blocks and popouts.
- Both games use the same tracker window, layout, search, area groups, details, filters, text size and background opacity. The window follows the active game and keeps its position.
- Both games support **Always**, **While paused**, **Button toggle** and **Button hold**. Button toggle opens with one press and closes with the next. The default shortcut is **L**; an existing MM mode/combination is carried over. Pick a different button or chord under **Change shortcut**.
- Tap a check for its status, known reward, optional logic details or skip/unskip action. Collected checks update automatically. Skipped checks have a separate count and do not inflate the collected count.
- A check listed in several areas is counted once in the overall totals.
- Search matches check names, areas, types and known rewards. Collected, Skipped, Current area, Expand all, Collapse all and Find current work identically in both games.
- Touches over the tracker reach its controls instead of invisible sticks or covered game buttons. Menu stays accessible. The movable Menu button and invisible sticks from 0.1.9 remain available.
- Existing check data and save formats are retained. Unknown rewards and unrevealed dungeon layouts stay hidden. Save normally before exiting, especially after changing skipped checks in MM.

## Please test on your phone

1. Enable **Settings → Check Tracker → Button toggle**. Use your shortcut repeatedly in both games, including while paused.
2. Change the shortcut and try Button hold. Check that releasing it hides the tracker without leaving buttons stuck.
3. Search, expand an area, tap a check, skip/unskip it and change the filters. Collect a check normally, verify the count, then save and reload.
4. Travel between games. Confirm the appearance, filters and shortcut stay consistent and the list follows the game. Move/resize the tracker or Menu button if they overlap.

## Validation

Both native engines and the shared UI build for ARM64. Android build/lint pass, along with 58 real-MotionEvent touch checks and tracker tests for toggle edges, holds, pause visibility, game/slot transitions, filters and totals. APK signature, native entry points, dependencies, 16 KiB alignment and support hashes pass verification.

Both views and their controls were inspected in the local Android 15 emulator using private game data. This does not replace testing on your phone or hardware controller. No ROM or game-derived archive is bundled in either artifact.

APK SHA-256: `453b4388e7f59f41be620ab614e524c55fdbdbc7b76783316d946768e50f4a00`
