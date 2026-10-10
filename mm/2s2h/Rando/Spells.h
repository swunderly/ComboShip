#pragma once
#include "global.h"
#ifdef __cplusplus
extern "C" {
#endif
// Pinned spell indices: Wind 0, Love 1, Fire 2. These functions accept only 1 or 2.
int Combo_MMSpellEnabled(int spell);
int Combo_MMSpellPrepareResources(int spell);
int Combo_MMSpellEffectiveHumanAge(void);
void Combo_MMSpellGrantItem(int spell);
const char* Combo_MMSpellItemName(int item);
#ifdef __cplusplus
}
#endif
