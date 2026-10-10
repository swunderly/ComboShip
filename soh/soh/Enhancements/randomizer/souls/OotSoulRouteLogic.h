#pragma once
namespace Rando::Souls {
// Pinned Ganon Castle Shadow chest2/pots/hearts, excluding unavailable climb_anywhere.
inline bool ShadowTrialSoulAlternative(bool longshot, bool likeLikeSoul, bool fireArrows, bool hoverBoots,
                                       bool dinsFire) {
    return ((longshot && likeLikeSoul) || fireArrows) && (hoverBoots || fireArrows || dinsFire);
}
} // namespace Rando::Souls
