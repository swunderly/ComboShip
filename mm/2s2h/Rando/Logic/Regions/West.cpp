#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

#include "2s2h/Rando/Logic/Logic.h"
#ifdef COMBO_BUILD
#include "rando/MmAdultWorldRoutes.h"
#endif
#ifdef COMBO_BUILD
#include "rando/MmEggLogic.h"
#endif

using namespace Rando::Logic;
#ifdef COMBO_BUILD
#define COMBO_WORLD37_ENABLED                                                          \
    (RANDO_SAVE_OPTIONS[RO_MM_ADULT_MASK] || RANDO_SAVE_OPTIONS[RO_MM_STARTING_AGE] || \
     RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] || RANDO_SAVE_OPTIONS[RO_MM_BOOTS_IRON] ||    \
     RANDO_SAVE_OPTIONS[RO_MM_BOOTS_HOVER] || RANDO_SAVE_OPTIONS[RO_MM_TUNIC_ZORA])
#endif

static bool CanLearnNewWaveBossaNova() {
#ifdef COMBO_BUILD
    return ComboRando::CanLearnNewWaveBossaNova(COMBO_LOGIC_RULE("gEnhancements.Songs.ZoraEggCount"),
                                                RANDO_EVENTS[RE_ACCESS_ZORA_EGG], CAN_BE_ZORA, HAS_PLAYABLE_OCARINA,
                                                HAS_BOTTLE);
#else
    return CAN_BE_ZORA && HAS_PLAYABLE_OCARINA && RANDO_EVENTS[RE_ACCESS_ZORA_EGG] >= 7;
#endif
}

#ifdef COMBO_BUILD
#define COMBO_WORLD54_ENABLED (COMBO_WORLD37_ENABLED || RANDO_SAVE_OPTIONS[RO_MM_SPELL_LOVE])
#define COMBO_WORLD54_SELECT(legacy, pinned) (COMBO_WORLD54_ENABLED ? (pinned) : (legacy))
#define COMBO_WORLD54_PENINSULA \
    (MM_LOGIC_UNDERWATER_WALKING_STRICT || CAN_USE_NAYRUS_LOVE || (CAN_USE_MAGIC_ARROW(ICE) && CAN_SUPPLY_CHATEAU))
#else
#define COMBO_WORLD54_SELECT(legacy, pinned) (legacy)
#endif

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_FISHERMANS_HUT] = RandoRegion{ .sceneId = SCENE_FISHERMAN,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GREAT_BAY_COAST, 4),              ENTRANCE(FISHERMANS_HUT, 0), true),
        },
        .events = {
            // TODO: Should this be a check?
            EVENT(RE_ACCESS_SEAHORSE, RANDO_EVENTS[RE_ACCESS_PIRATE_PICTURE] && HAS_ITEM(ITEM_PICTOGRAPH_BOX) && HAS_BOTTLE),
        },
    };
    Regions[RR_GREAT_BAY_COAST_COW_GROTTO] = RandoRegion{ .name = "Great Bay Coast Cow Grotto", .sceneId = SCENE_KAKUSIANA,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_COW_BACK, CAN_PLAY_SONG(EPONA)),
            CHECK(RC_GREAT_BAY_COAST_COW_FRONT, CAN_PLAY_SONG(EPONA)),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_01, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_02, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_03, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_04, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_05, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_06, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_07, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_08, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_09, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_10, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_11, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_12, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_13, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_14, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_15, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_16, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_17, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_18, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_19, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_20, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_21, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_22, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_23, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_24, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_25, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_26, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_27, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_28, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_29, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_30, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_31, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_32, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_33, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_34, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_35, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_36, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_37, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_38, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_39, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_40, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_41, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_42, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_43, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_44, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_45, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_46, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_47, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_48, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_49, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_50, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_51, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_52, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_53, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_54, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_55, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_56, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_57, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_58, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_59, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_60, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_61, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_62, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_63, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_64, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_65, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_66, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_67, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_68, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_69, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_70, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_71, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_GRASS_72, true),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_BUTTERFLY_01, CAN_USE_STICKS),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_BUTTERFLY_02, CAN_USE_STICKS),
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_BUTTERFLY_03, CAN_USE_STICKS),
            CHECK(RC_ENEMY_DROP_GIANT_BEE, CAN_USE_PROJECTILE && CanKillEnemy(ACTOR_EN_BEE)), // In a beehive
            CHECK(RC_GREAT_BAY_COAST_COW_GROTTO_BEEHIVE, CAN_USE_PROJECTILE),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GREAT_BAY_COAST, 0),              ENTRANCE(GROTTOS, 27), true),
        }
    };
    Regions[RR_GREAT_BAY_COAST_FISHERMAN_GROTTO] = RandoRegion{ .name = "Great Bay Coast Fisherman Grotto", .sceneId = SCENE_KAKUSIANA,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_CHEST, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_01, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_02, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_03, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_04, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_05, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_06, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_07, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_08, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_09, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_10, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_11, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_12, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_13, true),
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_GROTTO_GRASS_14, true),
            CHECK(RC_ENEMY_DROP_MINI_BABA, CanKillEnemy(ACTOR_EN_KAREBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GREAT_BAY_COAST, 0),              ENTRANCE(GROTTOS, 26), true),
        },
    };
    Regions[RR_GREAT_BAY_COAST] = RandoRegion{ .sceneId = SCENE_30GYOSON,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_MIKAU, CAN_USE_ABILITY(SWIM) && CAN_PLAY_SONG(HEALING)),
            CHECK(RC_GREAT_BAY_COAST_POT_03, true),
            CHECK(RC_GREAT_BAY_COAST_POT_04, true),
            CHECK(RC_GREAT_BAY_COAST_GRASS_01, true),
            CHECK(RC_GREAT_BAY_COAST_GRASS_02, true),
            CHECK(RC_GREAT_BAY_COAST_GRASS_03, true),
            CHECK(RC_GREAT_BAY_COAST_GRASS_04, true),
            CHECK(RC_GREAT_BAY_COAST_GRASS_05, true),
            CHECK(RC_GREAT_BAY_COAST_BUTTERFLY_01, CAN_USE_STICKS),
            CHECK(RC_GREAT_BAY_COAST_BUTTERFLY_02, CAN_USE_STICKS),
            CHECK(RC_GREAT_BAY_COAST_TREE_02, true),
            CHECK(RC_GREAT_BAY_COAST_TREE_03, true),
            CHECK(RC_GREAT_BAY_COAST_TREE_04, true),
            CHECK(RC_ENEMY_DROP_LEEVER, CanKillEnemy(ACTOR_EN_NEO_REEBA)),
            CHECK(RC_ENEMY_DROP_LIKE_LIKE, CanKillEnemy(ACTOR_EN_RR)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(TERMINA_FIELD, 2),                ENTRANCE(GREAT_BAY_COAST, 0), true),
            EXIT(ENTRANCE(ZORA_CAPE, 0),                    ENTRANCE(GREAT_BAY_COAST, 1), true),
            EXIT(ENTRANCE(PINNACLE_ROCK, 0),                ENTRANCE(GREAT_BAY_COAST, 3), CAN_USE_ABILITY(SWIM)),
            EXIT(ENTRANCE(FISHERMANS_HUT, 0),               ENTRANCE(GREAT_BAY_COAST, 4), true),
            EXIT(ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0),    ENTRANCE(GREAT_BAY_COAST, 5), COMBO_WORLD54_SELECT(CAN_BE_ZORA && CAN_USE_ABILITY(SWIM), CAN_RESET_TIME_DUNGEON && MM_LOGIC_UNDERWATER_WALKING_STRICT)),
            EXIT(ENTRANCE(OCEANSIDE_SPIDER_HOUSE, 0),       ENTRANCE(GREAT_BAY_COAST, 8), true),
            EXIT(ENTRANCE(GROTTOS, 26),                     ENTRANCE(GREAT_BAY_COAST, 0), true),
        },
        .connections = {
            CONNECTION(RR_GREAT_BAY_COAST_CLIFFSIDE, CAN_USE_ABILITY(SWIM)),
            CONNECTION(RR_GREAT_BAY_COAST_MARINE_LAB_EXTERIOR, CAN_USE_ABILITY(SWIM)),
            CONNECTION(RR_GREAT_BAY_COAST_PIRATE_LEDGE, CAN_USE_ABILITY(SWIM)),
        },
    };
    // Infinite Deku hopping only widens the downward/surface return to the beach.
    // Reaching raised platforms, diving, Mikau and the fortress still require their original abilities.
    Regions[RR_GREAT_BAY_COAST_MARINE_LAB_EXTERIOR] = RandoRegion{ .sceneId = SCENE_30GYOSON,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_OWL_STATUE, CAN_ACTIVATE_OWL),
            CHECK(RC_GREAT_BAY_COAST_POT_09, true),
            CHECK(RC_GREAT_BAY_COAST_POT_10, true),
            CHECK(RC_GREAT_BAY_COAST_POT_11, true),
            CHECK(RC_GREAT_BAY_COAST_POT_12, true),
            CHECK(RC_GREAT_BAY_COAST_TINGLE_MAP_01, (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT)) && CAN_AFFORD(RC_GREAT_BAY_COAST_TINGLE_MAP_01)),
            CHECK(RC_GREAT_BAY_COAST_TINGLE_MAP_02, (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT)) && CAN_AFFORD(RC_GREAT_BAY_COAST_TINGLE_MAP_02)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(MARINE_RESEARCH_LAB, 0),          ENTRANCE(GREAT_BAY_COAST, 7), true),
        },
        .connections = {
            CONNECTION(RR_GREAT_BAY_COAST, CAN_CROSS_OPEN_WATER_SURFACE),
        },
        .events = {
            EVENT(RE_ACCESS_PICTOGRAPH_TINGLE, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_TINGLE) && (HAS_ITEM(ITEM_PICTOGRAPH_BOX))),
        },
        .oneWayEntrances = {
            ENTRANCE(GREAT_BAY_COAST, 11), // From Song of Soaring
        },
    };
    Regions[RR_GREAT_BAY_COAST_MINIGAME_PLATFORMS] = RandoRegion{ .sceneId = SCENE_30GYOSON,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_FISHERMAN_MINIGAME, (!NativeMmErContextsEnabled() || (NativeMmOceanCleared())) && (CAN_PAY(20) && NativeMmOceanCleared() && (HAS_ITEM(ITEM_HOOKSHOT)) && (BETWEEN(TIME_DAY1_AM_07_00, TIME_NIGHT1_AM_04_00) || BETWEEN(TIME_DAY2_AM_07_00, TIME_NIGHT2_AM_04_00) || BETWEEN(TIME_DAY3_AM_07_00, TIME_NIGHT3_AM_04_00)))),
            CHECK(RC_GREAT_BAY_COAST_TREE_01, (!NativeMmErContextsEnabled() || (NativeMmOceanCleared())) && (true)),
        },
        .connections = {
            CONNECTION(RR_GREAT_BAY_COAST_CLIFFSIDE, CAN_USE_ABILITY(SWIM)),
        },
    };
    Regions[RR_GREAT_BAY_COAST_CLIFFSIDE] = RandoRegion{ .sceneId = SCENE_30GYOSON,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_PIECE_OF_HEART, CAN_HOOK_SCARECROW && CAN_GROW_BEAN_PLANT),
            CHECK(RC_GREAT_BAY_COAST_POT_05, true),
            CHECK(RC_GREAT_BAY_COAST_POT_06, true),
            CHECK(RC_GREAT_BAY_COAST_POT_07, true),
            CHECK(RC_GREAT_BAY_COAST_POT_08, true),
            CHECK(RC_GREAT_BAY_COAST_LEDGE_POT_01, HAS_ITEM(ITEM_HOOKSHOT)),
            CHECK(RC_GREAT_BAY_COAST_LEDGE_POT_02, HAS_ITEM(ITEM_HOOKSHOT)),
            CHECK(RC_GREAT_BAY_COAST_LEDGE_POT_03, HAS_ITEM(ITEM_HOOKSHOT)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GROTTOS, 27),                     ENTRANCE(GREAT_BAY_COAST, 0), HAS_ITEM(ITEM_HOOKSHOT)),
        },
        .connections = {
            CONNECTION(RR_GREAT_BAY_COAST, CAN_CROSS_OPEN_WATER_SURFACE),
            CONNECTION(RR_GREAT_BAY_COAST_MINIGAME_PLATFORMS, NativeMmOceanCleared() && (HAS_ITEM(ITEM_HOOKSHOT))),
        },
    };
    Regions[RR_GREAT_BAY_GREAT_FAIRY_FOUNTAIN] = RandoRegion{ .sceneId = SCENE_YOUSEI_IZUMI,
        .checks = {
            CHECK(RC_GREAT_BAY_GREAT_FAIRY, HAS_ENOUGH_STRAY_FAIRIES(DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_CAPE, 5),                    ENTRANCE(FAIRY_FOUNTAIN, 3), true),
        },
    };
    Regions[RR_GREAT_BAY_COAST_PIRATE_LEDGE] = RandoRegion{ .sceneId = SCENE_30GYOSON,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_POT_01, true),
            CHECK(RC_GREAT_BAY_COAST_POT_02, true),
        },
        .connections = {
            CONNECTION(RR_GREAT_BAY_COAST, CAN_CROSS_OPEN_WATER_SURFACE),
        },
        .oneWayEntrances = {
            // This region can feel like overkill now, but this one-way entrance will be significant for entrance rando.
            ENTRANCE(GREAT_BAY_COAST, 12), // From being captured in Pirate Fortress Moat
        },
    };
    Regions[RR_MARINE_RESEARCH_LAB] = RandoRegion{ .sceneId = SCENE_LABO,
        .checks = {
            CHECK(RC_GREAT_BAY_COAST_NEW_WAVE_BOSSA_NOVA, CanLearnNewWaveBossaNova()),
            CHECK(RC_GREAT_BAY_COAST_MARINE_LAB_FISH_PIECE_OF_HEART, HAS_BOTTLE && CAN_ACCESS(FISH)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GREAT_BAY_COAST, 7),              ENTRANCE(MARINE_RESEARCH_LAB, 0), true),
        },
    };
    Regions[RR_PINNACLE_ROCK_ENTRANCE] = RandoRegion{ .name = "Entrance", .sceneId = SCENE_SINKAI,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GREAT_BAY_COAST, 3),              ENTRANCE(PINNACLE_ROCK, 0), true),
        },
        .connections = {
            // Pinned pinnacle_rock.yml: strict underwater walking permits owned Iron Boots + Zora Tunic.
#ifdef COMBO_BUILD
            CONNECTION(RR_PINNACLE_ROCK_INNER, CAN_CROSS_PINNACLE_ROCK ||
                (RANDO_SAVE_OPTIONS[RO_MM_BOOTS_IRON] && HAS_ITEM(ITEM_COMBO_BOOTS_IRON) &&
                 RANDO_SAVE_OPTIONS[RO_MM_TUNIC_ZORA] && HAS_ITEM(ITEM_COMBO_TUNIC_ZORA) &&
                 (RANDO_EVENTS[RE_ACCESS_SEAHORSE] || COMBO_MM_TRICK("PinnacleNoSeahorse") != 0)))
#else
            CONNECTION(RR_PINNACLE_ROCK_INNER, CAN_CROSS_PINNACLE_ROCK)
#endif
        }
    };
    Regions[RR_PINNACLE_ROCK_INNER] = RandoRegion{ .name = "Inner", .sceneId = SCENE_SINKAI,
        .checks = {
            CHECK(RC_PINNACLE_ROCK_CHEST_01,          CAN_BE_ZORA),
            CHECK(RC_PINNACLE_ROCK_CHEST_02,          CanKillEnemy(ACTOR_EN_DRAGON)),
            CHECK(RC_PINNACLE_ROCK_POT_01,            CanKillEnemy(ACTOR_EN_DRAGON)),
            CHECK(RC_PINNACLE_ROCK_POT_02,            CanKillEnemy(ACTOR_EN_DRAGON)),
            CHECK(RC_PINNACLE_ROCK_POT_03,            CanKillEnemy(ACTOR_EN_DRAGON)),
            CHECK(RC_PINNACLE_ROCK_POT_04,            CanKillEnemy(ACTOR_EN_DRAGON)),
            CHECK(RC_PINNACLE_ROCK_POT_05,            CanKillEnemy(ACTOR_EN_DRAGON)),
            CHECK(RC_PINNACLE_ROCK_POT_06,            CAN_BE_ZORA),
            CHECK(RC_PINNACLE_ROCK_POT_07,            CAN_BE_ZORA),
            CHECK(RC_PINNACLE_ROCK_POT_08,            CAN_BE_ZORA),
            CHECK(RC_PINNACLE_ROCK_POT_09,            CAN_BE_ZORA),
            CHECK(RC_PINNACLE_ROCK_POT_10,            CAN_BE_ZORA),
            CHECK(RC_PINNACLE_ROCK_POT_11,            CAN_BE_ZORA),
            CHECK(RC_PINNACLE_ROCK_REUNITE_SEAHORSE,  CanKillEnemy(ACTOR_EN_DRAGON) && RANDO_EVENTS[RE_ACCESS_SEAHORSE]),
            CHECK(RC_ENEMY_DROP_DEEP_PYTHON,          CanKillEnemy(ACTOR_EN_DRAGON)),
        },
        .connections = {
            CONNECTION(RR_PINNACLE_ROCK_ENTRANCE, CAN_USE_ABILITY(SWIM))
        },
        .events = {
            EVENT(RE_ACCESS_ZORA_EGG, HAS_BOTTLE && CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
            EVENT(RE_ACCESS_ZORA_EGG, HAS_BOTTLE && CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
            EVENT(RE_ACCESS_ZORA_EGG, HAS_BOTTLE && CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
        },
    };
    Regions[RR_WATERFALL_RAPIDS] = RandoRegion{ .sceneId = SCENE_35TAKI,
        .checks = {
            CHECK(RC_WATERFALL_RAPIDS_BEAVER_RACE_01, CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
            CHECK(RC_WATERFALL_RAPIDS_BEAVER_RACE_02, CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_CAPE, 4),                    ENTRANCE(WATERFALL_RAPIDS, 0), true),
        },
    };
    Regions[RR_ZORA_CAPE_BEFORE_GREAT_BAY_TEMPLE] = RandoRegion{ .sceneId = SCENE_31MISAKI,
        .checks = {
            CHECK(RC_ZORA_CAPE_NEAR_OWL_STATUE_POT_01, true),
            CHECK(RC_ZORA_CAPE_NEAR_OWL_STATUE_POT_02, true),
            CHECK(RC_ZORA_CAPE_NEAR_OWL_STATUE_POT_03, true),
            CHECK(RC_ZORA_CAPE_NEAR_OWL_STATUE_POT_04, true),
            CHECK(RC_ZORA_CAPE_OWL_STATUE,            CAN_ACTIVATE_OWL),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_HALL,        1),             ENTRANCE(ZORA_CAPE, 2), true),
            EXIT(ENTRANCE(GREAT_BAY_TEMPLE, 0),             ENTRANCE(ZORA_CAPE, 7), HAS_ITEM(ITEM_HOOKSHOT) && CanAccessDungeon(DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE)),
        },
        .connections = {
            CONNECTION(RR_ZORA_CAPE, COMBO_WORLD54_SELECT(CAN_BE_ZORA, COMBO_WORLD54_PENINSULA)),
        },
        .oneWayEntrances = {
            ENTRANCE(ZORA_CAPE, 6), // From Song of Soaring
            ENTRANCE(ZORA_CAPE, 9), // From Great Bay Temple Blue Warp
        }
    };
    Regions[RR_ZORA_CAPE_GROTTO] = RandoRegion{ .name = "Zora Cape Grotto", .sceneId = SCENE_KAKUSIANA,
        .checks = {
            CHECK(RC_ZORA_CAPE_GROTTO_CHEST, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_01, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_02, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_03, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_04, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_05, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_06, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_07, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_08, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_09, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_10, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_11, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_12, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_13, true),
            CHECK(RC_ZORA_CAPE_GROTTO_GRASS_14, true),
            CHECK(RC_ENEMY_DROP_MINI_BABA, CanKillEnemy(ACTOR_EN_KAREBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_CAPE, 0),                    ENTRANCE(GROTTOS, 28), true),
        },
    };
    Regions[RR_ZORA_CAPE_OUTSIDE_FAIRY_FOUNTAIN] = RandoRegion{ .sceneId = SCENE_31MISAKI,
        .checks = {
            CHECK(RC_ENEMY_DROP_GUAY, CanKillEnemy(ACTOR_EN_CROW)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(FAIRY_FOUNTAIN, 3),               ENTRANCE(ZORA_CAPE, 5), CAN_USE_EXPLOSIVE),
        },
        .connections = {
            CONNECTION(RR_ZORA_CAPE, CAN_USE_ABILITY(SWIM)),
        }
    };
    Regions[RR_ZORA_CAPE] = RandoRegion{ .sceneId = SCENE_31MISAKI,
        .checks = {
            CHECK(RC_ZORA_CAPE_LEDGE_CHEST_01,             HAS_ITEM(ITEM_HOOKSHOT)),
            CHECK(RC_ZORA_CAPE_LEDGE_CHEST_02,             HAS_ITEM(ITEM_HOOKSHOT)),
#ifdef COMBO_BUILD
            CHECK(RC_ZORA_CAPE_UNDERWATER_CHEST, COMBO_WORLD37_ENABLED ? (MM_LOGIC_UNDERWATER_WALKING_STRICT) : (          CAN_BE_ZORA && CAN_USE_ABILITY(SWIM))),
#else
            CHECK(RC_ZORA_CAPE_UNDERWATER_CHEST,           CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
#endif
#ifdef COMBO_BUILD
            CHECK(RC_ZORA_CAPE_WATERFALL_PIECE_OF_HEART, COMBO_WORLD37_ENABLED ? (HAS_ITEM(ITEM_MASK_ZORA) || (MM_LOGIC_HAS_IRON_BOOTS && MM_LOGIC_HAS_ZORA_TUNIC_STRICT && (CAN_USE_BOW || HAS_ITEM(ITEM_MASK_BLAST)))) : (  CAN_BE_ZORA && CAN_USE_ABILITY(SWIM))),
#else
            CHECK(RC_ZORA_CAPE_WATERFALL_PIECE_OF_HEART,   CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
#endif
            CHECK(RC_ZORA_CAPE_NEAR_BEAVERS_POT_01,        true),
            CHECK(RC_ZORA_CAPE_NEAR_BEAVERS_POT_02,        true),
            CHECK(RC_ZORA_CAPE_TREE_01,                    true),
            CHECK(RC_ZORA_CAPE_TREE_02,                    true),
#ifdef COMBO_BUILD
            CHECK(RC_ZORA_CAPE_TREE_03, COMBO_WORLD37_ENABLED ? (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && (MM_LOGIC_IS_TALL || HAS_ITEM(ITEM_HOOKSHOT)))) : (                   (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA)))),
#else
            CHECK(RC_ZORA_CAPE_TREE_03,                    (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA))),
#endif
#ifdef COMBO_BUILD
            CHECK(RC_ZORA_CAPE_TREE_04, COMBO_WORLD37_ENABLED ? (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && (MM_LOGIC_IS_TALL || HAS_ITEM(ITEM_HOOKSHOT)))) : (                   (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA)))),
#else
            CHECK(RC_ZORA_CAPE_TREE_04,                    (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA))),
#endif
#ifdef COMBO_BUILD
            CHECK(RC_ZORA_CAPE_TREE_05, COMBO_WORLD37_ENABLED ? (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && (MM_LOGIC_IS_TALL || HAS_ITEM(ITEM_HOOKSHOT)))) : (                   (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA)))),
#else
            CHECK(RC_ZORA_CAPE_TREE_05,                    (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA))),
#endif
            CHECK(RC_ENEMY_DROP_LEEVER,                    CanKillEnemy(ACTOR_EN_NEO_REEBA)),
            CHECK(RC_ENEMY_DROP_LIKE_LIKE,                 CanKillEnemy(ACTOR_EN_RR)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GREAT_BAY_COAST, 1),              ENTRANCE(ZORA_CAPE, 0), true),
            EXIT(ENTRANCE(GROTTOS, 28),                     ENTRANCE(ZORA_CAPE, 0), CAN_USE_EXPLOSIVE || CAN_BE_GORON),
            EXIT(ENTRANCE(ZORA_HALL, 0),                    ENTRANCE(ZORA_CAPE, 1), COMBO_WORLD54_SELECT(CAN_BE_ZORA && CAN_USE_ABILITY(SWIM), MM_LOGIC_UNDERWATER_WALKING_STRICT)),
            EXIT(ENTRANCE(WATERFALL_RAPIDS, 0),             ENTRANCE(ZORA_CAPE, 4), HAS_ITEM(ITEM_HOOKSHOT)),
        },
        .connections = {
            CONNECTION(RR_ZORA_CAPE_BEFORE_GREAT_BAY_TEMPLE, COMBO_WORLD54_SELECT(CAN_BE_ZORA, COMBO_WORLD54_PENINSULA)),
#ifdef COMBO_BUILD
            CONNECTION(RR_ZORA_CAPE_OUTSIDE_FAIRY_FOUNTAIN, COMBO_WORLD37_ENABLED ? ((HAS_ITEM(ITEM_HOOKSHOT) && CAN_USE_EXPLOSIVE) || (CAN_USE_MAGIC_ARROW(ICE) && (MM_LOGIC_IS_TALL || HAS_ITEM(ITEM_HOOKSHOT)))) : (HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA))),
#else
            CONNECTION(RR_ZORA_CAPE_OUTSIDE_FAIRY_FOUNTAIN, HAS_ITEM(ITEM_HOOKSHOT) || (CAN_USE_MAGIC_ARROW(ICE) && CAN_BE_ZORA)),
#endif
        }
    };
    Regions[RR_ZORA_HALL_EVANS_ROOM] = RandoRegion{ .name = "Evan's Room", .sceneId = SCENE_BANDROOM,
        .checks = {
            CHECK(RC_ZORA_HALL_EVANS_PIECE_OF_HEART,           HAS_PLAYABLE_OCARINA && (canPlaySong(OCARINA_SONG_EVAN_PART1) && canPlaySong(OCARINA_SONG_EVAN_PART2))),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_HALL, 4),                    ENTRANCE(ZORA_HALL_ROOMS, 3), true),
        },
    };
    Regions[RR_ZORA_HALL_JAPAS_ROOM] = RandoRegion{ .name = "Japa's Room", .sceneId = SCENE_BANDROOM,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_HALL, 5),                    ENTRANCE(ZORA_HALL_ROOMS, 1), true),
        },
    };
    Regions[RR_ZORA_HALL_LULUS_ROOM] = RandoRegion{ .name = "Lulu's Room", .sceneId = SCENE_BANDROOM,
        .checks = {
            CHECK(RC_ZORA_HALL_SCRUB_DEED,           Flags_GetRandoInf(RANDO_INF_OBTAINED_DEED_MOUNTAIN) && CAN_BE_GORON),
#ifdef COMBO_BUILD
            CHECK(RC_ZORA_HALL_SCRUB_PIECE_OF_HEART, (RANDO_SAVE_OPTIONS[RO_MM_ADULT_MASK] || RANDO_SAVE_OPTIONS[RO_MM_STARTING_AGE] || RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE])
                ? ComboRando::MmAdultWorldRoutes::ZoraHallScrubHeart(COMBO_MM_TRICK("ZoraHallScrubNoDeku"),
                    HAS_ITEM(ITEM_MASK_GORON), MM_LOGIC_IS_TALL, HAS_ITEM(ITEM_MASK_DEKU),
                    Flags_GetRandoInf(RANDO_INF_OBTAINED_DEED_MOUNTAIN), false, false)
                : (Flags_GetRandoInf(RANDO_INF_OBTAINED_DEED_MOUNTAIN) && CAN_BE_GORON && CAN_BE_DEKU)),
#else
            CHECK(RC_ZORA_HALL_SCRUB_PIECE_OF_HEART, Flags_GetRandoInf(RANDO_INF_OBTAINED_DEED_MOUNTAIN) && CAN_BE_GORON && CAN_BE_DEKU),
#endif
            CHECK(RC_ZORA_HALL_SCRUB_POTION_REFILL,  CAN_BE_ZORA),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_HALL, 3),                    ENTRANCE(ZORA_HALL_ROOMS, 2), true),
        },
        .events = {
#ifndef COMBO_BUILD
            EVENT(RE_ACCESS_GREEN_POTION_REFILL, Rando::HaveExpandedSoul(Rando::ExpandedSoul::MISC_BUSINESS_SCRUB) && CAN_BE_ZORA),
#endif // COMBO_BUILD: actual saved renewable stock is resolved centrally.
        },
    };
    Regions[RR_ZORA_HALL_MIKAUS_ROOM] = RandoRegion{ .name = "Mikau's Room", .sceneId = SCENE_BANDROOM,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_HALL, 6),                    ENTRANCE(ZORA_HALL_ROOMS, 0), true),
        },
    };
    Regions[RR_ZORA_HALL_SHOP] = RandoRegion{ .name = "Shop", .sceneId = SCENE_BANDROOM,
        .checks = {
            CHECK(RC_ZORA_SHOP_ITEM_01, CAN_AFFORD(RC_ZORA_SHOP_ITEM_01)),
            CHECK(RC_ZORA_SHOP_ITEM_02, CAN_AFFORD(RC_ZORA_SHOP_ITEM_02)),
            CHECK(RC_ZORA_SHOP_ITEM_03, CAN_AFFORD(RC_ZORA_SHOP_ITEM_03)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_HALL, 2),                    ENTRANCE(ZORA_HALL_ROOMS, 5), true),
        },
    };
    Regions[RR_ZORA_HALL] = RandoRegion{ .sceneId = SCENE_33ZORACITY,
        .checks = {
            // OoTMM zora_hall.yml:19: can_use_fire_arrows || (trick(MM_STAGE_LIGHTS_DIN) && has_arrows && can_use_din && can_hookshot)
            CHECK(RC_ZORA_HALL_SCENE_LIGHTS, CAN_USE_MAGIC_ARROW(FIRE) || (COMBO_MM_TRICK("StageLightsDin") && CAN_USE_BOW && CAN_USE_DINS_FIRE && HAS_ITEM(ITEM_HOOKSHOT))),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ZORA_CAPE, 1),                    ENTRANCE(ZORA_HALL, 0), COMBO_WORLD54_SELECT(true, MM_LOGIC_UNDERWATER_WALKING_STRICT)),
            EXIT(ENTRANCE(ZORA_CAPE, 2),                    ENTRANCE(ZORA_HALL, 1), true), // Back entrance beside the owl           
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 5),              ENTRANCE(ZORA_HALL, 2), true), // To Shop
#ifdef COMBO_BUILD
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 2),              ENTRANCE(ZORA_HALL, 3), ((RANDO_SAVE_OPTIONS[RO_MM_ADULT_MASK] || RANDO_SAVE_OPTIONS[RO_MM_STARTING_AGE] || RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE]) ? ComboRando::MmAdultWorldRoutes::ZoraHallDoor(HAS_ITEM(ITEM_MASK_ZORA), RANDO_EVENTS[RE_CLEARED_GREAT_BAY_TEMPLE], false, false) : CAN_BE_ZORA)), // To Lulu's Room
#else
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 2),              ENTRANCE(ZORA_HALL, 3), CAN_BE_ZORA), // To Lulu's Room
#endif
#ifdef COMBO_BUILD
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 3),              ENTRANCE(ZORA_HALL, 4), ((RANDO_SAVE_OPTIONS[RO_MM_ADULT_MASK] || RANDO_SAVE_OPTIONS[RO_MM_STARTING_AGE] || RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE]) ? ComboRando::MmAdultWorldRoutes::ZoraHallDoor(HAS_ITEM(ITEM_MASK_ZORA), RANDO_EVENTS[RE_CLEARED_GREAT_BAY_TEMPLE], false, false) : CAN_BE_ZORA)), // To Evan's Room
#else
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 3),              ENTRANCE(ZORA_HALL, 4), CAN_BE_ZORA), // To Evan's Room
#endif
#ifdef COMBO_BUILD
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 1),              ENTRANCE(ZORA_HALL, 5), ((RANDO_SAVE_OPTIONS[RO_MM_ADULT_MASK] || RANDO_SAVE_OPTIONS[RO_MM_STARTING_AGE] || RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE]) ? ComboRando::MmAdultWorldRoutes::ZoraHallDoor(HAS_ITEM(ITEM_MASK_ZORA), RANDO_EVENTS[RE_CLEARED_GREAT_BAY_TEMPLE], false, false) : CAN_BE_ZORA)), // To Japas's Room
#else
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 1),              ENTRANCE(ZORA_HALL, 5), CAN_BE_ZORA), // To Japas's Room
#endif
#ifdef COMBO_BUILD
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 0),              ENTRANCE(ZORA_HALL, 6), ((RANDO_SAVE_OPTIONS[RO_MM_ADULT_MASK] || RANDO_SAVE_OPTIONS[RO_MM_STARTING_AGE] || RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE]) ? ComboRando::MmAdultWorldRoutes::ZoraHallDoor(HAS_ITEM(ITEM_MASK_ZORA), RANDO_EVENTS[RE_CLEARED_GREAT_BAY_TEMPLE], false, false) : CAN_BE_ZORA)), // To Mikaus's Room
#else
            EXIT(ENTRANCE(ZORA_HALL_ROOMS, 0),              ENTRANCE(ZORA_HALL, 6), CAN_BE_ZORA), // To Mikaus's Room
#endif
        },
    };
}, RANDO_LOGIC_INIT_PATHS);
// clang-format on
