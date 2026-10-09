"""Compile native time propagation and actual Clock Town opening predicates."""
from pathlib import Path
import argparse
import subprocess
import re
import json

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
a = p.parse_args()
root = a.source.resolve()
work = a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)

def function(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

def macro_condition(text, prefix):
    start = text.index(prefix)
    start = text.index('(', start) + 1
    depth, begin, args = 0, start, []
    for i in range(start, len(text)):
        c = text[i]
        if c == '(':
            depth += 1
        elif c == ')':
            if depth == 0:
                args.append(text[begin:i])
                return args[-1].strip()
            depth -= 1
        elif c == ',' and depth == 0:
            args.append(text[begin:i])
            begin = i + 1
    raise AssertionError(prefix)

h = (root/'mm/2s2h/Rando/Logic/Logic.h').read_text()
logic = (root/'mm/2s2h/Rando/Logic/Logic.cpp').read_text()
time = (root/'mm/2s2h/Rando/Logic/TimeLogic.cpp').read_text()
central = (root/'mm/2s2h/Rando/Logic/Regions/Central.cpp').read_text()
reference = Path(__file__).resolve().parents[3]/'references/ootmm-stable-32.3'
# Compare the actual community predicates with matching supported options:
# rusty-key shuffle is not supported, so those doors have their normal keys.
contracts = json.loads((Path(__file__).parent/'fixtures/ootmm-32.3-contracts.json').read_text())
if reference.exists():
    community = (reference/'data/world/mm/overworld/clock_town.yml').read_text()
    for name, expected in contracts['doors'].items():
        actual = re.search(r'^    "'+re.escape(name)+r'": "([^\n]+)"$',community,re.M).group(1)
        assert actual == expected, 'Community snapshot drift: '+name
def community_exit(name):
    expression = contracts['doors'][name]
    expression = re.sub(r'rusty_key\([^)]*\)', 'true', expression)
    expression = expression.replace('has(ROOM_KEY)','roomKey').replace('has(MASK_ROMANI)','romani')
    expression = re.sub(r'\b(DAY[123]|NIGHT[123])_',r'TIME_\1_',expression)
    expression = expression.replace('between(', 'BETWEEN(').replace('after(', 'AFTER(')
    return expression

enum_start = h.index('enum TimeSlice')
enum_end = h.index('extern uint64_t gCurrentRegionTime', enum_start)
clock_start = h.index('inline uint32_t ClockCount()')
clock_end = h.index('// PUBLIC TIME API', clock_start)
macro_start = h.index('#define AT(slice)', clock_end)
macro_end = h.index('\n', h.index('#define BETWEEN(s, e)', macro_start))
code = '''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#define SPDLOG_WARN(...) ((void)0)
namespace Rando { namespace Logic {
using u16 = uint16_t;
using s32 = int32_t;
enum RandoInf { RANDO_INF_OBTAINED_CLOCK_DAY_1=0 };
enum { RO_CLOCK_SHUFFLE, RO_CLOCK_SHUFFLE_PROGRESSIVE,
       RO_CLOCK_SHUFFLE_RANDOM=10, RO_CLOCK_SHUFFLE_ASCENDING, RO_CLOCK_SHUFFLE_DESCENDING };
uint32_t RANDO_SAVE_OPTIONS[2]{};
bool clocks[6]{};
bool Flags_GetRandoInf(RandoInf flag) { return clocks[int(flag)]; }
''' + h[enum_start:enum_end] + '''
uint64_t gCurrentRegionTime=0;
using RandoRegionId = int;
constexpr int RR_MAX=0;
struct RandoRegionExit { std::function<bool()> condition; };
struct RandoRegion {
    bool canStayOverTime=true;
    std::map<int,std::pair<std::function<bool()>,std::string>> connections;
    std::map<int,RandoRegionExit> exits;
    std::unordered_map<TimeSlice,std::function<bool()>> timeStayRestrictions;
};
std::map<int,RandoRegion> Regions;
int GetRegionIdFromEntrance(int entrance) {return entrance;}
''' + h[clock_start:clock_end] + h[macro_start:macro_end] + '\n' + '''
namespace TimeLogic {
uint64_t GetOwnedTimeSlices();
''' + '\n'.join(function(time, s) for s in (
    'uint64_t EvaluateConditionTimes(', 'uint64_t ExpandTimeForward(', 'uint64_t GetOwnedTimeSlices(')) + '''
}
''' + '\n'.join(function(logic, s) for s in (
    'RegionTimeState InitialTimeState(',
    'std::unordered_map<RandoRegionId, RegionTimeState> InitializeRegionTimeStates(',
    'void EnsureRegionTimeState(', 'void FindReachableRegions(')) + '''
enum {ITEM_MASK_ROMANI};
bool romani=false, roomKey=false;
#define HAS_ITEM(item) romani
bool MilkDoor() {return ''' + macro_condition(central,'EXIT(ENTRANCE(MILK_BAR, 0)') + ''';}
bool CommunityMilkDoor() {return ''' + community_exit('Milk Bar') + ''';}
bool CommunityInnDoor() {return ''' + community_exit('Stock Pot Inn') + ''';}
bool InnDoor() {return ''' + macro_condition(central,'EXIT(ENTRANCE(STOCK_POT_INN, 0)') + ''';}
} }
using namespace Rando::Logic;
uint64_t Bit(TimeSlice s) {return TIME_BIT_ONE << s;}
int main() {
    RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE]=0;
    for(int membership=0;membership<2;++membership)for(int key=0;key<2;++key)
      for(int slice=0;slice<TIME_SLICE_COUNT;++slice) {
        romani=membership!=0;roomKey=key!=0;gCurrentRegionTime=TIME_BIT_ONE<<slice;
        assert(MilkDoor()==CommunityMilkDoor());
        assert(InnDoor()==CommunityInnDoor());
      }
    gCurrentRegionTime=TIME_ALL_SLICES;
    assert(TimeLogic::EvaluateConditionTimes([]{return true;},TIME_ALL_SLICES)==TIME_ALL_SLICES);
    assert(TimeLogic::EvaluateConditionTimes([]{return false;},TIME_ALL_SLICES)==0);
    assert(TimeLogic::EvaluateConditionTimes([]{return IS_DAY1() && IS_DAY3();},TIME_ALL_SLICES)==0);
    assert(gCurrentRegionTime==TIME_ALL_SLICES);
    assert(TimeLogic::EvaluateConditionTimes([]{return IS_DAY1() || IS_DAY3();},TIME_ALL_SLICES)==
           (GetHalfDayTimeMask(0)|GetHalfDayTimeMask(4)));
    assert(TimeLogic::EvaluateConditionTimes([]{return true;},0)==0);
    try {TimeLogic::EvaluateConditionTimes([]()->bool{throw std::runtime_error("input failure");},Bit(TIME_DAY1_AM_06_00));}
    catch(const std::runtime_error&) {}
    assert(gCurrentRegionTime==TIME_ALL_SLICES);
    // Real native opening hours, checked against an independent interval table.
    const int milkStarts[]={TIME_DAY1_AM_10_00,TIME_DAY2_AM_10_00,TIME_DAY3_AM_10_00};
    const int milkEnds[]={TIME_NIGHT1_PM_09_00,TIME_NIGHT2_PM_09_00,TIME_NIGHT3_PM_09_00};
    const int nights[]={TIME_NIGHT1_PM_10_00,TIME_NIGHT2_PM_10_00,TIME_NIGHT3_PM_10_00};
    const int nightEnds[]={TIME_NIGHT1_AM_05_00,TIME_NIGHT2_AM_05_00,TIME_SLICE_COUNT};
    for(int member=0;member<2;++member) {
        romani=member!=0;
        uint64_t expected=0;
        for(int i=0;i<TIME_SLICE_COUNT;++i)for(int day=0;day<3;++day)
            if((i>=milkStarts[day]&&i<milkEnds[day])||
               (member&&i>=nights[day]&&i<nightEnds[day])) expected|=TIME_BIT_ONE<<i;
        assert(TimeLogic::EvaluateConditionTimes(MilkDoor,TIME_ALL_SLICES)==expected);
    }
    // Compile the actual search. A timed connection must not carry closed hours.
    for(int useExit=0;useExit<2;++useExit)for(int member=0;member<2;++member) {
        romani=member!=0; Regions.clear();Regions[0].canStayOverTime=true;
        Regions[1].canStayOverTime=true;
        if(useExit) Regions[0].exits[1]={MilkDoor};
        else Regions[0].connections[1]={MilkDoor,"native milk bar door"};
        for(auto s: {TIME_NIGHT1_PM_10_00,TIME_NIGHT1_AM_05_00,
                     TIME_NIGHT2_PM_10_00,TIME_NIGHT2_AM_05_00,
                     TIME_NIGHT3_PM_10_00,TIME_NIGHT3_AM_05_00})
            Regions[1].timeStayRestrictions[s]=[]{return false;};
        std::set<int> reachable{0};auto states=InitializeRegionTimeStates(0);states[0].canStayOverTime=true;
        FindReachableRegions(0,reachable,states);
        assert(reachable.count(1));
        assert(bool(states[1].timeSlices&Bit(TIME_NIGHT1_AM_12_00))==bool(member));
        assert(!(states[1].timeSlices&Bit(TIME_NIGHT1_AM_05_00)));
    }
    // Restricted stay can stop one visit; a later legitimate arrival reopens it.
    RandoRegion restricted;
    restricted.timeStayRestrictions[TIME_NIGHT1_PM_08_00]=[]{return false;};
    uint64_t expanded=TimeLogic::ExpandTimeForward(Bit(TIME_DAY1_AM_10_00),restricted);
    assert(!(expanded&Bit(TIME_NIGHT1_PM_08_00)));
    assert(!(expanded&Bit(TIME_NIGHT1_AM_12_00)));
    expanded=TimeLogic::ExpandTimeForward(Bit(TIME_DAY1_AM_10_00)|Bit(TIME_DAY2_AM_10_00),restricted);
    assert(expanded&Bit(TIME_DAY2_AM_10_00));
    // Canonical progressive clocks grant actual flags in the same order as runtime.
    for(int mode: {RO_CLOCK_SHUFFLE_RANDOM,RO_CLOCK_SHUFFLE_ASCENDING,RO_CLOCK_SHUFFLE_DESCENDING})
      for(int count=1;count<=6;++count) {
        std::fill(std::begin(clocks),std::end(clocks),false);
        RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE]=1;
        RANDO_SAVE_OPTIONS[RO_CLOCK_SHUFFLE_PROGRESSIVE]=mode;
        uint64_t expected=0;
        for(int i=0;i<count;++i) {const int index=mode==RO_CLOCK_SHUFFLE_DESCENDING?5-i:i;
            clocks[index]=true;expected|=GetHalfDayTimeMask(index);}
        assert(TimeLogic::GetOwnedTimeSlices()==expected);
        assert(TimeLogic::EvaluateConditionTimes([]{return true;},expected)==expected);
        assert((TimeLogic::ExpandTimeForward(expected,RandoRegion{})&~expected)==0);
      }
    std::cout << "Native MM scheduled arrivals, stay boundaries and clock modes passed\\n";
}
'''
# Inn needs its real flag input; no copied opening predicate is substituted.
code = code.replace('Flags_GetRandoInf(RANDO_INF_OBTAINED_ROOM_KEY)', 'roomKey')
(work/'mm_schedules.cpp').write_text(code,encoding='utf-8')
result=subprocess.run(['cl','/nologo','/std:c++20','/EHsc','/W4','/WX','/wd4018','/UNDEBUG',
                      str(work/'mm_schedules.cpp'),'/Fe:'+str(work/'mm_schedules.exe'),
                      '/Fo:'+str(work/'mm_schedules.obj')],capture_output=True,text=True)
(work/'compile.log').write_text(result.stdout+result.stderr)
assert result.returncode==0,result.stdout+result.stderr
subprocess.run([str(work/'mm_schedules.exe')],check=True)
