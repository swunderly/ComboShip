"""Temple probe starting items: native StartingInventory once, then foreign shared starters once.

Compiles the production ApplyProbeStartingItems (TempleProbe.cpp) and ApplyStartingInventory
(starting_inventory.cpp) against stub items whose effects land on the active Logic, as Item::ApplyEffect
does through Rando::ActiveLogic(). Prepare's name validation is checked in the source.
"""
from pathlib import Path
import argparse
import subprocess
import hostcxx
import temple_probe_stubs as stubs

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source = args.source.resolve()
work = args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
randomizer = source/stubs.RANDOMIZER
probe = (randomizer/'TempleProbe.cpp').read_text(encoding='utf-8')
starting = (randomizer/'3drando/starting_inventory.cpp').read_text(encoding='utf-8')
apply_probe = stubs.function(probe, 'void ApplyProbeStartingItems(Rando::Logic& probe, const std::vector<RandomizerGet>& foreign)')
apply_native = stubs.function(starting, 'void ApplyStartingInventory()')

# Prepare: unknown names and names outside the shared-starter domain (warp songs, progressive
# placeholders) are rejected; duplicates are kept once; the OoT
# dump's sharedStartingItems (the whole native list) is never read, and SetOwnedItems is never used.
prepare = stubs.function(probe, 'int Prepare(const char* json)')
domain = stubs.function(probe, 'bool IsSharedStarterName(const std::string& name)')
for needle in ['name == "Fairy Ocarina" || name == "Ocarina of Time"', 'i = ComboRando::SF_OCARINA_BUTTON_A',
               'i != ComboRando::SF_OCARINA', 'SharedFamilyByIndex(i).ootName']:
    assert needle in domain, needle
for needle in ['Rando::StaticData::itemNameToEnum.find(name)', 'throw Failure(kUnsupported, "unknown foreign starting item " + name);',
               '!IsSharedStarterName(name) || IsWarpSong(it->second)',
               'std::find(p.foreign.begin(), p.foreign.end(), it->second) == p.foreign.end()']:
    assert needle in prepare, needle
assert 'sharedStartingItems' not in probe and 'SetOwnedItems(' not in probe
assert probe.count('ApplyStartingInventory();') == 1

cpp = r'''
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
enum RandomizerGet {
    RG_NONE, RG_FAIRY_OCARINA, RG_OCARINA_OF_TIME, RG_PROGRESSIVE_OCARINA, RG_PROGRESSIVE_HOOKSHOT,
    RG_OCARINA_A_BUTTON, RG_IRON_BOOTS, RG_PIECE_OF_HEART, RG_HEART_CONTAINER, RG_TREASURE_GAME_HEART
};
std::vector<RandomizerGet> StartingInventory;
namespace Rando {
struct Logic {
    int ocarina = 0, hookshot = 0;
    std::set<int> held;
    std::map<int, int> applied;
    bool HasItem(RandomizerGet rg) const {
        if (rg == RG_FAIRY_OCARINA) return ocarina >= 1;
        if (rg == RG_OCARINA_OF_TIME) return ocarina >= 2;
        return held.count(rg) != 0;
    }
};
}
std::shared_ptr<Rando::Logic> logic; // the active Logic: the probe's private one inside its scope
namespace Rando {
struct Item {
    RandomizerGet rg = RG_NONE;
    void ApplyEffect() const { // lands on ActiveLogic(), i.e. ::logic during a probe query
        ++logic->applied[rg];
        if (rg == RG_PROGRESSIVE_OCARINA) ++logic->ocarina;
        else if (rg == RG_PROGRESSIVE_HOOKSHOT) ++logic->hookshot;
        else logic->held.insert(rg);
    }
};
namespace StaticData {
inline Item& RetrieveItem(RandomizerGet rg) {
    static std::map<int, Item> items;
    auto& item = items[rg];
    item.rg = rg;
    return item;
}
}
}
''' + apply_native + '\n' + apply_probe + r'''
Rando::Logic Run(std::vector<RandomizerGet> native, std::vector<RandomizerGet> foreign) {
    auto live = std::make_shared<Rando::Logic>(), probe = std::make_shared<Rando::Logic>();
    StartingInventory = native;
    logic = probe; // inside the probe scope
    ApplyProbeStartingItems(*probe, foreign);
    logic = live;
    assert(live->applied.empty());
    return *probe;
}
int main() {
    // The native list is applied exactly once, hearts skipped, progressive items not doubled.
    auto p = Run({ RG_PROGRESSIVE_HOOKSHOT, RG_PIECE_OF_HEART, RG_HEART_CONTAINER, RG_TREASURE_GAME_HEART,
                   RG_OCARINA_A_BUTTON }, {});
    assert(p.hookshot == 1 && p.applied.size() == 2 && p.applied[RG_OCARINA_A_BUTTON] == 1);
    // A foreign starter the native list already gave is skipped; a new one is applied once.
    p = Run({ RG_OCARINA_A_BUTTON, RG_PROGRESSIVE_HOOKSHOT }, { RG_OCARINA_A_BUTTON, RG_IRON_BOOTS });
    assert(p.applied[RG_OCARINA_A_BUTTON] == 1 && p.applied[RG_IRON_BOOTS] == 1 && p.hookshot == 1);
    // Ocarina tiers are idempotent and never exceed the shared tier.
    p = Run({ RG_PROGRESSIVE_OCARINA }, { RG_FAIRY_OCARINA });
    assert(p.ocarina == 1);
    p = Run({ RG_PROGRESSIVE_OCARINA }, { RG_OCARINA_OF_TIME });
    assert(p.ocarina == 2 && p.applied[RG_PROGRESSIVE_OCARINA] == 2);
    p = Run({}, { RG_OCARINA_OF_TIME, RG_FAIRY_OCARINA });
    assert(p.ocarina == 2);
    p = Run({ RG_PROGRESSIVE_OCARINA, RG_PROGRESSIVE_OCARINA }, { RG_FAIRY_OCARINA, RG_OCARINA_OF_TIME });
    assert(p.ocarina == 2 && p.applied[RG_PROGRESSIVE_OCARINA] == 2);
    std::cout << "Temple probe starting items: native once, foreign once, ocarina tiers idempotent\n";
}
'''
target = work/'temple_starting_items.cpp'
target.write_text(cpp, encoding='utf-8')
exe = hostcxx.build(args.compiler, target)
subprocess.run([str(exe)], check=True, timeout=hostcxx.RUN_TIMEOUT)
