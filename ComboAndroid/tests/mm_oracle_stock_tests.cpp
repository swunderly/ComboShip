#include "rando/ComboPlaythrough.h"
#include <cassert>
#include <map>

using namespace ComboRando;
static std::map<std::string,std::string> stock;
static std::string output;
static void ResetMm() { stock.clear(); }
static void SetOwned(const char*) {}
static void Place(const char* check,const char* item) { stock[check]=item; }
static const char* ReachMm() {
    nlohmann::json checks={"Potion shelf","Other shelf"};
    if (stock["Potion shelf"]=="Blue Potion Refill") {
        checks.push_back("Potion gate");checks.push_back("Defeat Majora");
    }
    output=checks.dump();return output.c_str();
}
static void ResetOot() {}
static void PlaceOot(const char*,const char*) {}
static const char* ReachOot() { return "[\"Ganon\"]"; }
static uint8_t PortalOpen() { return 1; }
static const char* FillReachMm() {
    nlohmann::json checks={"Trading Post Shop Item 01","Trading Post Shop Item 02","Normal reward"};
    for (const auto& [check,item] : stock) {
        if (check.find("Shop") != std::string::npos && item=="Blue Potion Refill") {
            checks.push_back("Potion gate");checks.push_back("Defeat Majora");break;
        }
    }
    output=checks.dump();return output.c_str();
}
int main() {
    OracleFns oot{ResetOot,SetOwned,ReachOot,nullptr};
    OracleFns mm{ResetMm,SetOwned,ReachMm,Place};
    const std::vector<CwPlacedItem> placements={
        {GAME_MM,"Potion shelf",GAME_MM,"Blue Potion Refill",true,true},
        {GAME_MM,"Other shelf",GAME_OOT,"Blue Potion Refill",true,true}};
    // Inventory queries used to erase renewable shelves. Every fresh query must
    // reinstall this seed's stock; an identically named foreign item is not a
    // local refill. A counterfactual removing the shelf must remove its supply.
    for(int i=0;i<3;++i) {
        const auto reach=QueryReachableWithMmStock(mm,{},true,placements);
        assert(reach.count("Potion gate"));
        assert(stock.at("Other shelf").empty());
    }
    assert(!QueryReachableWithMmStock(mm,{},true,placements,{0}).count("Potion gate"));
    const nlohmann::json spoiler={
        {"oot",nlohmann::json::object()},
        {"mm",{{"Potion shelf","Blue Potion Refill"},{"Potion gate","Recovery Heart"}}}};
    nlohmann::json spheres;
    auto walk=RunPlaythrough(spoiler.dump(),oot,mm,"stock-regression",nullptr,&spheres,"","",false);
    assert(walk.beatable && walk.majoraReachable);
    auto required=PareDownPlaythrough(spoiler.dump(),oot,mm,nullptr,"","",{}, {},DefaultGanonMajoraGoal,false);
    assert(required.requiredByCheck.at("mm:Potion shelf"));
    assert(!required.requiredByCheck.at("mm:Potion gate"));

    // The first critical potion must survive balancing and land at renewable
    // stock. Other copies retain unrestricted randomized placement. Across
    // seeds, a one-time reward cannot masquerade as a repeatable source.
    const nlohmann::json ootDump={
        {"checks",{{{"name","Ganon"}}}},
        {"pool",{{{"name","Recovery Heart"},{"advancement",false},{"category","junk"}}}}};
    nlohmann::json mmDump;
    mmDump["checks"]=nlohmann::json::array();
    for(const auto* check:{"Trading Post Shop Item 01","Trading Post Shop Item 02","Normal reward","Potion gate"})
        mmDump["checks"].push_back({{"name",check}});
    mmDump["pool"]={
        {{"name","Blue Potion Refill"},{"advancement",true},{"category","junk"},{"criticalRenewable",true}},
        {{"name","Blue Potion Refill"},{"advancement",true},{"category","junk"},{"criticalRenewable",true}},
        {{"name","Recovery Heart"},{"advancement",false},{"category","junk"}},
        {{"name","Recovery Heart"},{"advancement",false},{"category","junk"}}};
    OracleFns fillOot{ResetOot,SetOwned,ReachOot,PlaceOot,PortalOpen};
    OracleFns fillMm{ResetMm,SetOwned,FillReachMm,Place};
    for(uint32_t seed=1;seed<=25;++seed) {
        const auto result=CrossWorldCombinedFill(ootDump.dump(),mmDump.dump(),seed,fillOot,fillMm);
        assert(result.success);
        const auto combined=nlohmann::json::parse(result.spoilerJson);
        int potions=0,renewablePotions=0;
        for(const auto& [check,item] : combined.at("mm").items()) {
            if(item=="Blue Potion Refill") {
                ++potions;if(check.find("Shop")!=std::string::npos)++renewablePotions;
            }
        }
        for(const auto& [check,item] : combined.at("oot").items()) {
            (void)check;if(item=="Blue Potion Refill")++potions;
        }
        assert(potions==2 && renewablePotions>=1);
    }
}
