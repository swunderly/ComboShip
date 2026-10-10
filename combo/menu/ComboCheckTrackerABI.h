#pragma once
#include <stdint.h>

// Strings belong to the game module and are valid only during the visitor call.
// No game structs, STL containers, textures, or allocator ownership cross this ABI.
enum ComboCheckFlags {
    COMBO_CHECK_COLLECTED = 1,
    COMBO_CHECK_SKIPPED = 2,
    COMBO_CHECK_SEEN = 4,
    COMBO_CHECK_SCUMMED = 8,
    COMBO_CHECK_SAVED = 16,
    COMBO_CHECK_AVAILABLE = 32,
    COMBO_CHECK_LOGIC_KNOWN = 64,
    COMBO_CHECK_AREA_HIDDEN = 128,
    COMBO_CHECK_HINTED = 256,
    COMBO_CHECK_KNOWN = 512,
};
struct ComboCheckInfo {
    int32_t loaded;
    int32_t slot;
    int32_t currentArea;
};
struct ComboCheckRow {
    int32_t id;
    int32_t area;
    uint32_t flags;
    const char* areaName;
    const char* name;
    const char* type;
    const char* reward;
    const char* logic;
};
typedef void (*ComboCheckVisitor)(const ComboCheckRow*, void*);
typedef void (*ComboCheckSnapshot)(ComboCheckInfo*, ComboCheckVisitor, void*);
typedef int (*ComboCheckSetSkipped)(int32_t slot, int32_t id, int32_t skipped);
