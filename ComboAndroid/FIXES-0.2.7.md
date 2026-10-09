# Combo Ship 0.2.7

Cross-game departure effects now follow the original games' animation code and timing.

- MM to OoT uses OoT's original DemoKankyo warp particles, spline calculations, flash, song colors and camera path. The previous shortened particle sequence is replaced, and the actor now updates every gameplay tick; skipping one in three MM ticks was making it visibly choppy.
- OoT to MM retains MM's original wing skeleton, animation keyframes, feather physics, wind capsule and camera interpolation. The port now uses MM's gameplay cadence, motion blur and scene-hiding behavior. OoT's instrument camera is released before the native Soaring camera takes over.
- The imported owl map uses MM's exact coordinates, cursor radius, rotation and pause-screen update rate. Custom cursor offsets are removed. The song's screen wipe finishes before the map opens, so the map no longer freezes a white overlay behind it.

The original assets load from your locally extracted games. They are not bundled in the APK. Adapters connect the two engines' resource, camera and graphics APIs; this is reuse of the native effects, not a new animation design.

This release addresses animation and map fidelity. The existing Soaring sound-effect/music fallback in OoT remains; it is not a complete import of MM's audio engine. Differences between the engines and player models mean the full cross-game presentation is not certified pixel-identical to the original games.

Install over the previous APK. Existing seeds and saves work; no new seed is required. See [validation](VALIDATION-0.2.7.md) for build and emulator evidence.
