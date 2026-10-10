#pragma once
#include "randomizerTypes.h"

namespace Rando {
inline bool IsKokiriInvisibleRupee(RandomizerCheck rc) {
    return rc == RC_KF_SOUTH_GRASS_WEST_RUPEE || rc == RC_KF_NORTH_GRASS_WEST_RUPEE ||
           rc == RC_KF_NORTH_GRASS_EAST_RUPEE || rc == RC_KF_SOUTH_GRASS_EAST_RUPEE;
}
inline RandomizerCheckType KokiriEffectiveType(RandomizerCheck rc, RandomizerCheckType nativeType, int revision) {
    return revision == 1 && IsKokiriInvisibleRupee(rc) ? RCTYPE_WONDER_ITEM : nativeType;
}
// These four physical actors stay EnItem00. Select their option through the same effective
// family used by the pool, native fixed-item counts, saved tracker and oracle metadata.
inline bool KokiriItem00Enabled(RandomizerCheck rc, int revision, int freestanding, int wonder) {
    const auto type = KokiriEffectiveType(rc, RCTYPE_FREESTANDING, revision);
    if (type == RCTYPE_WONDER_ITEM)
        return wonder == RO_SHUFFLE_WONDER_ITEMS_OVERWORLD || wonder == RO_SHUFFLE_WONDER_ITEMS_ALL;
    return freestanding == RO_SHUFFLE_FREESTANDING_OVERWORLD || freestanding == RO_SHUFFLE_FREESTANDING_ALL;
}
} // namespace Rando
