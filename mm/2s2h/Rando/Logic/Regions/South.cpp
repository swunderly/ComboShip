#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

#include "2s2h/Rando/Logic/Logic.h"
#ifdef COMBO_BUILD
#include "rando/MmAdultWorldRoutes.h"
#endif

#include "Rando/ActorBehavior/ExpandedSoulsRoutes.h"
using namespace Rando::Logic;
#ifdef COMBO_BUILD
#define COMBO_WORLD45_ENABLED MM_LOGIC_SOUTH_PINNED
#define COMBO_WORLD45_SELECT(legacy, pinned) (COMBO_WORLD45_ENABLED ? (pinned) : (legacy))
#else
#define COMBO_WORLD45_SELECT(legacy, pinned) (legacy)
#endif

#define CAN_TRAVERSE_WAIST_DEEP_WATER CAN_WADE_SHALLOW_WATER

inline bool CanGetPastBigOctoWithoutBoat() {
    // After killing Big Octo the water is still poisoned. OoTMM's
    // cross_poison_swamp permits Deku hopping and the taller Zora/Goron forms,
    // whose heads stay above this shallow water; submerged damage boosts stay out.
    if (RANDO_EVENTS[RE_SOUTHERN_SWAMP_KILL_OCTOROK] && (CAN_BE_DEKU || CAN_BE_ZORA || CAN_BE_GORON)) {
        return true;
    }
    /*
     * The water is clear in this state, so swimming comes into question. Either Link uses the swim ability, he hops
     * over the water as Deku, or he uses one of the forms tall enough to walk through the water. This may also be
     * covered for FD only when its anywhere capability is a seed logic assumption.
     */
    if (NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER) {
        return true;
    }
    return false;
}

inline bool CanGetPastBigOcto() {
    // Boat ride. This takes Link to a landing in the next region and thus has no additional requirements.
    if (RANDO_EVENTS[RE_SOUTHERN_SWAMP_RIDE_BOAT]) {
        return true;
    }

    return CanGetPastBigOctoWithoutBoat();
}

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_DEKU_KINGS_CHAMBER_HOLDING_CELL] = RandoRegion{ .name = "Holding Cell", .sceneId = SCENE_DEKU_KING,
        .checks = {
            CHECK(RC_DEKU_KINGS_CHAMBER_MONKEY, (!NativeMmErContextsEnabled() || (NativeMmSwampPoisoned())) && (CAN_BE_DEKU && HAS_PLAYABLE_OCARINA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(DEKU_PALACE, 3),                  ENTRANCE(DEKU_KINGS_CHAMBER, 1), true),
        },
    };
    Regions[RR_DEKU_KINGS_CHAMBER] = RandoRegion{ .sceneId = SCENE_DEKU_KING,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(DEKU_PALACE, 2),                  ENTRANCE(DEKU_KINGS_CHAMBER, 0), true),
        },
        .events = {
            EVENT(RE_ACCESS_PICTOGRAPH_DEKU_KING, HAS_ITEM(ITEM_PICTOGRAPH_BOX) && CAN_BE_DEKU),
            EVENT(RE_RETURN_DEKU_PRINCESS, HAS_BOTTLE && CAN_ACCESS(DEKU_PRINCESS)),
        }
    };
    Regions[RR_DEKU_PALACE_BEAN_SALESMAN_GROTTO] = RandoRegion{ .name = "Deku Palace Bean Salesman Grotto", .sceneId = SCENE_KAKUSIANA,
        .checks = {
            CHECK(RC_DEKU_PALACE_GROTTO_CHEST, CAN_GROW_BEAN_PLANT || HAS_ITEM(ITEM_HOOKSHOT)),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_01, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_02, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_03, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_04, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_05, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_06, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_07, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_08, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_09, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_10, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_11, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_GRASS_12, true),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_BUTTERFLY_01, CAN_USE_STICKS),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_BUTTERFLY_02, CAN_USE_STICKS),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_BUTTERFLY_03, CAN_USE_STICKS),
            CHECK(RC_DEKU_PALACE_BEAN_SALESMAN_GROTTO_BUTTERFLY_04, CAN_USE_STICKS),
            // TODO: Bean salesman check
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(DEKU_PALACE, 9),                  ENTRANCE(GROTTOS, 12), true),
        },
        .events = {
            // OoTMM MAGIC_BEANS_PALACE: this unshuffled salesman supplies
            // beans even when the player has not found a randomized bean.
            EVENT(RE_ACCESS_BEANS_REFILL, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_BEAN_SALESMAN) && CAN_PAY(10)),
            EVENT(RE_ACCESS_SPRING_WATER, true),
        },
    };
    Regions[RR_DEKU_PALACE_INSIDE_LOWER] = RandoRegion{ .name = "Inside, Lower", .sceneId = SCENE_22DEKUCITY,
        .checks = {
#ifdef COMBO_BUILD
            // Pinned Deku Palace Main: child height, owned Deku Mask, or bean-skip trick.
            CHECK(RC_DEKU_PALACE_PIECE_OF_HEART, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS),
#else
            CHECK(RC_DEKU_PALACE_PIECE_OF_HEART,    true),
#endif
            // deku_palace.yml:33-54 and us/deku_palace_us.yml:19-20: 15 of the Deku Palace Main rupees are free.
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_01, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_02, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_03, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_04, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_05, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_06, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_07, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_08, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_09, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_10, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_11, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_12, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_13, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_14, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_15, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_16, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_17, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_18, COMBO_WORLD45_SELECT(CAN_BE_DEKU, MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_19, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_20, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_21, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_22, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_23, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
            CHECK(RC_DEKU_PALACE_FREESTANDING_RUPEE_24, COMBO_WORLD45_SELECT(CAN_BE_DEKU, true)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(DEKU_KINGS_CHAMBER, 0),           ENTRANCE(DEKU_PALACE, 2), true),
#ifdef COMBO_BUILD
            EXIT(ENTRANCE(GROTTOS, 12), ENTRANCE(DEKU_PALACE, 9),
                MM_LOGIC_IS_CHILD || HAS_ITEM(ITEM_MASK_DEKU) || CAN_SKIP_PALACE_BEANS),
#else
            EXIT(ENTRANCE(GROTTOS, 12),                     ENTRANCE(DEKU_PALACE, 9), true),
#endif
        },
        .connections = {
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE, CAN_SKIP_PALACE_BEANS),
            CONNECTION(RR_DEKU_PALACE_OUTSIDE, true),
        },
    };
    Regions[RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE_LEDGE] = RandoRegion{ .name = "Inside, Upper", .sceneId = SCENE_22DEKUCITY,
        // TODO: Region exists for entrance rando
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(DEKU_KINGS_CHAMBER, 1),           ENTRANCE(DEKU_PALACE, 3), true), // Cell
        },
        .connections = {
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU))),
            CONNECTION(RR_DEKU_PALACE_INSIDE_LOWER, true),
        },
    };
    // The optional bean skip enters this west-garden ledge via the doorway jump.
    // Deku flower flight and Mad Scrub combat still gate onward travel.
    Regions[RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE] = RandoRegion{ .name = "Inside, Upper", .sceneId = SCENE_22DEKUCITY,
        .checks = {
            CHECK(RC_ENEMY_DROP_MAD_SCRUB, CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU),
        },
        .connections = {
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE_LEDGE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU))),
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_MIDDLE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU))),
            CONNECTION(RR_DEKU_PALACE_INSIDE_LOWER, true),
        },
    };
    Regions[RR_DEKU_PALACE_INSIDE_UPPER_MIDDLE] = RandoRegion { .name = "Inside, Upper", .sceneId = SCENE_22DEKUCITY,
        .checks = {
            CHECK(RC_DEKU_PALACE_POT_01, COMBO_WORLD45_SELECT(true, HAS_ITEM(ITEM_MASK_DEKU))),
            CHECK(RC_DEKU_PALACE_POT_02, COMBO_WORLD45_SELECT(true, HAS_ITEM(ITEM_MASK_DEKU))),
        },
        .connections = {
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU))),
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_BEAN_SIDE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU))),
            CONNECTION(RR_DEKU_PALACE_INSIDE_LOWER, true),
        }
    };
    Regions[RR_DEKU_PALACE_INSIDE_UPPER_BEAN_SIDE] = RandoRegion { .name = "Inside, Upper", .sceneId = SCENE_22DEKUCITY,
        .checks = {
            CHECK(RC_ENEMY_DROP_MAD_SCRUB, CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU),
        },
        .connections = {
            CONNECTION(RR_DEKU_PALACE_OUTSIDE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, true)),
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_MIDDLE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU))),
            CONNECTION(RR_DEKU_PALACE_INSIDE_LOWER, true),
        }
    };
    Regions[RR_DEKU_PALACE_OUTSIDE] = RandoRegion{ .name = "Outside", .sceneId = SCENE_22DEKUCITY,
        .checks = {
            CHECK(RC_ENEMY_DROP_MINI_BABA, (CAN_BE_DEKU || (NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER)) && CanKillEnemy(ACTOR_EN_KAREBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),      ENTRANCE(DEKU_PALACE, 0), true),
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 4),      ENTRANCE(DEKU_PALACE, 5), CAN_BE_DEKU), // Treetop
            EXIT(ENTRANCE(DEKU_SHRINE, 0),                  ENTRANCE(DEKU_PALACE, 4), COMBO_WORLD45_SELECT(NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER, (NativeMmSwampCleared() && (CAN_USE_ABILITY(SWIM) || HAS_ITEM(ITEM_MASK_ZORA))) || (CAN_USE_NAYRUS_LOVE && (HAS_ITEM(ITEM_MASK_ZORA) || (MM_LOGIC_IS_ADULT && (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_DINS_FIRE)))) || (MM_LOGIC_HAS_HOVER_BOOTS && (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_DINS_FIRE)))),
        },
        .connections = {
            CONNECTION(RR_DEKU_PALACE_INSIDE_LOWER, CAN_BE_DEKU),
            CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_BEAN_SIDE, COMBO_WORLD45_SELECT((CAN_BE_DEKU || (NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER)) && CAN_USE_DAY2_RAIN_BEAN, (((NativeMmSwampCleared() || CAN_USE_NAYRUS_LOVE) && (CAN_USE_ABILITY(SWIM) || HAS_ITEM(ITEM_MASK_ZORA))) || HAS_ITEM(ITEM_MASK_DEKU)) && CAN_USE_DAY2_RAIN_BEAN)), // deku_palace.yml:9
        },
    };
    Regions[RR_DEKU_SHRINE_ENTRANCE] = RandoRegion{ .name = "Entrance", .sceneId = SCENE_DANPEI,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(DEKU_PALACE, 4),                  ENTRANCE(DEKU_SHRINE, 0), COMBO_WORLD45_SELECT(true, ((NativeMmSwampCleared() || CAN_USE_NAYRUS_LOVE) && (CAN_USE_ABILITY(SWIM) || HAS_ITEM(ITEM_MASK_ZORA))) || HAS_ITEM(ITEM_MASK_DEKU) || MM_LOGIC_HAS_HOVER_BOOTS)),
        },
        .connections = {
            CONNECTION(RR_DEKU_SHRINE, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_BUTLER_DEKU) && RANDO_EVENTS[RE_RETURN_DEKU_PRINCESS]),
        },
    };
    Regions[RR_DEKU_SHRINE] = RandoRegion{ .sceneId = SCENE_DANPEI,
        .checks = {
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_01,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_02,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_03,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_04,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_05,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_06,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_07,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_08,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_09,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_10,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_11,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_12,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_13,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_14,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_15,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_16,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_17,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_18,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_19,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_20,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_21,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_22,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_23,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_24,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_25,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_26,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_27,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_28,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_29,             true),
            CHECK(RC_DEKU_SHRINE_FREESTANDING_RUPEE_30,             true),
            CHECK(RC_DEKU_SHRINE_POT_01,                            true),
            CHECK(RC_DEKU_SHRINE_POT_02,                            true),
            CHECK(RC_DEKU_SHRINE_MASK_OF_SCENTS,                    true),
        },
        .connections = {
            CONNECTION(RR_DEKU_SHRINE_ENTRANCE, RANDO_EVENTS[RE_RETURN_DEKU_PRINCESS]),
        },
    };
    Regions[RR_MAGIC_HAGS_POTION_SHOP] = RandoRegion{ .sceneId = SCENE_WITCH_SHOP,
        .checks = {
            CHECK(RC_HAGS_POTION_SHOP_FREESTANDING_RUPEE, true),
            // TODO: Add CAN_ACCESS(MUSHROOM) once that is shuffled.
            CHECK(RC_HAGS_POTION_SHOP_ITEM_01, (FIRST_DAY() || (RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] == RO_GENERIC_YES ? RANDO_EVENTS[RE_EXPANDED_MEET_KOUME] : RANDO_EVENTS[RE_SAVED_KOUME])) && CAN_AFFORD(RC_HAGS_POTION_SHOP_ITEM_01) && HAS_ITEM(ITEM_MASK_SCENTS) && HAS_BOTTLE),
            CHECK(RC_HAGS_POTION_SHOP_ITEM_02, (FIRST_DAY() || (RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] == RO_GENERIC_YES ? RANDO_EVENTS[RE_EXPANDED_MEET_KOUME] : RANDO_EVENTS[RE_SAVED_KOUME])) && CAN_AFFORD(RC_HAGS_POTION_SHOP_ITEM_02)),
            CHECK(RC_HAGS_POTION_SHOP_ITEM_03, (FIRST_DAY() || (RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] == RO_GENERIC_YES ? RANDO_EVENTS[RE_EXPANDED_MEET_KOUME] : RANDO_EVENTS[RE_SAVED_KOUME])) && CAN_AFFORD(RC_HAGS_POTION_SHOP_ITEM_03)),
            CHECK(RC_HAGS_POTION_SHOP_KOTAKE, RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] != RO_GENERIC_YES || Rando::ExpandedKotakeReward(FIRST_DAY(), RANDO_EVENTS[RE_EXPANDED_MEET_KOUME_DAY1])),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 5),      ENTRANCE(MAGIC_HAGS_POTION_SHOP, 0), true),
        },
        .events = {
            EVENT(RE_ACCESS_RED_POTION_REFILL, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_KOUME_KOTAKE) && (RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] != RO_GENERIC_YES || FIRST_DAY() || RANDO_EVENTS[RE_EXPANDED_MEET_KOUME]) && !RANDO_SAVE_CHECKS[RC_HAGS_POTION_SHOP_ITEM_03].shuffled && CAN_PAY(30)),
            // Kotake gives the first blue potion free for a mushroom each cycle.
            // Shuffled shop stock replaces that gift, so it cannot supply blue potion.
            EVENT(RE_ACCESS_BLUE_POTION_REFILL, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_KOUME_KOTAKE) && (RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] != RO_GENERIC_YES || FIRST_DAY() || RANDO_EVENTS[RE_EXPANDED_MEET_KOUME]) && !RANDO_SAVE_CHECKS[RC_HAGS_POTION_SHOP_ITEM_01].shuffled &&
                  HAS_ITEM(ITEM_MASK_SCENTS) && HAS_BOTTLE),
        },
    };
    Regions[RR_ROAD_TO_SOUTHERN_SWAMP_GROTTO] = RandoRegion{ .name = "Road to Southern Swamp Grotto", .sceneId = SCENE_KAKUSIANA,
        .checks = {
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_CHEST, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_01, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_02, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_03, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_04, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_05, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_06, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_07, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_08, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_09, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_10, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_11, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_12, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_13, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GROTTO_GRASS_14, true),
            CHECK(RC_ENEMY_DROP_MINI_BABA, CanKillEnemy(ACTOR_EN_KAREBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0),       ENTRANCE(GROTTOS, 20), true),
        },
    };
    Regions[RR_ROAD_TO_SOUTHERN_SWAMP] = RandoRegion{ .sceneId = SCENE_24KEMONOMITI,
        .checks = {
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_PIECE_OF_HEART, CanKillEnemy(ACTOR_EN_BAT) && (CAN_USE_PROJECTILE || CAN_USE_BOMBCHUS)),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_01, CAN_USE_PROJECTILE && CAN_AFFORD(RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_01)),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_02, CAN_USE_PROJECTILE && CAN_AFFORD(RC_ROAD_TO_SOUTHERN_SWAMP_TINGLE_MAP_02)),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_01, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_02, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_03, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_04, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_05, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_06, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_07, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_08, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_09, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_10, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_11, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_12, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_13, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_14, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_15, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_16, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_17, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_18, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_19, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_GRASS_20, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TREE_01, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TREE_02, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TREE_03, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TREE_04, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TREE_05, true),
            CHECK(RC_ROAD_TO_SOUTHERN_SWAMP_TREE_06, true),
            CHECK(RC_ENEMY_DROP_DEKU_BABA, CanKillEnemy(ACTOR_EN_DEKUBABA)),
            CHECK(RC_ENEMY_DROP_CHUCHU, CanKillEnemy(ACTOR_EN_SLIME) && IS_DAY()), // Day only
            CHECK(RC_ENEMY_DROP_WOLFOS, CanKillEnemy(ACTOR_EN_WF) && IS_NIGHT()), // Night only
            CHECK(RC_ENEMY_DROP_BAD_BAT, CanKillEnemy(ACTOR_EN_BAT)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(TERMINA_FIELD, 1),                ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0), true),
            EXIT(ENTRANCE(GROTTOS, 20),                     ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0), true),
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 0),      ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 1), true),
            EXIT(ENTRANCE(SWAMP_SHOOTING_GALLERY, 0),       ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 2), BEFORE(TIME_NIGHT1_PM_10_00) || BETWEEN(TIME_DAY2_AM_06_00, TIME_NIGHT2_PM_10_00) || BETWEEN(TIME_DAY3_AM_06_00, TIME_NIGHT3_PM_10_00)),
        },
        .events = {
            EVENT(RE_ACCESS_PICTOGRAPH_TINGLE, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_TINGLE) && (HAS_ITEM(ITEM_PICTOGRAPH_BOX))),
            EVENT(RE_ACCESS_SPRING_WATER, true),
        },
    };
    Regions[RR_SOUTHERN_SWAMP_GROTTO] = RandoRegion{ .name = "Southern Swamp Grotto", .sceneId = SCENE_KAKUSIANA,
        .checks = {
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_CHEST, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_01, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_02, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_03, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_04, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_05, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_06, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_07, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_08, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_09, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_10, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_11, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_12, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_13, true),
            CHECK(RC_SOUTHERN_SWAMP_GROTTO_GRASS_14, true),
            CHECK(RC_ENEMY_DROP_MINI_BABA, CanKillEnemy(ACTOR_EN_KAREBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),      ENTRANCE(GROTTOS, 21), true),
        },
    };
    Regions[RR_SOUTHERN_SWAMP_NORTH] = RandoRegion{ .name = "North Tourist Section", .sceneId = SCENE_20SICHITAI,
        .checks = {
            CHECK(RC_SOUTHERN_SWAMP_POISON_TOURIST_GRASS_01, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_TOURIST_GRASS_02, true),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_TOURIST_GRASS_01, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_TOURIST_GRASS_02, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_PIECE_OF_HEART, CAN_BE_DEKU && Flags_GetRandoInf(RANDO_INF_OBTAINED_DEED_LAND)),
            CHECK(RC_SOUTHERN_SWAMP_SCRUB_DEED, Flags_GetRandoInf(RANDO_INF_OBTAINED_DEED_LAND)),
            CHECK(RC_SOUTHERN_SWAMP_SCRUB_BEANS, CAN_BE_DEKU),
            CHECK(RC_SOUTHERN_SWAMP_OWL_STATUE, CAN_ACTIVATE_OWL),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_01, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_02, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_03, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_04, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_05, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_06, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_07, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_08, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_09, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_10, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_11, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_12, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_01, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_02, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_03, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_04, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_05, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_06, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_07, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_08, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_09, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_10, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_11, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_12, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_BEEHIVE, CAN_USE_PROJECTILE && CAN_TRAVERSE_WAIST_DEEP_WATER),
            CHECK(RC_ENEMY_DROP_MINI_BABA, CanKillEnemy(ACTOR_EN_KAREBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 1),       ENTRANCE(SOUTHERN_SWAMP_POISONED, 0), true),
            EXIT(ENTRANCE(TOURIST_INFORMATION, 0),          ENTRANCE(SOUTHERN_SWAMP_POISONED, 1), true),
        },
        .connections = {
            CONNECTION(RR_SOUTHERN_SWAMP_SOUTH, CanGetPastBigOcto()), // Via Octo near Tourist Center
    		CONNECTION(RR_SOUTHERN_SWAMP_NEAR_FLOWERS, CAN_TRAVERSE_WAIST_DEEP_WATER),
    		CONNECTION(RR_SOUTHERN_SWAMP_NEAR_WOODS, CAN_TRAVERSE_WAIST_DEEP_WATER),
        },
        .events = {
            EVENT(RE_ACCESS_SPRING_WATER, true),
            // Finite owned beans are not a refill source. The Palace salesman
            // supplies RE_ACCESS_BEANS_REFILL through its actual NPC event.
            EVENT(RE_SOUTHERN_SWAMP_KILL_OCTOROK, (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || CAN_BE_ZORA || (CAN_USE_DEITY_OUTSIDE_BOSS && HAS_MAGIC) || CAN_USE_NAYRUS_LOVE || CAN_USE_DINS_FIRE)), // southern_swamp.yml:23, :98, :111 kill_big_octo_close (macros_mm.yml:143)
            EVENT(RE_ACCESS_PICTOGRAPH_SWAMP_GENERIC, HAS_ITEM(ITEM_PICTOGRAPH_BOX)),
        },
        .oneWayEntrances = {
            ENTRANCE(SOUTHERN_SWAMP_POISONED, 10), // From Song of Soaring
        }
    };
    Regions[RR_SOUTHERN_SWAMP_NEAR_FLOWERS] = RandoRegion{ .name = "Flowers Section", .sceneId = SCENE_20SICHITAI,
        .checks = {
            CHECK(RC_SOUTHERN_SWAMP_FROG, HAS_ITEM(ITEM_MASK_DON_GERO)),
            CHECK(RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_01, CAN_BE_DEKU || CAN_BE_ZORA),
            CHECK(RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_02, CAN_BE_DEKU || CAN_BE_ZORA),
            CHECK(RC_ENEMY_DROP_OCTOROK, CanKillEnemy(ACTOR_EN_OKUTA) && (CAN_USE_ABILITY(SWIM) || CAN_BE_ZORA || CAN_BE_GORON)),
        },
        .connections = {
            CONNECTION(RR_SOUTHERN_SWAMP_SOUTH, CanGetPastBigOctoWithoutBoat()),
    		CONNECTION(RR_SOUTHERN_SWAMP_NORTH, CAN_TRAVERSE_WAIST_DEEP_WATER),
            CONNECTION(RR_SOUTHERN_SWAMP_NEAR_WOODS, CAN_TRAVERSE_WAIST_DEEP_WATER),
        },
        .events = {
            // Any form but Deku can bottle water here. Human can do it from a lilypad before it sinks.
            // If human is ever shuffled, revisit this.
            EVENT(RE_ACCESS_SPRING_WATER, true),
            EVENT(RE_SOUTHERN_SWAMP_KILL_OCTOROK, (CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT) || CAN_BE_ZORA || (CAN_USE_DEITY_OUTSIDE_BOSS && HAS_MAGIC) || CAN_USE_NAYRUS_LOVE || CAN_USE_DINS_FIRE)), // southern_swamp.yml:23, :98, :111 kill_big_octo_close (macros_mm.yml:143)
            EVENT(RE_ACCESS_PICTOGRAPH_SWAMP_GENERIC, HAS_ITEM(ITEM_PICTOGRAPH_BOX)),
        },
    };
    Regions[RR_SOUTHERN_SWAMP_NEAR_WOODS] = RandoRegion{ .name = "North Woods Section", .sceneId = SCENE_20SICHITAI,
        .checks = {
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_WOODS_GRASS_01, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_WOODS_GRASS_02, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_POISON_WOODS_GRASS_01, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_WOODS_GRASS_02, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_POT_01, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_POT_02, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_POT_03, true),
            CHECK(RC_SOUTHERN_SWAMP_CLEAR_POT_01, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEAR_POT_02, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEAR_POT_03, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_13, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_14, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_15, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_16, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_17, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_18, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_19, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_20, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_21, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_22, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_23, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_24, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_25, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_26, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_27, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_28, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_29, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_CLEARED_GRASS_30, NativeMmSwampCleared()),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_13, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_14, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_15, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_16, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_17, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_18, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_19, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_20, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_21, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_22, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_23, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_24, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_25, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_26, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_27, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_28, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_29, true),
            CHECK(RC_SOUTHERN_SWAMP_POISON_GRASS_30, true),
            CHECK(RC_ENEMY_DROP_MINI_BABA, CanKillEnemy(ACTOR_EN_KAREBABA)),
            CHECK(RC_ENEMY_DROP_DEKU_BABA, CanKillEnemy(ACTOR_EN_DEKUBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(MAGIC_HAGS_POTION_SHOP, 0),       ENTRANCE(SOUTHERN_SWAMP_POISONED, 5), true),
            EXIT(ENTRANCE(WOODS_OF_MYSTERY, 0),             ENTRANCE(SOUTHERN_SWAMP_POISONED, 7), true),
        },
        .connections = {
            CONNECTION(RR_SOUTHERN_SWAMP_NEAR_FLOWERS, CAN_TRAVERSE_WAIST_DEEP_WATER),
    		CONNECTION(RR_SOUTHERN_SWAMP_NORTH, CAN_TRAVERSE_WAIST_DEEP_WATER),
        },
        .events = {
            EVENT(RE_EXPANDED_SWAMP_KOTAKE, SECOND_DAY() || FINAL_DAY()),
            EVENT(RE_ACCESS_SPRING_WATER, true),
            EVENT(RE_ACCESS_PICTOGRAPH_SWAMP_GENERIC, HAS_ITEM(ITEM_PICTOGRAPH_BOX)),
        },
        .oneWayEntrances = {
            ENTRANCE(SOUTHERN_SWAMP_POISONED, 9), // From river in Ikana
        }
    };
    Regions[RR_SOUTHERN_SWAMP_SOUTH] = RandoRegion{ .name = "South Section", .sceneId = SCENE_20SICHITAI,
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GROTTOS, 21),                     ENTRANCE(SOUTHERN_SWAMP_POISONED, 3), CAN_BE_DEKU || CAN_BE_ZORA || (NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER)),
            EXIT(ENTRANCE(DEKU_PALACE, 0),                  ENTRANCE(SOUTHERN_SWAMP_POISONED, 3), true),
            // southern_swamp.yml:178: has_sticks || has_arrows; Din's Fire does not count.
            EXIT(ENTRANCE(SWAMP_SPIDER_HOUSE, 0),           ENTRANCE(SOUTHERN_SWAMP_POISONED, 8), (CAN_USE_STICKS || CAN_USE_BOW) && (CAN_BE_DEKU || CAN_BE_ZORA || (NativeMmSwampCleared() && CAN_USE_ABILITY(SWIM)))),
        },
        .connections = {
            CONNECTION(RR_SOUTHERN_SWAMP_NORTH, CanGetPastBigOcto()),
            CONNECTION(RR_SOUTHERN_SWAMP_NEAR_FLOWERS, CanGetPastBigOctoWithoutBoat()),
            CONNECTION(RR_SOUTHERN_SWAMP_SOUTH_UPPER, NativeMmSwampCleared() && CAN_TRAVERSE_WAIST_DEEP_WATER), // Climbable vines, only in cleared swamp
        },
        .events = {
            EVENT(RE_ACCESS_PICTOGRAPH_SWAMP_GENERIC, HAS_ITEM(ITEM_PICTOGRAPH_BOX))
        }
    };
    Regions[RR_SOUTHERN_SWAMP_SOUTH_UPPER] = RandoRegion{ .name = "Upper South Section", .sceneId = SCENE_20SICHITAI,
        .checks = {
            CHECK(RC_SOUTHERN_SWAMP_SONG_OF_SOARING, CAN_BE_DEKU),
            CHECK(RC_ENEMY_DROP_DRAGONFLY, CanKillEnemy(ACTOR_EN_GRASSHOPPER)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(WOODFALL, 0),                     ENTRANCE(SOUTHERN_SWAMP_POISONED, 2), CAN_BE_DEKU),
            EXIT(ENTRANCE(DEKU_PALACE, 5),                  ENTRANCE(SOUTHERN_SWAMP_POISONED, 4), CAN_BE_DEKU), // Treetop
        },
        .connections = {
            CONNECTION(RR_SOUTHERN_SWAMP_SOUTH, true),
        },
        .events = {
            EVENT(RE_ACCESS_PICTOGRAPH_SWAMP_GENERIC, HAS_ITEM(ITEM_PICTOGRAPH_BOX)) //Adding here to future proof for later when map shuffle is in.
        }
    };
    Regions[RR_SWAMP_SHOOTING_GALLERY] = RandoRegion{ .sceneId = SCENE_SYATEKI_MORI,
        .checks = {
            CHECK(RC_SWAMP_SHOOTING_GALLERY_HIGH_SCORE, HAS_ITEM(ITEM_BOW) && CAN_PAY(20)),
            CHECK(RC_SWAMP_SHOOTING_GALLERY_PERFECT_SCORE, HAS_ITEM(ITEM_BOW) && CAN_PAY(20)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 2),       ENTRANCE(SWAMP_SHOOTING_GALLERY, 0), true),
        },
        .timeStayRestrictions = {
            STAY(TIME_NIGHT1_PM_10_00, false),
            STAY(TIME_NIGHT2_PM_10_00, false),
            STAY(TIME_NIGHT3_PM_10_00, false),
        },
    };
    Regions[RR_TOURIST_INFORMATION] = RandoRegion{ .sceneId = SCENE_MAP_SHOP,
        .checks = {
            // Also requires poison to not be cleared
            CHECK(RC_TOURIST_INFORMATION_ARCHERY, (!NativeMmErContextsEnabled() || (NativeMmSwampCleared())) && (RANDO_EVENTS[RE_SAVED_KOUME] && NativeMmSwampCleared())),
            CHECK(RC_TOURIST_INFORMATION_GOOD_PHOTO, (!NativeMmErContextsEnabled() || (NativeMmSwampPoisoned())) && (CAN_ACCESS(PICTOGRAPH_TINGLE) || CAN_ACCESS(PICTOGRAPH_DEKU_KING))),
            CHECK(RC_TOURIST_INFORMATION_PICTOBOX, (!NativeMmErContextsEnabled() || (NativeMmSwampPoisoned())) && (RANDO_EVENTS[RE_SAVED_KOUME])),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 1),      ENTRANCE(TOURIST_INFORMATION, 0), true),
        },
        .events = {
            EVENT(RE_SOUTHERN_SWAMP_RIDE_BOAT, Rando::ExpandedBoatAuthorization(RANDO_EVENTS[RE_SAVED_KOUME], CAN_ACCESS(PICTOGRAPH_SWAMP_GENERIC))),
        },
    };
    Regions[RR_WOODFALL_GREAT_FAIRY_FOUNTAIN] = RandoRegion{ .name = "Woodfall", .sceneId = SCENE_YOUSEI_IZUMI,
        .checks = {
            CHECK(RC_WOODFALL_GREAT_FAIRY, HAS_ENOUGH_STRAY_FAIRIES(DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(WOODFALL, 2),                     ENTRANCE(FAIRY_FOUNTAIN, 1), true),
        },
    };
    Regions[RR_WOODFALL] = RandoRegion{ .sceneId = SCENE_21MITURINMAE,
        .checks = {
#ifdef COMBO_BUILD
            CHECK(RC_WOODFALL_ENTRANCE_CHEST, COMBO_WORLD45_ENABLED ? ComboRando::MmAdultWorldRoutes::WoodfallEntrance(HAS_ITEM(ITEM_MASK_DEKU), HAS_ITEM(ITEM_HOOKSHOT), NativeMmSwampCleared(), CAN_USE_NAYRUS_LOVE, MM_LOGIC_HAS_HOVER_BOOTS) : (CAN_BE_DEKU || (NativeMmSwampCleared() && (CAN_BE_ZORA || CAN_BE_GORON || CAN_USE_ABILITY(SWIM))))),
#else
            CHECK(RC_WOODFALL_ENTRANCE_CHEST, CAN_BE_DEKU || (NativeMmSwampCleared() && (CAN_BE_ZORA || CAN_BE_GORON || CAN_USE_ABILITY(SWIM)))),
#endif
#ifdef COMBO_BUILD
            CHECK(RC_WOODFALL_PIECE_OF_HEART_CHEST, COMBO_WORLD45_ENABLED ? ComboRando::MmAdultWorldRoutes::WoodfallHeart(HAS_ITEM(ITEM_MASK_DEKU), (!(RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] || RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS]) || HaveEnemySoul(ACTOR_EN_DEKUNUTS)), NativeMmSwampCleared(), HAS_ITEM(ITEM_HOOKSHOT), MM_LOGIC_IS_ADULT, HAS_ITEM(ITEM_HOOKSHOT), CAN_USE_NAYRUS_LOVE) : (CAN_BE_DEKU && (CanKillEnemy(ACTOR_EN_DEKUNUTS) || HAS_ITEM(ITEM_HOOKSHOT)))),
#else
            CHECK(RC_WOODFALL_PIECE_OF_HEART_CHEST, CAN_BE_DEKU && (CanKillEnemy(ACTOR_EN_DEKUNUTS) || HAS_ITEM(ITEM_HOOKSHOT))),
#endif
#ifdef COMBO_BUILD
            CHECK(RC_WOODFALL_FREESTANDING_RUPEE, COMBO_WORLD45_ENABLED ? ComboRando::MmAdultWorldRoutes::WoodfallRupee(HAS_ITEM(ITEM_MASK_DEKU), (!(RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] || RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS]) || HaveEnemySoul(ACTOR_EN_DEKUNUTS)), NativeMmSwampCleared(), false, HAS_ITEM(ITEM_MASK_ZORA), MM_LOGIC_HAS_HOVER_BOOTS) : (CAN_BE_DEKU && CanKillEnemy(ACTOR_EN_DEKUNUTS))),
#else
            CHECK(RC_WOODFALL_FREESTANDING_RUPEE, CAN_BE_DEKU && CanKillEnemy(ACTOR_EN_DEKUNUTS)),
#endif
#ifdef COMBO_BUILD
            CHECK(RC_WOODFALL_NEAR_OWL_CHEST, COMBO_WORLD45_ENABLED && ((HAS_ITEM(ITEM_MASK_DEKU) && ((!(RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] || RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS]) || HaveEnemySoul(ACTOR_EN_DEKUNUTS)) || NativeMmSwampCleared())) || (NativeMmSwampCleared() && HAS_ITEM(ITEM_HOOKSHOT)) || (MM_LOGIC_IS_ADULT && HAS_ITEM(ITEM_HOOKSHOT) && (NativeMmSwampCleared() || CAN_USE_NAYRUS_LOVE)))), // woodfall.yml:15
#endif
            CHECK(RC_WOODFALL_GRASS_01, true),
            CHECK(RC_WOODFALL_GRASS_02, true),
            CHECK(RC_WOODFALL_GRASS_03, true),
            CHECK(RC_WOODFALL_GRASS_04, true),
            CHECK(RC_WOODFALL_GRASS_05, true),
            CHECK(RC_WOODFALL_GRASS_06, true),
            CHECK(RC_ENEMY_DROP_DRAGONFLY, CanKillEnemy(ACTOR_EN_GRASSHOPPER)),
            CHECK(RC_ENEMY_DROP_HIPLOOP, CanKillEnemy(ACTOR_EN_PP)),
            CHECK(RC_ENEMY_DROP_MAD_SCRUB, CanKillEnemy(ACTOR_EN_DEKUNUTS)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 2),      ENTRANCE(WOODFALL, 0), true),
            EXIT(ENTRANCE(FAIRY_FOUNTAIN, 1),               ENTRANCE(WOODFALL, 2), CAN_BE_DEKU && CanKillEnemy(ACTOR_EN_DEKUNUTS)),
            EXIT(ENTRANCE(WOODFALL_TEMPLE, 2),              ENTRANCE(WOODFALL, 3), NativeMmSwampCleared()),
        },
        .connections = {
            CONNECTION(RR_WOODFALL_OWL_STATUE_PLATFORM, CAN_BE_DEKU && CanKillEnemy(ACTOR_EN_DEKUNUTS)),
        },
    };
    Regions[RR_WOODFALL_OWL_STATUE_PLATFORM] = RandoRegion{ .name = "Owl Statue Platform", .sceneId = SCENE_21MITURINMAE,
        .checks = {
            CHECK(RC_WOODFALL_OWL_STATUE, CAN_ACTIVATE_OWL),
            CHECK(RC_WOODFALL_NEAR_OWL_CHEST, CAN_BE_DEKU || HAS_ITEM(ITEM_HOOKSHOT)),
            CHECK(RC_WOODFALL_POT_01, true),
            CHECK(RC_WOODFALL_POT_02, true),
            CHECK(RC_WOODFALL_POT_03, true),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(WOODFALL_TEMPLE, 0),              ENTRANCE(WOODFALL, 1), CAN_BE_DEKU && CanAccessDungeon(DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE)),
        },
        .connections = {
            CONNECTION(RR_WOODFALL, COMBO_WORLD45_SELECT(CAN_BE_DEKU, (HAS_ITEM(ITEM_MASK_DEKU) && (!(RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] || RANDO_SAVE_OPTIONS[RO_MM_COMPLETE_SOULS]) || HaveEnemySoul(ACTOR_EN_DEKUNUTS))) || NativeMmSwampCleared() || CAN_USE_NAYRUS_LOVE)), // woodfall.yml:29, :35
        },
        .oneWayEntrances = {
            ENTRANCE(WOODFALL, 4), // Song of Soaring lands on the owl platform.
        },
    };
    Regions[RR_WOODS_OF_MYSTERY_GROTTO] = RandoRegion{ .name = "Woods of Mystery Grotto", .sceneId = SCENE_KAKUSIANA,
        .checks = {
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_CHEST, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_01, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_02, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_03, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_04, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_05, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_06, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_07, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_08, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_09, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_10, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_11, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_12, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_13, true),
            CHECK(RC_WOODS_OF_MYSTERY_GROTTO_GRASS_14, true),
            CHECK(RC_ENEMY_DROP_MINI_BABA, CanKillEnemy(ACTOR_EN_KAREBABA)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(WOODS_OF_MYSTERY, 0),             ENTRANCE(GROTTOS, 22), true),
        },
    };
    Regions[RR_WOODS_OF_MYSTERY] = RandoRegion{ .sceneId = SCENE_26SARUNOMORI,
        .checks = {
            CHECK(RC_HAGS_POTION_SHOP_KOTAKE, RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS] == RO_GENERIC_YES && Rando::ExpandedKotakeWoodsReward(SECOND_DAY() || FINAL_DAY(), RANDO_EVENTS[RE_EXPANDED_SWAMP_KOTAKE])),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_01, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_02, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_03, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_04, FIRST_DAY()),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_05, FIRST_DAY()),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_06, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_07, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_08, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_09, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_10, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_11, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_12, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_13, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_14, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_15, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_16, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_17, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_18, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_19, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_20, true),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_21, SECOND_DAY()),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_22, FINAL_DAY()),
            CHECK(RC_WOODS_OF_MYSTERY_GRASS_23, FINAL_DAY()),
            CHECK(RC_ENEMY_DROP_SNAPPER, CAN_BE_DEKU && CanKillEnemy(ACTOR_EN_KAME)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(GROTTOS, 22),                     ENTRANCE(WOODS_OF_MYSTERY, 0), SECOND_DAY()),
            EXIT(ENTRANCE(SOUTHERN_SWAMP_POISONED, 7),      ENTRANCE(WOODS_OF_MYSTERY, 0), true),
        },
        .events = {
            EVENT(RE_EXPANDED_MEET_KOUME_DAY1, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_KOUME_KOTAKE) && FIRST_DAY()),
            EVENT(RE_EXPANDED_MEET_KOUME, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_KOUME_KOTAKE)),
            EVENT(RE_SAVED_KOUME, HAS_BOTTLE && (CAN_ACCESS(RED_POTION_REFILL) || CAN_ACCESS(BLUE_POTION_REFILL))),
        },
    };
}, RANDO_LOGIC_INIT_PATHS);
// clang-format on
