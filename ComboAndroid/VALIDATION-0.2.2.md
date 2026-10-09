# Combo Ship 0.2.2 validation

Final APK: `ComboShip-Android-0.2.2-arm64.apk`, version code 27, 54,271,028 bytes.

SHA-256: `10246cfa0d47d185d9f04320c4f6a9b1096d0b673450d6d9e661d5fd1bcef4b3`

## Build and automated checks

- ARM64 native build and Android APK assembly succeeded. Android lint reported zero errors; 84 existing non-error findings remain.
- All 28 host tests passed, including navigation/search, gameplay-rule ownership, saved-rule migration, Bremen count migration, and the existing logic, tracker and multiplayer regression tests.
- The actual production checkbox was exercised with host ImGui events: wrapped-label bounds, one toggle on release, cancelled drag, disabled input and ordinary-label behavior passed.
- Applying the distributed overlays to upstream commit `94eb185e4abcc2d568aa8241fa02c43cdd86c439` reproduced all 185 patched native files. Their hashes match the APK's source manifest.
- APK signing verified and matches 0.2.1. The APK passed 16 KiB alignment, archive CRC, eight ARM64 library and 219 native-callback checks. All 9,036 bundled support-file hashes verified.

## Android emulator checks

The menus were exercised on the disposable Android API 35 emulator with both the normal 2280×1080 landscape display and a reduced 1280×720 display.

| Check | Result |
| --- | --- |
| OoT and MM Starting inventory | Both use grouped, faded icon grids. MM has one main scrolling region, with no small top scrolling panel or separate Available Items pane. |
| MM item selection | Adding/removing Ice Arrows keeps its icon in the same grid position and changes the persisted starting-item list. |
| Progressive tiers | None and Gilded Sword produce zero and three progressive-sword entries respectively. The picker shows the actual tier icons. |
| Inventory resources | Starting Hearts responds to its slider and persists the selected count of eight. |
| Randomizer rules | Former MM Difficulty options appear in Gameplay rules. The separate Randomizer Difficulty destination and Bremen entry are absent. |
| Bremen count | The Enhancements Difficulty slider persists a count of four; its obsolete logic value remains zero and no Include in logic control is shown. |
| OoT minigames | The original native fishing controls are available under Enhancements Difficulty and persist their changes. |
| Camera | Both game buttons open their native camera controls under Video & Audio. OoT Right Stick Aiming and MM Free Look changes persist. |
| Search | Bremen opens Enhancements Difficulty; frogs open Randomizer Gameplay rules; free-look search opens Video & Audio Camera. The focused rule remains visible after navigation. |
| Reduced display | The header, both inventory grids and MM's resource controls fit the 1280×720 menu viewport. Longer content scrolls in the main panel. |
| Restart and version | The final APK retained the inventory, heart count, Bremen count, fishing and camera settings across a cold restart. About displays 0.2.2. |
| Cleanup | Ten independently captured original save/config files and the original touch preferences were restored byte for byte. The display override was reset and the task's emulator stopped. |

Item, tier and native-control interactions were tested during the 0.2.2 candidate run. After the final search-scroll correction, the packaged APK was checked again for navigation, camera placement, inventory layout, reduced-display layout, installed-byte identity and cold-restart persistence.

## Scope and distribution

This verifies the requested menu changes and the listed regression checks. It does not establish physical-phone touch behavior, a complete gameplay run, or the absence of every possible bug. Existing camera and minigame callbacks were relocated, not replaced.

Only the APK, reviewed source ZIP, fixes, validation report and SHA-256 manifest are distributed. The source ZIP uses an explicit 165-file allowlist. ROMs, extracted ROM assets, saves, private emulator screenshots/configs, credentials and signing keys are excluded. The APK contains only the existing ROM-free support archives; players supply their own games.
