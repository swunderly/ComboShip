#pragma once
// OoTMM v32.3 patches the En_Horse overlay for adult Link with 53 u16 pairs (src/mm/actors.c:901-955, adultEponaPatch)
// and swaps the horse object (En/En_Horse.c:4-24). Each is a typed change here. The MM US overlay starts at VRAM
// 0x8087B730 (D_80889204 is +0xDAD4); ComboAndroid/tests/fixtures/ootmm-32.3-mm-adult-epona.json lists all 53.
//   0x865A-0x8F2E  38 lui/addiu immediates: the 19 cutscene animation references (sAdultEponaCsAnims below)
//   0xBBDE         the HORSE_TYPE_2 scale (EnHorse_Update)
//   0xC8AC         the rider-position branch (EnHorse_PostDraw)
//   0xD792-0xD7B2  sEponaAnimations, 0xD81A sSkeletonHeaders[HORSE_TYPE_2], 0xDAD6-0xDADE D_80889204 (eyes)
// The skeleton, animations and eyes are OoT's own object_horse (2s2h/ComboAdultEponaResources.h).
static AnimationHeader* sAdultEponaChildAnimations[ENHORSE_ANIM_MAX];
static SkeletonHeader* sAdultEponaChildSkeleton;
static TexturePtr sAdultEponaChildEyes[3];
static int sAdultEponaDefaultsValid;
static int sAdultEponaTablesActive;
// Each cutscene animation reference in overlay order, as the En_Horse animation whose OoT object_horse offset OoTMM
// writes into it (actors.c:902-939). In each function the first reference is the Animation_GetLastFrame argument,
// which IDO evaluates before Animation_Change's own.
static const u8 sAdultEponaCsAnims[] = {
    ENHORSE_ANIM_GALLOP,  // 0x865A/0x865E func_80883D64 (gEponaGallopingAnim, 0x1E2C)
    ENHORSE_ANIM_REARING, // 0x867A/0x868E func_80883D64 (gEponaRearingAnim, 0x3CEC)
    ENHORSE_ANIM_REARING, // 0x8706/0x870A func_80883E10
    ENHORSE_ANIM_REARING, // 0x872A/0x8736 func_80883E10
    ENHORSE_ANIM_WALK,    // 0x879A/0x87A2 func_80883EA0 (gEponaWalkingAnim, 0x75F0)
    ENHORSE_ANIM_WALK,    // 0x87AE/0x87BE func_80883EA0
    ENHORSE_ANIM_IDLE,    // 0x880E/0x8812 func_80883F18 (gEponaIdleAnim, 0x6D50)
    ENHORSE_ANIM_IDLE,    // 0x8832/0x883E func_80883F18
    ENHORSE_ANIM_IDLE,    // 0x887E/0x8886 func_80883F98
    ENHORSE_ANIM_REARING, // 0x8C22/0x8C2E func_80884314
    ENHORSE_ANIM_REARING, // 0x8C3A/0x8C4E func_80884314
    ENHORSE_ANIM_GALLOP,  // 0x8D52/0x8D5E func_80884444
    ENHORSE_ANIM_GALLOP,  // 0x8D6A/0x8D7E func_80884444
    ENHORSE_ANIM_GALLOP,  // 0x8DDE/0x8DE6 func_808844E0
    ENHORSE_ANIM_GALLOP,  // 0x8DF2/0x8E06 func_808844E0
    ENHORSE_ANIM_IDLE,    // 0x8E72/0x8E7E func_80884564
    ENHORSE_ANIM_IDLE,    // 0x8E92/0x8E9E func_80884564
    ENHORSE_ANIM_IDLE,    // 0x8F06/0x8F0E func_80884604
    ENHORSE_ANIM_IDLE,    // 0x8F1A/0x8F2E func_80884604
};
int Combo_MM_IsAdultEponaVariantApplied(void) {
    return sAdultEponaTablesActive;
}
void* Combo_MM_EponaCsAnimation(s32 reference, const void* childAnimation) {
    return sAdultEponaTablesActive ? sEponaAnimations[sAdultEponaCsAnims[reference]] : (void*)childAnimation;
}
// Adult tables apply only with every OoT binding in hand; otherwise En_Horse keeps MM's own Epona (decision D4).
void Combo_MM_ApplyEponaVariantTables(int adult) {
    void* bindings[13];
    int i;

    if (adult) {
        adult = Combo_MM_AdultEponaResourcesReady();
        for (i = 0; adult && i < 13; ++i) {
            bindings[i] = Combo_MM_GetAdultEponaBinding(i);
            adult = bindings[i] != NULL;
        }
    }
    if (!sAdultEponaDefaultsValid) {
        memcpy(sAdultEponaChildAnimations, sEponaAnimations, sizeof(sAdultEponaChildAnimations));
        sAdultEponaChildSkeleton = sSkeletonHeaders[HORSE_TYPE_2];
        memcpy(sAdultEponaChildEyes, D_80889204, sizeof(sAdultEponaChildEyes));
        sAdultEponaDefaultsValid = 1;
    }
    if (adult) {
        for (i = 0; i < ENHORSE_ANIM_MAX; ++i) {
            sEponaAnimations[i] = (AnimationHeader*)bindings[i + 1];
        }
        sSkeletonHeaders[HORSE_TYPE_2] = (SkeletonHeader*)bindings[0];
        for (i = 0; i < 3; ++i) {
            D_80889204[i] = (TexturePtr)bindings[i + 10];
        }
        sAdultEponaTablesActive = 1;
    } else {
        memcpy(sEponaAnimations, sAdultEponaChildAnimations, sizeof(sAdultEponaChildAnimations));
        sSkeletonHeaders[HORSE_TYPE_2] = sAdultEponaChildSkeleton;
        memcpy(D_80889204, sAdultEponaChildEyes, sizeof(sAdultEponaChildEyes));
        sAdultEponaTablesActive = 0;
    }
}

// OoTMM loads its adult object in place of OBJECT_HORSE_LINK_CHILD (En_Horse.c:4-24). The port's skeleton, animation
// and eye bindings are resources, not object-segment data, so the real MM object stays for slot and dependency
// bookkeeping.
int Combo_MM_EponaVariantObjectId(int adult) {
    if (adult && !Combo_MM_AdultEponaResourcesReady()) {
        return OBJECT_UNSET_0;
    }
    return OBJECT_HORSE_LINK_CHILD;
}
