#pragma once
// Exact pinned adult Epona bindings, loaded only through native MM typed factories.
#include "2s2h/resource/type/Animation.h"
#include <fast/resource/type/Texture.h>
extern "C" void Combo_MM_ApplyEponaVariantTables(int adult);
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sAdultEponaOwner;
static std::array<std::shared_ptr<Ship::IResource>,13> sAdultEponaResources{};
static bool sAdultEponaListener=false;
alignas(2) static const char sAdultEponaEyePaths0[]="__OTR__objects/combo_adult_epona/gEponaEyeOpenTex";
alignas(2) static const char sAdultEponaEyePaths1[]="__OTR__objects/combo_adult_epona/gEponaEyeHalfTex";
alignas(2) static const char sAdultEponaEyePaths2[]="__OTR__objects/combo_adult_epona/gEponaEyeClosedTex";
static const char* const sAdultEponaEyePaths[]={sAdultEponaEyePaths0,sAdultEponaEyePaths1,sAdultEponaEyePaths2};
inline void DropAdultEpona(){
    // Restore actor tables before releasing the typed resource owners.
    Combo_MM_ApplyEponaVariantTables(0);sAdultEponaResources={};sAdultEponaOwner.reset();
}
inline bool LoadAdultEpona(){
    auto mm=Ship::CrossRMRegistry::Get("mm");if(!mm)return false;
    if(sAdultEponaOwner==mm)return true;
    auto context=Ship::Context::GetRawInstance();if(!context||context->GetResourceManager()!=mm)return false;
    std::array<std::shared_ptr<Ship::IResource>,13> loaded{};
    loaded[0]=mm->LoadResourceProcess("objects/combo_adult_epona/gEponaSkel");
    auto skel=std::dynamic_pointer_cast<SOH::Skeleton>(loaded[0]);
    if(!skel||skel->type!=SOH::SkeletonType::Normal||skel->limbType!=SOH::LimbType::Skin||skel->limbCount!=47||skel->skeletonHeaderSegments.size()!=47)return false;
    for(auto limb:skel->skeletonHeaderSegments)if(!limb)return false;
    // Exact common-animation order in pinned En_Horse.c's D7xx table patch.
    const char* names[]={"gEponaIdleAnim","gEponaWhinnyAnim","gEponaRefuseAnim","gEponaRearingAnim","gEponaWalkingAnim","gEponaTrottingAnim","gEponaGallopingAnim","gEponaJumpingAnim","gEponaJumpingHighAnim"};
    const int frames[]={120,30,65,33,35,29,24,23,30};
    for(size_t i=0;i<9;++i){
        loaded[i+1]=mm->LoadResourceProcess(std::string("objects/combo_adult_epona/")+names[i]);
        auto animation=std::dynamic_pointer_cast<SOH::Animation>(loaded[i+1]);
        if(!animation||animation->type!=SOH::AnimationType::Normal||animation->animationData.animationHeader.common.frameCount!=frames[i]||!animation->animationData.animationHeader.frameData||!animation->animationData.animationHeader.jointIndices)return false;
    }
    for(size_t i=0;i<3;++i){
        loaded[i+10]=mm->LoadResourceProcess(sAdultEponaEyePaths[i]+7);
        auto texture=std::dynamic_pointer_cast<Fast::Texture>(loaded[i+10]);
        if(!texture||texture->Width!=32||texture->Height!=16||!texture->GetPointer())return false;
    }
    if(sAdultEponaOwner)DropAdultEpona();sAdultEponaOwner=mm;sAdultEponaResources=std::move(loaded);
    if(!sAdultEponaListener){Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultEpona);sAdultEponaListener=true;}
    return true;
}
}
extern "C" int Combo_MM_AdultEponaResourcesReady(void){return ComboAdultMM::LoadAdultEpona();}
extern "C" void* Combo_MM_GetAdultEponaBinding(int index){
    if(index<0||index>=13||!ComboAdultMM::LoadAdultEpona())return nullptr;
    // Eye segments follow native MM's named-texture path convention, retaining the typed owner.
    if(index>=10)return const_cast<char*>(ComboAdultMM::sAdultEponaEyePaths[index-10]);
    return ComboAdultMM::sAdultEponaResources[index]->GetRawPointer();
}
