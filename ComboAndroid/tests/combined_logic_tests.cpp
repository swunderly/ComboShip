#include "rando/ComboPlaythrough.h"
#include "rando/MmAbilities.h"
#include "rando/AgeTravel.h"
#include "rando/MmTricks.h"
#include "rando/CrossHints.h"
#include "rando/MmAgeLogic.h"
#include <cassert>
#include <iostream>
using namespace ComboRando;
static std::vector<std::string> owned[2];
static bool blockMm=false, blockFixed=false, won=true;
static void resetOot(){owned[0].clear();}
static void resetMm(){owned[1].clear();}
static void grantOot(const char* text){owned[0]=nlohmann::json::parse(text).get<std::vector<std::string>>();}
static void grantMm(const char* text){owned[1]=nlohmann::json::parse(text).get<std::vector<std::string>>();}
static bool has(int game,const char* item){return std::find(owned[game].begin(),owned[game].end(),item)!=owned[game].end();}
static const char* ootReach(){
    static std::string buffer;
    nlohmann::json checks={"Oot start"};
    if(won) checks.push_back("Ganon");
    if(has(0,"Open optional")) checks.push_back("Oot optional");
    if(has(0,"Circular")) checks.push_back("Oot circular");
    buffer=checks.dump();return buffer.c_str();
}
static const char* mmReach(){
    static std::string buffer;
    nlohmann::json checks=nlohmann::json::array();
    if(!blockMm) checks.push_back("Mm start");
    if(!blockFixed) checks.push_back("Mm fixed");
    if(won) checks.push_back("Defeat Majora");
    if(has(1,"Gyorg route")) checks.push_back("Great Bay Temple Boss Warp");
    buffer=checks.dump();return buffer.c_str();
}
static uint8_t portal(){return 1;}
static void place(const char*,const char*){}
static bool blockCounterToken=false;
static const char* mmCounterReach(){
    static std::string buffer;
    nlohmann::json checks={"Defeat Majora","Vanilla frog 1","Vanilla frog 2","Vanilla token 1"};
    if(!blockCounterToken) checks.push_back("Vanilla token 2");
    if(std::count(owned[1].begin(),owned[1].end(),"Ocean token")==2 &&
       has(1,"Blue frog") && has(1,"White frog")) checks.push_back("Counter reward");
    buffer=checks.dump();return buffer.c_str();
}
static OracleFns oot{resetOot,grantOot,ootReach,place,portal};
static OracleFns mm{resetMm,grantMm,mmReach,place};
static std::string dump(const char* check, bool fixed=false){
    nlohmann::json j={{"checks",nlohmann::json::array({{{"name",check}}})},
      {"pool",nlohmann::json::array({{{"name","Filler"},{"advancement",false},{"category","junk"}}})}};
    if(fixed)j["fixed"]=nlohmann::json::array({{{"check","Mm fixed"},{"item","Fixed filler"},{"advancement",false}}});
    return j.dump();
}
static const char* questReach(){
    static std::string buffer; nlohmann::json checks={"Quest start","Quest early"};
    if(has(0,"Open optional")) for(const char* check:{"Quest forest","Quest lake","Quest mountain"}) checks.push_back(check);
    buffer=checks.dump(); return buffer.c_str();
}
static const char* sharedTimeReach(){
    static std::string buffer;
    nlohmann::json checks={"Oot start"}; if(has(0,"Song of Time")) checks.push_back("Ganon");
    buffer=checks.dump(); return buffer.c_str();
}
int main(){
    static_assert(SF_MASK_OF_TRUTH==16 && SF_OCARINA_BUTTON_A==17 && SF_MIRROR_SHIELD==23 && SF_OCARINA==24 && SF_SPELL_WIND==25 && SF_SPELL_LOVE==27);
    static_assert(SF_BOOTS_IRON==28 && SF_BOOTS_HOVER==29 && SF_TUNIC_GORON==30 && SF_TUNIC_ZORA==31 && SF_COUNT==32);
    assert(SharedMaskFromKeys(nlohmann::json::array({"bows","maskOfTruth"}))==((1u<<0)|(1u<<16)));
    // One table names the MM option every cross-game shared family needs, with OoTMM's setting name.
    const char* extensions[]={"RO_MM_SPELL_WIND","RO_MM_SPELL_FIRE","RO_MM_SPELL_LOVE","RO_MM_BOOTS_IRON","RO_MM_BOOTS_HOVER","RO_MM_TUNIC_GORON","RO_MM_TUNIC_ZORA"};
    const char* labels[]={"Farore's Wind (MM)","Din's Fire (MM)","Nayru's Love (MM)","Iron Boots (MM)","Hover Boots (MM)","Goron Tunic (MM)","Zora Tunic (MM)"};
    for(int family=0;family<SF_COUNT;++family) {
        if(family<SF_SPELL_WIND) { assert(!SharedFamilyMmExtension(family) && !SharedFamilyMmExtensionLabel(family)); continue; }
        assert(std::string(SharedFamilyMmExtension(family))==extensions[family-SF_SPELL_WIND]);
        assert(std::string(SharedFamilyMmExtensionLabel(family))==labels[family-SF_SPELL_WIND]);
    }
    for(int family=SF_OCARINA_BUTTON_A;family<SF_COUNT;++family) {
        if (family == SF_OCARINA) continue; // progressive tiers use their source-backed contracts
        const auto& def=SharedFamilyByIndex(family);
        std::vector<std::string> o, m{def.mmName};
        MirrorSharedSingletons(1u<<family,o,m);
        assert(o==std::vector<std::string>{def.ootName} && m==std::vector<std::string>{def.mmName});
        size_t given[SF_COUNT]={}; ApplySharedMirror(1u<<family,o,m,given); ApplySharedMirror(1u<<family,o,m,given);
        assert(o.size()==1 && m.size()==1); // singleton mirroring cannot invent a second pickup
        const auto ootStart=nlohmann::json{{"sharedStartingItems",{def.ootName}},{"sharedButtonsEnabled",true}};
        auto mmStart=nlohmann::json{{"sharedStartingItems",{def.mmName}},{"sharedButtonsEnabled",true}};
        // As in OoTMM (data.ts:3000-3054), each cross-game shared item needs its MM extension.
        const char* extension = SharedFamilyMmExtension(family);
        if (extension) {
            // Extra equipment cannot become a shared starter without its MM
            // extension. A disabled extension must remain rejected as well.
            assert(SharedStartingInventories(1u<<family,ootStart.dump(),mmStart.dump()).first.empty());
            mmStart["options"][extension] = 0;
            assert(SharedStartingInventories(1u<<family,ootStart.dump(),mmStart.dump()).first.empty());
            mmStart["options"][extension] = 1;
        }
        auto starts=SharedStartingInventories(1u<<family,ootStart.dump(),mmStart.dump());
        assert(starts.first==o && starts.second==m);
        starts=SharedStartingInventories(1u<<family,"{\"sharedButtonsEnabled\":true}",mmStart.dump());
        assert(starts.first==o && starts.second==m);
        if(def.requiresButtons) assert(SharedStartingInventories(1u<<family,"{}",mmStart.dump()).first.empty());
    }
    assert(std::string(SharedFamilyByIndex(SF_OCARINA_BUTTON_C_RIGHT).ootName)=="Ocarina C Right Button");
    assert(std::string(SharedFamilyByIndex(SF_OCARINA_BUTTON_C_LEFT).mmName)=="C Left Button");
    auto startingOot=nlohmann::json::parse(dump("Oot start"));
    startingOot["pool"]=nlohmann::json::array({{{"name","Song of Time"},{"advancement",true},{"category","major"}},{{"name","Filler"},{"advancement",false},{"category","junk"}}});
    auto startingMm=nlohmann::json::parse(dump("Mm start"));
    startingMm["sharedStartingItems"]=nlohmann::json::array({"Song of Time"});
    OracleFns sharedTimeOracle{resetOot,grantOot,sharedTimeReach,place,portal};
    for(auto startingGame:{GAME_OOT,GAME_MM}) {
        auto sharedFill=CrossWorldCombinedFill(startingOot.dump(),startingMm.dump(),31,sharedTimeOracle,mm,nullptr,"",OotAccess::ALL_REACHABLE,{},startingGame,1u<<SF_SONG_OF_TIME);
        assert(sharedFill.success);
        auto json=nlohmann::json::parse(sharedFill.spoilerJson);
        assert(json.at("sharedItems")==nlohmann::json::array({"songOfTime"}));
        for(const auto& p:ParseSpoilerPlacements(sharedFill.spoilerJson)) assert(p.item!="Song of Time");
        auto playthrough=RunPlaythrough(sharedFill.spoilerJson,sharedTimeOracle,mm,"shared-time-start",nullptr,nullptr,startingOot.dump(),startingMm.dump(),true,false,{},startingGame==GAME_MM,1u<<SF_SONG_OF_TIME);
        assert(playthrough.beatable && playthrough.beatableSphere==0);
    }
    // Native restricted song fill can confine Song of Time before the combo
    // shared-start pass. Its check must remain present with local filler.
    auto confinedStartingOot=startingOot;
    confinedStartingOot["pool"]=nlohmann::json::array({{{"name","Filler"},{"advancement",false},{"category","junk"}}});
    confinedStartingOot["checks"]=nlohmann::json::array();
    confinedStartingOot["fixed"]=nlohmann::json::array({{{"check","Oot start"},{"item","Song of Time"},{"advancement",true}}});
    for(auto startingGame:{GAME_OOT,GAME_MM}) {
        auto fill=CrossWorldCombinedFill(confinedStartingOot.dump(),startingMm.dump(),31,sharedTimeOracle,mm,nullptr,"",OotAccess::ALL_REACHABLE,{},startingGame,1u<<SF_SONG_OF_TIME);
        assert(fill.success);
        bool found=false;
        for(const auto& p:ParseSpoilerPlacements(fill.spoilerJson)) {
            assert(p.item!="Song of Time");
            if(p.checkGame==GAME_OOT && p.check=="Oot start") { assert(p.itemGame==GAME_OOT && p.item=="Filler"); found=true; }
        }
        assert(found);
    }
    auto withoutSharedStart=startingMm;
    withoutSharedStart.erase("sharedStartingItems");
    for(const auto mask:{0u,1u<<SF_SONG_OF_TIME,1u<<SF_MIRROR_SHIELD}) {
        auto fill=CrossWorldCombinedFill(confinedStartingOot.dump(),withoutSharedStart.dump(),31,oot,mm,nullptr,"",OotAccess::ALL_REACHABLE,{},GAME_MM,mask);
        assert(fill.success);
        bool retained=false;
        for(const auto& p:ParseSpoilerPlacements(fill.spoilerJson)) if(p.checkGame==GAME_OOT && p.check=="Oot start") retained=p.item=="Song of Time";
        assert(retained);
    }
    for(int family=SF_OCARINA_BUTTON_A;family<SF_COUNT;++family) {
        if (family == SF_OCARINA) continue; // tiered instruments have separate native pool tests
        const auto& def=SharedFamilyByIndex(family);
        auto o=confinedStartingOot, m=startingMm;
        // As in OoTMM (data.ts:3000-3054), each cross-game shared item needs its MM extension.
        const char* extension = SharedFamilyMmExtension(family);
        o["sharedButtonsEnabled"]=m["sharedButtonsEnabled"]=true;
        o["fixed"][0]["item"]=def.ootName;
        m["checks"]=nlohmann::json::array();
        m["pool"]=nlohmann::json::array({{{"name","Mm filler"},{"advancement",false},{"category","junk"}}});
        m["fixed"]=nlohmann::json::array({{{"check","Mm start"},{"item",def.mmName},{"advancement",true}}});
        m["sharedStartingItems"]=nlohmann::json::array({def.mmName});
        if (extension) {
            for (bool explicitDisabled : {false, true}) {
                if (explicitDisabled) m["options"][extension] = 0;
                const auto disabled = CrossWorldCombinedFill(o.dump(),m.dump(),31,oot,mm,nullptr,"",OotAccess::ALL_REACHABLE,{},GAME_MM,1u<<family);
                assert(disabled.success);
                assert(nlohmann::json::parse(disabled.spoilerJson).at("sharedItems").empty());
                bool retainedO=false, retainedM=false;
                for(const auto& p:ParseSpoilerPlacements(disabled.spoilerJson)) {
                    if(p.checkGame==GAME_OOT && p.check=="Oot start") { assert(p.itemGame==GAME_OOT && p.item==def.ootName); retainedO=true; }
                    if(p.checkGame==GAME_MM && p.check=="Mm start") { assert(p.itemGame==GAME_MM && p.item==def.mmName); retainedM=true; }
                }
                assert(retainedO && retainedM);
            }
            m["options"][extension] = 1;
        }
        auto fill=CrossWorldCombinedFill(o.dump(),m.dump(),31,oot,mm,nullptr,"",OotAccess::ALL_REACHABLE,{},GAME_MM,1u<<family);
        assert(fill.success);
        bool foundO=false,foundM=false;
        for(const auto& p:ParseSpoilerPlacements(fill.spoilerJson)) {
            if(p.checkGame==GAME_OOT && p.check=="Oot start") { assert(p.itemGame==GAME_OOT && p.item=="Filler");foundO=true; }
            if(p.checkGame==GAME_MM && p.check=="Mm start") { assert(p.itemGame==GAME_MM && p.item=="Mm filler");foundM=true; }
        }
        assert(foundO && foundM);
        m.erase("sharedStartingItems");
        fill=CrossWorldCombinedFill(o.dump(),m.dump(),31,oot,mm,nullptr,"",OotAccess::ALL_REACHABLE,{},GAME_MM,1u<<family);
        assert(fill.success);
        foundO=foundM=false;
        for(const auto& p:ParseSpoilerPlacements(fill.spoilerJson)) {
            if(p.checkGame==GAME_OOT && p.check=="Oot start") { assert(p.itemGame==GAME_OOT && p.item==def.ootName);foundO=true; }
            if(p.checkGame==GAME_MM && p.check=="Mm start") { assert(p.itemGame==GAME_MM && p.item==def.mmName);foundM=true; }
        }
        assert(foundO && foundM);
    }

    for (auto type : {CwGoal::BOTH, CwGoal::EITHER, CwGoal::GANON, CwGoal::MAJORA}) {
        CwGoal goal; goal.type = type;
        for (int bits=0; bits<4; ++bits) {
            bool ganon=bits&1, majora=bits&2;
            bool expected=type==CwGoal::BOTH ? ganon&&majora : type==CwGoal::EITHER ? ganon||majora : type==CwGoal::GANON ? ganon : majora;
            assert(CwGoalReached(goal,ganon,majora)==expected);
            assert(CwGoalType(CwGoalFromJson(CwGoalJson(goal)))==type);
        }
    }
    assert(CwGoalType(CwGoalFromJson({{"type","bosses"}}))==CwGoal::BOTH);
    assert(CwGoalType(CwGoalFromJson({{"type","both"}}))==CwGoal::BOTH);
    CwGoal quest; quest.type=CwGoal::QUEST;
    assert(!CwGoalReached(quest,true,true,0,3));
    assert(CwGoalReached(quest,false,false,0,7));
    assert(!CwGoalReached(quest,false,false,std::vector<std::string>{"Triforce of Power","Triforce of Power","Triforce of Courage"},{}));
    assert(CwGoalReached(quest,false,false,std::vector<std::string>{"Triforce of Power"},std::vector<std::string>{"Triforce of Courage","Triforce of Wisdom"}));
    auto questSpoiler=nlohmann::json{{"oot",{{"Oot start","Triforce of Power"}}},
        {"mm",{{"Mm start","Triforce of Wisdom"},{"Mm fixed","Triforce of Courage"}}},{"foreign",nlohmann::json::array()}};
    auto namedPaths=PareDownPlaythrough(questSpoiler.dump(),oot,mm,nullptr,"","",{}, {},MakeCombinedGoal(quest));
    assert(!namedPaths.requiredByCheck.empty());
    assert(std::find(namedPaths.pathsByCheck.at("oot:Oot start").begin(),namedPaths.pathsByCheck.at("oot:Oot start").end(),"Triforce of Power")!=namedPaths.pathsByCheck.at("oot:Oot start").end());

    nlohmann::json questDump={{"checks",nlohmann::json::array()}, {"pool",nlohmann::json::array()}};
    for(const char* check:{"Quest start","Quest early","Quest forest","Quest lake","Quest mountain"})
        questDump["checks"].push_back({{"name",check},{"area",check}});
    questDump["pool"].push_back({{"name","Open optional"},{"advancement",true},{"category","major"}});
    for(int i=0;i<4;++i) questDump["pool"].push_back({{"name","Filler"},{"advancement",false},{"category","junk"}});
    auto questMm=nlohmann::json::parse(dump("Mm start")); questMm["checks"][0]["area"]="Clock Town";
    OracleFns questOracle{resetOot,grantOot,questReach,place,portal};
    for(bool mmStart:{false,true}) {
        auto generatedQuest=CrossWorldCombinedFill(questDump.dump(),questMm.dump(),234,questOracle,mm,nullptr,"",OotAccess::ALL_REACHABLE,quest,mmStart ? GAME_MM : GAME_OOT);
        assert(generatedQuest.success);
        auto namedPlacements=ParseSpoilerPlacements(generatedQuest.spoilerJson,questDump.dump(),questMm.dump());
        unsigned mask=0; std::set<std::string> namedChecks;
        for(const auto& p:namedPlacements) if(unsigned bit=CwNamedTriforceMask({p.item},{})) {
            mask|=bit; namedChecks.insert(p.check);
            assert(p.check=="Quest forest" || p.check=="Quest lake" || p.check=="Quest mountain");
        }
        assert(mask==7 && namedChecks.size()==3);
    }
    const std::string nativeFailure="Native confined fill failed: required songs could not be placed";
    auto preparationFailure=CrossWorldCombinedFill(
        nlohmann::json{{"error",nativeFailure},{"retryable",true}}.dump(),dump("Mm start"),17,oot,mm);
    assert(!preparationFailure.success && preparationFailure.error==nativeFailure);
    auto run=[](OotAccess policy,bool fixed=false){return CrossWorldCombinedFill(dump("Oot start"),dump("Mm start",fixed),17,oot,mm,nullptr,"",policy);};
    assert(run(OotAccess::ALL_REACHABLE).success);
    blockMm=true;
    assert(!run(OotAccess::ALL_REACHABLE).success); // junk is a real MM check
    assert(run(OotAccess::BEATABLE_ONLY).success); // off-path checks permitted symmetrically
    blockMm=false;blockFixed=true;
    assert(!run(OotAccess::ALL_REACHABLE,true).success); // confined checks have no exception
    assert(run(OotAccess::BEATABLE_ONLY,true).success);
    blockFixed=false;won=false;
    assert(!run(OotAccess::ALL_REACHABLE).success); // all locations alone is not a win
    assert(!run(OotAccess::BEATABLE_ONLY).success);
    won=true;
    // Vanilla counter items are collected at reachable fixed checks; they must not be
    // omitted from the dump or assumed solely because the reward is in the pool.
    OracleFns mmCounters{resetMm,grantMm,mmCounterReach,place};
    nlohmann::json counterDump={{"checks",nlohmann::json::array({{{"name","Counter reward"}}})},
      {"pool",nlohmann::json::array({{{"name","Filler"},{"advancement",false},{"category","junk"}}})},
      {"fixed",nlohmann::json::array({
        {{"check","Vanilla frog 1"},{"item","Blue frog"},{"advancement",true},{"hintable",false}},
        {{"check","Vanilla frog 2"},{"item","White frog"},{"advancement",true},{"hintable",false}},
        {{"check","Vanilla token 1"},{"item","Ocean token"},{"advancement",true},{"hintable",false}},
        {{"check","Vanilla token 2"},{"item","Ocean token"},{"advancement",true},{"hintable",false}}
      })}};
    assert(CrossWorldCombinedFill(dump("Oot start"),counterDump.dump(),17,oot,mmCounters,
                                 nullptr,"",OotAccess::ALL_REACHABLE).success);
    blockCounterToken=true;
    assert(!CrossWorldCombinedFill(dump("Oot start"),counterDump.dump(),17,oot,mmCounters,
                                  nullptr,"",OotAccess::ALL_REACHABLE).success);
    blockCounterToken=false;
    nlohmann::json spoiler={{"oot",{{"Oot start","Open optional"},{"Oot optional","Filler"},{"Oot circular","Circular"}}},
        {"mm",{{"Mm start","Filler"}}}};
    auto result=RunPlaythrough(spoiler.dump(),oot,mm,"test",nullptr);
    assert(result.beatable && result.beatableSphere==0);
    assert(result.reachableOot==2 && result.unreachableOot==1); // winning still traverses optional checks; no circular assumption
    assert(result.reachableMm==1 && result.unreachableMm==0);
    // The spoiler's route stops at the winning sphere; validation still walks every sphere.
    nlohmann::json routeSpoiler={{"oot",{{"Oot start",kOotTriforcePiece},{"Oot optional","Filler"}}},
        {"mm",{{"Mm start","Open optional"}}},
        {"foreign",nlohmann::json::array({{{"checkGame","mm"},{"checkName","Mm start"},
            {"itemGame","oot"},{"itemName","Open optional"},{"advancement",true}}})}};
    nlohmann::json route=nlohmann::json::array(),walk=nlohmann::json::array();
    auto routed=RunPlaythrough(routeSpoiler.dump(),oot,mm,"route",nullptr,&route,"","",true,true,CwGoal{true,1});
    RunPlaythrough(routeSpoiler.dump(),oot,mm,"walk",nullptr,&walk,"","",true,false,CwGoal{true,1});
    assert(routed.beatableSphere==1 && route.size()==1 && walk.size()==2);
    nlohmann::json pathSpoiler={{"oot",{{"Oot start","Open optional"},{"Oot optional","Gyorg route"}}},
        {"mm",{{"Mm start","Filler"}}},
        {"foreign",nlohmann::json::array({{{"checkGame","oot"},{"checkName","Oot optional"},
            {"itemGame","mm"},{"itemName","Gyorg route"},{"advancement",true}}})}};
    std::unordered_map<std::string,std::string> areas={{"Oot start","Forest"},{"Oot optional","Village"}};
    // OoTMM gives a boss a path only when the goal fails without it, and only to required items.
    // Gyorg is optional under these goals, so nothing is labelled and no area becomes WotH.
    auto paths=PareDownPlaythrough(pathSpoiler.dump(),oot,mm,nullptr,"","",areas,{},DefaultGanonMajoraGoal,false);
    assert(!paths.requiredByCheck.at("oot:Oot start") && !paths.requiredByCheck.at("oot:Oot optional"));
    assert(paths.pathsByCheck.empty() && paths.pathsByArea.empty());
    auto huntSpoiler=pathSpoiler;
    huntSpoiler["mm"]["Mm start"]=kMmTriforcePiece;
    auto huntPaths=PareDownPlaythrough(huntSpoiler.dump(),oot,mm,nullptr,"","",areas,{},MakeCombinedGoal(CwGoal{true,1}),false);
    assert(huntPaths.requiredByCheck.at("mm:Mm start"));
    assert(!huntPaths.requiredByCheck.at("oot:Oot start"));
    assert(huntPaths.pathsByCheck.empty());
    nlohmann::json hintDump={{"checks",nlohmann::json::array({{{"name","Oot start"},{"area","Forest"}},
        {{"name","Oot optional"},{"area","Village"}}})}};
    // Keep the generated json alive: a range-for over .at() of a temporary dangles.
    const auto optionalHints=Generate(77,"{}",hintDump.dump(),"{}",nlohmann::json::array(),pathSpoiler.dump(),paths);
    for(const auto& hint:optionalHints.at("mm").at("gossipPool"))
        assert(!hint.contains("pathTarget"));
    // When the goal needs Gyorg, the items on the way are required and on the Path to Gyorg.
    auto needsGyorg=[](const auto&,const auto& mmReached,const auto&,const auto&){
        return mmReached.count("Great Bay Temple Boss Warp")!=0 && mmReached.count("Defeat Majora")!=0;};
    paths=PareDownPlaythrough(pathSpoiler.dump(),oot,mm,nullptr,"","",areas,{},needsGyorg,false);
    assert(paths.requiredByCheck.at("oot:Oot start") && paths.requiredByCheck.at("oot:Oot optional"));
    assert(paths.pathsByCheck.at("oot:Oot start")==std::vector<std::string>{"Gyorg"});
    assert(paths.pathsByCheck.at("oot:Oot optional")==std::vector<std::string>{"Gyorg"});
    assert(paths.pathsByArea.at("oot:Forest")==std::vector<std::string>{"Gyorg"});
    auto hints=Generate(77,"{}",hintDump.dump(),"{}",nlohmann::json::array(),pathSpoiler.dump(),paths);
    bool sawPath=false;
    for(const auto& hint:hints.at("mm").at("gossipPool")) {
        if(!hint.contains("pathTarget")) continue;
        sawPath=true;
        assert(hint.at("pathTarget")=="Gyorg");
        const std::string text=hint.at("text");
        assert(text.find("is on the Path to Gyorg.")!=std::string::npos);
        assert(text.find("Gyorg route")==std::string::npos && text.find("Open optional")==std::string::npos);
    }
    assert(sawPath);
    assert(hints==Generate(77,"{}",hintDump.dump(),"{}",nlohmann::json::array(),pathSpoiler.dump(),paths));
    // An always hint and an OoT location hint carry the exact check they name, so the check tracker
    // can mark it Hinted; area, path and MM-check hints name no OoT check.
    nlohmann::json stoneDump={{"options",{{"gossipStoneHints",1},{"hintDistribution",1}}},
        {"stones",{"Stone 1","Stone 2","Stone 3","Stone 4","Stone 5","Stone 6"}},
        {"alwaysHintChecks",{"Oot optional"}},
        {"checks",nlohmann::json::array({
            {{"name","Oot start"},{"area","Forest"},{"overworld",true},{"locationHint",{{"clear",{{"en","Start has [[1]]."}}}}}},
            {{"name","Oot optional"},{"area","Village"},{"locationHint",{{"clear",{{"en","Optional has [[1]]."}}}}}}})}};
    bool sawLocation=false;
    for(uint32_t seed=1;seed<64;++seed) {
        int always=0;
        const auto stoneHints=Generate(seed,"{}",stoneDump.dump(),"{}",nlohmann::json::array(),pathSpoiler.dump(),paths);
        for(const auto& hint:stoneHints.at("oot")) {
            const std::string type=hint.at("type");
            if(type=="always") {
                ++always;
                assert(hint.at("hintedCheck")=="Oot optional");
            } else if(type=="Overworld" && hint.contains("hintedCheck")) {
                sawLocation=true;
                assert(hint.at("hintedCheck")=="Oot start");
                assert(hint.at("messages").at(0).at("en").get<std::string>().rfind("Start has ",0)==0);
            } else {
                assert(!hint.contains("hintedCheck"));
            }
        }
        assert(always==1);
    }
    assert(sawLocation);
    // OoTMM path locks: Majora needs Gyorg's remains here, so an item on both paths is hinted for
    // Gyorg alone, while the remains themselves are on the Path to Majora.
    OracleFns remainsMm{resetMm,grantMm,+[]()->const char*{
        static std::string buffer; nlohmann::json checks={"Mm start"};
        if(has(1,"Gyorg route")) checks.push_back("Great Bay Temple Boss Warp");
        if(has(1,"Remains")) checks.push_back("Defeat Majora");
        buffer=checks.dump();return buffer.c_str();},place};
    auto remainsSpoiler=pathSpoiler;
    remainsSpoiler["mm"]["Great Bay Temple Boss Warp"]="Remains";
    auto locked=PareDownPlaythrough(remainsSpoiler.dump(),oot,remainsMm,nullptr,"","",areas,{},DefaultGanonMajoraGoal,false);
    assert(locked.pathsByCheck.at("oot:Oot start")==std::vector<std::string>{"Gyorg"});
    assert(locked.pathsByCheck.at("mm:Great Bay Temple Boss Warp")==std::vector<std::string>{"Majora"});
    // An MM bean delivered by an OoT check is a one-time reward: OoT shop slots sell out and shuffled
    // cows give once, so only native MM stock renews. Licenses keep the donor table's answer.
    BeanSourceMap beanDump;
    ParseBeanSources(GAME_OOT,{{"beanSources",{{"KF Shop Item 1",{{"license",false},{"renewable",true}}},
        {"KF Links House Cow",{{"license",true},{"renewable",true}}}}}},beanDump);
    auto ootShopBean=CreditPlacedItem(GAME_OOT,"KF Shop Item 1",GAME_MM,"Magic Bean",beanDump);
    auto ootCowBean=CreditPlacedItem(GAME_OOT,"KF Links House Cow",GAME_MM,"Magic Bean",beanDump);
    auto ootScrubBean=CreditPlacedItem(GAME_OOT,"LW Deku Scrub Near Bridge",GAME_MM,"Magic Bean",beanDump);
    auto mmScrubBean=CreditPlacedItem(GAME_MM,"Southern Swamp Scrub Beans",GAME_MM,"Magic Bean",beanDump);
    assert(!ootShopBean.owned && !ootShopBean.renewal && ootCowBean.owned && !ootCowBean.renewal);
    assert(!ootScrubBean.owned && !ootScrubBean.renewal && !mmScrubBean.owned && mmScrubBean.renewal);
    assert(CreditPlacedItem(GAME_OOT,"KF Shop Item 1",GAME_OOT,"Deku Shield",beanDump).owned);
    assert(!CanNavigateHidden(false,false,false));assert(!CanNavigateHidden(true,false,false));
    assert(CanNavigateHidden(true,true,false));assert(CanNavigateHidden(false,false,true));
    assert(FindMmTrick(0) && FindMmTrick(1) && !FindMmTrick(-1));
    for (const auto& trick : kMmTricks) assert(FindMmTrick(trick.id) == &trick);
    assert(!FindMmTrick(static_cast<int32_t>(std::size(kMmTricks))));
    assert(!CanUseDeityOutsideBoss(true,false));assert(!CanUseDeityOutsideBoss(false,true));assert(CanUseDeityOutsideBoss(true,true));
    assert(!CanTraverseWaistWater(false,false,false,false,CanUseDeityOutsideBoss(true,false)));
    assert(!CanTraverseWaistWater(false,false,false,false,CanUseDeityOutsideBoss(false,true)));
    assert(CanTraverseWaistWater(false,false,false,false,CanUseDeityOutsideBoss(true,true)));
    assert(!CanSpikeRoll(false,true,true));assert(!CanSpikeRoll(true,false,false));assert(CanSpikeRoll(true,false,true));
    assert(!CanUsePowderKeg(false,true,false,false));assert(!CanUsePowderKeg(false,false,true,false));
    assert(CanUsePowderKeg(false,false,true,true));assert(CanUsePowderKeg(true,true,false,false));
    assert(!CanDefeatMajora(false,false,true,false,false,false,false,false)); // bow alone cannot finish
    assert(!CanDefeatMajora(false,false,false,false,false,false,true,false)); // sword alone cannot reach first phase
    assert(CanDefeatMajora(false,false,true,false,false,false,true,false));
    assert(CanDefeatMajora(false,false,false,true,false,false,false,false));
    assert(CanDefeatMajora(false,false,false,false,true,true,false,false));
    assert(!CanDefeatMajora(false,false,false,false,true,false,false,false));
    assert(!CanDefeatMajora(true,false,true,false,false,false,true,false));
    assert(CanDefeatMajora(true,true,true,false,false,false,true,false));
    AgeTimeAccess a{false,false,true,false};assert(!ExpandSongAgeAccess(a,false,false));
    assert(ExpandSongAgeAccess(a,false,true) && a.childDay && !a.childNight);
    assert(!ExpandSongAgeAccess(a,false,true));
    AgeTimeAccess b{false,true,false,false};assert(ExpandSongAgeAccess(b,true,false) && b.adultNight && !b.adultDay);
    // OoTMM macros_mm.yml:59-62: without Cross-Games Age MM Link is always child; with it, the explored age decides.
    // Logic.h MM_LOGIC_AGE hands these the save's linkAge only while its adult state is enabled, else 1 (child).
    for(int cross=0;cross<2;++cross) for(int age=0;age<2;++age) for(int zora=0;zora<2;++zora) for(int deku=0;deku<2;++deku){
        const bool child=MmAgeLogic::IsChild(cross,age), adult=MmAgeLogic::IsAdult(cross,age);
        assert(child==(!cross || age==1) && adult==(cross && age==0) && child!=adult);
        assert(MmAgeLogic::IsTall(zora,adult)==(zora || adult) && MmAgeLogic::IsShort(deku,child)==(deku || child));
    }
    // The MM age at an MM entry (util.c:74-81): OoT's age while Cross-Games Age is active, else the stored age when
    // OoT's is unknown (-1), and always child without it.
    for(int stored=0;stored<3;++stored) for(int oot=-1;oot<3;++oot){
        assert(MmAgeLogic::EntryAge(false,oot,stored)==1);
        assert(MmAgeLogic::EntryAge(true,oot,stored)==(oot==0 || oot==1 ? oot : stored==0 ? 0 : 1));
    }
    std::cout<<"Combined reachability, collected fixed point, native ability and age-time checks passed\n";
}
