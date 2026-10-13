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
    assert(std::string(GameplayRuleHeading(count.name, false)) == "Quest requirements");
    // Time-of-day rules sit with the other time-of-day conveniences on Quality of life.
    for (const char* rule :
         { "gEnhancements.OpenAllHours", "gEnhancements.DayGravePull", "gEnhancements.NightGSAlwaysSpawn" }) {
        assert(std::string(GameplayRulePage(rule, false)) == "Quality of life");
        assert(std::string(GameplayRuleHeading(rule, false)) == "Time of day & cycle");
        assert(std::string(GameplayRuleHeading(rule, true)) == "Time of day");
    }
    assert(std::string(GameplayRulePage("gEnhancements.BlueFireArrows", false)) == "Items & equipment");
    assert(std::string(GameplayRuleHeading("gEnhancements.BlueFireArrows", false)) == "Bow & arrows");
    // Every rule has a heading for its in-game half, and every logic half one on Randomizer > Gameplay rules.
    for (const auto& rule : kGameplayRules) {
        if (IsLegacyGameplayRule(rule.name) || rule.trickGoverned)
            continue;
        assert(*GameplayRuleHeading(rule.name, false));
        if (!rule.runtimeOnly && !IsSongTravelRule(rule.name))
            assert(*GameplayRuleHeading(rule.name, true));
    }
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
    // Same check types under the same headings in both worlds (MM ProfileMmLayout.inc cards).
    assert(OotLocationPoolGroup("gRandoSettings.ShuffleFreestanding",0)==1);
    assert(OotLocationPoolGroup("gRandoSettings.ShuffleWonderItems",0)==1);
    assert(OotLocationPoolGroup("gRandoSettings.ShuffleFairySpots",0)==1);
    assert(OotLocationPoolGroup("gRandoSettings.ShuffleBeehives",0)==2);
    assert(OotLocationPoolGroup("gRandoSettings.ShufflePots",0)==2);
    assert(OotLocationPoolGroup("gRandoSettings.ShopsanityCount",1)==3);
    assert(SF_OCARINA == 24 && SF_SPELL_WIND == 25 && SF_SPELL_FIRE == 26 && SF_SPELL_LOVE == 27);
    assert(SF_BOOTS_IRON == 28 && SF_BOOTS_HOVER == 29 && SF_TUNIC_GORON == 30 && SF_TUNIC_ZORA == 31 && SF_COUNT == 32);
    // The MM gear is public with OoTMM's shared settings (data.ts:3024-3054), keeping the stored lowercase CVar keys.
    // Each shared item stays off unless its MM option is on: the menu greys it out (ComboMenu.cpp DrawSharedItems) and
    // generation ignores it (SharedExtraSpellEnabled), as OoTMM's cond `hasOoTMM && <x>Mm` resets it (util.ts:80-93).
    const char* gearKeys[] = {"bootsIron", "bootsHover", "tunicGoron", "tunicZora"};
    const char* gearNames[] = {"Iron Boots", "Hover Boots", "Goron Tunic", "Zora Tunic"};
    const char* gearLabels[] = {"Shared Iron Boots", "Shared Hover Boots", "Shared Goron Tunic", "Shared Zora Tunic"};
    const char* gearTooltips[] = {"Combines the Iron Boots from OoT and MM into one item for both games",
        "Combines the Hover Boots from OoT and MM into one item for both games",
        "Combines the Goron Tunics from OoT and MM into two items for both games",
        "Combines the Zora Tunics from OoT and MM into two items for both games"};
    const char* gearOptions[] = {"RO_MM_BOOTS_IRON", "RO_MM_BOOTS_HOVER", "RO_MM_TUNIC_GORON", "RO_MM_TUNIC_ZORA"};
    const char* gearSettings[] = {"Iron Boots (MM)", "Hover Boots (MM)", "Goron Tunic (MM)", "Zora Tunic (MM)"};
    for (int i=0;i<4;++i) {
        const auto& gear=SharedFamilyByIndex(SF_BOOTS_IRON+i);
        assert(gear.publicOption);
        assert(std::string(gear.key)==gearKeys[i]);
        assert(std::string(gear.cvar)==std::string("gCombo.Rando.Shared.")+gearKeys[i]);
        assert(std::string(gear.ootName)==gearNames[i] && std::string(gear.mmName)==gearNames[i]);
        assert(std::string(gear.label)==gearLabels[i] && std::string(gear.tooltip)==gearTooltips[i]);
        assert(std::string(SharedFamilyMmExtension(SF_BOOTS_IRON+i))==gearOptions[i]);
        assert(std::string(SharedFamilyMmExtensionLabel(SF_BOOTS_IRON+i))==gearSettings[i]);
        for (int on : {0, 1}) {
            const nlohmann::json dump={{"options",{{gearOptions[i],on}}}};
            assert(SharedExtraSpellEnabled(SF_BOOTS_IRON+i,dump)==(on==1));
        }
        assert(!SharedExtraSpellEnabled(SF_BOOTS_IRON+i,nlohmann::json::object()));
    }
    // The three spells are public with their MM settings (Farore's Wind since 0.3.9), like the gear.
    const char* spellNames[]={"Shared Farore's Wind","Shared Din's Fire","Shared Nayru's Love"};
    const char* spellTooltips[]={"Combines Farore's Wind from OoT and MM into one item for both games",
        "Combines Din's Fire from OoT and MM into one item for both games",
        "Combines Nayru's Love from OoT and MM into one item for both games"};
    for (int i=0;i<3;++i) {
        const auto& spell=SharedFamilyByIndex(SF_SPELL_WIND+i);
        assert(spell.publicOption && std::string(spell.label)==spellNames[i] && std::string(spell.tooltip)==spellTooltips[i]);
    }
    assert(std::string(SharedFamilyByIndex(SF_SPELL_FIRE).cvar)=="gCombo.Rando.Shared.SpellFire");
    assert(std::string(SharedFamilyByIndex(SF_SPELL_LOVE).cvar)=="gCombo.Rando.Shared.SpellLove");
    std::set<std::string> presetNames;
    for (const auto& preset:kCommunityPresets) {
        assert(presetNames.insert(preset.name).second);
        cv.clear();
        // A file whose goal was a Triforce Hunt: the profile derives TriforceHunt from Goal, so the preset sets Goal.
        write("gCombo.Rando.Goal",1); write("gCombo.Rando.TriforceHunt",1);
        for (const auto& value:CommunityPresetOptions(preset.id)) {
            assert(value.key && *value.key && value.value>=0);
            write(value.key,value.value);
        }
        // Glitchless; only Allsanity's OoTMM logic is beatable-only (presets.ts:430).
        assert(read(kNativeLogicRule,-1)==0 && read(kCombinedGuarantee,-1)==(preset.id==CommunityPreset::Allsanity));
        assert(read("gCombo.Rando.Goal",-1)==0 && read("gCombo.Rando.TriforceHunt",-1)==0); // OoTMM: Ganon & Majora
        for (int i=0;i<SF_COUNT;++i) {
            if (i == SF_OCARINA || !SharedFamilyByIndex(i).publicOption) { assert(!cv.count(SharedFamilyByIndex(i).cvar)); continue; }
            const bool shared=preset.id==CommunityPreset::Allsanity || (preset.id==CommunityPreset::Beginner && BeginnerSharesFamily(i));
            assert(read(SharedFamilyByIndex(i).cvar,-1)==(shared?1:0));
        }
        assert(!cv.count("gCombo.Rando.SyncCosmetics")); // visual preferences are preserved
        assert(!cv.count("gSettings.Volume.Master"));
    }
    std::cout << "Menu refresh: legacy migration, all count values, rule ownership, shared preference routing and preset scope passed\n";
}
