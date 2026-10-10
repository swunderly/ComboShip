// Native typed context for OoTMM entrance/save adapters. See LICENSE.OoTMM.
#pragma once
#include "Registry.h"
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace ComboRando::Entrances {
struct Key {
    Game game = Game::Oot;
    uint32_t id = 0; // Full logical ID; MM grotto IDs may exceed u16.
    friend bool operator==(const Key&, const Key&) = default;
};
struct Position {
    float x = 0, y = 0, z = 0;
    friend bool operator==(const Position&, const Position&) = default;
};
struct MmRespawn {
    Position pos;
    int16_t yaw = 0, playerParams = 0;
    uint16_t entrance = 0;
    uint8_t roomIndex = 0;
    int8_t data = 0;
    uint32_t tempSwitchFlags = 0, unk18 = 0, tempCollectFlags = 0;
};
static_assert(sizeof(MmRespawn) == 0x20);
} // namespace ComboRando::Entrances
