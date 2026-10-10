#pragma once
#include "2s2h/Rando/Types.h"
namespace Rando {
struct BeanSource { bool license; bool renewable; };
// Pinned locations.ts: ordinary rewards license; cows license and renew;
// shops, Tingle, the three scrub vendors and three milk merchants only renew.
// The three one-time MM shelves grant neither resource capability.
constexpr BeanSource BeanSourceForCheck(RandoCheckId id,RandoCheckType type) {
    if(id==RC_UNKNOWN)return {false,false};
    switch(id){
        case RC_BOMB_SHOP_ITEM_03:case RC_BOMB_SHOP_ITEM_04_OR_CURIOSITY_SHOP_ITEM:
        case RC_CURIOSITY_SHOP_SPECIAL_ITEM:return {false,false};
        case RC_SOUTHERN_SWAMP_SCRUB_BEANS:case RC_ZORA_HALL_SCRUB_POTION_REFILL:
        case RC_IKANA_CANYON_SCRUB_POTION_REFILL:case RC_GORMAN_MILK_PURCHASE:
        case RC_MILK_BAR_PURCHASE_MILK:case RC_MILK_BAR_PURCHASE_CHATEAU:return {false,true};
        default:break;
    }
    if(type==RCTYPE_COW)return {true,true};
    if(type==RCTYPE_SHOP||type==RCTYPE_TINGLE_SHOP)return {false,true};
    return {type!=RCTYPE_UNKNOWN,false};
}
}
