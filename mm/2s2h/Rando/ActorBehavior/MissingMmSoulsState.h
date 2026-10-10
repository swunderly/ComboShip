#pragma once
#include <cstdint>
#include <stdexcept>
#include <nlohmann/json.hpp>
namespace Rando {
inline constexpr unsigned MissingMmSoulCount = 6;
inline bool MissingMmSoulOwned(uint8_t owned, unsigned index) {
    return index < 6 && (owned & (1u << index));
}
inline bool ChangeMissingMmSoul(uint8_t& owned, unsigned index, bool grant) {
    if (index >= 6)
        return false;
    if (grant)
        owned |= 1u << index;
    else
        owned &= ~(1u << index);
    return true;
}
inline uint8_t RestoreMissingMmSouls(const nlohmann::json& saved) {
    if (!saved.contains("mmMissingSoulsOwned"))
        return 0;
    const auto& value = saved.at("mmMissingSoulsOwned");
    if (!value.is_number_integer())
        throw std::runtime_error("Invalid MM missing soul ownership type");
    const auto bits = value.get<int64_t>();
    if (bits < 0 || bits > 63)
        throw std::runtime_error("Invalid MM missing soul ownership bits");
    return static_cast<uint8_t>(bits);
}
// PlayState identity and room are scoped runtime state, never persistent save data.
struct MissingSoulRoomGuard {
    const void* play = nullptr;
    int room = -1;
    bool blocked = false;
    void reset(const void* current, int currentRoom) {
        play = current;
        room = currentRoom;
        blocked = false;
    }
    void deny(const void* current, int currentRoom, int spawnResult) {
        if (spawnResult != 0)
            return;
        if (play != current || room != currentRoom)
            reset(current, currentRoom);
        blocked = true;
    }
    bool canClear(const void* current, int currentRoom) const {
        return !(blocked && play == current && room == currentRoom);
    }
};
} // namespace Rando
