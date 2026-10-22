// Unified OoTMM entrance layout (schema ootmm-entrance-layout-v1): the envelope a seed
// records under "entranceLayout", its strict codec and its validator. Adapted from OoTMM
// v32.3 (413573d) packages/logic/src/solver/entrances.ts; see LICENSE.OoTMM.
//
// Validate() recomputes the pools from LayoutV1 or LayoutV2, frozen copies of the pinned pool
// rules and rows, never from the live Shuffle.h or Registry.h, so a layout keeps validating when
// the live rules change or the reference is re-pinned (M1 plan, critic correction C6). The
// layout's capability id picks the rule set: LayoutV2 for wallmasters-full-v1, LayoutV1 for
// every earlier id. Tests keep both equal to ResolvePools and to tools/ootmm/m1-contract.json.
// Nothing here installs a layout.
#pragma once
#include "OotmmSettings.h"
#include <algorithm>
#include <array>
#include <initializer_list>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ComboRando::Entrances {
inline constexpr std::string_view kEntranceLayoutSchema = "ootmm-entrance-layout-v1";
// The dev-only MM tranche: OoT Wallmaster rows are not bound yet, so their areas count as absent.
inline constexpr std::string_view kMmTrancheCapability = "wallmasters-own-game-mm-tranche";
inline constexpr std::string_view kVanillaWallmasterCapability = "wallmasters-own-game-vanilla-v1";
// Own Game Wallmasters with at least one Master Quest dungeon; an all-vanilla layout takes the vanilla id.
inline constexpr std::string_view kQuestsWallmasterCapability = "wallmasters-own-game-quests-v1";
static_assert(CapabilitySet::Known(kMmTrancheCapability) &&
                  kEntranceCapabilities[CapabilityDetail::Find(kMmTrancheCapability)].devOnly,
              "The MM tranche is a devOnly capability");
static_assert(CapabilitySet::Known(kQuestsWallmasterCapability) &&
                  !kEntranceCapabilities[CapabilityDetail::Find(kQuestsWallmasterCapability)].devOnly,
              "The Master Quest Wallmaster capability is an ordinary staged row");
static_assert(CapabilitySet::Known(kSongsWallmasterCapability) &&
                  !kEntranceCapabilities[CapabilityDetail::Find(kSongsWallmasterCapability)].devOnly,
              "The foreign-song Wallmaster capability is an ordinary staged row");
static_assert(CapabilitySet::Known(kStartingSongsWallmasterCapability) &&
                  !kEntranceCapabilities[CapabilityDetail::Find(kStartingSongsWallmasterCapability)].devOnly,
              "The starting-song Wallmaster capability is an ordinary staged row");
static_assert(CapabilitySet::Known(kFullWallmasterCapability) &&
                  !kEntranceCapabilities[CapabilityDetail::Find(kFullWallmasterCapability)].devOnly,
              "The Full Wallmaster capability is an ordinary staged row");

class EntranceLayoutError : public std::invalid_argument {
  public:
    using std::invalid_argument::invalid_argument;
};

// The OoT dungeons with a Master Quest layout, by their mqDungeons names
// (packages/core/src/settings/data.ts:3653-3666). A layout records the quest of all twelve.
inline constexpr std::array<std::string_view, 12> kOotMqDungeons = {
    "DT", "DC", "JJ", "Forest", "Fire", "Water", "Spirit", "Shadow", "BotW", "IC", "GTG", "Ganon",
};
using OotQuests = std::array<bool, kOotMqDungeons.size()>; // true: Master Quest
inline constexpr size_t OotMqIndex(std::string_view dungeon) {
    for (size_t i = 0; i < kOotMqDungeons.size(); ++i)
        if (kOotMqDungeons[i] == dungeon)
            return i;
    return kOotMqDungeons.size();
}

// Frozen copy of the pinned rules for every layout v1 capability id. Never edit it to follow
// Shuffle.h or Registry.h: a new rule set gets a new copy and new capability ids.
namespace LayoutV1 {
inline constexpr std::string_view kUpstreamCommit = "413573d22d1d53e8a81931a70927f625b73488b9";
inline constexpr std::string_view kRegistrySha256 = "b95aee4e8dfb5b9015aca60ad98f11aeb12e86fec42528e9dddd4043119f709d";

// Every row of the types v1 pools use, their reverses and the game links, as Registry.h has
// them at the pin above (the last field is the data/defs/entrances.yml line).
// clang-format off
inline constexpr Record kRows[] = {
    { "OOT_DEKU_TREE", Game::Oot, 0x0u, Type::Dungeon, "OOT_KOKIRI_FOREST_FROM_DEKU_TREE",
      "OOT Kokiri Forest Near Deku Tree", "OOT Deku Tree", "OOT_KOKIRI_FOREST", "OOT_DEKU_TREE", "NONE", "NONE", false,
      11 },
    { "OOT_DODONGO_CAVERN", Game::Oot, 0x4u, Type::Dungeon, "OOT_MOUNTAIN_TRAIL_FROM_DODONGO_CAVERN",
      "OOT Death Mountain", "OOT Dodongo Cavern", "OOT_DEATH_MOUNTAIN_TRAIL", "OOT_DODONGO_CAVERN", "NONE", "NONE",
      false, 12 },
    { "OOT_JABU_JABU", Game::Oot, 0x28u, Type::Dungeon, "OOT_ZORA_FOUNTAIN_FROM_JABU_JABU", "OOT Zora Fountain",
      "OOT Jabu-Jabu", "OOT_ZORA_FOUNTAIN", "OOT_JABU_JABU", "NONE", "NONE", false, 13 },
    { "OOT_TEMPLE_FOREST", Game::Oot, 0x169u, Type::Dungeon, "OOT_SACRED_MEADOW_FROM_TEMPLE_FOREST",
      "OOT Sacred Meadow Forest Platform", "OOT Forest Temple", "OOT_SACRED_MEADOW", "OOT_TEMPLE_FOREST", "NONE",
      "NONE", false, 14 },
    { "OOT_TEMPLE_FIRE", Game::Oot, 0x165u, Type::Dungeon, "OOT_DEATH_CRATER_FROM_TEMPLE_FIRE",
      "OOT Death Mountain Crater Near Temple", "OOT Fire Temple", "OOT_DEATH_MOUNTAIN_CRATER", "OOT_TEMPLE_FIRE",
      "NONE", "NONE", false, 15 },
    { "OOT_TEMPLE_WATER", Game::Oot, 0x10u, Type::Dungeon, "OOT_LAKE_HYLIA_FROM_TEMPLE_WATER", "OOT Lake Hylia",
      "OOT Water Temple", "OOT_LAKE_HYLIA", "OOT_TEMPLE_WATER", "NONE", "NONE", false, 16 },
    { "OOT_TEMPLE_SHADOW", Game::Oot, 0x37u, Type::Dungeon, "OOT_GRAVEYARD_FROM_TEMPLE_SHADOW", "OOT Graveyard Upper",
      "OOT Shadow Temple", "OOT_GRAVEYARD", "OOT_TEMPLE_SHADOW", "NONE", "NONE", false, 17 },
    { "OOT_TEMPLE_SPIRIT", Game::Oot, 0x82u, Type::Dungeon, "OOT_DESERT_COLOSSUS_FROM_TEMPLE_SPIRIT",
      "OOT Desert Colossus", "OOT Spirit Temple", "OOT_COLOSSUS", "OOT_TEMPLE_SPIRIT", "NONE", "NONE", false, 18 },
    { "OOT_BOTTOM_OF_THE_WELL", Game::Oot, 0x98u, Type::DungeonMinor, "OOT_KAKARIKO_FROM_BOTTOM_OF_THE_WELL",
      "OOT Kakariko Well", "OOT Bottom of the Well", "OOT_KAKARIKO", "OOT_BOTTOM_OF_THE_WELL", "NONE", "NONE", false,
      19 },
    { "OOT_ICE_CAVERN", Game::Oot, 0x88u, Type::DungeonMinor, "OOT_ZORA_FOUNTAIN_FROM_ICE_CAVERN",
      "OOT Zora Fountain Frozen", "OOT Ice Cavern", "OOT_ZORA_FOUNTAIN", "OOT_ICE_CAVERN", "NONE", "NONE", false, 20 },
    { "OOT_GERUDO_TRAINING_GROUNDS", Game::Oot, 0x8u, Type::DungeonMinor, "OOT_GERUDO_FORTRESS_FROM_GERUDO_TRAINING",
      "OOT Gerudo Fortress Exterior", "OOT Gerudo Training Grounds", "OOT_GERUDO_FORTRESS",
      "OOT_GERUDO_TRAINING_GROUNDS", "NONE", "NONE", false, 21 },
    { "OOT_KOKIRI_FOREST_FROM_DEKU_TREE", Game::Oot, 0x209u, Type::DungeonExit, "OOT_DEKU_TREE", "OOT Deku Tree",
      "OOT Kokiri Forest Near Deku Tree", "OOT_DEKU_TREE", "OOT_KOKIRI_FOREST", "NONE", "NONE", false, 24 },
    { "OOT_MOUNTAIN_TRAIL_FROM_DODONGO_CAVERN", Game::Oot, 0x242u, Type::DungeonExit, "OOT_DODONGO_CAVERN",
      "OOT Dodongo Cavern", "OOT Death Mountain", "OOT_DODONGO_CAVERN", "OOT_DEATH_MOUNTAIN_TRAIL", "NONE", "NONE",
      false, 25 },
    { "OOT_ZORA_FOUNTAIN_FROM_JABU_JABU", Game::Oot, 0x221u, Type::DungeonExit, "OOT_JABU_JABU", "OOT Jabu-Jabu",
      "OOT Zora Fountain", "OOT_JABU_JABU", "OOT_ZORA_FOUNTAIN", "NONE", "NONE", false, 26 },
    { "OOT_SACRED_MEADOW_FROM_TEMPLE_FOREST", Game::Oot, 0x215u, Type::DungeonExit, "OOT_TEMPLE_FOREST",
      "OOT Forest Temple", "OOT Sacred Meadow Forest Platform", "OOT_TEMPLE_FOREST", "OOT_SACRED_MEADOW", "NONE",
      "NONE", false, 27 },
    { "OOT_DEATH_CRATER_FROM_TEMPLE_FIRE", Game::Oot, 0x24au, Type::DungeonExit, "OOT_TEMPLE_FIRE", "OOT Fire Temple",
      "OOT Death Mountain Crater Near Temple", "OOT_TEMPLE_FIRE", "OOT_DEATH_MOUNTAIN_CRATER", "NONE", "NONE", false,
      28 },
    { "OOT_LAKE_HYLIA_FROM_TEMPLE_WATER", Game::Oot, 0x21du, Type::DungeonExit, "OOT_TEMPLE_WATER", "OOT Water Temple",
      "OOT Lake Hylia Near Water Temple", "OOT_TEMPLE_WATER", "OOT_LAKE_HYLIA", "NONE", "NONE", false, 29 },
    { "OOT_GRAVEYARD_FROM_TEMPLE_SHADOW", Game::Oot, 0x205u, Type::DungeonExit, "OOT_TEMPLE_SHADOW",
      "OOT Shadow Temple", "OOT Graveyard Upper", "OOT_TEMPLE_SHADOW", "OOT_GRAVEYARD", "NONE", "NONE", false, 30 },
    { "OOT_DESERT_COLOSSUS_FROM_TEMPLE_SPIRIT", Game::Oot, 0x1e1u, Type::DungeonExit, "OOT_TEMPLE_SPIRIT",
      "OOT Spirit Temple", "OOT Desert Colossus Spirit Exit", "OOT_TEMPLE_SPIRIT", "OOT_COLOSSUS", "NONE", "NONE",
      false, 31 },
    { "OOT_KAKARIKO_FROM_BOTTOM_OF_THE_WELL", Game::Oot, 0x2a6u, Type::DungeonExit, "OOT_BOTTOM_OF_THE_WELL",
      "OOT Bottom of the Well", "OOT Kakariko Well", "OOT_BOTTOM_OF_THE_WELL", "OOT_KAKARIKO", "NONE", "NONE", false,
      32 },
    { "OOT_ZORA_FOUNTAIN_FROM_ICE_CAVERN", Game::Oot, 0x3d4u, Type::DungeonExit, "OOT_ICE_CAVERN", "OOT Ice Cavern",
      "OOT Zora Fountain Frozen", "OOT_ICE_CAVERN", "OOT_ZORA_FOUNTAIN", "NONE", "NONE", false, 33 },
    { "OOT_GERUDO_FORTRESS_FROM_GERUDO_TRAINING", Game::Oot, 0x3a8u, Type::DungeonExit, "OOT_GERUDO_TRAINING_GROUNDS",
      "OOT Gerudo Training Grounds", "OOT Gerudo Fortress Exterior", "OOT_GERUDO_TRAINING_GROUNDS",
      "OOT_GERUDO_FORTRESS", "NONE", "NONE", false, 34 },
    { "OOT_LAKE_HYLIA_FROM_VALLEY", Game::Oot, 0x219u, Type::OneWay, "", "OOT Gerudo Valley Falls", "OOT Lake Hylia",
      "NONE", "NONE", "NONE", "NONE", false, 89 },
    { "OOT_VILLAGE_OWL", Game::Oot, 0x554u, Type::OneWayOwl, "", "OOT Death Mountain Summit", "OOT Kakariko Rooftop",
      "OOT_DEATH_MOUNTAIN_TRAIL", "OOT_KAKARIKO", "NONE", "NONE", false, 180 },
    { "OOT_FIELD_OWL", Game::Oot, 0x27eu, Type::OneWayOwl, "", "OOT Lake Hylia", "OOT Hyrule Field Drawbridge",
      "OOT_LAKE_HYLIA", "OOT_HYRULE_FIELD", "NONE", "NONE", false, 181 },
    { "OOT_WALLMASTER_FOREST_CORRIDOR_WEST", Game::Oot, 0xf10u, Type::Wallmaster, "",
      "OOT Forest Temple Wallmaster West", "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 182 },
    { "OOT_WALLMASTER_FOREST_CORRIDOR_EAST", Game::Oot, 0xf11u, Type::Wallmaster, "",
      "OOT Forest Temple Wallmaster East", "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 183 },
    { "OOT_WALLMASTER_SHADOW", Game::Oot, 0xf12u, Type::Wallmaster, "", "OOT Shadow Temple Wallmaster", "OOT VOID",
      "NONE", "NONE", "NONE", "NONE", false, 184 },
    { "OOT_WALLMASTER_SPIRIT_CHILD_RUPEES", Game::Oot, 0xf13u, Type::Wallmaster, "",
      "OOT Spirit Temple Wallmaster Child Rupees", "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 185 },
    { "OOT_WALLMASTER_SPIRIT_ADULT_CLIMB", Game::Oot, 0xf14u, Type::Wallmaster, "",
      "OOT Spirit Temple Wallmaster Adult Climb", "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 186 },
    { "OOT_WALLMASTER_SPIRIT_CHILD_SUN", Game::Oot, 0xf15u, Type::Wallmaster, "",
      "OOT Spirit Temple Wallmaster Child Sun", "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 187 },
    { "OOT_WALLMASTER_SPIRIT_STATUE", Game::Oot, 0xf16u, Type::Wallmaster, "", "OOT Spirit Temple Wallmaster Statue",
      "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 188 },
    { "OOT_WALLMASTER_BOTW_MAIN", Game::Oot, 0xf17u, Type::Wallmaster, "", "OOT Bottom of the Well Wallmaster Main",
      "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 189 },
    { "OOT_WALLMASTER_BOTW_BASEMENT", Game::Oot, 0xf18u, Type::Wallmaster, "",
      "OOT Bottom of the Well Wallmaster Basement", "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 190 },
    { "OOT_WALLMASTER_BOTW_PIT", Game::Oot, 0xf19u, Type::Wallmaster, "", "OOT Bottom of the Well Wallmaster Pit",
      "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 191 },
    { "OOT_WALLMASTER_GTG", Game::Oot, 0xf1au, Type::Wallmaster, "", "OOT Gerudo Training Grounds Wallmaster",
      "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 192 },
    { "OOT_WALLMASTER_GANON_SPIRIT", Game::Oot, 0xf1bu, Type::Wallmaster, "", "OOT Ganon Castle Spirit Wallmaster",
      "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 193 },
    { "OOT_WALLMASTER_GANON_LIGHT", Game::Oot, 0xf1cu, Type::Wallmaster, "", "OOT Ganon Castle Light Wallmaster",
      "OOT VOID", "NONE", "NONE", "NONE", "NONE", false, 194 },
    { "OOT_MARKET_FROM_MASK_SHOP", Game::Oot, 0x1d1u, Type::IndoorsLink, "OOT_SHOP_MASKS", "OOT Market Mask Shop",
      "OOT Market", "NONE", "OOT_MARKET", "NONE", "NONE", false, 276 },
    { "OOT_SHOP_MASKS", Game::Oot, 0x530u, Type::None, "OOT_MARKET_FROM_MASK_SHOP", "OOT Market",
      "OOT Market Mask Shop", "OOT_MARKET", "NONE", "NONE", "NONE", false, 277 },
    { "MM_TEMPLE_WOODFALL", Game::Mm, 0x3000u, Type::Dungeon, "MM_WOODFALL_FROM_TEMPLE", "MM Woodfall Front of Temple",
      "MM Woodfall Temple", "MM_WOODFALL", "MM_TEMPLE_WOODFALL", "NONE", "NONE", true, 292 },
    { "MM_TEMPLE_SNOWHEAD", Game::Mm, 0x3c00u, Type::Dungeon, "MM_SNOWHEAD_FROM_TEMPLE", "MM Snowhead",
      "MM Snowhead Temple", "MM_SNOWHEAD", "MM_TEMPLE_SNOWHEAD", "NONE", "NONE", true, 293 },
    { "MM_TEMPLE_GREAT_BAY", Game::Mm, 0x8c00u, Type::Dungeon, "MM_GREAT_BAY_FROM_TEMPLE", "MM Zora Cape Peninsula",
      "MM Great Bay Temple", "MM_ZORA_CAPE", "MM_TEMPLE_GREAT_BAY", "NONE", "NONE", true, 294 },
    { "MM_TEMPLE_STONE_TOWER_INVERTED", Game::Mm, 0x2a00u, Type::Dungeon, "MM_STONE_TOWER_INVERTED_FROM_TEMPLE",
      "MM Stone Tower Top Inverted", "MM Stone Tower Temple Inverted", "MM_STONE_TOWER_INVERTED",
      "MM_TEMPLE_STONE_TOWER_INVERTED", "NONE", "NONE", true, 295 },
    { "MM_TEMPLE_STONE_TOWER", Game::Mm, 0x2600u, Type::Dungeon, "MM_STONE_TOWER_FROM_TEMPLE",
      "MM Stone Tower Front of Temple", "MM Stone Tower Temple", "MM_STONE_TOWER", "MM_TEMPLE_STONE_TOWER", "NONE",
      "NONE", true, 296 },
    { "MM_SPIDER_HOUSE_SWAMP", Game::Mm, 0x4800u, Type::DungeonSh, "MM_SWAMP_FROM_SPIDER_HOUSE",
      "MM Near Swamp Spider House", "MM Swamp Spider House", "MM_SWAMP", "MM_SPIDER_HOUSE_SWAMP", "NONE", "NONE", true,
      297 },
    { "MM_SPIDER_HOUSE_OCEAN", Game::Mm, 0x4a00u, Type::DungeonSh, "MM_GREAT_BAY_FROM_SPIDER_HOUSE",
      "MM Great Bay Coast", "MM Ocean Spider House", "MM_GREAT_BAY_COAST", "MM_SPIDER_HOUSE_OCEAN", "NONE", "NONE",
      true, 298 },
    { "MM_WOODFALL_FROM_TEMPLE", Game::Mm, 0x8610u, Type::DungeonExit, "MM_TEMPLE_WOODFALL", "MM Woodfall Temple",
      "MM Woodfall Front of Temple", "MM_TEMPLE_WOODFALL", "MM_WOODFALL", "NONE", "NONE", false, 305 },
    { "MM_SNOWHEAD_FROM_TEMPLE", Game::Mm, 0xb210u, Type::DungeonExit, "MM_TEMPLE_SNOWHEAD", "MM Snowhead Temple",
      "MM Snowhead", "MM_TEMPLE_SNOWHEAD", "MM_SNOWHEAD", "NONE", "NONE", false, 306 },
    { "MM_GREAT_BAY_FROM_TEMPLE", Game::Mm, 0x6a70u, Type::DungeonExit, "MM_TEMPLE_GREAT_BAY", "MM Great Bay Temple",
      "MM Zora Cape Peninsula", "MM_TEMPLE_GREAT_BAY", "MM_ZORA_CAPE", "NONE", "NONE", false, 307 },
    { "MM_STONE_TOWER_INVERTED_FROM_TEMPLE", Game::Mm, 0xac10u, Type::DungeonExit, "MM_TEMPLE_STONE_TOWER_INVERTED",
      "MM Stone Tower Temple Inverted", "MM Stone Tower Top Inverted", "MM_TEMPLE_STONE_TOWER_INVERTED",
      "MM_STONE_TOWER_INVERTED", "NONE", "NONE", false, 308 },
    { "MM_STONE_TOWER_FROM_TEMPLE", Game::Mm, 0xaa20u, Type::DungeonExit, "MM_TEMPLE_STONE_TOWER",
      "MM Stone Tower Temple", "MM Stone Tower Front of Temple", "MM_TEMPLE_STONE_TOWER", "MM_STONE_TOWER", "NONE",
      "NONE", false, 309 },
    { "MM_SWAMP_FROM_SPIDER_HOUSE", Game::Mm, 0x8480u, Type::DungeonExit, "MM_SPIDER_HOUSE_SWAMP",
      "MM Swamp Spider House", "MM Near Swamp Spider House", "MM_SPIDER_HOUSE_SWAMP", "MM_SWAMP", "NONE", "NONE",
      false, 310 },
    { "MM_GREAT_BAY_FROM_SPIDER_HOUSE", Game::Mm, 0x6880u, Type::DungeonExit, "MM_SPIDER_HOUSE_OCEAN",
      "MM Ocean Spider House", "MM Great Bay Coast", "MM_SPIDER_HOUSE_OCEAN", "MM_GREAT_BAY_COAST", "NONE", "NONE",
      false, 311 },
    { "MM_SWAMP_FROM_IKANA_CANYON", Game::Mm, 0x8490u, Type::OneWay, "", "MM Ikana Valley",
      "MM Swamp Near Potion Shop", "NONE", "NONE", "NONE", "NONE", false, 398 },
    { "MM_IKANA_CASTLE_INTERIOR_KEG", Game::Mm, 0x3440u, Type::OneWayIkana, "",
      "MM Ancient Castle of Ikana Roof Interior", "MM Ancient Castle of Ikana", "NONE", "NONE", "NONE", "NONE", false,
      399 },
    { "MM_IKANA_CASTLE_INTERIOR_BLOCK", Game::Mm, 0x3450u, Type::OneWayIkana, "",
      "MM Ancient Castle of Ikana Roof Interior", "MM Ancient Castle of Ikana Interior South", "NONE", "NONE", "NONE",
      "NONE", false, 400 },
    { "MM_WALLMASTER_DAMPE", Game::Mm, 0xff00u, Type::Wallmaster, "", "MM Beneath The Graveyard Night 3 Wallmaster",
      "MM VOID", "NONE", "NONE", "NONE", "NONE", false, 517 },
    { "MM_WALLMASTER_BTW_ENTRANCE", Game::Mm, 0xff01u, Type::Wallmaster, "",
      "MM Beneath The Well Wallmaster Near Entrance", "MM VOID", "NONE", "NONE", "NONE", "NONE", false, 518 },
    { "MM_WALLMASTER_BTW_FOUNTAIN", Game::Mm, 0xff02u, Type::Wallmaster, "",
      "MM Beneath The Well Wallmaster Near Fountain", "MM VOID", "NONE", "NONE", "NONE", "NONE", false, 519 },
    { "MM_WALLMASTER_BTW_EXIT", Game::Mm, 0xff03u, Type::Wallmaster, "", "MM Beneath The Well Wallmaster Near Exit",
      "MM VOID", "NONE", "NONE", "NONE", "NONE", false, 520 },
    { "MM_VOID_GREAT_BAY", Game::Mm, 0x6820u, Type::OneWayWaterVoid, "", "MM Great Bay Coast Water Void", "MM VOID",
      "NONE", "MM_GREAT_BAY_COAST", "NONE", "NONE", false, 587 },
    { "MM_VOID_GREAT_BAY_BY_PINNACLE_ROCK", Game::Mm, 0x6860u, Type::OneWayWaterVoid, "",
      "MM Great Bay Coast Near Pinnacle Water Void", "MM VOID", "NONE", "MM_GREAT_BAY_COAST", "NONE", "NONE", false,
      588 },
    { "MM_VOID_ZORA_CAPE", Game::Mm, 0x6a30u, Type::OneWayWaterVoid, "", "MM Zora Cape Water Void", "MM VOID", "NONE",
      "MM_ZORA_CAPE", "NONE", "NONE", false, 589 },
    { "MM_VOID_PINNACLE_ROCK", Game::Mm, 0x4410u, Type::OneWayWaterVoid, "", "MM Pinnacle Rock Water Void", "MM VOID",
      "NONE", "MM_PINNACLE_ROCK", "NONE", "NONE", false, 590 },
    { "MM_CLOCK_TOWN_FROM_CLOCK_TOWER", Game::Mm, 0xd800u, Type::IndoorsLink, "MM_CLOCK_TOWER_FROM_CLOCK_TOWN",
      "MM Clock Tower", "MM Clock Town From Clock Tower", "NONE", "MM_CLOCK_TOWN", "NONE", "NONE", true, 594 },
    { "MM_CLOCK_TOWER_FROM_CLOCK_TOWN", Game::Mm, 0xc010u, Type::None, "MM_CLOCK_TOWN_FROM_CLOCK_TOWER",
      "MM Clock Town Near Clock Tower", "MM Clock Tower", "MM_CLOCK_TOWN", "NONE", "NONE", "NONE", false, 595 },
};
// clang-format on

// Registry areas that only one quest defines; every other area of a row above exists in both.
struct QuestArea {
    std::string_view dungeon;
    bool mq;
    std::string_view area;
};
inline constexpr QuestArea kQuestOnlyAreas[] = {
    { "Shadow", false, "OOT Shadow Temple Wallmaster" },              // dungeons/shadow_temple.yml:8
    { "Spirit", false, "OOT Spirit Temple Wallmaster Child Rupees" }, // dungeons/spirit_temple.yml:18
    { "Spirit", true, "OOT Spirit Temple Wallmaster Child Sun" },     // dungeons_mq/spirit_temple_mq.yml:32
    { "Spirit", true, "OOT Spirit Temple Wallmaster Statue" },        // dungeons_mq/spirit_temple_mq.yml:42
    { "BotW", true, "OOT Bottom of the Well Wallmaster Basement" },   // dungeons_mq/bottom_of_the_well_mq.yml:19
    { "BotW", true, "OOT Bottom of the Well Wallmaster Pit" },        // dungeons_mq/bottom_of_the_well_mq.yml:24
    { "Ganon", false, "OOT Ganon Castle Light Wallmaster" },          // dungeons/ganon_castle.yml:19
};
// connectGamesDefault for games=ootmm (entrances.ts:888-903): implicit, never stored.
inline constexpr std::array<std::array<std::string_view, 2>, 2> kGameLinks = { {
    { "OOT_SHOP_MASKS", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER" },
    { "MM_CLOCK_TOWER_FROM_CLOCK_TOWN", "OOT_MARKET_FROM_MASK_SHOP" },
} };
// The settings v1 covers; every other key stays at its default.
inline constexpr std::string_view kKeys[] = {
    "erWallmasters", "erOneWays",           "erOneWaysMajor",    "erOneWaysIkana",
    "erOneWaysOwls", "erOneWaysWaterVoids", "erOneWaysAnywhere",
};

constexpr bool RowsAreUnique() {
    for (size_t i = 0; i < std::size(kRows); ++i)
        for (size_t j = 0; j < i; ++j)
            if (kRows[i].name == kRows[j].name || (kRows[i].game == kRows[j].game && kRows[i].id == kRows[j].id))
                return false;
    return true;
}
static_assert(RowsAreUnique(), "A v1 row name or (game, id) is ambiguous");

inline const Record* FindRow(std::string_view name) {
    for (const auto& row : kRows)
        if (row.name == name)
            return &row;
    return nullptr;
}
// computeAllEntrances (entrances.ts:179-201) for the quests a layout records.
inline bool AreaExists(std::string_view area, const OotQuests& quests, bool mmTranche) {
    if (area == "NONE")
        return true;
    if (mmTranche)
        for (const auto& row : kRows)
            if (row.game == Game::Oot && row.type == Type::Wallmaster && row.from == area)
                return false;
    for (const auto& quest : kQuestOnlyAreas)
        if (quest.area == area)
            return quests[OotMqIndex(quest.dungeon)] == quest.mq;
    return true;
}
// POLARITY_IN / POLARITY_OUT (entrances.ts:50-80) for the v1 types, with erOverworld and
// erNoPolarity off (entrances.ts:139-163).
inline Polarity RowPolarity(const Record& e) {
    switch (e.type) {
        case Type::Dungeon:
        case Type::DungeonMinor:
        case Type::DungeonSh:
        case Type::Wallmaster:
            return Polarity::In;
        case Type::DungeonExit:
            return Polarity::Out;
        default:
            return Polarity::Any;
    }
}
// isSelfLoop (entrances.ts:459-482).
inline bool RowSelfLoop(const Record& src, const Record& dst) {
    if (src.fromMap == "NONE" || dst.toMap != src.fromMap)
        return false;
    if (src.fromMap != src.toMap || dst.fromMap != dst.toMap)
        return true;
    return src.fromSubmap == dst.toSubmap;
}
// isAssignableConstraints (entrances.ts:484-498) with erSelfLoops off.
inline bool RowAssignable(const Record& src, const Record& dst, bool ownGame) {
    if (ownGame && src.game != dst.game)
        return false;
    const auto a = RowPolarity(src), b = RowPolarity(dst);
    if (a != Polarity::Any && b != Polarity::Any && a != b)
        return false;
    return !RowSelfLoop(src, dst);
}
// makePools (entrances.ts:657-669, 742-886) and placePools' pool construction
// (entrances.ts:567-589, 601-626) for the settings v1 covers.
inline Pools MakePools(const Settings& s, const OotQuests& quests, bool mmTranche) {
    for (const auto& row : kEntranceSettings)
        if (row.get(s) != row.defaultValue && std::find(std::begin(kKeys), std::end(kKeys), row.key) == std::end(kKeys))
            throw EntranceLayoutError(std::string("Layout v1 does not cover ") + row.key);
    if (s.games != Games::Both || s.erWarps != Warps::None || s.erOneWaysSongs || s.erOneWaysStatues ||
        s.erOneWaysWoods || s.erWallmasters == Mode::Full || s.erOneWays == Mode::Full)
        throw EntranceLayoutError("Layout v1 covers own-game Wallmaster and One-Way shuffles only");
    // poolOneWays (entrances.ts:758-784).
    Types oneWays;
    if (s.erOneWaysMajor)
        oneWays.insert(Type::OneWay);
    if (s.erOneWaysIkana)
        oneWays.insert(Type::OneWayIkana);
    if (s.erOneWaysOwls)
        oneWays.insert(Type::OneWayOwl);
    if (s.erOneWaysWaterVoids)
        oneWays.insert(Type::OneWayWaterVoid);
    const bool permutation = s.erOneWays != Mode::None && !s.erOneWaysAnywhere;
    std::vector<PoolDescription> descriptions;
    if (permutation) // makePoolsSimple (entrances.ts:791-861)
        descriptions.push_back({ "ONE_WAYS", oneWays, oneWays, true, false });
    // poolsTypesDst (entrances.ts:881-886) plus the dungeon types.
    Types extended = permutation ? oneWays : Types{};
    extended.insert({ Type::Dungeon, Type::DungeonMinor, Type::DungeonSh });
    if (s.erWallmasters != Mode::None) // poolWallmasters (entrances.ts:657-669)
        descriptions.push_back({ "WALLMASTERS", { Type::Wallmaster }, extended, true, true });
    if (s.erOneWays != Mode::None && s.erOneWaysAnywhere) { // poolOneWaysAnywhere (entrances.ts:742-756)
        auto dst = extended;
        dst.insert(oneWays.begin(), oneWays.end());
        descriptions.push_back({ "ONE_WAYS", oneWays, std::move(dst), true, true });
    }
    Names available;
    for (const auto& row : kRows)
        if (AreaExists(row.from, quests, mmTranche) && AreaExists(row.to, quests, mmTranche))
            available.insert(row.name);
    const auto forTypes = [&](const Types& types) {
        Names result;
        for (const auto name : available) {
            const auto& row = *FindRow(name);
            if (!types.contains(row.type))
                continue;
            result.insert(name);
            if (!row.reverse.empty()) {
                if (!available.contains(row.reverse))
                    throw EntranceLayoutError("Missing reverse area for " + std::string(name));
                result.insert(row.reverse);
            }
        }
        return result;
    };
    Pools pools;
    for (const auto& d : descriptions) {
        Pool p{ d.name, forTypes(d.src), forTypes(d.dst), d.ownGame, d.alias };
        for (const auto& [src, dst] : kGameLinks) { // used by connectGamesDefault (entrances.ts:628-635)
            p.src.erase(src);
            if (!p.alias)
                p.dst.erase(dst);
        }
        for (const auto& [src, dst] : kGameLinks) { // the game-link fixup (entrances.ts:616-620)
            p.src.erase(dst);
            p.dst.erase(src);
        }
        if (p.src.empty()) {
            if (!p.dst.empty() && !p.alias)
                throw EntranceLayoutError("Unbalanced pool " + p.name);
            continue;
        }
        if (p.dst.empty())
            throw EntranceLayoutError("No destinations for pool " + p.name);
        pools.push_back(std::move(p));
    }
    return pools;
}
} // namespace LayoutV1

// Frozen copy of the pinned rules for every layout v2 capability id (limits design, Stage 3): Full
// Wallmaster Shuffle alone, on LayoutV1's frozen rows, area rules, game links and assignability at
// the same pin. Never edit it to follow Shuffle.h or Registry.h, nor LayoutV1: a new rule set gets
// a new copy and new capability ids.
namespace LayoutV2 {
inline constexpr std::string_view kUpstreamCommit = LayoutV1::kUpstreamCommit;
inline constexpr std::string_view kRegistrySha256 = LayoutV1::kRegistrySha256;
// The settings v2 covers; every other key stays at its default.
inline constexpr std::string_view kKeys[] = { "erWallmasters" };

// makePools (entrances.ts:863-879) for erWallmasters=full alone: no simple pool, so poolsTypesDst is
// empty and poolWallmasters (:657-669) gives the dungeon types with ownGame false (:668). The pool's
// members follow placePools (:567-589) and poolEntrancesForTypes (:601-626), as LayoutV1's do.
inline Pools MakePools(const Settings& s, const OotQuests& quests) {
    for (const auto& row : kEntranceSettings)
        if (row.get(s) != row.defaultValue && std::find(std::begin(kKeys), std::end(kKeys), row.key) == std::end(kKeys))
            throw EntranceLayoutError(std::string("Layout v2 does not cover ") + row.key);
    if (s.games != Games::Both || s.erWarps != Warps::None || s.erOneWaysSongs || s.erOneWaysStatues ||
        s.erOneWaysWoods || s.erWallmasters != Mode::Full)
        throw EntranceLayoutError("Layout v2 covers Full Wallmaster Shuffle only");
    const Types src{ Type::Wallmaster }, dst{ Type::Dungeon, Type::DungeonMinor, Type::DungeonSh };
    Names available;
    for (const auto& row : LayoutV1::kRows)
        if (LayoutV1::AreaExists(row.from, quests, false) && LayoutV1::AreaExists(row.to, quests, false))
            available.insert(row.name);
    const auto forTypes = [&](const Types& types) {
        Names result;
        for (const auto name : available) {
            const auto& row = *LayoutV1::FindRow(name);
            if (!types.contains(row.type))
                continue;
            result.insert(name);
            if (!row.reverse.empty()) {
                if (!available.contains(row.reverse))
                    throw EntranceLayoutError("Missing reverse area for " + std::string(name));
                result.insert(row.reverse);
            }
        }
        return result;
    };
    Pool p{ "WALLMASTERS", forTypes(src), forTypes(dst), false, true };
    for (const auto& [from, to] : LayoutV1::kGameLinks)   // used by connectGamesDefault (entrances.ts:628-635)
        p.src.erase(from);                                // an alias pool keeps its destinations
    for (const auto& [from, to] : LayoutV1::kGameLinks) { // the game-link fixup (entrances.ts:616-620)
        p.src.erase(to);
        p.dst.erase(from);
    }
    if (p.src.empty() || p.dst.empty())
        throw EntranceLayoutError("Layout v2 has an empty Wallmaster pool");
    return { std::move(p) };
}
} // namespace LayoutV2

// What a seed stores under "entranceLayout".
struct EntranceLayout {
    std::string upstreamCommit{ LayoutV1::kUpstreamCommit }, registrySha256{ LayoutV1::kRegistrySha256 };
    // "requires": the capability ids a build needs, sorted. One catalog row admits a whole
    // combination (Capabilities.h), so this is one id.
    std::vector<std::string> required;
    bool devBuild = false;
    Settings settings;
    OotQuests ootMq{};
    // Every placed route by registry name; none in a profile settings document, which keeps
    // the rest of the envelope (ComboRandomizerProfiles.h).
    std::optional<Overrides> overrides;
};
enum class LayoutPart : uint8_t { Seed, SettingsOnly };

inline bool SameEntranceSettings(const Settings& a, const Settings& b) {
    for (const auto& row : kEntranceSettings)
        if (row.get(a) != row.get(b))
            return false;
    return a.games == b.games && a.erWarps == b.erWarps && a.erOneWaysSongs == b.erOneWaysSongs &&
           a.erOneWaysStatues == b.erOneWaysStatues && a.erOneWaysWoods == b.erOneWaysWoods;
}

namespace LayoutDetail {
inline bool BooleanSetting(const EntranceSetting& row) {
    return row.choices.size() == 2 && row.choices[0].value == "false" && row.choices[1].value == "true";
}
// Exactly these keys: nothing unknown and nothing missing.
inline void RequireKeys(const nlohmann::json& value, const std::string& what,
                        std::initializer_list<std::string_view> keys) {
    if (!value.is_object())
        throw EntranceLayoutError("Entrance layout " + what + " must be an object");
    for (const auto& item : value.items())
        if (std::find(keys.begin(), keys.end(), item.key()) == keys.end())
            throw EntranceLayoutError("Entrance layout " + what + " has unknown key " + item.key());
    for (const auto key : keys)
        if (!value.contains(key))
            throw EntranceLayoutError("Entrance layout " + what + " is missing " + std::string(key));
}
inline std::string Text(const nlohmann::json& value, const std::string& what) {
    if (!value.is_string())
        throw EntranceLayoutError("Entrance layout " + what + " must be a string");
    return value.get<std::string>();
}
inline const Record& Row(const nlohmann::json& value, const std::string& what) {
    const auto name = Text(value, what);
    const auto* row = LayoutV1::FindRow(name);
    if (!row)
        throw EntranceLayoutError("Entrance layout " + what + " " + name + " is not a layout v1 entrance");
    return *row;
}
} // namespace LayoutDetail

// Strict decode: wrong types and unknown or missing keys are rejected. A JSON parser keeps
// the last of two equal object keys, so only repeated ids and sources can be seen here.
inline EntranceLayout DecodeEntranceLayout(const nlohmann::json& value, LayoutPart part = LayoutPart::Seed) {
    using LayoutDetail::RequireKeys, LayoutDetail::Text;
    if (part == LayoutPart::Seed)
        RequireKeys(
            value, "envelope",
            { "schema", "upstreamCommit", "registrySha256", "requires", "devBuild", "settings", "oot", "overrides" });
    else
        RequireKeys(value, "settings document",
                    { "schema", "upstreamCommit", "registrySha256", "requires", "devBuild", "settings", "oot" });
    if (Text(value.at("schema"), "schema") != kEntranceLayoutSchema)
        throw EntranceLayoutError("Unknown entrance layout schema " + value.at("schema").get<std::string>());
    EntranceLayout layout;
    layout.upstreamCommit = Text(value.at("upstreamCommit"), "upstreamCommit");
    layout.registrySha256 = Text(value.at("registrySha256"), "registrySha256");
    if (!value.at("requires").is_array())
        throw EntranceLayoutError("Entrance layout requires must be an array");
    for (const auto& id : value.at("requires"))
        layout.required.push_back(Text(id, "capability id"));
    if (!value.at("devBuild").is_boolean())
        throw EntranceLayoutError("Entrance layout devBuild must be a boolean");
    layout.devBuild = value.at("devBuild").get<bool>();

    // All 40 settings by OoTMM name, booleans as JSON booleans and the rest by value name.
    const auto& settings = value.at("settings");
    if (!settings.is_object())
        throw EntranceLayoutError("Entrance layout settings must be an object");
    for (const auto& item : settings.items())
        if (!FindEntranceSetting(item.key()))
            throw EntranceLayoutError("Entrance layout settings has unknown key " + item.key());
    for (const auto& row : kEntranceSettings) {
        if (!settings.contains(row.key))
            throw EntranceLayoutError(std::string("Entrance layout settings is missing ") + row.key);
        const auto& setting = settings.at(row.key);
        int index = -1;
        if (LayoutDetail::BooleanSetting(row)) {
            if (setting.is_boolean())
                index = setting.get<bool>() ? 1 : 0;
        } else if (setting.is_string()) {
            for (size_t i = 0; i < row.choices.size(); ++i)
                if (row.choices[i].value == setting.get<std::string>())
                    index = static_cast<int>(i);
        }
        if (index < 0)
            throw EntranceLayoutError(std::string("Entrance layout settings has an invalid ") + row.key);
        row.set(layout.settings, index);
    }

    RequireKeys(value.at("oot"), "oot", { "mq" });
    const auto& mq = value.at("oot").at("mq");
    if (!mq.is_object() || mq.size() != kOotMqDungeons.size())
        throw EntranceLayoutError("Entrance layout oot.mq must name the twelve Master Quest dungeons");
    for (size_t i = 0; i < kOotMqDungeons.size(); ++i) {
        const auto quest = mq.find(std::string(kOotMqDungeons[i]));
        if (quest == mq.end() || !quest->is_boolean())
            throw EntranceLayoutError("Entrance layout oot.mq needs a boolean for " + std::string(kOotMqDungeons[i]));
        layout.ootMq[i] = quest->get<bool>();
    }

    if (part == LayoutPart::Seed) {
        const auto& rows = value.at("overrides");
        if (!rows.is_array())
            throw EntranceLayoutError("Entrance layout overrides must be an array");
        Overrides overrides;
        for (const auto& row : rows) {
            RequireKeys(row, "route", { "source", "destination" });
            const auto& src = LayoutDetail::Row(row.at("source"), "source");
            const auto& dst = LayoutDetail::Row(row.at("destination"), "destination");
            if (!overrides.empty() && src.name <= overrides.rbegin()->first)
                throw EntranceLayoutError("Entrance layout routes must be sorted by a unique source");
            overrides[src.name] = dst.name;
        }
        layout.overrides = std::move(overrides);
    }
    return layout;
}

// The encoder generation uses (M1-07); Validate() a layout before writing it.
inline nlohmann::json EncodeEntranceLayout(const EntranceLayout& layout) {
    nlohmann::json settings = nlohmann::json::object();
    for (const auto& row : kEntranceSettings) {
        const int value = row.get(layout.settings);
        if (value < 0 || static_cast<size_t>(value) >= row.choices.size())
            throw EntranceLayoutError(std::string("Cannot encode entrance setting ") + row.key);
        if (LayoutDetail::BooleanSetting(row))
            settings[row.key] = value != 0;
        else
            settings[row.key] = std::string(row.choices[value].value);
    }
    nlohmann::json mq = nlohmann::json::object();
    for (size_t i = 0; i < kOotMqDungeons.size(); ++i)
        mq[std::string(kOotMqDungeons[i])] = layout.ootMq[i];
    nlohmann::json result = { { "schema", kEntranceLayoutSchema },
                              { "upstreamCommit", layout.upstreamCommit },
                              { "registrySha256", layout.registrySha256 },
                              { "requires", layout.required },
                              { "devBuild", layout.devBuild },
                              { "settings", std::move(settings) },
                              { "oot", { { "mq", std::move(mq) } } } };
    if (layout.overrides) {
        result["overrides"] = nlohmann::json::array();
        for (const auto& [src, dst] : *layout.overrides)
            result["overrides"].push_back({ { "source", src }, { "destination", dst } });
    }
    return result;
}

// Everything but the routes; returns the pools the routes must fill. The settings must be a
// fixed point of OoTMM's conditions and admitted by the required capabilities alone, and
// every required id must be needed.
inline Pools ValidateEntranceLayoutSettings(const EntranceLayout& layout, const CapabilitySet& caps) {
    if (layout.upstreamCommit != LayoutV1::kUpstreamCommit || layout.registrySha256 != LayoutV1::kRegistrySha256)
        throw EntranceLayoutError("Entrance layout is pinned to another OoTMM reference or registry");
    if (layout.required.empty())
        throw EntranceLayoutError("Entrance layout requires no capability");
    if (!std::is_sorted(layout.required.begin(), layout.required.end()) ||
        std::adjacent_find(layout.required.begin(), layout.required.end()) != layout.required.end())
        throw EntranceLayoutError("Entrance layout requires must be sorted and unique");
    CapabilitySet required;
    for (const auto& id : layout.required) {
        if (!CapabilitySet::Known(id))
            throw EntranceLayoutError("Entrance layout requires unknown capability " + id);
        if (!caps.Active(id))
            throw EntranceLayoutError("Entrance layout requires " + id + ", which this build does not have");
        required = required | caps.Only(id);
    }
    if (layout.devBuild && !caps.devBuild)
        throw EntranceLayoutError("Entrance layout comes from a dev build");
    Settings normalized = layout.settings;
    ValidateEntranceSettings(normalized);
    if (!SameEntranceSettings(normalized, layout.settings))
        throw EntranceLayoutError("Entrance layout settings break an OoTMM setting condition");
    if (!AdmitsEntranceSettings(layout.settings, required))
        throw EntranceLayoutError("Entrance layout settings are not admitted by its required capabilities");
    for (size_t i = 0; layout.required.size() > 1 && i < layout.required.size(); ++i) {
        CapabilitySet others;
        for (size_t j = 0; j < layout.required.size(); ++j)
            if (j != i)
                others = others | caps.Only(layout.required[j]);
        if (AdmitsEntranceSettings(layout.settings, others))
            throw EntranceLayoutError("Entrance layout requires " + layout.required[i] + " without using it");
    }
    const bool mmTranche =
        std::find(layout.required.begin(), layout.required.end(), kMmTrancheCapability) != layout.required.end();
    // A new capability contract, not a change to the frozen pool definitions or old profile semantics.
    const bool anyMq = std::any_of(layout.ootMq.begin(), layout.ootMq.end(), [](bool mq) { return mq; });
    if (std::find(layout.required.begin(), layout.required.end(), kVanillaWallmasterCapability) !=
            layout.required.end() &&
        anyMq)
        throw EntranceLayoutError("Wallmaster Shuffle currently supports vanilla OoT dungeons only");
    // The quests id is the vanilla id's rules for a context with a Master Quest dungeon, so every all-vanilla layout
    // keeps the one id builds before it read.
    if (std::find(layout.required.begin(), layout.required.end(), kQuestsWallmasterCapability) !=
            layout.required.end() &&
        !anyMq)
        throw EntranceLayoutError(std::string("Entrance layout requires ") + std::string(kQuestsWallmasterCapability) +
                                  " without a Master Quest dungeon");
    // The songs ids and the Full id take either quest context. Their song facts are the seed's, outside the layout:
    // ComboSeedFormat.h RequireSeedEntranceFacts checks them.
    const bool v2 =
        std::find(layout.required.begin(), layout.required.end(), kFullWallmasterCapability) != layout.required.end();
    auto pools = v2 ? LayoutV2::MakePools(layout.settings, layout.ootMq)
                    : LayoutV1::MakePools(layout.settings, layout.ootMq, mmTranche);
    if (pools.empty())
        throw EntranceLayoutError("Entrance layout settings place no entrance");
    return pools;
}

// A validated route, as the frozen v1 rows name it.
struct LayoutRoute {
    const Record* source = nullptr;
    const Record* destination = nullptr;
};
// The routes by source game, each sorted by source name. A route whose destination is in the other game (only
// LayoutV2's Full pool has them) is listed apart, by its source game, so a reader of own-game routes never meets one.
struct ValidatedLayout {
    std::vector<LayoutRoute> oot, mm;
    std::vector<LayoutRoute> ootToMm, mmToOot;
};
// A layout against this build's capabilities and the frozen v1 pools: every available
// source routed exactly once and nothing else, every destination in its pool and
// assignable, no destination reused inside a pool, a non-alias pool filled exactly, and no
// stored game link.
inline ValidatedLayout Validate(const EntranceLayout& layout, const CapabilitySet& caps) {
    const auto pools = ValidateEntranceLayoutSettings(layout, caps);
    if (!layout.overrides)
        throw EntranceLayoutError("Entrance layout has no routes");
    std::map<std::string_view, size_t> poolOf;
    for (size_t i = 0; i < pools.size(); ++i)
        for (const auto src : pools[i].src)
            if (!poolOf.emplace(src, i).second)
                throw EntranceLayoutError("Entrance layout source " + std::string(src) + " is in two pools");
    std::vector<Names> used(pools.size());
    ValidatedLayout result;
    for (const auto& [srcName, dstName] : *layout.overrides) {
        for (const auto& link : LayoutV1::kGameLinks)
            if (srcName == link[0])
                throw EntranceLayoutError("Entrance layout stores the implicit game link " + std::string(srcName));
        const auto pool = poolOf.find(srcName);
        if (pool == poolOf.end())
            throw EntranceLayoutError("Entrance layout routes " + std::string(srcName) + ", which no pool shuffles");
        const auto& p = pools[pool->second];
        const auto& src = *LayoutV1::FindRow(srcName);
        // Only pool rows are looked up: a layout built in code may name a row outside v1.
        const auto* dst = p.dst.contains(dstName) ? LayoutV1::FindRow(dstName) : nullptr;
        if (!dst || !LayoutV1::RowAssignable(src, *dst, p.ownGame))
            throw EntranceLayoutError("Entrance layout cannot route " + std::string(srcName) + " to " +
                                      std::string(dstName));
        if (!used[pool->second].insert(dstName).second)
            throw EntranceLayoutError("Entrance layout uses " + std::string(dstName) + " twice in " + p.name);
        if (src.game == dst->game)
            (src.game == Game::Oot ? result.oot : result.mm).push_back({ &src, dst });
        else
            (src.game == Game::Oot ? result.ootToMm : result.mmToOot).push_back({ &src, dst });
    }
    for (size_t i = 0; i < pools.size(); ++i) {
        for (const auto src : pools[i].src)
            if (!layout.overrides->contains(src))
                throw EntranceLayoutError("Entrance layout has no route for " + std::string(src));
        if (!pools[i].alias && used[i] != pools[i].dst)
            throw EntranceLayoutError("Entrance layout does not fill " + pools[i].name);
    }
    return result;
}
} // namespace ComboRando::Entrances
