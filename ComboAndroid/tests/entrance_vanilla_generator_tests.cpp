// Both-game generator and full host-fill differential. Native engines/oracles
// are explicit fakes; this proves the production algorithm/transactions, not a
// native seed corpus or gameplay.
#include "rando/entrances/UnifiedBridge.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace ComboRando;
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
static CapabilitySet Caps() {
  auto c = CapabilitySet::AtLeast(Stage::Hidden);
  c.devBuild = true;
  return c;
}
template <class F> static void Reject(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const std::exception &) {
    rejected = true;
  }
  assert(rejected);
}
struct Fake {
  static inline OotQuests quests{};
  static inline Json contextOverride;
  static inline Json baseMm, baseOot, baseTemple;
  static inline std::string mmDump, ootDump, contextDump, checks;
  static inline int applies = 0, assumptions = 0, generations = 0, prepares = 0,
                    queries = 0, releases = 0;
  static inline int settingsEnemy = 0, settingsMq = 0;
  static inline uint8_t startAges = 2;
  static inline bool restricted = false, changeQuestOnReset = false,
                     throwOnComplete = false, prepared = false;
  static inline bool rejectEmptyTemple = false;
  static void Reset() {
    ActiveMmUnified().clear();
    ActiveOotUnified().clear();
    ActiveMmUnifiedAssumptions().reset();
    ActiveOotUnifiedAssumptions().reset();
    ActiveMmGenerationSettings().reset();
    quests = {};
    contextOverride = Json();
    baseMm = baseOot = baseTemple = Json();
    applies = assumptions = generations = prepares = queries = releases = 0;
    settingsEnemy = settingsMq = 0;
    restricted = changeQuestOnReset = throwOnComplete = prepared = false;
    rejectEmptyTemple = false;
  }
  static int ApplyMm(const char *text) {
    ++applies;
    ActiveMmUnified().clear();
    ActiveMmUnifiedAssumptions().reset();
    const auto j = Json::parse(text);
    if (j.contains("entranceLayout"))
      ActiveMmUnified().install(j, Caps());
    return 1;
  }
  static int ApplyOot(const char *text) {
    ++applies;
    ActiveOotUnified().clear();
    ActiveOotUnifiedAssumptions().reset();
    const auto j = Json::parse(text);
    if (j.contains("entranceLayout")) {
      RequireVanillaOotQuests(quests);
      ActiveOotUnified().install(j, Caps());
      assert(*ActiveOotUnified().quests() == quests);
    }
    return 1;
  }
  static const char *DumpMm() {
    mmDump = (ActiveMmUnified().active()
                  ? Json{{"entranceLayout", ActiveMmUnified().json()}}
                  : Json::object())
                 .dump();
    return mmDump.c_str();
  }
  static const char *DumpOot() {
    ootDump = (ActiveOotUnified().active()
                   ? Json{{"entranceLayout", ActiveOotUnified().json()}}
                   : Json::object())
                  .dump();
    return ootDump.c_str();
  }
  static const char *Context() {
    auto mq = Json::object();
    for (size_t i = 0; i < quests.size(); ++i)
      mq[std::string(kOotMqDungeons[i])] = quests[i];
    auto j = Json{{"profile", "ootmm-oot-area-context-v1"},
                  {"registry", kRegistrySourceSha256},
                  {"ootMq", mq}};
    contextDump = (contextOverride.is_null() ? j : contextOverride).dump();
    return contextDump.c_str();
  }
  static int AssumeMm(const char *text) {
    ++assumptions;
    const auto payload = Json::parse(text);
    if (payload.at("fixed").empty() && payload.at("remaining").empty())
      baseMm = payload;
    ActiveMmUnifiedAssumptions() =
        DecodeMmUnifiedAssumptions(Json::parse(text));
    return 1;
  }
  static int AssumeOot(const char *text) {
    ++assumptions;
    const auto payload = Json::parse(text);
    if (payload.at("fixed").empty() && payload.at("remaining").empty())
      baseOot = payload;
    ActiveOotUnifiedAssumptions() =
        DecodeOotUnifiedAssumptions(Json::parse(text), quests);
    return 1;
  }
  static void ClearMm() { ActiveMmUnifiedAssumptions().reset(); }
  static void ClearOot() { ActiveOotUnifiedAssumptions().reset(); }
  static int Generation(const char *text) {
    ++generations;
    if (!text) {
      ActiveMmGenerationSettings().reset();
      return 1;
    }
    const auto layout =
        DecodeEntranceLayout(Json::parse(text), LayoutPart::SettingsOnly);
    ValidateEntranceLayoutSettings(layout, Caps());
    ActiveMmGenerationSettings() = layout.settings;
    return 1;
  }
  static NativeFns Native() {
    return {ApplyMm,  AssumeMm,  ClearMm,  DumpMm,  Generation,
            ApplyOot, AssumeOot, ClearOot, DumpOot, Context};
  }
  static void OracleReset() {
    if (changeQuestOnReset)
      quests[0] = true;
  }
  static void Owned(const char *) {}
  static void Place(const char *, const char *) {}
  static uint8_t Portal() { return 1; }
  static uint8_t Age() { return startAges; }
  static bool OotRoute() {
    const auto routes = OotUnifiedGraphRoutes(Find("OOT_WALLMASTER_GTG")->id);
    return std::any_of(routes.begin(), routes.end(), [](const auto &r) {
      return r.destination == Find("OOT_DEKU_TREE")->id;
    });
  }
  static bool MmRoute() {
    const auto routes = MmUnifiedGraphRoutes(Find("MM_WALLMASTER_DAMPE")->id);
    return std::any_of(routes.begin(), routes.end(), [](const auto &r) {
      return r.destination.id == Find("MM_TEMPLE_WOODFALL")->id;
    });
  }
  static const char *Checks(bool mm) {
    if (throwOnComplete && ActiveOotUnified().active() &&
        !ActiveOotUnifiedAssumptions())
      throw std::runtime_error("final validation must run");
    Json names = mm ? Json{"Mm start"} : Json{"Oot start"};
    if (!restricted || (mm ? MmRoute() : OotRoute())) {
      names.push_back(mm ? "Mm prize" : "Oot prize");
      names.push_back(mm ? "Defeat Majora" : "Ganon");
    }
    checks = names.dump();
    return checks.c_str();
  }
  static const char *OotChecks() { return Checks(false); }
  static const char *MmChecks() { return Checks(true); }
  static OracleFns Oot() {
    OracleFns f{OracleReset, Owned, OotChecks, Place, Portal};
    f.GetStartAges = Age;
    return f;
  }
  static OracleFns Mm() { return {OracleReset, Owned, MmChecks, Place}; }
  static int Prepare(const char *) {
    ++prepares;
    prepared = true;
    return 0;
  }
  static int Query(const char *text, int (*poll)(), uint8_t *ages) {
    assert(prepared);
    ++queries;
    if (poll && poll())
      return -3;
    const auto payload = Json::parse(text);
    if (payload.at("edges").empty() && payload.at("mm").at("edges").empty()) {
      baseTemple = payload;
      if (rejectEmptyTemple)
        return 0;
    }
    *ages = startAges == 1 ? 2 : 1;
    return 1;
  }
  static void Release() {
    prepared = false;
    ++releases;
  }
  static const char *Error() { return "fake Temple"; }
  static TempleProbeFns Probe() {
    return {Prepare, Query, Error, Release, nullptr};
  }
};
static std::string Dump(bool mm) {
  const std::string prefix = mm ? "Mm" : "Oot";
  return Json{
      {"checks",
       {{{"name", prefix + " start"}}, {{"name", prefix + " prize"}}}},
      {"pool",
       {{{"name", "Filler"}, {"category", "junk"}, {"advancement", false}},
        {{"name", "Filler"}, {"category", "junk"}, {"advancement", false}}}},
      {"sharedStartingItems", Json::array()}}
      .dump();
}
static UnifiedGeneration Generation(uint32_t seed = 1) {
  UnifiedGeneration g;
  g.settings.erWallmasters = Mode::OwnGame;
  g.capability = kVanillaWallmasterCapability;
  g.active = Caps();
  g.seed = seed;
  g.oot = Fake::Oot();
  g.mm = Fake::Mm();
  g.ootDump = Dump(false);
  g.mmDump = Dump(true);
  g.readSetting = [](const char *key, int fallback) {
    if (std::string_view(key) == "gEnhancements.RandomizedEnemies")
      return Fake::settingsEnemy;
    if (std::string_view(key) == kMqDungeonsSetting)
      return Fake::settingsMq;
    return fallback;
  };
  return g;
}
static void Untouched() {
  assert(Fake::applies == 0 && Fake::assumptions == 0 &&
         Fake::generations == 0 && Fake::prepares == 0);
}
struct Result {
  std::string completeSeed;
  Json layout;
  UnifiedEntranceStats stats;
  int temple;
};
static Result Run(uint32_t seed, uint8_t age, bool mmStart, OotAccess access,
                  bool shortcut, bool restricted = false,
                  bool templeBaseFalse = false) {
  Fake::Reset();
  Fake::startAges = age;
  Fake::restricted = restricted;
  Fake::rejectEmptyTemple = templeBaseFalse;
  auto g = Generation(seed);
  g.mmStart = mmStart;
  g.access = access;
  g.wallmasterShortcut = shortcut;
  Result result;
  g.stats = &result.stats;
  result.layout = GenerateEntranceLayout(g, Fake::Native(), Fake::Probe());
  if (result.stats.monotoneShortcutUsed) {
    assert(Fake::baseMm.at("deleted").size() == 4 &&
           Fake::baseOot.at("deleted").size() == 9 &&
           Fake::baseTemple.at("deleted").size() == 9 &&
           Fake::baseTemple.at("mm").at("deleted").size() == 4);
    assert(Fake::baseMm.at("world") == Fake::baseOot.at("world") &&
           Fake::baseOot.at("world") == Fake::baseTemple.at("world"));
    for (const auto *payload : {&Fake::baseMm, &Fake::baseOot}) {
      assert(payload->at("fixed").empty() && payload->at("remaining").empty() &&
             payload->at("assumed").empty());
      for (const auto &deleted : payload->at("deleted")) {
        const auto *row = Find(deleted.at("row").get<std::string>());
        assert(row && row->type == Type::Wallmaster && IsVoidArea(row->to) &&
               deleted.at("void") == true);
      }
    }
  }
  assert(!ActiveOotUnified().active() && !ActiveMmUnified().active());
  assert(!ActiveOotUnifiedAssumptions() && !ActiveMmUnifiedAssumptions() &&
         !ActiveMmGenerationSettings());
  assert(!Fake::prepared &&
         (access == OotAccess::NO_LOGIC || Fake::releases == 1));
  assert(result.layout.at("overrides").size() == 13 &&
         result.stats.completeChecks == 1);
  result.temple = Fake::queries;
  ApplyUnifiedLayout(Fake::Native(), result.layout, Caps());
  auto filled =
      CrossWorldCombinedFill(g.ootDump, g.mmDump, seed, g.oot, g.mm, nullptr,
                             "", access, g.goal, mmStart ? GAME_MM : GAME_OOT);
  assert(filled.success);
  auto complete = Json::parse(filled.spoilerJson);
  complete["entranceLayout"] = result.layout;
  complete["comboGoal"] = CwGoalJson(g.goal);
  complete["startingGame"] = mmStart ? "mm" : "oot";
  StampComboSeedFormat(complete);
  result.completeSeed = complete.dump();
  ApplyNativeSeed(Fake::Native(), Json::object(), Caps());
  return result;
}
static void CheckShortcutPoolGuards() {
  for (int variant = 0; variant < 4; ++variant) {
    Settings s;
    s.erWallmasters = variant == 1   ? Mode::None
                      : variant == 3 ? Mode::Full
                                     : Mode::OwnGame;
    if (variant == 1 || variant == 2)
      s.erOneWays = Mode::OwnGame;
    if (variant == 1)
      s.erOneWaysOwls = true;
    int assumptions = 0, temple = 0, complete = 0;
    Bindings b;
    b.areaExists = [](std::string_view area) {
      return LayoutV1::AreaExists(area, {}, false);
    };
    b.sourceSupported =
        b.destinationSupported = [](const Record &) { return true; };
    b.validateAssumed = [&](const Overrides &, const Pools &) {
      ++assumptions;
      return true;
    };
    b.validateAgeTemple = [&](const Overrides &, const Pools &) {
      ++temple;
      return true;
    };
    b.validateComplete = [&](const Overrides &) {
      ++complete;
      return true;
    };
    b.chooseIndex = [](size_t) { return size_t(0); };
    b.monotoneWallmasterBaseProven = true;
    assert(!Shuffler(s, b).run().empty() && complete == 1);
    if (variant == 0)
      assert(assumptions == 0 && temple == 0);
    else
      assert(assumptions > 0 && temple > 0);
  }
}
int main(int argc, char **argv) {
  assert(argc == 2);
  Fake::Reset();
  auto g = Generation();
  g.settings = {};
  assert(GenerateEntranceLayout(g, {}, {}).is_null());
  Untouched();
  // Public still refuses before any native state changes. Hidden tests opt in
  // by exact id.
  g = Generation();
  g.active = CapabilitySet::AtLeast(Stage::Public);
  Reject([&] { GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()); });
  Untouched();
  for (unsigned mask = 1; mask < 4096; ++mask) {
    Fake::Reset();
    for (size_t i = 0; i < Fake::quests.size(); ++i)
      Fake::quests[i] = (mask & (1u << i)) != 0;
    UnifiedEntranceStats stats;
    g = Generation();
    g.stats = &stats;
    Reject([&] { GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()); });
    Untouched();
    assert(stats.randomDraws == 0);
  }
  Fake::Reset();
  const auto validContext = Json::parse(Fake::Context());
  for (int bad = 0; bad < 7; ++bad) {
    Fake::Reset();
    Fake::contextOverride = validContext;
    switch (bad) {
    case 0:
      Fake::contextOverride.erase("profile");
      break;
    case 1:
      Fake::contextOverride["profile"] = 0;
      break;
    case 2:
      Fake::contextOverride["registry"] = "wrong";
      break;
    case 3:
      Fake::contextOverride["ootMq"].erase(std::string(kOotMqDungeons[0]));
      break;
    case 4:
      Fake::contextOverride["ootMq"][std::string(kOotMqDungeons[0])] = 0;
      break;
    case 5:
      Fake::contextOverride["extra"] = false;
      break;
    case 6:
      Fake::contextOverride = Json::array();
      break;
    }
    g = Generation();
    Reject([&] { GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()); });
    Untouched();
  }
  Fake::Reset();
  Fake::changeQuestOnReset = true;
  g = Generation();
  Reject([&] { GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()); });
  Untouched();
  Fake::Reset();
  Fake::settingsEnemy = 1;
  g = Generation();
  Reject([&] { GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()); });
  Untouched();
  for (int field = 0; field < 10; ++field) {
    Fake::Reset();
    g = Generation();
    auto missing = Fake::Native();
    switch (field) {
    case 0:
      missing.applySeed = nullptr;
      break;
    case 1:
      missing.setAssumptions = nullptr;
      break;
    case 2:
      missing.clearAssumptions = nullptr;
      break;
    case 3:
      missing.dumpSeed = nullptr;
      break;
    case 4:
      missing.setGenerationSettings = nullptr;
      break;
    case 5:
      missing.applyOotLayout = nullptr;
      break;
    case 6:
      missing.setOotAssumptions = nullptr;
      break;
    case 7:
      missing.clearOotAssumptions = nullptr;
      break;
    case 8:
      missing.dumpOotLayout = nullptr;
      break;
    case 9:
      missing.ootAreaContext = nullptr;
      break;
    }
    Reject([&] { GenerateEntranceLayout(g, missing, Fake::Probe()); });
    Untouched();
  }
  // The authoritative native quest context wins over an unapplied settings
  // CVar.
  Fake::Reset();
  Fake::settingsMq = 1;
  g = Generation();
  g.access = OotAccess::NO_LOGIC;
  const auto actual = DecodeEntranceLayout(
      GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()));
  assert(actual.ootMq == OotQuests{});
  CheckShortcutPoolGuards();
  std::ostringstream quiet;
  auto *previousOutput = std::cout.rdbuf(quiet.rdbuf());
  size_t pairs = 0, shortcuts = 0;
  uint64_t corpus = 14695981039346656037ull;
  for (uint32_t seed = 1; seed <= 200; ++seed)
    for (uint8_t age : {uint8_t(1), uint8_t(2)})
      for (bool mmStart : {false, true})
        for (auto access : {OotAccess::ALL_REACHABLE, OotAccess::BEATABLE_ONLY,
                            OotAccess::NO_LOGIC}) {
          const auto full = Run(seed, age, mmStart, access, false);
          const auto fast = Run(seed, age, mmStart, access, true);
          assert(full.completeSeed == fast.completeSeed &&
                 full.layout == fast.layout);
          assert(full.stats.randomDraws == fast.stats.randomDraws &&
                 fast.stats.randomDraws == 39);
          assert(!full.stats.monotoneShortcutUsed);
          if (access != OotAccess::NO_LOGIC) {
            assert(fast.stats.monotoneShortcutUsed &&
                   fast.stats.assumedChecks == 1 && fast.temple == 1);
            assert(full.stats.assumedChecks == 13 && full.temple == 13);
            ++shortcuts;
          } else
            assert(!fast.stats.monotoneShortcutUsed && fast.temple == 0);
          for (unsigned char byte : fast.completeSeed)
            corpus = (corpus ^ byte) * 1099511628211ull;
          ++pairs;
          quiet.str("");
          quiet.clear();
        }
  for (uint32_t seed = 1; seed <= 20; ++seed) {
    const auto full =
        Run(seed, 2, false, OotAccess::ALL_REACHABLE, false, true);
    const auto fallback =
        Run(seed, 2, false, OotAccess::ALL_REACHABLE, true, true);
    assert(full.completeSeed == fallback.completeSeed &&
           full.stats.randomDraws == fallback.stats.randomDraws);
    assert(!fallback.stats.monotoneShortcutUsed &&
           fallback.stats.assumedChecks > full.stats.assumedChecks);
    quiet.str("");
    quiet.clear();
    const auto templeFull =
        Run(seed, 1, true, OotAccess::BEATABLE_ONLY, false, false, true);
    const auto templeFallback =
        Run(seed, 1, true, OotAccess::BEATABLE_ONLY, true, false, true);
    assert(templeFull.completeSeed == templeFallback.completeSeed &&
           templeFull.stats.randomDraws == templeFallback.stats.randomDraws);
    assert(!templeFallback.stats.monotoneShortcutUsed &&
           templeFallback.temple == templeFull.temple + 1);
    quiet.str("");
    quiet.clear();
  }
  Fake::Reset();
  g = Generation();
  UnifiedEntranceStats stats;
  g.stats = &stats;
  Fake::throwOnComplete = true;
  Reject([&] { GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()); });
  assert(stats.monotoneShortcutUsed && stats.completeChecks == 1 &&
         stats.randomDraws == 39);
  assert(!ActiveMmUnified().active() && !ActiveOotUnified().active() &&
         !Fake::prepared);
  assert(!ActiveMmGenerationSettings() && !ActiveOotUnifiedAssumptions() &&
         !ActiveMmUnifiedAssumptions());
  std::cout.rdbuf(previousOutput);
  Json report{{"scope", "Real generator and combined fill with explicit fake "
                        "native modules/oracles"},
              {"differentialPairs", pairs},
              {"shortcutPairs", shortcuts},
              {"fallbackPairs", 20},
              {"templeFallbackPairs", 20},
              {"nativeQuestContextsRejected", 4095},
              {"corpusFnv64", corpus},
              {"nativeSeedCorpusOrGameplay", false}};
  std::ofstream(argv[1]) << report.dump(2) << '\n';
  std::cout << report.dump() << '\n';
}
