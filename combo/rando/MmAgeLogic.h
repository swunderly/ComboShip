#pragma once
// OoTMM v32.3 macros_mm.yml:59-62 is_child/is_adult/is_tall/is_short. Age 0 adult, 1 child. Only Cross-Games
// Age makes Link adult in MM; v32.3 has no MM starting age and no Adult Mask (those were 41a2b7c's rules).
namespace ComboRando::MmAgeLogic {
inline bool IsChild(bool crossAge, int exploredAge) {
    return !crossAge || exploredAge == 1;
}
inline bool IsAdult(bool crossAge, int exploredAge) {
    return crossAge && exploredAge == 0;
}
inline bool IsTall(bool zoraMask, bool adult) {
    return zoraMask || adult;
}
inline bool IsShort(bool dekuMask, bool child) {
    return dekuMask || child;
}
} // namespace ComboRando::MmAgeLogic
