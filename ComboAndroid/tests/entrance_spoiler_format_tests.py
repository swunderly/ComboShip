"""Compare the pure formatter's bytes and route identities with pinned OoTMM text.

No engine, seed generation or device work. Repeated destinations must preserve every
source; display-key order differs from registry-id order in the literal golden below.
"""
from pathlib import Path
import argparse
import json
import re
import subprocess

import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
parser.add_argument('--json-include', type=Path, required=True)
args = parser.parse_args()
args.source = args.source.resolve()
args.build_dir.mkdir(parents=True, exist_ok=True)
fixture = json.loads((args.source/'tools/ootmm/m1-contract.json').read_text(encoding='utf-8'))
assert fixture['upstreamCommit'] == '413573d22d1d53e8a81931a70927f625b73488b9'
assert fixture['inputs']['packages/generator/lib/combo/spoiler.ts'] == \
    '2f09cdd163cdb6bb6db1812810c975148379c336117d8210522b23b65f7ce211'
rows = fixture['rows']
# The contract's noErSection separately pins the implicit links, which are not pool rows.
rows.update({
    'OOT_SHOP_MASKS': {'from': 'OOT Market', 'to': 'OOT Market Mask Shop'},
    'MM_CLOCK_TOWN_FROM_CLOCK_TOWER': {'from': 'MM Clock Tower', 'to': 'MM Clock Town From Clock Tower'},
    'MM_CLOCK_TOWER_FROM_CLOCK_TOWN': {'from': 'MM Clock Town Near Clock Tower', 'to': 'MM Clock Tower'},
    'OOT_MARKET_FROM_MASK_SHOP': {'from': 'OOT Market Mask Shop', 'to': 'OOT Market'},
})
# Byte sorting/width must remain equivalent to JavaScript UTF-16 sort/padEnd.
assert all(rows[name][field].isascii() for name in rows for field in ('from', 'to'))
assert all(name.isascii() for name in rows)

source = r'''
#include <cassert>
#include <iostream>
#include "combo/rando/entrances/EntranceSpoiler.h"
using namespace ComboRando::Entrances;
int main() {
    nlohmann::json input;
    std::cin >> input;
    nlohmann::json output = nlohmann::json::array();
    for (const auto& test : input) {
        if (test.is_null()) {
            output.push_back(FormatEntranceSpoiler(nullptr));
            continue;
        }
        ValidatedLayout layout;
        for (const auto& pair : test) {
            const auto* source = LayoutV1::FindRow(pair.at(0).get<std::string>());
            const auto* destination = LayoutV1::FindRow(pair.at(1).get<std::string>());
            assert(source && destination);
            (source->game == Game::Oot ? layout.oot : layout.mm).push_back({source, destination});
        }
        output.push_back(FormatEntranceSpoiler(&layout));
    }
    std::cout << output.dump();
}
'''
cpp = args.build_dir/'entrance_spoiler_format.cpp'
cpp.write_text(source, encoding='utf-8', newline='\n')
exe = hostcxx.build(args.compiler, cpp, includes=[args.source, args.json_include], werror=True)

LINKS = [('OOT_SHOP_MASKS', 'MM_CLOCK_TOWN_FROM_CLOCK_TOWER'),
         ('MM_CLOCK_TOWER_FROM_CLOCK_TOWN', 'OOT_MARKET_FROM_MASK_SHOP')]
MIXED = [('OOT_WALLMASTER_FOREST_CORRIDOR_WEST', 'OOT_TEMPLE_SPIRIT'),
         ('MM_WALLMASTER_BTW_FOUNTAIN', 'MM_TEMPLE_WOODFALL'),
         ('MM_WALLMASTER_DAMPE', 'MM_TEMPLE_WOODFALL')]
TRANCHE = [('MM_WALLMASTER_DAMPE', 'MM_TEMPLE_WOODFALL'),
           ('MM_WALLMASTER_BTW_ENTRANCE', 'MM_TEMPLE_SNOWHEAD'),
           ('MM_WALLMASTER_BTW_FOUNTAIN', 'MM_TEMPLE_GREAT_BAY'),
           ('MM_WALLMASTER_BTW_EXIT', 'MM_TEMPLE_STONE_TOWER')]
# Literal snapshot from pinned spoiler.ts templates and entrances.yml, including padding.
MIXED_GOLDEN = (
    'Entrances\n'
    '  MM Beneath The Graveyard Night 3 Wallmaster to MM VOID (MM_WALLMASTER_DAMPE)         -> MM Woodfall Temple from MM Woodfall Front of Temple (MM_TEMPLE_WOODFALL)\n'
    '  MM Beneath The Well Wallmaster Near Fountain to MM VOID (MM_WALLMASTER_BTW_FOUNTAIN) -> MM Woodfall Temple from MM Woodfall Front of Temple (MM_TEMPLE_WOODFALL)\n'
    '  MM Clock Town Near Clock Tower to MM Clock Tower (MM_CLOCK_TOWER_FROM_CLOCK_TOWN)    -> OOT Market from OOT Market Mask Shop (OOT_MARKET_FROM_MASK_SHOP)\n'
    '  OOT Forest Temple Wallmaster West to OOT VOID (OOT_WALLMASTER_FOREST_CORRIDOR_WEST)  -> OOT Spirit Temple from OOT Desert Colossus (OOT_TEMPLE_SPIRIT)\n'
    '  OOT Market to OOT Market Mask Shop (OOT_SHOP_MASKS)                                  -> MM Clock Town From Clock Tower from MM Clock Tower (MM_CLOCK_TOWN_FROM_CLOCK_TOWER)\n'
)
cases = [None, [], MIXED, list(reversed(MIXED)), TRANCHE]
result = subprocess.run([str(exe)], input=json.dumps(cases), capture_output=True, text=True,
                        check=True, timeout=hostcxx.RUN_TIMEOUT)
texts = json.loads(result.stdout)
assert texts[0] == ''  # no validated unified layout: no legacy/no-layout format change
assert texts[1] == '\n'.join(fixture['spoiler']['noErSection'])
assert texts[2] == MIXED_GOLDEN
assert texts[3] == texts[2]  # insertion/game order is irrelevant

for case, text in zip(cases[1:], texts[1:]):
    assert text.startswith('Entrances\n') and text.endswith('\n') and not text.endswith('\n\n')
    lines = text.splitlines()[1:]
    assert len(lines) == len(case) + 2
    # Recover source/destination identities from text and compare with the input JSON.
    # The same destination may appear twice: C19 demonstrated this in OoTMM itself.
    identities = []
    keys = []
    arrow_columns = set()
    for line in lines:
        match = re.fullmatch(r'  (.+? \(([^()]+)\)) +-> (.+? \(([^()]+)\))', line)
        assert match, line
        key, src, value, dst = match.groups()
        identities.append((src, dst))
        keys.append(key)
        arrow_columns.add(line.index(' -> '))
        assert key == f'{rows[src]["from"]} to {rows[src]["to"]} ({src})'
        assert value == f'{rows[dst]["to"]} from {rows[dst]["from"]} ({dst})'
    assert sorted(identities) == sorted([tuple(pair) for pair in case] + LINKS)
    assert keys == sorted(keys) and len(arrow_columns) == 1
    assert arrow_columns == {2 + max(map(len, keys))}
assert texts[2].index('(MM_WALLMASTER_DAMPE)') < texts[2].index('(MM_WALLMASTER_BTW_FOUNTAIN)')
print('PASS: pinned spoiler golden bytes, implicit links, sorted/padded labels, route identity and shared destinations')
