#pragma once
// OoT's scene restriction table (sRestrictionFlags, soh/src/code/z_parameter.c) as OoTMM v32.3 patches it, and the seed
// marker that selects it.
//
// OoTMM rewrites six bytes of the table in every seed it generates (packages/generator/src/oot/patches/
// scene_restrictions.S, which has no PATCH_GROUP): the Gerudo Training Ground and inside Ganon's Castle allow warp
// songs and Farore's Wind, and Granny's Potion Shop and the Shooting Gallery allow the ocarina. Its addresses are
// bytes of OoT USA 1.0's table, at 0x800f7350, whose 4-byte entries are { scene, flags1, flags2, flags3 } in SoH's
// order; Interface_SetSceneRestrictions decodes each flags byte two bits per restriction, high bits first:
//   flags1: hGauge, bButton, aButton, bottles
//   flags2: tradeItems, hookshot, ocarina, warpSongs
//   flags3: sunsSong, farores, dinsNayrus, all
// OoTMM keeps vanilla's refusal of OoT's warp songs where warpSongs is 3 (oot/ocarina.S patches only the song test
// before it, at 0x800df1bc), which Ship's randomizer drops (z_message_PAL.c, `!IS_RANDO`). With OoTMM's table only
// the Windmill and Dampe's grave keep warpSongs 3.
//
// A seed records that it follows OoTMM's table in its "oot" section. Absence is Ship's table, so every seed and save
// made before the marker keeps SoH's behaviour. oot_scene_restrictions_reference_tests.py pins the bytes to OoTMM's
// file and oot_scene_restrictions_tests.py checks the table and the refusal in play.
#include "GameMode.h"

#include <cstdint>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace ComboRando {

// One byte OoTMM writes: field 1, 2 or 3 is the entry's flags1, flags2 or flags3. Scene ids are SoH's SceneID
// values (soh/include/tables/scene_table.h).
struct OotSceneRestrictionByte {
    uint32_t address; // in OoT USA 1.0's code, as scene_restrictions.S writes it
    uint8_t scene;
    uint8_t field;
    uint8_t value;
};
inline constexpr uint32_t kOotSceneRestrictionTableAddress = 0x800f7350;
inline constexpr OotSceneRestrictionByte kOotmmSceneRestrictionBytes[] = {
    // Warp songs: warpSongs 3 -> 0 (flags2 0x03 -> 0x00)
    { 0x800f7472, 0x0B, 2, 0x00 }, // SCENE_GERUDO_TRAINING_GROUND
    { 0x800f74ca, 0x0D, 2, 0x00 }, // SCENE_INSIDE_GANONS_CASTLE
    // Farore's Wind: farores 1 -> 0 (flags3 0x10 -> 0x00)
    { 0x800f7473, 0x0B, 3, 0x00 }, // SCENE_GERUDO_TRAINING_GROUND
    { 0x800f74cb, 0x0D, 3, 0x00 }, // SCENE_INSIDE_GANONS_CASTLE
    // Ocarina: ocarina 1 -> 0
    { 0x800f73e6, 0x4E, 2, 0x11 }, // SCENE_POTION_SHOP_GRANNY, flags2 0x15 -> 0x11
    { 0x800f73aa, 0x42, 2, 0x51 }, // SCENE_SHOOTING_GALLERY, flags2 0x55 -> 0x51
};
// Calls set(field, value) for each byte OoTMM writes in this scene's entry.
template <class Set> void ForEachOotmmSceneRestrictionByte(uint8_t scene, Set set) {
    for (const auto& patch : kOotmmSceneRestrictionBytes)
        if (patch.scene == scene)
            set(patch.field, patch.value);
}

inline constexpr const char* kOotSceneRestrictionsKey = "sceneRestrictions";
inline constexpr const char* kOotmmSceneRestrictionsMarker = "ootmm-32.3";
// A new seed's "oot" section, in a mode that randomizes OoT (OoTMM patches every seed; in an MM-only one OoT is not
// played). A reproduction keeps its seed's own marker or its absence (ComboRandoHeadless.cpp --settings).
inline void StampOotmmSceneRestrictions(nlohmann::json& oot) {
    oot[kOotSceneRestrictionsKey] = kOotmmSceneRestrictionsMarker;
}
// Whether an "oot" section has this build's marker. A missing or unknown one is Ship's table.
inline bool OotmmSceneRestrictionsMarked(const nlohmann::json& oot) {
    if (!oot.is_object())
        return false;
    const auto marker = oot.find(kOotSceneRestrictionsKey);
    return marker != oot.end() && marker->is_string() && *marker == kOotmmSceneRestrictionsMarker;
}
// Whether a slot's seed plays OoT with OoTMM's table: the marker, in a mode that randomizes OoT.
inline bool SeedUsesOotmmSceneRestrictions(const nlohmann::json& seed) {
    if (!seed.is_object() || !OotmmSceneRestrictionsMarked(seed.value("oot", nlohmann::json())))
        return false;
    try {
        return ModeRandomizesOot(ReadGameMode(seed));
    } catch (const std::exception&) { return false; }
}

} // namespace ComboRando
