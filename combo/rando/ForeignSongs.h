#pragma once
#include <cstdint>

namespace ComboRando {
// OoTMM mm/ocarina.c and macros_common.yml: A, down, right, left, up.
inline constexpr uint8_t kForeignNotes[7][8] = { { 0, 4, 3, 2, 3, 2 }, { 1, 0, 1, 0, 2, 1, 2, 1 }, { 0, 1, 2, 2, 3 },
                                                 { 0, 1, 0, 2, 1, 0 }, { 3, 2, 2, 0, 3, 2, 1 },    { 4, 2, 4, 2, 3, 4 },
                                                 { 1, 3, 4, 1, 3, 4 } };
inline constexpr int kForeignLengths[] = { 6, 8, 5, 6, 7, 6, 6 };
inline constexpr uint8_t kSongPitches[] = { 2, 5, 9, 11, 14 };
inline uint32_t RequiredSongButtons(int song) {
    uint32_t mask = 0;
    for (int i = 0; i < kForeignLengths[song]; i++)
        mask |= 1u << kForeignNotes[song][i];
    return mask;
}
inline uint32_t ForeignWarpSources(bool enabled, uint32_t knownSongs, bool instrument, uint32_t buttons) {
    if (!enabled || !instrument)
        return 0;
    uint32_t mask = 0;
    for (int i = 0; i < 6; i++)
        if ((knownSongs & (1u << i)) && (buttons & RequiredSongButtons(i)) == RequiredSongButtons(i))
            mask |= 1u << i;
    return mask;
}
// OoTMM macros_oot.yml:33 and oot/ocarina.c:140-146: OoT plays Soaring as child Link only, unless
// agelessSoaring (data.ts:3240-3246) is on.
inline uint32_t ForeignSoaringSources(bool enabled, uint32_t knownOwls, bool instrument, uint32_t buttons, bool child,
                                      bool ageless = false) {
    return enabled && instrument && (child || ageless) && (buttons & RequiredSongButtons(6)) == RequiredSongButtons(6)
               ? knownOwls & 0xffc0
               : 0;
}
inline int RecognizeForeignSong(const uint8_t* pitches, int count, uint32_t allowed) {
    if (!pitches || count < 0 || count > 8)
        return -1;
    for (int song = 0; song < 7; song++) {
        if (!(allowed & (1u << song)) || count < kForeignLengths[song])
            continue;
        bool match = true;
        for (int i = 0; i < kForeignLengths[song]; i++)
            match &= pitches[count - kForeignLengths[song] + i] == kSongPitches[kForeignNotes[song][i]];
        if (match)
            return song;
    }
    return -1;
}
inline constexpr const char* kSongLandingNames[] = { "Sacred Forest Meadow",
                                                     "Death Mountain Crater",
                                                     "Lake Hylia",
                                                     "Desert Colossus",
                                                     "Graveyard warp pad",
                                                     "Temple of Time",
                                                     "Great Bay Coast",
                                                     "Zora Cape",
                                                     "Snowhead",
                                                     "Mountain Village",
                                                     "Clock Town",
                                                     "Milk Road",
                                                     "Woodfall",
                                                     "Southern Swamp",
                                                     "Ikana Canyon",
                                                     "Stone Tower" };
} // namespace ComboRando
