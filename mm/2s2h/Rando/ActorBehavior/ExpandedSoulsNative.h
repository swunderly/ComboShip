#pragma once
#include <nlohmann/json.hpp>
#include "Rando/Rando.h"
#include "ExpandedSoulsSpawn.h"
namespace Rando {
inline int ExpandedSoulIndex(RandoItemId item) {
    switch (item) {
        case RI_SOUL_ANIMAL_BUTTERFLY:
            return 0;
        case RI_SOUL_ANIMAL_COW:
            return 1;
        case RI_SOUL_ANIMAL_CUCCO:
            return 2;
        case RI_SOUL_ANIMAL_DOG:
            return 3;
        case RI_SOUL_MISC_BUSINESS_SCRUB:
            return 4;
        case RI_SOUL_MISC_GS:
            return 5;
        case RI_SOUL_NPC_ANJU:
            return 6;
        case RI_SOUL_NPC_AROMA:
            return 7;
        case RI_SOUL_NPC_ASTRONOMER:
            return 8;
        case RI_SOUL_NPC_BANKER:
            return 9;
        case RI_SOUL_NPC_BAZAAR_SHOPKEEPER:
            return 10;
        case RI_SOUL_NPC_BEAN_SALESMAN:
            return 11;
        case RI_SOUL_NPC_BIGGORON:
            return 12;
        case RI_SOUL_NPC_BLACKSMITHS:
            return 13;
        case RI_SOUL_NPC_BOMBCHU_BOWLING_LADY:
            return 14;
        case RI_SOUL_NPC_BOMBCHU_SHOPKEEPER:
            return 15;
        case RI_SOUL_NPC_BOMBERS:
            return 16;
        case RI_SOUL_NPC_BUTLER_DEKU:
            return 17;
        case RI_SOUL_NPC_CARPENTERS:
            return 18;
        case RI_SOUL_NPC_CARPET_MAN:
            return 19;
        case RI_SOUL_NPC_CHEST_GAME_OWNER:
            return 20;
        case RI_SOUL_NPC_CITIZEN:
            return 21;
        case RI_SOUL_NPC_COMPOSER_BROS:
            return 22;
        case RI_SOUL_NPC_DAMPE:
            return 23;
        case RI_SOUL_NPC_DEKU_KING:
            return 24;
        case RI_SOUL_NPC_DEKU_PRINCESS:
            return 25;
        case RI_SOUL_NPC_DOG_LADY:
            return 26;
        case RI_SOUL_NPC_FISHING_POND_OWNER:
            return 27;
        case RI_SOUL_NPC_GORMAN:
            return 28;
        case RI_SOUL_NPC_GORON:
            return 29;
        case RI_SOUL_NPC_GORON_CHILD:
            return 30;
        case RI_SOUL_NPC_GORON_ELDER:
            return 31;
        case RI_SOUL_NPC_GORON_SHOPKEEPER:
            return 32;
        case RI_SOUL_NPC_GROG:
            return 33;
        case RI_SOUL_NPC_GURU_GURU:
            return 34;
        case RI_SOUL_NPC_HONEY_DARLING:
            return 35;
        case RI_SOUL_NPC_KAFEI:
            return 36;
        case RI_SOUL_NPC_KEATON:
            return 37;
        case RI_SOUL_NPC_KOUME_KOTAKE:
            return 38;
        case RI_SOUL_NPC_MALON:
            return 39;
        case RI_SOUL_NPC_MAYOR_DOTOUR:
            return 40;
        case RI_SOUL_NPC_MEDIGORON:
            return 41;
        case RI_SOUL_NPC_MOON_CHILDREN:
            return 42;
        case RI_SOUL_NPC_OLD_HAG:
            return 43;
        case RI_SOUL_NPC_PLAYGROUND_SCRUBS:
            return 44;
        case RI_SOUL_NPC_POE_COLLECTOR:
            return 45;
        case RI_SOUL_NPC_ROOFTOP_MAN:
            return 46;
        case RI_SOUL_NPC_RUTO:
            return 47;
        case RI_SOUL_NPC_SCIENTIST:
            return 48;
        case RI_SOUL_NPC_SHOOTING_GALLERY_OWNER:
            return 49;
        case RI_SOUL_NPC_TALON:
            return 50;
        case RI_SOUL_NPC_THIEVES:
            return 51;
        case RI_SOUL_NPC_TINGLE:
            return 52;
        case RI_SOUL_NPC_TOILET_HAND:
            return 53;
        case RI_SOUL_NPC_TOTO:
            return 54;
        case RI_SOUL_NPC_TOURIST_CENTER:
            return 55;
        case RI_SOUL_NPC_ZORA:
            return 56;
        case RI_SOUL_NPC_ZORA_MUSICIANS:
            return 57;
        case RI_SOUL_NPC_ZORA_SHOPKEEPER:
            return 58;
        default:
            return -1;
    }
}
inline constexpr RandoItemId expandedSoulItems[] = { RI_SOUL_ANIMAL_BUTTERFLY,
                                                     RI_SOUL_ANIMAL_COW,
                                                     RI_SOUL_ANIMAL_CUCCO,
                                                     RI_SOUL_ANIMAL_DOG,
                                                     RI_SOUL_MISC_BUSINESS_SCRUB,
                                                     RI_SOUL_MISC_GS,
                                                     RI_SOUL_NPC_ANJU,
                                                     RI_SOUL_NPC_AROMA,
                                                     RI_SOUL_NPC_ASTRONOMER,
                                                     RI_SOUL_NPC_BANKER,
                                                     RI_SOUL_NPC_BAZAAR_SHOPKEEPER,
                                                     RI_SOUL_NPC_BEAN_SALESMAN,
                                                     RI_SOUL_NPC_BIGGORON,
                                                     RI_SOUL_NPC_BLACKSMITHS,
                                                     RI_SOUL_NPC_BOMBCHU_BOWLING_LADY,
                                                     RI_SOUL_NPC_BOMBCHU_SHOPKEEPER,
                                                     RI_SOUL_NPC_BOMBERS,
                                                     RI_SOUL_NPC_BUTLER_DEKU,
                                                     RI_SOUL_NPC_CARPENTERS,
                                                     RI_SOUL_NPC_CARPET_MAN,
                                                     RI_SOUL_NPC_CHEST_GAME_OWNER,
                                                     RI_SOUL_NPC_CITIZEN,
                                                     RI_SOUL_NPC_COMPOSER_BROS,
                                                     RI_SOUL_NPC_DAMPE,
                                                     RI_SOUL_NPC_DEKU_KING,
                                                     RI_SOUL_NPC_DEKU_PRINCESS,
                                                     RI_SOUL_NPC_DOG_LADY,
                                                     RI_SOUL_NPC_FISHING_POND_OWNER,
                                                     RI_SOUL_NPC_GORMAN,
                                                     RI_SOUL_NPC_GORON,
                                                     RI_SOUL_NPC_GORON_CHILD,
                                                     RI_SOUL_NPC_GORON_ELDER,
                                                     RI_SOUL_NPC_GORON_SHOPKEEPER,
                                                     RI_SOUL_NPC_GROG,
                                                     RI_SOUL_NPC_GURU_GURU,
                                                     RI_SOUL_NPC_HONEY_DARLING,
                                                     RI_SOUL_NPC_KAFEI,
                                                     RI_SOUL_NPC_KEATON,
                                                     RI_SOUL_NPC_KOUME_KOTAKE,
                                                     RI_SOUL_NPC_MALON,
                                                     RI_SOUL_NPC_MAYOR_DOTOUR,
                                                     RI_SOUL_NPC_MEDIGORON,
                                                     RI_SOUL_NPC_MOON_CHILDREN,
                                                     RI_SOUL_NPC_OLD_HAG,
                                                     RI_SOUL_NPC_PLAYGROUND_SCRUBS,
                                                     RI_SOUL_NPC_POE_COLLECTOR,
                                                     RI_SOUL_NPC_ROOFTOP_MAN,
                                                     RI_SOUL_NPC_RUTO,
                                                     RI_SOUL_NPC_SCIENTIST,
                                                     RI_SOUL_NPC_SHOOTING_GALLERY_OWNER,
                                                     RI_SOUL_NPC_TALON,
                                                     RI_SOUL_NPC_THIEVES,
                                                     RI_SOUL_NPC_TINGLE,
                                                     RI_SOUL_NPC_TOILET_HAND,
                                                     RI_SOUL_NPC_TOTO,
                                                     RI_SOUL_NPC_TOURIST_CENTER,
                                                     RI_SOUL_NPC_ZORA,
                                                     RI_SOUL_NPC_ZORA_MUSICIANS,
                                                     RI_SOUL_NPC_ZORA_SHOPKEEPER };
bool HaveExpandedSoul(ExpandedSoul soul);
bool GrantExpandedSoulItem(RandoItemId item);
bool RemoveExpandedSoulItem(RandoItemId item);
inline bool ExpandedSoulOption(const uint32_t* options, unsigned i) {
    return options[i < 4   ? RO_SHUFFLE_ANIMAL_SOULS
                   : i < 6 ? RO_SHUFFLE_MISC_SOULS
                           : RO_SHUFFLE_NPC_SOULS] == RO_GENERIC_YES;
}
template <class Options> inline void AddExpandedSoulPool(const Options& options, std::vector<RandoItemId>& pool) {
    for (unsigned i = 0; i < 59; ++i)
        if (ExpandedSoulOption(options, i))
            pool.push_back(expandedSoulItems[i]);
}
template <class Options>
inline void AddUnshuffledExpandedSouls(const Options& options, std::vector<RandoItemId>& starts) {
    for (unsigned i = 0; i < 59; ++i)
        if (!ExpandedSoulOption(options, i))
            starts.push_back(expandedSoulItems[i]);
}
} // namespace Rando
