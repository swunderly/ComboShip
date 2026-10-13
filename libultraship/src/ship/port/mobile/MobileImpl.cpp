#if defined(__ANDROID__) || defined(__IOS__)
#include "ship/port/mobile/MobileImpl.h"
#include <SDL2/SDL.h>

#include <imgui_internal.h>
#ifdef __ANDROID__
#include <atomic>
#include <algorithm>
#include <cmath>
#include "libultraship/bridge/consolevariablebridge.h"
#include "ship/Context.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"

namespace {
std::atomic<bool> menuRequested{false}, backRequested{false}, menuVisible{true};
std::atomic<float> requestedScale{0.0f};
}
void Ship::Mobile::RequestMenu() { menuRequested = true; }
bool Ship::Mobile::ConsumeMenuRequest() { return menuRequested.exchange(false); }
void Ship::Mobile::RequestBack() { backRequested = true; }
bool Ship::Mobile::ConsumeBackRequest() { return backRequested.exchange(false); }
void Ship::Mobile::UpdateMenuState(bool visible) { menuVisible = visible; }
bool Ship::Mobile::IsMenuVisible() { return menuVisible.load(); }
void Ship::Mobile::RequestMenuScale(float scale) {
    if (std::isfinite(scale)) requestedScale = std::clamp(scale, 1.0f, 3.0f);
}
void Ship::Mobile::ApplyMenuScale() {
    const float requested = requestedScale.exchange(0.0f);
    if (requested > 0.0f) {
        CVarSetFloat("gSettings.Menu.AndroidScale", requested);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    const float scale = std::clamp(CVarGetFloat("gSettings.Menu.AndroidScale", 1.45f), 1.0f, 3.0f);
    ImGuiStyle& style = ImGui::GetStyle();
    style.FramePadding = ImVec2(4.0f * scale, 6.0f * scale);
    style.ItemSpacing = ImVec2(8.0f * scale, 6.0f * scale);
    style.ItemInnerSpacing = ImVec2(4.0f * scale, 4.0f * scale);
    style.ScrollbarSize = 14.0f * scale;
    style.GrabMinSize = 12.0f * scale;
    ImGui::GetIO().FontGlobalScale = scale;
}
#endif

static bool isShowingVirtualKeyboard = true;

void Ship::Mobile::ImGuiProcessEvent(bool wantsTextInput) {
    ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetActiveID());

    if (wantsTextInput) {
        if (!isShowingVirtualKeyboard) {
#ifndef __ANDROID__
            if (state) state->ClearText();
#endif

            isShowingVirtualKeyboard = true;
            SDL_StartTextInput();
        }
    } else {
        if (isShowingVirtualKeyboard) {
            isShowingVirtualKeyboard = false;
            SDL_StopTextInput();
        }
    }
}
#endif
