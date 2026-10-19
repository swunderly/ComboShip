# Master Quest data from OoTMM

This folder holds the Master Quest (MQ) dungeon data that [OoTMM](https://github.com/OoTMM/OoTMM) uses to turn the vanilla Ocarina of Time dungeons into their MQ versions, and a decoder for it. This data lets MQ dungeons be played from the vanilla OoT ROM alone, as in OoTMM, without an MQ ROM.

**Status:** the scene and room hooks of OoTMM's runtime code (`mq.c`) are ported, so a seed that records this data plays its MQ dungeons' actor, object, door and path lists and collision patches. Its minimap and pause-map chest marks are OoTMM's too. Where this data can be played, the generator and both menus offer SoH's Master Quest options: a new seed rolls its MQ dungeons from its own master seed and records them, and a reload must roll the same ones.

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
- [MqMode.h](MqMode.h) decides, per dungeon scene, whether it plays as MQ and where its data comes from: a real `oot-mq.o2r`, this bundled data, or nowhere. The bundled data is used only over an NTSC-U 1.0 vanilla ROM, and only for a seed that records it. [MqSeedMarker.h](MqSeedMarker.h) reads and writes that per-seed and per-save record (`"mqDataSource": "ootmm-32.3"` with the seed's MQ dungeons, by id and by name). Seeds and saves without it keep SoH's behaviour. SoH's `ResourceMgr_IsSceneMasterQuest` answers from `MqMode.h`; scene and texture paths ask `ResourceMgr_SceneUsesMqAssets`, which is false for bundled seeds. `settings.cpp` offers the MQ options when `ResourceMgr_GameCanPlayMasterQuest` (a real `oot-mq.o2r` or this data) and keeps a bundled seed's MQ dungeons when finalizing; it rolls them on their own stream, seeded by `MqRollSeed` from the seed's master seed, so the generation, its oracle, a reload and a headless replay agree. The profile catalogue publishes the gate as `mqPlayable`, with `mqSource`.
- [MqRuntime.h](MqRuntime.h) is `mq.c`'s scene and room hooks as pure code: which commands of a vanilla scene or room header the MQ lists replace, the room and scene buffers, and the collision patcher. `soh/soh/Enhancements/randomizer/ComboMqRuntime.cpp` runs them in SoH's command runner on owned copies, never on the cached scene resources.
- [MqMaps.h](MqMaps.h) is `mq.c`'s map-mark hooks as pure code: the minimap block over a copy of the dungeon's vanilla marks, and `mq_maps.bin` as the pause-map table. `soh/soh/Enhancements/randomizer/ComboMqMaps.cpp` fills separate tables in SoH's map-mark data files once, and `MapMark_Init` and `PauseMapMark_Init` pick them for a bundled MQ dungeon. Vanilla dungeons and dungeons from a real `oot-mq.o2r` keep SoH's tables.
- Host tests: `ComboAndroid/tests/combo_mq_data_tests.cpp` checks the counts, the fixes above and the refusal of damaged copies. `ComboAndroid/tests/mq_bins_reference_tests.py` checks the pins and compares the decoder with an independent reader (`mq_bins.py`). `mq_runtime_plan_tests.cpp` checks the runtime against the bins and, through `fixtures/mq-vanilla-command-ids.txt` (only the command ids of the vanilla scene and room headers), against the vanilla headers. `mq_runtime_archive_tests.py` checks it and that fixture against the user's vanilla archive. `mq_runtime_wiring_tests.py` runs the real `ComboMqRuntime.cpp` in SoH's command runner. `mq_map_marks_tests.py` checks the map-mark tables against the bins and SoH's own tables. `mq_generation_settings_tests.py` runs the production settings gates, profile policy, catalogue and MQ roll in every install state and checks that a reload rolls a seed's recorded MQ dungeons.

## Updating

Copy the files only from an OoTMM release, unchanged. Then update the pins in `ComboMq.cmake`, `ComboAndroid/tests/mq_bins_reference_tests.py` and this README, and the counts in the tests.
