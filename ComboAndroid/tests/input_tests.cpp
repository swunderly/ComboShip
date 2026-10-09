#include "InputState.h"
#include <atomic>
#include <cassert>
#include <limits>
#include <thread>
using namespace comboandroid;
int main() {
    InputState input;
    input.set(0xffffffffU, {2, -2, std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(), 1, 0});
    auto sample = input.snapshot();
    assert(sample.buttons == 0x001fffffU);
    assert(sample.axes[0] == 1 && sample.axes[1] == -1);
    assert(sample.axes[2] == 0 && sample.axes[3] == 0);
    assert(axisToSdl(-1) == -32768 && axisToSdl(1) == 32767);
    assert(axisToSdl(std::numeric_limits<float>::quiet_NaN()) == 0);
    input.releaseAll();
    std::atomic<bool> done{false};
    std::thread writer([&] {
        for (int i = 0; i < 100000; ++i) {
            input.set(1, {1, 1, 1, 1, 1, 1});
            input.set(2, {-1, -1, -1, -1, 0, 0});
        }
        done = true;
    });
    do {
        sample = input.snapshot();
        const float expected = sample.buttons == 1 ? 1.f : sample.buttons == 2 ? -1.f : 0.f;
        for (int i = 0; i < 4; ++i) assert(sample.axes[i] == expected);
        assert(sample.axes[4] == (sample.buttons == 1 ? 1.f : 0.f));
    } while (!done);
    writer.join();
    input.releaseAll();
    sample = input.snapshot();
    assert(sample.buttons == 0);
    for (float axis : sample.axes) assert(axis == 0);
}
