// The rules a live tracker search runs under, as part of the key of the answers the game not in front keeps for the
// game in front (LiveWallmasterRoutes.h LiveDepartureMemo). Both games' live searches read the options as the player
// has them now: the gameplay rules, by their runtime value as the trackers read them (GameplayRules.h; OoT's logic.cpp
// and SOH_LiveIncomingSongAges, MM's Logic.h GameplayRuleValue), MM's tricks (MmTricks.h, Logic.h COMBO_MM_TRICK) and
// Save on Moon Crash (MM's Logic.cpp). They also route through the song travel map (SongTravel.h) and the installed
// layout, by its revision (OotUnifiedLayout, MmUnifiedLayout revision()). A change to any of them changes the key, so
// a changed option is searched again.
#pragma once
#include "GameplayRules.h"
#include "LiveWallmasterRoutes.h"
#include "MmTricks.h"
#include "SongTravel.h"
#include <cstdint>
#include <vector>

namespace ComboRando::Entrances {
inline constexpr const char* kLiveMoonCrashRule = "gEnhancements.Cycle.SaveOnMoonCrash";
// read(name, fallback) reads an option as CVarGetInteger does; layoutRevision is the installed layout's revision.
template <class Read> uint64_t LiveRulesKey(Read read, uint64_t layoutRevision, uint64_t seed = kLiveKeySeed) {
    std::vector<int32_t> values;
    for (const auto& rule : kGameplayRules)
        values.push_back(ReadGameplayRule(rule, false, read));
    for (const auto& trick : kMmTricks)
        values.push_back(static_cast<int32_t>(read(trick.key, 0)));
    values.push_back(static_cast<int32_t>(read(kLiveMoonCrashRule, 0)));
    const auto& songs = ActiveSongTravel();
    values.insert(values.end(), { songs.enabled, songs.foreignSongs, songs.logicEnabled, songs.logicForeignSongs });
    values.insert(values.end(), songs.destinations.begin(), songs.destinations.end());
    values.push_back(static_cast<int32_t>(layoutRevision & 0xffffffffu));
    values.push_back(static_cast<int32_t>(layoutRevision >> 32));
    return LiveStateKey(values.data(), values.size() * sizeof(int32_t), seed);
}
} // namespace ComboRando::Entrances
