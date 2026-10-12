"""Adult Link's Fairy Ocarina in MM, option B3: the production fit on the user's own extracted archives.

Local only: needs the oot.o2r and mm.o2r the app extracts from the user's ROMs (COMBO_TEST_OOT_O2R / COMBO_TEST_MM_O2R,
or a local-roms/test-output/**/ folder holding both beside the checkout); reports SKIPPED without them. The archives
are only read, never copied: the lists and vertex arrays go to this test's build directory.

Runs combo/menu/ComboFairyOcarinaFit.h on OoT's four near hand lists, as ComboFairyOcarinaHand.h does on the device,
once with the vertex offsets the archive holds and once with the addresses libultraship writes back after drawing, and
on the four far lists. Checks that the fit is accepted within sqrt(3), that both forms give the same fit, and that the
near fit also maps the far child instrument onto the far adult one (the composite draws the near Fairy item on both
LODs). Then reports where the instrument sits in t6 face units (tooling/claude-cleanup/t6/kinematics.md sections 5 and
6: OoT's adult skeleton posed with MM's link_normal_okarina_swing frame 0, measured from the mouth line of
gLinkAdultHeadNearDL, 1 = the eye line) for OoT's own adult Ocarina of Time, today's unmoved Fairy item and the fitted
one, and checks that the fitted item is nearer the mouth than today's.
"""
from pathlib import Path
import argparse
import math
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
work.mkdir(parents=True, exist_ok=True)
failures = []


def check(condition, message):
    if not condition:
        failures.append(message)


TWO_SLOT = {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}


def crc64(text):
    crc = (1 << 64) - 1
    for byte in text.encode():
        crc ^= byte << 56
        for _ in range(8):
            crc = ((crc << 1) ^ 0x42F0E1EBA9EA3693 if crc >> 63 else crc << 1) & ((1 << 64) - 1)
    return crc


class Archive:
    def __init__(self, path):
        self.zip = zipfile.ZipFile(path)
        self.names = set(self.zip.namelist())
        self.by_hash = {crc64(n): n for n in self.names}

    def data(self, name):
        return self.zip.read(name[7:] if name.startswith('__OTR__') else name)

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
        """(op, w0, w1, hash or None) per command."""
        slots, i, out = self.slots(name), 0, []
        while i < len(slots):
            w0, w1 = slots[i]
            wide = (w0 >> 24) in TWO_SLOT
            out.append((w0 >> 24, w0, w1, (slots[i + 1][0] << 32) | slots[i + 1][1] if wide else None))
            i += 2 if wide else 1
        return out

    def vertex_bytes(self, name):
        d = self.data(name)
        kind, count = struct.unpack_from('<II', d, 0x40)
        assert kind == 0x19, name
        return d[0x48:0x48 + 16 * count]

    def vertices(self, name):
        raw = self.vertex_bytes(name)
        return [struct.unpack_from('<hhhHhhBBBB', raw, o) for o in range(0, len(raw), 16)]

    def string(self, d, off):
        n = struct.unpack_from('<I', d, off)[0]
        return d[off + 4:off + 4 + n].decode(), off + 4 + n

    def limbs(self, skeleton):
        d = self.data(skeleton)
        count = struct.unpack_from('<I', d, 0x4B)[0]
        off, out = 0x4F, []
        for _ in range(count):
            name, off = self.string(d, off)
            l = self.data(name)
            o = 0x42
            _, o = self.string(l, o)
            o += 2
            assert struct.unpack_from('<I', l, o)[0] == 0, name
            o += 4
            _, o = self.string(l, o)
            o += 18
            for _ in range(2):
                _, o = self.string(l, o)
            near, o = self.string(l, o)
            _, o = self.string(l, o)
            joint = struct.unpack_from('<hhh', l, o)
            out.append(dict(joint=joint, child=l[o + 6], sibling=l[o + 7], dl=near or None))
        return out

    def link_frame(self, animation, frame):
        d = self.data(animation)
        assert struct.unpack_from('<I', d, 0x40)[0] == 1, animation
        path, _ = self.string(d, 0x46)
        data = self.data(path)
        n = struct.unpack_from('<I', data, 0x40)[0]
        values = struct.unpack_from('<%dh' % n, data, 0x44)
        s = values[67 * frame:67 * frame + 67]
        return [s[3 * i:3 * i + 3] for i in range(22)]


oot, mm = Archive(oot_path), Archive(mm_path)
CHILD, ADULT = 'objects/object_link_child/', 'objects/object_link_boy/'
LISTS = {
    'near': [CHILD + 'gLinkChildRightHandAndOotNearDL', CHILD + 'gLinkChildRightHandHoldingFairyOcarinaNearDL',
             ADULT + 'gLinkAdultRightHandHoldingOotNearDL', ADULT + 'gLinkAdultRightHandNearDL'],
    'far': [CHILD + 'gLinkChildRightHandHoldingOOTFarDL', CHILD + 'gLinkChildRightHandHoldingFairyOcarinaFarDL',
            ADULT + 'gLinkAdultRightHandHoldingOotFarDL', ADULT + 'gLinkAdultRightHandFarDL'],
}
hand_src = (root / 'combo/menu/ComboFairyOcarinaHand.h').read_text(encoding='utf-8')
check(all(f'"{name}"' in hand_src for name in LISTS['near']), 'the runtime does not fit the near lists checked here')
TEXTURE = 'objects/gameplay_keep/gOcarinaofTimeDesignTex'
check(f'"{TEXTURE}"' in hand_src, 'the runtime does not mark the instrument with the Ocarina of Time texture')
INSTRUMENT = crc64(TEXTURE)

# ---- Inputs for the production fit: every list's slots and the vertex arrays they load.
inputs = work / 'fairy_fit_inputs.txt'
arrays = {}
with inputs.open('w', encoding='utf-8') as f:
    for lod, names in LISTS.items():
        for index, name in enumerate(names):
            slots = oot.slots(name)
            f.write(f'list {lod} {index} {len(slots)}\n' + ' '.join(f'{w0} {w1}' for w0, w1 in slots) + '\n')
            for op, _, _, h in oot.commands(name):
                if op == 0x32:
                    arrays[h] = oot.by_hash[h]
    for h, name in arrays.items():
        raw = oot.vertex_bytes(name)
        f.write(f'array {h} {len(raw)}\n' + ' '.join(map(str, raw)) + '\n')

HARNESS = r'''
#include "ComboFairyOcarinaFit.h"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
using namespace ComboFairyOcarinaFit;
struct Gfx { struct { uintptr_t w0, w1; } words; };
static std::map<std::string, std::vector<Gfx>> lists;
static std::map<uint64_t, std::vector<uint8_t>> arrays;

static bool Loads(std::vector<Gfx> list, bool addresses, std::vector<VertexLoad>& loads) {
    if (addresses) // as libultraship leaves a list it has drawn: each load's offset replaced by its vertices' address
        for (size_t i = 0; i + 1 < list.size(); ++i)
            if (list[i].words.w0 >> 24 == kOpVtxHash) {
                const uint64_t hash = (uint64_t(list[i + 1].words.w0) << 32) + list[i + 1].words.w1;
                list[i].words.w1 += uintptr_t(arrays.at(hash).data());
                ++i;
            }
    if (!ListVertexLoads(list.data(), list.size(), loads))
        return false;
    for (auto& load : loads) {
        auto it = arrays.find(load.array);
        if (it == arrays.end() || !ReadPoints(it->second.data(), it->second.size(), load))
            return false;
    }
    return true;
}

static void Fit(const char* lod, bool addresses, uint64_t instrument) {
    std::vector<VertexLoad> l[4];
    for (int i = 0; i < 4; ++i)
        if (!Loads(lists[std::string(lod) + " " + std::to_string(i)], addresses, l[i])) {
            std::printf("fit %s%s unreadable %d\n", lod, addresses ? "-addresses" : "", i);
            return;
        }
    RigidFit fit;
    const FitResult result = FitFairyPlacement(l[0], l[1], l[2], l[3], instrument, fit);
    std::printf("fit %s%s %s %.17g %.17g %.17g", lod, addresses ? "-addresses" : "",
                result == FitResult::Fitted ? "fitted" : "refused", RotationDegrees(fit), fit.rms, fit.max);
    for (auto& row : fit.rotation)
        for (double v : row)
            std::printf(" %.17g", v);
    for (double v : fit.translation)
        std::printf(" %.17g", v);
    std::printf(" | %s\n", FitResultName(result));
}

int main(int argc, char** argv) {
    if (argc < 3)
        return 2;
    FILE* in = std::fopen(argv[1], "r");
    char kind[16];
    while (in && std::fscanf(in, "%15s", kind) == 1) {
        if (std::string(kind) == "list") {
            char lod[8];
            int index;
            size_t count;
            if (std::fscanf(in, "%7s %d %zu", lod, &index, &count) != 3)
                return 2;
            std::vector<Gfx> list(count);
            for (auto& g : list) {
                unsigned long long w0, w1;
                if (std::fscanf(in, "%llu %llu", &w0, &w1) != 2)
                    return 2;
                g.words.w0 = uintptr_t(w0);
                g.words.w1 = uintptr_t(w1);
            }
            lists[std::string(lod) + " " + std::to_string(index)] = list;
        } else {
            unsigned long long hash;
            size_t count;
            if (std::fscanf(in, "%llu %zu", &hash, &count) != 2)
                return 2;
            std::vector<uint8_t>& bytes = arrays[hash];
            bytes.resize(count);
            for (auto& b : bytes) {
                unsigned v;
                if (std::fscanf(in, "%u", &v) != 1)
                    return 2;
                b = uint8_t(v);
            }
        }
    }
    const uint64_t instrument = std::strtoull(argv[2], nullptr, 10);
    Fit("near", false, instrument);
    Fit("near", true, instrument);
    Fit("far", false, instrument);
    std::printf("bound %.17g\n", RoundingBound());
}
'''
source = work / 'fairy_fit_archive.cpp'
source.write_text(HARNESS, encoding='utf-8')
exe = hostcxx.build(a.compiler, source, std='c++17', includes=[root / 'combo/menu'], werror=True,
                    defines=['_CRT_SECURE_NO_WARNINGS'])
output = subprocess.run([str(exe), str(inputs), str(INSTRUMENT)], cwd=work, capture_output=True, text=True, check=True,
                        timeout=hostcxx.RUN_TIMEOUT).stdout.splitlines()
fits = {}
for line in output:
    if line.startswith('fit '):
        head, _, reason = line.partition(' | ')
        parts = head.split()
        fits[parts[1]] = dict(result=parts[2], reason=reason, angle=float(parts[3]) if len(parts) > 3 else None,
                              rms=float(parts[4]) if len(parts) > 4 else None,
                              max=float(parts[5]) if len(parts) > 5 else None,
                              r=[list(map(float, parts[6 + 3 * i:9 + 3 * i])) for i in range(3)] if len(parts) > 6 else None,
                              t=list(map(float, parts[15:18])) if len(parts) > 6 else None)
bound = math.sqrt(3)
near = fits.get('near')
check(near and near['result'] == 'fitted' and near['max'] <= bound,
      f'the near lists are not fitted: {near and (near["result"], near["reason"], near["max"])}')
check(fits.get('near-addresses') and {k: v for k, v in fits['near-addresses'].items()} == near,
      'the fit differs once libultraship has written the vertex addresses back')
check(fits.get('far') and fits['far']['result'] == 'fitted', f'the far lists are not fitted: {fits.get("far")}')
if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)


def apply(fit, point):
    return tuple(sum(fit['r'][j][k] * point[k] for k in range(3)) + fit['t'][j] for j in range(3))


def loads(name):
    """[(material path, positions)] per vertex load, in load order."""
    material, out = None, []
    for op, w0, w1, h in oot.commands(name):
        if op == 0x20:
            material = oot.by_hash.get(h)
        elif op == 0x32:
            n = (w0 >> 12) & 0xFF
            out.append((material, [v[:3] for v in oot.vertices(oot.by_hash[h])[w1 // 16:w1 // 16 + n]]))
    return out


def instrument(name):
    return [pt for m, pts in loads(name) if m == TEXTURE for pt in pts]


# The near fit maps the far child instrument onto the far adult one: the composite uses it on both LODs.
far_child, far_adult = instrument(LISTS['far'][0]), instrument(LISTS['far'][2])
far_error = max(math.dist(apply(near, p), q) for p, q in zip(far_child, far_adult))
check(len(far_child) == len(far_adult) > 0 and far_error <= bound,
      f'the near fit moves the far child instrument {far_error:.2f} from the far adult one')

# ---- Face units (t6 kinematics.md section 5): OoT's adult skeleton posed with MM's okarina_swing frame 0.
limbs = oot.limbs(ADULT + 'gLinkAdultSkel')
joints = mm.link_frame('objects/gameplay_keep/gPlayerAnim_link_normal_okarina_swing', 0)


def mul(x, y):
    return [[sum(x[i][k] * y[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def trs(pos, rot):
    """Matrix_TranslateRotateZYX: T(pos) Rz Ry Rx, column vectors, binary angles."""
    rx, ry, rz = (r * 2 * math.pi / 65536 for r in rot)
    cx, sx, cy, sy, cz, sz = math.cos(rx), math.sin(rx), math.cos(ry), math.sin(ry), math.cos(rz), math.sin(rz)
    m = [[1, 0, 0, pos[0]], [0, 1, 0, pos[1]], [0, 0, 1, pos[2]], [0, 0, 0, 1]]
    for r in ([[cz, -sz, 0, 0], [sz, cz, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]],
              [[cy, 0, sy, 0], [0, 1, 0, 0], [-sy, 0, cy, 0], [0, 0, 0, 1]],
              [[1, 0, 0, 0], [0, cx, -sx, 0], [0, sx, cx, 0], [0, 0, 0, 1]]):
        m = mul(m, r)
    return m


def pose():
    """SkelAnime_DrawFlexLod: the root takes the animation's translation; limb i takes jointPos and jointTable[i + 1]."""
    mats = [None] * len(limbs)

    def draw(i, parent):
        mats[i] = mul(parent, trs(joints[0] if i == 0 else limbs[i]['joint'], joints[i + 1]))
        if limbs[i]['child'] != 0xFF:
            draw(limbs[i]['child'], mats[i])
        if limbs[i]['sibling'] != 0xFF:
            draw(limbs[i]['sibling'], parent)

    draw(0, [[1 if i == j else 0 for j in range(4)] for i in range(4)])
    return mats


def head_landmarks():
    """Eye line and mouth: where the centre row of the eye (segment 8) and mouth (segment 9) textures meets the face
    midline (|z| <= 100), on the head's own vertices (kinematics.md section 5)."""
    order = []

    def visit(i):
        if limbs[i]['dl']:
            order.append(i)
        if limbs[i]['child'] != 0xFF:
            visit(limbs[i]['child'])
        if limbs[i]['sibling'] != 0xFF:
            visit(limbs[i]['sibling'])

    visit(0)
    own = order.index(10)
    tris, buf, state = [], [None] * 64, dict(mtx=None, tex=None, pending=None, tile=None)

    def run(name):
        for op, w0, w1, h in oot.commands(name):
            if op == 0x32:
                n, v0 = (w0 >> 12) & 0xFF, ((w0 >> 1) & 0x7F) - ((w0 >> 12) & 0xFF)
                vs = oot.vertices(oot.by_hash[h])
                for k in range(n):
                    buf[v0 + k] = (vs[w1 // 16 + k][:3], state['mtx'], vs[w1 // 16 + k][5])
            elif op in (0x05, 0x06, 0x07):
                groups = [(w0 >> 16) & 0xFF, (w0 >> 8) & 0xFF, w0 & 0xFF]
                groups = [groups] + ([[(w1 >> 16) & 0xFF, (w1 >> 8) & 0xFF, w1 & 0xFF]] if op != 0x05 else [])
                for g in groups:
                    tris.append((state['tex'], [buf[i // 2] for i in g], state['tile']))
            elif op == 0xDA and (w1 >> 24) == 0x0D:
                state['mtx'] = ((w1 & 0xFFFFFF) & ~0x3F) // 0x40
            elif op == 0xD8:
                state['mtx'] = None
            elif op == 0x20:
                state['pending'] = oot.by_hash.get(h)
            elif op == 0xFD:
                state['pending'] = 'seg%02X' % (w1 >> 24)
            elif op in (0xF3, 0xF4):
                state['tex'] = state['pending']
            elif op == 0xF2:
                state['tile'] = (((w1 >> 12) & 0xFFF) // 4 + 1, (w1 & 0xFFF) // 4 + 1)
            elif op == 0x31:
                run(oot.by_hash[h])

    run(ADULT + 'gLinkAdultHeadNearDL')

    def landmark(tex):
        points, tile = {}, None
        for t, vs, tl in tris:
            if t != tex or None in vs:
                continue
            tile = tl
            for pt, m, st in vs:
                if (m is None or m == own) and abs(pt[2]) <= 100:
                    points[pt] = st / 32.0
        ordered = sorted(points.items(), key=lambda kv: kv[1])
        for (p0, t0), (p1, t1) in zip(ordered, ordered[1:]):
            if t0 <= tile[1] / 2 <= t1 and t1 > t0:
                f = (tile[1] / 2 - t0) / (t1 - t0)
                return tuple(p0[i] + f * (p1[i] - p0[i]) for i in range(3))
        return None

    return landmark('seg08'), landmark('seg09')


mats = pose()
eye, mouth = head_landmarks()
check(eye is not None and mouth is not None, 'the eye line or the mouth of gLinkAdultHeadNearDL was not found')
if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)
unit = mouth[1] - eye[1]
head = mats[10]
rot_t = [[head[j][i] for j in range(3)] for i in range(3)]


def face(point):
    """Face coordinates (up from the mouth line, forward of the mouth, toward Link's left) of a right-hand point."""
    w = [sum(mats[18][i][k] * (point[k] if k < 3 else 1) for k in range(4)) for i in range(3)]
    h = [sum(rot_t[i][k] * (w[k] - head[k][3]) for k in range(3)) for i in range(3)]
    return ((mouth[1] - h[1]) / unit, (h[0] - mouth[0]) / unit, h[2] / unit)


def placement(points):
    fc = [face(pt) for pt in sorted(set(points))]
    centroid = tuple(sum(v[i] for v in fc) / len(fc) for i in range(3))
    return centroid, min(math.sqrt(sum(x * x for x in v)) for v in fc)


fairy = [pt for m, pts in loads(LISTS['near'][1]) if m and 'FairyOcarina' in m for pt in pts]
native = placement(instrument(LISTS['near'][2]))
today = placement(fairy)
fitted = placement([apply(near, pt) for pt in fairy])
check(len(fairy) == 36, f'the Fairy item has {len(fairy)} vertices, not the stripped item\'s 36')
check(fitted[1] < today[1], f'the fitted Fairy item ({fitted[1]:.2f}) is not nearer the mouth than today ({today[1]:.2f})')
mapped = max(math.dist(apply(near, p), q) for p, q in zip(instrument(LISTS['near'][0]), instrument(LISTS['near'][2])))

print(f'Fit (near lists, production code): rotation {near["angle"]:.2f} deg, translation '
      f'{"/".join(f"{v:.1f}" for v in near["t"])}, residual rms {near["rms"]:.2f} max {near["max"]:.2f} model units '
      f'(limit {bound:.2f}); same with written-back addresses; far lists rotation {fits["far"]["angle"]:.2f} deg, '
      f'max {fits["far"]["max"]:.2f}; near fit on the far instrument max {far_error:.2f}; child Ocarina of Time mapped '
      f'onto the adult one max {mapped:.2f}')
print(f'Face units (okarina_swing frame 0, 0 = mouth line, 1 = eye line, unit {unit:.0f} model units): '
      'centroid (up, fwd, lat) / nearest to the mouth')
for label, (c, n) in (('OoT adult Ocarina of Time (native)', native), ('Fairy item today (unmoved)', today),
                      ('Fairy item, fitted placement (B3)', fitted)):
    print(f'  {label:36s} ({c[0]:+.2f}, {c[1]:+.2f}, {c[2]:+.2f}) / {n:.2f}')
if failures:
    print('\n'.join('FAIL: ' + f for f in failures))
    sys.exit(1)
print(f'PASS: archives {oot_path.parent}')
