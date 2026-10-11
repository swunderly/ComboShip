#pragma once
// Adult Link's spin attack in MM (OoTMM v32.3 UpdateGameplayKeepForAdult, generator/src/mm/scene.c:58-96). OoTMM's
// three adult vertex files are MM's own gameplay_keep spin vertices with x, y and z scaled (plan F5b 1.6), so they are
// derived here at run time from MM's own arrays (lead decision D3). OoTMM also makes the spin-charge list's first
// command a call to [pipe sync, multiply by a 1.25 scale, end] (scene.c:68-78); the same list fits Keaton, Bunny and
// Scents to adult Link (2s2h/ComboAdultMaskScale.h). The matrix is built in code. Only MM's own resources change, and
// child Link gets them back.
#include <fast/lus_gbi.h>
#include "2s2h/resource/type/Array.h"
#include "ComboAdultOotModel.h"
#include <algorithm>
#include <iterator>
namespace ComboAdultMM {
// gameplay_keep's spin vertices, 26 + 26 for OoTMM's first file, 26 + 26 for its second and 14 for its third.
static const char* const kSpinVertexArrays[5] = {
    "objects/gameplay_keep/gameplay_keepVtx_025510", "objects/gameplay_keep/gameplay_keepVtx_0256B0",
    "objects/gameplay_keep/gameplay_keepVtx_025A90", "objects/gameplay_keep/gameplay_keepVtx_025C30",
    "objects/gameplay_keep/gameplay_keepVtx_026810",
};
static const size_t kSpinVertexCounts[5] = { 26, 26, 26, 26, 14 };
static const char kSpinChargingDL[] = "objects/gameplay_keep/gSpinAttackChargingDL";
static std::shared_ptr<Ship::ResourceManager> sAdultKeepOwner;
static std::weak_ptr<Ship::ResourceManager> sAdultKeepFailed;
static std::array<std::shared_ptr<SOH::Array>, 5> sSpinArrays{};
static std::array<std::vector<Fast::F3DVtx>, 5> sChildSpinVertices{}, sAdultSpinVertices{};
static std::shared_ptr<Fast::DisplayList> sSpinChargingDL;
static long sSpinChargingSlot = -1;
static Gfx sChildChargingFirst{};
static bool sAdultKeepApplied = false, sAdultKeepListener = false;
static Mtx sAdultMaskScaleMtx;
static Gfx sAdultMaskScaleDL[3];
// OoTMM's mask matrix list, gameplay_keep 0x5A2E0 (scene.c:72-78): a mask patch calls it from the pipe sync (0x5A2E0)
// or from the matrix (0x5A2E8).
inline Gfx* AdultMaskScaleDL(bool withSync) {
    static bool built = false;
    if (!built) {
        MtxF scale;
        SkinMatrix_SetScale(&scale, ComboAdultOotModel::kMaskScale, ComboAdultOotModel::kMaskScale,
                            ComboAdultOotModel::kMaskScale);
        SkinMatrix_MtxFToMtx(&scale, &sAdultMaskScaleMtx);
        const Gfx list[] = { gsDPPipeSync(),
                             gsSPMatrix(&sAdultMaskScaleMtx, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW),
                             gsSPEndDisplayList() };
        std::copy(std::begin(list), std::end(list), sAdultMaskScaleDL);
        built = true;
    }
    return withSync ? &sAdultMaskScaleDL[0] : &sAdultMaskScaleDL[1];
}
inline void RestoreAdultKeep() {
    if (!sAdultKeepApplied)
        return;
    for (size_t i = 0; i < sSpinArrays.size(); ++i)
        std::copy(sChildSpinVertices[i].begin(), sChildSpinVertices[i].end(), sSpinArrays[i]->Vertices.begin());
    sSpinChargingDL->Instructions[sSpinChargingSlot] = sChildChargingFirst;
    sAdultKeepApplied = false;
}
inline void DropAdultKeep() {
    RestoreAdultKeep();
    sSpinArrays = {};
    sChildSpinVertices = {};
    sAdultSpinVertices = {};
    sSpinChargingDL.reset();
    sSpinChargingSlot = -1;
    sAdultKeepOwner.reset();
}
inline bool LoadAdultKeep() {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    if (!mm || sAdultKeepFailed.lock() == mm)
        return false;
    if (sAdultKeepOwner == mm)
        return true;
    auto fail = [&]() {
        sAdultKeepFailed = mm;
        return false;
    };
    // Custom (mod) assets keep their own geometry, as 2S2H's own patches do.
    std::array<std::shared_ptr<SOH::Array>, 5> arrays{};
    for (size_t i = 0; i < arrays.size(); ++i) {
        arrays[i] = std::dynamic_pointer_cast<SOH::Array>(mm->LoadResourceProcess(kSpinVertexArrays[i]));
        if (!arrays[i] || arrays[i]->GetInitData()->IsCustom ||
            arrays[i]->ArrayType != SOH::ArrayResourceType::Vertex ||
            arrays[i]->Vertices.size() != kSpinVertexCounts[i])
            return fail();
    }
    auto charging = std::dynamic_pointer_cast<Fast::DisplayList>(mm->LoadResourceProcess(kSpinChargingDL));
    if (!charging || charging->GetInitData()->IsCustom)
        return fail();
    // OoTMM replaces the list's first N64 command, a pipe sync. The port's list starts with a two-slot marker.
    const long slot =
        ComboAdultOotModel::N64CommandSlot(charging->Instructions.data(), charging->Instructions.size(), 0);
    if (slot < 0 || (charging->Instructions[slot].words.w0 >> 24) != G_RDPPIPESYNC)
        return fail();
    if (sAdultKeepOwner)
        DropAdultKeep();
    sAdultKeepOwner = mm;
    sSpinArrays = std::move(arrays);
    for (size_t i = 0; i < sSpinArrays.size(); ++i) {
        sChildSpinVertices[i] = sSpinArrays[i]->Vertices;
        sAdultSpinVertices[i] = sChildSpinVertices[i];
        for (auto& vertex : sAdultSpinVertices[i])
            for (auto& coord : vertex.v.ob)
                coord = ComboAdultOotModel::ScaleSpinCoord(coord, ComboAdultOotModel::kSpinScale[i]);
    }
    sSpinChargingDL = std::move(charging);
    sSpinChargingSlot = slot;
    sChildChargingFirst = sSpinChargingDL->Instructions[slot];
    if (!sAdultKeepListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultKeep);
        sAdultKeepListener = true;
    }
    return true;
}
inline void ApplyAdultKeep(int adult) {
    if (!adult) {
        RestoreAdultKeep();
        return;
    }
    if (!LoadAdultKeep() || sAdultKeepApplied)
        return;
    // The native vertex storage and every other command of the list stay where they are.
    for (size_t i = 0; i < sSpinArrays.size(); ++i)
        std::copy(sAdultSpinVertices[i].begin(), sAdultSpinVertices[i].end(), sSpinArrays[i]->Vertices.begin());
    const Gfx call = gsSPDisplayList(AdultMaskScaleDL(true));
    sSpinChargingDL->Instructions[sSpinChargingSlot] = call;
    sAdultKeepApplied = true;
}
} // namespace ComboAdultMM
extern "C" int Combo_MM_AdultGameplayKeepResourcesReady(void) {
    return ComboAdultMM::LoadAdultKeep();
}
extern "C" void Combo_MM_ApplyAdultGameplayKeep(PlayState*, int adult) {
    ComboAdultMM::ApplyAdultKeep(adult);
}
