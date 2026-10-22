#pragma once
#include <stdexcept>
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
// One of MM's starting items (gRando.StartingItems) by the name the refusals compare. Seeds and profiles store 2Ship's
// spoiler names, which are the item ids (Items.cpp RI(), #id: "RI_SONG_SOARING"); only generation's dump uses display
// names (BenPort.cpp sharedStartingItems: "Song of Soaring"). Both spellings give the display name. Soaring is the only
// MM item a refusal names, as 2Ship has no OoT warp song item; any other item keeps its stored name.
inline std::string MmStartingItemName(std::string_view item) {
    return item == "RI_SONG_SOARING" ? "Song of Soaring" : std::string(item);
}
// The first OoT warp song among the starting items of either game, or empty. A seed with one under logic takes
// wallmasters-own-game-songs-v2 (UnifiedBridge.h UnifiedWarpSongRequirement, ComboSeedFormat.h); every earlier
// Wallmaster id refuses it.
inline std::string StartingWarpSong(const EntranceSettingsFacts& f) {
    for (const auto* list : { &f.ootStartingItems, &f.mmStartingItems, &f.foreignStartingItems })
        for (const auto& item : *list)
            for (const auto& setting : kOotStartingSongSettings)
                if (item == setting.second)
                    return item;
    return {};
}
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
// A refusal: this build, these settings or the seed's other options cannot generate the layout. Most are raised
// before any random draw (UnifiedBridge.h GenerateEntranceLayout); Full Wallmasters' Temple refusal is raised by the
// first candidate the Temple check would have to search through MM (TempleAdapter.h, the Temple validator). Not a
// ShuffleError, so neither the Shuffler nor an outer generation attempt retries it (M1 plan, critic correction C1).
class UnifiedEntranceRefused : public std::logic_error {
  public:
    using std::logic_error::logic_error;
    // A settings or dungeon data reason: what() keeps the developer text for logs.
    explicit UnifiedEntranceRefused(const EntranceRefusal& why) : std::logic_error(why.log), sentence(why.player) {
    }
    // The generation error the player sees: the reason's plain sentence (owner decision D3), or the developer text
    // of an internal fault, which has none.
    std::string player() const {
        return sentence.empty() ? what() : sentence;
    }

  private:
    std::string sentence;
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
// Full Wallmasters when the world without Wallmaster routes fails the opposite-age Temple check (UnifiedBridge.h, the
// monotone shortcut): OoTMM's validateAgeTemple (entrances.ts:265-287) would then check each candidate, through both
// games, but this port's probe cannot search MM yet (limits design, Stage 5). The first candidate with a route into
// the other game stops generation with this sentence (TempleAdapter.h, the Temple validator); it is not retried. The
// menu does not show it: only generation finds out whether that world passes.
inline constexpr const char* kFullWallmasterTempleText =
    "Full Wallmaster Shuffle can't be generated with these settings yet. Choose Own Game, or start as child Link.";
inline EntranceRefusal FullWallmasterTempleRefusal() {
    return { kFullWallmasterTempleText,
             "Temple probe: a cross-game Wallmaster candidate needs a Temple check through MM (deferred)" };
}
// What the Temple probe cannot validate yet. Foreign-song logic ("Play warp songs across games") is not here: with no
// starting song it cannot change OoTMM's Temple check (lemma S1, entrance_temple_song_lemma_tests.py), so the probe
// takes it as it is. Nor is a starting OoT warp song: the probe plays it from its GLOBAL node (lemma L3,
// entrance_temple_global_lemma_tests.py), and MM adds no Temple path while song destinations stay put and Soaring does
// not start (the Mask Shop lemma). A starting Song of Soaring could reach MM's owls first, which needs a Temple check
// through MM.
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
    // The readers store MM's items by display name (MmStartingItemName); facts made elsewhere may not.
    for (const auto* list : { &f.ootStartingItems, &f.mmStartingItems, &f.foreignStartingItems })
        for (const auto& stored : *list)
            if (const auto item = MmStartingItemName(stored); item == "Song of Soaring")
                return { "Remove " + item + " from the starting items to use Wallmaster Shuffle.",
                         "starting " + item + " needs a Temple check through MM (deferred)" };
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
