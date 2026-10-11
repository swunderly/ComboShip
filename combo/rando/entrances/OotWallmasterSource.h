// Source classification adapted from OoTMM v32.3 oot/actors/En/En_Wallmas.c.
// See LICENSE.OoTMM. This helper does not install routes or perform transitions.
#pragma once
#include "Registry.h"

namespace ComboRando::Entrances {
enum class OotWallmasterScene : uint8_t {
    Unknown,
    ForestTemple,
    ShadowTemple,
    SpiritTemple,
    BottomOfTheWell,
    GerudoTrainingGround,
    InsideGanonsCastle,
};

// Pass Actor.room, not the currently loaded room. A future native caller must
// exclude ShadowTag before classification and preserve vanilla respawn if no
// validated route is installed. The returned ID is a logical source, not an arrival.
inline const Record* OotWallmasterSource(OotWallmasterScene scene, uint8_t actorRoom) {
    switch (scene) {
        case OotWallmasterScene::ForestTemple:
            return Find(actorRoom == 0x14 ? "OOT_WALLMASTER_FOREST_CORRIDOR_EAST"
                                          : "OOT_WALLMASTER_FOREST_CORRIDOR_WEST");
        case OotWallmasterScene::ShadowTemple:
            return Find("OOT_WALLMASTER_SHADOW");
        case OotWallmasterScene::SpiritTemple:
            switch (actorRoom) {
                case 0x02:
                    return Find("OOT_WALLMASTER_SPIRIT_CHILD_RUPEES");
                case 0x05:
                    return Find("OOT_WALLMASTER_SPIRIT_STATUE");
                case 0x08:
                    return Find("OOT_WALLMASTER_SPIRIT_CHILD_SUN");
                case 0x0f:
                    return Find("OOT_WALLMASTER_SPIRIT_ADULT_CLIMB");
                default:
                    return nullptr;
            }
        case OotWallmasterScene::BottomOfTheWell:
            switch (actorRoom) {
                case 0x00:
                    return Find("OOT_WALLMASTER_BOTW_MAIN");
                case 0x01:
                    return Find("OOT_WALLMASTER_BOTW_BASEMENT");
                case 0x03:
                    return Find("OOT_WALLMASTER_BOTW_PIT");
                default:
                    return nullptr;
            }
        case OotWallmasterScene::GerudoTrainingGround:
            return Find("OOT_WALLMASTER_GTG");
        case OotWallmasterScene::InsideGanonsCastle:
            return Find(actorRoom == 0x0a ? "OOT_WALLMASTER_GANON_LIGHT" : "OOT_WALLMASTER_GANON_SPIRIT");
        default:
            return nullptr;
    }
}
} // namespace ComboRando::Entrances
