#pragma once
// Exact capability/resource seams, all defined (BenPort.cpp and the native include headers). The Adult MM
// feature is gated at runtime by Combo_MM_AdultResourcesReady() (hard off through
// kAdultMmGameplayIntegrationComplete) and save.shipSaveInfo.adultMm.enabled.
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
