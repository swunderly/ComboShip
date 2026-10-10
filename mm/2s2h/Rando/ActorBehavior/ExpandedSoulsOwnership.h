#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <nlohmann/json.hpp>
// Dedicated append-only save field. Does not reuse contiguous enemy RandoInf IDs.
namespace Rando {
using ExpandedSoulsOwnership = std::array<uint8_t, 8>;
inline bool ExpandedSoulOwned(const ExpandedSoulsOwnership& owned, unsigned index) {
    return index < 59 && (owned[index >> 3] & (1u << (index & 7))) != 0;
}
inline ExpandedSoulsOwnership RestoreExpandedSouls(const nlohmann::json& savedRando) {
    ExpandedSoulsOwnership owned{};
    if (!savedRando.contains("expandedSoulsOwned"))
        return owned;
    const auto& field = savedRando.at("expandedSoulsOwned");
    if (!field.is_array() || field.size() != 8)
        throw std::runtime_error("Invalid expanded soul save bitmap");
    for (unsigned i = 0; i < 8; ++i) {
        if (!field.at(i).is_number_integer())
            throw std::runtime_error("Invalid expanded soul bitmap byte type");
        const int64_t value = field.at(i).get<int64_t>();
        if (value < 0 || value > 255)
            throw std::runtime_error("Invalid expanded soul bitmap byte");
        owned[i] = static_cast<uint8_t>(value);
    }
    owned[7] &= 7; // Only 59 assigned identities. Reserved bits cannot manufacture future souls.
    return owned;
}
} // namespace Rando
