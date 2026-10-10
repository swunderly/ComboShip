#pragma once
// Pinned OoTMM custom Adult Link model by Skilar; see ComboAdultUpstream.LICENSE.
// Resource descriptors contain no ROM/model payload; archive conversion is a separate private step.
#include "2s2h/resource/type/Skeleton.h"
#include <fast/resource/type/DisplayList.h>
#include <array>
extern "C" void Combo_MM_ApplyAdultModelTables(int adult);
namespace ComboAdultMM {
static std::shared_ptr<Ship::ResourceManager> sAdultModelOwner;
static std::array<std::shared_ptr<Ship::IResource>, 30> sAdultModelResources{};
static bool sAdultModelListener = false;
inline void DropAdultModelResources() {
    // Restore native child tables before any borrowed display-list/skeleton pointer dies.
    Combo_MM_ApplyAdultModelTables(0);
    sAdultModelResources = {};
    sAdultModelOwner.reset();
}
inline bool LoadAdultModelResources() {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    if (!mm)
        return false;
    if (sAdultModelOwner == mm && sAdultModelResources[0])
        return true;
    // Current native SkeletonFactory resolves limb dependencies through active MM context.
    // Reject another owner's context rather than swapping it during an asynchronous load.
    auto context = Ship::Context::GetRawInstance();
    if (!context || context->GetResourceManager() != mm)
        return false;
    std::array<std::shared_ptr<Ship::IResource>, 30> loaded{};
    loaded[0] = mm->LoadResourceProcess("objects/combo_adult_link/gComboAdultMmSkel");
    auto skeleton = std::dynamic_pointer_cast<SOH::Skeleton>(loaded[0]);
    if (!skeleton || skeleton->type != SOH::SkeletonType::Flex || skeleton->limbType != SOH::LimbType::LOD ||
        skeleton->limbCount != 21 || skeleton->dListCount != 18 || skeleton->skeletonHeaderSegments.size() != 21)
        return false;
    for (auto limb : skeleton->skeletonHeaderSegments)
        if (!limb)
            return false;
    for (size_t i = 1; i < loaded.size(); ++i) {
        if (i == 16)
            continue; // Native default-Kokiri alias is repaired below.
        loaded[i] = mm->LoadResourceProcess("objects/combo_adult_link/gComboAdultMmBinding_" + std::to_string(i));
        auto display = std::dynamic_pointer_cast<Fast::DisplayList>(loaded[i]);
        if (!display || display->Instructions.empty())
            return false;
    }
    // Native MM default one-hand/Kokiri banks share the same DL. Upstream binding16 is
    // an unrelocated old-child wrapper; binding17 is its exact intended pinned Kokiri composite.
    loaded[16] = loaded[17];
    // Commit only a complete typed set, retaining every resource and its owning native manager.
    if (sAdultModelOwner)
        DropAdultModelResources();
    sAdultModelOwner = mm;
    sAdultModelResources = std::move(loaded);
    if (!sAdultModelListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultModelResources);
        sAdultModelListener = true;
    }
    return true;
}
} // namespace ComboAdultMM
extern "C" int Combo_MM_AdultModelResourcesReady(void) {
    return ComboAdultMM::LoadAdultModelResources();
}
extern "C" void* Combo_MM_GetAdultModelBinding(int index) {
    if (index < 0 || index >= 30 || !ComboAdultMM::LoadAdultModelResources())
        return nullptr;
    return ComboAdultMM::sAdultModelResources[index]->GetRawPointer();
}
