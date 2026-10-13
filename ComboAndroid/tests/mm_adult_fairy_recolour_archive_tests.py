"""Adult Link's Fairy Ocarina in MM, recoloured hand: the production texture swap on the user's own OoT archive.

Local only: needs the oot.o2r the app extracts from the user's OoT ROM (COMBO_TEST_OOT_O2R, or a
local-roms/test-output/**/ folder holding oot.o2r beside the checkout); reports SKIPPED without it. The archive is only
read, never copied: the lists go to this test's build directory.

Runs combo/menu/ComboFairyOcarinaRecolour.h on OoT's gLinkAdultRightHandHoldingOotNear/FarDL as
ComboFairyOcarinaHand.h does on the device, once as extracted and once with the addresses libultraship writes into the
first slots after a draw, and checks that exactly one slot changes: the hash of gOcarinaofTimeDesignTex becomes that
of gLinkChildFairyOcarinaTex; and that its device check (SameLoad) passes against OoT's child hand holding the Fairy
Ocarina (gLinkChildRightHandHoldingFairyOcarinaNear/FarDL), as extracted and after a draw. Then checks the facts the
swap relies on (tooling/claude-cleanup/t10): both textures are RGBA16 32x16 in 1024 bytes; in each adult list the
instrument's texture load and tile commands are byte for byte the ones OoT's child hand uses for the Fairy Ocarina;
and at the instrument's vertex load the combiner, render mode, texture LUT mode, texture scale, prim and env colours
are the same in both, and the geometry mode differs at most in culling.
"""
from pathlib import Path
import argparse
import os
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


def find_archive():
    if os.environ.get('COMBO_TEST_OOT_O2R'):
        return Path(os.environ['COMBO_TEST_OOT_O2R'])
    for parent in [root, *root.parents]:
        base = parent / 'local-roms/test-output'
        if base.is_dir():
            # The archive the other local-only tests use (beside an mm.o2r) first.
            found = sorted(base.glob('**/oot.o2r'))
            for oot in found:
                if (oot.parent / 'mm.o2r').is_file():
                    return oot
            if found:
                return found[0]
    return None


oot_path = find_archive()
if not oot_path or not oot_path.is_file():
    print('SKIPPED: the extracted oot.o2r is not on this machine (local-only test)')
    sys.exit(0)
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


TWO_SLOT = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}
OTEX, ODLT = 0x4F544558, 0x4F444C54


def crc64(text):
    crc = (1 << 64) - 1
    for byte in text.encode():
        crc ^= byte << 56
        for _ in range(8):
            crc = ((crc << 1) ^ 0x42F0E1EBA9EA3693 if crc >> 63 else crc << 1) & ((1 << 64) - 1)
    return crc


archive = zipfile.ZipFile(oot_path)
names = set(archive.namelist())
by_hash = {crc64(n): n for n in names}


def slots(name):
    """The display list as the port loads it: one (w0, w1) per slot, two for a 128-bit command."""
    d = archive.read(name)
    assert struct.unpack_from('<I', d, 4)[0] == ODLT, name
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


def commands(name):
    """(op, w0, w1, hash or None) per command."""
    s, i, out = slots(name), 0, []
    while i < len(s):
        w0, w1 = s[i]
        wide = (w0 >> 24) in TWO_SLOT
        out.append((w0 >> 24, w0, w1, (s[i + 1][0] << 32) | s[i + 1][1] if wide else None))
        i += 2 if wide else 1
    return out


hand_src = (root / 'combo/menu/ComboFairyOcarinaHand.h').read_text(encoding='utf-8')
ADULT, CHILD = 'objects/object_link_boy/', 'objects/object_link_child/'
LISTS = {
    'near': (ADULT + 'gLinkAdultRightHandHoldingOotNearDL', CHILD + 'gLinkChildRightHandHoldingFairyOcarinaNearDL'),
    'far': (ADULT + 'gLinkAdultRightHandHoldingOotFarDL', CHILD + 'gLinkChildRightHandHoldingFairyOcarinaFarDL'),
}
DESIGN = 'objects/gameplay_keep/gOcarinaofTimeDesignTex'
FAIRY = CHILD + 'gLinkChildFairyOcarinaTex'
for name in (LISTS['near'][0], LISTS['far'][0], DESIGN, FAIRY):
    check(f'"{name}"' in hand_src, f'the runtime does not use {name}')
    check(name in names, f'{name} is not in {oot_path}')
for adult, child in LISTS.values():
    check(child in names, f'{child} is not in {oot_path}')
if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)

# ---- Both textures: RGBA16 (Fast::TextureType 2), 32x16, 1024 bytes.
layouts = {}
for name in (DESIGN, FAIRY):
    d = archive.read(name)
    kind, version = struct.unpack_from('<II', d, 4)
    layouts[name] = (kind, version) + struct.unpack_from('<IIII', d, 0x40)
    check(layouts[name] == (OTEX, 0, 2, 32, 16, 1024) and len(d) >= 0x50 + 1024,
          f'{name} is not an RGBA16 32x16 texture of 1024 bytes: {layouts[name]}')

# ---- The production swap on each adult list, as extracted and as left after a draw.
inputs = work / 'fairy_recolour_inputs.txt'
with inputs.open('w', encoding='utf-8') as f:
    for lod, lists in LISTS.items():
        f.write(f'{lod}\n')
        for name in lists:
            s = slots(name)
            f.write(f'{len(s)}\n' + ' '.join(f'{w0} {w1}' for w0, w1 in s) + '\n')

HARNESS = r'''
#include "ComboFairyOcarinaRecolour.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace ComboFairyOcarinaRecolour;
struct Gfx { struct { uintptr_t w0, w1; } words; };

static bool Read(FILE* in, std::vector<Gfx>& list) {
    size_t count;
    if (std::fscanf(in, "%zu", &count) != 1)
        return false;
    list.resize(count);
    for (auto& g : list) {
        unsigned long long w0, w1;
        if (std::fscanf(in, "%llu %llu", &w0, &w1) != 2)
            return false;
        g.words.w0 = uintptr_t(w0);
        g.words.w1 = uintptr_t(w1);
    }
    return true;
}
// As libultraship leaves a drawn list: addresses in the first slots of its 128-bit commands.
static std::vector<Gfx> Drawn(std::vector<Gfx> list) {
    for (size_t i = 0; i + 1 < list.size(); ++i)
        if (Wide(uint8_t(list[i].words.w0 >> 24))) {
            list[i].words.w1 += 0x7F000000 + 16 * i;
            ++i;
        }
    return list;
}

int main(int argc, char** argv) {
    if (argc < 4)
        return 2;
    const uint64_t from = std::strtoull(argv[2], nullptr, 10), to = std::strtoull(argv[3], nullptr, 10);
    FILE* in = std::fopen(argv[1], "r");
    char lod[8];
    while (in && std::fscanf(in, "%7s", lod) == 1) {
        std::vector<Gfx> list, child;
        if (!Read(in, list) || !Read(in, child))
            return 2;
        for (int drawn = 0; drawn < 2; ++drawn) {
            std::vector<Gfx> source = drawn ? Drawn(list) : list, copy;
            const bool swapped = SwapTexture(source, copy, from, to);
            std::printf("%s%s %d %zu", lod, drawn ? "-drawn" : "", swapped ? 1 : 0, copy.size());
            for (size_t i = 0; i < copy.size() && i < source.size(); ++i)
                if (copy[i].words.w0 != source[i].words.w0 || copy[i].words.w1 != source[i].words.w1)
                    std::printf(" %zu:%llu:%llu", i, (unsigned long long)copy[i].words.w0,
                                (unsigned long long)copy[i].words.w1);
            std::printf("\n");
            // The device check against OoT's child hand, both as extracted or both drawn.
            std::printf("%s%s-load %d\n", lod, drawn ? "-drawn" : "",
                        SameLoad(source, from, drawn ? Drawn(child) : child, to) ? 1 : 0);
        }
    }
    return in ? 0 : 2;
}
'''
source = work / 'fairy_recolour_archive.cpp'
source.write_text(HARNESS, encoding='utf-8')
exe = hostcxx.build(a.compiler, source, std='c++17', includes=[root / 'combo/menu'], werror=True,
                    defines=['_CRT_SECURE_NO_WARNINGS'])
output = subprocess.run([str(exe), str(inputs), str(crc64(DESIGN)), str(crc64(FAIRY))], cwd=work, capture_output=True,
                        text=True, check=True, timeout=hostcxx.RUN_TIMEOUT).stdout.splitlines()
results = {line.split()[0]: line.split()[1:] for line in output}
swap_slots = {}
for lod, (adult, _) in LISTS.items():
    s = slots(adult)
    design_slots = [i + 1 for i, (w0, _) in enumerate(s[:-1]) if w0 >> 24 == 0x20 and
                    (s[i + 1][0] << 32 | s[i + 1][1]) == crc64(DESIGN)]
    for label in (lod, lod + '-drawn'):
        check(results.get(label + '-load') == ['1'],
              f'{adult} ({label}): SameLoad refuses it against OoT\'s child hand: {results.get(label + "-load")}')
        r = results.get(label)
        check(r is not None and r[0] == '1' and int(r[1]) == len(s), f'{adult} ({label}): the swap is refused: {r}')
        if not r or r[0] != '1':
            continue
        changed = [tuple(int(x) for x in item.split(':')) for item in r[2:]]
        check(len(changed) == 1 and len(design_slots) == 1 and changed[0][0] == design_slots[0],
              f'{adult} ({label}): the swap changes {changed}, not the one Ocarina of Time hash slot {design_slots}')
        if len(changed) == 1:
            index, w0, w1 = changed[0]
            check(by_hash.get(w0 << 32 | w1) == FAIRY and s[index - 1][0] == 0x20100000,
                  f'{adult} ({label}): the copy sets {by_hash.get(w0 << 32 | w1)} with {s[index - 1][0]:08X}')
            swap_slots[lod] = index


# ---- The material the swap keeps: the same as OoT's own child Fairy Ocarina.
def material(name, texture):
    """The commands that load and tile `texture` (its G_SETTIMG_OTR_HASH through the next G_SETTILESIZE), and the
    render state when the first vertex load after it runs."""
    cmds = commands(name)
    at = [i for i, (op, _, _, h) in enumerate(cmds) if op == 0x20 and h == crc64(texture)]
    if len(at) != 1:
        return None, None
    end = next(i for i in range(at[0], len(cmds)) if cmds[i][0] == 0xF2)
    load = [(op, w0, w1) for op, w0, w1, _ in cmds[at[0] + 1:end + 1]]
    state, geometry = {}, 0
    vtx = next(i for i in range(at[0], len(cmds)) if cmds[i][0] == 0x32)
    for op, w0, w1, _ in cmds[:vtx]:
        if op in (0xFC, 0xD7, 0xFA, 0xFB):
            state[op] = (w0, w1)
        elif op in (0xE2, 0xE3):  # othermode L / H: keyed by the bits they set
            state[(op, w0 & 0xFFFF)] = w1
        elif op == 0xD9:
            geometry = (geometry & (w0 & 0xFFFFFF)) | w1
    state['first'] = cmds[at[0]][1]
    return load, (state, geometry)


CULL = 0x00000600  # G_CULL_FRONT | G_CULL_BACK (F3DEX2)
for lod, (adult, child) in LISTS.items():
    adult_load, adult_state = material(adult, DESIGN)
    child_load, child_state = material(child, FAIRY)
    check(adult_load is not None and child_load is not None,
          f'{lod}: one list does not set its instrument texture exactly once')
    if adult_load is None or child_load is None:
        continue
    check(adult_load == child_load and len(adult_load) == 6,
          f'{lod}: the adult instrument loads its texture differently from the child Fairy Ocarina:\n'
          f'  {adult_load}\n  {child_load}')
    check(adult_state[0] == child_state[0],
          f'{lod}: the colour state at the instrument differs:\n  {adult_state[0]}\n  {child_state[0]}')
    check(adult_state[1] & ~CULL == child_state[1] & ~CULL,
          f'{lod}: the geometry mode differs beyond culling: {adult_state[1]:08X} / {child_state[1]:08X}')

if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)
print(f'Textures: {DESIGN} and {FAIRY} both RGBA16 32x16, 1024 bytes')
print(f'Swap (production code): near list slot {swap_slots["near"]}, far list slot {swap_slots["far"]}; only that hash '
      'slot changes, as extracted and after a draw; SameLoad accepts each against OoT\'s child hand')
print('Material: texture load and tiles byte for byte the child Fairy Ocarina\'s; combiner, render mode, texture LUT '
      'mode, texture scale, prim and env the same; geometry mode the same apart from culling')
print(f'PASS: archive {oot_path}')
