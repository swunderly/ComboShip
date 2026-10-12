"""Adult Link in MM with OoT's own adult model (plan F5b, OoTMM v32.3 Cross-Games Age): pure helpers.

Compiles the production helpers in mm/2s2h/ComboAdultOotModel.h with a stand-in Gfx and checks:
H2 the shadow skeleton builder routes both LOD lists of every limb to OoT, keeps missing lists missing, copies the
   joints and tree, never writes the source, and rejects broken trees;
H3 the composite builder emits only [routed OoT hand, routed MM item..., end];
H6 the walker that finds an N64 command in an extracted list skips the port's marker and counts two-slot commands once.
The mask fit copies a mask list for Link with the scale calls written in and leaves the shared list unchanged.
"""
from pathlib import Path
import argparse
import subprocess
import sys

import hostcxx

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--compiler', default=hostcxx.DEFAULT_COMPILER)
a = p.parse_args()
root, work = a.source.resolve(), a.build_dir.resolve()
work.mkdir(parents=True, exist_ok=True)

HARNESS = r'''
#include "ComboAdultOotModel.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace ComboAdultOotModel;
struct Gfx { struct { uintptr_t w0, w1; } words; };
struct Limb { int16_t jointPos[3]; uint8_t child, sibling; Gfx* dLists[2]; };

// The Link limb tree (child, sibling) shared by OoT's adult and MM's human and Fierce Deity skeletons; limbs 0, 2
// and 9 (root, lower and upper controls) have no lists.
static const uint8_t kTree[21][2] = { { 1, 255 }, { 2, 9 }, { 3, 255 }, { 4, 6 }, { 5, 255 }, { 255, 255 },
    { 7, 255 }, { 8, 255 }, { 255, 255 }, { 10, 255 }, { 11, 12 }, { 255, 255 }, { 255, 13 }, { 14, 16 },
    { 15, 255 }, { 255, 255 }, { 17, 19 }, { 18, 255 }, { 255, 255 }, { 255, 20 }, { 255, 255 } };

struct Skeleton {
    std::vector<std::string> names;
    Limb limbs[21];
    const Limb* table[21];
    Skeleton() {
        names.reserve(42);
        for (int i = 0; i < 21; ++i) {
            Limb& l = limbs[i];
            l.jointPos[0] = int16_t(i * 3); l.jointPos[1] = int16_t(-i); l.jointPos[2] = int16_t(i * 7);
            l.child = kTree[i][0]; l.sibling = kTree[i][1];
            l.dLists[0] = l.dLists[1] = nullptr;
            if (i != 0 && i != 2 && i != 9)
                for (int lod = 0; lod < 2; ++lod) {
                    names.push_back("__OTR__objects/object_link_boy/limb" + std::to_string(i) + (lod ? "FarDL" : "NearDL"));
                    l.dLists[lod] = (Gfx*)names.back().c_str();
                }
            table[i] = &limbs[i];
        }
    }
};

static void ShadowSkeleton() {
    Skeleton s;
    assert(ValidLodTree(s.table, 21, 18));
    assert(!ValidLodTree(s.table, 21, 17) && !ValidLodTree(s.table, 21, 19));
    Limb before[21];
    std::memcpy(before, s.limbs, sizeof(before));
    Limb shadow[21];
    std::vector<std::string> paths;
    assert(BuildShadowLimbs(s.table, 21, "oot", shadow, paths));
    assert(std::memcmp(before, s.limbs, sizeof(before)) == 0); // the source is never written
    assert(paths.size() == 36);
    size_t lists = 0;
    for (int i = 0; i < 21; ++i) {
        assert(std::memcmp(shadow[i].jointPos, s.limbs[i].jointPos, sizeof(shadow[i].jointPos)) == 0);
        assert(shadow[i].child == s.limbs[i].child && shadow[i].sibling == s.limbs[i].sibling);
        for (int lod = 0; lod < 2; ++lod) {
            if (!s.limbs[i].dLists[lod]) {
                assert(shadow[i].dLists[lod] == nullptr);
                continue;
            }
            const std::string expect = "__OTR__@oot:" + std::string((const char*)s.limbs[i].dLists[lod] + 7);
            assert(expect == (const char*)shadow[i].dLists[lod]);
            assert((void*)shadow[i].dLists[lod] != (void*)s.limbs[i].dLists[lod]);
            ++lists;
        }
    }
    assert(lists == 36);
    const Limb* shadowTable[21];
    for (int i = 0; i < 21; ++i)
        shadowTable[i] = &shadow[i];
    assert(ValidLodTree(shadowTable, 21, 18)); // dListCount 18, as the header says

    // Broken skeletons are refused.
    Skeleton missing; missing.table[5] = nullptr;
    assert(!ValidLodTree(missing.table, 21, 18) && !BuildShadowLimbs(missing.table, 21, "oot", shadow, paths));
    Skeleton oneLod; oneLod.limbs[4].dLists[1] = nullptr;
    assert(!ValidLodTree(oneLod.table, 21, 18));
    Skeleton cycle; cycle.limbs[20].sibling = 13;
    assert(!ValidLodTree(cycle.table, 21, 18));
    Skeleton orphan; orphan.limbs[19].sibling = 255; // limb 20 is no longer reached
    assert(!ValidLodTree(orphan.table, 21, 18));
    Skeleton outside; outside.limbs[18].child = 21;
    assert(!ValidLodTree(outside.table, 21, 18));
    Skeleton routed; routed.names[0] = "__OTR__@oot:objects/object_link_boy/limb1NearDL";
    routed.limbs[1].dLists[0] = (Gfx*)routed.names[0].c_str();
    assert(!BuildShadowLimbs(routed.table, 21, "oot", shadow, paths)); // already routed: not a native limb list
    Skeleton raw; raw.limbs[3].dLists[0] = (Gfx*)"objects/object_link_boy/limb3NearDL";
    assert(!BuildShadowLimbs(raw.table, 21, "oot", shadow, paths));
    std::puts("H2 shadow skeleton: both LODs routed, nulls kept, 18 list limbs, broken trees refused");
}

static void Composite() {
    const char* hand = "__OTR__@oot:objects/object_link_boy/gLinkAdultLeftHandClosedNearDL";
    const char* items[] = { "__OTR__@mm:objects/object_link_child/gLinkHumanGildedSwordHandleDL",
                            "__OTR__@mm:objects/object_link_child/gLinkHumanGildedSwordBladeDL" };
    Gfx out[4] = {};
    assert(BuildComposite(out, 4, hand, items, 2) == 4);
    assert(out[0].words.w0 == 0x27000000u && out[0].words.w1 == (uintptr_t)hand);
    assert(out[1].words.w0 == 0x27000000u && out[1].words.w1 == (uintptr_t)items[0]);
    assert(out[2].words.w0 == 0x27000000u && out[2].words.w1 == (uintptr_t)items[1]);
    assert(out[3].words.w0 == 0xDF000000u && out[3].words.w1 == 0);
    Gfx three[3] = {};
    assert(BuildComposite(three, 3, hand, items, 1) == 3 && three[2].words.w0 == 0xDF000000u);
    assert(BuildComposite(three, 3, hand, items, 2) == 0);             // does not fit
    const char* plain[] = { "__OTR__objects/object_link_child/gLinkHumanBowDL" };
    assert(BuildComposite(out, 4, hand, plain, 1) == 0);               // an item that would not route
    assert(BuildComposite(out, 4, "__OTR__objects/x/y", items, 1) == 0); // a hand that would not route
    assert(BuildComposite(out, 4, nullptr, items, 1) == 0);
    std::puts("H3 composite: only [routed hand, routed items..., end]");
}

static Gfx G(uint8_t op, uintptr_t w1 = 0) { Gfx g; g.words.w0 = uintptr_t(op) << 24; g.words.w1 = w1; return g; }

static void Walker() {
    // marker(2) TEXTURE PIPESYNC SETTIMG_HASH(2) VTX_HASH(2) TRI1 DL_HASH(2) MTX_OTR(2) PIPESYNC ENDDL
    std::vector<Gfx> list = { G(kOpMarker), G(0, 1), G(kOpTexture), G(kOpPipeSync), G(kOpSetTImgHash), G(0, 2),
                              G(kOpVtxHash), G(0, 3), G(0x05), G(kOpDlHash), G(0, 4), G(kOpMtxOtr), G(0, 5),
                              G(kOpPipeSync), G(kOpEndDl) };
    const long expect[] = { 2, 3, 4, 6, 8, 9, 11, 13, 14 };
    for (size_t n = 0; n < 9; ++n)
        assert(N64CommandSlot(list.data(), list.size(), n) == expect[n]);
    assert(N64CommandSlot(list.data(), list.size(), 9) == -1); // past the end command
    assert(N64CommandSlot(list.data(), list.size() - 1, 8) == -1); // truncated list
    assert(N64CommandSlot(list.data(), 3, 1) == -1);
    // A list without the port's marker counts from its first slot; movemem/branch commands are two slots too.
    std::vector<Gfx> bare = { G(kOpPipeSync), G(kOpMovememOtr), G(0, 1), G(kOpBranchZOtr), G(0, 2), G(kOpTexture),
                              G(kOpEndDl) };
    assert(N64CommandSlot(bare.data(), bare.size(), 0) == 0 && N64CommandSlot(bare.data(), bare.size(), 1) == 1 &&
           N64CommandSlot(bare.data(), bare.size(), 2) == 3 && N64CommandSlot(bare.data(), bare.size(), 3) == 5);
    // A two-slot command cut off at the end of the list is not returned.
    std::vector<Gfx> cut = { G(kOpMarker), G(0), G(kOpPipeSync), G(kOpDlHash) };
    assert(N64CommandSlot(cut.data(), cut.size(), 1) == -1);
    std::puts("H6 walker: marker skipped, two-slot commands counted once");
}

static void FittedMask() {
    // Link's own copy of a mask list gets the calls at the walked slots; the shared list Kafei draws is untouched.
    const std::vector<Gfx> shared = { G(kOpMarker), G(0, 1), G(kOpPipeSync), G(kOpTexture), G(kOpDlHash), G(0, 2),
                                      G(kOpPipeSync), G(kOpEndDl) };
    const std::vector<Gfx> before = shared;
    const long slots[] = { N64CommandSlot(shared.data(), shared.size(), 0),
                           N64CommandSlot(shared.data(), shared.size(), 3) };
    assert(slots[0] == 2 && slots[1] == 6);
    const Gfx calls[] = { G(0xDE, 0x100), G(0xDE, 0x108) };
    const std::vector<Gfx> fitted = FittedMaskList(shared.data(), shared.size(), slots, calls, 2);
    assert(fitted.size() == shared.size());
    for (size_t i = 0; i < shared.size(); ++i) {
        assert(shared[i].words.w0 == before[i].words.w0 && shared[i].words.w1 == before[i].words.w1);
        const Gfx& want = i == 2 ? calls[0] : i == 6 ? calls[1] : shared[i];
        assert(fitted[i].words.w0 == want.words.w0 && fitted[i].words.w1 == want.words.w1);
    }
    assert(fitted.data() != shared.data());
    const long outside[] = { 2, 8 };
    assert(FittedMaskList(shared.data(), shared.size(), outside, calls, 2).empty());
    const long missing[] = { -1 };
    assert(FittedMaskList(shared.data(), shared.size(), missing, calls, 1).empty());
    std::puts("Mask fit: Link's own copy carries the calls, the shared list (Kafei's Keaton Mask) is unchanged");
}

static void Routes() {
    std::string out;
    assert(RoutePath("__OTR__objects/object_link_boy/gLinkAdultSkel", "oot", out) &&
           out == "__OTR__@oot:objects/object_link_boy/gLinkAdultSkel");
    assert(!RoutePath("__OTR__@oot:objects/a", "oot", out) && !RoutePath("objects/a", "oot", out) &&
           !RoutePath("__OTR__", "oot", out) && !RoutePath("__OTR__objects/a", "", out) &&
           !RoutePath(nullptr, "oot", out));
}

int main() {
    Routes();
    ShadowSkeleton();
    Composite();
    Walker();
    FittedMask();
    std::puts("PASS");
}
'''

source = work / 'cross_age_model_tests.cpp'
source.write_text(HARNESS, encoding='utf-8')
exe = hostcxx.build(a.compiler, source, std='c++17', includes=[root / 'mm/2s2h'], werror=True)
subprocess.run([str(exe)], cwd=work, check=True, timeout=hostcxx.RUN_TIMEOUT)
