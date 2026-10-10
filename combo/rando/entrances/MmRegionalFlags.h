// Pin expr/data.ts CONSTRAINT_FLAGS and pathfind.ts area-domain semantics.
#pragma once
#include <cstdint>
#include <functional>
#include <stdexcept>
namespace ComboRando::Entrances {
struct MmFlagDomain {uint8_t on=0,off=0;};
inline bool Valid(MmFlagDomain a){return !(a.on&a.off);}
inline bool Covers(MmFlagDomain a,MmFlagDomain b){return (a.on&b.on)==a.on&&(a.off&b.off)==a.off;}
inline MmFlagDomain Merge(MmFlagDomain a,MmFlagDomain b){return {uint8_t(a.on&b.on),uint8_t(a.off&b.off)};}
inline thread_local MmFlagDomain mmFlagSource,mmFlagResult;
inline thread_local uint8_t mmFlagAssigned=0,mmFlagAssignedMask=0,mmFlagReadMask=0;
inline bool MmFlagOn(unsigned bit){if(bit>=8)throw std::invalid_argument("Unknown MM regional flag");const uint8_t mask=uint8_t(1u<<bit);mmFlagReadMask|=mask;return !(mmFlagAssignedMask&mask)||(mmFlagAssigned&mask);}
inline bool MmFlagOff(unsigned bit){if(bit>=8)throw std::invalid_argument("Unknown MM regional flag");const uint8_t mask=uint8_t(1u<<bit);mmFlagReadMask|=mask;return !(mmFlagAssignedMask&mask)||!(mmFlagAssigned&mask);}
class MmFlagScope {
 MmFlagDomain old;public:explicit MmFlagScope(MmFlagDomain source):old(mmFlagSource){if(!Valid(source))throw std::invalid_argument("Conflicting MM regional flags");mmFlagSource=source;}
 ~MmFlagScope(){mmFlagSource=old;} MmFlagScope(const MmFlagScope&)=delete;
};
struct MmFlagEvaluation{bool allowed;MmFlagDomain domain;};
inline MmFlagEvaluation EvaluateMmFlags(const std::function<bool()>& predicate){
 const auto priorAssigned=mmFlagAssigned,priorMask=mmFlagAssignedMask,priorRead=mmFlagReadMask;
 struct Restore{uint8_t value,mask,reads;~Restore(){mmFlagAssigned=value;mmFlagAssignedMask=mask;mmFlagReadMask=reads;}} restore{priorAssigned,priorMask,priorRead};
 mmFlagAssignedMask=mmFlagSource.on|mmFlagSource.off;mmFlagAssigned=mmFlagSource.on;mmFlagReadMask=0;
 const bool first=predicate(); // Discover regional inputs even through short-circuited expressions.
 if(!(mmFlagReadMask&~mmFlagAssignedMask)){mmFlagResult=mmFlagSource;return {first,mmFlagResult};}
 uint8_t mask=mmFlagReadMask|mmFlagSource.on|mmFlagSource.off;
 for(;;){
  bool any=false;uint8_t allOn=255,allOff=255;mmFlagReadMask=0;
  const unsigned variable=mask&~(mmFlagSource.on|mmFlagSource.off);
  unsigned subset=variable;
  for(;;){
   const unsigned assignment=mmFlagSource.on|subset;
   mmFlagAssigned=uint8_t(assignment);mmFlagAssignedMask=mask;
   if(predicate()){any=true;allOn&=mmFlagAssigned;allOff&=uint8_t(~mmFlagAssigned);}
   if(!subset)break;
   subset=(subset-1)&variable;
  }
  const uint8_t expanded=mask|mmFlagReadMask;
  if(expanded!=mask){mask=expanded;continue;}
  mmFlagResult=any?MmFlagDomain{uint8_t(allOn&mask),uint8_t(allOff&mask)}:mmFlagSource;
  return {any,mmFlagResult};
 }
}
inline bool MmRegionalCursed(unsigned pair){return MmFlagOn(pair*2)&&MmFlagOff(pair*2+1);}
inline bool MmRegionalCleared(unsigned pair,bool available){return MmFlagOn(pair*2+1)&&MmFlagOff(pair*2)&&available;}
}
