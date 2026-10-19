# Master Quest data from OoTMM

This folder holds the Master Quest (MQ) dungeon data that [OoTMM](https://github.com/OoTMM/OoTMM) uses to turn the vanilla Ocarina of Time dungeons into their MQ versions, and a decoder for it. This data lets MQ dungeons be played from the vanilla OoT ROM alone, as in OoTMM, without an MQ ROM.

**Status:** the data is compiled in, but the game does not use it yet. The port of OoTMM's runtime code (`mq.c`) is still in progress.

## The files

`data/` holds three files copied unchanged from OoTMM v32.3 (tag `v32.3`, commit `413573d22d1d53e8a81931a70927f625b73488b9`), `packages/generator/data/static/`:

| File | Bytes | SHA-256 | Contents |
|---|---|---|---|
| `mq_rooms.bin` | 58896 | `3b3e1f836124f756560382e7ae033bfb7bd84570b2b1c5ea228655728cb6f636` | The actor and object lists of the 220 MQ rooms |
| `mq_scenes.bin` | 36352 | `e1b77a41950ef656b7fcf411b119ea5b9617dc3dac3aa1560faf3567881c9a49` | For each of the 12 MQ dungeons: transition actors (doors), paths, minimap marks and Fire Temple collision patches |
| `mq_maps.bin` | 16728 | `43f8f13fa1b8290f661dbc66d2e89cfcc6f59169e70ae5fa1ad629ac657bffaa` | The pause-map chest and boss marks of the 34 MQ dungeon floors |

**Where the data comes from.** OoTMM's `packages/generator/scripts/make_mq.ts` wrote these files from room files, scene files and two overlays extracted from a Master Quest ROM. So the files are data extracted from the MQ ROM: actor and object lists, door lists, paths, map marks and collision patches. They contain no models, textures, sound or code. OoTMM distributes them in its repository under the MIT license; a copy is in [LICENSE.OoTMM](LICENSE.OoTMM). The maintainer of this fork approved bundling them, with that notice, in this repository and in the app.

**Fixes that make_mq.ts bakes in** (all from OoTMM, line numbers in v32.3's `make_mq.ts`):

- Shadow Temple: Obj_Roomtimer params set to `0xffff` in room 7 (actor 4) and room 6 (actor 11) (lines 111-119).
- Fire Temple room `0x11`: objects `0x00cb` and `0x00dc` (object_gi_shield_1 and object_gi_shield_2, for the Like Like) appended to the object list (lines 131-139).
- Water Temple: transition actor `0x16`, an En_Door between rooms 15 and 12, gets params `0x0082` (lines 226-229).
- Fire Temple minimap, room 3: a chest mark (flag 0 at `0x48`, `0x3c`) and no second record (lines 285-294).
- Fire Temple collision: 553 polygon patches and 33 surface-type patches from `scripts/mqdata_fire.json` (lines 307-347).
- Fire Temple pause map, floor 17: a sixth chest, flag 0 at (46.5, -17.5) (lines 401-409).

Some MQ changes are not in the files. OoTMM makes them at runtime in `packages/generator/src/oot/mq.c`: removing six broken actors, the Ganon's Castle polygon patch and turning Ice Cavern's alternate-header command into its path list.

## How the build uses them

- [ComboMq.cmake](ComboMq.cmake) pins the three SHA-256 values. [CMake/ComboEmbedBinary.cmake](../../CMake/ComboEmbedBinary.cmake) compiles the files into the `soh` library as byte arrays and stops the configuration if a file does not match its pin. The Android app, the desktop build and the headless generator all get the same copy. The app needs no extra files at runtime.
- [ComboMqData.h](ComboMqData.h) and [ComboMqData.cpp](ComboMqData.cpp) decode the big-endian files once into native structs. The decoder checks every count, offset and size, and refuses a damaged copy. It follows the formats of `make_mq.ts` and `mq.c`; the comments in the header give the details.
- Host tests: `ComboAndroid/tests/combo_mq_data_tests.cpp` checks the counts, the fixes above and the refusal of damaged copies. `ComboAndroid/tests/mq_bins_reference_tests.py` checks the pins and compares the decoder with an independent reader (`mq_bins.py`).

## Updating

Copy the files only from an OoTMM release, unchanged. Then update the pins in `ComboMq.cmake`, `ComboAndroid/tests/mq_bins_reference_tests.py` and this README, and the counts in the tests.
