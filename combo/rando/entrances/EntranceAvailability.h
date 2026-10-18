#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace ComboRando::Entrances {
// Settings-level facts shared by the profile menu, generation rules and Temple
// gate. Finalized native starting inventories are checked again before any draw.
struct EntranceSettingsFacts {
    bool logicEnabled = true, glitchless = true;
    bool shipwrightEntrances = false, enemyRandomizer = false;
    bool songTravelLogic = false, foreignSongsLogic = false;
    std::vector<std::string> ootStartingItems, mmStartingItems, foreignStartingItems;
};
inline constexpr const char* kShipEntranceSettings[] = {
    "gRandoSettings.ShuffleDungeonsEntrances",
    "gRandoSettings.ShuffleBossEntrances",
    "gRandoSettings.ShuffleOverworldEntrances",
    "gRandoSettings.ShuffleInteriorsEntrances",
    "gRandoSettings.ShuffleThievesHideoutEntrances",
    "gRandoSettings.ShuffleGrottosEntrances",
    "gRandoSettings.ShuffleOwlDrops",
    "gRandoSettings.ShuffleWarpSongs",
    "gRandoSettings.ShuffleOverworldSpawns",
};
template <class Read> bool ShipEntrancesActive(Read read) {
    for (const char* key : kShipEntranceSettings)
        if (read(key, 0) != 0)
            return true;
    return false;
}
inline bool IsShipEntranceSetting(std::string_view key) {
    for (const char* row : kShipEntranceSettings)
        if (key == row)
            return true;
    return false;
}
inline std::string TempleSettingsRefusal(const EntranceSettingsFacts& f) {
    if (!f.logicEnabled)
        return {};
    if (!f.glitchless)
        return "only glitchless OoT logic is supported";
    if (f.shipwrightEntrances)
        return "Ship's own entrance shuffle must be off";
    if (f.songTravelLogic || f.foreignSongsLogic)
        return "song travel logic needs GLOBAL song sources (deferred)";
    for (const auto* list : { &f.ootStartingItems, &f.mmStartingItems, &f.foreignStartingItems })
        for (const auto& item : *list)
            for (std::string_view song : { "Song of Soaring", "Minuet of Forest", "Bolero of Fire", "Serenade of Water",
                                           "Nocturne of Shadow", "Requiem of Spirit", "Prelude of Light" })
                if (item == song)
                    return "starting " + item + " needs GLOBAL song sources (deferred)";
    return {};
}
inline std::string WallmasterSettingsRefusal(const EntranceSettingsFacts& f) {
    if (f.shipwrightEntrances)
        return "Wallmaster and One-Way shuffles need Ship's own entrance shuffle off";
    if (f.enemyRandomizer)
        return "Wallmaster Shuffle needs Enemy Randomizer off";
    return TempleSettingsRefusal(f);
}
} // namespace ComboRando::Entrances
