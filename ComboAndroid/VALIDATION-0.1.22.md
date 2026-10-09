# Combo Ship 0.1.22 validation

Candidate: `ComboShip-Android-0.1.22-dev-arm64.apk`, package
`org.comboship.android`, version code 23, version name
`0.1.22-dev-combo-94eb185`.

SHA-256: `bdbe5632239d86b341e7e6ed7b86eee20fa9e873a81900fb97162697176d5d63`

Size: 56,949,443 bytes. This update covers the two save-editor improvements and
the requested Shadow door restoration. It retains the previous changes described
in [0.1.21 validation](VALIDATION-0.1.21.md). This report does not claim a complete
playthrough, physical-phone validation or full parity with the OoTMM ROM hack.

## Build and source provenance

The complete Android port stack replays from pinned ComboShip revision
`94eb185e4abcc2d568aa8241fa02c43cdd86c439`. All 167 changed native files match
the fresh replay after newline normalization. The APK source marker matches the
current source bytes. The new integration patch SHA-256 is
`be09e87a45aaf3bb1eac453edef9ec50879262d046b49ee1d331d5856b95b040`.

Native compilation, runtime staging and Gradle assembly passed. Eight ARM64
libraries, 218 required native callbacks and 9,036 support-file hashes were
verified. APK signature verification matches 0.1.21, 16 KiB ZIP alignment passes,
and installed APK bytes match this candidate. Android lint reports zero errors
and 84 non-error findings; the build is not described as warning-free.

## Host checks

| Check | Result |
| --- | --- |
| OoT inventory metadata | All 24 slots; 13 bottle contents per bottle slot, both ocarinas and hookshots, 11 adult and 12 child trade variants. Uses the production picker function and actual item/icon metadata. |
| MM owl flags | All 65,536 possible flags, single destinations, invalid IDs and Activate all/Clear all. Only the ten real destination bits change; reserved bits survive. |
| Shadow room reloads | All 64 encoded transition indices and 1,024 type/switch variants; exact staircase identity, five complete cycles, duplicate loads, both starting rooms, reset/disable behavior and narrow door removal. |
| Native Graveyard logic | 64 cases compile the actual Graveyard exit definitions against controlled engine inputs. Restoration alone does not grant access in logic; the normal trick does. Existing fire alternatives are retained; reaching the warp pad remains required. |
| Rules and snapshots | Defaults, legacy saves, run overrides, original seed immutability and separate runtime/logical settings pass. An absent or old Shadow setting cannot enable the trick silently. |
| Menu bindings | Normal trick filters/search and restoration callbacks pass. The native Fixes page partitions its 56 controls once into 38 fixes and 18 restorations; 191 other routes are unchanged. |

These checks test production helpers and bindings. They do not simulate a full
gameplay route or replace the native emulator checks below.

## Native emulator checks

Tests used a disposable Android 15 emulator, with ARM64 game engines running
through its native bridge. Save files were inspected as strict UTF-8 JSON with
required-field assertions, in addition to reviewing live menus and screenshots.

- OoT's empty inventory slots display faded icons and open the item picker.
  Adding a Deku Stick makes it bright. Normal saving and reloading retain it;
  removing it through None restores the faded slot. Saved slot values are
  `255 -> 0 -> 255`, with the other 23 inventory slots unchanged.
- MM's named owl controls display the correct count. Clock Town and Woodfall
  were enabled, saved and reloaded. Native Soaring shows those two destinations,
  and moving the selection reaches Woodfall. Activate all shows ten; Clear all
  shows zero. Saved flags are `0xA400 -> 0xA450 -> 0xA400`; reserved bits remain
  intact. With only reserved bits set, native Soaring displays its no-destination
  textbox instead of an empty warp map. The untouched cycle-save flags remain
  `0xA400`.
- With Shadow Door Unload off, five complete stair trips leave the real door
  present. With it on, completed torch-room trips count 1 through 5; the door
  stays present for trips 1-4 and is removed on trip 5. Normal movement then
  enters the actual Shadow Temple, scene `0x07`, entrance `0x37`, without Din's
  Fire. No debug position edit or permanent door switch is used for this route.
- The final APK directly repeats the five-trip door removal. Disabling the
  restoration and completing another room reload brings the door back. Its
  shared Restorations page displays either game's controls through Options for.
- Enabling the restoration alone produces runtime/logic `1/0`. Checking Shadow
  Door Unload under normal Tricks & glitches produces `1/1`. Unchecking that
  trick preserves runtime and clears logic. Disabling the restoration clears
  both; the final APK's ordinary native save records `0/0`.

The original combined seed, both games' item placements and generation settings
remain unchanged through these edits. Native OoT seed settings and its 3,314
item-location records also remain unchanged. Graveyard's saved switch flags
remain zero. Whole save bytes are not immutable: normal gameplay statistics,
run overrides and native hint serialization can change. The native hint writer
removed two pre-existing Euro characters from French messages; the original
combined hint payload remained unchanged.

The editor, native Soaring, five OFF trips and temple-entry checks used an earlier
0.1.22 engine candidate with SHA-256
`4e1c922c59e4af14b21c24e81f3228b4bf04c60dca4e5fe9beab52fe146d1f1e`.
The final APK has byte-identical OoT and MM engines; only `libcomboui.so` changed
to route OoT restorations onto the shared page. That final menu, normal trick
selection, five ON trips, disable/door return and native `0/0` save were tested
directly on the final APK. This distinction is retained in the private receipts.

## Test-data cleanup and delivery

The five original save/configuration files under each of the emulator's internal
and visible storage roots were restored independently. All ten restored files
match their captured original lengths, SHA-256 hashes and bytes. The app was
left stopped, the disposable emulator was shut down, and its serial disappeared
from the device list. No physical device was modified.

The public source package uses an explicit 139-file allowlist and verifies ZIP
members, CRCs and source bytes. The APK and source archive exclude local ROMs,
ROM-derived game archives, private saves, test logs and screenshots. Only the
APK, source ZIP, fixes, validation and checksums are copied to the delivery and
authorized local OneDrive folders. Those copies are hash-verified; remote
OneDrive synchronization/download has not been verified.

Repeat the focused [phone checklist](FIXES-0.1.22.md#focused-phone-checks) on the
actual device. Hardware touch/controller behavior, acoustic audio latency,
device-specific performance and a full randomized playthrough remain outside
this patch's emulator validation.
