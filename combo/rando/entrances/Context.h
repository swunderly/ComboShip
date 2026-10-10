// Native typed context for OoTMM entrance/save adapters. See LICENSE.OoTMM.
#pragma once
#include "Registry.h"
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <variant>

namespace ComboRando::Entrances {
// Numerical values match both native engines and pinned OoTMM save layouts.
enum class OotAge : uint8_t { Adult = 0, Child = 1 };
enum class MmForm : uint8_t { FierceDeity = 0, Goron = 1, Zora = 2, Deku = 3, Human = 4 };
enum class ArrivalReason : uint8_t { Entrance, Portal, Song, SaveResume, Death, Void, Farore };
struct Key {
    Game game = Game::Oot;
    uint32_t id = 0; // Full logical ID; MM grotto IDs may exceed u16.
    friend bool operator==(const Key&, const Key&) = default;
};
struct Position {
    float x = 0, y = 0, z = 0;
    friend bool operator==(const Position&, const Position&) = default;
};
struct OotRespawn {
    Position pos;
    int16_t yaw = 0, playerParams = 0;
    uint16_t entrance = 0;
    uint8_t roomIndex = 0;
    int8_t data = 0;
    uint32_t tempSwitchFlags = 0, tempCollectFlags = 0;
};
struct MmRespawn {
    Position pos;
    int16_t yaw = 0, playerParams = 0;
    uint16_t entrance = 0;
    uint8_t roomIndex = 0;
    int8_t data = 0;
    uint32_t tempSwitchFlags = 0, unk18 = 0, tempCollectFlags = 0;
};
static_assert(sizeof(OotRespawn) == 0x1c);
static_assert(sizeof(MmRespawn) == 0x20);
using Respawn = std::variant<OotRespawn, MmRespawn>;

struct RespawnAnchor {
    Key entrance; // Explicit game replaces OoTMM's overloaded high data bit.
    Respawn nativeData;
    OotAge age = OotAge::Child;
    MmForm form = MmForm::Human;
};
struct Arrival {
    Key destination;
    ArrivalReason reason = ArrivalReason::Entrance;
    OotAge age = OotAge::Child;
    MmForm form = MmForm::Human;
    std::optional<RespawnAnchor> dungeonIngress;
    std::optional<RespawnAnchor> farore;
    bool alreadyResolved = false; // Cross-game arrival must not re-enter source ER lookup.
};
struct SaveContexts {
    // Optional extension only; absent in old seeds/saves means disabled.
    std::optional<RespawnAnchor> dungeonIngress;
    std::array<std::optional<RespawnAnchor>, 2> farore;
    void clearDungeonIngress() {
        dungeonIngress.reset();
    }
};
inline uint32_t EncodeWireId(Game source, Key destination) {
    if (destination.id & 0x80000000u)
        throw std::invalid_argument("Logical ID overlaps foreign-game bit");
    return destination.id | (source != destination.game ? 0x80000000u : 0);
}
inline Key DecodeWireId(Game source, uint32_t value) {
    return { (value & 0x80000000u) ? (source == Game::Oot ? Game::Mm : Game::Oot) : source, value & 0x7fffffffu };
}
inline bool ShouldOverride(ArrivalReason reason) {
    return reason == ArrivalReason::Entrance || reason == ArrivalReason::Portal || reason == ArrivalReason::Song;
}
inline bool ShouldOverride(const Arrival& arrival) {
    return !arrival.alreadyResolved && ShouldOverride(arrival.reason);
}
} // namespace ComboRando::Entrances
