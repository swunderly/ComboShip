#pragma once
// Adult Link's Fairy Ocarina in MM (owner ruling 2026-10-12, revised): adult Link holds the Ocarina of Time as OoT's
// own adult hand does (gLinkAdultRightHandHoldingOotNear/FarDL), coloured like the Fairy Ocarina. OoT draws the Ocarina
// of Time in that hand and the Fairy Ocarina in the child's hand (gLinkChildRightHandHoldingFairyOcarinaNear/FarDL)
// with the same texture load, tile, combiner, prim colour and render mode; only the texture differs, and both are
// RGBA16 32x16 (tooling/claude-cleanup/t10). So a private copy of each adult list sets the Fairy Ocarina's own texture,
// gLinkChildFairyOcarinaTex, where OoT sets gOcarinaofTimeDesignTex, and changes nothing else. These pure helpers do
// that rewrite; ComboFairyOcarinaHand.h builds the copies. No engine headers:
// ComboAndroid/tests/mm_adult_fairy_recolour_tests.py compiles this file on its own.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ComboFairyOcarinaRecolour {

// libultraship opcodes (libultra/gbi.h); ComboFairyOcarinaHand.h static_asserts that they match. An extracted list
// holds these as 128-bit commands, two slots each, the path hash in the second (DisplayListFactory.cpp).
constexpr uint8_t kOpSetTImgHash = 0x20, kOpDlHash = 0x31, kOpVtxHash = 0x32, kOpMarker = 0x33, kOpBranchZHash = 0x35,
                  kOpMtxHash = 0x36, kOpMoveMemHash = 0x42, kOpEndDl = 0xDF;

constexpr bool Wide(uint8_t op) {
    return op == kOpSetTImgHash || op == kOpDlHash || op == kOpVtxHash || op == kOpMarker || op == kOpBranchZHash ||
           op == kOpMtxHash || op == kOpMoveMemHash;
}

// Copies `source`, an extracted list, into `result` with its one texture image `from` replaced by `to`: only the path
// hash in that command's second slot changes. libultraship reads the hash there each time it draws the list, and the
// first slot's w1 only receives the resolved image (interpreter.cpp, gfx_set_timg_otr_hash_handler_custom). False,
// with `result` empty, unless `source` ends with its G_ENDDL and sets `from` exactly once.
template <typename Gfx>
bool SwapTexture(const std::vector<Gfx>& source, std::vector<Gfx>& result, uint64_t from, uint64_t to) {
    result.clear();
    size_t hashSlot = 0, matches = 0;
    for (size_t i = 0; i < source.size();) {
        const uint8_t op = uint8_t(source[i].words.w0 >> 24);
        const size_t width = Wide(op) ? 2 : 1;
        if (i + width > source.size())
            return false;
        if (op == kOpSetTImgHash &&
            (uint64_t(source[i + 1].words.w0) << 32) + uint64_t(source[i + 1].words.w1) == from) {
            hashSlot = i + 1;
            ++matches;
        }
        i += width;
        if (op == kOpEndDl) {
            if (matches != 1 || i != source.size())
                return false;
            result = source;
            result[hashSlot].words.w0 = uintptr_t(to >> 32);
            result[hashSlot].words.w1 = uintptr_t(to & 0xFFFFFFFFu);
            return true;
        }
    }
    return false;
}

// The list's load and tile commands were set for `from`, so `to` must have its layout: the same format, size, scale
// and load flags (Fast::Texture's fields).
template <typename Texture> bool SameLayout(const Texture& from, const Texture& to) {
    return from.Type == to.Type && from.Width == to.Width && from.Height == to.Height && from.Flags == to.Flags &&
           from.HByteScale == to.HByteScale && from.VPixelScale == to.VPixelScale &&
           from.ImageDataSize == to.ImageDataSize && from.ImageDataSize != 0;
}

} // namespace ComboFairyOcarinaRecolour
