// Coupled same-MM ordinary + full-US grotto identities. MIT; see LICENSE.OoTMM.
#pragma once
#include "rando/entrances/MmOrdinaryLayout.h"
#include "rando/entrances/MmGrottoLayout.h"
#include "rando/entrances/Shuffle.h"
namespace ComboRando::Entrances {
inline bool MmMixedBound(const Record* r){return r&&r->game==Game::Mm&&!r->reverse.empty()&&(MmOrdinaryBound(r->name)||MmGrottoBound(r->name));}
inline uint32_t MmMixedSourceKey(uint32_t id){return id>0xffffu?id:CanonicalMmOrdinarySource(id);}
inline bool MmMixedOrdinarySource(uint32_t source){for(const auto& row:kMmOrdinaryBindings)if(MmMixedSourceKey(row.physicalId)==MmMixedSourceKey(source))return true;return false;}
class MmMixedLayout {
 std::map<uint32_t,uint32_t> routes; nlohmann::json saved;
public:
 void clear(){routes.clear();saved=nlohmann::json();}
 bool active()const{return !saved.is_null();}
 bool containsSource(uint32_t source)const{return routes.contains(MmMixedSourceKey(source));}
 uint32_t destination(uint32_t source)const{auto i=routes.find(MmMixedSourceKey(source));return i==routes.end()?source:i->second;}
 const nlohmann::json& json()const{return saved;}
 void install(const nlohmann::json& value){
  MmMixedLayout next;
  if(!value.is_object()||value.value("schemaVersion",0)!=1||value.value("profile",std::string())!="mm-mixed-coupled-v1"||value.value("upstreamCommit",std::string())!=kUpstreamCommit||value.value("registrySha256",std::string())!=kRegistrySourceSha256||!value.at("noPolarity").is_boolean()||!value.at("selfLoops").is_boolean()||!value.at("overworld").is_boolean()||!value.at("overrides").is_array())throw std::invalid_argument("Invalid coupled MM mixed envelope");
  Settings policy;policy.erNoPolarity=value.at("noPolarity");policy.erSelfLoops=value.at("selfLoops");if(value.at("overworld").get<bool>())policy.erOverworld=Mode::OwnGame;
  std::map<std::string,std::string> named;
  for(const auto& row:value.at("overrides")){
   const auto src=row.at("source").get<std::string>(),dst=row.at("destination").get<std::string>();const auto* a=Find(src);const auto* b=Find(dst);
   if(!MmMixedBound(a)||!MmMixedBound(b))throw std::invalid_argument("Unbound coupled MM mixed route: "+src+" -> "+dst);
   const auto* ra=Reverse(*a);const auto* rb=Reverse(*b);
   // Pin tests assignability on the sampled row, then adds its coupled reverse
   // without re-testing asymmetric self-loop metadata. Either orientation may
   // have been sampled; requiring both would reject valid generated layouts.
   if(!MmMixedBound(ra)||!MmMixedBound(rb)||(!Assignable(*a,*b,true,policy)&&!Assignable(*rb,*ra,true,policy)))throw std::invalid_argument("Incompatible coupled MM mixed pair: "+src+" -> "+dst);
   if(!named.emplace(src,dst).second||!next.routes.emplace(MmMixedSourceKey(a->id),b->id).second)throw std::invalid_argument("Duplicate MM mixed source");
   for(const auto& alias:kMmOrdinaryBindings)if(alias.name==src){auto [it,ok]=next.routes.emplace(MmMixedSourceKey(alias.physicalId),b->id);if(!ok&&it->second!=b->id)throw std::invalid_argument("Ambiguous MM mixed physical source");}
  }
  for(const auto& [src,dst]:named){auto a=Find(src),b=Find(dst);auto reverse=named.find(std::string(b->reverse));if(reverse==named.end()||reverse->second!=a->reverse)throw std::invalid_argument("Uncoupled MM mixed return");}
  next.saved=value;*this=std::move(next);
 }
};
inline MmMixedLayout& ActiveMmMixed(){static MmMixedLayout value;return value;}
inline void ValidateMmMixedSeed(const nlohmann::json& seed){
 if(!seed.contains("mixedEntranceExpansion"))return;
 if(seed.contains("ordinaryEntranceExpansion")||seed.contains("grottoEntranceExpansion"))throw std::invalid_argument("Conflicting separate and mixed MM entrance layouts");
 MmMixedLayout proof;proof.install(seed.at("mixedEntranceExpansion"));
}
}
