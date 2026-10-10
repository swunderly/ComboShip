#pragma once
namespace ComboRando {
struct AgeTimeAccess {
    bool childDay, childNight, adultDay, adultNight;
};
inline bool ExpandSongAgeAccess(AgeTimeAccess& access, bool childCanChange, bool adultCanChange) {
    const auto before = access;
    if (childCanChange) {
        access.adultDay |= before.childDay;
        access.adultNight |= before.childNight;
    }
    if (adultCanChange) {
        access.childDay |= before.adultDay;
        access.childNight |= before.adultNight;
    }
    return access.childDay != before.childDay || access.childNight != before.childNight ||
           access.adultDay != before.adultDay || access.adultNight != before.adultNight;
}
} // namespace ComboRando
