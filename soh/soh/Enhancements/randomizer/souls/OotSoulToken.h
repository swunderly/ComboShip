#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace Rando::Souls {
// OoTMM draws a general OoT soul as the native Gold Skulltula token, recoloured (src/common/drawgi.c:759-830):
// white skull with a gray env colour, the flame in the family colour, prim LOD fraction 0x80.
inline constexpr uint8_t TokenSkullPrim[3] = { 255, 255, 255 }, TokenSkullEnv[3] = { 170, 170, 170 };
inline constexpr uint8_t TokenPrimLodFrac = 0x80;
// Flame colour by soul family (GeneralOotSoulFamily): enemy, NPC, animal, misc.
inline constexpr uint8_t TokenFlameColors[4][3] = { { 255, 0, 0 }, { 0, 255, 0 }, { 34, 34, 255 }, { 136, 136, 136 } };
// The colour commands inside the token's display lists (object_gi_sutaru), which the soul colours replace.
inline constexpr size_t TokenSkullPrimCmd = 5, TokenSkullEnvCmd = 6, TokenFlamePrimCmd = 32, TokenFlameEnvCmd = 33;

// Copies a display list with the colour commands at prim and env replaced by noOp. Returns false, leaving
// out unchanged, unless both are in range and carry the expected opcodes (the top byte of w0).
template <class Gfx>
bool ColorlessTokenCopy(const std::vector<Gfx>& in, size_t prim, size_t env, uint32_t primOp, uint32_t envOp,
                        const Gfx& noOp, std::vector<Gfx>& out) {
    if (prim >= in.size() || env >= in.size() || uint32_t(in[prim].words.w0 >> 24) != primOp ||
        uint32_t(in[env].words.w0 >> 24) != envOp)
        return false;
    out = in;
    out[prim] = noOp;
    out[env] = noOp;
    return true;
}
} // namespace Rando::Souls
