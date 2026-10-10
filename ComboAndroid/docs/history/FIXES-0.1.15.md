# Combo Ship Android 0.1.15 — MM seen checks

MM's check tracker now remembers revealed, uncollected rewards. Seeing a shuffled item model or reading dialogue that explicitly names a check's reward records it as **Seen**. Both games use the same **Seen: item name** row format; the reward is also available in the check's details and search.

- Covers visible freestanding items, shop models and other randomized reward models, plus explicit shop, merchant, fairy, minigame, bank-sign and exact-location hint dialogue.
- A seen check remains uncollected and does not raise the collected count. Skipping a check retains its known reward.
- The tracker remembers the native item tier/model shown. Disguised traps retain their apparent reward until collected. Foreign OoT items in MM use the foreign reward/disguise name.
- Hidden rewards, ambiguous area hints, off-camera models and failed foreign-model fallbacks do not reveal seed placements. This uses the actor's camera projection, not pixel-level occlusion testing.
- MM knowledge is stored with the game's ordinary save. Existing saves load normally with unknown checks left unknown; previously seen checks need to be revisited after updating. This does not force autosaves when an item is viewed.
- Multiplayer snapshots carry optional seen-check knowledge without changing the existing seven-field check table. An older or stale peer cannot clear local knowledge. Observations are shared on team-state synchronization, not as a new immediate per-frame packet.

Install over the existing app to keep saves and settings. The package ID and signing certificate are unchanged.

## Phone checklist

1. Look at an MM shop item or freestanding reward without taking it. Open the tracker: its row should show **Seen: [reward]**, while the collected total stays unchanged.
2. Read a merchant offer or an explicit Great Fairy reward hint. Confirm that reward appears in the corresponding tracker row.
3. Save normally, close and reopen the app, and check that the known rewards remain. An unrevealed chest should still have no reward name.
4. Collect a known reward. Confirm that its status becomes Collected (or it disappears if collected checks are hidden).
5. Toggle the tracker in both games with your existing shortcut. The shared layout and controls are unchanged apart from the inline seen reward name.

See VALIDATION.md for the completed build and runtime checks. A complete seed playthrough and physical-phone testing are not part of the local verification.

## Verification

Build, lint, signature, packaging, source replay and regression checks passed. Android gameplay verified visible model and dialogue observations, hidden-reward guards, trap disguises, uncollected totals and a real save/cold-reload/save round trip. See VALIDATION.md for scope and remaining device checks.
