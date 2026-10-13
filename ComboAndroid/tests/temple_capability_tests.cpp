// The Temple probe capability gate and code mapping with the real Shuffler
// (Shuffle.h), registry, hidden settings model and TempleAdapter.h. Native
// entry points are fakes here; the native probe itself is covered by the
// temple_probe_* harnesses.
#include "rando/entrances/OotmmSettings.h"
#include "rando/entrances/TempleAdapter.h"
#include <cassert>
#include <iostream>

using namespace ComboRando;
using namespace ComboRando::Entrances;
using Json = nlohmann::json;

template <class E, class F> bool Throws(F f) {
  try {
    f();
  } catch (const E &) {
    return true;
  }
  return false;
}
Settings OneWays() {
  Settings s;
  s.erOneWays = Mode::OwnGame;
  s.erOneWaysMajor = s.erOneWaysOwls = s.erOneWaysIkana =
      s.erOneWaysWaterVoids = true;
  return s;
}
size_t draws = 0;
Bindings BaseBindings() {
  Bindings b;
  b.areaExists = [](std::string_view) { return true; };
  b.sourceSupported =
      b.destinationSupported = [](const Record &) { return true; };
  b.validateAssumed = [](const Overrides &, const Pools &) { return true; };
  b.validateComplete = [](const Overrides &) { return true; };
  b.chooseIndex = [](size_t count) {
    ++draws;
    return count - 1;
  };
  return b;
}

// Fake native probe: checks each request, then answers with `answer`.
int answer = 1, queries = 0, prepares = 0, releases = 0;
std::string lastQuery;
int FakePrepare(const char *json) {
  ++prepares;
  const auto j = Json::parse(json);
  assert(j["schema"] == "oot-temple-probe-prepare-v1" &&
         j["registry"] == std::string(kRegistrySourceSha256));
  return answer == -11 ? -1 : 0;
}
int FakeQuery(const char *json, int (*)(void), uint8_t *ages) {
  ++queries;
  lastQuery = json;
  const auto j = Json::parse(json);
  assert(j["schema"] == "oot-temple-probe-query-v1" &&
         j["assumedInstalled"] == true && j["assumed"].empty());
  const std::set<std::string_view> bound(kOotTempleCandidateRows.begin(),
                                         kOotTempleCandidateRows.end());
  for (const auto &row : j["deleted"])
    assert(bound.contains(row.get<std::string>()));
  for (const auto &e : j["edges"])
    assert(bound.contains(e["src"].get<std::string>()) &&
           bound.contains(e["dst"].get<std::string>()));
  assert(j["portalLinks"].size() == 2);
  *ages = answer == 1 ? 2 : 0;
  return answer;
}
const char *FakeError() { return "fake detail"; }
void FakeRelease() { ++releases; }
const TempleProbeFns kFns{FakePrepare, FakeQuery, FakeError, FakeRelease,
                          nullptr};

// What wiring will do: gate first, then build and run the Shuffler.
Overrides Gated(const Settings &s, const TempleFacts &f, Bindings b) {
  CheckTempleCapability(s, f, MakePools(s));
  b.validateAgeTemple = MakeTempleValidator(kFns, s);
  TempleProbeSession session(kFns, f);
  return Shuffler(s, std::move(b)).run();
}

void UnsupportedBeforeAnyDraw() {
  struct Case {
    const char *name;
    std::function<void(Settings &, TempleFacts &)> edit;
  };
  const std::vector<Case> cases{
      {"Full mode",
       [](Settings &s, TempleFacts &) { s.erOneWays = Mode::Full; }},
      {"game links",
       [](Settings &s, TempleFacts &) {
         s.erIndoors = Mode::OwnGame;
         s.erIndoorsGameLinks = true;
       }},
      {"spawns",
       [](Settings &s, TempleFacts &) { s.erSpawns = Spawns::Adult; }},
      {"boss", [](Settings &s, TempleFacts &) { s.erBoss = Mode::OwnGame; }},
      {"dungeonBeaten",
       [](Settings &, TempleFacts &f) { f.dungeonBeaten = true; }},
      {"grottos",
       [](Settings &s, TempleFacts &) { s.erGrottos = Mode::OwnGame; }},
      {"overworld",
       [](Settings &s, TempleFacts &) { s.erOverworld = Mode::OwnGame; }},
      {"indoors",
       [](Settings &s, TempleFacts &) {
         s.erIndoors = Mode::OwnGame;
         s.erIndoorsMajor = true;
       }},
      {"regions",
       [](Settings &s, TempleFacts &) { s.erRegions = Mode::OwnGame; }},
      {"dungeons",
       [](Settings &s, TempleFacts &) {
         s.erDungeons = Mode::OwnGame;
         s.erMajorDungeons = true;
       }},
      {"wallmasters",
       [](Settings &s, TempleFacts &) { s.erWallmasters = Mode::OwnGame; }},
      {"mixed",
       [](Settings &s, TempleFacts &) {
         s.erDungeons = Mode::OwnGame;
         s.erMajorDungeons = true;
         s.erMixed = Mode::OwnGame;
         s.erMixedDungeons = true;
       }},
      {"one-ways anywhere (unbound destinations)",
       [](Settings &s, TempleFacts &) { s.erOneWaysAnywhere = true; }},
      {"starting Soaring",
       [](Settings &, TempleFacts &f) {
         f.mmStartingItems = {"Song of Soaring"};
       }},
      {"starting OoT warp song",
       [](Settings &, TempleFacts &f) {
         f.ootStartingItems = {"Prelude of Light"};
       }},
      {"MM-side OoT warp song",
       [](Settings &, TempleFacts &f) {
         f.mmStartingItems = {"Minuet of Forest"};
       }},
      {"foreign warp song",
       [](Settings &, TempleFacts &f) {
         f.foreignStartingItems = {"Serenade of Water"};
       }},
      {"song travel logic",
       [](Settings &, TempleFacts &f) { f.songTravelLogic = true; }},
      {"foreign songs logic",
       [](Settings &, TempleFacts &f) { f.foreignSongsLogic = true; }},
      {"glitched logic",
       [](Settings &, TempleFacts &f) { f.glitchless = false; }},
      {"Ship entrance shuffle",
       [](Settings &, TempleFacts &f) { f.shipwrightEntrances = true; }},
      {"random starting age",
       [](Settings &, TempleFacts &f) {
         f.startingAge = TempleStartingAge::Random;
       }},
      {"registry identity",
       [](Settings &, TempleFacts &f) { f.registry = "0000"; }},
      {"OoT-only games",
       [](Settings &s, TempleFacts &) { s.games = Games::Oot; }},
  };
  for (const auto &c : cases) {
    Settings s = OneWays();
    TempleFacts f;
    c.edit(s, f);
    draws = queries = prepares = 0;
    const bool rejected =
        Throws<TempleProbeUnsupported>([&] { Gated(s, f, BaseBindings()); });
    if (!rejected || draws || queries || prepares)
      std::cerr << "case: " << c.name << '\n';
    assert(rejected && draws == 0 && queries == 0 && prepares == 0);
  }
}

void SupportedEnvelope() {
  for (auto age : {TempleStartingAge::Child, TempleStartingAge::Adult}) {
    TempleFacts f;
    f.startingAge = age;
    answer = 1;
    draws = queries = prepares = releases = 0;
    const auto layout = Gated(OneWays(), f, BaseBindings());
    assert(!layout.empty() && queries > 0 && draws > 0 && prepares == 1 &&
           releases == 1);
    assert(Json::parse(lastQuery)["mm"].is_object());
  }
  // False for every candidate: generation fails after the pinned 1000 attempts,
  // with no fallback.
  answer = 0;
  queries = releases = 0;
  assert(Throws<ShuffleError>(
      [] { Gated(OneWays(), TempleFacts{}, BaseBindings()); }));
  assert(queries >= 1000 && releases == 1);
  // Cancellation and native failures escape the retry loop on the first query.
  answer = -3;
  queries = releases = 0;
  assert(Throws<GenerationCancelled>(
      [] { Gated(OneWays(), TempleFacts{}, BaseBindings()); }));
  assert(queries == 1 && releases == 1);
  for (int code : {-1, -2, -4, 7}) {
    answer = code;
    queries = 0;
    bool detail = false;
    try {
      Gated(OneWays(), TempleFacts{}, BaseBindings());
    } catch (const TempleProbeUnsupported &e) {
      detail = std::string(e.what()).find("fake detail") != std::string::npos;
    }
    assert(detail && queries == 1);
  }
  // A failed prepare stops before the Shuffler and still releases.
  answer = -11;
  draws = releases = 0;
  assert(Throws<TempleProbeUnsupported>(
      [] { Gated(OneWays(), TempleFacts{}, BaseBindings()); }));
  assert(draws == 0 && releases == 1);
}

void ExistingBoundaries() {
  // Logic none never calls the hook; games=Mm never calls it.
  int calls = 0;
  auto counting = BaseBindings();
  counting.validateAgeTemple = [&](const Overrides &, const Pools &) {
    ++calls;
    return true;
  };
  counting.logicEnabled = false;
  assert(!Shuffler(OneWays(), counting).run().empty() && calls == 0);
  Settings mm;
  mm.games = Games::Mm;
  mm.erOneWays = Mode::OwnGame;
  mm.erOneWaysIkana = mm.erOneWaysWaterVoids = true;
  counting.logicEnabled = true;
  assert(!Shuffler(mm, counting).run().empty() && calls == 0);
  // The missing-hook rejection for active OoT/Both logic is unchanged and draws
  // nothing.
  draws = 0;
  bool missing = false;
  try {
    Shuffler(OneWays(), BaseBindings());
  } catch (const std::invalid_argument &e) {
    missing =
        std::string(e.what()) == "OoT entrance validation requires the "
                                 "starting-item opposite-age Temple probe";
  }
  assert(missing && draws == 0);
  // No entrance value is admitted by this build (Capabilities.h), and the
  // reader strips every non-default value.
  assert(std::size(kEntranceSettings) == 40);
  const auto active = ActiveCapabilities();
  for (const auto &row : kEntranceSettings)
    assert(!row.visible(active));
  const auto stripped =
      ReadEntranceSettings([](const char *, int) { return 1; }, active);
  assert(MakePools(stripped).empty() && !stripped.erIndoorsGameLinks &&
         stripped.erSpawns == Spawns::None);
}

void ForeignStartingItems() {
  // Only shared-family starters cross; the OoT dump's own starting list (Kokiri
  // Sword) never does.
  const uint32_t mask = (1u << SF_OCARINA_BUTTON_A) | (1u << SF_SONG_OF_TIME) |
                        (1u << SF_OCARINA);
  const auto oot = Json{
      {"sharedStartingItems", {"Kokiri Sword", "Song of Time", "Song of Time"}},
      {"sharedButtonsEnabled",
       true}}.dump();
  const auto mm = Json{
      {"sharedStartingItems", {"Ocarina of Time"}},
      {"sharedButtonsEnabled",
       true}}.dump();
  const auto items = NormalizeForeignStartingItems(mask, oot, mm);
  assert(std::find(items.begin(), items.end(), "Kokiri Sword") == items.end());
  assert(std::count(items.begin(), items.end(), "Song of Time") == 1);
  assert(std::count(items.begin(), items.end(), "Ocarina of Time") == 1);
  TempleFacts f;
  f.startingAge = TempleStartingAge::Adult;
  f.foreignStartingItems = items;
  const auto prepare = Json::parse(EncodeTemplePrepare(f));
  assert(prepare["startingAge"] == "adult" &&
         prepare["foreignStartingItems"].size() == items.size());
}

int main() {
  UnsupportedBeforeAnyDraw();
  SupportedEnvelope();
  ExistingBoundaries();
  ForeignStartingItems();
  std::cout << "Temple capability gate: unsupported worlds stop before any "
               "draw, the T1 envelope runs, codes map "
               "without retry, and the existing hidden boundaries hold\n";
}
