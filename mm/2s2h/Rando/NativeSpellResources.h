#pragma once
#include <fast/resource/type/Texture.h>
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/ResourceType.h>
#include "2s2h/resource/type/Animation.h"
#include "2s2h/resource/type/PlayerAnimation.h"
#include "2s2h/resource/type/2shResourceType.h"
#include <ship/resource/ResourceManager.h>
#include <string>
#include <cstdint>

namespace ComboNativeSpells {
// Player_Init uses flags1|8: native Human has21 joints plus the root, and each
// packed frame contains PLAYER_LIMB_MAX Vec3s followed by its two flag bytes.
inline constexpr size_t kFrameBytes = sizeof(Vec3s) * PLAYER_LIMB_MAX + sizeof(u16);
inline bool Typed(const std::shared_ptr<Ship::IResource>& r,uint32_t type) {
 const auto init=r?r->GetInitData():nullptr;
 return init&&init->Type==type&&!r->IsDirty()&&r->GetRawPointer()&&r->GetPointerSize();
}
inline bool Texture(const std::shared_ptr<Ship::IResource>& r,Fast::TextureType type,
                    unsigned width,unsigned height,size_t bytes) {
 if(!Typed(r,static_cast<uint32_t>(Fast::ResourceType::Texture)))return false;
 const auto texture=std::dynamic_pointer_cast<Fast::Texture>(r);
 return texture&&texture->Type==type&&texture->Width==width&&texture->Height==height&&
        texture->GetPointerSize()>=bytes;
}
inline bool Valid(const std::shared_ptr<Ship::IResource>& r,const char* path,
                  const std::shared_ptr<Ship::IResource>& frameOwner={}) {
 const std::string name(path);
 if(name.starts_with("objects/gameplay_keep/gPlayerAnim_link_magic_")) {
  if(!Typed(r,static_cast<uint32_t>(SOH::ResourceType::SOH_Animation))||
     !Typed(frameOwner,static_cast<uint32_t>(SOH::ResourceType::SOH_PlayerAnimation)))return false;
  const auto header=std::dynamic_pointer_cast<SOH::Animation>(r);
  const auto frames=std::dynamic_pointer_cast<SOH::PlayerAnimation>(frameOwner);
  return header&&frames&&header->type==SOH::AnimationType::Link&&
         header->animationData.linkAnimationHeader.common.frameCount>0&&
         header->animationData.linkAnimationHeader.segment==frames->GetPointer()&&
         frames->GetPointerSize()==size_t(header->animationData.linkAnimationHeader.common.frameCount)*kFrameBytes;
 }
 if(name.starts_with("textures/icon_item_static/"))
  return Texture(r,Fast::TextureType::RGBA32bpp,32,32,4096);
 if(name.starts_with("textures/item_name_static/"))
  return Texture(r,Fast::TextureType::GrayscaleAlpha4bpp,128,16,1024);
 if(name=="overlays/ovl_Magic_Dark/sDiamondTex")
  return Texture(r,Fast::TextureType::Grayscale8bpp,32,64,2048);
 if(name=="overlays/ovl_Magic_Wind/sTex"||name=="overlays/ovl_Magic_Fire/sTex")
  return Texture(r,Fast::TextureType::Grayscale8bpp,64,64,4096);
 if(name=="objects/gameplay_keep/gameplay_keep_Tex_028BB8")
  return Texture(r,Fast::TextureType::Grayscale8bpp,32,32,1024);
 if(!Typed(r,static_cast<uint32_t>(Fast::ResourceType::DisplayList)))return false;
 const auto dl=std::dynamic_pointer_cast<Fast::DisplayList>(r);
 return dl&&!dl->Instructions.empty()&&dl->GetPointerSize()==dl->Instructions.size()*sizeof(Gfx);
}
inline std::shared_ptr<Ship::IResource> Load(const std::shared_ptr<Ship::ResourceManager>& rm,
                                           const char* path,std::shared_ptr<Ship::IResource>& frameOwner) {
 auto resource=rm->LoadResource(path);
 const std::string name(path);
 if(name.starts_with("objects/gameplay_keep/gPlayerAnim_link_magic_")) {
  const auto leaf=name.substr(name.find_last_of('/')+1);
  frameOwner=rm->LoadResource("misc/link_animetion/"+leaf+"_Data");
 }
 return Valid(resource,path,frameOwner)?resource:nullptr;
}
}
