#pragma once

#include <imgui.h>
#include <algorithm>
#include <limits>
#include <type_traits>

// Both save editors use this adapter for bounded values. Work in an int local so
// a write never exceeds the field's size, and only write after a user edit. The
// range is also limited to the field's type, so a stored value cannot wrap.
namespace ComboTouch {
template <typename T>
inline bool Count(const char* label, T* value, int minimum, int maximum, const char* format = "%d") {
    static_assert(std::is_integral_v<T> && sizeof(T) <= sizeof(int));
    minimum = static_cast<int>(std::max<long long>(minimum, std::numeric_limits<T>::min()));
    maximum = static_cast<int>(std::min<long long>(maximum, std::numeric_limits<T>::max()));
    int edited = static_cast<int>(*value);
    if (!ImGui::SliderInt(label, &edited, minimum, maximum, format, ImGuiSliderFlags_NoInput))
        return false;
    *value = static_cast<T>(std::clamp(edited, minimum, maximum));
    return true;
}
} // namespace ComboTouch
