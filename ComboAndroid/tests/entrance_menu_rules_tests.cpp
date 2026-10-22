#include "gui/ComboMenuPlacement.h"
#include "headless_settings.inc"
#include "rando/GenerationRules.h"
#include "rando/entrances/EntranceProfile.h"
#include "rando/entrances/TempleAdapter.h"
#include <cassert>
#include <fstream>
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
int main(int argc, char **argv) {
  assert(argc == 2); // a seed of the 0.4.12 release
  const auto caps = ActiveCapabilities();
  // The frozen vanilla id alone, and the catalog with the Master Quest id
  // (U11), whatever this build's stages.
  const auto vanillaOnly =
      CapabilitySet::AtLeast(Stage::Hidden).Only(kVanillaWallmasterCapability);
  const auto withQuests = CapabilitySet::AtLeast(Stage::Hidden);
  // Every Own Game Wallmaster id but the starting-song one (limits design,
  // Stage 2), as in a build without it, which none is now.
  const auto withoutStartingSongs =
      withQuests.Only(kVanillaWallmasterCapability) |
      withQuests.Only(kQuestsWallmasterCapability) |
      withQuests.Only(kSongsWallmasterCapability);
  const auto *row = FindEntranceSetting("erWallmasters");
  assert(row);
  // Both builds offer the row with None and Own Game (P1-05), and Full where
  // the build has wallmasters-full-v1 (limits design, Stage 3), by OoTMM's
  // value name, under the row's own description (data.ts:3545-3555: Full has
  // no description of its own).
  const bool full = caps.Active(kFullWallmasterCapability);
  assert(row->visible(caps.Selected("")) &&
         row->supports(1, caps.Selected("")));
  assert(row->supports(2, caps.Selected("")) == full);
  const auto catalogue = EntranceProfileCatalog(caps);
  assert(catalogue.size() == 1 &&
         catalogue[0]["choices"].size() == (full ? 3u : 2u));
  assert(catalogue[0]["label"] == row->name &&
         catalogue[0]["description"] == row->description);
  if (full)
    assert(catalogue[0]["choices"][2] ==
           (Json{{"value", 2}, {"label", "Full"}, {"description", ""}}));
  assert(std::string(row->description) ==
         "Enables the ability for Wallmasters to take you to random locations "
         "within their own game or across both games, based on other entrance "
         "settings");
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
  settings["gCombo.Rando.SongTravel"] = 1;
  check("Turn off Warp Songs and Soaring Spots Shuffle to use Wallmaster "
        "Shuffle.",
        "song travel logic needs GLOBAL song sources (deferred)");
  settings.clear();
  // "Play warp songs across games" (limits design, Stage 1): the Temple gate
  // takes it (lemma S1), and so do the menu and generation, which agree: every
  // build has wallmasters-own-game-songs-v1, and its sentence is gone, whatever
  // the capability set. A starting warp song needs the songs-v2 id.
  assert(caps.Active(kSongsWallmasterCapability));
  settings["gCombo.Rando.ForeignSongs"] = 1;
  {
    const auto facts = ProfileEntranceFacts(draft, read);
    TempleFacts temple;
    static_cast<EntranceSettingsFacts &>(temple) = facts;
    Settings s;
    s.erWallmasters = Mode::OwnGame;
    CheckTempleCapability(s, temple, [](std::string_view) { return true; });
    assert(facts.foreignSongsLogic && TempleSettingsRefusal(temple).empty());
    for (const auto &set : {caps, vanillaOnly, withQuests})
      assert(ProfileWallmasterRefusal(draft, assets, read, set).empty());
    assert(EntranceGenerationRefusal(facts).empty());
    settings["gRandoSettings.StartingPreludeOfLight"] = 1;
    assert(
        EntranceGenerationRefusal(ProfileEntranceFacts(draft, read)).empty());
    for (const auto &set : {caps, withQuests, withoutStartingSongs})
      assert(ProfileWallmasterRefusal(draft, assets, read, set).empty());
  }
  settings.clear();
  // Ship's Logic row offers Glitchless (0) and No Logic (1) only.
  settings[kNativeLogicRule] = 2;
  check("Set Logic to Glitchless to use Wallmaster Shuffle.",
        "only glitchless OoT logic is supported");
  settings.clear();
  const std::string use = " from the starting items to use Wallmaster Shuffle.";
  // A starting OoT warp song (limits design, Stage 2): the Temple gate takes it
  // (the probe's GLOBAL node, lemma L3), and so do the menu and generation,
  // which agree: every build has wallmasters-own-game-songs-v2, and the
  // sentence "Remove <song> from the starting items to use Wallmaster
  // Shuffle." is gone for the six songs, whatever the capability set. A set
  // without the id, which no build has, refuses the song in generation with
  // developer text only (entrance_vanilla_generator_tests.cpp).
  for (const auto &[key, name] :
       {std::pair{"StartingMinuetOfForest", "Minuet of Forest"},
        std::pair{"StartingBoleroOfFire", "Bolero of Fire"},
        std::pair{"StartingSerenadeOfWater", "Serenade of Water"},
        std::pair{"StartingRequiemOfSpirit", "Requiem of Spirit"},
        std::pair{"StartingNocturneOfShadow", "Nocturne of Shadow"},
        std::pair{"StartingPreludeOfLight", "Prelude of Light"}}) {
    settings[std::string("gRandoSettings.") + key] = 1;
    const auto facts = ProfileEntranceFacts(draft, read);
    TempleFacts temple;
    static_cast<EntranceSettingsFacts &>(temple) = facts;
    Settings s;
    s.erWallmasters = Mode::OwnGame;
    CheckTempleCapability(s, temple, [](std::string_view) { return true; });
    assert(StartingWarpSong(facts) == name &&
           TempleSettingsRefusal(temple).empty() &&
           EntranceGenerationRefusal(facts).empty());
    assert(caps.Active(kStartingSongsWallmasterCapability));
    for (const auto &set :
         {caps, vanillaOnly, withQuests, withoutStartingSongs})
      assert(ProfileWallmasterRefusal(draft, assets, read, set).empty());
    settings.clear();
  }
  // The MM side's names count too, as the seed guard reads them.
  draft["mm"]["settings"]["gRando.StartingItems"] = {"Minuet of Forest"};
  assert(StartingWarpSong(ProfileEntranceFacts(draft, read)) ==
             "Minuet of Forest" &&
         ProfileWallmasterRefusal(draft, assets, read, vanillaOnly).empty());
  // A starting Song of Soaring stays refused by every id: it needs a Temple
  // check through MM. MM's starting items as the menu writes them are 2Ship's
  // spoiler names, the MM_DumpProfileCatalog values (BenPort.cpp); the display
  // name counts too.
  for (const char *soaring : {"Song of Soaring", "RI_SONG_SOARING"}) {
    draft["mm"]["settings"]["gRando.StartingItems"] = {"RI_SONG_TIME", soaring};
    check(
        "Remove Song of Soaring" + use,
        "starting Song of Soaring needs a Temple check through MM (deferred)");
  }
  settings["gRandoSettings.StartingPreludeOfLight"] = 1;
  check("Remove Song of Soaring" + use,
        "starting Song of Soaring needs a Temple check through MM (deferred)");
  settings.clear();
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
  const std::string legacyLog = "Wallmaster Shuffle supports Master Quest "
                                "dungeons only from OoTMM's bundled data";
  assert(mq == kWallmasterMasterQuestText);
  // Every catalogue state (OTRGlobals.cpp SOH_DumpProfileCatalog): with the
  // frozen vanilla id only, MQ selected is refused wherever MQ can be played,
  // from a real oot-mq.o2r or OoTMM's bundled data (mqPlayable), and an MQ-only
  // install is always refused. The Master Quest id takes them only where a new
  // seed's MQ dungeons are OoTMM's bundled data (mqSource "ootmm-32.3"): with a
  // real oot-mq.o2r alone (Legacy) they roll on the shared stream, and keep the
  // same sentence.
  for (int state = 0; state < 8; ++state) {
    const bool original = state & 1, archive = state & 2, bundled = state & 4;
    if (bundled && !original)
      continue; // The bundled data needs the vanilla oot.o2r.
    const bool playable = (archive || bundled) && original;
    Json catalogue{{"hasOriginal", original},
                   {"hasMasterQuest", archive},
                   {"mqPlayable", playable},
                   {"mqSource", bundled   ? "ootmm-32.3"
                                : archive ? "oot-mq.o2r"
                                          : "none"}};
    assert(CatalogueMqBundled(catalogue) == bundled);
    for (int mqMode : {0, 1, 2, 3}) {
      settings["gRandoSettings.MQDungeons"] = mqMode;
      const bool refused = (archive && !original) || (playable && mqMode != 0);
      const auto why =
          ProfileWallmasterRefusal(draft, catalogue, read, vanillaOnly);
      if (refused)
        plain(why, mq, mqLog);
      else
        assert(why.empty());
      // With the bundled data the Master Quest id takes every one of these
      // states: no selection of the MQ Dungeon Setting alone fixes the four
      // dungeons the capacity needs. Legacy data keeps the refusal.
      const auto quests =
          ProfileWallmasterRefusal(draft, catalogue, read, withQuests);
      if (refused && !bundled)
        plain(quests, mq, legacyLog);
      else
        assert(quests.empty());
      // A catalogue without mqSource is not bundled.
      auto old = catalogue;
      old.erase("mqSource");
      assert(!CatalogueMqBundled(old) &&
             ProfileWallmasterRefusal(draft, old, read, withQuests).player ==
                 (refused ? mq : ""));
    }
  }
  auto mqAssets = assets;
  mqAssets["hasMasterQuest"] = true;
  mqAssets["mqPlayable"] = true;
  mqAssets["mqSource"] = "oot-mq.o2r";
  plain(ProfileWallmasterRefusal(draft, mqAssets, read, vanillaOnly), mq,
        mqLog);
  settings.clear();
  assert(ProfileWallmasterRefusal(draft, mqAssets, read, vanillaOnly).empty());
  mqAssets["hasOriginal"] = false;
  mqAssets["mqPlayable"] = false;
  plain(ProfileWallmasterRefusal(draft, mqAssets, read, vanillaOnly), mq,
        mqLog);
  // An MQ-only install plays all twelve MQ from a real oot-mq.o2r: Legacy
  // data, refused with either id, whatever this build's stages.
  plain(ProfileWallmasterRefusal(draft, mqAssets, read, withQuests), mq,
        legacyLog);
  assert(ProfileWallmasterRefusal(draft, mqAssets, read).player == mq);

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
  // The generation error of a random roll that lands there on the seed's own
  // first attempt (UnifiedBridge.h WallmasterShortfallStopsGeneration).
  const std::string seedCapacity =
      "This seed's Master Quest dungeons have more Wallmasters than Wallmaster "
      "Shuffle can place. Try another seed.";
  assert(seedCapacity == kWallmasterCapacitySeedText);
  plain(WallmasterSeedCapacityRefusal(), seedCapacity, capacityLog);
  auto playable = assets;
  playable["mqPlayable"] = true;
  playable["mqSource"] = "ootmm-32.3";
  // The same selections from a real oot-mq.o2r (Legacy): the Master Quest
  // sentence, never the capacity one.
  auto legacy = playable;
  legacy["hasMasterQuest"] = true;
  legacy["mqSource"] = "oot-mq.o2r";
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
    // Full gives every source both games' 18 dungeons, so it is never short:
    // the same selections are offered with Full, as generation places them.
    assert(!MqWallmasterShortfallForced(read, false, Mode::Full));
    assert(ProfileWallmasterRefusal(draft, playable, read, withQuests,
                                    Mode::Full)
               .empty());
    plain(ProfileWallmasterRefusal(draft, legacy, read, withQuests, Mode::Full),
          mq, legacyLog);
    plain(ProfileWallmasterRefusal(draft, legacy, read, withQuests), mq,
          legacyLog);
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
  // Solo mode, Ship ER, Enemy Randomizer, glitched logic, song destination
  // shuffle, Soaring, Master Quest, the Master Quest Wallmaster capacity
  // (forced, and a seed's roll) and unknown dungeon data. "Play warp songs
  // across games" and the six warp songs have none since 0.4.13.
  assert(sentences.size() == 10);
  for (const auto &sentence : sentences)
    assert(sentence.find("across games") == std::string::npos &&
           (sentence.find("Remove ") == std::string::npos ||
            sentence.find("Song of Soaring") != std::string::npos));
  auto raw = draft;
  raw["setupCVars"][wall] = 2;
  raw["setupCVars"]["gCombo.Rando.Entrances.erOneWays"] = 1;
  raw["setupCVars"]["gRandoSettings.ShuffleWarpSongs"] = 1;
  raw["futureExtension"] = {{"preserve", 123}};
  const auto immutable = raw;
  assert(NormalizeEntranceDraft(raw, caps));
  // Full stays only where this build has wallmasters-full-v1, and alone.
  assert(raw["setupCVars"][wall] == (caps.Active(kFullWallmasterCapability) ? 2 : 0));
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
  // Full is a profile value only where this build has wallmasters-full-v1.
  if (caps.Active(kFullWallmasterCapability))
    assert(EntranceSettingsForHeadless(profile, {}, "", false, caps)
               .settings.erWallmasters == Mode::Full);
  else
    Reject([&] { EntranceSettingsForHeadless(profile, {}, "", false, caps); });
  profile["setupCVars"][wall] = uint64_t(1) << 40;
  Reject([&] { EntranceSettingsForHeadless(profile, {}, "", false, caps); });
  profile.erase("setupCVars");
  assert(MakePools(
             EntranceSettingsForHeadless(profile, {}, "", false, caps).settings)
             .empty());
  // A seed of the 0.4.12 release (fixtures/wallmasters-vanilla-v1) as it was
  // dumped, whose MM starting items are 2Ship's spoiler names. With Soaring
  // added as the MM menu adds it, the menu, generation, the Temple gate and the
  // seed guard refuse it with one sentence, under its own id and the songs id.
  {
    std::ifstream file(argv[1]);
    const auto released = Json::parse(file);
    assert(released.at("mm").at("settings").at("gRando.StartingItems") ==
           Json::array({"RI_PROGRESSIVE_SWORD", "RI_SHIELD_HERO", "RI_OCARINA",
                        "RI_SONG_TIME"}));
    const auto seedRead = [&](const char *key, int fallback) {
      return released.at("oot").at("settings").value(key, fallback);
    };
    SongTravelMap foreign;
    foreign.foreignSongs = foreign.logicForeignSongs = true;
    auto songsLayout = DecodeEntranceLayout(released.at("entranceLayout"));
    assert(songsLayout.required ==
           std::vector{std::string(kVanillaWallmasterCapability)});
    songsLayout.required = {std::string(kSongsWallmasterCapability)};
    auto songsSeed = released;
    songsSeed["entranceLayout"] = EncodeEntranceLayout(songsLayout);
    songsSeed["songTravel"] = foreign.json();
    const std::string sentence = "Remove Song of Soaring from the starting "
                                 "items to use Wallmaster Shuffle.",
                      log = "starting Song of Soaring needs a Temple check "
                            "through MM (deferred)";
    const Json *seeds[] = {&released, &songsSeed};
    for (const auto *seed : seeds) {
      RequireSupportedSeedCapabilities(*seed, caps);
      assert(ProfileWallmasterRefusal(*seed, assets, seedRead).empty());
      auto soaring = *seed;
      soaring["mm"]["settings"]["gRando.StartingItems"].push_back(
          "RI_SONG_SOARING");
      const auto facts = SeedEntranceFacts(soaring);
      assert(facts.mmStartingItems.back() == "Song of Soaring");
      assert(ProfileEntranceFacts(soaring, seedRead).mmStartingItems ==
             facts.mmStartingItems);
      const auto menu = ProfileWallmasterRefusal(soaring, assets, seedRead);
      plain(menu, sentence, log);
      // Generation reads the MM dump's display names (BenPort.cpp
      // sharedStartingItems).
      TempleFacts generated;
      static_cast<EntranceSettingsFacts &>(generated) = facts;
      generated.mmStartingItems = {"Song of Time", "Song of Soaring"};
      assert(menu == EntranceGenerationRefusal(generated) &&
             menu == TempleSettingsRefusal(generated));
      std::string why;
      try {
        RequireSupportedSeedCapabilities(soaring, caps);
      } catch (const UnsupportedComboSeedFormat &error) {
        why = error.what();
      }
      assert(why.find(log) != std::string::npos);
    }
  }
  std::cout << "PASS: shared menu/generation/Temple refusals, "
            << sentences.size()
            << " plain refusal sentences, vanilla asset policy, capability "
               "choices and detached draft normalization\n";
}
