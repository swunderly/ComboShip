// Exact pinned outside contexts; enabled only by the typed saved grotto layout.
inline bool TypedMmGrottoContextsActive() {
    return ComboRando::Entrances::ActiveMmMixed().active() ||
           ComboRando::Entrances::ActiveMmMixedAssumptions().has_value() ||
           ComboRando::Entrances::ActiveMmGrottos().active() ||
           ComboRando::Entrances::ActiveMmGrottoAssumptions().has_value();
}
inline bool TypedMmGrottoSourceActive(uint32_t id) {
    return ComboRando::Entrances::MmMixedGraphOverrides(id) ||
           ComboRando::Entrances::ActiveMmMixedAssumptions().has_value() ||
           ComboRando::Entrances::ActiveMmMixed().active() ||
           ComboRando::Entrances::MmGrottoGraphOverrides({ ComboRando::Entrances::Game::Mm, id }) ||
           ComboRando::Entrances::ActiveMmOrdinary().active() ||
           ComboRando::Entrances::ActiveMmOrdinaryAssumptions().has_value();
}
inline bool NativeMmOverworldShuffled() {
    for (const auto& record : ComboRando::Entrances::kRegistry)
        if (record.game == ComboRando::Entrances::Game::Mm && record.type == ComboRando::Entrances::Type::Overworld) {
            if (ComboRando::Entrances::MmMixedGraphOverrides(record.id))
                return true;
            if (ComboRando::Entrances::ActiveMmOrdinary().containsSource(record.id))
                return true;
            if (const auto& assumed = ComboRando::Entrances::ActiveMmOrdinaryAssumptions();
                assumed && assumed->routes.contains(record.id))
                return true;
        }
    return false;
}
#include "MmSwampContexts.inc.h"
#include "MmRawGrottoContexts.inc.h"
inline void EnsureMmGrottoContextRegions() {
    EnsureMmRawGrottoContexts();
    EnsureMmSwampContexts();
    if (Regions.contains(RR_MM_GROTTO_WOODS_BANK))
        return;
    auto legacyCheck = [&](RandoRegionId source, RandoCheckId check, uint32_t key) {
        auto& slot = Regions.at(source).checks.at(check);
        auto original = slot.first;
        slot.first = [original, key] { return !TypedMmGrottoSourceActive(key) && original(); };
    };
    auto route = [&](RandoRegionId source, RandoRegionId bank, s32 entry, std::function<bool()> gate, uint32_t key) {
        auto original = Regions.at(source).exits.at(entry);
        auto old = original.condition;
        Regions.at(source).exits.at(entry).condition = [old, key] { return !TypedMmGrottoSourceActive(key) && old(); };
        original.condition = [gate, key] { return TypedMmGrottoSourceActive(key) && gate(); };
        Regions.at(bank).exits.emplace(entry, std::move(original));
    };
    Regions[RR_MM_GROTTO_WOODS_BANK] =
        RandoRegion{ .name = "Near Woods of Mystery Grotto",
                     .sceneId = SCENE_26SARUNOMORI,
                     .checks = { CHECK(RC_WOODS_OF_MYSTERY_GRASS_21, TypedMmGrottoSourceActive(65539u)) },
                     .connections = {
                         CONNECTION(RR_WOODS_OF_MYSTERY, TypedMmGrottoSourceActive(65539u) && SECOND_DAY()),
                         // Lost Woods -> Swamp Near Potion Shop is unconditional at the pin.
                         CONNECTION(RR_SOUTHERN_SWAMP_NEAR_WOODS, TypedMmGrottoSourceActive(65539u)),
                     } };
    legacyCheck(RR_WOODS_OF_MYSTERY, RC_WOODS_OF_MYSTERY_GRASS_21, 65539u);
    Regions.at(RR_WOODS_OF_MYSTERY)
        .connections.insert(CONNECTION(RR_MM_GROTTO_WOODS_BANK, TypedMmGrottoSourceActive(65539u) && SECOND_DAY()));
    route(RR_WOODS_OF_MYSTERY, RR_MM_GROTTO_WOODS_BANK, ENTRANCE(GROTTOS, 22), [] { return true; }, 65539u);
    Regions[RR_MM_GROTTO_TWIN_RAMP]=RandoRegion{.name="Near Ramp Grotto",.sceneId=SCENE_17SETUGEN,
  .checks={
   CHECK(RC_TWIN_ISLANDS_SMALL_SNOWBALL_04,TypedMmGrottoSourceActive(65542u)&&NativeMmNorthWinter()),
   CHECK(RC_TWIN_ISLANDS_SMALL_SNOWBALL_05,TypedMmGrottoSourceActive(65542u)&&NativeMmNorthWinter()),
   CHECK(RC_TWIN_ISLANDS_SMALL_SNOWBALL_06,TypedMmGrottoSourceActive(65542u)&&NativeMmNorthWinter()),
   CHECK(RC_TWIN_ISLANDS_TREE_03,TypedMmGrottoSourceActive(65542u)&&NativeMmNorthWinter()),
   CHECK(RC_TWIN_ISLANDS_SPRING_TREE_03,TypedMmGrottoSourceActive(65542u)&&NativeMmNorthSpring()),
  },
  .connections={CONNECTION(RR_PATH_TO_GORON_VILLAGE,TypedMmGrottoSourceActive(65542u))},
 };
    for (auto check : { RC_TWIN_ISLANDS_SMALL_SNOWBALL_04, RC_TWIN_ISLANDS_SMALL_SNOWBALL_05,
                        RC_TWIN_ISLANDS_SMALL_SNOWBALL_06, RC_TWIN_ISLANDS_TREE_03, RC_TWIN_ISLANDS_SPRING_TREE_03 })
        legacyCheck(RR_PATH_TO_GORON_VILLAGE, check, 65542u);
    Regions.at(RR_PATH_TO_GORON_VILLAGE)
        .connections.insert(CONNECTION(RR_MM_GROTTO_TWIN_RAMP, TypedMmGrottoSourceActive(65542u) && CAN_BE_GORON));
    route(
        RR_PATH_TO_GORON_VILLAGE, RR_MM_GROTTO_TWIN_RAMP, ENTRANCE(GROTTOS, 24), [] { return CAN_USE_EXPLOSIVE; },
        65542u);
    Regions[RR_MM_TWIN_NEAR_RACE] =
        RandoRegion{ .name = "Near Goron Race",
                     .sceneId = SCENE_17SETUGEN,
                     .connections = { CONNECTION(RR_PATH_TO_GORON_VILLAGE, TypedMmGrottoSourceActive(65542u)),
                                      CONNECTION(RR_MM_GROTTO_TWIN_RAMP, TypedMmGrottoSourceActive(65542u)) },
                     .events = { EVENT(RE_EXPANDED_TRIAL_BOULDER,
                                       TypedMmGrottoSourceActive(65542u) &&
                                           (CAN_USE_POWDER_KEG || (!NativeMmOverworldShuffled() &&
                                                                   RANDO_EVENTS[RE_EXPANDED_POWDER_KEG_TRIAL]))) } };
    Regions.at(RR_PATH_TO_GORON_VILLAGE)
        .connections.insert(CONNECTION(RR_MM_TWIN_NEAR_RACE,
                                       TypedMmGrottoSourceActive(65542u) && (CAN_BE_GORON || CAN_HOOK_SCARECROW)));
    for (auto& event : Regions.at(RR_PATH_TO_GORON_VILLAGE).events)
        if (event.first == RE_EXPANDED_TRIAL_BOULDER) {
            auto legacy = event.second;
            event.second = [legacy] { return !TypedMmGrottoSourceActive(65542u) && legacy(); };
        }
    route(
        RR_PATH_TO_GORON_VILLAGE, RR_MM_TWIN_NEAR_RACE, ENTRANCE(GORON_RACETRACK, 0),
        [] { return RANDO_EVENTS[RE_EXPANDED_TRIAL_BOULDER]; }, 65542u);
    Regions[RR_MM_GROTTO_IKANA_SHRINE_BANK] = RandoRegion{
        .name = "Ikana Valley Near Secret Shrine",
        .sceneId = SCENE_IKANA,
        .connections = { CONNECTION(RR_IKANA_CANYON_LOWER, TypedMmGrottoSourceActive(65548u) &&
                                                               ((CAN_USE_ABILITY(SWIM) || CAN_BE_ZORA) ||
                                                                (CAN_USE_MAGIC_ARROW(ICE) && CAN_SUPPLY_CHATEAU))) }
    };
    Regions.at(RR_IKANA_CANYON_LOWER)
        .connections.insert(
            CONNECTION(RR_MM_GROTTO_IKANA_SHRINE_BANK,
                       TypedMmGrottoSourceActive(65548u) && ((CAN_USE_ABILITY(SWIM) || CAN_BE_ZORA) ||
                                                             (CAN_USE_MAGIC_ARROW(ICE) && CAN_SUPPLY_CHATEAU))));
    route(RR_IKANA_CANYON_LOWER, RR_MM_GROTTO_IKANA_SHRINE_BANK, ENTRANCE(GROTTOS, 29), [] { return true; }, 65548u);
    route(
        RR_IKANA_CANYON_LOWER, RR_MM_GROTTO_IKANA_SHRINE_BANK, ENTRANCE(SECRET_SHRINE, 0), [] { return true; }, 65548u);
}
