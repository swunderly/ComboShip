#pragma once
// Pinned macros_mm.yml is_child/is_adult/is_tall/is_short. Age0adult,1child.
namespace ComboRando::MmAgeLogic {
inline bool IsChild(bool crossAge, int exploredAge, int startingAge, bool adultMaskOwned) {
    return crossAge ? exploredAge == 1 : startingAge == 1 || adultMaskOwned;
}
inline bool IsAdult(bool crossAge, int exploredAge, int startingAge, bool adultMaskOwned) {
    return crossAge ? exploredAge == 0 : startingAge == 0 || adultMaskOwned;
}
inline bool IsTall(bool zoraMask, bool adult) {
    return zoraMask || adult;
}
inline bool IsShort(bool dekuMask, bool child) {
    return dekuMask || child;
}
} // namespace ComboRando::MmAgeLogic
