"""Exercise the production region allowlist and real search age-propagation block."""
from pathlib import Path
import argparse
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
fill = (source/'soh/soh/Enhancements/randomizer/3drando/fill.cpp').read_text(encoding='utf-8')
block = '    const bool ageSafe = SongAgeChangeAreaAllowed('+fill.split(
    '    const bool ageSafe = SongAgeChangeAreaAllowed(', 1)[1].split('\n#endif', 1)[0]
native_logic = (source/'soh/soh/Enhancements/randomizer/logic.cpp').read_text(encoding='utf-8')
age_method = 'bool TestLogic::CanUseSongAgeChange() {' + native_logic.split(
    'bool Logic::CanUseSongAgeChange() {', 1)[1].split('\n}', 1)[0] + '\n}\n'
switch_age = (source/'soh/soh/Enhancements/SwitchAge.cpp').read_text(encoding='utf-8')
assert 'roomIndex = gPlayState->roomCtx.curRoom.num' in switch_age
assert 'pos = player->actor.world.pos' in switch_age
assert 'playerParams = 0x0D00 | camId' in switch_age
prelude = r'''
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "soh/Enhancements/randomizer/randomizerEnums.h"
#include "soh/Enhancements/randomizer/age_change_areas.h"
#include "rando/GameplayRules.h"
#include "rando/AgeTravel.h"
using namespace Rando;
int ageMode=0, runtimeAgeMode=0;
int CVarGetInteger(const char* key,int fallback) {
    assert(fallback==0);
    if(std::string(key)=="gEnhancements.TimeTravel") return runtimeAgeMode;
    assert(std::string(key)==ComboRando::LogicRuleKey("gEnhancements.TimeTravel"));
    return ageMode;
}
struct Region {
    RandomizerRegion randomizerRegionKey=RR_NONE;
    bool childDay=false,childNight=false,adultDay=false,adultNight=false;
    bool Child() const {return childDay||childNight;}
    bool Adult() const {return adultDay||adultNight;}
};
Region rootRegion;
Region* RegionTable(RandomizerRegion key) {assert(key==RR_ROOT);return &rootRegion;}
struct TestLogic {
    bool IsChild=true,IsAdult=false,song=true,instrument=true,buttons=true,oot=true,sword=true;
    bool CalculatingAvailableChecks=false;
    int GameplayRuleValue(const char*) {return CalculatingAvailableChecks ? runtimeAgeMode : ageMode;}
    bool CanUseSongAgeChange();
    bool CanUse(RandomizerGet item) {assert(item==RG_SONG_OF_TIME);return song&&instrument&&buttons;}
    bool HasItem(RandomizerGet item) {assert(item==RG_OCARINA_OF_TIME||item==RG_MASTER_SWORD);return item==RG_OCARINA_OF_TIME?oot:sword;}
};
struct Search {bool logicUpdated=false,resetSphere=false;};
'''
prelude += '#define COMBO_BUILD\n' + age_method
tests = r'''
int main() {
    const RandomizerRegion interiors[]={RR_KF_LINKS_HOUSE,RR_KF_MIDOS_HOUSE,RR_KF_SARIAS_HOUSE,
        RR_KF_HOUSE_OF_TWINS,RR_KF_KNOW_IT_ALL_HOUSE,RR_KF_KOKIRI_SHOP,
        RR_KAK_CARPENTER_BOSS_HOUSE,RR_KAK_HOUSE_OF_SKULLTULA,RR_KAK_IMPAS_HOUSE,
        RR_KAK_IMPAS_HOUSE_BACK,RR_KAK_BAZAAR,RR_KAK_POTION_SHOP,RR_MARKET_BAZAAR,
        RR_LH_LAB,RR_GV_CARPENTER_TENT,RR_GC_SHOP,RR_ZD_SHOP,
        RR_LLR_TALONS_HOUSE,RR_LLR_STABLES,RR_LLR_TOWER};
    for(auto area:interiors) assert(SongAgeChangeAreaAllowed(area));
    assert(SongAgeChangeAreaAllowed(RR_LON_LON_RANCH));
    const RandomizerRegion waterEntrances[]={RR_WATER_TEMPLE_ENTRYWAY,
        RR_WATER_TEMPLE_ENTRANCE_LEDGE,RR_WATER_TEMPLE_MQ_ENTRANCE_LEDGE};
    for(auto area:waterEntrances) assert(SongAgeChangeAreaAllowed(area));
    const RandomizerRegion targetRooms[]={RR_FOREST_TEMPLE_LOBBY,RR_FOREST_TEMPLE_NE_COURTYARD_LOWER,
        RR_FOREST_TEMPLE_NE_COURTYARD_UPPER,RR_FOREST_TEMPLE_NE_COURTYARD_ISLAND,
        RR_FOREST_TEMPLE_NW_COURTYARD_LOWER,RR_FOREST_TEMPLE_NW_COURTYARD_UPPER,
        RR_FOREST_TEMPLE_MIDDLE_BLOCK_PUSH_ROOM,RR_FOREST_TEMPLE_UPPER_BLOCK_PUSH_ROOM,
        RR_FOREST_TEMPLE_MQ_LOBBY,RR_FOREST_TEMPLE_MQ_MIDDLE_BLOCK_PUZZLE,
        RR_FOREST_TEMPLE_MQ_UPPER_BLOCK_PUZZLE,RR_GERUDO_TRAINING_GROUND_LOBBY,
        RR_GERUDO_TRAINING_GROUND_MQ_LOBBY};
    for(auto area:targetRooms) assert(SongAgeChangeAreaAllowed(area));
    for(auto area:{RR_DEKU_TREE_LOBBY,RR_DEKU_TREE_MQ_OUTSIDE_BOSS_ROOM,
        RR_JABU_JABUS_BELLY_BEGINNING,RR_JABU_JABUS_BELLY_MQ_NEAR_BOSS_ROOM,
        RR_FIRE_TEMPLE_FOYER,RR_FIRE_TEMPLE_MQ_ABOVE_FIRE_MAZE,
        RR_SHADOW_TEMPLE_BEGINNING,RR_SHADOW_TEMPLE_MQ_SPIKE_WALLS_ROOM,
        RR_BOTW_CORRIDOR,RR_BOTW_MQ_B3_PLATFORM,RR_ICE_CAVERN_BEGINNING,
        RR_WATER_TEMPLE_RIVER,RR_WATER_TEMPLE_RIVER_POTS,RR_WATER_TEMPLE_ABOVE_DRAGON,
        RR_GC_DARUNIAS_CHAMBER,RR_DMC_PAD_ENTRY,RR_DMC_CENTRAL,RR_DMC_POTS,
        RR_WASTELAND_NEAR_FORTRESS,RR_HAUNTED_WASTELAND,RR_WASTELAND_NEAR_COLOSSUS})
        assert(SongAgeChangeAreaAllowed(area));
    for(auto area:{RR_LH_LAB_UNDERWATER,RR_LH_FISHING_POND,RR_KAK_ODD_POTION_BUILDING,
        RR_GRAVEYARD_DAMPES_HOUSE,RR_DEKU_TREE_BOSS_ENTRYWAY,RR_FOREST_TEMPLE_ENTRYWAY,
        RR_WATER_TEMPLE_MAIN,RR_WATER_TEMPLE_MQ_MAIN,RR_WATER_TEMPLE_DARK_LINK_ROOM,
        RR_FOREST_TEMPLE_SEWER,RR_FOREST_TEMPLE_BOSS_ENTRYWAY,
        RR_ICE_CAVERN_FINAL_ROOM_UNDERWATER,RR_ICE_CAVERN_MQ_STALFOS_ROOM_UNDERWATER,
        RR_SPIRIT_TEMPLE_BOSS_ENTRYWAY,RR_SPIRIT_TEMPLE_ENTRYWAY,
        RR_FOREST_TEMPLE_FROZEN_EYE_ROOM,
        RR_GERUDO_TRAINING_GROUND_LAVA_ROOM,RR_GERUDO_TRAINING_GROUND_LAVA_ROOM_UPPER_LEDGE,
        RR_GERUDO_TRAINING_GROUND_UNDERWATER,RR_GERUDO_TRAINING_GROUND_MQ_UNDERWATER,
        RR_MARKET_BOMBCHU_BOWLING,RR_MARKET_SHOOTING_GALLERY,RR_LLR_GROTTO,RR_NONE,RR_ROOT,RR_MAX})
        assert(!SongAgeChangeAreaAllowed(area));
    int allowed=0;
    // Compile every native region enum through the actual search block; access
    // must propagate only at a permitted area and preserve day/night and context.
    for(int r=RR_NONE;r<RR_MAX;++r) {
        auto area=static_cast<RandomizerRegion>(r);
        const bool permitted=SongAgeChangeAreaAllowed(area);
        allowed+=permitted;
        for(bool tracker:{false,true}) for(int mode=0;mode<=4;++mode) for(int origin=0;origin<4;++origin) {
            Region region;region.randomizerRegionKey=area;
            region.childDay=origin==0;region.childNight=origin==1;
            region.adultDay=origin==2;region.adultNight=origin==3;
            rootRegion=Region{};TestLogic logic;Search search;
            logic.CalculatingAvailableChecks=tracker;
            ageMode=tracker?4-mode:mode;runtimeAgeMode=tracker?mode:4-mode;
            Propagate(&region,&logic,search,true);
            const bool changed=permitted&&mode!=0;
            assert(search.logicUpdated==changed&&search.resetSphere==changed);
            assert(logic.IsChild&&!logic.IsAdult);
            assert(region.childDay==(origin==0||(changed&&origin==2)));
            assert(region.childNight==(origin==1||(changed&&origin==3)));
            assert(region.adultDay==(origin==2||(changed&&origin==0)));
            assert(region.adultNight==(origin==3||(changed&&origin==1)));
            assert(rootRegion.Child()==changed&&rootRegion.Adult()==changed);
            Search again;Propagate(&region,&logic,again,true);
            assert(!again.logicUpdated&&!again.resetSphere); // fixed point terminates
        }
    }
    // Every logical equipment mode, both ages and each separately missing
    // song/instrument/button/OoT/Master Sword requirement.
    std::vector<RandomizerRegion> equipmentAreas;
    for(int r=RR_NONE;r<RR_MAX;++r) {
        auto area=static_cast<RandomizerRegion>(r);
        if(SongAgeChangeAreaAllowed(area)) equipmentAreas.push_back(area);
    }
    for(auto area:equipmentAreas) for(bool tracker:{false,true}) for(int mode=0;mode<=4;++mode)
      for(int origin=0;origin<2;++origin) for(int missing=0;missing<6;++missing) {
        Region region;region.randomizerRegionKey=area;
        region.childDay=origin==0;region.adultNight=origin==1;
        TestLogic logic;rootRegion=Region{};Search search;
        logic.CalculatingAvailableChecks=tracker;
        ageMode=tracker?4-mode:mode;runtimeAgeMode=tracker?mode:4-mode;
        logic.song=missing!=1;logic.instrument=missing!=2;logic.buttons=missing!=3;
        logic.oot=missing!=4;logic.sword=missing!=5;
        Propagate(&region,&logic,search,false);
        bool equip=mode==3 || (mode==1&&logic.oot) || (mode==2&&logic.oot&&logic.sword)
                   || (mode==4&&logic.sword);
        const bool changed=logic.song&&logic.instrument&&logic.buttons&&equip;
        assert(search.logicUpdated==changed&&!search.resetSphere);
        assert(rootRegion.Child()==changed&&rootRegion.Adult()==changed);
        assert(logic.IsChild&&!logic.IsAdult);
      }
    std::cout<<"PASS: dry Spirit/Forest/Training Ground rooms, ordinary interiors and Water Temple ledge, all "<<RR_MAX
             <<" native areas ("<<allowed<<" permitted), independent live and seed rules, five modes, both ages/day/night, missing equipment, fixed point and savewarp roots\n";
}
'''
test = work/'song_age_areas.cpp'
test.write_text(prelude + 'void Propagate(Region* region,TestLogic* logic,Search& gals,bool addToPlaythrough) {\n' +
                block + '\n}\n' + tests, encoding='utf-8')
exe = hostcxx.build(args.compiler, test, werror=True, msvc=['/Zc:preprocessor'],
                    includes=[source/'combo', source/'soh', args.json_include])
run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
(work/'result.txt').write_text(run.stdout + run.stderr, encoding='utf-8')
print(run.stdout + run.stderr, end='')
run.check_returncode()
