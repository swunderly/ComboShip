#ifndef Z_MAGIC_DARK_H
#define Z_MAGIC_DARK_H

#include "global.h"
#include "2s2h/Rando/Spells.h"
#include "align_asset_macro.h"

struct ComboMagicDark;

typedef struct ComboMagicDark {
    /* 0x0000 */ Actor actor;
    /* 0x013C */ s16 timer;
    /* 0x013E */ u8 primAlpha;
    /* 0x0140 */ Vec3f orbOffset;
    /* 0x014C */ f32 scale;
    /* 0x0150 */ char unk_160[0x4];
} ComboMagicDark; /* size = 0x0154 */

#endif
