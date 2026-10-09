#include "rando/GameplayRuleSnapshots.h"
#include "rando/GenerationRules.h"
#include "gui/ComboTricksModel.h"
#include "soh/Enhancements/randomizer/location_access.h"
// Include the actual native exit definitions, not a duplicate Shadow predicate.
#define logic nativeRouteLogic
#include "../soh/soh/Enhancements/randomizer/location_access/overworld/graveyard.cpp"
#undef logic
#include <cassert>
#include <cstring>
#include <iostream>

using namespace ComboRando;
using namespace Rando;

static bool ShadowReachable(bool startsAtWarpPad) {
    std::vector<RandomizerRegion> pending = {
        startsAtWarpPad ? RR_GRAVEYARD_WARP_PAD_REGION : RR_THE_GRAVEYARD
    };
    std::set<RandomizerRegion> reached;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        if (!reached.insert(current).second) continue;
        for (const auto& exit : areaTable[current].exits) {
            if (exit.condition()) pending.push_back(exit.destination);
        }
    }
    return reached.count(RR_SHADOW_TEMPLE_ENTRYWAY) != 0;
}

int main() {
    RegionTable_Init_Graveyard();
    const auto* rule = FindGameplayRule("gEnhancements.RestoreShadowDoorUnload");
    assert(rule && rule->fallback == 0 && rule->maximum == 1);
    assert(rule->trickGoverned);
    assert(std::strcmp(rule->label, "Shadow Door Unload") == 0);
    assert(FindGameplayRule(LogicRuleKey(rule->name).c_str()) == rule);
    assert(!IsRetiredNativeRuleControl(rule->name)); // native restoration remains visible
    assert(IsRetiredNativeRuleControl(LogicRuleKey(rule->name).c_str())); // no duplicate logical widget
    const auto read = [](const char* key, int fallback) {
        const auto found = probeCVars.find(key);
        return found == probeCVars.end() ? fallback : found->second;
    };
    const auto write = [](const char* key, int value) { probeCVars[key] = value; };
    const auto baseline = CaptureGameplayRules(read);
    assert(baseline["values"][rule->name] == 0 && baseline["logicValues"][rule->name] == 0);
    const auto row = ComboTricks::RestoredShadowDoorInfo(rule->label, rule->help, false);
    assert(row.difficulty == 5 && (row.tagMask & (1u << 6)) != 0);
    assert(ComboTricks::Matches(row, 0, "shadow door", 0, 2, -1));
    assert(ComboTricks::Matches(row, 0, "Graveyard", 1, 0, 5));
    assert(!ComboTricks::Matches(row, 0, "shadow", 2, 0, -1));
    assert(!ComboTricks::Matches(row, 0, "shadow", 0, 1, -1));

    SetEnhancementRule(*rule, 1, read, write);
    assert(read(rule->name, 0) == 1 && read(LogicRuleKey(rule->name).c_str(), 0) == 0);
    for (bool adult : {false, true}) {
        nativeRouteLogic->IsAdult = adult; nativeRouteLogic->IsChild = !adult;
        assert(!ShadowReachable(true)); // enhancement only never adds the route
    }
    SetTrickGovernedRule(*rule, true, write); // exact setter used by the normal trick checkbox
    assert(read(rule->name, 0) == 1 && read(LogicRuleKey(rule->name).c_str(), 0) == 1);
    for (bool adult : {false, true}) {
        nativeRouteLogic->IsAdult = adult; nativeRouteLogic->IsChild = !adult;
        assert(ShadowReachable(true)); // no Din's Fire, fire arrows or other item
        assert(!ShadowReachable(false)); // still must reach the real warp-pad region
    }
    SetTrickGovernedRule(*rule, false, write);
    assert(read(rule->name, 0) == 1 && read(LogicRuleKey(rule->name).c_str(), 9) == 0);
    assert(!ShadowReachable(true)); // deselecting the trick keeps runtime restoration only
    SetTrickGovernedRule(*rule, true, write);

    const auto seedRules = CaptureGameplayRules(read);
    const nlohmann::json seed = {{"gameplayRules", seedRules}};
    const auto originalSeed = seed.dump();
    SetEnhancementRule(*rule, 0, read, write);
    assert(read(rule->name, 9) == 0 && read(LogicRuleKey(rule->name).c_str(), 9) == 0);
    auto runRules = CaptureGameplayRules(read);
    ApplyGameplayRules(GameplayRulesForRun(seed, runRules), write);
    assert(!ShadowReachable(true) && seed.dump() == originalSeed);
    ApplyGameplayRules(GameplayRulesForRun(seed, nlohmann::json()), write);
    assert(ShadowReachable(true) && seed.dump() == originalSeed);
    auto oldSeed = seed;
    oldSeed["gameplayRules"]["values"].erase(rule->name);
    oldSeed["gameplayRules"]["logicValues"].erase(rule->name);
    ApplyGameplayRules(GameplayRulesForRun(oldSeed, nlohmann::json()), write);
    assert(read(rule->name, 9) == 0 && read(LogicRuleKey(rule->name).c_str(), 9) == 0);
    assert(!ShadowReachable(true)); // older slots cannot inherit a previous assumption
    auto legacyRestoration = nlohmann::json{{"version", 1}, {"values", {{rule->name, 1}}}};
    ApplyGameplayRules(legacyRestoration, write);
    assert(read(rule->name, 0) == 1 && read(LogicRuleKey(rule->name).c_str(), 9) == 0);
    assert(!ShadowReachable(true)); // a legacy restoration is never inferred to be a selected trick
    legacyRestoration["logicValues"][rule->name] = 1; // v1 did not define this field
    const auto normalizedLegacy = NormalizeGameplayRuleSnapshot(legacyRestoration);
    assert(normalizedLegacy["values"][rule->name] == 1);
    assert(normalizedLegacy["logicValues"][rule->name] == 0);

    // Compile the unchanged native Din's Fire / adult fire-arrow alternatives
    // in the same graph. The new option may affect only the warp-pad exit.
    unsigned routeCases = 0;
    for (bool logical : {false, true}) {
        SetTrickGovernedRule(*rule, logical, write);
        for (bool pad : {false, true}) for (bool adult : {false, true})
        for (bool dins : {false, true}) for (bool trick : {false, true})
        for (bool arrows : {false, true}) {
            nativeRouteLogic->IsAdult = adult; nativeRouteLogic->IsChild = !adult;
            nativeRouteLogic->usableItems.clear();
            if (dins) nativeRouteLogic->usableItems.insert(RG_DINS_FIRE);
            if (arrows) nativeRouteLogic->usableItems.insert(RG_FIRE_ARROWS);
            ctx->fireArrowTrick = trick;
            assert(ShadowReachable(pad) == (pad && (logical || dins || (trick && adult && arrows))));
            ++routeCases;
        }
    }
    std::cout << "PASS: " << routeCases << " production Graveyard route cases; normal trick vs restoration-only selection, search filters, snapshots, legacy slots and preserved fire requirements\n";
}
