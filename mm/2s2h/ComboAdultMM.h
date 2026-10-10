#pragma once
#include "objects/gameplay_keep/gameplay_keep.h"
// Exact pinned custom/index.ts gameplay dimensions and MM animation bindings.
// This module does not enable Adult MM: transformation/model resources must be integrated first.
namespace ComboAdultMM {
inline PlayerAgeProperties BuildAdultProperties() {
    PlayerAgeProperties p{};
    p.ceilingCheckHeight = 56.0f;
    p.shadowScale = 90.0f;
    p.unk_08 = 1.0f;
    p.unk_0C = 111.0f;
    p.unk_10 = 70.0f;
    p.unk_14 = 79.4f;
    p.unk_18 = 59.0f;
    p.unk_1C = 41.0f;
    p.unk_20 = 19.0f;
    p.unk_24 = 36.0f;
    p.unk_28 = 44.8f;
    p.unk_2C = 56.0f;
    p.unk_30 = 68.0f;
    p.unk_34 = 70.0f;
    p.wallCheckRadius = 18.0f;
    p.unk_3C = 15.0f;
    p.unk_40 = 70.0f;
    p.unk_44 = { 9, 0x123F, 0x0167 };
    const Vec3s a[4] = { { 8, 0x1256, 0x017C }, { 9, 0x17EA, 0x0167 }, { 8, 0x1256, 0x017C }, { 9, 0x17EA, 0x0167 } };
    const Vec3s b[4] = { { 9, 0x17EA, 0x0167 }, { 9, 0x1E0D, 0x017C }, { 9, 0x17EA, 0x0167 }, { 9, 0x1E0D, 0x017C } };
    const Vec3s c[4] = {
        { 8, 0x1256, 0x017C }, { 9, 0x17EA, 0x0167 }, { -0x638, 0x1256, 0x017C }, { -0x637, 0x17EA, 0x0167 }
    };
    for (int i = 0; i < 4; ++i) {
        p.unk_4A[i] = a[i];
        p.unk_62[i] = b[i];
        p.unk_7A[i] = c[i];
    }
    p.voiceSfxIdOffset = 0x20;
    p.surfaceSfxIdOffset = 0x80;
    p.unk_98 = 22.0f;
    p.unk_9C = 36.0f;
#define ADULT_ANIM(field, name) p.field = (PlayerAnimationHeader*)&name
    ADULT_ANIM(openChestAnim, gPlayerAnim_link_demo_Tbox_open);
    ADULT_ANIM(timeTravelStartAnim, gPlayerAnim_link_demo_back_to_past);
    ADULT_ANIM(timeTravelEndAnim, gPlayerAnim_link_demo_return_to_past);
    ADULT_ANIM(unk_AC, gPlayerAnim_link_normal_climb_startA);
    ADULT_ANIM(unk_B0, gPlayerAnim_link_normal_climb_startB);
    ADULT_ANIM(unk_B4[0], gPlayerAnim_link_normal_climb_upL);
    ADULT_ANIM(unk_B4[1], gPlayerAnim_link_normal_climb_upR);
    ADULT_ANIM(unk_B4[2], gPlayerAnim_link_normal_Fclimb_upL);
    ADULT_ANIM(unk_B4[3], gPlayerAnim_link_normal_Fclimb_upR);
    ADULT_ANIM(unk_C4[0], gPlayerAnim_link_normal_Fclimb_sideL);
    ADULT_ANIM(unk_C4[1], gPlayerAnim_link_normal_Fclimb_sideR);
    ADULT_ANIM(unk_CC[0], gPlayerAnim_link_normal_climb_endAL);
    ADULT_ANIM(unk_CC[1], gPlayerAnim_link_normal_climb_endAR);
    ADULT_ANIM(unk_D4[0], gPlayerAnim_link_normal_climb_endBR);
    ADULT_ANIM(unk_D4[1], gPlayerAnim_link_normal_climb_endBL);
#undef ADULT_ANIM
    return p;
}
inline bool IsAdult(const Save& save) {
    return save.shipSaveInfo.adultMm.enabled && save.linkAge == 0;
}
inline void InitializeLoadouts(ComboAdultMmSave& state) {
    state.initialized = 1;
    for (auto& x : state.loadouts) {
        x.boots = x.tunic = 0;
        for (int i = 0; i < 4; ++i) {
            x.buttonItems[i] = x.cButtonSlots[i] = x.dpadItems[i] = x.dpadSlots[i] = ITEM_NONE;
        }
    }
}
// Save-only counterpart of pinned Age_SwapEquipmentMm. Call only from a complete age transition.
// Native MM lacks these fields; appended saved custom equipment preserves pinned age ownership.
inline bool SwapHumanLoadout(Save& save, u8 age) {
    auto& state = save.shipSaveInfo.adultMm;
    if (!state.enabled || age > 1 || save.linkAge > 1)
        return false;
    if (age == save.linkAge)
        return true;
    if (!state.initialized)
        InitializeLoadouts(state);
    auto& current = state.loadouts[save.linkAge];
    auto& target = state.loadouts[age];
    current.boots = state.boots;
    current.tunic = state.tunic;
    state.boots = target.boots;
    state.tunic = target.tunic;
    for (int i = 1; i < 4; ++i) {
        current.buttonItems[i] = save.saveInfo.equips.buttonItems[0][i];
        current.cButtonSlots[i] = save.saveInfo.equips.cButtonSlots[0][i];
        save.saveInfo.equips.buttonItems[0][i] = target.buttonItems[i];
        save.saveInfo.equips.cButtonSlots[0][i] = target.cButtonSlots[i];
        const u8 slot = target.cButtonSlots[i];
        if (slot >= SLOT_BOTTLE_1 && slot <= SLOT_BOTTLE_6)
            save.saveInfo.equips.buttonItems[0][i] = save.saveInfo.inventory.items[slot];
    }
    // Port D-pad extension; reload bottle contents using the same native inventory slot rule.
    for (int i = 0; i < 4; ++i) {
        current.dpadItems[i] = save.shipSaveInfo.dpadEquips.dpadItems[0][i];
        current.dpadSlots[i] = save.shipSaveInfo.dpadEquips.dpadSlots[0][i];
        save.shipSaveInfo.dpadEquips.dpadItems[0][i] = target.dpadItems[i];
        save.shipSaveInfo.dpadEquips.dpadSlots[0][i] = target.dpadSlots[i];
        const u8 slot = target.dpadSlots[i];
        if (slot >= SLOT_BOTTLE_1 && slot <= SLOT_BOTTLE_6)
            save.shipSaveInfo.dpadEquips.dpadItems[0][i] = save.saveInfo.inventory.items[slot];
    }
    save.linkAge = age;
    return true;
}
// Native Player_Init uses exact age properties; collision/culling rules stay in native code.

} // namespace ComboAdultMM
