// Vanilla scope, frozen-pool coverage, candidate precedence and two-module
// rollback.
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <fstream>
#include <iostream>
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
  assert(!ActiveCapabilities().Active(kVanillaWallmasterCapability));
  Reject([&] { Validate(layout, ActiveCapabilities()); });
  OotUnifiedLayout installed;
  installed.install(seed, Caps());
  assert(installed.routes().size() == 9 &&
         installed.json() == seed.at("entranceLayout"));
  for (const auto &[src, dst] : *layout.overrides)
    if (Find(src)->game == Game::Oot)
      assert(installed.routesFrom(Find(src)->id) ==
             std::vector<OotUnifiedRoute>{{Find(dst)->id}});
  // Every nonempty MQ context fails, including dungeons without a capture
  // source. No pool shrinking.
  for (unsigned mask = 1; mask < 4096; ++mask) {
    auto mq = layout;
    for (size_t i = 0; i < mq.ootMq.size(); ++i)
      mq.ootMq[i] = (mask & (1u << i)) != 0;
    Reject([&] { Validate(mq, Caps()); });
    Reject([&] { installed.install(Seed(mq), Caps()); });
    assert(installed.json() == seed.at("entranceLayout"));
  }
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
    Reject([&] { DecodeOotUnifiedAssumptions(bad, {}); });
  }
  OotQuests mq{};
  mq[0] = true;
  Reject([&] { DecodeOotUnifiedAssumptions(overlay, mq); });
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
  std::cout << "PASS vanilla nine-source/eleven-arrival overlay, all MQ "
               "contexts rejected, both-module rollback\n";
  return 0;
}
