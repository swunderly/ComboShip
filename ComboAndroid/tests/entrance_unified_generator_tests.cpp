// The unified entrance generator (combo/rando/entrances/UnifiedBridge.h) for
// the dev-only MM Wallmaster tranche, with a fake MM module (the real unified
// MM layout and assumption decoder behind it), fake reachability oracles and a
// fake Temple probe: the disabled path, the refusals before any random draw,
// determinism, the candidates against the pinned M1 contract
// (tools/ootmm/m1-contract.json), the output against the capability sets, the
// Temple pass, scope restoration on every exit path, the install for the fill
// and playthrough, the seed identity, the NativeBridge.h scopes, and the frozen
// private MM Wallmaster generator.
#include "rando/ComboPlaythrough.h"
#include "rando/entrances/UnifiedBridge.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

using namespace ComboRando;
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
using Strings = std::set<std::string>;

namespace {
Json fixture;
// Neither refusal is a ShuffleError, which the Shuffler and an outer attempt
// retry (C1).
static_assert(!std::is_base_of_v<ShuffleError, UnifiedEntranceRefused> &&
              !std::is_base_of_v<ShuffleError, TempleProbeUnsupported>);
int ootResets = 0; // the OoT oracle's resets
ComboGenProgress progress;
const std::string kWallmasters = "erWallmasters=ownGame;erOneWays=none";

CapabilitySet WholeCatalog(bool devBuild = true) {
  auto caps = CapabilitySet::AtLeast(Stage::Hidden);
  caps.devBuild = devBuild;
  return caps;
}
template <class E, class F> std::string Error(F action) {
  try {
    action();
  } catch (const E &error) {
    return error.what();
  }
  assert(!"an exception was expected");
  return {};
}
bool Has(const std::string &text, const char *part) {
  return text.find(part) != std::string::npos;
}

// The fixture's candidates of each MM Wallmaster source.
std::map<std::string, Strings> PinnedMmTargets() {
  std::map<std::string, Strings> result;
  for (const auto &c : fixture.at("combinations"))
    if (c.at("id") == kWallmasters)
      for (const auto &pool : c.at("pools"))
        for (const auto &group : pool.at("candidates"))
          for (const auto &src : group.at("sources"))
            if (src.get<std::string>().starts_with("MM_"))
              for (const auto &dst : group.at("destinations"))
                result[src.get<std::string>()].insert(dst.get<std::string>());
  assert(result.size() == 4);
  return result;
}

// The MM module: the export's rules over the real unified table and
// assumptions (ComboEntrancesMM.cpp; the Python harness runs the real
// exports), against this module's own capabilities.
struct FakeMm {
  static inline CapabilitySet caps = WholeCatalog();
  static inline std::string dump;
  static inline std::vector<Json> overlays;
  static inline int applies = 0, sets = 0, clears = 0, dumps = 0;
  static inline bool malformed = false;
  static void reset() {
    ActiveMmUnified().clear();
    ActiveMmUnifiedAssumptions().reset();
    overlays.clear();
    applies = sets = clears = dumps = 0;
    malformed = false;
  }
  static int apply(const char *text) {
    ++applies;
    ActiveMmUnified().clear();
    ActiveMmUnifiedAssumptions().reset();
    try {
      const auto seed = Json::parse(text);
      if (seed.contains("entranceLayout"))
        ActiveMmUnified().install(seed, caps);
    } catch (const std::exception &) {
      ActiveMmUnified().clear();
      return 0;
    }
    return 1;
  }
  static int assume(const char *text) {
    ++sets;
    ActiveMmUnifiedAssumptions().reset();
    try {
      overlays.push_back(Json::parse(text));
      ActiveMmUnifiedAssumptions() =
          DecodeMmUnifiedAssumptions(overlays.back());
    } catch (const std::exception &) {
      return 0;
    }
    return 1;
  }
  static void clear() {
    ++clears;
    ActiveMmUnifiedAssumptions().reset();
  }
  static const char *snapshot() {
    ++dumps;
    Json seed = Json::object();
    if (ActiveMmUnified().active())
      seed["entranceLayout"] = ActiveMmUnified().json();
    if (malformed)
      seed["entranceLayout"] = Json::object();
    dump = seed.dump();
    return dump.c_str();
  }
  static int calls() { return applies + sets + clears + dumps; }
};
const NativeFns kMm{FakeMm::apply, FakeMm::assume, FakeMm::clear,
                    FakeMm::snapshot};

// The Temple probe: the query must name the candidate the MM oracle just saw.
struct FakeProbe {
  static inline int prepares = 0, queries = 0, releases = 0;
  static inline int prepareCode = 0, queryCode = 1, resetsAtPrepare = -1;
  static inline bool prepared = false;
  static inline Json prepare, first, last;
  static void reset() {
    prepares = queries = releases = 0;
    prepareCode = 0;
    queryCode = 1;
    resetsAtPrepare = -1;
    prepared = false;
    prepare = first = last = Json();
  }
  static int Prepare(const char *json) {
    ++prepares;
    resetsAtPrepare = ootResets;
    prepare = Json::parse(json);
    prepared = prepareCode == 0;
    return prepareCode;
  }
  static int Query(const char *json, int (*poll)(void), uint8_t *ages) {
    ++queries;
    if (!prepared)
      return -1;
    if (poll && poll())
      return -3;
    last = Json::parse(json);
    if (first.is_null())
      first = last;
    assert(last["deleted"].empty() && last["edges"].empty());
    assert(!FakeMm::overlays.empty() &&
           last["world"] == FakeMm::overlays.back()["world"]);
    *ages = 0;
    return queryCode;
  }
  static const char *LastError() { return "fake probe detail"; }
  static void Release() {
    ++releases;
    prepared = false;
  }
  static int calls() { return prepares + queries + releases; }
};
TempleProbeFns Probe() {
  TempleProbeFns fns;
  fns.prepare = FakeProbe::Prepare;
  fns.query = FakeProbe::Query;
  fns.lastError = FakeProbe::LastError;
  fns.release = FakeProbe::Release;
  fns.poll = TempleProbeCancelPoll;
  return fns;
}

// The reachability oracles. "Wallmaster prize" is reachable only while the MM
// logic graph (MmUnifiedGraphRoutes: candidate assumptions, else the installed
// layout) routes Dampe's Wallmaster to Woodfall Temple.
struct Oracles {
  static inline int mmQueries = 0, cancelAt = 0;
  static inline bool majora = true, prizeNeeded = false;
  static inline std::vector<std::string> owned[2];
  static void reset() {
    ootResets = mmQueries = cancelAt = 0;
    majora = true;
    prizeNeeded = false;
  }
  static void ResetOot() {
    ++ootResets;
    owned[0].clear();
  }
  static void ResetMm() { owned[1].clear(); }
  static void OwnOot(const char *text) {
    owned[0] = Json::parse(text).get<std::vector<std::string>>();
  }
  static void OwnMm(const char *text) {
    owned[1] = Json::parse(text).get<std::vector<std::string>>();
  }
  static const char *OotChecks() {
    static std::string out;
    out = Json{"Ganon", "Oot start"}.dump();
    return out.c_str();
  }
  static bool Prize() {
    const auto *dampe = Find("MM_WALLMASTER_DAMPE");
    const auto *woodfall = Find("MM_TEMPLE_WOODFALL");
    for (const auto &route : MmUnifiedGraphRoutes(dampe->id))
      if (route.destination == Key{Entrances::Game::Mm, woodfall->id})
        return true;
    return false;
  }
  static const char *MmChecks() {
    static std::string out;
    if (++mmQueries == cancelAt)
      progress.RequestCancel();
    Json checks = {"Mm start"};
    if (majora)
      checks.push_back("Defeat Majora");
    if (Prize())
      checks.push_back("Wallmaster prize");
    out = checks.dump();
    return out.c_str();
  }
  static uint8_t Portal() { return 1; }
  static uint8_t Adult() { return 1; }
  static uint8_t Child() { return 2; }
  static void Place(const char *, const char *) {}
};
OracleFns OotOracle() {
  OracleFns fns{Oracles::ResetOot, Oracles::OwnOot, Oracles::OotChecks,
                Oracles::Place, Oracles::Portal};
  fns.GetStartAges = Oracles::Child;
  return fns;
}
OracleFns MmOracle() {
  return {Oracles::ResetMm, Oracles::OwnMm, Oracles::MmChecks, Oracles::Place};
}

std::string Dump(std::vector<std::string> checks,
                 std::vector<std::string> starting = {}) {
  Json value = {{"checks", Json::array()},
                {"pool", Json::array()},
                {"sharedStartingItems", starting}};
  for (const auto &check : checks) {
    value["checks"].push_back({{"name", check}});
    value["pool"].push_back(
        {{"name", "Filler"}, {"advancement", false}, {"category", "junk"}});
  }
  return value.dump();
}

// Ship's settings: everything off unless set.
std::map<std::string, int> shipSettings;
int reads = 0;
int Read(const char *key, int fallback) {
  ++reads;
  const auto it = shipSettings.find(key);
  return it == shipSettings.end() ? fallback : it->second;
}

Settings Wallmasters() {
  Settings s;
  s.erWallmasters = Mode::OwnGame;
  return s;
}
UnifiedGeneration Tranche(uint32_t seed = 7) {
  UnifiedGeneration g;
  g.settings = Wallmasters();
  g.capability = std::string(kMmTrancheCapability);
  g.active = WholeCatalog();
  g.seed = seed;
  g.readSetting = Read;
  g.ootDump = Dump({"Oot start"});
  g.mmDump = Dump({"Mm start"});
  g.oot = OotOracle();
  g.mm = MmOracle();
  return g;
}
void ResetAll() {
  FakeMm::reset();
  FakeProbe::reset();
  Oracles::reset();
  shipSettings.clear();
  reads = 0;
  progress.disposition = 0;
}
Json Generate(const UnifiedGeneration &g) {
  return GenerateEntranceLayout(g, kMm, Probe());
}
std::map<std::string, std::string> Routes(const Json &layout) {
  std::map<std::string, std::string> result;
  for (const auto &row : layout.at("overrides"))
    result[row.at("source").get<std::string>()] =
        row.at("destination").get<std::string>();
  return result;
}
// Nothing native was called and no Temple session was opened.
void Untouched() {
  assert(FakeMm::calls() == 0 && FakeProbe::calls() == 0 && ootResets == 0 &&
         Oracles::mmQueries == 0);
}

// No pool to place: no layout, no read, no native call.
void CheckDisabledPath() {
  ResetAll();
  auto g = Tranche();
  g.settings = {};
  g.capability.clear();
  assert(Generate(g).is_null());
  // The shipping set, an empty build and no exports at all change nothing.
  g.active = CapabilitySet::AtLeast(Stage::Public);
  assert(GenerateEntranceLayout(g, {}, {}).is_null());
  // Pools named by the settings that place nothing: One-Ways without a flag,
  // or with only Anywhere (OoTMM keeps worldChanged false, M1-01 finding 3).
  g.active = WholeCatalog();
  g.capability = "one-ways-own-game-v1";
  g.settings.erOneWays = Mode::OwnGame;
  assert(Generate(g).is_null());
  g.settings.erOneWaysAnywhere = true;
  assert(Generate(g).is_null());
  assert(reads == 0);
  Untouched();
}

// Every refusal comes before any random draw, Temple session or MM call.
void CheckRefusals() {
  const auto refused = [](const UnifiedGeneration &g, const char *reason) {
    ResetAll();
    const auto error = Error<UnifiedEntranceRefused>([&] { Generate(g); });
    assert(Has(error, reason));
    Untouched();
  };
  auto g = Tranche();
  // Settings this build or the selected row does not admit.
  g.active = CapabilitySet::AtLeast(Stage::Public);
  refused(g, "not available in this build");
  g = Tranche();
  g.capability = "nope";
  refused(g, "Unknown entrance capability");
  g = Tranche();
  g.settings.erOneWays = Mode::OwnGame;
  g.settings.erOneWaysMajor = true;
  refused(g, "not available in this build");
  // Without the tranche the OoT Wallmaster rows are in the pool, and no OoT
  // row is bound: the pool is never shrunk.
  g = Tranche();
  g.capability.clear();
  refused(g, "OOT_WALLMASTER_");
  // One-ways (any flag) touch rows with no binding yet.
  for (const auto flag :
       {&Settings::erOneWaysMajor, &Settings::erOneWaysIkana,
        &Settings::erOneWaysOwls, &Settings::erOneWaysWaterVoids}) {
    g = Tranche();
    g.capability = "one-ways-own-game-v1";
    g.settings = {};
    g.settings.erOneWays = Mode::OwnGame;
    g.settings.*flag = true;
    refused(g, "has no native entrance binding yet");
  }
  // A private MM profile, Ship's own entrance shuffle (D1), Master Quest.
  g = Tranche();
  g.privateMmProfiles = true;
  refused(g, "private MM entrance profile");
  for (const char *key : kShipEntranceSettings) {
    g = Tranche();
    ResetAll();
    shipSettings[key] = 1;
    const auto error = Error<UnifiedEntranceRefused>([&] { Generate(g); });
    assert(Has(error, "Ship's own entrance shuffle off"));
    Untouched();
  }
  ResetAll();
  shipSettings[kMqDungeonsSetting] = 3;
  assert(Has(Error<UnifiedEntranceRefused>([&] { Generate(Tranche()); }),
             "Master Quest"));
  Untouched();
  g = Tranche();
  g.readSetting = nullptr;
  refused(g, "cannot read Ship's settings");
}

// The Temple gate refuses before Prepare; the code mapping is never retried.
void CheckTemplePass() {
  const auto unsupported = [](const UnifiedGeneration &g, const char *reason,
                              int prepares, int queries) {
    const auto error = Error<TempleProbeUnsupported>([&] { Generate(g); });
    assert(Has(error, reason));
    assert(FakeProbe::prepares == prepares && FakeProbe::queries == queries &&
           FakeProbe::releases == prepares);
    assert(!ActiveMmUnified().active() && !ActiveMmUnifiedAssumptions());
  };
  ResetAll();
  auto g = Tranche();
  g.songTravelLogic = true;
  unsupported(g, "song travel", 0, 0);
  ResetAll();
  g = Tranche();
  g.mmDump = Dump({"Mm start"}, {"Song of Soaring"});
  unsupported(g, "Song of Soaring", 0, 0);
  ResetAll();
  g = Tranche();
  g.oot.GetStartAges = nullptr;
  unsupported(g, "starting age", 0, 0);
  ResetAll();
  FakeProbe::prepareCode = -2;
  unsupported(Tranche(), "fake probe detail", 1, 0);
  ResetAll();
  FakeProbe::queryCode = -1;
  unsupported(Tranche(), "fake probe detail", 1, 1);
  assert(ootResets >= 1 && FakeMm::sets == 1);

  // Prepare gets the resolved age; the OoT oracle is prepared before it.
  for (bool adult : {false, true}) {
    ResetAll();
    g = Tranche();
    g.oot.GetStartAges = adult ? Oracles::Adult : Oracles::Child;
    Generate(g);
    assert(FakeProbe::prepares == 1 && FakeProbe::releases == 1);
    assert(FakeProbe::prepare["startingAge"] == (adult ? "adult" : "child"));
    // The OoT oracle was prepared before the probe (TempleProbe.cpp P1, P2).
    assert(FakeProbe::resetsAtPrepare == 1);
    assert(FakeProbe::queries == FakeMm::sets && FakeProbe::queries >= 4);
    // MM rows only, listed under mm, never probed.
    for (const auto &e : FakeProbe::last["mm"]["edges"])
      assert(e["src"].get<std::string>().starts_with("MM_WALLMASTER_"));
  }
  // No Logic: no Temple pass, no candidate check, still a complete install.
  ResetAll();
  g = Tranche();
  g.access = OotAccess::NO_LOGIC;
  g.songTravelLogic = true; // the gate never runs for logic none
  const auto layout = Generate(g);
  assert(FakeProbe::calls() == 0 && FakeMm::sets == 0 && ootResets == 0 &&
         Oracles::mmQueries == 0);
  assert(Routes(layout).size() == 4 && !ActiveMmUnified().active());
}

// Same seed, same layout; the 4 MM sources get 4 of the 7 pinned arrivals.
void CheckLayouts() {
  const auto pinned = PinnedMmTargets();
  auto normalized = Wallmasters();
  ValidateEntranceSettings(normalized);
  std::set<std::string> distinct;
  for (uint32_t seed = 1; seed <= 20; ++seed)
    for (int variant = 0; variant < (seed <= 3 ? 4 : 1); ++variant) {
      ResetAll();
      auto g = Tranche(seed);
      g.mmStart = (variant & 1) != 0;
      g.access =
          variant & 2 ? OotAccess::BEATABLE_ONLY : OotAccess::ALL_REACHABLE;
      const auto layout = Generate(g);
      ResetAll();
      assert(Generate(g) == layout);
      distinct.insert(layout.dump());
      const auto routes = Routes(layout);
      assert(routes.size() == 4);
      Strings used;
      for (const auto &[src, dst] : routes) {
        assert(pinned.at(src).contains(dst));
        used.insert(dst);
      }
      assert(used.size() == 4);
      // The output: the tranche id, this build's devBuild stamp, valid
      // with the tranche, refused by a release set and without the id.
      const auto decoded = DecodeEntranceLayout(layout);
      assert(decoded.required ==
             std::vector<std::string>{std::string(kMmTrancheCapability)});
      assert(decoded.devBuild &&
             SameEntranceSettings(decoded.settings, normalized));
      assert(decoded.ootMq == OotQuests{});
      assert(EncodeEntranceLayout(decoded) == layout);
      Validate(decoded, WholeCatalog());
      assert(Has(Error<EntranceLayoutError>([&] {
                   Validate(decoded, CapabilitySet::AtLeast(Stage::Public));
                 }),
                 "does not have"));
      assert(Has(Error<EntranceLayoutError>([&] {
                   Validate(decoded, WholeCatalog().WithoutDevOnly());
                 }),
                 "does not have"));
      assert(Has(Error<EntranceLayoutError>(
                     [&] { Validate(decoded, WholeCatalog(false)); }),
                 "dev build"));
      // The generation's scope put the MM module back as it was.
      assert(!ActiveMmUnified().active() && !ActiveMmUnifiedAssumptions());
    }
  assert(distinct.size() > 8);
  // The stream entrances:ootmm-entrances-v1:<seed>, as M1-07 first drew it.
  const std::map<uint32_t, std::pair<uint32_t, std::vector<std::string>>>
      golden = {
          {1u,
           {3861620466u,
            {"MM_TEMPLE_STONE_TOWER_INVERTED", "MM_TEMPLE_GREAT_BAY",
             "MM_TEMPLE_WOODFALL", "MM_SPIDER_HOUSE_OCEAN"}}},
          {2u,
           {3054938091u,
            {"MM_TEMPLE_STONE_TOWER_INVERTED", "MM_SPIDER_HOUSE_OCEAN",
             "MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_GREAT_BAY"}}},
          {3u,
           {3548060248u,
            {"MM_TEMPLE_STONE_TOWER_INVERTED", "MM_TEMPLE_WOODFALL",
             "MM_SPIDER_HOUSE_SWAMP", "MM_TEMPLE_GREAT_BAY"}}},
      };
  for (const auto &[seed, expected] : golden) {
    ResetAll();
    const auto layout = Generate(Tranche(seed));
    std::vector<std::string>
        destinations; // BtW entrance, exit, fountain, Dampe
    for (const auto &[src, dst] : Routes(layout))
      destinations.push_back(dst);
    assert(CwHashName(layout.dump()) == expected.first &&
           destinations == expected.second);
  }
  // A release-stamped build writes devBuild false.
  ResetAll();
  auto g = Tranche();
  g.active = WholeCatalog(false);
  FakeMm::caps = WholeCatalog(false);
  assert(!DecodeEntranceLayout(Generate(g)).devBuild);
  FakeMm::caps = WholeCatalog();
}

// The candidates the oracles and the probe see are the pinned 4x7 table.
void CheckCandidates() {
  const auto pinned = PinnedMmTargets();
  for (uint32_t seed : {3u, 11u, 29u}) {
    ResetAll();
    Generate(Tranche(seed));
    const auto &overlay = FakeMm::overlays.front();
    assert(overlay["profile"] == "ootmm-entrance-assumptions-v1" &&
           overlay["game"] == "mm" && overlay["deleted"].size() == 4 &&
           overlay["fixed"].size() == 1);
    const auto placed = overlay["fixed"][0]["src"].get<std::string>();
    const auto chosen = overlay["fixed"][0]["dst"].get<std::string>();
    assert(pinned.at(placed).contains(chosen));
    std::map<std::string, Strings> remaining;
    for (const auto &e : overlay["remaining"]) {
      assert(!e["resetTime"].get<bool>() && !e["void"].get<bool>());
      remaining[e["src"].get<std::string>()].insert(
          e["dst"].get<std::string>());
    }
    assert(remaining.size() == 3 && !remaining.contains(placed));
    for (const auto &[src, dsts] : remaining) {
      auto expected = pinned.at(src);
      expected.erase(chosen); // no reuse inside the alias pool
      assert(dsts == expected);
    }
    // The probe's first query is the same candidate.
    Strings probeEdges, overlayEdges;
    for (const auto &e : FakeProbe::first["mm"]["edges"])
      probeEdges.insert(e["src"].get<std::string>() + ">" +
                        e["dst"].get<std::string>());
    for (const char *part : {"fixed", "remaining"})
      for (const auto &e : overlay[part])
        overlayEdges.insert(e["src"].get<std::string>() + ">" +
                            e["dst"].get<std::string>());
    assert(probeEdges == overlayEdges &&
           FakeProbe::first["world"] == overlay["world"]);
  }
}

// Every exit path restores the MM module, clears the assumptions and releases
// the probe; an installed layout survives.
void CheckScopes() {
  // A layout bound before generation (the loaded slot's).
  ResetAll();
  ApplyUnifiedLayout(kMm, Generate(Tranche(5)), WholeCatalog());
  const auto bound = ActiveMmUnified().json();
  const auto restored = [&](int released) {
    assert(ActiveMmUnified().active() && ActiveMmUnified().json() == bound);
    assert(!ActiveMmUnifiedAssumptions() && FakeProbe::releases == released &&
           !FakeProbe::prepared);
  };
  const auto from = [&] {
    FakeProbe::reset();
    Oracles::reset();
    FakeMm::overlays.clear();
    ApplyUnifiedLayout(kMm, bound, WholeCatalog());
    FakeMm::applies = FakeMm::sets = FakeMm::clears = FakeMm::dumps = 0;
  };
  GenerationScope scope(&progress);
  // Success.
  from();
  Generate(Tranche(6));
  restored(1);
  // The Temple probe stops generation.
  from();
  FakeProbe::queryCode = 7;
  Error<TempleProbeUnsupported>([&] { Generate(Tranche(6)); });
  restored(1);
  // Cancelled during an MM oracle query, and through the probe's poll.
  for (int at : {1, 2, 3}) {
    from();
    progress.disposition = 0;
    Oracles::cancelAt = at;
    Error<GenerationCancelled>([&] { Generate(Tranche(6)); });
    restored(1);
  }
  from();
  progress.disposition = 0;
  Oracles::cancelAt = 1000000;
  struct CancelAtQuery {
    static int Query(const char *json, int (*poll)(void), uint8_t *ages) {
      progress.RequestCancel();
      return FakeProbe::Query(json, poll, ages);
    }
  };
  auto probe = Probe();
  probe.query = CancelAtQuery::Query;
  Error<GenerationCancelled>(
      [&] { GenerateEntranceLayout(Tranche(6), kMm, probe); });
  restored(1);
  progress.disposition = 0;
  // A malformed snapshot stops generation before anything changes.
  ResetAll();
  FakeMm::malformed = true;
  Error<EntranceLayoutError>([&] { Generate(Tranche(6)); });
  assert(FakeMm::applies == 0 && FakeMm::sets == 0 && FakeProbe::releases == 1);
}

// The layout is installed for the fill and the playthrough of its attempt
// (C13): an item behind a Wallmaster route is placed and walked.
void CheckInstallForFill() {
  ResetAll();
  auto g = Tranche(4);
  g.mmDump = Dump({"Mm start", "Wallmaster prize"});
  const auto layout = Generate(g);
  // Every accepted candidate kept Dampe's route to Woodfall reachable.
  assert(Routes(layout).at("MM_WALLMASTER_DAMPE") == "MM_TEMPLE_WOODFALL");
  assert(!ActiveMmUnified().active());
  ApplyUnifiedLayout(kMm, layout, WholeCatalog());
  assert(ActiveMmUnified().json() == layout && Oracles::Prize());
  auto mmDump = Json::parse(g.mmDump);
  mmDump["pool"][1] = {
      {"name", "Prize item"}, {"advancement", true}, {"category", "major"}};
  const auto fill =
      CrossWorldCombinedFill(g.ootDump, mmDump.dump(), 4, g.oot, g.mm, nullptr,
                             "", OotAccess::ALL_REACHABLE);
  assert(fill.success);
  Json walk = Json::array();
  const auto result =
      RunPlaythrough(fill.spoilerJson, g.oot, g.mm, "unified", nullptr, &walk,
                     g.ootDump, mmDump.dump(), true, false);
  assert(result.beatable && result.unreachableMm == 0 &&
         result.reachableMm == 2);
  bool walked = false;
  for (const auto &sphere : walk)
    for (const auto &step : sphere.value("steps", Json::array()))
      walked = walked || step.value("check", "") == "Wallmaster prize";
  assert(walked);
  // Without the layout the prize check is out of reach and the fill fails.
  ApplyNativeSeed(kMm, Json::object(), WholeCatalog());
  assert(!ActiveMmUnified().active() && !Oracles::Prize());
  assert(!CrossWorldCombinedFill(g.ootDump, mmDump.dump(), 4, g.oot, g.mm,
                                 nullptr, "", OotAccess::ALL_REACHABLE)
              .success);
  // The install checks its round trip and this build's capabilities.
  assert(Has(Error<std::exception>([&] {
               ApplyUnifiedLayout(kMm, layout,
                                  CapabilitySet::AtLeast(Stage::Public));
             }),
             "does not have"));
}

// C5: the seed identity covers a layout; any other seed hashes as before.
void CheckSeedIdentity() {
  ResetAll();
  const auto layout = Generate(Tranche(9));
  assert(SeedIdentityText("seed+dumps", Json()) == "seed+dumps");
  assert(SeedIdentityText("seed+dumps", layout) ==
         "seed+dumps" + layout.dump());
  ResetAll();
  const auto other = Generate(Tranche(10));
  assert(other != layout &&
         SeedIdentityText("x", other) != SeedIdentityText("x", layout));
}

// NativeBridge.h: the scopes carry the unified MM state.
void CheckNativeScopes() {
  ResetAll();
  const auto layout = Generate(Tranche(12));
  ApplyUnifiedLayout(kMm, layout, WholeCatalog());
  {
    NativeScope scope(kMm);
    ApplyNativeSeed(kMm, Json::object(), WholeCatalog());
    assert(!ActiveMmUnified().active());
  }
  assert(ActiveMmUnified().json() == layout);
  FakeMm::malformed = true;
  Error<EntranceLayoutError>([&] { NativeScope scope(kMm); });
  FakeMm::malformed = false;
  // The overlay scope sends one candidate and clears it on every exit.
  const auto world = BuildCandidateWorld(
      {{"MM_WALLMASTER_DAMPE", "MM_TEMPLE_SNOWHEAD"}}, {}, Wallmasters());
  FakeMm::clears = 0;
  try {
    AssumptionScope scope(kMm, EncodeMmOverlay(world));
    assert(ActiveMmUnifiedAssumptions() &&
           ActiveMmUnifiedAssumptions()->world == CandidateWorldToken(world));
    throw std::runtime_error("unwind");
  } catch (const std::runtime_error &) {
  }
  assert(!ActiveMmUnifiedAssumptions() && FakeMm::clears == 1);
  assert(Error<std::runtime_error>([&] { AssumptionScope scope(kMm, "{}"); }) ==
         "MM entrance assumption rejected");
  assert(Error<std::runtime_error>([&] {
           AssumptionScope scope(NativeFns{}, EncodeMmOverlay(world));
         }) == "MM entrance assumption exports unavailable");
}

// The launcher resolves the four probe exports through its own lookup.
void CheckProbeResolution() {
  std::vector<std::string> looked;
  const auto fns = ResolveUnifiedTempleProbe([&](const char *name) -> void * {
    looked.push_back(name);
    return nullptr;
  });
  assert((looked == std::vector<std::string>{"SOH_TempleProbe_Prepare",
                                             "SOH_TempleProbe_Query",
                                             "SOH_TempleProbe_LastError",
                                             "SOH_TempleProbe_Release"}));
  assert(!fns.prepare && !fns.query && fns.poll == TempleProbeCancelPoll);
  // Missing exports stop a run that needs the probe, before any draw.
  ResetAll();
  assert(Has(Error<TempleProbeUnsupported>(
                 [&] { GenerateEntranceLayout(Tranche(), kMm, fns); }),
             "entry points"));
  assert(FakeMm::calls() == 0);
}

// The private MM Wallmaster generator is unchanged: its outputs for these
// fakes were captured before M1-07 (base d75b64dd).
struct Legacy {
  static inline Json seed = Json::object();
  static inline std::string dump, assumption;
  static inline int sets = 0;
  static int apply(const char *text) {
    seed = Json::parse(text);
    return 1;
  }
  static int assume(const char *text) {
    assumption = text;
    ++sets;
    return 1;
  }
  static void clear() { assumption.clear(); }
  static const char *snapshot() {
    dump = seed.dump();
    return dump.c_str();
  }
  static void reset() {}
  static void owned(const char *) {}
  static const char *ootChecks() { return "[\"Ganon\",\"Oot check\"]"; }
  static const char *mmChecks() {
    static std::string out;
    Json checks = {"Mm check"};
    if (assumption.find(
            "\"MM_WALLMASTER_DAMPE\":\"MM_TEMPLE_STONE_TOWER_INVERTED\"") ==
            std::string::npos &&
        assumption.find(
            "\"MM_WALLMASTER_BTW_FOUNTAIN\":\"MM_TEMPLE_GREAT_BAY\"") ==
            std::string::npos)
      checks.push_back("Defeat Majora");
    out = checks.dump();
    return out.c_str();
  }
  static uint8_t portal() { return 1; }
};
void CheckFrozenPrivateGenerator() {
  struct Row {
    uint32_t seed;
    bool logic;
    int sets;
    uint32_t hash;
    std::vector<std::string> destinations; // Dampe, BtW entrance/fountain/exit
  };
  const std::vector<Row> frozen = {
      {1u,
       true,
       5,
       0xd43651aeu,
       {"MM_TEMPLE_STONE_TOWER", "MM_SPIDER_HOUSE_OCEAN",
        "MM_TEMPLE_STONE_TOWER_INVERTED", "MM_SPIDER_HOUSE_SWAMP"}},
      {1u,
       false,
       0,
       0xacccaba9u,
       {"MM_TEMPLE_STONE_TOWER_INVERTED", "MM_SPIDER_HOUSE_OCEAN",
        "MM_TEMPLE_GREAT_BAY", "MM_TEMPLE_STONE_TOWER"}},
      {2u,
       true,
       5,
       0x46757c1fu,
       {"MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_WOODFALL", "MM_SPIDER_HOUSE_SWAMP",
        "MM_TEMPLE_GREAT_BAY"}},
      {2u,
       false,
       0,
       0x56b8f3c3u,
       {"MM_TEMPLE_STONE_TOWER_INVERTED", "MM_TEMPLE_WOODFALL",
        "MM_TEMPLE_GREAT_BAY", "MM_SPIDER_HOUSE_OCEAN"}},
      {3u,
       true,
       4,
       0x2650bd4au,
       {"MM_TEMPLE_WOODFALL", "MM_TEMPLE_STONE_TOWER", "MM_TEMPLE_SNOWHEAD",
        "MM_SPIDER_HOUSE_SWAMP"}},
      {3u,
       false,
       0,
       0x2650bd4au,
       {"MM_TEMPLE_WOODFALL", "MM_TEMPLE_STONE_TOWER", "MM_TEMPLE_SNOWHEAD",
        "MM_SPIDER_HOUSE_SWAMP"}},
      {21u,
       true,
       4,
       0x1127d403u,
       {"MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_GREAT_BAY", "MM_TEMPLE_WOODFALL",
        "MM_SPIDER_HOUSE_SWAMP"}},
      {21u,
       false,
       0,
       0x1127d403u,
       {"MM_TEMPLE_SNOWHEAD", "MM_TEMPLE_GREAT_BAY", "MM_TEMPLE_WOODFALL",
        "MM_SPIDER_HOUSE_SWAMP"}},
      {2654435769u,
       true,
       5,
       0x4dd466ccu,
       {"MM_SPIDER_HOUSE_OCEAN", "MM_TEMPLE_STONE_TOWER_INVERTED",
        "MM_SPIDER_HOUSE_SWAMP", "MM_TEMPLE_WOODFALL"}},
      {2654435769u,
       false,
       0,
       0x8503d83bu,
       {"MM_SPIDER_HOUSE_OCEAN", "MM_TEMPLE_STONE_TOWER_INVERTED",
        "MM_TEMPLE_GREAT_BAY", "MM_TEMPLE_WOODFALL"}},
      {123456789u,
       true,
       4,
       0x64c34f26u,
       {"MM_TEMPLE_GREAT_BAY", "MM_TEMPLE_STONE_TOWER_INVERTED",
        "MM_TEMPLE_SNOWHEAD", "MM_SPIDER_HOUSE_SWAMP"}},
      {123456789u,
       false,
       0,
       0x64c34f26u,
       {"MM_TEMPLE_GREAT_BAY", "MM_TEMPLE_STONE_TOWER_INVERTED",
        "MM_TEMPLE_SNOWHEAD", "MM_SPIDER_HOUSE_SWAMP"}},
  };
  const auto oot =
      Json{{"pool", Json::array()}, {"checks", {{{"name", "Oot check"}}}}}
          .dump();
  const auto mm =
      Json{{"pool", Json::array()}, {"checks", {{{"name", "Mm check"}}}}}
          .dump();
  OracleFns o{Legacy::reset, Legacy::owned, Legacy::ootChecks, nullptr,
              Legacy::portal};
  OracleFns m{Legacy::reset, Legacy::owned, Legacy::mmChecks, nullptr};
  const NativeFns native{Legacy::apply, Legacy::assume, Legacy::clear,
                         Legacy::snapshot};
  for (const auto &row : frozen) {
    Legacy::sets = 0;
    const auto value = GenerateNativeMmWallmasters(
        true, row.seed, native, oot, mm, "", o, m, false, {},
        row.logic ? OotAccess::ALL_REACHABLE : OotAccess::NO_LOGIC);
    assert(Legacy::sets == row.sets && CwHashName(value.dump()) == row.hash);
    std::vector<std::string> destinations;
    for (const auto &r : value.at("overrides"))
      destinations.push_back(r.at("destination").get<std::string>());
    assert(destinations == row.destinations);
  }
}

// No layout in the pinned 1000 attempts: a ShuffleError, the only failure an
// outer attempt may retry (C1). From a cleared module, as each launcher attempt
// starts, the failed attempt leaves no layout installed. Its own ctest, as it
// validates 7000 candidates.
void CheckExhausted() {
  ResetAll();
  Oracles::majora = false;
  const auto error = Error<ShuffleError>([&] { Generate(Tranche(6)); });
  assert(Has(error, "Unable to place"));
  assert(FakeMm::sets == 7000 && FakeMm::clears == 7001 &&
         FakeProbe::queries == 0 && FakeProbe::releases == 1);
  assert(!ActiveMmUnified().active() && !ActiveMmUnifiedAssumptions());
}

void Run(const char *path, bool exhausted) {
  std::ifstream input(path, std::ios::binary);
  fixture = Json::parse(input);
  assert(fixture.at("upstreamCommit") ==
         std::string(LayoutV1::kUpstreamCommit));
  if (exhausted) {
    CheckExhausted();
    std::cout << "PASS unified generator: 1000 failed attempts give a "
                 "retryable ShuffleError and leave no layout installed\n";
    return;
  }
  CheckDisabledPath();
  CheckRefusals();
  CheckTemplePass();
  CheckLayouts();
  CheckCandidates();
  CheckScopes();
  CheckInstallForFill();
  CheckSeedIdentity();
  CheckNativeScopes();
  CheckProbeResolution();
  CheckFrozenPrivateGenerator();
  std::cout << "PASS unified generator: disabled path, refusals before RNG, "
               "Temple pass, pinned 4x7 candidates, deterministic valid "
               "layouts, scopes on every exit, install for fill and "
               "playthrough, seed identity, frozen private generator\n";
}
} // namespace

int main(int argc, char **argv) {
  assert(argc == 2 || (argc == 3 && std::string(argv[2]) == "exhausted"));
  Run(argv[1], argc == 3);
}
