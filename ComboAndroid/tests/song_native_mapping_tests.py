"""Compile actual native song search/export blocks with independent map flags.

The small engine stubs replace region storage, not route selection. Endpoint
tables, entrance encoding, search blocks and oracle exports come from source.
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
oot_block = routing_block(oot, '        if (ComboRando::ActiveSongTravel().logicEnabled) {')
mm_block = function(mm, '            if (currentRegion==RR_MAX && ComboRando::ActiveSongTravel().logicEnabled) {')
globals_cpp = (source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')
mm_port = (source/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8')
oracle = function(globals_cpp, 'extern "C" COMBO_EXPORT int SOH_SongDestination(')
logical_oracle = function(globals_cpp, 'extern "C" COMBO_EXPORT int SOH_SongLogicDestination(')
oot_runtime = function(globals_cpp, 'extern "C" COMBO_EXPORT int SOH_ForeignSongDestination(')
mm_runtime = function(mm_port, 'extern "C" COMBO_EXPORT int MM_ForeignSongDestination(')

# Bind the exercised export to the real launcher resolver. The legacy export
# is also tested so consumers cannot accidentally revive a runtime flag leak.
launcher = (source/'combo/ComboShip.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(launcher, 'SOH_SongDestination=(int(*)(int))GetSym(sohModule,"SOH_SongLogicDestination");')
assert hostcxx.contains(logical_oracle, 'map.logicEnabled || map.logicForeignSongs')
assert hostcxx.contains(oot_runtime, '.destination(source)') and '.logicDestination' not in oot_runtime
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
#include <cassert>
#include <iostream>
#include "rando/SongTravel.h"
#include "soh/Enhancements/randomizer/randomizerEnums.h"
'''
storage = r'''
struct Region { RandomizerRegion randomizerRegionKey; };
Region targets[RR_MAX];
Region* RegionTable(RandomizerRegion key) { return &targets[key]; }
int GetRegionIdFromEntrance(int entrance) { return entrance; }
int OotRoute(RandomizerRegion key,int fallback) {
    Region input{key}; Region* region=&input;
    for (int once=0;once<1;++once) {
        Region* exitRegion=RegionTable(static_cast<RandomizerRegion>(fallback));
'''
oot_tail = r'''
        return static_cast<int>(exitRegion-targets);
    }
    return -1; // Native exit is consumed by the combined cross-game search.
}
int MmRoute(bool root,int exitId,int fallback) {
    const int currentRegion=root ? RR_MAX : RR_ROOT;
    for (int once=0;once<1;++once) {
        int connectedRegionId=fallback;
        int destinationEntrance=fallback;
'''
mm_tail = r'''
        return connectedRegionId;
    }
    return -1;
}
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
            assert(SOH_SongDestination(source)==oracleExpected);
            assert(SOH_SongLogicDestination(source)==oracleExpected);
            assert(SOH_ForeignSongDestination(source)==runtimeDestination);
            assert(MM_ForeignSongDestination(source)==runtimeDestination);
            oracleQueries+=4;
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
            assert(SOH_SongDestination(invalid)==-1);
            assert(SOH_SongLogicDestination(invalid)==-1);
            assert(SOH_ForeignSongDestination(invalid)==-1);
            assert(MM_ForeignSongDestination(invalid)==-1);
        }
    }
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
             <<" logical/runtime export queries, "<<arrivalCases
             <<" seasonal/form arrivals; all endpoint permutations, saved overrides and unrelated exits\n";
}
'''
generated = prelude + scene_enum + entrance_macro + '\nnamespace ComboRando {\n' + oot_tables + '\n' + oot_source + mm_tables + '\n' + mm_source + '}\n'
generated += oracle + logical_oracle + oot_runtime + mm_runtime + storage + oot_block + oot_tail + mm_block + mm_tail + arrival_block + tests
test = work/'song_native_mapping.cpp'
test.write_text(generated, encoding='utf-8')
exe = hostcxx.build(args.compiler, test, werror=True, msvc=['/Zc:preprocessor'],
                    includes=[source/'combo', source/'soh', args.json_include.resolve()])
run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=hostcxx.RUN_TIMEOUT)
(work/'result.txt').write_text(run.stdout + run.stderr, encoding='utf-8')
print(run.stdout + run.stderr, end='')
run.check_returncode()
