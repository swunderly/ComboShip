#pragma once
#include "OotSouls.h"
namespace Rando::Souls {
// Source vanilla FOREST_POE_1..4 and MQ JOELLE/BETH/AMY_CLEAR all require the Poe soul.
// Events that a setting also grants in RR_ROOT (Kakariko gate, Talon, Malon) check their NPC soul
// at the in-world grant instead, so the setting grant stays unconditional as in pinned OoTMM.
inline bool EventPrerequisite(Logic* logic, LogicVal event) {
    if (!Enabled())
        return true;
    switch (event) {
        case LOGIC_COULD_PLAY_BOWLING:
            return Has(logic, OotSoul::NPC_BOMBCHU_BOWLING_LADY);
        case LOGIC_CARPET_MERCHANT:
            return Has(logic, OotSoul::NPC_CARPET_MAN);
        case LOGIC_SPIRIT_TEMPLE_CLEAR:
            return Has(logic, OotSoul::ENEMY_IRON_KNUCKLE);
        case LOGIC_FOREST_JOELLE:
        case LOGIC_FOREST_BETH:
        case LOGIC_FOREST_AMY:
        case LOGIC_FOREST_MEG:
            return Has(logic, OotSoul::ENEMY_POE);
        case LOGIC_JABU_WEST_TENTACLE:
        case LOGIC_JABU_NORTH_TENTACLE:
        case LOGIC_JABU_EAST_TENTACLE:
            return Has(logic, OotSoul::ENEMY_PARASITE);
        case LOGIC_JABU_MQ_WATER_SWITCH_LIFT_ACCESS:
            return Has(logic, OotSoul::ENEMY_LIZALFOS_DINOLFOS) && Has(logic, OotSoul::ENEMY_LIKE_LIKE) &&
                   Has(logic, OotSoul::ENEMY_STINGER);
        case LOGIC_DEKU_TREE_MQ_CLEARED_SE_ROOM:
            return Has(logic, OotSoul::ENEMY_DEKU_SCRUB) && Has(logic, OotSoul::ENEMY_GOHMA_LARVA);
        case LOGIC_GORON_CITY_STOP_ROLLING_GORON_AS_ADULT:
            return Has(logic, OotSoul::NPC_GORON_CHILD);
        case LOGIC_GTG_GATE_OPEN:
            return Has(logic, OotSoul::NPC_THIEVES);
        case LOGIC_SPIRIT_MQ_3SUNS_ENEMIES:
            return Has(logic, OotSoul::ENEMY_STALFOS) && Has(logic, OotSoul::ENEMY_WALLMASTER);
        case LOGIC_LINKS_COW:
            return Has(logic, OotSoul::NPC_MALON);
        case LOGIC_BIG_POE_KILL:
            return Has(logic, OotSoul::ENEMY_POE);
        default:
            return true;
    }
}
} // namespace Rando::Souls
