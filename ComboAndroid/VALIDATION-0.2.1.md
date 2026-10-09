# Combo Ship 0.2.1 validation

Build date: October 3, 2026. Package `org.comboship.android`, version name `0.2.1`, version code `26`, ARM64. Install over 0.2.0 with the existing signing identity.

- APK SHA-256: `dfd7b07753a9401538a57e9dfbb210c6310882578c7e653b9037fa83c315227a`
- APK size: 54,266,026 bytes.
- Signing certificate SHA-256: `162c4deb267e71cee159fffe81be7a0dc582f2aad126b599d3a43bd7d6246676`.
- Pinned upstream: `94eb185e4abcc2d568aa8241fa02c43cdd86c439`.
- Final menu integration patch SHA-256: `d556e4349c513c65fad0ddaa03081373a3e63c33f110af81e3813f652889bff0`.

The installed emulator APK matches the delivered APK hash. Signature verification, 16 KiB page alignment, ZIP integrity, all eight ARM64 libraries, 219 required native callbacks, and hashes for 9,036 support assets passed. Both support archives contain port resources; no ROMs or extracted game data are bundled. Source provenance covers 182 changed native files, with a clean replay of the full patch chain on the pinned upstream.

`assembleDebug` and `lintDebug` passed with zero lint errors. The 84 existing lint non-error findings are not represented as a warning-free build. All 28 host tests passed, including menu/search routing, real native preset option mapping, Bremen count execution and legacy migration, song routes and prerequisites, tracker knowledge, multiplayer protocol/state, gameplay-rule persistence, cancellation and combined reachability guarantees. A separate test drove actual ImGui events through the production checkbox helper, covering wrapped labels, release-only toggles, dragging, disabled controls and a normal single-line label.

The actual Android game libraries reproduce the preset-switch defect before the fix: restoring Allsanity then generating Default with seed hash `4276352294` rejects 52 unreachable OoT checks. With the placement reset, the same reproduction passes on its first fill pass with zero unreachable OoT/MM checks and a balanced item pool. Headless and normal initialization now both build dynamically registered check-name lookup tables.

Android API 35 emulator checks completed:

- Applied Default, Beginner and Allsanity through the app and generated a complete combined seed with each. Each seed includes an every-check accessibility guarantee, a playthrough, both games' placements and 40 hints. The respective OoT/MM placement counts were 443/362, 443/362 and 1,494/2,639.
- Switched Allsanity back to Default and verified that shared items, cross-song assumptions, added scenery checks and MM starting lists reset. Audio preferences were preserved.
- Checked both games' grouped item pages, the 17-family Shared items page, long equipment-page scrolling, the separate Video & Audio pages, and narrower 1,280-by-720 layouts.
- Changed the Bremen slider from 10 to 3 and confirmed the runtime and logic values both became 3. Verified the paired control/equipment/saving preferences and D-pad on/off changes in both native games' configuration keys.
- Verified enhancement-only cross songs, the optional logic link, and Randomizer changes that set both values. Search queries for cross songs, warp and Dpad reached the correct settings.
- Played Song of Soaring as Adult Link with the legacy adult option disabled. The native song performance and confirmation appeared, and accepting it arrived at the mapped Mountain Village owl in MM.
- Checked the final APK's HUD, check tracker, hint tracker, timer, multiplayer and Plandomizer pages. Their explanations appear in tooltips; no automatic guide appeared. About displays 0.2.1. The Android crash buffer was empty.
- Restored and byte-verified the independently captured original saves, configuration, layout and touch preferences, then stopped the read-only test emulator. Tests did not access a physical device.

Generation and wider UI checks used the first fully integrated candidate. Search/link and adult transport checks used the next candidate. A source comparison verifies that the final changes affect only the Dpad search alias and six menu presentation files; the generator, logic and game actors are unchanged. The final APK was installed and its changed tooltip pages checked again. The 28 host tests and actual ImGui interaction tests were rerun on the final source. Hashed evidence and source-provenance receipts gate release packaging.

This is emulator validation, not physical-phone validation, acoustic latency measurement or a full playthrough. Phone checks remain useful for the preferred menu scale, fold/cutout behavior, touch tooltips, controls and Bremen difficulty. Existing saves retain their own seed settings.

See [changes](FIXES-0.2.1.md) and [the ten-request audit](RELEASE-AUDIT-0.2.1.md).
