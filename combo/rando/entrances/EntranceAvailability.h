#pragma once
#include "Capabilities.h"
#include <string>
#include <string_view>
#include <utility>
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
// The OoT starting warp song options (settings.cpp RSK_STARTING_*): their CVars under gRandoSettings. and the songs.
inline constexpr std::pair<const char*, const char*> kOotStartingSongSettings[] = {
    { "StartingMinuetOfForest", "Minuet of Forest" },     { "StartingBoleroOfFire", "Bolero of Fire" },
    { "StartingSerenadeOfWater", "Serenade of Water" },   { "StartingRequiemOfSpirit", "Requiem of Spirit" },
    { "StartingNocturneOfShadow", "Nocturne of Shadow" }, { "StartingPreludeOfLight", "Prelude of Light" },
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
// Master Quest dungeons that Wallmaster Shuffle cannot take: any, for the frozen vanilla id, and any played from data
// other than OoTMM's bundled data (a real oot-mq.o2r), whose roll is not the seed's own (UnifiedBridge.h).
inline constexpr const char* kWallmasterMasterQuestText =
    "Wallmaster Shuffle does not support Master Quest dungeons yet.";
inline constexpr const char* kWallmasterDungeonDataText = "Wallmaster Shuffle cannot read the OoT dungeon data.";
// Master Quest Spirit Temple and Bottom of the Well with a vanilla Shadow Temple and Ganon's Castle have 12 OoT
// Wallmasters for 11 dungeon entrances, which OoTMM cannot place (UnifiedBridge.h RequireWallmasterCapacity).
// Two sentences. Where every roll of the MQ settings is such a context (EntranceProfile.h MqWallmasterShortfallForced),
// the row, its tooltip and the Generate button name the rows that fix it: the per-dungeon rows, which those settings
// always show, and either one set to Vanilla removes every such context. Where a random roll lands on one on the
// seed's own first attempt, generation stops with the second, as OoTMM's seed fails; another seed rolls again.
inline constexpr const char* kWallmasterCapacityText =
    "Set Spirit Temple Quest or Bottom of the Well Quest to Vanilla to use Wallmaster Shuffle.";
inline constexpr const char* kWallmasterCapacitySeedText =
    "This seed's Master Quest dungeons have more Wallmasters than Wallmaster Shuffle can place. Try another seed.";
inline constexpr const char* kWallmasterCapacityLog =
    "Master Quest Spirit Temple and Bottom of the Well with vanilla Shadow Temple and Ganon's Castle have more OoT "
    "Wallmasters than dungeon entrances";
inline EntranceRefusal WallmasterCapacityRefusal(std::string log = kWallmasterCapacityLog) {
    return { kWallmasterCapacityText, std::move(log) };
}
inline EntranceRefusal WallmasterSeedCapacityRefusal(std::string log = kWallmasterCapacityLog) {
    return { kWallmasterCapacitySeedText, std::move(log) };
}
inline EntranceRefusal ShipEntrancesRefusal() {
    return { kWallmasterShipEntrancesText, "Wallmaster and One-Way shuffles need Ship's own entrance shuffle off" };
}
inline EntranceRefusal EnemyRandomizerRefusal() {
    return { "Turn off Enemy Randomizer to use Wallmaster Shuffle.", "Wallmaster Shuffle needs Enemy Randomizer off" };
}
// What the Temple probe cannot validate yet. Foreign-song logic ("Play warp songs across games") is not here: with no
// starting warp song or Soaring, which this refuses, it cannot change OoTMM's Temple check (lemma S1,
// entrance_temple_song_lemma_tests.py), so the probe takes it as it is.
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
    for (const auto* list : { &f.ootStartingItems, &f.mmStartingItems, &f.foreignStartingItems })
        for (const auto& item : *list)
            for (std::string_view song : { "Song of Soaring", "Minuet of Forest", "Bolero of Fire", "Serenade of Water",
                                           "Nocturne of Shadow", "Requiem of Spirit", "Prelude of Light" })
                if (item == song)
                    return { "Remove " + item + " from the starting items to use Wallmaster Shuffle.",
                             "starting " + item + " needs GLOBAL song sources (deferred)" };
    return {};
}
// Foreign-song logic needs wallmasters-own-game-songs-v1 (UnifiedBridge.h UnifiedSongRequirement), staged at Dev: a
// build without that id keeps this sentence. `caps` are the build's.
inline EntranceRefusal WallmasterSettingsRefusal(const EntranceSettingsFacts& f,
                                                 const CapabilitySet& caps = ActiveCapabilities()) {
    if (!f.combinedMode)
        return { "Wallmaster Shuffle works only in combined mode.", "Wallmaster Shuffle works only in combined mode." };
    if (f.shipwrightEntrances)
        return ShipEntrancesRefusal();
    if (f.enemyRandomizer)
        return EnemyRandomizerRefusal();
    if (auto why = TempleSettingsRefusal(f); !why.empty())
        return why;
    if (f.logicEnabled && f.foreignSongsLogic && !caps.Active(kSongsWallmasterCapability))
        return { "Turn off \"Play warp songs across games\" to use Wallmaster Shuffle.",
                 "foreign-song logic needs wallmasters-own-game-songs-v1, which this build does not have" };
    return {};
}
} // namespace ComboRando::Entrances
