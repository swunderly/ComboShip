# Combo Ship 0.2.19

Cross-game song travel retains the native warp presentation polished in 0.2.18: MM's Soaring map, wings/feathers and owl arrival when travelling from OoT; OoT's song fanfare, colored departure particles and native arrival when travelling from MM. The applicable fast song playback and native departure-skip settings also apply to these routes. Both full directions and adult fast/skipped travel passed the 0.2.19 emulator regression checks. Phone sound and pacing remain user checks.

New seed features adapted from OoTMM:

- **OoT silver rupees and pouches:** Vanilla, Own Dungeon or Anywhere placement, with puzzle pouches. Includes 80 vanilla and 74 Master Quest physical checks, persistent puzzle counters, native collection, foreign rewards and corresponding dungeon logic.
- **MM dungeon key rings:** Woodfall, Snowhead, Great Bay and Stone Tower. A ring grants only the dungeon keys not already found; duplicates do not refill spent keys. Placement and starting-item accounting retain the original key checks.
- **More victory conditions:** Both final bosses, Either final boss, Ganon only, Majora only, Triforce Hunt and Triforce Quest. Quest requires distinct Power, Courage and Wisdom pieces; duplicates cannot replace a missing piece. Generation, playthrough, hints and victory predicates use the chosen goal.
- **Seven additional shared families:** the five Ocarina buttons, Song of Time and nonprogressive Mirror Shield. Button sharing requires button shuffle in both games.

The legacy MM save contract remains compatible: old option arrays expand with disabled new defaults, existing item/check IDs stay stable, and the native save compatibility version remains separate from the Android app version. An actual MM save and cold reload preserved legacy inventory, songs, completed checks, seen rewards, goal and shared rules.

This completes requested items 1, 2 and 12 in the release source, with item 9 partially implemented. Full MM/cross-game entrance shuffle, expanded one-way/Wallmaster pools, cross-game Farore's Wind, actual Adult Link gameplay in MM, expanded souls, and the remaining cross-game equipment/sharing families are **not included**. Isolated future drafts are not delivered features. The release does not establish full feature or logic parity with every OoTMM setting.

New randomizer item/goal settings apply to newly generated seeds. Install this signed update over the existing app; no uninstall or fresh save is required. See VALIDATION-0.2.19.md for test scope and phone checks.
