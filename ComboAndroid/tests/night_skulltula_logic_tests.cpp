#include "rando/OotDaytimeSkulltulas.h"
#include "rando/GameplayRuleSnapshots.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string>

int main() {
    using namespace ComboRando;
    for (int flags = 0; flags < 64; ++flags) {
        const bool day = (flags & 1) != 0;
        const bool night = (flags & 2) != 0;
        const bool song = (flags & 4) != 0;
        const bool expectSong = (flags & 8) != 0;
        const bool rule = (flags & 16) != 0;
        const bool nativeActor = (flags & 32) != 0;
        const bool ordinary = night && (song || !expectSong);
        const bool daytime = rule && nativeActor && (day || night);
        assert(CanCollectNightSkulltula(day, night, song, expectSong, rule, nativeActor) ==
               (ordinary || daytime));
    }
    std::map<std::string, int> values;
    const auto read = [&](const char* key, int fallback) {
        const auto it = values.find(key); return it == values.end() ? fallback : it->second;
    };
    const auto write = [&](const char* key, int value) { values[key] = value; };
    const auto& rule = *FindGameplayRule("gEnhancements.NightGSAlwaysSpawn");
    assert(rule.minimum == 0 && rule.maximum == 1 && rule.fallback == 0 && !rule.trickGoverned);
    assert(LogicRuleKey(rule.name) == "gCombo.Logic.gEnhancements.NightGSAlwaysSpawn");
    SetEnhancementRule(rule, 1, read, write);
    assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 0) == 0);
    assert(!CanCollectNightSkulltula(true, false, false, true,
        read(LogicRuleKey(rule.name).c_str(), 0) != 0, true));
    SetLogicRule(rule, 1, write);
    assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 0) == 1);
    assert(CanCollectNightSkulltula(true, false, false, true,
        read(LogicRuleKey(rule.name).c_str(), 0) != 0, true));
    assert(!CanCollectNightSkulltula(true, false, false, true, true, false));
    const nlohmann::json seed = {{"gameplayRules", CaptureGameplayRules(read)}};
    const auto original = seed.dump();
    SetEnhancementRule(rule, 0, read, write);
    assert(read(rule.name, 9) == 0 && read(LogicRuleKey(rule.name).c_str(), 9) == 0);
    SetEnhancementRule(rule, 1, read, write);
    const auto run = CaptureGameplayRules(read);
    ApplyGameplayRules(GameplayRulesForRun(seed, run), write);
    assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 9) == 0);
    assert(seed.dump() == original);
    ApplyGameplayRules(GameplayRulesForSeed(seed), write);
    assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 0) == 1);
    auto old = seed["gameplayRules"];
    old["values"].erase(rule.name); old["logicValues"].erase(rule.name);
    for (int version : {1, 2}) {
        old["version"] = version;
        ApplyGameplayRules(GameplayRulesForSeed({{"gameplayRules", old}}), write);
        assert(read(rule.name, 9) == 0 && read(LogicRuleKey(rule.name).c_str(), 9) == 0);
    }
    std::cout << "PASS: 64 night-Skulltula time/song/rule/actor cases, separate runtime/logic, saved overrides and legacy defaults\n";
}
