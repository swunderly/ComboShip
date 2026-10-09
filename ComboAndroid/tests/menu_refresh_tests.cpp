#include "gui/ComboNavigation.h"
#include "rando/ComboPresets.h"
#include "rando/GameplayRuleSnapshots.h"
#include "rando/GenerationRules.h"
#include <cassert>
#include <iostream>
#include <map>
#include <set>

int main() {
    using namespace ComboRando;
    const auto& count = *FindGameplayRule("gEnhancements.DifficultyOptions.BremenChickCount");
    assert(count.minimum == 1 && count.maximum == 10 && count.fallback == 10);
    assert(std::string(GameplayRulePage(count.name, true)) == "Gameplay rules");
    assert(std::string(GameplayRulePage(count.name, false)) == "Difficulty");
    std::map<std::string, int> cv;
    auto read = [&](const char* name, int fallback) { auto it=cv.find(name); return it==cv.end() ? fallback : it->second; };
    auto write = [&](const char* name, int value) { cv[name]=value; };
    const char* legacy = "gEnhancements.Masks.BremenOneChick";
    assert(ReadGameplayRule(count, false, read) == 10);
    write(legacy, 1); write(LogicRuleKey(legacy).c_str(), 1);
    assert(ReadGameplayRule(count, false, read) == 1 && ReadGameplayRule(count, true, read) == 0);
    for (int version : {1, 2}) for (int old : {0, 1}) {
        nlohmann::json block={{"version",version},{"values",{{legacy,old}}},{"logicValues",{{legacy,old}}}};
        auto normalized=NormalizeGameplayRuleSnapshot(block);
        assert(normalized["values"][count.name] == (old ? 1 : 10));
        cv.clear(); ApplyGameplayRules(block, write);
        assert(read(count.name, -1) == (old ? 1 : 10));
        const nlohmann::json seed={{"gameplayRules",block}};
        const auto original=seed.dump();
        const auto run=GameplayRulesForRun(seed,block);
        assert(run["values"][count.name] == (old ? 1 : 10) && seed.dump()==original);
        for (int n=1;n<=10;++n) {
            block["values"][count.name]=n; block["logicValues"][count.name]=n;
            normalized=NormalizeGameplayRuleSnapshot(block);
            assert(normalized["values"][count.name]==n); // explicit counts beat the old bool
        }
    }
    for (int n=1;n<=10;++n) {
        cv.clear(); SetLogicRule(count,n,write);
        assert(ReadGameplayRule(count,true,read)==0 && !cv.count(count.name));
        SetEnhancementRule(count,n,read,write);
        assert(read(count.name,0)==n && ReadGameplayRule(count,true,read)==0);
        SetEnhancementRule(count,n==10?1:n+1,read,write);
        assert(ReadGameplayRule(count,true,read)==0); // runtime-only count stays optional
    }
    assert(count.runtimeOnly);
    cv.clear(); write(count.name, 4); write(LogicRuleKey(count.name).c_str(), 4);
    const auto runtimeCount = CaptureGameplayRules(read);
    assert(runtimeCount["values"][count.name] == 4 && runtimeCount["logicValues"][count.name] == 0);
    auto oldCount = runtimeCount; oldCount["logicValues"][count.name] = 4;
    cv.clear(); ApplyGameplayRules(oldCount,write);
    assert(read(count.name,0)==4 && read(LogicRuleKey(count.name).c_str(),9)==0);
    assert(NormalizeGameplayRuleSnapshot(oldCount)["logicValues"][count.name] == 0);
    for (bool oot : {true, false}) {
        const auto camera = MenuRoute(oot,"Enhancements","Camera");
        assert(camera.scope=="media" && camera.page=="Camera");
        const auto cameraSearch = NativeSearchPageRoute(oot,"Enhancements","Camera");
        assert(cameraSearch.scope=="media" && cameraSearch.page=="Camera");
    }
    const auto minigames = MenuRoute(true,"Enhancements","Minigames");
    const auto ootCamera = NativeSearchPageRoute(true,"Settings","Camera");
    assert(ootCamera.scope=="media" && ootCamera.page=="Camera");
    assert(minigames.scope=="gameplay" && minigames.page=="Difficulty");
    for (const auto& rule : kGameplayRules)
        if (!IsSongTravelRule(rule.name)) assert(std::string(GameplayRulePage(rule.name,true))=="Gameplay rules");
    for (const auto& page : kRandomizerPages) assert(std::string(page.name)!="Difficulty");
    assert(IsLegacyGameplayRule(legacy));
    assert(IsLegacyGameplayRule("gCombo.Enhancements.AgelessSoaring"));
    assert(!IsLegacyGameplayRule(count.name));
    const auto& cross=*FindGameplayRule("gCombo.Enhancements.ForeignSongs");
    assert(std::string(GameplayRulePage(cross.name,true))=="Warps & travel");
    assert(std::string(GameplayRulePage(cross.name,false))=="Warps & travel");
    for (const auto& page:kRandomizerPages) assert(std::string(page.name)!="Everyday");
    for (const auto& page:kSharedPages) assert(std::string(page.name)!="Everyday");
    for (bool oot:{true,false}) {
        const auto saving=MenuGroupRoute(oot,"Enhancements",oot?"Quality of Life":"Gameplay",2,"Saving");
        assert(saving.scope=="gameplay" && saving.page=="Saving");
        const auto controls=MenuGroupRoute(oot,"Enhancements",oot?"Quality of Life":"Gameplay",1,"Controls");
        assert(controls.scope=="settings" && controls.page=="Controls");
    }
    assert(OotLocationPoolGroup("gRandoSettings.ShuffleSongs",0)==0);
    assert(OotLocationPoolGroup("gRandoSettings.FishsanityPondCount",0)==1);
    assert(OotLocationPoolGroup("gRandoSettings.ShuffleButterflyFairies",0)==1);
    assert(OotLocationPoolGroup("gRandoSettings.ShuffleFreestanding",0)==2);
    assert(OotLocationPoolGroup("gRandoSettings.ShopsanityCount",1)==3);
    assert(SF_OCARINA == 24 && SF_SPELL_WIND == 25 && SF_SPELL_FIRE == 26 && SF_SPELL_LOVE == 27);
    assert(SF_BOOTS_IRON == 28 && SF_BOOTS_HOVER == 29 && SF_TUNIC_GORON == 30 && SF_TUNIC_ZORA == 31 && SF_COUNT == 32);
    const char* privateGearKeys[] = {"bootsIron", "bootsHover", "tunicGoron", "tunicZora"};
    const char* privateGearNames[] = {"Iron Boots", "Hover Boots", "Goron Tunic", "Zora Tunic"};
    for (int i=0;i<4;++i) {
        const auto& gear=SharedFamilyByIndex(SF_BOOTS_IRON+i);
        assert(!gear.publicOption);
        assert(std::string(gear.key)==privateGearKeys[i]);
        assert(std::string(gear.ootName)==privateGearNames[i] && std::string(gear.mmName)==privateGearNames[i]);
    }
    assert(!SharedFamilyByIndex(SF_SPELL_WIND).publicOption);
    std::set<std::string> presetNames;
    for (const auto& preset:kCommunityPresets) {
        assert(presetNames.insert(preset.name).second);
        cv.clear();
        for (const auto& value:CommunityPresetOptions(preset.id)) {
            assert(value.key && *value.key && value.value>=0);
            write(value.key,value.value);
        }
        assert(read(kNativeLogicRule,-1)==0 && read(kCombinedGuarantee,-1)==0);
        assert(read("gCombo.Rando.TriforceHunt",-1)==0);
        for (int i=0;i<SF_COUNT;++i) {
            if (i == SF_OCARINA || !SharedFamilyByIndex(i).publicOption) { assert(!cv.count(SharedFamilyByIndex(i).cvar)); continue; }
            assert(read(SharedFamilyByIndex(i).cvar,-1)==(preset.id!=CommunityPreset::Default));
        }
        assert(!cv.count("gCombo.Rando.SyncCosmetics")); // visual preferences are preserved
        assert(!cv.count("gSettings.Volume.Master"));
    }
    std::cout << "Menu refresh: legacy migration, all count values, rule ownership, shared preference routing and preset scope passed\n";
}
