"""Adult Link's Fairy Ocarina in MM, option B3 (tooling/claude-cleanup/t6/decision.md, ruling 2026-10-12): pure fit.

Compiles combo/menu/ComboFairyOcarinaFit.h on its own, with synthetic lists and meshes only (no ROM-derived data), and
checks:
- the list walker finds each vertex load with its material, array, slots and triangles; skips the port's marker and
  render state; accepts Player's cull-list call; and rejects lists with other vertex, matrix or list commands, a
  triangle before any load, or no end;
- a load's first vertex is found from its byte offset and from the address libultraship writes back after drawing;
- the rigid fit recovers a known rotation (9.66 degrees about a general axis) and translation from rounded s16 meshes
  within sqrt(3), and is refused for swapped pairs, noise above sqrt(3), a scale change, fewer than three pairs,
  different counts and points on one line;
- the placement is refused when the instrument is missing or doubled, when the two instruments differ in slots or
  triangles, when the child holds the two instruments with different hands, or when the adult open hand is not the
  hand holding the Ocarina of Time;
- the matrix handed to guMtxF2L moves a point as the fit does under MM's own row-vector convention (MtxF from
  libultraship fast/types.h, SkinMatrix_Vec3fMtxFMultXYZ from mm/src/code/z_skin_matrix.c).
The real archives are checked by mm_adult_fairy_fit_archive_tests.py (local only).
"""
from pathlib import Path
import argparse
import re
import subprocess

import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)


def block(source, start):
    """Text from start through the brace matching the first opening brace at or after it."""
    i = source.index('{', start)
    depth = 0
    while True:
        depth += (source[i] == '{') - (source[i] == '}')
        i += 1
        if depth == 0:
            return source[start:i]


skin = (root / 'mm/src/code/z_skin_matrix.c').read_text(encoding='utf-8')
mult = block(skin, hostcxx.find(skin, 'void SkinMatrix_Vec3fMtxFMultXYZ(MtxF* mf, Vec3f* src, Vec3f* dest) {'))
types = (root / 'libultraship/include/fast/types.h').read_text(encoding='utf-8')
mtxf = re.search(r'typedef float MtxF_t\[4\]\[4\];.*?\} MtxF;', types, re.S).group(0)

HARNESS = r'''
#include "ComboFairyOcarinaFit.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace ComboFairyOcarinaFit;
typedef float f32;
struct Vec3f { f32 x, y, z; };
@MTXF@
@MULT@

struct Gfx { struct { uintptr_t w0, w1; } words; };
static Gfx G(uintptr_t w0, uintptr_t w1) { return Gfx{ { w0, w1 } }; }
static void Wide(std::vector<Gfx>& l, uint8_t op, uintptr_t w0, uintptr_t w1, uint64_t hash) {
    l.push_back(G((uintptr_t(op) << 24) | w0, w1));
    l.push_back(G(uintptr_t(hash >> 32), uintptr_t(hash & 0xFFFFFFFFu)));
}
static void Marker(std::vector<Gfx>& l) { Wide(l, 0x33, 0, 0xBEEFBEEF, 0x1234); }
static void Material(std::vector<Gfx>& l, uint64_t tex) {
    l.push_back(G(0xE7000000, 0));        // pipe sync
    l.push_back(G(0xD9F3FFFF, 0));        // geometry mode
    Wide(l, 0x20, 0x100000, 0, tex);      // G_SETTIMG_OTR_HASH
    l.push_back(G(0xF5100000, 0x07090250)); // tile
    l.push_back(G(0xFC127E03, 0xFFFFFDF8)); // combine
}
static void Load(std::vector<Gfx>& l, uint64_t array, unsigned offset, unsigned count, unsigned first = 0) {
    Wide(l, 0x32, (count << 12) | ((first + count) << 1), offset * 16, array); // G_VTX_OTR_HASH
}
static void Tris(std::vector<Gfx>& l, unsigned count, unsigned seed) {
    for (unsigned i = 0; i < 3; ++i)
        l.push_back(G(0x06000000 | ((seed + i) % count) << 17 | ((seed + 2 * i + 1) % count) << 9 | ((i + 2) % count) << 1,
                      ((seed + i + 3) % count) << 17 | ((i + 1) % count) << 9 | ((seed + 5) % count) << 1));
}
static void End(std::vector<Gfx>& l) { l.push_back(G(0xDF000000, 0)); }

// Vertex arrays by path hash: raw 16-byte vertices as the resource holds them.
struct Arrays {
    std::vector<std::pair<uint64_t, std::vector<uint8_t>>> data;
    void Add(uint64_t hash, const std::vector<Point>& points) {
        std::vector<uint8_t> bytes(points.size() * kVertexBytes, 0xAB);
        for (size_t i = 0; i < points.size(); ++i)
            std::memcpy(&bytes[i * kVertexBytes], points[i].data(), 6);
        data.push_back({ hash, bytes });
    }
    bool Read(std::vector<VertexLoad>& loads) const {
        for (auto& load : loads) {
            const std::vector<uint8_t>* bytes = nullptr;
            for (auto& d : data)
                if (d.first == load.array)
                    bytes = &d.second;
            if (!bytes || !ReadPoints(bytes->data(), bytes->size(), load))
                return false;
        }
        return true;
    }
};

static std::vector<VertexLoad> Loads(const std::vector<Gfx>& list, const Arrays& arrays) {
    std::vector<VertexLoad> loads;
    assert(ListVertexLoads(list.data(), list.size(), loads));
    assert(arrays.Read(loads));
    return loads;
}

// A deterministic instrument-like mesh: n integer points spread over a few hundred units in all three axes.
static std::vector<Point> Mesh(unsigned n, unsigned seed) {
    std::vector<Point> points;
    uint32_t s = seed;
    for (unsigned i = 0; i < n; ++i) {
        Point p;
        for (int k = 0; k < 3; ++k) {
            s = s * 1664525u + 1013904223u;
            p[k] = int16_t(int(s >> 8) % 601 - 300);
        }
        points.push_back(p);
    }
    return points;
}

static void Rotation(const double axis[3], double degrees, double r[3][3]) {
    const double len = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    const double x = axis[0] / len, y = axis[1] / len, z = axis[2] / len;
    const double t = degrees * 3.14159265358979323846 / 180, c = std::cos(t), s = std::sin(t), k = 1 - c;
    const double m[3][3] = { { c + x * x * k, x * y * k - z * s, x * z * k + y * s },
                             { y * x * k + z * s, c + y * y * k, y * z * k - x * s },
                             { z * x * k - y * s, z * y * k + x * s, c + z * z * k } };
    std::memcpy(r, m, sizeof(m));
}

// q = round(scale * R p + t)
static std::vector<Point> Moved(const std::vector<Point>& from, const double r[3][3], const double t[3], double scale = 1) {
    std::vector<Point> to;
    for (const Point& p : from) {
        Point q;
        for (int j = 0; j < 3; ++j)
            q[j] = int16_t(std::lround(scale * (r[j][0] * p[0] + r[j][1] * p[1] + r[j][2] * p[2]) + t[j]));
        to.push_back(q);
    }
    return to;
}

static const double kAxis[3] = { 0.3, -0.8, 0.52 }, kTranslation[3] = { 59.4, 229.3, -22.6 };

static void Fit() {
    double r[3][3];
    Rotation(kAxis, 9.66, r);
    const std::vector<Point> from = Mesh(23, 7), to = Moved(from, r, kTranslation);
    RigidFit fit;
    assert(FitRigid(from, to, fit));
    assert(fit.max <= RoundingBound() && fit.rms <= fit.max);
    for (int j = 0; j < 3; ++j) {
        assert(std::fabs(fit.translation[j] - kTranslation[j]) < 2);
        for (int k = 0; k < 3; ++k)
            assert(std::fabs(fit.rotation[j][k] - r[j][k]) < 2e-3);
    }
    assert(std::fabs(RotationDegrees(fit) - 9.66) < 0.1);
    // A proper rotation: orthonormal with determinant +1.
    const double (*m)[3] = fit.rotation;
    const double det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                       m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    assert(std::fabs(det - 1) < 1e-9);
    // Identity and a half turn are found too.
    const double none[3] = { 0, 0, 0 };
    double id[3][3];
    Rotation(kAxis, 0, id);
    assert(FitRigid(from, Moved(from, id, none), fit) && fit.max < 1e-9 && RotationDegrees(fit) < 1e-3);
    double half[3][3];
    Rotation(kAxis, 180, half);
    assert(FitRigid(from, Moved(from, half, kTranslation), fit) && fit.max <= RoundingBound() &&
           std::fabs(RotationDegrees(fit) - 180) < 0.1);

    // Refused: swapped pairs, noise above sqrt(3), a scale change (as rigid copies), fewer than three pairs, counts
    // that differ, points on one line.
    std::vector<Point> swapped = to;
    std::swap(swapped[3], swapped[17]);
    assert(FitRigid(from, swapped, fit) && fit.max > RoundingBound());
    std::vector<Point> noisy = to;
    noisy[5][1] = int16_t(noisy[5][1] + 3);
    assert(FitRigid(from, noisy, fit) && fit.max > RoundingBound());
    assert(FitRigid(from, Moved(from, r, kTranslation, 1.02), fit) && fit.max > RoundingBound());
    const std::vector<Point> two(from.begin(), from.begin() + 2), twoTo(to.begin(), to.begin() + 2);
    assert(!FitRigid(two, twoTo, fit));
    assert(!FitRigid(from, std::vector<Point>(to.begin(), to.end() - 1), fit));
    std::vector<Point> line;
    for (int i = 0; i < 10; ++i)
        line.push_back(Point{ int16_t(10 * i), int16_t(-20 * i), int16_t(30 * i) });
    assert(!FitRigid(line, Moved(line, r, kTranslation), fit));
    std::puts("Rigid fit: 9.66 deg recovered within sqrt(3); swapped, noisy, scaled, short, unequal and collinear refused");
}

static void Offsets() {
    VertexLoad load;
    load.count = 4;
    size_t offset = 0;
    load.where = 32;
    assert(VertexOffset(load, 0x7000000000, 96, offset) && offset == 32);
    load.where = 0x7000000000 + 48; // the address libultraship writes back after the first draw
    assert(VertexOffset(load, 0x7000000000, 112, offset) && offset == 48);
    assert(!VertexOffset(load, 0x7000000000, 96, offset));     // runs past the array
    load.where = 0x7000000000 + 40;                           // not on a vertex
    assert(!VertexOffset(load, 0x7000000000, 1024, offset));
    load.where = 0x6FFFFFFFF0;                                // before the array
    assert(!VertexOffset(load, 0x7000000000, 1024, offset));
    load.where = 1024;
    assert(!VertexOffset(load, 0x7000000000, 1024, offset));
    std::vector<uint8_t> bytes(5 * kVertexBytes, 0);
    const int16_t xyz[3] = { -300, 7, 299 };
    std::memcpy(&bytes[2 * kVertexBytes], xyz, 6);
    load.where = 2 * kVertexBytes;
    load.count = 3;
    assert(ReadPoints(bytes.data(), bytes.size(), load) && load.points.size() == 3 &&
           load.points[0] == (Point{ -300, 7, 299 }));
    load.where = uintptr_t(bytes.data()) + kVertexBytes;
    assert(ReadPoints(bytes.data(), bytes.size(), load) && load.points[1] == (Point{ -300, 7, 299 }));
    std::puts("Vertex offsets: byte offset and written-back address both found, out-of-range refused");
}

static void Walker() {
    std::vector<Gfx> list;
    Marker(list);
    Material(list, 0xAAAA);
    Load(list, 0x51, 0, 32);
    Tris(list, 32, 1);
    list.push_back(G(0xDE000000, 0x0C000001)); // Player's cull list
    Load(list, 0x51, 30, 23);
    Tris(list, 23, 2);
    Material(list, 0xBBBB);
    Load(list, 0x52, 53, 23);
    list.push_back(G(0x05000204, 0));
    End(list);
    list.push_back(G(0xDA380003, 0x0D000000)); // past the end: never read
    std::vector<VertexLoad> loads;
    assert(ListVertexLoads(list.data(), list.size(), loads) && loads.size() == 3);
    assert(loads[0].material == 0xAAAA && loads[0].array == 0x51 && loads[0].where == 0 && loads[0].count == 32 &&
           loads[0].first == 0 && loads[0].triangles.size() == 3);
    assert(loads[1].where == 30 * 16 && loads[1].count == 23 && loads[1].triangles.size() == 3);
    assert(loads[2].material == 0xBBBB && loads[2].array == 0x52 && loads[2].triangles.size() == 1);
    const std::vector<Gfx> base = list;
    // Slot v0 and count as G_VTX encodes them.
    list = base;
    list[std::find_if(list.begin(), list.end(), [](const Gfx& g) { return g.words.w0 >> 24 == 0x32; }) - list.begin()]
        .words.w0 = (0x32u << 24) | (4u << 12) | ((10u + 4) << 1);
    assert(ListVertexLoads(list.data(), list.size(), loads) && loads[0].count == 4 && loads[0].first == 10);
    // Rejected: a matrix, raw vertices, another list, a file-path call, a triangle before any load, no end.
    const uintptr_t bad[][2] = { { 0xDA380003, 0x0D000040 }, { 0x01020040, 0x06000000 }, { 0xDE000000, 0x06000100 },
                                 { 0xDE000000, 0x0D000001 }, { 0x27000000, 0x1000 }, { 0xD8380002, 0x40 } };
    for (const auto& b : bad) {
        list = base;
        list.insert(list.begin() + 2, G(b[0], b[1]));
        assert(!ListVertexLoads(list.data(), list.size(), loads) && loads.empty());
    }
    list = base;
    list.insert(list.begin() + 2, G(0x06000204, 0));
    assert(!ListVertexLoads(list.data(), list.size(), loads));
    list = base;
    list.resize(list.size() - 2);
    assert(!ListVertexLoads(list.data(), list.size(), loads));
    list = base;
    list.resize(list.size() - 4); // cut inside the last load's hash slot
    assert(!ListVertexLoads(list.data(), list.size(), loads));
    std::puts("Walker: loads, materials, slots and triangles found; matrix, raw vertex, call and unended lists refused");
}

// Synthetic OoT-shaped lists: the child's hand with the Ocarina of Time (hand, then instrument), the child's hand with
// the Fairy item (item, then the same hand), the adult's hand with the Ocarina of Time and the adult's open hand.
struct Hands {
    Arrays arrays;
    std::vector<Gfx> childOot, childFairy, adultOot, adultOpen;
    static constexpr uint64_t kOcarina = 0x0CA, kFairyTex = 0xFA1, kChildHand = 0xC4, kAdultHand = 0xA4, kGauntlet = 0x6A;
    double r[3][3];
    Hands() {
        Rotation(kAxis, 9.66, r);
        const std::vector<Point> childHand = Mesh(55, 3), adultHand = Mesh(47, 4), item = Mesh(23, 5), fairy = Mesh(36, 6);
        std::vector<Point> childOotArray = childHand, childFairyArray = fairy, adultArray = adultHand,
                           adultOpenArray = adultHand;
        childOotArray.insert(childOotArray.end(), item.begin(), item.end());
        childFairyArray.insert(childFairyArray.end(), childHand.begin(), childHand.end());
        const std::vector<Point> moved = Moved(item, r, kTranslation);
        adultArray.insert(adultArray.end(), moved.begin(), moved.end());
        arrays.Add(1, childOotArray);
        arrays.Add(2, childFairyArray);
        arrays.Add(3, adultArray);
        arrays.Add(4, adultOpenArray);
        Marker(childOot);
        Material(childOot, kChildHand);
        Load(childOot, 1, 0, 32);
        Tris(childOot, 32, 1);
        Load(childOot, 1, 32, 23);
        Tris(childOot, 23, 2);
        Material(childOot, kOcarina);
        Load(childOot, 1, 55, 23);
        Tris(childOot, 23, 3);
        End(childOot);
        Marker(childFairy);
        Material(childFairy, kFairyTex);
        Load(childFairy, 2, 0, 32);
        Tris(childFairy, 32, 4);
        Load(childFairy, 2, 32, 4);
        Tris(childFairy, 4, 5);
        Material(childFairy, kChildHand);
        Load(childFairy, 2, 36, 32);
        Tris(childFairy, 32, 1);
        Load(childFairy, 2, 68, 23);
        Tris(childFairy, 23, 2);
        End(childFairy);
        for (auto* l : { &adultOot, &adultOpen }) {
            const uint64_t array = l == &adultOot ? 3 : 4;
            Marker(*l);
            Material(*l, kGauntlet);
            Load(*l, array, 0, 32);
            Tris(*l, 32, 6);
            l->push_back(G(0xDE000000, 0x0C000001));
            Material(*l, kAdultHand);
            Load(*l, array, 32, 15);
            Tris(*l, 15, 7);
        }
        Material(adultOot, kOcarina);
        Load(adultOot, 3, 47, 23);
        Tris(adultOot, 23, 3);
        End(adultOot);
        End(adultOpen);
    }
    FitResult Fit(RigidFit& fit) const {
        return FitFairyPlacement(Loads(childOot, arrays), Loads(childFairy, arrays), Loads(adultOot, arrays),
                                 Loads(adultOpen, arrays), kOcarina, fit);
    }
};

static void Placement() {
    RigidFit fit;
    {
        Hands h;
        assert(h.Fit(fit) == FitResult::Fitted && fit.max <= RoundingBound());
        for (int j = 0; j < 3; ++j)
            assert(std::fabs(fit.translation[j] - kTranslation[j]) < 2);
        assert(std::fabs(RotationDegrees(fit) - 9.66) < 0.1);
    }
    {
        Hands h; // the instrument's material missing from the adult list
        for (auto& g : h.adultOot)
            if (g.words.w1 == (Hands::kOcarina & 0xFFFFFFFFu) && g.words.w0 == 0)
                g.words.w1 = 0x0CB;
        assert(h.Fit(fit) == FitResult::NoInstrument);
    }
    {
        Hands h; // two loads with the instrument's material
        h.childOot.pop_back();
        Load(h.childOot, 1, 55, 23);
        End(h.childOot);
        assert(h.Fit(fit) == FitResult::NoInstrument);
    }
    {
        Hands h; // the adult instrument drawn with other triangles
        h.adultOot.pop_back();
        h.adultOot.push_back(G(0x05000204, 0));
        End(h.adultOot);
        assert(h.Fit(fit) == FitResult::InstrumentDiffers);
    }
    {
        Hands h; // the adult instrument loaded into other slots
        for (auto& g : h.adultOot)
            if (g.words.w0 >> 24 == 0x32 && g.words.w1 == 47 * 16)
                g.words.w0 = (0x32u << 24) | (23u << 12) | ((1u + 23) << 1);
        assert(h.Fit(fit) == FitResult::InstrumentDiffers);
    }
    {
        Hands h; // the child's Fairy hand is not the hand holding the Ocarina of Time
        std::vector<uint8_t>& bytes = h.arrays.data[1].second;
        bytes[(36 + 5) * kVertexBytes] ^= 1;
        assert(h.Fit(fit) == FitResult::ChildHandDiffers);
    }
    {
        Hands h; // the adult open hand differs from the hand holding the Ocarina of Time
        std::vector<uint8_t>& bytes = h.arrays.data[3].second;
        bytes[(32 + 2) * kVertexBytes + 2] ^= 1;
        assert(h.Fit(fit) == FitResult::AdultHandDiffers);
    }
    {
        Hands h; // the adult open hand draws something more
        h.adultOpen.pop_back();
        Material(h.adultOpen, 0x5E);
        Load(h.adultOpen, 4, 0, 3);
        Tris(h.adultOpen, 3, 1);
        End(h.adultOpen);
        assert(h.Fit(fit) == FitResult::AdultHandDiffers);
    }
    {
        Hands h; // the adult instrument is not a rigid copy
        std::vector<uint8_t>& bytes = h.arrays.data[2].second;
        int16_t x;
        std::memcpy(&x, &bytes[(47 + 9) * kVertexBytes], 2);
        x = int16_t(x + 4);
        std::memcpy(&bytes[(47 + 9) * kVertexBytes], &x, 2);
        assert(h.Fit(fit) == FitResult::NotRigid);
    }
    for (FitResult result : { FitResult::Fitted, FitResult::NoInstrument, FitResult::InstrumentDiffers,
                              FitResult::ChildHandDiffers, FitResult::AdultHandDiffers, FitResult::Degenerate,
                              FitResult::NotRigid })
        assert(std::strcmp(FitResultName(result), "unknown") != 0);
    std::puts("Placement: fitted from the hands; missing, doubled or different instruments and different hands refused");
}

// The matrix for guMtxF2L moves points as the fit does, through MM's own row-vector multiply.
static void Matrix() {
    double r[3][3];
    Rotation(kAxis, 9.66, r);
    const std::vector<Point> from = Mesh(23, 7);
    RigidFit fit;
    assert(FitRigid(from, Moved(from, r, kTranslation), fit));
    MtxF mtx;
    PlacementMatrix(fit, mtx.mf);
    assert(mtx.xw == float(fit.translation[0]) && mtx.yw == float(fit.translation[1]) &&
           mtx.zw == float(fit.translation[2]) && mtx.ww == 1 && mtx.wx == 0 && mtx.wy == 0 && mtx.wz == 0);
    const Vec3f units[] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 120, -45, 300 } };
    for (Vec3f src : units) {
        Vec3f dest;
        SkinMatrix_Vec3fMtxFMultXYZ(&mtx, &src, &dest);
        const double p[3] = { src.x, src.y, src.z };
        const float want[3] = { float(fit.rotation[0][0] * p[0] + fit.rotation[0][1] * p[1] + fit.rotation[0][2] * p[2] + fit.translation[0]),
                                float(fit.rotation[1][0] * p[0] + fit.rotation[1][1] * p[1] + fit.rotation[1][2] * p[2] + fit.translation[1]),
                                float(fit.rotation[2][0] * p[0] + fit.rotation[2][1] * p[1] + fit.rotation[2][2] * p[2] + fit.translation[2]) };
        assert(std::fabs(dest.x - want[0]) < 1e-3 && std::fabs(dest.y - want[1]) < 1e-3 && std::fabs(dest.z - want[2]) < 1e-3);
    }
    std::puts("Matrix: MM's row-vector multiply moves points as the fit does");
}

int main() {
    Fit();
    Offsets();
    Walker();
    Placement();
    Matrix();
    std::puts("PASS");
}
'''

source = work / 'adult_fairy_fit_tests.cpp'
source.write_text(HARNESS.replace('@MTXF@', mtxf).replace('@MULT@', mult), encoding='utf-8')
# libultraship's MtxF names its elements through an anonymous struct, as the engine compiles it.
exe = hostcxx.build(a.compiler, source, std='c++17', includes=[root / 'combo/menu'], werror=True, msvc=['/wd4201'])
subprocess.run([str(exe)], cwd=work, check=True, timeout=hostcxx.RUN_TIMEOUT)
