// Included after the owning engine's typed Scene/scene-command headers.
// Scene deliberately has no raw payload pointer; validate typed commands instead.
#pragma once
#include <ship/resource/ResourceManager.h>
#include <memory>
#include <string>
namespace ComboRando::Entrances {
struct NativeSceneSelection {
 std::shared_ptr<SOH::SetEntranceList> entrances;
 std::shared_ptr<SOH::SetStartPositionList> starts;
 std::shared_ptr<SOH::SetRoomList> rooms;
};
inline bool NativeSceneMetadata(const std::shared_ptr<SOH::Scene>& scene) {
 return scene&&!scene->IsDirty()&&scene->GetInitData()&&
  scene->GetInitData()->Type==0x4f524f4d&& !scene->commands.empty();
}
inline bool SelectNativeSceneCommands(const std::shared_ptr<SOH::Scene>& scene,int layer,
 bool ootAdultNightFallback,NativeSceneSelection& selection,unsigned depth=0) {
 if(!NativeSceneMetadata(scene)||layer<0||depth>16)return false;
 for(const auto& command:scene->commands) {
  if(!command)return false;
  switch(command->cmdId) {
   case SOH::SceneCommandID::SetEntranceList:
    selection.entrances=std::dynamic_pointer_cast<SOH::SetEntranceList>(command);
    if(!selection.entrances)return false;break;
   case SOH::SceneCommandID::SetStartPositionList:
    selection.starts=std::dynamic_pointer_cast<SOH::SetStartPositionList>(command);
    if(!selection.starts)return false;break;
   case SOH::SceneCommandID::SetRoomList:
    selection.rooms=std::dynamic_pointer_cast<SOH::SetRoomList>(command);
    if(!selection.rooms)return false;break;
   case SOH::SceneCommandID::SetAlternateHeaders: {
    const auto headers=std::dynamic_pointer_cast<SOH::SetAlternateHeaders>(command);
    if(!headers||headers->numHeaders!=headers->headers.size())return false;
    if(!layer)break;
    if(static_cast<size_t>(layer)>headers->headers.size())return false;
    auto desired=headers->headers[layer-1];
    // Exact native OoT fallback: absent adult-night header uses adult-day.
    if(!desired&&ootAdultNightFallback&&layer==3)desired=headers->headers[1];
    if(desired)return SelectNativeSceneCommands(desired,layer,ootAdultNightFallback,selection,depth+1);
    break;
   }
   case SOH::SceneCommandID::EndMarker:return true;
   default:break;
  }
 }
 return true;
}
inline bool ValidateOwnedNativeSpawn(const std::shared_ptr<Ship::ResourceManager>& owner,
 const std::string& path,int layer,int spawn,int respawnRoom,bool ootAdultNightFallback) {
 if(!owner||path.empty()||spawn<0)return false;
 try {
  const auto scene=std::dynamic_pointer_cast<SOH::Scene>(owner->LoadResource(path));
  NativeSceneSelection selection;
  if(!SelectNativeSceneCommands(scene,layer,ootAdultNightFallback,selection)||
     !selection.entrances||!selection.starts||!selection.rooms)return false;
  const auto& entries=*selection.entrances;const auto& starts=*selection.starts;const auto& rooms=*selection.rooms;
  if(entries.numEntrances!=entries.entrances.size()||starts.numStartPositions!=starts.startPositions.size()||
     rooms.numRooms!=rooms.rooms.size()||rooms.numRooms!=rooms.fileNames.size()||
     static_cast<size_t>(spawn)>=entries.entrances.size())return false;
  const auto& entrance=entries.entrances[spawn];
  const int room=respawnRoom>=0?respawnRoom:entrance.room;
  if(entrance.spawn>=starts.startPositions.size()||room<0||static_cast<size_t>(room)>=rooms.fileNames.size())return false;
  return NativeSceneMetadata(std::dynamic_pointer_cast<SOH::Scene>(owner->LoadResource(rooms.fileNames[room])));
 } catch(...) {return false;}
}
}
