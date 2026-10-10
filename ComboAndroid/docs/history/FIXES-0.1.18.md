# Combo Ship 0.1.18 — saved gameplay rules and screen fitting

Install this APK over the existing Combo Ship app. It uses the same package and signing certificate. Existing saves, placements and collected checks are preserved. This is the first delivery from the larger authorized patch list; it does not complete the whole list.

## Included

- **Shared Gameplay rules page**, available under both Randomizer and Gameplay. Song of Time age change, Fierce Deity Anywhere and Bremen Mask: one chick use the same underlying settings in both places.
- **Rules saved with seeds and runs.** Newly generated combined seeds record those three enhancement rules. Each save restores its own choices, with intentional mid-run overrides stored separately from the original seed rules. Older saves without these fields adopt the current choices when first loaded. This extends the existing saved randomizer configuration; it is not yet a complete audit of every enhancement or trick.
- **Small mid-run warning.** Changing a supported gameplay rule or an exposed randomizer setting while a game is loaded briefly warns that the change could affect completion. The change is allowed immediately, without a validity check or regeneration requirement. Save in-game to keep the three added gameplay-rule overrides for that run. Ordinary generator options still apply to new seeds, not existing placements.
- **Bottom of the Well logic.** Adult access to the drained well can lead to Child access through Song of Time age transformation, without Song of Storms, when the selected transformation mode and required song, ocarina and equipment permit it. The shared search used by generation and OoT tracker reachability now includes this route. Other new age-change shortcuts are not yet added.
- **Fierce Deity sword logic.** Outside boss-room exceptions, MM's common sword requirement now checks the Anywhere setting as well as mask ownership. Owning the mask with Anywhere disabled no longer supplies an out-of-boss sword. This patch does not yet add Fierce Deity jump/gap shortcuts.
- **One-chick Bremen quest.** Enable this on the Gameplay rules page, then recruit one chick while marching. Its normal growth animation completes Grog's flock and enables the randomized reward. The mask and access to the Cucco Shack remain required. This implements the requested shorter quest option, not faster marching.
- **Settings → Screen & HUD.** Automatic balanced fitting preserves the original 4:3 view and adds side visibility up to 16:9. Full width allows more side visibility; Original 4:3 and Manual remain available. Narrow game panels use borders instead of cropping. Fitting updates when the viewport changes, including the tracker panel. Android cutout/gesture insets are applied to the game surface and touch overlay together. Existing advanced-resolution overrides stay manual.
- **Discoverable HUD editing.** Screen & HUD exposes each game's existing position/size controls from one page, with Playing now and explicit world choices. The two underlying HUD editors are not yet identical. Touch-controller placement is still accessed by holding Menu.
- **More sliders.** OoT's maximum health, rupees and Gold Skulltula tokens, plus combined Triforce-hunt counts, now use bounded sliders without mandatory text entry. This is a first pass, not a replacement of every numeric field.
- **MM tracker knowledge sources.** Newly recorded visual observations use Seen; explicit item-reveal dialogue uses Hinted. Save files and multiplayer knowledge messages retain the source. Older observations, and OoT records whose source is ambiguous, display Known rather than claiming a visual observation. Previously saved false observations are not erased.

## Still on the list

Cross-game song travel and shuffled destinations with actual logic routes; the full progression-setting audit and Fierce Deity movement routes; a unified tricks catalogue; the stronger combined every-check/completion guarantee; boss/event path hints; and the remaining slider/HUD-editor consistency work. These features are not claimed by this APK. See the full status table in [NEXT-PATCH-BACKLOG.md](NEXT-PATCH-BACKLOG.md).

## Check on your phone

1. Load your existing run. Confirm its progress, seed and collected checks remain intact. On an older save, choose the gameplay rules you want once, then save in-game.
2. Change a rule in Randomizer → Gameplay rules. Confirm the short warning appears, the change is immediate, and Gameplay → Gameplay rules shows the same value. Save and reload to check persistence.
3. Try Settings → Screen & HUD in your usual screen/fold layout. Check hearts, magic, rupees, action buttons and text at the edges, with the tracker both open and closed. Compare Balanced and Full width; use Manual if you prefer your previous framing. Try a HUD position control and its reset.
4. Enable Bremen Mask: one chick and complete Grog's check by recruiting one chick while marching.
5. With the appropriate Song of Time age-change mode, song and equipment, try the Adult-well-to-Child route without Song of Storms and compare tracker access. Fierce Deity extra jumps are still outside the new logic coverage.
6. For previously unknown MM checks, compare a visible item with a reward explicitly revealed by dialogue. Expect Seen versus Hinted, without marking either Collected. Older entries may say Known.

## Validation boundary

Native ARM64 compilation, APK assembly and Android lint passed. Host regressions cover rule persistence, malformed imports, well prerequisites, Fierce Deity sword gating, Bremen recruitment safety, screen geometry, tracker knowledge and multiplayer merging. Emulator tests exercised a combined generation, per-run rule restoration, the warning and synchronized menus, one-chick Grog reward collection, HUD-editor access and resized views. See [VALIDATION.md](VALIDATION.md) for exact evidence and limits. This is not a full-seed playthrough or physical-phone validation; acoustic latency and two-device multiplayer were not retested for this patch. ROMs and private test data are excluded from delivery.
