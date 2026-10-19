#pragma once
// ComboShip: OoTMM v32.3's Master Quest minimap and pause-map marks (src/oot/mq.c:439-517) for a dungeon that is MQ
// from the bundled data (ResourceMgr_GameUsesBundledMq, combo/mq/MqMode.h). OoTMM writes the MQ marks over the vanilla
// tables of the overlays it has just loaded; SoH's tables are compiled in, so z_map_mark_data.c and z_lmap_mark_data.c
// keep separate tables filled once from the bins (combo/mq/MqMaps.h), and z_map_mark.c and z_lmap_mark.c pick them.
// Vanilla dungeons and dungeons that are MQ from a real oot-mq.o2r keep SoH's tables.
#ifdef COMBO_BUILD
#include <z64.h>

#ifdef __cplusplus
extern "C" {
#endif

// ComboMqMaps.cpp: the bundled MQ block of OoTMM dungeon `dungeon` (0-9, the scene id) over the first entries of
// `table`, `rooms` entries that already hold that dungeon's vanilla marks (LoadMapMarkWrapper, mq.c:479-515). Returns
// false and writes nothing when the bundled data is missing or the block does not fit.
bool ComboMq_OverlayMinimapMarks(s32 dungeon, MapMarkData* table, s32 rooms);
// ComboMqMaps.cpp: mq_maps.bin as the whole pause-map table of `floors` (34) floors, its marks drawn with `vtx`
// (comboMqKaleidoHook, mq.c:452-477). Returns false and writes nothing when the bundled data is missing or the table
// has another size.
bool ComboMq_FillPauseMapMarks(PauseMapMarksData* table, s32 floors, const Vtx* vtx);

// z_map_mark_data.c: the minimap marks of the ten dungeons as OoTMM shows them in MQ, filled on first use; NULL when
// they cannot be filled.
MapMarkData** ComboMq_GetMapMarkDataTable(void);
// z_lmap_mark_data.c: the pause-map marks of the 34 MQ floors as OoTMM shows them, filled on first use; NULL when they
// cannot be filled.
PauseMapMarksData* ComboMq_GetPauseMapMarkDataTable(void);

#ifdef __cplusplus
}
#endif
#endif
