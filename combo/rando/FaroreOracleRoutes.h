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
} // namespace ComboRando::FaroreOracle
