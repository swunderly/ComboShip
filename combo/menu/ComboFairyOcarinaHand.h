#pragma once
#include "ComboFairyOcarinaGeometry.h"
#include "ComboFairyOcarinaFit.h"
#include "ComboFairyOcarinaRecolour.h"
#include "2s2h/resource/type/2shResourceType.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Texture.h>
#include <fast/resource/ResourceType.h>
#include <fast/lus_gbi.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <ship/utils/StrHash64.h>
static std::shared_ptr<Ship::IResource> sFairyInstrumentSource;
static std::shared_ptr<Fast::DisplayList> sFairyInstrumentCopy;
static std::weak_ptr<Ship::ResourceManager> sFairyInstrumentOwner;
static bool sFairyInstrumentListener;
// B3: the fitted placement for the current OoT manager, or the manager it could not be fitted for (once each).
static Mtx sFairyPlacementMtx;
static std::weak_ptr<Ship::ResourceManager> sFairyPlacementOwner, sFairyPlacementFailed;
// The recoloured hands (near, far) for the current OoT manager with the OoT lists they copy, or the manager they could
// not be built for (once each).
static std::array<std::shared_ptr<Fast::DisplayList>, 2> sFairyRecolouredSources, sFairyRecolouredCopies;
static std::weak_ptr<Ship::ResourceManager> sFairyRecolouredOwner, sFairyRecolouredFailed;
static void Combo_DropFairyInstrument() {
    sFairyInstrumentCopy.reset();
    sFairyInstrumentSource.reset();
    sFairyInstrumentOwner.reset();
    sFairyPlacementOwner.reset();
    sFairyPlacementFailed.reset();
    sFairyRecolouredCopies = {};
    sFairyRecolouredSources = {};
    sFairyRecolouredOwner.reset();
    sFairyRecolouredFailed.reset();
}
static void Combo_ListenFairyInstrumentTeardown() {
    if (!sFairyInstrumentListener) {
        Ship::CrossRMRegistry::RegisterTeardownListener(Combo_DropFairyInstrument);
        sFairyInstrumentListener = true;
    }
}
static const char* Combo_FairyInstrumentPath() {
    auto rm = Ship::CrossRMRegistry::Get("oot");
    if (!rm)
        return nullptr;
    if (sFairyInstrumentOwner.lock() != rm) {
        auto source = std::dynamic_pointer_cast<Fast::DisplayList>(
            rm->LoadResource("objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaNearDL"));
        if (!source)
            return nullptr;
        auto copy = std::make_shared<Fast::DisplayList>(source->GetInitData());
        copy->UCode = source->UCode;
        if (!ComboRando::StripPinnedFairyOcarinaHand(source->Instructions, copy->Instructions, G_MARKER,
                                                     G_SETTIMG_OTR_HASH, G_VTX_OTR_HASH, G_DL_OTR_HASH))
            return nullptr;
        rm->CacheExternalResource("combo/runtime/fairy_ocarina_instrument", copy);
        sFairyInstrumentSource = source;
        sFairyInstrumentCopy = copy;
        sFairyInstrumentOwner = rm;
        Combo_ListenFairyInstrumentTeardown();
    }
    // Runtime resources have no archive backing. Reinsert after cache eviction, retaining
    // source-owned pointer payloads until registry teardown drops both references.
    rm->CacheExternalResource("combo/runtime/fairy_ocarina_instrument", sFairyInstrumentCopy);
    return "__OTR__@oot:combo/runtime/fairy_ocarina_instrument";
}
static_assert(ComboFairyOcarinaFit::kOpTri1 == G_TRI1 && ComboFairyOcarinaFit::kOpTri2 == G_TRI2 &&
                  ComboFairyOcarinaFit::kOpQuad == G_QUAD &&
                  ComboFairyOcarinaFit::kOpSetTImgHash == G_SETTIMG_OTR_HASH &&
                  ComboFairyOcarinaFit::kOpVtxHash == G_VTX_OTR_HASH && ComboFairyOcarinaFit::kOpMarker == G_MARKER &&
                  ComboFairyOcarinaFit::kOpTexture == G_TEXTURE &&
                  ComboFairyOcarinaFit::kOpGeometryMode == G_GEOMETRYMODE && ComboFairyOcarinaFit::kOpDl == G_DL &&
                  ComboFairyOcarinaFit::kOpEndDl == G_ENDDL &&
                  ComboFairyOcarinaFit::kVertexBytes == sizeof(Fast::F3DVtx),
              "libultraship opcodes and vertex size");
// OoT's lists the fit reads (ComboFairyOcarinaFit::FitFairyPlacement), and the Ocarina of Time texture that marks the
// instrument in OoT's child and adult hands.
static const char* const kFairyPlacementLists[4] = {
    "objects/object_link_child/gLinkChildRightHandAndOotNearDL",
    "objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaNearDL",
    "objects/object_link_boy/gLinkAdultRightHandHoldingOotNearDL",
    "objects/object_link_boy/gLinkAdultRightHandNearDL",
};
static const char kFairyPlacementInstrumentTexture[] = "objects/gameplay_keep/gOcarinaofTimeDesignTex";
// The vertex loads of one of OoT's lists with their positions. Display lists and vertex arrays load nothing nested. A
// custom (mod) list or array keeps its own geometry and is not fitted.
static bool Combo_FairyPlacementLoads(Ship::ResourceManager& rm, const char* name,
                                      std::vector<ComboFairyOcarinaFit::VertexLoad>& loads) {
    auto list = rm.LoadResourceProcess(name);
    if (!list || !list->GetInitData() || list->GetInitData()->IsCustom ||
        list->GetInitData()->Type != static_cast<uint32_t>(Fast::ResourceType::DisplayList))
        return false;
    const auto& commands = std::static_pointer_cast<Fast::DisplayList>(list)->Instructions;
    if (!ComboFairyOcarinaFit::ListVertexLoads(commands.data(), commands.size(), loads))
        return false;
    for (auto& load : loads) {
        const std::string* path = rm.GetArchiveManager()->HashToString(load.array);
        auto array = path ? rm.LoadResourceProcess(*path) : nullptr;
        if (!array || !array->GetInitData() || array->GetInitData()->IsCustom ||
            array->GetInitData()->Type != static_cast<uint32_t>(SOH::ResourceType::SOH_Array) ||
            !ComboFairyOcarinaFit::ReadPoints(static_cast<const uint8_t*>(array->GetRawPointer()),
                                              array->GetPointerSize(), load))
            return false;
    }
    return true;
}
// B3: the matrix that moves the Fairy item from the child's hand frame to adult Link's, fitted once per OoT manager
// from OoT's own data, or null when it cannot be fitted (the reason is logged once).
static Mtx* Combo_FairyInstrumentPlacement() {
    auto rm = Ship::CrossRMRegistry::Get("oot");
    if (!rm || sFairyPlacementFailed.lock() == rm)
        return nullptr;
    if (sFairyPlacementOwner.lock() == rm)
        return &sFairyPlacementMtx;
    Combo_ListenFairyInstrumentTeardown();
    std::array<std::vector<ComboFairyOcarinaFit::VertexLoad>, 4> lists;
    for (size_t i = 0; i < lists.size(); ++i)
        if (!Combo_FairyPlacementLoads(*rm, kFairyPlacementLists[i], lists[i])) {
            SPDLOG_WARN("[ComboShip AdultFairy] {} cannot be read; adult Link keeps the unmoved Fairy item",
                        kFairyPlacementLists[i]);
            sFairyPlacementFailed = rm;
            return nullptr;
        }
    ComboFairyOcarinaFit::RigidFit fit;
    const auto result = ComboFairyOcarinaFit::FitFairyPlacement(lists[0], lists[1], lists[2], lists[3],
                                                                CRC64(kFairyPlacementInstrumentTexture), fit);
    if (result != ComboFairyOcarinaFit::FitResult::Fitted) {
        SPDLOG_WARN("[ComboShip AdultFairy] no fit ({}; residual max {}); adult Link keeps the unmoved Fairy item",
                    ComboFairyOcarinaFit::FitResultName(result), fit.max);
        sFairyPlacementFailed = rm;
        return nullptr;
    }
    // Built here, not through SkinMatrix_MtxFToMtx: the matrix is constant, so it is not recorded for interpolation.
    float mf[4][4];
    ComboFairyOcarinaFit::PlacementMatrix(fit, mf);
    guMtxF2L(mf, &sFairyPlacementMtx);
    SPDLOG_INFO(
        "[ComboShip AdultFairy] fitted: rotation {:.2f} deg, translation {:.1f}/{:.1f}/{:.1f}, residual rms {:.2f} "
        "max {:.2f} (limit {:.2f})",
        ComboFairyOcarinaFit::RotationDegrees(fit), fit.translation[0], fit.translation[1], fit.translation[2], fit.rms,
        fit.max, ComboFairyOcarinaFit::RoundingBound());
    sFairyPlacementOwner = rm;
    return &sFairyPlacementMtx;
}
// 1 when the placement is fitted for the current OoT manager, -1 when it cannot be, 0 before it is tried (the probe).
extern "C" int Combo_MM_FairyPlacementState(void) {
    auto rm = Ship::CrossRMRegistry::Get("oot");
    return !rm ? 0 : sFairyPlacementOwner.lock() == rm ? 1 : sFairyPlacementFailed.lock() == rm ? -1 : 0;
}
static_assert(ComboFairyOcarinaRecolour::kOpSetTImgHash == G_SETTIMG_OTR_HASH &&
                  ComboFairyOcarinaRecolour::kOpDlHash == G_DL_OTR_HASH &&
                  ComboFairyOcarinaRecolour::kOpVtxHash == G_VTX_OTR_HASH &&
                  ComboFairyOcarinaRecolour::kOpMarker == G_MARKER &&
                  ComboFairyOcarinaRecolour::kOpBranchZHash == G_BRANCH_Z_OTR &&
                  ComboFairyOcarinaRecolour::kOpMtxHash == G_MTX_OTR &&
                  ComboFairyOcarinaRecolour::kOpMoveMemHash == G_MOVEMEM_OTR &&
                  ComboFairyOcarinaRecolour::kOpSetTileSize == G_SETTILESIZE &&
                  ComboFairyOcarinaRecolour::kOpEndDl == G_ENDDL,
              "libultraship opcodes");
// The recoloured hands: OoT's adult hands holding the Ocarina of Time, near and far, and the texture that colours the
// Fairy Ocarina in OoT's child hands, near and far, which show how OoT loads it (ComboFairyOcarinaRecolour.h). Each
// copy is cached in OoT's manager under its routed path's name; paths in display lists must be even
// (ResourceMgr_OTRSigCheck).
static const char* const kFairyRecolouredLists[2] = {
    "objects/object_link_boy/gLinkAdultRightHandHoldingOotNearDL",
    "objects/object_link_boy/gLinkAdultRightHandHoldingOotFarDL",
};
static const char* const kFairyRecolouredReferences[2] = {
    "objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaNearDL",
    "objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaFarDL",
};
static const char kFairyRecolouredTexture[] = "objects/object_link_child/gLinkChildFairyOcarinaTex";
alignas(2) static const char kFairyRecolouredNear[] = "__OTR__@oot:combo/runtime/adult_fairy_ocarina_hand_near";
alignas(2) static const char kFairyRecolouredFar[] = "__OTR__@oot:combo/runtime/adult_fairy_ocarina_hand_far";
static const char* const kFairyRecolouredPaths[2] = { kFairyRecolouredNear, kFairyRecolouredFar };
static constexpr size_t kFairyRecolouredRoute = sizeof("__OTR__@oot:") - 1;
// A texture from OoT's manager (a texture pack's replacement included), with its image, as the interpreter draws it.
static std::shared_ptr<Fast::Texture> Combo_FairyRecolourTexture(Ship::ResourceManager& rm, const char* name) {
    auto texture = rm.LoadResourceProcess(name);
    if (!texture || !texture->GetInitData() || texture->IsDirty() ||
        texture->GetInitData()->Type != static_cast<uint32_t>(Fast::ResourceType::Texture))
        return nullptr;
    auto typed = std::static_pointer_cast<Fast::Texture>(texture);
    return typed->ImageData ? typed : nullptr;
}
// A display list from OoT's manager, as served.
static std::shared_ptr<Fast::DisplayList> Combo_FairyRecolourList(Ship::ResourceManager& rm, const char* name) {
    auto list = rm.LoadResourceProcess(name);
    if (!list || !list->GetInitData() || list->IsDirty() ||
        list->GetInitData()->Type != static_cast<uint32_t>(Fast::ResourceType::DisplayList))
        return nullptr;
    return std::static_pointer_cast<Fast::DisplayList>(list);
}
// Builds both copies from OoT's manager, or says why not. Display lists and textures load nothing nested, so no
// manager scope is needed. A custom (mod) adult list is not copied, as for the fit; the child list is only compared,
// so it is read as served. A texture pack may replace either texture: each copy draws the Fairy Ocarina's texture as
// served, with the commands OoT's child hand draws it with, so the interpreter sizes a replacement as it does there.
// Nothing cached is changed.
static const char* Combo_BuildFairyRecolouredHands(Ship::ResourceManager& rm) {
    if (!Combo_FairyRecolourTexture(rm, kFairyRecolouredTexture))
        return "the Fairy Ocarina texture cannot be read";
    std::array<std::shared_ptr<Fast::DisplayList>, 2> sources, copies;
    for (size_t lod = 0; lod < sources.size(); ++lod) {
        sources[lod] = Combo_FairyRecolourList(rm, kFairyRecolouredLists[lod]);
        if (!sources[lod] || sources[lod]->GetInitData()->IsCustom)
            return "OoT's adult hand holding the Ocarina of Time cannot be read";
        copies[lod] = std::make_shared<Fast::DisplayList>(sources[lod]->GetInitData());
        copies[lod]->UCode = sources[lod]->UCode;
        if (!ComboFairyOcarinaRecolour::SwapTexture(sources[lod]->Instructions, copies[lod]->Instructions,
                                                    CRC64(kFairyPlacementInstrumentTexture),
                                                    CRC64(kFairyRecolouredTexture)))
            return "OoT's adult hand does not set the Ocarina of Time texture exactly once";
        auto reference = Combo_FairyRecolourList(rm, kFairyRecolouredReferences[lod]);
        if (!reference ||
            !ComboFairyOcarinaRecolour::SameLoad(sources[lod]->Instructions, CRC64(kFairyPlacementInstrumentTexture),
                                                 reference->Instructions, CRC64(kFairyRecolouredTexture)))
            return "OoT's child hand cannot be read or loads the Fairy Ocarina texture otherwise";
    }
    sFairyRecolouredSources = sources;
    sFairyRecolouredCopies = copies;
    return nullptr;
}
// The recoloured hand for `lod` (0 near, 1 far), routed to OoT, built once per OoT manager, or null when it cannot be
// built (the reason is logged once).
static const char* Combo_FairyRecolouredHandPath(s32 lod) {
    auto rm = Ship::CrossRMRegistry::Get("oot");
    if (!rm || lod < 0 || lod > 1 || sFairyRecolouredFailed.lock() == rm)
        return nullptr;
    if (sFairyRecolouredOwner.lock() != rm) {
        Combo_ListenFairyInstrumentTeardown();
        if (const char* reason = Combo_BuildFairyRecolouredHands(*rm)) {
            SPDLOG_WARN("[ComboShip AdultFairy] no recoloured hand ({}); adult Link keeps the Fairy item composite",
                        reason);
            sFairyRecolouredFailed = rm;
            return nullptr;
        }
        sFairyRecolouredOwner = rm;
    }
    // Runtime resources have no archive backing: reinsert after cache eviction, as for the instrument.
    rm->CacheExternalResource(kFairyRecolouredPaths[lod] + kFairyRecolouredRoute, sFairyRecolouredCopies[lod]);
    return kFairyRecolouredPaths[lod];
}
// 1 when the recoloured hands are built for the current OoT manager, -1 when they cannot be, 0 before they are tried.
extern "C" int Combo_MM_FairyRecolourState(void) {
    auto rm = Ship::CrossRMRegistry::Get("oot");
    return !rm ? 0 : sFairyRecolouredOwner.lock() == rm ? 1 : sFairyRecolouredFailed.lock() == rm ? -1 : 0;
}
extern "C" int Combo_MM_AdultFairyFittedPlacement(void);
extern "C" int Combo_MM_AdultFairyRecolouredOot(void);
extern "C" Gfx* Combo_MM_FairyOcarinaHand(PlayState* play, Gfx* nativeHand, s32 lod) {
    // Adult Link only (BenPort.cpp). B3, if switched on, wins over the recoloured hand.
    const bool fitted = Combo_MM_AdultFairyFittedPlacement();
    if (!fitted && Combo_MM_AdultFairyRecolouredOot()) {
        // OoT's own adult hand holding the Ocarina of Time, coloured like the Fairy Ocarina, drawn as the limb's list
        // as the native hand would be; where it cannot be built, the composite below.
        if (const char* recoloured = Combo_FairyRecolouredHandPath(lod))
            return (Gfx*)recoloured;
    }
    const char* instrument = Combo_FairyInstrumentPath();
    if (!instrument)
        return nullptr;
    // B3: the item moved by OoT's own placement; without a fit, the composite as before.
    Mtx* placement = fitted ? Combo_FairyInstrumentPlacement() : nullptr;
    Gfx* list = (Gfx*)GRAPH_ALLOC(play->state.gfxCtx, sizeof(Gfx) * (placement ? 5 : 3));
    if (!list)
        return nullptr;
    Gfx* d = list;
    if (placement) {
        // Item first, as in OoT's own child list, under the hand limb's matrix times the fit. The pop restores the limb
        // matrix before the hand, on every path: a route that fails to resolve skips only the item.
        gSPMatrix(d++, placement, G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        gDma1p(d++, G_DL_OTR_FILEPATH, instrument, 0, G_DL_PUSH);
        gSPPopMatrix(d++, G_MTX_MODELVIEW);
        gSPDisplayList(d++, nativeHand);
        gSPEndDisplayList(d++);
        return list; // fitted item + native hand, the matrix pushed and popped inside the limb
    }
    gSPDisplayList(d++, nativeHand);
    // Routed path call: the stripped copy's texture/vertex hashes must resolve in OoT's RM. A route
    // that fails to resolve is skipped, so the list still ends here instead of past its allocation.
    gDma1p(d++, G_DL_OTR_FILEPATH, instrument, 0, G_DL_PUSH);
    gSPEndDisplayList(d++);
    return list; // native hand + stripped instrument, same limb matrix, no fabricated transform
}
