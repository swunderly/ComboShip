#pragma once
#include <fast/resource/type/Texture.h>
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sHoverCircleOot,sHoverCircleMm;
static std::array<std::shared_ptr<Fast::Texture>,2> sHoverCircleTextures{};
static bool sHoverCircleListener=false;
inline void DropHoverCircle(){sHoverCircleTextures={};sHoverCircleOot.reset();sHoverCircleMm.reset();}
inline bool LoadHoverCircle(){
 auto oot=Ship::CrossRMRegistry::Get("oot"),mm=Ship::CrossRMRegistry::Get("mm");if(!oot||!mm)return false;
 if(sHoverCircleOot==oot&&sHoverCircleMm==mm)return true;
 std::array<std::shared_ptr<Fast::Texture>,2> loaded{};
 loaded[0]=std::dynamic_pointer_cast<Fast::Texture>(oot->LoadResourceProcess("objects/gameplay_keep/gEffUnknown13Tex"));
 loaded[1]=std::dynamic_pointer_cast<Fast::Texture>(mm->LoadResourceProcess("objects/gameplay_keep/gameplay_keep_Tex_01AAB0"));
 for(const auto& t:loaded)if(!t||!t->GetRawPointer()||t->Type!=Fast::TextureType::Grayscale8bpp||t->Width!=16||t->Height!=32||t->ImageDataSize!=512)return false;
 if(sHoverCircleOot||sHoverCircleMm)DropHoverCircle();sHoverCircleOot=oot;sHoverCircleMm=mm;sHoverCircleTextures=std::move(loaded);
 if(!sHoverCircleListener){Ship::CrossRMRegistry::RegisterTeardownListener(DropHoverCircle);sHoverCircleListener=true;}return true;
}
}
extern "C" int Combo_MM_HoverCircleResourcesReady(void){return ComboAdultMM::LoadHoverCircle();}
