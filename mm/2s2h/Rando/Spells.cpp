#ifdef COMBO_BUILD
#include "Spells.h"
#include "FaroresWind.h"
#include "ComboExport.h"
#include "Rando/Rando.h"
#include "rando/FaroreRules.h"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/Resource.h>
#include "NativeSpellResources.h"
#include <array>
#include <memory>
extern "C" int Combo_MMFaroreEffectiveHumanAge(void);
namespace {
static const ALIGN_ASSET(2) char sComboLoveItemName[] =
    "__OTR__@oot:textures/item_name_static/gNayrusLoveItemNameENGTex";
static const ALIGN_ASSET(2) char sComboFireItemName[] = "__OTR__@oot:textures/item_name_static/gDinsFireItemNameENGTex";
static const ALIGN_ASSET(2) char sComboEmptyItemName[] = "__OTR__textures/icon_item_static/gEmptyTexture";
std::array<std::shared_ptr<Ship::ResourceManager>, 2> sOotResources, sMmResources;
std::array<std::array<std::shared_ptr<Ship::IResource>, 12>, 2> sOwners, sFrameOwners;
std::array<bool, 2> sOotAlt{}, sMmAlt{};
void (*sInvalidateIntegration)() = nullptr;
// MM_PrepareNativeSpellsForCombo bits 1 (Love) and 2 (Fire) as found when the integration came up.
int sReadySpells = 0;
bool sHeadlessGeneration = false;
constexpr const char* sPaths[2][12] = {
    { "overlays/ovl_Magic_Dark/sDiamondTex", "objects/gameplay_keep/gPlayerAnim_link_magic_tame",
      "objects/gameplay_keep/gPlayerAnim_link_magic_honoo1", "objects/gameplay_keep/gPlayerAnim_link_magic_honoo2",
      "objects/gameplay_keep/gPlayerAnim_link_magic_honoo3", "textures/icon_item_static/gItemIconNayrusLoveTex",
      "objects/object_gi_goddess/gGiMagicSpellDiamondDL", "objects/object_gi_goddess/gGiNayrusLoveColorDL",
      "objects/object_gi_goddess/gGiMagicSpellOrbDL", "textures/item_name_static/gNayrusLoveItemNameENGTex",
      "objects/gameplay_keep/gEffFlash1DL", "objects/gameplay_keep/gameplay_keep_Tex_028BB8" },
    { "overlays/ovl_Magic_Fire/sTex", "objects/gameplay_keep/gPlayerAnim_link_magic_tame",
      "objects/gameplay_keep/gPlayerAnim_link_magic_tamashii1",
      "objects/gameplay_keep/gPlayerAnim_link_magic_tamashii2",
      "objects/gameplay_keep/gPlayerAnim_link_magic_tamashii3", "textures/icon_item_static/gItemIconDinsFireTex",
      "objects/object_gi_goddess/gGiMagicSpellDiamondDL", "objects/object_gi_goddess/gGiDinsFireColorDL",
      "objects/object_gi_goddess/gGiMagicSpellOrbDL", "textures/item_name_static/gDinsFireItemNameENGTex" }
};
void DropSpellResources() {
    ComboRando::ComboFaroreIntegrationReady = false;
    sReadySpells = 0;
    sOwners = {};
    sFrameOwners = {};
    sOotResources = {};
    sMmResources = {};
    auto invalidate = sInvalidateIntegration;
    sInvalidateIntegration = nullptr;
    if (invalidate)
        invalidate();
}
} // namespace
extern "C" int Combo_MMSpellEffectiveHumanAge() {
    return Combo_MMFaroreEffectiveHumanAge();
}
extern "C" int Combo_MMSpellEnabled(int spell) {
    if (!ComboRando::ComboFaroreIntegrationReady)
        return 0;
    const auto& state = gSaveContext.save.shipSaveInfo;
    if (gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2 || state.saveType != SAVETYPE_RANDO)
        return 0;
    if (spell == 1)
        return state.rando.randoSaveOptions[RO_MM_SPELL_LOVE] && state.spellLoveOwned;
    if (spell == 2)
        return state.rando.randoSaveOptions[RO_MM_SPELL_FIRE] && state.spellFireOwned;
    return 0;
}
extern "C" void Combo_MMSpellGrantItem(int spell) {
    if (spell != 1 && spell != 2)
        return;
    auto& state = gSaveContext.save.shipSaveInfo;
    if (spell == 1)
        state.spellLoveOwned = 1;
    else
        state.spellFireOwned = 1;
    auto& slot = gSaveContext.save.saveInfo.inventory.items[spell == 1 ? SLOT_TRADE_COUPLE : SLOT_TRADE_DEED];
    if (slot == ITEM_NONE)
        slot = spell == 1 ? ITEM_COMBO_NAYRUS_LOVE : ITEM_COMBO_DINS_FIRE;
}
extern "C" int Combo_MMSpellPrepareResources(int spell) {
    if (spell != 1 && spell != 2)
        return 0;
    try {
        const size_t index = spell - 1, count = spell == 1 ? 12u : 10u;
        auto oot = Ship::CrossRMRegistry::Get("oot"), mm = Ship::CrossRMRegistry::Get("mm");
        if (!oot || !mm)
            return 0;
        bool cached = sOwners[index][0] && sOotResources[index] == oot && sMmResources[index] == mm &&
                      sOotAlt[index] == oot->IsAltAssetsEnabled() && sMmAlt[index] == mm->IsAltAssetsEnabled();
        for (size_t i = 0; cached && i < count; ++i)
            cached = ComboNativeSpells::Valid(sOwners[index][i], sPaths[index][i], sFrameOwners[index][i]);
        if (cached)
            return 1;
        std::array<std::shared_ptr<Ship::IResource>, 12> owners, frames;
        for (size_t i = 0; i < count; ++i) {
            owners[i] = ComboNativeSpells::Load((i >= 1 && i <= 4) || i >= 10 ? mm : oot, sPaths[index][i], frames[i]);
            if (!owners[i])
                return 0;
        }
        sOotAlt[index] = oot->IsAltAssetsEnabled();
        sMmAlt[index] = mm->IsAltAssetsEnabled();
        sOwners[index] = std::move(owners);
        sFrameOwners[index] = std::move(frames);
        sOotResources[index] = std::move(oot);
        sMmResources[index] = std::move(mm);
        Ship::CrossRMRegistry::RegisterTeardownListener(DropSpellResources);
        return 1;
    } catch (const std::exception&) { return 0; }
}
// Private exact-candidate preflight; does not grant items, cast, or change a save.
// Bit0 Wind, bit1 Love, bit2 Fire. Both engines must have their real RMs registered.
extern "C" COMBO_EXPORT int MM_PrepareNativeSpellsForCombo() {
    int mask = 0;
    if (Combo_MMFarorePrepareResources())
        mask |= 1;
    if (Combo_MMSpellPrepareResources(1))
        mask |= 2;
    if (Combo_MMSpellPrepareResources(2))
        mask |= 4;
    return mask;
}
// Process startup/teardown only. Save rules and actual ownership remain independent.
// Readiness needs only Farore's Wind; Love and Fire check their own resources wherever they are used, and the
// result for each is kept so their seed options are offered only where they can be cast.
extern "C" COMBO_EXPORT int MM_SetNativeSpellIntegrationReady(int enable, void (*invalidate)()) {
    ComboRando::ComboFaroreIntegrationReady = false;
    sReadySpells = 0;
    sInvalidateIntegration = enable ? invalidate : nullptr;
    if (enable && Combo_MMFaroreBridgeReady()) {
        const int mask = MM_PrepareNativeSpellsForCombo();
        if (mask & 1) {
            ComboRando::ComboFaroreIntegrationReady = true;
            sReadySpells = mask & 6;
        }
    }
    return ComboRando::ComboFaroreIntegrationReady ? 1 : 0;
}
// A seed must not need a spell this device cannot cast: casting needs the integration (Combo_MMSpellEnabled) and
// the spell's own resources (Combo_MMSpellUseItem, ComboMagic*_Init). Without them the menu greys the option out
// and options are gathered with it off (BenPort.cpp kComboMmSpellOptions), as for the hidden adult options.
extern "C" int Combo_MMSpellOptionReady(int spell) {
    if (spell != 1 && spell != 2)
        return 0;
    if (sHeadlessGeneration)
        return 1;
    return ComboRando::ComboFaroreIntegrationReady && (sReadySpells & (spell == 1 ? 2 : 4)) ? 1 : 0;
}
extern "C" void Combo_MMSpellsForHeadlessGeneration() {
    sHeadlessGeneration = true;
}
extern "C" const char* Combo_MMSpellItemName(int item) {
    const int spell = item == ITEM_COMBO_NAYRUS_LOVE ? 1 : item == ITEM_COMBO_DINS_FIRE ? 2 : 0;
    if (!spell || !Combo_MMSpellPrepareResources(spell))
        return sComboEmptyItemName;
    return spell == 1 ? sComboLoveItemName : sComboFireItemName;
}
#endif
