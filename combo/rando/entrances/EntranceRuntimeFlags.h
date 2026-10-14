// The ER-wide switches OoTMM derives from the entrance settings, at the pin (413573d):
// - isEntranceShuffle (packages/logic/src/helpers.ts:6-55) turns on the PG_ER patches
//   (packages/generator/lib/combo/patch-build/index.ts:58), among them mm/patches/hot_water.S, and
//   CFG_ER_ANY (packages/generator/lib/combo/randomizer/world-config.ts:30);
// - er_enabled (data/macros/macros_mm.yml:305) is the MM logic macro behind has_hot_water_er (:273).
// Both read only OoTMM's er* settings, never Combo Ship's Song Travel (M1 owner decision D5). One-way
// woods, water voids and Anywhere count in neither (tools/ootmm/m1-contract.json truthTables).
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "Shuffle.h"

namespace ComboRando::Entrances {
// helpers.ts:6-55, term by term in its order.
inline bool IsEntranceShuffle(const Settings& s) {
    return Active(s.erBoss) || s.erMajorDungeons || s.erMinorDungeons || s.erSpiderHouses || s.erPirateFortress ||
           s.erBeneathWell || s.erIkanaCastle || s.erSecretShrine || s.erGanonCastle || s.erGanonTower || s.erMoon ||
           Active(s.erRegions) || s.erIndoorsMajor || s.erIndoorsExtra || s.erIndoorsTelescopes ||
           s.erWarps != Warps::None || s.erOneWaysMajor || s.erOneWaysIkana || s.erOneWaysSongs || s.erOneWaysStatues ||
           s.erOneWaysOwls || Active(s.erGrottos) || Active(s.erWallmasters) || Active(s.erOverworld);
}
// macros_mm.yml:305, term by term in its order.
inline bool ErEnabled(const Settings& s) {
    return s.erMajorDungeons || s.erMinorDungeons || s.erSpiderHouses || s.erGanonCastle || s.erGanonTower ||
           s.erBeneathWell || s.erPirateFortress || s.erSecretShrine || s.erIkanaCastle || s.erMoon ||
           s.erIndoorsMajor || s.erIndoorsExtra || s.erIndoorsTelescopes || Active(s.erRegions) ||
           Active(s.erOverworld) || Active(s.erBoss) || s.erWarps != Warps::None || s.erOneWaysMajor ||
           s.erOneWaysIkana || s.erOneWaysSongs || s.erOneWaysStatues || s.erOneWaysOwls || Active(s.erGrottos) ||
           Active(s.erWallmasters);
}
} // namespace ComboRando::Entrances
