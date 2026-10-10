#pragma once
#include "rando/entrances/MmGrottoNative.h"
#include "rando/entrances/NativeArrival.h"
namespace ComboRando::Entrances {
struct MmMixedMarkedSource {
    uint32_t logicalId;
    int previousScene;
};
class MmMixedMarkedTransition {
    const void* owner = nullptr;
    bool marked = false;
    int file = -1;
    uint16_t native = 0;
    MmMixedMarkedSource source{};

  public:
    bool pending() const {
        return marked;
    }
    void clear() {
        owner = nullptr;
        marked = false;
        file = -1;
        native = 0;
        source = {};
    }
    void mark(const void* play, int slot, uint32_t logical, uint16_t expectedNative, int previousScene) {
        clear();
        marked = true;
        owner = play;
        file = slot;
        native = expectedNative;
        source = { logical, previousScene };
    }
    std::optional<MmMixedMarkedSource> take(const void* play, int slot, uint16_t expectedNative) {
        const auto o = owner;
        const auto f = file;
        const auto n = native;
        const auto s = source;
        clear();
        if (!o || o != play || f < 0 || f > 2 || f != slot || n != expectedNative)
            return std::nullopt;
        return s;
    }
};
// Destination preparation is transactional: all fallible typed conversion precedes
// writes. Source actor collision/respawn setup stays native and is not reinterpreted.
template <class Native> struct PreparedMmMixedArrival {
    Native slots[4];
    MmPreviousScene previous;
    int32_t flag;
    int cutscene;
    uint16_t entrance;
};
template <class Native>
PreparedMmMixedArrival<Native> PrepareNativeMmMixedArrival(uint32_t destination, const Native* slots,
                                                           MmPreviousScene previous, int32_t flag, int cutscene,
                                                           bool wf, bool sh, bool gb, bool st) {
    PreparedMmMixedArrival<Native> out{};
    for (size_t i = 0; i < 4; ++i)
        out.slots[i] = slots[i];
    out.previous = previous;
    out.flag = flag;
    out.cutscene = cutscene;
    const auto native = ApplyNativeMmGrotto(Key{ Game::Mm, destination }, out.slots, out.previous, out.flag, sh);
    const auto fixed = FixMmNativeArrival(native, wf, sh, gb, st, out.cutscene);
    if (fixed > 0xffffu)
        throw std::invalid_argument("Unrepresentable MM mixed arrival");
    out.entrance = static_cast<uint16_t>(fixed);
    return out;
}
} // namespace ComboRando::Entrances
