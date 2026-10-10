#pragma once
#include "ComboFairyOcarinaGeometry.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/lus_gbi.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
static std::shared_ptr<Ship::IResource> sFairyInstrumentSource;
static std::shared_ptr<Fast::DisplayList> sFairyInstrumentCopy;
static std::weak_ptr<Ship::ResourceManager> sFairyInstrumentOwner;
static bool sFairyInstrumentListener;
static void Combo_DropFairyInstrument() {
    sFairyInstrumentCopy.reset();
    sFairyInstrumentSource.reset();
    sFairyInstrumentOwner.reset();
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
        if (!sFairyInstrumentListener) {
            Ship::CrossRMRegistry::RegisterTeardownListener(Combo_DropFairyInstrument);
            sFairyInstrumentListener = true;
        }
    }
    // Runtime resources have no archive backing. Reinsert after cache eviction, retaining
    // source-owned pointer payloads until registry teardown drops both references.
    rm->CacheExternalResource("combo/runtime/fairy_ocarina_instrument", sFairyInstrumentCopy);
    return "__OTR__@oot:combo/runtime/fairy_ocarina_instrument";
}
extern "C" Gfx* Combo_MM_FairyOcarinaHand(PlayState* play, Gfx* nativeHand) {
    const char* instrument = Combo_FairyInstrumentPath();
    if (!instrument)
        return nullptr;
    Gfx* list = (Gfx*)GRAPH_ALLOC(play->state.gfxCtx, sizeof(Gfx) * 2);
    if (!list)
        return nullptr;
    Gfx* d = list;
    gSPDisplayList(d++, nativeHand);
    // Routed path branch: the stripped copy's texture/vertex hashes must resolve in OoT's RM.
    gDma1p(d++, G_DL_OTR_FILEPATH, instrument, 0, G_DL_NOPUSH);
    return list; // native hand + stripped instrument, same limb matrix, no fabricated transform
}
