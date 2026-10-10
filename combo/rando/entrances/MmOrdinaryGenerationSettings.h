#pragma once
#include "rando/entrances/Shuffle.h"
namespace ComboRando::Entrances {
// Identical request construction for the native launcher and headless validator.
// Admission remains in GenerateNativeMmOrdinary; unsupported eligible routes fail.
inline Settings MmOrdinaryGenerationSettings(bool indoors, bool telescopes, bool overworld, bool dungeons) {
    Settings settings;
    settings.games = Games::Mm;
    if (indoors || telescopes)
        settings.erIndoors = Mode::OwnGame;
    settings.erIndoorsTelescopes = telescopes;
    if (overworld)
        settings.erOverworld = Mode::OwnGame;
    if (dungeons)
        settings.erDungeons = Mode::OwnGame;
    return settings;
}
} // namespace ComboRando::Entrances
