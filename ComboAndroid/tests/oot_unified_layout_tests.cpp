// Vanilla scope, the Master Quest contexts of the quests id (U11), frozen-pool
// coverage, candidate precedence and two-module rollback.
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
static CapabilitySet Caps() { return CapabilitySet::AtLeast(Stage::Hidden); }
template <class F> static void Reject(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const std::exception &) {
    rejected = true;
  }
  assert(rejected);
}
static EntranceLayout Sample(const Json &fixture) {
  EntranceLayout result;
  result.required = {std::string(kVanillaWallmasterCapability)};
  result.settings.erWallmasters = Mode::OwnGame;
  Overrides rows;
  for (const auto &c : fixture.at("combinations"))
    if (c.at("id") == "erWallmasters=ownGame;erOneWays=none")
      for (const auto &pair : c.at("sampleLayout"))
        rows[Find(pair[0].get<std::string>())->name] =
            Find(pair[1].get<std::string>())->name;
  assert(rows.size() == 13);
  result.overrides = rows;
  return result;
}
static Json Seed(const EntranceLayout &layout) {
  return {{"entranceLayout", EncodeEntranceLayout(layout)}};
}
// Installers deliberately clear before rejection, like the real native exports.
// Failure is one-shot, allowing the transaction to restore the old layout
// through the same exports.
struct Fake {
  static inline MmUnifiedLayout mm;
  static inline OotUnifiedLayout oot;
  static inline bool failMm = false, failOot = false, driftOot = false,
                     driftOnApply = false;
  static inline bool mmAssumed = false, ootAssumed = false,
                     failMmAssumed = false, failOotAssumed = false;
  static inline std::string mmText, ootText;
  static int ApplyMm(const char *text) {
    mm.clear();
    if (std::exchange(failMm, false))
      return 0;
    const auto value = Json::parse(text);
    if (value.contains("entranceLayout"))
      mm.install(value, Caps());
    return 1;
  }
  static int ApplyOot(const char *text) {
    oot.clear();
    if (std::exchange(failOot, false))
      return 0;
    const auto value = Json::parse(text);
    if (value.contains("entranceLayout"))
      oot.install(value, Caps());
    driftOot = std::exchange(driftOnApply, false);
    return 1;
  }
  static const char *DumpMm() {
    mmText =
        (mm.active() ? Seed(DecodeEntranceLayout(mm.json())) : Json::object())
            .dump();
    return mmText.c_str();
  }
  static const char *DumpOot() {
    auto value =
        oot.active() ? Seed(DecodeEntranceLayout(oot.json())) : Json::object();
    if (std::exchange(driftOot, false) && value.contains("entranceLayout"))
      value["entranceLayout"]["devBuild"] = true;
    ootText = value.dump();
    return ootText.c_str();
  }
  static int AssumeMm(const char *) {
    mmAssumed = true;
    return !failMmAssumed;
  }
  static int AssumeOot(const char *) {
    ootAssumed = true;
    return !failOotAssumed;
  }
  static void ClearMm() { mmAssumed = false; }
  static void ClearOot() { ootAssumed = false; }
  static NativeFns Fns() {
    return {ApplyMm,  AssumeMm,  ClearMm,  DumpMm,  nullptr,
            ApplyOot, AssumeOot, ClearOot, DumpOot, nullptr};
  }
};
int main(int argc, char **argv) {
  assert(argc == 2);
  std::ifstream input(argv[1]);
  const auto fixture = Json::parse(input);
  const auto layout = Sample(fixture);
  const auto seed = Seed(layout);
  // A shipping build admits the vanilla contract (P1-05): a v2 seed binds in
  // both games through the launcher, and a dev layout is refused.
  const auto shipping = ActiveCapabilities();
  assert(shipping.Active(kVanillaWallmasterCapability) && !shipping.devBuild);
  Validate(layout, shipping);
  const Json v2{{"fileType", ComboRando::kEntranceSeedFileType},
                {"version", ComboRando::kEntranceSeedVersion},
                {"entranceLayout", EncodeEntranceLayout(layout)}};
  ApplyNativeSeed(Fake::Fns(), v2, shipping);
  assert(Fake::mm.routes().size() == 4 && Fake::oot.routes().size() == 9);
  ApplyNativeSeed(Fake::Fns(), Json::object(), shipping);
  assert(!Fake::mm.active() && !Fake::oot.active());
  auto devLayout = layout;
  devLayout.devBuild = true;
  Reject([&] { Validate(devLayout, shipping); });
  OotUnifiedLayout installed;
  installed.install(seed, Caps());
  assert(installed.routes().size() == 9 &&
         installed.json() == seed.at("entranceLayout"));
  for (const auto &[src, dst] : *layout.overrides)
    if (Find(src)->game == Game::Oot)
      assert(installed.routesFrom(Find(src)->id) ==
             std::vector<OotUnifiedRoute>{{Find(dst)->id}});
  // With the frozen vanilla id every nonempty MQ context fails, including
  // dungeons without a capture source. No pool shrinking.
  for (unsigned mask = 1; mask < 4096; ++mask) {
    auto mq = layout;
    for (size_t i = 0; i < mq.ootMq.size(); ++i)
      mq.ootMq[i] = (mask & (1u << i)) != 0;
    Reject([&] { Validate(mq, Caps()); });
    Reject([&] { installed.install(Seed(mq), Caps()); });
    assert(installed.json() == seed.at("entranceLayout"));
  }
  // The quests id binds every MQ context its sources can fill: each OoT source
  // of the context's own pool to a dungeon entrance of its own, the MM routes
  // kept. Exactly the contexts the pinned contract lists as capacity shortfalls
  // have more OoT sources than entrances, so no layout at all; a build without
  // the quests id refuses every one of these layouts.
  std::set<std::set<std::string>> shortfalls;
  for (const auto &row : fixture.at("mqSources").at("capacityShortfalls"))
    shortfalls.insert(row.at("mq").get<std::set<std::string>>());
  const auto dungeons =
      fixture.at("mqSources").at("dungeons").get<std::vector<std::string>>();
  const auto vanillaOnly = Caps().Only(kVanillaWallmasterCapability);
  size_t bound = 0, short_ = 0;
  for (unsigned mask = 1; mask < 4096; ++mask) {
    auto mq = layout;
    mq.required = {std::string(kQuestsWallmasterCapability)};
    std::set<std::string> projection;
    for (size_t i = 0; i < mq.ootMq.size(); ++i)
      mq.ootMq[i] = (mask & (1u << i)) != 0;
    for (const auto &dungeon : dungeons)
      if (mq.ootMq[OotMqIndex(dungeon)])
        projection.insert(dungeon);
    Overrides rows;
    for (const auto &[src, dst] : *layout.overrides)
      if (Find(src)->game == Game::Mm)
        rows[src] = dst;
    std::vector<std::string_view> sources;
    for (const auto &pool : LayoutV1::MakePools(mq.settings, mq.ootMq, false))
      for (const auto src : pool.src)
        if (Find(src)->game == Game::Oot)
          sources.push_back(src);
    for (size_t i = 0; i < sources.size(); ++i)
      rows[sources[i]] =
          kOotWallmasterArrivalRows[i % kOotWallmasterArrivalRows.size()];
    mq.overrides = rows;
    if (sources.size() > kOotWallmasterArrivalRows.size()) {
      assert(shortfalls.contains(projection) && sources.size() == 12);
      Reject([&] { Validate(mq, Caps()); }); // one entrance taken twice
      ++short_;
      continue;
    }
    assert(!shortfalls.contains(projection));
    Validate(mq, Caps());
    OotUnifiedLayout mqInstalled;
    mqInstalled.install(Seed(mq), Caps());
    assert(mqInstalled.routes().size() == sources.size() &&
           *mqInstalled.quests() == mq.ootMq);
    for (const auto src : sources)
      assert(OotUnifiedSourceInQuests(*Find(src), mq.ootMq) &&
             mqInstalled.routesFrom(Find(src)->id).size() == 1);
    OotUnifiedLayout refusedInstall;
    Reject([&] { refusedInstall.install(Seed(mq), vanillaOnly); });
    assert(!refusedInstall.active());
    // The songs id (limits design, Stage 1) binds the same routes in every
    // context; its song facts are the seed's (ComboSeedFormat.h).
    if (mask % 16 == 1) {
      auto songs = mq;
      songs.required = {std::string(kSongsWallmasterCapability)};
      OotUnifiedLayout songsInstalled;
      songsInstalled.install(Seed(songs), Caps());
      assert(songsInstalled.routes() == mqInstalled.routes() &&
             *songsInstalled.quests() == mq.ootMq);
      Reject([&] { refusedInstall.install(Seed(songs), vanillaOnly); });
    }
    ++bound;
  }
  assert(short_ == 256 && bound == 4095 - 256);
  // The songs id on all-vanilla dungeons binds the vanilla id's routes, and
  // admits a Master Quest context before any layout exists, as the quests id
  // does; the vanilla id alone does not.
  auto vanillaSongs = layout;
  vanillaSongs.required = {std::string(kSongsWallmasterCapability)};
  OotUnifiedLayout songsInstalled;
  songsInstalled.install(Seed(vanillaSongs), Caps());
  assert(songsInstalled.routes() == installed.routes());
  OotQuests forestMq{};
  forestMq[OotMqIndex("Forest")] = true;
  RequireAdmittedOotQuests(forestMq, Caps().Only(kSongsWallmasterCapability));
  RequireAdmittedOotQuests(forestMq, Caps().Only(kQuestsWallmasterCapability));
  Reject([&] { RequireAdmittedOotQuests(forestMq, vanillaOnly); });
  // The quests id never names an all-vanilla layout: those keep the vanilla id.
  auto vanillaQuests = layout;
  vanillaQuests.required = {std::string(kQuestsWallmasterCapability)};
  Reject([&] { Validate(vanillaQuests, Caps()); });
  for (const char *key : ComboRando::kLegacyEntranceKeys) {
    auto mixed = seed;
    mixed[key] = Json::object();
    Reject([&] { installed.install(mixed, Caps()); });
  }
  auto oldCapability = layout;
  oldCapability.required = {"wallmasters-own-game-v1"};
  Reject([&] { installed.install(Seed(oldCapability), Caps()); });
  const auto pools = LayoutV1::MakePools(layout.settings, {}, false);
  const auto candidate = BuildCandidateWorld({}, pools, layout.settings);
  const auto overlay = Json::parse(EncodeOotOverlay(candidate));
  const auto decoded = DecodeOotUnifiedAssumptions(overlay, {});
  assert(decoded.routes.size() == 9 &&
         decoded.world == CandidateWorldToken(candidate));
  for (const auto &[src, routes] : decoded.routes) {
    assert(OotUnifiedSourceBound(Find(Game::Oot, src)->name) &&
           routes.size() == 11);
    std::set<uint32_t> destinations;
    for (const auto &route : routes)
      destinations.insert(route.destination);
    assert(destinations.size() == 11);
  }
  // Fixed and remaining edges are ORed, including a duplicate. Empty
  // assumptions suppress committed routes.
  auto duplicate = overlay;
  duplicate["fixed"].push_back(duplicate["remaining"][0]);
  const auto src =
      Find(duplicate["remaining"][0]["src"].get<std::string>())->id;
  assert(DecodeOotUnifiedAssumptions(duplicate, {}).routes.at(src).size() ==
         12);
  ActiveOotUnified() = installed;
  ActiveOotUnifiedAssumptions() = decoded;
  assert(OotUnifiedGraphRoutes(src).size() == 11);
  ActiveOotUnifiedAssumptions() = OotUnifiedAssumptions{};
  assert(OotUnifiedGraphRoutes(src).empty());
  ActiveOotUnifiedAssumptions().reset();
  assert(OotUnifiedGraphRoutes(src).size() == 1);
  for (const auto &field : overlay.items()) {
    auto bad = overlay;
    bad.erase(field.key());
    Reject([&] { DecodeOotUnifiedAssumptions(bad, {}); });
  }
  for (int change = 0; change < 12; ++change) {
    auto bad = overlay;
    switch (change) {
    case 0:
      bad["world"] = -1;
      break;
    case 1:
      bad["world"] = 0x100000000ull;
      break;
    case 2:
      bad["assumed"] = {"OOT_DEKU_TREE"};
      break;
    case 3:
      bad["game"] = "mm";
      break;
    case 4:
      bad["deleted"].push_back(bad["deleted"][0]);
      break;
    case 5:
      bad["deleted"][0]["void"] = false;
      break;
    case 6:
      bad["deleted"].erase(0);
      break;
    case 7:
      bad["remaining"][0]["resetTime"] = true;
      break;
    case 8:
      bad["remaining"][0]["void"] = 0;
      break;
    case 9:
      bad["remaining"][0]["dst"] = "MM_TEMPLE_WOODFALL";
      break;
    case 10:
      bad["deleted"][0]["row"] = "OOT_WALLMASTER_SPIRIT_STATUE";
      break;
    case 11:
      bad["registry"] = "wrong";
      break;
    }
    // Case 9 is refused only without the Full id (below).
    Reject([&] { DecodeOotUnifiedAssumptions(bad, {}, vanillaOnly); });
  }
  // With the Full id (limits design, Stage 3), an MM dungeon of the Full pool
  // is a foreign route, kept apart from the OoT ones.
  {
    auto foreign = overlay;
    foreign["remaining"][0]["dst"] = "MM_TEMPLE_WOODFALL";
    const auto from =
        Find(foreign["remaining"][0]["src"].get<std::string>())->id;
    const auto decoded = DecodeOotUnifiedAssumptions(foreign, {}, Caps());
    assert(decoded.foreign.at(from).size() == 1 &&
           decoded.foreign.at(from)[0].destination ==
               Find("MM_TEMPLE_WOODFALL")->id);
    auto shop = overlay;
    shop["remaining"][0]["dst"] = "MM_CLOCK_TOWN_FROM_CLOCK_TOWER";
    Reject([&] { DecodeOotUnifiedAssumptions(shop, {}, Caps()); });
  }
  // A Master Quest context is assumed only with the quests id; its sources must
  // exist there (MQ Shadow has none, so the vanilla candidate names one too
  // many).
  OotQuests mq{};
  mq[0] = true;
  Reject([&] { DecodeOotUnifiedAssumptions(overlay, mq, vanillaOnly); });
  assert(DecodeOotUnifiedAssumptions(overlay, mq, Caps()).routes.size() == 9);
  OotQuests mqShadow{};
  mqShadow[OotMqIndex("Shadow")] = true;
  Reject([&] { DecodeOotUnifiedAssumptions(overlay, mqShadow, Caps()); });
  OotQuests allMq{};
  allMq.fill(true);
  const auto mqPools = LayoutV1::MakePools(layout.settings, allMq, false);
  const auto mqOverlay = Json::parse(
      EncodeOotOverlay(BuildCandidateWorld({}, mqPools, layout.settings)));
  const auto mqDecoded = DecodeOotUnifiedAssumptions(mqOverlay, allMq, Caps());
  assert(mqDecoded.routes.size() == 10 &&
         mqDecoded.routes.contains(Find("OOT_WALLMASTER_SPIRIT_STATUE")->id));
  Reject([&] { DecodeOotUnifiedAssumptions(mqOverlay, OotQuests{}, Caps()); });
  const auto empty = EncodeOotOverlay(BuildCandidateWorld({}, {}, {}));
  assert(DecodeOotUnifiedAssumptions(Json::parse(empty), mq).routes.empty());
  auto native = Fake::Fns();
  ApplyNativeLayout(native, seed, Caps());
  assert(Fake::mm.routes().size() == 4 && Fake::oot.routes().size() == 9);
  auto next = layout;
  std::vector<std::string_view> ootSources;
  for (const auto &[source, destination] : *next.overrides)
    if (Find(source)->game == Game::Oot)
      ootSources.push_back(source);
  std::swap(next.overrides->at(ootSources[0]),
            next.overrides->at(ootSources[1]));
  for (bool *failure : {&Fake::failMm, &Fake::failOot, &Fake::driftOnApply}) {
    *failure = true;
    Reject([&] { ApplyNativeLayout(native, Seed(next), Caps()); });
    assert(Fake::mm.json() == seed.at("entranceLayout") &&
           Fake::oot.json() == seed.at("entranceLayout"));
  }
  ApplyNativeLayout(native, Seed(next), Caps());
  assert(Fake::mm.json() == EncodeEntranceLayout(next) &&
         Fake::oot.json() == EncodeEntranceLayout(next));
  {
    NativeScope scope(native);
    ApplyNativeSeed(native, Json::object(), Caps());
    assert(!Fake::mm.active() && !Fake::oot.active());
  }
  assert(Fake::mm.json() == EncodeEntranceLayout(next) &&
         Fake::oot.json() == EncodeEntranceLayout(next));
  Fake::failMm = true;
  Reject([&] { ApplyNativeSeed(native, Json::object(), Caps()); });
  assert(Fake::mm.json() == EncodeEntranceLayout(next) &&
         Fake::oot.json() == EncodeEntranceLayout(next));
  auto ignoredClear = native;
  ignoredClear.applyOotLayout = [](const char *text) {
    return Json::parse(text).empty() ? 1 : Fake::ApplyOot(text);
  };
  Reject([&] { ApplyNativeSeed(ignoredClear, Json::object(), Caps()); });
  assert(Fake::mm.json() == EncodeEntranceLayout(next) &&
         Fake::oot.json() == EncodeEntranceLayout(next));
  ApplyNativeSeed(native, Json::object(), Caps());
  assert(!Fake::mm.active() && !Fake::oot.active());
  const auto mmOverlay = EncodeMmOverlay(candidate),
             ootOverlay = EncodeOotOverlay(candidate);
  for (bool *failure : {&Fake::failMmAssumed, &Fake::failOotAssumed}) {
    *failure = true;
    Reject([&] { AssumptionScope scope(native, mmOverlay, ootOverlay); });
    *failure = false;
    assert(!Fake::mmAssumed && !Fake::ootAssumed);
  }
  {
    AssumptionScope scope(native, mmOverlay, ootOverlay);
    assert(Fake::mmAssumed && Fake::ootAssumed);
  }
  assert(!Fake::mmAssumed && !Fake::ootAssumed);
  Reject([&] {
    AssumptionScope scope(native, mmOverlay, ootOverlay);
    throw std::runtime_error("oracle failed");
  });
  assert(!Fake::mmAssumed && !Fake::ootAssumed);
  auto missing = native;
  missing.dumpOotLayout = nullptr;
  Reject([&] { ApplyNativeLayout(missing, seed, Caps()); });
  missing = native;
  missing.setOotAssumptions = nullptr;
  Reject([&] { AssumptionScope scope(missing, mmOverlay, ootOverlay); });
  ActiveOotUnified().clear();
  std::cout
      << "PASS vanilla nine-source/eleven-arrival overlay, every MQ "
         "context rejected by the vanilla id, 3839 bound by the quests id "
         "and 256 capacity shortfalls, both-module rollback\n";
  return 0;
}
