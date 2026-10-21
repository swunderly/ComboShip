// The 0.4.13 release corpus (fixtures/wallmasters-0413/). Built twice by
// entrance_corpus_0413_tests.py, with and without COMBO_ENTRANCE_DEV, after
// that script has checked every fixture against the manifest and the plan.
//
// Each seed was written by the 0.4.13 SHIPPING headless generator. It must keep
// its exact bytes when read and written again, pass the seed guard, play OoT
// with OoTMM's scene restriction table, record the Master Quest dungeons the
// manifest names from OoTMM's bundled data, and replay them. A Wallmaster seed
// must require the manifest's capability id with the seed's own quest context,
// validate against the shipping capability set and this build's, install into
// both games through the slot bind's installer with the routes the manifest
// records (the OoT installer, as SetLayout, only where the loaded seed's quests
// are the layout's), install again from what the games report back, give the
// recorded entrance spoiler and the ER runtime flag, and keep its layout in
// the profile view. A seed without entrance settings has none of that.
#include "ComboRandomizerSavedSeed.h"
#include "mq/MqSeedMarker.h"
#include "rando/OotSceneRestrictions.h"
#include "rando/entrances/EntranceSpoilerOutput.h"
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace ComboRando::Entrances;
using Json = nlohmann::json;
namespace fs = std::filesystem;
namespace Profiles = ComboRando::Profiles;

// The games' installers for a unified layout, as SOH_SetOotmmEntranceLayout
// (combo_entrance_overlay.cpp SetLayout) and MM_ApplyEntranceExpansionForCombo
// run them. sLoadedQuests is the native seed's quest context (ReadQuests).
static OotUnifiedLayout sOot;
static MmUnifiedLayout sMm;
static OotQuests sLoadedQuests{};
static std::string sOotDump, sMmDump;
static int ApplyOot(const char *text) {
  sOot.clear();
  try {
    const auto seed = Json::parse(text ? text : "null");
    if (!seed.is_object())
      return 0;
    if (seed.contains("entranceLayout")) {
      sOot.install(seed, ActiveCapabilities());
      if (sOot.hasOotRoutes() && *sOot.quests() != sLoadedQuests) {
        sOot.clear();
        return 0;
      }
    }
    return 1;
  } catch (const std::exception &) {
    sOot.clear();
    return 0;
  }
}
static int ApplyMm(const char *text) {
  sMm.clear();
  try {
    const auto seed = Json::parse(text ? text : "null");
    if (seed.contains("entranceLayout"))
      sMm.install(seed, ActiveCapabilities());
    else
      for (const char *key : ComboRando::kLegacyEntranceKeys)
        if (seed.contains(key))
          return 0; // the private MM profiles are not part of this corpus
    return 1;
  } catch (const std::exception &) {
    sMm.clear();
    return 0;
  }
}
static const char *DumpOot() {
  sOotDump =
      (sOot.active() ? Json{{"entranceLayout", sOot.json()}} : Json::object())
          .dump();
  return sOotDump.c_str();
}
static const char *DumpMm() {
  sMmDump =
      (sMm.active() ? Json{{"entranceLayout", sMm.json()}} : Json::object())
          .dump();
  return sMmDump.c_str();
}
static NativeFns sEntranceNative{ApplyMm,  nullptr, nullptr, DumpMm,  nullptr,
                                 ApplyOot, nullptr, nullptr, DumpOot, nullptr};
// InstallEntranceSeed and CheckPendingEntranceSeed, unchanged.
#include "corpus_0413_launcher.inc"

static void Require(bool ok, const std::string &where,
                    const std::string &what) {
  if (!ok) {
    std::cout << "FAIL: " << where << ": " << what << "\n";
    std::exit(1);
  }
}
template <class F>
static auto Must(const std::string &where, const std::string &what, F action) {
  try {
    return action();
  } catch (const std::exception &error) {
    std::cout << "FAIL: " << where << ": " << what << ": " << error.what()
              << "\n";
    std::exit(1);
  }
}
static std::string ReadBytes(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  Require(bool(input), path.string(), "cannot be read");
  return {std::istreambuf_iterator<char>(input), {}};
}

static void CheckSeed(const fs::path &root, const Json &row,
                      const CapabilitySet &active) {
  const std::string id = row.at("id");
  const auto bytes = ReadBytes(root / row.at("seed").get<std::string>());
  const auto seed =
      Must(id, "the seed does not parse", [&] { return Json::parse(bytes); });
  Require(seed.is_object() && seed.dump(2) == bytes, id,
          "the seed does not re-encode to its own bytes");
  const auto shipping = CapabilitySet::AtLeast(Stage::Public);
  for (const auto *caps : {&shipping, &active})
    Must(id, "the seed guard refuses it",
         [&] { ComboRando::RequireSupportedSeedDocument(seed, *caps); });

  // OoTMM's scene restriction table, and the bundled Master Quest dungeons the
  // manifest names, which a replay must roll again.
  Require(ComboRando::SeedUsesOotmmSceneRestrictions(seed), id,
          "the seed does not play OoT with OoTMM's scene restriction table");
  const auto &oot = seed.at("oot");
  int mask = 0;
  OotQuests quests{};
  for (const auto &name : row.at("facts").at("masterQuestDungeons")) {
    const auto index = OotMqIndex(name.get<std::string>());
    Require(index < quests.size(), id, "unknown Master Quest dungeon");
    quests[index] = true;
    mask |= 1 << index;
  }
  Require(ComboMq::MqDataSourceOf(oot) == ComboMq::SeedMqSource::Bundled &&
              Must(id, "the Master Quest record is refused",
                   [&] { return ComboMq::RecordedMqDungeonMask(oot); }) ==
                  mask &&
              ComboMq::SeedMqPlayable(oot, true) &&
              ComboMq::ReplayKeepsMqDungeons(oot, mask) &&
              !ComboMq::ReplayKeepsMqDungeons(oot, mask ^ 1),
          id, "the seed does not record its bundled Master Quest dungeons");

  if (!row.contains("entrances")) {
    // No entrance settings: no layout, no spoiler, no ER runtime flag, and
    // the slot bind leaves both games native.
    Require(!seed.contains("entranceLayout") &&
                ComboRando::SeedEntranceRuntimeFlags(seed) == 0,
            id, "a seed without entrance settings has a layout");
    Require(Must(id, "the entrance spoiler is refused",
                 [&] { return EntranceSpoilerForSeed(seed, active); })
                .empty(),
            id, "a seed without a layout has an entrance spoiler");
    sLoadedQuests = quests;
    Require(InstallEntranceSeed(seed) && !sOot.active() && !sMm.active(), id,
            "the slot bind does not leave both games native");
    return;
  }
  Require(seed.value("fileType", "") == ComboRando::kEntranceSeedFileType &&
              seed.value("version", 0) == ComboRando::kEntranceSeedVersion,
          id, "not a version 2 entrance layout seed");
  const auto &value = seed.at("entranceLayout");
  const auto layout = Must(id, "the layout does not decode",
                           [&] { return DecodeEntranceLayout(value); });
  Settings ownGame;
  ownGame.erWallmasters = Mode::OwnGame;
  const std::string capability = row.at("facts").at("capability");
  Require(layout.required == std::vector<std::string>{capability} &&
              !layout.devBuild && layout.ootMq == quests &&
              SameEntranceSettings(layout.settings, ownGame),
          id,
          "not a shipping Own Game Wallmaster layout of " + capability +
              " in the seed's quest context");
  Require(EncodeEntranceLayout(layout).dump() == value.dump(), id,
          "the layout does not re-encode byte-identically");
  ValidatedLayout validated;
  for (const auto *caps : {&shipping, &active})
    validated = Must(id, "the frozen rules of its id refuse the layout",
                     [&] { return Validate(layout, *caps); });

  // The slot bind installs it into both games where the loaded seed has the
  // layout's quests, and in no other quest context.
  sLoadedQuests = quests;
  sLoadedQuests[OotMqIndex("Forest")] = !sLoadedQuests[OotMqIndex("Forest")];
  Require(!InstallEntranceSeed(seed) && !sOot.active(), id,
          "the OoT installer takes the layout in another quest context");
  sLoadedQuests = quests;
  Require(InstallEntranceSeed(seed) && sOot.json() == value &&
              sMm.json() == value,
          id, "the slot bind refuses it or a game changes it");
  std::map<std::string, std::string> recorded;
  for (const auto &route : row.at("identity").at("routes"))
    recorded.emplace(route.at(0).get<std::string>(),
                     route.at(1).get<std::string>());
  Require(recorded.size() == validated.oot.size() + validated.mm.size(), id,
          "the manifest records another number of routes");
  for (const auto *routes : {&validated.oot, &validated.mm})
    for (const auto &route : *routes) {
      const auto &src = *route.source, &dst = *route.destination;
      const auto where = id + " " + std::string(src.name);
      const auto it = recorded.find(std::string(src.name));
      Require(it != recorded.end() && it->second == dst.name, where,
              "the manifest records another destination");
      const auto *liveSrc = Find(src.name), *liveDst = Find(dst.name);
      Require(liveSrc && liveDst && liveSrc->game == src.game &&
                  liveSrc->id == src.id && liveDst->game == dst.game &&
                  liveDst->id == dst.id,
              where, "the live registry names other rows");
      Require(src.game == Game::Oot
                  ? sOot.routesFrom(src.id) ==
                        std::vector<OotUnifiedRoute>{{dst.id}}
                  : sMm.routesFrom(src.id) ==
                        std::vector<MmUnifiedRoute>{MakeMmUnifiedRoute(dst)},
              where, "the installed route differs");
    }
  Require(sOot.routes().size() == validated.oot.size() &&
              sMm.routes().size() == validated.mm.size(),
          id, "a game installed other routes");
  const auto ootRoutes = sOot.routes();
  const auto mmRoutes = sMm.routes();
  auto reported = seed;
  reported["entranceLayout"] = Json::parse(DumpMm()).at("entranceLayout");
  Require(InstallEntranceSeed(reported) && sOot.routes() == ootRoutes &&
              sMm.routes() == mmRoutes,
          id, "installing the reported layout again changes its routes");
  // Importing it checks it without unbinding the bound slot.
  Require(CheckPendingEntranceSeed(seed) && sOot.json() == value &&
              sMm.json() == value,
          id, "the import check refuses it or changes the bound slot");

  Require(EntranceSpoilerForSeed(seed, active) ==
              ReadBytes(root / row.at("entrances").get<std::string>()),
          id, "the entrance spoiler differs from the recorded one");
  Require(ComboRando::SeedEntranceRuntimeFlags(seed) == kEntranceRuntimeErAny,
          id, "the seed does not turn on the ER runtime flag");
  auto routeless = value;
  routeless.erase("overrides");
  Must(id, "the profile view refuses it", [&] {
    Require(Profiles::SettingsDocument(seed, active).at("entranceLayout") ==
                routeless,
            id, "the profile view changes the layout");
    Profiles::Store store(seed, active);
    store.importSeed(2, seed, true);
    Require(store.pendingSeed(2) == seed &&
                store.draft(2).at("entranceLayout") == routeless,
            id, "the profile import changes the seed");
    Profiles::Store reread(seed, active);
    reread.restore(store.json());
    Require(reread.json() == store.json(), id,
            "the profile store does not restore it");
  });
}

int main(int argc, char **argv) {
  assert(argc == 2);
  const auto manifestPath = fs::absolute(argv[1]);
  const auto root = manifestPath.parent_path();
  const auto manifest = Json::parse(ReadBytes(manifestPath));
  const auto active = ActiveCapabilities();
#ifdef COMBO_ENTRANCE_DEV
  assert(active.devBuild);
#else
  assert(!active.devBuild && active == CapabilitySet::AtLeast(Stage::Public));
#endif
  std::map<std::string, int> ids;
  for (const auto &row : manifest.at("seeds")) {
    CheckSeed(root, row, active);
    const auto &capability = row.at("facts").at("capability");
    ids[capability.is_null() ? "(none)" : capability.get<std::string>()]++;
  }
  std::cout << "PASS " << manifest.at("seeds").size()
            << " 0.4.13 release corpus seeds (";
  for (const auto &[id, count] : ids)
    std::cout << id << " " << count << "; ";
  std::cout << "): exact bytes, seed guard, scene restriction table, Master "
               "Quest record and replay, frozen rules of each id, install in "
               "the seed's quest context only, replay and import in both "
               "games, entrance spoiler, profile view ("
            << (active.devBuild ? "dev" : "shipping") << " build)\n";
}
