#include "gui/ComboMenuPlacement.h"
#include "headless_settings.inc"
#include "rando/GenerationRules.h"
#include "rando/entrances/EntranceProfile.h"
#include "rando/entrances/TempleAdapter.h"
#include <cassert>
#include <iostream>
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
  // Only combined mode generates a layout. The row shows the solo reason ahead
  // of every other one in the two solo and the two vanilla Home modes.
  EntranceSettingsFacts solo;
  solo.combinedMode = false;
  const auto combinedOnly = EntranceGenerationRefusal(solo);
  assert(combinedOnly == "Wallmaster Shuffle works only in combined mode.");
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
  const auto check = [&] {
    const auto menu = ProfileWallmasterRefusal(draft, assets, read);
    const auto facts = ProfileEntranceFacts(draft, read);
    assert(!menu.empty() && menu == EntranceGenerationRefusal(facts));
    if (!facts.shipwrightEntrances && !facts.enemyRandomizer) {
      TempleFacts temple;
      static_cast<EntranceSettingsFacts &>(temple) = facts;
      Settings s;
      s.erWallmasters = Mode::OwnGame;
      Reject([&] {
        CheckTempleCapability(s, temple, [](std::string_view) { return true; });
      });
      assert(menu == TempleSettingsRefusal(temple));
    }
  };
  for (const char *key : kShipEntranceSettings) {
    settings[key] = 1;
    check();
    assert(IsShipEntranceSetting(key));
    settings.erase(key);
  }
  settings[kOotEnemyRandomizer] = 1;
  check();
  settings.clear();
  for (const char *key :
       {"gCombo.Rando.SongTravel", "gCombo.Rando.ForeignSongs"}) {
    settings[key] = 1;
    check();
    settings.clear();
  }
  settings[kNativeLogicRule] = 2;
  check();
  settings.clear();
  for (const char *key :
       {"StartingMinuetOfForest", "StartingBoleroOfFire",
        "StartingSerenadeOfWater", "StartingRequiemOfSpirit",
        "StartingNocturneOfShadow", "StartingPreludeOfLight"}) {
    settings[std::string("gRandoSettings.") + key] = 1;
    check();
    settings.clear();
  }
  draft["mm"]["settings"]["gRando.StartingItems"] = {"Song of Time",
                                                     "Song of Soaring"};
  check();
  settings[kNativeLogicRule] = 1;
  settings["gCombo.Rando.SongTravel"] = 1;
  assert(ProfileWallmasterRefusal(draft, assets, read)
             .empty()); // No Logic does not run Temple.
  settings[kOotEnemyRandomizer] = 1;
  check();
  settings.clear();
  draft["mm"]["settings"].erase("gRando.StartingItems");
  settings["gRandoSettings.MQDungeons"] = 2;
  assert(ProfileWallmasterRefusal(draft, assets, read)
             .empty()); // Native forces vanilla without MQ data.
  auto mqAssets = assets;
  mqAssets["hasMasterQuest"] = true;
  assert(!ProfileWallmasterRefusal(draft, mqAssets, read).empty());
  settings.clear();
  assert(ProfileWallmasterRefusal(draft, mqAssets, read).empty());
  mqAssets["hasOriginal"] = false;
  assert(!ProfileWallmasterRefusal(draft, mqAssets, read).empty());
  assert(!ProfileWallmasterRefusal(draft, Json::object(), read).empty());
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
  std::cout << "PASS: shared menu/generation/Temple refusals, vanilla asset "
               "policy, capability choices and detached draft normalization\n";
}
