#include "rando/NativeConfinedSongPreparation.h"
#include "rando/SongTravel.h"
#include "rando/ForeignSongs.h"
#include "rando/CrossWorldRando.h"
#include "rando/ComboPlaythrough.h"
#include <cassert>
#include <iostream>
using namespace ComboRando;
static uint32_t arrivals[2], sources[2];
static SongTravelMap map;
static void ootArr(uint32_t m){arrivals[0]=m;}
static void mmArr(uint32_t m){arrivals[1]=m;}
static uint32_t ootSources(){return sources[0];}
static uint32_t mmSources(){return sources[1];}
static uint8_t portal(){return 0;}
static int destination(int s){return map.logicEnabled || map.logicForeignSongs ? map.logicDestination(s) : -1;}
static uint32_t knowledge[2],foreign[2];
static void resetOot(){knowledge[0]=0;}
static void resetMm(){knowledge[1]=0;}
static void ownedOot(const char* json){for(auto& item:nlohmann::json::parse(json)) if(item=="Requiem") knowledge[0]|=1u<<3;}
static bool mmInstrument=false;
static void ownedMm(const char* text){
    mmInstrument=false;
    bool soaring=false; uint32_t owls=0;
    for(auto& item:nlohmann::json::parse(text)) {
        if(item=="Ocarina") mmInstrument=true;
        if(item=="Soaring") soaring=true;
        if(item=="Clock Town owl") owls|=1u<<10;
        if(item=="Great Bay owl") owls|=1u<<6;
    }
    knowledge[1]=soaring ? owls : 0;
}
static uint32_t knownOot(){return knowledge[0];}
static uint32_t knownMm(){return knowledge[1];}
static void foreignOot(uint32_t k){foreign[0]=k;}
static void foreignMm(uint32_t k){foreign[1]=k;}
static const char* reachableOot(){
    sources[0]=0;
    return arrivals[0]&(1u<<3) ? "[\"Song chest\",\"Junk chest\",\"Spirit child checks\"]" : "[\"Song chest\",\"Junk chest\"]";
}
static const char* reachableMm(){
    sources[1]=ForeignWarpSources(map.logicForeignSongs,foreign[1],mmInstrument,31);
    return "[\"Instrument chest\"]";
}
int main() {
    for(uint64_t seed=0;seed<1000;seed++) {
        auto m=SongTravelMap::generate(seed,true);
        assert(m.json()==SongTravelMap::generate(seed,true).json());
        assert(SongTravelMap::parse(m.json()).json()==m.json());
        assert(m.resolve(0xffff)==0xffff);
    }
    assert(!SongTravelMap::parse(nullptr).enabled);
    for (auto bad: {nlohmann::json::array(),nlohmann::json{{"version",2}},nlohmann::json{{"version",1},{"enabled",true},{"destinations",{0}}}}) {
        bool threw=false; try{SongTravelMap::parse(bad);}catch(...){threw=true;} assert(threw);
    }
    for(auto value: {nlohmann::json(-1),nlohmann::json(UINT64_MAX),nlohmann::json(16),nlohmann::json(1.5)}) {
        auto bad=SongTravelMap{}.json();bad["destinations"][0]=value;
        bool threw=false;try{SongTravelMap::parse(bad);}catch(...){threw=true;}assert(threw);
    }
    auto duplicate=SongTravelMap{}.json();duplicate["destinations"][0]=1;
    bool threw=false;try{SongTravelMap::parse(duplicate);}catch(...){threw=true;}assert(threw);
    OracleFns oot{},mm{};
    oot.SetSongArrivals=ootArr;oot.GetSongSources=ootSources;oot.SongDestination=destination;oot.GetPortalOpen=portal;
    mm.SetSongArrivals=mmArr;mm.GetSongSources=mmSources;mm.SongDestination=destination;
    // Separate optional destination shuffle. This is NOT the user's foreign Requiem example.
    map.enabled=map.logicEnabled=true;std::swap(map.destinations[0],map.destinations[10]);std::swap(map.destinations[10],map.destinations[3]);
    auto query=[&](const OracleFns& o,const std::vector<std::string>& items){
        bool isOot=&o==&oot;int game=isOot?0:1;std::unordered_set<std::string> result;
        auto has=[&](const char* item){return std::find(items.begin(),items.end(),item)!=items.end();};
        sources[game]=0;
        if(isOot) {
            result.insert("House"); if(has("Minuet")&&has("Ocarina")) sources[0]=1;
            if(arrivals[0]&(1u<<3)) result.insert("Spirit child checks");
        } else {
            if(has("Soaring")&&has("Ocarina")&&has("Clock Town owl")) sources[1]=1u<<10;
            if(arrivals[1]&(1u<<10)) result.insert("Town");
        }
        return result;
    };
    bool open=false;
    auto reached=QuerySongWorlds(oot,mm,{"Minuet","Ocarina"},{"Soaring","Ocarina","Clock Town owl"},open,query);
    assert(open && reached.first.count("Spirit child checks") && reached.second.count("Town"));
    // An assumed-fill probe with less inventory MUST lose the prior incoming route.
    open=false;reached=QuerySongWorlds(oot,mm,{}, {"Soaring","Ocarina","Clock Town owl"},open,query);
    assert(!open && !reached.first.count("Spirit child checks") && reached.second.empty());
    for(auto mmItems: {std::vector<std::string>{"Soaring","Clock Town owl"},std::vector<std::string>{"Soaring","Ocarina"},std::vector<std::string>{"Ocarina","Clock Town owl"}}) {
        open=false;reached=QuerySongWorlds(oot,mm,{"Minuet","Ocarina"},mmItems,open,query);
        assert(open && !reached.first.count("Spirit child checks"));
    }
    open=true;reached=QuerySongWorlds(oot,mm,{}, {"Soaring","Ocarina","Clock Town owl"},open,query);
    assert(reached.first.count("Spirit child checks")); // MM start can arrive without owning an OoT warp song
    map.enabled=map.logicEnabled=false;open=false;
    reached=QuerySongWorlds(oot,mm,{"Minuet","Ocarina"},{"Soaring","Ocarina","Clock Town owl"},open,query);
    assert(!open && reached.second.empty());

    // User's precise case: shared Requiem knowledge, only MM has an instrument, vanilla destinations.
    map=SongTravelMap::generate(42,false,true);
    oot.Reset=resetOot;oot.SetOwnedItems=ownedOot;oot.GetSongKnowledge=knownOot;oot.SetForeignSongKnowledge=foreignOot;
    mm.Reset=resetMm;mm.SetOwnedItems=ownedMm;mm.GetSongKnowledge=knownMm;mm.SetForeignSongKnowledge=foreignMm;
    auto foreignQuery=[&](const OracleFns& o,const std::vector<std::string>& items){
        bool isOot=&o==&oot;auto has=[&](const char* s){return std::find(items.begin(),items.end(),s)!=items.end();};
        std::unordered_set<std::string> result;
        if(isOot) { sources[0]=0; if(arrivals[0]&(1u<<3)) result.insert("Spirit child checks"); }
        else sources[1]=ForeignWarpSources(map.logicForeignSongs,foreign[1],has("Ocarina"),has("Missing A") ? 30 : 31);
        return result;
    };
    open=true;reached=QuerySongWorlds(oot,mm,{"Requiem"},{"Ocarina"},open,foreignQuery);
    assert(reached.first.count("Spirit child checks"));
    for(auto inv: {std::vector<std::string>{},std::vector<std::string>{"Ocarina","Missing A"}}) {
        open=true;reached=QuerySongWorlds(oot,mm,{"Requiem"},inv,open,foreignQuery);
        assert(!reached.first.count("Spirit child checks"));
    }
    open=true;reached=QuerySongWorlds(oot,mm,{}, {"Ocarina"},open,foreignQuery);
    assert(!reached.first.count("Spirit child checks")); // no learned Requiem, no warp
    open=false;reached=QuerySongWorlds(oot,mm,{"Requiem"},{"Ocarina"},open,foreignQuery);
    assert(!reached.first.count("Spirit child checks")); // MM itself must be accessible
    // The runtime convenience remains usable while generation/tracker cannot require it.
    map.logicForeignSongs=false;
    assert(ForeignWarpSources(map.foreignSongs,1u<<3,true,31)==(1u<<3));
    open=true;reached=QuerySongWorlds(oot,mm,{"Requiem"},{"Ocarina"},open,foreignQuery);
    assert(!reached.first.count("Spirit child checks"));
    auto saved=map.json(); assert(SongTravelMap::parse(saved).json()==saved);
    map.logicForeignSongs=true;
    // Destination mappings stay immutable when their runtime/logic shuffle flags differ.
    auto mixed=SongTravelMap::generate(99,true,true);
    mixed.logicEnabled=false;
    for(int source=0;source<16;source++) assert(mixed.logicDestination(source)==source);
    auto original=mixed.destinations; mixed.enabled=false;
    for(int source=0;source<16;source++) assert(mixed.destination(source)==source);
    assert(mixed.destinations==original);
    mixed.enabled=true; assert(mixed.destinations==original);
    for(int song=0;song<7;song++) {
        uint8_t pitches[8]{};
        for(int n=0;n<kForeignLengths[song];n++) pitches[n]=kSongPitches[kForeignNotes[song][n]];
        assert(RecognizeForeignSong(pitches,kForeignLengths[song],1u<<song)==song);
        assert(RecognizeForeignSong(pitches,kForeignLengths[song],0)==-1);
        pitches[kForeignLengths[song]-1]=255;
        assert(RecognizeForeignSong(pitches,kForeignLengths[song],1u<<song)==-1);
    }
    assert(ForeignSoaringSources(true,1u<<10,true,31,true)==(1u<<10));
    assert(ForeignSoaringSources(true,1u<<10,true,31,false)==(1u<<10));
    assert(!ForeignSoaringSources(true,1u<<10,false,31,true));
    assert(!ForeignSoaringSources(true,1u<<10,true,15,true));
    assert(!ForeignSoaringSources(true,0,true,31,true));
    // Ageless changes only the OoT age prerequisite. Exhaust every playable
    // note mask and native owl-activation word, retaining all other gates.
    unsigned soaringCases=0;
    for(bool enabled:{false,true}) for(bool instrument:{false,true})
    for(bool child:{false,true}) for(bool ageless:{false,true})
    for(uint32_t buttons=0;buttons<32;buttons++) for(uint32_t owls=0;owls<1024;owls++) {
        const uint32_t known=owls<<6;
        const auto actual=ForeignSoaringSources(enabled,known,instrument,buttons,child,ageless);
        const auto expected=enabled && instrument && (buttons & 26u)==26u ? known : 0;
        assert(actual==expected);
        ++soaringCases;
    }
    assert(!ForeignSoaringSources(false,1u<<10,true,31,false,true));
    assert(!ForeignSoaringSources(true,1u<<10,false,31,false,true));
    assert(!ForeignSoaringSources(true,1u<<10,true,15,false,true));
    assert(!ForeignSoaringSources(true,0,true,31,false,true));
    assert(ForeignSoaringSources(true,1u<<10,true,26,false,true)==(1u<<10));

    // Adult OoT has an instrument and only the D/L/U notes. MM has learned
    // Soaring + an activated owl but no MM instrument. Cross-game songs gives
    // both ages the same routes using OoT's instrument, regardless of the legacy age flag.
    map=SongTravelMap::generate(42,false,true);
    bool agelessRuntime=true,agelessLogic=false;
    assert(ForeignSoaringSources(map.foreignSongs,1u<<10,true,26,false,agelessRuntime)==(1u<<10));
    auto adultQuery=[&](const OracleFns& o,const std::vector<std::string>& items){
        const bool isOot=&o==&oot;
        auto has=[&](const char* s){return std::find(items.begin(),items.end(),s)!=items.end();};
        if(isOot) {
            sources[0]=ForeignSoaringSources(map.logicForeignSongs,foreign[0],has("Ocarina"),
                has("Missing Up") ? 10u : 26u,false,agelessLogic);
            return std::unordered_set<std::string>{"OoT start"};
        }
        sources[1]=0; // no MM instrument or native Soaring escape
        std::unordered_set<std::string> result;
        if(arrivals[1]&(1u<<10)) result.insert("Clock Town arrival check");
        if(arrivals[1]&(1u<<6)) result.insert("Great Bay arrival check");
        return result;
    };
    open=false;reached=QuerySongWorlds(oot,mm,{"Ocarina"},{"Soaring","Clock Town owl"},open,adultQuery);
    assert(open && reached.second.count("Clock Town arrival check"));
    agelessLogic=true;
    open=false;reached=QuerySongWorlds(oot,mm,{"Ocarina"},{"Soaring","Clock Town owl"},open,adultQuery);
    assert(open && reached.second.count("Clock Town arrival check"));
    for(auto inv:{std::vector<std::string>{},std::vector<std::string>{"Ocarina","Missing Up"}}) {
        open=false;reached=QuerySongWorlds(oot,mm,inv,{"Soaring","Clock Town owl"},open,adultQuery);
        assert(!open && reached.second.empty());
    }
    for(auto inv:{std::vector<std::string>{"Clock Town owl"},std::vector<std::string>{"Soaring"}}) {
        open=false;reached=QuerySongWorlds(oot,mm,{"Ocarina"},inv,open,adultQuery);
        assert(!open && reached.second.empty());
    }
    map.logicForeignSongs=false;
    open=false;reached=QuerySongWorlds(oot,mm,{"Ocarina"},{"Soaring","Clock Town owl"},open,adultQuery);
    assert(!open && reached.second.empty()); // ageless alone cannot grant foreign playback
    map.logicForeignSongs=true;
    map.enabled=map.logicEnabled=true;
    std::swap(map.destinations[10],map.destinations[6]);
    open=false;reached=QuerySongWorlds(oot,mm,{"Ocarina"},{"Soaring","Clock Town owl"},open,adultQuery);
    assert(open && reached.second.count("Great Bay arrival check")); // source owl, not destination owl
    auto recorded=map.destinations;
    auto recordedJson=map.json();
    map.enabled=map.logicEnabled=false;
    assert(map.destinations==recorded && map.destination(10)==10 && map.logicDestination(10)==10);
    map=SongTravelMap::parse(recordedJson);
    assert(map.destinations==recorded && map.destination(10)==6 && map.logicDestination(10)==6);
    map=SongTravelMap::generate(42,false,true); // restore the Requiem playthrough fixture
    assert(!ForeignWarpSources(false,63,true,31));
    oot.GetReachableChecks=reachableOot;mm.GetReachableChecks=reachableMm;
    const std::string spoiler=R"({"oot":{"Song chest":"Requiem","Junk chest":"Green Rupee"},"mm":{"Instrument chest":"Ocarina"}})";
    auto hints=PareDownPlaythrough(spoiler,oot,mm,nullptr,"","",{},{},
        [](const auto& a,const auto&,const auto&,const auto&){return a.count("Spirit child checks")!=0;},true,true);
    assert(hints.requiredByCheck.at("oot:Song chest"));
    assert(hints.requiredByCheck.at("mm:Instrument chest"));
    assert(!hints.requiredByCheck.at("oot:Junk chest"));
    // Cross-game warps cannot stand in for the absent second oracle during native confinement.
    size_t count=0;

 for(auto seed:{313219001u,2967654770u})for(bool enabled:{false,true})for(bool foreign:{false,true}){
  auto original=SongTravelMap::generateForRules(seed,enabled,foreign,enabled,foreign);ActiveSongTravel()=original;
  if(enabled){assert(original.logicDestination(4)==(seed==313219001u?6:13));++count;}
  try{NativeConfinedSongPreparation scope;
   assert(!ActiveSongTravel().logicEnabled&&!ActiveSongTravel().logicForeignSongs);
   assert(ActiveSongTravel().enabled==original.enabled&&ActiveSongTravel().foreignSongs==original.foreignSongs);count+=4;
   for(int source=0;source<16;++source){assert(ActiveSongTravel().logicDestination(source)==source);assert(ActiveSongTravel().destination(source)==original.destination(source));assert(ActiveSongTravel().destinations[source]==original.destinations[source]);count+=3;}
   {NativeConfinedSongPreparation nested;assert(!ActiveSongTravel().logicEnabled);++count;}
   assert(!ActiveSongTravel().logicEnabled);++count;throw 1;
  }catch(int){}
  assert(ActiveSongTravel().json()==original.json());++count;
 }
    assert(count==444);
    std::cout<<"Song routes, foreign Requiem ownership, local instrument/buttons, "<<soaringCases
             <<" Soaring prerequisite cases, adult enhancement-only/logic separation, source owls, saved maps, note recognition and probe isolation passed\n";
}
