# Combo Ship 0.2.1

Install this ARM64 APK over Combo Ship 0.2.0; the package and signing certificate are unchanged. Saves retain their original seed. No ROMs, extracted game archives, saves or test data are included.

- Setting descriptions use hover/focus/touch tooltips. Long explanatory paragraphs and automatic guide interruptions are removed; the short Guide remains available on request.
- Randomizer contains seed choices and supported logic assumptions. Gameplay-only controls, cosmetics and tracker display controls are placed with their corresponding enhancements, appearance or tracker settings.
- Cross-game songs use one setting for Child and Adult Link. Local instruments, playable buttons, song ownership and activated source owls remain required. The old independent adult option is hidden and retained only for save compatibility.
- Home offers Default, Beginner and Allsanity, adapted from [OoTMM's published presets](https://github.com/OoTMM/OoTMM/blob/master/packages/generator/lib/combo/presets.ts). Presets reset previous seed choices and MM starting/excluded item lists while preserving audio, screen and control preferences. The tooltip identifies unsupported differences.
- Generate contains seed entry, generation/cancellation, progress and seed status. Rules and shared-item editors no longer appear in a duplicate dropdown there.
- Shared items has its own Randomizer sidebar page.
- OoT/MM option selectors use matching buttons. Item pages use grouped World & NPCs, collectibles, scenery and shop sections, with consistent full-width dropdowns and responsive columns.
- Video & Audio groups screen/HUD, graphics, appearance, audio and music. Controls, saving, equipment and time savers have clear destinations.
- Bremen Mask uses a 1–10 chicks slider under Difficulty. Existing one-chick preferences and version 1/2 seed snapshots migrate; completing any selected count grows the flock correctly.
- Everyday is removed. D-pad settings move to Controls, arrow/unequip settings to Items & equipment, and autosave to Saving.
- Generating after switching from a larger preset clears obsolete OoT placements before the new fill. This fixes endless retries caused by old checks influencing confined song/key placement. Headless initialization now builds the same check-name lookup tables as normal boot.

Preset application is available at file select and is disabled during generation. Enhancements may be enabled without logic; Randomizer selections enable both. Existing mid-run changes remain allowed with a brief warning.

Please check your preferred screen/menu scale, touch tooltips, arrow controls and Bremen count on your phone. Emulator and automated tests do not replace physical-device testing or a full playthrough.

See [validation](VALIDATION-0.2.1.md) for the APK identity and completed checks, and the [request audit](RELEASE-AUDIT-0.2.1.md) for all ten menu changes.
