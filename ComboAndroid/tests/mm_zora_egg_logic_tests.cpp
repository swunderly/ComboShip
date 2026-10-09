#include "rando/MmEggLogic.h"
#include "rando/GameplayRuleSnapshots.h"
#include "rando/ForeignSongs.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string>

int main() {
    using namespace ComboRando;
    unsigned cases = 0;
    for (int count = -2; count <= 10; ++count) {
        for (int eggs = 0; eggs <= 7; ++eggs) {
            for (int flags = 0; flags < 8; ++flags) {
                const bool zora = (flags & 1) != 0;
                const bool instrument = (flags & 2) != 0;
                const bool bottle = (flags & 4) != 0;
                const int expectedCount = count >= 1 && count <= 7 ? count : 7;
                assert(RequiredZoraEggs(count) == expectedCount);
                assert(CanLearnNewWaveBossaNova(count, eggs, zora, instrument, bottle) ==
                       (zora && instrument && bottle && eggs >= expectedCount));
                ++cases;
            }
        }
    }
    // Four fortress eggs may complete the reduced quest without requiring the
    // three magic-gated Pinnacle eggs; three Pinnacle eggs work without the
    // fortress Hookshot route. Region access and egg-specific gates remain.
    assert(CanLearnNewWaveBossaNova(4, 4, true, true, true));
    assert(!CanLearnNewWaveBossaNova(0, 4, true, true, true));
    assert(CanLearnNewWaveBossaNova(3, 3, true, true, true));
    assert(!CanLearnNewWaveBossaNova(4, 3, true, true, true));

    std::map<std::string, int> values;
    const auto read = [&](const char* key, int fallback) {
        const auto found = values.find(key);
        return found == values.end() ? fallback : found->second;
    };
    const auto write = [&](const char* key, int value) { values[key] = value; };
    const auto& eggs = *FindGameplayRule("gEnhancements.Songs.ZoraEggCount");
    const auto& ageless = *FindGameplayRule("gCombo.Enhancements.AgelessSoaring");
    assert(eggs.fallback == 7 && eggs.minimum == 1 && eggs.maximum == 7 && !eggs.trickGoverned);
    assert(ageless.fallback == 0 && ageless.minimum == 0 && ageless.maximum == 1);
    assert(LogicRuleKey(eggs.name) == "gCombo.Logic.gEnhancements.Songs.ZoraEggCount");
    assert(LogicRuleKey(ageless.name) == "gCombo.Logic.gCombo.Enhancements.AgelessSoaring");
    const auto defaults = CaptureGameplayRules(read);
    assert(defaults["values"][eggs.name] == 7 && defaults["logicValues"][eggs.name] == 0);
    assert(defaults["values"][ageless.name] == 0 && defaults["logicValues"][ageless.name] == 0);

    // Both new rules use the existing separate ability/assumption policy.
    SetEnhancementRule(eggs, 1, read, write);
    SetEnhancementRule(ageless, 1, read, write);
    assert(read(eggs.name, 7) == 1 && read(LogicRuleKey(eggs.name).c_str(), 0) == 0);
    assert(read(ageless.name, 0) == 1 && read(LogicRuleKey(ageless.name).c_str(), 0) == 0);
    assert(!CanLearnNewWaveBossaNova(read(LogicRuleKey(eggs.name).c_str(), 0), 1, true, true, true));
    assert(ForeignSoaringSources(true, 1u << 10, true, 26u, false,
                                 read(LogicRuleKey(ageless.name).c_str(), 0) != 0));
    SetLogicRule(eggs, 3, write);
    SetLogicRule(ageless, 1, write);
    const nlohmann::json seed = {{"gameplayRules", CaptureGameplayRules(read)}};
    const auto immutable = seed.dump();
    assert(read(eggs.name, 7) == 3 && read(LogicRuleKey(eggs.name).c_str(), 0) == 3);
    auto run = seed["gameplayRules"];
    run["values"][eggs.name] = 1;
    run["logicValues"][eggs.name] = 0;
    run["logicValues"][ageless.name] = 0;
    values.clear();
    ApplyGameplayRules(GameplayRulesForRun(seed, run), write);
    assert(read(eggs.name, 7) == 1 && read(LogicRuleKey(eggs.name).c_str(), 9) == 0);
    assert(read(ageless.name, 0) == 1 && read(LogicRuleKey(ageless.name).c_str(), 9) == 0);
    assert(seed.dump() == immutable);
    ApplyGameplayRules(GameplayRulesForSeed(seed), write);
    assert(read(eggs.name, 7) == 3 && read(LogicRuleKey(eggs.name).c_str(), 0) == 3);
    assert(read(ageless.name, 0) == 1 && read(LogicRuleKey(ageless.name).c_str(), 0) == 1);

    // Older slots cannot inherit the new capabilities from the prior slot.
    auto old = seed["gameplayRules"];
    old["values"].erase(eggs.name); old["logicValues"].erase(eggs.name);
    old["values"].erase(ageless.name); old["logicValues"].erase(ageless.name);
    for (int version : {1, 2}) {
        old["version"] = version;
        ApplyGameplayRules(GameplayRulesForSeed({{"gameplayRules", old}}), write);
        assert(read(eggs.name, 9) == 7 && read(LogicRuleKey(eggs.name).c_str(), 9) == 0);
        assert(read(ageless.name, 9) == 0 && read(LogicRuleKey(ageless.name).c_str(), 9) == 0);
    }
    auto missingRun = run;
    missingRun["values"].erase(eggs.name); missingRun["logicValues"].erase(eggs.name);
    missingRun["values"].erase(ageless.name); missingRun["logicValues"].erase(ageless.name);
    ApplyGameplayRules(GameplayRulesForRun(seed, missingRun), write);
    assert(read(eggs.name, 7) == 3 && read(LogicRuleKey(eggs.name).c_str(), 0) == 3);
    assert(read(ageless.name, 0) == 1 && read(LogicRuleKey(ageless.name).c_str(), 0) == 1);
    for (const auto& invalid : {nlohmann::json(0), nlohmann::json(8), nlohmann::json(-1),
                                nlohmann::json(999999999999ULL), nlohmann::json("1"), nlohmann::json(true)}) {
        auto corrupt = defaults;
        corrupt["values"][eggs.name] = invalid;
        corrupt["logicValues"][eggs.name] = invalid;
        ApplyGameplayRules(GameplayRulesForSeed({{"gameplayRules", corrupt}}), write);
        assert(read(eggs.name, 9) == 7 && read(LogicRuleKey(eggs.name).c_str(), 9) == 0);
    }
    // Zero turns off the logic assumption; it is not a zero-egg runtime quest.
    SetLogicRule(eggs, 0, write);
    assert(read(eggs.name, 9) == 7 && read(LogicRuleKey(eggs.name).c_str(), 9) == 0);
    std::cout << "PASS: " << cases << " egg requirement cases; local prerequisites, runtime-only rules, "
                 "seed/run snapshots, missing legacy fields and Ageless Soaring defaults\n";
}
