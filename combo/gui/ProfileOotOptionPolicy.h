#pragma once
#include <nlohmann/json.hpp>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
namespace ComboRando::ProfileOotPolicy {
#include "../../soh/soh/Enhancements/randomizer/randomizerEnums.h"
static constexpr int IMFLAG_SEPARATOR_BOTTOM = 1;
static constexpr bool kComboOwnsWincon = true;
inline std::vector<std::string> NumOpts(const int min, const int max, const int step = 1,
                                        const std::string& textBefore = {}, const std::string& textAfter = {}) {
    std::vector<std::string> options;
    options.reserve((max - min) / step + 1);
    for (int i = min; i <= max; i += step) {
        options.push_back(textBefore + std::to_string(i) += textAfter);
    }
    return options;
}
struct Option {
    std::function<int()> current;
    std::function<void(int)> update;
    std::string key, disabledText;
    bool hidden = false, disabled = false;
    std::vector<std::string> choices;
    // Stored value of each choice; the catalog can omit one (Mask Quest has no Vanilla entry).
    std::vector<int> choiceValues;
    void Hide() {
        hidden = true;
    }
    void Unhide() {
        hidden = false;
    }
    bool IsHidden() const {
        return hidden;
    }
    void Disable(const char* text) {
        disabled = true;
        disabledText = text;
    }
    void Enable() {
        disabled = false;
        disabledText.clear();
    }
    void AddFlag(int) {
    }
    void RemoveFlag(int) {
    }
    size_t GetOptionCount() const {
        return choices.size();
    }
    void ChangeOptions(std::vector<std::string> values) {
        if (current && update && current() >= int(values.size()))
            update(static_cast<uint8_t>(values.size() - 1));
        choices = std::move(values);
        choiceValues.resize(choices.size());
        for (size_t i = 0; i < choices.size(); ++i)
            choiceValues[i] = int(i);
    }
};
class View {
    std::function<int(const char*, int)> read;
    std::map<std::string, int> effective;
    int CVarGetInteger(const char* key, int fallback) {
        auto it = effective.find(key);
        return it != effective.end() ? it->second : read(key, fallback);
    }
    void CVarSetInteger(const char* key, int value) {
        effective[key] = value;
    }
    uint8_t ComboOotTriforceTotal() {
        if (!CVarGetInteger("gCombo.Rando.TriforceHunt", 0))
            return 0;
        int total = std::clamp(CVarGetInteger("gCombo.Rando.TriforceTotal", 15), 1, 100);
        return uint8_t(total - total / 2);
    }
#define CVAR_RANDOMIZER_SETTING(s) "gRandoSettings." s
    void HandleShopsanityPriceUI() {
        bool isTycoon = CVarGetInteger(CVAR_RANDOMIZER_SETTING("IncludeTycoonWallet"), RO_GENERIC_OFF);
        mOptions[RSK_SHOPSANITY].RemoveFlag(IMFLAG_SEPARATOR_BOTTOM);
        mOptions[RSK_SHOPSANITY_PRICES].Unhide();
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShopsanityPrices"), RO_PRICE_VANILLA)) {
            case RO_PRICE_FIXED:
                mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].Unhide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_2].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                if (isTycoon ? mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].GetOptionCount() == 501
                             : mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].GetOptionCount() == 1000) {
                    mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].ChangeOptions(isTycoon ? NumOpts(0, 999)
                                                                                       : NumOpts(0, 500));
                }
                mOptions[RSK_SHOPSANITY_PRICES_AFFORDABLE].Hide();
                break;
            case RO_PRICE_RANGE:
                mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].Unhide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_2].Unhide();
                mOptions[RSK_SHOPSANITY_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                if (isTycoon ? mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].GetOptionCount() == 101
                             : mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].GetOptionCount() == 200) {
                    mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                   : NumOpts(0, 500, 5));
                    mOptions[RSK_SHOPSANITY_PRICES_RANGE_2].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                   : NumOpts(0, 500, 5));
                }
                mOptions[RSK_SHOPSANITY_PRICES_AFFORDABLE].Unhide();
                break;
            case RO_PRICE_SET_BY_WALLET:
                mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_2].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_NO_WALLET_WEIGHT].Unhide();
                mOptions[RSK_SHOPSANITY_PRICES_CHILD_WALLET_WEIGHT].Unhide();
                mOptions[RSK_SHOPSANITY_PRICES_ADULT_WALLET_WEIGHT].Unhide();
                mOptions[RSK_SHOPSANITY_PRICES_GIANT_WALLET_WEIGHT].Unhide();
                if (isTycoon) {
                    mOptions[RSK_SHOPSANITY_PRICES_TYCOON_WALLET_WEIGHT].Unhide();
                } else {
                    mOptions[RSK_SHOPSANITY_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                }
                mOptions[RSK_SHOPSANITY_PRICES_AFFORDABLE].Unhide();
                break;
            default:
                mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_2].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_AFFORDABLE].Unhide();
                break;
        }
    }
    void HandleMixedEntrancePoolsUI() {
        bool dungeonShuffle =
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonsEntrances"), RO_DUNGEON_ENTRANCE_SHUFFLE_OFF);
        bool bossShuffle =
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleBossEntrances"), RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF);
        bool overworldShuffle = CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleOverworldEntrances"), RO_GENERIC_OFF);
        bool interiorShuffle = CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleInteriorsEntrances"), RO_GENERIC_OFF);
        bool grottoShuffle = CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleGrottosEntrances"), RO_GENERIC_OFF);
        bool thievesHideoutShuffle =
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleThievesHideoutEntrances"), RO_GENERIC_OFF);

        // Hide Mixed Entrances option if 1 or no applicable entrance shuffles are visible
        if (dungeonShuffle + bossShuffle + overworldShuffle + interiorShuffle + grottoShuffle + thievesHideoutShuffle <=
            1) {
            mOptions[RSK_MIXED_ENTRANCE_POOLS].Hide();
        } else {
            mOptions[RSK_MIXED_ENTRANCE_POOLS].Unhide();
        }
    }
    void HandleStartingAgeUI() {
        // Starting Age - Disabled under very specific conditions unless it's No Logic
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("LogicRules"), RO_LOGIC_GLITCHLESS) != RO_LOGIC_NO_LOGIC &&
            // If Closed DoT requires OoT then we can only start as child
            ((CVarGetInteger(CVAR_RANDOMIZER_SETTING("DoorOfTime"), RO_DOOROFTIME_CLOSED) == RO_DOOROFTIME_CLOSED &&
              CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleOcarinas"), RO_GENERIC_OFF) == RO_GENERIC_OFF) ||
             // If Forest is Closed, we cannot start as Adult unless there's a sphere 0 entrance shuffle in Kokiri
             // forest, or there's random spawns, as the player may saveload as child and get stuck. Grottos only lead
             // somewhere if decoupled
             (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ClosedForest"), RO_CLOSED_FOREST_ON) == RO_CLOSED_FOREST_ON &&
              CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleOverworldSpawns"), RO_GENERIC_OFF) == RO_GENERIC_OFF &&
              (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleGrottosEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF ||
               CVarGetInteger(CVAR_RANDOMIZER_SETTING("DecoupleEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) &&
              CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleInteriorsEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF &&
              CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleOverworldEntrances"), RO_GENERIC_OFF) ==
                  RO_GENERIC_OFF))) {
            mOptions[RSK_STARTING_AGE].Disable(
                "This option is disabled due to other options making the game unbeatable.");
        } else {
            mOptions[RSK_STARTING_AGE].Enable();
        }
#ifdef COMBO_BUILD
        // ComboShip (#135): an explicit MM start forces Child, so that disable outranks the rule above.
        if (CVarGetInteger("gCombo.Rando.StartingGame", 0) == 1) {
            mOptions[RSK_STARTING_AGE].Disable("Starting Game is Majora's Mask");
        }
#endif
    }
    void Callback_RSK_FOREST() {
        HandleStartingAgeUI();
    }
    void Callback_RSK_DOOR_OF_TIME() {
        HandleStartingAgeUI();
    }
    void Callback_RSK_LOCK_OVERWORLD_DOORS() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("LockOverworldDoors"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_EXCLUDE_MASK_SHOP_KEY].Hide();
        } else {
            mOptions[RSK_EXCLUDE_MASK_SHOP_KEY].Unhide();
        }
    }
    void Callback_RSK_GERUDO_FORTRESS() {
        const uint8_t maxKeyringCount =
            (CVarGetInteger(CVAR_RANDOMIZER_SETTING("FortressCarpenters"), RO_GF_CARPENTERS_NORMAL) ==
                 RO_GF_CARPENTERS_NORMAL &&
             CVarGetInteger(CVAR_RANDOMIZER_SETTING("GerudoKeys"), RO_GERUDO_KEYS_VANILLA) != RO_GERUDO_KEYS_VANILLA)
                ? 9
                : 8;
        if (mOptions[RSK_KEYRINGS_RANDOM_COUNT].GetOptionCount() != maxKeyringCount + 1) {
            mOptions[RSK_KEYRINGS_RANDOM_COUNT].ChangeOptions(NumOpts(0, maxKeyringCount));
        }
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("FortressCarpenters"), RO_GF_CARPENTERS_NORMAL) !=
                RO_GF_CARPENTERS_NORMAL ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("GerudoKeys"), RO_GERUDO_KEYS_VANILLA) == RO_GERUDO_KEYS_VANILLA) {
            mOptions[RSK_KEYRINGS_GERUDO_FORTRESS].Disable(
                "Disabled because the currently selected Gerudo Fortress Carpenters\n"
                "setting and/or Gerudo Fortress Keys setting is incompatible with\n"
                "having a Gerudo Fortress Keyring.");
        } else {
            mOptions[RSK_KEYRINGS_GERUDO_FORTRESS].Enable();
        }
    }
    void Callback_RSK_RAINBOW_BRIDGE() {
        mOptions[RSK_BRIDGE_OPTIONS].Hide();
        mOptions[RSK_RAINBOW_BRIDGE_STONE_COUNT].Hide();
        mOptions[RSK_RAINBOW_BRIDGE_MEDALLION_COUNT].Hide();
        mOptions[RSK_RAINBOW_BRIDGE_REWARD_COUNT].Hide();
        mOptions[RSK_RAINBOW_BRIDGE_DUNGEON_COUNT].Hide();
        mOptions[RSK_RAINBOW_BRIDGE_TOKEN_COUNT].Hide();
        mOptions[RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT].Hide();
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("RainbowBridge"), RO_BRIDGE_VANILLA)) {
            case RO_BRIDGE_STONES:
                mOptions[RSK_RAINBOW_BRIDGE].RemoveFlag(IMFLAG_SEPARATOR_BOTTOM);
                mOptions[RSK_BRIDGE_OPTIONS].Unhide();
                mOptions[RSK_RAINBOW_BRIDGE_STONE_COUNT].Unhide();
                break;
            case RO_BRIDGE_MEDALLIONS:
                mOptions[RSK_RAINBOW_BRIDGE].RemoveFlag(IMFLAG_SEPARATOR_BOTTOM);
                mOptions[RSK_BRIDGE_OPTIONS].Unhide();
                mOptions[RSK_RAINBOW_BRIDGE_MEDALLION_COUNT].Unhide();
                break;
            case RO_BRIDGE_DUNGEON_REWARDS:
                mOptions[RSK_RAINBOW_BRIDGE].RemoveFlag(IMFLAG_SEPARATOR_BOTTOM);
                mOptions[RSK_BRIDGE_OPTIONS].Unhide();
                mOptions[RSK_RAINBOW_BRIDGE_REWARD_COUNT].Unhide();
                break;
            case RO_BRIDGE_DUNGEONS:
                mOptions[RSK_RAINBOW_BRIDGE].RemoveFlag(IMFLAG_SEPARATOR_BOTTOM);
                mOptions[RSK_BRIDGE_OPTIONS].Unhide();
                mOptions[RSK_RAINBOW_BRIDGE_DUNGEON_COUNT].Unhide();
                break;
            case RO_BRIDGE_TOKENS:
                mOptions[RSK_RAINBOW_BRIDGE].RemoveFlag(IMFLAG_SEPARATOR_BOTTOM);
                mOptions[RSK_BRIDGE_OPTIONS].Hide();
                mOptions[RSK_RAINBOW_BRIDGE_TOKEN_COUNT].Unhide();
                break;
            case RO_BRIDGE_TRIFORCE_PIECES:
                mOptions[RSK_RAINBOW_BRIDGE].RemoveFlag(IMFLAG_SEPARATOR_BOTTOM);
                mOptions[RSK_BRIDGE_OPTIONS].Hide();
                mOptions[RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT].Unhide();
                break;
            default:
                break;
        }
    }
    void Callback_RSK_BRIDGE_OPTIONS() {
        const uint8_t bridgeOpt =
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("BridgeRewardOptions"), RO_BRIDGE_STANDARD_REWARD);
        if (bridgeOpt == RO_BRIDGE_GREG_REWARD) {
            mOptions[RSK_RAINBOW_BRIDGE_STONE_COUNT].ChangeOptions(NumOpts(0, 4));
            mOptions[RSK_RAINBOW_BRIDGE_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 7));
            mOptions[RSK_RAINBOW_BRIDGE_REWARD_COUNT].ChangeOptions(NumOpts(0, 10));
            mOptions[RSK_RAINBOW_BRIDGE_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 9));
        } else {
            mOptions[RSK_RAINBOW_BRIDGE_STONE_COUNT].ChangeOptions(NumOpts(0, 3));
            mOptions[RSK_RAINBOW_BRIDGE_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 6));
            mOptions[RSK_RAINBOW_BRIDGE_REWARD_COUNT].ChangeOptions(NumOpts(0, 9));
            mOptions[RSK_RAINBOW_BRIDGE_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 8));
        }
    }
    void Callback_RSK_GANONS_TRIALS() {
        // Only show the trial count slider if Trials is set to Set Number
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("GanonTrial"), RO_GANONS_TRIALS_SET_NUMBER) ==
            RO_GANONS_TRIALS_SET_NUMBER) {
            mOptions[RSK_TRIAL_COUNT].Unhide();
        } else {
            mOptions[RSK_TRIAL_COUNT].Hide();
        }
    }
    void Callback_RSK_SHUFFLE_DUNGEON_ENTRANCES() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonsEntrances"), RO_DUNGEON_ENTRANCE_SHUFFLE_OFF) ==
                RO_DUNGEON_ENTRANCE_SHUFFLE_OFF ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("MixedEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_MIX_DUNGEON_ENTRANCES].Hide();
        } else {
            mOptions[RSK_MIX_DUNGEON_ENTRANCES].Unhide();
        }
    }
    void Callback_RSK_SHUFFLE_BOSS_ENTRANCES() {
        HandleMixedEntrancePoolsUI();

        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleBossEntrances"), RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF) ==
            RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF) {
            mOptions[RSK_SHUFFLE_GANONS_TOWER_ENTRANCE].Hide();
        } else {
            mOptions[RSK_SHUFFLE_GANONS_TOWER_ENTRANCE].Unhide();
        }

        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleBossEntrances"), RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF) ==
                RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("MixedEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_MIX_BOSS_ENTRANCES].Hide();
        } else {
            mOptions[RSK_MIX_BOSS_ENTRANCES].Unhide();
        }
    }
    void Callback_RSK_SHUFFLE_OVERWORLD_ENTRANCES() {
        HandleMixedEntrancePoolsUI();

        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleOverworldEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("MixedEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_MIX_OVERWORLD_ENTRANCES].Hide();
        } else {
            mOptions[RSK_MIX_OVERWORLD_ENTRANCES].Unhide();
        }

        HandleStartingAgeUI();
    }
    void Callback_RSK_SHUFFLE_INTERIOR_ENTRANCES() {
        HandleMixedEntrancePoolsUI();

        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleInteriorsEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("MixedEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_MIX_INTERIOR_ENTRANCES].Hide();
        } else {
            mOptions[RSK_MIX_INTERIOR_ENTRANCES].Unhide();
        }

        // ComboShip: (#134)
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleInteriorsEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_EXCLUDE_MASK_SHOP_ENTRANCE].Hide();
        } else {
            mOptions[RSK_EXCLUDE_MASK_SHOP_ENTRANCE].Unhide();
        }

        HandleStartingAgeUI();
    }
    void Callback_RSK_SHUFFLE_THIEVES_HIDEOUT_ENTRANCES() {
        HandleMixedEntrancePoolsUI();

        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleThievesHideoutEntrances"), RO_GENERIC_OFF) ==
                RO_GENERIC_OFF ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("MixedEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_MIX_THIEVES_HIDEOUT_ENTRANCES].Hide();
        } else {
            mOptions[RSK_MIX_THIEVES_HIDEOUT_ENTRANCES].Unhide();
        }
    }
    void Callback_RSK_SHUFFLE_GROTTO_ENTRANCES() {
        HandleMixedEntrancePoolsUI();

        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleGrottosEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("MixedEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF) {
            mOptions[RSK_MIX_GROTTO_ENTRANCES].Hide();
        } else {
            mOptions[RSK_MIX_GROTTO_ENTRANCES].Unhide();
        }

        HandleStartingAgeUI();
    }
    void Callback_RSK_SHUFFLE_WARP_SONGS() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleWarpSongs"), RO_GENERIC_ON)) {
            mOptions[RSK_WARP_SONG_HINTS].Enable();
        } else {
            mOptions[RSK_WARP_SONG_HINTS].Disable(
                "This option is disabled since warp song locations are not shuffled.");
        }
    }
    void Callback_RSK_SHUFFLE_OVERWORLD_SPAWNS() {
        HandleStartingAgeUI();
    }
    void Callback_RSK_MIXED_ENTRANCE_POOLS() {
        // Show mixed entrance pool options if mixed entrance pools are enabled, but only the ones that aren't off
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MixedEntrances"), RO_GENERIC_OFF) == RO_GENERIC_OFF ||
            mOptions[RSK_MIXED_ENTRANCE_POOLS].IsHidden()) {
            mOptions[RSK_MIXED_ENTRANCE_POOLS].AddFlag(IMFLAG_SEPARATOR_BOTTOM);
            mOptions[RSK_MIX_DUNGEON_ENTRANCES].Hide();
            mOptions[RSK_MIX_BOSS_ENTRANCES].Hide();
            mOptions[RSK_MIX_OVERWORLD_ENTRANCES].Hide();
            mOptions[RSK_MIX_INTERIOR_ENTRANCES].Hide();
            mOptions[RSK_MIX_THIEVES_HIDEOUT_ENTRANCES].Hide();
            mOptions[RSK_MIX_GROTTO_ENTRANCES].Hide();
        } else {
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonsEntrances"), RO_DUNGEON_ENTRANCE_SHUFFLE_OFF) !=
                RO_DUNGEON_ENTRANCE_SHUFFLE_OFF) {
                mOptions[RSK_MIX_DUNGEON_ENTRANCES].Unhide();
            }
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleBossEntrances"), RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF) !=
                RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF) {
                mOptions[RSK_MIX_BOSS_ENTRANCES].Unhide();
            }
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleOverworldEntrances"), RO_GENERIC_OFF) !=
                RO_GENERIC_OFF) {
                mOptions[RSK_MIX_OVERWORLD_ENTRANCES].Unhide();
            }
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleInteriorsEntrances"), RO_GENERIC_OFF) !=
                RO_GENERIC_OFF) {
                mOptions[RSK_MIX_INTERIOR_ENTRANCES].Unhide();
            }
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleThievesHideoutEntrances"), RO_GENERIC_OFF) !=
                RO_GENERIC_OFF) {
                mOptions[RSK_MIX_THIEVES_HIDEOUT_ENTRANCES].Unhide();
            }
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleGrottosEntrances"), RO_GENERIC_OFF) != RO_GENERIC_OFF) {
                mOptions[RSK_MIX_GROTTO_ENTRANCES].Unhide();
            }
        }
    }
    void Callback_RSK_DECOUPLED_ENTRANCES() {
        HandleStartingAgeUI();
    }
    void Callback_RSK_TRIFORCE_HUNT_PIECES_TOTAL() {
        uint8_t triforceTotal = CVarGetInteger(CVAR_RANDOMIZER_SETTING("TriforceHuntTotalPieces"), 0);
        // ComboShip: the cross-world fill can't honor a dungeon/overworld restriction â€” forced Anywhere.
        if (triforceTotal == 0 || kComboOwnsWincon) {
            mOptions[RSK_TRIFORCE_HUNT_PIECES_LOCATION].Hide();
        } else {
            mOptions[RSK_TRIFORCE_HUNT_PIECES_LOCATION].Unhide();
        }
        // ComboShip (#136): the combo menu owns the total, so hide this slider and range the dependent
        // counts below against OOT's forced share instead of this (now dead) CVar.
        if (kComboOwnsWincon) {
            mOptions[RSK_TRIFORCE_HUNT_PIECES_TOTAL].Hide();
            triforceTotal = ComboOotTriforceTotal();
        }
        if (mOptions[RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT].GetOptionCount() != triforceTotal + 1) {
            mOptions[RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT].ChangeOptions(NumOpts(0, triforceTotal));
        }
        if (mOptions[RSK_GBK_TRIFORCE_COUNT].GetOptionCount() != triforceTotal + 1) {
            mOptions[RSK_GBK_TRIFORCE_COUNT].ChangeOptions(NumOpts(0, triforceTotal));
        }
        if (mOptions[RSK_GANONS_SOUL_TRIFORCE_COUNT].GetOptionCount() != triforceTotal + 1) {
            mOptions[RSK_GANONS_SOUL_TRIFORCE_COUNT].ChangeOptions(NumOpts(0, triforceTotal));
        }
        if (mOptions[RSK_WINCON_TRIFORCE_COUNT].GetOptionCount() != triforceTotal + 1) {
            mOptions[RSK_WINCON_TRIFORCE_COUNT].ChangeOptions(NumOpts(0, triforceTotal));
        }
    }
    void Callback_RSK_MQ_DUNGEON_RANDOM() {
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeons"), RO_MQ_DUNGEONS_NONE)) {
            // If No MQ Dungeons, add a separator after the combobx and hide
            // the count slider and the toggle for individual dungeon selections.
            case RO_MQ_DUNGEONS_NONE:
                mOptions[RSK_MQ_DUNGEON_COUNT].Hide();
                mOptions[RSK_MQ_DUNGEON_SET].Hide();
                break;
            // If Set Number, remove the separator and show both the count slider and the
            // individual dungeon selection toggle.
            case RO_MQ_DUNGEONS_SET_NUMBER:
                mOptions[RSK_MQ_DUNGEON_COUNT].Unhide();
                mOptions[RSK_MQ_DUNGEON_SET].Unhide();
                break;
            // else if random number or selection only, remove the separator and only show
            // the individual dungeon selection toggle.
            case RO_MQ_DUNGEONS_RANDOM_NUMBER:
                mOptions[RSK_MQ_DUNGEON_COUNT].Hide();
                mOptions[RSK_MQ_DUNGEON_SET].Unhide();
                break;
            case RO_MQ_DUNGEONS_SELECTION:
                mOptions[RSK_MQ_DUNGEON_COUNT].Hide();
                mOptions[RSK_MQ_DUNGEON_SET].Hide();
                break;
            default:
                break;
        }
        // Controls whether or not to show the selectors for individual dungeons.
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeons"), RO_MQ_DUNGEONS_NONE) != RO_MQ_DUNGEONS_NONE &&
            (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeonsSelection"), RO_GENERIC_OFF) == RO_GENERIC_ON ||
             CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeons"), RO_MQ_DUNGEONS_NONE) == RO_MQ_DUNGEONS_SELECTION)) {
            // if showing the dungeon selectors, remove the separator after the Set Dungeons checkbox.
            mOptions[RSK_MQ_DEKU_TREE].Unhide();
            mOptions[RSK_MQ_DODONGOS_CAVERN].Unhide();
            mOptions[RSK_MQ_JABU_JABU].Unhide();
            mOptions[RSK_MQ_FOREST_TEMPLE].Unhide();
            mOptions[RSK_MQ_FIRE_TEMPLE].Unhide();
            mOptions[RSK_MQ_WATER_TEMPLE].Unhide();
            mOptions[RSK_MQ_SPIRIT_TEMPLE].Unhide();
            mOptions[RSK_MQ_SHADOW_TEMPLE].Unhide();
            mOptions[RSK_MQ_BOTTOM_OF_THE_WELL].Unhide();
            mOptions[RSK_MQ_ICE_CAVERN].Unhide();
            mOptions[RSK_MQ_GTG].Unhide();
            mOptions[RSK_MQ_GANONS_CASTLE].Unhide();
        } else {
            // If those are not shown, add a separator after the Set Dungeons checkbox.
            mOptions[RSK_MQ_DEKU_TREE].Hide();
            mOptions[RSK_MQ_DODONGOS_CAVERN].Hide();
            mOptions[RSK_MQ_JABU_JABU].Hide();
            mOptions[RSK_MQ_FOREST_TEMPLE].Hide();
            mOptions[RSK_MQ_FIRE_TEMPLE].Hide();
            mOptions[RSK_MQ_WATER_TEMPLE].Hide();
            mOptions[RSK_MQ_SPIRIT_TEMPLE].Hide();
            mOptions[RSK_MQ_SHADOW_TEMPLE].Hide();
            mOptions[RSK_MQ_BOTTOM_OF_THE_WELL].Hide();
            mOptions[RSK_MQ_ICE_CAVERN].Hide();
            mOptions[RSK_MQ_GTG].Hide();
            mOptions[RSK_MQ_GANONS_CASTLE].Hide();
        }
    }
    void Callback_RSK_MQ_DUNGEON_SET() {
        // Controls whether or not to show the selectors for individual dungeons.
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeons"), RO_MQ_DUNGEONS_NONE) != RO_MQ_DUNGEONS_NONE &&
            (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeonsSelection"), RO_GENERIC_OFF) == RO_GENERIC_ON ||
             CVarGetInteger(CVAR_RANDOMIZER_SETTING("MQDungeons"), RO_MQ_DUNGEONS_NONE) == RO_MQ_DUNGEONS_SELECTION)) {
            // if showing the dungeon selectors, remove the separator after the Set Dungeons checkbox.
            mOptions[RSK_MQ_DEKU_TREE].Unhide();
            mOptions[RSK_MQ_DODONGOS_CAVERN].Unhide();
            mOptions[RSK_MQ_JABU_JABU].Unhide();
            mOptions[RSK_MQ_FOREST_TEMPLE].Unhide();
            mOptions[RSK_MQ_FIRE_TEMPLE].Unhide();
            mOptions[RSK_MQ_WATER_TEMPLE].Unhide();
            mOptions[RSK_MQ_SPIRIT_TEMPLE].Unhide();
            mOptions[RSK_MQ_SHADOW_TEMPLE].Unhide();
            mOptions[RSK_MQ_BOTTOM_OF_THE_WELL].Unhide();
            mOptions[RSK_MQ_ICE_CAVERN].Unhide();
            mOptions[RSK_MQ_GTG].Unhide();
            mOptions[RSK_MQ_GANONS_CASTLE].Unhide();
        } else {
            // If those are not shown, add a separator after the Set Dungeons checkbox.
            mOptions[RSK_MQ_DEKU_TREE].Hide();
            mOptions[RSK_MQ_DODONGOS_CAVERN].Hide();
            mOptions[RSK_MQ_JABU_JABU].Hide();
            mOptions[RSK_MQ_FOREST_TEMPLE].Hide();
            mOptions[RSK_MQ_FIRE_TEMPLE].Hide();
            mOptions[RSK_MQ_WATER_TEMPLE].Hide();
            mOptions[RSK_MQ_SPIRIT_TEMPLE].Hide();
            mOptions[RSK_MQ_SHADOW_TEMPLE].Hide();
            mOptions[RSK_MQ_BOTTOM_OF_THE_WELL].Hide();
            mOptions[RSK_MQ_ICE_CAVERN].Hide();
            mOptions[RSK_MQ_GTG].Hide();
            mOptions[RSK_MQ_GANONS_CASTLE].Hide();
        }
    }
    void Callback_RSK_SHUFFLE_DUNGEON_REWARDS() {
        // Link's Pocket - Disabled when Dungeon Rewards are shuffled to End of Dungeon
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonReward"), RO_DUNGEON_REWARDS_END_OF_DUNGEON) ==
            RO_DUNGEON_REWARDS_END_OF_DUNGEON) {
            mOptions[RSK_LINKS_POCKET].Disable(
                "This option is disabled because \"Dungeon Rewards\" are shuffled to \"End of Dungeons\".");
            mOptions[RSK_LINKS_POCKET_REWARD].Enable();
            mOptions[RSK_LINKS_POCKET_REWARD].Unhide();
        } else {
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonReward"), RO_DUNGEON_REWARDS_END_OF_DUNGEON) ==
                RO_DUNGEON_REWARDS_OWN_DUNGEON) {
                mOptions[RSK_LINKS_POCKET].Enable();
                mOptions[RSK_LINKS_POCKET_REWARD].Disable(
                    "As \"Link's Pocket\" is set to \"Dungeon Reward\" while \"Dungeon Rewards\" is set to \"Own "
                    "Dungeon\", Link's Pocket will always have the Light Medallion");
            } else if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonReward"),
                                      RO_DUNGEON_REWARDS_END_OF_DUNGEON) == RO_DUNGEON_REWARDS_VANILLA) {
                mOptions[RSK_LINKS_POCKET].Enable();
                mOptions[RSK_LINKS_POCKET_REWARD].Disable(
                    "As \"Link's Pocket\" is set to \"Dungeon Reward\" while \"Dungeon Rewards\" is set to "
                    "\"Vanilla\", Link's Pocket will always have the Light Medallion");
            } else {
                mOptions[RSK_LINKS_POCKET].Enable();
                mOptions[RSK_LINKS_POCKET_REWARD].Enable();
            }
            if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("LinksPocket"), RO_LINKS_POCKET_DUNGEON_REWARD) ==
                RO_LINKS_POCKET_DUNGEON_REWARD) {
                mOptions[RSK_LINKS_POCKET_REWARD].Unhide();
            } else {
                mOptions[RSK_LINKS_POCKET_REWARD].Hide();
            }
        }
    }
    void Callback_RSK_LINKS_POCKET() {
        // Only show the dungeon reward type if Link's Pocket is set to Dungeon Reward and Dungeon Rewards are not
        // Vanilla, OR Dungeon Rewards are end of dungeon
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("LinksPocket"), RO_LINKS_POCKET_DUNGEON_REWARD) ==
                RO_LINKS_POCKET_DUNGEON_REWARD ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDungeonReward"), RO_DUNGEON_REWARDS_END_OF_DUNGEON) ==
                RO_DUNGEON_REWARDS_END_OF_DUNGEON) {
            mOptions[RSK_LINKS_POCKET_REWARD].Unhide();
        } else {
            mOptions[RSK_LINKS_POCKET_REWARD].Hide();
        }
    }
    void Callback_RSK_SHOPSANITY() {
        // Hide shopsanity prices if shopsanity is off or zero
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("Shopsanity"), RO_SHOPSANITY_OFF)) {
            case RO_SHOPSANITY_OFF:
                mOptions[RSK_SHOPSANITY].AddFlag(IMFLAG_SEPARATOR_BOTTOM);
                mOptions[RSK_SHOPSANITY_COUNT].Hide();
                mOptions[RSK_SHOPSANITY_COUNT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_AFFORDABLE].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_1].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_RANGE_2].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SHOPSANITY_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                break;
            case RO_SHOPSANITY_SPECIFIC_COUNT:
                mOptions[RSK_SHOPSANITY_COUNT].Unhide();
                HandleShopsanityPriceUI();
                break;
            case RO_SHOPSANITY_RANDOM:
                mOptions[RSK_SHOPSANITY_COUNT].Hide();
                HandleShopsanityPriceUI();
                break;
        }
    }
    void Callback_RSK_SHOPSANITY_PRICES() {
        HandleShopsanityPriceUI();
    }
    void Callback_RSK_SHUFFLE_SCRUBS() {
        bool isTycoon = CVarGetInteger(CVAR_RANDOMIZER_SETTING("IncludeTycoonWallet"), RO_GENERIC_OFF);
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleScrubs"), RO_SCRUBS_OFF)) {
            case RO_SCRUBS_OFF:
                mOptions[RSK_SCRUBS_PRICES].Hide();
                mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Hide();
                mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_1].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_2].Hide();
                mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                break;
            default:
                mOptions[RSK_SCRUBS_PRICES].Unhide();
                switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ScrubsPrices"), RO_PRICE_VANILLA)) {
                    case RO_PRICE_FIXED:
                        mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Unhide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_1].Hide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_2].Hide();
                        mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        if (isTycoon ? mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].GetOptionCount() == 501
                                     : mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].GetOptionCount() == 1000) {
                            mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].ChangeOptions(isTycoon ? NumOpts(0, 999)
                                                                                           : NumOpts(0, 500));
                        }
                        mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Hide();
                        break;
                    case RO_PRICE_RANGE:
                        mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Hide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_1].Unhide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_2].Unhide();
                        mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        if (isTycoon ? mOptions[RSK_SCRUBS_PRICES_RANGE_1].GetOptionCount() == 101
                                     : mOptions[RSK_SCRUBS_PRICES_RANGE_1].GetOptionCount() == 200) {
                            mOptions[RSK_SCRUBS_PRICES_RANGE_1].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                       : NumOpts(0, 500, 5));
                            mOptions[RSK_SCRUBS_PRICES_RANGE_2].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                       : NumOpts(0, 500, 5));
                        }
                        mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Unhide();
                        break;
                    case RO_PRICE_SET_BY_WALLET:
                        mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Hide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_1].Hide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_2].Hide();
                        mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Unhide();
                        mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Unhide();
                        mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Unhide();
                        mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Unhide();
                        if (isTycoon) {
                            mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Unhide();
                        } else {
                            mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        }
                        mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Unhide();
                        break;
                    default:
                        mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Hide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_1].Hide();
                        mOptions[RSK_SCRUBS_PRICES_RANGE_2].Hide();
                        mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Unhide();
                        break;
                }
                break;
        }
    }
    void Callback_RSK_SCRUBS_PRICES() {
        bool isTycoon = CVarGetInteger(CVAR_RANDOMIZER_SETTING("IncludeTycoonWallet"), RO_GENERIC_OFF);
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ScrubsPrices"), RO_PRICE_VANILLA)) {
            case RO_PRICE_FIXED:
                mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Unhide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_1].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_2].Hide();
                mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                if (isTycoon ? mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].GetOptionCount() == 501
                             : mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].GetOptionCount() == 1000) {
                    mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].ChangeOptions(isTycoon ? NumOpts(0, 999) : NumOpts(0, 500));
                }
                mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Hide();
                break;
            case RO_PRICE_RANGE:
                mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_1].Unhide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_2].Unhide();
                mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                if (isTycoon ? mOptions[RSK_SCRUBS_PRICES_RANGE_1].GetOptionCount() == 101
                             : mOptions[RSK_SCRUBS_PRICES_RANGE_1].GetOptionCount() == 200) {
                    mOptions[RSK_SCRUBS_PRICES_RANGE_1].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                               : NumOpts(0, 500, 5));
                    mOptions[RSK_SCRUBS_PRICES_RANGE_2].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                               : NumOpts(0, 500, 5));
                }
                mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Unhide();
                break;
            case RO_PRICE_SET_BY_WALLET:
                mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_1].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_2].Hide();
                mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Unhide();
                mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Unhide();
                mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Unhide();
                mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Unhide();
                if (isTycoon) {
                    mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Unhide();
                } else {
                    mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                }
                mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Unhide();
                break;
            default:
                mOptions[RSK_SCRUBS_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_1].Hide();
                mOptions[RSK_SCRUBS_PRICES_RANGE_2].Hide();
                mOptions[RSK_SCRUBS_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                mOptions[RSK_SCRUBS_PRICES_AFFORDABLE].Unhide();
                break;
        }
    }
    void Callback_RSK_SHUFFLE_BEEHIVES() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleBeehives"), RO_GENERIC_OFF)) {
            mOptions[RSK_SLINGBOW_BREAK_BEEHIVES].Enable();
        } else {
            mOptions[RSK_SLINGBOW_BREAK_BEEHIVES].Disable(
                "This option is disabled because Shuffle Beehives is not enabled.");
        }
    }
    void Callback_RSK_SHUFFLE_COWS() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleCows"), RO_GENERIC_OFF)) {
            mOptions[RSK_MALON_HINT].Enable();
        } else {
            mOptions[RSK_MALON_HINT].Disable("Malon's hint points to a cow, so requires cows to be shuffled.");
        }
    }
    void Callback_RSK_SHUFFLE_CHILD_WALLET() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleChildWallet"), 0)) {
            CVarSetInteger(CVAR_RANDOMIZER_SETTING("StartingWallet"), 0);
            mOptions[RSK_STARTING_WALLET].Disable("Disabled because Shuffle Child's Wallet is on.");
        } else {
            mOptions[RSK_STARTING_WALLET].Enable();
        }
    }
    void Callback_RSK_SHUFFLE_OCARINA() {
        HandleStartingAgeUI();
    }
    void Callback_RSK_SHUFFLE_SWIM() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleSwim"), 0)) {
            CVarSetInteger(CVAR_RANDOMIZER_SETTING("StartingScale"), 0);
            mOptions[RSK_STARTING_SCALE].Disable("Disabled because Shuffle Swim is on.");
        } else {
            mOptions[RSK_STARTING_SCALE].Enable();
        }
    }
    void Callback_RSK_SHUFFLE_GRAB() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleGrab"), 0)) {
            CVarSetInteger(CVAR_RANDOMIZER_SETTING("StartingStrength"), 0);
            mOptions[RSK_STARTING_STRENGTH].Disable("Disabled because Shuffle Grab is on.");
        } else {
            mOptions[RSK_STARTING_STRENGTH].Enable();
        }
    }
    void Callback_RSK_SHUFFLE_FISHING_POLE() {
        // Disable fishing pole hint if the fishing pole is not shuffled
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleFishingPole"), RO_GENERIC_OFF)) {
            mOptions[RSK_FISHING_POLE_HINT].Enable();
        } else {
            mOptions[RSK_FISHING_POLE_HINT].Disable("This option is disabled since the fishing pole is not shuffled.");
        }
    }
    void Callback_RSK_SHUFFLE_MERCHANTS() {
        bool isTycoon = CVarGetInteger(CVAR_RANDOMIZER_SETTING("IncludeTycoonWallet"), RO_GENERIC_OFF);
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleMerchants"), RO_SHUFFLE_MERCHANTS_OFF)) {
            case RO_SHUFFLE_MERCHANTS_OFF:
                mOptions[RSK_MERCHANT_PRICES].Hide();
                mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Hide();
                mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_1].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_2].Hide();
                mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                break;
            default:
                mOptions[RSK_MERCHANT_PRICES].Unhide();
                switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MerchantPrices"), RO_PRICE_VANILLA)) {
                    case RO_PRICE_FIXED:
                        mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Unhide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_1].Hide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_2].Hide();
                        mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        if (isTycoon ? mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].GetOptionCount() == 501
                                     : mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].GetOptionCount() == 1000) {
                            mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].ChangeOptions(isTycoon ? NumOpts(0, 999)
                                                                                             : NumOpts(0, 500));
                        }
                        mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Hide();
                        break;
                    case RO_PRICE_RANGE:
                        mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Hide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_1].Unhide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_2].Unhide();
                        mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        if (isTycoon ? mOptions[RSK_MERCHANT_PRICES_RANGE_1].GetOptionCount() == 101
                                     : mOptions[RSK_MERCHANT_PRICES_RANGE_1].GetOptionCount() == 200) {
                            mOptions[RSK_MERCHANT_PRICES_RANGE_1].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                         : NumOpts(0, 500, 5));
                            mOptions[RSK_MERCHANT_PRICES_RANGE_2].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                         : NumOpts(0, 500, 5));
                        }
                        mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Unhide();
                        break;
                    case RO_PRICE_SET_BY_WALLET:
                        mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Hide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_1].Hide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_2].Hide();
                        mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Unhide();
                        mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Unhide();
                        mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Unhide();
                        mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Unhide();
                        if (isTycoon) {
                            mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Unhide();
                        } else {
                            mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        }
                        mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Unhide();
                        break;
                    default:
                        mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Hide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_1].Hide();
                        mOptions[RSK_MERCHANT_PRICES_RANGE_2].Hide();
                        mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                        mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Unhide();
                        break;
                }
                break;
        }
    }
    void Callback_RSK_MERCHANT_PRICES() {
        bool isTycoon = CVarGetInteger(CVAR_RANDOMIZER_SETTING("IncludeTycoonWallet"), RO_GENERIC_OFF);
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("MerchantPrices"), RO_PRICE_VANILLA)) {
            case RO_PRICE_FIXED:
                mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Unhide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_1].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_2].Hide();
                mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                if (isTycoon ? mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].GetOptionCount() == 501
                             : mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].GetOptionCount() == 1000) {
                    mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].ChangeOptions(isTycoon ? NumOpts(0, 999)
                                                                                     : NumOpts(0, 500));
                }
                mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Hide();
                break;
            case RO_PRICE_RANGE:
                mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_1].Unhide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_2].Unhide();
                mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                if (isTycoon ? mOptions[RSK_MERCHANT_PRICES_RANGE_1].GetOptionCount() == 101
                             : mOptions[RSK_MERCHANT_PRICES_RANGE_1].GetOptionCount() == 200) {
                    mOptions[RSK_MERCHANT_PRICES_RANGE_1].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                 : NumOpts(0, 500, 5));
                    mOptions[RSK_MERCHANT_PRICES_RANGE_2].ChangeOptions(isTycoon ? NumOpts(0, 995, 5)
                                                                                 : NumOpts(0, 500, 5));
                }
                mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Unhide();
                break;
            case RO_PRICE_SET_BY_WALLET:
                mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_1].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_2].Hide();
                mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Unhide();
                mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Unhide();
                mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Unhide();
                mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Unhide();
                if (isTycoon) {
                    mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Unhide();
                } else {
                    mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                }
                mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Unhide();
                break;
            default:
                mOptions[RSK_MERCHANT_PRICES_FIXED_PRICE].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_1].Hide();
                mOptions[RSK_MERCHANT_PRICES_RANGE_2].Hide();
                mOptions[RSK_MERCHANT_PRICES_NO_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_CHILD_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_ADULT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_GIANT_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_TYCOON_WALLET_WEIGHT].Hide();
                mOptions[RSK_MERCHANT_PRICES_AFFORDABLE].Unhide();
                break;
        }
    }
    void Callback_RSK_SHUFFLE_ADULT_TRADE() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleAdultTrade"), RO_GENERIC_OFF)) {
            mOptions[RSK_EARLY_GRANNYS_SHOP].Disable("This has no effect when Shuffle Adult Trade is on.");
        } else {
            mOptions[RSK_EARLY_GRANNYS_SHOP].Enable();
        }
    }
    void Callback_RSK_SHUFFLE_100_GS_REWARD() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("Shuffle100GSReward"), RO_GENERIC_OFF)) {
            mOptions[RSK_KAK_100_SKULLS_HINT].Enable();
        } else {
            mOptions[RSK_KAK_100_SKULLS_HINT].Disable("There is no point to hinting 100 skulls if it is not shuffled.");
        }
    }
    void Callback_RSK_SHUFFLE_DEKU_STICK_BAG() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDekuStickBag"), 0)) {
            mOptions[RSK_STARTING_STICKS].Disable("Disabled because Shuffle Deku Stick Bag is on.");
        } else {
            mOptions[RSK_STARTING_STICKS].Enable();
        }
    }
    void Callback_RSK_SHUFFLE_DEKU_NUT_BAG() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleDekuNutBag"), 0)) {
            mOptions[RSK_STARTING_NUTS].Disable("Disabled because Shuffle Deku Nut Bag is on.");
        } else {
            mOptions[RSK_STARTING_NUTS].Enable();
        }
    }
    void Callback_RSK_FISHSANITY() {
        // Hide fishing pond settings if we aren't shuffling the fishing pond
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("Fishsanity"), RO_FISHSANITY_OFF)) {
            case RO_FISHSANITY_POND:
            case RO_FISHSANITY_BOTH:
                mOptions[RSK_FISHSANITY_POND_COUNT].Unhide();
                mOptions[RSK_FISHSANITY_AGE_SPLIT].Unhide();
                break;
            default:
                mOptions[RSK_FISHSANITY_POND_COUNT].Hide();
                mOptions[RSK_FISHSANITY_AGE_SPLIT].Hide();
        }
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("Fishsanity"), RO_FISHSANITY_OFF) == RO_FISHSANITY_HYRULE_LOACH) {
            mOptions[RSK_LOACH_HINT].Enable();
        } else {
            mOptions[RSK_LOACH_HINT].Disable("Loach hint is only avaliable with \"Fishsanity\" set to \"Shuffle only "
                                             "Hyrule Loach\"\nas that's the only "
                                             "setting where you present the loach to the fishing pond owner.");
        }
    }
    void Callback_RSK_GERUDO_KEYS() {
        const uint8_t maxKeyringCount =
            (CVarGetInteger(CVAR_RANDOMIZER_SETTING("FortressCarpenters"), RO_GF_CARPENTERS_NORMAL) ==
                 RO_GF_CARPENTERS_NORMAL &&
             CVarGetInteger(CVAR_RANDOMIZER_SETTING("GerudoKeys"), RO_GERUDO_KEYS_VANILLA) != RO_GERUDO_KEYS_VANILLA)
                ? 9
                : 8;
        if (mOptions[RSK_KEYRINGS_RANDOM_COUNT].GetOptionCount() != maxKeyringCount + 1) {
            mOptions[RSK_KEYRINGS_RANDOM_COUNT].ChangeOptions(NumOpts(0, maxKeyringCount));
        }
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("FortressCarpenters"), RO_GF_CARPENTERS_NORMAL) !=
                RO_GF_CARPENTERS_NORMAL ||
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("GerudoKeys"), RO_GERUDO_KEYS_VANILLA) == RO_GERUDO_KEYS_VANILLA) {
            mOptions[RSK_KEYRINGS_GERUDO_FORTRESS].Disable(
                "Disabled because the currently selected Gerudo Fortress Carpenters\n"
                "setting and/or Gerudo Fortress Keys setting is incompatible with\n"
                "having a Gerudo Fortress Keyring.");
        } else {
            mOptions[RSK_KEYRINGS_GERUDO_FORTRESS].Enable();
        }
    }
    void Callback_RSK_GANONS_BOSS_KEY() {
        mOptions[RSK_GBK_OPTIONS].Hide();
        mOptions[RSK_GBK_STONE_COUNT].Hide();
        mOptions[RSK_GBK_MEDALLION_COUNT].Hide();
        mOptions[RSK_GBK_REWARD_COUNT].Hide();
        mOptions[RSK_GBK_DUNGEON_COUNT].Hide();
        mOptions[RSK_GBK_TOKEN_COUNT].Hide();
        mOptions[RSK_GBK_TRIFORCE_COUNT].Hide();
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleGanonBossKey"), RO_GANON_BOSS_KEY_VANILLA)) {
            case RO_GANON_BOSS_KEY_STONES:
                mOptions[RSK_GBK_OPTIONS].Unhide();
                mOptions[RSK_GBK_STONE_COUNT].Unhide();
                break;
            case RO_GANON_BOSS_KEY_MEDALLIONS:
                mOptions[RSK_GBK_OPTIONS].Unhide();
                mOptions[RSK_GBK_MEDALLION_COUNT].Unhide();
                break;
            case RO_GANON_BOSS_KEY_REWARDS:
                mOptions[RSK_GBK_OPTIONS].Unhide();
                mOptions[RSK_GBK_REWARD_COUNT].Unhide();
                break;
            case RO_GANON_BOSS_KEY_DUNGEONS:
                mOptions[RSK_GBK_OPTIONS].Unhide();
                mOptions[RSK_GBK_DUNGEON_COUNT].Unhide();
                break;
            case RO_GANON_BOSS_KEY_TOKENS:
                mOptions[RSK_GBK_TOKEN_COUNT].Unhide();
                break;
            case RO_GANON_BOSS_KEY_TRIFORCE_PIECES:
                mOptions[RSK_GBK_TRIFORCE_COUNT].Unhide();
                break;
        }
    }
    void Callback_RSK_GBK_OPTIONS() {
        const uint8_t gbkOpts =
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("GbkRewardOptions"), RO_CHECK_TRIGGER_STANDARD_REWARD);
        if (gbkOpts == RO_CHECK_TRIGGER_GREG_REWARD) {
            mOptions[RSK_GBK_STONE_COUNT].ChangeOptions(NumOpts(0, 4));
            mOptions[RSK_GBK_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 7));
            mOptions[RSK_GBK_REWARD_COUNT].ChangeOptions(NumOpts(0, 10));
            mOptions[RSK_GBK_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 9));
        } else {
            mOptions[RSK_GBK_STONE_COUNT].ChangeOptions(NumOpts(0, 3));
            mOptions[RSK_GBK_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 6));
            mOptions[RSK_GBK_REWARD_COUNT].ChangeOptions(NumOpts(0, 9));
            mOptions[RSK_GBK_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 8));
        }
    }
    void Callback_RSK_GANONS_SOUL() {
        mOptions[RSK_GANONS_SOUL_OPTIONS].Hide();
        mOptions[RSK_GANONS_SOUL_STONE_COUNT].Hide();
        mOptions[RSK_GANONS_SOUL_MEDALLION_COUNT].Hide();
        mOptions[RSK_GANONS_SOUL_REWARD_COUNT].Hide();
        mOptions[RSK_GANONS_SOUL_DUNGEON_COUNT].Hide();
        mOptions[RSK_GANONS_SOUL_TOKEN_COUNT].Hide();
        mOptions[RSK_GANONS_SOUL_TRIFORCE_COUNT].Hide();
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleGanonsSoul"), RO_GANONS_SOUL_STARTWITH)) {
            case RO_GANONS_SOUL_STONES:
                mOptions[RSK_GANONS_SOUL_OPTIONS].Unhide();
                mOptions[RSK_GANONS_SOUL_STONE_COUNT].Unhide();
                break;
            case RO_GANONS_SOUL_MEDALLIONS:
                mOptions[RSK_GANONS_SOUL_OPTIONS].Unhide();
                mOptions[RSK_GANONS_SOUL_MEDALLION_COUNT].Unhide();
                break;
            case RO_GANONS_SOUL_REWARDS:
                mOptions[RSK_GANONS_SOUL_OPTIONS].Unhide();
                mOptions[RSK_GANONS_SOUL_REWARD_COUNT].Unhide();
                break;
            case RO_GANONS_SOUL_DUNGEONS:
                mOptions[RSK_GANONS_SOUL_OPTIONS].Unhide();
                mOptions[RSK_GANONS_SOUL_DUNGEON_COUNT].Unhide();
                break;
            case RO_GANONS_SOUL_TOKENS:
                mOptions[RSK_GANONS_SOUL_TOKEN_COUNT].Unhide();
                break;
            case RO_GANONS_SOUL_TRIFORCE_PIECES:
                mOptions[RSK_GANONS_SOUL_TRIFORCE_COUNT].Unhide();
                break;
        }
    }
    void Callback_RSK_GANONS_SOUL_OPTIONS() {
        const uint8_t soulOpts =
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("GanonsSoulRewardOptions"), RO_CHECK_TRIGGER_STANDARD_REWARD);
        if (soulOpts == RO_CHECK_TRIGGER_GREG_REWARD) {
            mOptions[RSK_GANONS_SOUL_STONE_COUNT].ChangeOptions(NumOpts(0, 4));
            mOptions[RSK_GANONS_SOUL_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 7));
            mOptions[RSK_GANONS_SOUL_REWARD_COUNT].ChangeOptions(NumOpts(0, 10));
            mOptions[RSK_GANONS_SOUL_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 9));
        } else {
            mOptions[RSK_GANONS_SOUL_STONE_COUNT].ChangeOptions(NumOpts(0, 3));
            mOptions[RSK_GANONS_SOUL_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 6));
            mOptions[RSK_GANONS_SOUL_REWARD_COUNT].ChangeOptions(NumOpts(0, 9));
            mOptions[RSK_GANONS_SOUL_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 8));
        }
    }
    void Callback_RSK_WINCON() {
        mOptions[RSK_WINCON_OPTIONS].Hide();
        mOptions[RSK_WINCON_STONE_COUNT].Hide();
        mOptions[RSK_WINCON_MEDALLION_COUNT].Hide();
        mOptions[RSK_WINCON_REWARD_COUNT].Hide();
        mOptions[RSK_WINCON_DUNGEON_COUNT].Hide();
        mOptions[RSK_WINCON_TOKEN_COUNT].Hide();
        mOptions[RSK_WINCON_TRIFORCE_COUNT].Hide();
        // ComboShip: combo forces the wincon, so no sub-option is ever offered.
        if (kComboOwnsWincon) {
            mOptions[RSK_WINCON].Hide();
            return;
        }
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleWincon"), RO_WINCON_DEFEAT_GANON)) {
            case RO_WINCON_STONES:
                mOptions[RSK_WINCON_OPTIONS].Unhide();
                mOptions[RSK_WINCON_STONE_COUNT].Unhide();
                break;
            case RO_WINCON_MEDALLIONS:
                mOptions[RSK_WINCON_OPTIONS].Unhide();
                mOptions[RSK_WINCON_MEDALLION_COUNT].Unhide();
                break;
            case RO_WINCON_REWARDS:
                mOptions[RSK_WINCON_OPTIONS].Unhide();
                mOptions[RSK_WINCON_REWARD_COUNT].Unhide();
                break;
            case RO_WINCON_DUNGEONS:
                mOptions[RSK_WINCON_OPTIONS].Unhide();
                mOptions[RSK_WINCON_DUNGEON_COUNT].Unhide();
                break;
            case RO_WINCON_TOKENS:
                mOptions[RSK_WINCON_TOKEN_COUNT].Unhide();
                break;
            case RO_WINCON_TRIFORCE_PIECES:
                mOptions[RSK_WINCON_TRIFORCE_COUNT].Unhide();
                break;
        }
    }
    void Callback_RSK_WINCON_OPTIONS() {
        const uint8_t winconOpts =
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("WinconRewardOptions"), RO_CHECK_TRIGGER_STANDARD_REWARD);
        if (winconOpts == RO_CHECK_TRIGGER_GREG_REWARD) {
            mOptions[RSK_WINCON_STONE_COUNT].ChangeOptions(NumOpts(0, 4));
            mOptions[RSK_WINCON_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 7));
            mOptions[RSK_WINCON_REWARD_COUNT].ChangeOptions(NumOpts(0, 10));
            mOptions[RSK_WINCON_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 9));
        } else {
            mOptions[RSK_WINCON_STONE_COUNT].ChangeOptions(NumOpts(0, 3));
            mOptions[RSK_WINCON_MEDALLION_COUNT].ChangeOptions(NumOpts(0, 6));
            mOptions[RSK_WINCON_REWARD_COUNT].ChangeOptions(NumOpts(0, 9));
            mOptions[RSK_WINCON_DUNGEON_COUNT].ChangeOptions(NumOpts(0, 8));
        }
    }
    void Callback_RSK_KEYRINGS() {
        switch (CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShuffleKeyRings"), RO_KEYRINGS_OFF)) {
            case RO_KEYRINGS_COUNT:
                // Show count slider.
                mOptions[RSK_KEYRINGS_RANDOM_COUNT].Unhide();
                mOptions[RSK_KEYRINGS_GERUDO_FORTRESS].Hide();
                mOptions[RSK_KEYRINGS_FOREST_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_FIRE_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_WATER_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_SPIRIT_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_SHADOW_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_BOTTOM_OF_THE_WELL].Hide();
                mOptions[RSK_KEYRINGS_GTG].Hide();
                mOptions[RSK_KEYRINGS_GANONS_CASTLE].Hide();
                break;
            case RO_KEYRINGS_SELECTION:
                // Show checkboxes for each dungeon with keys.
                mOptions[RSK_KEYRINGS_RANDOM_COUNT].Hide();
                mOptions[RSK_KEYRINGS_GERUDO_FORTRESS].Unhide();
                mOptions[RSK_KEYRINGS_FOREST_TEMPLE].Unhide();
                mOptions[RSK_KEYRINGS_FIRE_TEMPLE].Unhide();
                mOptions[RSK_KEYRINGS_WATER_TEMPLE].Unhide();
                mOptions[RSK_KEYRINGS_SPIRIT_TEMPLE].Unhide();
                mOptions[RSK_KEYRINGS_SHADOW_TEMPLE].Unhide();
                mOptions[RSK_KEYRINGS_BOTTOM_OF_THE_WELL].Unhide();
                mOptions[RSK_KEYRINGS_GTG].Unhide();
                mOptions[RSK_KEYRINGS_GANONS_CASTLE].Unhide();
                break;
            default:
                mOptions[RSK_KEYRINGS_RANDOM_COUNT].Hide();
                mOptions[RSK_KEYRINGS_GERUDO_FORTRESS].Hide();
                mOptions[RSK_KEYRINGS_FOREST_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_FIRE_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_WATER_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_SPIRIT_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_SHADOW_TEMPLE].Hide();
                mOptions[RSK_KEYRINGS_BOTTOM_OF_THE_WELL].Hide();
                mOptions[RSK_KEYRINGS_GTG].Hide();
                mOptions[RSK_KEYRINGS_GANONS_CASTLE].Hide();
                break;
        }
    }
    void Callback_RSK_BIG_POE_COUNT() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("BigPoeTargetCount"), 10) == 0) {
            mOptions[RSK_BIG_POES_HINT].Disable("Poe Collector will just give you the item instead with 0 big poes.");
        } else {
            mOptions[RSK_BIG_POES_HINT].Enable();
        }
    }
    void Callback_RSK_GOSSIP_STONE_HINTS() {
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("GossipStoneHints"), RO_GOSSIP_STONES_NEED_NOTHING) ==
            RO_GOSSIP_STONES_NONE) {
            mOptions[RSK_HINT_CLARITY].Hide();
            mOptions[RSK_HINT_DISTRIBUTION].Hide();
        } else {
            mOptions[RSK_HINT_CLARITY].Unhide();
            mOptions[RSK_HINT_DISTRIBUTION].Unhide();
        }
    }
    void Callback_RSK_LOGIC_RULES() {
        HandleStartingAgeUI();
        if (CVarGetInteger(CVAR_RANDOMIZER_SETTING("LogicRules"), RO_LOGIC_GLITCHLESS) != RO_LOGIC_NO_LOGIC &&
            CVarGetInteger(CVAR_RANDOMIZER_SETTING("ShopsanityCount"), 0) > 7) {
            CVarSetInteger(CVAR_RANDOMIZER_SETTING("ShopsanityCount"), 7);
        }
    }
#undef CVAR_RANDOMIZER_SETTING
  public:
    std::map<int, Option> mOptions;
    View(const nlohmann::json& catalog, std::function<int(const char*, int)> getter) : read(std::move(getter)) {
        for (const auto& field : catalog.at("options").items()) {
            Option option;
            option.key = field.key();
            const std::string key = option.key;
            option.current = [this, key] { return CVarGetInteger(key.c_str(), 0); };
            option.update = [this, key](int value) { CVarSetInteger(key.c_str(), value); };
            option.hidden = field.value().value("defaultHidden", false);
            for (const auto& choice : field.value().at("choices")) {
                option.choiceValues.push_back(choice.value("value", int(option.choices.size())));
                option.choices.push_back(choice.at("label"));
            }
            mOptions.emplace(field.value().at("id").get<int>(), std::move(option));
        }
        std::map<int, std::function<void()>> callbacks;
        callbacks[RSK_FOREST] = [this] { Callback_RSK_FOREST(); };
        callbacks[RSK_DOOR_OF_TIME] = [this] { Callback_RSK_DOOR_OF_TIME(); };
        callbacks[RSK_LOCK_OVERWORLD_DOORS] = [this] { Callback_RSK_LOCK_OVERWORLD_DOORS(); };
        callbacks[RSK_GERUDO_FORTRESS] = [this] { Callback_RSK_GERUDO_FORTRESS(); };
        callbacks[RSK_RAINBOW_BRIDGE] = [this] { Callback_RSK_RAINBOW_BRIDGE(); };
        callbacks[RSK_BRIDGE_OPTIONS] = [this] { Callback_RSK_BRIDGE_OPTIONS(); };
        callbacks[RSK_GANONS_TRIALS] = [this] { Callback_RSK_GANONS_TRIALS(); };
        callbacks[RSK_SHUFFLE_DUNGEON_ENTRANCES] = [this] { Callback_RSK_SHUFFLE_DUNGEON_ENTRANCES(); };
        callbacks[RSK_SHUFFLE_BOSS_ENTRANCES] = [this] { Callback_RSK_SHUFFLE_BOSS_ENTRANCES(); };
        callbacks[RSK_SHUFFLE_OVERWORLD_ENTRANCES] = [this] { Callback_RSK_SHUFFLE_OVERWORLD_ENTRANCES(); };
        callbacks[RSK_SHUFFLE_INTERIOR_ENTRANCES] = [this] { Callback_RSK_SHUFFLE_INTERIOR_ENTRANCES(); };
        callbacks[RSK_SHUFFLE_THIEVES_HIDEOUT_ENTRANCES] = [this] { Callback_RSK_SHUFFLE_THIEVES_HIDEOUT_ENTRANCES(); };
        callbacks[RSK_SHUFFLE_GROTTO_ENTRANCES] = [this] { Callback_RSK_SHUFFLE_GROTTO_ENTRANCES(); };
        callbacks[RSK_SHUFFLE_WARP_SONGS] = [this] { Callback_RSK_SHUFFLE_WARP_SONGS(); };
        callbacks[RSK_SHUFFLE_OVERWORLD_SPAWNS] = [this] { Callback_RSK_SHUFFLE_OVERWORLD_SPAWNS(); };
        callbacks[RSK_MIXED_ENTRANCE_POOLS] = [this] { Callback_RSK_MIXED_ENTRANCE_POOLS(); };
        callbacks[RSK_DECOUPLED_ENTRANCES] = [this] { Callback_RSK_DECOUPLED_ENTRANCES(); };
        callbacks[RSK_TRIFORCE_HUNT_PIECES_TOTAL] = [this] { Callback_RSK_TRIFORCE_HUNT_PIECES_TOTAL(); };
        callbacks[RSK_MQ_DUNGEON_RANDOM] = [this] { Callback_RSK_MQ_DUNGEON_RANDOM(); };
        callbacks[RSK_MQ_DUNGEON_SET] = [this] { Callback_RSK_MQ_DUNGEON_SET(); };
        callbacks[RSK_SHUFFLE_DUNGEON_REWARDS] = [this] { Callback_RSK_SHUFFLE_DUNGEON_REWARDS(); };
        callbacks[RSK_LINKS_POCKET] = [this] { Callback_RSK_LINKS_POCKET(); };
        callbacks[RSK_SHOPSANITY] = [this] { Callback_RSK_SHOPSANITY(); };
        callbacks[RSK_SHOPSANITY_PRICES] = [this] { Callback_RSK_SHOPSANITY_PRICES(); };
        callbacks[RSK_SHUFFLE_SCRUBS] = [this] { Callback_RSK_SHUFFLE_SCRUBS(); };
        callbacks[RSK_SCRUBS_PRICES] = [this] { Callback_RSK_SCRUBS_PRICES(); };
        callbacks[RSK_SHUFFLE_BEEHIVES] = [this] { Callback_RSK_SHUFFLE_BEEHIVES(); };
        callbacks[RSK_SHUFFLE_COWS] = [this] { Callback_RSK_SHUFFLE_COWS(); };
        callbacks[RSK_SHUFFLE_CHILD_WALLET] = [this] { Callback_RSK_SHUFFLE_CHILD_WALLET(); };
        callbacks[RSK_SHUFFLE_OCARINA] = [this] { Callback_RSK_SHUFFLE_OCARINA(); };
        callbacks[RSK_SHUFFLE_SWIM] = [this] { Callback_RSK_SHUFFLE_SWIM(); };
        callbacks[RSK_SHUFFLE_GRAB] = [this] { Callback_RSK_SHUFFLE_GRAB(); };
        callbacks[RSK_SHUFFLE_FISHING_POLE] = [this] { Callback_RSK_SHUFFLE_FISHING_POLE(); };
        callbacks[RSK_SHUFFLE_MERCHANTS] = [this] { Callback_RSK_SHUFFLE_MERCHANTS(); };
        callbacks[RSK_MERCHANT_PRICES] = [this] { Callback_RSK_MERCHANT_PRICES(); };
        callbacks[RSK_SHUFFLE_ADULT_TRADE] = [this] { Callback_RSK_SHUFFLE_ADULT_TRADE(); };
        callbacks[RSK_SHUFFLE_100_GS_REWARD] = [this] { Callback_RSK_SHUFFLE_100_GS_REWARD(); };
        callbacks[RSK_SHUFFLE_DEKU_STICK_BAG] = [this] { Callback_RSK_SHUFFLE_DEKU_STICK_BAG(); };
        callbacks[RSK_SHUFFLE_DEKU_NUT_BAG] = [this] { Callback_RSK_SHUFFLE_DEKU_NUT_BAG(); };
        callbacks[RSK_FISHSANITY] = [this] { Callback_RSK_FISHSANITY(); };
        callbacks[RSK_GERUDO_KEYS] = [this] { Callback_RSK_GERUDO_KEYS(); };
        callbacks[RSK_GANONS_BOSS_KEY] = [this] { Callback_RSK_GANONS_BOSS_KEY(); };
        callbacks[RSK_GBK_OPTIONS] = [this] { Callback_RSK_GBK_OPTIONS(); };
        callbacks[RSK_GANONS_SOUL] = [this] { Callback_RSK_GANONS_SOUL(); };
        callbacks[RSK_GANONS_SOUL_OPTIONS] = [this] { Callback_RSK_GANONS_SOUL_OPTIONS(); };
        callbacks[RSK_WINCON] = [this] { Callback_RSK_WINCON(); };
        callbacks[RSK_WINCON_OPTIONS] = [this] { Callback_RSK_WINCON_OPTIONS(); };
        callbacks[RSK_KEYRINGS] = [this] { Callback_RSK_KEYRINGS(); };
        callbacks[RSK_BIG_POE_COUNT] = [this] { Callback_RSK_BIG_POE_COUNT(); };
        callbacks[RSK_GOSSIP_STONE_HINTS] = [this] { Callback_RSK_GOSSIP_STONE_HINTS(); };
        callbacks[RSK_LOGIC_RULES] = [this] { Callback_RSK_LOGIC_RULES(); };
        for (const auto& row : callbacks)
            row.second();
    }
    const Option* Find(const char* key) const {
        for (const auto& row : mOptions)
            if (row.second.key == key)
                return &row.second;
        return nullptr;
    }
    const std::map<std::string, int>& Adjustments() const {
        return effective;
    }
};
} // namespace ComboRando::ProfileOotPolicy
