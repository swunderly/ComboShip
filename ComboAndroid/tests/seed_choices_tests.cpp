// combo/rando/SeedChoices.h: the record of the random choices of a seed's OoT
// settings finalize (Key Rings, Ganon's Trials, the starting age, a Legacy
// seed's Master Quest dungeons), which choices count as random, the streams a
// Seeded finalize draws them from, and the launcher scope that hands a seed's
// record to the native context for its reload or replay. A seed without the
// record stays Legacy; a settings document drops the record, a generation
// output, as it drops the placements.
#include "ComboRandomizerProfiles.h"
#include "mq/MqMode.h"
#include "rando/SeedChoices.h"

#include <cassert>
#include <cstdio>
#include <string>

using nlohmann::json;
using namespace ComboRando::SeedChoices;

namespace {

template <class F> bool Throws(F f) {
  try {
    f();
  } catch (const std::exception &) {
    return true;
  }
  return false;
}

uint32_t Fnv(const std::string &text) {
  uint32_t hash = 2166136261u;
  for (unsigned char c : text)
    hash = (hash ^ c) * 16777619u;
  return hash;
}

void CheckPredicates() {
  // Key Rings: off never, Random always, a Count of none or all never (its
  // shuffle resolves the same rings every time), any other Count always, a
  // Selection only with a dungeon set to Random.
  for (int rings : {8, 9})
    for (int count = 0; count <= rings; count++) {
      assert(!KeyRingsRandom(0, count, rings, true));
      assert(KeyRingsRandom(1, count, rings, false));
      assert(KeyRingsRandom(2, count, rings, false) ==
             (count > 0 && count < rings));
      assert(KeyRingsRandom(3, count, rings, true));
      assert(!KeyRingsRandom(3, count, rings, false));
    }
  // Trials: skip never, a set number of none or all never, else always.
  for (int count = 0; count <= 6; count++) {
    assert(!TrialsRandom(0, count, 6));
    assert(TrialsRandom(1, count, 6) == (count > 0 && count < 6));
    assert(TrialsRandom(2, count, 6));
  }
  assert(!StartingAgeRandom(0) && !StartingAgeRandom(1) &&
         StartingAgeRandom(2));
  // Master Quest: none never; a Selection only with a Random dungeon; a set
  // number of none or all of the dungeons it draws among never; a random
  // number whenever it draws among any.
  assert(!MasterQuestRandom(0, 5, false, 12, 0));
  assert(MasterQuestRandom(3, 0, false, 1, 0) &&
         !MasterQuestRandom(3, 12, true, 0, 12));
  for (int count = 0; count <= 12; count++) {
    assert(MasterQuestRandom(1, count, false, 0, 0) ==
           (count > 0 && count < 12));
    assert(MasterQuestRandom(2, count, false, 0, 0));
  }
  // Set Dungeon Quests: the draw is among the Random dungeons, after the
  // Master Quest ones.
  assert(!MasterQuestRandom(1, 4, true, 0, 4));  // nothing to draw
  assert(!MasterQuestRandom(1, 4, true, 3, 4));  // the count is met already
  assert(MasterQuestRandom(1, 5, true, 3, 4));   // one of three
  assert(!MasterQuestRandom(1, 7, true, 3, 4));  // all three
  assert(!MasterQuestRandom(1, 12, true, 3, 4)); // all three, capped
  assert(MasterQuestRandom(2, 0, true, 1, 11));  // a random number of one
  assert(!MasterQuestRandom(2, 0, true, 0, 11)); // none to draw among
}

void CheckStreams() {
  for (uint64_t seed :
       {0ull, 7ull, 419070006ull, 4294967295ull, 18446744073709551615ull}) {
    const auto digits = std::to_string(seed);
    assert(RollSeed(Choice::KeyRings, seed) ==
           Fnv("keyRings:soh-finalize-v1:" + digits));
    assert(RollSeed(Choice::Trials, seed) ==
           Fnv("trials:soh-finalize-v1:" + digits));
    assert(RollSeed(Choice::StartingAge, seed) ==
           Fnv("startingAge:soh-finalize-v1:" + digits));
    // Each choice, and the Master Quest dungeons, on a stream of its own.
    const uint32_t streams[] = {
        RollSeed(Choice::KeyRings, seed), RollSeed(Choice::Trials, seed),
        RollSeed(Choice::StartingAge, seed), ComboMq::MqRollSeed(seed)};
    for (int a = 0; a < 4; a++)
      for (int b = a + 1; b < 4; b++)
        assert(streams[a] != streams[b]);
  }
  static_assert(RollSeed(Choice::StartingAge, 1) !=
                RollSeed(Choice::StartingAge, 2));
}

Choices Full() {
  Choices choices;
  choices.keyRings =
      Choices::KeyRings{(1u << 5) | (1u << 6) | (1u << 8) | (1u << 10), true};
  choices.requiredTrials = static_cast<uint8_t>((1u << 0) | (1u << 4));
  choices.adult = true;
  choices.masterQuest = static_cast<uint16_t>((1u << 0) | (1u << 9));
  return choices;
}

void CheckRecord() {
  // A finalize whose settings made nothing random records nothing.
  assert(Encode(Choices{}) == json::object());
  assert(Decode(json::object()).Empty());
  const auto full = Full();
  const json record = Encode(full);
  assert(record == json({{"version", 1},
                         {"keyRings",
                          {{"dungeons", json::array({5, 6, 8, 10})},
                           {"gerudoFortress", true}}},
                         {"requiredTrials", json::array({0, 4})},
                         {"startingAge", "adult"},
                         {"masterQuestDungeons", json::array({0, 9})}}));
  assert(Decode(record) == full);
  // Each choice alone round-trips; the others stay unset.
  for (int field = 0; field < 4; field++) {
    Choices one;
    if (field == 0)
      one.keyRings = full.keyRings;
    if (field == 1)
      one.requiredTrials = static_cast<uint8_t>(0);
    if (field == 2)
      one.adult = false;
    if (field == 3)
      one.masterQuest = static_cast<uint16_t>(0);
    assert(!one.Empty() && Decode(Encode(one)) == one);
    assert(Decode(Encode(one)) != full);
  }
  assert(Encode(Choices{{}, {}, false, {}})["startingAge"] == "child");
  // Anything this build cannot read is refused, never dropped.
  const auto with = [&](const char *key, json value) {
    json copy = record;
    copy[key] = std::move(value);
    return copy;
  };
  for (const json &bad :
       {json::array(),
        json(1),
        json({{"version", 1}}),
        with("version", 2),
        with("version", true),
        with("future", 1),
        with("keyRings", {{"dungeons", json::array({5})}}),
        with("keyRings",
             {{"dungeons", json::array({5})}, {"gerudoFortress", 1}}),
        with("keyRings", {{"dungeons", json::array({0})},
                          {"gerudoFortress", false}}), // Deku Tree
        with("keyRings", {{"dungeons", json::array({9})},
                          {"gerudoFortress", false}}), // Ice Cavern
        with("keyRings",
             {{"dungeons", json::array({5, 5})}, {"gerudoFortress", false}}),
        with("keyRings",
             {{"rings", json::array({5})}, {"gerudoFortress", false}}),
        with("requiredTrials", json::array({6})),
        with("requiredTrials", json::array({true})),
        with("requiredTrials", json::array({-1})),
        with("requiredTrials", json::array({1, 1})),
        with("startingAge", "random"),
        with("startingAge", 1),
        with("masterQuestDungeons", json::array({12})),
        with("masterQuestDungeons", json::object())})
    assert(Throws([&] { (void)Decode(bad); }));
}

void CheckSeedDocument() {
  // A seed without the record (every older one, and every new one without a
  // random choice) is Legacy and replays whatever its finalize resolves.
  const json legacy = {{"settings", json::object()}};
  assert(!RecordedChoices(legacy) &&
         ModeForRecord(RecordedChoices(legacy)) == Mode::Legacy);
  assert(ReplayKeepsChoices(legacy, json::object()) &&
         ReplayKeepsChoices(legacy, Encode(Full())));
  // ...and re-rolls exactly the choices its settings make random.
  assert(!ReRollsChoices(legacy, json::object()) &&
         ReRollsChoices(legacy, Encode(Full())));
  assert(Throws([] { (void)RecordedChoices(json::array()); }));

  // Generation stamps what it resolved, and nothing without a random choice.
  json oot = legacy;
  StampSeedChoices(oot, json::object());
  assert(oot == legacy);
  StampSeedChoices(oot, Encode(Full()));
  assert(oot.at(kKey) == Encode(Full()));
  assert(ModeForRecord(RecordedChoices(oot)) == Mode::Seeded &&
         *RecordedChoices(oot) == Full());
  assert(!ReRollsChoices(oot, Encode(Full())));
  // A reload or replay keeps the seed only when its finalize resolved the same
  // choices: a different value, a missing choice or an extra one is refused.
  assert(ReplayKeepsChoices(oot, Encode(Full())));
  auto other = Full();
  other.adult = false;
  assert(!ReplayKeepsChoices(oot, Encode(other)));
  other = Full();
  other.requiredTrials.reset();
  assert(!ReplayKeepsChoices(oot, Encode(other)));
  assert(!ReplayKeepsChoices(oot, json::object()));
  auto less = oot;
  less[kKey].erase("masterQuestDungeons");
  assert(!ReplayKeepsChoices(less, Encode(Full())));
  // A present but empty record is malformed, as is an unreadable one.
  for (const json &bad : {json::object(), json({{"version", 1}}), json(0)}) {
    json broken = legacy;
    broken[kKey] = bad;
    assert(Throws([&] { (void)RecordedChoices(broken); }));
  }
  // Restamping with nothing random removes an earlier record.
  StampSeedChoices(oot, json::object());
  assert(oot == legacy);
}

// A native context's mode and record, as soh's exports keep them.
int nativeMode = 0;
std::string nativeRecord = "{}";
bool nativeAvailable = true;
int GetMode() { return nativeAvailable ? nativeMode : -1; }
const char *GetRecord() {
  return nativeAvailable ? nativeRecord.c_str() : nullptr;
}
int SetChoices(int mode, const char *record) {
  if (!nativeAvailable || mode < 0 || mode > 1)
    return 0;
  try {
    const auto choices = Decode(json::parse(record ? record : "{}"));
    if (!choices.Empty() && mode != 1)
      return 0;
    nativeMode = mode;
    nativeRecord = Encode(choices).dump();
    return 1;
  } catch (...) {
    return 0;
  }
}

void CheckScope() {
  // A reload or replay hands the seed's record to the native context for its
  // finalize, and puts the previous one back when it fails...
  nativeMode = 0;
  nativeRecord = "{}";
  const json seed = {{kKey, Encode(Full())}};
  {
    const auto record = RecordedChoices(seed);
    Scope scope(GetMode, GetRecord, SetChoices, ModeForRecord(record), record);
    assert(nativeMode == 1 && json::parse(nativeRecord) == Encode(Full()));
  }
  assert(nativeMode == 0 && nativeRecord == "{}");
  // ...or keeps it bound when the load succeeds.
  {
    const auto record = RecordedChoices(seed);
    Scope scope(GetMode, GetRecord, SetChoices, ModeForRecord(record), record);
    scope.Keep();
  }
  assert(nativeMode == 1 && json::parse(nativeRecord) == Encode(Full()));
  // A generation draws from the seed with no record, and restores the bound
  // seed's record afterwards.
  {
    Scope scope(GetMode, GetRecord, SetChoices, Mode::Seeded, std::nullopt);
    assert(nativeMode == 1 && nativeRecord == "{}");
  }
  assert(json::parse(nativeRecord) == Encode(Full()));
  // An older seed loads Legacy, with no record.
  {
    const auto record = RecordedChoices(json::object());
    Scope scope(GetMode, GetRecord, SetChoices, ModeForRecord(record), record);
    assert(nativeMode == 0 && nativeRecord == "{}");
  }
  // Without the native exports nothing is bound.
  nativeAvailable = false;
  assert(Throws([] {
    Scope scope(GetMode, GetRecord, SetChoices, Mode::Seeded, std::nullopt);
  }));
  assert(Throws([] {
    Scope scope(nullptr, nullptr, nullptr, Mode::Legacy, std::nullopt);
  }));
  assert(Throws([] { ApplySeedChoices(nullptr, Mode::Legacy, std::nullopt); }));
  nativeAvailable = true;
  // A Legacy context never takes a record.
  assert(Throws([] { ApplySeedChoices(SetChoices, Mode::Legacy, Full()); }));
}

void CheckSettingsDocument() {
  // A setup made from a seed drops the record: a new generation draws its own.
  const auto caps = ComboRando::Entrances::ActiveCapabilities();
  json doc = {{"fileType", "ComboShipRandomizer"},
              {"oot", {{"settings", json::object()}, {kKey, Encode(Full())}}}};
  const auto settings = ComboRando::Profiles::SettingsDocument(doc, caps);
  assert(settings.contains("oot") && !settings["oot"].contains(kKey));
  assert(settings["oot"].contains("settings"));
}

} // namespace

int main() {
  CheckPredicates();
  CheckStreams();
  CheckRecord();
  CheckSeedDocument();
  CheckScope();
  CheckSettingsDocument();
  std::puts("PASS: seed choices record, streams, scope and settings document");
  return 0;
}
