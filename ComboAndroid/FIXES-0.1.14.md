# Combo Ship 0.1.14 - combined multiplayer progress

Install `ComboShip-Android-0.1.14-dev-arm64.apk` over the existing app. It uses the same application ID and signing certificate, so saves and preferences are retained.

## What changed

- A teammate now receives **both OoT and MM progress** in one synchronization response. Previously, MM could not answer while running in the background, and the server's single team cache could replace one game's state with the other's.
- The standard Anchor server stores one combined snapshot. Rejoining can recover both inventories and check states, including when the other player has left. Pending item messages retain their destination game.
- Item messages carry duplicate protection, and cross-game checks retain their delivery history. Each game's duplicate records are saved with that game's inventory. Repeated messages and cached messages should not award extra items.
- Room sharing settings now reach MM while either game is active. Progress messages from a different combined seed are rejected. The multiplayer roster uses the same combined seed identifier for both games.
- Snapshot merging preserves health upgrades, heart-piece progress and MM equipment selections. Save synchronization is applied on the game thread; broken or partial socket writes trigger a reconnect.

## Playing together

Every player must install **this 0.1.14 build**, load the same combined randomizer seed, and use the same server, room and team. Enable item/progress sharing in the room settings. Use **Tools > Multiplayer** to connect. The Anchor server does not need a protocol modification.

Older Combo Ship builds and standalone Ship/2S2H clients are not compatible with this combined progress format. Seeing an older client in the roster does not mean shared progress will work. Use separate rooms for different seeds. Save in-game before closing the app; multiplayer is not a replacement for a save backup.

## Please test on your devices

1. Connect two updated clients to the same room/team with the same seed. Confirm both appear in the roster.
2. Collect an OoT reward and an MM reward. Confirm the other player receives each reward and the matching check is marked collected.
3. Put one player in OoT and the other in MM. Collect rewards for the other world and verify them after switching worlds.
4. Disconnect one player, collect a reward with the other, then reconnect. Verify both worlds' progress catches up and rewards are not duplicated.
5. Save, fully close the app, and reopen it. Check inventory and tracker progress again.

Verification details and remaining coverage are recorded in `VALIDATION.md`. This repair targets shared-progress co-op. It does not complete the upstream MM PvP receiver or establish parity for every scene flag, puzzle, cycle-reset behavior, or multiplayer option. Physical phones, Internet connections and a full seed playthrough still require testing.

ROMs, extracted game data and private test saves are excluded from the APK and source package.

## Verification

Native and Android builds, lint (zero errors), protocol regression, signature, APK packaging and source replay checks passed. Two local Android clients exercised both games' reward receipt, both cross-game directions, save/reload, server restart and duplicate suppression. Cached joins and offline reward queues also passed during this repair; VALIDATION.md distinguishes the earlier candidates from the final save-journal build. Original test data was restored. Physical-phone and Internet testing remain on the checklist.

APK: 56,795,687 bytes. SHA-256: `81b8d078ad34149328e28c746e13347d090f9b9db2d54c9fe0a051a10141eb68`.
