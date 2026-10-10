// Actual native source EXIT predicates; synthesized destination ACCESS is moved to arrival.
inline std::function<bool()> MmOrdinarySourcePredicate(RandoRegionId region,s32 exit,const std::function<bool()>& vanilla){
 if(region==RR_BENEATH_THE_WELL_BACK_EXIT && exit==13312)return []{return true;};
 if(region==RR_BENEATH_THE_WELL_ENTRANCE && exit==8272)return []{return true;};
 if(region==RR_SECRET_SHRINE_ENTRANCE && exit==8384)return []{return true;};
 if(region==RR_GREAT_BAY_TEMPLE_ENTRANCE && exit==27248)return []{return HAS_ITEM(ITEM_HOOKSHOT);};
 if(region==RR_IKANA_CASTLE_MAIN_ROOM && exit==13344)return []{return true;};
 if(region==RR_PIRATES_FORTRESS_MOAT_LOWER && exit==26704)return []{return CAN_BE_ZORA && CAN_USE_ABILITY(SWIM);};
 if(region==RR_SNOWHEAD_TEMPLE_ENTRANCE_BEFORE_BLOCK && exit==45584)return []{return true;};
 if(region==RR_OCEAN_SPIDER_HOUSE_ENTRANCE_UPPER && exit==26752)return []{return true;};
 if(region==RR_SWAMP_SPIDER_HOUSE_ENTRANCE && exit==33920)return []{return true;};
 if(region==RR_STONE_TOWER_TEMPLE_ENTRANCE && exit==43552)return []{return true;};
 if(region==RR_STONE_TOWER_TEMPLE_INVERTED_ENTRANCE && exit==44048)return []{return true;};
 if(region==RR_WOODFALL_TEMPLE_ENTRANCE && exit==34320)return []{return true;};
 return vanilla;
}
