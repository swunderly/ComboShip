#pragma once
#include "SilverRupees.h"
#include "soh/Enhancements/randomizer/location_access.h"
// OoTMM MIT behavior, revision 41a2b7c0d7dbbb53fed928652f8271bc336c113f.
// Full upstream notice: silver_rupees/LICENSE.
namespace Rando::SilverLogic {
inline bool Enabled() { return ctx->GetOption(RSK_SHUFFLE_SILVER_RUPEES).Get() != 0; }
inline bool Puzzle(RandomizerGet item) {
    const int slot = SilverRupees::PuzzleForItem(item);
    return slot >= 0 && SilverRupees::HasPuzzle(logic.get(), static_cast<size_t>(slot));
}
inline bool Gate(RandomizerGet item, bool vanilla) {
    return Enabled() ? Puzzle(item) : vanilla;
}
// One virtual region owns each physical check. Incoming exits retain every
// supported source alternative, with the solver's normal age/time evaluation.
inline void RegisterRoutes(SceneID scene) {
#define SR_VIRTUAL(region, check, checkScene, name) \
    if (scene == checkScene) areaTable[region] = Region(name, checkScene, {}, { LOCATION(check, true) }, {});
#define SR_ROUTE(parent, region, predicate)
#define SR_EVENT(parent, event, predicate)
#include "SilverRoutes.inc"
#undef SR_VIRTUAL
#undef SR_ROUTE
#undef SR_EVENT
#define SR_VIRTUAL(region, check, checkScene, name)
#define SR_ROUTE(parent, region, predicate) \
    if (areaTable[parent].scene == scene) areaTable[parent].AddExit(parent, region, [] { return Enabled() && (predicate); }, #predicate);
#define SR_EVENT(parent, event, predicate) \
    if (areaTable[parent].scene == scene) areaTable[parent].events.push_back(EVENT_ACCESS(event, Enabled() && (predicate)));
#include "SilverRoutes.inc"
#undef SR_VIRTUAL
#undef SR_ROUTE
#undef SR_EVENT
}
}
