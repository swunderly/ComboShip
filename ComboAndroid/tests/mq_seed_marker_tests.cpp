// combo/mq/MqSeedMarker.h: the per-seed and per-save Master Quest data-source
// marker. A seed or save without it is Legacy and keeps SoH's behaviour; a new
// seed made with the bundled data records the source and its MQ dungeons, and
// both survive the saved-seed and profile paths.
// `--local <repo>` also reads every .combosav under a local-roms folder beside
// the checkout (never committed) and checks that those real saves and seeds
// still read as Legacy and resolve exactly as before.
#include "ComboRandomizerSavedSeed.h"
#include "mq/ComboMqData.h"
#include "mq/MqSeedMarker.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using nlohmann::json;
using namespace ComboMq;

static_assert(kMqSeedDungeonCount == kDungeonCount,
              "the marker's dungeon ids are OoTMM's 12 MQ dungeons");

namespace {

template <class F> bool Throws(F f) {
  try {
    f();
  } catch (const std::exception &) {
    return true;
  }
  return false;
}

void CheckMarker() {
  assert(MqDataSourceOf(json::object()) == SeedMqSource::Legacy);
  assert(ReadMqDataSource(json::object()) == SeedMqSource::Legacy);
  assert(ReadMqDataSource({{"mqDataSource", "ootmm-32.3"}}) ==
         SeedMqSource::Bundled);
  for (const json &invalid :
       {json("ootmm-32.2"), json(""), json("OOTMM-32.3"), json(1), json(true),
        json(nullptr), json::array(), json::object()}) {
    const json object = {{"mqDataSource", invalid}};
    assert(MqDataSourceOf(object) == SeedMqSource::Unsupported);
    bool typed = false;
    try {
      (void)ReadMqDataSource(object);
    } catch (const UnsupportedMqDataSource &) {
      typed = true;
    }
    assert(typed);
  }
  assert(MqDataSourceOf(json::array()) == SeedMqSource::Unsupported);
  assert(Throws([] { (void)ReadMqDataSource(json::array()); }));

  // Legacy writes nothing and removes a stale marker: a Legacy object is
  // exactly what it was before the marker existed.
  json object = {{"settings", json::object()}};
  const json before = object;
  WriteMqDataSource(object, SeedMqSource::Legacy);
  assert(object == before);
  WriteMqDataSource(object, SeedMqSource::Bundled);
  assert(object["mqDataSource"] == "ootmm-32.3");
  assert(ReadMqDataSource(object) == SeedMqSource::Bundled);
  WriteMqDataSource(object, SeedMqSource::Legacy);
  assert(object == before);
  WriteMqDataSource(object, SeedMqSource::Unsupported);
  assert(object == before);
}

void CheckSeedDocument() {
  // A seed generated without the bundled data is unchanged.
  json legacy = {{"settings", {{"a", 1}}}, {"kokiriInvisibleRupeeRevision", 1}};
  const json legacyBefore = legacy;
  StampSeedMq(legacy, SeedMqSource::Legacy, 0x3f);
  assert(legacy == legacyBefore);
  assert(RecordedMqDungeonMask(legacy) == -1);
  assert(SeedMqPlayable(legacy, false));
  assert(ReplayKeepsMqDungeons(legacy, 0x3f));
  assert(ReplayKeepsMqDungeons(legacy, -1));

  // A bundled seed records its source and MQ dungeons (OoTMM ids, DungeonKey
  // order): Forest 3, Spirit 6, Ganon 11.
  const int mask =
      (1 << kForestTemple) | (1 << kSpiritTemple) | (1 << kGanonsCastle);
  json bundled = {{"settings", json::object()}};
  StampSeedMq(bundled, SeedMqSource::Bundled, mask);
  assert(bundled["mqDataSource"] == "ootmm-32.3");
  assert(bundled["masterQuestDungeons"] == json::array({3, 6, 11}));
  assert(RecordedMqDungeonMask(bundled) == mask);
  assert(SeedMqPlayable(bundled, true));
  // Refused cleanly where the bundled data is off (another ROM version).
  assert(!SeedMqPlayable(bundled, false));
  assert(ReplayKeepsMqDungeons(bundled, mask));
  assert(!ReplayKeepsMqDungeons(bundled, mask & ~(1 << kSpiritTemple)));
  assert(!ReplayKeepsMqDungeons(bundled, -1));

  // A bundled seed whose dice chose no MQ dungeon plays anywhere.
  json none = json::object();
  StampSeedMq(none, SeedMqSource::Bundled, 0);
  assert(none["masterQuestDungeons"] == json::array());
  assert(SeedMqPlayable(none, false));

  json all = json::object();
  StampSeedMq(all, SeedMqSource::Bundled, kMqDungeonMaskAll);
  assert(all["masterQuestDungeons"].size() == 12);
  assert(RecordedMqDungeonMask(all) == kMqDungeonMaskAll);

  // The generated dungeons must be known to stamp a bundled seed.
  for (int invalid : {-1, kMqDungeonMaskAll + 1}) {
    json doc = json::object();
    assert(Throws([&] { StampSeedMq(doc, SeedMqSource::Bundled, invalid); }));
  }
  // Malformed records are refused, never read as Legacy.
  for (const json &ids :
       {json(nullptr), json(3), json::array({12}), json::array({-1}),
        json::array({3, 3}), json::array({"3"}), json::array({1.5})}) {
    json doc = {{"mqDataSource", "ootmm-32.3"}, {"masterQuestDungeons", ids}};
    assert(Throws([&] { (void)RecordedMqDungeonMask(doc); }));
    assert(Throws([&] { (void)SeedMqPlayable(doc, true); }));
  }
  json missing = {{"mqDataSource", "ootmm-32.3"}};
  assert(Throws([&] { (void)RecordedMqDungeonMask(missing); }));
  json unknown = {{"mqDataSource", "ootmm-40.0"},
                  {"masterQuestDungeons", json::array()}};
  assert(Throws([&] { (void)SeedMqPlayable(unknown, true); }));
}

json SaveWith(const json &data) {
  return {{"version", 1},
          {"sections", {{"randomizer", {{"version", 1}, {"data", data}}}}}};
}

// SaveManager::LoadFile's refusal and the file-select rule.
void CheckSaves() {
  for (size_t count : {size_t(0), size_t(1), size_t(12)})
    for (int archive = 0; archive <= 1; archive++)
      for (int bundled = 0; bundled <= 1; bundled++) {
        // Old saves: never refused here, whatever is installed.
        const auto old =
            ReadSaveMqNeeds(SaveWith({{"masterQuestDungeonCount", count}}));
        assert(old.source == SeedMqSource::Legacy &&
               old.mqDungeonCount == count);
        assert(!SaveRefusedForMq(old, archive, bundled));
        // Bundled saves: refused only with MQ dungeons and no bundled data.
        const auto marked =
            ReadSaveMqNeeds(SaveWith({{"masterQuestDungeonCount", count},
                                      {"mqDataSource", "ootmm-32.3"}}));
        assert(marked.source == SeedMqSource::Bundled);
        assert(SaveRefusedForMq(marked, archive, bundled) ==
               (count > 0 && !bundled));
        // A marker this build does not know is never played.
        const auto unknown = ReadSaveMqNeeds(SaveWith(
            {{"masterQuestDungeonCount", count}, {"mqDataSource", 7}}));
        assert(unknown.source == SeedMqSource::Unsupported);
        assert(SaveRefusedForMq(unknown, archive, bundled));
      }
  // Saves without a randomizer section, or with a malformed one, are left to
  // the native loader.
  assert(ReadSaveMqNeeds(json::object()).source == SeedMqSource::Legacy);
  assert(ReadSaveMqNeeds({{"sections", json::object()}}).source ==
         SeedMqSource::Legacy);
  assert(ReadSaveMqNeeds(SaveWith(json::array())).source ==
         SeedMqSource::Legacy);
  assert(ReadSaveMqNeeds(SaveWith({{"masterQuestDungeonCount", "2"}}))
             .mqDungeonCount == 0);
}

void CheckSavedSeedPaths() {
  const auto caps = ComboRando::Entrances::ActiveCapabilities();
  json data = {{"masterQuestDungeonCount", 2},
               {"masterQuestDungeons", json::array({3, 6})},
               {"kokiriInvisibleRupeeRevision", 1},
               {"kokiriGrassCatalogRevision", 1}};
  json seed = {{"fileType", "ComboShipRandomizer"},
               {"oot",
                {{"kokiriInvisibleRupeeRevision", 1},
                 {"kokiriGrassCatalogRevision", 1},
                 {"settings", json::object()}}}};
  // An old container: the replay rules carry no marker, as before.
  json container = {{"oot", SaveWith(data)}, {"combo", {{"rando", seed}}}};
  auto rules =
      ComboRando::Profiles::NativeSeedRulesFromContainer(container, caps);
  assert(!rules["oot"].contains("mqDataSource"));
  assert(MqDataSourceOf(rules["oot"]) == SeedMqSource::Legacy);
  // A bundled save keeps its marker in the replay rules.
  data["mqDataSource"] = "ootmm-32.3";
  StampSeedMq(seed["oot"], SeedMqSource::Bundled, (1 << 3) | (1 << 6));
  container = {{"oot", SaveWith(data)}, {"combo", {{"rando", seed}}}};
  rules = ComboRando::Profiles::NativeSeedRulesFromContainer(container, caps);
  assert(rules["oot"]["mqDataSource"] == "ootmm-32.3");
  assert(rules["oot"]["masterQuestDungeons"] == json::array({3, 6}));

  // A setup made from a seed keeps the seed's own source, never a requested
  // profile's, and drops its MQ dungeons (a generation output).
  for (SeedMqSource source : {SeedMqSource::Legacy, SeedMqSource::Bundled}) {
    json doc = {{"fileType", "ComboShipRandomizer"},
                {"oot", {{"settings", json::object()}}}};
    StampSeedMq(doc["oot"], source, 1 << 4);
    doc["comboProfile"] = {
        {"version", 1},
        {"settings",
         {{"oot",
           {{"mqDataSource",
             source == SeedMqSource::Bundled ? "x" : "ootmm-32.3"}}}}}};
    const auto setup = ComboRando::Profiles::SetupForSeed(doc, caps);
    assert(MqDataSourceOf(setup["oot"]) == source);
    assert(!setup["oot"].contains("masterQuestDungeons"));
    const auto settings = ComboRando::Profiles::SettingsDocument(doc, caps);
    assert(!settings["oot"].contains("masterQuestDungeons"));
    assert(MqDataSourceOf(settings["oot"]) == source);
  }
  json future = {{"fileType", "ComboShipRandomizer"},
                 {"oot", {{"mqDataSource", "ootmm-40.0"}}}};
  assert(
      Throws([&] { (void)ComboRando::Profiles::SetupForSeed(future, caps); }));
}

int bound = 0;
int GetBound() { return bound; }
int SetBound(int value) {
  if (value != 0 && value != 1)
    return 0;
  bound = value;
  return 1;
}

void CheckScope() {
  for (SeedMqSource source : {SeedMqSource::Legacy, SeedMqSource::Bundled}) {
    bound = 0;
    try {
      MqDataSourceScope scope(GetBound, SetBound, source);
      assert(bound == static_cast<int>(source));
      throw std::runtime_error("failed import");
    } catch (const std::runtime_error &) {
    }
    assert(bound == 0);
    {
      MqDataSourceScope scope(GetBound, SetBound, source);
      scope.Keep();
    }
    assert(bound == static_cast<int>(source));
  }
  // Loading another file never inherits the previous file's source.
  bound = 1;
  ApplyMqDataSource(SetBound, MqDataSourceOf(json::object()));
  assert(bound == 0);
  assert(
      Throws([] { ApplyMqDataSource(SetBound, SeedMqSource::Unsupported); }));
  assert(Throws([] { ApplyMqDataSource(nullptr, SeedMqSource::Legacy); }));
  assert(Throws([] {
    MqDataSourceScope scope([] { return -1; }, SetBound, SeedMqSource::Legacy);
  }));
}

// Every real save and seed on this machine predates the marker: each reads as
// Legacy, so the resolver gives SoH's answer for it on every install.
int CheckLocalSaves(const std::filesystem::path &repo) {
  std::filesystem::path folder;
  for (const auto &candidate :
       {repo.parent_path() / "local-roms",
        repo.parent_path().parent_path() / "local-roms"})
    if (std::filesystem::is_directory(candidate)) {
      folder = candidate;
      break;
    }
  if (folder.empty()) {
    std::puts("SKIPPED: no local-roms folder beside the checkout; this check "
              "is local only");
    return 0;
  }
  const auto caps = ComboRando::Entrances::ActiveCapabilities();
  int containers = 0, seeds = 0, mqSaves = 0, refused = 0;
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(folder)) {
    if (entry.path().extension() != ".combosav")
      continue;
    std::ifstream file(entry.path(), std::ios::binary);
    json container;
    try {
      file >> container;
    } catch (const std::exception &) {
      continue; // not a container this test can read
    }
    if (!container.is_object())
      continue;
    containers++;
    const auto oot = container.value("oot", json());
    const auto needs = ReadSaveMqNeeds(oot);
    assert(needs.source == SeedMqSource::Legacy);
    mqSaves += needs.mqDungeonCount > 0;
    for (int archive = 0; archive <= 1; archive++)
      for (int bundled = 0; bundled <= 1; bundled++) {
        assert(!SaveRefusedForMq(needs, archive, bundled));
        // File select: exactly SoH's rule for this save.
        assert(CanPlayMqDungeons(needs.source, needs.mqDungeonCount, archive,
                                 bundled) ==
               (needs.mqDungeonCount == 0 || archive));
        for (int dungeon = 0; dungeon <= 2; dungeon++) {
          MqModeInputs in;
          in.hasOriginal = true;
          in.hasArchiveMq = archive;
          in.bundledAvailable = bundled;
          in.quest = QuestKind::Randomizer;
          in.dungeon = static_cast<DungeonQuest>(dungeon);
          in.seedSource = needs.source;
          const MqMode mode = ResolveMqMode(in);
          assert(mode.gameplayMq == LegacySceneIsMq(in) &&
                 mode.assetsMq == LegacySceneIsMq(in) &&
                 mode.source != MqDataSource::Bundled);
        }
      }
    // The baked seed, as stored.
    const json raw =
        container.contains("combo") && container["combo"].is_object()
            ? container["combo"].value("rando", json())
            : json();
    if (raw.is_object()) {
      seeds++;
      const auto seedOot = raw.value("oot", json::object());
      assert(ReadMqDataSource(seedOot) == SeedMqSource::Legacy);
      assert(!seedOot.contains("masterQuestDungeons"));
      assert(SeedMqPlayable(seedOot, false));
      assert(ReplayKeepsMqDungeons(seedOot, 0));
    }
    // The saved-seed and profile paths, for the containers this build loads.
    try {
      auto rules =
          ComboRando::Profiles::NativeSeedRulesFromContainer(container, caps);
      assert(!rules.value("oot", json::object()).contains("mqDataSource"));
      const auto seed =
          ComboRando::Profiles::SeedFromContainer(container, caps);
      if (!seed.is_null()) {
        const auto setup = ComboRando::Profiles::SetupForSeed(seed, caps);
        assert(!setup["oot"].contains("mqDataSource"));
      }
    } catch (const ComboRando::UnsupportedKokiriSeedRevision &) {
      refused++; // refused for its Kokiri catalog before its MQ data matters
    }
  }
  std::printf("PASS: %d local containers (%d seeds, %d with MQ dungeons, %d "
              "refused for their Kokiri catalog) read as Legacy and resolve "
              "as before\n",
              containers, seeds, mqSaves, refused);
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string(argv[1]) == "--local")
    return CheckLocalSaves(std::filesystem::path(argv[2]));
  CheckMarker();
  CheckSeedDocument();
  CheckSaves();
  CheckSavedSeedPaths();
  CheckScope();
  std::puts("PASS: the MQ data-source marker is absent for Legacy seeds and "
            "saves, recorded with the MQ dungeons for bundled seeds, and kept "
            "by the saved-seed and profile paths");
  return 0;
}
