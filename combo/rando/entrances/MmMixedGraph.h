#pragma once
#include "rando/entrances/MmMixedLayout.h"
#include "rando/entrances/MmGrottoGraph.h"
namespace ComboRando::Entrances {
struct MmMixedAssumptions{std::map<uint32_t,std::set<uint32_t>> routes;std::set<uint32_t> syntheticDestinations;};
inline std::optional<MmMixedAssumptions>& ActiveMmMixedAssumptions(){static thread_local std::optional<MmMixedAssumptions> value;return value;}
inline MmMixedAssumptions BuildMmMixedAssumptions(const Overrides& rows,const Pools& pools,const Settings& settings){
 MmMixedAssumptions result;
 auto source=[&](const Record* a){if(!MmMixedBound(a))throw std::invalid_argument("Unbound MM mixed assumed source");result.routes.try_emplace(MmMixedSourceKey(a->id));for(const auto& alias:kMmOrdinaryBindings)if(alias.name==a->name)result.routes.try_emplace(MmMixedSourceKey(alias.physicalId));};
 auto bind=[&](const Record* a,const Record* b){source(a);if(!MmMixedBound(b))throw std::invalid_argument("Unbound MM mixed assumed destination");result.routes[MmMixedSourceKey(a->id)].insert(b->id);for(const auto& alias:kMmOrdinaryBindings)if(alias.name==a->name)result.routes[MmMixedSourceKey(alias.physicalId)].insert(b->id);};
 for(const auto& [src,dst]:rows){if(IsMmGrottoClockTowerReservation(src,dst))continue;bind(Find(src),Find(dst));}
 for(const auto& pool:pools){
  const bool synthetic=pool.src.size()*pool.dst.size()>5000&&pool.src.size()>50;
  for(const auto src:pool.src){const auto* a=Find(src);source(a);if(!synthetic)for(const auto dst:pool.dst){const auto* b=Find(dst);if(!MmMixedBound(b))throw std::invalid_argument("Unbound MM mixed remaining destination");if(Assignable(*a,*b,pool.ownGame,settings))bind(a,b);}}
  if(synthetic)for(const auto dst:pool.dst){const auto* b=Find(dst);if(!MmMixedBound(b))throw std::invalid_argument("Unbound synthetic MM mixed arrival");result.syntheticDestinations.insert(b->id);}
 }
 return result;
}
inline bool MmMixedGraphOverrides(uint32_t source){if(const auto& assumed=ActiveMmMixedAssumptions();assumed&&assumed->routes.contains(MmMixedSourceKey(source)))return true;return ActiveMmMixed().containsSource(source);}
inline std::vector<uint32_t> MmMixedGraphDestinations(uint32_t source){if(const auto& assumed=ActiveMmMixedAssumptions()){auto i=assumed->routes.find(MmMixedSourceKey(source));if(i!=assumed->routes.end())return {i->second.begin(),i->second.end()};}return {ActiveMmMixed().destination(source)};}
inline nlohmann::json EncodeMmMixedAssumptions(const Overrides& rows,const Pools& pools,const Settings& settings){
 BuildMmMixedAssumptions(rows,pools,settings);
 nlohmann::json value={{"schemaVersion",1},{"profile","mm-mixed-coupled-assumptions-v1"},{"upstreamCommit",kUpstreamCommit},{"registrySha256",kRegistrySourceSha256},{"noPolarity",settings.erNoPolarity},{"selfLoops",settings.erSelfLoops},{"overworld",Active(settings.erOverworld)},{"overrides",nlohmann::json::object()},{"remainingPools",nlohmann::json::array()}};
 for(const auto& [src,dst]:rows)value["overrides"][std::string(src)]=dst;
 for(const auto& pool:pools)value["remainingPools"].push_back({{"name",pool.name},{"src",pool.src},{"dst",pool.dst},{"ownGame",pool.ownGame},{"alias",pool.alias}});
 return value;
}
inline MmMixedAssumptions DecodeMmMixedAssumptions(const nlohmann::json& value){
 if(!value.is_object()||value.value("schemaVersion",0)!=1||value.value("profile",std::string())!="mm-mixed-coupled-assumptions-v1"||value.value("upstreamCommit",std::string())!=kUpstreamCommit||value.value("registrySha256",std::string())!=kRegistrySourceSha256||!value.at("noPolarity").is_boolean()||!value.at("selfLoops").is_boolean())throw std::invalid_argument("Invalid MM mixed assumptions");
 Settings settings;settings.games=Games::Mm;settings.erNoPolarity=value.at("noPolarity");settings.erSelfLoops=value.at("selfLoops");if(value.at("overworld").get<bool>())settings.erOverworld=Mode::OwnGame;Overrides rows;Pools pools;
 for(const auto& [src,dst]:value.at("overrides").items()){auto a=Find(src),b=Find(dst.get<std::string>());if(!a||!b)throw std::invalid_argument("Unknown MM mixed assumed record");rows[a->name]=b->name;}
 for(const auto& p:value.at("remainingPools")){Pool pool;pool.name=p.at("name").get<std::string>();pool.ownGame=p.at("ownGame").get<bool>();pool.alias=p.at("alias").get<bool>();for(const auto& v:p.at("src")){auto a=Find(v.get<std::string>());if(!a)throw std::invalid_argument("Unknown mixed source");pool.src.insert(a->name);}for(const auto& v:p.at("dst")){auto a=Find(v.get<std::string>());if(!a)throw std::invalid_argument("Unknown mixed destination");pool.dst.insert(a->name);}pools.push_back(std::move(pool));}
 return BuildMmMixedAssumptions(rows,pools,settings);
}
}
