#pragma once
// Native typed imports of the exact pinned artist Adult Mask objects.
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sAdultMaskOwner;
static std::array<std::shared_ptr<Fast::DisplayList>,4> sAdultMaskResources{};
static bool sAdultMaskListener=false;
inline void DropAdultMaskResources(){sAdultMaskResources={};sAdultMaskOwner.reset();}
inline bool LoadAdultMaskResources(){
    auto mm=Ship::CrossRMRegistry::Get("mm");if(!mm)return false;
    if(sAdultMaskOwner==mm)return true;
    const char* paths[]={"objects/combo_adult_mask_gi/gComboAdultMaskGiOpaqueDL","objects/combo_adult_mask_gi/gComboAdultMaskGiTranslucentDL","objects/combo_adult_mask_transform/gComboAdultMaskTransformDL","objects/combo_adult_mask_equipment/gComboAdultMaskEquipmentDL"};
    std::array<std::shared_ptr<Fast::DisplayList>,4> loaded{};
    for(size_t i=0;i<4;++i){loaded[i]=std::dynamic_pointer_cast<Fast::DisplayList>(mm->LoadResourceProcess(paths[i]));if(!loaded[i]||loaded[i]->Instructions.empty())return false;}
    if(sAdultMaskOwner)DropAdultMaskResources();sAdultMaskOwner=mm;sAdultMaskResources=std::move(loaded);
    if(!sAdultMaskListener){Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultMaskResources);sAdultMaskListener=true;}return true;
}
}
extern "C" int Combo_MM_AdultMaskResourcesReady(void){return ComboAdultMM::LoadAdultMaskResources();}
extern "C" void* Combo_MM_GetAdultMaskBinding(int index){if(index<0||index>=4||!ComboAdultMM::LoadAdultMaskResources())return nullptr;return ComboAdultMM::sAdultMaskResources[index]->GetRawPointer();}
