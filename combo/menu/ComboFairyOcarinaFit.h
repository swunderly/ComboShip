#pragma once
// Adult Link's Fairy Ocarina in MM, owner option B3 (tooling/claude-cleanup/t6/decision.md, ruling 2026-10-12). OoT
// holds the Ocarina of Time in the child's and in the adult's right hand with the same instrument mesh: the child's
// gLinkChildRightHandAndOotNearDL and the adult's gLinkAdultRightHandHoldingOotNearDL load it as one vertex load with
// the same slots and triangles, each in its own hand's frame. How OoT moves the instrument from the child's hand to the
// adult's is therefore a rigid transform that OoT's data determines. These pure helpers find it at run time from the
// lists the app extracted, so no placement number is written anywhere; ComboFairyOcarinaHand.h draws the child Fairy
// item under it on the adult open hand. No engine headers: ComboAndroid/tests/mm_adult_fairy_fit_tests.py compiles
// this file on its own.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ComboFairyOcarinaFit {

// libultraship opcodes (libultra/gbi.h, F3DEX2); ComboFairyOcarinaHand.h static_asserts that they match.
constexpr uint8_t kOpTri1 = 0x05, kOpTri2 = 0x06, kOpQuad = 0x07, kOpSetTImgHash = 0x20, kOpVtxHash = 0x32,
                  kOpMarker = 0x33, kOpTexture = 0xD7, kOpGeometryMode = 0xD9, kOpDl = 0xDE, kOpEndDl = 0xDF,
                  kOpFirstRdp = 0xE0;
// Player's segment for the cull list its limb lists call (OoT's adult hands call it).
constexpr uint8_t kCullSegment = 0x0C;
// Size of a vertex (Vtx: s16 x, y, z, u16 flag, s16 s, t, u8 r, g, b, a).
constexpr size_t kVertexBytes = 16;

using Point = std::array<int16_t, 3>;

// One vertex load of an extracted list (G_VTX_OTR_HASH): the material it is drawn with (the path hash of the last
// texture image set before it), its vertex array (path hash), where its first vertex is (a byte offset, or the address
// libultraship writes back once it has drawn the list), the slots it fills, and the triangle commands that follow it.
// The caller reads `points` from the array (ReadPoints).
struct VertexLoad {
    uint64_t material = 0;
    uint64_t array = 0;
    uintptr_t where = 0;
    unsigned count = 0, first = 0;
    std::vector<std::array<uintptr_t, 2>> triangles;
    std::vector<Point> points;
};

// The vertex loads of an extracted list (one slot per 64-bit command, two for a 128-bit one; libultraship
// DisplayListFactory.cpp). Besides the loads and their triangles, the list may only set render state (RDP commands,
// gsSPTexture, the geometry mode) and call Player's cull list; anything else (other vertex or matrix commands, calls to
// other lists) fails, as does a triangle before any load or a list without an end. So every vertex the list draws comes
// from a load listed here, in the limb's own frame.
template <typename Gfx> bool ListVertexLoads(const Gfx* list, size_t size, std::vector<VertexLoad>& loads) {
    loads.clear();
    uint64_t material = 0;
    for (size_t i = 0; list && i < size;) {
        const uintptr_t w0 = list[i].words.w0, w1 = list[i].words.w1;
        const uint8_t op = uint8_t(w0 >> 24);
        const bool wide = op == kOpSetTImgHash || op == kOpVtxHash || op == kOpMarker;
        if (i + (wide ? 2 : 1) > size)
            break;
        // The second slot of a 128-bit command holds the path hash (interpreter.cpp, gfx_vtx_hash_handler_custom).
        const uint64_t hash = wide ? (uint64_t(list[i + 1].words.w0) << 32) + uint64_t(list[i + 1].words.w1) : 0;
        if (op == kOpEndDl)
            return !loads.empty();
        if (op == kOpSetTImgHash) {
            material = hash;
        } else if (op == kOpVtxHash) {
            VertexLoad load;
            load.material = material;
            load.array = hash;
            load.where = w1;
            load.count = unsigned((w0 >> 12) & 0xFF);
            const unsigned end = unsigned((w0 >> 1) & 0x7F);
            if (load.count == 0 || end < load.count)
                break;
            load.first = end - load.count;
            loads.push_back(std::move(load));
        } else if (op == kOpTri1 || op == kOpTri2 || op == kOpQuad) {
            if (loads.empty())
                break;
            loads.back().triangles.push_back({ w0, w1 });
        } else if (op == kOpDl) {
            if (w1 > 0xFFFFFFFFu || uint8_t(w1 >> 24) != kCullSegment)
                break;
        } else if (op != kOpMarker && op != kOpTexture && op != kOpGeometryMode && op < kOpFirstRdp) {
            break;
        }
        i += wide ? 2 : 1;
    }
    loads.clear();
    return false;
}

// Byte offset of `load`'s first vertex in an array of `bytes` bytes at `base`. libultraship keeps the offset until it
// first draws the list, then writes the vertices' address over it (interpreter.cpp, gfx_vtx_hash_handler_custom: a
// value above 0xFFFFF is an address). False unless the load covers whole vertices inside the array.
inline bool VertexOffset(const VertexLoad& load, uintptr_t base, size_t bytes, size_t& offset) {
    uintptr_t at = load.where;
    if (at > 0xFFFFF) {
        if (at < base)
            return false;
        at -= base;
    }
    if (at % kVertexBytes || at > bytes || load.count > (bytes - at) / kVertexBytes)
        return false;
    offset = size_t(at);
    return true;
}

// Reads the positions of `load`'s vertices from its array (the vertex array resource's raw data).
inline bool ReadPoints(const uint8_t* base, size_t bytes, VertexLoad& load) {
    size_t offset = 0;
    load.points.clear();
    if (!base || !VertexOffset(load, reinterpret_cast<uintptr_t>(base), bytes, offset))
        return false;
    for (unsigned i = 0; i < load.count; ++i) {
        Point point;
        std::memcpy(point.data(), base + offset + i * kVertexBytes, sizeof(point));
        load.points.push_back(point);
    }
    return true;
}

// The one load drawn with `material`, or null when there is none or more than one.
inline const VertexLoad* LoadWith(const std::vector<VertexLoad>& loads, uint64_t material) {
    const VertexLoad* found = nullptr;
    for (const VertexLoad& load : loads)
        if (load.material == material) {
            if (found)
                return nullptr;
            found = &load;
        }
    return found;
}

inline bool SameLoad(const VertexLoad& a, const VertexLoad& b) {
    return a.material == b.material && a.count == b.count && a.first == b.first && a.points == b.points &&
           a.triangles == b.triangles && a.points.size() == a.count;
}

// The hand of `holding` (its loads not drawn with `instrument`) is the hand that `other` draws: `other`'s loads with
// those materials are the same meshes, slots and triangles, load for load. With `whole`, `other` draws nothing else.
inline bool SameHand(const std::vector<VertexLoad>& holding, const std::vector<VertexLoad>& other, uint64_t instrument,
                     bool whole) {
    std::vector<const VertexLoad*> hand, theirs;
    for (const VertexLoad& load : holding)
        if (load.material != instrument)
            hand.push_back(&load);
    for (const VertexLoad& load : other) {
        bool handMaterial = false;
        for (const VertexLoad* mine : hand)
            handMaterial = handMaterial || mine->material == load.material;
        if (handMaterial)
            theirs.push_back(&load);
        else if (whole)
            return false;
    }
    if (hand.empty() || hand.size() != theirs.size())
        return false;
    for (size_t i = 0; i < hand.size(); ++i)
        if (!SameLoad(*hand[i], *theirs[i]))
            return false;
    return true;
}

// q = rotation * p + translation (column vectors), and its residuals over the fitted pairs.
struct RigidFit {
    double rotation[3][3] = {};
    double translation[3] = {};
    double rms = 0, max = 0;
};

// Both meshes are s16 integers, so a vertex of an exact rigid copy differs from the rounded image by at most 0.5 per
// axis on each side, sqrt(3) in all. A fit is accepted only within it.
inline double RoundingBound() {
    return std::sqrt(3.0);
}

// Eigen-decomposition of a symmetric 4x4 matrix by cyclic Jacobi rotations: `a` ends diagonal (the eigenvalues), the
// columns of `v` are the eigenvectors.
inline void Jacobi4(double a[4][4], double v[4][4]) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            v[i][j] = i == j;
    for (int sweep = 0; sweep < 100; ++sweep) {
        double off = 0, diagonal = 0;
        for (int i = 0; i < 4; ++i) {
            diagonal += a[i][i] * a[i][i];
            for (int j = i + 1; j < 4; ++j)
                off += a[i][j] * a[i][j];
        }
        if (off <= 1e-30 * diagonal)
            return;
        for (int p = 0; p < 4; ++p)
            for (int q = p + 1; q < 4; ++q) {
                if (a[p][q] == 0)
                    continue;
                const double theta = (a[q][q] - a[p][p]) / (2 * a[p][q]);
                const double t = (theta >= 0 ? 1 : -1) / (std::fabs(theta) + std::sqrt(theta * theta + 1));
                const double c = 1 / std::sqrt(t * t + 1), s = t * c;
                for (int k = 0; k < 4; ++k) {
                    const double kp = a[k][p], kq = a[k][q];
                    a[k][p] = c * kp - s * kq;
                    a[k][q] = s * kp + c * kq;
                }
                for (int k = 0; k < 4; ++k) {
                    const double pk = a[p][k], qk = a[q][k];
                    a[p][k] = c * pk - s * qk;
                    a[q][k] = s * pk + c * qk;
                }
                for (int k = 0; k < 4; ++k) {
                    const double kp = v[k][p], kq = v[k][q];
                    v[k][p] = c * kp - s * kq;
                    v[k][q] = s * kp + c * kq;
                }
            }
    }
}

// The rigid transform (rotation and translation, no scale) that best maps `from` onto `to` pair by pair, in the
// least-squares sense: Horn's closed form with unit quaternions (J. Opt. Soc. Am. A 4(4), 629-642, 1987). False for
// fewer than three pairs, different counts, or points whose best rotation is not unique (all on one line).
inline bool FitRigid(const std::vector<Point>& from, const std::vector<Point>& to, RigidFit& fit) {
    const size_t n = from.size();
    if (n < 3 || to.size() != n)
        return false;
    double a[3] = {}, b[3] = {};
    for (size_t i = 0; i < n; ++i)
        for (int k = 0; k < 3; ++k) {
            a[k] += from[i][k] / double(n);
            b[k] += to[i][k] / double(n);
        }
    double s[3][3] = {};
    for (size_t i = 0; i < n; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                s[j][k] += (from[i][j] - a[j]) * (to[i][k] - b[k]);
    double m[4][4] = {
        { s[0][0] + s[1][1] + s[2][2], s[1][2] - s[2][1], s[2][0] - s[0][2], s[0][1] - s[1][0] },
        { s[1][2] - s[2][1], s[0][0] - s[1][1] - s[2][2], s[0][1] + s[1][0], s[2][0] + s[0][2] },
        { s[2][0] - s[0][2], s[0][1] + s[1][0], -s[0][0] + s[1][1] - s[2][2], s[1][2] + s[2][1] },
        { s[0][1] - s[1][0], s[2][0] + s[0][2], s[1][2] + s[2][1], -s[0][0] - s[1][1] + s[2][2] },
    };
    double v[4][4];
    Jacobi4(m, v);
    int best = 0;
    for (int i = 1; i < 4; ++i)
        if (m[i][i] > m[best][best])
            best = i;
    double second = -INFINITY;
    for (int i = 0; i < 4; ++i)
        if (i != best && m[i][i] > second)
            second = m[i][i];
    // Points on one line leave the turn about that line free: the two largest eigenvalues are then equal. The tolerance
    // only absorbs floating-point error.
    const double top = m[best][best];
    if (!std::isfinite(top) || !(top - second > 1e-9 * (std::fabs(top) + std::fabs(second))))
        return false;
    const double w = v[0][best], x = v[1][best], y = v[2][best], z = v[3][best];
    const double r[3][3] = { { 1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w) },
                             { 2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w) },
                             { 2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y) } };
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k)
            fit.rotation[j][k] = r[j][k];
        fit.translation[j] = b[j] - (r[j][0] * a[0] + r[j][1] * a[1] + r[j][2] * a[2]);
    }
    double sum = 0;
    fit.max = 0;
    for (size_t i = 0; i < n; ++i) {
        double d2 = 0;
        for (int j = 0; j < 3; ++j) {
            const double d =
                r[j][0] * from[i][0] + r[j][1] * from[i][1] + r[j][2] * from[i][2] + fit.translation[j] - to[i][j];
            d2 += d * d;
        }
        sum += d2;
        fit.max = std::fmax(fit.max, std::sqrt(d2));
    }
    fit.rms = std::sqrt(sum / double(n));
    return std::isfinite(fit.rms) && std::isfinite(fit.max);
}

enum class FitResult {
    Fitted,
    NoInstrument,
    InstrumentDiffers,
    ChildHandDiffers,
    AdultHandDiffers,
    Degenerate,
    NotRigid
};

inline const char* FitResultName(FitResult result) {
    switch (result) {
        case FitResult::Fitted:
            return "fitted";
        case FitResult::NoInstrument:
            return "no single Ocarina of Time load in the child or adult list";
        case FitResult::InstrumentDiffers:
            return "the child and adult instruments differ in slots or triangles";
        case FitResult::ChildHandDiffers:
            return "the child holds the Ocarina of Time and the Fairy item with different hands";
        case FitResult::AdultHandDiffers:
            return "the adult open hand differs from the adult hand holding the Ocarina of Time";
        case FitResult::Degenerate:
            return "the instrument does not determine a rotation";
        case FitResult::NotRigid:
            return "the adult instrument is not a rigid copy of the child's";
    }
    return "unknown";
}

// The placement of the Fairy item on adult Link's open hand. `childOot` and `adultOot` are OoT's child and adult right
// hands holding the Ocarina of Time (gLinkChildRightHandAndOotNearDL, gLinkAdultRightHandHoldingOotNearDL),
// `childFairy` the child's hand holding the Fairy item (gLinkChildRightHandHoldingFairyOcarinaNearDL) and `adultOpen`
// the adult open hand the Fairy composite draws (gLinkAdultRightHandNearDL); `instrument` is the path hash of the
// Ocarina of Time texture. The fit holds relative to the hands: the child holds both instruments with the same hand,
// and the adult open hand is the hand holding the Ocarina of Time without it.
inline FitResult FitFairyPlacement(const std::vector<VertexLoad>& childOot, const std::vector<VertexLoad>& childFairy,
                                   const std::vector<VertexLoad>& adultOot, const std::vector<VertexLoad>& adultOpen,
                                   uint64_t instrument, RigidFit& fit) {
    const VertexLoad* from = LoadWith(childOot, instrument);
    const VertexLoad* to = LoadWith(adultOot, instrument);
    if (!from || !to)
        return FitResult::NoInstrument;
    if (from->count != to->count || from->first != to->first || from->triangles.empty() ||
        from->triangles != to->triangles || from->points.size() != from->count || to->points.size() != to->count)
        return FitResult::InstrumentDiffers;
    if (!SameHand(childOot, childFairy, instrument, false))
        return FitResult::ChildHandDiffers;
    if (!SameHand(adultOot, adultOpen, instrument, true))
        return FitResult::AdultHandDiffers;
    if (!FitRigid(from->points, to->points, fit))
        return FitResult::Degenerate;
    return fit.max <= RoundingBound() ? FitResult::Fitted : FitResult::NotRigid;
}

// The fit as the games' float matrix (MtxF: mf[i][j], a row vector times the matrix, translation in mf[3]), for
// guMtxF2L and a G_MTX_MUL onto the hand limb's matrix.
inline void PlacementMatrix(const RigidFit& fit, float mf[4][4]) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            mf[i][j] = float(fit.rotation[j][i]);
        mf[i][3] = 0;
        mf[3][i] = float(fit.translation[i]);
    }
    mf[3][3] = 1;
}

// The rotation's angle, for the log.
inline double RotationDegrees(const RigidFit& fit) {
    const double c = (fit.rotation[0][0] + fit.rotation[1][1] + fit.rotation[2][2] - 1) / 2;
    return std::acos(std::fmax(-1.0, std::fmin(1.0, c))) * 180 / 3.14159265358979323846;
}

} // namespace ComboFairyOcarinaFit
