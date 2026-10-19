#include "ComboMqMaps.h"
#ifdef COMBO_BUILD
#include "mq/MqMaps.h"

#include <cstddef>

// The bundled bins decode once (ComboMqData.cpp); a scene is only bundled-MQ when they did (MqMode.h), so the null
// checks here only keep a broken build from drawing anything.
extern "C" bool ComboMq_OverlayMinimapMarks(s32 dungeon, MapMarkData* table, s32 rooms) {
    const ComboMq::Data* data = ComboMq::Bundled();
    return data != nullptr && rooms >= 0 &&
           ComboMq::OverlayMinimapMarks(*data, dungeon, table, static_cast<size_t>(rooms));
}

extern "C" bool ComboMq_FillPauseMapMarks(PauseMapMarksData* table, s32 floors, const Vtx* vtx) {
    const ComboMq::Data* data = ComboMq::Bundled();
    return data != nullptr && floors >= 0 && ComboMq::FillPauseMapMarks(*data, table, static_cast<size_t>(floors), vtx);
}
#endif
