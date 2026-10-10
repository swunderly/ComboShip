// Full logical identity retains the native source subregion on returns.
inline RandoRegionId MmTypedGrottoArrivalRegion(uint32_t logical) {
    switch (logical) {
        case 65536u:
            return RR_TERMINA_FIELD_PILLAR_GROTTO;
        case 65792u:
            return RR_TERMINA_FIELD;
        case 65537u:
            return RR_TERMINA_FIELD_TALL_GRASS_GROTTO;
        case 65793u:
            return RR_TERMINA_FIELD;
        case 65538u:
            return RR_ROAD_TO_SOUTHERN_SWAMP_GROTTO;
        case 65794u:
            return RR_ROAD_TO_SOUTHERN_SWAMP;
        case 65539u:
            return RR_WOODS_OF_MYSTERY_GROTTO;
        case 65795u:
            return RR_MM_GROTTO_WOODS_BANK;
        case 65540u:
            return RR_SOUTHERN_SWAMP_GROTTO;
        case 65796u:
            return RR_MM_GROTTO_SWAMP_LEDGE;
        case 65541u:
            return RR_MOUNTAIN_VILLAGE_TUNNEL_GROTTO;
        case 65797u:
            return RR_MOUNTAIN_VILLAGE;
        case 65542u:
            return RR_PATH_TO_GORON_VILLAGE_RAMP_GROTTO;
        case 65798u:
            return RR_MM_GROTTO_TWIN_RAMP;
        case 65543u:
            return RR_PATH_TO_SNOWHEAD_GROTTO;
        case 65799u:
            return RR_PATH_TO_SNOWHEAD_UPPER;
        case 65544u:
            return RR_GREAT_BAY_COAST_FISHERMAN_GROTTO;
        case 65800u:
            return RR_GREAT_BAY_COAST;
        case 65545u:
            return RR_ZORA_CAPE_GROTTO;
        case 65801u:
            return RR_ZORA_CAPE;
        case 65546u:
            return RR_ROAD_TO_IKANA_GROTTO;
        case 65802u:
            return RR_ROAD_TO_IKANA_FIELD_SIDE;
        case 65547u:
            return RR_IKANA_GRAVEYARD_GROTTO;
        case 65803u:
            return RR_IKANA_GRAVEYARD_LOWER;
        case 65548u:
            return RR_IKANA_CANYON_GROTTO;
        case 65804u:
            return RR_MM_GROTTO_IKANA_SHRINE_BANK;
        case 65549u:
            return RR_TERMINA_FIELD_COW_GROTTO;
        case 65805u:
            return RR_TERMINA_FIELD;
        case 65550u:
            return RR_GREAT_BAY_COAST_COW_GROTTO;
        case 65806u:
            return RR_GREAT_BAY_COAST_CLIFFSIDE;
        case 5168u:
            return RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_2;
        case 65807u:
            return RR_TERMINA_FIELD;
        case 5152u:
            return RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_4;
        case 65808u:
            return RR_TERMINA_FIELD;
        case 5120u:
            return RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_3;
        case 65809u:
            return RR_TERMINA_FIELD;
        case 5136u:
            return RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_1;
        case 65810u:
            return RR_TERMINA_FIELD;
        case 5232u:
            return RR_TERMINA_FIELD_DODONGO_GROTTO;
        case 65811u:
            return RR_TERMINA_FIELD;
        case 5264u:
            return RR_TERMINA_FIELD_SCRUB_GROTTO;
        case 65812u:
            return RR_TERMINA_FIELD;
        case 5328u:
            return RR_TERMINA_FIELD_PEAHAT_GROTTO;
        case 65813u:
            return RR_TERMINA_FIELD;
        case 5296u:
            return RR_TERMINA_FIELD_BIO_BABA_GROTTO;
        case 65814u:
            return RR_TERMINA_FIELD;
        case 5200u:
            return RR_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO;
        case 65815u:
            return RR_PATH_TO_GORON_VILLAGE;
        case 5312u:
            return RR_DEKU_PALACE_BEAN_SALESMAN_GROTTO;
        case 20624u:
            return RR_MM_GROTTO_BEANS_BANK;
        case 13824u:
            return RR_TOWN_DEKU_PLAYGROUND;
        case 54848u:
            return RR_CLOCK_TOWN_NORTH;
        case 2576u:
            return RR_BENEATH_THE_GRAVEYARD_NIGHT_1_GRAVE;
        case 32816u:
            return RR_IKANA_GRAVEYARD_LOWER;
        case 2560u:
            return RR_BENEATH_THE_GRAVEYARD_NIGHT_2_GRAVE_BEFORE_PIT;
        case 32800u:
            return RR_IKANA_GRAVEYARD_LOWER;
        case 23040u:
            return RR_BENEATH_THE_GRAVEYARD_DAMPE;
        case 32784u:
            return RR_IKANA_GRAVEYARD_LOWER;
        default:
            throw std::invalid_argument("Unknown typed MM grotto region");
    }
}

inline std::optional<ComboRando::Entrances::Key> MmNativeGrottoSource(RandoRegionId region, uint32_t exit) {
    if (region == RR_TERMINA_FIELD && exit == 5408u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65536u };
    if (region == RR_TERMINA_FIELD_PILLAR_GROTTO && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65792u };
    if (region == RR_TERMINA_FIELD && exit == 5424u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65537u };
    if (region == RR_TERMINA_FIELD_TALL_GRASS_GROTTO && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65793u };
    if (region == RR_ROAD_TO_SOUTHERN_SWAMP && exit == 5440u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65538u };
    if (region == RR_ROAD_TO_SOUTHERN_SWAMP_GROTTO && exit == 31232u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65794u };
    if (region == RR_MM_GROTTO_WOODS_BANK && exit == 5472u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65539u };
    if (region == RR_WOODS_OF_MYSTERY_GROTTO && exit == 49664u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65795u };
    if (region == RR_MM_GROTTO_SWAMP_LEDGE && exit == 5456u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65540u };
    if (region == RR_SOUTHERN_SWAMP_GROTTO && exit == 33840u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65796u };
    if (region == RR_MOUNTAIN_VILLAGE && exit == 5488u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65541u };
    if (region == RR_MOUNTAIN_VILLAGE_TUNNEL_GROTTO && exit == 39424u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65797u };
    if (region == RR_MM_GROTTO_TWIN_RAMP && exit == 5504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65542u };
    if (region == RR_PATH_TO_GORON_VILLAGE_RAMP_GROTTO && exit == 46080u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65798u };
    if (region == RR_PATH_TO_SNOWHEAD_UPPER && exit == 5520u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65543u };
    if (region == RR_PATH_TO_SNOWHEAD_GROTTO && exit == 45072u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65799u };
    if (region == RR_GREAT_BAY_COAST && exit == 5536u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65544u };
    if (region == RR_GREAT_BAY_COAST_FISHERMAN_GROTTO && exit == 26624u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65800u };
    if (region == RR_ZORA_CAPE && exit == 5568u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65545u };
    if (region == RR_ZORA_CAPE_GROTTO && exit == 27136u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65801u };
    if (region == RR_ROAD_TO_IKANA_FIELD_SIDE && exit == 5616u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65546u };
    if (region == RR_ROAD_TO_IKANA_GROTTO && exit == 40960u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65802u };
    if (region == RR_IKANA_GRAVEYARD_LOWER && exit == 5600u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65547u };
    if (region == RR_IKANA_GRAVEYARD_GROTTO && exit == 32768u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65803u };
    if (region == RR_MM_GROTTO_IKANA_SHRINE_BANK && exit == 5584u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65548u };
    if (region == RR_IKANA_CANYON_GROTTO && exit == 8192u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65804u };
    if (region == RR_TERMINA_FIELD && exit == 5392u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65549u };
    if (region == RR_TERMINA_FIELD_COW_GROTTO && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65805u };
    if (region == RR_GREAT_BAY_COAST_CLIFFSIDE && exit == 5552u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65550u };
    if (region == RR_GREAT_BAY_COAST_COW_GROTTO && exit == 26624u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65806u };
    if (region == RR_TERMINA_FIELD && exit == 5168u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5168u };
    if (region == RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_2 && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65807u };
    if (region == RR_TERMINA_FIELD && exit == 5152u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5152u };
    if (region == RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_4 && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65808u };
    if (region == RR_TERMINA_FIELD && exit == 5120u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5120u };
    if (region == RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_3 && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65809u };
    if (region == RR_TERMINA_FIELD && exit == 5136u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5136u };
    if (region == RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_1 && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65810u };
    if (region == RR_TERMINA_FIELD && exit == 5232u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5232u };
    if (region == RR_TERMINA_FIELD_DODONGO_GROTTO && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65811u };
    if (region == RR_TERMINA_FIELD && exit == 5264u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5264u };
    if (region == RR_TERMINA_FIELD_SCRUB_GROTTO && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65812u };
    if (region == RR_TERMINA_FIELD && exit == 5328u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5328u };
    if (region == RR_TERMINA_FIELD_PEAHAT_GROTTO && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65813u };
    if (region == RR_TERMINA_FIELD && exit == 5296u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5296u };
    if (region == RR_TERMINA_FIELD_BIO_BABA_GROTTO && exit == 21504u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65814u };
    if (region == RR_PATH_TO_GORON_VILLAGE && exit == 5200u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5200u };
    if (region == RR_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO && exit == 46080u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 65815u };
    if (region == RR_MM_GROTTO_BEANS_BANK && exit == 5312u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 5312u };
    if (region == RR_DEKU_PALACE_BEAN_SALESMAN_GROTTO && exit == 20624u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 20624u };
    if (region == RR_CLOCK_TOWN_NORTH && exit == 13824u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 13824u };
    if (region == RR_TOWN_DEKU_PLAYGROUND && exit == 54848u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 54848u };
    if (region == RR_IKANA_GRAVEYARD_LOWER && exit == 2576u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 2576u };
    if (region == RR_BENEATH_THE_GRAVEYARD_NIGHT_1_GRAVE && exit == 32816u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 32816u };
    if (region == RR_IKANA_GRAVEYARD_LOWER && exit == 2560u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 2560u };
    if (region == RR_BENEATH_THE_GRAVEYARD_NIGHT_2_GRAVE_BEFORE_PIT && exit == 32800u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 32800u };
    if (region == RR_IKANA_GRAVEYARD_LOWER && exit == 23040u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 23040u };
    if (region == RR_MM_GRAVE_NIGHT3_FAKE_EXIT && exit == 32784u)
        return ComboRando::Entrances::Key{ ComboRando::Entrances::Game::Mm, 32784u };
    return std::nullopt;
}
