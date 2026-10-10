#pragma once
// Pinned custom/index.ts BOOTS_IRON/HOVER: exact owning OoT object_link_boy display lists.
// The upstream generic builder only relocates object segment06 to0a; native named assets carry relocation.
// Adult object lists and exact pinned child static geometry borrow their owning OoT textures.
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sAdultBootOwner;
static std::array<std::shared_ptr<Fast::DisplayList>,4> sAdultBootResources{};
static bool sAdultBootListener=false;
static const char* const sAdultBootNames[]={"objects/object_link_boy/gLinkAdultLeftIronBootDL","objects/object_link_boy/gLinkAdultRightIronBootDL","objects/object_link_boy/gLinkAdultLeftHoverBootDL","objects/object_link_boy/gLinkAdultRightHoverBootDL"};
alignas(2) static const char sAdultBootPaths0[]="__OTR__@oot:objects/object_link_boy/gLinkAdultLeftIronBootDL";
alignas(2) static const char sAdultBootPaths1[]="__OTR__@oot:objects/object_link_boy/gLinkAdultRightIronBootDL";
alignas(2) static const char sAdultBootPaths2[]="__OTR__@oot:objects/object_link_boy/gLinkAdultLeftHoverBootDL";
alignas(2) static const char sAdultBootPaths3[]="__OTR__@oot:objects/object_link_boy/gLinkAdultRightHoverBootDL";
static const char* const sAdultBootPaths[]={sAdultBootPaths0,sAdultBootPaths1,sAdultBootPaths2,sAdultBootPaths3};
inline void DropAdultBootResources(){sAdultBootResources={};sAdultBootOwner.reset();}
inline bool LoadAdultBootResources(){
    auto oot=Ship::CrossRMRegistry::Get("oot");if(!oot)return false;
    if(sAdultBootOwner==oot)return true;
    std::array<std::shared_ptr<Fast::DisplayList>,4> loaded{};
    for(size_t i=0;i<4;++i){loaded[i]=std::dynamic_pointer_cast<Fast::DisplayList>(oot->LoadResourceProcess(sAdultBootNames[i]));if(!loaded[i]||loaded[i]->Instructions.empty())return false;}
    if(sAdultBootOwner)DropAdultBootResources();sAdultBootOwner=oot;sAdultBootResources=std::move(loaded);
    if(!sAdultBootListener){Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultBootResources);sAdultBootListener=true;}return true;
}
}
extern "C" int Combo_MM_AdultBootDrawResourcesReady(void){return ComboAdultMM::LoadAdultBootResources();}
extern "C" const char* Combo_MM_GetAdultBootDrawPath(int index){if(index<0||index>=4||!ComboAdultMM::LoadAdultBootResources())return nullptr;return ComboAdultMM::sAdultBootPaths[index];}

#include <fast/resource/type/Texture.h>
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sChildBootOwner;
static std::array<std::shared_ptr<Fast::Texture>,5> sChildBootTextures{};
static bool sChildBootListener=false;
inline void DropChildBootTextures(){sChildBootTextures={};sChildBootOwner.reset();}
inline bool LoadChildBootTextures(){
 auto oot=Ship::CrossRMRegistry::Get("oot");if(!oot)return false;if(sChildBootOwner==oot)return true;
 static const char* names[]={"objects/object_link_boy/gLinkAdultIronBootTex","objects/object_link_boy/object_link_boyTLUT_00CB40","objects/object_link_boy/gLinkAdultHoverBootsHeelTex","objects/object_link_boy/gLinkAdultHoverBootsJetTex","objects/object_link_boy/gLinkAdultHoverBootsFeatherTex"};
 static const int widths[]={16,16,16,32,32},heights[]={16,16,8,32,16},bytes[]={256,512,256,2048,1024};
 std::array<std::shared_ptr<Fast::Texture>,5> loaded{};
 for(size_t i=0;i<5;++i){loaded[i]=std::dynamic_pointer_cast<Fast::Texture>(oot->LoadResourceProcess(names[i]));if(!loaded[i]||!loaded[i]->GetRawPointer()||loaded[i]->Type!=(i==0?Fast::TextureType::Palette8bpp:Fast::TextureType::RGBA16bpp)||loaded[i]->Width!=widths[i]||loaded[i]->Height!=heights[i]||loaded[i]->ImageDataSize!=bytes[i])return false;}
 if(sChildBootOwner)DropChildBootTextures();sChildBootOwner=oot;sChildBootTextures=std::move(loaded);
 if(!sChildBootListener){Ship::CrossRMRegistry::RegisterTeardownListener(DropChildBootTextures);sChildBootListener=true;}return true;
}
}
extern "C" int Combo_MM_ChildBootResourcesReady(void){return ComboAdultMM::LoadChildBootTextures();}
