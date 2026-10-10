# Combo Ship 0.2.4

Cross-game song travel now uses the games' warp presentation.

- **OoT → MM:** Song of Soaring plays before opening MM's Termina owl map. Move between activated statues, confirm the destination, or cancel. The map folds open and closed. Link's departure uses MM's native wing keyframes, loose feathers, wind capsule, camera and fade timing. Child and adult Link can both soar.
- **MM → OoT:** OoT warp songs play before the native song-colored confirmation. Departure uses OoT's particle paths, six song colors, player warp animation, white flash and camera path. Arrival uses OoT's native warp-pad sequence.
- **MM arrival:** Soaring uses the selected statue's arrival sequence, rather than the ordinary save-resume path. Swamp and Mountain Village destinations honor the target save's cleared-temple state.
- **No destination:** Playing Soaring without activated owls shows the in-game message and returns control when dismissed. A missing warp asset is reported separately.
- **Cancellation:** Back from confirmation returns to the map; Back or Start from the map cancels travel. Controls return to gameplay after closing.

The approved menus and progression settings are retained. Songs still require song knowledge, an instrument and the playable buttons in the current game; Soaring destinations still require their source owl unlocks. Existing seeds do not need to be regenerated for these presentation changes.

Install over the previous Combo Ship APK to retain data. All co-op players should use the same build.

## Phone checks

1. Play Soaring in OoT, select a different unlocked owl on the map, and confirm. Check the feather departure and the selected MM arrival.
2. Cancel an owl confirmation, choose another destination, then cancel the map. Gameplay should resume normally.
3. Play an OoT warp song in MM. Check its playback, confirmation, matching particle color and destination. Try a second song with a different color.
4. If no owls are activated, play Soaring in OoT and dismiss the message.

See [VALIDATION-0.2.4.md](VALIDATION-0.2.4.md) for the build and emulator evidence. Audio uses the active engine's available sound bank; this update does not import the other game's fanfare or sound-effect bank.
