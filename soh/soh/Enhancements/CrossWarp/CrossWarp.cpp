#ifdef COMBO_BUILD
#include "CrossWarp.h"
#include "MMWarpAssets.h"
#include "menu/ComboForeignAnim.h"
#include "ComboResolve.h"
#include <algorithm>
extern "C" int Combo_SOHPrepareWarpAssets() {
    auto ensure = (int (*)())Combo_ResolveSym("2ship", "MM_EnsureWarpResources");
    if (!ensure || !ensure())
        return 0;
    const char* required[] = { gGameplayKeepKFSkel_85640,
                               gGameplayKeepKFAnim_83534,
                               gSoaringWarpCsFeatherDL,
                               gSoaringWarpCsWindCapsuleDL,
                               gSoaringWarpCsWindCapsuleTexAnim,
                               gOceff6DL,
                               gOceff6Vtx,
                               ovl_Oceff_Wipe6_Matanimheader_000338,
                               gWorldMapImageTex,
                               gWorldMapImageTLUT,
                               gWorldMapOwlFaceTex,
                               gItemNamePanelDL };
    for (const char* path : required)
        if (!Combo_SOHWarpResource(path))
            return 0;
    return 1;
}
extern "C" void* Combo_SOHWarpResource(const char* path) {
    auto load = (void* (*)(const char*))Combo_ResolveSym("2ship", "MM_LoadWarpResource");
    return load ? load(path) : nullptr;
}
extern "C" uint32_t Combo_SOHMapClouds() {
    auto clouds = (uint32_t(*)())Combo_ResolveSym("2ship", "MM_GetWarpMapClouds");
    return clouds ? clouds() : 0;
}
extern "C" void Combo_SOHWarpMaterial(PlayState* play, const char* path, int step) {
    auto* mat = (const CfaMatEntry*)Combo_SOHWarpResource(path);
    if (!mat || !mat->segment)
        return;
    for (int entry = 0; entry < kMaxMatEntries; entry++, mat++) {
        const int segment = std::abs(mat->segment) + 7;
        if (mat->type == kMatTypeTwoTexScroll && mat->params) {
            CfaDrawTwoTexScroll(play, segment, (const CfaTexScrollEntry*)mat->params, step, false);
        } else if (mat->type == 2 || mat->type == 3 || mat->type == 4) {
            auto* colors = (const CfaColorParams*)mat->params;
            if (!colors || !colors->keyFrameLength || !colors->primColors)
                return;
            if (mat->type == 2)
                CfaDrawColorIndexed(play, segment, colors, step);
            else {
                if (!colors->keyFrames || colors->keyFrameCount < 2 || colors->keyFrameCount > kMaxKeyFrames)
                    return;
                if (mat->type == 4)
                    CfaDrawColorLagrange(play, segment, colors, step);
                else {
                    // MM AnimatedMat_DrawColorLerp, retaining its integer color conversion.
                    const int frame = step % colors->keyFrameLength;
                    int upper = 1;
                    while (upper < colors->keyFrameCount - 1 && frame >= colors->keyFrames[upper])
                        ++upper;
                    const int length = colors->keyFrames[upper] - colors->keyFrames[upper - 1];
                    const float phase =
                        length > 0 ? std::clamp(float(frame - colors->keyFrames[upper - 1]) / length, 0.0f, 1.0f)
                                   : 0.0f;
                    CfaPrimColor prim;
                    CfaEnvColor env;
                    auto lerp = [phase](uint8_t a, uint8_t b) { return uint8_t(a + (b - a) * phase); };
                    const auto& a = colors->primColors[upper - 1];
                    const auto& b = colors->primColors[upper];
                    prim = { lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a),
                             lerp(a.lodFrac, b.lodFrac) };
                    if (colors->envColors) {
                        const auto& c = colors->envColors[upper - 1];
                        const auto& d = colors->envColors[upper];
                        env = { lerp(c.r, d.r), lerp(c.g, d.g), lerp(c.b, d.b), lerp(c.a, d.a) };
                    }
                    CfaSetColorSegment(play, segment, &prim, colors->envColors ? &env : nullptr);
                }
            }
        } else
            return;
        if (mat->segment < 0)
            break;
    }
}
extern "C" void Combo_SOHRestoreWarpMaterial(PlayState* play, const char* path) {
    auto* mat = (const CfaMatEntry*)Combo_SOHWarpResource(path);
    if (!mat || !mat->segment)
        return;
    int32_t segments[kMaxMatEntries], count = 0;
    do {
        segments[count++] = std::abs(mat->segment) + 7;
    } while (mat++->segment >= 0 && count < kMaxMatEntries);
    ComboForeignTexAnim_Restore(play, segments, count, false);
}

extern "C" void Combo_SOHWindMaterial(PlayState* play, int step) {
    Combo_SOHWarpMaterial(play, gSoaringWarpCsWindCapsuleTexAnim, step);
}
extern "C" int Combo_SOHWarpVertexCount(const char* path) {
    auto count = (int (*)(const char*))Combo_ResolveSym("2ship", "MM_WarpVertexCount");
    return count ? count(path) : 0;
}
#endif
