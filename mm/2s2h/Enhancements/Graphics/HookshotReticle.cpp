#include <cstring>
#include <fast/resource/type/DisplayList.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <libultraship/bridge/consolevariablebridge.h>
extern "C" {
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "HookshotReticle.h"
}

extern "C" Gfx* MM_GetHookshotReticleDisplayList(PlayState* play, Player* player, CollisionPoly* poly, s32 bgId) {
    Gfx* original = (Gfx*)gHookshotReticleDL;
    // Use the exact same collision-surface predicate as native Arms_Hook and OoT.
    // The caller retains all original line tests, range, projection and draw timing.
    if (!CVarGetInteger("gEnhancements.HookshotableReticle", 0) || player->heldItemAction != PLAYER_IA_HOOKSHOT ||
        !SurfaceType_IsHookshotSurface(&play->colCtx, poly, bgId))
        return original;
    try {
        auto context = Ship::Context::GetRawInstance();
        if (!context)
            return original;
        auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(
            context->GetResourceManager()->LoadResource(gHookshotReticleDL));
        if (!resource || resource->Instructions.empty() || resource->Instructions.size() > 128 ||
            (resource->Instructions.back().words.w0 >> 24) != G_ENDDL)
            return original;
        size_t colorIndex = 0, colorCount = 0;
        for (size_t i = 0; i < resource->Instructions.size(); ++i) {
            const auto opcode = resource->Instructions[i].words.w0 >> 24;
            // Match BinaryDisplayListV0's expanded command payload slots. Hashes
            // and pointer payloads are data, even if their high byte looks like FA.
            const bool expanded = opcode == G_SETTIMG_OTR_HASH || opcode == G_DL_OTR_HASH || opcode == G_VTX_OTR_HASH ||
                                  opcode == G_BRANCH_Z_OTR || opcode == G_MARKER || opcode == G_MTX_OTR ||
                                  opcode == G_MOVEMEM_OTR;
            if (expanded) {
                if (i + 1 >= resource->Instructions.size() - 1)
                    return original;
                ++i;
                continue;
            }
            if (opcode == G_ENDDL && i + 1 != resource->Instructions.size())
                return original;
            if (opcode == G_SETPRIMCOLOR) {
                colorIndex = i;
                ++colorCount;
            }
        }
        // Unknown/custom layouts with ambiguous color commands keep their own drawing.
        if (colorCount != 1)
            return original;
        size_t bytes = resource->Instructions.size() * sizeof(Gfx);
        auto colored = (Gfx*)GRAPH_ALLOC(play->state.gfxCtx, bytes);
        std::memcpy(colored, resource->Instructions.data(), bytes);
        // The native DL sets its red color internally. Change only that RGBA word;
        // retain texture, geometry, render state, LOD and all original commands.
        colored[colorIndex].words.w1 = 0x00FF00FF;
        return colored;
    } catch (...) {
        // No C++ resource exception crosses the native C draw entry point.
        return original;
    }
}
