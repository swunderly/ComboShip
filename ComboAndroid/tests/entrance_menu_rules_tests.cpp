#include "gui/ComboMenuPlacement.h"
#include "headless_settings.inc"
#include "rando/GenerationRules.h"
#include "rando/entrances/EntranceProfile.h"
#include "rando/entrances/TempleAdapter.h"
#include <cassert>
#include <iostream>
#include <set>
using namespace ComboRando;
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
static const Json assets{{"hasOriginal", true}, {"hasMasterQuest", false}};
static const char *wall = "gCombo.Rando.Entrances.erWallmasters";
template <class F> void Reject(F fn) {
  bool failed = false;
  try {
    fn();
  } catch (const std::exception &) {
    failed = true;
  }
  assert(failed);
}
int main() {
  const auto caps = ActiveCapabilities();
  const auto *row = FindEntranceSetting("erWallmasters");
  assert(row);
#if COMBO_ENTRANCE_DEV
  assert(row->visible(caps.Selected("")) &&
         row->supports(1, caps.Selected("")));
#else
  assert(!row->visible(caps.Selected("")));
#endif
  assert(!row->supports(2, caps.Selected(""))); // Full stays hidden.
  const auto catalogue = EntranceProfileCatalog(caps);
#if COMBO_ENTRANCE_DEV
  assert(catalogue.size() == 1 && catalogue[0]["choices"].size() == 2);
  assert(catalogue[0]["label"] == row->name &&
         catalogue[0]["description"] == row->description);
#else
  assert(catalogue.empty());
#endif
  for (bool oot : {true, false}) {
    const auto *placement =
        Detail::FindMenuPlacementKey(oot, "entrance:erWallmasters");
    assert(placement && std::string(placement->page) == "Logic & access" &&
           std::string(placement->heading) == "Entrances");
  }
  Json settings = Json::object();
  Json draft{{"fileType", kComboSeedFileType},
             {"version", 1},
             {"setupCVars", Json::object()},
             {"mm", {{"settings", Json::object()}}}};
  const auto read = [&](const char *key, int fallback) {
    return settings.value(key, fallback);
  };
  assert(ProfileWallmasterRefusal(draft, assets, read).empty());
  // Every reason shows one plain sentence on the row, its tooltip, the Generate
  // button and the generation error (owner decision D3). The developer text is
  // kept, for logs and exceptions only.
  std::set<std::string> sentences;
  const auto plain = [&](const EntranceRefusal &why, const std::string &player,
                         const std::string &log) {
    assert(why.player == player && why.log == log);
    for (const char *word : {"GLOBAL", "deferred", "One-Way", "Ship's own"})
      assert(why.player.find(word) == std::string::npos);
    sentences.insert(why.player);
  };
  // Only combined mode generates a layout. The row shows the solo reason ahead
  // of every other one in the two solo and the two vanilla Home modes.
  EntranceSettingsFacts solo;
  solo.combinedMode = false;
  const auto combinedOnly = EntranceGenerationRefusal(solo);
  plain(combinedOnly, "Wallmaster Shuffle works only in combined mode.",
        "Wallmaster Shuffle works only in combined mode.");
  for (int mode = 0; mode < 5; ++mode) {
    auto moded = draft;
    moded["gameMode"] = kGameModeKeys[mode];
    for (const char *other :
         {"", kOotEnemyRandomizer, "gRandoSettings.ShuffleDungeonsEntrances"}) {
      if (*other)
        settings[other] = 1;
      const auto facts = ProfileEntranceFacts(moded, read);
      const auto menu = ProfileWallmasterRefusal(moded, assets, read);
      assert(facts.combinedMode == (mode == COMBO_MODE_COMBINED));
      assert(menu == EntranceGenerationRefusal(facts));
      if (mode != COMBO_MODE_COMBINED)
        assert(menu == combinedOnly);
      else
        assert(menu.empty() == !*other);
      settings.clear();
    }
  }
  const auto check = [&](const std::string &player, const std::string &log) {
    const auto menu = ProfileWallmasterRefusal(draft, assets, read);
    const auto facts = ProfileEntranceFacts(draft, read);
    assert(!menu.empty() && menu == EntranceGenerationRefusal(facts));
    plain(menu, player, log);
    if (!facts.shipwrightEntrances && !facts.enemyRandomizer) {
      TempleFacts temple;
      static_cast<EntranceSettingsFacts &>(temple) = facts;
      Settings s;
      s.erWallmasters = Mode::OwnGame;
      bool rejected = false;
      try {
        CheckTempleCapability(s, temple, [](std::string_view) { return true; });
      } catch (const TempleProbeUnsupported &error) {
        rejected = error.what() == "Temple probe: " + log;
      }
      assert(rejected && menu == TempleSettingsRefusal(temple));
    }
  };
  const std::string ship =
      "Turn off the other OoT entrance shuffles to use Wallmaster Shuffle.";
  for (const char *key : kShipEntranceSettings) {
    settings[key] = 1;
    check(ship, "Wallmaster and One-Way shuffles need Ship's own entrance "
                "shuffle off");
    assert(IsShipEntranceSetting(key));
    settings.erase(key);
  }
  const std::string enemy =
      "Turn off Enemy Randomizer to use Wallmaster Shuffle.";
  settings[kOotEnemyRandomizer] = 1;
  check(enemy, "Wallmaster Shuffle needs Enemy Randomizer off");
  settings.clear();
  for (const auto &[key, player] :
       {std::pair{"gCombo.Rando.SongTravel",
                  "Turn off Warp Songs and Soaring Spots Shuffle to use "
                  "Wallmaster Shuffle."},
        std::pair{"gCombo.Rando.ForeignSongs",
                  "Turn off \"Play warp songs across games\" to use "
                  "Wallmaster Shuffle."}}) {
    settings[key] = 1;
    check(player, "song travel logic needs GLOBAL song sources (deferred)");
    settings.clear();
  }
  // Ship's Logic row offers Glitchless (0) and No Logic (1) only.
  settings[kNativeLogicRule] = 2;
  check("Set Logic to Glitchless to use Wallmaster Shuffle.",
        "only glitchless OoT logic is supported");
  settings.clear();
  const auto song = [&](const std::string &name) {
    const std::string use =
        " from the starting items to use Wallmaster Shuffle.";
    check("Remove " + name + use,
          "starting " + name + " needs GLOBAL song sources (deferred)");
  };
  for (const auto &[key, name] :
       {std::pair{"StartingMinuetOfForest", "Minuet of Forest"},
        std::pair{"StartingBoleroOfFire", "Bolero of Fire"},
        std::pair{"StartingSerenadeOfWater", "Serenade of Water"},
        std::pair{"StartingRequiemOfSpirit", "Requiem of Spirit"},
        std::pair{"StartingNocturneOfShadow", "Nocturne of Shadow"},
        std::pair{"StartingPreludeOfLight", "Prelude of Light"}}) {
    settings[std::string("gRandoSettings.") + key] = 1;
    song(name);
    settings.clear();
  }
  draft["mm"]["settings"]["gRando.StartingItems"] = {"Song of Time",
                                                     "Song of Soaring"};
  song("Song of Soaring");
  settings[kNativeLogicRule] = 1;
  settings["gCombo.Rando.SongTravel"] = 1;
  assert(ProfileWallmasterRefusal(draft, assets, read)
             .empty()); // No Logic does not run Temple.
  settings[kOotEnemyRandomizer] = 1;
  check(enemy, "Wallmaster Shuffle needs Enemy Randomizer off");
  settings.clear();
  draft["mm"]["settings"].erase("gRando.StartingItems");
  settings["gRandoSettings.MQDungeons"] = 2;
  assert(ProfileWallmasterRefusal(draft, assets, read)
             .empty()); // Native forces vanilla without MQ data.
  // MQ data with an MQ selection, and MQ data alone, give one sentence, the
  // one the generator gives for a Master Quest dungeon.
  const std::string mq =
      "Wallmaster Shuffle does not support Master Quest dungeons yet.";
  const std::string mqLog =
      "Wallmaster Shuffle currently supports vanilla dungeons only";
  assert(mq == kWallmasterMasterQuestText);
  auto mqAssets = assets;
  mqAssets["hasMasterQuest"] = true;
  plain(ProfileWallmasterRefusal(draft, mqAssets, read), mq, mqLog);
  settings.clear();
  assert(ProfileWallmasterRefusal(draft, mqAssets, read).empty());
  mqAssets["hasOriginal"] = false;
  plain(ProfileWallmasterRefusal(draft, mqAssets, read), mq, mqLog);
  const std::string data =
      "Wallmaster Shuffle cannot read the OoT dungeon data.";
  assert(data == kWallmasterDungeonDataText);
  plain(ProfileWallmasterRefusal(draft, Json::object(), read), data,
        "Wallmaster Shuffle cannot verify the available dungeon data");
  // Solo mode, Ship ER, Enemy Randomizer, glitched logic, the two song logic
  // options, six warp songs and Soaring, Master Quest and unknown dungeon data.
  assert(sentences.size() == 15);
  auto raw = draft;
  raw["setupCVars"][wall] = 2;
  raw["setupCVars"]["gCombo.Rando.Entrances.erOneWays"] = 1;
  raw["setupCVars"]["gRandoSettings.ShuffleWarpSongs"] = 1;
  raw["futureExtension"] = {{"preserve", 123}};
  const auto immutable = raw;
  assert(NormalizeEntranceDraft(raw, caps));
  assert(raw["setupCVars"][wall] == 0);
  assert(raw["setupCVars"]["gCombo.Rando.Entrances.erOneWays"] == 0);
  assert(raw["setupCVars"]["gRandoSettings.ShuffleWarpSongs"] == 1);
  assert(raw["futureExtension"] == immutable["futureExtension"]);
  assert(!NormalizeEntranceDraft(raw, caps));
  assert(immutable["setupCVars"][wall] ==
         2); // Source never mutated by a detached draft.
  raw["setupCVars"] = "malformed";
  Reject([&] { NormalizeEntranceDraft(raw, caps); });

  EntranceLayout layout;
  layout.required = {std::string(kVanillaWallmasterCapability)};
  layout.settings.erWallmasters = Mode::OwnGame;
  layout.devBuild = true;
  auto imported = draft;
  imported["fileType"] = kEntranceSeedFileType;
  imported["version"] = 2;
  imported["entranceLayout"] = EncodeEntranceLayout(layout);
  const auto originalSeedSettings = imported;
  assert(SavedEntranceValue(imported, wall, 0) == 1);
  imported["setupCVars"][wall] = 0;
  assert(SavedEntranceValue(imported, wall, 0) == 1); // Saved view follows the layout.
  imported["setupCVars"].erase(wall);
#if COMBO_ENTRANCE_DEV
  assert(!NormalizeEntranceDraft(imported, caps));
  assert(!imported.contains("entranceLayout") &&
         imported["fileType"] == kComboSeedFileType);
  assert(imported["setupCVars"][wall] == 1 && imported["version"] == 1);
  RequireSupportedSettingsCapabilities(imported, caps);
  imported["setupCVars"][wall] = 0;
  assert(!NormalizeEntranceDraft(imported, caps));
  assert(imported["setupCVars"][wall] ==
         0); // Old envelope cannot turn a cleared row back on.
#else
  Reject([&] {
    NormalizeEntranceDraft(imported, caps);
  }); // Shipping guard before any conversion.
#endif
  assert(originalSeedSettings.contains("entranceLayout"));
  Json profile = draft;
  profile["setupCVars"][wall] = 1;
#if COMBO_ENTRANCE_DEV
  assert(EntranceSettingsForHeadless(profile, {}, "", false, caps)
             .settings.erWallmasters == Mode::OwnGame);
#else
  Reject([&] { EntranceSettingsForHeadless(profile, {}, "", false, caps); });
#endif
  assert(EntranceSettingsForHeadless(profile, {"erWallmasters=none"}, "", false,
                                     caps)
             .settings.erWallmasters == Mode::None);
  Reject([&] { EntranceSettingsForHeadless(profile, {}, "", true, caps); });
  profile["setupCVars"][wall] = 2;
  Reject([&] { EntranceSettingsForHeadless(profile, {}, "", false, caps); });
  profile["setupCVars"][wall] = uint64_t(1) << 40;
  Reject([&] { EntranceSettingsForHeadless(profile, {}, "", false, caps); });
  profile.erase("setupCVars");
  assert(MakePools(
             EntranceSettingsForHeadless(profile, {}, "", false, caps).settings)
             .empty());
  std::cout << "PASS: shared menu/generation/Temple refusals, "
            << sentences.size()
            << " plain refusal sentences, vanilla asset policy, capability "
               "choices and detached draft normalization\n";
}
