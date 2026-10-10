#pragma once
#include <fast/resource/type/Texture.h>
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sGearIconOwner;
static std::array<std::shared_ptr<Fast::Texture>,4> sGearIcons{},sGearNames{};
static bool sGearIconListener=false,sGearIconTables=false;
static std::array<TexturePtr,4> sGearOldIcons{};
static std::array<u8,4> sGearOldSlots{};
static std::array<std::array<u8,4>,PLAYER_FORM_MAX> sGearOldRestrictions{};
static const char* const sGearIconNames[]={"textures/icon_item_static/gItemIconBootsIronTex","textures/icon_item_static/gItemIconBootsHoverTex","textures/icon_item_static/gItemIconTunicGoronTex","textures/icon_item_static/gItemIconTunicZoraTex"};
alignas(2) static const char sGearIconPaths0[]="__OTR__@oot:textures/icon_item_static/gItemIconBootsIronTex";
alignas(2) static const char sGearIconPaths1[]="__OTR__@oot:textures/icon_item_static/gItemIconBootsHoverTex";
alignas(2) static const char sGearIconPaths2[]="__OTR__@oot:textures/icon_item_static/gItemIconTunicGoronTex";
alignas(2) static const char sGearIconPaths3[]="__OTR__@oot:textures/icon_item_static/gItemIconTunicZoraTex";
static const char* const sGearIconPaths[]={sGearIconPaths0,sGearIconPaths1,sGearIconPaths2,sGearIconPaths3};
static const char* const sGearNameNames[]={"textures/item_name_static/gIronBootsItemNameENGTex","textures/item_name_static/gHoverBootsItemNameENGTex","textures/item_name_static/gGoronTunicItemNameENGTex","textures/item_name_static/gZoraTunicItemNameENGTex"};
alignas(2) static const char sGearNamePaths0[]="__OTR__@oot:textures/item_name_static/gIronBootsItemNameENGTex";
alignas(2) static const char sGearNamePaths1[]="__OTR__@oot:textures/item_name_static/gHoverBootsItemNameENGTex";
alignas(2) static const char sGearNamePaths2[]="__OTR__@oot:textures/item_name_static/gGoronTunicItemNameENGTex";
alignas(2) static const char sGearNamePaths3[]="__OTR__@oot:textures/item_name_static/gZoraTunicItemNameENGTex";
static const char* const sGearNamePaths[]={sGearNamePaths0,sGearNamePaths1,sGearNamePaths2,sGearNamePaths3};
inline void ApplyGearIconTables(bool enable){
 if(enable==sGearIconTables)return;
 for(int i=0;i<4;++i){const int item=ITEM_COMBO_BOOTS_IRON+i;
  if(enable){sGearOldIcons[i]=gItemIcons[item];sGearOldSlots[i]=gItemSlots[item];gItemIcons[item]=(TexturePtr)sGearIconPaths[i];gItemSlots[item]=(i%2==0)?SLOT_TRADE_KEY_MAMA:SLOT_TRADE_COUPLE;}
  else{gItemIcons[item]=sGearOldIcons[i];gItemSlots[item]=sGearOldSlots[i];}
  for(int form=0;form<PLAYER_FORM_MAX;++form){if(enable){sGearOldRestrictions[form][i]=gPlayerFormItemRestrictions[form][item];gPlayerFormItemRestrictions[form][item]=form==PLAYER_FORM_HUMAN;}else gPlayerFormItemRestrictions[form][item]=sGearOldRestrictions[form][i];}
 }
 sGearIconTables=enable;
}
inline void DropGearIcons(){ApplyGearIconTables(false);sGearIcons={};sGearNames={};sGearIconOwner.reset();}
inline bool LoadGearIcons(){
 auto oot=Ship::CrossRMRegistry::Get("oot");if(!oot)return false;if(sGearIconOwner==oot)return true;
 std::array<std::shared_ptr<Fast::Texture>,4> loaded{};
 for(size_t i=0;i<4;++i){loaded[i]=std::dynamic_pointer_cast<Fast::Texture>(oot->LoadResourceProcess(sGearIconNames[i]));if(!loaded[i]||!loaded[i]->GetRawPointer()||loaded[i]->Type!=Fast::TextureType::RGBA32bpp||loaded[i]->Width!=32||loaded[i]->Height!=32||loaded[i]->ImageDataSize!=4096)return false;}
 std::array<std::shared_ptr<Fast::Texture>,4> names{};
 for(size_t i=0;i<4;++i){names[i]=std::dynamic_pointer_cast<Fast::Texture>(oot->LoadResourceProcess(sGearNameNames[i]));if(!names[i]||!names[i]->GetRawPointer()||names[i]->Type!=Fast::TextureType::GrayscaleAlpha4bpp||names[i]->Width!=128||names[i]->Height!=16||names[i]->ImageDataSize!=1024)return false;}
 if(sGearIconOwner)DropGearIcons();sGearIconOwner=oot;sGearIcons=std::move(loaded);sGearNames=std::move(names);
 if(!sGearIconListener){Ship::CrossRMRegistry::RegisterTeardownListener(DropGearIcons);sGearIconListener=true;}return true;
}
}
extern "C" int Combo_MM_AdultGearIconsReady(void){return ComboAdultMM::LoadGearIcons();}
extern "C" void Combo_MM_ApplyAdultGearInventoryResources(int enabled){if(enabled&&!ComboAdultMM::LoadGearIcons())return;ComboAdultMM::ApplyGearIconTables(enabled!=0);}

extern "C" const char* Combo_MM_AdultGearItemName(int item){const int index=item-ITEM_COMBO_BOOTS_IRON;return index>=0&&index<4&&ComboAdultMM::LoadGearIcons()?ComboAdultMM::sGearNamePaths[index]:nullptr;}
