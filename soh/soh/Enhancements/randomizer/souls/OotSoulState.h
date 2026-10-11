#pragma once
#include "OotSoulPolicy.h"
#include <array>
#include <cstdint>
#include <stdexcept>
#include <nlohmann/json.hpp>
namespace Rando::Souls {
inline constexpr unsigned GeneralOotSoulCount = 95;
using GeneralOotSoulsOwnership = std::array<uint8_t, 12>;
// Save bit order, taken from the item table so the two cannot drift apart.
inline constexpr std::array<OotSoul, GeneralOotSoulCount> generalOotSoulItems = {
#define OOT_SOUL_ITEM(item, index, soul, label, inGame) soul,
#include "OotSoulData.inc"
#undef OOT_SOUL_ITEM
};
// Families are contiguous in index order: enemies, NPCs, animals, misc.
enum class OotSoulFamily : unsigned { Enemy, Npc, Animal, Misc };
inline constexpr unsigned GeneralOotSoulFamily(unsigned index) {
    return index < 43 ? 0 : index < 89 ? 1 : index < 93 ? 2 : 3;
}
static_assert(generalOotSoulItems[42] == OotSoul::ENEMY_THIEVES && generalOotSoulItems[43] == OotSoul::NPC_OLD_HAG &&
              generalOotSoulItems[88] == OotSoul::NPC_THIEVES && generalOotSoulItems[89] == OotSoul::ANIMAL_CUCCO &&
              generalOotSoulItems[92] == OotSoul::ANIMAL_BUTTERFLY &&
              generalOotSoulItems[93] == OotSoul::MISC_BUSINESS_SCRUB &&
              generalOotSoulItems[GeneralOotSoulCount - 1] == OotSoul::MISC_GS);
// Logic asks for souls constantly, so look the save bit up instead of scanning the table.
inline constexpr std::array<int, size_t(OotSoul::COUNT)> generalOotSoulIndexOf = [] {
    std::array<int, size_t(OotSoul::COUNT)> table{};
    for (auto& index : table)
        index = -1;
    for (unsigned i = 0; i < GeneralOotSoulCount; ++i)
        table[size_t(generalOotSoulItems[i])] = int(i);
    return table;
}();
inline int GeneralOotSoulIndex(OotSoul soul) {
    return size_t(soul) < generalOotSoulIndexOf.size() ? generalOotSoulIndexOf[size_t(soul)] : -1;
}
inline bool GeneralOotSoulOwned(const GeneralOotSoulsOwnership& owned, unsigned index) {
    return index < GeneralOotSoulCount && (owned[index >> 3] & (1u << (index & 7)));
}
inline bool ChangeGeneralOotSoul(GeneralOotSoulsOwnership& owned, unsigned index, bool grant) {
    if (index >= GeneralOotSoulCount)
        return false;
    if (grant)
        owned[index >> 3] |= 1u << (index & 7);
    else
        owned[index >> 3] &= ~(1u << (index & 7));
    return true;
}
inline GeneralOotSoulsOwnership RestoreGeneralOotSouls(const nlohmann::json& saved) {
    GeneralOotSoulsOwnership out{};
    if (!saved.contains("ootExpandedSoulsOwned"))
        return out;
    const auto& source = saved.at("ootExpandedSoulsOwned");
    if (!source.is_array() || source.size() != out.size())
        throw std::runtime_error("Invalid OoT soul ownership length");
    for (unsigned i = 0; i < out.size(); ++i) {
        if (!source[i].is_number_integer())
            throw std::runtime_error("Invalid OoT soul ownership type");
        const int64_t value = source[i].get<int64_t>();
        // The last byte holds only the remaining souls' bits.
        if (value < 0 || value > (i == out.size() - 1 ? (1 << (GeneralOotSoulCount - 8 * i)) - 1 : 255))
            throw std::runtime_error("Invalid OoT soul ownership bits");
        out[i] = static_cast<uint8_t>(value);
    }
    return out;
}
struct GeneralOotSoulOptions {
    bool enabled = false, enemy = false, npc = false, animal = false, misc = false;
};
// OoTMM v32.3 has one boolean per soul family and no master switch (packages/core/src/settings/
// data.ts:2336-2414). A family is offered only once its gates and runtime are finished, as MM's
// ExpandedSoulsReadiness.h does; an unpublished family stays off whatever a profile, CVar or seed says.
inline constexpr bool OotEnemySoulsPublic = false, OotNpcSoulsPublic = false, OotAnimalSoulsPublic = false,
                      OotMiscSoulsPublic = false;
// The stored RSK_COMPLETE_OOT_SOULS of a new seed: on when any published family is on.
inline constexpr bool DeriveGeneralOotSoulSwitch(bool enemy, bool npc, bool animal, bool misc) {
    return (OotEnemySoulsPublic && enemy) || (OotNpcSoulsPublic && npc) || (OotAnimalSoulsPublic && animal) ||
           (OotMiscSoulsPublic && misc);
}
// The options a seed plays with. A save keeps its stored switch, so old saves behave as they did.
inline constexpr GeneralOotSoulOptions PublishedGeneralOotSoulOptions(bool stored, bool enemy, bool npc, bool animal,
                                                                      bool misc) {
    return { stored && DeriveGeneralOotSoulSwitch(enemy, npc, animal, misc), OotEnemySoulsPublic && enemy,
             OotNpcSoulsPublic && npc, OotAnimalSoulsPublic && animal, OotMiscSoulsPublic && misc };
}
inline bool GeneralOotSoulFamilyShuffled(const GeneralOotSoulOptions& options, OotSoulFamily family) {
    if (!options.enabled)
        return false;
    switch (family) {
        case OotSoulFamily::Enemy:
            return options.enemy;
        case OotSoulFamily::Npc:
            return options.npc;
        case OotSoulFamily::Animal:
            return options.animal;
        case OotSoulFamily::Misc:
            return options.misc;
    }
    return false;
}
inline bool GeneralOotSoulShuffled(const GeneralOotSoulOptions& options, unsigned index) {
    return index < GeneralOotSoulCount &&
           GeneralOotSoulFamilyShuffled(options, OotSoulFamily(GeneralOotSoulFamily(index)));
}
inline bool HaveGeneralOotSoul(const GeneralOotSoulsOwnership& owned, const GeneralOotSoulOptions& options,
                               OotSoul soul) {
    const int index = GeneralOotSoulIndex(soul);
    return index >= 0 && (!GeneralOotSoulShuffled(options, index) || GeneralOotSoulOwned(owned, index));
}
} // namespace Rando::Souls
