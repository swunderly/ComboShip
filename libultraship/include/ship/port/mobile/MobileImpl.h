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
    static void UpdateMenuState(bool visible);
    static bool IsMenuVisible();
    static void RequestMenuScale(float scale);
    static void ApplyMenuScale();
#endif
};
}; // namespace Ship
