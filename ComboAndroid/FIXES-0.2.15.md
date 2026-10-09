# Combo Ship Android 0.2.15

This hotfix restores compatibility with existing MM saves. Logic changes appended events to MM's saved event array: older saves have 83 or 84 entries, while 0.2.14 expects 88. The fixed-length reader rejected those older saves. That left MM unloaded, made the save editor show empty progress and repeated ocarina icons, and prevented OoT from recognizing MM's Song of Soaring ownership.

The reader now preserves the existing event entries and initializes only the appended entries. Inventory, songs, owl unlocks, collected checks, placements, and settings are retained. Invalid save structures still fail rather than silently creating empty progress. The MM editor now blocks editing when no valid MM save is loaded.

Install 0.2.15 over the existing app; do not uninstall or clear app data. Copy the ComboShip/Save folder before updating. Existing runs do not require a new seed. The reproduced failure did not erase the stored MM section, but recovery of any individual phone save cannot be confirmed without inspecting it.

Validated on an older save in a disposable Android emulator: MM loads, its editor shows the saved items and songs, enhancement-only Song of Soaring works after toggling it during the run, and native owl saving/reopening retains progress. See VALIDATION-0.2.15.md for the scope of verification.
