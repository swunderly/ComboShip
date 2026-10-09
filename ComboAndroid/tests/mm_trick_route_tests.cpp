#include "rando/GameplayRuleSnapshots.h"
#include "rando/MmTrickRoutes.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

// The build extracts these eight definitions from the production COMBO_BUILD
// branch of Logic.h. Exercise their actual item/event/time wiring with controlled
// inputs, rather than pasting a second implementation of those macros here.
static std::set<int> items;
static std::map<std::string, int> cvars;
static bool deku, zora, goron, swim, usableBean, iceArrow, playableElegy, moonsTear;
static bool dayOneWindow, dayTwoWindow;
enum { ITEM_MASK_BUNNY, ITEM_BOW, ITEM_MASK_CAPTAIN, ITEM_HOOKSHOT,
       RE_ACCESS_SEAHORSE, RE_MAIL_LETTER_TO_KAFEI,
       TIME_DAY1_PM_03_00, TIME_NIGHT1_AM_12_00, TIME_NIGHT2_PM_06_00, TIME_NIGHT2_AM_12_00 };
static bool events[12];
static int CVarGetInteger(const char* key, int fallback) {
    const auto value = cvars.find(key);
    return value == cvars.end() ? fallback : value->second;
}
#define HAS_ITEM(item) (items.count(item) != 0)
#define CAN_USE_BOW HAS_ITEM(ITEM_BOW) // This fixture supplies renewable arrows.
#define CAN_BE_DEKU deku
#define CAN_BE_ZORA zora
#define CAN_BE_GORON goron
#define CAN_USE_ABILITY(ability) swim
#define CAN_USE_DAY2_RAIN_BEAN usableBean
#define CAN_USE_MAGIC_ARROW(arrow) iceArrow
#define CAN_PLAY_SONG(song) playableElegy
#define Flags_GetRandoInf(flag) moonsTear
#define RANDO_EVENTS events
#define BETWEEN(start, end) ((start) == TIME_DAY1_PM_03_00 ? dayOneWindow : dayTwoWindow)
#define COMBO_MM_TRICK(name) CVarGetInteger("gCombo.Tricks.MM." name, 0)
#include "MmTrickProductionMacros.h"

static std::string Source(const char* file) {
    std::ifstream input(std::string(COMBO_MM_LOGIC_DIR) + "/Regions/" + file);
    assert(input.is_open());
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}
static std::string Region(const std::string& source, const std::string& id) {
    const auto start = source.find("Regions[" + id + "]");
    assert(start != std::string::npos);
    const auto end = source.find("Regions[", start + 8);
    return source.substr(start, end == std::string::npos ? source.size() - start : end - start);
}
static void Has(const std::string& source, const std::string& expression) {
    assert(source.find(expression) != std::string::npos);
}
static void SetItem(int item, bool owned) {
    if (owned) items.insert(item); else items.erase(item);
}
static void Enable(int id, bool enabled) {
    const auto* trick = ComboRando::FindMmTrick(id);
    assert(trick);
    cvars[trick->key] = int(enabled);
}

int main() {
    using namespace ComboRando;
    // Preserve the exact legacy prefix; only the reviewed hidden appendages may follow.
    assert(std::size(kMmTricks) >= 16 && std::size(kMmTricks) <= 17);
    const char* legacyKeys[] = {"Lensless", "DarmaniWall", "PinnacleNoSeahorse", "PostOfficeTiming", "OceansideCode", "ClockTownLowerJump", "DoggyFenceJump", "PalaceBeanSkip", "IkanaIceSkip", "StoneTowerOneMask", "DekuStickFighting", "TwinmoldBow", "TwinmoldFireIce", "RanchFarore", "GbtCentralGeyser"};
    const char* legacyNames[] = {"Fewer Lens Requirements", "Climb to Darmani without the Lens", "Pinnacle Rock without the Seahorse", "Post Office timing without Bunny Hood", "Guess Oceanside Spider House code", "South Clock Town lower chest with a precise jump", "Doggy Racetrack chest with a fence jump", "Skip planting beans in Deku Palace", "Climb Ikana Canyon without Ice Arrows", "Climb Stone Tower with one transformation mask", "Fight enemies with Deku Sticks", "Defeat Twinmold with the Bow", "Defeat Twinmold with Fire and Ice Arrows", "Ranch events with Farore's Wind", "Great Bay central geyser"};
    for (int id=0;id<15;++id) {
        const auto* row=FindMmTrick(id);
        assert(row && std::string(row->key)==std::string("gCombo.Tricks.MM.")+legacyKeys[id]);
        assert(std::string(row->name)==legacyNames[id] && row->publicOption==(id<13));
    }
    const char* appendedKeys[] = {"gCombo.Tricks.MM.LullabySkipIrons", "gCombo.Tricks.MM.ZoraHallScrubNoDeku"};
    for (int id=15;id<int(std::size(kMmTricks));++id) {
        assert(!FindMmTrick(id)->publicOption && std::string(FindMmTrick(id)->key)==appendedKeys[id-15]);
    }
    assert(std::string(kMmTricks[0].key) == "gCombo.Tricks.MM.Lensless");
    assert(std::string(kMmTricks[1].key) == "gCombo.Tricks.MM.DarmaniWall");
    std::set<std::string> keys;
    for (int id = 0; id < int(std::size(kMmTricks)); ++id) {
        const auto* row = FindMmTrick(id);
        assert(row && row->id == id && keys.insert(row->key).second);
        assert(row->name[0] && row->description[0] && row->area[0] && row->tags[0]);
        assert(row->difficulty >= 1 && row->difficulty <= 5);
        assert(CVarGetInteger(row->key, 0) == 0); // no implicit new assumptions
    }
    assert(!FindMmTrick(-1) && !FindMmTrick(int(std::size(kMmTricks))));
    assert(std::string(FindMmTrick(10)->key) == "gCombo.Tricks.MM.DekuStickFighting");
    assert(std::string(FindMmTrick(11)->key) == "gCombo.Tricks.MM.TwinmoldBow");
    assert(std::string(FindMmTrick(12)->key) == "gCombo.Tricks.MM.TwinmoldFireIce");
    assert(std::string(FindMmTrick(13)->key) == "gCombo.Tricks.MM.RanchFarore");
    assert(std::string(FindMmTrick(14)->key) == "gCombo.Tricks.MM.GbtCentralGeyser");
    for (int id = 0; id < 15; ++id) assert(FindMmTrick(id)->publicOption == (id < 13));
    unsigned cases = 0;
    for (bool enabled : {false, true}) {
        for (int id = 2; id < int(std::size(kMmTricks)); ++id) Enable(id, enabled);
        for (bool form : {false, true}) for (bool ability : {false, true})
        for (bool seahorse : {false, true}) {
            zora = form; swim = ability; events[RE_ACCESS_SEAHORSE] = seahorse;
            assert(CAN_CROSS_PINNACLE_ROCK == (form && ability && (seahorse || enabled)));
            ++cases;
        }
        for (bool hood : {false, true}) for (bool day1 : {false, true})
        for (bool letter : {false, true}) for (bool day2 : {false, true}) {
            SetItem(ITEM_MASK_BUNNY, hood); dayOneWindow = day1; dayTwoWindow = day2;
            events[RE_MAIL_LETTER_TO_KAFEI] = letter;
            assert(CAN_WIN_POST_OFFICE == ((hood || enabled) && (day1 || (letter && day2))));
            ++cases;
        }
        for (bool bow : {false, true}) for (bool captain : {false, true}) {
            SetItem(ITEM_BOW, bow); SetItem(ITEM_MASK_CAPTAIN, captain);
            assert(CAN_OPEN_OCEANSIDE_CODE_CHEST == (bow && (captain || enabled)));
            ++cases;
        }
        for (bool form : {false, true}) for (bool tear : {false, true})
        for (bool hook : {false, true}) {
            deku = form; moonsTear = tear; SetItem(ITEM_HOOKSHOT, hook);
            assert(CAN_REACH_CLOCK_TOWN_LOWER_CHEST == ((form && tear) || hook || enabled));
            ++cases;
        }
        for (bool hook : {false, true}) for (bool bean : {false, true})
        for (bool form : {false, true}) {
            SetItem(ITEM_HOOKSHOT, hook); usableBean = bean; zora = form;
            assert(CAN_REACH_DOGGY_CHEST == (hook || bean || form || enabled));
            ++cases;
        }
        assert(CAN_SKIP_PALACE_BEANS == enabled);
        ++cases;
        for (bool hook : {false, true}) for (bool ice : {false, true}) {
            SetItem(ITEM_HOOKSHOT, hook); iceArrow = ice;
            assert(CAN_CLIMB_IKANA_RIVERSIDE == (hook && (ice || enabled)));
            ++cases;
        }
        for (bool hook : {false, true}) for (bool song : {false, true})
        for (bool g : {false, true}) for (bool z : {false, true}) for (bool d : {false, true}) {
            SetItem(ITEM_HOOKSHOT, hook); playableElegy = song; goron = g; zora = z; deku = d;
            assert(CAN_CLIMB_STONE_TOWER == (hook && song && ((g && z) || (enabled && (g || z)))));
            ++cases;
        }
    }

    // Prove actual region bindings and protected neighboring conditions. A
    // technique may change only its named check/edge, not the Seahorse reward,
    // upper Clock Town chest, race reward or palace flight/combat.
    const auto central = Source("Central.cpp"), west = Source("West.cpp");
    const auto spiders = Source("SpiderHouses.cpp"), milkRoad = Source("MilkRoad.cpp");
    const auto south = Source("South.cpp"), east = Source("East.cpp");
    Has(Region(central, "RR_CLOCK_TOWN_SOUTH"),
        "CHECK(RC_CLOCK_TOWN_SOUTH_CHEST_LOWER, CAN_REACH_CLOCK_TOWN_LOWER_CHEST)");
    Has(Region(central, "RR_CLOCK_TOWN_SOUTH"),
        "CHECK(RC_CLOCK_TOWN_SOUTH_CHEST_UPPER, ((CAN_BE_DEKU && Flags_GetRandoInf(RANDO_INF_OBTAINED_MOONS_TEAR)) || HAS_ITEM(ITEM_HOOKSHOT)) && FINAL_DAY())");
    Has(Region(central, "RR_POST_OFFICE"), "CHECK(RC_CLOCK_TOWN_WEST_POSTMAN_MINIGAME, CAN_WIN_POST_OFFICE)");
    Has(Region(west, "RR_PINNACLE_ROCK_ENTRANCE"), "CONNECTION(RR_PINNACLE_ROCK_INNER, CAN_CROSS_PINNACLE_ROCK)");
    Has(west, "EVENT(RE_ACCESS_SEAHORSE, RANDO_EVENTS[RE_ACCESS_PIRATE_PICTURE] && HAS_ITEM(ITEM_PICTOGRAPH_BOX) && HAS_BOTTLE)");
    Has(Region(west, "RR_PINNACLE_ROCK_INNER"), "CanKillEnemy(ACTOR_EN_DRAGON) && RANDO_EVENTS[RE_ACCESS_SEAHORSE]");
    Has(spiders, "CHECK(RC_OCEAN_SPIDER_HOUSE_CHEST_PIECE_OF_HEART, CAN_OPEN_OCEANSIDE_CODE_CHEST)");
    Has(Region(milkRoad, "RR_DOGGY_RACETRACK"), "CHECK(RC_DOGGY_RACETRACK_CHEST, CAN_REACH_DOGGY_CHEST)");
    Has(Region(milkRoad, "RR_DOGGY_RACETRACK"), "CAN_WIN_DOGGY_RACE");
    Has(Region(south, "RR_DEKU_PALACE_INSIDE_LOWER"),
        "CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE, CAN_SKIP_PALACE_BEANS)");
    Has(Region(south, "RR_DEKU_PALACE_INSIDE_UPPER_CELL_SIDE"),
        "CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_MIDDLE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU)))");
    Has(Region(south, "RR_DEKU_PALACE_INSIDE_UPPER_BEAN_SIDE"),
        "CONNECTION(RR_DEKU_PALACE_INSIDE_UPPER_MIDDLE, COMBO_WORLD45_SELECT(CanKillEnemy(ACTOR_EN_DEKUNUTS) && CAN_BE_DEKU, HAS_ITEM(ITEM_MASK_DEKU)))");
    Has(south, "#define COMBO_WORLD45_SELECT(legacy,pinned) (COMBO_WORLD45_ENABLED ? (pinned) : (legacy))");
    Has(Region(east, "RR_IKANA_CANYON_LOWER"), "CONNECTION(RR_IKANA_CANYON_UPPER, CAN_CLIMB_IKANA_RIVERSIDE)");
    Has(Region(east, "RR_STONE_TOWER_BOTTOM"), "CONNECTION(RR_STONE_TOWER_MIDDLE, CAN_CLIMB_STONE_TOWER)");
    Has(Region(east, "RR_STONE_TOWER_MIDDLE"), "CONNECTION(RR_STONE_TOWER_BOTTOM, true)");
    Has(Region(east, "RR_STONE_TOWER_TOP"),
        "CONNECTION(RR_STONE_TOWER_MIDDLE, CAN_BE_GORON || (CAN_PLAY_SONG(ELEGY) && CAN_BE_ZORA))");
    Has(Region(east, "RR_STONE_TOWER_MIDDLE"), "CONNECTION(RR_STONE_TOWER_UPPER, HAS_ITEM(ITEM_HOOKSHOT))");

    // Every new assumption goes through the real snapshot migration/apply
    // functions. Old slots clear new techniques even after another slot used
    // them; per-run edits must not mutate original settings or placements.
    const auto read = [](const char* key, int fallback) { return CVarGetInteger(key, fallback); };
    const auto write = [](const char* key, int value) { cvars[key] = value; };
    for (int id = 0; id < int(std::size(kMmTricks)); ++id) Enable(id, true);
    const auto rules = CaptureGameplayRules(read);
    const nlohmann::json seed = {{"gameplayRules", rules}, {"placements", {{"fixed-check", "fixed-item"}}}};
    const auto original = seed.dump();
    auto run = rules;
    for (int id = 2; id < int(std::size(kMmTricks)); ++id) run["mmTricks"][FindMmTrick(id)->key] = 0;
    ApplyGameplayRules(GameplayRulesForRun(seed, run), write);
    for (int id = 2; id < int(std::size(kMmTricks)); ++id) assert(CVarGetInteger(FindMmTrick(id)->key, 9) == 0);
    assert(seed.dump() == original);
    ApplyGameplayRules(GameplayRulesForRun(seed, nlohmann::json()), write);
    for (const auto& row : kMmTricks) assert(CVarGetInteger(row.key, 0) == 1);
    auto oldSeed = seed;
    for (int id = 2; id < int(std::size(kMmTricks)); ++id) oldSeed["gameplayRules"]["mmTricks"].erase(FindMmTrick(id)->key);
    ApplyGameplayRules(GameplayRulesForRun(oldSeed, nlohmann::json()), write);
    for (int id = 2; id < int(std::size(kMmTricks)); ++id) assert(CVarGetInteger(FindMmTrick(id)->key, 9) == 0);
    assert(CVarGetInteger(kMmTricks[0].key, 0) == 1 && CVarGetInteger(kMmTricks[1].key, 0) == 1);
    for (int id = 2; id < int(std::size(kMmTricks)); ++id) {
        auto corrupt = rules;
        corrupt["mmTricks"][FindMmTrick(id)->key] = "1";
        ApplyGameplayRules(NormalizeGameplayRuleSnapshot(corrupt), write);
        assert(CVarGetInteger(FindMmTrick(id)->key, 9) == 0);
    }
    assert(seed.dump() == original);
    std::cout << "PASS: " << cases << " production MM trick macro cases, region bindings, neighboring gates, stable IDs and immutable seed/run snapshots\n";
}
