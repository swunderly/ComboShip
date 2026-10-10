# Combo Ship 0.2.18

Cross-game warp songs now use the originating game's departure presentation and audio resources. Playing Song of Soaring in OoT uses MM's wing animation, feather physics, wind capsule, camera timing, motion blur, and native Soaring scores. Playing an OoT warp song in MM uses OoT's sparkle departure, white flash, player disappearance, camera splines, and native warp-song fanfare.

The departures release the instrument camera before taking over, keep their actors active during the transition, and suppress the HUD through message-close cleanup. The native song audio remains available while its score runs; the audio adapters handle the originating game's sequence commands without changing unrelated music. Scores and samples come from the user's existing game archives.

Fast Ocarina Playback and Skip Soaring Cutscene now apply to cross-game Soaring. Skipping departure retains the selected destination and the destination game's native arrival animation. MM song arrivals also clear a remembered pause-save entrance that could otherwise replace the selected landing with a normal resume spawn.

This release targets warp presentation and routing. Existing save files remain supported; it does not require a new randomizer seed. Physical-phone playback and sound quality still need the checks listed in VALIDATION-0.2.18.md.
