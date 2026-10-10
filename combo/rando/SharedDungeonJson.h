#pragma once
#include "SharedDungeon.h"
#include <nlohmann/json.hpp>
#include <cmath>
inline void to_json(nlohmann::json& j,const ComboSharedDungeon& d) {
 j={{"valid",d.valid},{"game",d.game},{"age",d.age},{"form",d.form},
 {"pos",{{"x",d.x},{"y",d.y},{"z",d.z}}},{"yaw",d.yaw},{"playerParams",d.playerParams},
 {"entrance",d.entrance},{"roomIndex",d.roomIndex},{"data",d.data},
 {"tempSwitchFlags",d.tempSwitchFlags},{"unk18",d.unk18},{"tempCollectFlags",d.tempCollectFlags}};
}
inline void from_json(const nlohmann::json& j,ComboSharedDungeon& out) {
 auto bounded=[&](const char* field,double low,double high) {
  const auto& v=j.at(field);
  if(!v.is_number_integer()||v.get<double>()<low||v.get<double>()>high)
   throw std::invalid_argument("Invalid tagged dungeon scalar");
 };
 bounded("valid",0,1);bounded("game",0,1);bounded("age",0,1);bounded("form",0,4);
 bounded("yaw",-32768,32767);bounded("playerParams",-32768,32767);bounded("entrance",-2147483648.0,2147483647.0);
 bounded("roomIndex",-128,127);bounded("data",-128,127);
 bounded("tempSwitchFlags",0,4294967295.0);bounded("unk18",0,4294967295.0);bounded("tempCollectFlags",0,4294967295.0);
 ComboSharedDungeon d{};j.at("valid").get_to(d.valid);j.at("game").get_to(d.game);
 j.at("age").get_to(d.age);j.at("form").get_to(d.form);j.at("pos").at("x").get_to(d.x);
 j.at("pos").at("y").get_to(d.y);j.at("pos").at("z").get_to(d.z);
 j.at("yaw").get_to(d.yaw);j.at("playerParams").get_to(d.playerParams);j.at("entrance").get_to(d.entrance);
 j.at("roomIndex").get_to(d.roomIndex);j.at("data").get_to(d.data);
 j.at("tempSwitchFlags").get_to(d.tempSwitchFlags);j.at("unk18").get_to(d.unk18);
 j.at("tempCollectFlags").get_to(d.tempCollectFlags);
 if(!std::isfinite(d.x)||!std::isfinite(d.y)||!std::isfinite(d.z))throw std::invalid_argument("Invalid tagged dungeon position");
 out=d;
}
namespace ComboFarore {
template<class R> ComboSharedDungeon OotDungeon(const R& p,int age) {
 return {1,0,static_cast<uint8_t>(age),4,p.pos.x,p.pos.y,p.pos.z,p.yaw,p.playerParams,
 p.entranceIndex,static_cast<int8_t>(p.roomIndex),static_cast<int8_t>(p.data),p.tempSwchFlags,0,p.tempCollectFlags};
}
template<class R> ComboSharedDungeon MmDungeon(const R& p,int age,int form) {
 return {1,1,static_cast<uint8_t>(age),static_cast<uint8_t>(form),p.pos.x,p.pos.y,p.pos.z,
 p.yaw,p.playerParams,p.entrance,static_cast<int8_t>(p.roomIndex),static_cast<int8_t>(p.data),p.tempSwitchFlags,p.unk_18,p.tempCollectFlags};
}
template<class R> bool ApplyOotDungeon(const ComboSharedDungeon& d,R& p) {
 if(!d.valid||d.game!=0)return false;
 p.pos={d.x,d.y,d.z};p.yaw=d.yaw;p.playerParams=d.playerParams;p.entranceIndex=d.entrance;
 p.roomIndex=d.roomIndex;p.data=d.data;p.tempSwchFlags=d.tempSwitchFlags;p.tempCollectFlags=d.tempCollectFlags;return true;
}
template<class R> bool ApplyMmDungeon(const ComboSharedDungeon& d,R& p) {
 if(!d.valid||d.game!=1)return false;
 p.pos={d.x,d.y,d.z};p.yaw=d.yaw;p.playerParams=d.playerParams;p.entrance=d.entrance;
 p.roomIndex=d.roomIndex;p.data=d.data;p.tempSwitchFlags=d.tempSwitchFlags;p.unk_18=d.unk18;
 p.tempCollectFlags=d.tempCollectFlags;return true;
}
}
