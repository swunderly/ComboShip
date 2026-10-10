# Roadmap

What is authorized, what is not, and the order the remaining work will be built in. Released changes are in [CHANGELOG.md](CHANGELOG.md).

## Rules for all feature work

- Copy OoTMM's logic, setting names, descriptions and behaviour (reference: OoTMM v32.3). Reuse native assets and animations from either game. Do not invent logic, text or animations.
- A setting that affects logic lives in the Randomizer menu. Turning it on there checks and greys out the matching enhancement, which can also be used alone without affecting logic.
- Old saves must keep loading. Old seeds do not need to be repaired.
- Every release lists what is finished, partial and remaining, with short phone checks. Build checks are not phone tests.

## Remaining authorized work (October 7 list)

Done earlier: silver rupees and pouches, MM key rings, victory conditions, MM time-of-day items, cross-warp native animation and audio, MM NPC/animal/misc souls, Farore's Wind in MM.

Planned order (each step ships when it is complete and tested):

1. **Cleanup fixes** from the October 10 code review, plus a working one-command test suite.
2. **Farore's Wind draw fix**: an MM Farore's Wind placed in OoT currently draws as a blue rupee.
3. **OoT enemy souls** (OoTMM `soulsEnemyOot`, "Enemy Souls (OoT)"). The 43 souls already exist but are unreachable; this publishes the setting and fixes the room-clear, boss and pool differences from OoTMM.
4. **Din's Fire and Nayru's Love in MM** (`spellFireMm`, `spellLoveMm`, shared variants), after re-auditing the MM logic branches they switch on.
5. **Shared MM player groundwork** for adult Link and MM gear: one age check, one gear check, and saves that never refuse to load because of adult or gear state.
6. **Iron Boots, Hover Boots, Goron and Zora Tunics in MM** (`bootsIronMm`, `bootsHoverMm`, `tunicGoronMm`, `tunicZoraMm`, shared variants).
7. **Adult Link in MM** (OoTMM `crossAge`, "Cross-Games Age"). Needs an owner decision on OoTMM's adult-Link model files for MM (neither game has one natively).
8. **Entrance randomization**: Wallmaster and one-way pools, then own-game pools for both games, MM extras, cross-game ("Full") pools, and boss entrances, following OoTMM's `er*` settings. Shipped in milestones.
9. **Parity check** of the finished APK's logic against OoTMM, comparing like features only; mismatches that are bugs get fixed.

## Not authorized (October 5 list)

Song-event shuffle, song-note shuffle, custom access conditions and coins, more MM dungeon-item controls, extra item upgrades.
