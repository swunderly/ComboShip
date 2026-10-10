#pragma once
#include "OotSouls.h"
#include "OotSoulRouteLogic.h"
namespace Rando::Souls {
inline bool CheckPrerequisite(Logic* logic, RandomizerCheck check) {
    if (!Enabled())
        return true;
    switch (check) {
        case RC_HC_MALON_EGG:
            return Has(logic, OotSoul::NPC_MALON);
        case RC_MARKET_BOMBCHU_BOWLING_FIRST_PRIZE:
            return Has(logic, OotSoul::NPC_BOMBCHU_BOWLING_LADY);
        case RC_MARKET_BOMBCHU_BOWLING_SECOND_PRIZE:
            return Has(logic, OotSoul::NPC_BOMBCHU_BOWLING_LADY);
        case RC_TH_1_TORCH_CARPENTER:
            return Has(logic, OotSoul::NPC_CARPENTERS);
        case RC_TH_DOUBLE_CELL_CARPENTER:
            return Has(logic, OotSoul::NPC_CARPENTERS);
        case RC_TH_DEAD_END_CARPENTER:
            return Has(logic, OotSoul::NPC_CARPENTERS);
        case RC_TH_STEEP_SLOPE_CARPENTER:
            return Has(logic, OotSoul::NPC_CARPENTERS);

        case RC_BOTTOM_OF_THE_WELL_LENS_OF_TRUTH_CHEST:
            return Has(logic, OotSoul::ENEMY_DEAD_HAND);
        case RC_BOTTOM_OF_THE_WELL_GS_LIKE_LIKE_CAGE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_BOTTOM_OF_THE_WELL_GS_WEST_INNER_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_BOTTOM_OF_THE_WELL_GS_EAST_INNER_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_GS_COMPASS_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_GS_BASEMENT_GATE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_GS_BASEMENT_VINES:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_GS_BASEMENT_BACK_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_DEKU_SCRUB_LOBBY:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_GS_SCARECROW:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_GS_SIDE_ROOM_NEAR_LOWER_LIZALFOS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_DEKU_SCRUB_SIDE_ROOM_NEAR_DODONGOS:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_GS_VINES_ABOVE_STAIRS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_GS_ALCOVE_ABOVE_STAIRS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_DEKU_SCRUB_NEAR_BOMB_BAG_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_DEKU_SCRUB_NEAR_BOMB_BAG_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_GS_BACK_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_GS_BOSS_KEY_LOOP:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_FLARE_DANCER_CHEST:
            return Has(logic, OotSoul::ENEMY_FLARE_DANCER);
        case RC_FIRE_TEMPLE_GS_SONG_OF_TIME_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_GS_BOULDER_MAZE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_GS_SCARECROW_CLIMB:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_GS_SCARECROW_TOP:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_GS_FIRST_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_GS_LOBBY:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_FIRST_STALFOS_CHEST:
            return Has(logic, OotSoul::ENEMY_STALFOS);
        case RC_FOREST_TEMPLE_GS_LEVEL_ISLAND_COURTYARD:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_FLOORMASTER_CHEST:
            return Has(logic, OotSoul::ENEMY_FLOORMASTER);
        case RC_FOREST_TEMPLE_MAP_CHEST:
            return Has(logic, OotSoul::ENEMY_BUBBLE);
        case RC_FOREST_TEMPLE_GS_RAISED_ISLAND_COURTYARD:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_RED_POE_CHEST:
            return Has(logic, OotSoul::ENEMY_POE);
        case RC_FOREST_TEMPLE_BOW_CHEST:
            return Has(logic, OotSoul::ENEMY_STALFOS);
        case RC_FOREST_TEMPLE_BLUE_POE_CHEST:
            return Has(logic, OotSoul::ENEMY_POE);
        case RC_FOREST_TEMPLE_GS_BASEMENT:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GANONS_CASTLE_DEKU_SCRUB_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_DEKU_SCRUB_CENTER_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_DEKU_SCRUB_CENTER_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_DEKU_SCRUB_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_LIGHT_TRIAL_INVISIBLE_ENEMIES_CHEST:
            return Has(logic, OotSoul::ENEMY_KEESE) && Has(logic, OotSoul::ENEMY_SKULLTULA);
        case RC_GANONS_CASTLE_FOREST_TRIAL_CHEST:
            return Has(logic, OotSoul::ENEMY_WOLFOS);
        case RC_GANONS_TOWER_BOSS_KEY_CHEST:
            return Has(logic, OotSoul::ENEMY_STALFOS);
        case RC_GERUDO_TRAINING_GROUND_STALFOS_CHEST:
            return Has(logic, OotSoul::ENEMY_STALFOS);
        case RC_GERUDO_TRAINING_GROUND_BEFORE_HEAVY_BLOCK_CHEST:
            return Has(logic, OotSoul::ENEMY_WOLFOS);
        case RC_GERUDO_TRAINING_GROUND_HEAVY_BLOCK_SECOND_CHEST:
            return Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RC_GERUDO_TRAINING_GROUND_HEAVY_BLOCK_FIRST_CHEST:
            return Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RC_GERUDO_TRAINING_GROUND_HAMMER_ROOM_CLEAR_CHEST:
            return Has(logic, OotSoul::ENEMY_KEESE) && Has(logic, OotSoul::ENEMY_TORCH_SLUG);
        case RC_ICE_CAVERN_GS_SPINNING_SCYTHE_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ICE_CAVERN_GS_HEART_PIECE_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ICE_CAVERN_GS_PUSH_BLOCK_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ICE_CAVERN_IRON_BOOTS_CHEST:
            return Has(logic, OotSoul::ENEMY_WOLFOS);
        case RC_JABU_JABUS_BELLY_DEKU_SCRUB:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_JABU_JABUS_BELLY_BOOMERANG_CHEST:
            return Has(logic, OotSoul::ENEMY_STINGER) && Has(logic, OotSoul::NPC_RUTO);
        case RC_JABU_JABUS_BELLY_MAP_CHEST:
            return Has(logic, OotSoul::ENEMY_PARASITE);
        case RC_JABU_JABUS_BELLY_COMPASS_CHEST:
            return Has(logic, OotSoul::ENEMY_SHABOM);
        case RC_JABU_JABUS_BELLY_GS_LOBBY_BASEMENT_LOWER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_JABU_JABUS_BELLY_GS_LOBBY_BASEMENT_UPPER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_JABU_JABUS_BELLY_GS_WATER_SWITCH_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_JABU_JABUS_BELLY_GS_NEAR_BOSS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_MAP_CHEST:
            return Has(logic, OotSoul::ENEMY_KEESE) && Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_HOVER_BOOTS_CHEST:
            return Has(logic, OotSoul::ENEMY_DEAD_HAND);
        case RC_SHADOW_TEMPLE_COMPASS_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_INVISIBLE_BLADES_VISIBLE_CHEST:
            return Has(logic, OotSoul::ENEMY_KEESE) && Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RC_SHADOW_TEMPLE_INVISIBLE_BLADES_INVISIBLE_CHEST:
            return Has(logic, OotSoul::ENEMY_KEESE) && Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RC_SHADOW_TEMPLE_GS_LIKE_LIKE_ROOM:
            return Has(logic, OotSoul::ENEMY_KEESE) && Has(logic, OotSoul::ENEMY_LIKE_LIKE) &&
                   Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_GS_FALLING_SPIKES_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_INVISIBLE_SPIKES_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_GS_SINGLE_GIANT_POT:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_AFTER_WIND_ENEMY_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_GS_NEAR_SHIP:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_INVISIBLE_FLOORMASTER_CHEST:
            return Has(logic, OotSoul::ENEMY_FLOORMASTER);
        case RC_SHADOW_TEMPLE_GS_TRIPLE_GIANT_POT:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_GS_METAL_FENCE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_GS_SUN_ON_FLOOR_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_GS_HALL_AFTER_SUN_BLOCK_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_GS_BOULDER_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_GS_LOBBY:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_MAP_CHEST:
            return Has(logic, OotSoul::ENEMY_SPIKE);
        case RC_WATER_TEMPLE_TORCHES_CHEST:
            return Has(logic, OotSoul::ENEMY_SHELL_BLADE);
        case RC_WATER_TEMPLE_GS_CENTRAL_PILLAR:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_CENTRAL_PILLAR_CHEST:
            return Has(logic, OotSoul::ENEMY_SHELL_BLADE) && Has(logic, OotSoul::ENEMY_SPIKE);
        case RC_WATER_TEMPLE_GS_NEAR_BOSS_KEY_CHEST:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_GS_FALLING_PLATFORM_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_GS_RIVER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_GS_BEHIND_GATE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_BOTTOM_OF_THE_WELL_MQ_GS_BASEMENT:
            return Has(logic, OotSoul::MISC_GS);
        case RC_BOTTOM_OF_THE_WELL_MQ_GS_WEST_INNER_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_BOTTOM_OF_THE_WELL_MQ_GS_COFFIN_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_BOTTOM_OF_THE_WELL_MQ_COMPASS_CHEST:
            return Has(logic, OotSoul::ENEMY_DEAD_HAND);
        case RC_DEKU_TREE_MQ_GS_LOBBY:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_MQ_GS_PAST_BOULDER_VINES:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_MQ_SLINGSHOT_CHEST:
            return Has(logic, OotSoul::ENEMY_DEKU_BABA) && Has(logic, OotSoul::ENEMY_GOHMA_LARVA);
        case RC_DEKU_TREE_MQ_GS_BASEMENT_GRAVES_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_MQ_GS_BASEMENT_BACK_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DEKU_TREE_MQ_DEKU_SCRUB:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_MQ_DEKU_SCRUB_LOBBY_FRONT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_MQ_DEKU_SCRUB_LOBBY_REAR:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_MQ_GS_SONG_OF_TIME_BLOCK_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_MQ_DEKU_SCRUB_STAIRCASE:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_MQ_COMPASS_CHEST:
            return Has(logic, OotSoul::ENEMY_DODONGO);
        case RC_DODONGOS_CAVERN_MQ_LARVAE_ROOM_CHEST:
            return Has(logic, OotSoul::ENEMY_GOHMA_LARVA);
        case RC_DODONGOS_CAVERN_MQ_GS_LARVAE_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_MQ_GS_LIZALFOS_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_MQ_DEKU_SCRUB_SIDE_ROOM_NEAR_LOWER_LIZALFOS:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DODONGOS_CAVERN_MQ_GS_SCRUB_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DODONGOS_CAVERN_MQ_GS_BACK_AREA:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_MQ_MAP_ROOM_SIDE_CHEST:
            return Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RC_FIRE_TEMPLE_MQ_MEGATON_HAMMER_CHEST:
            return Has(logic, OotSoul::ENEMY_FLARE_DANCER);
        case RC_FIRE_TEMPLE_MQ_GS_BIG_LAVA_ROOM_OPEN_DOOR:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_MQ_GS_SKULL_ON_FIRE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_MQ_GS_FIRE_WALL_MAZE_SIDE_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_MQ_GS_FIRE_WALL_MAZE_CENTER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FIRE_TEMPLE_MQ_FREESTANDING_KEY:
            return Has(logic, OotSoul::ENEMY_FLARE_DANCER);
        case RC_FIRE_TEMPLE_MQ_GS_ABOVE_FIRE_MAZE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_MQ_GS_FIRST_HALLWAY:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_MQ_WOLFOS_CHEST:
            return Has(logic, OotSoul::ENEMY_WOLFOS);
        case RC_FOREST_TEMPLE_MQ_GS_BLOCK_PUSH_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_MQ_REDEAD_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_FOREST_TEMPLE_MQ_GS_LEVEL_ISLAND_COURTYARD:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_MQ_GS_WELL:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_MQ_GS_RAISED_ISLAND_COURTYARD:
            return Has(logic, OotSoul::MISC_GS);
        case RC_FOREST_TEMPLE_MQ_MAP_CHEST:
            return Has(logic, OotSoul::ENEMY_POE);
        case RC_FOREST_TEMPLE_MQ_BOW_CHEST:
            return Has(logic, OotSoul::ENEMY_STALFOS) && Has(logic, OotSoul::ENEMY_WOLFOS);
        case RC_FOREST_TEMPLE_MQ_COMPASS_CHEST:
            return Has(logic, OotSoul::ENEMY_POE);
        case RC_GANONS_CASTLE_MQ_DEKU_SCRUB_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_MQ_DEKU_SCRUB_CENTER_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_MQ_DEKU_SCRUB_CENTER:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_MQ_DEKU_SCRUB_CENTER_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GANONS_CASTLE_MQ_DEKU_SCRUB_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GERUDO_TRAINING_GROUND_MQ_DINOLFOS_CHEST:
            return Has(logic, OotSoul::ENEMY_ARMOS) && Has(logic, OotSoul::ENEMY_DODONGO) &&
                   Has(logic, OotSoul::ENEMY_LIZALFOS_DINOLFOS);
        case RC_GERUDO_TRAINING_GROUND_MQ_FIRST_IRON_KNUCKLE_CHEST:
            return Has(logic, OotSoul::ENEMY_IRON_KNUCKLE);
        case RC_GERUDO_TRAINING_GROUND_MQ_BEFORE_HEAVY_BLOCK_CHEST:
            return Has(logic, OotSoul::ENEMY_SKULLTULA) && Has(logic, OotSoul::ENEMY_STALFOS);
        case RC_GERUDO_TRAINING_GROUND_MQ_HEAVY_BLOCK_CHEST:
            return Has(logic, OotSoul::ENEMY_FREEZARD) && Has(logic, OotSoul::ENEMY_SPIKE);
        case RC_GERUDO_TRAINING_GROUND_MQ_SECOND_IRON_KNUCKLE_CHEST:
            return Has(logic, OotSoul::ENEMY_IRON_KNUCKLE) && Has(logic, OotSoul::ENEMY_TORCH_SLUG);
        case RC_ICE_CAVERN_MQ_GS_RED_ICE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ICE_CAVERN_MQ_GS_ICE_BLOCK:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ICE_CAVERN_MQ_GS_SCARECROW:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ICE_CAVERN_MQ_IRON_BOOTS_CHEST:
            return Has(logic, OotSoul::ENEMY_STALFOS);
        case RC_JABU_JABUS_BELLY_MQ_BOOMERANG_CHEST:
            return Has(logic, OotSoul::ENEMY_LIKE_LIKE) && Has(logic, OotSoul::ENEMY_LIZALFOS_DINOLFOS) &&
                   Has(logic, OotSoul::ENEMY_STINGER);
        case RC_JABU_JABUS_BELLY_MQ_GS_BOOMERANG_CHEST_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_JABU_JABUS_BELLY_MQ_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_JABU_JABUS_BELLY_MQ_FALLING_LIKE_LIKE_ROOM_CHEST:
            return Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RC_JABU_JABUS_BELLY_MQ_GS_TAILPASARAN_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_JABU_JABUS_BELLY_MQ_GS_INVISIBLE_ENEMIES_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_JABU_JABUS_BELLY_MQ_GS_NEAR_BOSS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_MQ_COMPASS_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_MQ_HOVER_BOOTS_CHEST:
            return Has(logic, OotSoul::ENEMY_DEAD_HAND);
        case RC_SHADOW_TEMPLE_MQ_EARLY_GIBDOS_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_MQ_GS_FALLING_SPIKES_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_MQ_INVISIBLE_SPIKES_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_MQ_STALFOS_ROOM_CHEST:
            return Has(logic, OotSoul::ENEMY_STALFOS);
        case RC_SHADOW_TEMPLE_MQ_WIND_HINT_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_MQ_GS_WIND_HINT_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_MQ_AFTER_WIND_ENEMY_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_SHADOW_TEMPLE_MQ_GS_AFTER_WIND:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_MQ_GS_AFTER_SHIP:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHADOW_TEMPLE_MQ_BOMB_FLOWER_CHEST:
            return Has(logic, OotSoul::ENEMY_DEAD_HAND);
        case RC_SHADOW_TEMPLE_MQ_GS_NEAR_BOSS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_MQ_MAP_ROOM_ENEMY_CHEST:
            return Has(logic, OotSoul::ENEMY_ANUBIS) && Has(logic, OotSoul::ENEMY_KEESE) &&
                   Has(logic, OotSoul::ENEMY_REDEAD_GIBDO) && Has(logic, OotSoul::ENEMY_STALFOS) &&
                   Has(logic, OotSoul::ENEMY_TORCH_SLUG);
        case RC_SPIRIT_TEMPLE_MQ_CHILD_CLIMB_NORTH_CHEST:
            return Has(logic, OotSoul::ENEMY_BABY_DODONGO) && Has(logic, OotSoul::ENEMY_BEAMOS) &&
                   Has(logic, OotSoul::ENEMY_LIKE_LIKE);
        case RC_SPIRIT_TEMPLE_MQ_GS_SUN_BLOCK_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_MQ_LEEVER_ROOM_CHEST:
            return Has(logic, OotSoul::ENEMY_LEEVER);
        case RC_SPIRIT_TEMPLE_MQ_GS_LEEVER_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_MQ_GS_SYMPHONY_ROOM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_MQ_BEAMOS_ROOM_CHEST:
            return Has(logic, OotSoul::ENEMY_BEAMOS);
        case RC_SPIRIT_TEMPLE_MQ_GS_NINE_THRONES_ROOM_WEST:
            return Has(logic, OotSoul::ENEMY_IRON_KNUCKLE) && Has(logic, OotSoul::MISC_GS);
        case RC_SPIRIT_TEMPLE_MQ_GS_NINE_THRONES_ROOM_NORTH:
            return Has(logic, OotSoul::ENEMY_IRON_KNUCKLE) && Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_MQ_COMPASS_CHEST:
            return Has(logic, OotSoul::ENEMY_LIZALFOS_DINOLFOS) && Has(logic, OotSoul::ENEMY_SPIKE);
        case RC_WATER_TEMPLE_MQ_GS_RIVER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_MQ_GS_TRIPLE_WALL_TORCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_MQ_GS_FREESTANDING_KEY_AREA:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_MQ_GS_LIZALFOS_HALLWAY:
            return Has(logic, OotSoul::MISC_GS);
        case RC_WATER_TEMPLE_MQ_GS_BEFORE_UPPER_WATER_SWITCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DMC_GS_CRATE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DMC_DEKU_SCRUB:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_SHEIK_IN_CRATER:
            return Has(logic, OotSoul::NPC_SHEIK);
        case RC_DMC_GS_BEAN_PATCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DMC_UPPER_BOULDER_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_DMC_DEKU_SCRUB_GROTTO_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DMC_DEKU_SCRUB_GROTTO_CENTER:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DMC_DEKU_SCRUB_GROTTO_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_DMT_GS_NEAR_KAK:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DMT_GS_BEAN_PATCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DMT_GS_ABOVE_DODONGOS_CAVERN:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DMT_TRADE_BROKEN_SWORD:
            return Has(logic, OotSoul::NPC_BIGGORON);
        case RC_DMT_TRADE_EYEDROPS:
            return Has(logic, OotSoul::NPC_BIGGORON);
        case RC_DMT_TRADE_CLAIM_CHECK:
            return Has(logic, OotSoul::NPC_BIGGORON);
        case RC_DMT_GS_FALLING_ROCKS_PATH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_DMT_STORMS_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_DMT_COW_GROTTO_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_COLOSSUS_GS_BEAN_PATCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_COLOSSUS_GS_TREE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_COLOSSUS_GS_HILL:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SHEIK_AT_COLOSSUS:
            return Has(logic, OotSoul::NPC_SHEIK);
        case RC_COLOSSUS_DEKU_SCRUB_GROTTO_FRONT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_COLOSSUS_DEKU_SCRUB_GROTTO_REAR:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GF_HBA_1000_POINTS:
            return Has(logic, OotSoul::NPC_THIEVES);
        case RC_GF_HBA_1500_POINTS:
            return Has(logic, OotSoul::NPC_THIEVES);
        case RC_GF_HBA_RANGE_GS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GF_GS_TOP_FLOOR:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GV_GS_BEAN_PATCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GV_GS_SMALL_BRIDGE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GV_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_GV_TRADE_SAW:
            return Has(logic, OotSoul::NPC_CARPENTERS);
        case RC_GV_GS_BEHIND_TENT:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GV_GS_PILLAR:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GV_DEKU_SCRUB_GROTTO_FRONT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GV_DEKU_SCRUB_GROTTO_REAR:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GC_ROLLING_GORON_AS_ADULT:
            return Has(logic, OotSoul::NPC_GORON_CHILD);
        case RC_GC_ROLLING_GORON_AS_CHILD:
            return Has(logic, OotSoul::NPC_GORON);
        case RC_GC_MEDIGORON:
            return Has(logic, OotSoul::NPC_MEDIGORON);
        case RC_GC_GS_CENTER_PLATFORM:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GC_GS_BOULDER_MAZE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GC_DARUNIAS_JOY:
            return Has(logic, OotSoul::NPC_DARUNIA);
        case RC_GC_SHOP_ITEM_1:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_SHOP_ITEM_2:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_SHOP_ITEM_3:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_SHOP_ITEM_4:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_SHOP_ITEM_5:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_SHOP_ITEM_6:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_SHOP_ITEM_7:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_SHOP_ITEM_8:
            return Has(logic, OotSoul::NPC_GORON_SHOPKEEPER);
        case RC_GC_DEKU_SCRUB_GROTTO_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GC_DEKU_SCRUB_GROTTO_CENTER:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GC_DEKU_SCRUB_GROTTO_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_GRAVEYARD_DAMPE_GRAVEDIGGING_TOUR:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GRAVEYARD_GS_BEAN_PATCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GRAVEYARD_GS_WALL:
            return Has(logic, OotSoul::MISC_GS);
        case RC_GY_NEAR_HUT_GRAVE_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_SONG_FROM_ROYAL_FAMILYS_TOMB:
            return Has(logic, OotSoul::ENEMY_KEESE);
        case RC_GRAVEYARD_HOOKSHOT_CHEST:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GRAVEYARD_DAMPE_RACE_FREESTANDING_POH:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GRAVEYARD_DAMPE_RACE_RUPEE_4:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GRAVEYARD_DAMPE_RACE_RUPEE_5:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GRAVEYARD_DAMPE_RACE_RUPEE_6:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GRAVEYARD_DAMPE_RACE_RUPEE_7:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GRAVEYARD_DAMPE_RACE_RUPEE_8:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_6:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_7:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_8:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_9:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_10:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_11:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_14:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_13:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_12:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_GY_WONDER_DAMPE_RACE_15:
            return Has(logic, OotSoul::NPC_DAMPE);
        case RC_WASTELAND_BOMBCHU_SALESMAN:
            return Has(logic, OotSoul::NPC_CARPET_MAN);
        case RC_WASTELAND_GS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_HF_OCARINA_OF_TIME_ITEM:
            return Has(logic, OotSoul::NPC_ZELDA);
        case RC_SONG_FROM_OCARINA_OF_TIME:
            return Has(logic, OotSoul::NPC_ZELDA);
        case RC_HF_DEKU_SCRUB_GROTTO:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_HF_OPEN_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_HF_SOUTHEAST_BOULDER_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_HF_NEAR_MARKET_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_HF_GS_COW_GROTTO:
            return Has(logic, OotSoul::MISC_GS);
        case RC_HF_COW_GROTTO_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_HF_GS_NEAR_KAK_GROTTO:
            return Has(logic, OotSoul::MISC_GS);
        case RC_HC_GS_TREE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_HC_NEAR_ARCHWAY_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_HC_NEAR_BOULDER_PATH_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_HC_NEAR_STAIRS_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_HC_NEAR_WALL_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_HC_GS_STORMS_GROTTO:
            return Has(logic, OotSoul::MISC_GS);
        case RC_OGC_GS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KAK_ANJU_AS_CHILD:
            return Has(logic, OotSoul::ANIMAL_CUCCO) && Has(logic, OotSoul::NPC_ANJU);
        case RC_KAK_ANJU_AS_ADULT:
            return Has(logic, OotSoul::NPC_ANJU);
        case RC_KAK_TRADE_POCKET_CUCCO:
            return Has(logic, OotSoul::NPC_ANJU);
        case RC_SHEIK_IN_KAKARIKO:
            return Has(logic, OotSoul::NPC_SHEIK);
        case RC_KAK_MAN_ON_ROOF:
            return Has(logic, OotSoul::NPC_ROOFTOP_MAN);
        case RC_KAK_GS_HOUSE_UNDER_CONSTRUCTION:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KAK_GS_TREE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KAK_GS_SKULLTULA_HOUSE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KAK_GS_GUARDS_HOUSE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KAK_GS_WATCHTOWER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KAK_WATCHTOWER_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_KAK_GS_ABOVE_IMPAS_HOUSE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KAK_BAZAAR_ITEM_1:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_BAZAAR_ITEM_2:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_BAZAAR_ITEM_3:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_BAZAAR_ITEM_4:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_BAZAAR_ITEM_5:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_BAZAAR_ITEM_6:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_BAZAAR_ITEM_7:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_BAZAAR_ITEM_8:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_1:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_2:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_3:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_4:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_5:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_6:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_7:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_POTION_SHOP_ITEM_8:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_KAK_TRADE_ODD_MUSHROOM:
            return Has(logic, OotSoul::NPC_OLD_HAG);
        case RC_KAK_GRANNYS_SHOP:
            return Has(logic, OotSoul::NPC_OLD_HAG);
        case RC_KAK_SHOOTING_GALLERY_REWARD:
            return Has(logic, OotSoul::NPC_SHOOTING_GALLERY_OWNER);
        case RC_KAK_IMPAS_HOUSE_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_SONG_FROM_WINDMILL:
            return Has(logic, OotSoul::NPC_GURU_GURU);
        case RC_KAK_REDEAD_GROTTO_CHEST:
            return Has(logic, OotSoul::ENEMY_REDEAD_GIBDO);
        case RC_KAK_OPEN_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_KF_LINKS_HOUSE_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_KF_GS_BEAN_PATCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KF_GS_HOUSE_OF_TWINS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KF_GS_KNOW_IT_ALL_HOUSE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_KF_SHOP_ITEM_1:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_SHOP_ITEM_2:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_SHOP_ITEM_3:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_SHOP_ITEM_4:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_SHOP_ITEM_5:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_SHOP_ITEM_6:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_SHOP_ITEM_7:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_SHOP_ITEM_8:
            return Has(logic, OotSoul::NPC_KOKIRI_SHOPKEEPER);
        case RC_KF_STORMS_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_LH_GS_LAB_WALL:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LH_GS_SMALL_ISLAND:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LH_GS_BEAN_PATCH:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LH_GS_TREE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LH_SCARECROW_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_LH_LAB_DIVE:
            return Has(logic, OotSoul::NPC_SCIENTIST);
        case RC_LH_TRADE_FROG:
            return Has(logic, OotSoul::NPC_SCIENTIST);
        case RC_LH_GS_LAB_CRATE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LH_CHILD_FISHING:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISHING:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_1:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_2:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_3:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_4:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_5:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_6:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_7:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_8:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_9:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_10:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_11:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_12:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_13:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_14:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_FISH_15:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_LOACH_1:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_CHILD_LOACH_2:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_1:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_2:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_3:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_4:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_5:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_6:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_7:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_8:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_9:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_10:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_11:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_12:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_13:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_14:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_FISH_15:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_ADULT_LOACH:
            return Has(logic, OotSoul::NPC_FISHING_POND_OWNER);
        case RC_LH_DEKU_SCRUB_GROTTO_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LH_DEKU_SCRUB_GROTTO_CENTER:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LH_DEKU_SCRUB_GROTTO_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LLR_GS_TREE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LLR_GS_HOUSE_WINDOW:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LLR_GS_RAIN_SHED:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LLR_GS_BACK_WALL:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LLR_STABLES_LEFT_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_LLR_STABLES_RIGHT_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_LLR_TOWER_LEFT_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_LLR_TOWER_RIGHT_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_LLR_TALONS_CHICKENS:
            return Has(logic, OotSoul::ANIMAL_CUCCO) && Has(logic, OotSoul::NPC_TALON);
        case RC_LLR_DEKU_SCRUB_GROTTO_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LLR_DEKU_SCRUB_GROTTO_CENTER:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LLR_DEKU_SCRUB_GROTTO_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LW_DEKU_SCRUB_NEAR_BRIDGE:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LW_TRADE_COJIRO:
            return Has(logic, OotSoul::NPC_GROG);
        case RC_LW_TRADE_ODD_POTION:
            return Has(logic, OotSoul::NPC_KOKIRI);
        case RC_LW_GS_BEAN_PATCH_NEAR_BRIDGE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LW_TUNNEL_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_LW_GIFT_FROM_SARIA:
            return Has(logic, OotSoul::NPC_SARIA);
        case RC_LW_DEKU_SCRUB_NEAR_DEKU_THEATER_LEFT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LW_DEKU_SCRUB_NEAR_DEKU_THEATER_RIGHT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LW_GS_BEAN_PATCH_NEAR_THEATER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LW_GS_ABOVE_THEATER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_LW_MEADOW_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_LW_DEKU_SCRUB_GROTTO_FRONT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_LW_DEKU_SCRUB_GROTTO_REAR:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_MARKET_BAZAAR_ITEM_1:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_BAZAAR_ITEM_2:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_BAZAAR_ITEM_3:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_BAZAAR_ITEM_4:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_BAZAAR_ITEM_5:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_BAZAAR_ITEM_6:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_BAZAAR_ITEM_7:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_BAZAAR_ITEM_8:
            return Has(logic, OotSoul::NPC_BAZAAR_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_1:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_2:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_3:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_4:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_5:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_6:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_7:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_POTION_SHOP_ITEM_8:
            return Has(logic, OotSoul::NPC_POTION_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_1:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_2:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_3:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_4:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_5:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_6:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_7:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_BOMBCHU_SHOP_ITEM_8:
            return Has(logic, OotSoul::NPC_BOMBCHU_SHOPKEEPER);
        case RC_MARKET_GS_GUARD_HOUSE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_MARKET_10_BIG_POES:
            return Has(logic, OotSoul::NPC_POE_COLLECTOR);
        case RC_MARKET_LOST_DOG:
            return Has(logic, OotSoul::ANIMAL_DOG) && Has(logic, OotSoul::NPC_DOG_LADY);
        case RC_MARKET_SHOOTING_GALLERY_REWARD:
            return Has(logic, OotSoul::NPC_SHOOTING_GALLERY_OWNER);
        case RC_SFM_WOLFOS_GROTTO_CHEST:
            return Has(logic, OotSoul::ENEMY_WOLFOS);
        case RC_SONG_FROM_SARIA:
            return Has(logic, OotSoul::NPC_SARIA);
        case RC_SHEIK_IN_FOREST:
            return Has(logic, OotSoul::NPC_SHEIK);
        case RC_SFM_GS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_SFM_DEKU_SCRUB_GROTTO_FRONT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_SFM_DEKU_SCRUB_GROTTO_REAR:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_SHEIK_AT_TEMPLE:
            return Has(logic, OotSoul::NPC_SHEIK);
        case RC_TOT_LIGHT_ARROWS_CUTSCENE:
            return Has(logic, OotSoul::NPC_SHEIK) && Has(logic, OotSoul::NPC_ZELDA);
        case RC_ZD_DIVING_MINIGAME:
            return Has(logic, OotSoul::NPC_ZORA);
        case RC_ZD_KING_ZORA_THAWED:
            return Has(logic, OotSoul::NPC_KING_ZORA);
        case RC_ZD_TRADE_PRESCRIPTION:
            return Has(logic, OotSoul::NPC_KING_ZORA);
        case RC_ZD_GS_FROZEN_WATERFALL:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZD_SHOP_ITEM_1:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZD_SHOP_ITEM_2:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZD_SHOP_ITEM_3:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZD_SHOP_ITEM_4:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZD_SHOP_ITEM_5:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZD_SHOP_ITEM_6:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZD_SHOP_ITEM_7:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZD_SHOP_ITEM_8:
            return Has(logic, OotSoul::NPC_ZORA_SHOPKEEPER);
        case RC_ZF_GS_ABOVE_THE_LOG:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZF_GS_TREE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZF_GS_HIDDEN_CAVE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZR_GS_TREE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZR_MAGIC_BEAN_SALESMAN:
            return Has(logic, OotSoul::NPC_BEAN_SALESMAN);
        case RC_ZR_GS_LADDER:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZR_GS_NEAR_RAISED_GROTTOS:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZR_GS_ABOVE_BRIDGE:
            return Has(logic, OotSoul::MISC_GS);
        case RC_ZR_NEAR_ROCK_CIRCLE_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_ZR_WATERFALL_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_ZR_DEKU_SCRUB_GROTTO_FRONT:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_ZR_DEKU_SCRUB_GROTTO_REAR:
            return Has(logic, OotSoul::MISC_BUSINESS_SCRUB);
        case RC_ZR_OPEN_GROTTO_BUTTERFLY_FAIRY:
            return Has(logic, OotSoul::ANIMAL_BUTTERFLY);
        case RC_GANONS_CASTLE_SHADOW_TRIAL_GOLDEN_GAUNTLETS_CHEST:
            return ShadowTrialSoulAlternative(logic->CanUse(RG_LONGSHOT), Has(logic, OotSoul::ENEMY_LIKE_LIKE),
                                              logic->CanUse(RG_FIRE_ARROWS), logic->CanUse(RG_HOVER_BOOTS),
                                              logic->CanUse(RG_DINS_FIRE));
        case RC_GANONS_CASTLE_SHADOW_TRIAL_POT_1:
            return ShadowTrialSoulAlternative(logic->CanUse(RG_LONGSHOT), Has(logic, OotSoul::ENEMY_LIKE_LIKE),
                                              logic->CanUse(RG_FIRE_ARROWS), logic->CanUse(RG_HOVER_BOOTS),
                                              logic->CanUse(RG_DINS_FIRE));
        case RC_GANONS_CASTLE_SHADOW_TRIAL_POT_2:
            return ShadowTrialSoulAlternative(logic->CanUse(RG_LONGSHOT), Has(logic, OotSoul::ENEMY_LIKE_LIKE),
                                              logic->CanUse(RG_FIRE_ARROWS), logic->CanUse(RG_HOVER_BOOTS),
                                              logic->CanUse(RG_DINS_FIRE));
        case RC_GANONS_CASTLE_SHADOW_TRIAL_HEART_1:
            return ShadowTrialSoulAlternative(logic->CanUse(RG_LONGSHOT), Has(logic, OotSoul::ENEMY_LIKE_LIKE),
                                              logic->CanUse(RG_FIRE_ARROWS), logic->CanUse(RG_HOVER_BOOTS),
                                              logic->CanUse(RG_DINS_FIRE));
        case RC_GANONS_CASTLE_SHADOW_TRIAL_HEART_2:
            return ShadowTrialSoulAlternative(logic->CanUse(RG_LONGSHOT), Has(logic, OotSoul::ENEMY_LIKE_LIKE),
                                              logic->CanUse(RG_FIRE_ARROWS), logic->CanUse(RG_HOVER_BOOTS),
                                              logic->CanUse(RG_DINS_FIRE));
        case RC_GANONS_CASTLE_SHADOW_TRIAL_HEART_3:
            return ShadowTrialSoulAlternative(logic->CanUse(RG_LONGSHOT), Has(logic, OotSoul::ENEMY_LIKE_LIKE),
                                              logic->CanUse(RG_FIRE_ARROWS), logic->CanUse(RG_HOVER_BOOTS),
                                              logic->CanUse(RG_DINS_FIRE));
        case RC_GV_CRATE_FREESTANDING_POH:
        case RC_GV_FREESTANDING_POH_CRATE:
            return (logic->IsChild && Has(logic, OotSoul::ANIMAL_CUCCO)) || logic->CanUse(RG_LONGSHOT);
        case RC_GV_WATERFALL_FREESTANDING_POH:
            return (logic->IsChild && Has(logic, OotSoul::ANIMAL_CUCCO)) || logic->HasItem(RG_BRONZE_SCALE) ||
                   logic->CanUse(RG_LONGSHOT) || (logic->CanUse(RG_IRON_BOOTS) && logic->CanUse(RG_HOOKSHOT));
        case RC_ZR_NEAR_OPEN_GROTTO_FREESTANDING_POH:
        case RC_ZR_NEAR_DOMAIN_FREESTANDING_POH:
            return (logic->IsChild && Has(logic, OotSoul::ANIMAL_CUCCO)) || logic->CanUse(RG_BOOMERANG) ||
                   logic->CanUse(RG_HOVER_BOOTS);
        case RC_ZR_NEAR_FREESTANDING_POH_GRASS:
            return (((logic->IsChild && Has(logic, OotSoul::ANIMAL_CUCCO)) || logic->CanUse(RG_HOVER_BOOTS)) &&
                    logic->CanCutShrubs()) ||
                   logic->CanUse(RG_BOOMERANG);
        case RC_SHEIK_IN_ICE_CAVERN:
            return Has(logic, OotSoul::NPC_SHEIK);
        case RC_JABU_JABUS_BELLY_MQ_FIRST_ROOM_SIDE_CHEST:
        case RC_JABU_JABUS_BELLY_MQ_WONDER_ENTRANCE_LEFT_COW:
        case RC_JABU_JABUS_BELLY_MQ_WONDER_ENTRANCE_RIGHT_COW:
            return Has(logic, OotSoul::ANIMAL_COW);
        case RC_SPIRIT_TEMPLE_MQ_MAP_CHEST:
            return Has(logic, OotSoul::ENEMY_TORCH_SLUG);
        case RC_SPIRIT_TEMPLE_HALLWAY_RIGHT_INVISIBLE_CHEST:
        case RC_SPIRIT_TEMPLE_HALLWAY_LEFT_INVISIBLE_CHEST:
            return Has(logic, OotSoul::ENEMY_ARMOS);
        case RC_MARKET_TREASURE_CHEST_GAME_REWARD:
        case RC_MARKET_TREASURE_CHEST_GAME_KEY_1:
        case RC_MARKET_TREASURE_CHEST_GAME_ITEM_1:
        case RC_MARKET_TREASURE_CHEST_GAME_KEY_2:
        case RC_MARKET_TREASURE_CHEST_GAME_ITEM_2:
        case RC_MARKET_TREASURE_CHEST_GAME_KEY_3:
        case RC_MARKET_TREASURE_CHEST_GAME_ITEM_3:
        case RC_MARKET_TREASURE_CHEST_GAME_KEY_4:
        case RC_MARKET_TREASURE_CHEST_GAME_ITEM_4:
        case RC_MARKET_TREASURE_CHEST_GAME_KEY_5:
        case RC_MARKET_TREASURE_CHEST_GAME_ITEM_5:
            return TcgSoulAccess(logic);
        default:
            return true;
    }
}
} // namespace Rando::Souls
