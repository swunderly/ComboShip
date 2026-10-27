"""Compile actual native song search/export blocks with independent map flags.

The small engine stubs replace region storage, not route selection. Endpoint
tables, entrance encoding, search blocks and oracle exports come from source.
With Warp Songs and Soaring Spots Shuffle placed by the entrance shuffler (limits
design, Stage 4) each source may land at several destinations while a candidate
is checked: the same blocks then route to every landing of the candidate's mask
in each game, and the oracle export answers with the mask.
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


def routing_block(text, prefix):
    start = hostcxx.find(text, prefix)
    return text[start:text.index('\n#endif', start)]


oot = (source/'soh/soh/Enhancements/randomizer/3drando/fill.cpp').read_text(encoding='utf-8')
mm = (source/'mm/2s2h/Rando/Logic/Logic.cpp').read_text(encoding='utf-8')
oot_block = routing_block(oot, '        std::array<Region*, 6> songTargets{ exitRegion };')
assert hostcxx.contains(oot_block, 'if (ComboRando::ActiveSongTravel().logicEnabled) {')
mm_start = hostcxx.find(mm, 'std::array<std::pair<s32, RandoRegionId>, 10> songTargets{};')
mm_end = hostcxx.find(mm, 'arrivalTimes = sourceArrivalTimes;', mm_start)
mm_block = mm[mm_start:mm.index(';', mm_end) + 1] + '\n'
assert hostcxx.contains(mm_block, 'if (currentRegion==RR_MAX && ComboRando::ActiveSongTravel().logicEnabled) {')
globals_cpp = (source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
mm_port = (source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
logical_oracle = function(globals_cpp, 'extern "C" COMBO_EXPORT int SOH_SongLogicDestination(')
logical_landings = function(globals_cpp, 'extern "C" COMBO_EXPORT uint32_t SOH_SongLogicDestinations(')
mm_runtime = function(mm_port, 'extern "C" COMBO_EXPORT int MM_ForeignSongDestination(')

# Bind the exercised export to the real launcher resolver.
launcher = (source/'combo/ComboShip.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(launcher, 'SOH_SongDestination=(int(*)(int))GetSym(sohModule,"SOH_SongLogicDestination");')
assert hostcxx.contains(launcher, 'sAdultOotAgeOracle.SongDestinations=(uint32_t(*)(int))GetSym(sohModule,'
                                  '"SOH_SongLogicDestinations");')
assert hostcxx.contains(launcher, 'oot.SongDestinations=sAdultOotAgeOracle.SongDestinations;')
assert hostcxx.contains(launcher, 'mm.SongDestinations=sAdultOotAgeOracle.SongDestinations;')
headless = (source/'combo/ComboRandoHeadless.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(headless, 'oot.SongDestinations=mmO.SongDestinations=Sym<uint32_t(*)(int)>(soh,'
                                  '"SOH_SongLogicDestinations");')
# A layout that shuffles the warps draws the song map: the app and the headless generator install its WARPS routes
# right after the layout, and keep the attempt's own map for every other layout and for no layout.
launcher_apply = launcher[hostcxx.find(launcher, 'ComboRando::Entrances::ApplyUnifiedLayout(sEntranceNative,entranceLayout,'
                                                 'entranceCaps);'):]
for needle in ('const auto songs=ComboRando::Entrances::LayoutSongTravel(sSongTravel.json(),entranceLayout,'
               'entranceCaps);', 'if (songs!=sSongTravel.json()) InstallSongTravel(songs);'):
    assert hostcxx.find(launcher_apply, needle) < hostcxx.find(launcher_apply, '} catch ('), needle
headless_apply = headless[hostcxx.find(headless, 'ComboRando::Entrances::ApplyUnifiedLayout(entranceNative,'
                                                 'entranceLayout,unified.active);'):]
for needle in ('const auto songs=ComboRando::Entrances::LayoutSongTravel(generatedSongs.json(),entranceLayout,'
               'unified.active);', 'if (songs!=generatedSongs.json()) {',
               'generatedSongs=ComboRando::SongTravelMap::parse(songs);', 'installSongs(generatedSongs);'):
    assert hostcxx.find(headless_apply, needle) < hostcxx.find(headless_apply, 'std::cout<<"[comborando] seed "'), \
        needle
assert hostcxx.contains(logical_oracle, 'map.logicEnabled || map.logicForeignSongs')
assert hostcxx.contains(mm_runtime, '.destination(source)') and '.logicDestination' not in mm_runtime

oot_header = (source/'combo/rando/SongTravelOOT.h').read_text(encoding='utf-8')
oot_tables = '\n'.join(re.findall(
    r'inline constexpr RandomizerRegion kOotSong(?:Regions|Sources)\[\].*?;', oot_header, re.S))
assert oot_tables.count('inline constexpr') == 2
oot_source = function(oot_header, 'inline int OotSongSource(')
mm_header = (source/'combo/rando/SongTravelMM.h').read_text(encoding='utf-8')
mm_tables = re.search(r'inline constexpr int kMmSongEntrances\[\].*?;', mm_header, re.S).group()
mm_source = function(mm_header, 'inline int MmSongSource(')
scene = (source/'mm/include/z64scene.h').read_text(encoding='utf-8')
scene_enum = 'typedef enum {\n' + scene.split('    /* 0x00 */ ENTR_SCENE_MAYORS_RESIDENCE,', 1)[1].split(
    '} EntranceSceneId;', 1)[0]
scene_enum = scene_enum.replace('typedef enum {\n', 'typedef enum {\n    ENTR_SCENE_MAYORS_RESIDENCE,', 1) + '} EntranceSceneId;\n'
entrance_macro = next(line for line in scene.splitlines() if line.startswith('#define ENTRANCE(scene, spawn)'))

# Cross-game arrivals load the target save before checking its cleared-world
# flags. This existing behavior must survive the routing correction.
title = (source/'mm/src/code/title_setup.c').read_text(encoding='utf-8')
arrival_block = function(title, 'if (songArrival>=0) {')
assert hostcxx.contains(arrival_block, 'gSaveContext.save.playerForm = PLAYER_FORM_HUMAN')
assert hostcxx.contains(arrival_block, 'gSaveContext.save.equippedMask = PLAYER_MASK_NONE')

prelude = r'''
#define COMBO_EXPORT
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>
#include "rando/SongTravel.h"
#include "soh/Enhancements/randomizer/randomizerEnums.h"
// The warp landings of a candidate of Warp Songs and Soaring Spots Shuffle, as each engine's layout gives them.
namespace Rando::ComboEntrances {
std::optional<std::array<uint32_t, 16>> testMasks;
std::optional<std::array<uint32_t, 16>> WarpLogicMasks() { return testMasks; }
}
bool MmWarpLogicLandings(int source, uint32_t& landings) {
    if (!Rando::ComboEntrances::testMasks)
        return false;
    landings = (*Rando::ComboEntrances::testMasks)[source];
    return true;
}
using s32 = int;
using RandoRegionId = int;
'''
storage = r'''
struct Region { RandomizerRegion randomizerRegionKey; };
Region targets[RR_MAX];
Region* RegionTable(RandomizerRegion key) { return &targets[key]; }
int GetRegionIdFromEntrance(int entrance) { return entrance; }
// Every region the exit leads to; none when the native exit is consumed by the combined cross-game search.
std::vector<int> OotRoutes(RandomizerRegion key,int fallback) {
    Region input{key}; Region* region=&input;
    std::vector<int> routes;
    for (int once=0;once<1;++once) {
        Region* exitRegion=RegionTable(static_cast<RandomizerRegion>(fallback));
'''
oot_tail = r'''
        routes.push_back(static_cast<int>(exitRegion-targets));
        }
    }
    return routes;
}
std::vector<int> MmRoutes(bool root,int exitId,int fallback) {
    const int currentRegion=root ? RR_MAX : RR_ROOT;
    std::vector<int> routes;
    for (int once=0;once<1;++once) {
        int connectedRegionId=fallback;
        int destinationEntrance=fallback;
        uint64_t arrivalTimes=1;
'''
mm_tail = r'''
        routes.push_back(connectedRegionId);
        }
    }
    return routes;
}
int One(const std::vector<int>& routes) {
    assert(routes.size()<=1);
    return routes.empty() ? -1 : routes.front();
}
int OotRoute(RandomizerRegion key,int fallback) { return One(OotRoutes(key,fallback)); }
int MmRoute(bool root,int exitId,int fallback) { return One(MmRoutes(root,exitId,fallback)); }
enum {PLAYER_FORM_HUMAN=4,PLAYER_MASK_NONE=0,
      WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE=1,WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE=2};
struct { struct { int playerForm=0,equippedMask=99,entrance=0; bool isOwlSave=true; } save; } gSaveContext;
unsigned cleared=0;
#define CHECK_WEEKEVENTREG(flag) ((cleared & (flag))!=0)
void TargetArrival(int songArrival) {
'''
tests = r'''
}
int main() {
    auto& map=ComboRando::ActiveSongTravel();
    unsigned routes=0,oracleQueries=0;
    // Sixteen rotated permutations make every source reach every destination.
    // Independent flags include runtime-only enhancements and saved overrides.
    for (int rotation=0;rotation<16;++rotation) for (unsigned flags=0;flags<16;++flags) {
        map.enabled=(flags&1)!=0; map.foreignSongs=(flags&2)!=0;
        map.logicEnabled=(flags&4)!=0; map.logicForeignSongs=(flags&8)!=0;
        for (int source=0;source<16;++source) map.destinations[source]=(source+rotation)%16;
        // Exercise the exact persisted representation, including unequal flags.
        map=ComboRando::SongTravelMap::parse(map.json());
        for (int source=0;source<16;++source) {
            const int logicalDestination=map.logicEnabled ? (source+rotation)%16 : source;
            const int runtimeDestination=map.enabled ? (source+rotation)%16 : source;
            const int oracleExpected=map.logicEnabled||map.logicForeignSongs ? logicalDestination : -1;
            assert(SOH_SongLogicDestination(source)==oracleExpected);
            assert(MM_ForeignSongDestination(source)==runtimeDestination);
            oracleQueries+=2;
            if (source<6) {
                const int fallback=ComboRando::kOotSongRegions[source];
                const int expected=logicalDestination>=6 ? -1 : ComboRando::kOotSongRegions[logicalDestination];
                assert(OotRoute(ComboRando::kOotSongSources[source],fallback)==expected);
                ++routes;
            } else {
                const int fallback=ComboRando::kMmSongEntrances[source-6];
                const int expected=logicalDestination<6 ? -1 : ComboRando::kMmSongEntrances[logicalDestination-6];
                assert(MmRoute(true,fallback,fallback)==expected);
                // A normal scene exit to the same entrance is never a song warp.
                assert(MmRoute(false,fallback,fallback)==fallback);
                routes+=2;
            }
        }
        assert(OotRoute(RR_ROOT,RR_LAKE_HYLIA)==RR_LAKE_HYLIA);
        assert(MmRoute(true,ENTRANCE(SOUTH_CLOCK_TOWN,0),12345)==12345);
        for(int invalid:{-1,16}) {
            assert(SOH_SongLogicDestination(invalid)==-1);
            assert(MM_ForeignSongDestination(invalid)==-1);
            assert(SOH_SongLogicDestinations(invalid)==0);
        }
        // Without a candidate the landing mask is the one destination.
        for (int source=0;source<16;++source) {
            const int destination=SOH_SongLogicDestination(source);
            assert(SOH_SongLogicDestinations(source)==(destination<0 ? 0u : 1u<<destination));
        }
    }
    // A candidate of Warp Songs and Soaring Spots Shuffle: each source may land at two destinations. The OoT search
    // takes every OoT landing of a song, MM's every MM landing of an owl, and each leaves the other game's landings
    // to the combined search; the oracle export answers with the mask whenever song travel logic is on.
    unsigned maskCases=0;
    for (int rotation=0;rotation<16;++rotation) for (int second=1;second<16;second+=7) {
        std::array<uint32_t,16> masks{};
        for (int source=0;source<16;++source)
            masks[source]=(1u<<((source+rotation)%16))|(1u<<((source+rotation+second)%16));
        Rando::ComboEntrances::testMasks=masks;
        for (bool logic:{false,true}) {
            map.enabled=map.logicEnabled=logic; map.logicForeignSongs=!logic;
            for (int source=0;source<16;++source) {
                assert(SOH_SongLogicDestinations(source)==(logic ? masks[source] : 1u<<source));
                if (!logic)
                    continue;
                std::vector<int> oot, mmLandings;
                for (int landing=0;landing<16;++landing)
                    if (masks[source]&(1u<<landing))
                        (landing<6 ? oot : mmLandings).push_back(landing);
                if (source<6) {
                    std::vector<int> expected;
                    for (int landing:oot) expected.push_back(ComboRando::kOotSongRegions[landing]);
                    assert(OotRoutes(ComboRando::kOotSongSources[source],ComboRando::kOotSongRegions[source])==expected);
                } else {
                    std::vector<int> expected;
                    for (int landing:mmLandings) expected.push_back(ComboRando::kMmSongEntrances[landing-6]);
                    const int entrance=ComboRando::kMmSongEntrances[source-6];
                    assert(MmRoutes(true,entrance,entrance)==expected);
                    assert(MmRoute(false,entrance,entrance)==entrance);
                }
                ++maskCases;
            }
        }
        assert(OotRoute(RR_ROOT,RR_LAKE_HYLIA)==RR_LAKE_HYLIA);
    }
    Rando::ComboEntrances::testMasks.reset();
    // The two MM seasonal destinations honor the newly loaded target save;
    // form/mask normalize without granting songs, owls or clearing a dungeon.
    unsigned arrivalCases=0;
    for(cleared=0;cleared<4;++cleared) for(int source=6;source<16;++source) {
        gSaveContext.save.playerForm=0;gSaveContext.save.equippedMask=99;
        gSaveContext.save.isOwlSave=true;
        const int entrance=ComboRando::kMmSongEntrances[source-6];
        TargetArrival(entrance);
        int expected=entrance;
        if(source==13&&(cleared&1)) expected=ENTRANCE(SOUTHERN_SWAMP_CLEARED,10);
        if(source==9&&(cleared&2)) expected=ENTRANCE(MOUNTAIN_VILLAGE_SPRING,8);
        assert(gSaveContext.save.entrance==expected);
        assert(gSaveContext.save.playerForm==PLAYER_FORM_HUMAN);
        assert(gSaveContext.save.equippedMask==PLAYER_MASK_NONE);
        assert(!gSaveContext.save.isOwlSave);
        ++arrivalCases;
    }
    gSaveContext.save={2,5,123};TargetArrival(-1);
    assert(gSaveContext.save.playerForm==2&&gSaveContext.save.equippedMask==5&&gSaveContext.save.entrance==123);
    assert(gSaveContext.save.isOwlSave); // An ordinary save resume retains its arrival state.
    std::cout<<"PASS: "<<routes<<" actual native route cases, "<<oracleQueries
             <<" logical/runtime export queries, "<<maskCases<<" candidate landing masks, "<<arrivalCases
             <<" seasonal/form arrivals; all endpoint permutations, saved overrides and unrelated exits\n";
}
'''
# The owl statues' warp rows (EntranceLayout.h LayoutV3::kWarpRows) are the Song of Soaring's native entrances, in the
# map's order: a Wallmaster sent to an owl with the warps shuffled arrives where Soaring to it does (CrossEntrance.inc.h
# MmArrivalBound binds those rows' ids), and the pads' ids are OoT's warp-song entrances (combo_entrance_runtime.cpp).
layout_header = (source/'combo/rando/entrances/EntranceLayout.h').read_text(encoding='utf-8')
warp_ids = re.findall(r'\{ "((?:OOT_WARP_SONG|MM_WARP_OWL)_\w+)", Game::\w+, (0x[0-9a-f]+)u,', layout_header)
assert [name for name, _ in warp_ids][:6] == ['OOT_WARP_SONG_' + n for n in
                                              ('MEADOW', 'CRATER', 'LAKE', 'DESERT', 'GRAVE', 'TEMPLE')], warp_ids
assert [int(i, 16) for _, i in warp_ids[:6]] == [0x600, 0x4f6, 0x604, 0x1f1, 0x568, 0x5f4], warp_ids
assert len(warp_ids) == 16 and all(name.startswith('MM_WARP_OWL_') for name, _ in warp_ids[6:]), warp_ids
owl_check = ('    constexpr int owls[] = {' + ', '.join(i for _, i in warp_ids[6:]) + '};\n'
             '    for (int i=0;i<10;++i) assert(ComboRando::kMmSongEntrances[i]==owls[i]);\n')
cross = (source/'mm/2s2h/Rando/CrossEntrance.inc.h').read_text(encoding='utf-8')
owl_bound = function(cross, 'bool MmArrivalBound(uint32_t id) {')
for needle in ('if (CEMmArrival::ActiveMmUnified().warps())',
               'for (int i = CEMmArrival::kOotWarpCount; i < 16; ++i)',
               'if (const auto* row = CEMmArrival::Find(CEMmArrival::kWarpRows[i]); row && row->id == id)'):
    assert hostcxx.contains(owl_bound, needle), needle
assert owl_bound.index('ActiveMmUnified().warps()') < owl_bound.index('if (!bound)')
tests = tests.replace('int main() {\n', 'int main() {\n' + owl_check, 1)
generated = prelude + scene_enum + entrance_macro + '\nnamespace ComboRando {\n' + oot_tables + '\n' + oot_source + mm_tables + '\n' + mm_source + '}\n'
generated += logical_oracle + logical_landings + mm_runtime + storage + oot_block + oot_tail + mm_block + mm_tail
generated += arrival_block + tests
test = work/'song_native_mapping.cpp'
test.write_text(generated, encoding='utf-8')
exe = hostcxx.build(args.compiler, test, werror=True, msvc=['/Zc:preprocessor'],
                    includes=[source/'combo', source/'soh', args.json_include.resolve()])
run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
(work/'result.txt').write_text(run.stdout + run.stderr, encoding='utf-8')
print(run.stdout + run.stderr, end='')
run.check_returncode()
