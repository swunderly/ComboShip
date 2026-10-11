#include "SharedOcarina.h"
// combo/rando/SharedItems.h
// ComboShip: OoTMM-style Shared Items — one item counts for both games. Header-only table + mask
// helpers, compiled into exe, soh.dll, 2ship.dll, comborando. See docs/deviations/rando.md.
#pragma once

#include <algorithm>
#include <utility>
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ComboRando {

enum SharedFamily {
    SF_BOW = 0,
    SF_BOMB_BAG,
    SF_BOMBCHU_BAG,
    SF_MAGIC,
    SF_WALLET,
    SF_HOOKSHOT,
    SF_FIRE_ARROWS,
    SF_ICE_ARROWS,
    SF_LIGHT_ARROWS,
    SF_LENS,
    SF_EPONAS_SONG,
    SF_SONG_OF_STORMS,
    SF_GORON_MASK,
    SF_ZORA_MASK,
    SF_KEATON_MASK,
    SF_BUNNY_HOOD,
    SF_MASK_OF_TRUTH,
    SF_OCARINA_BUTTON_A,
    SF_OCARINA_BUTTON_C_DOWN,
    SF_OCARINA_BUTTON_C_RIGHT,
    SF_OCARINA_BUTTON_C_LEFT,
    SF_OCARINA_BUTTON_C_UP,
    SF_SONG_OF_TIME,
    SF_MIRROR_SHIELD,
    SF_OCARINA,    // append only; progressive family, excluded from singleton paths
    SF_SPELL_WIND, // the spells are public with their MM settings; the gear families stay private
    SF_SPELL_FIRE,
    SF_SPELL_LOVE,
    SF_BOOTS_IRON,
    SF_BOOTS_HOVER,
    SF_TUNIC_GORON,
    SF_TUNIC_ZORA, // append private bits28..31
    SF_COUNT
};

struct SharedFamilyDef {
    SharedFamily family;
    const char* key;     // spoiler/CLI key
    const char* cvar;    // menu CVar
    const char* label;   // menu checkbox label
    const char* tooltip; // OoTMM-style tooltip
    const char* ootName; // OOT pool item name
    const char* mmName;  // MM pool item name (trimmed / oracle-credited)
    int mmTierCap;       // max tiers MM can hold
    bool isMask;         // requires OOT Mask Quest = Shuffle
    bool mmHasItem;      // false = MM has no pool copy of this item (no trim, no oracle mirror)
    bool ootToMmOnly;    // true = one-way OOT->MM; MM's tier signal isn't a reliable OOT source
    bool requiresButtons = false;
    bool publicOption = true;
};

inline const SharedFamilyDef* SharedFamilyTable() {
    static const SharedFamilyDef table[SF_COUNT] = {
        { SF_BOW, "bows", "gCombo.Rando.Shared.Bows", "Shared Bows", "One Progressive Bow counts for both games.",
          "Progressive Bow", "Progressive Bow", 3, false, true, false },
        { SF_BOMB_BAG, "bombBags", "gCombo.Rando.Shared.BombBags", "Shared Bomb Bags",
          "One Progressive Bomb Bag counts for both games.", "Progressive Bomb Bag", "Progressive Bomb Bag", 3, false,
          true, false },
        { SF_BOMBCHU_BAG, "bombchuBags", "gCombo.Rando.Shared.BombchuBags", "Shared Bombchu Bags",
          "One Bombchu Bag counts for both games. Requires OOT Bombchu Bag != None.", "Bombchu Bag", "", 1, false,
          false, true },
        { SF_MAGIC, "magic", "gCombo.Rando.Shared.Magic", "Shared Magic",
          "One Progressive Magic Meter counts for both games.", "Progressive Magic Meter", "Progressive Magic", 2,
          false, true, false },
        { SF_WALLET, "wallets", "gCombo.Rando.Shared.Wallets", "Shared Wallets",
          "One Progressive Wallet counts for both games. Forces Shuffle Child Wallet off.", "Progressive Wallet",
          "Progressive Wallet", 3, false, true, false },
        { SF_HOOKSHOT, "hookshot", "gCombo.Rando.Shared.Hookshot", "Shared Hookshot",
          "One Progressive Hookshot counts for both games (MM only needs tier 1).", "Progressive Hookshot", "Hookshot",
          1, false, true, false },
        { SF_FIRE_ARROWS, "fireArrows", "gCombo.Rando.Shared.FireArrows", "Shared Fire Arrows",
          "One Fire Arrows counts for both games.", "Fire Arrows", "Fire Arrows", 1, false, true, false },
        { SF_ICE_ARROWS, "iceArrows", "gCombo.Rando.Shared.IceArrows", "Shared Ice Arrows",
          "One Ice Arrows counts for both games.", "Ice Arrows", "Ice Arrows", 1, false, true, false },
        { SF_LIGHT_ARROWS, "lightArrows", "gCombo.Rando.Shared.LightArrows", "Shared Light Arrows",
          "One Light Arrows counts for both games.", "Light Arrows", "Light Arrows", 1, false, true, false },
        { SF_LENS, "lens", "gCombo.Rando.Shared.Lens", "Shared Lens of Truth",
          "One Lens of Truth counts for both games.", "Lens of Truth", "Lens of Truth", 1, false, true, false },
        { SF_EPONAS_SONG, "eponasSong", "gCombo.Rando.Shared.EponasSong", "Shared Epona's Song",
          "One Epona's Song counts for both games.", "Epona's Song", "Epona's Song", 1, false, true, false },
        { SF_SONG_OF_STORMS, "songOfStorms", "gCombo.Rando.Shared.SongOfStorms", "Shared Song of Storms",
          "One Song of Storms counts for both games.", "Song of Storms", "Song of Storms", 1, false, true, false },
        { SF_GORON_MASK, "goronMask", "gCombo.Rando.Shared.GoronMask", "Shared Goron Mask",
          "One Goron Mask counts for both games. Requires OOT Mask Quest = Shuffle.", "Goron Mask", "Goron Mask", 1,
          true, true, false },
        { SF_ZORA_MASK, "zoraMask", "gCombo.Rando.Shared.ZoraMask", "Shared Zora Mask",
          "One Zora Mask counts for both games. Requires OOT Mask Quest = Shuffle.", "Zora Mask", "Zora Mask", 1, true,
          true, false },
        { SF_KEATON_MASK, "keatonMask", "gCombo.Rando.Shared.KeatonMask", "Shared Keaton Mask",
          "One Keaton Mask counts for both games. Requires OOT Mask Quest = Shuffle.", "Keaton Mask", "Keaton Mask", 1,
          true, true, false },
        { SF_BUNNY_HOOD, "bunnyHood", "gCombo.Rando.Shared.BunnyHood", "Shared Bunny Hood",
          "One Bunny Hood counts for both games. Requires OOT Mask Quest = Shuffle.", "Bunny Hood", "Bunny Hood", 1,
          true, true, false },
        { SF_MASK_OF_TRUTH, "maskOfTruth", "gCombo.Rando.Shared.MaskOfTruth", "Shared Mask of Truth",
          "One Mask of Truth counts for both games. Requires OOT Mask Quest = Shuffle.", "Mask of Truth",
          "Mask of Truth", 1, true, true, false },
        { SF_OCARINA_BUTTON_A, "ocarinaA", "gCombo.Rando.Shared.OcarinaA", "Shared Ocarina A Button",
          "One A Button counts for both games. Requires Shuffle Ocarina Buttons in both games.", "Ocarina A Button",
          "A Button", 1, false, true, false, true },
        { SF_OCARINA_BUTTON_C_DOWN, "ocarinaCDown", "gCombo.Rando.Shared.OcarinaCDown", "Shared Ocarina C Down Button",
          "One C Down Button counts for both games. Requires Shuffle Ocarina Buttons in both games.",
          "Ocarina C Down Button", "C Down Button", 1, false, true, false, true },
        { SF_OCARINA_BUTTON_C_RIGHT, "ocarinaCRight", "gCombo.Rando.Shared.OcarinaCRight",
          "Shared Ocarina C Right Button",
          "One C Right Button counts for both games. Requires Shuffle Ocarina Buttons in both games.",
          "Ocarina C Right Button", "C Right Button", 1, false, true, false, true },
        { SF_OCARINA_BUTTON_C_LEFT, "ocarinaCLeft", "gCombo.Rando.Shared.OcarinaCLeft", "Shared Ocarina C Left Button",
          "One C Left Button counts for both games. Requires Shuffle Ocarina Buttons in both games.",
          "Ocarina C Left Button", "C Left Button", 1, false, true, false, true },
        { SF_OCARINA_BUTTON_C_UP, "ocarinaCUp", "gCombo.Rando.Shared.OcarinaCUp", "Shared Ocarina C Up Button",
          "One C Up Button counts for both games. Requires Shuffle Ocarina Buttons in both games.",
          "Ocarina C Up Button", "C Up Button", 1, false, true, false, true },
        { SF_SONG_OF_TIME, "songOfTime", "gCombo.Rando.Shared.SongOfTime", "Shared Song of Time",
          "One Song of Time counts for both games, including starting songs.", "Song of Time", "Song of Time", 1, false,
          true, false },
        { SF_MIRROR_SHIELD, "mirrorShield", "gCombo.Rando.Shared.MirrorShield", "Shared Mirror Shield",
          "One Mirror Shield counts for both games. Shares the native nonprogressive shields.", "Mirror Shield",
          "Mirror Shield", 1, false, true, false },
        { SF_OCARINA, "ocarina", "gCombo.Rando.Shared.Ocarina", "Shared Ocarina",
          "Progressive Ocarina shared with pinned MM Fairy restrictions.", "Progressive Ocarina", "Progressive Ocarina",
          2, false, true, false },
        { SF_SPELL_WIND, "spellWind", "gCombo.Rando.Shared.SpellWind", "Shared Farore's Wind",
          "Combines Farore's Wind from OoT and MM into one item for both games", "Farore's Wind", "Farore's Wind", 1,
          false, true, false, false, true },
        { SF_SPELL_FIRE, "spellFire", "gCombo.Rando.Shared.SpellFire", "Shared Din's Fire",
          "Combines Din's Fire from OoT and MM into one item for both games", "Din's Fire", "Din's Fire", 1, false,
          true, false, false, true },
        { SF_SPELL_LOVE, "spellLove", "gCombo.Rando.Shared.SpellLove", "Shared Nayru's Love",
          "Combines Nayru's Love from OoT and MM into one item for both games", "Nayru's Love", "Nayru's Love", 1,
          false, true, false, false, true },
        { SF_BOOTS_IRON, "bootsIron", "gCombo.Rando.Shared.bootsIron", "Shared Iron Boots",
          "Combines the Iron Boots from OoT and MM into one item for both games", "Iron Boots", "Iron Boots", 1, false,
          true, false, false, false },
        { SF_BOOTS_HOVER, "bootsHover", "gCombo.Rando.Shared.bootsHover", "Shared Hover Boots",
          "Combines the Hover Boots from OoT and MM into one item for both games", "Hover Boots", "Hover Boots", 1,
          false, true, false, false, false },
        { SF_TUNIC_GORON, "tunicGoron", "gCombo.Rando.Shared.tunicGoron", "Shared Goron Tunic",
          "Combines the Goron Tunics from OoT and MM into two items for both games", "Goron Tunic", "Goron Tunic", 1,
          false, true, false, false, false },
        { SF_TUNIC_ZORA, "tunicZora", "gCombo.Rando.Shared.tunicZora", "Shared Zora Tunic",
          "Combines the Zora Tunics from OoT and MM into two items for both games", "Zora Tunic", "Zora Tunic", 1,
          false, true, false, false, false },
    };
    return table;
}

// OoTMM data.ts:3000-3054: each cross-game shared item has cond `hasOoTMM && <x>Mm`. The MM option that a
// family needs (its RO name), or nullptr when the family needs none.
inline const char* SharedFamilyMmExtension(int family) {
    switch (family) {
        case SF_SPELL_WIND:
            return "RO_MM_SPELL_WIND";
        case SF_SPELL_FIRE:
            return "RO_MM_SPELL_FIRE";
        case SF_SPELL_LOVE:
            return "RO_MM_SPELL_LOVE";
        case SF_BOOTS_IRON:
            return "RO_MM_BOOTS_IRON";
        case SF_BOOTS_HOVER:
            return "RO_MM_BOOTS_HOVER";
        case SF_TUNIC_GORON:
            return "RO_MM_TUNIC_GORON";
        case SF_TUNIC_ZORA:
            return "RO_MM_TUNIC_ZORA";
        default:
            return nullptr;
    }
}
// OoTMM's name of that MM setting (data.ts:2000-2054), or nullptr.
inline const char* SharedFamilyMmExtensionLabel(int family) {
    switch (family) {
        case SF_SPELL_WIND:
            return "Farore's Wind (MM)";
        case SF_SPELL_FIRE:
            return "Din's Fire (MM)";
        case SF_SPELL_LOVE:
            return "Nayru's Love (MM)";
        case SF_BOOTS_IRON:
            return "Iron Boots (MM)";
        case SF_BOOTS_HOVER:
            return "Hover Boots (MM)";
        case SF_TUNIC_GORON:
            return "Goron Tunic (MM)";
        case SF_TUNIC_ZORA:
            return "Zora Tunic (MM)";
        default:
            return nullptr;
    }
}

// A shared family is effective only when its MM extension is on in the MM dump (util.ts:80-93 resets it).
inline bool SharedExtraSpellEnabled(int family, const nlohmann::json& mmDump) {
    const char* key = SharedFamilyMmExtension(family);
    if (!key)
        return true;
    const auto options = mmDump.value("options", nlohmann::json::object());
    const auto i = options.find(key);
    return i != options.end() && i->is_number_integer() && *i == 1;
}
inline const SharedFamilyDef& SharedFamilyByIndex(int i) {
    return SharedFamilyTable()[i];
}

// New single-item families expose their native starting possessions in the dump.
// Native oracles apply their own starting inventory; this also credits the opposite
// world before its first query, including an MM starter while the portal is shut.
inline std::pair<std::vector<std::string>, std::vector<std::string>>
SharedStartingInventories(uint32_t mask, const std::string& ootDump, const std::string& mmDump) {
    std::pair<std::vector<std::string>, std::vector<std::string>> result;
    const auto oot = nlohmann::json::parse(ootDump.empty() ? "{}" : ootDump);
    const auto mm = nlohmann::json::parse(mmDump.empty() ? "{}" : mmDump);
    const auto ootItems = oot.value("sharedStartingItems", std::vector<std::string>{});
    const auto mmItems = mm.value("sharedStartingItems", std::vector<std::string>{});
    for (int i = SF_OCARINA_BUTTON_A; i < SF_COUNT; ++i) {
        if (i == SF_OCARINA)
            continue; // handled by tier-aware instrument adapter
        if (!SharedExtraSpellEnabled(i, mm))
            continue;
        const auto& def = SharedFamilyByIndex(i);
        if (!(mask & (1u << i)))
            continue;
        if (def.requiresButtons &&
            (!oot.value("sharedButtonsEnabled", false) || !mm.value("sharedButtonsEnabled", false)))
            continue;
        if (std::find(ootItems.begin(), ootItems.end(), def.ootName) != ootItems.end() ||
            std::find(mmItems.begin(), mmItems.end(), def.mmName) != mmItems.end()) {
            result.first.push_back(def.ootName);
            result.second.push_back(def.mmName);
        }
    }
    if (mask & (1u << SF_OCARINA)) {
        auto o = ootItems, m = mmItems;
        MirrorSharedOcarina(o, m, mm.value("fairyOcarinaMm", false));
        result.first.insert(result.first.end(), o.begin() + ootItems.size(), o.end());
        result.second.insert(result.second.end(), m.begin() + mmItems.size(), m.end());
    }
    return result;
}

inline void MirrorSharedSingletons(uint32_t mask, std::vector<std::string>& oot, std::vector<std::string>& mm) {
    for (int i = SF_OCARINA_BUTTON_A; i < SF_COUNT; ++i) {
        if (!(mask & (1u << i)))
            continue;
        if (i == SF_OCARINA)
            continue; // handled by tier-aware instrument adapter
        const auto& def = SharedFamilyByIndex(i);
        const bool o = std::find(oot.begin(), oot.end(), def.ootName) != oot.end();
        const bool m = std::find(mm.begin(), mm.end(), def.mmName) != mm.end();
        if (o && !m)
            mm.push_back(def.mmName);
        if (m && !o)
            oot.push_back(def.ootName);
    }
}

inline uint32_t SharedMaskFromKeys(const nlohmann::json& keys) {
    uint32_t mask = 0;
    if (!keys.is_array())
        return mask;
    for (const auto& k : keys) {
        if (!k.is_string())
            continue;
        const std::string s = k.get<std::string>();
        for (int i = 0; i < SF_COUNT; ++i)
            if (s == SharedFamilyTable()[i].key)
                mask |= (1u << i);
    }
    return mask;
}

inline uint32_t SharedMaskAll() {
    return (SF_COUNT >= 32) ? 0xFFFFFFFFu : static_cast<uint32_t>((uint64_t{ 1 } << SF_COUNT) - 1u);
}

// Names to leave untagged by SuffixCrossGameItems: both games' pool name for every effective family.
inline std::set<std::string> SharedUntaggedNames(uint32_t mask) {
    std::set<std::string> out;
    for (int i = 0; i < SF_COUNT; ++i) {
        if (!(mask & (1u << i)))
            continue;
        out.insert(SharedFamilyTable()[i].ootName);
        if (SharedFamilyTable()[i].mmHasItem)
            out.insert(SharedFamilyTable()[i].mmName);
    }
    if (mask & (1u << SF_OCARINA)) {
        out.insert("Fairy Ocarina");
        out.insert("Ocarina of Time");
    }
    return out;
}

inline std::vector<std::string> SharedKeysFromMask(uint32_t mask) {
    std::vector<std::string> out;
    for (int i = 0; i < SF_COUNT; ++i)
        if (mask & (1u << i))
            out.push_back(SharedFamilyTable()[i].key);
    return out;
}

} // namespace ComboRando
