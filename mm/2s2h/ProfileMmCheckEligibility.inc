// Source GeneratePools.cpp type gates; graph membership is captured without generating or consuming RNG.
std::map<int,RandoOptionId> profileCheckFlags={{RCTYPE_SKULL_TOKEN,RO_SHUFFLE_GOLD_SKULLTULAS},{RCTYPE_OWL,RO_SHUFFLE_OWL_STATUES},{RCTYPE_POT,RO_SHUFFLE_POT_DROPS},{RCTYPE_CRATE,RO_SHUFFLE_CRATE_DROPS},{RCTYPE_BARREL,RO_SHUFFLE_BARREL_DROPS},{RCTYPE_GRASS,RO_SHUFFLE_GRASS_DROPS},{RCTYPE_BEEHIVE,RO_SHUFFLE_HIVE_DROPS},{RCTYPE_TREE,RO_SHUFFLE_TREE_DROPS},{RCTYPE_BUTTERFLY,RO_SHUFFLE_BUTTERFLIES},{RCTYPE_FREESTANDING,RO_SHUFFLE_FREESTANDING_ITEMS},{RCTYPE_SNOWBALL,RO_SHUFFLE_SNOWBALL_DROPS},{RCTYPE_FROG,RO_SHUFFLE_FROGS},{RCTYPE_REMAINS,RO_SHUFFLE_BOSS_REMAINS},{RCTYPE_COW,RO_SHUFFLE_COWS},{RCTYPE_ENEMY_DROP,RO_SHUFFLE_ENEMY_DROPS},{RCTYPE_WONDER_ITEM,RO_SHUFFLE_WONDER_ITEMS},{RCTYPE_TINGLE_SHOP,RO_SHUFFLE_TINGLE_SHOPS},{RCTYPE_SHOP,RO_SHUFFLE_SHOPS}};
data["checkRequirements"]=nlohmann::json::object();
for(const auto& [regionId,region]:Rando::Logic::Regions)for(const auto& [id,predicate]:region.checks){
 const auto& check=Rando::StaticData::Checks.at(id);if(id==RC_UNKNOWN||check.sceneId==SCENE_LAST_BS)continue;
 nlohmann::json row={{"vanillaItem",profileText(Rando::StaticData::Items.at(check.randoItemId).spoilerName)},{"junk",Rando::StaticData::Items.at(check.randoItemId).randoItemType==RITYPE_JUNK}};
 auto flag=profileCheckFlags.find(check.randoCheckType);
 if(flag!=profileCheckFlags.end()&&!(check.randoCheckType==RCTYPE_SHOP&&(id==RC_CURIOSITY_SHOP_SPECIAL_ITEM||id==RC_BOMB_SHOP_ITEM_03||id==RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM)))row["enabledKey"]=profileText(Rando::StaticData::Options.at(flag->second).cvar);
 if(check.randoCheckType==RCTYPE_SKULL_TOKEN)row["countKey"]=profileText(Rando::StaticData::Options.at(RO_SKULLTULA_SHUFFLED).cvar);
 data["checkRequirements"][profileText(check.name)]=row;
}
