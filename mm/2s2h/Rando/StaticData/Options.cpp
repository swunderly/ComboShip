#include "StaticData.h"

extern "C" {
#include "overlays/actors/ovl_En_Sth/z_en_sth.h"
}

namespace Rando {

namespace StaticData {

#define RO(id, defaultValue)                             \
    {                                                    \
        id, {                                            \
            id, #id, "gRando.Options." #id, defaultValue \
        }                                                \
    }

// Unreleased features have no CVar, so no profile, seed or config can switch them on: generation
// reads the default and ApplyToSaveContext saves it. Use RO() once the feature is released.
#define RO_UNRELEASED(id, defaultValue) \
    {                                   \
        id, {                           \
            id, #id, "", defaultValue   \
        }                               \
    }

// clang-format off
std::map<RandoOptionId, RandoStaticOption> Options = {
    RO_UNRELEASED(RO_MM_COMPLETE_SOULS, RO_GENERIC_NO), // Internal only; no UI enable before linked validation.
    RO(RO_MM_SPELL_WIND, RO_GENERIC_NO),
    RO(RO_CROSS_GAME_FARORE, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_CROSS_AGE, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_ADULT_MASK, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_BOOTS_IRON, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_BOOTS_HOVER, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_TUNIC_GORON, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_TUNIC_ZORA, RO_GENERIC_NO),

    RO_UNRELEASED(RO_MM_STARTING_AGE, 0),
    RO(RO_SHARED_SPELL_WIND, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_SPELL_FIRE, RO_GENERIC_NO),
    RO_UNRELEASED(RO_MM_SPELL_LOVE, RO_GENERIC_NO),
    RO(RO_SHARED_SPELL_FIRE, RO_GENERIC_NO),
    RO(RO_SHARED_SPELL_LOVE, RO_GENERIC_NO),
    RO(RO_SHARED_OCARINA, RO_GENERIC_NO), // internal effective-seed mask, no UI row
    RO_UNRELEASED(RO_FAIRY_OCARINA_MM, RO_GENERIC_NO), // internal draft, no option-group/UI row

    RO(RO_SHUFFLE_NPC_SOULS, RO_GENERIC_NO),
    RO(RO_SHUFFLE_ANIMAL_SOULS, RO_GENERIC_NO),
    RO(RO_SHUFFLE_MISC_SOULS, RO_GENERIC_NO),
    RO(RO_KEY_RING_WOODFALL, RO_GENERIC_NO),
    RO(RO_KEY_RING_SNOWHEAD, RO_GENERIC_NO),
    RO(RO_KEY_RING_GREAT_BAY, RO_GENERIC_NO),
    RO(RO_KEY_RING_STONE_TOWER, RO_GENERIC_NO),

    RO(RO_ACCESS_DUNGEONS,             RO_ACCESS_DUNGEONS_FORM_AND_SONG),
    RO(RO_ACCESS_MAJORA_MASKS_COUNT,   0),
    RO(RO_ACCESS_MAJORA_REMAINS_COUNT, 0),
    RO(RO_ACCESS_MOON_MASKS_COUNT,     0),
    RO(RO_ACCESS_MOON_REMAINS_COUNT,   4),
    RO(RO_ACCESS_TRIALS,               RO_ACCESS_TRIALS_20_MASKS),
    RO(RO_CLOCK_SHUFFLE_PROGRESSIVE,   RO_CLOCK_SHUFFLE_RANDOM),
    RO(RO_CLOCK_SHUFFLE,               RO_GENERIC_OFF),
    RO(RO_CLOCK_TERMINAL_TIME,         350), // Default: 05:50 (10 "minutes" before crash)
    RO(RO_HINTS_BANK_SIGN,             RO_GENERIC_OFF),
    RO(RO_HINTS_BOSS_REMAINS,          RO_GENERIC_OFF),
    RO(RO_HINTS_GOSSIP_STONE_STRENGTH, 50),
    RO(RO_HINTS_GOSSIP_STONES,         RO_GENERIC_OFF),
    RO(RO_HINTS_HOOKSHOT,              RO_GENERIC_OFF),
    RO(RO_HINTS_OATH_TO_ORDER,         RO_GENERIC_OFF),
    RO(RO_HINTS_PURCHASEABLE,          RO_GENERIC_OFF),
    RO(RO_HINTS_SONG_OF_SOARING,       RO_GENERIC_OFF),
    RO(RO_HINTS_SPIDER_HOUSES,         RO_GENERIC_OFF),
    RO(RO_HINTS_TRANSFORMATIONS,       RO_GENERIC_OFF),
    RO(RO_LOGIC,                       RO_LOGIC_GLITCHLESS),
    RO(RO_PLACEMENT_BOSS_KEYS,         RO_DUNGEON_ITEM_ANYWHERE),
    RO(RO_PLACEMENT_SMALL_KEYS,        RO_DUNGEON_ITEM_ANYWHERE),
    RO(RO_PLACEMENT_STRAY_FAIRIES,     RO_DUNGEON_ITEM_ANYWHERE),
    RO(RO_PLENTIFUL_ITEMS,             RO_GENERIC_OFF),
    RO(RO_PURCHASE_INFINITE_RUPEES,    RO_GENERIC_OFF),
    RO(RO_SHUFFLE_BARREL_DROPS,        RO_GENERIC_OFF),
    RO(RO_SHUFFLE_BOSS_REMAINS,        RO_GENERIC_OFF),
    RO(RO_SHUFFLE_BOSS_SOULS,          RO_GENERIC_OFF),
    RO(RO_SHUFFLE_BUTTERFLIES,         RO_GENERIC_OFF),
    RO(RO_SHUFFLE_COWS,                RO_GENERIC_OFF),
    RO(RO_SHUFFLE_CRATE_DROPS,         RO_GENERIC_OFF),
    RO(RO_SHUFFLE_ENEMY_DROPS,         RO_GENERIC_OFF),
    RO(RO_SHUFFLE_ENEMY_SOULS,         RO_GENERIC_OFF),
    RO(RO_SHUFFLE_FREESTANDING_ITEMS,  RO_GENERIC_OFF),
    RO(RO_SHUFFLE_FROGS,               RO_GENERIC_OFF),
    RO(RO_SHUFFLE_GOLD_SKULLTULAS,     RO_GENERIC_OFF),
    RO(RO_SHUFFLE_GRASS_DROPS,         RO_GENERIC_OFF),
    RO(RO_SHUFFLE_HIVE_DROPS,          RO_GENERIC_OFF),
    RO(RO_SHUFFLE_OCARINA_BUTTONS,     RO_GENERIC_OFF),
    RO(RO_SHUFFLE_OWL_STATUES,         RO_GENERIC_OFF),
    RO(RO_SHUFFLE_POT_DROPS,           RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SHOPS,               RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SKELETON_KEY,        RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SNOWBALL_DROPS,      RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SONG_DOUBLE_TIME,    RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SONG_INVERTED_TIME,  RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SONG_SARIA,          RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SONG_SUN,            RO_GENERIC_OFF),
    RO(RO_SHUFFLE_SWIM,                RO_GENERIC_OFF),
    RO(RO_SHUFFLE_TINGLE_SHOPS,        RO_GENERIC_OFF),
    RO(RO_SHUFFLE_TRAPS,               RO_GENERIC_OFF),
    RO(RO_SHUFFLE_TREE_DROPS,          RO_GENERIC_OFF),
    RO(RO_SHUFFLE_TRIFORCE_PIECES,     RO_GENERIC_OFF),
    RO(RO_SHUFFLE_TYCOON_WALLET,       RO_GENERIC_OFF),
    RO(RO_SHUFFLE_WONDER_ITEMS,        RO_GENERIC_OFF),
    RO(RO_SKULLTULA_SHUFFLED,          SPIDER_HOUSE_TOKENS_REQUIRED),
    RO(RO_SKULLTULA_TOKENS_REQUIRED,   SPIDER_HOUSE_TOKENS_REQUIRED),
    RO(RO_STARTING_CONSUMABLES,        RO_GENERIC_OFF),
    RO(RO_STARTING_HEALTH,             3),
    RO(RO_STARTING_MAPS_AND_COMPASSES, RO_GENERIC_OFF),
    RO(RO_STARTING_RUPEES,             RO_GENERIC_OFF),
    RO(RO_STRAY_FAIRIES_MAX,           STRAY_FAIRY_SCATTERED_TOTAL),
    RO(RO_STRAY_FAIRIES_REQUIRED,      STRAY_FAIRY_SCATTERED_TOTAL),
    RO(RO_TRAP_AMOUNT,                 5),
    RO(RO_TRIFORCE_PIECES_MAX,         DEFAULT_TRIFORCE_PIECES_MAX),
    RO(RO_TRIFORCE_PIECES_REQUIRED,    DEFAULT_TRIFORCE_PIECES_MAX),
};
// clang-format on

RandoOptionId GetOptionIdFromName(const char* name) {
    for (auto& [randoOptionId, randoStaticOption] : Options) {
        if (strcmp(name, randoStaticOption.name) == 0) {
            return randoOptionId;
        }
    }
    return RO_MAX;
}

} // namespace StaticData

} // namespace Rando
