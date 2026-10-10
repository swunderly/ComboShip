// Native raw grotto/grave contexts, preserving disabled native predicates.
inline void EnsureMmRawGrottoContexts(){
 if(const auto bank=Regions.find(RR_MM_GROTTO_BEANS_BANK);bank!=Regions.end()){
  const auto lower=Regions.find(RR_DEKU_PALACE_INSIDE_LOWER);
  const auto fake=Regions.find(RR_MM_GRAVE_NIGHT3_FAKE_EXIT);
  if(bank->second.exits.contains(ENTRANCE(GROTTOS,12))&&
     bank->second.connections.contains(RR_DEKU_PALACE_INSIDE_LOWER)&&
     lower!=Regions.end()&&lower->second.connections.contains(RR_MM_GROTTO_BEANS_BANK)&&
     fake!=Regions.end()&&fake->second.exits.contains(0x8010))return;
  // A partial context must never silently preserve vanilla edges or receive a
  // second layer of condition wrappers. Fail without modifying the graph.
  throw std::logic_error("Incomplete native MM raw grotto contexts");
 }
 auto route=[&](RandoRegionId source,RandoRegionId bank,s32 entry,std::function<bool()> gate,uint32_t key){
  auto original=Regions.at(source).exits.at(entry);auto old=original.condition;
  Regions.at(source).exits.at(entry).condition=[old,key]{return !TypedMmGrottoSourceActive(key)&&old();};
  original.condition=[gate,key]{return TypedMmGrottoSourceActive(key)&&gate();};
  Regions.at(bank).exits.emplace(entry,std::move(original));
 };
 // The pin separates the Palace approach from the actual hole. Arbitrary
 // arrivals return to this bank and must not re-pay its age/mask approach gate.
 Regions[RR_MM_GROTTO_BEANS_BANK]=RandoRegion{.name="Near Beans Grotto",.sceneId=SCENE_22DEKUCITY,
  .connections={CONNECTION(RR_DEKU_PALACE_INSIDE_LOWER,TypedMmGrottoSourceActive(0x14c0u))}};
 Regions.at(RR_DEKU_PALACE_INSIDE_LOWER).connections.insert(CONNECTION(RR_MM_GROTTO_BEANS_BANK,
  TypedMmGrottoSourceActive(0x14c0u)&&(MM_LOGIC_IS_CHILD||HAS_ITEM(ITEM_MASK_DEKU)||CAN_SKIP_PALACE_BEANS)));
 route(RR_DEKU_PALACE_INSIDE_LOWER,RR_MM_GROTTO_BEANS_BANK,ENTRANCE(GROTTOS,12),[]{return true;},0x14c0u);
 // Clock Town North -> Playground is true at the pin. Only the game/checks
 // require Deku; preserve the standalone/disabled original predicate.
 auto& playground=Regions.at(RR_CLOCK_TOWN_NORTH).exits.at(ENTRANCE(DEKU_SCRUB_PLAYGROUND,0));
 auto legacyPlayground=playground.condition;
 playground.condition=[legacyPlayground]{return TypedMmGrottoSourceActive(0x3600u)||legacyPlayground();};
 for(const auto entry:{ENTRANCE(BENEATH_THE_GRAVERYARD,1),ENTRANCE(BENEATH_THE_GRAVERYARD,0),ENTRANCE(DAMPES_HOUSE,0)}){
  auto& grave=Regions.at(RR_IKANA_GRAVEYARD_LOWER).exits.at(entry);auto legacy=grave.condition;
  grave.condition=[legacy,entry]{return legacy()&&(!TypedMmGrottoSourceActive(entry)||HaveEnemySoul(ACTOR_EN_SKB));};
 }
 // Actual Night3 has only its native Dampe door0x8040 and Wallmaster. The
 // registry's0x8010 reverse starts at a disconnected NONE region in the pin;
 // never connect this fake region to the actual Night3 interior.
 Regions[RR_MM_GRAVE_NIGHT3_FAKE_EXIT]=RandoRegion{.name="Night3 Fake Exit",.sceneId=SCENE_MAX,
  .exits={EXIT(0x8010,ONE_WAY_EXIT,true)}};
}
