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
    // Home game mode (GameMode.h). OoTMM v32.3 offers Own Game only when both games are played
    // (packages/core/src/settings/data.ts:3551, hasOoTMM :7); solo and vanilla runs have no entrance layout.
    bool combinedMode = true;
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
// Why Wallmaster Shuffle is unavailable. The player sees one plain sentence per reason on the Entrances row, its
// tooltip, the Generate button and the generation error (owner decision D3); logs and exceptions keep the developer
// text. Both are empty when nothing refuses it.
struct EntranceRefusal {
    std::string player, log;
    bool empty() const {
        return player.empty();
    }
    bool operator==(const EntranceRefusal& other) const {
        return player == other.player && log == other.log;
    }
};
// The menu shows two of kShipEntranceSettings, Warp Songs and Owl Drops, on Warps & travel (ComboNavigation.h), not
// beside Wallmaster Shuffle under Entrances, so the sentence names them and their page.
inline constexpr const char* kWallmasterShipEntrancesText =
    "Turn off the other OoT entrance shuffles, and Warp Songs and Owl Drops on the Warps & travel page, to use "
    "Wallmaster Shuffle.";
inline constexpr const char* kWallmasterMasterQuestText =
    "Wallmaster Shuffle does not support Master Quest dungeons yet.";
inline constexpr const char* kWallmasterDungeonDataText = "Wallmaster Shuffle cannot read the OoT dungeon data.";
inline EntranceRefusal ShipEntrancesRefusal() {
    return { kWallmasterShipEntrancesText, "Wallmaster and One-Way shuffles need Ship's own entrance shuffle off" };
}
inline EntranceRefusal EnemyRandomizerRefusal() {
    return { "Turn off Enemy Randomizer to use Wallmaster Shuffle.", "Wallmaster Shuffle needs Enemy Randomizer off" };
}
inline EntranceRefusal TempleSettingsRefusal(const EntranceSettingsFacts& f) {
    if (!f.logicEnabled)
        return {};
    // Ship's Logic row offers only Glitchless and No Logic; any other value comes from an old or edited profile.
    if (!f.glitchless)
        return { "Set Logic to Glitchless to use Wallmaster Shuffle.", "only glitchless OoT logic is supported" };
    if (f.shipwrightEntrances)
        return { kWallmasterShipEntrancesText, "Ship's own entrance shuffle must be off" };
    if (f.songTravelLogic)
        return { "Turn off Warp Songs and Soaring Spots Shuffle to use Wallmaster Shuffle.",
                 "song travel logic needs GLOBAL song sources (deferred)" };
    if (f.foreignSongsLogic)
        return { "Turn off \"Play warp songs across games\" to use Wallmaster Shuffle.",
                 "song travel logic needs GLOBAL song sources (deferred)" };
    for (const auto* list : { &f.ootStartingItems, &f.mmStartingItems, &f.foreignStartingItems })
        for (const auto& item : *list)
            for (std::string_view song : { "Song of Soaring", "Minuet of Forest", "Bolero of Fire", "Serenade of Water",
                                           "Nocturne of Shadow", "Requiem of Spirit", "Prelude of Light" })
                if (item == song)
                    return { "Remove " + item + " from the starting items to use Wallmaster Shuffle.",
                             "starting " + item + " needs GLOBAL song sources (deferred)" };
    return {};
}
inline EntranceRefusal WallmasterSettingsRefusal(const EntranceSettingsFacts& f) {
    if (!f.combinedMode)
        return { "Wallmaster Shuffle works only in combined mode.", "Wallmaster Shuffle works only in combined mode." };
    if (f.shipwrightEntrances)
        return ShipEntrancesRefusal();
    if (f.enemyRandomizer)
        return EnemyRandomizerRefusal();
    return TempleSettingsRefusal(f);
}
} // namespace ComboRando::Entrances
