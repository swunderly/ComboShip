#pragma once
// Adult Link in MM (OoTMM v32.3 Cross-Games Age) drawn with OoT's own adult model: the human slots OoTMM rebinds
// (play/play.c:620-724) take OoT's adult lists and the composites of 2s2h/ComboAdultModelResources.h (plan F5b 1.3).
// The slots OoTMM fills from its custom model's sheaths, shields, sheathed swords and bow string keep MM's own (lead
// decisions D1, D2), and the first-person left forearm draws nothing, as OoTMM's empty list there.
#include "2s2h/ComboAdultOotSlots.h"
extern s32 Combo_MM_PrepareCrossAgeModel(void);
extern FlexSkeletonHeader* Combo_MM_CrossAgeSkeleton(void);
extern Gfx* Combo_MM_CrossAgeSlotDL(s32 slot);
extern TexturePtr Combo_MM_CrossAgeFaceTexture(s32 index);
typedef struct {
    Gfx** destination;
    s32 count;
    u8 slot[6];
    Gfx* original[6];
} ComboAdultModelRecipe;
static ComboAdultModelRecipe sComboAdultModelRecipes[] = {
    { gPlayerWaistDLs + 2 * PLAYER_FORM_HUMAN, 2, { COMBO_ADULT_WAIST_NEAR, COMBO_ADULT_WAIST_FAR } },
    { gPlayerHandHoldingShields,
      4,
      { COMBO_ADULT_HEROS_SHIELD_NEAR, COMBO_ADULT_HEROS_SHIELD_FAR, COMBO_ADULT_MIRROR_SHIELD_NEAR,
        COMBO_ADULT_MIRROR_SHIELD_FAR } },
    { gPlayerLeftHandTwoHandSwordDLs + 2 * PLAYER_FORM_HUMAN,
      2,
      { COMBO_ADULT_GREAT_FAIRYS_SWORD_NEAR, COMBO_ADULT_GREAT_FAIRYS_SWORD_FAR } },
    { gPlayerLeftHandOpenDLs + 2 * PLAYER_FORM_HUMAN,
      2,
      { COMBO_ADULT_LEFT_HAND_OPEN_NEAR, COMBO_ADULT_LEFT_HAND_OPEN_FAR } },
    { gPlayerLeftHandClosedDLs + 2 * PLAYER_FORM_HUMAN,
      2,
      { COMBO_ADULT_LEFT_HAND_CLOSED_NEAR, COMBO_ADULT_LEFT_HAND_CLOSED_FAR } },
    { gPlayerLeftHandOneHandSwordDLs + 2 * PLAYER_FORM_HUMAN,
      2,
      { COMBO_ADULT_KOKIRI_SWORD_NEAR, COMBO_ADULT_KOKIRI_SWORD_FAR } },
    { D_801C018C,
      6,
      { COMBO_ADULT_KOKIRI_SWORD_NEAR, COMBO_ADULT_KOKIRI_SWORD_FAR, COMBO_ADULT_RAZOR_SWORD_NEAR,
        COMBO_ADULT_RAZOR_SWORD_FAR, COMBO_ADULT_GILDED_SWORD_NEAR, COMBO_ADULT_GILDED_SWORD_FAR } },
    { gPlayerRightHandOpenDLs + 2 * PLAYER_FORM_HUMAN,
      2,
      { COMBO_ADULT_RIGHT_HAND_OPEN_NEAR, COMBO_ADULT_RIGHT_HAND_OPEN_FAR } },
    { gPlayerRightHandClosedDLs + 2 * PLAYER_FORM_HUMAN,
      2,
      { COMBO_ADULT_RIGHT_HAND_CLOSED_NEAR, COMBO_ADULT_RIGHT_HAND_CLOSED_FAR } },
    { gPlayerRightHandBowDLs + 2 * PLAYER_FORM_HUMAN, 2, { COMBO_ADULT_BOW_NEAR, COMBO_ADULT_BOW_FAR } },
    { gPlayerRightHandInstrumentDLs + 2 * PLAYER_FORM_HUMAN,
      2,
      { COMBO_ADULT_RIGHT_HAND_INSTRUMENT_NEAR, COMBO_ADULT_RIGHT_HAND_INSTRUMENT_FAR } },
    { gPlayerRightHandHookshotDLs + 2 * PLAYER_FORM_HUMAN, 2, { COMBO_ADULT_HOOKSHOT_NEAR, COMBO_ADULT_HOOKSHOT_FAR } },
    { gPlayerLeftHandBottleDLs + 2 * PLAYER_FORM_HUMAN, 2, { COMBO_ADULT_LEFT_HAND_BOTTLE, COMBO_ADULT_LEFT_HAND_BOTTLE } },
    { sPlayerFirstPersonLeftForearmDLs + PLAYER_FORM_HUMAN, 1, { COMBO_ADULT_SLOT_EMPTY } },
    { sPlayerFirstPersonLeftHandDLs + PLAYER_FORM_HUMAN, 1, { COMBO_ADULT_LEFT_HAND_CLOSED_NEAR } },
    { sPlayerFirstPersonRightShoulderDLs + PLAYER_FORM_HUMAN, 1, { COMBO_ADULT_RIGHT_SHOULDER_NEAR } },
    { sPlayerFirstPersonRightHandDLs + PLAYER_FORM_HUMAN, 1, { COMBO_ADULT_FIRST_PERSON_BOW } },
    { sPlayerFirstPersonRightHandHookshotDLs + PLAYER_FORM_HUMAN, 1, { COMBO_ADULT_FIRST_PERSON_HOOKSHOT } },
};
static s32 sComboAdultModelCaptured;
static s32 sComboAdultModelApplied;
static FlexSkeletonHeader* sComboChildSkeleton;
static f32 sComboChildWeaponLengths[PLAYER_MELEEWEAPON_MAX];
static TexturePtr sComboChildFace[COMBO_ADULT_EYE_TEXTURES + COMBO_ADULT_MOUTH_TEXTURES];

static TexturePtr* Combo_MM_HumanFaceRow(s32 index) {
    return index < COMBO_ADULT_EYE_TEXTURES ? &sPlayerEyesTextures[PLAYER_FORM_HUMAN][index]
                                            : &sPlayerMouthTextures[PLAYER_FORM_HUMAN][index - COMBO_ADULT_EYE_TEXTURES];
}

// MM's child tables back. A face row keeps whatever replaced the adult texture since (Play As Kafei rewrites the
// human rows when a scene ends, PlayAsKafei.cpp UpdatePlayAsKafei).
void Combo_MM_RestoreChildModelTables(void) {
    s32 i, j;
    if (!sComboAdultModelApplied) {
        return;
    }
    gPlayerSkeletons[PLAYER_FORM_HUMAN] = sComboChildSkeleton;
    for (i = 0; i < ARRAY_COUNT(sComboAdultModelRecipes); ++i) {
        ComboAdultModelRecipe* recipe = &sComboAdultModelRecipes[i];
        for (j = 0; j < recipe->count; ++j) {
            recipe->destination[j] = recipe->original[j];
        }
    }
    for (i = 0; i < ARRAY_COUNT(sComboChildFace); ++i) {
        if (*Combo_MM_HumanFaceRow(i) == Combo_MM_CrossAgeFaceTexture(i)) {
            *Combo_MM_HumanFaceRow(i) = sComboChildFace[i];
        }
    }
    memcpy(sMeleeWeaponLengths, sComboChildWeaponLengths, sizeof(sMeleeWeaponLengths));
    sComboAdultModelApplied = 0;
}

// Puts adult Link's model in the human tables, or MM's child one back. Returns whether adult Link is drawn: never
// while OoT's or MM's side of the model is missing.
s32 Combo_MM_ApplyAdultModelTables(s32 adult) {
    FlexSkeletonHeader* skeleton = NULL;
    s32 i, j;

    if (adult && Combo_MM_PrepareCrossAgeModel()) {
        skeleton = Combo_MM_CrossAgeSkeleton();
        for (i = 0; skeleton != NULL && i < ARRAY_COUNT(sComboAdultModelRecipes); ++i) {
            for (j = 0; j < sComboAdultModelRecipes[i].count; ++j) {
                if (sComboAdultModelRecipes[i].slot[j] != COMBO_ADULT_SLOT_EMPTY &&
                    Combo_MM_CrossAgeSlotDL(sComboAdultModelRecipes[i].slot[j]) == NULL) {
                    skeleton = NULL;
                }
            }
        }
        for (i = 0; skeleton != NULL && i < ARRAY_COUNT(sComboChildFace); ++i) {
            if (Combo_MM_CrossAgeFaceTexture(i) == NULL) {
                skeleton = NULL;
            }
        }
    }
    if (skeleton == NULL) {
        Combo_MM_RestoreChildModelTables();
        return false;
    }
    if (!sComboAdultModelCaptured) {
        sComboChildSkeleton = gPlayerSkeletons[PLAYER_FORM_HUMAN];
        memcpy(sComboChildWeaponLengths, sMeleeWeaponLengths, sizeof(sMeleeWeaponLengths));
        for (i = 0; i < ARRAY_COUNT(sComboAdultModelRecipes); ++i) {
            ComboAdultModelRecipe* recipe = &sComboAdultModelRecipes[i];
            for (j = 0; j < recipe->count; ++j) {
                recipe->original[j] = recipe->destination[j];
            }
        }
        sComboAdultModelCaptured = true;
    }
    gPlayerSkeletons[PLAYER_FORM_HUMAN] = skeleton;
    for (i = 0; i < ARRAY_COUNT(sComboAdultModelRecipes); ++i) {
        ComboAdultModelRecipe* recipe = &sComboAdultModelRecipes[i];
        for (j = 0; j < recipe->count; ++j) {
            recipe->destination[j] =
                recipe->slot[j] == COMBO_ADULT_SLOT_EMPTY ? NULL : Combo_MM_CrossAgeSlotDL(recipe->slot[j]);
        }
    }
    // OoT's eyes and mouths for the head's segments 08 and 09, index for index (plan F5b 1.3). A row that does not
    // hold the adult texture is captured first, so it comes back as it was.
    for (i = 0; i < ARRAY_COUNT(sComboChildFace); ++i) {
        if (*Combo_MM_HumanFaceRow(i) != Combo_MM_CrossAgeFaceTexture(i)) {
            sComboChildFace[i] = *Combo_MM_HumanFaceRow(i);
            *Combo_MM_HumanFaceRow(i) = Combo_MM_CrossAgeFaceTexture(i);
        }
    }
    // OoTMM's adult weapon lengths (play.c:713-717, lead decision D5).
    memcpy(sMeleeWeaponLengths, sComboChildWeaponLengths, sizeof(sMeleeWeaponLengths));
    sMeleeWeaponLengths[PLAYER_MELEEWEAPON_SWORD_KOKIRI] = 4000.0f;
    sMeleeWeaponLengths[PLAYER_MELEEWEAPON_SWORD_RAZOR] = 4000.0f;
    sMeleeWeaponLengths[PLAYER_MELEEWEAPON_SWORD_GILDED] = 5500.0f;
    sMeleeWeaponLengths[PLAYER_MELEEWEAPON_SWORD_TWO_HANDED] = 5500.0f;
    sComboAdultModelApplied = true;
    return true;
}
