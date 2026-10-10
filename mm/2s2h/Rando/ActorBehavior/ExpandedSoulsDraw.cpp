#include "ExpandedSoulsNative.h"
#include "ExpandedSoulsDrawPolicy.h"
#include "Rando/DrawFuncs.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "fast/resource/type/DisplayList.h"
#include "ship/resource/CrossRMRegistry.h"
#include "ship/resource/ResourceManager.h"
extern "C" {
#include "variables.h"
#include "functions.h"
}
namespace Rando {
// MM's display macros redeclare these at block scope. Give those declarations
// their native C linkage here, rather than creating namespaced C++ imports.
extern "C" {
void FrameInterpolation_RecordOpenChild(const void*, int);
void FrameInterpolation_RecordCloseChild(void);
}
namespace {
std::shared_ptr<Fast::DisplayList> bodySource, flameSource;
std::vector<Gfx> bodyModel, flameModel, jawModel;
void ClearExpandedSoulModel() {
    bodyModel.clear();
    flameModel.clear();
    jawModel.clear();
    bodySource.reset();
    flameSource.reset();
}
bool LoadExpandedSoulModel() {
    static bool registered = false;
    if (!registered) {
        Ship::CrossRMRegistry::RegisterTeardownListener(ClearExpandedSoulModel);
        registered = true;
    }
    if (!bodyModel.empty())
        return true;
    auto rm = Ship::CrossRMRegistry::Get("oot");
    if (!rm)
        return false;
    bodySource =
        std::dynamic_pointer_cast<Fast::DisplayList>(rm->LoadResource("objects/object_gi_sutaru/gGiSkulltulaTokenDL"));
    flameSource = std::dynamic_pointer_cast<Fast::DisplayList>(
        rm->LoadResource("objects/object_gi_sutaru/gGiSkulltulaTokenFlameDL"));
    if (!bodySource || !flameSource ||
        !PrepareExpandedSoulModel(bodySource->Instructions, flameSource->Instructions, bodyModel, flameModel,
                                  jawModel)) {
        ClearExpandedSoulModel();
        return false;
    }
    return true;
}
} // namespace
bool DrawExpandedSoulItem(RandoItemId item) {
    int index = ExpandedSoulIndex(item);
    if (index < 0 || !gPlayState || !LoadExpandedSoulModel())
        return false;
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    unsigned flame = ExpandedSoulFlameColor(index);
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, 0xaa, 0xaa, 0xaa, 255);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
    gSPComboRMPush(POLY_OPA_DISP++, "oot");
    gSPDisplayList(POLY_OPA_DISP++, bodyModel.data());
    gSPComboRMPop(POLY_OPA_DISP++);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 8,
               (uintptr_t)Gfx_TwoTexScroll(gfxCtx, 0, 0, -(s32)gPlayState->state.frames * 5, 32, 32, 1, 0, 0, 32, 64));
    gDPSetEnvColor(POLY_XLU_DISP++, flame >> 24, (flame >> 16) & 255, (flame >> 8) & 255, 255);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, flame >> 24, (flame >> 16) & 255, (flame >> 8) & 255, 255);
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
    gSPDisplayList(POLY_XLU_DISP++, flameModel.data());
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetEnvColor(POLY_XLU_DISP++, 0xaa, 0xaa, 0xaa, 255);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 255, 255);
    gSPDisplayList(POLY_XLU_DISP++, jawModel.data());
    gSPComboRMPop(POLY_XLU_DISP++);
    // Foreign seg8 scroll is consumed before this restore in the same stream.
    static Gfx emptyScroll[] = { gsSPEndDisplayList() };
    gSPSegment(POLY_XLU_DISP++, 8, (uintptr_t)emptyScroll);
    CLOSE_DISPS(gfxCtx);
    return true;
}
} // namespace Rando
