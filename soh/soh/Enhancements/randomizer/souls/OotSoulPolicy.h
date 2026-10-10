// Adapted exactly from OoTMM 41a2b7c0d7dbbb53fed928652f8271bc336c113f oot/actors.c; MIT.
#pragma once
#include "z64actor_enum.h"
#ifdef OOT_SOUL_POLICY_HOST_ENUMS
#include "z64scene_enum.h"
#else
#include "z64scene.h"
#endif
namespace Rando::Souls {
enum class OotSoul {
    NPC_OLD_HAG,
    NPC_CARPET_MAN,
    NPC_GROG,
    NPC_GORMAN,
    NPC_SCIENTIST,
    NPC_BEAN_SALESMAN,
    NPC_HONEY_DARLING,
    NPC_CHEST_GAME_OWNER,
    NPC_COMPOSER_BROS,
    ENEMY_POE,
    MISC_BUSINESS_SCRUB,
    ENEMY_STALFOS,
    ENEMY_OCTOROK,
    ENEMY_WALLMASTER,
    ENEMY_DODONGO,
    ENEMY_KEESE,
    ENEMY_TEKTITE,
    ENEMY_PEAHAT,
    ENEMY_LIZALFOS_DINOLFOS,
    ENEMY_GOHMA_LARVA,
    ENEMY_SHABOM,
    ENEMY_BABY_DODONGO,
    ENEMY_DARK_LINK,
    ENEMY_BIRI_BARI,
    ENEMY_TAILPASARN,
    ENEMY_SKULLTULA,
    ENEMY_TORCH_SLUG,
    ENEMY_MOBLIN,
    ENEMY_ARMOS,
    ENEMY_DEKU_BABA,
    ENEMY_DEKU_SCRUB,
    ENEMY_BUBBLE,
    ENEMY_BEAMOS,
    ENEMY_FLOORMASTER,
    ENEMY_REDEAD_GIBDO,
    MISC_GS,
    ENEMY_SKULLWALLTULA,
    ENEMY_FLARE_DANCER,
    ENEMY_DEAD_HAND,
    ENEMY_SHELL_BLADE,
    ENEMY_LIKE_LIKE,
    ENEMY_SPIKE,
    ENEMY_ANUBIS,
    ENEMY_IRON_KNUCKLE,
    ENEMY_SKULL_KID,
    ENEMY_FLYING_POT,
    ENEMY_FREEZARD,
    ENEMY_STINGER,
    ENEMY_WOLFOS,
    ENEMY_GUAY,
    BOSS_QUEEN_GOHMA,
    BOSS_KING_DODONGO,
    BOSS_BARINADE,
    BOSS_PHANTOM_GANON,
    BOSS_VOLVAGIA,
    BOSS_MORPHA,
    BOSS_BONGO_BONGO,
    BOSS_TWINROVA,
    ENEMY_PARASITE,
    ENEMY_LEEVER,
    ENEMY_STALCHILD,
    NPC_SARIA,
    NPC_DARUNIA,
    NPC_RUTO,
    NPC_KING_ZORA,
    NPC_ANJU,
    NPC_CARPENTERS,
    NPC_GURU_GURU,
    NPC_MIDO,
    NPC_KOKIRI,
    NPC_KOKIRI_SHOPKEEPER,
    NPC_POTION_SHOPKEEPER,
    NPC_BOMBCHU_SHOPKEEPER,
    NPC_BAZAAR_SHOPKEEPER,
    NPC_ZORA_SHOPKEEPER,
    NPC_GORON_SHOPKEEPER,
    NPC_HYLIAN_GUARD,
    NPC_ROOFTOP_MAN,
    NPC_CITIZEN,
    NPC_DOG_LADY,
    NPC_BANKER,
    NPC_ASTRONOMER,
    NPC_MALON,
    NPC_TALON,
    NPC_FISHING_POND_OWNER,
    NPC_GORON,
    NPC_GORON_CHILD,
    NPC_BIGGORON,
    NPC_MEDIGORON,
    NPC_ZORA,
    NPC_BOMBCHU_BOWLING_LADY,
    NPC_SHOOTING_GALLERY_OWNER,
    NPC_DAMPE,
    NPC_BOMBERS,
    NPC_POE_COLLECTOR,
    NPC_SHEIK,
    NPC_ZELDA,
    NPC_THIEVES,
    ENEMY_THIEVES,
    ANIMAL_CUCCO,
    ANIMAL_COW,
    ANIMAL_DOG,
    ANIMAL_BUTTERFLY,
    COUNT
};
template <class Has>
inline int EvaluateOotSoulPreSpawn(int actorId, unsigned variable, int scene, bool adult, bool credits, Has has) {
    const auto optional = [](int owned) { return owned > 0 ? 1 : -1; };

    if (credits)
        return 1;

    switch (actorId) {
        case ACTOR_EN_DS:
            return optional(has(OotSoul::NPC_OLD_HAG));
        case ACTOR_EN_JS:
            return optional(has(OotSoul::NPC_CARPET_MAN));
        case ACTOR_EN_HS:
        case ACTOR_EN_HS2:
            return optional(has(OotSoul::NPC_GROG));
        case ACTOR_EN_IN:
            return optional(has(OotSoul::NPC_GORMAN));
        case ACTOR_EN_MK:
            return optional(has(OotSoul::NPC_SCIENTIST));
        case ACTOR_EN_MS:
            return optional(has(OotSoul::NPC_BEAN_SALESMAN));
        case ACTOR_EN_TG:
            return optional(has(OotSoul::NPC_HONEY_DARLING));
        case ACTOR_EN_TAKARA_MAN:
            return optional(has(OotSoul::NPC_CHEST_GAME_OWNER));
        case ACTOR_EN_POH:
            switch (variable) {
                case 2:
                case 3:
                    return optional(has(OotSoul::NPC_COMPOSER_BROS));
                default:
                    return has(OotSoul::ENEMY_POE);
            }
        case ACTOR_EN_PO_SISTERS:
        case ACTOR_EN_PO_FIELD:
            return has(OotSoul::ENEMY_POE);
        case ACTOR_EN_DNS:
        case ACTOR_EN_SHOPNUTS:
            return optional(has(OotSoul::MISC_BUSINESS_SCRUB));
        case ACTOR_EN_TEST:
            return has(OotSoul::ENEMY_STALFOS);
        case ACTOR_BG_MORI_BIGST:
            return optional(has(OotSoul::ENEMY_STALFOS));
        case ACTOR_BG_BDAN_OBJECTS:
            if (variable & 0xff)
                return 1;
            /* Fallthrough */
        case ACTOR_EN_OKUTA:
        case ACTOR_EN_BIGOKUTA:
            return has(OotSoul::ENEMY_OCTOROK);
        case ACTOR_EN_WALLMAS:
            return has(OotSoul::ENEMY_WALLMASTER);
        case ACTOR_EN_DODONGO:
            return has(OotSoul::ENEMY_DODONGO);
        case ACTOR_EN_FIREFLY:
            return has(OotSoul::ENEMY_KEESE);
        case ACTOR_EN_TITE:
            return has(OotSoul::ENEMY_TEKTITE);
        case ACTOR_EN_PEEHAT:
            return has(OotSoul::ENEMY_PEAHAT);
        case ACTOR_EN_ZF:
            return has(OotSoul::ENEMY_LIZALFOS_DINOLFOS);
        case ACTOR_EN_GOMA:
            if (scene == SCENE_DEKU_TREE_BOSS)
                return 1;
            return has(OotSoul::ENEMY_GOHMA_LARVA);
        case ACTOR_EN_BUBBLE:
            return has(OotSoul::ENEMY_SHABOM);
        case ACTOR_EN_DODOJR:
            return has(OotSoul::ENEMY_BABY_DODONGO);
        case ACTOR_EN_TORCH2:
            return has(OotSoul::ENEMY_DARK_LINK);
        case ACTOR_EN_BILI:
        case ACTOR_EN_VALI:
            return has(OotSoul::ENEMY_BIRI_BARI);
        case ACTOR_EN_TP:
            return has(OotSoul::ENEMY_TAILPASARN);
        case ACTOR_EN_ST:
            return has(OotSoul::ENEMY_SKULLTULA);
        case ACTOR_EN_BW:
            return has(OotSoul::ENEMY_TORCH_SLUG);
        case ACTOR_EN_MB:
            return has(OotSoul::ENEMY_MOBLIN);
        case ACTOR_EN_AM:
            if (variable == 0)
                return 1;
            return has(OotSoul::ENEMY_ARMOS);
        case ACTOR_EN_DEKUBABA:
        case ACTOR_EN_KAREBABA:
            return has(OotSoul::ENEMY_DEKU_BABA);
        case ACTOR_EN_DEKUNUTS:
        case ACTOR_EN_HINTNUTS:
            return has(OotSoul::ENEMY_DEKU_SCRUB);
        case ACTOR_EN_BB:
            return has(OotSoul::ENEMY_BUBBLE);
        case ACTOR_EN_VM:
            return has(OotSoul::ENEMY_BEAMOS);
        case ACTOR_EN_FLOORMAS:
            return has(OotSoul::ENEMY_FLOORMASTER);
        case ACTOR_EN_RD:
            return has(OotSoul::ENEMY_REDEAD_GIBDO);
        case ACTOR_EN_SW:
            if (variable & 0xe000)
                return optional(has(OotSoul::MISC_GS));
            return has(OotSoul::ENEMY_SKULLWALLTULA);
        case ACTOR_EN_FD:
            return has(OotSoul::ENEMY_FLARE_DANCER);
        case ACTOR_EN_DH:
        case ACTOR_EN_DHA:
            return has(OotSoul::ENEMY_DEAD_HAND);
        case ACTOR_EN_SB:
            return has(OotSoul::ENEMY_SHELL_BLADE);
        case ACTOR_EN_RR:
            return has(OotSoul::ENEMY_LIKE_LIKE);
        case ACTOR_EN_NY:
            return has(OotSoul::ENEMY_SPIKE);
        case ACTOR_EN_ANUBICE_TAG:
        case ACTOR_EN_ANUBICE:
            return has(OotSoul::ENEMY_ANUBIS);
        case ACTOR_EN_IK:
            return has(OotSoul::ENEMY_IRON_KNUCKLE);
        case ACTOR_EN_SKJ:
            if (!adult)
                return 1;
            return has(OotSoul::ENEMY_SKULL_KID);
        case ACTOR_EN_TUBO_TRAP:
            return optional(has(OotSoul::ENEMY_FLYING_POT));
        case ACTOR_EN_FZ:
            return has(OotSoul::ENEMY_FREEZARD);
        case ACTOR_EN_WEIYER:
        case ACTOR_EN_EIYER:
            return has(OotSoul::ENEMY_STINGER);
        case ACTOR_EN_WF:
            return has(OotSoul::ENEMY_WOLFOS);
        case ACTOR_EN_CROW:
            return has(OotSoul::ENEMY_GUAY);
        case ACTOR_BOSS_GOMA:
            return has(OotSoul::BOSS_QUEEN_GOHMA);
        case ACTOR_BOSS_DODONGO:
            return has(OotSoul::BOSS_KING_DODONGO);
        case ACTOR_BOSS_VA:
            return has(OotSoul::BOSS_BARINADE);
        case ACTOR_BOSS_GANONDROF:
            return has(OotSoul::BOSS_PHANTOM_GANON);
        case ACTOR_BOSS_FD:
            return has(OotSoul::BOSS_VOLVAGIA);
        case ACTOR_BOSS_MO:
            return has(OotSoul::BOSS_MORPHA);
        case ACTOR_BOSS_SST:
            return has(OotSoul::BOSS_BONGO_BONGO);
        case ACTOR_BOSS_TW:
            return has(OotSoul::BOSS_TWINROVA);
        case ACTOR_EN_BA:
            return has(OotSoul::ENEMY_PARASITE);
        case ACTOR_EN_REEBA:
            return has(OotSoul::ENEMY_LEEVER);
        case ACTOR_EN_SKB:
            return has(OotSoul::ENEMY_STALCHILD);
        case ACTOR_EN_SA:
            return optional(has(OotSoul::NPC_SARIA));
        case ACTOR_EN_DU:
            return optional(has(OotSoul::NPC_DARUNIA));
        case ACTOR_EN_RU1:
            return optional(has(OotSoul::NPC_RUTO));
        case ACTOR_EN_KZ:
            return optional(has(OotSoul::NPC_KING_ZORA));
        case ACTOR_EN_NIW_LADY:
            return optional(has(OotSoul::NPC_ANJU));
        case ACTOR_EN_TORYO:
        case ACTOR_EN_DAIKU:
        case ACTOR_EN_DAIKU_KAKARIKO:
            return optional(has(OotSoul::NPC_CARPENTERS));
        case ACTOR_EN_FU:
            return optional(has(OotSoul::NPC_GURU_GURU));
        case ACTOR_EN_MD:
            return optional(has(OotSoul::NPC_MIDO));
        case ACTOR_EN_KO:
            return optional(has(OotSoul::NPC_KOKIRI));
        case ACTOR_EN_OSSAN:
            switch (variable) {
                case 0:
                    return optional(has(OotSoul::NPC_KOKIRI_SHOPKEEPER));
                case 1:
                    return optional(has(OotSoul::NPC_POTION_SHOPKEEPER));
                case 2:
                    return optional(has(OotSoul::NPC_BOMBCHU_SHOPKEEPER));
                case 3:
                    return optional(has(OotSoul::NPC_POTION_SHOPKEEPER));
                case 4:
                    return optional(has(OotSoul::NPC_BAZAAR_SHOPKEEPER));
                case 7:
                    return optional(has(OotSoul::NPC_ZORA_SHOPKEEPER));
                case 8:
                    return optional(has(OotSoul::NPC_GORON_SHOPKEEPER));
                default:
                    return 1;
            }
            break;
        case ACTOR_EN_HEISHI2:
        case ACTOR_EN_HEISHI3:
        case ACTOR_EN_HEISHI4:
            return optional(has(OotSoul::NPC_HYLIAN_GUARD));
        case ACTOR_EN_ANI:
            return optional(has(OotSoul::NPC_ROOFTOP_MAN));
        case ACTOR_EN_MU:
        case ACTOR_EN_NIW_GIRL:
        case ACTOR_EN_MM:
        case ACTOR_EN_MM2:
            return optional(has(OotSoul::NPC_CITIZEN));
        case ACTOR_EN_HY:
            switch (variable & 0x3f) {
                case 0x00:
                    return optional(has(OotSoul::NPC_DOG_LADY));
                case 0x05:
                    return optional(has(OotSoul::NPC_BANKER));
                case 0x07:
                    return optional(has(OotSoul::NPC_ASTRONOMER));
                default:
                    return optional(has(OotSoul::NPC_CITIZEN));
            }
        case ACTOR_EN_MA1:
        case ACTOR_EN_MA2:
        case ACTOR_EN_MA3:
            return optional(has(OotSoul::NPC_MALON));
        case ACTOR_BG_SPOT15_RRBOX:
            if (scene != SCENE_HYRULE_CASTLE)
                return 1;
            /* Fallthrough */
        case ACTOR_EN_TA:
            return optional(has(OotSoul::NPC_TALON));
        case ACTOR_FISHING:
            return optional(has(OotSoul::NPC_FISHING_POND_OWNER));
        case ACTOR_EN_GO:
            return optional(has(OotSoul::NPC_GORON));
        case ACTOR_EN_GO2:
            switch (variable & 0x1f) {
                case 1:
                    return optional(has(OotSoul::NPC_GORON_CHILD));
                case 2:
                    return optional(has(OotSoul::NPC_BIGGORON));
                default:
                    return optional(has(OotSoul::NPC_GORON));
            }
            break;
        case ACTOR_EN_GM:
            return optional(has(OotSoul::NPC_MEDIGORON));
        case ACTOR_EN_ZO:
        case ACTOR_EN_DIVING_GAME:
            return optional(has(OotSoul::NPC_ZORA));
        case ACTOR_EN_BOM_BOWL_MAN:
        case ACTOR_BG_BOWL_WALL:
            return optional(has(OotSoul::NPC_BOMBCHU_BOWLING_LADY));
        case ACTOR_EN_SYATEKI_MAN:
            return optional(has(OotSoul::NPC_SHOOTING_GALLERY_OWNER));
        case ACTOR_EN_TK:
        case ACTOR_EN_PO_RELAY:
            return optional(has(OotSoul::NPC_DAMPE));
        case ACTOR_EN_CS:
            return optional(has(OotSoul::NPC_BOMBERS));
        case ACTOR_EN_GB:
            return optional(has(OotSoul::NPC_POE_COLLECTOR));
        case ACTOR_EN_XC:
            return optional(has(OotSoul::NPC_SHEIK));
        case ACTOR_EN_ZL1:
        case ACTOR_EN_ZL3:
        case ACTOR_EN_ZL4:
            return optional(has(OotSoul::NPC_ZELDA));
        case ACTOR_EN_GE1:
        case ACTOR_EN_GE2:
            return optional(has(OotSoul::NPC_THIEVES));
        case ACTOR_EN_GELDB:
            return has(OotSoul::ENEMY_THIEVES);
        case ACTOR_EN_NIW:
        case ACTOR_EN_NWC:
            return optional(has(OotSoul::ANIMAL_CUCCO));
        case ACTOR_EN_COW:
            return optional(has(OotSoul::ANIMAL_COW));
        case ACTOR_EN_DOG:
            return optional(has(OotSoul::ANIMAL_DOG));
        case ACTOR_EN_BUTTE:
            return optional(has(OotSoul::ANIMAL_BUTTERFLY));
        default:
            return 1;
    }
}
// Soul-bearing branches from pinned canSpawnActor, independent of room-clear suppression.
template <class Has>
inline int EvaluateOotAuxSoulPreSpawn(int actor, unsigned params, int scene, bool allStones, Has has) {
    if (actor == ACTOR_ITEM_OCARINA)
        return allStones && has(OotSoul::NPC_ZELDA) ? 1 : 0;
    if (actor == ACTOR_EN_RIVER_SOUND && scene == SCENE_MARKET_DAY && (params & 0xFF) == 0xA)
        return has(OotSoul::NPC_CITIZEN) ? 1 : -1;
    return 1;
}

} // namespace Rando::Souls
