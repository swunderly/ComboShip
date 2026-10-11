#pragma once
#include <cstdint>
#include <string_view>

namespace ComboRando {
struct MmTrickDefinition {
    int32_t id;
    const char* key;
    const char* name;
    const char* description;
    const char* area;
    const char* tags;
    int32_t difficulty;
    uint32_t tagMask;
    bool publicOption = true;
};
inline constexpr MmTrickDefinition kMmTricks[] = {
    { 0, "gCombo.Tricks.MM.Lensless", "Fewer Lens Requirements",
      "Traverse remembered invisible platforms and collect invisible chests without the Lens of Truth. Following "
      "Darmani and seeing Shiro still require the Lens.",
      "Overworld and dungeons", "Lens, navigation", 1, 1u << 1 },
    { 1, "gCombo.Tricks.MM.DarmaniWall", "Climb to Darmani without the Lens",
      "Climb the hidden wall using its fixed collision. This reaches the graveyard; following and healing Darmani "
      "still require the Lens.",
      "Mountain Village", "Lens, navigation", 2, 1u << 2 },
    { 2, "gCombo.Tricks.MM.PinnacleNoSeahorse", "Pinnacle Rock without the Seahorse",
      "Follow the signs through Pinnacle Rock's murky water without the guide. The Zora Mask and swimming ability "
      "remain required. Reuniting the seahorses still requires the actual Seahorse and defeating the Deep Pythons.",
      "Pinnacle Rock", "Navigation, swimming", 2, 1u << 2 },
    { 3, "gCombo.Tricks.MM.PostOfficeTiming", "Post Office timing without Bunny Hood",
      "Stop the Postman's timer at ten seconds without seeing the timer. The game must be available during its usual "
      "day-one hours, or day-two evening after mailing the Letter to Kafei. This does not win the game automatically.",
      "West Clock Town", "Minigames, timing", 2, 1u << 2 },
    { 4, "gCombo.Tricks.MM.OceansideCode", "Guess Oceanside Spider House code",
      "Try the colored-mask arrow sequences without learning the clues from the Stalchildren. The Bow and access to "
      "the colored-skull room remain required; the Captain's Hat is not required for this chest.",
      "Oceanside Spider House", "Puzzle, bow", 1, 1u << 1 },
    { 5, "gCombo.Tricks.MM.ClockTownLowerJump", "South Clock Town lower chest with a precise jump",
      "Climb the roof with a precise human jump to reach the lower chest without the Deku flower or Hookshot. This "
      "applies only to the lower chest; the upper chest retains its ordinary access and final-day requirement.",
      "South Clock Town", "Movement, precise jump", 2, 1u << 2 },
    { 6, "gCombo.Tricks.MM.DoggyFenceJump", "Doggy Racetrack chest with a fence jump",
      "Climb the fence and jump precisely to the chest as human Link without beans, the Hookshot or the Zora Mask. "
      "Reaching the racetrack remains required, and this does not affect winning the dog race.",
      "Doggy Racetrack", "Movement, precise jump", 2, 1u << 2 },
    { 7, "gCombo.Tricks.MM.PalaceBeanSkip", "Skip planting beans in Deku Palace",
      "Backflip onto the west garden doorway and make an angled roll jump from the lower palace to the upper cell-side "
      "ledge. This removes the bean and watering requirement for that climb. Crossing the upper palace still requires "
      "the Deku Mask and its Mad Scrub combat requirements.",
      "Deku Palace", "Movement, precise jump", 2, 1u << 2 },
    { 8, "gCombo.Tricks.MM.IkanaIceSkip", "Climb Ikana Canyon without Ice Arrows",
      "Aim the Hookshot from a precise position on the riverside to reach the first tree without making an ice "
      "platform. The Hookshot and access to lower Ikana Canyon remain required.",
      "Ikana Canyon", "Hookshot, precise aim", 2, 1u << 2 },
    { 9, "gCombo.Tricks.MM.StoneTowerOneMask", "Climb Stone Tower with one transformation mask",
      "Use human Link's statue plus a Goron or Zora statue, and ride the moving blocks while managing Elegy statues to "
      "reach the tower's top and owl area. Requires the Hookshot and a playable Elegy of Emptiness, including its "
      "instrument and notes. A Deku Mask alone does not suffice. Moving the temple bridge itself requires only Elegy.",
      "Stone Tower", "Elegy, movement", 3, 1u << 3 },
    { 10, "gCombo.Tricks.MM.DekuStickFighting", "Fight enemies with Deku Sticks",
      "Allow Deku Stick combat where OoTMM expects this technique. A renewable stick source is still required. This "
      "does not change normal stick use for torches or owl statues.",
      "Overworld and dungeons", "Combat, sticks", 1, 1u << 1 },
    { 11, "gCombo.Tricks.MM.TwinmoldBow", "Defeat Twinmold with the Bow",
      "Aim ordinary arrows at both worms' heads and tails without the Giant's Mask. Requires a renewable arrow source.",
      "Stone Tower Temple", "Combat, bow", 2, 1u << 2 },
    { 12, "gCombo.Tricks.MM.TwinmoldFireIce", "Defeat Twinmold with Fire and Ice Arrows",
      "Use Fire Arrows on the blue worm and Ice Arrows on the red worm without the Giant's Mask. Requires arrows and "
      "magic.",
      "Stone Tower Temple", "Combat, magic arrows", 1, 1u << 1 },
    { 13, "gCombo.Tricks.MM.RanchFarore", "Ranch events with Farore's Wind",
      "Return to a recorded ranch point across the cycle; the requested half-day clock, Romani's soul and the event's "
      "equipment remain required.",
      "Romani Ranch", "Farore, clocks", 2, 1u << 2, false },
    // OoTMM tricks.ts:598-601 MM_GBT_CENTRAL_GEYSER.
    { 14, "gCombo.Tricks.MM.GbtCentralGeyser",
      "GBT Central Room without Zora using Fire & Ice Arrows or an OoT Magic Spell",
      "Using Fire and Ice Arrows on the water stream above the ladder or an OoT Spell while on the spinning platform, "
      "it is possible to sink down during the cutscene, then swim into one of the tunnels.",
      "Great Bay Temple", "Farore, movement", 2, 1u << 2, false },
    // OoTMM tricks.ts:679-682 MM_LULLABY_SKIP_IRONS (snowhead.yml:7-8). The key is unchanged, so stored seeds and
    // profiles keep their value.
    { 15, "gCombo.Tricks.MM.LullabySkipIrons", "Skip playing Goron Lullaby by using Iron Boots",
      "By combining Iron Boots to not be blown away by the wind and Goron to roll up the slope, Goron Lullaby can be "
      "skipped.",
      "Snowhead", "Iron Boots, movement", 2, 1u << 2 },
    { 16, "gCombo.Tricks.MM.ZoraHallScrubNoDeku", "Zora Hall Scrub heart without Deku",
      "Pinned Goron or tall-form jump reaches the heart without Deku. Hidden until actual Adult gameplay is validated.",
      "Zora Hall", "Movement, masks", 2, 1u << 2, false },
    // OoTMM tricks.ts:644-647 MM_IGOS_DINS (macros_mm.yml:401 can_kill_igos).
    { 17, "gCombo.Tricks.MM.IgosDins", "Burn Igos' curtain with Din's Fire",
      "Igos' left curtain can be burned by standing at the top of the stairs next to his throne.",
      "Ancient Castle of Ikana", "Combat, Din's Fire", 2, 1u << 2 },
    // OoTMM tricks.ts:740-743 MM_STAGE_LIGHTS_DIN (zora_hall.yml:19).
    { 18, "gCombo.Tricks.MM.StageLightsDin", "Light the Zora Hall Stage Lights using Din's Fire, Bow, and Hookshot",
      "The item can be obtained without Fire Arrows by first reaching either torch with the hookshot, using Din's "
      "Fire, then shooting an arrow to the other torch.",
      "Zora Hall", "Din's Fire, bow, hookshot", 2, 1u << 2 },
    // OoTMM tricks.ts:372-375 MM_TUNICS (macros_mm.yml:76-77 has_tunic_goron, has_tunic_zora).
    { 19, "gCombo.Tricks.MM.Tunics", "Fewer Tunic Requirements (MM)",
      "Most things that would normally require Zora Tunic no longer will. Pirate Fortress Sewers and Pinnacle Rock "
      "will still require it.",
      "Overworld and dungeons", "Zora Tunic, Iron Boots", 2, 1u << 2 },
    // OoTMM tricks.ts:709-712 MM_GYORG_IRONS (macros_mm.yml:399 can_kill_gyorg).
    { 20, "gCombo.Tricks.MM.GyorgIrons", "Fight Gyorg as Human using Iron Boots and Hookshot",
      "Gyorg can be damaged with Hookshot. Turning this trick on will enable the fight in logic with Iron Boots and "
      "Hookshot.",
      "Great Bay Temple", "Combat, Iron Boots, hookshot", 2, 1u << 2 },
    // OoTMM tricks.ts:659-662 MM_WF_SHRINE_HOVERS (woodfall.yml:42).
    { 21, "gCombo.Tricks.MM.WfShrineHovers", "Woodfall Owl Chest with Hover Boots and Jump Slash",
      "The chest can be reached with a tight jump slash at the end of the hover duration.", "Woodfall",
      "Hover Boots, jump slash", 2, 1u << 2 },
    // OoTMM tricks.ts:664-667 MM_WFT_LOBBY_HOVERS (woodfall_temple.yml:16).
    { 22, "gCombo.Tricks.MM.WftLobbyHovers", "Woodfall Temple Lobby with Damage Boost and Hover Boots",
      "The lobby can be traversed by damaging yourself with bombs at certain points. Very difficult.",
      "Woodfall Temple", "Hover Boots, damage boost", 2, 1u << 2 },
    // OoTMM tricks.ts:674-677 MM_SOARING_HOVERS (southern_swamp.yml:300).
    { 23, "gCombo.Tricks.MM.SoaringHovers",
      "Jump Slash or Damage Boost to the Soaring Tablet with Hover Boots and Bunny Hood",
      "Using Bunny Hood for extra speed, Hover Boots can be used to reach the Soaring Tablet with a tight jump slash "
      "or a damage boost with explosives.",
      "Southern Swamp", "Hover Boots, Bunny Hood", 2, 1u << 2 },
    // OoTMM tricks.ts:684-687 MM_PATH_SNOWHEAD_HOVERS (path_to_snowhead.yml:7, :28).
    { 24, "gCombo.Tricks.MM.PathSnowheadHovers", "Cross Path to Snowhead using Hover Boots",
      "Hover Boots can be used to slide off the slopes on the side and get across the gaps.", "Path to Snowhead",
      "Hover Boots, movement", 2, 1u << 2 },
    // OoTMM tricks.ts:689-692 MM_GBT_WATERWHEEL_HOVERS (great_bay_temple.yml:37).
    { 25, "gCombo.Tricks.MM.GbtWaterwheelHovers", "Cross GBT Waterwheel Room using Hover Boots",
      "Similar to using Goron, Hover Boots can be used to get on the water wheel and then reach the central room.",
      "Great Bay Temple", "Hover Boots, movement", 2, 1u << 2 },
    // OoTMM tricks.ts:694-697 MM_GBT_CENTER_POT_IRONS (great_bay_temple.yml:62).
    { 26, "gCombo.Tricks.MM.GbtCenterPotIrons", "Great Bay Temple Center Underwater Pot using only Iron Boots",
      "The pot can be broken by rolling into it, allowing you to collect the Stray Fairy.", "Great Bay Temple",
      "Iron Boots, Zora Tunic", 2, 1u << 2 },
    // OoTMM tricks.ts:699-702 MM_GBT_RED1_HOVERS (great_bay_temple.yml:135).
    { 27, "gCombo.Tricks.MM.GbtRed1Hovers",
      "Reach the First Red Turnkey in GBT using Hover Boots, Bunny Hood, and a Jump Slash",
      "The red turnkey in front of Wart can be reached with a precise jump slash after the hover duration, using "
      "Bunny Hood for extra speed.",
      "Great Bay Temple", "Hover Boots, Bunny Hood, jump slash", 2, 1u << 2 },
    // OoTMM tricks.ts:704-707 MM_GBT_GREEN2_UPPER_HOVERS (great_bay_temple.yml:252).
    { 28, "gCombo.Tricks.MM.GbtGreen2UpperHovers", "Reach the Upper Chest in GBT's Second Green Room using Hover Boots",
      "The upper chest in the green water wheel room can be reached by using Hover Boots to move between the wheel's "
      "blades.",
      "Great Bay Temple", "Hover Boots, movement", 2, 1u << 2 },
    // OoTMM tricks.ts:714-717 MM_STT_LAVA_BLOCK_HOVERS (stone_tower_temple.yml:65).
    { 29, "gCombo.Tricks.MM.SttLavaBlockHovers", "Stone Tower Temple Map Chest using Hover Boots",
      "After using the Hookshot to reach the chest on the high platform in the lava room, one can then use the Hover "
      "Boots to land on top of the sun block.",
      "Stone Tower Temple", "Hover Boots, hookshot", 2, 1u << 2 },
    // OoTMM tricks.ts:719-722 MM_ISTT_ENTRY_HOVER (stone_tower_temple_inverted.yml:16).
    { 30, "gCombo.Tricks.MM.IsttEntryHover", "Inverted Stone Tower Temple Death Armos using Hover Boots and Bunny Hood",
      "The Death Armos switch can be reached by using the Hover Boots and then sidehopping onto the platform, with "
      "Bunny Hood for extra speed.",
      "Stone Tower Temple", "Hover Boots, Bunny Hood", 2, 1u << 2 },
    // OoTMM tricks.ts:583-586 MM_ST_UPDRAFTS (stone_tower_temple.yml:176-187, stone_tower_temple_inverted.yml:33).
    { 31, "gCombo.Tricks.MM.StUpdrafts", "Stone Tower Updrafts without Deku Mask",
      "This room can be traversed using Recoil Flips or Goron Mask instead of Deku Mask.", "Stone Tower Temple",
      "Movement, updrafts", 2, 1u << 2 },
};
inline const MmTrickDefinition* FindMmTrick(int32_t id) {
    for (const auto& trick : kMmTricks)
        if (trick.id == id)
            return &trick;
    return nullptr;
}
// Hidden rows are not validated in play yet: no seed, profile or saved CVar may switch one on, and
// the menus offer no way to switch it off again. Only making the row public enables it. constexpr,
// so a literal key (MM logic's trick macro) can be decided at compile time.
inline constexpr bool IsHiddenMmTrick(std::string_view key) {
    for (const auto& trick : kMmTricks)
        if (key == trick.key)
            return !trick.publicOption;
    return false;
}
inline constexpr bool CanNavigateHidden(bool lensOwned, bool magicOwned, bool lenslessEnabled) {
    return (lensOwned && magicOwned) || lenslessEnabled;
}
} // namespace ComboRando
