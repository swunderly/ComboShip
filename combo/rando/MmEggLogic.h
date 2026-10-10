#pragma once

namespace ComboRando {
// Zero means the runtime enhancement is not assumed by logic. The native
// slider permits 1..7; invalid imported values retain the seven-egg quest.
inline constexpr int RequiredZoraEggs(int logicalCount) {
    return logicalCount >= 1 && logicalCount <= 7 ? logicalCount : 7;
}

// Egg events count distinct reachable eggs, including their own diving,
// combat and area prerequisites. One reusable bottle can deposit them in
// successive trips. This is a song lesson: the reward/song and its note
// buttons need not be owned, but Zora's local instrument must be usable.
inline constexpr bool CanLearnNewWaveBossaNova(int logicalCount, int accessibleEggs, bool canBeZora,
                                               bool localInstrument, bool bottle) {
    return canBeZora && localInstrument && bottle && accessibleEggs >= RequiredZoraEggs(logicalCount);
}
} // namespace ComboRando
