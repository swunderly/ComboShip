#pragma once
#include <stdint.h>
typedef struct ComboSharedDungeon {
    uint8_t valid, game, age, form;
    float x, y, z;
    int16_t yaw, playerParams;
    int32_t entrance;
    int8_t roomIndex, data;
    uint32_t tempSwitchFlags, unk18, tempCollectFlags;
} ComboSharedDungeon;
typedef int (*ComboReadSharedDungeon)(int fileNum, ComboSharedDungeon*);
typedef int (*ComboWriteSharedDungeon)(int fileNum, const ComboSharedDungeon*);
