"""ABI-only, non-engine evidence for SOH_GetOracleTempleAges (bit order, target region, no search).

This compiles the extracted production function against a stub region table. It says nothing about
what a real loaded SoH engine reports after a native search; that needs a real-engine run.
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
globals_cpp = (source/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8')


def function(signature):
    start = hostcxx.find(globals_cpp, signature)
    opening = globals_cpp.index('{', start)
    end = globals_cpp.index('\n}\n', opening) + 2
    return globals_cpp[start:end]


getter = function('extern "C" COMBO_EXPORT uint8_t SOH_GetOracleTempleAges(void)')
portal = function('extern "C" COMBO_EXPORT uint8_t SOH_GetOraclePortalAges(void)')
# Same layout as the portal getter, beside it, inside the same COMBO_BUILD block.
assert portal.replace('portal', 'x').replace('RR_MARKET_MASK_SHOP', 'R') == \
       getter.replace('SOH_GetOracleTempleAges', 'SOH_GetOraclePortalAges').replace('temple', 'x').replace(
           'RR_TEMPLE_OF_TIME', 'R')
block = globals_cpp[globals_cpp.rindex('#ifdef COMBO_BUILD', 0, globals_cpp.index(getter)):]
assert block.index(getter) < block.index('#endif')
# Read-only: one region read, no search, reset, oracle flag or setter.
calls = set(re.findall(r'\b([A-Za-z_][A-Za-z0-9_:]*)\s*\(', getter)) - {'SOH_GetOracleTempleAges', 'return'}
assert calls == {'RegionTable', 'Adult', 'Child'}, calls
assert re.findall(r'\bRR_[A-Z_]+', getter) == ['RR_TEMPLE_OF_TIME']
# Not looked up by any combo or launcher source in this unit.
for root in (source/'combo', source/'ComboAndroid/app'):
    for path in root.rglob('*'):
        if path.suffix in ('.cpp', '.h', '.inc', '.java', '.kt'):
            assert 'SOH_GetOracleTempleAges' not in path.read_text(encoding='utf-8', errors='replace'), path

cpp = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
#define COMBO_EXPORT
enum RandomizerRegion { RR_MARKET_MASK_SHOP, RR_TEMPLE_OF_TIME, RR_MAX };
struct Region {
    bool childDay = false, childNight = false, adultDay = false, adultNight = false;
    bool Child() const { return childDay || childNight; }
    bool Adult() const { return adultDay || adultNight; }
};
Region regions[RR_MAX];
int reads[RR_MAX];
Region* RegionTable(RandomizerRegion key) { ++reads[key]; return &regions[key]; }
'''
cpp += getter + '\n' + r'''
int main() {
    for (int bits = 0; bits < 16; ++bits) {
        Region& t = regions[RR_TEMPLE_OF_TIME];
        t.childDay = bits & 1; t.childNight = bits & 2; t.adultDay = bits & 4; t.adultNight = bits & 8;
        regions[RR_MARKET_MASK_SHOP] = Region{true, true, true, true};  // must not leak in
        const uint8_t expected = ((bits & 12) ? 1 : 0) | ((bits & 3) ? 2 : 0);  // bit0 adult, bit1 child
        assert(SOH_GetOracleTempleAges() == expected);
    }
    assert(reads[RR_TEMPLE_OF_TIME] == 16 && reads[RR_MARKET_MASK_SHOP] == 0);
    std::cout << "ABI-only: SOH_GetOracleTempleAges bit order and target region (no engine run)\n";
}
'''
target = work/'oot_temple_ages_getter.cpp'
target.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(args.compiler, target)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
