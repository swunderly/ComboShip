#include "rando/GameplayRuleSnapshots.h"
#include "rando/GenerationRules.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string>

// Compile the port's actual option ordering rather than assuming the No Logic number.
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name,
#define RANDO_ENUM_END(name) };
#include "../soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerOptions.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
static_assert(ComboRando::kNativeNoLogicOption == RO_LOGIC_NO_LOGIC);

using nlohmann::json;
using namespace ComboRando;

struct RunningGame {
    std::map<std::string, int> cvars;
    std::string ootTricks;
    int read(const char* key, int fallback = 0) const {
        const auto value = cvars.find(key);
        return value == cvars.end() ? fallback : value->second;
    }
    void load(const json& seed, const json& savedRun = json()) {
        const auto rules = GameplayRulesForRun(seed, savedRun);
        ApplyGameplayRules(rules, [&](const char* key, int value) { cvars[key] = value; });
        ootTricks = rules["ootTricks"].get<std::string>();
    }
    json snapshot() const {
        auto result = CaptureGameplayRules([&](const char* key, int fallback) { return read(key, fallback); });
        result["ootTricks"] = ootTricks;
        return result;
    }
};

static int Runtime(const RunningGame& game, const char* key) { return game.read(key); }
static int Logical(const RunningGame& game, const char* key) { return game.read(LogicRuleKey(key).c_str()); }

int main() {
    // A busy modern slot must not supply any state when an older independent slot is loaded.
    RunningGame game;
    for (const auto& rule : kGameplayRules) {
        game.cvars[rule.name] = rule.maximum;
        game.cvars[LogicRuleKey(rule.name)] = rule.maximum;
    }
    for (const auto& trick : kMmTricks) game.cvars[trick.key] = 1;
    game.ootTricks = "slot_a_technique";
    const json slotA = {{"gameplayRules", game.snapshot()}};
    const json slotB = {
        {"oot", {{"settings", {{"gRandoSettings.BlueFireArrows", 1}, {"gRandoSettings.SunlightArrows", 0}}},
                 {"enabledTricks", json::array({"slot_b_technique", 7, "slot_b_second"})}}},
        {"songTravel", SongTravelMap::generate(12, false, true).json()}
    };
    const auto originalA = slotA.dump(), originalB = slotB.dump();
    game.load(slotA);
    assert(Runtime(game, "gEnhancements.TimeTravel") == 4);
    assert(Logical(game, "gEnhancements.Masks.FierceDeitysAnywhere") == 1);
    game.load(slotB);
    assert(Runtime(game, "gEnhancements.TimeTravel") == 0);
    assert(Logical(game, "gEnhancements.TimeTravel") == 0);
    assert(Runtime(game, "gEnhancements.Masks.FierceDeitysAnywhere") == 0);
    assert(Runtime(game, "gEnhancements.Masks.BlastMaskKeg") == 0);
    assert(Runtime(game, "gEnhancements.DifficultyOptions.FrogChoirCount") == 5);
    assert(Logical(game, "gEnhancements.DifficultyOptions.FrogChoirCount") == 0);
    assert(Runtime(game, "gEnhancements.BlueFireArrows") == 1);
    assert(Logical(game, "gEnhancements.BlueFireArrows") == 1);
    assert(Runtime(game, "gEnhancements.SunlightArrows") == 0);
    assert(Runtime(game, "gCombo.Enhancements.ForeignSongs") == 1);
    assert(Logical(game, "gCombo.Enhancements.ForeignSongs") == 1);
    assert(Runtime(game, "gCombo.Enhancements.SongTravel") == 0);
    for (const auto& trick : kMmTricks) assert(game.read(trick.key) == 0);
    assert(game.ootTricks == "slot_b_technique,slot_b_second");
    assert(slotA.dump() == originalA && slotB.dump() == originalB);

    // Old run snapshots predate several rules. Present run overrides are authoritative; only
    // missing entries migrate from their original seed (including song maps and native aliases).
    const json legacyRun = {{"version", 1}, {"values", {
        {"gEnhancements.TimeTravel", 2}, {"gEnhancements.Masks.FierceDeitysAnywhere", 1},
        {"gEnhancements.Masks.BremenOneChick", 0}, {"gCombo.Enhancements.ForeignSongs", 0}}}};
    const auto originalRun = legacyRun.dump();
    game.load(slotA);
    game.load(slotB, legacyRun);
    assert(Runtime(game, "gEnhancements.TimeTravel") == 2 && Logical(game, "gEnhancements.TimeTravel") == 2);
    assert(Runtime(game, "gCombo.Enhancements.ForeignSongs") == 0 && Logical(game, "gCombo.Enhancements.ForeignSongs") == 0);
    assert(Runtime(game, "gEnhancements.BlueFireArrows") == 1 && Logical(game, "gEnhancements.BlueFireArrows") == 1);
    assert(game.ootTricks == "slot_b_technique,slot_b_second");
    assert(legacyRun.dump() == originalRun && slotB.dump() == originalB);

    // Version2 enhancement-only behavior, a deliberately empty technique list and a run's
    // edited techniques must survive seed load/reload without rewriting original generation.
    RunningGame source;
    source.cvars["gEnhancements.TimeTravel"] = 4;
    source.cvars["gCombo.Enhancements.ForeignSongs"] = 1;
    source.cvars[kMmTricks[0].key] = 1;
    source.ootTricks = "original_seed_technique";
    const json modernSeed = {{"gameplayRules", source.snapshot()}};
    auto run = source.snapshot();
    run["values"]["gEnhancements.TimeTravel"] = 3;
    run["logicValues"]["gEnhancements.TimeTravel"] = 3;
    run["ootTricks"] = "";
    run["mmTricks"][kMmTricks[0].key] = 0;
    game.load(modernSeed, run);
    assert(Runtime(game, "gEnhancements.TimeTravel") == 3 && Logical(game, "gEnhancements.TimeTravel") == 3);
    assert(game.ootTricks.empty() && game.read(kMmTricks[0].key) == 0);
    assert(modernSeed["gameplayRules"]["ootTricks"] == "original_seed_technique");
    game.load(modernSeed);
    assert(Runtime(game, "gEnhancements.TimeTravel") == 4 && Logical(game, "gEnhancements.TimeTravel") == 0);
    assert(Runtime(game, "gCombo.Enhancements.ForeignSongs") == 1 && Logical(game, "gCombo.Enhancements.ForeignSongs") == 0);
    assert(game.ootTricks == "original_seed_technique" && game.read(kMmTricks[0].key) == 1);
    run.erase("ootTricks");
    run.erase("mmTricks");
    game.load(modernSeed, run);
    assert(game.ootTricks == "original_seed_technique" && game.read(kMmTricks[0].key) == 1);

    // Corrupted known values cannot narrow/wrap or leak the preceding slot's state. Native
    // historical aliases and the original CSV provide deterministic migration, not live CVars.
    auto corrupt = GameplayRulesForSeed(json::object());
    corrupt["values"]["gEnhancements.TimeTravel"] = 999999999999ULL;
    corrupt["values"]["gEnhancements.Masks.FierceDeitysAnywhere"] = true;
    corrupt["values"]["gEnhancements.Masks.BremenOneChick"] = "1";
    corrupt["values"]["gEnhancements.BlueFireArrows"] = -1;
    corrupt["values"]["gEnhancements.SunlightArrows"] = "1";
    corrupt["values"]["gEnhancements.DifficultyOptions.FrogChoirCount"] = 0;
    corrupt["values"]["gUnrelated.Cheat"] = 1;
    corrupt["logicValues"]["gEnhancements.Player.InfiniteDekuHopping"] = true;
    corrupt["mmTricks"][kMmTricks[0].key] = "1";
    corrupt["ootTricks"] = 7;
    const json corruptSeed = {{"gameplayRules", corrupt}, {"oot", {{"settings", {
        {"gRandoSettings.BlueFireArrows", true}, {"gRandoSettings.SunlightArrows", "1"},
        {"gRandoSettings.EnabledTricks", "historical_csv"}}}}}};
    game.load(slotA);
    game.cvars["gUnrelated.Cheat"] = 9;
    game.load(corruptSeed);
    assert(Runtime(game, "gEnhancements.TimeTravel") == 0);
    assert(Runtime(game, "gEnhancements.Masks.FierceDeitysAnywhere") == 0);
    assert(Runtime(game, "gEnhancements.Masks.BremenOneChick") == 0);
    assert(Runtime(game, "gEnhancements.DifficultyOptions.FrogChoirCount") == 5);
    assert(Logical(game, "gEnhancements.DifficultyOptions.FrogChoirCount") == 0);
    assert(Runtime(game, "gEnhancements.BlueFireArrows") == 1 && Logical(game, "gEnhancements.BlueFireArrows") == 1);
    assert(Runtime(game, "gEnhancements.SunlightArrows") == 0);
    assert(Logical(game, "gEnhancements.Player.InfiniteDekuHopping") == 0);
    assert(game.ootTricks == "historical_csv" && game.read(kMmTricks[0].key) == 0);
    assert(game.read("gUnrelated.Cheat") == 9);
    // A vanilla file, or a save from before rules were stored, adopts the player's own settings
    // (taken before any slot restored rules) instead of switching enhancements back to defaults.
    // Whatever the seed records still wins, and a seed with stored rules ignores the baseline.
    RunningGame player;
    player.cvars["gEnhancements.TimeTravel"] = 3;
    player.cvars["gCheats.SuperTunic"] = 1;
    player.ootTricks = "player_technique";
    const auto baseline = player.snapshot();
    const auto vanilla = GameplayRulesForRun(json::object(), json(), baseline);
    assert(vanilla["values"]["gEnhancements.TimeTravel"] == 3 && vanilla["values"]["gCheats.SuperTunic"] == 1);
    assert(vanilla["ootTricks"] == "player_technique");
    const auto legacy = GameplayRulesForRun(slotB, json(), baseline);
    assert(legacy["values"]["gEnhancements.TimeTravel"] == 3 && legacy["values"]["gCheats.SuperTunic"] == 1);
    assert(legacy["values"]["gEnhancements.BlueFireArrows"] == 1 && legacy["values"]["gEnhancements.SunlightArrows"] == 0);
    assert(legacy["values"]["gCombo.Enhancements.ForeignSongs"] == 1);
    assert(legacy["ootTricks"] == "slot_b_technique,slot_b_second");
    assert(GameplayRulesForRun(slotA, json(), baseline) == GameplayRulesForRun(slotA, json()));
    assert(GameplayRulesForRun(json::object(), json())["values"]["gEnhancements.TimeTravel"] == 0);
    auto unknownVersion = corruptSeed;
    unknownVersion["gameplayRules"]["version"] = 999;
    game.load(slotA);
    game.load(unknownVersion);
    assert(Runtime(game, "gEnhancements.TimeTravel") == 0 && game.ootTricks == "historical_csv");

    // The frog slider's zero logic sentinel is not a zero-frog runtime requirement.
    const auto& frogs = *FindGameplayRule("gEnhancements.DifficultyOptions.FrogChoirCount");
    const auto write = [&](const char* key, int value) { game.cvars[key] = value; };
    const auto read = [&](const char* key, int fallback) { return game.read(key, fallback); };
    SetLogicRule(frogs, 1, write);
    assert(Runtime(game, frogs.name) == 1 && Logical(game, frogs.name) == 1);
    SetEnhancementRule(frogs, 3, read, write);
    assert(Runtime(game, frogs.name) == 3 && Logical(game, frogs.name) == 0);
    SetLogicRule(frogs, 0, write);
    assert(Runtime(game, frogs.name) == 5 && Logical(game, frogs.name) == 0);
    // Clearing only the logic assumption leaves the player's own enhancement as it was.
    SetLogicRule(frogs, 2, write);
    ClearLogicRule(frogs, write);
    assert(Runtime(game, frogs.name) == 2 && Logical(game, frogs.name) == 0);

    // Extra abilities are monotone; a shuffled destination map is not. A runtime-only choice
    // before generation must not make the engine warp somewhere different from its logic.
    for (uint64_t seed = 0; seed < 100; ++seed) {
        const auto runtimeOnly = SongTravelMap::generateForRules(seed, true, true, false, false);
        assert(runtimeOnly.enabled && runtimeOnly.foreignSongs);
        assert(!runtimeOnly.logicEnabled && !runtimeOnly.logicForeignSongs);
        for (int sourceIndex = 0; sourceIndex < kSongDestinations; ++sourceIndex)
            assert(runtimeOnly.destination(sourceIndex) == sourceIndex && runtimeOnly.logicDestination(sourceIndex) == sourceIndex);
        const auto modeled = SongTravelMap::generateForRules(seed, true, true, true, true);
        assert(modeled.destinations == SongTravelMap::generate(seed, true).destinations);
        for (int sourceIndex = 0; sourceIndex < kSongDestinations; ++sourceIndex)
            assert(modeled.destination(sourceIndex) == modeled.logicDestination(sourceIndex));
        auto existingRun = SongTravelMap::parse(modeled.json());
        existingRun.enabled = false;
        const auto storedDestinations = existingRun.destinations;
        existingRun = SongTravelMap::parse(existingRun.json());
        existingRun.enabled = true;
        assert(existingRun.destinations == storedDestinations);
    }

    // The old native all-location setting must agree with the combined choice before confined
    // fill and entrance validation. No Logic cannot advertise either generation guarantee.
    for (int guarantee = 0; guarantee < 2; ++guarantee) {
        for (int nativeLogic : {int(RO_LOGIC_GLITCHLESS), int(RO_LOGIC_NO_LOGIC)}) {
            game.cvars[kCombinedGuarantee] = guarantee;
            game.cvars[kNativeLogicRule] = nativeLogic;
            game.cvars[kNativeAllLocations] = guarantee; // deliberately disagree before synchronization
            SynchronizeGenerationGuarantee(read, write);
            assert(game.read(kNativeAllLocations) == (nativeLogic == RO_LOGIC_GLITCHLESS && guarantee == 0));
            assert(IsNativeNoLogic(nativeLogic) == (nativeLogic == RO_LOGIC_NO_LOGIC));
        }
    }
    for (const auto& rule : kGameplayRules) {
        assert(IsRetiredNativeRuleControl(rule.name) == !rule.trickGoverned);
        assert(IsRetiredNativeRuleControl(LogicRuleKey(rule.name).c_str()));
    }
    assert(IsRetiredNativeRuleControl(kNativeAllLocations));
    assert(!IsRetiredNativeRuleControl(kNativeLogicRule));
    assert(!IsRetiredNativeRuleControl(nullptr));
    assert(!IsRetiredNativeRuleControl("gUnrelated.Cheat"));

    // A native confined-fill failure must be retried before the incomplete pool can be padded
    // or misdiagnosed as missing goal pieces. Mixed failures preserve the fatal engine's error.
    const std::string pool = R"({"checks":[],"pool":[]})";
    const std::string retry = R"({"error":"Native confined fill failed: exhausted placements","retryable":true})";
    assert(!ParseNativePoolFailure(pool, pool));
    const auto retryOot = ParseNativePoolFailure(retry, pool);
    assert(retryOot && retryOot->retryable && retryOot->error.find("exhausted placements") != std::string::npos);
    const auto retryMm = ParseNativePoolFailure(pool, retry);
    assert(retryMm && retryMm->retryable && retryMm->error.find("MM:") == 0);
    const auto mixedFailure = ParseNativePoolFailure(retry, R"({"error":"missing static data","retryable":false})");
    assert(mixedFailure && !mixedFailure->retryable && mixedFailure->error.find("missing static data") != std::string::npos);
    for (const std::string bad : {"", "{broken", "[]", "{}", R"({"error":false,"retryable":true})",
             R"({"error":"","retryable":true})", R"({"error":"failure","retryable":1})",
             R"({"retryable":true})", R"({"checks":null})", R"({"pool":{}})"}) {
        const auto malformed = ParseNativePoolFailure(retry, bad);
        assert(malformed && !malformed->retryable);
    }
    const auto unclassified = ParseNativePoolFailure(R"({"error":"unknown native exception"})", pool);
    assert(unclassified && !unclassified->retryable);
    std::cout << "Gameplay rule snapshots: slots, immutable seeds, run overrides, tricks, frog minimum, modeled maps, native guarantee/control policy and native pool failures passed\n";
}
