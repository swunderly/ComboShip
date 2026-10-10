#pragma once
// Exact pinned custom/index.ts adult model writes, MIT; see 2s2h/ComboAdultUpstream.LICENSE.
extern int Combo_MM_AdultResourcesReady(void);
extern void* Combo_MM_GetAdultModelBinding(int index);
// Resolver must retain its owning ResourceManager for the entire native player/resource lifetime.
typedef struct {Gfx** destination;int count;u8 binding[6];Gfx* original[6];} ComboAdultModelRecipe;
static ComboAdultModelRecipe sComboAdultModelRecipes[]={
    {gPlayerWaistDLs+2*PLAYER_FORM_HUMAN,2,{1,1},{0}},
    {gPlayerHandHoldingShields,4,{2,2,3,3},{0}},
    {gPlayerSheath12DLs+2*PLAYER_FORM_HUMAN,2,{4,4},{0}},
    {gPlayerSheath13DLs+2*PLAYER_FORM_HUMAN,2,{4,4},{0}},
    {gPlayerSheath14DLs+2*PLAYER_FORM_HUMAN,2,{4,4},{0}},
    {gPlayerShields,4,{5,5,6,6},{0}},
    {gPlayerSheathedSwords,6,{7,7,8,8,9,9},{0}},
    {gPlayerSwordSheaths,6,{10,10,11,11,12,12},{0}},
    {gPlayerLeftHandTwoHandSwordDLs+2*PLAYER_FORM_HUMAN,2,{13,13},{0}},
    {gPlayerLeftHandOpenDLs+2*PLAYER_FORM_HUMAN,2,{14,14},{0}},
    {gPlayerLeftHandClosedDLs+2*PLAYER_FORM_HUMAN,2,{15,15},{0}},
    {gPlayerLeftHandOneHandSwordDLs+2*PLAYER_FORM_HUMAN,2,{16,16},{0}},
    {D_801C018C,6,{17,17,18,18,19,19},{0}},
    {gPlayerRightHandOpenDLs+2*PLAYER_FORM_HUMAN,2,{20,20},{0}},
    {gPlayerRightHandClosedDLs+2*PLAYER_FORM_HUMAN,2,{21,21},{0}},
    {gPlayerRightHandBowDLs+2*PLAYER_FORM_HUMAN,2,{22,22},{0}},
    {gPlayerRightHandInstrumentDLs+2*PLAYER_FORM_HUMAN,2,{23,23},{0}},
    {gPlayerRightHandHookshotDLs+2*PLAYER_FORM_HUMAN,2,{24,24},{0}},
    {gPlayerLeftHandBottleDLs+2*PLAYER_FORM_HUMAN,2,{25,25},{0}},
    {sPlayerFirstPersonLeftForearmDLs+PLAYER_FORM_HUMAN,1,{4},{0}},
    {sPlayerFirstPersonLeftHandDLs+PLAYER_FORM_HUMAN,1,{15},{0}},
    {sPlayerFirstPersonRightShoulderDLs+PLAYER_FORM_HUMAN,1,{26},{0}},
    {sPlayerFirstPersonRightHandDLs+PLAYER_FORM_HUMAN,1,{27},{0}},
    {sPlayerFirstPersonRightHandHookshotDLs+PLAYER_FORM_HUMAN,1,{28},{0}},
    {&D_801C0D94,1,{29},{0}},
 };
static int sComboAdultModelCaptured;
static FlexSkeletonHeader* sComboChildSkeleton;
static f32 sComboChildWeaponLengths[PLAYER_MELEEWEAPON_MAX];
void Combo_MM_ApplyAdultModelTables(int adult) {
    int i,j;void* bindings[30];
    if(adult){
        if(!Combo_MM_AdultResourcesReady())return;
        for(i=0;i<30;++i){bindings[i]=Combo_MM_GetAdultModelBinding(i);if(!bindings[i])return;}
        FlexSkeletonHeader* skeleton=bindings[0];
        if(skeleton->sh.limbCount!=21 || skeleton->dListCount!=18)return;
    }
    if(!sComboAdultModelCaptured){
        sComboChildSkeleton=gPlayerSkeletons[PLAYER_FORM_HUMAN];
        memcpy(sComboChildWeaponLengths,sMeleeWeaponLengths,sizeof(sMeleeWeaponLengths));
        for(i=0;i<ARRAY_COUNT(sComboAdultModelRecipes);++i){
            ComboAdultModelRecipe* recipe=&sComboAdultModelRecipes[i];
            for(j=0;j<recipe->count;++j)recipe->original[j]=recipe->destination[j];
        }
        sComboAdultModelCaptured=1;
    }
    gPlayerSkeletons[PLAYER_FORM_HUMAN]=adult?bindings[0]:sComboChildSkeleton;
    for(i=0;i<ARRAY_COUNT(sComboAdultModelRecipes);++i){
        ComboAdultModelRecipe* recipe=&sComboAdultModelRecipes[i];
        for(j=0;j<recipe->count;++j)recipe->destination[j]=adult?bindings[recipe->binding[j]]:recipe->original[j];
    }
    memcpy(sMeleeWeaponLengths,sComboChildWeaponLengths,sizeof(sMeleeWeaponLengths));
    if(adult){sMeleeWeaponLengths[1]=4000;sMeleeWeaponLengths[2]=4000;sMeleeWeaponLengths[3]=5500;sMeleeWeaponLengths[4]=5500;}
}
