"""Map the in-game clock to the live tracker's current MM time slice (CurrentTimeSlice)."""
from pathlib import Path
import argparse, re, subprocess
import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
logic = (a.source/'mm/2s2h/Rando/Logic/Logic.h').read_text(encoding='utf-8')
enum = re.search(r'enum TimeSlice \{.*?\};', logic, re.S)
assert enum, 'TimeSlice enum missing'
timeLogic = (a.source/'mm/2s2h/Rando/Logic/TimeLogic.cpp').read_text(encoding='utf-8')
table = re.search(r'static constexpr uint16_t kSliceStartMinutes\[TIME_SLICE_COUNT\] = \{.*?\};', timeLogic, re.S)
function = re.search(r'uint64_t CurrentTimeSlice\(\) \{.*?\n\}', timeLogic, re.S)
assert table and function, 'CurrentTimeSlice or its table missing'
tracker = (a.source/'mm/2s2h/Rando/CheckTracker/CheckTracker.cpp').read_text(encoding='utf-8')
assert hostcxx.contains(tracker, 'regionTimeStates[here] = { .timeSlices = Rando::Logic::TimeLogic::CurrentTimeSlice()'), \
    'The live tracker must seed the current region at the current time, not every owned slice'

code = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
struct { struct { int day; uint16_t time; } save; } gSaveContext;
''' + enum.group() + r'''
constexpr int TIME_SLICE_COUNT = TIME_NIGHT3_AM_05_00 + 1;
constexpr uint64_t TIME_BIT_ONE = 1ULL;
''' + table.group() + '\n' + function.group() + r'''
static int SliceAt(int day, int hour, int minute) {
    gSaveContext.save.day = day;
    gSaveContext.save.time = uint16_t(((hour * 60 + minute) * 0x10000 + 24 * 60 - 1) / (24 * 60));
    const uint64_t bit = CurrentTimeSlice();
    assert(bit && (bit & (bit - 1)) == 0);
    int slice = 0;
    while (!(bit & (TIME_BIT_ONE << slice)))
        ++slice;
    return slice;
}
int main() {
    for (int i = 1; i < TIME_SLICE_COUNT; ++i)
        assert(kSliceStartMinutes[i - 1] < kSliceStartMinutes[i]);
    assert(SliceAt(1, 6, 0) == TIME_DAY1_AM_06_00);
    assert(SliceAt(1, 13, 44) == TIME_DAY1_AM_10_00);
    assert(SliceAt(1, 13, 45) == TIME_DAY1_PM_01_45);
    assert(SliceAt(1, 18, 0) == TIME_NIGHT1_PM_06_00);
    // After midnight the day counter has not advanced yet: still Night 1.
    assert(SliceAt(1, 2, 29) == TIME_NIGHT1_AM_12_00);
    assert(SliceAt(1, 2, 30) == TIME_NIGHT1_AM_02_30);
    assert(SliceAt(1, 5, 59) == TIME_NIGHT1_AM_05_00);
    assert(SliceAt(2, 6, 0) == TIME_DAY2_AM_06_00);
    assert(SliceAt(2, 5, 30) == TIME_NIGHT2_AM_05_30);
    // The reported case: Romani Ranch entered on Day 3 is not reachable on Day 1 or Night 1.
    assert(SliceAt(3, 9, 0) == TIME_DAY3_AM_08_00);
    assert(SliceAt(3, 13, 0) == TIME_DAY3_PM_01_00);
    assert(SliceAt(3, 5, 59) == TIME_NIGHT3_AM_05_00);
    // Out-of-range day counters (title screen, Moon) clamp into the cycle.
    assert(SliceAt(0, 6, 0) == TIME_DAY1_AM_06_00);
    assert(SliceAt(4, 6, 0) == TIME_DAY3_AM_06_00);
    std::puts("MM current time slice: clock mapping and live seeding passed");
}
'''
a.build_dir.mkdir(parents=True, exist_ok=True)
source = a.build_dir/'current_time_slice.cpp'
source.write_text(code, encoding='utf-8')
binary = hostcxx.build(a.compiler, source, werror=True)
subprocess.run([str(binary)], check=True, timeout=hostcxx.RUN_TIMEOUT)
