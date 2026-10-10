#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

#include "2s2h/Rando/Logic/Logic.h"

using namespace Rando::Logic;
#ifdef COMBO_BUILD
#include "rando/MmMoonTrialRules.h"
static bool MoonSoul(ActorId actor) {
    return !RANDO_SAVE_OPTIONS[RO_SHUFFLE_ENEMY_SOULS] || HaveEnemySoul(actor);
}
static bool CanPassMoonDinolfos() {
    return MoonSoul(ACTOR_EN_DINOFOS) &&
           ComboRando::MmMoonTrialRules::Dinolfos(CAN_USE_SWORD, CAN_BE_GORON, CAN_BE_ZORA, (CAN_BE_DEKU && HAS_MAGIC),
                                                  CAN_USE_BOW);
}
static bool CanPassMoonGaro() {
    return CanPassMoonDinolfos() && MoonSoul(ACTOR_EN_JSO2) &&
           ComboRando::MmMoonTrialRules::Garo(CAN_USE_SWORD, CAN_BE_GORON, (CAN_BE_DEKU && HAS_MAGIC), CAN_USE_BOW);
}
static bool CanPassMoonIronKnuckle() {
    return CanPassMoonGaro() && MoonSoul(ACTOR_EN_IK) &&
           ComboRando::MmMoonTrialRules::IronKnuckle(CAN_USE_SWORD, CAN_BE_GORON, CAN_USE_BOMBS, CAN_USE_BOMBCHUS,
                                                     HAS_ITEM(ITEM_MASK_BLAST), MoonMaskCount());
}
#else
static bool CanPassMoonGaro() {
    return true;
}
static bool CanPassMoonIronKnuckle() {
    return true;
}
#endif

static bool CanReturnFromMoon() {
#ifdef COMBO_BUILD
    // OoTMM v32.3 can_reset_time_on_moon: Moon rewards need a way back.
    // The native save menu is disabled on the Moon and inside its trials.
    // The optional upstream Majora-in-logic technique is not supported here.
    return CAN_PLAY_SONG(TIME);
#else
    return true;
#endif
}

// clang-format off
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_MOON_DEKU_TRIAL] = RandoRegion{ .sceneId = SCENE_LAST_DEKU,
        .checks = {
            CHECK(RC_MOON_TRIAL_DEKU_PIECE_OF_HEART, CAN_BE_DEKU),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(THE_MOON, 0),                     ENTRANCE(MOON_DEKU_TRIAL, 0), true),
            EXIT(ENTRANCE(THE_MOON, 0),                              ONE_WAY_EXIT, CAN_BE_DEKU), // Exit from Mask NPC
        },
    };
    Regions[RR_MOON_GORON_TRIAL] = RandoRegion{ .sceneId = SCENE_LAST_GORON,
        .checks = {
            CHECK(RC_MOON_TRIAL_GORON_EARLY_POT_01, true),
            CHECK(RC_MOON_TRIAL_GORON_EARLY_POT_02, true),
            CHECK(RC_MOON_TRIAL_GORON_EARLY_POT_03, true),
            CHECK(RC_MOON_TRIAL_GORON_EARLY_POT_04, true),
            CHECK(RC_MOON_TRIAL_GORON_POT_01, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_02, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_03, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_04, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_05, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_06, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_07, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_08, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_09, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_10, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_POT_11, CAN_GORON_SPIKE_ROLL),
            CHECK(RC_MOON_TRIAL_GORON_PIECE_OF_HEART, CAN_GORON_SPIKE_ROLL),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(THE_MOON, 0),                     ENTRANCE(MOON_GORON_TRIAL, 0), true),
            EXIT(ENTRANCE(THE_MOON, 0),                              ONE_WAY_EXIT, CAN_GORON_SPIKE_ROLL), // Exit from Mask NPC
        },
    };
    Regions[RR_MOON_LINK_TRIAL] = RandoRegion{ .sceneId = SCENE_LAST_LINK,
        .checks = {
            CHECK(RC_MOON_TRIAL_LINK_GARO_MASTER_CHEST, HAS_ITEM(ITEM_HOOKSHOT) && CanPassMoonGaro()),
            CHECK(RC_MOON_TRIAL_LINK_IRON_KNUCKLE_CHEST, HAS_ITEM(ITEM_HOOKSHOT) && CanPassMoonIronKnuckle()),
            CHECK(RC_MOON_TRIAL_LINK_POT_01, true),
            CHECK(RC_MOON_TRIAL_LINK_POT_02, true),
            CHECK(RC_MOON_TRIAL_LINK_POT_03, true),
            CHECK(RC_MOON_TRIAL_LINK_POT_04, true),
            CHECK(RC_MOON_TRIAL_LINK_POT_05, true),
            CHECK(RC_MOON_TRIAL_LINK_POT_06, true),
            CHECK(RC_MOON_TRIAL_LINK_POT_07, true),
            CHECK(RC_MOON_TRIAL_LINK_POT_08, true),
            CHECK(RC_MOON_TRIAL_LINK_PIECE_OF_HEART, HAS_ITEM(ITEM_HOOKSHOT) && CanPassMoonIronKnuckle() && CAN_USE_BOMBCHUS && CAN_USE_BOW),
            CHECK(RC_ENEMY_DROP_IRON_KNUCKLE, CanKillEnemy(ACTOR_EN_IK)),
            CHECK(RC_ENEMY_DROP_GARO_MASTER, CanKillEnemy(ACTOR_EN_JSO2)),
            CHECK(RC_ENEMY_DROP_WIZROBE, CanKillEnemy(ACTOR_EN_WIZ)),
            CHECK(RC_ENEMY_DROP_DINOLFOS, CanKillEnemy(ACTOR_EN_DINOFOS)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(THE_MOON, 0),                     ENTRANCE(MOON_LINK_TRIAL, 0), true),
            EXIT(ENTRANCE(THE_MOON, 0),                              ONE_WAY_EXIT, HAS_ITEM(ITEM_HOOKSHOT) && CanPassMoonIronKnuckle() && CAN_USE_BOMBCHUS && CAN_USE_BOW), // Exit from Mask NPC
        },
    };
    Regions[RR_MOON_MAJORAS_LAIR] = RandoRegion{ .sceneId = SCENE_LAST_BS,
        .checks = {
            // Pinned Moon Boss: can_play_time || event(MAJORA). Pool admission
            // separately excludes both Triforce goals before fill.
#ifdef COMBO_BUILD
            CHECK(RC_MOON_MAJORA_POT_01, CanReturnFromMoon() || CanDefeatMajora()),
            CHECK(RC_MOON_MAJORA_POT_02, CanReturnFromMoon() || CanDefeatMajora()),
#else
            CHECK(RC_MOON_MAJORA_POT_01, true),
            CHECK(RC_MOON_MAJORA_POT_02, true),
#endif
        },
        .oneWayEntrances = {
            ENTRANCE(MAJORAS_LAIR, 0), // From moon
        },
    };
    Regions[RR_MOON_ZORA_TRIAL] = RandoRegion{ .sceneId = SCENE_LAST_ZORA,
        .checks = {
            CHECK(RC_MOON_TRIAL_ZORA_PIECE_OF_HEART, CAN_BE_ZORA && CAN_USE_ABILITY(SWIM)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(THE_MOON, 0),                     ENTRANCE(MOON_ZORA_TRIAL, 0), true),
            EXIT(ENTRANCE(THE_MOON, 0),                              ONE_WAY_EXIT, true), // Exit from Mask NPC
        },
    };
    Regions[RR_MOON] = RandoRegion{ .sceneId = SCENE_SOUGEN,
        .checks = {
            // With 20 regular masks each pinned trial entrance is available (1/2/3/4).
            // Compose MOON_TRIAL_* completion events from their exact supported routes.
            CHECK(RC_MOON_FIERCE_DEITY_MASK, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_MOON_CHILDREN) &&
                CanReturnFromMoon() && MoonMaskCount() >= 20 && CAN_BE_DEKU && CAN_GORON_SPIKE_ROLL &&
                CAN_BE_ZORA && HAS_ITEM(ITEM_HOOKSHOT) && CanPassMoonIronKnuckle() && CAN_USE_BOMBCHUS && CAN_USE_BOW && CAN_USE_MAGIC_ARROW(FIRE)),
        },
        .exits = { //     TO                                         FROM
            EXIT(ENTRANCE(MOON_DEKU_TRIAL, 0),              ENTRANCE(THE_MOON, 0), 
                Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_MOON_CHILDREN) && CanReturnFromMoon() && ((RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_20_MASKS && MoonMaskCount() >= 1) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_REMAINS && CHECK_QUEST_ITEM(QUEST_REMAINS_ODOLWA)) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_FORMS && CAN_BE_DEKU) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_OPEN))
            ),
            EXIT(ENTRANCE(MOON_GORON_TRIAL, 0),             ENTRANCE(THE_MOON, 0), 
                Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_MOON_CHILDREN) && CanReturnFromMoon() && ((RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_20_MASKS && MoonMaskCount() >= 2) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_REMAINS && CHECK_QUEST_ITEM(QUEST_REMAINS_GOHT)) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_FORMS && CAN_BE_GORON) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_OPEN))
            ),
            EXIT(ENTRANCE(MOON_ZORA_TRIAL, 0),              ENTRANCE(THE_MOON, 0),
                Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_MOON_CHILDREN) && CanReturnFromMoon() && ((RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_20_MASKS && MoonMaskCount() >= 3) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_REMAINS && CHECK_QUEST_ITEM(QUEST_REMAINS_GYORG)) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_FORMS && CAN_BE_ZORA) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_OPEN))
            ),
            EXIT(ENTRANCE(MOON_LINK_TRIAL, 0),              ENTRANCE(THE_MOON, 0),
                Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_MOON_CHILDREN) && CanReturnFromMoon() && ((RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_20_MASKS && MoonMaskCount() >= 4) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_REMAINS && CHECK_QUEST_ITEM(QUEST_REMAINS_TWINMOLD)) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_FORMS) ||
                (RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] == RO_ACCESS_TRIALS_OPEN))
            ),
            EXIT(ENTRANCE(MAJORAS_LAIR, 0),                          ONE_WAY_EXIT, Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_MOON_CHILDREN) && (RemainsCount() >= RANDO_SAVE_OPTIONS[RO_ACCESS_MAJORA_REMAINS_COUNT]) && (MoonMaskCount() >= RANDO_SAVE_OPTIONS[RO_ACCESS_MAJORA_MASKS_COUNT])
            ),
        },
        .oneWayEntrances = {
            ENTRANCE(THE_MOON, 0), // From rooftop and trials
        },
    };
}, RANDO_LOGIC_INIT_PATHS);
// clang-format on
