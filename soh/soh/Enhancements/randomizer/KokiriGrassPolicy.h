#pragma once
#include "randomizerTypes.h"

namespace Rando {
// Revision 1 shares the two physical bushes across ages. Existing numeric check
// and RandomizerInf identities are unchanged; revision 0 keeps four rewards.
inline bool KokiriGrassCatalogCheckActive(RandomizerCheck rc, int revision) {
    return revision != 1 || (rc != RC_KF_ADULT_GRASS_19 && rc != RC_KF_ADULT_GRASS_20);
}
inline RandomizerCheck KokiriCanonicalGrassCheck(RandomizerCheck rc, int revision) {
    if (revision == 1) {
        if (rc == RC_KF_ADULT_GRASS_19) return RC_KF_CHILD_GRASS_5;
        if (rc == RC_KF_ADULT_GRASS_20) return RC_KF_CHILD_GRASS_6;
    }
    return rc;
}
inline bool KokiriSharedGrassAgeAllowed(bool child, bool adult, int revision) {
    return child || (revision == 1 && adult);
}
} // namespace Rando
