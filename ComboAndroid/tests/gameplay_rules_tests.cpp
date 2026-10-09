#include "rando/GameplayRules.h"
#include <cassert>
#include <iostream>
#include <map>
#include <string>

int main() {
    using namespace ComboRando;
    std::map<std::string, int> values;
    const auto read = [&](const char* key, int fallback) {
        auto it = values.find(key);
        return it == values.end() ? fallback : it->second;
    };
    const auto write = [&](const char* key, int value) { values[key] = value; };
    const auto defaults = CaptureGameplayRules(read);
    assert(defaults["version"] == 2 && defaults["values"].size() == std::size(kGameplayRules));
    assert(defaults["logicValues"].size()==std::size(kGameplayRules));
    const auto& age=*FindGameplayRule("gEnhancements.TimeTravel");
    SetLogicRule(age,3,write);
    assert(read(age.name,0)==3 && read(LogicRuleKey(age.name).c_str(),0)==3);
    // An enhancement-only activation does not create a route in generation.
    SetEnhancementRule(age,0,read,write);
    SetEnhancementRule(age,4,read,write);
    assert(read(age.name,0)==4 && read(LogicRuleKey(age.name).c_str(),0)==0);
    SetLogicRule(age,4,write);
    values["gEnhancements.TimeTravel"] = 4;
    values["gEnhancements.Masks.FierceDeitysAnywhere"] = 1;
    values["gEnhancements.Masks.BremenOneChick"] = 1;
    const auto seed = CaptureGameplayRules(read);
    values.clear();
    ApplyGameplayRules(nlohmann::json::parse(seed.dump()), write);
    assert(CaptureGameplayRules(read) == seed);
    // Unknown keys and malformed values cannot change unrelated preferences.
    auto imported = seed;
    imported["values"]["gUnrelated.Cheat"] = 1;
    imported["values"]["gEnhancements.TimeTravel"] = 999999999999ULL;
    imported["values"]["gEnhancements.Masks.FierceDeitysAnywhere"] = -1;
    imported["values"]["gEnhancements.Masks.BremenOneChick"] = "1";
    ApplyGameplayRules(imported, write);
    assert(values.size() == std::size(kGameplayRules)*2+std::size(kMmTricks) && CaptureGameplayRules(read) == seed);
    imported["version"] = 999;
    imported["values"]["gEnhancements.TimeTravel"] = 0;
    ApplyGameplayRules(imported, write);
    assert(CaptureGameplayRules(read) == seed);
    ApplyGameplayRules(nlohmann::json::object(), write);
    assert(CaptureGameplayRules(read) == seed); // legacy saves retain current choices
    // An intentional run override is separate from the original generated rules.
    values["gEnhancements.TimeTravel"] = 0;
    const auto run = CaptureGameplayRules(read);
    assert(run != seed && seed["values"]["gEnhancements.TimeTravel"] == 4);
    ApplyGameplayRules(seed, write);
    ApplyGameplayRules(run, write);
    assert(values["gEnhancements.TimeTravel"] == 0);
    // Legacy coupled seed rules migrate into both fields without changing their abilities.
    auto legacy=seed; legacy["version"]=1; legacy.erase("logicValues");
    values.clear(); ApplyGameplayRules(legacy,write);
    assert(read(age.name,0)==4 && read(LogicRuleKey(age.name).c_str(),0)==4);
    // Missing additions in an older seed clear a previous slot's assumptions.
    auto old=legacy; old["values"].erase("gEnhancements.Masks.BlastMaskKeg"); old.erase("mmTricks");
    values["gEnhancements.Masks.BlastMaskKeg"]=1;
    values[LogicRuleKey("gEnhancements.Masks.BlastMaskKeg")]=1;
    values[kMmTricks[0].key]=1;
    ApplyGameplayRules(old,write);
    assert(read("gEnhancements.Masks.BlastMaskKeg",9)==0 && read(LogicRuleKey("gEnhancements.Masks.BlastMaskKeg").c_str(),9)==0);
    assert(read(kMmTricks[0].key,9)==0);
    // Real MM techniques have the same immutable seed / intentional override separation.
    values[kMmTricks[0].key]=1; auto trickSeed=CaptureGameplayRules(read);
    values[kMmTricks[0].key]=0; auto trickRun=CaptureGameplayRules(read);
    ApplyGameplayRules(trickSeed,write); assert(read(kMmTricks[0].key,0)==1);
    ApplyGameplayRules(trickRun,write); assert(read(kMmTricks[0].key,9)==0);
    // Both panels identify the same rule; untrusted imports cannot set unrelated CVars.
    assert(FindGameplayRule(age.name)==&age);
    assert(FindGameplayRule(LogicRuleKey(age.name).c_str())==&age);
    assert(!FindGameplayRule("gUnrelated.Cheat"));
    // Restoration UI and normal tricks use one capability and one permission.
    for (const char* name : {"gEnhancements.RestoreShadowDoorUnload", "gEnhancements.EarlyEyeballFrog"}) {
        const auto& rule = *FindGameplayRule(name);
        assert(rule.trickGoverned && rule.fallback == 0);
        values.clear();
        SetEnhancementRule(rule, 1, read, write);
        assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 0) == 0);
        SetTrickGovernedRule(rule, true, write);
        assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 0) == 1);
        SetTrickGovernedRule(rule, false, write);
        assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 9) == 0);
        SetTrickGovernedRule(rule, true, write);
        SetEnhancementRule(rule, 0, read, write);
        assert(read(rule.name, 9) == 0 && read(LogicRuleKey(rule.name).c_str(), 9) == 0);
        auto oldRestoration = CaptureGameplayRules(read);
        oldRestoration["version"] = 1;
        oldRestoration["values"][rule.name] = 1;
        oldRestoration.erase("logicValues");
        ApplyGameplayRules(oldRestoration, write);
        assert(read(rule.name, 0) == 1 && read(LogicRuleKey(rule.name).c_str(), 9) == 0);
    }
    const auto& ageless = *FindGameplayRule("gCombo.Enhancements.AgelessSoaring");
    assert(!ageless.trickGoverned && ageless.fallback == 0 && ageless.maximum == 1);
    values.clear();
    SetEnhancementRule(ageless, 1, read, write);
    assert(read(ageless.name, 0) == 1 && read(LogicRuleKey(ageless.name).c_str(), 0) == 0);
    SetLogicRule(ageless, 1, write);
    auto newSeed = CaptureGameplayRules(read);
    values.clear(); ApplyGameplayRules(newSeed, write);
    assert(read(ageless.name, 0) == 1 && read(LogicRuleKey(ageless.name).c_str(), 0) == 1);
    newSeed["values"].erase(ageless.name);
    newSeed["logicValues"].erase(ageless.name);
    ApplyGameplayRules(newSeed, write);
    assert(read(ageless.name, 9) == 0 && read(LogicRuleKey(ageless.name).c_str(), 9) == 0);
    for (int mode = 0; mode <= 5; ++mode) {
        for (int song = 0; song <= 1; ++song) {
            for (int oot = 0; oot <= 1; ++oot) {
                for (int sword = 0; sword <= 1; ++sword) {
                    const bool expected = song && ((mode == 1 && oot) || (mode == 2 && oot && sword) ||
                                                   mode == 3 || (mode == 4 && sword));
                    assert(CanChangeAgeWithSong(mode, song != 0, oot != 0, sword != 0) == expected);
                }
            }
        }
    }
    std::cout << "Gameplay rules: persistence, legacy import, allowlist, run overrides and age requirements passed\n";
}
