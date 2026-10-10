# Combo Ship 0.2.1 request audit

This release implements the ten menu requests accepted after 0.2.0. It retains the previously supported feature scope in [the 0.2.0 audit](RELEASE-AUDIT-0.2.0.md).

| Request | Result |
| --- | --- |
| Remove persistent setting explanations | Descriptions appear in hover, focus and touch tooltips. No automatic first-open guide dialogs; Guide remains an explicit action. |
| Keep runtime-only controls out of Randomizer | Randomizer contains seed options and supported logic assumptions. Pure gameplay, cosmetic and tracker preferences have their own destinations. |
| Single cross-game song setting for both ages | The cross-game songs rule applies to Child and Adult Link. The legacy independent adult control is hidden; song, local instrument, buttons and source owl prerequisites are retained. |
| Home presets based on community use | Default, Beginner and Allsanity are adaptations of OoTMM's published presets. Applying one resets previous seed choices, shared-item choices and MM start/excluded lists, preserving audio, display and controller preferences. |
| Generate without duplicated dropdown editors | Seed entry, Generate, Cancel, progress and status only. Rules, goals and shared-item choices have dedicated pages. |
| Shared Items in the sidebar | Dedicated Randomizer > Shared items page for the 17 supported families. |
| Consistent game selectors | Matching OoT/MM buttons replace game dropdowns in the shared pages. |
| Cohesive grouped layouts | Responsive grouped columns and matching widget widths. OoT item locations are organized like MM's world/NPC, collectibles and scenery groups; native setting choices and callbacks are preserved. |
| Bremen difficulty count | Difficulty > Majora's Mask offers a 1–10 chicks slider, replacing the one-chick toggle. Actual flock completion supports every count; existing one-chick preferences and old seed rules migrate. |
| Remove Everyday | D-pad controls go to Settings > Controls; unequip/arrow controls to Enhancements > Items & equipment; autosave to Enhancements > Saving. |

Top-level navigation: Home, Enhancements, Randomizer, Video & Audio, Trackers, Settings and Tools. Video & Audio groups screen/HUD, graphics, appearance, overlays, audio and music. Numeric controls retain sliders and per-setting tooltips.

The additional preset-switch fix clears the complete OoT placement table before generating a new settings-scoped pool. A controlled Allsanity-to-Default reproduction previously rejected 52 unreachable OoT checks; the fixed game libraries validate the same seed with zero unreachable checks. Headless initialization now registers check-name lookup maps after dynamic check registration, allowing an accurate existing-seed reproduction.

Presets follow [OoTMM's source definitions](https://github.com/OoTMM/OoTMM/blob/master/packages/generator/lib/combo/presets.ts) where these engines support the corresponding options. They do not claim identical output or complete feature parity with the ROM randomizer. Unsupported differences are described in preset tooltips.

See [validation](VALIDATION-0.2.1.md) for build identity, checks and physical-device limitations.
