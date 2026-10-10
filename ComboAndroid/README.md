# Combo Ship for Android

Current version: **0.3.9**. What changed in each release is in [docs/CHANGELOG.md](docs/CHANGELOG.md).

This repository is a fork of Varuuna/ComboShip (forked at 94eb185e4abcc2d568aa8241fa02c43cdd86c439) with the Android port and its randomizer additions committed directly to the source tree. This directory holds the Android app, its build tools and tests. The port keeps the real two-game runtime and combined randomizer rather than wrapping two standalone apps.

Shared progression controls connect Randomizer and Enhancements, with explicit enhancement-only choices. Native foreign-song performances, adult Soaring, saved mixed destinations, a unified techniques catalogue, bounded sliders, a common HUD editor, combined generation guarantees, saved rule/technique snapshots, boss/event paths and distinct tracker knowledge are retained. The [previous completion audit](docs/history/RELEASE-AUDIT-0.2.0.md) reconciles the earlier feature list. The full source comparison covers 402 stable OoTMM settings; it records differences rather than claiming complete equivalence.

Earlier audio, visible-folder storage, combined multiplayer, compact check tracker, movable Menu button, invisible sticks, two-ROM setup and single-task startup fixes are retained. All co-op players need the same build and combined seed. Android 9+, ARM64 and OpenGL ES 3.0 are required. Automated tests and emulator checks do not establish a full physical-device playthrough or complete QoL/ROM-hack parity.

The app identity is `org.comboship.android`, separate from `com.twoshipfork.mm`. Existing 2S2H data is not reset, overwritten, or automatically migrated. ROMs are supplied by the user; no game ROMs or ROM-derived game archives are included in the repository or build artifacts.

## Use the app

The top tabs are **Home**, **Enhancements**, **Randomizer**, **Video & Audio**, **Trackers**, **Settings** and **Tools**. Home contains Default, Beginner and Allsanity adaptations from [OoTMM's preset definitions](https://github.com/OoTMM/OoTMM/blob/master/packages/generator/lib/combo/presets.ts). Apply a preset at file select, fine-tune its settings, then use **Randomizer → Generate**. These adapt supported options; they are not identical to the ROM randomizer's presets.

**Randomizer** configures the next seed. **Items & checks**, **Shared items**, **Warps & travel** and **Gameplay rules** have distinct pages. Gameplay rules includes MM's logic-affecting quest requirements. Randomizer enables an ability and its generation assumption; the matching enhancement is then checked and greyed out. **Enhancements** can enable the same ability alone, without affecting logic. The short **Guide** is available on request; settings explain themselves through hover, focus or touch tooltips. Mid-run progression changes show a small temporary warning. **Video & Audio → Camera** contains both games' camera settings.

**Warps & travel** has one cross-game song toggle for both Child and Adult Link. Playing a song still requires knowledge of that song, a local instrument and playable buttons; Soaring needs an activated source owl. **Enhancements → Difficulty** contains OoT's minigames and MM's runtime-only **1–10 chicks** slider for Bremen Mask. Old one-chick settings and saved seeds retain their quest counts without adding a logic assumption.

**Video & Audio → Screen & HUD** contains automatic framing and the common HUD editor. Audio, graphics, appearance and overlays are grouped in this tab. **Settings → Controls** contains the shared D-pad preference and controller settings. **Enhancements → Items & equipment** contains shared arrow and unequip preferences; **Enhancements → Saving** contains autosave. Game-specific options consistently use OoT/MM buttons. There is no Everyday page.

Under **Trackers**, Check, Item and Hint trackers use the same access and appearance controls. Fresh settings use **While paused**; existing saved shortcuts are retained. Choose **Button toggle** and **L** for your preferred access, or **On-screen button** for a separate **CT**, **IT** or **HT** touch button. Hold **Menu → Move controls** to move it; **Tracker button sizes** adjusts each button separately. These buttons work with the ordinary gamepad hidden. Choose **Side panel** to share a dock beside the game. Search/filter controls are optional, and unrevealed hint text stays hidden by default.

Default controller mapping is L/L1 → L, ZL/L2 → Z and ZR/R2 → R; R/R1 is spare. Modifier 1 and Modifier 2 are extra virtual inputs. Map a physical button to one in **Settings → Controls**, then choose it in the tracker shortcut. Selecting several inputs requires them together.

1. Install the ARM64 development APK over your existing app and open Combo Ship. Allow file access if prompted; existing data is copied and verified into `Internal storage/ComboShip`, with the original preserved.
2. Choose your supported **OoT ROM**, then your **MM ROM**. Import accepts `.z64`, `.v64` and `.n64` byte ordering; the native extractor checks the game/version.
3. After the second import the game starts automatically and prepares its data. Later launches open the game directly; updates may briefly prepare support files. Use ComboShip's native combined seed generator; Web OoTMM seed compatibility is not implied. Wait for **Seed ready for OoT + MM**, then choose **Start Randomizer** in an empty save slot. Existing saves retain their original seeds.
4. Drag from an empty spot on the left half for movement or the right half for the right stick; both sticks are invisible by default. Tap **Menu**, Android Back, or controller Select to open native settings. Hold **Menu** for touch layout, **Control sticks**, menu size, and exit controls. Save in-game before exiting.
5. Hold **Menu → App options** after saving, or long-press the Android app icon and choose **App options**, to manage ROMs, mods, storage, backups or repairs. The game must be closed before changing its files. **Mods and recovery → Play without mods** skips mods for that launch without changing them or their preferences. A recorded startup error also keeps recovery available instead of immediately relaunching.

To move Menu, pause the game, hold **Menu → Move controls**, and drag the button labeled **Done** to the desired spot. Release it, then tap **Done** in its new position to save the layout. Menu uses the same screen-shape and face-layout profiles as the other buttons. **Reset current screen layout** also resets its position.

## Included Android work

| Feature | Implementation and limits |
| --- | --- |
| Shared trackers | Check, Item and Hint use common access modes, shortcuts and appearance. Independent CT/IT/HT touch buttons can be moved and resized. One side panel contains tracker tabs; hiding it restores the game view. Native ownership, check knowledge and saved hints remain authoritative. Tracker bodies and popups bypass invisible sticks. |
| Combined runtime and randomizer | Both actual game libraries, shared engine, ComboShip UI, generator, transitions and cross-game hooks retained. Emulator checks include a deterministic combined seed, normal portals in both directions, and an OoT chest delivering an MM mask. |
| Existing game enhancements | Native graphics, audio, controls, camera, timesaver, tracker and randomizer menus retained from ComboShip's OoT/MM trees. They are not all independently tested on Android. |
| Touch controls | Multi-touch virtual SDL gamepad with invisible floating sticks on both halves, plus optional visible fixed or floating-left modes. C buttons, D-pad, shoulders, Start, Nintendo/Xbox/GameCube face layouts, visibility, opacity, size and drag layout. Buttons take priority; each stick retains its own finger until release. Layouts are stored by screen shape and preset. |
| Menu usability | Adjustable native menu scale, touch controls suppressed over native menus, Android Back and controller Select support. |
| Storage | Visible `Internal storage/ComboShip` root with `Save/`, `Randomizer/`, `Backups/` and `mods/`. Existing private data is copied and hash-verified before switching, with originals preserved. Active missing or inaccessible storage never silently becomes an empty save folder. Legacy locations remain explicit recovery choices. |
| Fork fixes | MM cutscene/interpolation/clock/pictograph backports, user's Grandma background texture fix, enlarged MM save-editor touch targets and separated ammo fields. |
| Recovery | Launch without mods; native startup log and previous log; Java exception report; Android process-exit reports on API 30+ and traces where the OS provides them. |
| Imports/backups | Atomic byte-order-normalizing ROM import, separate mod folders, save/settings/log export, and cross-process locking to prevent imports or backups during gameplay. Backup restoration is manual. |
| Packaging | Eight real ARM64 libraries, GLES shaders, both support archives, extractor XML, controller database, dependency notices, and 16 KiB ELF alignment checks. |

Reference sources: [ComboShip](https://github.com/Varuuna/ComboShip/tree/94eb185e4abcc2d568aa8241fa02c43cdd86c439), [user's 2S2H Android fork](https://github.com/swunderly/2ship2harkinian-Android/tree/11d8fb3c067c397ac038d6372fde0cdb436b097b), [Grandma fix](https://github.com/swunderly/2ship2harkinian-Android/commit/c3e2886c638af11b7514e52455b1f3d8b9789aed), [save-editor touch fix](https://github.com/swunderly/2ship2harkinian-Android/commit/9031e2d7b85d999782e8083a391ad52572e1d511), and [Shipwright Android](https://github.com/linkzenic/Shipwright-Android/tree/94e8d7af3fb3a2e21fb5bf1d250cd0a63a5dd09d).

Build instructions are in [BUILDING.md](BUILDING.md), completed checks in [VALIDATION.md](docs/history/VALIDATION.md), and outstanding phone/gameplay checks in [DEVICE_TESTS.md](DEVICE_TESTS.md). The release history is in [docs/CHANGELOG.md](docs/CHANGELOG.md).

Local ROMs, extracted archives, test saves and logs are kept outside the repository and APK.

Source licenses remain those of each upstream component. Added port-specific code is available under MIT; this does not relicense the upstream games, engine, or dependencies.

Song options are under **Randomizer → Warps & travel** and **Enhancements → Warps & travel**. The first configures new seed logic; the second can enable runtime travel without requiring it in generation. Destination shuffling is independent. Interactive generation retries invalid layouts until success or cancellation, with a reminder after five minutes.
