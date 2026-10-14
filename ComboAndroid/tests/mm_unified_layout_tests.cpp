// The MM half of the unified entrance layout
// (combo/rando/entrances/MmUnifiedLayout.h) and the launcher's layout bind
// (NativeBridge.h ApplyNativeLayout): install, clear and round trip, the
// refusals (a private MM profile beside a layout, rows without an MM binding,
// capabilities), the candidate-world assumptions against the pinned M1
// contract's 4x7 MM Wallmaster table (tools/ootmm/m1-contract.json), and which
// MM Wallmaster graph the logic reads. Built twice: as shipped and with
// COMBO_ENTRANCE_DEV.
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/NativeBridge.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace ComboRando::Entrances;
using ComboRando::kEntranceSeedFileType;
using ComboRando::kEntranceSeedVersion;
using ComboRando::kLegacyEntranceKeys;
using ComboRando::UnsupportedComboSeedFormat;
using Json = nlohmann::json;
using Strings = std::set<std::string>;
using Targets = std::map<std::string, Strings>; // source -> destinations

namespace {
Json fixture;
const std::string kWallmasters = "erWallmasters=ownGame;erOneWays=none";
const std::string kWidest =
    "erWallmasters=ownGame;erOneWays=ownGame;erOneWaysMajor;erOneWaysIkana;"
    "erOneWaysOwls;erOneWaysWaterVoids";
const Overrides kGameLinks = {
    {"OOT_SHOP_MASKS", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER"},
    {"MM_CLOCK_TOWER_FROM_CLOCK_TOWN", "OOT_MARKET_FROM_MASK_SHOP"}};

const Json &Combination(const std::string &id) {
  for (const auto &c : fixture.at("combinations"))
    if (c.at("id") == id)
      return c;
  throw std::invalid_argument("No fixture combination " + id);
}
Settings FromFixture(const Json &j) {
  const auto mode = [](const Json &value) {
    return value == "none" ? Mode::None : Mode::OwnGame;
  };
  Settings s;
  s.erWallmasters = mode(j.at("erWallmasters"));
  s.erOneWays = mode(j.at("erOneWays"));
  s.erOneWaysMajor = j.at("erOneWaysMajor").get<bool>();
  s.erOneWaysIkana = j.at("erOneWaysIkana").get<bool>();
  s.erOneWaysOwls = j.at("erOneWaysOwls").get<bool>();
  s.erOneWaysWaterVoids = j.at("erOneWaysWaterVoids").get<bool>();
  s.erOneWaysAnywhere = j.at("erOneWaysAnywhere").get<bool>();
  return s;
}
std::string Requirement(const Settings &s) {
  if (s.erWallmasters != Mode::None && s.erOneWays != Mode::None)
    return "wallmasters-with-one-ways-v1";
  return s.erWallmasters != Mode::None ? "wallmasters-own-game-v1"
                                       : "one-ways-own-game-v1";
}
CapabilitySet WholeCatalog(bool devBuild = true) {
  auto caps = CapabilitySet::AtLeast(Stage::Hidden);
  caps.devBuild = devBuild;
  return caps;
}

// The fixture's committed sample layout of one combination; with `tranche`,
// only its MM routes under the dev-only MM tranche id.
EntranceLayout SampleLayout(const std::string &id, bool tranche = false) {
  const auto &combination = Combination(id);
  EntranceLayout layout;
  layout.settings = FromFixture(combination.at("settings"));
  layout.required = {tranche ? std::string(kMmTrancheCapability)
                             : Requirement(layout.settings)};
  Overrides overrides;
  for (const auto &row : combination.at("sampleLayout")) {
    const auto *src = LayoutV1::FindRow(row[0].get<std::string>());
    if (!tranche || src->game == Game::Mm)
      overrides[src->name] = LayoutV1::FindRow(row[1].get<std::string>())->name;
  }
  layout.overrides = std::move(overrides);
  return layout;
}
Json Seed(const EntranceLayout &layout) {
  return {{"fileType", kEntranceSeedFileType},
          {"version", kEntranceSeedVersion},
          {"entranceLayout", EncodeEntranceLayout(layout)}};
}
Json Seed(const Overrides &routes) {
  auto layout = SampleLayout(kWallmasters, true);
  layout.overrides = routes;
  return Seed(layout);
}
std::string Name(Key key) { return std::string(Find(key.game, key.id)->name); }

template <class F> std::string Error(F action) {
  try {
    action();
  } catch (const std::exception &error) {
    return error.what();
  }
  assert(!"an error was expected");
  return {};
}
bool Has(const std::string &text, const char *part) {
  return text.find(part) != std::string::npos;
}

// The fixture's per-source candidates of the MM Wallmaster sources.
Targets PinnedMmWallmasterTargets(const std::string &id) {
  Targets result;
  for (const auto &pool : Combination(id).at("pools"))
    if (pool.at("name") == "WALLMASTERS")
      for (const auto &group : pool.at("candidates"))
        for (const auto &src : group.at("sources"))
          if (src.get<std::string>().starts_with("MM_"))
            for (const auto &dst : group.at("destinations"))
              result[src.get<std::string>()].insert(dst.get<std::string>());
  return result;
}
Targets ByName(const MmUnifiedRoutes &routes) {
  Targets result;
  for (const auto &[source, list] : routes)
    for (const auto &route : list)
      result[Name({Game::Mm, source})].insert(Name(route.destination));
  return result;
}

// The installed layout: the MM routes of the envelope, one per source, and
// the envelope back unchanged.
void CheckInstall() {
  const auto seed = Seed(SampleLayout(kWallmasters, true));
  MmUnifiedLayout layout;
  assert(!layout.active() && layout.json().is_null() &&
         layout.routes().empty());
  layout.install(seed, WholeCatalog());
  assert(layout.active() && layout.routes().size() == 4);
  for (const auto &row : Combination(kWallmasters).at("sampleLayout")) {
    const auto *src = Find(row[0].get<std::string>());
    if (src->game != Game::Mm)
      continue;
    const auto *dst = Find(row[1].get<std::string>());
    const auto routes = layout.routesFrom(src->id);
    assert(routes.size() == 1 &&
           (routes[0].destination == Key{Game::Mm, dst->id}));
    assert(!routes[0].resetTime && !routes[0].toVoid);
  }
  assert(layout.routesFrom(0xff04u).empty());
  // Round trip: the dump is the envelope, byte for byte, and installs again.
  assert(layout.json() == seed.at("entranceLayout") &&
         layout.json().dump() == seed.at("entranceLayout").dump());
  MmUnifiedLayout again;
  again.install(Json{{"entranceLayout", layout.json()}}, WholeCatalog());
  assert(again.json() == layout.json() && again.routes() == layout.routes());
  layout.clear();
  assert(!layout.active() && layout.json().is_null() &&
         layout.routes().empty());

  // Each MM source takes each of its seven arrivals (no reuse in the pool).
  for (size_t shift = 0; shift < kMmBaseWallmasterDestinations.size();
       ++shift) {
    Overrides routes;
    for (size_t i = 0; i < kMmWallmasterSources.size(); ++i)
      routes[kMmWallmasterSources[i]] =
          kMmBaseWallmasterDestinations[(i + shift) %
                                        kMmBaseWallmasterDestinations.size()];
    MmUnifiedLayout rotated;
    rotated.install(Seed(routes), WholeCatalog());
    for (const auto &[src, dst] : routes)
      assert(rotated.routesFrom(Find(src)->id) ==
             std::vector<MmUnifiedRoute>{MakeMmUnifiedRoute(*Find(dst))});
  }
  // The MM half only: a layout with OoT routes installs its MM routes, and a
  // layout without MM routes installs none.
  MmUnifiedLayout both;
  both.install(Seed(SampleLayout(kWallmasters)), WholeCatalog());
  assert(both.routes().size() == 4);
  MmUnifiedLayout owls;
  owls.install(Seed(SampleLayout("erWallmasters=none;erOneWays=ownGame;"
                                 "erOneWaysOwls")),
               WholeCatalog());
  assert(owls.active() && owls.routes().empty());
}

// Refusals leave the installed layout as it was.
void CheckRefusals() {
  const auto good = Seed(SampleLayout(kWallmasters, true));
  MmUnifiedLayout layout;
  layout.install(good, WholeCatalog());
  const auto before = layout.json();
  const auto refused = [&](const Json &seed, const CapabilitySet &caps,
                           const char *reason) {
    const auto error = Error([&] { layout.install(seed, caps); });
    assert(Has(error, reason));
    assert(layout.active() && layout.json() == before);
  };
  for (const char *key : kLegacyEntranceKeys) {
    auto both = good;
    both[key] = Json::object();
    refused(both, WholeCatalog(), "cannot be combined");
  }
  refused(Json::object(), WholeCatalog(), "no entrance layout");
  refused(Json::array(), WholeCatalog(), "no entrance layout");
  for (const auto &bad : {Json(), Json::object(), Json::array(), Json(1)})
    refused({{"entranceLayout", bad}}, WholeCatalog(), "Entrance layout");
  // This build has no capability in Phase 0, dev build or not; the tranche is
  // used only by its own id, and a dev layout never by a release build.
  assert(ActiveCapabilities().Empty());
  refused(good, ActiveCapabilities(), "does not have");
  refused(good, WholeCatalog().WithoutDevOnly(), "does not have");
  auto devLayout = SampleLayout(kWallmasters, true);
  devLayout.devBuild = true;
  layout.install(Seed(devLayout), WholeCatalog());
  layout.install(good, WholeCatalog());
  refused(Seed(devLayout), WholeCatalog(false), "dev build");
  // The validator's refusals reach the installer: a foreign destination.
  refused(Seed(Overrides{{"MM_WALLMASTER_BTW_ENTRANCE", "OOT_DEKU_TREE"},
                         {"MM_WALLMASTER_BTW_EXIT", "MM_TEMPLE_SNOWHEAD"},
                         {"MM_WALLMASTER_BTW_FOUNTAIN", "MM_TEMPLE_GREAT_BAY"},
                         {"MM_WALLMASTER_DAMPE", "MM_SPIDER_HOUSE_SWAMP"}}),
          WholeCatalog(), "cannot route");
  // Rows without an MM binding yet: the one-way rows, which every layout with
  // an MM one-way flag routes (the assumptions also test them as arrivals).
  refused(Seed(SampleLayout(kWidest)), WholeCatalog(), "no unified MM binding");
  refused(
      Seed(SampleLayout("erWallmasters=none;erOneWays=ownGame;erOneWaysIkana")),
      WholeCatalog(), "no unified MM binding");
  // With Anywhere a one-way source may land on a bound arrival; its source
  // still has no MM binding.
  auto anywhere = SampleLayout(
      "erWallmasters=none;erOneWays=ownGame;erOneWaysIkana;erOneWaysAnywhere");
  anywhere.overrides =
      Overrides{{"MM_IKANA_CASTLE_INTERIOR_BLOCK", "MM_TEMPLE_WOODFALL"},
                {"MM_IKANA_CASTLE_INTERIOR_KEG", "MM_TEMPLE_SNOWHEAD"}};
  assert(Validate(anywhere, WholeCatalog()).mm.size() == 2);
  refused(Seed(anywhere), WholeCatalog(), "no unified MM binding");
}

// One candidate world's MM overlay against the pinned 4x7 table.
MmUnifiedAssumptions Decoded(const CandidateWorld &world) {
  return DecodeMmUnifiedAssumptions(Json::parse(EncodeMmOverlay(world)));
}
void CheckAssumptions() {
  const auto pinned = PinnedMmWallmasterTargets(kWallmasters);
  assert(pinned.size() == 4);
  for (const auto &[src, dsts] : pinned)
    assert(dsts.size() == 7 && Contains(kMmWallmasterSources, src));
  const auto s = FromFixture(Combination(kWallmasters).at("settings"));
  // The tranche (OoT Wallmaster areas absent) and the whole pool give the
  // same MM part; every MM edge keeps no-sot and leads into a real area.
  for (bool tranche : {true, false}) {
    const auto pools = LayoutV1::MakePools(s, OotQuests{}, tranche);
    const auto world = BuildCandidateWorld(kGameLinks, pools, s);
    const auto decoded = Decoded(world);
    assert(ByName(decoded.routes) == pinned);
    assert(decoded.world == CandidateWorldToken(world));
    for (const auto &[source, routes] : decoded.routes)
      for (const auto &route : routes)
        assert(route.destination.game == Game::Mm && !route.resetTime &&
               !route.toVoid);
  }
  // A committed layout: one fixed edge per source.
  const auto committed = SampleLayout(kWallmasters, true);
  auto overrides = kGameLinks;
  overrides.insert(committed.overrides->begin(), committed.overrides->end());
  const auto fixed = Decoded(BuildCandidateWorld(overrides, {}, s));
  MmUnifiedLayout installed;
  installed.install(Seed(committed), WholeCatalog());
  assert(fixed.routes == installed.routes());
  // Part placed, part open: the placed source keeps its one edge, the others
  // every assignable one left; a pair both fixed and open stays twice (OR).
  auto partial = kGameLinks;
  partial["MM_WALLMASTER_DAMPE"] = "MM_TEMPLE_WOODFALL";
  Pool rest{"WALLMASTERS", {}, {}, true, true};
  for (const auto src : kMmWallmasterSources)
    rest.src.insert(src);
  for (const auto dst : kMmBaseWallmasterDestinations)
    rest.dst.insert(dst);
  const auto open = Decoded(BuildCandidateWorld(partial, {rest}, s));
  const auto dampe = open.routes.at(0xff00u);
  assert(dampe.size() == 8 &&
         std::count(dampe.begin(), dampe.end(),
                    MakeMmUnifiedRoute(*Find("MM_TEMPLE_WOODFALL"))) == 2);
  rest.src.erase("MM_WALLMASTER_DAMPE");
  rest.dst.erase("MM_TEMPLE_WOODFALL");
  const auto placed = Decoded(BuildCandidateWorld(partial, {rest}, s));
  assert(placed.routes.at(0xff00u).size() == 1 &&
         placed.routes.at(0xff01u).size() == 6);

  // Refusals.
  const auto good = Json::parse(EncodeMmOverlay(BuildCandidateWorld(
      kGameLinks, LayoutV1::MakePools(s, OotQuests{}, true), s)));
  const auto refused = [](const Json &value, const char *reason) {
    assert(Has(Error([&] { DecodeMmUnifiedAssumptions(value); }), reason));
  };
  DecodeMmUnifiedAssumptions(good);
  for (const auto &item : good.items()) {
    auto missing = good;
    missing.erase(item.key());
    refused(missing, "Malformed");
  }
  auto extra = good;
  extra["schemaVersion"] = 1;
  refused(extra, "Malformed");
  extra.erase("assumed");
  refused(extra, "miss assumed");
  for (const auto &[key, value] : std::vector<std::pair<std::string, Json>>{
           {"profile", "mm-wallmasters-own-game-v1"},
           {"game", "oot"},
           {"registry", "0"},
           {"profile", 1}}) {
    auto bad = good;
    bad[key] = value;
    refused(bad, "Unsupported MM entrance assumptions");
  }
  for (const Json &world :
       {Json(-1), Json(0x100000000ull), Json("1"), Json()}) {
    auto bad = good;
    bad["world"] = world;
    refused(bad, "candidate world token");
  }
  auto assumed = good;
  assumed["assumed"] = {"MM_TEMPLE_WOODFALL"};
  refused(assumed, "ASSUMED");
  auto twice = good;
  twice["deleted"].push_back(twice["deleted"][0]);
  refused(twice, "twice");
  auto unknown = good;
  unknown["deleted"][0]["row"] = "OOT_WALLMASTER_GTG";
  refused(unknown, "unknown MM row");
  auto unmarked = good;
  unmarked["deleted"][0]["void"] = false;
  refused(unmarked, "wrongly");
  auto reset = good;
  reset["remaining"][0]["resetTime"] = true;
  refused(reset, "wrongly");
  auto intoVoid = good;
  intoVoid["remaining"][0]["void"] = true;
  refused(intoVoid, "wrongly");
  auto flag = good;
  flag["remaining"][0]["void"] = 0;
  refused(flag, "booleans");
  auto undeleted = good;
  undeleted["deleted"].erase(0);
  refused(undeleted, "without deleting");
  // A foreign arrival, and the one-way and water-void arrivals that only
  // one-ways add to the Wallmaster pool.
  for (const char *arrival :
       {"OOT_DEKU_TREE", "MM_SWAMP_FROM_IKANA_CANYON", "MM_VOID_GREAT_BAY"}) {
    auto unbound = good;
    unbound["remaining"][0]["dst"] = arrival;
    refused(unbound, "no unified MM binding");
  }
  auto nowhere = good;
  nowhere["remaining"][0]["dst"] = "NOT_A_ROW";
  refused(nowhere, "unknown destination");
  // One-ways: a Wallmaster sent to a one-way or water-void arrival, a deleted
  // one-way exit, a deleted water-void source.
  for (const char *id :
       {kWidest.c_str(), "erWallmasters=none;erOneWays=ownGame;erOneWaysIkana",
        "erWallmasters=none;erOneWays=ownGame;erOneWaysWaterVoids"}) {
    const auto ws = FromFixture(Combination(id).at("settings"));
    refused(Json::parse(EncodeMmOverlay(BuildCandidateWorld(
                kGameLinks, LayoutV1::MakePools(ws, OotQuests{}, false), ws))),
            "no unified MM binding");
  }
  // OoT one-ways alone leave nothing to the MM overlay.
  const auto owls = FromFixture(
      Combination("erWallmasters=none;erOneWays=ownGame;erOneWaysOwls")
          .at("settings"));
  assert(Decoded(BuildCandidateWorld(
                     kGameLinks, LayoutV1::MakePools(owls, OotQuests{}, false),
                     owls))
             .routes.empty());
  // The OoT overlay is not the MM one.
  refused(Json::parse(EncodeOotOverlay(BuildCandidateWorld(
              kGameLinks, LayoutV1::MakePools(s, OotQuests{}, false), s))),
          "Unsupported MM entrance assumptions");
}

// Candidate assumptions before an installed layout, whichever kind.
void CheckGraph() {
  auto &unified = ActiveMmUnified();
  auto &legacy = ActiveMmWallmasters();
  auto &unifiedAssumed = ActiveMmUnifiedAssumptions();
  auto &legacyAssumed = ActiveMmWallmasterAssumptions();
  const auto reset = [&] {
    unified.clear();
    legacy.clear();
    unifiedAssumed.reset();
    legacyAssumed.reset();
  };
  reset();
  assert(!MmUnifiedGraphActive() && MmUnifiedGraphRoutes(0xff00u).empty());
  std::vector<SavedOverride> rows;
  for (size_t i = 0; i < kMmWallmasterSources.size(); ++i) {
    const auto *src = Find(kMmWallmasterSources[i]);
    const auto *dst = Find(kMmBaseWallmasterDestinations[6 - i]);
    rows.push_back({std::string(src->name), std::string(dst->name), src->game,
                    dst->game, src->id, dst->id});
  }
  legacy.install(rows);
  assert(!MmUnifiedGraphActive());
  legacy.clear();
  unified.install(Seed(SampleLayout(kWallmasters, true)), WholeCatalog());
  assert(MmUnifiedGraphActive() &&
         MmUnifiedGraphRoutes(0xff00u) == unified.routesFrom(0xff00u));
  legacyAssumed = BuildMmWallmasterAssumptions({}, {});
  assert(!MmUnifiedGraphActive());
  legacyAssumed.reset();
  MmUnifiedAssumptions candidate;
  candidate.routes[0xff00u] = {MakeMmUnifiedRoute(*Find("MM_TEMPLE_SNOWHEAD")),
                               MakeMmUnifiedRoute(*Find("MM_TEMPLE_WOODFALL"))};
  unifiedAssumed = candidate;
  assert(MmUnifiedGraphActive() &&
         MmUnifiedGraphRoutes(0xff00u) == candidate.routes.at(0xff00u) &&
         MmUnifiedGraphRoutes(0xff01u).empty());
  unified.clear();
  legacy.install(rows);
  assert(MmUnifiedGraphActive() && MmUnifiedGraphRoutes(0xff00u).size() == 2);
  reset();
}

// A fake MM module with the export's install rules (the Python harness runs
// the real exports).
struct FakeMm {
  static inline MmUnifiedLayout unified;
  static inline std::optional<std::vector<SavedOverride>> legacy;
  static inline std::string dump;
  static inline int applies = 0;
  static inline bool refuse = false, lose = false, drift = false,
                     beside = false;
  static int apply(const char *text) {
    ++applies;
    unified.clear();
    legacy.reset();
    if (refuse)
      return 0;
    const auto seed = Json::parse(text);
    try {
      if (seed.contains("entranceLayout"))
        unified.install(seed, WholeCatalog());
      else
        legacy = DecodeMmWallmasterSeed(seed);
    } catch (const std::exception &) {
      unified.clear();
      return 0;
    }
    return 1;
  }
  static const char *snapshot() {
    if (lose)
      return nullptr;
    Json seed = Json::object();
    if (unified.active())
      seed["entranceLayout"] = unified.json();
    if (legacy)
      seed["entranceExpansion"] = EncodeMmWallmasterExtension(*legacy);
    if (drift && seed.contains("entranceLayout"))
      seed["entranceLayout"]["devBuild"] = true;
    if (beside)
      seed["grottoEntranceExpansion"] = Json::object();
    dump = seed.dump();
    return dump.c_str();
  }
};

void CheckBind() {
  const NativeFns native{FakeMm::apply, nullptr, nullptr, FakeMm::snapshot};
  const auto seed = Seed(SampleLayout(kWallmasters, true));
  const auto bindError = [&](const Json &value, const NativeFns &fns,
                             const CapabilitySet &caps) {
    return Error([&] { ApplyNativeSeed(fns, value, caps); });
  };
  // This build refuses before decoding and never reaches the MM module.
  assert(bindError(seed, native, ActiveCapabilities()) ==
         UnsupportedComboSeedFormat().what());
  assert(FakeMm::applies == 0);
  // A build with the tranche binds the MM routes and checks the round trip.
  ApplyNativeSeed(native, seed, WholeCatalog());
  assert(FakeMm::applies == 1 && FakeMm::unified.routes().size() == 4 &&
         FakeMm::unified.json() == seed.at("entranceLayout"));
  // No OoT installer yet: any OoT route refuses the bind before the MM module.
  for (const char *id : {kWallmasters.c_str(),
                         "erWallmasters=none;erOneWays=ownGame;erOneWaysOwls"})
    assert(bindError(Seed(SampleLayout(id)), native, WholeCatalog()) ==
           "OoT entrance layout installer unavailable");
  assert(FakeMm::applies == 1);
  assert(bindError(seed, {}, WholeCatalog()) ==
         "MM entrance layout exports unavailable");
  assert(bindError(seed, {FakeMm::apply, nullptr, nullptr, nullptr},
                   WholeCatalog()) == "MM entrance layout exports unavailable");
  const auto failing = [&](bool &switch_, const char *message) {
    switch_ = true;
    assert(bindError(seed, native, WholeCatalog()) == message);
    switch_ = false;
  };
  failing(FakeMm::refuse, "MM entrance layout rejected");
  failing(FakeMm::lose, "MM entrance layout roundtrip failed");
  failing(FakeMm::drift, "MM entrance layout ignored or changed");
  failing(FakeMm::beside,
          "MM entrance layout installed beside grottoEntranceExpansion");
  // A module that installs nothing for the key.
  const NativeFns ignoring{[](const char *) { return 1; }, nullptr, nullptr,
                           [] { return "{}"; }};
  assert(bindError(seed, ignoring, WholeCatalog()) ==
         "MM entrance layout ignored or changed");
  // The seed guard's refusals come first: a private profile beside the layout.
  auto both = seed;
  both["entranceExpansion"] = Json::object();
  assert(Has(bindError(both, native, WholeCatalog()), "cannot be combined"));
  // Private-profile seeds and the clear still bind as before.
  std::vector<SavedOverride> rows;
  for (size_t i = 0; i < kMmWallmasterSources.size(); ++i) {
    const auto *src = Find(kMmWallmasterSources[i]);
    const auto *dst = Find(kMmBaseWallmasterDestinations[i]);
    rows.push_back({std::string(src->name), std::string(dst->name), src->game,
                    dst->game, src->id, dst->id});
  }
  ApplyNativeSeed(native,
                  {{"entranceExpansion", EncodeMmWallmasterExtension(rows)}},
                  ActiveCapabilities());
  assert(FakeMm::legacy && !FakeMm::unified.active());
  ApplyNativeSeed(native, Json::object(), ActiveCapabilities());
  assert(!FakeMm::legacy && !FakeMm::unified.active());
}

void Run(const char *path) {
  std::ifstream input(path, std::ios::binary);
  fixture = Json::parse(input);
  assert(fixture.at("upstreamCommit") ==
         std::string(LayoutV1::kUpstreamCommit));
#ifdef COMBO_ENTRANCE_DEV
  assert(ActiveCapabilities().devBuild);
#else
  assert(!ActiveCapabilities().devBuild);
#endif
  CheckInstall();
  CheckRefusals();
  CheckAssumptions();
  CheckGraph();
  CheckBind();
  std::cout << "PASS MM unified layout: install, clear, round trip, "
               "refusals, 4x7 pinned assumptions, graph precedence, bind\n";
}
} // namespace

int main(int argc, char **argv) {
  assert(argc == 2);
  Run(argv[1]);
}
