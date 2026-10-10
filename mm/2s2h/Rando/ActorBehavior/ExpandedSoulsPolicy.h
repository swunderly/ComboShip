#pragma once
// Adapted pinned OoTMM 41a2b7c0d7dbbb53fed928652f8271bc336c113f. See LICENSE.
namespace Rando {
enum class ExpandedSoul {
    ANIMAL_BUTTERFLY,
    ANIMAL_COW,
    ANIMAL_CUCCO,
    ANIMAL_DOG,
    MISC_BUSINESS_SCRUB,
    MISC_GS,
    NPC_ANJU,
    NPC_AROMA,
    NPC_ASTRONOMER,
    NPC_BANKER,
    NPC_BAZAAR_SHOPKEEPER,
    NPC_BEAN_SALESMAN,
    NPC_BIGGORON,
    NPC_BLACKSMITHS,
    NPC_BOMBCHU_BOWLING_LADY,
    NPC_BOMBCHU_SHOPKEEPER,
    NPC_BOMBERS,
    NPC_BUTLER_DEKU,
    NPC_CARPENTERS,
    NPC_CARPET_MAN,
    NPC_CHEST_GAME_OWNER,
    NPC_CITIZEN,
    NPC_COMPOSER_BROS,
    NPC_DAMPE,
    NPC_DEKU_KING,
    NPC_DEKU_PRINCESS,
    NPC_DOG_LADY,
    NPC_FISHING_POND_OWNER,
    NPC_GORMAN,
    NPC_GORON,
    NPC_GORON_CHILD,
    NPC_GORON_ELDER,
    NPC_GORON_SHOPKEEPER,
    NPC_GROG,
    NPC_GURU_GURU,
    NPC_HONEY_DARLING,
    NPC_KAFEI,
    NPC_KEATON,
    NPC_KOUME_KOTAKE,
    NPC_MALON,
    NPC_MAYOR_DOTOUR,
    NPC_MEDIGORON,
    NPC_MOON_CHILDREN,
    NPC_OLD_HAG,
    NPC_PLAYGROUND_SCRUBS,
    NPC_POE_COLLECTOR,
    NPC_ROOFTOP_MAN,
    NPC_RUTO,
    NPC_SCIENTIST,
    NPC_SHOOTING_GALLERY_OWNER,
    NPC_TALON,
    NPC_THIEVES,
    NPC_TINGLE,
    NPC_TOILET_HAND,
    NPC_TOTO,
    NPC_TOURIST_CENTER,
    NPC_ZORA,
    NPC_ZORA_MUSICIANS,
    NPC_ZORA_SHOPKEEPER,
    Count
};
inline int opt(int x) {
    return x > 0 ? 1 : -1;
}
template <class Owned>
inline int ExpandedSoulSpawnPolicy(PlayState* play, int actorId, unsigned variable, bool credits, Owned owned) {
    if (credits)
        return 1;

    switch (actorId) {
        case ACTOR_EN_GINKO_MAN:
            return opt(owned(ExpandedSoul::NPC_BANKER));
        case ACTOR_EN_AN:
            return opt(owned(ExpandedSoul::NPC_ANJU));
        case ACTOR_EN_GURUGURU:
            return opt(owned(ExpandedSoul::NPC_GURU_GURU));
        case ACTOR_EN_DAIKU:
        case ACTOR_EN_DAIKU2:
            if (play->sceneId == SCENE_SONCHONOIE && play->roomCtx.curRoom.num == 0x01)
                return opt(owned(ExpandedSoul::NPC_MAYOR_DOTOUR));
            return opt(owned(ExpandedSoul::NPC_CARPENTERS));
        case ACTOR_EN_MS:
            return opt(owned(ExpandedSoul::NPC_BEAN_SALESMAN));
        case ACTOR_EN_MA_YTO:
        case ACTOR_EN_MA_YTS:
        case ACTOR_EN_MA4:
            return opt(owned(ExpandedSoul::NPC_MALON));
        case ACTOR_EN_INVADEPOH:
            switch ((variable >> 4) & 0xf) {
                case 0x4:
                case 0x5:
                case 0x7:
                case 0x8:
                case 0x9:
                case 0xb:
                case 0xc:
                    return opt(owned(ExpandedSoul::NPC_MALON));
            }
            break;
        case ACTOR_EN_ZOT:
        case ACTOR_EN_ZOW:
            return opt(owned(ExpandedSoul::NPC_ZORA));
        case ACTOR_EN_BJT:
            return opt(owned(ExpandedSoul::NPC_TOILET_HAND));
        case ACTOR_EN_BAL:
            return opt(owned(ExpandedSoul::NPC_TINGLE));
        case ACTOR_EN_TAKARAYA:
            return opt(owned(ExpandedSoul::NPC_BOMBCHU_BOWLING_LADY));
        case ACTOR_EN_KBT:
        case ACTOR_EN_KGY:
            return opt(owned(ExpandedSoul::NPC_BLACKSMITHS));
        case ACTOR_EN_GB2:
            return opt(owned(ExpandedSoul::NPC_POE_COLLECTOR));
        case ACTOR_EN_TAB:
            return opt(owned(ExpandedSoul::NPC_TALON));
        case ACTOR_EN_BJI_01:
            return opt(owned(ExpandedSoul::NPC_ASTRONOMER));
        case ACTOR_EN_MUTO:
        case ACTOR_EN_HEISHI:
        case ACTOR_EN_BAISEN:
            if (play->sceneId == SCENE_SONCHONOIE && play->roomCtx.curRoom.num != 0x01)
                return 1;
            /* Fallthrough */
        case ACTOR_EN_DT:
            return opt(owned(ExpandedSoul::NPC_MAYOR_DOTOUR));
        case ACTOR_EN_TRT:
        case ACTOR_EN_TRT2:
        case ACTOR_EN_TRU:
        case ACTOR_EN_TRU_MT:
            return opt(owned(ExpandedSoul::NPC_KOUME_KOTAKE));
        case ACTOR_EN_KITAN:
            return opt(owned(ExpandedSoul::NPC_KEATON));
        case ACTOR_EN_TEST3:
            return opt(owned(ExpandedSoul::NPC_KAFEI));
        case ACTOR_EN_TOTO:
            return opt(owned(ExpandedSoul::NPC_TOTO));
        case ACTOR_EN_ZOV:
            return opt(owned(ExpandedSoul::NPC_RUTO));
        case ACTOR_EN_ZOD:
        case ACTOR_EN_ZOS:
        case ACTOR_EN_ZOB:
            return opt(owned(ExpandedSoul::NPC_ZORA_MUSICIANS));
        case ACTOR_EN_FU:
            return opt(owned(ExpandedSoul::NPC_HONEY_DARLING));
        case ACTOR_EN_SOB1:
            switch (variable & 0xf) {
                case 0:
                    return opt(owned(ExpandedSoul::NPC_ZORA_SHOPKEEPER));
                case 1:
                    return opt(owned(ExpandedSoul::NPC_GORON_SHOPKEEPER));
                case 2:
                    return opt(owned(ExpandedSoul::NPC_BOMBCHU_SHOPKEEPER));
                default:
                    return 1;
            }
        case ACTOR_EN_JGAME_TSN:
        case ACTOR_EN_TSN:
            return opt(owned(ExpandedSoul::NPC_CHEST_GAME_OWNER));
        case ACTOR_EN_LIFT_NUTS:
            return opt(owned(ExpandedSoul::NPC_PLAYGROUND_SCRUBS));
        case ACTOR_EN_DNP:
            return opt(owned(ExpandedSoul::NPC_DEKU_PRINCESS));
        case ACTOR_EN_DNQ:
            return opt(owned(ExpandedSoul::NPC_DEKU_KING));
        case ACTOR_EN_TK:
            return opt(owned(ExpandedSoul::NPC_DAMPE));
        case ACTOR_EN_PO_COMPOSER:
            return opt(owned(ExpandedSoul::NPC_COMPOSER_BROS));
        case ACTOR_EN_STH:
        case ACTOR_EN_JA:
        case ACTOR_EN_YB:
        case ACTOR_EN_RZ:
        case ACTOR_EN_MM3:
        case ACTOR_EN_PM:
            return opt(owned(ExpandedSoul::NPC_CITIZEN));
        case ACTOR_EN_DNO:
            return opt(owned(ExpandedSoul::NPC_BUTLER_DEKU));
        case ACTOR_EN_BOM_BOWL_MAN:
        case ACTOR_EN_BOMJIMA:
        case ACTOR_EN_BOMJIMB:
        case ACTOR_EN_BOMBERS:
        case ACTOR_EN_BOMBERS2:
        case ACTOR_EN_BOMBAL:
            return opt(owned(ExpandedSoul::NPC_BOMBERS));
        case ACTOR_EN_RSN:
            return opt(owned(ExpandedSoul::NPC_BOMBCHU_SHOPKEEPER));
        case ACTOR_EN_OSSAN:
            switch (variable & 0xf) {
                case 0x00:
                    return opt(owned(ExpandedSoul::NPC_FISHING_POND_OWNER));
                case 0x01:
                    return opt(owned(ExpandedSoul::NPC_ROOFTOP_MAN));
                default:
                    return 1; // Invalid legacy actor params: leave native behavior unchanged.
            }
        case ACTOR_EN_ANI:
            return opt(owned(ExpandedSoul::NPC_ROOFTOP_MAN));
        case ACTOR_EN_GO:
            switch (variable) {
                case 0x08:
                    return opt(owned(ExpandedSoul::NPC_MEDIGORON));
                default:
                    return opt(owned(ExpandedSoul::NPC_GORON));
            }
        case ACTOR_EN_S_GORO:
            return opt(owned(ExpandedSoul::NPC_GORON));
        case ACTOR_EN_GK:
            return opt(owned(ExpandedSoul::NPC_GORON_CHILD));
        case ACTOR_EN_JG:
            return opt(owned(ExpandedSoul::NPC_GORON_ELDER));
        case ACTOR_EN_DAI:
            return opt(owned(ExpandedSoul::NPC_BIGGORON));
        case ACTOR_EN_SYATEKI_MAN:
            if (play->sceneId == SCENE_SYATEKI_MORI)
                return opt(owned(ExpandedSoul::NPC_BAZAAR_SHOPKEEPER));
            else
                return opt(owned(ExpandedSoul::NPC_SHOOTING_GALLERY_OWNER));
        case ACTOR_EN_SYATEKI_OKUTA:
            return opt(owned(ExpandedSoul::NPC_SHOOTING_GALLERY_OWNER));
        case ACTOR_EN_MK:
            return opt(owned(ExpandedSoul::NPC_SCIENTIST));
        case ACTOR_EN_IN:
        case ACTOR_EN_GM:
            return opt(owned(ExpandedSoul::NPC_GORMAN));
        case ACTOR_EN_HS:
            return opt(owned(ExpandedSoul::NPC_GROG));
        case ACTOR_EN_AOB_01:
            return opt(owned(ExpandedSoul::NPC_DOG_LADY));
        case ACTOR_EN_AL:
            return opt(owned(ExpandedSoul::NPC_AROMA));
        case ACTOR_EN_JS:
            return opt(owned(ExpandedSoul::NPC_MOON_CHILDREN));
        case ACTOR_EN_SHN:
            return opt(owned(ExpandedSoul::NPC_TOURIST_CENTER));
        case ACTOR_EN_NB:
            return opt(owned(ExpandedSoul::NPC_OLD_HAG));
        case ACTOR_EN_KENDO_JS:
            return opt(owned(ExpandedSoul::NPC_CARPET_MAN));
        case ACTOR_EN_OKUTA:
        case ACTOR_EN_BIGOKUTA:
            return 1;
        case ACTOR_EN_WALLMAS:
            return 1;
        case ACTOR_EN_DODONGO:
            return 1;
        case ACTOR_EN_FIREFLY:
            return 1;
        case ACTOR_EN_TITE:
            return 1;
        case ACTOR_EN_PEEHAT:
            return 1;
        case ACTOR_EN_DINOFOS:
            return 1;
        case ACTOR_EN_ST:
            return 1;
        case ACTOR_EN_AM:
        case ACTOR_EN_FAMOS:
            return 1;
        case ACTOR_EN_DEKUBABA:
        case ACTOR_EN_KAREBABA:
            return 1;
        case ACTOR_EN_DEKUNUTS:
            return 1;
        case ACTOR_EN_BBFALL:
        case ACTOR_EN_BB:
            return 1;
        case ACTOR_EN_VM:
            return 1;
        case ACTOR_EN_RD:
        case ACTOR_EN_TALK_GIBUD:
        case ACTOR_EN_RAILGIBUD:
            return 1;
        case ACTOR_EN_SW:
            if (variable & 0x03)
                return opt(owned(ExpandedSoul::MISC_GS));
            return 1;
        case ACTOR_OBJ_MAKEKINSUTA:
            return opt(owned(ExpandedSoul::MISC_GS));
        case ACTOR_EN_SB:
            return 1;
        case ACTOR_EN_RR:
            return 1;
        case ACTOR_EN_IK:
            return 1;
        case ACTOR_EN_FZ:
            return 1;
        case ACTOR_EN_WF:
            return 1;
        case ACTOR_EN_CROW:
            return 1;
        case ACTOR_EN_TUBO_TRAP:
            return 1 ? 1 : -1;
        case ACTOR_EN_FLOORMAS:
            return 1;
        case ACTOR_EN_SLIME:
            return 1;
        case ACTOR_EN_DRAGON:
            return 1;
        case ACTOR_EN_PR:
        case ACTOR_EN_PR2:
        case ACTOR_EN_PRZ:
            return 1;
        case ACTOR_EN_WDHAND:
            return 1;
        case ACTOR_EN_GRASSHOPPER:
            return 1;
        case ACTOR_EN_SNOWMAN:
            return 1;
        case ACTOR_EN_EGOL:
            return 1;
        case ACTOR_EN_PP:
            return 1;
        case ACTOR_EN_RAT:
            return 1;
        case ACTOR_EN_THIEFBIRD:
            return 1;
        case ACTOR_EN_MKK:
            return 1;
        case ACTOR_EN_BAGUO:
            return 1;
        case ACTOR_BOSS_05:
            return 1;
        case ACTOR_EN_JSO:
        case ACTOR_EN_JSO2:
            return 1;
        case ACTOR_EN_WIZ:
        case ACTOR_EN_WIZ_BROCK:
            return 1;
        case ACTOR_EN_DEATH:
            return 1;
        case ACTOR_EN_PAMETFROG:
        case ACTOR_EN_BIGSLIME:
        case ACTOR_EN_BIGPAMET:
            return 1;
        case ACTOR_EN_BAT:
            return 1;
        case ACTOR_EN_KAME:
            return 1;
        case ACTOR_BOSS_04:
            return 1;
        case ACTOR_EN_BSB:
            return 1;
        case ACTOR_BOSS_01:
            return 1;
        case ACTOR_BOSS_HAKUGIN:
            return 1;
        case ACTOR_BOSS_03:
            return 1;
        case ACTOR_BOSS_02:
            return 1;
        case ACTOR_EN_KNIGHT:
        case ACTOR_BOSS_06:
            return 1;
        case ACTOR_EN_NEO_REEBA:
            return 1;
        case ACTOR_EN_SKB:
            if ((play->sceneId == SCENE_BOTI) && (play->roomCtx.curRoom.num == 0x01)) /* Upper graveyard */
                return 1;
            /* Fallthrough */
        case ACTOR_EN_HINT_SKB:
        case ACTOR_EN_RAIL_SKB:
            return 1;
        case ACTOR_EN_SELLNUTS:
        case ACTOR_EN_SCOPENUTS:
        case ACTOR_EN_AKINDONUTS:
            return opt(owned(ExpandedSoul::MISC_BUSINESS_SCRUB));
        case ACTOR_EN_GE1:
        case ACTOR_EN_GE2:
        case ACTOR_EN_GE3:
            return opt(owned(ExpandedSoul::NPC_THIEVES));
        case ACTOR_EN_KAIZOKU:
            return 1;
        case ACTOR_EN_NIW:
        case ACTOR_EN_NWC:
            return opt(owned(ExpandedSoul::ANIMAL_CUCCO));
        case ACTOR_EN_COW:
            return opt(owned(ExpandedSoul::ANIMAL_COW));
        case ACTOR_EN_DG:
        case ACTOR_EN_RACEDOG:
            return opt(owned(ExpandedSoul::ANIMAL_DOG));
        case ACTOR_EN_BUTTE:
            return opt(owned(ExpandedSoul::ANIMAL_BUTTERFLY));
        case ACTOR_EN_PO_SISTERS:
        case ACTOR_EN_POH:
        case ACTOR_EN_BIGPO:
            return 1;
    }

    return 1;
}
template <class Owned> inline int ExpandedAuxSpawn(PlayState* play, int actorId, unsigned variable, Owned owned) {
    if (actorId == ACTOR_OBJ_SOUND && (play->sceneId == SCENE_CLOCKTOWER || play->sceneId == SCENE_TOWN) &&
        (variable & 0x7f) == 0x10)
        return opt(owned(ExpandedSoul::NPC_CARPENTERS));
    return 1;
}
template <class Owned> inline void ExpandedGormanParams(int actorId, unsigned& variable, Owned owned) {
    if (actorId == ACTOR_EN_HORSE && ((variable & 0x1fff) == 0x13 || (variable & 0x1fff) == 0x14) &&
        !owned(ExpandedSoul::NPC_GORMAN))
        variable &= 0xe000;
}
} // namespace Rando
