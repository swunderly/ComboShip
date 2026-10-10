#pragma once
#include <vector>
#include <cstddef>
namespace Rando {
// Mirrors pinned gi_sutaru.S patches against imported OOT display-list commands.
// Expanded hash commands are two entries; never interpret their payload as opcodes.
template<class Command> bool PrepareExpandedSoulModel(const std::vector<Command>& sourceBody,
 const std::vector<Command>& sourceFlame,std::vector<Command>& body,
 std::vector<Command>& flame,std::vector<Command>& jaw) {
 auto colors=[](const std::vector<Command>& source) {
  std::vector<size_t> positions;
  for(size_t i=0;i<source.size();++i){unsigned op=(source[i].words.w0>>24)&255;
   if(op==0xfa || op==0xfb) positions.push_back(i);
   if(op==0x20||op==0x31||op==0x32||op==0x33||op==0x35||op==0x36||op==0x42) ++i;
  } return positions;
 };
 auto bc=colors(sourceBody),fc=colors(sourceFlame);
 if(bc.size()!=2||fc.size()!=6||fc[2]<2||sourceFlame[fc[2]-1].words.w0>>24!=0xfc||sourceFlame[fc[2]-2].words.w0>>24!=0xe7) return false;
 body=sourceBody;for(auto i:bc){body[i].words.w0=0;body[i].words.w1=0;}
 // 0x500 becomes ENDDL; 0x508 starts the independent jaw list.
 size_t split=fc[2]-2;flame.assign(sourceFlame.begin(),sourceFlame.begin()+split+1);
 flame.back().words.w0=0xdf000000;flame.back().words.w1=0;
 for(unsigned i=0;i<2;++i){flame[fc[i]].words.w0=0;flame[fc[i]].words.w1=0;}
 jaw.assign(sourceFlame.begin()+split+1,sourceFlame.end());
 for(unsigned i=2;i<4;++i){jaw[fc[i]-split-1].words.w0=0;jaw[fc[i]-split-1].words.w1=0;}
 return true;
}
inline constexpr unsigned ExpandedSoulFlameColor(unsigned index) {
 return index<4?0x2222ffff:index<6?0x888888ff:0x00ff00ff;
}
}
