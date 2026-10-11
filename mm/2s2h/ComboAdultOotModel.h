#pragma once
// Pure helpers for Adult Link in MM drawn with OoT's own adult model (OoTMM v32.3 Cross-Games Age; plan F5b 1.2-1.6).
// No engine headers, so the host tests (ComboAndroid/tests/mm_cross_age_model_tests.py) compile this file on its own.
// Nothing here holds model data: limbs, display lists and vertices all come from the archives the app extracts.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace ComboAdultOotModel {

// libultraship opcodes (libultraship/libultra/gbi.h); BenPort.cpp static_asserts that they match.
constexpr uint8_t kOpSetTImgHash = 0x20, kOpDlFilePath = 0x27, kOpDlHash = 0x31, kOpVtxHash = 0x32, kOpMarker = 0x33,
                  kOpBranchZOtr = 0x35, kOpMtxOtr = 0x36, kOpMovememOtr = 0x42, kOpTexture = 0xD7, kOpEndDl = 0xDF,
                  kOpPipeSync = 0xE7;

// An extracted list keeps each 128-bit command (marker, hashed reference, OTR matrix or movemem) in two slots
// (libultraship DisplayListFactory.cpp, isExpanded).
inline bool IsTwoSlotOpcode(uint8_t op) {
    return op == kOpSetTImgHash || op == kOpDlHash || op == kOpVtxHash || op == kOpMarker || op == kOpBranchZOtr ||
           op == kOpMtxOtr || op == kOpMovememOtr;
}

// Slot of N64 command `n64Index` in an extracted display list, or -1. OoTMM counts N64 commands from the start of the
// ROM list; the port's list starts with a G_MARKER that is not an N64 command, and its two-slot commands count once.
template <typename Gfx> long N64CommandSlot(const Gfx* list, size_t size, size_t n64Index) {
    size_t command = 0;
    for (size_t i = 0; i < size;) {
        const uint8_t op = uint8_t(list[i].words.w0 >> 24);
        const size_t width = IsTwoSlotOpcode(op) ? 2 : 1;
        if (op != kOpMarker) {
            if (command == n64Index)
                return i + width <= size ? long(i) : -1;
            if (op == kOpEndDl)
                return -1;
            ++command;
        }
        i += width;
    }
    return -1;
}

// "__OTR__<path>" routed to a game's resource manager: "__OTR__@<game>:<path>" (libultraship interpreter.cpp,
// gfx_dl_otr_filepath_handler_custom; ResourceManager.cpp LoadResourceProcess).
inline bool RoutePath(const char* path, const char* game, std::string& out) {
    static const char kPrefix[] = "__OTR__";
    const size_t prefix = sizeof(kPrefix) - 1;
    if (!path || !game || !*game || std::strncmp(path, kPrefix, prefix) || !path[prefix] || path[prefix] == '@')
        return false;
    out = std::string(kPrefix) + "@" + game + ":" + (path + prefix);
    return true;
}

// Layout of an LOD limb (LodLimb in both games' z64animation.h).
struct LodLimbView {
    int16_t jointPos[3];
    uint8_t child, sibling;
    const void* dLists[2];
};
constexpr uint8_t kLimbDone = 0xFF;

// An LOD skeleton is usable when it has `limbCount` limbs, none missing, forming one tree from limb 0 that reaches
// every limb exactly once, with exactly `dListCount` limbs that have both LOD display lists and none with only one.
template <typename Limb> bool ValidLodTree(const Limb* const* limbs, size_t limbCount, size_t dListCount) {
    if (!limbs || !limbCount || limbCount >= kLimbDone)
        return false;
    for (size_t i = 0; i < limbCount; ++i)
        if (!limbs[i])
            return false;
    std::vector<uint8_t> seen(limbCount, 0);
    std::vector<size_t> stack{ 0 };
    size_t visited = 0, withLists = 0;
    while (!stack.empty()) {
        const size_t index = stack.back();
        stack.pop_back();
        if (index >= limbCount || seen[index])
            return false;
        seen[index] = 1;
        ++visited;
        const Limb* limb = limbs[index];
        if (!limb->dLists[0] != !limb->dLists[1])
            return false;
        withLists += limb->dLists[0] != nullptr;
        if (limb->sibling != kLimbDone)
            stack.push_back(limb->sibling);
        if (limb->child != kLimbDone)
            stack.push_back(limb->child);
    }
    return visited == limbCount && withLists == dListCount;
}

// Copies a game's LOD limbs into `out`, owned by the caller: the same joints and tree, with each display list path
// ("__OTR__<path>") routed to the owning game and interned in `paths`. A missing display list stays missing. The
// source limbs are never written.
template <typename Limb>
bool BuildShadowLimbs(const Limb* const* source, size_t limbCount, const char* game, Limb* out,
                      std::vector<std::string>& paths) {
    std::vector<std::string> routed;
    routed.reserve(limbCount * 2);
    for (size_t i = 0; i < limbCount; ++i) {
        if (!source[i])
            return false;
        for (int lod = 0; lod < 2; ++lod) {
            if (!source[i]->dLists[lod])
                continue;
            routed.emplace_back();
            if (!RoutePath(reinterpret_cast<const char*>(source[i]->dLists[lod]), game, routed.back()))
                return false;
        }
    }
    paths = std::move(routed);
    size_t next = 0;
    for (size_t i = 0; i < limbCount; ++i) {
        out[i] = *source[i];
        for (int lod = 0; lod < 2; ++lod) {
            using List = std::remove_reference_t<decltype(out[i].dLists[lod])>;
            out[i].dLists[lod] = source[i]->dLists[lod] ? (List)paths[next++].c_str() : nullptr;
        }
    }
    return true;
}

// A display list that calls a hand, then each item, and ends. MM's own "hand holding X" lists are [hand, item] (for
// example gLinkHumanRightHandHoldingBowDL); OoTMM draws MM's items in OoT's adult hand the same way
// (oot/actors/Player.c:651-664, 804-846). Every call is a routed path, so nothing is resolved until it is drawn.
// Returns the number of commands written, or 0 when they do not fit or a path is not a routed path.
template <typename Gfx>
size_t BuildComposite(Gfx* out, size_t capacity, const char* hand, const char* const* items, size_t itemCount) {
    if (!out || !items || capacity < itemCount + 2)
        return 0;
    auto routed = [](const char* path) { return path && !std::strncmp(path, "__OTR__@", 8) && std::strchr(path, ':'); };
    if (!routed(hand))
        return 0;
    for (size_t i = 0; i < itemCount; ++i)
        if (!routed(items[i]))
            return 0;
    size_t n = 0;
    auto call = [&](const char* path) {
        // gsSPDisplayListOTRFilePath: gsDma1p(G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH).
        out[n].words.w0 = uintptr_t(kOpDlFilePath) << 24;
        out[n].words.w1 = uintptr_t(path);
        ++n;
    };
    call(hand);
    for (size_t i = 0; i < itemCount; ++i)
        call(items[i]);
    out[n].words.w0 = uintptr_t(kOpEndDl) << 24; // gsSPEndDisplayList()
    out[n].words.w1 = 0;
    return n + 1;
}

// OoTMM's adult spin-attack vertex files are MM's own gameplay_keep spin vertices with x, y and z scaled (1.5 for
// gameplay_keepVtx_025510/0256B0, 1.3 for 025A90/025C30, 1.05 for 026810) and truncated as float math does;
// texture coordinates and colours are unchanged. scene.c:58-96 loads them over the native ones.
inline int16_t ScaleSpinCoord(int16_t value, float factor) {
    return int16_t(float(value) * factor);
}
constexpr float kSpinScale[5] = { 1.5f, 1.5f, 1.3f, 1.3f, 1.05f };
// OoTMM's mm_adult_link_mask_mtx.bin is a uniform 1.25 scale (scene.c:68-78; actors/Player.c:2099-2135).
constexpr float kMaskScale = 1.25f;

} // namespace ComboAdultOotModel
