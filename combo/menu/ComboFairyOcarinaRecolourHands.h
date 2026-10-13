#pragma once
// The recoloured hands (ComboFairyOcarinaRecolour.h) built from OoT's resource manager. Adult Link holding the Fairy
// Ocarina is drawn with them in both games, so it looks the same in each (owner ruling 2026-10-12): MM draws them
// through routed paths (ComboFairyOcarinaHand.h), OoT as the hand limb's list (OTRGlobals.cpp,
// Combo_OotFairyOcarinaHand). Each game's library builds and keeps its own copies with this code.
#include "ComboFairyOcarinaRecolour.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Texture.h>
#include <fast/resource/ResourceType.h>
#include <ship/resource/ResourceManager.h>
#include <ship/utils/StrHash64.h>
#include <array>
#include <memory>
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
// The recoloured hands: OoT's adult hands holding the Ocarina of Time, near and far, the texture that marks the
// instrument in them, and the texture that colours the Fairy Ocarina in OoT's child hands, near and far, which show how
// OoT loads it (ComboFairyOcarinaRecolour.h).
static const char* const kFairyRecolouredLists[2] = {
    "objects/object_link_boy/gLinkAdultRightHandHoldingOotNearDL",
    "objects/object_link_boy/gLinkAdultRightHandHoldingOotFarDL",
};
static const char* const kFairyRecolouredReferences[2] = {
    "objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaNearDL",
    "objects/object_link_child/gLinkChildRightHandHoldingFairyOcarinaFarDL",
};
static const char kFairyRecolouredInstrument[] = "objects/gameplay_keep/gOcarinaofTimeDesignTex";
static const char kFairyRecolouredTexture[] = "objects/object_link_child/gLinkChildFairyOcarinaTex";
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
// Builds both copies from OoT's manager into `copies`, with the OoT lists they copy in `sources`, or says why not and
// leaves both unchanged. Display lists and textures load nothing nested, so no manager scope is needed. A custom (mod)
// adult list is not copied, as for the fit; the child list is only compared, so it is read as served. A texture pack
// may replace either texture: each copy draws the Fairy Ocarina's texture as served, with the commands OoT's child hand
// draws it with, so the interpreter sizes a replacement as it does there. Nothing cached is changed.
static const char* Combo_BuildFairyRecolouredHands(Ship::ResourceManager& rm,
                                                   std::array<std::shared_ptr<Fast::DisplayList>, 2>& sources,
                                                   std::array<std::shared_ptr<Fast::DisplayList>, 2>& copies) {
    if (!Combo_FairyRecolourTexture(rm, kFairyRecolouredTexture))
        return "the Fairy Ocarina texture cannot be read";
    std::array<std::shared_ptr<Fast::DisplayList>, 2> adult, recoloured;
    for (size_t lod = 0; lod < adult.size(); ++lod) {
        adult[lod] = Combo_FairyRecolourList(rm, kFairyRecolouredLists[lod]);
        if (!adult[lod] || adult[lod]->GetInitData()->IsCustom)
            return "OoT's adult hand holding the Ocarina of Time cannot be read";
        recoloured[lod] = std::make_shared<Fast::DisplayList>(adult[lod]->GetInitData());
        recoloured[lod]->UCode = adult[lod]->UCode;
        if (!ComboFairyOcarinaRecolour::SwapTexture(adult[lod]->Instructions, recoloured[lod]->Instructions,
                                                    CRC64(kFairyRecolouredInstrument), CRC64(kFairyRecolouredTexture)))
            return "OoT's adult hand does not set the Ocarina of Time texture exactly once";
        auto reference = Combo_FairyRecolourList(rm, kFairyRecolouredReferences[lod]);
        if (!reference ||
            !ComboFairyOcarinaRecolour::SameLoad(adult[lod]->Instructions, CRC64(kFairyRecolouredInstrument),
                                                 reference->Instructions, CRC64(kFairyRecolouredTexture)))
            return "OoT's child hand cannot be read or loads the Fairy Ocarina texture otherwise";
    }
    sources = adult;
    copies = recoloured;
    return nullptr;
}
