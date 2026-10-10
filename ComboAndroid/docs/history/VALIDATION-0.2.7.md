# Combo Ship 0.2.7 validation

APK: `ComboShip-Android-0.2.7-arm64.apk`, version code 32, 45,328,243 bytes.

SHA-256: `d6fb327b83d1661c5006b33d90b75d62cb6f64a5b24782fcf9203ec555ee52d2`

## Build and source checks

- ARM64 native compilation, clean Android assembly and lint succeeded. Lint reported zero errors and 84 non-error findings.
- All 28 existing host regression tests passed. They cover tracker knowledge and visibility, menus, progression rules, song routes and multiplayer protocol behavior.
- The complete overlay reproduced all 207 patched native files from upstream commit `94eb185e4abcc2d568aa8241fa02c43cdd86c439`. The APK source manifest matches the compiled checkout and the fresh replay. Seven native files changed from 0.2.6, confined to the cross-warp actors/map and OoT scene-drawing adapters.
- Signing matches earlier releases. Archive CRC, 16 KiB alignment, eight ARM64 libraries, 231 callbacks and 9,036 support-file hashes passed verification. The installed emulator APK matches this APK byte for byte.
- Nine MM keyframe functions and twelve feather/camera functions match the native implementation after normalizing engine names and ABI adapters. All ten owl coordinates, the OoT camera points and six warp-song colors match their original sources. The MM rotation-matrix math also matches OoT's equivalent implementation.

## Android emulator checks

Testing used a disposable, read-only API 35 Android emulator. Private copies of existing saves supplied owned songs, usable instruments and unlocked owl statues. Full recordings and contact sheets were inspected; actual native MM Soaring footage provided a reference for the imported wing sequence. Failed input attempts and earlier candidates are not counted as passing captures.

| Case | Result |
| --- | --- |
| Owl map | All ten selections were inspected. Cursor centers and displayed destinations agree with the native owl positions. The completed song wipe is not frozen behind the map. |
| Cancel and repeat | Cancelling the imported owl map returns control without changing games. Playing Soaring again opens the map and permits a confirmed departure. |
| Child OoT to MM | Native wing keyframes, feathers, wind capsule, scene hiding and camera behavior execute; the recorded warp arrives at Great Bay Coast. |
| Adult OoT to MM | The same native departure executes with the Adult Link model; the recorded warp arrives at Great Bay Coast. |
| MM to OoT | Requiem completes playback and native confirmation. OoT's orange particles, flash and departure camera run; the recorded warp arrives at Desert Colossus with its native arrival effect. |
| No unlocked owls | Soaring completes its performance, explains that there are no activated owl statues and returns control without transporting the player. |
| Data isolation | After discarding the read-only emulator writes, a fresh boot matched all five original internal save/config files and touch preferences byte for byte. The test folder was absent and the task emulator was stopped. |

These checks establish the inspected sequences and destinations, not physical-phone frame pacing or pixel-identical rendering across the two engines. OoT and MM player models, starting cameras and HUD behavior can differ. The existing OoT-side Soaring sound-effect/music fallback was not replaced in this animation fix. Other warp songs share the verified native palette and spline code but were not each given a new full runtime capture. A full seed playthrough was not repeated.

Only the APK, 180-file source ZIP, fixes, this report and SHA-256 manifest are distributed. ROMs, extracted game archives, saves, private screenshots, fixtures, logs, credentials and signing keys are excluded.
