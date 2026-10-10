#pragma once
// Exact Scene_ApplyHumanAgeGameplayKeep edits from OoTMM development commit 41a2b7c0 (not in v32.3),
// adapted to native typed resources.
// Artist vertices/matrix are prepared locally; native MM keep remains owned by MM RM.
#include <fast/lus_gbi.h>
#include "2s2h/resource/type/Array.h"
#include <fast/resource/type/Matrix.h>
#include <algorithm>
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sAdultKeepOwner;
static std::array<std::shared_ptr<SOH::Array>, 5> sChildSpinArrays{};
static std::array<std::shared_ptr<SOH::Array>, 3> sAdultSpinArrays{};
static std::array<std::vector<Fast::F3DVtx>, 5> sChildSpinVertices{};
static std::shared_ptr<Fast::DisplayList> sAdultChargingDL;
static std::shared_ptr<Fast::Matrix> sAdultMaskMatrix;
static std::array<Gfx, 3> sAdultMaskMatrixDL{};
static Gfx sChildChargingFirst{};
static bool sAdultKeepApplied = false, sAdultKeepListener = false;
inline void RestoreAdultKeep() {
    if (!sAdultKeepApplied)
        return;
    for (size_t i = 0; i < 5; ++i)
        std::copy(sChildSpinVertices[i].begin(), sChildSpinVertices[i].end(), sChildSpinArrays[i]->Vertices.begin());
    sAdultChargingDL->Instructions[0] = sChildChargingFirst;
    sAdultKeepApplied = false;
}
inline void DropAdultKeep() {
    RestoreAdultKeep();
    sChildSpinArrays = {};
    sAdultSpinArrays = {};
    sChildSpinVertices = {};
    sAdultChargingDL.reset();
    sAdultMaskMatrix.reset();
    sAdultKeepOwner.reset();
}
inline bool LoadAdultKeep() {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    if (!mm)
        return false;
    if (sAdultKeepOwner == mm)
        return true;
    std::array<std::shared_ptr<SOH::Array>, 5> child{};
    std::array<std::shared_ptr<SOH::Array>, 3> adult{};
    // Native ZAPD splits each 52-vertex ROM span into two 26-vertex resources.
    const char* offsets[] = { "025510", "0256B0", "025A90", "025C30", "026810" };
    const size_t childCounts[] = { 26, 26, 26, 26, 14 }, adultCounts[] = { 52, 52, 14 };
    for (size_t i = 0; i < 5; ++i) {
        child[i] = std::dynamic_pointer_cast<SOH::Array>(
            mm->LoadResourceProcess(std::string("objects/gameplay_keep/gameplay_keepVtx_") + offsets[i]));
        if (!child[i] || child[i]->ArrayType != SOH::ArrayResourceType::Vertex ||
            child[i]->Vertices.size() != childCounts[i])
            return false;
    }
    for (size_t i = 0; i < 3; ++i) {
        adult[i] = std::dynamic_pointer_cast<SOH::Array>(mm->LoadResourceProcess(
            std::string("objects/combo_adult_gameplay_keep/gComboAdultSpinVtx") + std::to_string(i + 1)));
        if (!adult[i] || adult[i]->ArrayType != SOH::ArrayResourceType::Vertex ||
            adult[i]->Vertices.size() != adultCounts[i])
            return false;
    }
    auto dl = std::dynamic_pointer_cast<Fast::DisplayList>(
        mm->LoadResourceProcess("objects/gameplay_keep/gSpinAttackChargingDL"));
    auto matrix = std::dynamic_pointer_cast<Fast::Matrix>(
        mm->LoadResourceProcess("objects/combo_adult_gameplay_keep/gComboAdultMaskMtx"));
    if (!dl || dl->Instructions.empty() || !matrix)
        return false;
    if (sAdultKeepOwner)
        DropAdultKeep();
    sAdultKeepOwner = mm;
    sChildSpinArrays = std::move(child);
    sAdultSpinArrays = std::move(adult);
    for (size_t i = 0; i < 5; ++i)
        sChildSpinVertices[i] = sChildSpinArrays[i]->Vertices;
    sAdultChargingDL = std::move(dl);
    sChildChargingFirst = sAdultChargingDL->Instructions[0];
    sAdultMaskMatrix = std::move(matrix);
    // 41a2b7c0 wrapper: PipeSync; modelview multiply/no-push matrix; EndDL.
    Gfx wrapper[] = { gsDPPipeSync(),
                      gsSPMatrix(sAdultMaskMatrix->GetPointer(), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW),
                      gsSPEndDisplayList() };
    std::copy(std::begin(wrapper), std::end(wrapper), sAdultMaskMatrixDL.begin());
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
    // Preserve native vertex storage addresses and every command after the patched first one.
    const size_t source[] = { 0, 0, 1, 1, 2 }, start[] = { 0, 26, 0, 26, 0 };
    for (size_t i = 0; i < 5; ++i)
        std::copy_n(sAdultSpinArrays[source[i]]->Vertices.begin() + start[i], sChildSpinArrays[i]->Vertices.size(),
                    sChildSpinArrays[i]->Vertices.begin());
    Gfx call = gsSPDisplayList(sAdultMaskMatrixDL.data());
    sAdultChargingDL->Instructions[0] = call;
    sAdultKeepApplied = true;
}
} // namespace ComboAdultMM
extern "C" int Combo_MM_AdultGameplayKeepResourcesReady(void) {
    return ComboAdultMM::LoadAdultKeep();
}
extern "C" void Combo_MM_ApplyAdultGameplayKeep(PlayState*, int adult) {
    ComboAdultMM::ApplyAdultKeep(adult);
}
