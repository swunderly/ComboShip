#pragma once

#include <imgui.h>
#include <algorithm>
#include <type_traits>

// Both save editors use this adapter for bounded values. Work in an int local
// so narrow save fields cannot overflow, and only write after a user edit.
namespace ComboTouch {
template <typename T>
inline bool Count(const char* label, T* value, int minimum, int maximum, const char* format = "%d") {
    static_assert(std::is_integral_v<T> && sizeof(T) <= sizeof(int));
    int edited = static_cast<int>(*value);
    if (!ImGui::SliderInt(label, &edited, minimum, maximum, format, ImGuiSliderFlags_NoInput)) return false;
    *value = static_cast<T>(std::clamp(edited, minimum, maximum));
    return true;
}
} // namespace ComboTouch
