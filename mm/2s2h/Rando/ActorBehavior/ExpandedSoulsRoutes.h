#pragma once
#include "ExpandedSoulsNative.h"
namespace Rando {
inline bool ExpandedTradingPostRoute(bool earlyWindow, bool lateWindow) {
    return (HaveExpandedSoul(ExpandedSoul::NPC_FISHING_POND_OWNER) && earlyWindow) ||
           (HaveExpandedSoul(ExpandedSoul::NPC_ROOFTOP_MAN) && lateWindow);
}
inline bool ExpandedKotakeReward(bool firstDay, bool metKoumeDay1) {
    return firstDay && metKoumeDay1;
}
inline bool ExpandedKotakeWoodsReward(bool secondOrFinalDay, bool kotakeNearShop) {
    return HaveExpandedSoul(ExpandedSoul::NPC_KOUME_KOTAKE) && secondOrFinalDay && kotakeNearShop;
}
inline bool ExpandedGoronShrineDoor(bool firstDay, bool goronForm) {
    return (HaveExpandedSoul(ExpandedSoul::NPC_GORON) && firstDay) || goronForm;
}
inline bool ExpandedBoatAuthorization(bool savedKoume, bool swampPhoto) {
    return savedKoume || (HaveExpandedSoul(ExpandedSoul::NPC_TOURIST_CENTER) && swampPhoto);
}
inline bool ExpandedGoronFoodTorches(bool fireArrows, bool songAndSticks) {
    return fireArrows || (songAndSticks && HaveExpandedSoul(ExpandedSoul::NPC_GORON_CHILD));
}
}
