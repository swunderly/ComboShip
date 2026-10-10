# Combo Ship 0.1.16 — Android audio response

Actions were audible late because the shared Android audio device inherited OoT's desktop-sized queue: a 4,096-frame target at 32 kHz (128 ms), followed by two 1,024-frame OpenSL output blocks. MM's separate initialization could not replace that already-open shared device. Sending 32 kHz directly to Android also required conversion when the speaker route ran at 48 kHz.

## Changes

- Use a shared Android queue target of 1,536 source frames (48 ms) and 256-frame output blocks. MM still produces a whole game frame per wake, so its queue can exceed the target by that batch; this is not a promise of 48 ms end-to-end latency.
- Query Android's native output rate and continuously convert the games' 32 kHz PCM before submitting it to SDL. Keep conversion history between submissions and include its retained samples in queue accounting. Fall back to 32 kHz output if the route rejects the native-rate request.
- Produce OoT audio one engine tick at a time, retaining its existing sample-debt accumulator and independent five-millisecond pump. Desktop batching and queue settings are unchanged.
- Serialize Android converter use and device close/reopen so an audio-settings change cannot free the converter while the producer uses it.
- Incorporate the previously completed visible-storage frontend into the current multiplayer/seen-check build. Existing visible-folder selections remain valid. Older private-storage installs copy and verify their data before selecting `Internal storage/ComboShip`; originals are preserved. Save format is unchanged.

## Install and check

Save and close your game, then install this APK over the existing app. Do not uninstall first. If you have not installed the visible-folder update, allow file access when prompted and wait for the verified copy to finish.

Using the phone speakers, compare sword swings, rolls, menu movement and item sounds in both games. Also check sustained music and sound after returning from the home screen. Report any remaining delay, crackling or silence and which game it occurs in.

Phone acoustic latency depends on the device and has not been measured here. See `VALIDATION.md` for the actual build, host and Android checks.

The five requests for next week's larger patch remain recorded in `NEXT-PATCH-BACKLOG.md` and are not implemented in this update.
