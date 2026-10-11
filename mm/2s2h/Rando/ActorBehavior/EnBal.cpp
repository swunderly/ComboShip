#include "ActorBehavior.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/CustomMessage/CustomMessage.h"

extern "C" {
#include "variables.h"
#include "overlays/actors/ovl_En_Bal/z_en_bal.h"
void EnBal_SetupTalk(EnBal* enBal);
}

std::map<int16_t, std::vector<RandoCheckId>> tingleMap = {
    { SCENE_BACKTOWN, { RC_CLOCK_TOWN_NORTH_TINGLE_MAP_01, RC_CLOCK_TOWN_NORTH_TINGLE_MAP_02 } },
    { SCENE_24KEMONOMITI, { RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_01, RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_02 } },
    { SCENE_17SETUGEN, { RC_TWIN_ISLANDS_TINGLE_MAP_01, RC_TWIN_ISLANDS_TINGLE_MAP_02 } },
    { SCENE_17SETUGEN2, { RC_TWIN_ISLANDS_TINGLE_MAP_01, RC_TWIN_ISLANDS_TINGLE_MAP_02 } },
    { SCENE_ROMANYMAE, { RC_MILK_ROAD_TINGLE_MAP_01, RC_MILK_ROAD_TINGLE_MAP_02 } },
    { SCENE_30GYOSON, { RC_GREAT_BAY_COAST_TINGLE_MAP_01, RC_GREAT_BAY_COAST_TINGLE_MAP_02 } },
    { SCENE_IKANA, { RC_IKANA_CANYON_TINGLE_MAP_01, RC_IKANA_CANYON_TINGLE_MAP_02 } }
};

// A bought slot whose item cannot be given again reads "SOLD OUT" in red without a price, as in OoTMM
// (En_Bal.c EnBal_OverrideTextMapName: the bought flag persists across cycles and only renewables stay for sale).
static bool IsTingleSlotSoldOut(RandoCheckId randoCheckId) {
    return RANDO_SAVE_CHECKS[randoCheckId].obtained &&
           !Rando::IsItemObtainable(RANDO_SAVE_CHECKS[randoCheckId].randoItemId, randoCheckId);
}

static std::string TingleSlotText(RandoCheckId randoCheckId) {
    if (IsTingleSlotSoldOut(randoCheckId))
        return "\x01SOLD OUT";
    return "\x02" +
           Rando::StaticData::GetRevealedItemName(RANDO_SAVE_CHECKS[randoCheckId].randoItemId, false, randoCheckId,
                                                  true) +
           "\x01 " + std::to_string(RANDO_SAVE_CHECKS[randoCheckId].price) + " Rupees";
}

void OnOpenShopText(u16* textId, bool* loadFromMessageTable) {
    RandoCheckId randoCheckId1 = tingleMap[gPlayState->sceneId][0];
    RandoCheckId randoCheckId2 = tingleMap[gPlayState->sceneId][1];

    auto entry = CustomMessage::LoadVanillaMessageTableEntry(*textId);
    entry.autoFormat = false;

    // Same layout as before: the three-choice code follows the first line's color code.
    std::string first = TingleSlotText(randoCheckId1);
    first.insert(1, "\xC3");
    entry.msg = first + "\x11" + TingleSlotText(randoCheckId2) + "\x11" + "\x02No thanks";
    CustomMessage::EnsureMessageEnd(&entry.msg);
    CustomMessage::LoadCustomMessageIntoFont(entry);
    *loadFromMessageTable = false;
};

void Rando::ActorBehavior::InitEnBalBehavior() {
    bool shouldRegister = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_TINGLE_SHOPS];

    COND_VB_SHOULD(VB_NOT_AFFORD_TINGLE_MAP, shouldRegister, {
        EnBal* enBal = va_arg(args, EnBal*);
        s32* price = va_arg(args, s32*);
        auto randoCheckId = tingleMap[gPlayState->sceneId][gPlayState->msgCtx.choiceIndex];

        *price = RANDO_SAVE_CHECKS[randoCheckId].price;

        *should = gSaveContext.save.saveInfo.playerData.rupees < *price;
    });

    COND_VB_SHOULD(VB_ALREADY_HAVE_TINGLE_MAP, shouldRegister, {
        EnBal* enBal = va_arg(args, EnBal*);

        auto randoCheckId = tingleMap[gPlayState->sceneId][gPlayState->msgCtx.choiceIndex];

        if (Rando::IsItemObtainable(RANDO_SAVE_CHECKS[randoCheckId].randoItemId, randoCheckId)) {
            *should = false;
        } else {
            *should = true;
        }
    });

    COND_VB_SHOULD(VB_TINGLE_GIVE_MAP_UNLOCK, shouldRegister, {
        EnBal* enBal = va_arg(args, EnBal*);
        RANDO_SAVE_CHECKS[tingleMap[gPlayState->sceneId][gPlayState->msgCtx.choiceIndex]].eligible = true;
        Message_StartTextbox(gPlayState, 0x1D17, &enBal->picto.actor);
        enBal->textId = 0x1D17;
        EnBal_SetupTalk(enBal);
        *should = false;
    });

    COND_ID_HOOK(OnOpenText, 0x1D11, shouldRegister, OnOpenShopText);
    COND_ID_HOOK(OnOpenText, 0x1D12, shouldRegister, OnOpenShopText);
    COND_ID_HOOK(OnOpenText, 0x1D13, shouldRegister, OnOpenShopText);
    COND_ID_HOOK(OnOpenText, 0x1D14, shouldRegister, OnOpenShopText);
    COND_ID_HOOK(OnOpenText, 0x1D15, shouldRegister, OnOpenShopText);
    COND_ID_HOOK(OnOpenText, 0x1D16, shouldRegister, OnOpenShopText);
    // Picking a slot that cannot be bought keeps Tingle's native reply (0x1D09), as OoTMM does.

    COND_VB_SHOULD(VB_HAVE_MAGIC_FOR_TINGLE, shouldRegister, { *should = true; });
}
