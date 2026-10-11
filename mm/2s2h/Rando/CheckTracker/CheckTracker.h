#ifndef RANDO_CHECK_TRACKER_H
#define RANDO_CHECK_TRACKER_H

#include "Rando/Rando.h"
#include <ship/window/gui/GuiWindow.h>
#include <unordered_map>

namespace Rando {

namespace CheckTracker {

void Init();
void OnFileLoad();
// The unobtained shuffled checks in logic from the save's current place and time, as the tracker shows them.
void ComputeChecksInLogic(std::unordered_map<RandoCheckId, bool>& inLogic);
#ifdef COMBO_BUILD
// Only call for a model or an unambiguous item name actually revealed by the game.
void RecordSeenItem(RandoCheckId check, RandoItemId shownItem, uint8_t source = 1);
// Whether RecordSeenItem(check, shownItem) would change anything; lets draw code skip its visibility test.
bool NeedsSighting(RandoCheckId check, RandoItemId shownItem);
// Recompute the checks in logic on the next tracker draw, without waiting for the refresh interval: something the
// logic reads changed outside a check (BenPort.cpp Combo_MM_SyncCrossAgeOnEntry changes Link's age).
void MarkChecksInLogicStale();
#endif

class CheckTrackerWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override{};
    void Draw() override;
    void UpdateElement() override{};
};

class SettingsWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};

} // namespace CheckTracker

} // namespace Rando

typedef enum { SCENE_LOAD, SCENE_UPDATE };

#endif // RANDO_CHECK_TRACKER_H
