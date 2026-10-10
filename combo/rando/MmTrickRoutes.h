#pragma once

namespace ComboRando::MmTrickRoutes {
// Route-specific permissions never replace the remaining native items, forms,
// schedule or reward conditions. Ordinary alternatives remain valid when off.
inline constexpr bool PinnacleRock(bool zora, bool swim, bool seahorse, bool blindRoute) {
    return zora && swim && (seahorse || blindRoute);
}
inline constexpr bool PostOffice(bool bunnyHood, bool blindTiming, bool dayOneWindow,
                                 bool letterMailed, bool dayTwoWindow) {
    return (bunnyHood || blindTiming) && (dayOneWindow || (letterMailed && dayTwoWindow));
}
inline constexpr bool OceansideCode(bool bow, bool captainMask, bool guessCode) {
    return bow && (captainMask || guessCode);
}
inline constexpr bool ClockTownLowerChest(bool deku, bool moonsTear, bool hookshot, bool preciseJump) {
    return (deku && moonsTear) || hookshot || preciseJump;
}
inline constexpr bool DoggyChest(bool hookshot, bool usableBean, bool zora, bool preciseJump) {
    return hookshot || usableBean || zora || preciseJump;
}
inline constexpr bool PalaceDoorwayClimb(bool doorwayJump) {
    return doorwayJump; // Only this climb; upper flight/enemy gates remain in their own regions.
}
inline constexpr bool IkanaClimb(bool hookshot, bool usableIceArrows, bool preciseHookshot) {
    return hookshot && (usableIceArrows || preciseHookshot);
}
inline constexpr bool StoneTowerClimb(bool hookshot, bool playableElegy, bool goron, bool zora, bool oneMask) {
    return hookshot && playableElegy && ((goron && zora) || (oneMask && (goron || zora)));
}
} // namespace ComboRando::MmTrickRoutes
