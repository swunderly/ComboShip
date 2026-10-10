#pragma once
#include "SongTravel.h"
#include "soh/Enhancements/randomizer/location_access.h"
namespace ComboRando {
inline constexpr RandomizerRegion kOotSongRegions[] = {RR_SACRED_FOREST_MEADOW, RR_DMC_PAD_ENTRY, RR_LAKE_HYLIA,
    RR_DESERT_COLOSSUS, RR_GRAVEYARD_WARP_PAD_REGION, RR_TEMPLE_OF_TIME};
inline constexpr RandomizerRegion kOotSongSources[] = {RR_MINUET_OF_FOREST_WARP, RR_BOLERO_OF_FIRE_WARP,
    RR_SERENADE_OF_WATER_WARP, RR_REQUIEM_OF_SPIRIT_WARP, RR_NOCTURNE_OF_SHADOW_WARP, RR_PRELUDE_OF_LIGHT_WARP};
inline constexpr int kOotSongEntrances[] = {ENTR_SACRED_FOREST_MEADOW_WARP_PAD, ENTR_DEATH_MOUNTAIN_CRATER_WARP_PAD,
    ENTR_LAKE_HYLIA_WARP_PAD, ENTR_DESERT_COLOSSUS_WARP_PAD, ENTR_GRAVEYARD_WARP_PAD, ENTR_TEMPLE_OF_TIME_WARP_PAD};
inline int OotSongSource(RandomizerRegion region) { for(int i=0;i<6;i++) if(kOotSongSources[i]==region) return i; return -1; }
}
