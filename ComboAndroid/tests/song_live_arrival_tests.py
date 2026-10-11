"""Compile the OoT live tracker's song arrivals and the real root arrival block.

SOH_LiveIncomingSongAges, OotSoaringSources and fill.cpp's RR_ROOT arrival
block come from source. The stubs replace region storage, the loaded saves and
the MM exports, not the age or option rules.
"""
from pathlib import Path
import argparse
import re
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


def function(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


fill = (source/'soh/soh/Enhancements/randomizer/3drando/fill.cpp').read_text(encoding='utf-8')
start = hostcxx.find(fill, '    // Live arrivals read the loaded saves and the player\'s active song options')
root_block = fill[start:fill.index('\n#endif', start)]
assert hostcxx.contains(fill, 'ComboRando::SongAgeArrivals SOH_LiveIncomingSongAges();')
globals_cpp = (source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
live_arrivals = function(globals_cpp, 'ComboRando::SongAgeArrivals SOH_LiveIncomingSongAges() {')
soaring = function(globals_cpp, 'static uint32_t OotSoaringSources(')
# The MM export the live arrivals resolve is MM's runtime (not seed) song sources.
mm_port = (source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(mm_port, 'extern "C" COMBO_EXPORT uint32_t MM_GetLiveSongSources() { return MmLiveSongSources(false); }')
oot_header = (source/'combo/rando/SongTravelOOT.h').read_text(encoding='utf-8')
oot_tables = '\n'.join(re.findall(
    r'inline constexpr RandomizerRegion kOotSong(?:Regions|Sources)\[\].*?;', oot_header, re.S))
assert oot_tables.count('inline constexpr') == 2

prelude = r'''
#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "rando/SongTravel.h"
#include "rando/ForeignSongs.h"
#include "soh/Enhancements/randomizer/randomizerEnums.h"
struct Region {
    RandomizerRegion randomizerRegionKey=RR_NONE;
    bool childDay=false,childNight=false,adultDay=false,adultNight=false,addedToPool=false;
    bool Child() const {return childDay||childNight;}
    bool Adult() const {return adultDay||adultNight;}
};
Region regions[RR_MAX];
Region* RegionTable(RandomizerRegion key) {return &regions[key];}
struct GetAccessibleLocationsStruct {bool logicUpdated=false;std::vector<RandomizerRegion> regionPool;};
namespace Rando {
struct Logic {
    std::set<RandomizerGet> items;
    bool CalculatingAvailableChecks=false;
    bool HasItem(RandomizerGet item) const {return items.count(item)!=0;}
};
}
Rando::Logic liveSave; // the loaded OoT save's items
std::shared_ptr<Rando::Logic> LiveSongLogic() {return std::make_shared<Rando::Logic>(liveSave);}
bool isRando=true,linkIsChild=true,ootForeground=true;
#define IS_RANDO isRando
#define LINK_IS_CHILD linkIsChild
static bool sMmVisitedForSongs=false;
bool Combo_OotIsForeground(void) {return ootForeground;}
// MM's loaded save: OoT songs it knows, its instrument and buttons, and its unlocked owls.
uint32_t ootKnownSongs=0,mmButtons=0,mmKnownOwls=0;
bool mmInstrument=false;
uint32_t MmLiveSources() {
    return ComboRando::ForeignWarpSources(ComboRando::ActiveSongTravel().foreignSongs,ootKnownSongs,mmInstrument,mmButtons);
}
uint32_t MmLiveKnowledge() {return mmKnownOwls;}
void* Combo_ResolveSym(const char* module,const char* name) {
    assert(std::string(module)=="2ship");
    if(std::string(name)=="MM_GetLiveSongSources") return reinterpret_cast<void*>(&MmLiveSources);
    assert(std::string(name)=="MM_GetLiveSongKnowledge");
    return reinterpret_cast<void*>(&MmLiveKnowledge);
}
'''
tests = r'''
constexpr uint32_t kRequiem=1u<<3; // Requiem of Spirit's source bit and its Desert Colossus landing
Rando::Logic search; // the region search's own logic
void Reset() {
    for(int r=0;r<RR_MAX;++r) {regions[r]=Region{};regions[r].randomizerRegionKey=static_cast<RandomizerRegion>(r);}
    // Door of Time open: both ages stand at the root, by day.
    regions[RR_ROOT].childDay=regions[RR_ROOT].adultDay=true;
    auto& map=ComboRando::ActiveSongTravel();
    map={};map.foreignSongs=map.logicForeignSongs=true;
    ComboRando::OotSongOracle()=false;ComboRando::OotSongArrivals()=0;ComboRando::OotSongAgeArrivals()={};
    liveSave={};isRando=true;linkIsChild=true;ootForeground=true;sMmVisitedForSongs=false;
    // Requiem learned, MM's instrument with A, C-down and C-right, no OoT ocarina and no owls.
    ootKnownSongs=kRequiem;mmInstrument=true;mmButtons=7;mmKnownOwls=0;
    search=Rando::Logic{};search.CalculatingAvailableChecks=true;
}
bool Arrivals(uint32_t adult,uint32_t child) {
    const auto arrivals=SOH_LiveIncomingSongAges();
    return arrivals.active&&arrivals.masks[0]==adult&&arrivals.masks[1]==child;
}
GetAccessibleLocationsStruct Search() {
    GetAccessibleLocationsStruct gals;
    ProcessRoot(&regions[RR_ROOT],gals,&search);
    return gals;
}
int main() {
    const Region& colossus=regions[RR_DESERT_COLOSSUS];
    const std::vector<RandomizerRegion> colossusOnly{RR_DESERT_COLOSSUS};
    // (a) Only the child Mask Shop portal leads into MM, so MM's Requiem lands at Colossus as
    // child alone, by the root's day, although adult also stands at the root.
    Reset();
    regions[RR_MARKET_MASK_SHOP].childDay=true;
    assert(Arrivals(0,kRequiem));
    auto gals=Search();
    assert(gals.logicUpdated&&gals.regionPool==colossusOnly);
    assert(colossus.childDay&&!colossus.childNight&&!colossus.Adult());
    // OoT's own Soaring (OoT ocarina, C-down/C-left/C-up, a known owl) takes adult into MM too...
    liveSave.items={RG_FAIRY_OCARINA,RG_OCARINA_C_DOWN_BUTTON,RG_OCARINA_C_LEFT_BUTTON,RG_OCARINA_C_UP_BUTTON};
    mmKnownOwls=1u<<6;
    assert(Arrivals(kRequiem,kRequiem));
    gals=Search();
    assert(gals.logicUpdated&&colossus.childDay&&colossus.adultDay&&!colossus.childNight&&!colossus.adultNight);
    // ...but only for an age the root has.
    regions[RR_ROOT].adultDay=false;
    assert(Arrivals(0,kRequiem));
    // An adult-only warp pad whose shuffled song leads into MM (Prelude <-> Great Bay Coast).
    Reset();
    auto& map=ComboRando::ActiveSongTravel();
    map.enabled=map.logicEnabled=true;
    std::swap(map.destinations[5],map.destinations[6]);
    regions[RR_PRELUDE_OF_LIGHT_WARP].adultDay=true;
    assert(Arrivals(kRequiem,0));
    regions[RR_ROOT].adultDay=false;
    assert(Arrivals(0,0));
    // OoT's own Soaring to a shuffled OoT landing keeps the age that plays it.
    Reset();
    map.enabled=map.logicEnabled=true;
    std::swap(map.destinations[3],map.destinations[6]);
    liveSave.items={RG_FAIRY_OCARINA,RG_OCARINA_C_DOWN_BUTTON,RG_OCARINA_C_LEFT_BUTTON,RG_OCARINA_C_UP_BUTTON};
    mmKnownOwls=1u<<6;
    regions[RR_ROOT].adultDay=false;
    assert(Arrivals(0,kRequiem));

    // (b) "Play warp songs across games" on only as a gameplay enhancement: the tracker follows it,
    // seed searches (no tracker pass, or the generation oracle) do not.
    Reset();
    map.logicForeignSongs=false;
    regions[RR_MARKET_MASK_SHOP].childDay=true;
    search.CalculatingAvailableChecks=false;
    gals=Search();
    assert(!gals.logicUpdated&&gals.regionPool.empty()&&!colossus.Child()&&!colossus.Adult());
    search.CalculatingAvailableChecks=true;
    ComboRando::OotSongOracle()=true;
    gals=Search();
    assert(!gals.logicUpdated&&!colossus.Child()&&!colossus.Adult());
    ComboRando::OotSongOracle()=false;
    gals=Search();
    assert(gals.logicUpdated&&gals.regionPool==colossusOnly&&colossus.childDay&&!colossus.Adult());
    // On in the seed's logic but off in play: the tracker plans with the active option.
    Reset();
    map.foreignSongs=false;
    regions[RR_MARKET_MASK_SHOP].childDay=true;
    assert(Arrivals(0,0));
    gals=Search();
    assert(!gals.logicUpdated&&!colossus.Child()&&!colossus.Adult());
    // A seed search reads the oracle's per-age arrivals instead.
    Reset();
    search.CalculatingAvailableChecks=false;
    ComboRando::OotSongOracle()=true;
    ComboRando::OotSongAgeArrivals()={true,{0,kRequiem}};
    gals=Search();
    assert(gals.logicUpdated&&colossus.childDay&&!colossus.Adult());

    // (c) A past MM visit counts only while MM is in the foreground, with the dormant OoT save's age.
    for(bool child:{true,false}) {
        Reset();
        sMmVisitedForSongs=true;
        linkIsChild=child;
        ootForeground=false;
        assert(Arrivals(child?0:kRequiem,child?kRequiem:0));
        gals=Search();
        assert(gals.logicUpdated&&colossus.Child()==child&&colossus.Adult()==!child);
        Reset();
        sMmVisitedForSongs=true;
        linkIsChild=child;
        assert(Arrivals(0,0));
        gals=Search();
        assert(!gals.logicUpdated&&!colossus.Child()&&!colossus.Adult());
    }
    Reset();
    isRando=false;
    regions[RR_MARKET_MASK_SHOP].childDay=true;
    assert(Arrivals(0,0));
    std::cout<<"PASS: MM songs land in OoT only with the ages that can be in MM (portal, per-age OoT departures, "
               "foreground visit with the dormant age), OoT Soaring keeps its age, the enhancement-only option "
               "reaches the tracker but not seed searches\n";
}
'''
generated = prelude + 'namespace ComboRando {\n' + oot_tables + '\n}\n' + soaring + live_arrivals
generated += 'void ProcessRoot(Region* region,GetAccessibleLocationsStruct& gals,Rando::Logic* logic) {\n'
generated += root_block + '\n}\n' + tests
test = work/'song_live_arrivals.cpp'
test.write_text(generated, encoding='utf-8')
exe = hostcxx.build(args.compiler, test, werror=True, msvc=['/Zc:preprocessor'],
                    includes=[source/'combo', source/'soh', args.json_include.resolve()])
run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
(work/'result.txt').write_text(run.stdout + run.stderr, encoding='utf-8')
print(run.stdout + run.stderr, end='')
run.check_returncode()
