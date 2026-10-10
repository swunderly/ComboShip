#pragma once
// Include after randomizerTypes.h; the enum source intentionally supports
// multiple macro expansions and must not be redeclared here.

namespace Rando {
// OoTMM permits local age changes in ordinary dungeon areas. SwitchAge preserves
// room/position here; age-specific movement, equipment and encounter requirements
// remain on their edges/checks. Boss arenas have separate transitions and do not
// inherit this permission. Enum ranges below are the contiguous dungeon groups.
inline constexpr bool SongAgeChangeAreaAllowed(RandomizerRegion region) {
    if ((region >= RR_DEKU_TREE_LOBBY && region <= RR_DEKU_TREE_MQ_OUTSIDE_BOSS_ROOM) ||
        (region >= RR_JABU_JABUS_BELLY_BEGINNING && region <= RR_JABU_JABUS_BELLY_MQ_NEAR_BOSS_ROOM) ||
        (region >= RR_FIRE_TEMPLE_FOYER && region <= RR_FIRE_TEMPLE_MQ_ABOVE_FIRE_MAZE) ||
        (region >= RR_SHADOW_TEMPLE_BEGINNING && region <= RR_SHADOW_TEMPLE_MQ_SPIKE_WALLS_ROOM) ||
        (region >= RR_BOTW_CORRIDOR && region <= RR_BOTW_MQ_B3_PLATFORM) ||
        (region >= RR_ICE_CAVERN_BEGINNING && region <= RR_ICE_CAVERN_MQ_ABOVE_BEGINNING &&
         region != RR_ICE_CAVERN_MQ_STALFOS_ROOM_UNDERWATER)) {
        return true;
    }
    switch (region) {
        case RR_KOKIRI_FOREST: case RR_KF_LINKS_PORCH: case RR_KF_BOULDER_LOOP:
        case RR_KF_OUTSIDE_DEKU_TREE: case RR_KF_OUTSIDE_LOST_WOODS:
        case RR_THE_LOST_WOODS: case RR_LW_BEYOND_MIDO: case RR_LW_BRIDGE:
        case RR_SFM_ENTRYWAY: case RR_SFM_MAZE: case RR_SFM_ABOVE_MAZE:
        case RR_SACRED_FOREST_MEADOW: case RR_HYRULE_FIELD: case RR_LON_LON_RANCH:
        case RR_LAKE_HYLIA: case RR_LH_FISHING_ISLAND:
        case RR_GERUDO_VALLEY: case RR_GV_FORTRESS_SIDE:
        case RR_GF_OUTSKIRTS: case RR_GF_OUTSIDE_GATE: case RR_GF_HBA_RANGE:
        case RR_DESERT_COLOSSUS: case RR_DESERT_COLOSSUS_OASIS:
        case RR_DESERT_COLOSSUS_OUTSIDE_TEMPLE:
        case RR_WASTELAND_NEAR_FORTRESS: case RR_WASTELAND_NEAR_COLOSSUS:
        case RR_HAUNTED_WASTELAND:
        case RR_KAKARIKO_VILLAGE: case RR_KAK_BACKYARD: case RR_KAK_BEHIND_GATE:
        case RR_KAK_WELL: case RR_KAK_ROOFTOP: case RR_KAK_IMPAS_ROOFTOP:
        case RR_THE_GRAVEYARD: case RR_GRAVEYARD_WARP_PAD_REGION:
        case RR_DEATH_MOUNTAIN_TRAIL: case RR_GORON_CITY:
        case RR_GC_DARUNIAS_CHAMBER:
        case RR_DMC_UPPER_ENTRY: case RR_DMC_ROCKS_GROTTO_ENTRY:
        case RR_DMC_CRACKED_WALL: case RR_DMC_POTS_ENTRY: case RR_DMC_BLOCKED_ENTRY:
        case RR_DMC_POT_GROTTO_ENTRY: case RR_DMC_PAD_ENTRY: case RR_DMC_TEMPLE_ENTRY:
        case RR_DMC_CRATE: case RR_DMC_ROCK_GROTTO: case RR_DMC_SCRUB:
        case RR_DMC_POTS: case RR_DMC_CENTRAL: case RR_DMC_FAR_PLATFORM:
        case RR_DMC_BLOCKED_EXIT: case RR_DMC_POT_GROTTO_EXIT: case RR_DMC_TEMPLE_EXIT:
        case RR_ZR_FRONT: case RR_ZORAS_RIVER: case RR_ZR_ATOP_LADDER:
        case RR_ZORAS_DOMAIN: case RR_ZORAS_DOMAIN_ISLAND: case RR_ZORAS_FOUNTAIN:
            return true;
        // OoTMM's matching ordinary interior areas also permit age changes.
        // Entry keys, checks, item usability and exits remain their native rules.
        case RR_KF_LINKS_HOUSE: case RR_KF_MIDOS_HOUSE: case RR_KF_SARIAS_HOUSE:
        case RR_KF_HOUSE_OF_TWINS: case RR_KF_KNOW_IT_ALL_HOUSE: case RR_KF_KOKIRI_SHOP:
        case RR_KAK_CARPENTER_BOSS_HOUSE: case RR_KAK_HOUSE_OF_SKULLTULA:
        case RR_KAK_IMPAS_HOUSE: case RR_KAK_IMPAS_HOUSE_BACK:
        case RR_KAK_BAZAAR: case RR_KAK_POTION_SHOP: case RR_MARKET_BAZAAR:
        case RR_LH_LAB: case RR_GV_CARPENTER_TENT: case RR_GC_SHOP: case RR_ZD_SHOP:
        case RR_LLR_TALONS_HOUSE: case RR_LLR_STABLES: case RR_LLR_TOWER:
            return true;
        // Shuffled child-only entrances can lead to Water Temple. Its dry
        // entrance/ledge has common collision for both ages and no competing
        // Song-of-Time interaction, so adult equipment can be used after entry.
        // Keep underwater and other dungeon subregions outside this allowlist.
        case RR_WATER_TEMPLE_ENTRYWAY:
        case RR_WATER_TEMPLE_ENTRANCE_LEDGE:
        case RR_WATER_TEMPLE_MQ_ENTRANCE_LEDGE:
        case RR_WATER_TEMPLE_RIVER: case RR_WATER_TEMPLE_RIVER_POTS:
        case RR_WATER_TEMPLE_ABOVE_DRAGON:
            return true;
        // Dry target rooms use the same scene collision for both ages. Age
        // access propagates at the reached room, so entering with Hookshot and
        // shooting an eye as Child Link are separate steps. Native eye/switch
        // rules retain Slingshot support; Poe paintings and fire shots keep
        // their explicit Bow requirements. Sewers and bosses stay excluded.
        case RR_FOREST_TEMPLE_LOBBY:
        case RR_FOREST_TEMPLE_NE_COURTYARD_LOWER:
        case RR_FOREST_TEMPLE_NE_COURTYARD_UPPER:
        case RR_FOREST_TEMPLE_NE_COURTYARD_ISLAND:
        case RR_FOREST_TEMPLE_NW_COURTYARD_LOWER:
        case RR_FOREST_TEMPLE_NW_COURTYARD_UPPER:
        case RR_FOREST_TEMPLE_MIDDLE_BLOCK_PUSH_ROOM:
        case RR_FOREST_TEMPLE_UPPER_BLOCK_PUSH_ROOM:
        case RR_FOREST_TEMPLE_MQ_LOBBY:
        case RR_FOREST_TEMPLE_MQ_MIDDLE_BLOCK_PUZZLE:
        case RR_FOREST_TEMPLE_MQ_UPPER_BLOCK_PUZZLE:
        case RR_GERUDO_TRAINING_GROUND_LOBBY:
        case RR_GERUDO_TRAINING_GROUND_MQ_LOBBY:
            return true;
        // Additional dry vanilla rooms preserve room/position on SwitchAge.
        // Keys, climbing, combat and actual target compatibility remain
        // on the native edges/checks. A reached child crawlspace does not
        // imply that adult collision can pass back through that crawlspace.
        case RR_SPIRIT_TEMPLE_FOYER:
        case RR_SPIRIT_TEMPLE_CHILD_SIDE_HUB:
        case RR_SPIRIT_TEMPLE_SWITCH_BRIDGE_SOUTH:
        case RR_SPIRIT_TEMPLE_SWITCH_BRIDGE_NORTH:
        case RR_SPIRIT_TEMPLE_1F_ANUBIS:
        case RR_SPIRIT_TEMPLE_RUPEE_BRIDGE_NORTH:
        case RR_SPIRIT_TEMPLE_RUPEE_BRIDGE_SOUTH:
        case RR_SPIRIT_TEMPLE_CHILD_BOXES:
        case RR_SPIRIT_TEMPLE_SUN_ON_FLOOR_1F:
        case RR_SPIRIT_TEMPLE_SUN_ON_FLOOR_2F:
        case RR_SPIRIT_TEMPLE_ADULT_SIDE_HUB:
        case RR_SPIRIT_TEMPLE_SAND_PIT:
        case RR_SPIRIT_TEMPLE_ABOVE_BOULDERS:
        case RR_SPIRIT_TEMPLE_BOULDERS:
        case RR_SPIRIT_TEMPLE_PAST_BOULDERS:
        case RR_SPIRIT_TEMPLE_1F_MIRROR_ROOM:
        case RR_SPIRIT_TEMPLE_2F_MIRROR_ROOM:
        case RR_SPIRIT_TEMPLE_STATUE_ROOM_CHILD:
        case RR_SPIRIT_TEMPLE_INNER_WEST_HAND:
        case RR_SPIRIT_TEMPLE_GS_LEDGE:
        case RR_SPIRIT_TEMPLE_STATUE_ROOM:
        case RR_SPIRIT_TEMPLE_EMPTY_STAIRS:
        case RR_SPIRIT_TEMPLE_SUN_BLOCK_ROOM:
        case RR_SPIRIT_TEMPLE_SUN_BLOCK_CHEST_LEDGE:
        case RR_SPIRIT_TEMPLE_SKULLTULA_STAIRS:
        case RR_SPIRIT_TEMPLE_CHILD_THRONE:
        case RR_SPIRIT_TEMPLE_RIGHT_HAND_EXIT:
        case RR_SPIRIT_TEMPLE_OUTER_RIGHT_HAND:
        case RR_SPIRIT_TEMPLE_STATUE_ROOM_ADULT:
        case RR_SPIRIT_TEMPLE_INNER_LEFT_HAND:
        case RR_SPIRIT_TEMPLE_SHORTCUT_SWITCH:
        case RR_SPIRIT_TEMPLE_SHORTCUT:
        case RR_SPIRIT_TEMPLE_POT_STAIRS:
        case RR_SPIRIT_TEMPLE_BEAMOS_PITS:
        case RR_SPIRIT_TEMPLE_4_ARMOS:
        case RR_SPIRIT_TEMPLE_4_ARMOS_SIDE_ROOM:
        case RR_SPIRIT_TEMPLE_CHEST_STAIRS:
        case RR_SPIRIT_TEMPLE_ADULT_THRONE:
        case RR_SPIRIT_TEMPLE_LEFT_HAND_EXIT:
        case RR_SPIRIT_TEMPLE_OUTER_LEFT_HAND:
        case RR_SPIRIT_TEMPLE_BIG_WALL_BASE:
        case RR_SPIRIT_TEMPLE_BIG_WALL_UPPER:
        case RR_SPIRIT_TEMPLE_4F_CENTRAL:
        case RR_SPIRIT_TEMPLE_FAKE_DOORS_ROOM:
        case RR_SPIRIT_TEMPLE_BIG_MIRROR_ROOM:
        case RR_SPIRIT_TEMPLE_BIG_MIRROR_CAVE:
        case RR_SPIRIT_TEMPLE_PLATFORM:
        case RR_SPIRIT_TEMPLE_STATUE_HEAD:
        case RR_FOREST_TEMPLE_TREES:
        case RR_FOREST_TEMPLE_OVERGROWN_HALLWAY_LOWER:
        case RR_FOREST_TEMPLE_OVERGROWN_HALLWAY_UPPER:
        case RR_FOREST_TEMPLE_NORTH_HALLWAY:
        case RR_FOREST_TEMPLE_LOWER_STALFOS:
        case RR_FOREST_TEMPLE_NW_COURTYARD_UPPER_ALCOVE:
        case RR_FOREST_TEMPLE_NE_COURTYARD_DOORFRAME:
        case RR_FOREST_TEMPLE_NE_COURTYARD_SCARECROW_LEDGE:
        case RR_FOREST_TEMPLE_MAP_ROOM:
        case RR_FOREST_TEMPLE_DRAINED_SEWER:
        case RR_FOREST_TEMPLE_FLOORMASTER_ROOM:
        case RR_FOREST_TEMPLE_RED_DOORMAT_HALLWAY:
        case RR_FOREST_TEMPLE_RED_DOORMAT_HALLWAY_DOORMAT:
        case RR_FOREST_TEMPLE_BLOCK_PUSH_FLOOR:
        case RR_FOREST_TEMPLE_LOWER_BLOCK_PUSH_ROOM:
        case RR_FOREST_TEMPLE_BLOCK_PUSH_ROOM_TOP:
        case RR_FOREST_TEMPLE_BLOCK_PUSH_ROOM_COURTYARD_ALCOVE:
        case RR_FOREST_TEMPLE_NW_HALLWAY_TWISTED:
        case RR_FOREST_TEMPLE_NW_HALLWAY_STRAIGHTENED:
        case RR_FOREST_TEMPLE_RED_POE_ROOM:
        case RR_FOREST_TEMPLE_UPPER_STALFOS:
        case RR_FOREST_TEMPLE_BLUE_POE_ROOM:
        case RR_FOREST_TEMPLE_NE_HALLWAY_STRAIGHTENED:
        case RR_FOREST_TEMPLE_NE_HALLWAY_TWISTED:
        case RR_FOREST_TEMPLE_FALLING_ROOM:
        case RR_FOREST_TEMPLE_GREEN_POE_ROOM:
        case RR_FOREST_TEMPLE_BLUE_DOORMAT_HALLWAY_DOORMAT:
        case RR_FOREST_TEMPLE_BLUE_DOORMAT_HALLWAY:
        case RR_FOREST_TEMPLE_BASEMENT:
        case RR_GERUDO_TRAINING_GROUND_SAND_ROOM:
        case RR_GERUDO_TRAINING_GROUND_BOULDER_ROOM:
        case RR_GERUDO_TRAINING_GROUND_CENTRAL_MAZE:
        case RR_GERUDO_TRAINING_GROUND_CENTRAL_MAZE_RIGHT:
        case RR_GERUDO_TRAINING_GROUND_HEAVY_BLOCK_ROOM:
        case RR_GERUDO_TRAINING_GROUND_HEAVY_BLOCK_ROOM_UPPER:
        case RR_GERUDO_TRAINING_GROUND_BEHIND_HEAVY_BLOCK:
        case RR_GERUDO_TRAINING_GROUND_LIKE_LIKE_ROOM:
        case RR_GERUDO_TRAINING_GROUND_EYE_STATUE_UPPER:
        case RR_GERUDO_TRAINING_GROUND_ABOVE_MAZE:
        case RR_GERUDO_TRAINING_GROUND_EYE_STATUE_LOWER:
        case RR_GERUDO_TRAINING_GROUND_HAMMER_ROOM:
        case RR_GERUDO_TRAINING_GROUND_DINALFOS:
            return true;
        default:
            return false;
    }
}
}
