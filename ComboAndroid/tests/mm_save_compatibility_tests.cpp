#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include "BenJsonConversions.hpp"
extern "C" {
#include "z64player.h"
}
#include "ComboAdultSettings.h"
#include "rando/GearRules.h"

static void VerifyRando(const json& before, const RandoSaveInfo& loaded) {
    json after = loaded;
    assert(after["randoEvents"].size() == RE_MAX);
    for (size_t i = 0; i < before["randoEvents"].size(); ++i)
        assert(after["randoEvents"][i] == before["randoEvents"][i]);
    for (size_t i = before["randoEvents"].size(); i < RE_MAX; ++i)
        assert(after["randoEvents"][i] == 0);
    for (const char* key : {"randoInf", "randoSaveChecks", "finalSeed", "randoSaveOptions",
         "randoStartingItems", "foundDungeonKeys", "foundTriforcePieces", "namedTriforceMask", "sariaHintsAvailable",
         "sariaPriorityItems", "pendingTrapCount"}) {
        if (!before.contains(key)) continue;
        if (std::string(key) == "randoSaveOptions") {
            for (size_t i = 0; i < before[key].size(); ++i) assert(after[key][i] == before[key][i]);
            for (size_t i = before[key].size(); i < RO_MAX; ++i) assert(after[key][i] == 0);
        } else if (std::string(key) == "randoSaveChecks") {
            // Earlier saves do not have seenItemId/knowledgeSource on every check.
            for (size_t i = 0; i < before[key].size(); ++i)
                for (const auto& field : before[key][i].items())
                    assert(after[key][i].at(field.key()) == field.value());
        } else assert(after[key] == before[key]);
    }
}

static void VerifySave(const json& before) {
    auto loaded = std::make_unique<Save>();
    before.get_to(*loaded);
    json after = *loaded;
    assert(after["saveInfo"] == before["saveInfo"]);
    for (const auto& field : before.items()) {
        if (field.key() == "shipSaveInfo") continue;
        // Older fixtures write native u8 flags as JSON booleans. The native
        // serializer emits 0/1, which represents the same field value.
        if (field.value().is_boolean() && after.at(field.key()).is_number_integer()) {
            assert(after.at(field.key()).get<int>() == static_cast<int>(field.value().get<bool>()));
            continue;
        }
        if (after.at(field.key()) != field.value())
            std::cerr << "Historical save field mismatch: " << field.key()
                      << " before=" << field.value() << " after=" << after.at(field.key()) << '\n';
        assert(after.at(field.key()) == field.value());
    }
    VerifyRando(before["shipSaveInfo"]["rando"], loaded->shipSaveInfo.rando);
    auto reread=std::make_unique<Save>();after.get_to(*reread);assert(json(*reread)==after);
}

// The load policy (critique X3, lead decision 2): a save never fails to load for its adult or gear state. The adult
// runtime binds the seed age only when it is ready; gear never touches the adult state.
static void VerifyLoadPolicy(const json& page, bool expectGear) {
    for (bool adultReady : {false, true}) {
        auto save = std::make_unique<Save>();
        page.get_to(*save);
        const json before = *save;
        ComboAdultMM::PrepareLoadedSave(*save, adultReady);
        assert(ComboAdultMM::SaveHasGear(*save) == expectGear);
        const auto& options = save->shipSaveInfo.rando.randoSaveOptions;
        const bool adultSeed = options[RO_MM_ADULT_MASK] || options[RO_MM_CROSS_AGE] || options[RO_MM_STARTING_AGE];
        if (!adultReady || !adultSeed || before["shipSaveInfo"]["adultMm"]["enabled"] == 1)
            assert(json(*save) == before); // loads as it is: child Link, gear only while its runtime is active
        else
            assert(save->shipSaveInfo.adultMm.enabled == 1);
    }
}

static void VerifyGearRules() {
    using ComboRando::GearRules;
    GearRules rules;
    rules.bootsIronMm = rules.tunicZoraMm = true;
    assert(ComboRando::ReadGearRules(nullptr, rules) && !rules.bootsIronMm && !rules.bootsHoverMm &&
           !rules.tunicGoronMm && !rules.tunicZoraMm); // old seeds without gearRules: all off
    for (int mask = 0; mask < 16; ++mask) {
        const GearRules written{ (mask & 1) != 0, (mask & 2) != 0, (mask & 4) != 0, (mask & 8) != 0 };
        GearRules read;
        assert(ComboRando::ReadGearRules(json::parse(written.json().dump()), read));
        assert(read.bootsIronMm == written.bootsIronMm && read.bootsHoverMm == written.bootsHoverMm &&
               read.tunicGoronMm == written.tunicGoronMm && read.tunicZoraMm == written.tunicZoraMm);
        const json dump = { { "options",
                              { { "RO_MM_BOOTS_IRON", int(written.bootsIronMm) },
                                { "RO_MM_BOOTS_HOVER", int(written.bootsHoverMm) },
                                { "RO_MM_TUNIC_GORON", int(written.tunicGoronMm) },
                                { "RO_MM_TUNIC_ZORA", int(written.tunicZoraMm) } } } };
        assert(ComboRando::GearRulesFromMmDump(dump).json() == written.json());
    }
    assert(ComboRando::GearRulesFromMmDump(json::object()).json() == GearRules{}.json());
    const json bad[] = { json{ { "version", 2 } }, json{ { "version", 1 }, { "bootsIronMm", true } },
                         json{ { "version", 1 }, { "bootsIronMm", 2 }, { "bootsHoverMm", 0 }, { "tunicGoronMm", 0 },
                               { "tunicZoraMm", 0 } },
                         json::array() };
    for (const auto& j : bad) {
        GearRules read;
        read.tunicGoronMm = true;
        assert(!ComboRando::ReadGearRules(j, read) && !read.tunicGoronMm);
    }
}

int main(int argc, char** argv) {
    static_assert(RE_LEARN_SCARECROW_SONG == 83, "Legacy event IDs must remain stable");
    static_assert(RE_SNOWHEAD_RAISE_PILLAR == 84, "0.2.13 event IDs must remain stable");
    static_assert(RE_EXPANDED_MEET_KOUME_DAY1 == 88 && RE_GORON_GRAVE_FARORE == 94 &&
                  RE_SHT_FARORE_MAP == 95 && RE_SPI_ROOF_FARORE == 96,
                  "Existing event IDs must remain stable");
    static_assert(RE_MM_SWAMP_OCTO_LEFT == 97 && RE_MM_SWAMP_OCTO_RIGHT == 98 &&
                  RE_MM_SWAMP_HIVE == 99 && RE_MM_SNOWHEAD_PUSH_BLOCK == 100 && RE_MAX == 101,
                  "Review appended grotto event compatibility");
    auto source = std::make_unique<SaveContext>();
    source->save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    auto& info = source->save.saveInfo;
    std::fill(std::begin(info.inventory.items), std::end(info.inventory.items), ITEM_NONE);
    info.inventory.items[SLOT_OCARINA] = ITEM_OCARINA_OF_TIME;
    info.inventory.items[SLOT_BOW] = ITEM_BOW;
    info.inventory.questItems = (1u << QUEST_SONG_SOARING) | (1u << QUEST_REMAINS_ODOLWA);
    info.skullTokenCount = 0x0014001e;
    info.playerData.owlActivationFlags = 0x2a5;
    info.playerData.rupees = 175;
    auto& r = source->save.shipSaveInfo.rando;
    r.finalSeed = 990020;
    r.namedTriforceMask = 5;
    r.ocarinaOwnedBits = 3;
    r.expandedSoulsOwned[0] = 0xa5;
    r.expandedSoulsOwned[7] = 3;
    r.mmMissingSoulsOwned = 0x2a;
    auto& wind = source->save.shipSaveInfo.faroresWind;
    wind.owned = 1;
    wind.point[0].pos = {123.5f, -42.0f, 819.0f};
    wind.point[0].data = -7;
    wind.point[0].entrance = 0xfffd;
    wind.point[0].unk_18 = 0xa5a55a5a;
    wind.point[1].pos = {-9.0f, 11.5f, 400.0f};
    wind.point[1].data = 1;
    wind.point[1].entrance = 0x6890;
    wind.top[1].tempSwitchFlags = 0x12345678;
    wind.dungeon[0].tempCollectFlags = 0x87654321;
    for (size_t i = 0; i < RO_MAX; ++i) r.randoSaveOptions[i] = static_cast<u32>(i * 17 + 3);
    r.foundDungeonKeys[1] = 2;
    info.inventory.dungeonKeys[1] = 0;
    r.randoStartingItems[0] = RI_WOODFALL_SMALL_KEY;
    r.pendingTrapCount = 2;
    for (size_t i = 0; i < RE_MAX; ++i) r.randoEvents[i] = static_cast<u8>(i % 3);
    r.randoSaveChecks[12].obtained = true;
    r.randoSaveChecks[18].skipped = true;
    r.randoSaveChecks[25].randoItemId = RI_HEART_PIECE;
    r.randoSaveChecks[25].seenItemId = RI_HEART_PIECE;
    r.randoSaveChecks[25].knowledgeSource = 1;

    json modern = *source;
    auto current = std::make_unique<SaveContext>();
    modern.get_to(*current);
    assert(json(*current) == modern);
    // Exact production serializer/deserializer: every supported predecessor prefix.
    for(size_t length : {size_t(83),size_t(84),size_t(88),size_t(94),size_t(95),size_t(96),size_t(97),size_t(100),size_t(101)}) {
        auto prefix=modern["save"]["shipSaveInfo"]["rando"];
        while(prefix["randoEvents"].size()>length)prefix["randoEvents"].erase(prefix["randoEvents"].size()-1);
        RandoSaveInfo target{};prefix.get_to(target);VerifyRando(prefix,target);
        json saved=target;RandoSaveInfo reloaded{};saved.get_to(reloaded);assert(json(reloaded)==saved);
    }
    for(size_t length : {size_t(0),size_t(82),size_t(98),size_t(99),size_t(102)}) {
        auto bad=modern["save"]["shipSaveInfo"]["rando"];
        bad["randoEvents"]=std::vector<int>(length,0);
        bool rejected=false;try{RandoSaveInfo target{};bad.get_to(target);}catch(const std::exception&){rejected=true;}assert(rejected);
    }
    auto legacy = modern;
    legacy["save"]["shipSaveInfo"]["rando"].erase("namedTriforceMask");
    legacy["save"]["shipSaveInfo"]["rando"].erase("ocarinaOwnedBits");
    legacy["save"]["shipSaveInfo"]["rando"].erase("expandedSoulsOwned");
    legacy["save"]["shipSaveInfo"]["rando"].erase("mmMissingSoulsOwned");
    legacy["save"]["shipSaveInfo"].erase("faroresWind");
    auto& events = legacy["save"]["shipSaveInfo"]["rando"]["randoEvents"];
    while (events.size() > 83) events.erase(events.size() - 1);
    auto& oldOptions = legacy["save"]["shipSaveInfo"]["rando"]["randoSaveOptions"];
    while (oldOptions.size() > RO_KEY_RING_WOODFALL) oldOptions.erase(oldOptions.size() - 1);
    auto old = std::make_unique<SaveContext>();
    legacy.get_to(*old);
    assert(old->save.shipSaveInfo.rando.namedTriforceMask == 0);
    assert(old->save.shipSaveInfo.rando.ocarinaOwnedBits == 0);
    for (auto bits : old->save.shipSaveInfo.rando.expandedSoulsOwned) assert(bits == 0);
    assert(old->save.shipSaveInfo.rando.mmMissingSoulsOwned == 0);
    assert(old->save.shipSaveInfo.faroresWind.owned == 0);
    for (auto& point : old->save.shipSaveInfo.faroresWind.point) assert(point.data == 0);
    VerifySave(legacy["save"]);
    assert(json(*old)["eventInf"] == legacy["eventInf"]);
    assert(json(*old)["bottleTimerCurTimes"] == legacy["bottleTimerCurTimes"]);
    auto previousRelease = modern["save"];
    auto& previousEvents = previousRelease["shipSaveInfo"]["rando"]["randoEvents"];
    while (previousEvents.size() > 84) previousEvents.erase(previousEvents.size() - 1);
    VerifySave(previousRelease);

    // Truncated, future, and malformed payloads must still fail; never silently
    // default missing inventory/placement data to create a playable empty save.
    for (auto mutation : {0, 1, 2, 3}) {
        auto broken = legacy["save"]["shipSaveInfo"]["rando"];
        if (mutation == 0) broken["randoEvents"].erase(82);
        if (mutation == 1) broken["randoEvents"] = json::array();
        if (mutation == 2) broken["randoEvents"] = std::vector<int>(RE_MAX + 1, 0);
        if (mutation == 3) broken["randoEvents"][12] = "broken";
        bool rejected = false;
        try { RandoSaveInfo target{}; broken.get_to(target); }
        catch (const std::exception&) { rejected = true; }
        assert(rejected);
    }
    for (size_t length : {size_t(0), size_t(RO_TRIFORCE_PIECES_REQUIRED - 1), size_t(RO_KEY_RING_WOODFALL + 1), size_t(RO_MAX + 1)}) {
        auto broken = modern["save"]["shipSaveInfo"]["rando"];
        broken["randoSaveOptions"] = std::vector<int>(length, 0);
        bool badLengthRejected = false;
        try { RandoSaveInfo target{}; broken.get_to(target); }
        catch (const std::exception&) { badLengthRejected = true; }
        assert(badLengthRejected);
    }
    for (size_t length : {size_t(RO_TRIFORCE_PIECES_REQUIRED), size_t(RO_KEY_RING_WOODFALL),
                         size_t(RO_FAIRY_OCARINA_MM), size_t(RO_SHUFFLE_NPC_SOULS)}) {
        auto prefix = modern["save"]["shipSaveInfo"]["rando"];
        while (prefix["randoSaveOptions"].size() > length)
            prefix["randoSaveOptions"].erase(prefix["randoSaveOptions"].size()-1);
        RandoSaveInfo target{}; prefix.get_to(target); VerifyRando(prefix,target);
    }
    static_assert(RO_MM_ADULT_MASK==85 && RO_MAX==91,"Review immutable complete85-option predecessor");
    auto save85=modern["save"];
    auto& options85=save85["shipSaveInfo"]["rando"]["randoSaveOptions"];
    while(options85.size()>RO_MM_ADULT_MASK)options85.erase(options85.size()-1);
    VerifySave(save85);
    for(size_t length : {size_t(84),size_t(86),size_t(87),size_t(88),size_t(89),size_t(90)}) {
        auto bad=modern["save"]["shipSaveInfo"]["rando"];
        bad["randoSaveOptions"]=std::vector<int>(length,0);
        bool rejected=false;try{RandoSaveInfo target{};bad.get_to(target);}catch(const std::exception&){rejected=true;}assert(rejected);
    }
    auto malformedOptions = modern["save"]["shipSaveInfo"]["rando"];
    malformedOptions["randoSaveOptions"][12] = "broken";
    bool badOptionRejected = false;
    try { RandoSaveInfo target{}; malformedOptions.get_to(target); }
    catch (const std::exception&) { badOptionRejected = true; }
    assert(badOptionRejected);
    auto missingInventory = legacy["save"];
    missingInventory["saveInfo"].erase("inventory");
    bool rejected = false;
    try { auto target = std::make_unique<Save>(); missingInventory.get_to(*target); }
    catch (const std::exception&) { rejected = true; }
    assert(rejected);

    // Adult and gear state (lead decision 2): every one of these loads, and appended gear/spell bytes read as 0.
    static_assert(RO_MM_BOOTS_IRON == 87 && RO_MM_TUNIC_ZORA == 90 && RO_MAX == 91, "Review the gear option ids");
    auto release039 = modern["save"]; // 0.3.9 saves carry all RO_MAX options
    for (auto id : {RO_MM_ADULT_MASK, RO_MM_STARTING_AGE, RO_MM_CROSS_AGE, RO_MM_BOOTS_IRON, RO_MM_BOOTS_HOVER,
                    RO_MM_TUNIC_GORON, RO_MM_TUNIC_ZORA})
        release039["shipSaveInfo"]["rando"]["randoSaveOptions"][id] = 0;
    VerifySave(release039);
    VerifyLoadPolicy(release039, false);
    VerifyLoadPolicy(save85, false);
    auto noAdult = release039;
    noAdult["shipSaveInfo"].erase("adultMm");
    noAdult["shipSaveInfo"].erase("spellFireOwned");
    noAdult["shipSaveInfo"].erase("spellLoveOwned");
    {
        auto loaded = std::make_unique<Save>();
        noAdult.get_to(*loaded);
        const auto& state = loaded->shipSaveInfo.adultMm;
        assert(!state.enabled && !state.boots && !state.tunic && !state.gearTrade2Bits && !state.gearTrade3Bits);
        assert(!loaded->shipSaveInfo.spellFireOwned && !loaded->shipSaveInfo.spellLoveOwned);
    }
    VerifyLoadPolicy(noAdult, false);
    // Gear options on while the gear or adult runtime is missing: loads, adult state untouched (the gear runtime stays
    // inactive until Combo_MM_GearActive, checked in mm_gear_runtime_tests.py).
    auto gear = release039;
    for (auto id : {RO_MM_BOOTS_IRON, RO_MM_BOOTS_HOVER, RO_MM_TUNIC_GORON, RO_MM_TUNIC_ZORA})
        gear["shipSaveInfo"]["rando"]["randoSaveOptions"][id] = 1;
    gear["shipSaveInfo"]["adultMm"]["gearTrade2Bits"] = 6;
    gear["shipSaveInfo"]["adultMm"]["boots"] = 2;
    gear["shipSaveInfo"]["adultMm"]["tunic"] = 3;
    VerifySave(gear);
    VerifyLoadPolicy(gear, true);
    // A private diagnostics save whose gear turned the adult state on (0.3.9 ConfigureSeedAge) still loads unchanged.
    auto diagnostic = gear;
    diagnostic["shipSaveInfo"]["adultMm"]["enabled"] = 1;
    diagnostic["shipSaveInfo"]["adultMm"]["initialized"] = 1;
    diagnostic["linkAge"] = 1;
    VerifyLoadPolicy(diagnostic, true);
    // Adult options with the adult runtime unavailable (always, while it is unfinished): loads as child.
    auto adultSeed = release039;
    adultSeed["shipSaveInfo"]["rando"]["randoSaveOptions"][RO_MM_STARTING_AGE] = 1;
    adultSeed["shipSaveInfo"]["rando"]["randoSaveOptions"][RO_MM_BOOTS_IRON] = 1;
    VerifyLoadPolicy(adultSeed, true);
    {
        auto loaded = std::make_unique<Save>();
        adultSeed.get_to(*loaded);
        ComboAdultMM::PrepareLoadedSave(*loaded, true);
        assert(loaded->shipSaveInfo.adultMm.enabled && loaded->linkAge == 0); // a ready adult runtime binds the age
        auto gearOnly = std::make_unique<Save>();
        gear.get_to(*gearOnly);
        assert(ComboAdultMM::ConfigureSeedAge(*gearOnly, 1, false, false) && !gearOnly->shipSaveInfo.adultMm.enabled);
    }
    // Cross-Games Age (F5b 5a-2): the seed only stores an age. A new file starts child with its gear and buttons
    // untouched (no 41a2b7c0 loadout swap); a crossAge save with the model missing loads as it is; the saved adult
    // state survives a round trip.
    auto crossSeed = gear;
    crossSeed["shipSaveInfo"]["rando"]["randoSaveOptions"][RO_MM_CROSS_AGE] = 1;
    crossSeed["linkAge"] = 0; // native MM's raw child age
    VerifySave(crossSeed);
    VerifyLoadPolicy(crossSeed, true);
    {
        auto created = std::make_unique<Save>();
        crossSeed.get_to(*created);
        const json before = *created;
        assert(ComboAdultMM::ConfigureSeedAge(*created, 1, false, true));
        const auto& state = created->shipSaveInfo.adultMm;
        assert(state.enabled && !state.maskEnabled && !state.initialized && created->linkAge == 1);
        json expected = before;
        expected["linkAge"] = 1;
        expected["shipSaveInfo"]["adultMm"]["enabled"] = 1;
        assert(json(*created) == expected); // boots, tunic, gear bits, buttons and loadouts untouched
        created->linkAge = 0;                // OoT's adult age, copied in at an MM entry
        assert(ComboAdultMM::ConfigureSeedAge(*created, 1, false, true) && created->linkAge == 0); // bound once
        const json adult = *created;
        auto reread = std::make_unique<Save>();
        adult.get_to(*reread);
        assert(ComboAdultMM::IsAdult(*reread) && json(*reread) == adult);
        ComboAdultMM::PrepareLoadedSave(*reread, false);
        assert(json(*reread) == adult);
    }
    for (const char* field : {"boots", "tunic", "gearTrade2Bits"}) { // malformed gear bytes are still a parse error
        auto broken = gear;
        broken["shipSaveInfo"]["adultMm"][field] = 7;
        bool rejected = false;
        try { auto target = std::make_unique<Save>(); broken.get_to(*target); }
        catch (const std::exception&) { rejected = true; }
        assert(rejected);
    }
    VerifyGearRules();

    if (argc == 2) {
        // Optional private historical container, never bundled with the tests.
        std::ifstream stream(argv[1]); assert(stream);
        json container; stream >> container;
        for (const auto* page : {"newCycleSave", "owlSave"})
            if (container["mm"].contains(page)) VerifySave(container["mm"][page]["save"]);
    }
    std::cout << "MM legacy/current save reads preserve inventory, songs, owls, checks and settings; adult and gear state never refuse a load; gearRules round-trip; malformed saves rejected\n";
}
