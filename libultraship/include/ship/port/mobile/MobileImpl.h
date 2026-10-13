#pragma once

#include <cstdint>
#include <string>

#include <imgui.h>

namespace Ship {

class Mobile {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
#ifdef __ANDROID__
    static void RequestMenu();
    static bool ConsumeMenuRequest();
    // ComboShip: Android Back, which steps back inside the open menu before it toggles it (Gui::SetMenuBackHandler).
    static void RequestBack();
    static bool ConsumeBackRequest();
    static void UpdateMenuState(bool visible);
    static bool IsMenuVisible();
    static void RequestMenuScale(float scale);
    static void ApplyMenuScale();
#endif
};
}; // namespace Ship
