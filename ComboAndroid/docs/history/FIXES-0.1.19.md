# Combo Ship 0.1.19 — cross-game songs and route logic

Install over the existing Combo Ship app. The package and signing certificate are unchanged. Existing saves retain their own seed and song options. This build includes the previous 0.1.18 improvements and adds the song-travel portion of the authorized backlog.

## Included

- **Play warp songs across games**, under Randomizer → Generate → Combined seed options. Learned song knowledge is shared; instruments and playable buttons belong to the game you are currently playing. For example, learned Requiem plus MM's instrument and A/C-down/C-right permits MM → Desert Colossus even with no OoT ocarina. Removing Requiem, the local instrument, or a required local button closes that route.
- **Song of Soaring in OoT.** Child Link can play it with OoT's ocarina and C-down/C-left/C-up, provided Soaring is learned and the chosen MM owl is activated. MM does not also need an instrument for this direction. Adult Soaring is not included.
- **Optional destination shuffle**, independent of cross-game playback. Mix the six OoT warp pads with ten MM soaring destinations. Songs retain their source requirements; Soaring requires the selected source owl, not an unlock at the shuffled destination. Leave this off for Requiem to keep its normal Colossus destination.
- **Combined route search.** Generation, playthrough search and live tracker reachability exchange reachable destinations across the games. Foreign playback uses shared song ownership and the current game's own instrument/buttons. The saved route map and playback option are part of the seed identity and restored for each run. Generating another seed does not replace an existing run's routes.
- **Travel confirmation.** A touch-friendly destination prompt supports confirmation/cancellation. The final note of a song no longer immediately accepts the prompt. Transitions happen on the game thread, retain the receiving OoT age, and arrive in MM as Human Link.
- **Automatic generation retries and Cancel.** Invalid layouts are retried until a valid seed is found or you cancel. The progress popup shows the attempt number and elapsed time. At five minutes it suggests changing settings and offers Keep trying or Stop and change settings; choosing Keep trying leaves generation running and defers the reminder another five minutes. Cancellation is cooperative at search checkpoints, so a current engine step may finish first. Once a complete seed starts its short final save, the popup shows Saving the completed seed. Existing saves remain intact; a cancelled attempt cannot be started as a new file.
- **Song-aware hint search.** Required-item analysis now considers both inventories and cross-game song routes together, including during its cached replay searches.

These are **new-seed options**. Updating the APK does not silently enable them in an existing seed. Generate a new seed with Play warp songs across games enabled to use the new routing rules.

## Check on your phone

1. Update without uninstalling. Load an existing save and check its seed, progress and tracker; its travel rules should remain unchanged.
2. At the file list, open Randomizer → Generate → Combined seed options. Enable **Play warp songs across games**; leave destination shuffle off. Generate and start a new randomizer file.
3. Once you have Requiem and MM's instrument/buttons, play A, C-down, A, C-right, C-down, A in MM. The prompt should offer Desert Colossus. Cancel once, then play again and confirm. No OoT ocarina should be needed.
4. Try Soaring as Child Link in OoT. Only activated MM owl choices should appear. Confirm a landing and check that you can move, save and reload normally.
5. Test your controller's A/B and D-pad in the travel prompt, as well as touch controls. Controller bindings and physical-device timing still need your feedback.
6. If testing destination shuffle, use a separate new seed. Check that the displayed landing matches the arrival and that reloading preserves it.
7. Start generation and cancel while it is working. Confirm you can change settings and generate again. For a difficult setup, watch the attempt counter advance automatically; if it reaches five minutes, try the reminder's choices.

## Remaining scope and validation

The targeted comparison with OoTMM is in [OOTMM-LOGIC-AUDIT.md](OOTMM-LOGIC-AUDIT.md). This does not claim full logic parity: native save/return behavior differs, separate foreign-song item pools and ageless Soaring are not included, and existing MM oracle exemptions still prevent a stronger every-check guarantee. Fierce Deity movement routes, the full enhancement/trick audit, boss/event path hints and remaining editor/HUD consistency are still on [NEXT-PATCH-BACKLOG.md](NEXT-PATCH-BACKLOG.md).

Build, regression and emulator evidence, with the exact test limits, is recorded in [VALIDATION.md](VALIDATION.md). This is not a full-seed playthrough or physical-phone validation. ROMs and private test saves are excluded from delivery.
