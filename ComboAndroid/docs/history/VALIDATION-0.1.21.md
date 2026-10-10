# Combo Ship 0.1.21 validation

Candidate: `ComboShip-Android-0.1.21-dev-arm64.apk`, package
`org.comboship.android`, version code 22, version name
`0.1.21-dev-combo-94eb185`.

SHA-256: `ac7c464ea3f31ff7d66e69f628f92678da58de496c1ed6f11461906b7ff32838`

Size: 56,950,066 bytes. This report distinguishes host regressions, actual native
generation, emulator gameplay and outstanding physical-phone testing. It does
not claim a complete playthrough or complete parity with the OoTMM ROM hack.

## Build and provenance

The full port stack replays from pinned ComboShip revision
`94eb185e4abcc2d568aa8241fa02c43cdd86c439`. All 161 changed native files match
the production replay after newline normalization; the APK's source marker also
matches the current source bytes. The final integration patch hash is
`72c91469702556e47a1e20c962dd231dbdf964e122f2527ca728e986befb9537`.

The APK contains eight ARM64 libraries, 218 required native callbacks and 9,036
verified support-file hashes. APK and ELF alignment support 16 KiB pages.
Signature verification passed with the same certificate as 0.1.20. The installed
APK bytes match the delivered candidate. Android lint reports zero errors and
84 non-error findings; these are not described as a warning-free build.

## Host regression tests

The final production helper suite passed **16/16** tests. Coverage includes
rule and technique snapshots, legacy migration and slot isolation, malformed
values, runtime-only and linked capabilities, song mapping and prerequisites,
native pool errors and retries, combined guarantees and goals, post-win fixed
point traversal, boss/event paths without reward leakage, tracker knowledge,
visibility predicates, HUD geometry, catalogue filters, input and multiplayer
identity. A passing multiplayer regression is not a two-phone network session.

## Actual native generation

All seven final-source native integration verdicts passed sequentially on the
Android ARM64 engines through an isolated emulator native bridge. The headless
adapter and test faults are private test artifacts and are absent from the APK.

| Test | Result |
| --- | --- |
| Two default strict seeds | Masters 313221031 and 313221032: zero missing checks in either game; no trimming or padding. |
| MM-start mixed routes and rule combination | First confined preparation rejected explicitly; next master 35219245 passed strict coverage with logical foreign/mixed songs, age mode 3, FDAnywhere and both MM techniques. |
| Saved mixed-route replay | Original assumptions restored despite contradictory older native settings; winning sphere 23, collected fixed point 805 placements. |
| Failed preparation integrity | A confined-fill failure requests another attempt before missing progression can be replaced with junk. |
| Forced combined-oracle retry | Deliberate first-attempt fault removed; next master 2967656801 passed. Check universe and configured advancement counts retained. |
| Completion hunt and replay | Master 491482061: all 20 hunt pieces placed, 8 required; winning sphere 4, fixed point 805. Eight MM junk items trimmed for the configured hunt, no progression padding. |
| No Logic and saved technique | Master 527484916 preserves explicit No Logic and the canonical MM Lensless choice through native settings preparation. No glitchless guarantee is claimed. |

The final universe contains 443 OoT and 362 MM placements, including ten fixed
vanilla owl activations. Every-check coverage includes fixed and junk checks;
completion-only mode tests the chosen goal. The native completion seed happened
to reach every check; a separate production-helper fixture demonstrates the
intentional optional-check relaxation. See [logic notes](LOGIC-0121-VALIDATION.md).

## Live emulator checks

The final-candidate song, visibility, display and touch gates below passed.
Original test data was restored and byte-verified before emulator shutdown.

Passed gameplay and menu checks:

- Shared Gameplay rules: enhancement-only Blue Fire stays outside logic;
  Include in logic links it; clearing that choice preserves the ability.
  Randomizer Sunlight activates runtime and logic on both menu pages. Mid-run
  warning appears without blocking changes. OoT save and cold reload retain the
  choices and edited rupees while the original seed hash remains unchanged.
- `warp` search reaches the shared Randomizer Gameplay rules; retired controls
  are absent. The MM techniques filter and `lens` search show the two genuine
  techniques and their prerequisites.
- HUD guides drag and reset the actual hearts in both games. OoT rupees 81,
  MM rupees 64 and MM bank 1,563 change through sliders without opening the
  Android keyboard. MM numeric editing was observed live; a separate MM numeric
  save/reload was not repeated.
- Adult Link walks and drops normally from the Windmill approach into the dry
  shaft, stopping west of the adult stone. Song of Time changes to Child Link,
  then normal movement enters the real Bottom of the Well (scene 8, entrance
  0x98). No Song of Storms, no drained-well flag and no position edits. Seed
  placements and rules remain unchanged; live hint knowledge may update.
- Final-candidate Cancel was pressed during an actual placement attempt. The
  menu reports cancellation, and every existing seed file plus the saved slot
  remain byte-for-byte unchanged.
- Final-candidate palace entry without seeing the heart leaves both the heart
  and New Wave Bossa Nova unknown and uncollected after a native MM save.
- A separate private Palace visibility fixture displays the original heart
  placement's Stone Tower Small Key model. The production draw, clipping and
  line-of-sight path records **Seen** while it remains uncollected. Debug player
  relocation and actor-update freeze isolate visibility from guards and pickup;
  this is renderer/tracker evidence, not a legitimate Palace route test. The
  freeze was removed before the native pause-save. Seen survives a cold restart
  at the Palace entrance, where the reward is again out of view.
- Final-candidate Marine laboratory entry with the professor visible and zero
  eggs leaves New Wave Bossa Nova unknown and uncollected after a native save.
  A specific reward-revealing gossip hint is a separate Hinted source; the
  professor's presence alone does not reveal the reward. These tracker cases
  leave the seed's original placements unchanged.

The shared-menu and well tests used candidate
`48e61b31a6e92f9cece89799a424655d05dcae87601b14f41bf096e01bb6cae5`.
The final APK differs only in two ASCII menu labels; their gameplay and rule
code is identical. Earlier exhaustive song tests used candidate
`47cc7edb45513c1f6344983133bd873db7f382c3434990a003a5b7ae7caf9d33`
with the same song engine:
Requiem/Bolero playback and PCM, held-note safety, cancellation and cross-game
arrival, zero/one/two owl cases, ordinary native songs and six negative cases.
Final APK spot checks follow.

Final-candidate song spot checks passed with native diagnostic events and live
screenshots: MM Requiem recognizes, performs, opens the native confirmation,
cancels with B, then on replay reaches Desert Colossus with A despite no OoT
instrument. OoT Soaring recognizes and performs with zero owls, shows the
native no-destination textbox, and dismisses without departure. Neither
successful route enters the invalid-song path. Both final captures contain
nonzero 44.1 kHz stereo guest PCM. Longer automated taps were needed after a
short-tap attempt missed notes; that attempt is not counted as a pass.

The actual Android TouchOverlay passed **58** checks using real MotionEvents
and isolated preferences, including invisible independent sticks, button
priority, release handling, Menu dragging/persistence and tracker input
priority. Its test pad records inputs; this instrumentation is separate from
native game testing.

Final-candidate display tests passed in live OoT gameplay and shared menus at
2280×1080 and 1440×1080, both undocked and with the tracker side panel. The
complete native HUD remains visible. All ten post-edit snapshots preserve the
same manual 4:3 keys. Live internal-resolution readouts change from 1234×926
Manual to 1646×926 Balanced at the wider size, and 1440×926 Balanced at the
narrower size; returning to Manual restores 1234×926 at either size. This tests
actual UI selections and rendering, not merely saved configuration values.
Touch buttons fit after Android relayout. Controls overlapping the tracker are
intentionally suppressed so tracker touches do not control the game; Menu stays
accessible. First-boot selection has existing host coverage and was not
re-exercised in this final live resize session. MM HUD drag/reset is covered by
the earlier functionally identical candidate.

Cleanup completed: display-size override reset, app force-stopped, five original
baseline files byte-verified in both the visible and internal test storage
locations, and the dedicated disposable emulator stopped. No emulator device or
matching host process remains. Private ROMs, seeds, saves, screenshots, geometry,
audio captures and diagnostic logs are excluded from the delivered artifacts.

## Scope and phone checks

The one-chick Bremen alternative and automatic balanced/full-width/4:3 fitting
were implemented and tested in 0.1.18 and are retained. The shared HUD editor is
new in this release. Faster marching is not also added because the selected
request allowed the one-chick alternative.

Eighteen audited capabilities and 199 actual techniques (197 OoT, two MM) are
covered. FD sword, measured shallow water and magical Big Octo beams are modeled;
arbitrary larger jumps are not. Safe age propagation covers audited overworld
areas and the Well, not arbitrary dungeon rooms. The OoTMM reference is pinned
and documented in [the comparison](OOTMM-LOGIC-AUDIT.md).

Physical phone speakers, GPU behavior, cutouts/folds, hardware controllers,
two-device multiplayer and a full seed playthrough still need user testing.
Previously saved explicit Seen records are preserved; the new fixture checks
start unknown and prove subsequent observation behavior, not retroactive repair
of every older knowledge record.
Emulator audio capture proves generated PCM and song timing, not the latency
of a physical phone's speakers. The five-minute reminder has model coverage;
the final cancellation check does not pretend to wait five minutes.

Use the eight [phone checks](FIXES-0.1.21.md#phone-checks) to report remaining
device-specific problems.
