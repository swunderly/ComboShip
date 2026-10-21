"""Pin the port's copy of OoTMM's OoT scene restriction changes to the exact OoTMM v32.3 file.

OoTMM writes six bytes of OoT's scene restriction table in every seed (packages/generator/src/oot/patches/
scene_restrictions.S, no PATCH_GROUP). This reads that file from the pinned revision and decodes each byte against SoH's
table (soh/src/code/z_parameter.c sRestrictionFlags, OoT USA 1.0's entries in the same order) at the table's address in
OoT USA 1.0, 0x800f7350 (read from that ROM's code file when the port was written). The decoded bytes must be exactly
combo/rando/OotSceneRestrictions.h's kOotmmSceneRestrictionBytes, and each must do what the file's comment says: clear
warpSongs 3, Farore's Wind or the ocarina, and nothing else. It also checks that OoTMM keeps vanilla's refusal of OoT's
warp songs where warpSongs is 3: its only patch in that code (oot/ocarina.S, 0x800df1bc) ends before the refusal test,
which starts at 0x800df1d4 in OoT USA 1.0.

Skipped only without the OoTMM checkout (COMBO_OOTMM_REFERENCE, or references/ootmm-stable-32.3 beside the repository);
a checkout at another revision fails.
"""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
import sys
import hostcxx

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--json-include', type=Path, required=True)
parser.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
args = parser.parse_args()
source, work = args.source.resolve(), args.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)
reference = os.environ.get('COMBO_OOTMM_REFERENCE')
if reference:
    reference = Path(reference)
else:
    reference = next((parent/'references/ootmm-stable-32.3' for parent in source.parents
                      if (parent/'references/ootmm-stable-32.3/.git').exists()), None)
if reference is None:
    print('SKIPPED: set COMBO_OOTMM_REFERENCE to the exact local OoTMM v32.3 checkout')
    sys.exit(0)

sys.path.insert(0, str(source/'tools/ootmm'))
import export_registry as registry

TABLE = 0x800f7350            # OoT USA 1.0 sRestrictionFlags
WARP_SONG_TEST = 0x800df1bc   # OoTMM's Ocarina_ShouldIgnoreWarpSong hook (oot/ocarina.S)
REFUSAL = (0x800df1d4, 0x800df214)  # vanilla: disableWarpSongs || warpSongs == 3 -> "You can't warp here!" (0x88C)
failures = []


def check(ok, message):
    if not ok:
        failures.append(message)
    return ok


def blob(path):
    return registry.verified_blob(reference, path).decode('utf-8')  # raises on any other revision


def patches(text):
    """[(address, comment, [lines])] for each PATCH_START ... PATCH_END block, with the comment above it."""
    blocks, comment = [], None
    lines = iter(text.splitlines())
    for line in lines:
        stripped = line.strip()
        found = re.fullmatch(r'/\*\s*(.*?)\s*\*/', stripped)
        if found:
            comment = found.group(1)
            continue
        start = re.fullmatch(r'PATCH_START\s+(0x[0-9a-fA-F]+)', stripped)
        if not start:
            continue
        body = []
        for inner in lines:
            if inner.strip() == 'PATCH_END':
                break
            if inner.strip():
                body.append(inner.strip())
        blocks.append((int(start.group(1), 16), comment, body))
    return blocks


# OoTMM's file: six one-byte writes in three labelled groups, applied to every seed.
restrictions = blob('packages/generator/src/oot/patches/scene_restrictions.S')
check('PATCH_GROUP' not in restrictions, 'scene_restrictions.S now depends on a setting (PATCH_GROUP)')
check(restrictions.count('PATCH_START') == restrictions.count('PATCH_END') == 6,
      'scene_restrictions.S has other patches')
writes = []
for address, comment, body in patches(restrictions):
    value = re.fullmatch(r'\.byte\s+(0x[0-9a-fA-F]+)', body[0]) if len(body) == 1 else None
    if check(value is not None, f'{address:#x}: not a single .byte write: {body}'):
        writes.append((address, comment, int(value.group(1), 16)))

# SoH's table, which is OoT USA 1.0's, and its scene ids.
scenes = {}
for line in (source/'soh/include/tables/scene_table.h').read_text(encoding='utf-8').splitlines():
    found = re.match(r'/\* 0x([0-9A-F]+) \*/\s*DEFINE_SCENE\([^,]+,[^,]+,\s*(\w+)', line)
    if found:
        scenes[found.group(2)] = int(found.group(1), 16)
parameter = (source/'soh/src/code/z_parameter.c').read_text(encoding='utf-8')
table = parameter[hostcxx.find(parameter, 'static RestrictionFlags sRestrictionFlags[] = {'):]
table = table[:table.index('};')]
entries = [(name, int(a, 16), int(b, 16), int(c, 16)) for name, a, b, c in
           re.findall(r'\{\s*(\w+),\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+)\s*\}', table)]
check(len(entries) == 101 and entries[-1][0] == '0xFF', f'SoH\'s table has {len(entries)} rows, not 100 and the end')
FIELDS = {2: ('tradeItems', 'hookshot', 'ocarina', 'warpSongs'), 3: ('sunsSong', 'farores', 'dinsNayrus', 'all')}


def unpack(byte, field):
    return dict(zip(FIELDS[field], ((byte >> shift) & 3 for shift in (6, 4, 2, 0))))


# What each labelled group of the file must do to the entry's restrictions.
GROUPS = {'Warp songs': (2, 'warpSongs', 3), 'Farore\'s Wind': (3, 'farores', None), 'Ocarina': (2, 'ocarina', None)}
expected = []
for address, comment, value in writes:
    row, field = divmod(address - TABLE, 4)
    if not check(0 <= row < len(entries) - 1 and field in (2, 3) and comment in GROUPS,
                 f'{address:#x} ({comment}) is not flags2 or flags3 of a table entry'):
        continue
    name, *flags = entries[row]
    want_field, restriction, before = GROUPS[comment]
    old, new = unpack(flags[field - 1], field), unpack(value, field)
    check(field == want_field and old[restriction] != 0 and (before is None or old[restriction] == before) and
          new[restriction] == 0 and {k: v for k, v in old.items() if k != restriction} ==
          {k: v for k, v in new.items() if k != restriction},
          f'{address:#x} ({comment}) on {name} changes {old} to {new}, not only {restriction}')
    expected.append({'address': address, 'scene': scenes[name], 'field': field, 'value': value})
    print(f'{address:#x} {comment}: {name} flags{field} {flags[field - 1]:#04x} -> {value:#04x}')
# With OoTMM's table only the Windmill and Dampe's grave keep warpSongs 3, where both games refuse OoT's warp songs.
patched = {(e['scene'], e['field']): e['value'] for e in expected}
still = [name for name, _, flags2, _ in entries[:-1] if patched.get((scenes[name], 2), flags2) & 3 == 3]
check(still == ['SCENE_WINDMILL_AND_DAMPES_GRAVE'], f'warpSongs 3 is left in {still}')

# The port's copy (combo/rando/OotSceneRestrictions.h), as compiled.
dump = work/'dump_scene_restrictions.cpp'
dump.write_text(r'''
#include "rando/OotSceneRestrictions.h"
#include <cstdio>
int main() {
    std::printf("{\"table\": %u, \"bytes\": [", unsigned(ComboRando::kOotSceneRestrictionTableAddress));
    const char* comma = "";
    for (const auto& b : ComboRando::kOotmmSceneRestrictionBytes) {
        std::printf("%s{\"address\": %u, \"scene\": %u, \"field\": %u, \"value\": %u}", comma, unsigned(b.address),
                    unsigned(b.scene), unsigned(b.field), unsigned(b.value));
        comma = ", ";
    }
    std::printf("]}\n");
}
''', encoding='utf-8')
exe = hostcxx.build(args.compiler, dump, includes=[source/'combo', args.json_include],
                    exe=work/'dump_scene_restrictions.exe')
port = json.loads(subprocess.run([str(exe)], capture_output=True, text=True, check=True,
                                 timeout=hostcxx.RUN_TIMEOUT).stdout)
check(port['table'] == TABLE, 'the header names another table address')
check(port['bytes'] == expected, f'kOotmmSceneRestrictionBytes {port["bytes"]} is not OoTMM\'s {expected}')

# OoTMM's warp-song hook replaces only the song test; the refusal after it is vanilla's.
hooks = subprocess.run(['git', '-C', str(reference), 'grep', '-n', '-i', '-E', r'PATCH_START[[:space:]]+0x800df[12]',
                        registry.PIN, '--', 'packages/generator/src'], capture_output=True, text=True, timeout=60)
found = re.findall(r'PATCH_START\s+(0x[0-9a-fA-F]+)', hooks.stdout)
check([int(a, 16) for a in found] == [WARP_SONG_TEST], f'OoTMM patches near the warp-song refusal: {found}')
ocarina = [block for block in patches(blob('packages/generator/src/oot/ocarina.S')) if block[0] == WARP_SONG_TEST]
if check(len(ocarina) == 1, 'oot/ocarina.S has no single patch at 0x800df1bc'):
    end = WARP_SONG_TEST + 4 * len(ocarina[0][2])  # one MIPS instruction per line
    check(end <= REFUSAL[0], f'OoTMM\'s warp-song patch runs to {end:#x}, into the refusal at {REFUSAL[0]:#x}')

if failures:
    print('\n'.join('FAIL: ' + failure for failure in failures))
    sys.exit(1)
print(f'PASS the port\'s {len(expected)} scene restriction bytes are OoTMM v32.3\'s ({registry.PIN[:7]}), and OoTMM '
      'keeps the refusal of OoT\'s warp songs where warpSongs is 3')
