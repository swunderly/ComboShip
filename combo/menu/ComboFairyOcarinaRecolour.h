#pragma once
// Adult Link's Fairy Ocarina in MM (owner ruling 2026-10-12, revised): adult Link holds the Ocarina of Time as OoT's
// own adult hand does (gLinkAdultRightHandHoldingOotNear/FarDL), coloured like the Fairy Ocarina. OoT draws the Ocarina
// of Time in that hand and the Fairy Ocarina in the child's hand (gLinkChildRightHandHoldingFairyOcarinaNear/FarDL)
// with the same texture load, tile, combiner, prim colour and render mode; only the texture differs, and both are
// RGBA16 32x16 (tooling/claude-cleanup/t10). So a private copy of each adult list sets the Fairy Ocarina's own texture,
// gLinkChildFairyOcarinaTex, where OoT sets gOcarinaofTimeDesignTex, and changes nothing else. These pure helpers do
// that rewrite and check it; ComboFairyOcarinaHand.h builds the copies. No engine headers:
// ComboAndroid/tests/mm_adult_fairy_recolour_tests.py compiles this file on its own.
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ComboFairyOcarinaRecolour {

// libultraship opcodes (libultra/gbi.h); ComboFairyOcarinaHand.h static_asserts that they match. An extracted list
// holds these as 128-bit commands, two slots each, the path hash in the second (DisplayListFactory.cpp).
constexpr uint8_t kOpSetTImgHash = 0x20, kOpDlHash = 0x31, kOpVtxHash = 0x32, kOpMarker = 0x33, kOpBranchZHash = 0x35,
                  kOpMtxHash = 0x36, kOpMoveMemHash = 0x42, kOpSetTileSize = 0xF2, kOpEndDl = 0xDF;

constexpr bool Wide(uint8_t op) {
    return op == kOpSetTImgHash || op == kOpDlHash || op == kOpVtxHash || op == kOpMarker || op == kOpBranchZHash ||
           op == kOpMtxHash || op == kOpMoveMemHash;
}

// The first slot of the one G_SETTIMG_OTR_HASH in `list`, an extracted list, that sets `texture`. False unless `list`
// ends with its G_ENDDL and sets `texture` exactly once.
template <typename Gfx> bool FindTexture(const std::vector<Gfx>& list, uint64_t texture, size_t& at) {
    size_t matches = 0;
    for (size_t i = 0; i < list.size();) {
        const uint8_t op = uint8_t(list[i].words.w0 >> 24);
        const size_t width = Wide(op) ? 2 : 1;
        if (i + width > list.size())
            return false;
        if (op == kOpSetTImgHash &&
            (uint64_t(list[i + 1].words.w0) << 32) + uint64_t(list[i + 1].words.w1) == texture) {
            at = i;
            ++matches;
        }
        i += width;
        if (op == kOpEndDl)
            return matches == 1 && i == list.size();
    }
    return false;
}

// Copies `source`, an extracted list, into `result` with its one texture image `from` replaced by `to`: only the path
// hash in that command's second slot changes. libultraship reads the hash there each time it draws the list, and the
// first slot's w1 only receives the resolved image (interpreter.cpp, gfx_set_timg_otr_hash_handler_custom). False,
// with `result` empty, unless `source` ends with its G_ENDDL and sets `from` exactly once.
template <typename Gfx>
bool SwapTexture(const std::vector<Gfx>& source, std::vector<Gfx>& result, uint64_t from, uint64_t to) {
    result.clear();
    size_t at = 0;
    if (!FindTexture(source, from, at))
        return false;
    result = source;
    result[at + 1].words.w0 = uintptr_t(to >> 32);
    result[at + 1].words.w1 = uintptr_t(to & 0xFFFFFFFFu);
    return true;
}

// How `list` loads and tiles `texture`: the first word of its one G_SETTIMG_OTR_HASH of `texture` (format, size and
// width; the w1 after it only receives the image when the list is drawn) and every command after it through the next
// G_SETTILESIZE, all 64-bit. False, with `load` empty, unless FindTexture finds it and it is tiled before the next
// 128-bit command or the end.
template <typename Gfx>
bool TextureLoad(const std::vector<Gfx>& list, uint64_t texture, std::vector<std::pair<uintptr_t, uintptr_t>>& load) {
    load.clear();
    size_t at = 0;
    if (!FindTexture(list, texture, at))
        return false;
    load.emplace_back(list[at].words.w0, uintptr_t(0));
    for (size_t i = at + 2; i < list.size(); ++i) {
        const uint8_t op = uint8_t(list[i].words.w0 >> 24);
        if (Wide(op) || op == kOpEndDl)
            break;
        load.emplace_back(list[i].words.w0, list[i].words.w1);
        if (op == kOpSetTileSize)
            return true;
    }
    load.clear();
    return false;
}

// The copy draws `to` with the commands `list` loads and tiles `from` with. That is right for `to` as OoT serves it (a
// texture pack's replacement, through alt/ or a mod archive, included) when OoT's own list for `to`, `reference`,
// loads and tiles it with the same commands: the interpreter sizes the served image from its own metadata (width,
// height, scales and flags; interpreter.cpp, gfx_set_timg_otr_hash_handler_custom) in both lists alike.
template <typename Gfx>
bool SameLoad(const std::vector<Gfx>& list, uint64_t from, const std::vector<Gfx>& reference, uint64_t to) {
    std::vector<std::pair<uintptr_t, uintptr_t>> copied, native;
    return TextureLoad(list, from, copied) && TextureLoad(reference, to, native) && copied == native;
}

} // namespace ComboFairyOcarinaRecolour
