#pragma once
#include <algorithm>
#include <cstring>
#include <string>
#include <nlohmann/json.hpp>
#include "MmTricks.h"

namespace ComboRando {
// Explicit, versioned allowlist: loading a seed never imports arbitrary CVars or visual preferences.
// Only add a rule after auditing its runtime/default and documenting its current logic coverage.
struct GameplayRule {
    const char* name;
    const char* label;
    int fallback;
    int maximum;
    const char* help;
    int minimum = 0;
    bool trickGoverned = false;
    bool runtimeOnly = false;
};
inline constexpr GameplayRule kGameplayRules[] = {
    { "gEnhancements.BlueFireArrows", "Blue Fire Arrows (OoT)", 0, 1,
      "Ice Arrows melt red ice. Logic still requires usable Ice Arrows, a bow and magic; bottles of blue fire remain "
      "valid alternatives." },
    { "gEnhancements.SunlightArrows", "Sunlight Arrows (OoT)", 0, 1,
      "Light Arrows activate sun switches. Logic still requires usable Light Arrows, a bow and magic." },
    { "gEnhancements.OpenAllHours", "Shops open at any hour (OoT)", 0, 1,
      "Remove day/night restrictions from shop doors. Age and NPC requirements still apply. Locked-overworld-door mode "
      "retains its key and time rules." },
    { "gEnhancements.DifficultyOptions.GibdoTradeSequence", "Gibdo trades (MM)", 0, 2,
      "MM3D accepts red or blue potion; No trade bypasses requested resources. The Gibdo Mask and room lighting are "
      "still required." },
    { "gEnhancements.DifficultyOptions.FrogChoirCount", "Frogs required for choir (MM)", 5, 5,
      "Choose how many frogs the choir requires, counting the yellow frog already there. Logic counts the exact number "
      "returned and still requires the Don Gero Mask and spring.",
      1 },
    { "gEnhancements.Minigames.AlwaysWinDoggyRace", "Dog race results (MM)", 0, 2,
      "With Mask of Truth preserves the mask requirement; Always guarantees a winning dog without it. Logic retains "
      "ranch access and enough rupees to enter." },
    { "gEnhancements.Player.InfiniteDekuHopping", "Unlimited Deku water hops (MM)", 0, 1,
      "Unlimited surface hopping. Logic permits verified returns from Great Bay's lab exterior, cliffside and pirate "
      "ledge to the low beach with the Deku Mask; it does not assume climbing platforms or diving." },
    { "gEnhancements.DayGravePull", "Pull ordinary graves during the day (OoT)", 0, 1,
      "Child Link can pull ordinary Graveyard graves during the day. Logic adds Shield and Heart Piece grave access "
      "and retains strength, chest and Sun's Song requirements; Dampe's race remains age-specific." },
    { "gEnhancements.NightGSAlwaysSpawn", "Night Skulltulas during the day (OoT)", 0, 1,
      "Logic permits daytime collection for the verified Graveyard wall, Fountain log, Fortress roof and range, "
      "Kakariko and Ranch nighttime checks. Age, access, combat and collection equipment still apply; these checks no "
      "longer need Sun's Song when this rule is included. Unaudited spawns retain their normal night rules." },
    { "gEnhancements.RestoreShadowDoorUnload", "Shadow Door Unload", 0, 1,
      "After reaching the Graveyard warp pad, complete five full staircase unload/reload cycles in one visit to leave "
      "the Shadow Door unloaded. Both ages can use this shortcut without Din's Fire. Enable this technique under "
      "Randomizer / Tricks & glitches to let the seed require it. The restoration alone never adds a logic assumption. "
      "Leaving the Graveyard resets it; disabling restores the door on its next reload.",
      0, true },
    { "gEnhancements.EarlyEyeballFrog", "Early Eyeball Frog (restoration)", 0, 1,
      "After thawing King Zora, Adult Link can hold R with a usable equipped shield and present an earlier adult-trade "
      "item to receive the randomized Eyeball Frog reward. Pocket Egg, Pocket Cucco, Cojiro, Odd Mushroom, Odd Potion, "
      "Poacher's Saw and Broken Sword qualify; empty slots and later trade items do not. Speaking to Zora and thawing "
      "requirements still apply. Select this technique under Randomizer / Tricks & glitches to let the seed require "
      "it; enabling the restoration alone does not change logic.",
      0, true },
    { "gCheats.SuperTunic", "All tunics protect against hazards (OoT)", 0, 1,
      "All tunics protect against heat and underwater timers. Logic retains swimming, depth, boots, age and other "
      "equipment requirements." },
    { "gCheats.DekuStick", "Deku stick behavior (OoT)", 0, 2,
      "Only Always burning supplies its own flame on usable sticks. Logic includes audited ground torches and "
      "cow-grotto webs; distant or high fire targets still require their normal fire source." },
    { "gEnhancements.TimeTravel", "Song of Time age change (OoT)", 0, 4,
      "Changes age at your current position, away from other song interactions. Requires Song of Time and the selected "
      "equipment. Logic includes audited overworld areas, ordinary houses and shops, the laboratory, the dry well, "
      "Water Temple's dry entrance ledge, and dry vanilla Spirit Temple, Forest Temple and Training Ground rooms. "
      "Compatible eyes and switches can use Child Link's Slingshot; Bow-only targets still need a Bow. Unsafe "
      "interiors, underwater positions, boss arenas and unaudited dungeon shortcuts are not assumed." },
    { "gEnhancements.Masks.FierceDeitysAnywhere", "Fierce Deity anywhere (MM)", 0, 1,
      "Allows the mask outside boss rooms. Logic counts its sword, measured shallow-water traversal and magic sword "
      "beams against Big Octos when enabled and owned. Boss-room use remains available without this setting. Extra "
      "jumps and other movement shortcuts are not yet required by logic." },
    { "gEnhancements.DifficultyOptions.BremenChickCount", "Chicks required for Bremen Mask (MM)", 10, 10,
      "Choose how many chicks must follow your march before Grog's flock grows up. Ten is the original requirement. "
      "The Bremen Mask and Cucco Shack access are still required. This changes the quest length, not seed logic.",
      1, false, true },
    { "gEnhancements.Masks.BremenOneChick", "Bremen Mask: one chick (MM)", 0, 1,
      "Recruit one chick while marching to complete Grog's flock. The randomizer still requires the Bremen Mask and "
      "access to the Cucco Shack.",
      0, false, true },
    { "gEnhancements.Songs.ZoraEggCount", "Zora eggs required for Bossa Nova (MM)", 7, 7,
      "Choose how many distinct accessible eggs must be deposited for the lesson. Logic keeps a bottle, Zora form and "
      "the local instrument; a lower count can avoid otherwise required egg areas.",
      1 },
    { "gCombo.Enhancements.ForeignSongs", "Play warp songs across games", 0, 1,
      "Play learned OoT warp songs in MM using MM's instrument and playable song buttons. Play learned Soaring in OoT "
      "using OoT's instrument and D/L/U buttons, with activated MM owls as destinations. Both Child and Adult Link can "
      "use Soaring. With no owls, the song still performs and explains that there are no destinations." },
    { "gCombo.Enhancements.AgelessSoaring", "Song of Soaring at any age (OoT)", 0, 1,
      "Adult Link can play learned Soaring in OoT. Also requires Play warp songs across games, a usable local "
      "instrument and the D/L/U song buttons. Routes still require an activated source owl; no activated owls produces "
      "the native no-destination message. Enable both options in logic to let the seed require Adult Link's Soaring "
      "routes." },
    { "gCombo.Enhancements.SongTravel", "Use saved mixed warp destinations", 0, 1,
      "Use the warp-song and owl destination map saved with this seed. Randomizer creates a shuffled map for a new "
      "seed. Switching this on never reshuffles an existing run." },
    { "gEnhancements.Masks.GoronRollingIgnoresMagic", "Goron rolling without magic (MM)", 0, 1,
      "Spiked Goron rolling no longer requires magic. Logic still requires the Goron Mask and any other requirements "
      "of the route." },
    { "gEnhancements.Masks.BlastMaskKeg", "Blast Mask acts as a powder keg (MM)", 0, 1,
      "Blast Mask explosions can break powder-keg barriers. Logic allows the owned Blast Mask as an alternative; "
      "access and other check requirements still apply." },
    { "gEnhancements.Playback.SkipScarecrowSong", "Scarecrow without teaching a song (MM)", 0, 1,
      "Summon the Scarecrow without teaching it first. Still requires the local instrument, Hookshot and two different "
      "playable notes. With this rule off, logic must reach a Scarecrow that can be taught." },
    { "gEnhancements.Timesavers.SkipBalladOfWindfish", "Shorter Ballad of the Wind Fish (MM)", 0, 1,
      "Play the complete Ballad after playing in one form if you have all three transformation masks.", 0, false,
      true },
};

inline bool IsLegacyGameplayRule(const char* name) {
    return name && (!std::strcmp(name, "gEnhancements.Masks.BremenOneChick") ||
                    !std::strcmp(name, "gCombo.Enhancements.AgelessSoaring"));
}
inline bool IsBremenCount(const char* name) {
    return name && !std::strcmp(name, "gEnhancements.DifficultyOptions.BremenChickCount");
}
template <class Read> int ReadGameplayRule(const GameplayRule& rule, bool logical, Read read);

inline std::string LogicRuleKey(const char* runtime) {
    if (!std::strcmp(runtime, "gEnhancements.BlueFireArrows"))
        return "gRandoSettings.BlueFireArrows";
    if (!std::strcmp(runtime, "gEnhancements.SunlightArrows"))
        return "gRandoSettings.SunlightArrows";
    if (!std::strcmp(runtime, "gCombo.Enhancements.ForeignSongs"))
        return "gCombo.Rando.ForeignSongs";
    if (!std::strcmp(runtime, "gCombo.Enhancements.SongTravel"))
        return "gCombo.Rando.SongTravel";
    return std::string("gCombo.Logic.") + runtime;
}
inline const GameplayRule* FindGameplayRule(const char* name) {
    if (!name)
        return nullptr;
    for (const auto& rule : kGameplayRules)
        if (!std::strcmp(name, rule.name) || LogicRuleKey(rule.name) == name)
            return &rule;
    return nullptr;
}

template <class Read> int ReadGameplayRule(const GameplayRule& rule, bool logical, Read read) {
    if (logical && rule.runtimeOnly)
        return 0;
    int fallback = logical ? 0 : rule.fallback;
    if (IsBremenCount(rule.name)) {
        const char* legacy =
            logical ? "gCombo.Logic.gEnhancements.Masks.BremenOneChick" : "gEnhancements.Masks.BremenOneChick";
        if (read(legacy, 0) == 1)
            fallback = 1;
    }
    return std::clamp(int(read(logical ? LogicRuleKey(rule.name).c_str() : rule.name, fallback)),
                      logical ? 0 : rule.minimum, rule.maximum);
}
// Convert the old one-chick toggle only when no count was saved. Explicit new counts win.
inline nlohmann::json UpgradeGameplayRules(nlohmann::json block) {
    if (!block.is_object())
        return block;
    for (const char* field : { "values", "logicValues" }) {
        if (!block.contains(field) || !block[field].is_object())
            continue;
        auto& values = block[field];
        if (values.contains("gEnhancements.DifficultyOptions.BremenChickCount"))
            continue;
        const auto old = values.find("gEnhancements.Masks.BremenOneChick");
        if (old == values.end() || !old->is_number_integer() || (*old != 0 && *old != 1))
            continue;
        values["gEnhancements.DifficultyOptions.BremenChickCount"] = *old == 1                       ? 1
                                                                     : !std::strcmp(field, "values") ? 10
                                                                                                     : 0;
    }
    return block;
}

// Randomizer enables both the actual ability and its model. Enhancement-only edits leave
// generation independent; changing/removing an ability clears a now-incompatible logic value.
template <class Write> void SetLogicRule(const GameplayRule& rule, int value, Write write) {
    if (rule.runtimeOnly) {
        write(LogicRuleKey(rule.name).c_str(), 0);
        return;
    }
    value = std::clamp(value, 0, rule.maximum);
    write(LogicRuleKey(rule.name).c_str(), value);
    write(rule.name, value < rule.minimum ? rule.fallback : value);
}
template <class Read, class Write>
void SetEnhancementRule(const GameplayRule& rule, int value, Read read, Write write) {
    value = std::clamp(value, rule.minimum, rule.maximum);
    write(rule.name, value);
    auto key = LogicRuleKey(rule.name);
    int logic = read(key.c_str(), 0);
    if (rule.runtimeOnly || !value || (logic && logic != value))
        write(key.c_str(), 0);
}

// A restoration capability and permission to require its technique are separate.
// Only the normal Tricks & glitches checkbox adds this assumption. Removing the
// assumption leaves the independently selected restoration available to play.
template <class Write> void SetTrickGovernedRule(const GameplayRule& rule, bool enabled, Write write) {
    if (!rule.trickGoverned)
        return;
    if (enabled)
        SetLogicRule(rule, 1, write);
    else
        write(LogicRuleKey(rule.name).c_str(), 0);
}

template <class Read> nlohmann::json CaptureGameplayRules(Read read) {
    nlohmann::json values = nlohmann::json::object();
    nlohmann::json logic = nlohmann::json::object();
    for (const auto& rule : kGameplayRules) {
        values[rule.name] = ReadGameplayRule(rule, false, read);
        logic[rule.name] = ReadGameplayRule(rule, true, read);
    }
    nlohmann::json tricks = nlohmann::json::object();
    for (const auto& trick : kMmTricks)
        tricks[trick.key] = int(trick.publicOption && read(trick.key, 0) != 0); // see IsHiddenMmTrick
    return { { "version", 2 }, { "values", values }, { "logicValues", logic }, { "mmTricks", tricks } };
}
template <class Write> void ApplyGameplayRules(const nlohmann::json& input, Write write) {
    const auto rules = UpgradeGameplayRules(input);
    if (!rules.is_object() || !rules.contains("version") || (rules["version"] != 1 && rules["version"] != 2) ||
        !rules.contains("values") || !rules["values"].is_object())
        return;
    for (const auto& rule : kGameplayRules) {
        const auto it = rules["values"].find(rule.name);
        if (it == rules["values"].end()) {
            write(rule.name, rule.fallback);
            write(LogicRuleKey(rule.name).c_str(), 0);
            continue;
        }
        if (!it->is_number_integer())
            continue;
        // Reject out-of-range values before narrowing; malformed imports cannot wrap into valid rules.
        if (*it < rule.minimum || *it > rule.maximum)
            continue;
        write(rule.name, it->get<int>());
        int logical = rule.trickGoverned ? 0 : it->get<int>(); // legacy restoration alone cannot select a trick
        if (rules["version"] == 2) {
            logical = 0;
            if (rules.contains("logicValues") && rules["logicValues"].is_object()) {
                auto l = rules["logicValues"].find(rule.name);
                if (l != rules["logicValues"].end() && l->is_number_integer() && *l >= 0 && *l <= rule.maximum)
                    logical = l->get<int>();
            }
        }
        write(LogicRuleKey(rule.name).c_str(), rule.runtimeOnly ? 0 : logical);
    }
    for (const auto& trick : kMmTricks) {
        int enabled = 0;
        if (rules.contains("mmTricks") && rules["mmTricks"].is_object()) {
            auto it = rules["mmTricks"].find(trick.key);
            if (it != rules["mmTricks"].end() && it->is_number_integer() && (*it == 0 || *it == 1))
                enabled = it->get<int>();
        }
        write(trick.key, trick.publicOption ? enabled : 0); // a hidden trick is cleared, never imported
    }
}

// Matches TimeTravelType and the gameplay hook's equipment checks. Callers separately enforce
// whether the local scene/position supports playing Song of Time without another song interaction.
inline bool CanChangeAgeWithSong(int mode, bool canPlaySong, bool hasOot, bool hasMasterSword) {
    if (!canPlaySong)
        return false;
    switch (mode) {
        case 1:
            return hasOot;
        case 2:
            return hasOot && hasMasterSword;
        case 3:
            return true;
        case 4:
            return hasMasterSword;
        default:
            return false;
    }
}
} // namespace ComboRando
