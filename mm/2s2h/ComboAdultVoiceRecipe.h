#pragma once
#include <array>
#include <vector>
#include <cstdint>
namespace ComboAdultMM {
// Exact OOT_ADULT_EFFECTS from pinned custom/index.ts; preserves MM font0 and aligns foreign block64.
inline constexpr std::array<unsigned, 46> AdultVoiceEffects = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                                                12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                                                24, 25, 26, 27, 55, 56, 60, 61, 67, 77, 78, 79,
                                                                80, 81, 82, 83, 84, 85, 86, 87, 88, 134 };
// NOTE array extent must exactly match the pinned set; the contract rejects accidental padding.
template <class Effect>
bool BuildAdultVoiceEffects(const Effect* mm, size_t mmCount, const Effect* oot, size_t ootCount,
                            std::vector<Effect>& out, size_t& adultBase) {
    out.clear();
    adultBase = 0;
    if (!mm || !mmCount || !oot || ootCount <= 134)
        return false;
    adultBase = (mmCount + 63) & ~size_t(63);
    if (adultBase + AdultVoiceEffects.size() > 65535)
        return false;
    out.assign(mm, mm + mmCount);
    out.resize(adultBase, mm[0]);
    for (auto effect : AdultVoiceEffects)
        out.push_back(oot[effect]);
    return true;
}
} // namespace ComboAdultMM
