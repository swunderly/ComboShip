"""Adult Link in MM with OoT's own adult model (plan F5b): checks against the user's own extracted archives.

Local only: needs the oot.o2r and mm.o2r the app extracts from the user's ROMs (COMBO_TEST_OOT_O2R / COMBO_TEST_MM_O2R,
or a local-roms/test-output/**/ folder holding both beside the checkout); reports SKIPPED without them. The archives
are only read, never copied.
H8: OoT's gLinkAdultSkel is Flex/LOD with 21 limbs and 18 list limbs, joint for joint and limb for limb MM's Fierce
    Deity skeleton; every name the runtime asks for is in the archives with its type; OoT's adult lists use only the
    segments MM sets (08, 09, 0C, 0D); MM's "hand holding X" lists are [hand, the items the composites use];
    object_horse's gEponaSkel has 47 Skin limbs; the production walker finds a pipe sync or gsSPTexture at every mask
    patch and the pipe sync after the marker in gSpinAttackChargingDL.
H7: MM's spin vertices scaled by the production ScaleSpinCoord are within 1 unit of OoTMM's adult vertex files
    (needs the OoTMM v32.3 checkout as well).
"""
from pathlib import Path
import argparse
import os
import re
import struct
import subprocess
import sys
import zipfile

import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()


def find_archives():
    if os.environ.get('COMBO_TEST_OOT_O2R') and os.environ.get('COMBO_TEST_MM_O2R'):
        return Path(os.environ['COMBO_TEST_OOT_O2R']), Path(os.environ['COMBO_TEST_MM_O2R'])
    for parent in [root, *root.parents]:
        base = parent / 'local-roms/test-output'
        if base.is_dir():
            for oot in sorted(base.glob('**/oot.o2r')):
                if (oot.parent / 'mm.o2r').is_file():
                    return oot, oot.parent / 'mm.o2r'
    return None, None


oot_path, mm_path = find_archives()
if not oot_path or not oot_path.is_file() or not mm_path.is_file():
    print('SKIPPED: the extracted oot.o2r and mm.o2r are not on this machine (local-only test)')
    sys.exit(0)
reference = next((q / 'references/ootmm-stable-32.3' for q in [root, *root.parents]
                  if (q / 'references/ootmm-stable-32.3/packages/generator/data/static').is_dir()), None)
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


TWO_SLOT = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}
TYPES = {'DL': 0x4F444C54, 'SKEL': 0x4F534B4C, 'LIMB': 0x4F534C42, 'TEX': 0x4F544558, 'ARRAY': 0x4F415252}


class Archive:
    def __init__(self, path):
        self.zip = zipfile.ZipFile(path)
        self.names = set(self.zip.namelist())

    def data(self, name):
        return self.zip.read(name)

    def type(self, name):
        return struct.unpack_from('<I', self.data(name), 4)[0] if name in self.names else None

    def slots(self, name):
        """The display list as the port loads it: one (w0, w1) per slot, two for a 128-bit command."""
        d = self.data(name)
        off = 0x41
        while off % 8:
            off += 1
        out = []
        while True:
            w0, w1 = struct.unpack_from('<II', d, off)
            off += 8
            out.append((w0, w1))
            if (w0 >> 24) in TWO_SLOT:
                out.append(struct.unpack_from('<II', d, off))
                off += 8
            if w0 >> 24 == 0xDF:
                return out

    def commands(self, name):
        slots, i, out = self.slots(name), 0, []
        while i < len(slots):
            op = slots[i][0] >> 24
            out.append(slots[i] + (slots[i + 1] if op in TWO_SLOT else ()))
            i += 2 if op in TWO_SLOT else 1
        return out

    def string(self, d, off):
        n = struct.unpack_from('<I', d, off)[0]
        return d[off + 4:off + 4 + n].decode(), off + 4 + n

    def skeleton(self, name):
        d = self.data(name)
        kind, limb_type = d[0x40], d[0x41]
        limbs, lists = struct.unpack_from('<II', d, 0x42)
        count = struct.unpack_from('<I', d, 0x4B)[0]
        off, table = 0x4F, []
        for _ in range(count):
            s, off = self.string(d, off)
            table.append(s)
        return kind, limb_type, limbs, lists, table

    def limb(self, name):
        d = self.data(name)
        limb_type, off = d[0x40], 0x42
        _, off = self.string(d, off)
        off += 2
        assert struct.unpack_from('<I', d, off)[0] == 0, name
        off += 4
        _, off = self.string(d, off)
        off += 18
        for _ in range(2):
            _, off = self.string(d, off)
        near, off = self.string(d, off)
        far, off = self.string(d, off)
        joint = struct.unpack_from('<hhh', d, off)
        return limb_type, joint, d[off + 6], d[off + 7], near, far

    def vertices(self, name):
        d = self.data(name)
        kind, count = struct.unpack_from('<II', d, 0x40)
        return kind, [struct.unpack_from('<hhhHhhBBBB', d, 0x48 + 16 * i) for i in range(count)]


def crc64(text):
    crc = (1 << 64) - 1
    for byte in text.encode():
        crc ^= byte << 56
        for _ in range(8):
            crc = ((crc << 1) ^ 0x42F0E1EBA9EA3693 if crc >> 63 else crc << 1) & ((1 << 64) - 1)
    return crc


oot, mm = Archive(oot_path), Archive(mm_path)
loader = (root / 'mm/2s2h/ComboAdultModelResources.h').read_text(encoding='utf-8')
keep_src = (root / 'mm/2s2h/ComboAdultGameplayKeepResources.h').read_text(encoding='utf-8')
mask_src = (root / 'mm/2s2h/ComboAdultMaskScale.h').read_text(encoding='utf-8')


def strings(source, name):
    return re.findall(r'"([^"]+)"', re.search(re.escape(name) + r'\[[^\]]*\] = \{(.*?)\};', source, re.S).group(1))


# ---- H8: OoT's adult skeleton, limb for limb MM's Fierce Deity skeleton.
kind, limb_type, limbs, lists, table = oot.skeleton('objects/object_link_boy/gLinkAdultSkel')
check((kind, limb_type, limbs, lists, len(table)) == (1, 2, 21, 18, 21), 'gLinkAdultSkel is not Flex/LOD 21/18')
fd = mm.skeleton('objects/object_link_boy/gLinkFierceDeitySkel')
check(fd[:5] == (1, 2, 21, 18, fd[4]) and len(fd[4]) == 21, 'gLinkFierceDeitySkel is not Flex/LOD 21/18')
limb_lists = []
for adult, deity in zip(table, fd[4]):
    a_limb, d_limb = oot.limb(adult), mm.limb(deity)
    check(a_limb[0] == 2 and a_limb[1:4] == d_limb[1:4], f'{adult} differs from {deity}: {a_limb[1:4]} {d_limb[1:4]}')
    check(bool(a_limb[4]) == bool(a_limb[5]), f'{adult} has one LOD list')
    limb_lists += [x for x in a_limb[4:] if x]
check(len(limb_lists) == 36 and all(x.startswith('objects/object_link_boy/gLinkAdult') for x in limb_lists),
      'adult limb lists')

# Every name the runtime asks for, with its type.
names = {name: 'DL' for name in strings(loader, 'kAdultBodyDLs') + limb_lists}
names.update({name: 'TEX' for name in strings(loader, 'kAdultFaceTextures')})
names['objects/object_link_boy/gLinkAdultSkel'] = 'SKEL'
for name, kind in names.items():
    check(oot.type(name) == TYPES[kind], f'{name} is not an OoT {kind}')
mm_names = {name: 'DL' for name in strings(loader, 'kAdultMmItemDLs')}
mm_names['objects/object_link_boy/gLinkFierceDeitySkel'] = 'SKEL'
mm_names.update({name: 'ARRAY' for name in strings(keep_src, 'kSpinVertexArrays')})
mm_names['objects/gameplay_keep/gSpinAttackChargingDL'] = 'DL'
mask_lists = sorted(set(re.findall(r'"__OTR__(objects/\w+/\w+)"', mask_src)))
mm_names.update({name: 'DL' for name in mask_lists})
for name, kind in mm_names.items():
    check(mm.type(name) == TYPES[kind], f'{name} is not an MM {kind}')

# OoT's adult lists use only the segments MM's player draw sets: 08/09 (eyes, mouth), 0C (cull list), 0D (matrices).
segments = set()
for name in [n for n, k in names.items() if k == 'DL']:
    for command in oot.commands(name):
        op, w1 = command[0] >> 24, command[1]
        if op in (0xDE, 0xDA, 0xFD, 0x01, 0xDC, 0xDB) and w1 >> 24 and not w1 & 0xF0000000:
            segments.add(w1 >> 24)
check(segments == {0x08, 0x09, 0x0C, 0x0D}, f'OoT adult lists use segments {sorted(map(hex, segments))}')
head = oot.commands('objects/object_link_boy/gLinkAdultHeadNearDL')
check({c[1] >> 24 for c in head if c[0] >> 24 == 0xFD and c[1] >> 24 in (8, 9)} == {8, 9},
      'the adult head does not read the eye and mouth segments')

# MM's own "hand holding X" lists are [marker, hand, item(s), end]; the composites take the same items.
by_hash = {crc64(n): n for n in mm.names}
items = strings(loader, 'kAdultMmItemDLs')
for holding, want in (('gLinkHumanLeftHandHoldingKokiriSwordDL', [0]), ('gLinkHumanLeftHandHoldingRazorSwordDL', [1]),
                      ('gLinkHumanLeftHandHoldingGildedSwordDL', [2, 3]),
                      ('gLinkHumanLeftHandHoldingGreatFairysSwordDL', [4]),
                      ('gLinkHumanRightHandHoldingHerosShieldDL', [5]), ('gLinkHumanRightHandHoldingMirrorShieldDL', [6]),
                      ('gLinkHumanRightHandHoldingBowDL', [7]), ('gLinkHumanRightHandHoldingHookshotDL', [8])):
    commands = mm.commands('objects/object_link_child/' + holding)
    ops = [c[0] >> 24 for c in commands]
    calls = [by_hash.get((c[2] << 32) | c[3]) for c in commands if c[0] >> 24 == 0x31]
    check(ops == [0x33] + [0x31] * (len(want) + 1) + [0xDF] and calls[1:] == [items[i] for i in want] and
          calls[0] in ('objects/object_link_child/gLinkHumanLeftHandClosedDL',
                       'objects/object_link_child/gLinkHumanRightHandClosedDL'),
          f'{holding} is not [closed hand, {[items[i] for i in want]}]: {calls}')

# OoTMM's adult Epona is OoT's own object_horse (plan F5b 1.7).
horse = oot.skeleton('objects/object_horse/gEponaSkel')
check(horse[0] == 0 and horse[1] == 3 and horse[2] == 47, f'gEponaSkel is not Normal/Skin with 47 limbs: {horse[:3]}')

# Production walker on the user's lists: the mask patch commands and the spin-charge pipe sync.
walks = [('objects/gameplay_keep/gSpinAttackChargingDL', 0, 0xE7)]
for mask, name, command, sync in re.findall(r'\{\s*PLAYER_MASK_(\w+),\s*"__OTR__(objects/\w+/\w+)",\s*(\d+),\s*(true|false),',
                                            mask_src):
    walks.append((name, int(command), 0xE7 if sync == 'true' else 0xD7))
check(len(walks) == 8, 'mask patches')
charging = mm.slots('objects/gameplay_keep/gSpinAttackChargingDL')
check(charging[0][0] >> 24 == 0x33 and charging[2][0] >> 24 == 0xE7,
      'gSpinAttackChargingDL does not start with a two-slot marker, then a pipe sync')
inputs = work / 'walks.txt'
with inputs.open('w', encoding='utf-8') as f:
    for name, command, _ in walks:
        slots = mm.slots(name)
        f.write(f'{command} {len(slots)} ' + ' '.join(str(w0) for w0, _ in slots) + '\n')

# ---- H7 inputs: MM's spin vertices.
spin_names = strings(keep_src, 'kSpinVertexArrays')
spin = [mm.vertices(n) for n in spin_names]
check([len(v) for _, v in spin] == [26, 26, 26, 26, 14] and all(k == 25 for k, _ in spin), 'spin vertex arrays')
with (work / 'spin.txt').open('w', encoding='utf-8') as f:
    for index, (_, vertices) in enumerate(spin):
        for vertex in vertices:
            f.write(f'{index} {vertex[0]} {vertex[1]} {vertex[2]}\n')

HARNESS = r'''
#include "ComboAdultOotModel.h"
#include <cstdio>
#include <vector>
using namespace ComboAdultOotModel;
struct Gfx { struct { uintptr_t w0, w1; } words; };
int main(int argc, char** argv) {
    if (argc < 3) return 2;
    FILE* walks = std::fopen(argv[1], "r");
    size_t command, count;
    while (std::fscanf(walks, "%zu %zu", &command, &count) == 2) {
        std::vector<Gfx> list(count);
        for (auto& g : list) {
            unsigned long long w0;
            if (std::fscanf(walks, "%llu", &w0) != 1) return 2;
            g.words.w0 = uintptr_t(w0);
            g.words.w1 = 0;
        }
        const long slot = N64CommandSlot(list.data(), list.size(), command);
        std::printf("walk %ld %u\n", slot, slot < 0 ? 0u : unsigned(list[size_t(slot)].words.w0 >> 24));
    }
    FILE* spin = std::fopen(argv[2], "r");
    int index, x, y, z;
    while (std::fscanf(spin, "%d %d %d %d", &index, &x, &y, &z) == 4)
        std::printf("spin %d %d %d\n", ScaleSpinCoord(int16_t(x), kSpinScale[index]),
                    ScaleSpinCoord(int16_t(y), kSpinScale[index]), ScaleSpinCoord(int16_t(z), kSpinScale[index]));
}
'''
source = work / 'cross_age_archive.cpp'
source.write_text(HARNESS, encoding='utf-8')
exe = hostcxx.build(a.compiler, source, std='c++17', includes=[root / 'mm/2s2h'], werror=True,
                    defines=['_CRT_SECURE_NO_WARNINGS'])
output = subprocess.run([str(exe), str(inputs), str(work / 'spin.txt')], cwd=work, capture_output=True, text=True,
                        check=True, timeout=hostcxx.RUN_TIMEOUT).stdout.split('\n')
walked = [tuple(map(int, line.split()[1:])) for line in output if line.startswith('walk ')]
for (name, command, want), (slot, op) in zip(walks, walked):
    check(slot >= 0 and op == want, f'{name} N64 command {command}: slot {slot} holds {op:#x}, not {want:#x}')
check(len(walked) == len(walks), 'walker output')
check(walked[0][0] == 2, 'the spin-charge pipe sync is not the slot after the two-slot marker')

# ---- H7: within 1 unit of OoTMM's adult spin vertex files, other fields unchanged.
scaled = [tuple(map(int, line.split()[1:])) for line in output if line.startswith('spin ')]
if reference:
    static = reference / 'packages/generator/data/static'
    upstream = []
    for i in (1, 2, 3):
        d = (static / f'mm_adult_link_spin_attack_vtx_{i}.bin').read_bytes()
        upstream += [struct.unpack_from('>hhhHhhBBBB', d, o) for o in range(0, len(d), 16)]
    native = [v for _, vertices in spin for v in vertices]
    check(len(upstream) == len(native) == len(scaled) == 118, 'spin vertex counts')
    exact = near = 0
    for mine, theirs, base in zip(scaled, upstream, native):
        for k in range(3):
            exact += mine[k] == theirs[k]
            near += abs(mine[k] - theirs[k]) == 1
            check(abs(mine[k] - theirs[k]) <= 1, f'spin vertex {base[:3]} -> {mine} vs OoTMM {theirs[:3]}')
        check(theirs[3:] == base[3:], f'OoTMM changed more than the position of {base}')
    print(f'H7: {exact} of 354 coordinates exact, {near} within 1')
else:
    print('H7 not run: the OoTMM v32.3 checkout is not beside this worktree')

if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)
print(f'PASS: archives {oot_path.parent}; {len(names)} OoT and {len(mm_names)} MM names, segments, composites, '
      f'Epona skeleton, {len(walks)} walked commands')
