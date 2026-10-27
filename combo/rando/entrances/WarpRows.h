// OoTMM v32.3's Warp Songs and Soaring Spots Shuffle rows (erWarps, packages/core/src/settings/data.ts:3557-3570):
// the six OoT warp-song pads (data/defs/entrances.yml:174-179, type one-way-song, from OOT SONG_TP_*) and the ten MM
// owl statues (:507-516, type one-way-statue, from MM SOARING, every one no-sot), in the endpoint order of the port's
// song travel map (SongTravel.h): index i names the source and the landing of song or owl i. poolWarps
// (packages/logic/src/solver/entrances.ts:729-740) shuffles these rows among each other, and with erWarps on
// poolWallmasters gives Wallmasters their landings as destinations too (:657-669 through poolsTypesDst, :881-886).
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "Registry.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace ComboRando::Entrances {
inline constexpr std::array<std::string_view, 16> kWarpRows = {
    // OoT: Minuet, Bolero, Serenade, Requiem, Nocturne, Prelude (SongTravelOOT.h kOotSongRegions).
    "OOT_WARP_SONG_MEADOW",
    "OOT_WARP_SONG_CRATER",
    "OOT_WARP_SONG_LAKE",
    "OOT_WARP_SONG_DESERT",
    "OOT_WARP_SONG_GRAVE",
    "OOT_WARP_SONG_TEMPLE",
    // MM: the owl statues in the Song of Soaring map's order (SongTravelMM.h kMmSongEntrances).
    "MM_WARP_OWL_GREAT_BAY",
    "MM_WARP_OWL_ZORA_CAPE",
    "MM_WARP_OWL_SNOWHEAD",
    "MM_WARP_OWL_MOUNTAIN_VILLAGE",
    "MM_WARP_OWL_CLOCK_TOWN",
    "MM_WARP_OWL_MILK_ROAD",
    "MM_WARP_OWL_WOODFALL",
    "MM_WARP_OWL_SOUTHERN_SWAMP",
    "MM_WARP_OWL_IKANA_CANYON",
    "MM_WARP_OWL_STONE_TOWER",
};
inline constexpr int kOotWarpCount = 6;
// The index of a warp row (song or owl i), or -1.
constexpr int WarpIndex(std::string_view row) {
    for (size_t i = 0; i < kWarpRows.size(); ++i)
        if (kWarpRows[i] == row)
            return static_cast<int>(i);
    return -1;
}
constexpr bool IsWarpRow(std::string_view row) {
    return WarpIndex(row) >= 0;
}
// One destination set per warp source (bit d: landing d). A placed route has one bit; a source still in the WARPS
// pool of a candidate world has every landing it may still take (changeWorldAssumePools, entrances.ts:289-305).
using WarpMasks = std::array<uint32_t, 16>;
inline constexpr uint32_t kOotWarpBits = (1u << kOotWarpCount) - 1;
inline constexpr uint32_t kMmWarpBits = 0xffffu & ~kOotWarpBits;
// The one landing of each source of a placed permutation (destination i of source i).
constexpr WarpMasks WarpMasksOf(const std::array<int, 16>& destinations) {
    WarpMasks masks{};
    for (size_t i = 0; i < masks.size(); ++i)
        masks[i] = destinations[i] >= 0 && destinations[i] < 16 ? 1u << destinations[i] : 0u;
    return masks;
}

// Every row is the registry's: its game, its type, no reverse, MM's from MM SOARING, and no-sot exactly for MM's.
inline bool WarpRowsMatchTheRegistry() {
    for (size_t i = 0; i < kWarpRows.size(); ++i) {
        const auto* row = Find(kWarpRows[i]);
        const bool oot = i < static_cast<size_t>(kOotWarpCount);
        if (row == nullptr || row->game != (oot ? Game::Oot : Game::Mm) ||
            row->type != (oot ? Type::OneWaySong : Type::OneWayStatue) || !row->reverse.empty() ||
            (!oot && row->from != "MM SOARING") || row->noSongOfTime == oot)
            return false;
    }
    return true;
}
} // namespace ComboRando::Entrances
