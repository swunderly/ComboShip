#pragma once
namespace ComboRando::FaroreOracle {
inline bool CanUse(bool spellWindMm, bool hasMagic, bool sharedSpellWind, bool ownSpellWind, bool sharedWind) {
    return spellWindMm && hasMagic && (sharedSpellWind ? sharedWind : ownSpellWind);
}
inline bool RanchAliens(bool soulMalon, bool at0230, bool ranchTrick, bool farore, bool clockNight1, bool arrows,
                        bool din, bool aliensDinTrick) {
    return soulMalon && (at0230 || (ranchTrick && farore && clockNight1)) && (arrows || (din && aliensDinTrick));
}
inline bool RanchEpona(bool soulMalon, bool beforeNight1, bool ranchTrick, bool farore, bool clockDay1) {
    return soulMalon && (beforeNight1 || (ranchTrick && farore && clockDay1));
}
inline bool RanchCremia(bool aliens, bool atNight2, bool ranchTrick, bool farore, bool clockNight2) {
    return aliens && (atNight2 || (ranchTrick && farore && clockNight2));
}
inline bool SpiRoof(bool deku, bool shortHookAnywhere, bool hover, bool rememberedRoof) {
    return deku || shortHookAnywhere || hover || rememberedRoof;
}
inline bool GbtGeyser(bool trick, bool fire, bool ice, bool din, bool farore, bool nayru) {
    return trick && ((fire && ice) || din || farore || nayru);
}
inline bool GbtMapUnderwater(bool underwaterWalking, bool swim, bool hookshotAnywhere, bool geyser) {
    return underwaterWalking || (swim && (hookshotAnywhere || geyser));
}
inline bool GbtRedPipe(bool underwaterWalking, bool geyser) {
    return underwaterWalking || geyser;
}
inline bool GbtCompass(bool underwaterWalking, bool hookshotAnywhere, bool geyser, bool waterWheel) {
    return (underwaterWalking || hookshotAnywhere || geyser) && waterWheel;
}
inline bool GbtPreBoss(bool underwaterWalking, bool geyser, bool waterWheel) {
    return (underwaterWalking || geyser) && waterWheel;
}
// These complete predicates require the native region-state / hot-water event binding.
// Callers may not replace independent cursed/cleared reachability flags with a single completion bit.
inline bool Evan(bool soulMusicians, bool playEvan, bool oceanCursed, bool trick, bool farore) {
    return soulMusicians && playEvan && (oceanCursed || (trick && farore));
}
inline bool HotWaterMountain(bool bottle, bool winter, bool spring, bool northWinter, bool northSpring) {
    return (winter && bottle && northWinter) || (spring && bottle && northSpring) ||
           ((bottle && northWinter) && (bottle && northSpring));
}
inline bool HotWaterWell(bool bottle, bool winter, bool spring, bool wellWinter, bool wellSpring) {
    return (winter && bottle && wellWinter) || (spring && bottle && wellSpring) ||
           ((bottle && wellWinter) && (bottle && wellSpring));
}
inline bool HotWaterAny(bool mountain, bool well, bool bottle, bool northWinter, bool wellWinter, bool northSpring,
                        bool wellSpring) {
    return mountain || well ||
           (((bottle && northWinter) || (bottle && wellWinter)) && ((bottle && northSpring) || (bottle && wellSpring)));
}
inline bool HotWaterDistance(bool soaring, bool farore, bool waterAny) {
    return (soaring || farore) && waterAny;
}
inline bool HotWaterEr(bool entranceRandomized, bool waterAny) {
    return entranceRandomized && waterAny;
}
inline bool HotWaterFarore(bool farore, bool waterAny) {
    return farore && waterAny;
}
inline bool ShtHotWater(bool distance, bool snowheadOwl, bool er, bool faroreWater, bool trick) {
    return ((distance && snowheadOwl) || er || faroreWater) && trick;
}
inline bool ShtHotWaterEr(bool er, bool faroreWater, bool trick) {
    return (er || faroreWater) && trick;
}
inline bool GbtBossKeyRoom(bool ice, bool fireShort, bool waterFarore) {
    return ice && (fireShort || waterFarore);
}
inline bool ShtDinolfosRoom(bool fireShort, bool hotWaterEr, bool hotWater, bool farore) {
    return fireShort || hotWaterEr || (hotWater && farore);
}
inline bool ShtRaisePillar(bool goron, bool hammer, bool fireShort, bool stickRun, bool stickTrick, bool hotWaterEr,
                           bool fireArrows, bool scarecrow, bool explosives, bool hookshot, bool keys3, bool faroreMap,
                           bool shortHookAnywhere, bool hover) {
    return ((goron || hammer) && (fireShort || (stickRun && stickTrick && hotWaterEr))) &&
           (fireArrows || scarecrow || (explosives && hookshot && keys3) ||
            (faroreMap && (goron || hookshot || shortHookAnywhere || hover)));
}
} // namespace ComboRando::FaroreOracle
