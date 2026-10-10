#pragma once
#include <stdint.h>

// Read-only metadata stays in its owning game module. No C++ containers cross
// the boundary; strings live until the next metadata rebuild in that module.
typedef struct {
    int32_t id;
    const char* name;
    const char* description;
    const char* area;
    const char* tags;
    int32_t difficulty; // 0 novice, 1 intermediate, 2 advanced, 3 expert, 4 extreme, 5 experimental
    uint32_t tagMask;   // bits 0-5 difficulty tags, bit 6 glitch
    int32_t enabled;
} CwTrickInfo;

typedef const CwTrickInfo* (*Fn_ComboTricks)(int32_t* count);
typedef int32_t (*Fn_ComboSetTrick)(int32_t id, int32_t enabled);
typedef void (*Fn_ComboApplyTricks)(void);
