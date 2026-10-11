#pragma once
// Adult Link in MM drawn with OoT's own adult model from the OoT archive the app already extracts (owner ruling; plan
// F5b 1.1-1.8). OoT's gLinkAdultSkel has the limb tree of MM's human and Fierce Deity skeletons (MM already animates
// it with its human animations as the Fierce Deity), so MM draws it through a shadow skeleton whose limbs route their
// display lists to OoT. Only names live here: the skeleton, display lists and textures stay in the OoT archive and
// MM's items in MM's own.
#include "ComboAdultOotModel.h"
#include "ComboAdultOotSlots.h"
#include "2s2h/resource/type/2shResourceType.h"
#include <fast/resource/ResourceType.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#include <array>
#include <cstddef>
#include <iterator>
#include <string>
#include <vector>
extern "C" s32 Combo_MM_ApplyAdultModelTables(s32 adult);
extern "C" void Combo_MM_RestoreChildModelTables(void);
namespace ComboAdultMM {
static const char kAdultSkeleton[] = "objects/object_link_boy/gLinkAdultSkel";
static const char kFierceDeitySkeleton[] = "objects/object_link_boy/gLinkFierceDeitySkel";
// OoT's adult display lists for MM's human slots, in ComboAdultOotSlot order (plan F5b 1.3; OoT's own tables at
// soh/src/code/z_player_lib.c:268-273, 380-385 and 398-401 pick the same lists).
static const char* const kAdultBodyDLs[COMBO_ADULT_BODY_MAX] = {
    "objects/object_link_boy/gLinkAdultWaistNearDL",
    "objects/object_link_boy/gLinkAdultWaistFarDL",
    "objects/object_link_boy/gLinkAdultLeftHandNearDL",
    "objects/object_link_boy/gLinkAdultLeftHandFarDL",
    "objects/object_link_boy/gLinkAdultLeftHandClosedNearDL",
    "objects/object_link_boy/gLinkAdultLeftHandClosedFarDL",
    "objects/object_link_boy/gLinkAdultRightHandNearDL",
    "objects/object_link_boy/gLinkAdultRightHandFarDL",
    "objects/object_link_boy/gLinkAdultRightHandClosedNearDL",
    "objects/object_link_boy/gLinkAdultRightHandClosedFarDL",
    "objects/object_link_boy/gLinkAdultRightHandHoldingOotNearDL",
    "objects/object_link_boy/gLinkAdultRightHandHoldingOotFarDL",
    "objects/object_link_boy/gLinkAdultLeftHandOutNearDL",
    "objects/object_link_boy/gLinkAdultRightShoulderNearDL",
};
// OoT's adult eyes and mouths, index for index with MM's human rows (sPlayerEyesTextures, sPlayerMouthTextures).
static const char* const kAdultFaceTextures[COMBO_ADULT_EYE_TEXTURES + COMBO_ADULT_MOUTH_TEXTURES] = {
    "objects/object_link_boy/gLinkAdultEyesOpenTex",      "objects/object_link_boy/gLinkAdultEyesHalfTex",
    "objects/object_link_boy/gLinkAdultEyesClosedfTex",   "objects/object_link_boy/gLinkAdultEyesRollLeftTex",
    "objects/object_link_boy/gLinkAdultEyesRollRightTex", "objects/object_link_boy/gLinkAdultEyesShockTex",
    "objects/object_link_boy/gLinkAdultEyesUnk1Tex",      "objects/object_link_boy/gLinkAdultEyesUnk2Tex",
    "objects/object_link_boy/gLinkAdultMouth1Tex",        "objects/object_link_boy/gLinkAdultMouth2Tex",
    "objects/object_link_boy/gLinkAdultMouth3Tex",        "objects/object_link_boy/gLinkAdultMouth4Tex",
};
// MM's own items for OoT's adult hands (lead decision D1): the items of MM's "hand holding X" lists.
enum AdultMmItem {
    ADULT_MM_ITEM_NONE = -1,
    ADULT_MM_KOKIRI_SWORD,
    ADULT_MM_RAZOR_SWORD,
    ADULT_MM_GILDED_SWORD_HANDLE,
    ADULT_MM_GILDED_SWORD_BLADE,
    ADULT_MM_GREAT_FAIRYS_SWORD,
    ADULT_MM_HEROS_SHIELD,
    ADULT_MM_MIRROR_SHIELD,
    ADULT_MM_BOW,
    ADULT_MM_HOOKSHOT,
    ADULT_MM_ITEM_MAX,
};
static const char* const kAdultMmItemDLs[ADULT_MM_ITEM_MAX] = {
    "objects/gameplay_keep/gKokiriSwordDL",
    "objects/gameplay_keep/gRazorSwordDL",
    "objects/object_link_child/gLinkHumanGildedSwordHandleDL",
    "objects/object_link_child/gLinkHumanGildedSwordBladeDL",
    "objects/object_link_child/gLinkHumanGreatFairysSwordDL",
    "objects/object_link_child/gLinkHumanHerosShieldDL",
    "objects/object_link_child/gLinkHumanMirrorShieldDL",
    "objects/object_link_child/gLinkHumanBowDL",
    "objects/object_link_child/gLinkHumanHookshotDL",
};
// Composite slots, from COMBO_ADULT_BODY_MAX on: an OoT adult hand, then MM's item(s), as MM's own lists hold the
// child hand and the item.
struct AdultComposite {
    ComboAdultOotSlot hand;
    AdultMmItem items[2];
};
static const AdultComposite kAdultComposites[COMBO_ADULT_SLOT_MAX - COMBO_ADULT_BODY_MAX] = {
    { COMBO_ADULT_LEFT_HAND_CLOSED_NEAR, { ADULT_MM_KOKIRI_SWORD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_LEFT_HAND_CLOSED_FAR, { ADULT_MM_KOKIRI_SWORD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_LEFT_HAND_CLOSED_NEAR, { ADULT_MM_RAZOR_SWORD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_LEFT_HAND_CLOSED_FAR, { ADULT_MM_RAZOR_SWORD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_LEFT_HAND_CLOSED_NEAR, { ADULT_MM_GILDED_SWORD_HANDLE, ADULT_MM_GILDED_SWORD_BLADE } },
    { COMBO_ADULT_LEFT_HAND_CLOSED_FAR, { ADULT_MM_GILDED_SWORD_HANDLE, ADULT_MM_GILDED_SWORD_BLADE } },
    { COMBO_ADULT_LEFT_HAND_CLOSED_NEAR, { ADULT_MM_GREAT_FAIRYS_SWORD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_LEFT_HAND_CLOSED_FAR, { ADULT_MM_GREAT_FAIRYS_SWORD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_NEAR, { ADULT_MM_HEROS_SHIELD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_FAR, { ADULT_MM_HEROS_SHIELD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_NEAR, { ADULT_MM_MIRROR_SHIELD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_FAR, { ADULT_MM_MIRROR_SHIELD, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_NEAR, { ADULT_MM_BOW, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_FAR, { ADULT_MM_BOW, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_NEAR, { ADULT_MM_HOOKSHOT, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_FAR, { ADULT_MM_HOOKSHOT, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_NEAR, { ADULT_MM_BOW, ADULT_MM_ITEM_NONE } },
    { COMBO_ADULT_RIGHT_HAND_CLOSED_NEAR, { ADULT_MM_HOOKSHOT, ADULT_MM_ITEM_NONE } },
};
// OoT's skeleton header as SoH's factory fills it (soh/soh/resource/type/Skeleton.h): MM's FlexSkeletonHeader plus
// SoH's skeletonType byte in the padding after limbCount.
struct AdultOotSkeletonView {
    void** segment;
    uint8_t limbCount;
    uint8_t skeletonType; // SOH::SkeletonType: 1 is Flex
};
static_assert(offsetof(FlexSkeletonHeader, dListCount) == sizeof(AdultOotSkeletonView), "SoH flex header layout");
static_assert(ComboAdultOotModel::kOpDlFilePath == G_DL_OTR_FILEPATH && ComboAdultOotModel::kOpEndDl == G_ENDDL &&
                  ComboAdultOotModel::kOpMarker == G_MARKER && ComboAdultOotModel::kOpPipeSync == G_RDPPIPESYNC &&
                  ComboAdultOotModel::kOpTexture == G_TEXTURE && G_DL_PUSH == 0,
              "libultraship opcodes");
constexpr size_t kAdultLimbs = PLAYER_LIMB_MAX - 1, kAdultDListLimbs = 18;
// Model state. OoT's manager stays resident from boot to deinit (OTRGlobals.cpp), so the resources are checked once
// per manager; a failure is not retried for the same manager. Every pin is dropped at teardown.
static std::shared_ptr<Ship::ResourceManager> sAdultOotOwner, sAdultMmOwner;
static std::weak_ptr<Ship::ResourceManager> sAdultOotFailed, sAdultMmFailed;
static std::vector<std::shared_ptr<Ship::IResource>> sAdultOotPins, sAdultMmPins;
static bool sAdultOotListener = false, sAdultShadowBuilt = false;
// Storage MM owns, alive until the process ends because Player's SkelAnime keeps the limb pointers. OoT's cached
// limbs are only read.
static FlexSkeletonHeader sAdultShadowSkeleton;
static std::array<LodLimb, kAdultLimbs> sAdultShadowLimbs;
static std::array<void*, kAdultLimbs> sAdultShadowLimbTable;
static std::vector<std::string> sAdultShadowPaths;
static std::array<std::string, COMBO_ADULT_BODY_MAX> sAdultBodyPaths;
static std::array<std::string, COMBO_ADULT_EYE_TEXTURES + COMBO_ADULT_MOUTH_TEXTURES> sAdultFacePaths;
static std::array<std::string, ADULT_MM_ITEM_MAX> sAdultMmItemPaths;
static Gfx sAdultComposites[COMBO_ADULT_SLOT_MAX - COMBO_ADULT_BODY_MAX][4];

inline bool AdultResourceIs(const std::shared_ptr<Ship::IResource>& resource, uint32_t type) {
    // Types are compared by tag, not dynamic_cast: the resource may belong to the other game's module.
    return resource && resource->GetInitData() && resource->GetInitData()->Type == type;
}
inline bool AdultPathAligned(const std::string& path) {
    // Paths in display lists and segments must be even (ResourceMgr_OTRSigCheck, the interpreter's SegAddr).
    return !(reinterpret_cast<uintptr_t>(path.c_str()) & 1);
}
inline void DropAdultOotModel() {
    // MM's child tables go back before anything they point into is released.
    Combo_MM_RestoreChildModelTables();
    sAdultOotPins.clear();
    sAdultMmPins.clear();
    sAdultOotOwner.reset();
    sAdultMmOwner.reset();
}
inline bool RouteAdultPath(const char* name, const char* game, std::string& out) {
    return ComboAdultOotModel::RoutePath(("__OTR__" + std::string(name)).c_str(), game, out) && AdultPathAligned(out);
}
// Routed paths and composite lists. They hold names only and resolve at draw time, so they are built once.
inline bool BuildAdultPaths() {
    static int built = 0;
    if (built)
        return built > 0;
    built = -1;
    for (size_t i = 0; i < sAdultBodyPaths.size(); ++i)
        if (!RouteAdultPath(kAdultBodyDLs[i], "oot", sAdultBodyPaths[i]))
            return false;
    for (size_t i = 0; i < sAdultFacePaths.size(); ++i)
        if (!RouteAdultPath(kAdultFaceTextures[i], "oot", sAdultFacePaths[i]))
            return false;
    for (size_t i = 0; i < sAdultMmItemPaths.size(); ++i)
        if (!RouteAdultPath(kAdultMmItemDLs[i], "mm", sAdultMmItemPaths[i]))
            return false;
    for (size_t i = 0; i < std::size(kAdultComposites); ++i) {
        const AdultComposite& recipe = kAdultComposites[i];
        const char* items[2];
        size_t count = 0;
        for (AdultMmItem item : recipe.items)
            if (item != ADULT_MM_ITEM_NONE)
                items[count++] = sAdultMmItemPaths[item].c_str();
        if (!ComboAdultOotModel::BuildComposite(sAdultComposites[i], std::size(sAdultComposites[i]),
                                                sAdultBodyPaths[recipe.hand].c_str(), items, count))
            return false;
    }
    built = 1;
    return true;
}
// Loads and checks OoT's adult model in OoT's manager. The skeleton is loaded only inside a scope that makes OoT's
// manager the active one, because SoH's skeleton factory resolves its limbs through the active manager and caches a
// missing limb as null (SkeletonFactory.cpp:48-51). Call it on the game thread at a quiet point (the menu, or
// Player_Init during Play_Init), never mid-draw. Display lists and textures load nothing nested.
inline bool LoadAdultOotModel() {
    auto oot = Ship::CrossRMRegistry::Get("oot");
    if (!oot || sAdultOotFailed.lock() == oot)
        return false;
    if (sAdultOotOwner == oot)
        return true;
    auto fail = [&]() {
        sAdultOotFailed = oot;
        return false;
    };
    if (!BuildAdultPaths())
        return fail();
    std::vector<std::shared_ptr<Ship::IResource>> pins;
    std::shared_ptr<Ship::IResource> skeleton;
    {
        Ship::ResourceManagerScope scope(oot);
        skeleton = oot->LoadResourceProcess(kAdultSkeleton);
    }
    if (!AdultResourceIs(skeleton, static_cast<uint32_t>(SOH::ResourceType::SOH_Skeleton)))
        return fail();
    pins.push_back(skeleton);
    const auto* header = static_cast<const AdultOotSkeletonView*>(skeleton->GetRawPointer());
    if (!header || !header->segment || header->skeletonType != 1 || header->limbCount != kAdultLimbs ||
        static_cast<const FlexSkeletonHeader*>(skeleton->GetRawPointer())->dListCount != kAdultDListLimbs)
        return fail();
    std::array<const LodLimb*, kAdultLimbs> limbs;
    for (size_t i = 0; i < kAdultLimbs; ++i)
        limbs[i] = static_cast<const LodLimb*>(header->segment[i]);
    if (!ComboAdultOotModel::ValidLodTree(limbs.data(), kAdultLimbs, kAdultDListLimbs))
        return fail();
    // Every limb list must be one of OoT's adult lists, resolvable in OoT's manager.
    static const char kLimbPrefix[] = "__OTR__objects/object_link_boy/";
    for (const LodLimb* limb : limbs)
        for (Gfx* list : limb->dLists) {
            const char* path = reinterpret_cast<const char*>(list);
            if (!path)
                continue;
            if (strncmp(path, kLimbPrefix, sizeof(kLimbPrefix) - 1))
                return fail();
            auto resource = oot->LoadResourceProcess(path + 7);
            if (!AdultResourceIs(resource, static_cast<uint32_t>(Fast::ResourceType::DisplayList)))
                return fail();
            pins.push_back(std::move(resource));
        }
    for (const char* name : kAdultBodyDLs) {
        auto resource = oot->LoadResourceProcess(name);
        if (!AdultResourceIs(resource, static_cast<uint32_t>(Fast::ResourceType::DisplayList)))
            return fail();
        pins.push_back(std::move(resource));
    }
    for (const char* name : kAdultFaceTextures) {
        auto resource = oot->LoadResourceProcess(name);
        if (!AdultResourceIs(resource, static_cast<uint32_t>(Fast::ResourceType::Texture)))
            return fail();
        pins.push_back(std::move(resource));
    }
    if (!sAdultShadowBuilt) {
        // The shadow skeleton: OoT's joints and tree, its limb lists routed to OoT, owned by MM. It names lists
        // rather than pointing into OoT's cache, so it stays valid for the life of the process.
        std::vector<std::string> paths;
        if (!ComboAdultOotModel::BuildShadowLimbs(limbs.data(), kAdultLimbs, "oot", sAdultShadowLimbs.data(), paths))
            return fail();
        for (const auto& path : paths)
            if (!AdultPathAligned(path))
                return fail();
        sAdultShadowPaths = std::move(paths);
        for (size_t i = 0; i < kAdultLimbs; ++i)
            sAdultShadowLimbTable[i] = &sAdultShadowLimbs[i];
        sAdultShadowSkeleton.sh.segment = sAdultShadowLimbTable.data();
        sAdultShadowSkeleton.sh.limbCount = kAdultLimbs;
        sAdultShadowSkeleton.dListCount = kAdultDListLimbs;
        sAdultShadowBuilt = true;
    } else {
        for (size_t i = 0; i < kAdultLimbs; ++i)
            if (limbs[i]->child != sAdultShadowLimbs[i].child || limbs[i]->sibling != sAdultShadowLimbs[i].sibling)
                return fail();
    }
    sAdultOotOwner = oot;
    sAdultOotPins = std::move(pins);
    if (!sAdultOotListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(DropAdultOotModel);
        sAdultOotListener = true;
    }
    return true;
}
// MM's side, checked once MM runs: MM's own items, and MM's Fierce Deity skeleton, whose limb tree OoT's adult
// skeleton must share for MM's human animations to fit.
inline bool PrepareAdultMm() {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    if (!mm || !sAdultShadowBuilt || sAdultMmFailed.lock() == mm)
        return false;
    if (sAdultMmOwner == mm)
        return true;
    auto fail = [&]() {
        sAdultMmFailed = mm;
        return false;
    };
    std::vector<std::shared_ptr<Ship::IResource>> pins;
    for (const char* name : kAdultMmItemDLs) {
        auto resource = mm->LoadResourceProcess(name);
        if (!AdultResourceIs(resource, static_cast<uint32_t>(Fast::ResourceType::DisplayList)))
            return fail();
        pins.push_back(std::move(resource));
    }
    std::shared_ptr<Ship::IResource> fierceDeity;
    {
        Ship::ResourceManagerScope scope(mm);
        fierceDeity = mm->LoadResourceProcess(kFierceDeitySkeleton);
    }
    if (!AdultResourceIs(fierceDeity, static_cast<uint32_t>(SOH::ResourceType::SOH_Skeleton)))
        return fail();
    const auto* header = static_cast<const FlexSkeletonHeader*>(fierceDeity->GetRawPointer());
    if (!header || !header->sh.segment || header->sh.limbCount != kAdultLimbs)
        return fail();
    for (size_t i = 0; i < kAdultLimbs; ++i) {
        const auto* limb = static_cast<const LodLimb*>(header->sh.segment[i]);
        if (!limb || limb->child != sAdultShadowLimbs[i].child || limb->sibling != sAdultShadowLimbs[i].sibling)
            return fail();
    }
    pins.push_back(std::move(fierceDeity));
    sAdultMmOwner = mm;
    sAdultMmPins = std::move(pins);
    return true;
}
inline bool AdultMmFailed() {
    auto mm = Ship::CrossRMRegistry::Get("mm");
    return mm && sAdultMmFailed.lock() == mm;
}
} // namespace ComboAdultMM
// OoT's adult model is in OoT's manager and checked. It needs nothing from MM, so it can answer before MM has booted;
// it turns false for good if MM's side later fails its check.
extern "C" int Combo_MM_CrossAgeModelReady(void) {
    return ComboAdultMM::LoadAdultOotModel() && !ComboAdultMM::AdultMmFailed();
}
// Both sides checked, before the human tables take the model (Player_Init).
extern "C" int Combo_MM_PrepareCrossAgeModel(void) {
    return ComboAdultMM::LoadAdultOotModel() && ComboAdultMM::PrepareAdultMm();
}
extern "C" FlexSkeletonHeader* Combo_MM_CrossAgeSkeleton(void) {
    return ComboAdultMM::sAdultShadowBuilt ? &ComboAdultMM::sAdultShadowSkeleton : nullptr;
}
// The list for a human slot (ComboAdultOotSlot): a routed OoT list or a composite; nothing for an empty slot.
extern "C" Gfx* Combo_MM_CrossAgeSlotDL(int slot) {
    if (!ComboAdultMM::BuildAdultPaths())
        return nullptr;
    if (slot >= 0 && slot < COMBO_ADULT_BODY_MAX)
        return (Gfx*)ComboAdultMM::sAdultBodyPaths[slot].c_str();
    if (slot >= COMBO_ADULT_BODY_MAX && slot < COMBO_ADULT_SLOT_MAX)
        return ComboAdultMM::sAdultComposites[slot - COMBO_ADULT_BODY_MAX];
    return nullptr;
}
// OoT's eye (0-7) and mouth (8-11) textures, routed to OoT.
extern "C" TexturePtr Combo_MM_CrossAgeFaceTexture(int index) {
    if (!ComboAdultMM::BuildAdultPaths() || index < 0 || index >= COMBO_ADULT_EYE_TEXTURES + COMBO_ADULT_MOUTH_TEXTURES)
        return nullptr;
    return (TexturePtr)ComboAdultMM::sAdultFacePaths[index].c_str();
}
