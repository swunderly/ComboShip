#pragma once
#include "Rando/Rando.h"
#include <algorithm>
#include "MissingMmSoulsState.h"
namespace Rando {
// Complete MM souls (Igos as a boss, Bio Baba, Floormaster, Flying Pot, Real Bombchu, Skullwalltula) is
// unreleased: RO_MM_COMPLETE_SOULS has no CVar and stays off, so every path keyed on it is dormant and
// seeds keep the legacy Igos enemy soul. Its item and option ids stay for save compatibility.
inline constexpr RandoItemId missingMmSoulItems[] = { RI_SOUL_BOSS_IGOS,          RI_SOUL_ENEMY_BIO_BABA,
                                                      RI_SOUL_ENEMY_FLOORMASTER,  RI_SOUL_ENEMY_FLYING_POT,
                                                      RI_SOUL_ENEMY_REAL_BOMBCHU, RI_SOUL_ENEMY_SKULLWALLTULA };
inline int MissingMmSoulIndex(RandoItemId item) {
    for (unsigned i = 0; i < 6; ++i)
        if (missingMmSoulItems[i] == item)
            return i;
    return -1;
}
// Exact pinned mm/actors.c identity split, including gold-vs-wall Skulltulas.
inline int MissingMmSoulForActor(int actor, unsigned params) {
    switch (actor) {
        case ACTOR_EN_KNIGHT:
        case ACTOR_BOSS_06:
            return 0;
        case ACTOR_BOSS_05:
            return 1;
        case ACTOR_EN_FLOORMAS:
            return 2;
        case ACTOR_EN_TUBO_TRAP:
            return 3;
        case ACTOR_EN_RAT:
            return 4;
        case ACTOR_EN_SW:
            return (params & 3) ? -1 : 5;
        default:
            return -1;
    }
}
inline int EvaluateMissingMmSoulPreSpawn(int actor, unsigned params, bool enabled, bool credits, bool boss,
                                         bool enemies, uint8_t owned) {
    if (!enabled || credits)
        return 1;
    const int index = MissingMmSoulForActor(actor, params);
    if (index < 0 || !(index ? enemies : boss) || MissingMmSoulOwned(owned, index))
        return 1;
    return index == 3 ? -1 : 0;
}

template <class Options> inline bool MissingMmSoulShuffled(const Options& options, unsigned index) {
    return index < 6 && options[RO_MM_COMPLETE_SOULS] &&
           options[index ? RO_SHUFFLE_ENEMY_SOULS : RO_SHUFFLE_BOSS_SOULS];
}
template <class Options> inline void AddMissingMmSoulPool(const Options& options, std::vector<RandoItemId>& pool) {
    if (!options[RO_MM_COMPLETE_SOULS])
        return;
    // With complete souls Igos moves to the boss family, so the legacy enemy soul leaves the pool.
    pool.erase(std::remove(pool.begin(), pool.end(), RI_SOUL_ENEMY_IGOS_DU_IKANA), pool.end());
    for (unsigned i = 0; i < 6; ++i)
        if (MissingMmSoulShuffled(options, i))
            pool.push_back(missingMmSoulItems[i]);
}
template <class Options>
inline void AddUnshuffledMissingMmSouls(const Options& options, std::vector<RandoItemId>& starts) {
    if (!options[RO_MM_COMPLETE_SOULS])
        return;
    // With complete souls the boss identity replaces the legacy Igos starter.
    starts.erase(std::remove(starts.begin(), starts.end(), RI_SOUL_ENEMY_IGOS_DU_IKANA), starts.end());
    for (unsigned i = 0; i < 6; ++i)
        if (!MissingMmSoulShuffled(options, i))
            starts.push_back(missingMmSoulItems[i]);
}
bool HaveMissingMmSoul(unsigned index);
bool GrantMissingMmSoul(RandoItemId item);
bool RemoveMissingMmSoul(RandoItemId item);
} // namespace Rando
