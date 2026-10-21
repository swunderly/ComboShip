// Both-game generator and full host-fill differential. Native engines/oracles
// are explicit fakes; this proves the production algorithm/transactions, not a
// native seed corpus or gameplay. The vanilla id named explicitly refuses every
// Master Quest context; chosen by settings in a build with the Master Quest id
// (U11), every all-vanilla seed is byte for byte the vanilla id's, and every
// Master Quest context of OoTMM's bundled data generates under the quests id
// except the capacity shortfalls of tools/ootmm/m1-contract.json, which are
// refused before any draw. Legacy Master Quest data is refused with either id.
// The quest context read before the oracle's first reset must be the one after
// it, for either id. Run with a third argument, "songs", it checks foreign-song
// logic instead (limits design, Stage 1): the songs id in every quest context,
// and the shortcut differential with the songs on; with "warp", a starting OoT
// warp song (Stage 2): the songs-v2 id in every quest context, and the shortcut
// differential with the song.
#include "rando/entrances/UnifiedBridge.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <tuple>
#include <type_traits>
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
  // The quests the oracle's first reset finalizes, when they differ from the
  // dump's (a native roll on the shared stream).
  static inline std::optional<OotQuests> questsOnReset;
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
    questsOnReset.reset();
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
      // The installer binds the quests the layout's id admits
      // (combo_entrance_overlay.cpp SetLayout).
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
        DecodeOotUnifiedAssumptions(Json::parse(text), quests, Caps());
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
    if (questsOnReset)
      quests = *std::exchange(questsOnReset, std::nullopt);
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
// An OoT dump whose native starting inventory holds an OoT warp song.
static std::string WarpDump(const std::string &song = "Prelude of Light") {
  auto dump = Json::parse(Dump(false));
  dump["sharedStartingItems"] = {song};
  return dump.dump();
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
  // A new seed on an install with OoTMM's bundled MQ data (MqMode.h).
  g.mqSource = ComboMq::SeedMqSource::Bundled;
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
                  bool templeBaseFalse = false,
                  const CapabilitySet *chosenBySettings = nullptr,
                  bool foreignSongs = false, const std::string &warpSong = "") {
  Fake::Reset();
  Fake::startAges = age;
  Fake::restricted = restricted;
  Fake::rejectEmptyTemple = templeBaseFalse;
  auto g = Generation(seed);
  if (chosenBySettings) { // in-game: no id named, this set's own choice
    g.capability.clear();
    g.active = *chosenBySettings;
  }
  g.foreignSongsLogic = foreignSongs;
  if (!warpSong.empty())
    g.ootDump = WarpDump(warpSong);
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
// A refusal is thrown before any native state changes.
static UnifiedEntranceRefused
Refusal(const UnifiedGeneration &g, const NativeFns &native = Fake::Native()) {
  try {
    GenerateEntranceLayout(g, native, Fake::Probe());
  } catch (const UnifiedEntranceRefused &error) {
    Untouched();
    return error;
  }
  assert(!"a refusal was expected");
  std::abort();
}
// The generation error shows the plain sentence the menu shows for the same
// reason (GenerationRules, owner decision D3); what() keeps the developer text.
static void CheckPlainRefusals() {
  const auto same = [](const UnifiedGeneration &g,
                       const EntranceSettingsFacts &facts) {
    Fake::Reset();
    const auto error = Refusal(g);
    const auto rules = EntranceGenerationRefusal(facts);
    assert(!rules.empty() && error.player() == rules.player &&
           error.what() == rules.log);
  };
  const auto reading = [](std::string key, int value) {
    return [key, value](const char *read, int fallback) {
      return read == key ? value : fallback;
    };
  };
  UnifiedGeneration g;
  EntranceSettingsFacts facts;
  for (const char *key : kShipEntranceSettings) {
    g = Generation();
    g.readSetting = reading(key, 1);
    facts = {};
    facts.shipwrightEntrances = true;
    same(g, facts);
  }
  g = Generation();
  g.readSetting = reading("gEnhancements.RandomizedEnemies", 1);
  facts = {};
  facts.enemyRandomizer = true;
  same(g, facts);
  g = Generation();
  g.songTravelLogic = true;
  facts = {};
  facts.songTravelLogic = true;
  same(g, facts);
  // Foreign-song logic has no sentence: every build has
  // wallmasters-own-game-songs-v1. A capability set without it (no build has
  // one) refuses it before any draw or native change, as an internal fault.
  Fake::Reset();
  g = Generation();
  g.capability.clear();
  g.active =
      CapabilitySet::AtLeast(Stage::Hidden).Only(kVanillaWallmasterCapability) |
      CapabilitySet::AtLeast(Stage::Hidden).Only(kQuestsWallmasterCapability);
  g.active.devBuild = true;
  g.foreignSongsLogic = true;
  facts = {};
  facts.foreignSongsLogic = true;
  assert(EntranceGenerationRefusal(facts).empty());
  {
    const auto error = Refusal(g);
    assert(error.what() == std::string(kVanillaWallmasterCapability) +
                               " does not admit foreign-song logic" &&
           error.player() == error.what());
  }
  // With the songs id, an id named by name must match the seed's song facts,
  // before any draw or native change; this is developer text, not a sentence.
  for (const auto &[id, foreign, why] :
       {std::tuple{kVanillaWallmasterCapability, true,
                   " does not admit foreign-song logic"},
        std::tuple{kQuestsWallmasterCapability, true,
                   " does not admit foreign-song logic"},
        std::tuple{
            kSongsWallmasterCapability, false,
            " needs foreign-song logic, which this seed does not have"}}) {
    Fake::Reset();
    if (id == kQuestsWallmasterCapability)
      Fake::quests[OotMqIndex("Forest")] = true;
    g = Generation();
    g.capability = std::string(id);
    g.foreignSongsLogic = foreign;
    const auto error = Refusal(g);
    assert(error.what() == std::string(id) + why &&
           error.player() == error.what());
  }
  // No Logic: the songs id has no song facts to carry.
  Fake::Reset();
  g = Generation();
  g.capability = std::string(kSongsWallmasterCapability);
  g.foreignSongsLogic = true;
  g.access = OotAccess::NO_LOGIC;
  assert(Refusal(g).what() ==
         std::string(kSongsWallmasterCapability) +
             " needs foreign-song logic, which this seed does not have");
  // A starting Song of Soaring, in either game's list, is refused by every id.
  for (bool mm : {false, true}) {
    g = Generation();
    auto dump = Json::parse(mm ? g.mmDump : g.ootDump);
    dump["sharedStartingItems"] = {"Song of Soaring"};
    (mm ? g.mmDump : g.ootDump) = dump.dump();
    facts = {};
    (mm ? facts.mmStartingItems : facts.ootStartingItems) = {"Song of Soaring"};
    same(g, facts);
  }
  // A starting OoT warp song (limits design, Stage 2) has no sentence: every
  // build has wallmasters-own-game-songs-v2. A set without it (no build has
  // one) refuses the song before any draw or native change, as an internal
  // fault.
  const auto withoutStartingSongs =
      CapabilitySet::AtLeast(Stage::Hidden).Only(kVanillaWallmasterCapability) |
      CapabilitySet::AtLeast(Stage::Hidden).Only(kQuestsWallmasterCapability) |
      CapabilitySet::AtLeast(Stage::Hidden).Only(kSongsWallmasterCapability);
  for (const bool mm : {false, true}) {
    Fake::Reset();
    g = Generation();
    g.capability.clear();
    g.active = withoutStartingSongs;
    g.active.devBuild = true;
    auto dump = Json::parse(mm ? g.mmDump : g.ootDump);
    dump["sharedStartingItems"] = {"Nocturne of Shadow"};
    (mm ? g.mmDump : g.ootDump) = dump.dump();
    facts = {};
    (mm ? facts.mmStartingItems
        : facts.ootStartingItems) = {"Nocturne of Shadow"};
    assert(EntranceGenerationRefusal(facts).empty());
    const auto error = Refusal(g);
    assert(error.what() == std::string(kVanillaWallmasterCapability) +
                               " does not admit a starting warp song" &&
           error.player() == error.what());
  }
  // With the songs-v2 id an id named by name must match the starting song,
  // before any draw or native change (developer text). No Logic has none.
  for (const auto &[id, warp, foreign, why] :
       {std::tuple{kVanillaWallmasterCapability, true, false,
                   " does not admit a starting warp song"},
        std::tuple{kQuestsWallmasterCapability, true, false,
                   " does not admit a starting warp song"},
        std::tuple{kSongsWallmasterCapability, true, true,
                   " does not admit a starting warp song"},
        std::tuple{
            kStartingSongsWallmasterCapability, false, false,
            " needs a starting warp song, which this seed does not have"},
        std::tuple{kStartingSongsWallmasterCapability, false, true,
                   " needs a starting warp song, which this seed does not "
                   "have"}}) {
    Fake::Reset();
    if (id == kQuestsWallmasterCapability)
      Fake::quests[OotMqIndex("Forest")] = true;
    g = Generation();
    g.capability = std::string(id);
    g.foreignSongsLogic = foreign;
    if (warp)
      g.ootDump = WarpDump();
    const auto error = Refusal(g);
    assert(error.what() == std::string(id) + why &&
           error.player() == error.what());
  }
  Fake::Reset();
  g = Generation();
  g.capability = std::string(kStartingSongsWallmasterCapability);
  g.ootDump = WarpDump();
  g.access = OotAccess::NO_LOGIC;
  assert(Refusal(g).what() ==
         std::string(kStartingSongsWallmasterCapability) +
             " needs a starting warp song, which this seed does not have");
  g = Generation();
  g.readSetting = reading("gRandoSettings.LogicRules", 2);
  facts = {};
  facts.glitchless = false;
  same(g, facts);
  // Master Quest and unreadable dungeon data use the menu's sentences; an
  // internal fault has no plain sentence and shows its developer text.
  Fake::Reset();
  Fake::quests[3] = true;
  assert(Refusal(Generation()).player() == kWallmasterMasterQuestText);
  Fake::Reset();
  Fake::contextOverride = Json::array();
  assert(Refusal(Generation()).player() == kWallmasterDungeonDataText);
  Fake::Reset();
  auto missing = Fake::Native();
  missing.applySeed = nullptr;
  const auto fault = Refusal(Generation(), missing);
  assert(fault.player() == fault.what());
  Fake::Reset();
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
// A capacity shortfall: its own refusal type, so the caller can retry it after
// the seed's own roll; refused before any draw or native change.
static UnifiedEntranceRefused Shortfall(const UnifiedGeneration &g) {
  try {
    GenerateEntranceLayout(g, Fake::Native(), Fake::Probe());
  } catch (const WallmasterCapacityShortfall &error) {
    Untouched();
    return error;
  }
  assert(!"a capacity shortfall was expected");
  std::abort();
}
// Every Master Quest context, the id chosen by settings in a build with the
// Master Quest id: the capacity shortfalls the pinned contract lists are
// refused before any draw or native change, with the sentence for a random
// roll that landed there; every other context generates a quests-id layout of
// its own sources.
static void CheckQuestContexts(const Json &contract) {
  std::set<std::set<std::string>> shortfalls;
  for (const auto &row : contract.at("mqSources").at("capacityShortfalls"))
    shortfalls.insert(row.at("mq").get<std::set<std::string>>());
  const auto dungeons =
      contract.at("mqSources").at("dungeons").get<std::vector<std::string>>();
  assert(shortfalls.size() == 4 && dungeons.size() == 6);
  size_t refused = 0, generated = 0;
  // Co-op's seed identity covers the layout (SeedIdentityText), so two players
  // whose Master Quest dungeons differ never share one.
  std::set<std::string> identities;
  for (unsigned mask = 1; mask < 4096; ++mask) {
    Fake::Reset();
    std::set<std::string> projection;
    for (size_t i = 0; i < Fake::quests.size(); ++i)
      Fake::quests[i] = (mask & (1u << i)) != 0;
    for (const auto &dungeon : dungeons)
      if (Fake::quests[OotMqIndex(dungeon)])
        projection.insert(dungeon);
    UnifiedEntranceStats stats;
    auto g = Generation(mask);
    g.capability.clear();
    g.stats = &stats;
    if (shortfalls.contains(projection)) {
      const auto error = Shortfall(g);
      assert(error.player() == kWallmasterCapacitySeedText &&
             error.what() == std::string("WALLMASTERS has 12 sources for 11 "
                                         "destinations; OoTMM cannot place "
                                         "them") &&
             stats.randomDraws == 0);
      ++refused;
      continue;
    }
    const auto layout = DecodeEntranceLayout(
        GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()));
    assert(layout.required == std::vector<std::string>{std::string(
                                  kQuestsWallmasterCapability)} &&
           layout.ootMq == Fake::quests && stats.randomDraws > 0);
    const auto validated = Validate(layout, Caps());
    for (const auto &route : validated.oot)
      assert(OotUnifiedSourceInQuests(*route.source, Fake::quests));
    auto frozen = layout;
    frozen.required = {std::string(kVanillaWallmasterCapability)};
    Reject([&] { Validate(frozen, Caps()); });
    assert(!ActiveOotUnified().active() && !ActiveMmUnified().active());
    identities.insert(SeedIdentityText("seed", EncodeEntranceLayout(layout)));
    ++generated;
  }
  assert(refused == 256 && generated == 4095 - 256 &&
         identities.size() == generated);
}
// The quest context of the dump's finalize must be the one the oracle's first
// reset finalizes, for either id (0.4.12's check): the layout records the
// dump's, and the oracle, the fill and the seed play the reset's. A bundled
// seed rolls the same dungeons in both; a native roll on the shared stream
// (Legacy data) may not, and is refused before any draw or native change, with
// no plain sentence. Both reads are the gated ones: an unreadable dump context
// is refused even when the reset would make it readable.
static void CheckQuestsAcrossReset() {
  OotQuests spiritMq{}, spiritForestMq{};
  spiritMq[OotMqIndex("Spirit")] = true;
  spiritForestMq = spiritMq;
  spiritForestMq[OotMqIndex("Forest")] = true;
  const std::string changed =
      "OoT quest context changed while preparing entrance generation";
  // The same context on both sides generates, under the id it needs.
  for (const auto &same : {OotQuests{}, spiritMq}) {
    Fake::Reset();
    Fake::quests = same;
    Fake::questsOnReset = same;
    auto g = Generation(3);
    g.capability.clear();
    const auto layout = DecodeEntranceLayout(
        GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()));
    assert(layout.ootMq == same &&
           layout.required ==
               std::vector<std::string>{std::string(
                   AnyMasterQuest(same) ? kQuestsWallmasterCapability
                                        : kVanillaWallmasterCapability)});
  }
  struct Case {
    OotQuests dump, reset;
    std::string_view capability; // empty: chosen by settings
    const char *player;          // nullptr: the developer text
  };
  const Case cases[] = {
      // Chosen by settings: the dump's context sets the id, vanilla or quests.
      {{}, spiritMq, "", nullptr},
      {spiritMq, {}, "", nullptr},
      {spiritMq, spiritForestMq, "", nullptr},
      // The quests id named.
      {spiritMq, {}, kQuestsWallmasterCapability, nullptr},
      {spiritMq, spiritForestMq, kQuestsWallmasterCapability, nullptr},
      // The vanilla id named: the reset's context is refused by its own gate.
      {{}, spiritMq, kVanillaWallmasterCapability, kWallmasterMasterQuestText},
  };
  for (const auto &c : cases) {
    Fake::Reset();
    Fake::quests = c.dump;
    Fake::questsOnReset = c.reset;
    auto g = Generation(3);
    g.capability = std::string(c.capability);
    UnifiedEntranceStats stats;
    g.stats = &stats;
    const auto error = Refusal(g);
    if (c.player)
      assert(error.player() == c.player);
    else
      assert(error.what() == changed && error.player() == changed);
    assert(stats.randomDraws == 0 && !Fake::questsOnReset);
  }
  // An unreadable dump context is refused before the reset, which would have
  // made it readable.
  Fake::Reset();
  Fake::contextOverride = Json::array();
  Fake::questsOnReset = OotQuests{};
  auto g = Generation(3);
  g.oot.Reset = [] {
    Fake::contextOverride = Json();
    Fake::OracleReset();
  };
  assert(Refusal(g).player() == kWallmasterDungeonDataText &&
         Fake::questsOnReset);
}
// Only OoTMM's bundled data admits a Master Quest context. Legacy data (a real
// oot-mq.o2r, whose roll is on the shared stream) and an unknown source keep
// the refusal of 0.4.12, with either id and whatever this build's stages, and
// an all-vanilla seed of either source is the same seed.
static void CheckMqDataSource() {
  size_t refused = 0;
  for (const auto source :
       {ComboMq::SeedMqSource::Legacy, ComboMq::SeedMqSource::Unsupported})
    for (unsigned mask = 1; mask < 4096; ++mask)
      for (const std::string_view capability :
           {std::string_view(), kQuestsWallmasterCapability,
            kVanillaWallmasterCapability}) {
        Fake::Reset();
        for (size_t i = 0; i < Fake::quests.size(); ++i)
          Fake::quests[i] = (mask & (1u << i)) != 0;
        UnifiedEntranceStats stats;
        auto g = Generation(mask);
        g.capability = std::string(capability);
        g.mqSource = source;
        g.stats = &stats;
        const auto error = Refusal(g);
        assert(error.player() == kWallmasterMasterQuestText &&
               stats.randomDraws == 0);
        ++refused;
      }
  assert(refused == 2 * 4095 * 3);
  for (uint32_t seed = 1; seed <= 4; ++seed) {
    std::string layouts[2];
    for (int i = 0; i < 2; ++i) {
      Fake::Reset();
      auto g = Generation(seed);
      g.capability.clear();
      g.mqSource =
          i ? ComboMq::SeedMqSource::Legacy : ComboMq::SeedMqSource::Bundled;
      layouts[i] =
          GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()).dump();
    }
    assert(layouts[0] == layouts[1]);
    assert(DecodeEntranceLayout(Json::parse(layouts[0])).required ==
           std::vector<std::string>{std::string(kVanillaWallmasterCapability)});
  }
}
// The capacity sentences: the generation error of a random roll, and the
// menu's sentence when the MQ settings force the shortfall (every roll lands
// there, EntranceProfile.h MqWallmasterShortfallForced), should generation be
// reached anyway.
static void CheckCapacitySentences() {
  assert(std::string(kWallmasterCapacitySeedText) ==
         "This seed's Master Quest dungeons have more Wallmasters than "
         "Wallmaster Shuffle can place. Try another seed.");
  assert(std::string(kWallmasterCapacityText) ==
         "Set Spirit Temple Quest or Bottom of the Well Quest to Vanilla to "
         "use Wallmaster Shuffle.");
  for (const bool forced : {false, true}) {
    Fake::Reset();
    Fake::quests[OotMqIndex("Spirit")] = Fake::quests[OotMqIndex("BotW")] =
        true;
    auto g = Generation(5);
    g.capability.clear();
    // Selection Only: Spirit and BotW Master Quest, Shadow and Ganon vanilla,
    // and, unless forced, the Shadow Temple a coin flip.
    g.readSetting = [forced](const char *key, int fallback) {
      const std::string_view k(key);
      if (k == kMqDungeonsSetting)
        return 3;
      if (k == "gRandoSettings.MQDungeonsSpiritTemple" ||
          k == "gRandoSettings.MQDungeonsBottomOfTheWell")
        return 1;
      if (k == "gRandoSettings.MQDungeonsShadowTemple")
        return forced ? 0 : 2;
      return fallback;
    };
    const auto error = Shortfall(g);
    assert(error.player() ==
           (forced ? kWallmasterCapacityText : kWallmasterCapacitySeedText));
  }
  // The seed's own roll (the first attempt) stops generation; a retry's roll
  // fails only that attempt.
  static_assert(WallmasterShortfallStopsGeneration(0) &&
                !WallmasterShortfallStopsGeneration(1) &&
                !WallmasterShortfallStopsGeneration(7));
  static_assert(
      std::is_base_of_v<UnifiedEntranceRefused, WallmasterCapacityShortfall> &&
      !std::is_base_of_v<ShuffleError, WallmasterCapacityShortfall>);
}
// All-vanilla seeds made in game (no id named) are byte for byte the vanilla
// id's, whether the build has the Master Quest id or not.
static void CheckVanillaBytes() {
  const auto shipping = CapabilitySet::AtLeast(Stage::Public);
  for (uint32_t seed = 1; seed <= 40; ++seed)
    for (uint8_t age : {uint8_t(1), uint8_t(2)})
      for (auto access : {OotAccess::ALL_REACHABLE, OotAccess::BEATABLE_ONLY,
                          OotAccess::NO_LOGIC}) {
        const auto named = Run(seed, age, (seed % 2) != 0, access, true);
        auto withQuests = Caps();
        const auto quests = Run(seed, age, (seed % 2) != 0, access, true, false,
                                false, &withQuests);
        assert(quests.completeSeed == named.completeSeed &&
               quests.layout == named.layout);
        const auto shipped = Run(seed, age, (seed % 2) != 0, access, true,
                                 false, false, &shipping);
        auto expected = DecodeEntranceLayout(named.layout);
        expected.devBuild = false;
        assert(DecodeEntranceLayout(shipped.layout).required ==
                   expected.required &&
               EncodeEntranceLayout(expected) == shipped.layout);
      }
}
// "Play warp songs across games" in the seed's logic (limits design, Stage 1),
// chosen by settings in a build with wallmasters-own-game-songs-v1: every quest
// context of OoTMM's bundled data takes the songs id, the capacity shortfalls
// of tools/ootmm/m1-contract.json are refused as without the songs, Legacy
// Master Quest data keeps its refusal, and No Logic keeps the vanilla or quests
// id. The songs id has the same rules: a layout with its id swapped for the one
// the context takes without the songs validates and has that generation's
// routes. Co-op identities differ from the seeds without the songs. The
// shipping set takes the songs id.
static size_t CheckSongContexts(const Json &contract) {
  std::set<std::set<std::string>> shortfalls;
  for (const auto &row : contract.at("mqSources").at("capacityShortfalls"))
    shortfalls.insert(row.at("mq").get<std::set<std::string>>());
  const auto dungeons =
      contract.at("mqSources").at("dungeons").get<std::vector<std::string>>();
  const auto quests = [](unsigned mask) {
    OotQuests result{};
    for (size_t i = 0; i < result.size(); ++i)
      result[i] = (mask & (1u << i)) != 0;
    return result;
  };
  size_t refused = 0, generated = 0, compared = 0;
  for (unsigned mask = 0; mask < 4096; ++mask) {
    Fake::Reset();
    Fake::quests = quests(mask);
    std::set<std::string> projection;
    for (const auto &dungeon : dungeons)
      if (Fake::quests[OotMqIndex(dungeon)])
        projection.insert(dungeon);
    UnifiedEntranceStats stats;
    auto g = Generation(mask);
    g.capability.clear();
    g.foreignSongsLogic = true;
    g.stats = &stats;
    if (shortfalls.contains(projection)) {
      const auto error = Shortfall(g);
      assert(error.player() == kWallmasterCapacitySeedText &&
             stats.randomDraws == 0);
      ++refused;
      continue;
    }
    const auto encoded =
        GenerateEntranceLayout(g, Fake::Native(), Fake::Probe());
    const auto layout = DecodeEntranceLayout(encoded);
    assert(layout.required == std::vector<std::string>{std::string(
                                  kSongsWallmasterCapability)} &&
           layout.ootMq == Fake::quests && stats.randomDraws > 0);
    for (const auto &route : Validate(layout, Caps()).oot)
      assert(OotUnifiedSourceInQuests(*route.source, Fake::quests));
    auto without = layout;
    without.required = {std::string(mask ? kQuestsWallmasterCapability
                                         : kVanillaWallmasterCapability)};
    Validate(without, Caps());
    assert(SeedIdentityText("seed", encoded) !=
           SeedIdentityText("seed", EncodeEntranceLayout(without)));
    assert(!ActiveOotUnified().active() && !ActiveMmUnified().active());
    if (mask % 64 == 0) {
      Fake::Reset();
      Fake::quests = layout.ootMq;
      auto plain = Generation(mask);
      plain.capability.clear();
      const auto expected = DecodeEntranceLayout(
          GenerateEntranceLayout(plain, Fake::Native(), Fake::Probe()));
      assert(expected.required == without.required &&
             expected.overrides == layout.overrides);
      ++compared;
    }
    ++generated;
  }
  // Of the 64 contexts compared, the 4 with MQ Spirit and BotW and vanilla
  // Shadow and Ganon are shortfalls.
  assert(refused == 256 && generated == 4096 - 256 && compared == 60);
  // Only OoTMM's bundled data admits a Master Quest context, with the songs
  // too; an all-vanilla seed from Legacy data takes the songs id.
  for (const unsigned mask : {1u, 0x80u, 0xfffu}) {
    Fake::Reset();
    Fake::quests = quests(mask);
    auto g = Generation(mask);
    g.capability.clear();
    g.foreignSongsLogic = true;
    g.mqSource = ComboMq::SeedMqSource::Legacy;
    assert(Refusal(g).player() == kWallmasterMasterQuestText);
  }
  Fake::Reset();
  auto g = Generation(3);
  g.capability.clear();
  g.foreignSongsLogic = true;
  g.mqSource = ComboMq::SeedMqSource::Legacy;
  assert(DecodeEntranceLayout(
             GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()))
             .required ==
         std::vector<std::string>{std::string(kSongsWallmasterCapability)});
  // No Logic: no Temple pass and no song facts, so the seed is the one made
  // without the songs, under the vanilla or quests id.
  for (const unsigned mask : {0u, 1u}) {
    std::string layouts[2];
    for (const bool foreign : {false, true}) {
      Fake::Reset();
      Fake::quests = quests(mask);
      g = Generation(mask);
      g.capability.clear();
      g.foreignSongsLogic = foreign;
      g.access = OotAccess::NO_LOGIC;
      layouts[foreign] =
          GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()).dump();
    }
    assert(layouts[0] == layouts[1] &&
           DecodeEntranceLayout(Json::parse(layouts[1])).required ==
               std::vector<std::string>{
                   std::string(mask ? kQuestsWallmasterCapability
                                    : kVanillaWallmasterCapability)});
  }
  // The shipping set takes the songs id; a set without it (no build has one)
  // refuses foreign-song logic before any draw or native change, as an internal
  // fault, since no sentence is left for it.
  const auto withoutSongs =
      CapabilitySet::AtLeast(Stage::Hidden).Only(kVanillaWallmasterCapability) |
      CapabilitySet::AtLeast(Stage::Hidden).Only(kQuestsWallmasterCapability);
  const auto shipping = CapabilitySet::AtLeast(Stage::Public);
  assert(shipping.Active(kSongsWallmasterCapability));
  for (const auto *set : {&withoutSongs, &shipping}) {
    Fake::Reset();
    g = Generation(4);
    g.capability.clear();
    g.active = *set;
    g.foreignSongsLogic = true;
    if (set->Active(kSongsWallmasterCapability)) {
      assert(DecodeEntranceLayout(
                 GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()))
                 .required ==
             std::vector<std::string>{std::string(kSongsWallmasterCapability)});
      continue;
    }
    const auto error = Refusal(g);
    assert(error.what() == std::string(kVanillaWallmasterCapability) +
                               " does not admit foreign-song logic" &&
           error.player() == error.what());
  }
  return generated;
}
// The shortcut differential with the songs on (limits design, Stage 1), over
// the 200 seeds, both ages, both starting games and the three accessibility
// modes of the vanilla corpus: chosen by settings in a build with the songs id,
// every seed with logic takes it, and the monotone shortcut gives the full
// search's seed byte for byte. With the id swapped back, every seed is the
// vanilla id's (the 0.4.12 corpus, corpusFnv64 844767829324957485), so the
// songs change nothing but the id; No Logic seeds keep the vanilla id. The
// co-op identity of a songs seed differs from the seed without the songs.
static Json SongDifferential() {
  const auto all = Caps();
  const std::string songsId = "\"requires\":[\"" +
                              std::string(kSongsWallmasterCapability) + "\"]",
                    vanillaId = "\"requires\":[\"" +
                                std::string(kVanillaWallmasterCapability) +
                                "\"]";
  size_t pairs = 0, shortcuts = 0;
  uint64_t corpus = 14695981039346656037ull;
  std::ostringstream quiet;
  auto *previousOutput = std::cout.rdbuf(quiet.rdbuf());
  for (uint32_t seed = 1; seed <= 200; ++seed)
    for (uint8_t age : {uint8_t(1), uint8_t(2)})
      for (bool mmStart : {false, true})
        for (auto access : {OotAccess::ALL_REACHABLE, OotAccess::BEATABLE_ONLY,
                            OotAccess::NO_LOGIC}) {
          quiet.str("");
          quiet.clear();
          const auto full =
              Run(seed, age, mmStart, access, false, false, false, &all, true);
          const auto fast =
              Run(seed, age, mmStart, access, true, false, false, &all, true);
          assert(full.completeSeed == fast.completeSeed &&
                 full.layout == fast.layout);
          assert(full.stats.randomDraws == fast.stats.randomDraws &&
                 fast.stats.randomDraws == 39);
          assert(!full.stats.monotoneShortcutUsed);
          const bool logic = access != OotAccess::NO_LOGIC;
          assert(DecodeEntranceLayout(fast.layout).required ==
                 std::vector<std::string>{
                     std::string(logic ? kSongsWallmasterCapability
                                       : kVanillaWallmasterCapability)});
          if (logic) {
            assert(fast.stats.monotoneShortcutUsed &&
                   fast.stats.assumedChecks == 1 && fast.temple == 1);
            assert(full.stats.assumedChecks == 13 && full.temple == 13);
            ++shortcuts;
          } else
            assert(!fast.stats.monotoneShortcutUsed && fast.temple == 0);
          auto bytes = fast.completeSeed;
          const auto at = bytes.find(songsId);
          assert(logic == (at != std::string::npos) &&
                 bytes.find(songsId, at + 1) == std::string::npos);
          if (logic)
            bytes.replace(at, songsId.size(), vanillaId);
          for (unsigned char byte : bytes)
            corpus = (corpus ^ byte) * 1099511628211ull;
          ++pairs;
        }
  assert(corpus == 844767829324957485ull);
  // Co-op: the app's seed identity covers the song map's logic flags and the
  // layout's id (GeneratedSeedIdentityText), so a songs seed never shares one
  // with the seed made without the songs, under logic or No Logic.
  const Json rules = {{"logicValues", Json::object()},
                      {"mmTricks", Json::object()},
                      {"ootTricks", ""}};
  SongTravelMap plainSongs, songs;
  songs.foreignSongs = songs.logicForeignSongs = true;
  const auto songSeed = Run(9, 2, false, OotAccess::ALL_REACHABLE, true, false,
                            false, &all, true);
  const auto plainSeed = Run(9, 2, false, OotAccess::ALL_REACHABLE, true, false,
                             false, &all, false);
  const auto identity = [&](const SongTravelMap &map, const Json &layout) {
    return GeneratedSeedIdentityText("9", "oot", "mm", rules, map.json(), "",
                                     layout);
  };
  assert(identity(songs, songSeed.layout) !=
             identity(plainSongs, plainSeed.layout) &&
         identity(songs, songSeed.layout) !=
             identity(songs, plainSeed.layout) &&
         identity(songs, plainSeed.layout) !=
             identity(plainSongs, plainSeed.layout));
  std::cout.rdbuf(previousOutput);
  return Json{{"songDifferentialPairs", pairs},
              {"songShortcutPairs", shortcuts},
              {"songCorpusFnv64AsVanilla", corpus}};
}
// A starting OoT warp song (limits design, Stage 2), chosen by settings in a
// build with wallmasters-own-game-songs-v2: every quest context of OoTMM's
// bundled data takes the songs-v2 id, with foreign-song logic or without, the
// capacity shortfalls of tools/ootmm/m1-contract.json are refused as without
// the song, Legacy Master Quest data keeps its refusal, and No Logic keeps the
// earlier ids and their layout. The songs-v2 id has the same rules: a layout
// with its id swapped for the one the context takes without the song
// validates and has that generation's routes. The shipping set takes the
// songs-v2 id.
static size_t CheckWarpContexts(const Json &contract) {
  std::set<std::set<std::string>> shortfalls;
  for (const auto &row : contract.at("mqSources").at("capacityShortfalls"))
    shortfalls.insert(row.at("mq").get<std::set<std::string>>());
  const auto dungeons =
      contract.at("mqSources").at("dungeons").get<std::vector<std::string>>();
  const auto quests = [](unsigned mask) {
    OotQuests result{};
    for (size_t i = 0; i < result.size(); ++i)
      result[i] = (mask & (1u << i)) != 0;
    return result;
  };
  const char *songs[] = {"Minuet of Forest",  "Bolero of Fire",
                         "Serenade of Water", "Nocturne of Shadow",
                         "Requiem of Spirit", "Prelude of Light"};
  size_t refused = 0, generated = 0, compared = 0;
  for (unsigned mask = 0; mask < 4096; ++mask) {
    Fake::Reset();
    Fake::quests = quests(mask);
    std::set<std::string> projection;
    for (const auto &dungeon : dungeons)
      if (Fake::quests[OotMqIndex(dungeon)])
        projection.insert(dungeon);
    UnifiedEntranceStats stats;
    auto g = Generation(mask);
    g.capability.clear();
    g.ootDump = WarpDump(songs[mask % 6]);
    g.foreignSongsLogic = mask % 2 == 1;
    g.stats = &stats;
    if (shortfalls.contains(projection)) {
      const auto error = Shortfall(g);
      assert(error.player() == kWallmasterCapacitySeedText &&
             stats.randomDraws == 0);
      ++refused;
      continue;
    }
    const auto encoded =
        GenerateEntranceLayout(g, Fake::Native(), Fake::Probe());
    const auto layout = DecodeEntranceLayout(encoded);
    assert(layout.required == std::vector<std::string>{std::string(
                                  kStartingSongsWallmasterCapability)} &&
           layout.ootMq == Fake::quests && stats.randomDraws > 0);
    for (const auto &route : Validate(layout, Caps()).oot)
      assert(OotUnifiedSourceInQuests(*route.source, Fake::quests));
    auto without = layout;
    without.required = {std::string(g.foreignSongsLogic
                                        ? kSongsWallmasterCapability
                                    : mask ? kQuestsWallmasterCapability
                                           : kVanillaWallmasterCapability)};
    Validate(without, Caps());
    assert(SeedIdentityText("seed", encoded) !=
           SeedIdentityText("seed", EncodeEntranceLayout(without)));
    assert(!ActiveOotUnified().active() && !ActiveMmUnified().active());
    if (mask % 64 == 0 || mask % 64 == 1) {
      Fake::Reset();
      Fake::quests = layout.ootMq;
      auto plain = Generation(mask);
      plain.capability.clear();
      plain.foreignSongsLogic = g.foreignSongsLogic;
      const auto expected = DecodeEntranceLayout(
          GenerateEntranceLayout(plain, Fake::Native(), Fake::Probe()));
      assert(expected.required == without.required &&
             expected.overrides == layout.overrides);
      ++compared;
    }
    ++generated;
  }
  assert(refused == 256 && generated == 4096 - 256 && compared == 120);
  // Only OoTMM's bundled data admits a Master Quest context, with the song
  // too; an all-vanilla seed from Legacy data takes the songs-v2 id.
  for (const unsigned mask : {1u, 0x80u, 0xfffu}) {
    Fake::Reset();
    Fake::quests = quests(mask);
    auto g = Generation(mask);
    g.capability.clear();
    g.ootDump = WarpDump();
    g.mqSource = ComboMq::SeedMqSource::Legacy;
    assert(Refusal(g).player() == kWallmasterMasterQuestText);
  }
  Fake::Reset();
  auto g = Generation(3);
  g.capability.clear();
  g.ootDump = WarpDump();
  g.mqSource = ComboMq::SeedMqSource::Legacy;
  assert(DecodeEntranceLayout(
             GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()))
             .required == std::vector<std::string>{
                              std::string(kStartingSongsWallmasterCapability)});
  // No Logic: no Temple pass and no song facts, so the seed is the one made
  // without the song, under the vanilla or quests id.
  for (const unsigned mask : {0u, 1u}) {
    std::string layouts[2];
    for (const bool warp : {false, true}) {
      Fake::Reset();
      Fake::quests = quests(mask);
      g = Generation(mask);
      g.capability.clear();
      if (warp)
        g.ootDump = WarpDump();
      g.access = OotAccess::NO_LOGIC;
      layouts[warp] =
          GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()).dump();
    }
    assert(layouts[0] == layouts[1] &&
           DecodeEntranceLayout(Json::parse(layouts[1])).required ==
               std::vector<std::string>{
                   std::string(mask ? kQuestsWallmasterCapability
                                    : kVanillaWallmasterCapability)});
  }
  // The shipping set takes the songs-v2 id (Public from 0.4.13).
  const auto shipping = CapabilitySet::AtLeast(Stage::Public);
  assert(shipping.Active(kStartingSongsWallmasterCapability));
  Fake::Reset();
  g = Generation(4);
  g.capability.clear();
  g.active = shipping;
  g.ootDump = WarpDump();
  assert(DecodeEntranceLayout(
             GenerateEntranceLayout(g, Fake::Native(), Fake::Probe()))
             .required == std::vector<std::string>{
                              std::string(kStartingSongsWallmasterCapability)});
  return generated;
}
// The shortcut differential with a starting OoT warp song (limits design,
// Stage 2), over the 200 seeds, both ages, both starting games and the three
// accessibility modes of the vanilla corpus, each seed with one of the six
// songs and every other one with foreign-song logic too: chosen by settings in
// a build with the songs-v2 id, every seed with logic takes it, and the
// monotone shortcut gives the full search's seed byte for byte. With the id
// swapped back, every layout is the one the same seed makes without the song
// (the vanilla or songs id); No Logic seeds keep the vanilla id and layout.
static Json WarpDifferential() {
  const auto all = Caps();
  const char *songs[] = {"Minuet of Forest",  "Bolero of Fire",
                         "Serenade of Water", "Nocturne of Shadow",
                         "Requiem of Spirit", "Prelude of Light"};
  size_t pairs = 0, shortcuts = 0;
  std::ostringstream quiet;
  auto *previousOutput = std::cout.rdbuf(quiet.rdbuf());
  for (uint32_t seed = 1; seed <= 200; ++seed)
    for (uint8_t age : {uint8_t(1), uint8_t(2)})
      for (bool mmStart : {false, true})
        for (auto access : {OotAccess::ALL_REACHABLE, OotAccess::BEATABLE_ONLY,
                            OotAccess::NO_LOGIC}) {
          quiet.str("");
          quiet.clear();
          const bool foreign = seed % 2 == 0;
          const std::string song = songs[seed % 6];
          const auto full = Run(seed, age, mmStart, access, false, false, false,
                                &all, foreign, song);
          const auto fast = Run(seed, age, mmStart, access, true, false, false,
                                &all, foreign, song);
          assert(full.completeSeed == fast.completeSeed &&
                 full.layout == fast.layout);
          assert(full.stats.randomDraws == fast.stats.randomDraws &&
                 fast.stats.randomDraws == 39);
          assert(!full.stats.monotoneShortcutUsed);
          const bool logic = access != OotAccess::NO_LOGIC;
          const auto plain = Run(seed, age, mmStart, access, true, false, false,
                                 &all, foreign);
          auto swapped = DecodeEntranceLayout(fast.layout);
          assert(swapped.required ==
                 std::vector<std::string>{
                     std::string(logic ? kStartingSongsWallmasterCapability
                                       : kVanillaWallmasterCapability)});
          if (logic) {
            assert(fast.stats.monotoneShortcutUsed &&
                   fast.stats.assumedChecks == 1 && fast.temple == 1);
            assert(full.stats.assumedChecks == 13 && full.temple == 13);
            ++shortcuts;
          } else
            assert(!fast.stats.monotoneShortcutUsed && fast.temple == 0);
          swapped.required = DecodeEntranceLayout(plain.layout).required;
          assert(EncodeEntranceLayout(swapped) == plain.layout);
          ++pairs;
        }
  std::cout.rdbuf(previousOutput);
  return Json{{"warpDifferentialPairs", pairs},
              {"warpShortcutPairs", shortcuts}};
}
int main(int argc, char **argv) {
  assert(argc == 3 || (argc == 4 && (std::string_view(argv[3]) == "songs" ||
                                     std::string_view(argv[3]) == "warp")));
  std::ifstream contractFile(argv[2]);
  const auto contract = Json::parse(contractFile);
  if (argc == 4 && std::string_view(argv[3]) == "warp") {
    const size_t contexts = CheckWarpContexts(contract);
    auto report = WarpDifferential();
    report["scope"] = "Real generator and combined fill with explicit fake "
                      "native modules/oracles, a starting OoT warp song";
    report["warpQuestContextsGenerated"] = contexts;
    report["nativeSeedCorpusOrGameplay"] = false;
    std::ofstream(argv[1]) << report.dump(2) << '\n';
    std::cout << report.dump() << '\n';
    return 0;
  }
  if (argc == 4) {
    const size_t contexts = CheckSongContexts(contract);
    auto report = SongDifferential();
    report["scope"] = "Real generator and combined fill with explicit fake "
                      "native modules/oracles, foreign-song logic on";
    report["songQuestContextsGenerated"] = contexts;
    report["nativeSeedCorpusOrGameplay"] = false;
    std::ofstream(argv[1]) << report.dump(2) << '\n';
    std::cout << report.dump() << '\n';
    return 0;
  }
  Fake::Reset();
  auto g = Generation();
  g.settings = {};
  assert(GenerateEntranceLayout(g, {}, {}).is_null());
  Untouched();
  // The shipping set (P1-05) generates the dev set's layout, marked as a
  // shipping layout, which it accepts; it refuses the dev one.
  const auto shipping = CapabilitySet::AtLeast(Stage::Public);
  Fake::Reset();
  const auto devLayout =
      GenerateEntranceLayout(Generation(7), Fake::Native(), Fake::Probe());
  Fake::Reset();
  g = Generation(7);
  g.active = shipping;
  const auto shipped = GenerateEntranceLayout(g, Fake::Native(), Fake::Probe());
  auto expected = DecodeEntranceLayout(devLayout);
  assert(expected.devBuild);
  expected.devBuild = false;
  assert(EncodeEntranceLayout(expected) == shipped);
  Validate(DecodeEntranceLayout(shipped), shipping);
  Reject([&] { Validate(DecodeEntranceLayout(devLayout), shipping); });
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
  // The vanilla id named explicitly reads the context after the reset too, and
  // refuses the Master Quest dungeon it finds there.
  Fake::Reset();
  Fake::changeQuestOnReset = true;
  g = Generation();
  assert(Refusal(g).player() == kWallmasterMasterQuestText);
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
  // The generator must read the same settings-level logic fact as the menu,
  // rather than assuming that every logic-enabled caller is glitchless.
  for (int rawLogic : {-1, 2, 99}) {
    Fake::Reset();
    auto unsupported = Generation();
    unsupported.readSetting = [rawLogic](const char *key, int fallback) {
      return std::string_view(key) == "gRandoSettings.LogicRules" ? rawLogic
                                                                  : fallback;
    };
    UnifiedEntranceStats stats;
    unsupported.stats = &stats;
    bool refused = false;
    try {
      GenerateEntranceLayout(unsupported, Fake::Native(), Fake::Probe());
    } catch (const UnifiedEntranceRefused &error) {
      refused =
          std::string(error.what()).find("glitchless") != std::string::npos;
    }
    assert(refused && stats.randomDraws == 0);
    Untouched();
  }
  Fake::Reset();
  auto noLogic = Generation();
  noLogic.access = OotAccess::NO_LOGIC;
  noLogic.readSetting = [](const char *key, int fallback) {
    return std::string_view(key) == "gRandoSettings.LogicRules" ? 1 : fallback;
  };
  UnifiedEntranceStats noLogicStats;
  noLogic.stats = &noLogicStats;
  GenerateEntranceLayout(noLogic, Fake::Native(), Fake::Probe());
  assert(noLogicStats.randomDraws == 39 && noLogicStats.completeChecks == 1 &&
         noLogicStats.assumedChecks == 0 && Fake::queries == 0);
  CheckShortcutPoolGuards();
  CheckPlainRefusals();
  CheckQuestContexts(contract);
  CheckQuestsAcrossReset();
  CheckMqDataSource();
  CheckCapacitySentences();
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
  CheckVanillaBytes();
  std::cout.rdbuf(previousOutput);
  // The base build's whole-host corpus (P1-05, 0.4.12): the vanilla id's seeds
  // are unchanged byte for byte.
  assert(corpus == 844767829324957485ull);
  Json report{{"scope", "Real generator and combined fill with explicit fake "
                        "native modules/oracles"},
              {"differentialPairs", pairs},
              {"shortcutPairs", shortcuts},
              {"fallbackPairs", 20},
              {"templeFallbackPairs", 20},
              {"nativeQuestContextsRejected", 4095},
              {"questContextsGenerated", 4095 - 256},
              {"questContextsRefusedForCapacity", 256},
              {"nonBundledQuestContextsRefused", 2 * 4095 * 3},
              {"corpusFnv64", corpus},
              {"nativeSeedCorpusOrGameplay", false}};
  std::ofstream(argv[1]) << report.dump(2) << '\n';
  std::cout << report.dump() << '\n';
}
