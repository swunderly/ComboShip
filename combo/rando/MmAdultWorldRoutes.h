#pragma once
namespace ComboRando::MmAdultWorldRoutes {
// Pinned mountain_village.yml27 and macros_mm.yml81..83,187..188,220..222,324..325.
// Caller must derive canEnterWater from supported scale settings; no scales means true.
constexpr bool MountainVillageWaterfall(bool spring, bool lens, bool canEnterWater, bool zoraOwned, bool dekuOwned,
                                        bool hoverOwned, bool hookshot, bool shortHookAnywhere, bool hookshotAnywhere) {
    return spring && lens &&
           (canEnterWater || zoraOwned || dekuOwned || hoverOwned || hookshot || shortHookAnywhere || hookshotAnywhere);
}
// Pinned woodfall.yml13,14,22; region-specific owl alternatives are separate.
constexpr bool WoodfallEntrance(bool deku, bool hook, bool cleared, bool nayru, bool hover) {
    return deku || hook || cleared || nayru || hover;
}
constexpr bool WoodfallHeart(bool deku, bool soul, bool cleared, bool hook, bool adult, bool shortHook, bool nayru) {
    return (deku && (soul || cleared)) || hook || (adult && shortHook && (cleared || nayru));
}
constexpr bool WoodfallRupee(bool deku, bool soul, bool cleared, bool anywhere, bool zora, bool hover) {
    return (deku && (soul || cleared)) || anywhere || zora || hover;
}
// Pinned zora_hall.yml57 and macros_mm.yml250.
constexpr bool ZoraHallScrubHeart(bool trick, bool goron, bool tall, bool deku, bool mountainDeed, bool boomerang,
                                  bool shortHookAnywhere) {
    return (trick && (goron || tall)) || (goron && deku && mountainDeed) || boomerang || shortHookAnywhere;
}
constexpr bool ZoraHallDoor(bool zora, bool oceanCleared, bool shortHookAnywhere, bool doorTrick) {
    return zora || oceanCleared || (shortHookAnywhere && doorTrick);
}
// Pinned ancient_castle_of_ikana.yml27,40,42..45. Unsupported short-hook-anywhere stays false.
constexpr bool IkanaNorthEntry(bool deku, bool anywhere, bool nayru, bool tall) {
    return deku || anywhere || (nayru && tall);
}
constexpr bool IkanaNorthExit(bool lens, bool anywhere, bool hover) {
    return lens || anywhere || hover;
}
constexpr bool IkanaNorthPots(bool deku, bool anywhere, bool hover) {
    return deku || anywhere || hover;
}
} // namespace ComboRando::MmAdultWorldRoutes
