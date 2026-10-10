// OoTMM v32.3 refill events, adapted to Ship's region IDs and supported items.
// Reference commit: 413573d22d1d53e8a81931a70927f625b73488b9.
// See ComboAndroid/oot-resource-sources.csv for each source and adapter boundary.
#pragma once

#include "soh/Enhancements/randomizer/location_access.h"

inline std::vector<EventAccess> OotGrottoResourceEvents() {
    return {
        EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, logic->CanCutShrubs()),
        EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->CanCutShrubs()),
        EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanCutShrubs()),
        EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, logic->IsAdult && logic->CanCutShrubs()),
        EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanCutShrubs()),
    };
}

inline void RegisterOotResourceSources() {
    // Fire Temple Pre-Boss
    areaTable[RR_FIRE_TEMPLE_NEAR_BOSS_ROOM].events.insert(
        areaTable[RR_FIRE_TEMPLE_NEAR_BOSS_ROOM].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (logic->CanUse(RG_GORON_TUNIC)) && logic->CanBreakPots()),
        });
    // Gerudo Fortress Carpenter 1 Left
    areaTable[RR_TH_1_TORCH_CELL].events.insert(
        areaTable[RR_TH_1_TORCH_CELL].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && logic->CanBreakPots()),
        });
    // Gerudo Fortress Carpenter 2 Bottom
    areaTable[RR_TH_DOUBLE_CELL].events.insert(areaTable[RR_TH_DOUBLE_CELL].events.end(),
                                               {
                                                   EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                               });
    // Gerudo Fortress Carpenter 4 Top
    areaTable[RR_TH_STEEP_SLOPE_CELL].events.insert(areaTable[RR_TH_STEEP_SLOPE_CELL].events.end(),
                                                    {
                                                        EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                                    });
    // Gerudo Fortress Break Room Bottom
    areaTable[RR_TH_BREAK_ROOM].events.insert(
        areaTable[RR_TH_BREAK_ROOM].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (logic->CanUse(RG_FAIRY_BOW) || logic->CanUse(RG_HOOKSHOT) ||
                                                 logic->HasItem(RG_GERUDO_MEMBERSHIP_CARD)) &&
                                                    logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult && (logic->HasItem(RG_GERUDO_MEMBERSHIP_CARD) ||
                                                                     logic->CanUse(RG_HOOKSHOT))) &&
                                                     logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS,
                         (logic->IsChild && logic->HasItem(RG_GERUDO_MEMBERSHIP_CARD)) && logic->CanBreakPots()),
        });
    // Shadow Temple Pit
    areaTable[RR_SHADOW_TEMPLE_LOWER_HUGE_PIT].events.insert(
        areaTable[RR_SHADOW_TEMPLE_LOWER_HUGE_PIT].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (ctx->GetTrickOption(RT_LENS_SHADOW)) && logic->CanBreakPots()),
        });
    // Spirit Temple
    areaTable[RR_SPIRIT_TEMPLE_FOYER].events.insert(
        areaTable[RR_SPIRIT_TEMPLE_FOYER].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && logic->CanBreakPots()),
        });
    // Shadow Temple Truth Spinner
    areaTable[RR_SHADOW_TEMPLE_MQ_SPINNER_ROOM].events.insert(
        areaTable[RR_SHADOW_TEMPLE_MQ_SPINNER_ROOM].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (true) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (true) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && logic->CanBreakPots()),
        });
    // Spirit Temple
    areaTable[RR_SPIRIT_TEMPLE_MQ_FOYER].events.insert(
        areaTable[RR_SPIRIT_TEMPLE_MQ_FOYER].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (true) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && logic->CanBreakPots()),
        });
    // Death Mountain Crater Top
    areaTable[RR_DMC_POTS].events.insert(
        areaTable[RR_DMC_POTS].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU,
                         (logic->CanUse(RG_GORON_TUNIC) || logic->CanUse(RG_MEGATON_HAMMER)) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->CanUse(RG_GORON_TUNIC) || logic->BlastOrSmash()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS,
                         (logic->CanUse(RG_GORON_TUNIC) || logic->BlastOrSmash()) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC,
                         (logic->CanUse(RG_GORON_TUNIC) || logic->BlastOrSmash()) && logic->CanBreakPots()),
        });
    // Fairy Fountain Double Magic
    areaTable[RR_DMC_GREAT_FAIRY_FOUNTAIN].events.insert(
        areaTable[RR_DMC_GREAT_FAIRY_FOUNTAIN].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanUse(RG_ZELDAS_LULLABY)),
        });
    // Death Mountain
    areaTable[RR_DEATH_MOUNTAIN_TRAIL].events.insert(areaTable[RR_DEATH_MOUNTAIN_TRAIL].events.end(),
                                                     {
                                                         EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                                     });
    // Death Mountain Cow Grotto
    areaTable[RR_DMT_COW_GROTTO].events.insert(areaTable[RR_DMT_COW_GROTTO].events.end(),
                                               {
                                                   EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                               });
    // Fairy Fountain Magic
    areaTable[RR_DMT_GREAT_FAIRY_FOUNTAIN].events.insert(
        areaTable[RR_DMT_GREAT_FAIRY_FOUNTAIN].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanUse(RG_ZELDAS_LULLABY)),
        });
    // Fairy Fountain Nayru
    areaTable[RR_COLOSSUS_GREAT_FAIRY_FOUNTAIN].events.insert(
        areaTable[RR_COLOSSUS_GREAT_FAIRY_FOUNTAIN].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanUse(RG_ZELDAS_LULLABY)),
        });
    // Gerudo Valley
    areaTable[RR_GERUDO_VALLEY].events.insert(
        areaTable[RR_GERUDO_VALLEY].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (logic->IsChild) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (logic->IsChild) && logic->CanBreakPots()),
        });
    // Octorok Grotto
    areaTable[RR_GV_OCTOROK_GROTTO].events.insert(areaTable[RR_GV_OCTOROK_GROTTO].events.end(),
                                                  {
                                                      EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                                  });
    // Goron City
    areaTable[RR_GORON_CITY].events.insert(
        areaTable[RR_GORON_CITY].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult) && logic->CanBreakPots()),
        });
    // Darunia Chamber
    areaTable[RR_GC_DARUNIAS_CHAMBER].events.insert(areaTable[RR_GC_DARUNIAS_CHAMBER].events.end(),
                                                    {
                                                        EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->IsAdult),
                                                    });
    // Graveyard
    areaTable[RR_THE_GRAVEYARD].events.insert(
        areaTable[RR_THE_GRAVEYARD].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, logic->IsAdult && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanCutShrubs()),
        });
    // Dampe Grave
    areaTable[RR_GRAVEYARD_DAMPES_GRAVE].events.insert(
        areaTable[RR_GRAVEYARD_DAMPES_GRAVE].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (true) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult) && logic->CanBreakPots()),
        });
    // Hyrule Field
    areaTable[RR_HYRULE_FIELD].events.insert(
        areaTable[RR_HYRULE_FIELD].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
        });
    // Hyrule Field Grotto Near Kak
    areaTable[RR_HF_NEAR_KAK_GROTTO].events.insert(
        areaTable[RR_HF_NEAR_KAK_GROTTO].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanUse(RG_KOKIRI_SWORD)),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, logic->IsAdult && logic->CanJumpslashExceptHammer()),
        });
    // Hyrule Castle
    areaTable[RR_HC_PAST_GATE].events.insert(
        areaTable[RR_HC_PAST_GATE].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
        });
    // Fairy Fountain Din
    areaTable[RR_HC_GREAT_FAIRY_FOUNTAIN].events.insert(
        areaTable[RR_HC_GREAT_FAIRY_FOUNTAIN].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanUse(RG_ZELDAS_LULLABY)),
        });
    // Hyrule Castle Grotto
    areaTable[RR_HC_STORMS_GROTTO].events.insert(
        areaTable[RR_HC_STORMS_GROTTO].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->BlastOrSmash()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild && logic->BlastOrSmash()) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult && logic->BlastOrSmash()) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (logic->CanUse(RG_MEGATON_HAMMER)) && logic->CanBreakPots()),
        });
    // Fairy Fountain Defense
    areaTable[RR_OGC_GREAT_FAIRY_FOUNTAIN].events.insert(
        areaTable[RR_OGC_GREAT_FAIRY_FOUNTAIN].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanUse(RG_ZELDAS_LULLABY)),
        });
    // Kakariko
    areaTable[RR_KAKARIKO_VILLAGE].events.insert(
        areaTable[RR_KAKARIKO_VILLAGE].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult) && logic->CanBreakPots()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanCutShrubs()),
        });
    // Kokiri Forest
    areaTable[RR_KOKIRI_FOREST].events.insert(
        areaTable[RR_KOKIRI_FOREST].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
        });
    // Lake Hylia
    areaTable[RR_LAKE_HYLIA].events.insert(
        areaTable[RR_LAKE_HYLIA].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, logic->IsAdult && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanCutShrubs()),
        });
    // Lon Lon Ranch
    areaTable[RR_LON_LON_RANCH].events.insert(areaTable[RR_LON_LON_RANCH].events.end(),
                                              {
                                                  EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->IsChild),
                                              });
    // Lost Woods
    areaTable[RR_THE_LOST_WOODS].events.insert(
        areaTable[RR_THE_LOST_WOODS].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, logic->IsAdult && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanCutShrubs()),
        });
    // Lost Woods Deep
    areaTable[RR_LW_BEYOND_MIDO].events.insert(
        areaTable[RR_LW_BEYOND_MIDO].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, logic->IsAdult && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanCutShrubs()),
        });
    // Market
    areaTable[RR_THE_MARKET].events.insert(areaTable[RR_THE_MARKET].events.end(),
                                           {
                                               EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->IsChild),
                                           });
    // Market Pot House
    areaTable[RR_MARKET_GUARD_HOUSE].events.insert(areaTable[RR_MARKET_GUARD_HOUSE].events.end(),
                                                   {
                                                       EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                                   });
    // Bombchu Bowling Rewards
    areaTable[RR_MARKET_BOMBCHU_BOWLING].events.insert(
        areaTable[RR_MARKET_BOMBCHU_BOWLING].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS, logic->Get(LOGIC_COULD_PLAY_BOWLING) && logic->BombchusEnabled() &&
                                                    GetWalletCapacity() >= 200),
        });
    // Sacred Meadow Entryway
    areaTable[RR_SFM_ENTRYWAY].events.insert(areaTable[RR_SFM_ENTRYWAY].events.end(),
                                             {
                                                 EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                             });
    // Zora Domain
    areaTable[RR_ZORAS_DOMAIN].events.insert(areaTable[RR_ZORAS_DOMAIN].events.end(),
                                             {
                                                 EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
                                             });
    // Zora Fountain
    areaTable[RR_ZORAS_FOUNTAIN].events.insert(
        areaTable[RR_ZORAS_FOUNTAIN].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && logic->CanBreakPots()),
        });
    // Fairy Fountain Farore
    areaTable[RR_ZF_GREAT_FAIRY_FOUNTAIN].events.insert(
        areaTable[RR_ZF_GREAT_FAIRY_FOUNTAIN].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanUse(RG_ZELDAS_LULLABY)),
        });
    // Zora River Front
    areaTable[RR_ZR_FRONT].events.insert(
        areaTable[RR_ZR_FRONT].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, logic->IsChild && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, logic->IsAdult && logic->CanCutShrubs()),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, logic->CanCutShrubs()),
        });
    // Zora River
    areaTable[RR_ZORAS_RIVER].events.insert(
        areaTable[RR_ZORAS_RIVER].events.end(),
        {
            EVENT_ACCESS(LOGIC_RENEWABLE_BOMBS_OR_BOMBCHU, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_RUPEES, true),
            EVENT_ACCESS(LOGIC_RENEWABLE_SEEDS, (logic->IsChild) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_ARROWS, (logic->IsAdult) && (logic->CanBreakPots() || logic->CanCutShrubs())),
            EVENT_ACCESS(LOGIC_RENEWABLE_MAGIC, (true) && (logic->CanBreakPots() || logic->CanCutShrubs())),
        });
}
