# Combo Ship Android 0.1.22

This update addresses the OoT inventory editor, MM owl destinations and the requested Shadow Temple door-unload restoration. Validation and test limits are recorded in [VALIDATION-0.1.22.md](VALIDATION-0.1.22.md).

## Save editors

- **Tools → Save Editor → Ocarina of Time → Inventory** shows all 24 item slots. Empty slots display faded icons and remain clickable; owned items are bright. The picker retains removal, bottle contents, both ocarina/hookshot variants and the trade sequences.
- **Tools → Save Editor → Majora’s Mask → General → Owl Statues / Warp Destinations** provides ten named checkboxes, an active-destination count, Activate all and Clear all. The same controls appear in Flags. Use the normal game save to persist edits. Adding statues does not give the song or an instrument.
- Clear all preserves unrelated reserved save bits. Native Soaring and pause-map warping ignore those unused bits when deciding whether any destinations exist.

## Shadow Door Unload

Enable **Enhancements → Restorations → Shadow Door Unload** to use the stair shortcut. This shared page now includes OoT restorations; use **Options for** to select Ocarina of Time if MM is active. Cross the loading boundary on the Graveyard stairs five complete round trips in one visit. If the fifth trip ends upstairs, descend again to see the door disappear in the torch room. Small movements that do not reload the rooms do not count. The restoration works for either age and does not require Din’s Fire.

This is the requested five-trip compatibility recreation. It deliberately removes only the Shadow entrance door and its collision; it does not exhaust memory, change an actor limit or permanently set the door switch. Leaving the Graveyard resets the setup. Turning it off allows the ordinary door to return on its next room reload.

**Logic may require this shortcut only when Shadow Door Unload is checked under Randomizer → Tricks & glitches.** The trick enables the restoration too. Unchecking the trick leaves the gameplay restoration available but removes the assumption. Enabling the restoration alone does not enable the trick. Logic still requires reaching the Graveyard warp pad. Mid-run changes show the existing progression warning and preserve the original seed settings.

The [OoT Randomizer wiki](https://wiki.ootrandomizer.com/index.php?title=Advanced_%26_Glitched_Logic) explicitly lists unloading the Shadow door. [ZeldaSpeedRuns](https://www.zsr.gg/oot/region/graveyard-windmill) documents stair reloading for the related grave unload. The supplied [Shadow door demonstration](https://youtu.be/WtS0uIEAnNM) links to an earlier example. The browser could retrieve metadata and references but could not play the video; the port implementation was checked against native room-loading code and live tests.

## Focused phone checks

1. Add and remove an OoT inventory item using a faded slot, then save and reload.
2. Toggle two MM owls, play Soaring with its required song/instrument, and verify only those destinations are available. Save and reload. Try Clear all.
3. Test Shadow Door Unload with the restoration off and on. With it on, perform five full stair trips and enter the temple without opening the door normally.
4. Confirm the restoration and its normal Tricks & glitches assumption remain separate. Leave the trick off if the generator should not require this setup.

Update the existing installation in place. No ROMs or private test saves are included in the APK or source archive.
