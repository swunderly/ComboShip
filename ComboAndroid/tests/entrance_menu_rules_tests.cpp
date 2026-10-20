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
static const Json assets{
    {"hasOriginal", true}, {"hasMasterQuest", false}, {"mqPlayable", false}};
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
  // The frozen vanilla id alone, and the catalog with the Master Quest id
  // (U11), whatever this build's stages.
  const auto vanillaOnly =
      CapabilitySet::AtLeast(Stage::Hidden).Only(kVanillaWallmasterCapability);
  const auto withQuests = CapabilitySet::AtLeast(Stage::Hidden);
  const auto *row = FindEntranceSetting("erWallmasters");
  assert(row);
  // Both builds offer the row with None and Own Game (P1-05).
  assert(row->visible(caps.Selected("")) &&
         row->supports(1, caps.Selected("")));
  assert(!row->supports(2, caps.Selected(""))); // Full stays hidden.
  const auto catalogue = EntranceProfileCatalog(caps);
  assert(catalogue.size() == 1 && catalogue[0]["choices"].size() == 2);
  assert(catalogue[0]["label"] == row->name &&
         catalogue[0]["description"] == row->description);
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
      "Turn off the other OoT entrance shuffles, and Warp Songs and Owl Drops "
      "on the Warps & travel page, to use Wallmaster Shuffle.";
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
  // Every catalogue state (OTRGlobals.cpp SOH_DumpProfileCatalog): with the
  // frozen vanilla id only, MQ selected is refused wherever MQ can be played,
  // from a real oot-mq.o2r or OoTMM's bundled data (mqPlayable), and an MQ-only
  // install is always refused.
  for (int state = 0; state < 8; ++state) {
    const bool original = state & 1, archive = state & 2, bundled = state & 4;
    if (bundled && !original)
      continue; // The bundled data needs the vanilla oot.o2r.
    const bool playable = (archive || bundled) && original;
    Json catalogue{{"hasOriginal", original},
                   {"hasMasterQuest", archive},
                   {"mqPlayable", playable}};
    for (int mqMode : {0, 1, 2, 3}) {
      settings["gRandoSettings.MQDungeons"] = mqMode;
      const bool refused = (archive && !original) || (playable && mqMode != 0);
      const auto why =
          ProfileWallmasterRefusal(draft, catalogue, read, vanillaOnly);
      if (refused)
        plain(why, mq, mqLog);
      else
        assert(why.empty());
      // The Master Quest id takes every one of these states: no selection of
      // the MQ Dungeon Setting alone fixes the four dungeons the capacity
      // needs.
      assert(
          ProfileWallmasterRefusal(draft, catalogue, read, withQuests).empty());
    }
  }
  auto mqAssets = assets;
  mqAssets["hasMasterQuest"] = true;
  mqAssets["mqPlayable"] = true;
  plain(ProfileWallmasterRefusal(draft, mqAssets, read, vanillaOnly), mq,
        mqLog);
  settings.clear();
  assert(ProfileWallmasterRefusal(draft, mqAssets, read, vanillaOnly).empty());
  mqAssets["hasOriginal"] = false;
  mqAssets["mqPlayable"] = false;
  plain(ProfileWallmasterRefusal(draft, mqAssets, read, vanillaOnly), mq,
        mqLog);
  // An MQ-only install plays all twelve MQ: Shadow and Ganon too, so no
  // capacity shortfall.
  assert(ProfileWallmasterRefusal(draft, mqAssets, read, withQuests).empty());
  // The build's own set: the frozen sentence until the Master Quest id opens.
  assert(ProfileWallmasterRefusal(draft, mqAssets, read).player ==
         (caps.Active(kQuestsWallmasterCapability) ? "" : mq));

  // With the Master Quest id, a selection whose every native roll (settings.cpp
  // Context::FinalizeSettings) makes Spirit Temple and Bottom of the Well MQ
  // and Shadow Temple and Ganon's Castle vanilla can never place its 12 OoT
  // Wallmasters, in OoTMM either: refused with one sentence naming the rows.
  const std::string capacity = "Set Spirit Temple Quest or Bottom of the Well "
                               "Quest to Vanilla to use Wallmaster Shuffle.";
  const std::string capacityLog =
      "Master Quest Spirit Temple and Bottom of the Well with vanilla Shadow "
      "Temple and Ganon's Castle have more OoT Wallmasters than dungeon "
      "entrances";
  assert(capacity == kWallmasterCapacityText);
  auto playable = assets;
  playable["mqPlayable"] = true;
  const auto quest = [&](const char *dungeon, int value) {
    settings[std::string("gRandoSettings.MQDungeons") + dungeon] = value;
  };
  struct Case {
    int mode, presets, count, spirit, botw, shadow, ganon, others;
    bool forced;
  };
  // mode: Set Number 1, Random 2, Selection Only 3; quest: Vanilla 0, MQ 1,
  // Random 2; count -1 leaves the native default (12).
  const Case cases[] = {
      {3, 0, -1, 1, 1, 0, 0, 0, true},  {3, 0, -1, 1, 1, 0, 0, 2, true},
      {3, 0, -1, 1, 1, 1, 0, 0, false}, {3, 0, -1, 1, 1, 0, 2, 0, false},
      {3, 0, -1, 2, 1, 0, 0, 0, false}, {3, 0, -1, 2, 2, 2, 2, 2, false},
      {1, 1, 2, 1, 1, 2, 2, 2, true},   {1, 1, 3, 1, 1, 2, 2, 2, false},
      {1, 1, 10, 2, 2, 0, 0, 2, true},  {1, 1, 9, 2, 2, 0, 0, 2, false},
      {1, 0, 12, 0, 0, 0, 0, 0, false}, {1, 0, 0, 0, 0, 0, 0, 0, false},
      {1, 0, 5, 0, 0, 0, 0, 0, false},  {2, 1, -1, 1, 1, 0, 0, 2, true},
      {2, 1, -1, 1, 2, 0, 0, 0, false}, {2, 0, -1, 0, 0, 0, 0, 0, false},
  };
  for (const auto &c : cases) {
    settings.clear();
    settings["gRandoSettings.MQDungeons"] = c.mode;
    settings["gRandoSettings.MQDungeonsSelection"] = c.presets;
    if (c.count >= 0)
      settings["gRandoSettings.MQDungeonCount"] = c.count;
    for (const char *other :
         {"DekuTree", "DodongosCavern", "JabuJabu", "ForestTemple",
          "FireTemple", "WaterTemple", "IceCavern", "GTG"})
      quest(other, c.others);
    quest("SpiritTemple", c.spirit);
    quest("BottomOfTheWell", c.botw);
    quest("ShadowTemple", c.shadow);
    quest("GanonsCastle", c.ganon);
    assert(MqWallmasterShortfallForced(read, false) == c.forced);
    const auto why =
        ProfileWallmasterRefusal(draft, playable, read, withQuests);
    if (c.forced)
      plain(why, capacity, capacityLog);
    else
      assert(why.empty());
    // Without MQ data the native settings keep every dungeon vanilla.
    assert(ProfileWallmasterRefusal(draft, assets, read, withQuests).empty());
    // The frozen vanilla id keeps its own sentence for every MQ selection.
    plain(ProfileWallmasterRefusal(draft, playable, read, vanillaOnly), mq,
          mqLog);
  }
  // The roll model itself: Selection's Random dungeons are coin flips, Set
  // Number fixes its candidates when it picks none or all of them, and Random
  // picks any number.
  settings.clear();
  settings["gRandoSettings.MQDungeons"] = 3;
  quest("SpiritTemple", 1);
  quest("ShadowTemple", 2);
  auto range = MqRollRange(read, false);
  assert(range[OotMqIndex("Spirit")] == MqRoll::MasterQuest &&
         range[OotMqIndex("Shadow")] == MqRoll::Either &&
         range[OotMqIndex("DT")] == MqRoll::Vanilla);
  settings["gRandoSettings.MQDungeons"] = 1;
  settings["gRandoSettings.MQDungeonCount"] = 12;
  range = MqRollRange(read, false);
  assert(std::all_of(range.begin(), range.end(),
                     [](MqRoll roll) { return roll == MqRoll::MasterQuest; }));
  settings["gRandoSettings.MQDungeonCount"] = 4;
  range = MqRollRange(read, false);
  assert(std::all_of(range.begin(), range.end(),
                     [](MqRoll roll) { return roll == MqRoll::Either; }));
  settings["gRandoSettings.MQDungeons"] = 0;
  range = MqRollRange(read, false);
  assert(std::all_of(range.begin(), range.end(),
                     [](MqRoll roll) { return roll == MqRoll::Vanilla; }));
  range = MqRollRange(read, true);
  assert(std::all_of(range.begin(), range.end(),
                     [](MqRoll roll) { return roll == MqRoll::MasterQuest; }));
  settings.clear();
  const std::string data =
      "Wallmaster Shuffle cannot read the OoT dungeon data.";
  const std::string dataLog =
      "Wallmaster Shuffle cannot verify the available dungeon data";
  assert(data == kWallmasterDungeonDataText);
  plain(ProfileWallmasterRefusal(draft, Json::object(), read), data, dataLog);
  // A catalogue that does not say whether MQ can be played is refused.
  auto unknown = assets;
  unknown.erase("mqPlayable");
  plain(ProfileWallmasterRefusal(draft, unknown, read), data, dataLog);
  // Solo mode, Ship ER, Enemy Randomizer, glitched logic, the two song logic
  // options, six warp songs and Soaring, Master Quest, the Master Quest
  // Wallmaster capacity and unknown dungeon data.
  assert(sentences.size() == 16);
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

  // A seed made by this build: a dev build marks its layouts devBuild, which a
  // shipping build refuses before any conversion.
  EntranceLayout layout;
  layout.required = {std::string(kVanillaWallmasterCapability)};
  layout.settings.erWallmasters = Mode::OwnGame;
  layout.devBuild = caps.devBuild;
  auto imported = draft;
  imported["fileType"] = kEntranceSeedFileType;
  imported["version"] = 2;
  imported["entranceLayout"] = EncodeEntranceLayout(layout);
  const auto originalSeedSettings = imported;
  assert(SavedEntranceValue(imported, wall, 0) == 1);
  imported["setupCVars"][wall] = 0;
  assert(SavedEntranceValue(imported, wall, 0) == 1); // Saved view follows the layout.
  imported["setupCVars"].erase(wall);
#if !COMBO_ENTRANCE_DEV
  auto devSeed = imported;
  layout.devBuild = true;
  devSeed["entranceLayout"] = EncodeEntranceLayout(layout);
  Reject([&] {
    NormalizeEntranceDraft(devSeed, caps);
  }); // Shipping guard before any conversion.
  assert(devSeed.contains("entranceLayout"));
#endif
  assert(!NormalizeEntranceDraft(imported, caps));
  assert(!imported.contains("entranceLayout") &&
         imported["fileType"] == kComboSeedFileType);
  assert(imported["setupCVars"][wall] == 1 && imported["version"] == 1);
  RequireSupportedSettingsCapabilities(imported, caps);
  imported["setupCVars"][wall] = 0;
  assert(!NormalizeEntranceDraft(imported, caps));
  assert(imported["setupCVars"][wall] ==
         0); // Old envelope cannot turn a cleared row back on.
  assert(originalSeedSettings.contains("entranceLayout"));
  Json profile = draft;
  profile["setupCVars"][wall] = 1;
  assert(EntranceSettingsForHeadless(profile, {}, "", false, caps)
             .settings.erWallmasters == Mode::OwnGame);
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
