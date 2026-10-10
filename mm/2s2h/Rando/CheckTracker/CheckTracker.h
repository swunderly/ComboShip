#ifndef RANDO_CHECK_TRACKER_H
#define RANDO_CHECK_TRACKER_H

#include "Rando/Rando.h"
#include <ship/window/gui/GuiWindow.h>

namespace Rando {

namespace CheckTracker {

void Init();
void OnFileLoad();
#ifdef COMBO_BUILD
// Only call for a model or an unambiguous item name actually revealed by the game.
void RecordSeenItem(RandoCheckId check, RandoItemId shownItem, uint8_t source = 1);
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
