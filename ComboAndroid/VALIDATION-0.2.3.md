# Combo Ship 0.2.3 validation

Final APK: `ComboShip-Android-0.2.3-arm64.apk`, version code 28, 54,278,337 bytes.

SHA-256: `e44c6526a14a5ec059e5f8aa859bb194f4486497bbfb8f09210aee42fdf8a812`

## Build and automated checks

- ARM64 native build and Android assembly succeeded. Android lint reported zero errors and 84 existing non-error findings.
- All 28 host tests passed. Production visibility-state coverage includes fresh pause defaults, toggle/hold edges, input combinations, menu/context transitions and independent on-screen toggles. Existing rule, tracker knowledge, search and multiplayer regressions also passed.
- Android instrumentation passed 79 checks using real MotionEvents and isolated test preferences: independent utility callbacks, release-only toggles, drag cancellation, priority over tracker hit regions, hidden gamepad access, size/edge clamping, movement and persistence.
- The distributed source integration reproduced all 191 patched native files from upstream `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. Their hashes match the APK source manifest.
- Signing matches the previous APK. Archive CRC, 16 KiB alignment, all eight ARM64 libraries, 225 native callbacks and all 9,036 support-file hashes verified. Installed APK bytes match the delivered APK.

## Android emulator checks

The task-owned, read-only Android API 35 emulator was used at 2280×1080 and 1280×720. No physical Android device was changed.

| Check | Observed result |
| --- | --- |
| Fresh access | With tracker access preferences absent, all three visibility modes initialize to While paused. Check is enabled by default; Item and Hint remain opt-in. |
| Saved preference | Existing Check visibility 2 and shortcut 32 remain Button toggle and L after migration. |
| Shared settings | Check, Item and Hint use common access/appearance controls and the same five visibility choices. Detailed help remains in tooltips. |
| Visibility | OoT Check was exercised through Always, While paused, L toggle, L hold and CT modes. MM pause opens all enabled tracker tabs; unpause hides them. |
| Independent touch access | CT, IT and HT open/close only their own tracker through production JNI callbacks. Buttons work with the ordinary touch gamepad hidden. |
| Item content | Both games show native owned and faded absent icons in compact grouped grids. MM's final grid uses the same fixed column spacing as OoT. |
| Hint details | Unrevealed details show only the unrevealed notice. A saved revealed-hint fixture displays its text; other hints stay hidden. The fixture changes hint-read state, not item placements or game inventory. |
| Shared dock | Opening another tracker adds a tab to the existing side panel. It does not create additional game-view splits. |
| Button editing | Moving CT and setting its size to 1.43 persisted across cold restart and APK update. IT/HT retained their own default sizes. |
| Search | CT, IT and HT each return one tracker-page result; each Open action reaches the matching settings. Modifier search includes tracker page aliases. |
| Reduced display | All three settings pages, side-panel content and About fit the reduced viewport; longer settings/content use the main scroll region. |
| Version | About displays 0.2.3, matching APK metadata. |
| Isolation | After discarding the read-only AVD's test writes, a fresh boot matched all five untouched original internal save/config files byte for byte. The temporary visible game folder and tracker-button edits were absent. The display override was reset and the task's emulator stopped. |

Visibility and button editing were tested on the 0.2.3 candidates. The final two refinements affected MM item spacing and search matching; the packaged candidate was checked again for those changes, production touch callbacks, common settings, search destinations, MM pause access, reduced layout and version. Host tests and instrumentation passed again on the final build.

MM testing resumed a private combined-save copy in MM. The revealed hint test uses a private saved-read-state fixture. These are presentation/access tests, not evidence of legitimately completing the associated quest or reading that hint in gameplay.

## Scope and distribution

Physical-phone touch ergonomics, real controller shoulder labels/mappings, fold/cutout behavior and a full gameplay run remain outside these checks. No claim is made that every possible bug is eliminated.

Only the APK, reviewed source ZIP, fixes, validation report and SHA-256 manifest are distributed. The source ZIP uses an explicit 168-file allowlist. ROMs, extracted ROM assets, saves, private emulator screenshots/configs, credentials and signing keys are excluded. Players supply their own games.
