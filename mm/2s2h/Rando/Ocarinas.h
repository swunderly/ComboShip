#pragma once
#include "Rando/Types.h"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace Rando::Ocarinas {
// Pinned progressive.c: zero owned bits => Fairy, any owned bit => Time.
inline uint8_t EffectiveBits(uint8_t saved,uint8_t item) { return (saved&3) | (item==0 ? 2 : item==5 ? 1 : 0); }
inline int NextTier(uint8_t saved,uint8_t item) { return EffectiveBits(saved,item) ? 2 : 1; }
inline void Grant(uint8_t& saved,uint8_t& item,int tier) { saved=EffectiveBits(saved,item)|(1u<<(tier-1));item=tier>=2 ? 0 : 5; }
// Pinned macros_mm.yml has_ocarina; all song macros use this instrument gate.
inline bool CanPlay(bool shared,bool fairyEnabled,int tier) { return tier >= (shared&&!fairyEnabled ? 2 : 1); }
inline bool MirrorOotToMm(bool shared,bool fairyEnabled,int tier) { return shared&&(tier>=2||fairyEnabled); }
// Pinned transform.ts: non-shared MM Fairy adds one instrument to MM's original one.
// Shared pooling is deliberately owned by the combined transform and not approximated here.
inline void ReplaceNativePool(bool fairy, bool shared, std::vector<RandoItemId>& starts,
                              std::vector<RandoItemId>& pool) {
    if (!fairy || shared) return;
    for (auto& item : pool) if (item == RI_OCARINA) item = RI_PROGRESSIVE_OCARINA;
    pool.push_back(RI_PROGRESSIVE_OCARINA);
    for (auto it = starts.begin(); it != starts.end();) {
        int tiers = *it == RI_OCARINA ? 2 :
                    (*it == RI_OCARINA_FAIRY || *it == RI_PROGRESSIVE_OCARINA ? 1 : 0);
        if (!tiers) { ++it; continue; }
        while (tiers--) {
            auto found = std::find(pool.begin(), pool.end(), RI_PROGRESSIVE_OCARINA);
            if (found != pool.end()) pool.erase(found);
        }
        it = starts.erase(it); // local pool-removal list, not saved starting inventory
    }
}
}
