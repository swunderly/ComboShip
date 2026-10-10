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
    bool publicOption=true;
};
inline constexpr MmTrickDefinition kMmTricks[] = {
    {0, "gCombo.Tricks.MM.Lensless", "Fewer Lens Requirements",
     "Traverse remembered invisible platforms and collect invisible chests without the Lens of Truth. Following Darmani and seeing Shiro still require the Lens.",
     "Overworld and dungeons", "Lens, navigation", 1, 1u << 1},
    {1, "gCombo.Tricks.MM.DarmaniWall", "Climb to Darmani without the Lens",
     "Climb the hidden wall using its fixed collision. This reaches the graveyard; following and healing Darmani still require the Lens.",
     "Mountain Village", "Lens, navigation", 2, 1u << 2},
    {2, "gCombo.Tricks.MM.PinnacleNoSeahorse", "Pinnacle Rock without the Seahorse",
     "Follow the signs through Pinnacle Rock's murky water without the guide. The Zora Mask and swimming ability remain required. Reuniting the seahorses still requires the actual Seahorse and defeating the Deep Pythons.",
     "Pinnacle Rock", "Navigation, swimming", 2, 1u << 2},
    {3, "gCombo.Tricks.MM.PostOfficeTiming", "Post Office timing without Bunny Hood",
     "Stop the Postman's timer at ten seconds without seeing the timer. The game must be available during its usual day-one hours, or day-two evening after mailing the Letter to Kafei. This does not win the game automatically.",
     "West Clock Town", "Minigames, timing", 2, 1u << 2},
    {4, "gCombo.Tricks.MM.OceansideCode", "Guess Oceanside Spider House code",
     "Try the colored-mask arrow sequences without learning the clues from the Stalchildren. The Bow and access to the colored-skull room remain required; the Captain's Hat is not required for this chest.",
     "Oceanside Spider House", "Puzzle, bow", 1, 1u << 1},
    {5, "gCombo.Tricks.MM.ClockTownLowerJump", "South Clock Town lower chest with a precise jump",
     "Climb the roof with a precise human jump to reach the lower chest without the Deku flower or Hookshot. This applies only to the lower chest; the upper chest retains its ordinary access and final-day requirement.",
     "South Clock Town", "Movement, precise jump", 2, 1u << 2},
    {6, "gCombo.Tricks.MM.DoggyFenceJump", "Doggy Racetrack chest with a fence jump",
     "Climb the fence and jump precisely to the chest as human Link without beans, the Hookshot or the Zora Mask. Reaching the racetrack remains required, and this does not affect winning the dog race.",
     "Doggy Racetrack", "Movement, precise jump", 2, 1u << 2},
    {7, "gCombo.Tricks.MM.PalaceBeanSkip", "Skip planting beans in Deku Palace",
     "Backflip onto the west garden doorway and make an angled roll jump from the lower palace to the upper cell-side ledge. This removes the bean and watering requirement for that climb. Crossing the upper palace still requires the Deku Mask and its Mad Scrub combat requirements.",
     "Deku Palace", "Movement, precise jump", 2, 1u << 2},
    {8, "gCombo.Tricks.MM.IkanaIceSkip", "Climb Ikana Canyon without Ice Arrows",
     "Aim the Hookshot from a precise position on the riverside to reach the first tree without making an ice platform. The Hookshot and access to lower Ikana Canyon remain required.",
     "Ikana Canyon", "Hookshot, precise aim", 2, 1u << 2},
    {9, "gCombo.Tricks.MM.StoneTowerOneMask", "Climb Stone Tower with one transformation mask",
     "Use human Link's statue plus a Goron or Zora statue, and ride the moving blocks while managing Elegy statues to reach the tower's top and owl area. Requires the Hookshot and a playable Elegy of Emptiness, including its instrument and notes. A Deku Mask alone does not suffice. Moving the temple bridge itself requires only Elegy.",
     "Stone Tower", "Elegy, movement", 3, 1u << 3},
    {10, "gCombo.Tricks.MM.DekuStickFighting", "Fight enemies with Deku Sticks",
     "Allow Deku Stick combat where OoTMM expects this technique. A renewable stick source is still required. This does not change normal stick use for torches or owl statues.",
     "Overworld and dungeons", "Combat, sticks", 1, 1u << 1},
    {11, "gCombo.Tricks.MM.TwinmoldBow", "Defeat Twinmold with the Bow",
     "Aim ordinary arrows at both worms' heads and tails without the Giant's Mask. Requires a renewable arrow source.",
     "Stone Tower Temple", "Combat, bow", 2, 1u << 2},
    {12, "gCombo.Tricks.MM.TwinmoldFireIce", "Defeat Twinmold with Fire and Ice Arrows",
     "Use Fire Arrows on the blue worm and Ice Arrows on the red worm without the Giant's Mask. Requires arrows and magic.",
     "Stone Tower Temple", "Combat, magic arrows", 1, 1u << 1},
    {13, "gCombo.Tricks.MM.RanchFarore", "Ranch events with Farore's Wind",
     "Return to a recorded ranch point across the cycle; the requested half-day clock, Romani's soul and the event's equipment remain required.",
     "Romani Ranch", "Farore, clocks", 2, 1u << 2, false},
    {14, "gCombo.Tricks.MM.GbtCentralGeyser", "Great Bay central geyser",
     "Reach central-room pipes with the pinned spell or magic-arrow geyser technique. The water wheel and swimming requirements remain specific to the destination.",
     "Great Bay Temple", "Farore, movement", 2, 1u << 2, false},
    {15, "gCombo.Tricks.MM.LullabySkipIrons", "Snowhead blizzard with Iron Boots",
     "The pinned Iron Boots blizzard bypass still requires magic and Goron Mask ownership. Hidden until actual native gear gameplay is validated.",
     "Snowhead", "Iron Boots, movement", 2, 1u << 2, false},
    {16, "gCombo.Tricks.MM.ZoraHallScrubNoDeku", "Zora Hall Scrub heart without Deku",
     "Pinned Goron or tall-form jump reaches the heart without Deku. Hidden until actual Adult gameplay is validated.",
     "Zora Hall", "Movement, masks", 2, 1u << 2, false},
};
inline const MmTrickDefinition* FindMmTrick(int32_t id) {
    for (const auto& trick : kMmTricks) if (trick.id == id) return &trick;
    return nullptr;
}
inline constexpr bool CanNavigateHidden(bool lensOwned, bool magicOwned, bool lenslessEnabled) {
    return (lensOwned && magicOwned) || lenslessEnabled;
}
} // namespace ComboRando
