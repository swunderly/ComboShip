#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <mutex>
namespace comboandroid {
struct PadSnapshot { uint32_t buttons = 0; std::array<float, 6> axes{}; };
class InputState {
public:
    void set(uint32_t buttons, const std::array<float, 6>& axes) {
        std::lock_guard<std::mutex> guard(mutex_);
        state_.buttons = buttons & 0x001fffffU;
        for (size_t i = 0; i != axes.size(); ++i) {
            const float value = std::isfinite(axes[i]) ? axes[i] : 0.f;
            state_.axes[i] = value < -1.f ? -1.f : value > 1.f ? 1.f : value;
        }
    }
    PadSnapshot snapshot() const { std::lock_guard<std::mutex> guard(mutex_); return state_; }
    void releaseAll() { set(0, {0, 0, 0, 0, 0, 0}); }
private:
    mutable std::mutex mutex_;
    PadSnapshot state_;
};
inline int16_t axisToSdl(float value) {
    if (!std::isfinite(value)) return 0;
    value = value < -1.f ? -1.f : value > 1.f ? 1.f : value;
    return static_cast<int16_t>(std::lround(value * (value < 0.f ? 32768.f : 32767.f)));
}
}
