#pragma once
#include "NativeBridge.h"
#include "MmOrdinaryGraph.h"
#include "MmOrdinaryAreas.h"
namespace ComboRando::Entrances {
inline nlohmann::json EncodeMmOrdinaryExtension(const Overrides& overrides){
 nlohmann::json value={{"schemaVersion",1},{"profile","mm-ordinary-coupled-v1"},{"upstreamCommit",kUpstreamCommit},{"registrySha256",kRegistrySourceSha256},{"overrides",nlohmann::json::array()}};
 for(const auto& [src,dst]:overrides)value["overrides"].push_back({{"source",src},{"destination",dst}});
 MmOrdinaryLayout proof;proof.install(value);return value;
}
class MmOrdinaryAssumptionScope{
 NativeFns native;
public:
 MmOrdinaryAssumptionScope(NativeFns f,const Overrides& rows,const Pools& pools,const Settings& settings):native(f){
  if(!native.setAssumptions||!native.clearAssumptions)throw std::runtime_error("MM ordinary assumptions exports unavailable");
  if(!native.setAssumptions(EncodeMmOrdinaryAssumptions(rows,pools,settings).dump().c_str()))throw std::runtime_error("MM ordinary assumptions rejected");
 }
 ~MmOrdinaryAssumptionScope(){native.clearAssumptions();}
 MmOrdinaryAssumptionScope(const MmOrdinaryAssumptionScope&)=delete;
};
inline void ApplyNativeMmOrdinary(const NativeFns& native,const nlohmann::json& extension){
 MmOrdinaryLayout proof;proof.install(extension);
 if(!native.applySeed||!native.dumpSeed)throw std::runtime_error("MM ordinary seed exports unavailable");
 auto current=nlohmann::json::parse(native.dumpSeed());current["ordinaryEntranceExpansion"]=extension;
 if(!native.applySeed(current.dump().c_str()))throw std::runtime_error("MM ordinary seed rejected");
 // An older engine must not silently ignore the new optional JSON field.
 auto loaded=nlohmann::json::parse(native.dumpSeed());
 if(!loaded.contains("ordinaryEntranceExpansion")||loaded["ordinaryEntranceExpansion"]!=extension)throw std::runtime_error("MM ordinary seed failed actual engine roundtrip");
}
// Ordinary native graph coverage must not silently shrink a requested full pool.
// Unlike JP grotto contexts, these ordinary registry identities exist in Default US.
inline void ValidateMmOrdinaryGenerationCoverage(const Settings& settings){
 Types selected;for(const auto& pool:MakePools(settings)){
  selected.insert(pool.src.begin(),pool.src.end());selected.insert(pool.dst.begin(),pool.dst.end());
 }
 auto require=[](const Record& record){
  if(!MmOrdinaryBound(record.name)||
     (record.from!="NONE"&&!MmOrdinaryNativeAreaExists(record.from))||
     (record.to!="NONE"&&!MmOrdinaryNativeAreaExists(record.to)))
   throw std::invalid_argument("Requested MM ordinary pool has no complete native binding: "+std::string(record.name));
 };
 for(const auto& record:kRegistry)if(record.game==Game::Mm&&selected.contains(record.type)){
  require(record);if(const auto* reverse=Reverse(record))require(*reverse);
 }
}
inline nlohmann::json GenerateNativeMmOrdinary(Settings settings,uint32_t seed,const NativeFns& native,
 const std::string& ootDump,const std::string& mmDump,const std::string& forcedOot,
 const OracleFns& oot,const OracleFns& mm,bool mmStart,CwGoal goal){
 if(MakePools(settings).empty())return nlohmann::json();
 if(settings.games!=Games::Mm||settings.erDecoupled||settings.erBoss!=Mode::None||settings.erWallmasters!=Mode::None||settings.erOneWays!=Mode::None||settings.erWarps!=Warps::None||settings.erSpawns!=Spawns::None)
  throw std::invalid_argument("MM ordinary generator requires coupled MM-only ordinary pools");
 ValidateMmOrdinaryGenerationCoverage(settings);
 NativeScope restore(native);AssumedWorld world(ootDump,mmDump,forcedOot);
 CwRng rng(CwHashName("entrances:mm-ordinary-coupled-v1:"+std::to_string(seed)));Bindings bindings;
 bindings.areaExists=MmOrdinaryNativeAreaExists;
 bindings.sourceSupported=[](const Record& record){return MmOrdinaryBound(record.name);};
 bindings.destinationSupported=bindings.sourceSupported;
 bindings.chooseIndex=[&](size_t count){CheckGenerationCancellation();return rng.below(static_cast<uint32_t>(count));};
 bindings.validateAssumed=[&](const Overrides& rows,const Pools& pools){
  const auto graph=BuildMmOrdinaryAssumptions(rows,pools,settings);
  MmOrdinaryAssumptionScope scope(native,rows,pools,settings);
  // Pinned OOT SPAWN is unconditional; ASSUMED permits entry without the real portal.
  return world.validate(oot,mm,mmStart||!graph.syntheticDestinations.empty(),goal);
 };
 bindings.validateComplete=[&](const Overrides& rows){ApplyNativeMmOrdinary(native,EncodeMmOrdinaryExtension(rows));return world.validate(oot,mm,mmStart,goal);};
 auto rows=Shuffler(settings,std::move(bindings)).run();return EncodeMmOrdinaryExtension(rows);
}
}
