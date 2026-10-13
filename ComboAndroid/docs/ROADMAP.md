# Roadmap

What is authorized, what is not, and the order the remaining work will be built in. Released changes are in [CHANGELOG.md](CHANGELOG.md).

## Rules for all feature work

- Copy OoTMM's logic, setting names, descriptions and behaviour (reference: OoTMM v32.3). Reuse native assets and animations from either game. Do not invent logic, text or animations.
- A setting that affects logic lives in the Randomizer menu. Turning it on there checks and greys out the matching enhancement, which can also be used alone without affecting logic.
- Old saves must keep loading. Old seeds do not need to be repaired.
- Every release lists what is finished, partial and remaining, with short phone checks. Build checks are not phone tests.

## Remaining authorized work (October 7 list)

Done earlier: silver rupees and pouches, MM key rings, victory conditions, MM time-of-day items, cross-warp native animation and audio, MM NPC/animal/misc souls, Farore's Wind in MM; the October 10 code-review cleanup with a one-command test suite, and the Farore's Wind draw fix (0.3.10); OoT enemy souls, "Enemy Souls (OoT)" (0.4.0); Din's Fire and Nayru's Love in MM (0.4.1); the shared MM age and gear groundwork, Iron and Hover Boots, Goron and Zora Tunics in MM, and adult Link in MM, "Cross-Games Age" (0.4.3). See [CHANGELOG.md](CHANGELOG.md) for what each release includes and what is still untested.

Planned order (each step ships when it is complete and tested):

1. **Entrance randomization**: Wallmaster and one-way pools, then own-game pools for both games, MM extras, cross-game ("Full") pools, and boss entrances, following OoTMM's `er*` settings. Shipped in milestones.
2. **Parity check** of the finished APK's logic against OoTMM, comparing like features only; mismatches that are bugs get fixed.

## Not authorized (October 5 list)

Song-event shuffle, song-note shuffle, custom access conditions and coins, more MM dungeon-item controls, extra item upgrades.
