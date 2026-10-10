#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

#include "2s2h/Rando/Logic/Logic.h"
#ifdef COMBO_BUILD
#include "rando/MmAdultWorldRoutes.h"
#define IKANA_ADULT_GEAR_MODE (RANDO_SAVE_OPTIONS[RO_MM_ADULT_MASK] || RANDO_SAVE_OPTIONS[RO_MM_STARTING_AGE] || RANDO_SAVE_OPTIONS[RO_MM_CROSS_AGE] || RANDO_SAVE_OPTIONS[RO_MM_BOOTS_HOVER])
#endif

using namespace Rando::Logic;

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_IKANA_CASTLE_BEFORE_THRONE] = RandoRegion{ .name = "Before Throne Room", .sceneId = SCENE_CASTLE,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(IGOS_DU_IKANAS_LAIR, 0),          ENTRANCE(IKANA_CASTLE, 6), true)
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_MAIN_ROOM, CAN_USE_MAGIC_ARROW(LIGHT))
        }
    };
    Regions[RR_IKANA_CASTLE_BUBBLE_ROOM] = RandoRegion{ .name = "Bubble Room", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_THIRD_ROOM_POT_01, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_THIRD_ROOM_POT_02, true),
            CHECK(RC_ENEMY_DROP_BLUE_BUBBLE, CanKillEnemy(ACTOR_EN_BB)),
        },
        .connections = {
            #ifdef COMBO_BUILD
            CONNECTION(RR_IKANA_CASTLE_SKULLTULA_ROOM, IKANA_ADULT_GEAR_MODE ? true : CAN_USE_SWORD),
#else
            CONNECTION(RR_IKANA_CASTLE_SKULLTULA_ROOM, CAN_USE_SWORD),
#endif
            CONNECTION(RR_IKANA_CASTLE_OUTER_ROOF, true)
        },
    };
    Regions[RR_IKANA_CASTLE_CEILING_ROOM] = RandoRegion{ .name = "Ceiling Room", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_FIRST_ROOM_POT_01, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_FIRST_ROOM_POT_02, true)
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_MAIN_ROOM, true),
            #ifdef COMBO_BUILD
            CONNECTION(RR_IKANA_CASTLE_SKULLTULA_ROOM, IKANA_ADULT_GEAR_MODE ? ComboRando::MmAdultWorldRoutes::IkanaNorthEntry(HAS_ITEM(ITEM_MASK_DEKU), false, CAN_USE_NAYRUS_LOVE, MM_LOGIC_IS_TALL) : CAN_BE_DEKU),
#else
            CONNECTION(RR_IKANA_CASTLE_SKULLTULA_ROOM, CAN_BE_DEKU),
#endif
        },
    };
    Regions[RR_IKANA_CASTLE_COURTYARD] = RandoRegion{ .name = "Courtyard", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_01, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_02, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_03, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_04, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_05, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_06, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_07, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_08, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_09, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_10, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_11, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_COURTYARD_GRASS_12, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_EXTERIOR_POT, true),
            CHECK(RC_ENEMY_DROP_GUAY, CanKillEnemy(ACTOR_EN_CROW)),
            CHECK(RC_ENEMY_DROP_GARO, CanKillEnemy(ACTOR_EN_JSO)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(BENEATH_THE_WELL, 1),             ENTRANCE(IKANA_CASTLE, 0), true),
            EXIT(ENTRANCE(IKANA_CASTLE, 3),                 ENTRANCE(IKANA_CASTLE, 2), CAN_RESET_TIME_DUNGEON && (true)),
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_FRONT_ENTRANCE, CAN_USE_MAGIC_ARROW(LIGHT)),
        },
    };
    Regions[RR_IKANA_CASTLE_FLOORMASTER_ROOM] = RandoRegion{ .name = "Floormaster Room", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ENEMY_DROP_FLOORMASTER, CanKillEnemy(ACTOR_EN_FLOORMAS)),
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_MAIN_ROOM, true),
            CONNECTION(RR_IKANA_CASTLE_FLOORMASTER_ROOM_REDEAD_AREA, CAN_USE_MAGIC_ARROW(LIGHT) || (RANDO_EVENTS[RE_IKANA_CASTLE_RIGHT_SUNLIGHT] && (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) >= EQUIP_VALUE_SHIELD_MIRROR))),
        },
        .oneWayEntrances = {
            ENTRANCE(IKANA_CASTLE, 5), // From Inner Roof
        }
    };
    Regions[RR_IKANA_CASTLE_FLOORMASTER_ROOM_REDEAD_AREA] = RandoRegion{ .name = "Floormaster Room, Redead Area", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ENEMY_DROP_REDEAD, CanKillEnemy(ACTOR_EN_RD)),
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_FLOORMASTER_ROOM, CAN_USE_MAGIC_ARROW(LIGHT)),
            CONNECTION(RR_IKANA_CASTLE_WIZZROBE_ROOM, CAN_RESET_TIME),
        },
    };
    Regions[RR_IKANA_CASTLE_FRONT_ENTRANCE] = RandoRegion{ .name = "Front Entrance", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ENEMY_DROP_REDEAD, CanKillEnemy(ACTOR_EN_RD)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(IKANA_CANYON, 8),                 ENTRANCE(IKANA_CASTLE, 1), true),
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_COURTYARD, CAN_USE_MAGIC_ARROW(LIGHT) || ((GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) >= EQUIP_VALUE_SHIELD_MIRROR) && (CAN_USE_SWORD || CAN_BE_GORON || CAN_USE_PROJECTILE))),
        },
    };
    Regions[RR_IKANA_CASTLE_INNER_ROOF] = RandoRegion{ .name = "Inner Roof", .sceneId = SCENE_CASTLE,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(IKANA_CASTLE, 5),                          ONE_WAY_EXIT, RANDO_EVENTS[RE_IKANA_CASTLE_RIGHT_SUNLIGHT]),
            EXIT(ENTRANCE(IKANA_CASTLE, 4),                          ONE_WAY_EXIT, RANDO_EVENTS[RE_IKANA_CASTLE_MAIN_SUNLIGHT])
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_REDEAD_WALKWAY, true)
        },
        .events = {
            EVENT(RE_IKANA_CASTLE_MAIN_SUNLIGHT, CAN_USE_POWDER_KEG),
        }
    };
    Regions[RR_IKANA_CASTLE_MAIN_ROOM] = RandoRegion{ .name = "Main Room", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_ENTRANCE_POT_01, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_ENTRANCE_POT_02, true)
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(IKANA_CASTLE, 2),                 ENTRANCE(IKANA_CASTLE, 3), CAN_RESET_TIME && (true))
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_CEILING_ROOM, CAN_USE_MAGIC_ARROW(FIRE)),
            CONNECTION(RR_IKANA_CASTLE_FLOORMASTER_ROOM, CAN_USE_MAGIC_ARROW(FIRE)),
            CONNECTION(RR_IKANA_CASTLE_BEFORE_THRONE, CAN_USE_MAGIC_ARROW(LIGHT) || (RANDO_EVENTS[RE_IKANA_CASTLE_MAIN_SUNLIGHT] && (GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) >= EQUIP_VALUE_SHIELD_MIRROR)))
        },
        .oneWayEntrances = {
            ENTRANCE(IKANA_CASTLE, 4), // From Inner Roof Powder Keg Hole
        }
    };
    Regions[RR_IKANA_CASTLE_OUTER_ROOF] = RandoRegion{ .name = "Outer Roof", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_PIECE_OF_HEART, CAN_USE_PROJECTILE && CAN_BE_DEKU)
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_BUBBLE_ROOM , true),
            CONNECTION(RR_IKANA_CASTLE_FRONT_ENTRANCE, true)
        },
        .events = {
            EVENT(RE_IKANA_CASTLE_RIGHT_SUNLIGHT, true),
        }
    };
    Regions[RR_IKANA_CASTLE_REDEAD_WALKWAY] = RandoRegion{ .name = "Redead Walkway", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_RIGHT_POT_01, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_RIGHT_POT_02, true),
            CHECK(RC_ENEMY_DROP_REDEAD, CanKillEnemy(ACTOR_EN_RD)),
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_WIZZROBE_ROOM, true),
            CONNECTION(RR_IKANA_CASTLE_INNER_ROOF, true)
        },
    };
    Regions[RR_IKANA_CASTLE_SKULLTULA_ROOM] = RandoRegion{ .name = "Skulltula Room", .sceneId = SCENE_CASTLE,
        .checks = {
            #ifdef COMBO_BUILD
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_01, IKANA_ADULT_GEAR_MODE ? ComboRando::MmAdultWorldRoutes::IkanaNorthPots(HAS_ITEM(ITEM_MASK_DEKU), false, MM_LOGIC_HAS_HOVER_BOOTS) : (CAN_BE_DEKU || CAN_USE_BOW)),
#else
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_01, CAN_BE_DEKU || CAN_USE_BOW),
#endif
            #ifdef COMBO_BUILD
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_02, IKANA_ADULT_GEAR_MODE ? ComboRando::MmAdultWorldRoutes::IkanaNorthPots(HAS_ITEM(ITEM_MASK_DEKU), false, MM_LOGIC_HAS_HOVER_BOOTS) : (CAN_BE_DEKU || CAN_USE_BOW)),
#else
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_02, CAN_BE_DEKU || CAN_USE_BOW),
#endif
            #ifdef COMBO_BUILD
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_03, IKANA_ADULT_GEAR_MODE ? ComboRando::MmAdultWorldRoutes::IkanaNorthPots(HAS_ITEM(ITEM_MASK_DEKU), false, MM_LOGIC_HAS_HOVER_BOOTS) : (CAN_BE_DEKU || CAN_USE_BOW)),
#else
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_03, CAN_BE_DEKU || CAN_USE_BOW),
#endif
            #ifdef COMBO_BUILD
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_04, IKANA_ADULT_GEAR_MODE ? ComboRando::MmAdultWorldRoutes::IkanaNorthPots(HAS_ITEM(ITEM_MASK_DEKU), false, MM_LOGIC_HAS_HOVER_BOOTS) : (CAN_BE_DEKU || CAN_USE_BOW)),
#else
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_LEFT_SECOND_ROOM_POT_04, CAN_BE_DEKU || CAN_USE_BOW),
#endif
            CHECK(RC_ENEMY_DROP_SKULLTULA, CanKillEnemy(ACTOR_EN_ST)),
        },
        .connections = {
            #ifdef COMBO_BUILD
            CONNECTION(RR_IKANA_CASTLE_CEILING_ROOM, IKANA_ADULT_GEAR_MODE ? true : (CAN_USE_SWORD || CAN_USE_PROJECTILE)),
#else
            CONNECTION(RR_IKANA_CASTLE_CEILING_ROOM, CAN_USE_SWORD || CAN_USE_PROJECTILE),
#endif
            #ifdef COMBO_BUILD
            CONNECTION(RR_IKANA_CASTLE_BUBBLE_ROOM, IKANA_ADULT_GEAR_MODE ? ComboRando::MmAdultWorldRoutes::IkanaNorthExit(CAN_NAVIGATE_HIDDEN, false, MM_LOGIC_HAS_HOVER_BOOTS) : (CAN_NAVIGATE_HIDDEN && CAN_BE_DEKU && (CAN_USE_BOW || CAN_USE_BOMBS)))
#else
            CONNECTION(RR_IKANA_CASTLE_BUBBLE_ROOM, CAN_NAVIGATE_HIDDEN && CAN_BE_DEKU && (CAN_USE_BOW || CAN_USE_BOMBS))
#endif
        },
    };
    Regions[RR_IKANA_CASTLE_THRONE_ROOM] = RandoRegion{ .name = "Throne Room", .sceneId = SCENE_IKNINSIDE,
        .checks = {
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS, CanKillEnemy(ACTOR_EN_KNIGHT)),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_01, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_02, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_03, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_04, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_05, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_06, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_07, true),
            CHECK(RC_ANCIENT_CASTLE_OF_IKANA_BOSS_POT_08, true),
            CHECK(RC_ENEMY_DROP_IGOS_DU_IKANA, CanKillEnemy(ACTOR_EN_KNIGHT)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(IKANA_CASTLE, 6),                 ENTRANCE(IGOS_DU_IKANAS_LAIR, 0), true)
        }
    };
    Regions[RR_IKANA_CASTLE_WIZZROBE_ROOM] = RandoRegion{ .name = "Wizrobe Room", .sceneId = SCENE_CASTLE,
        .checks = {
            CHECK(RC_ENEMY_DROP_WIZROBE, CanKillEnemy(ACTOR_EN_WIZ)),
        },
        .connections = {
            CONNECTION(RR_IKANA_CASTLE_FLOORMASTER_ROOM_REDEAD_AREA, CanKillEnemy(ACTOR_EN_WIZ)),
            CONNECTION(RR_IKANA_CASTLE_REDEAD_WALKWAY, CanKillEnemy(ACTOR_EN_WIZ))
        },
    };
}, RANDO_LOGIC_INIT_PATHS);
// clang-format on
