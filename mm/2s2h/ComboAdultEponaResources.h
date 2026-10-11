#pragma once
// Adult Link's Epona is OoT's own Epona: OoTMM's mm_adult_epona.zobj is OoT's object_horse, so it is loaded from the
// OoT archive the app already extracts (feature plan F5b 1.7, 5b-7). Only resource names live here. MM draws it through
// an MM-owned shadow of OoT's skin skeleton whose display lists route to OoT ("__OTR__@oot:"), so OoT's cached
// resources are never changed.
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#include <fast/resource/ResourceType.h>
#include <fast/resource/type/Texture.h>
#include "2s2h/resource/type/2shResourceType.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
extern "C" void Combo_MM_ApplyEponaVariantTables(int adult);
namespace ComboAdultMM {
// OoT's object_horse, the same in every retail version (soh/assets/xml/*/objects/object_horse.xml).
static constexpr int kAdultEponaLimbs = 47;
static const char sAdultEponaSkelName[] = "objects/object_horse/gEponaSkel";
// En_Horse's animation table order (ENHORSE_ANIM_*) as OoTMM's table patch writes it (actors.c:942-950), with each
// animation's OoT frame count.
static const char* const sAdultEponaAnimNames[] = {
    "objects/object_horse/gEponaIdleAnim",       "objects/object_horse/gEponaWhinnyAnim",
    "objects/object_horse/gEponaRefuseAnim",     "objects/object_horse/gEponaRearingAnim",
    "objects/object_horse/gEponaWalkingAnim",    "objects/object_horse/gEponaTrottingAnim",
    "objects/object_horse/gEponaGallopingAnim",  "objects/object_horse/gEponaJumpingAnim",
    "objects/object_horse/gEponaJumpingHighAnim"
};
static const s16 sAdultEponaAnimFrames[] = { 120, 30, 65, 33, 35, 29, 24, 23, 30 };
// D_80889204 order (open, half, closed) as OoTMM's eye patch writes it (actors.c:952-954).
alignas(2) static const char sAdultEponaEyePaths0[] = "__OTR__@oot:objects/object_horse/gEponaEyeOpenTex";
alignas(2) static const char sAdultEponaEyePaths1[] = "__OTR__@oot:objects/object_horse/gEponaEyeHalfTex";
alignas(2) static const char sAdultEponaEyePaths2[] = "__OTR__@oot:objects/object_horse/gEponaEyeClosedTex";
static const char* const sAdultEponaEyePaths[] = { sAdultEponaEyePaths0, sAdultEponaEyePaths1, sAdultEponaEyePaths2 };
// The raw header OoT's skeleton resource hands out (soh/soh/resource/type/Skeleton.h).
struct OotSkeletonHeader {
    void** segment;
    uint8_t limbCount;
    uint8_t skeletonType; // SOH::SkeletonType::Normal is 0
};
// The skeleton MM's Skin code draws: OoT's limbs with their display lists routed to OoT and their skin data copied.
struct AdultEponaShadow {
    ::SkeletonHeader header;
    std::array<void*, kAdultEponaLimbs> table;
    std::array<::SkinLimb, kAdultEponaLimbs> limbs;
    std::array<::SkinAnimatedLimbData, kAdultEponaLimbs> animated;
    std::array<std::string, kAdultEponaLimbs> displayLists;
    std::vector<::SkinLimbModif> modifs;
    std::vector<::SkinVertex> vertices;
    std::vector<::SkinTransformation> transformations;
};
static std::shared_ptr<Ship::ResourceManager> sAdultEponaOwner;
static std::weak_ptr<Ship::ResourceManager> sAdultEponaFailed;
static std::unique_ptr<AdultEponaShadow> sAdultEponaShadow;
// The skeleton, the 9 animations (bindings 1-9), the eye textures and the routed display lists stay loaded until
// teardown.
static std::vector<std::shared_ptr<Ship::IResource>> sAdultEponaResources;
static std::array<::AnimationHeader*, 9> sAdultEponaAnimations{};
static bool sAdultEponaListener = false;
inline void DropAdultEpona() {
    // Restore actor tables before releasing the shadow skeleton and the pinned resources.
    Combo_MM_ApplyEponaVariantTables(0);
    sAdultEponaAnimations = {};
    sAdultEponaResources.clear();
    sAdultEponaShadow.reset();
    sAdultEponaOwner.reset();
}
inline bool IsOotPath(const void* path) {
    return path != nullptr && strncmp((const char*)path, "__OTR__", 7) == 0 && ((const char*)path)[7] != '@';
}
inline bool IsResourceType(const std::shared_ptr<Ship::IResource>& res, uint32_t type) {
    return res && res->GetInitData() && res->GetInitData()->Type == type && res->GetRawPointer();
}
// Checks one OoT skin limb and counts the skin data the shadow copies. MM's Skin code indexes the vertex buffer, the
// limb matrices and each modification's transformations with these values, so each is checked against its bound.
inline bool CheckAdultEponaLimb(const ::SkinLimb* limb, size_t& modifs, size_t& vertices, size_t& transformations) {
    if (limb == nullptr || (limb->child != 0xFF && limb->child >= kAdultEponaLimbs) ||
        (limb->sibling != 0xFF && limb->sibling >= kAdultEponaLimbs))
        return false;
    if (limb->segmentType == 0 || limb->segmentType == 5)
        return true; // OoT's empty skin limbs draw nothing.
    if (limb->segmentType == SKIN_LIMB_TYPE_NORMAL)
        return IsOotPath(limb->segment);
    if (limb->segmentType != SKIN_LIMB_TYPE_ANIMATED)
        return false;
    auto data = (const ::SkinAnimatedLimbData*)limb->segment;
    if (data == nullptr || !IsOotPath(data->dlist) || (data->limbModifCount != 0 && data->limbModifications == nullptr))
        return false;
    for (u16 m = 0; m < data->limbModifCount; ++m) {
        const ::SkinLimbModif& modif = data->limbModifications[m];
        if (modif.transformCount == 0 || modif.limbTransformations == nullptr || modif.unk_04 >= modif.transformCount ||
            (modif.vtxCount != 0 && modif.skinVertices == nullptr))
            return false;
        for (u16 v = 0; v < modif.vtxCount; ++v)
            if (modif.skinVertices[v].index >= data->totalVtxCount)
                return false;
        for (u16 t = 0; t < modif.transformCount; ++t)
            if (modif.limbTransformations[t].limbIndex >= kAdultEponaLimbs)
                return false;
        vertices += modif.vtxCount;
        transformations += modif.transformCount;
    }
    modifs += data->limbModifCount;
    return true;
}
inline bool BuildAdultEponaShadow(const OotSkeletonHeader* oot, AdultEponaShadow& shadow,
                                  std::vector<std::string>& displayLists) {
    size_t modifs = 0, vertices = 0, transformations = 0;
    for (int i = 0; i < kAdultEponaLimbs; ++i)
        if (!CheckAdultEponaLimb((const ::SkinLimb*)oot->segment[i], modifs, vertices, transformations))
            return false;
    // En_Horse sets the eye texture before limb 13 and rides on limb 30 (z_en_horse.c EnHorse_OverrideLimbDraw,
    // EnHorse_PostDraw); both must be display-list limbs, as in OoT.
    if (((const ::SkinLimb*)oot->segment[13])->segmentType != SKIN_LIMB_TYPE_NORMAL ||
        ((const ::SkinLimb*)oot->segment[30])->segmentType != SKIN_LIMB_TYPE_NORMAL)
        return false;
    shadow.modifs.resize(modifs);
    shadow.vertices.resize(vertices);
    shadow.transformations.resize(transformations);
    size_t m = 0, v = 0, t = 0;
    for (int i = 0; i < kAdultEponaLimbs; ++i) {
        ::SkinLimb& limb = shadow.limbs[i];
        limb = *(const ::SkinLimb*)oot->segment[i];
        shadow.table[i] = &limb;
        const void* path = limb.segmentType == SKIN_LIMB_TYPE_NORMAL ? limb.segment
                           : limb.segmentType == SKIN_LIMB_TYPE_ANIMATED
                               ? (const void*)((const ::SkinAnimatedLimbData*)limb.segment)->dlist
                               : nullptr;
        if (path == nullptr) {
            limb.segment = nullptr;
            continue;
        }
        displayLists.push_back((const char*)path + 7);
        shadow.displayLists[i] = "__OTR__@oot:" + displayLists.back();
        Gfx* routed = (Gfx*)shadow.displayLists[i].c_str();
        if ((uintptr_t)routed & 1)
            return false; // ResourceMgr_OTRSigCheck only reads a path at an even address.
        if (limb.segmentType == SKIN_LIMB_TYPE_NORMAL) {
            limb.segment = routed;
            continue;
        }
        const auto& data = *(const ::SkinAnimatedLimbData*)limb.segment;
        ::SkinAnimatedLimbData& copy = shadow.animated[i];
        copy = data;
        copy.dlist = routed;
        copy.limbModifications = shadow.modifs.data() + m;
        for (u16 j = 0; j < data.limbModifCount; ++j, ++m) {
            const ::SkinLimbModif& modif = data.limbModifications[j];
            shadow.modifs[m] = modif;
            std::copy_n(modif.skinVertices, modif.vtxCount, shadow.vertices.begin() + v);
            std::copy_n(modif.limbTransformations, modif.transformCount, shadow.transformations.begin() + t);
            shadow.modifs[m].skinVertices = shadow.vertices.data() + v;
            shadow.modifs[m].limbTransformations = shadow.transformations.data() + t;
            v += modif.vtxCount;
            t += modif.transformCount;
        }
        limb.segment = &copy;
    }
    shadow.header.segment = shadow.table.data();
    shadow.header.limbCount = kAdultEponaLimbs;
    return true;
}
inline bool LoadAdultEpona() {
    auto oot = Ship::CrossRMRegistry::Get("oot");
    if (!oot)
        return false;
    if (sAdultEponaOwner == oot)
        return true;
    // An archive without OoT's Epona stays without it, so a failed check is not repeated for the same OoT manager.
    if (sAdultEponaFailed.lock() == oot)
        return false;
    auto fail = [&](const char* what) {
        SPDLOG_WARN("Adult Epona: OoT's object_horse {} failed its check; adult Link keeps MM's Epona", what);
        sAdultEponaFailed = oot;
        return false;
    };
    auto shadow = std::make_unique<AdultEponaShadow>();
    std::vector<std::shared_ptr<Ship::IResource>> loaded;
    std::array<::AnimationHeader*, 9> animations{};
    {
        // OoT's skeleton factory resolves its limbs through the active manager and keeps a null for a miss, so the
        // skeleton is only ever loaded with OoT's manager active (feature plan F5b 1.8).
        Ship::ResourceManagerScope scope(oot);
        auto skeleton = oot->LoadResourceProcess(sAdultEponaSkelName);
        if (!IsResourceType(skeleton, static_cast<uint32_t>(SOH::ResourceType::SOH_Skeleton)))
            return fail("skeleton");
        auto header = (const OotSkeletonHeader*)skeleton->GetRawPointer();
        if (header->skeletonType != 0 || header->limbCount != kAdultEponaLimbs || header->segment == nullptr)
            return fail("skeleton");
        loaded.push_back(skeleton);
        std::vector<std::string> displayLists;
        if (!BuildAdultEponaShadow(header, *shadow, displayLists))
            return fail("skin limbs");
        for (const auto& name : displayLists) {
            loaded.push_back(oot->LoadResourceProcess(name));
            if (!IsResourceType(loaded.back(), static_cast<uint32_t>(Fast::ResourceType::DisplayList)))
                return fail("display lists");
        }
        for (size_t i = 0; i < animations.size(); ++i) {
            loaded.push_back(oot->LoadResourceProcess(sAdultEponaAnimNames[i]));
            if (!IsResourceType(loaded.back(), static_cast<uint32_t>(SOH::ResourceType::SOH_Animation)))
                return fail("animations");
            animations[i] = (::AnimationHeader*)loaded.back()->GetRawPointer();
            if (animations[i]->common.frameCount != sAdultEponaAnimFrames[i] || !animations[i]->frameData ||
                !animations[i]->jointIndices)
                return fail("animations");
        }
        for (const char* path : sAdultEponaEyePaths) {
            loaded.push_back(oot->LoadResourceProcess(path + 12));
            if (!IsResourceType(loaded.back(), static_cast<uint32_t>(Fast::ResourceType::Texture)))
                return fail("eye textures");
            auto texture = std::static_pointer_cast<Fast::Texture>(loaded.back());
            if (texture->Width != 32 || texture->Height != 16)
                return fail("eye textures");
        }
    }
    if (sAdultEponaOwner)
        DropAdultEpona();
    sAdultEponaOwner = oot;
    sAdultEponaShadow = std::move(shadow);
    sAdultEponaResources = std::move(loaded);
    sAdultEponaAnimations = animations;
    if (!sAdultEponaListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultEpona);
        sAdultEponaListener = true;
    }
    return true;
}
} // namespace ComboAdultMM
// Epona is ready when OoT's own Epona loads and checks out. It needs only OoT's manager, so it also answers before MM
// has booted; Combo_MM_CrossAgeReady ANDs it in (feature plan F5b 5b-9, decision D4).
extern "C" int Combo_MM_AdultEponaResourcesReady(void) {
    return ComboAdultMM::LoadAdultEpona();
}
// En_Horse's typed bindings: 0 the shadow skeleton, 1-9 the animations in ENHORSE_ANIM_* order, 10-12 the routed eyes.
extern "C" void* Combo_MM_GetAdultEponaBinding(int index) {
    if (index < 0 || index >= 13 || !ComboAdultMM::LoadAdultEpona())
        return nullptr;
    if (index == 0)
        return &ComboAdultMM::sAdultEponaShadow->header;
    if (index >= 10)
        return const_cast<char*>(ComboAdultMM::sAdultEponaEyePaths[index - 10]);
    return ComboAdultMM::sAdultEponaAnimations[index - 1];
}
