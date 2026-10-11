// OoTMM v32.3 data/world/mm/overworld/southern_swamp.yml (unchanged from 41a2b7c0 apart from the shovel and gossip
// lines). Active under MM grotto/ordinary entrance shuffle and for the pinned South graph (MM_LOGIC_SOUTH_PINNED);
// is_tall, hover and iron boots are false while their options are off. short_hook_anywhere is not ported.
inline bool TypedMmSwampActive() {
    return TypedMmGrottoSourceActive(65540u) || MM_LOGIC_SOUTH_PINNED;
}
// macros_mm.yml:83 can_swim = can_enter_water || has_mask_zora: the Swim ability (the port has no MM scales) or the
// Zora Mask. As in OoTMM, neither this nor is_tall counts Goron or Fierce Deity, so this graph has no Goron wading
// where OoTMM lacks it (that matters only with Swim shuffled and not found) and ignores the native logic rule
// "Fierce Deity's Mask anywhere". OoTMM gives Fierce Deity Anywhere "No logical applications" (data.ts:1601); its
// is_tall carries the note "#add Fierce Deity" (macros_mm.yml:61), which v32.3 does not act on.
inline bool MmSwampCanSwim() {
    return CAN_USE_ABILITY(SWIM) || CAN_BE_ZORA;
}
// macros_mm.yml:260 cross_poison_swamp.
inline bool MmSwampCrossPoison() {
    return MM_LOGIC_IS_TALL || CAN_BE_GORON || (MmSwampCanSwim() && (NativeMmSwampCleared() || CAN_USE_NAYRUS_LOVE)) ||
           MM_LOGIC_HAS_IRON_BOOTS;
}
// can_swim_or_sink || is_tall || has_mask_goron (macros_mm.yml:84; southern_swamp.yml:8, 54, 56, 67, 94, 109, 122).
inline bool MmSwampSwimOrTallOrGoron() {
    return MmSwampCanSwim() || MM_LOGIC_HAS_IRON_BOOTS || MM_LOGIC_IS_TALL || CAN_BE_GORON;
}
inline bool MmSwampKillOctoClose() {
    return CAN_BE_ZORA || CAN_USE_NAYRUS_LOVE || CAN_USE_DINS_FIRE || CAN_USE_BOW || HAS_ITEM(ITEM_HOOKSHOT);
}
inline bool MmExtraIndoorsShuffled() {
    for (const auto& record : ComboRando::Entrances::kRegistry)
        if (record.game == ComboRando::Entrances::Game::Mm &&
            record.type == ComboRando::Entrances::Type::IndoorsExtra) {
            if (ComboRando::Entrances::ActiveMmOrdinary().containsSource(record.id))
                return true;
            if (const auto& assumed = ComboRando::Entrances::ActiveMmOrdinaryAssumptions();
                assumed && assumed->routes.contains(record.id))
                return true;
        }
    return false;
}
inline void EnsureMmSwampContexts() {
    if (Regions.contains(RR_MM_GROTTO_SWAMP_LEDGE))
        return;
    const std::set<RandoRegionId> old = { RR_SOUTHERN_SWAMP_NORTH, RR_SOUTHERN_SWAMP_NEAR_WOODS,
                                          RR_SOUTHERN_SWAMP_NEAR_FLOWERS, RR_SOUTHERN_SWAMP_SOUTH,
                                          RR_SOUTHERN_SWAMP_SOUTH_UPPER };
    for (auto source : old)
        for (auto& [target, condition] : Regions.at(source).connections)
            if (old.contains(target)) {
                auto original = condition.first;
                condition.first = [original] { return !TypedMmSwampActive() && original(); };
            }
    for (auto id : { RR_MM_SWAMP_WATER_TOURIST, RR_MM_SWAMP_WATER_WITCH, RR_MM_SWAMP_WATER_FROG,
                     RR_MM_SWAMP_WATER_PALACE, RR_MM_SWAMP_WATER_SPIDER, RR_MM_SWAMP_NEAR_FROG, RR_MM_SWAMP_NEAR_SPIDER,
                     RR_MM_GROTTO_SWAMP_LEDGE, RR_MM_SWAMP_CANOPY_FRONT, RR_MM_SWAMP_BOAT_RIDE })
        Regions[id] = RandoRegion{ .name = "Pinned Swamp approach", .sceneId = SCENE_20SICHITAI };
    auto edge = [](RandoRegionId source, RandoRegionId target, std::function<bool()> gate) {
        auto& connections = Regions.at(source).connections;
        auto found = connections.find(target);
        if (found == connections.end())
            connections.emplace(target,
                                std::make_pair(std::function<bool()>([gate] { return TypedMmSwampActive() && gate(); }),
                                               std::string("Pinned Swamp context")));
        else {
            auto legacy = found->second.first;
            found->second.first = [legacy, gate] { return legacy() || (TypedMmSwampActive() && gate()); };
        }
    };
    // Native wide sections become exact pinned dry regions; water routes have their own frontier.
    edge(RR_SOUTHERN_SWAMP_NORTH, RR_MM_SWAMP_WATER_TOURIST, [] { return MmSwampSwimOrTallOrGoron(); });
    edge(RR_SOUTHERN_SWAMP_NORTH, RR_SOUTHERN_SWAMP_NEAR_WOODS, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_MM_SWAMP_WATER_TOURIST, RR_SOUTHERN_SWAMP_NORTH, [] { return true; });
    edge(RR_MM_SWAMP_WATER_TOURIST, RR_MM_SWAMP_WATER_WITCH, [] { return true; });
    edge(RR_MM_SWAMP_WATER_TOURIST, RR_MM_SWAMP_WATER_SPIDER,
         [] { return (NativeMmSwampCleared() || RANDO_EVENTS[RE_MM_SWAMP_OCTO_RIGHT]) && MmSwampCrossPoison(); });
    edge(RR_MM_SWAMP_WATER_WITCH, RR_SOUTHERN_SWAMP_NEAR_WOODS, [] { return MmSwampSwimOrTallOrGoron(); });
    edge(RR_MM_SWAMP_WATER_WITCH, RR_MM_SWAMP_WATER_TOURIST, [] { return true; });
    edge(RR_MM_SWAMP_WATER_WITCH, RR_MM_SWAMP_WATER_FROG, [] { return MmSwampSwimOrTallOrGoron(); });
    edge(RR_MM_SWAMP_WATER_FROG, RR_MM_SWAMP_NEAR_FROG, [] { return MmSwampCanSwim() || MM_LOGIC_IS_TALL; });
    edge(RR_MM_SWAMP_WATER_FROG, RR_SOUTHERN_SWAMP_NEAR_FLOWERS, [] { return MM_LOGIC_IS_TALL; });
    edge(RR_MM_SWAMP_WATER_FROG, RR_MM_SWAMP_WATER_WITCH, [] { return MmSwampSwimOrTallOrGoron(); });
    edge(RR_MM_SWAMP_WATER_FROG, RR_MM_SWAMP_WATER_PALACE,
         [] { return (NativeMmSwampCleared() || RANDO_EVENTS[RE_MM_SWAMP_OCTO_LEFT]) && MmSwampCrossPoison(); });
    edge(RR_MM_SWAMP_WATER_PALACE, RR_MM_SWAMP_WATER_FROG,
         [] { return (NativeMmSwampCleared() || RANDO_EVENTS[RE_MM_SWAMP_OCTO_LEFT]) && MmSwampCrossPoison(); });
    edge(RR_MM_SWAMP_WATER_PALACE, RR_MM_SWAMP_WATER_SPIDER, [] { return MmSwampCrossPoison(); });
    edge(RR_MM_SWAMP_WATER_PALACE, RR_SOUTHERN_SWAMP_SOUTH, [] {
        return MM_LOGIC_IS_TALL || (MmSwampCanSwim() && CAN_USE_NAYRUS_LOVE) || (CAN_BE_GORON && CAN_BE_DEKU) ||
               (NativeMmSwampCleared() && (MmSwampCanSwim() || MM_LOGIC_HAS_IRON_BOOTS || CAN_BE_GORON));
    });
    edge(RR_MM_SWAMP_WATER_SPIDER, RR_MM_SWAMP_WATER_PALACE, [] { return MmSwampCrossPoison(); });
    edge(RR_MM_SWAMP_WATER_SPIDER, RR_MM_SWAMP_WATER_TOURIST,
         [] { return (NativeMmSwampCleared() || RANDO_EVENTS[RE_MM_SWAMP_OCTO_RIGHT]) && MmSwampCrossPoison(); });
    edge(RR_MM_SWAMP_WATER_SPIDER, RR_MM_SWAMP_NEAR_SPIDER, [] {
        return MM_LOGIC_IS_TALL || (MmSwampCanSwim() && (NativeMmSwampCleared() || CAN_USE_NAYRUS_LOVE)) ||
               (CAN_BE_GORON && CAN_BE_DEKU);
    });
    edge(RR_MM_SWAMP_WATER_SPIDER, RR_MM_GROTTO_SWAMP_LEDGE, [] {
        return MM_LOGIC_IS_TALL || (CAN_BE_GORON && CAN_BE_DEKU) ||
               (NativeMmSwampCleared() && (MmSwampCanSwim() || CAN_BE_GORON || MM_LOGIC_HAS_IRON_BOOTS));
    });
    edge(RR_MM_SWAMP_NEAR_FROG, RR_SOUTHERN_SWAMP_NEAR_WOODS, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_MM_SWAMP_NEAR_FROG, RR_MM_SWAMP_WATER_FROG, [] { return MmSwampSwimOrTallOrGoron(); });
    edge(RR_MM_SWAMP_NEAR_FROG, RR_SOUTHERN_SWAMP_NEAR_FLOWERS, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_SOUTHERN_SWAMP_NEAR_FLOWERS, RR_MM_SWAMP_NEAR_FROG, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_SOUTHERN_SWAMP_NEAR_FLOWERS, RR_SOUTHERN_SWAMP_SOUTH, [] {
        return (NativeMmSwampCleared() || RANDO_EVENTS[RE_MM_SWAMP_OCTO_LEFT]) &&
               (CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS);
    });
    edge(RR_SOUTHERN_SWAMP_NEAR_FLOWERS, RR_MM_SWAMP_WATER_FROG, [] { return MmSwampSwimOrTallOrGoron(); });
    edge(RR_SOUTHERN_SWAMP_NEAR_WOODS, RR_SOUTHERN_SWAMP_NORTH, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_SOUTHERN_SWAMP_NEAR_WOODS, RR_MM_SWAMP_NEAR_FROG, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_SOUTHERN_SWAMP_NEAR_WOODS, RR_MM_SWAMP_WATER_WITCH, [] { return MmSwampSwimOrTallOrGoron(); });
    edge(RR_SOUTHERN_SWAMP_SOUTH, RR_MM_SWAMP_WATER_PALACE, [] { return MmSwampCrossPoison(); });
    edge(RR_SOUTHERN_SWAMP_SOUTH, RR_SOUTHERN_SWAMP_NEAR_FLOWERS, [] {
        return (NativeMmSwampCleared() || RANDO_EVENTS[RE_MM_SWAMP_OCTO_LEFT]) &&
               (CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS);
    });
    edge(RR_SOUTHERN_SWAMP_SOUTH, RR_MM_SWAMP_NEAR_SPIDER, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_SOUTHERN_SWAMP_SOUTH, RR_SOUTHERN_SWAMP_SOUTH_UPPER, [] {
        return NativeMmSwampCleared() && (MmSwampSwimOrTallOrGoron() || MM_LOGIC_HAS_HOVER_BOOTS || CAN_BE_DEKU);
    });
    edge(RR_MM_SWAMP_NEAR_SPIDER, RR_MM_SWAMP_WATER_SPIDER, [] { return MmSwampCrossPoison(); });
    edge(RR_MM_SWAMP_NEAR_SPIDER, RR_SOUTHERN_SWAMP_SOUTH, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_MM_SWAMP_NEAR_SPIDER, RR_MM_GROTTO_SWAMP_LEDGE, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_MM_GROTTO_SWAMP_LEDGE, RR_MM_SWAMP_WATER_SPIDER, [] { return MmSwampCrossPoison(); });
    edge(RR_MM_GROTTO_SWAMP_LEDGE, RR_MM_SWAMP_NEAR_SPIDER,
         [] { return CAN_BE_DEKU || HAS_ITEM(ITEM_HOOKSHOT) || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_MM_SWAMP_CANOPY_FRONT, RR_MM_SWAMP_WATER_PALACE,
         [] { return NativeMmSwampCleared() || MM_LOGIC_IS_TALL || MM_LOGIC_HAS_IRON_BOOTS || CAN_USE_NAYRUS_LOVE; });
    edge(RR_MM_SWAMP_CANOPY_FRONT, RR_SOUTHERN_SWAMP_SOUTH, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_MM_SWAMP_CANOPY_FRONT, RR_MM_SWAMP_NEAR_SPIDER, [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_MM_SWAMP_CANOPY_FRONT, RR_MM_GROTTO_SWAMP_LEDGE, [] { return true; });
    edge(RR_MM_SWAMP_CANOPY_FRONT, RR_SOUTHERN_SWAMP_SOUTH_UPPER, [] { return CAN_BE_DEKU; });
    edge(RR_SOUTHERN_SWAMP_SOUTH_UPPER, RR_SOUTHERN_SWAMP_SOUTH,
         [] { return CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS; });
    edge(RR_SOUTHERN_SWAMP_SOUTH_UPPER, RR_MM_SWAMP_WATER_SPIDER,
         [] { return MM_LOGIC_IS_TALL || MM_LOGIC_HAS_IRON_BOOTS || NativeMmSwampCleared() || CAN_USE_NAYRUS_LOVE; });
    // Pinned Tourist -> Boat Ride: paid rescued-Koume route is disabled by Extra Indoors ER.
    edge(RR_TOURIST_INFORMATION, RR_MM_SWAMP_BOAT_RIDE, [] {
        return NativeMmSwampPoisoned() && ((Rando::HaveExpandedSoul(Rando::ExpandedSoul::NPC_TOURIST_CENTER) &&
                                            CAN_ACCESS(PICTOGRAPH_SWAMP_GENERIC)) ||
                                           (RANDO_EVENTS[RE_SAVED_KOUME] && !MmExtraIndoorsShuffled() && CAN_PAY(1)));
    });
    edge(RR_MM_SWAMP_BOAT_RIDE, RR_SOUTHERN_SWAMP_SOUTH, [] { return true; });
    Regions.at(RR_MM_SWAMP_BOAT_RIDE).events.push_back(EVENT(RE_MM_SWAMP_OCTO_LEFT, TypedMmSwampActive()));
    Regions.at(RR_MM_SWAMP_BOAT_RIDE).events.push_back(EVENT(RE_MM_SWAMP_OCTO_RIGHT, TypedMmSwampActive()));
    // Derive left/right barriers independently. Never reuse the old global Octo event.
    for (auto source : { RR_SOUTHERN_SWAMP_NORTH, RR_MM_SWAMP_WATER_TOURIST })
        Regions.at(source).events.push_back(
            EVENT(RE_MM_SWAMP_OCTO_RIGHT, TypedMmSwampActive() && NativeMmSwampPoisoned() && MmSwampKillOctoClose()));
    for (auto source : { RR_MM_SWAMP_NEAR_FROG, RR_SOUTHERN_SWAMP_NEAR_FLOWERS })
        Regions.at(source).events.push_back(
            EVENT(RE_MM_SWAMP_OCTO_LEFT, TypedMmSwampActive() && NativeMmSwampPoisoned() && MmSwampKillOctoClose()));
    Regions.at(RR_MM_SWAMP_WATER_PALACE)
        .events.push_back(
            EVENT(RE_MM_SWAMP_OCTO_LEFT,
                  TypedMmSwampActive() && NativeMmSwampPoisoned() &&
                      (CAN_BE_ZORA || (MM_LOGIC_IS_ADULT && (CAN_USE_NAYRUS_LOVE || CAN_USE_DINS_FIRE)))));
    Regions.at(RR_MM_SWAMP_WATER_SPIDER)
        .events.push_back(
            EVENT(RE_MM_SWAMP_OCTO_RIGHT,
                  TypedMmSwampActive() && NativeMmSwampPoisoned() &&
                      (CAN_BE_ZORA || (MM_LOGIC_IS_ADULT && (CAN_USE_NAYRUS_LOVE || CAN_USE_DINS_FIRE)))));
    Regions.at(RR_SOUTHERN_SWAMP_SOUTH)
        .events.push_back(EVENT(RE_MM_SWAMP_OCTO_LEFT, TypedMmSwampActive() && NativeMmSwampPoisoned() &&
                                                           (CAN_USE_BOW || CAN_USE_NAYRUS_LOVE ||
                                                            (HAS_ITEM(ITEM_HOOKSHOT) && MM_LOGIC_HAS_HOVER_BOOTS))));
    Regions.at(RR_MM_GROTTO_SWAMP_LEDGE)
        .events.push_back(EVENT(RE_MM_SWAMP_OCTO_RIGHT, TypedMmSwampActive() && NativeMmSwampPoisoned() &&
                                                            (CAN_USE_BOW || CAN_USE_NAYRUS_LOVE)));
    // Breaking the hive and collecting its contents are distinct pinned events/routes.
    Regions.at(RR_MM_SWAMP_WATER_WITCH)
        .events.push_back(EVENT(RE_MM_SWAMP_HIVE,
                                TypedMmSwampActive() &&
                                    (CAN_BE_ZORA || (MM_LOGIC_IS_ADULT && (HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_BOW)))));
    Regions.at(RR_MM_SWAMP_NEAR_FROG)
        .events.push_back(EVENT(RE_MM_SWAMP_HIVE, TypedMmSwampActive() && (HAS_ITEM(ITEM_HOOKSHOT) || CAN_USE_BOW ||
                                                                           (CAN_BE_DEKU && HAS_MAGIC))));
    Regions.at(RR_SOUTHERN_SWAMP_NEAR_WOODS)
        .events.push_back(
            EVENT(RE_MM_SWAMP_HIVE, TypedMmSwampActive() && ((CAN_BE_DEKU && HAS_MAGIC) ||
                                                             (MM_LOGIC_HAS_HOVER_BOOTS && HAS_ITEM(ITEM_HOOKSHOT)))));
    auto& hive = Regions.at(RR_SOUTHERN_SWAMP_NORTH).checks.at(RC_SOUTHERN_SWAMP_POISON_BEEHIVE);
    auto oldHive = hive.first;
    hive.first = [oldHive] { return !TypedMmSwampActive() && oldHive(); };
    Regions.at(RR_MM_SWAMP_WATER_WITCH)
        .checks.insert(CHECK(RC_SOUTHERN_SWAMP_POISON_BEEHIVE, TypedMmSwampActive() && NativeMmSwampPoisoned() &&
                                                                   RANDO_EVENTS[RE_MM_SWAMP_HIVE] &&
                                                                   MmSwampSwimOrTallOrGoron()));
    Regions.at(RR_MM_SWAMP_NEAR_FROG)
        .checks.insert(CHECK(RC_SOUTHERN_SWAMP_POISON_BEEHIVE, TypedMmSwampActive() && NativeMmSwampPoisoned() &&
                                                                   RANDO_EVENTS[RE_MM_SWAMP_HIVE] &&
                                                                   (CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS)));
    // southern_swamp.yml:152: the hive can also be collected near the potion shop.
    Regions.at(RR_SOUTHERN_SWAMP_NEAR_WOODS)
        .checks.insert(CHECK(RC_SOUTHERN_SWAMP_POISON_BEEHIVE, TypedMmSwampActive() && NativeMmSwampPoisoned() &&
                                                                   RANDO_EVENTS[RE_MM_SWAMP_HIVE] &&
                                                                   (CAN_BE_DEKU || MM_LOGIC_HAS_HOVER_BOOTS)));
    for (auto check : { RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_01, RC_SOUTHERN_SWAMP_FREESTANDING_RUPEE_02 }) {
        auto& slot = Regions.at(RR_SOUTHERN_SWAMP_NEAR_FLOWERS).checks.at(check);
        auto legacy = slot.first;
        slot.first = [legacy] { return TypedMmSwampActive() ? NativeMmSwampPoisoned() : legacy(); };
    }
    auto moveExit = [](RandoRegionId source, RandoRegionId target, s32 entry, std::function<bool()> gate) {
        auto original = Regions.at(source).exits.at(entry);
        auto legacy = original.condition;
        Regions.at(source).exits.at(entry).condition = [legacy] { return !TypedMmSwampActive() && legacy(); };
        original.condition = [gate] { return TypedMmSwampActive() && gate(); };
        Regions.at(target).exits.emplace(entry, std::move(original));
    };
    moveExit(RR_SOUTHERN_SWAMP_SOUTH, RR_MM_GROTTO_SWAMP_LEDGE, ENTRANCE(GROTTOS, 21), [] { return true; });
    moveExit(RR_SOUTHERN_SWAMP_SOUTH, RR_MM_SWAMP_NEAR_SPIDER, ENTRANCE(SWAMP_SPIDER_HOUSE, 0),
             [] { return CAN_USE_STICKS || CAN_USE_BOW; });
    moveExit(RR_SOUTHERN_SWAMP_SOUTH_UPPER, RR_MM_SWAMP_CANOPY_FRONT, ENTRANCE(DEKU_PALACE, 5), [] { return true; });
    auto& woodfall = Regions.at(RR_SOUTHERN_SWAMP_SOUTH_UPPER).exits.at(ENTRANCE(WOODFALL, 0));
    auto prior = woodfall.condition;
    woodfall.condition = [prior] { return TypedMmSwampActive() || prior(); };
    // Frogs are on the bank, not the flower platforms.
    auto& frog = Regions.at(RR_SOUTHERN_SWAMP_NEAR_FLOWERS).checks.at(RC_SOUTHERN_SWAMP_FROG);
    auto originalFrog = frog.first;
    frog.first = [originalFrog] { return !TypedMmSwampActive() && originalFrog(); };
    Regions.at(RR_MM_SWAMP_NEAR_FROG)
        .checks.insert(CHECK(RC_SOUTHERN_SWAMP_FROG, TypedMmSwampActive() && HAS_ITEM(ITEM_MASK_DON_GERO)));
    // The native Octorok drop has no OoTMM location; keep it reachable from the water the Octoroks swim in, with
    // its native condition, now that the flower platforms are the dry Swamp Rupee Flowers region.
    Regions.at(RR_MM_SWAMP_WATER_FROG)
        .checks.insert(CHECK(RC_ENEMY_DROP_OCTOROK, TypedMmSwampActive() && CanKillEnemy(ACTOR_EN_OKUTA) &&
                                                        (CAN_USE_ABILITY(SWIM) || CAN_BE_ZORA || CAN_BE_GORON)));
}
