"""Rando::ActiveLogic(): the five predicate/item-effect call sites, no stray GetLogic(), and its behaviour.

Outside a Temple probe query ActiveLogic() must be exactly the previous ctx->GetLogic() lookup, even when
::logic is stale (a save load replaces the Context without resetting ::logic, SaveManager.cpp:1497-1501).
"""
from pathlib import Path
import argparse
import re
import subprocess
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
randomizer = source/'soh/soh/Enhancements/randomizer'
access = (randomizer/'location_access.cpp').read_text(encoding='utf-8')
item = (randomizer/'item.cpp').read_text(encoding='utf-8')


def body(text, signature):
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('unterminated ' + signature)


# The five call sites read the active Logic and no longer ask the Context directly.
for text, signature in [(access, 'bool Region::CanPlantBeanCheck(RandomizerGet bean) const'),
                        (access, 'bool BeanPlanted(const RandomizerGet bean)'),
                        (item, 'void Item::ApplyEffect() const'), (item, 'void Item::UndoEffect() const'),
                        (item, 'std::shared_ptr<GetItemEntry> Item::GetGIEntry(RandomizerGet* actualOut) const')]:
    function = body(text, signature)
    assert hostcxx.contains(function, 'auto logic = Rando::ActiveLogic();') or hostcxx.contains(
        function, 'auto logic = ActiveLogic();'), signature
    assert 'GetLogic()' not in function, signature

# Every remaining GetLogic() in the randomizer is a known non-predicate site: resets, context
# wiring, the ::logic assignments, static tables and the tracker. A new one on a predicate path
# would let the probe read live inventory, so it must be reviewed here.
allowed = {
    'location_access.cpp': 2,  # ::logic = ctx->GetLogic() in RegionTable_Init; ActiveLogic's fallback
    'entrance.cpp': 1, 'item_list.cpp': 1, 'randomizer_check_tracker.cpp': 1, 'savefile.cpp': 1,
    'SeedContext.cpp': 2, 'SeedContext.h': 1, '3drando/playthrough.cpp': 1, '3drando/hints.cpp': 3,
    'TempleProbe.cpp': 3,  # the probe's preconditions compare ::logic with the Context's Logic
}
found = {}
for path in sorted(randomizer.rglob('*')):
    if path.suffix in ('.cpp', '.h', '.hpp', '.inc', '.c'):
        count = len(re.findall(r'\bGetLogic\(\)', path.read_text(encoding='utf-8', errors='replace')))
        if count:
            found[path.relative_to(randomizer).as_posix()] = count
assert found == allowed, found
for path in (source/'combo/rando').rglob('*.h'):
    assert 'GetLogic()' not in path.read_text(encoding='utf-8'), path
assert hostcxx.contains(access, 'Context::GetInstance()->GetLogic();\n}')  # the fallback, last in ActiveLogic

# RegionTable_Init advances the epoch before it replaces any Region.
init = body(access, 'void RegionTable_Init()')
assert init.index('++sRegionTableEpoch;') < init.index('areaTable.fill(')

cpp = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
namespace Rando {
struct Logic { int id = 0; };
struct Context {
    std::shared_ptr<Logic> mLogic = std::make_shared<Logic>();
    static std::shared_ptr<Context> GetInstance() { static auto instance = std::make_shared<Context>(); return instance; }
    std::shared_ptr<Logic> GetLogic() { return mLogic; }
};
std::shared_ptr<Logic> ActiveLogic();
bool& ProbeLogicActive();
}
std::shared_ptr<Rando::Logic> logic;
'''
cpp += 'static uint32_t sRegionTableEpoch = 0;\n'
cpp += body(access, 'bool& Rando::ProbeLogicActive()') + '\n'
cpp += body(access, 'std::shared_ptr<Rando::Logic> Rando::ActiveLogic()') + '\n'
cpp += body(access, 'uint32_t RegionTable_Epoch()') + '\n'
cpp += r'''
int main() {
    auto live = Rando::Context::GetInstance()->GetLogic();
    assert(!logic && Rando::ActiveLogic() == live);            // before RegionTable_Init
    logic = live;
    assert(Rando::ActiveLogic() == live);                      // the normal generation state
    logic = std::make_shared<Rando::Logic>();                  // stale after a save load
    assert(Rando::ActiveLogic() == live);                      // still the Context's Logic, as before
    auto probe = std::make_shared<Rando::Logic>();
    logic = probe;
    Rando::ProbeLogicActive() = true;                          // inside one probe query only
    assert(Rando::ActiveLogic() == probe);
    Rando::ProbeLogicActive() = false;
    assert(Rando::ActiveLogic() == live && RegionTable_Epoch() == 0);
    std::cout << "ActiveLogic call sites, GetLogic allow-list and probe scope verified\n";
}
'''
target = work/'temple_active_logic.cpp'
target.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(args.compiler, target)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
