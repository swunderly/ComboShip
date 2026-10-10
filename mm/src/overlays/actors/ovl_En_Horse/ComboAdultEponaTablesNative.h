#pragma once
// Native typed version of the pinned Epona common-animation/skin/eye overlay writes.
// Object-slot allocation and actor initialization remain separate, unvalidated integration seams.
static AnimationHeader* sAdultEponaChildAnimations[ENHORSE_ANIM_MAX];
static SkeletonHeader* sAdultEponaChildSkeleton;
static TexturePtr sAdultEponaChildEyes[3];
static int sAdultEponaDefaultsValid;
static int sAdultEponaTablesActive;
int Combo_MM_IsAdultEponaVariantApplied(void){return sAdultEponaTablesActive;}
void Combo_MM_ApplyEponaVariantTables(int adult) {
    void* bindings[13];int i;
    if(adult){
        if(!Combo_MM_AdultEponaResourcesReady())return;
        for(i=0;i<13;++i){bindings[i]=Combo_MM_GetAdultEponaBinding(i);if(bindings[i]==NULL)return;}
    }
    if(!sAdultEponaDefaultsValid){
        memcpy(sAdultEponaChildAnimations,sEponaAnimations,sizeof(sAdultEponaChildAnimations));
        sAdultEponaChildSkeleton=sSkeletonHeaders[HORSE_TYPE_2];
        memcpy(sAdultEponaChildEyes,D_80889204,sizeof(sAdultEponaChildEyes));sAdultEponaDefaultsValid=1;
    }
    if(adult){
        for(i=0;i<ENHORSE_ANIM_MAX;++i)sEponaAnimations[i]=(AnimationHeader*)bindings[i+1];
        sSkeletonHeaders[HORSE_TYPE_2]=(SkeletonHeader*)bindings[0];
        for(i=0;i<3;++i)D_80889204[i]=(TexturePtr)bindings[i+10];
        sAdultEponaTablesActive=1;
    }else{
        memcpy(sEponaAnimations,sAdultEponaChildAnimations,sizeof(sAdultEponaChildAnimations));
        sSkeletonHeaders[HORSE_TYPE_2]=sAdultEponaChildSkeleton;
        memcpy(D_80889204,sAdultEponaChildEyes,sizeof(sAdultEponaChildEyes));
        sAdultEponaTablesActive=0;
    }
}

int Combo_MM_EponaVariantObjectId(int adult) {
    if(adult&&!Combo_MM_AdultEponaResourcesReady())return OBJECT_UNSET_0;
    // The native port's skin/animation/eye pointers are decoded resources, not segmented
    // object bytes. Keep the real MM object ID for slot/dependency bookkeeping; adult
    // geometry comes exclusively from the checked typed Epona bindings installed above.
    // Full actor/object-slot runtime validation remains required before feature activation.
    return OBJECT_HORSE_LINK_CHILD;
}
