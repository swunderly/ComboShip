#pragma once
#include "ExpandedSoulsPolicy.h"
#include "ExpandedSoulsOwnership.h"
namespace Rando {
struct ExpandedSoulsOptions {
    bool rando = false, npc = false, animal = false, misc = false;
};
// Bridge's native wrapper must pass this slot's real saved ownership/options and
// credits state. No globals, oracle inventory, or another slot's state is read.
inline int EvaluateExpandedSoulPreSpawn(PlayState* play, int actorId, unsigned& params, bool credits,
                                        const ExpandedSoulsOptions& options, const ExpandedSoulsOwnership& state) {
    if (!options.rando)
        return 1;
    const auto owned = [&](ExpandedSoul soul) {
        const unsigned index = static_cast<unsigned>(soul);
        const bool enabled = index < 4 ? options.animal : index < 6 ? options.misc : options.npc;
        return !enabled || ExpandedSoulOwned(state, index);
    };
    const int auxiliary = ExpandedAuxSpawn(play, actorId, params, owned);
    // Pinned wrapper tests !canSpawnActor: an auxiliary -1 passes through.
    if (auxiliary == 0)
        return 0;
    ExpandedGormanParams(actorId, params, owned);
    return ExpandedSoulSpawnPolicy(play, actorId, params, credits, owned);
}
static_assert(static_cast<unsigned>(ExpandedSoul::ANIMAL_BUTTERFLY) == 0);
static_assert(static_cast<unsigned>(ExpandedSoul::MISC_BUSINESS_SCRUB) == 4);
static_assert(static_cast<unsigned>(ExpandedSoul::NPC_ANJU) == 6);
static_assert(static_cast<unsigned>(ExpandedSoul::Count) == 59);
} // namespace Rando
