// Exact OoTMM41a2b7c0d7dbbb53fed928652f8271bc336c113f grotto returns;
// native Ship load identity/data schema. See LICENSE.OoTMM.
#pragma once
#include "rando/entrances/Registry.h"
#include <array>
#include <optional>
namespace ComboRando::Entrances {
struct OotNativeGrottoBinding {
 uint32_t input,output;int nativeOffset;uint16_t load,returnEntrance;uint8_t content;
 int returnRoom;std::array<int16_t,3> pos;
};
inline constexpr std::array<OotNativeGrottoBinding,33> kOotNativeGrottos{{
 {0x1000u,0x1100u,27,0x3f,0x286,0x2c,0,{-512,380,-1224}}, // OOT_GROTTO_GENERIC_KOKIRI_FOREST
 {0x1001u,0x1101u,26,0x3f,0x11e,0x14,2,{915,0,-925}}, // OOT_GROTTO_GENERIC_LOST_WOODS
 {0x1002u,0x1102u,10,0x3f,0x34d,0x28,0,{860,80,-260}}, // OOT_GROTTO_GENERIC_KAKARIKO
 {0x1003u,0x1103u,8,0x3f,0x1b9,0x57,0,{-383,1386,-1206}}, // OOT_GROTTO_GENERIC_DMT
 {0x1004u,0x1104u,6,0x3f,0x147,0x7a,1,{40,1233,1770}}, // OOT_GROTTO_GENERIC_DMC
 {0x1005u,0x1105u,4,0x3f,0xea,0x29,0,{360,570,130}}, // OOT_GROTTO_GENERIC_RIVER
 {0x1006u,0x1106u,20,0x3f,0x189,0x22,0,{-270,-500,12350}}, // OOT_GROTTO_GENERIC_HF_SOUTHEAST
 {0x1007u,0x1107u,19,0x3f,0x189,0x3,0,{-4030,-700,13860}}, // OOT_GROTTO_GENERIC_HF_OPEN
 {0x1008u,0x1108u,16,0x3f,0x1fd,0x0,0,{-1425,0,810}}, // OOT_GROTTO_GENERIC_HF_MARKET
 {0x1009u,0x1109u,24,0x36d,0xfc,0xff,0,{45,0,220}}, // OOT_GROTTO_FAIRY_SFM
 {0x100au,0x110au,15,0x36d,0x1fd,0xff,0,{-4450,-300,-425}}, // OOT_GROTTO_FAIRY_HF
 {0x100bu,0x110bu,3,0x36d,0xea,0xe6,0,{670,570,-365}}, // OOT_GROTTO_FAIRY_RIVER
 {0x100cu,0x110cu,28,0x36d,0x108,0xff,1,{-860,14,-470}}, // OOT_GROTTO_FAIRY_DOMAIN
 {0x100du,0x110du,29,0x36d,0x129,0xff,0,{376,333,-1564}}, // OOT_GROTTO_FAIRY_FORTRESS
 {0x100eu,0x110eu,23,0x5bc,0x600,0xee,0,{310,480,-2300}}, // OOT_GROTTO_SCRUBS2_SFM
 {0x100fu,0x110fu,2,0x5bc,0xea,0xeb,0,{-1630,100,-130}}, // OOT_GROTTO_SCRUBS2_RIVER
 {0x1010u,0x1110u,30,0x5bc,0x3d0,0xf0,0,{-1323,15,-969}}, // OOT_GROTTO_SCRUBS2_VALLEY
 {0x1011u,0x1111u,0,0x5bc,0x57c,0xfd,0,{60,-32,-1300}}, // OOT_GROTTO_SCRUBS2_COLOSSUS
 {0x1012u,0x1112u,21,0x5a4,0x378,0xfc,0,{1800,0,1500}}, // OOT_GROTTO_SCRUBS3_RANCH
 {0x1013u,0x1113u,7,0x5a4,0x14d,0xfb,3,{1100,580,-1190}}, // OOT_GROTTO_SCRUBS3_GORON_CITY
 {0x1014u,0x1114u,5,0x5a4,0x246,0xf9,1,{-1699,722,-472}}, // OOT_GROTTO_SCRUBS3_DMC
 {0x1015u,0x1115u,1,0x5a4,0x604,0xef,0,{-3040,-1033,6075}}, // OOT_GROTTO_SCRUBS3_LAKE
 {0x5b0u,0x1116u,25,0x5b0,0x11e,0xf5,8,{670,0,-2520}}, // OOT_GROTTO_SCRUB_UPGRADE
 {0x59cu,0x1117u,18,0x59c,0x189,0xe6,0,{-4990,-700,13820}}, // OOT_GROTTO_SCRUB_HEART_PIECE
 {0x5c4u,0x1118u,32,0x5c4,0x1a9,0xf3,6,{80,-20,-1600}}, // OOT_GROTTO_DEKU_THEATER
 {0x5b4u,0x1119u,22,0x5b4,0xfc,0xed,0,{-195,0,1900}}, // OOT_GROTTO_WOLFOS
 {0x5c0u,0x111au,13,0x5c0,0x1fd,0xe1,0,{-4945,-300,2835}}, // OOT_GROTTO_TEKTITE
 {0x5a8u,0x111bu,17,0x5a8,0x18d,0xe4,0,{-7870,-300,6920}}, // OOT_GROTTO_FIELD_COW
 {0x598u,0x111cu,14,0x598,0x17d,0xe5,0,{2060,20,-170}}, // OOT_GROTTO_FIELD_TREE
 {0x5fcu,0x111du,9,0x5fc,0x13d,0xf8,0,{-688,1946,-285}}, // OOT_GROTTO_TRAIL_COW
 {0x5b8u,0x111eu,12,0x5b8,0x138,0xf6,0,{996,1571,844}}, // OOT_GROTTO_CASTLE
 {0x5a0u,0x111fu,11,0x5a0,0xdb,0xe7,0,{-400,0,400}}, // OOT_GROTTO_REDEAD
 {0x5acu,0x1120u,31,0x5ac,0x117,0xf2,0,{280,-555,1470}}, // OOT_GROTTO_OCTOROK
}};
struct OotNativeArrival {uint32_t entrance;const OotNativeGrottoBinding* grotto=nullptr;bool returning=false;};
inline std::optional<OotNativeArrival> ResolveOotNativeArrival(uint32_t logical) {
 if(!Find(Game::Oot,logical))return std::nullopt;
 for(const auto& row:kOotNativeGrottos){
  if(logical==row.input)return OotNativeArrival{row.load,&row,false};
  if(logical==row.output)return OotNativeArrival{row.returnEntrance,&row,true};
 }
 // Exact playAdjustEntrance aliases; age is preserved by the request bridge.
 switch(logical){case 0xf20:return OotNativeArrival{0x5f4};
  case 0xf00:return OotNativeArrival{0x340};case 0xf01:return OotNativeArrival{0x11e};
  case 0x7a:return OotNativeArrival{0x400};case 0x296:return OotNativeArrival{0x23d};}
 return OotNativeArrival{logical};
}
template<class Respawn> inline void WriteOotNativeGrottoReturn(const OotNativeGrottoBinding& ge,Respawn& rs){
 // Verbatim applyGrottoExit fields, adapted only to native field names.
 rs.pos.x=ge.pos[0];rs.pos.y=ge.pos[1];rs.pos.z=ge.pos[2];rs.yaw=0;
 rs.entranceIndex=ge.returnEntrance;rs.playerParams=0x04ff;rs.data=0;
 rs.roomIndex=ge.returnRoom;rs.tempSwchFlags=0;rs.tempCollectFlags=0;
}
template<class Respawns,class Flag,class Transition> inline void ApplyOotNativeGrottoReturn(const OotNativeGrottoBinding& ge,
 Respawns& respawns,Flag& flag,Transition& nextTransition){
 WriteOotNativeGrottoReturn(ge,respawns[1]);respawns[0]=respawns[1];
 flag=2;nextTransition=3;
}
template<class Respawn> inline void PrepareOotNativeGrottoLoad(const OotNativeGrottoBinding& ge,Respawn& rs){
 // Native Door_Ana's RETURN owns the outside identity. A foreign arrival has
 // no Door_Ana source, so initialize that native point instead of reusing a
 // dormant scene's coordinates. The full native content byte selects the same
 // pinned generic/fairy/scrub identity, including native actor/check adapters.
 WriteOotNativeGrottoReturn(ge,rs);rs.data=ge.content;
}
}
