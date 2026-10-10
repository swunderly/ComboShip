#ifndef COMBO_OVL_MM_CUSOM_MAGIC_FIRE_H
#define COMBO_OVL_MM_CUSOM_MAGIC_FIRE_H

#include "global.h"
#include "2s2h/Rando/Spells.h"
#include "align_asset_macro.h"

typedef struct
{
    Actor               actor;
    ColliderCylinder    collider;
    float               alphaMultiplier;
    float               screenTintIntensity;
    float               scalingSpeed;
    s16                 action;
    s16                 screenTintBehaviour;
    s16                 actionTimer;
    s16                 screenTintBehaviourTimer;
}
Actor_CustomMagicFire;

#endif
