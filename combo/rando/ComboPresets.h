#pragma once
#include <vector>
#include "GameplayRules.h"
#include "SharedItems.h"

namespace ComboRando {
enum class CommunityPreset { Default, Beginner, Allsanity };
struct CommunityPresetInfo {
    CommunityPreset id;
    const char* name;
    const char* summary;
    const char* help;
};
inline constexpr CommunityPresetInfo kCommunityPresets[] = {
    { CommunityPreset::Default, "Default", "Classic combined randomizer",
      "Based on OoTMM's Default preset, using Combo Ship's supported options. Glitchless logic, original quest counts, "
      "ordinary locations and separate item upgrades." },
    { CommunityPreset::Beginner, "Beginner", "Easier access, strong hints, shared upgrades",
      "Adapted from OoTMM's Beginner preset. Start with instruments, Soaring and supplies; share supported upgrades "
      "and masks; open the Door of Time and reduce Fortress rescue to one carpenter. Maps and compasses start "
      "collected. Combo Ship does not pre-complete dungeons or copy OoTMM's individual plando exclusions." },
    { CommunityPreset::Allsanity, "Allsanity", "Explore a much larger check pool",
      "Adapted from OoTMM's Allsanity preset. Adds supported shops, tokens, NPC rewards, freestanding items and "
      "scenery drops in both worlds, with shared upgrades and cross-game songs. Unsupported OoTMM item families and "
      "entrance rules are omitted." },
};
struct PresetOption {
    const char* key;
    int value;
};
// Corresponding enums/values are validated against each native menu by the preset regression.
inline std::vector<PresetOption> CommunityPresetOptions(CommunityPreset preset) {
    std::vector<PresetOption> options = {
        { "gRandoSettings.LogicRules", 0 },      { "gCombo.Rando.Reachability", 0 },
        { "gCombo.Rando.StartingGame", 0 },      { "gCombo.Rando.TriforceHunt", 0 },
        { "gCombo.Rando.TriforceRequired", 15 }, { "gCombo.Rando.TriforceTotal", 15 },
        { "gRando.Options.RO_LOGIC", 0 },        { "gRandoSettings.GossipStoneHints", 1 },
        { "gRandoSettings.HintClarity", 2 },     { "gRandoSettings.HintDistribution", 1 },
    };
    const bool shared = preset != CommunityPreset::Default;
    for (int i = 0; i < SF_COUNT; ++i)
        if (i != SF_OCARINA && SharedFamilyByIndex(i).publicOption)
            options.push_back({ SharedFamilyByIndex(i).cvar, shared ? 1 : 0 });
    if (shared) {
        const PresetOption common[] = {
            { "gRandoSettings.CompleteMaskQuest", 2 },
            { "gRandoSettings.BombchuBag", 1 },
            { "gRando.Options.RO_HINTS_GOSSIP_STONES", 1 },
            { "gRando.Options.RO_HINTS_GOSSIP_STONE_STRENGTH", 75 },
        };
        options.insert(options.end(), std::begin(common), std::end(common));
    }
    if (preset == CommunityPreset::Beginner) {
        const PresetOption beginner[] = {
            { "gRandoSettings.DoorOfTime", 2 },
            { "gRandoSettings.FortressCarpenters", 1 },
            { "gRandoSettings.StartingAge", 2 },
            { "gRandoSettings.ClosedForest", 1 },
            { "gRandoSettings.StartingMapsCompasses", 0 },
            { "gRandoSettings.StartingOcarina", 1 },
            { "gRandoSettings.StartingDekuShield", 1 },
            { "gRandoSettings.StartingSticks", 1 },
            { "gRandoSettings.StartingNuts", 1 },
            { "gRandoSettings.HintDistribution", 3 },
            { "gRando.Options.RO_STARTING_MAPS_AND_COMPASSES", 1 },
            { "gRando.Options.RO_PLACEMENT_SMALL_KEYS", 2 },
            { "gRando.Options.RO_STARTING_CONSUMABLES", 1 },
            { "gRando.Options.RO_HINTS_BOSS_REMAINS", 1 },
            { "gRando.Options.RO_HINTS_TRANSFORMATIONS", 1 },
        };
        options.insert(options.end(), std::begin(beginner), std::end(beginner));
    }
    if (preset == CommunityPreset::Allsanity) {
        const PresetOption allsanity[] = {
            { "gRandoSettings.ShuffleSongs", 3 },
            { "gRandoSettings.ShuffleTokens", 3 },
            { "gRandoSettings.ShufflePots", 3 },
            { "gRandoSettings.ShuffleGrass", 3 },
            { "gRandoSettings.ShuffleCrates", 3 },
            { "gRandoSettings.ShuffleFreestanding", 3 },
            { "gRandoSettings.ShuffleWonderItems", 3 },
            { "gRandoSettings.ShuffleBeehives", 1 },
            { "gRandoSettings.ShuffleCows", 1 },
            { "gRandoSettings.ShuffleKokiriSword", 1 },
            { "gRandoSettings.ShuffleOcarinas", 1 },
            { "gRandoSettings.ShuffleAdultTrade", 1 },
            { "gRandoSettings.ShuffleZeldasLetter", 1 },
            { "gRandoSettings.ShuffleGerudoToken", 1 },
            { "gRandoSettings.ShuffleScrubs", 2 },
            { "gRandoSettings.ShuffleMerchants", 3 },
            { "gRandoSettings.Shopsanity", 1 },
            { "gRandoSettings.ShopsanityCount", 4 },
            { "gRando.Options.RO_SHUFFLE_SHOPS", 1 },
            { "gRando.Options.RO_SHUFFLE_TINGLE_SHOPS", 1 },
            { "gRando.Options.RO_SHUFFLE_OWL_STATUES", 1 },
            { "gRando.Options.RO_SHUFFLE_GOLD_SKULLTULAS", 1 },
            { "gRando.Options.RO_SHUFFLE_COWS", 1 },
            { "gRando.Options.RO_SHUFFLE_FROGS", 1 },
            { "gRando.Options.RO_SHUFFLE_BUTTERFLIES", 1 },
            { "gRando.Options.RO_SHUFFLE_FREESTANDING_ITEMS", 1 },
            { "gRando.Options.RO_SHUFFLE_WONDER_ITEMS", 1 },
            { "gRando.Options.RO_SHUFFLE_ENEMY_DROPS", 1 },
            { "gRando.Options.RO_SHUFFLE_POT_DROPS", 1 },
            { "gRando.Options.RO_SHUFFLE_CRATE_DROPS", 1 },
            { "gRando.Options.RO_SHUFFLE_BARREL_DROPS", 1 },
            { "gRando.Options.RO_SHUFFLE_SNOWBALL_DROPS", 1 },
            { "gRando.Options.RO_SHUFFLE_GRASS_DROPS", 1 },
            { "gRando.Options.RO_SHUFFLE_TREE_DROPS", 1 },
            { "gRando.Options.RO_SHUFFLE_HIVE_DROPS", 1 },
        };
        options.insert(options.end(), std::begin(allsanity), std::end(allsanity));
    }
    return options;
}
} // namespace ComboRando
