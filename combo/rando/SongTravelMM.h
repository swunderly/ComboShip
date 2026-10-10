#pragma once
#include "SongTravel.h"
namespace ComboRando {
inline constexpr int kMmSongEntrances[] = { ENTRANCE(GREAT_BAY_COAST, 11), ENTRANCE(ZORA_CAPE, 6),
                                            ENTRANCE(SNOWHEAD, 3),         ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 8),
                                            ENTRANCE(SOUTH_CLOCK_TOWN, 9), ENTRANCE(MILK_ROAD, 4),
                                            ENTRANCE(WOODFALL, 4),         ENTRANCE(SOUTHERN_SWAMP_POISONED, 10),
                                            ENTRANCE(IKANA_CANYON, 4),     ENTRANCE(STONE_TOWER, 3) };
inline int MmSongSource(int entrance) {
    for (int i = 0; i < 10; i++)
        if (kMmSongEntrances[i] == entrance)
            return i + 6;
    return -1;
}
} // namespace ComboRando
