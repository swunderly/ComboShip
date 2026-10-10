#pragma once
// Exact capability/resource seams. No success stubs: final Adult port must implement these
// before linking/enabling this overlay; incomplete source must not be merged as a feature.
int Combo_MM_AdultResourcesReady(void);
int Combo_MM_IsAdult(void);
int Combo_MM_AdultMaskAction(void);
int Combo_MM_AdultCanSwapAge(void);
int Combo_MM_AdultSwapAge(PlayState* play);
void Combo_MM_AdultRestoreFaroreRespawn(void);
void Combo_MM_ClearCustomMask(void);
void Combo_MM_CopyAdultProperties(PlayerAgeProperties* out);
void Combo_MM_DrawAdultMask(PlayState* play,Player* player,int mode);
PlayerAnimationHeader* Combo_MM_AdultMaskOffAnimation(void);
void Combo_MM_ApplyAdultGameplayKeep(PlayState* play,int adult);
void Combo_MM_ApplyAdultModelTables(int adult);
void Combo_MM_AdultCheckBoots(PlayState* play);
void EponaAge_ClearReload(void);
void EponaAge_Request(PlayState* play,int adult);
int EponaAge_IsBusy(void);
void AdultMask_StartCsItem(Player* player,PlayState* play);
