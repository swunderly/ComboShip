// C6 permanence fixtures (M1 plan A-12 host half and A-13, P1-06). Built twice
// by entrance_corpus_tests.py, with and without COMBO_ENTRANCE_DEV, after that
// script has checked every fixture against the SHA-256 manifest.
//
// Each corpus seed was written by the 0.4.12 SHIPPING headless generator. It
// must keep its exact bytes when read and written again, pass the seed guard,
// validate against the shipping capability set and this build's, install into
// both games through the slot bind's installer with the routes the manifest
// records, install again from what the games report back, pass the import
// check without disturbing the bound slot, give the recorded entrance spoiler
// and the ER runtime flag, and keep its layout in the profile view.
//
// Each File 3 save was made by the app from a corpus seed. Its container must
// keep its bytes, stay on this build's save line, hold that seed exactly, and
// go through the real launcher read (Combo_ReadGameSave, LoadOrCreateContainer)
// and the native preflight with only the ER runtime flag added. The bind must
// restore the saved layout into both games. A saved seed changed in a way this
// build must refuse is refused before the slot is touched, and a build with no
// capability refuses it before decoding, as 0.4.3 and 0.4.4 do.
#include "ComboRandomizerSavedSeed.h"
#include "rando/entrances/EntranceSpoilerOutput.h"
#include "rando/entrances/MmUnifiedLayout.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/OotUnifiedLayout.h"
#include <cassert>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

using namespace ComboRando::Entrances;
using ComboRando::UnsupportedComboSeedFormat;
using Json = nlohmann::json;
namespace fs = std::filesystem;
namespace Profiles = ComboRando::Profiles;

// The games' installers for a unified layout, as SOH_SetOotmmEntranceLayout
// (combo_entrance_overlay.cpp SetLayout) and MM_ApplyEntranceExpansionForCombo
// run them. Every dungeon is vanilla, as in the release corpus.
static OotUnifiedLayout sOot;
static MmUnifiedLayout sMm;
static std::string sOotDump, sMmDump;
static int ApplyOot(const char *text) {
  sOot.clear();
  try {
    const auto seed = Json::parse(text ? text : "null");
    if (!seed.is_object())
      return 0;
    if (seed.contains("entranceLayout")) {
      sOot.install(seed, ActiveCapabilities());
      if (sOot.hasOotRoutes() && *sOot.quests() != OotQuests{}) {
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

// The launcher state the production functions below use (combo/ComboShip.cpp).
static std::mutex g_containerMutex;
static std::map<int, Json> g_containerCache;
static std::vector<int> g_evictedSlots;
// ComboIsValidSlot, ComboContainerPath, ComboReleaseMajorMinor,
// LoadOrCreateContainer, Combo_ReadGameSave, InstallEntranceSeed and
// CheckPendingEntranceSeed, unchanged, with this checkout's
// COMBO_RELEASE_VERSION.
#include "corpus_launcher.inc"

static void Require(bool ok, const std::string &where,
                    const std::string &what) {
  if (!ok) {
    std::cout << "FAIL: " << where << ": " << what << "\n";
    std::exit(1);
  }
}
// The result of action, or a failure naming the fixture and the error.
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
template <class F> static std::string Refusal(F action) {
  try {
    action();
  } catch (const UnsupportedComboSeedFormat &error) {
    return error.what();
  }
  return {};
}
static std::string ReadBytes(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  Require(bool(input), path.string(), "cannot be read");
  return {std::istreambuf_iterator<char>(input), {}};
}
static void WriteBytes(const fs::path &path, const std::string &bytes) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output << bytes;
  output.close();
  assert(output);
}

// One corpus seed. bound is the seed the previous row left bound, if any.
static Json CheckSeed(const fs::path &root, const Json &row,
                      const CapabilitySet &active, const Json *bound) {
  const std::string id = row.at("id");
  const auto bytes = ReadBytes(root / row.at("seed").get<std::string>());
  const auto seed =
      Must(id, "the seed does not parse", [&] { return Json::parse(bytes); });
  // The headless writer's bytes (consolidated.dump(2)) come back unchanged.
  Require(seed.is_object() && seed.dump(2) == bytes, id,
          "the seed does not re-encode to its own bytes");
  const auto shipping = CapabilitySet::AtLeast(Stage::Public);
  for (const auto *caps : {&shipping, &active})
    Must(id, "the seed guard refuses it",
         [&] { ComboRando::RequireSupportedSeedDocument(seed, *caps); });
  Require(seed.value("fileType", "") == ComboRando::kEntranceSeedFileType &&
              seed.value("version", 0) == ComboRando::kEntranceSeedVersion,
          id, "not a version 2 entrance layout seed");

  const auto &value = seed.at("entranceLayout");
  const auto layout = Must(id, "the layout does not decode",
                           [&] { return DecodeEntranceLayout(value); });
  Settings vanilla;
  vanilla.erWallmasters = Mode::OwnGame;
  Require(layout.required == std::vector<std::string>{std::string(
                                 kVanillaWallmasterCapability)} &&
              !layout.devBuild && layout.ootMq == OotQuests{} &&
              SameEntranceSettings(layout.settings, vanilla),
          id,
          "not a shipping layout of Wallmaster Shuffle (Own Game) "
          "with vanilla dungeons");
  Require(EncodeEntranceLayout(layout).dump() == value.dump(), id,
          "the layout does not re-encode byte-identically");
  ValidatedLayout validated;
  for (const auto *caps : {&shipping, &active})
    validated = Must(id, "the frozen v1 rules refuse the layout",
                     [&] { return Validate(layout, *caps); });
  Require(validated.oot.size() == std::size(kOotVanillaWallmasterSources) &&
              validated.mm.size() == std::size(kMmWallmasterSources),
          id, "the layout does not route every Wallmaster source");

  // Importing it checks it without unbinding the slot that is bound.
  if (bound) {
    Require(InstallEntranceSeed(*bound), id, "the previous seed did not bind");
    const auto live = bound->at("entranceLayout");
    Require(CheckPendingEntranceSeed(seed) && sOot.json() == live &&
                sMm.json() == live,
            id, "the import check refuses it or changes the bound slot");
  }
  // The slot bind installs it into both games, by the frozen ids, which the
  // live registry still names the same, and with the manifest's routes.
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
  // Replay: what the games report back installs the same routes again.
  const auto ootRoutes = sOot.routes();
  const auto mmRoutes = sMm.routes();
  auto reported = seed;
  reported["entranceLayout"] = Json::parse(DumpMm()).at("entranceLayout");
  Require(InstallEntranceSeed(reported) && sOot.routes() == ootRoutes &&
              sMm.routes() == mmRoutes,
          id, "installing the reported layout again changes its routes");

  Require(EntranceSpoilerForSeed(seed, active) ==
              ReadBytes(root / row.at("entrances").get<std::string>()),
          id, "the entrance spoiler differs from the recorded one");
  Require(ComboRando::SeedEntranceRuntimeFlags(seed) == kEntranceRuntimeErAny,
          id, "the seed does not turn on the ER runtime flag");
  // The profile view keeps the layout without its routes, and an imported
  // seed survives the profile store's save and restore.
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
  return seed;
}

// One File 3 save fixture, made by the app from the corpus seed seed.
static void CheckSave(const fs::path &root, const fs::path &work,
                      const Json &row, const Json &seed,
                      const CapabilitySet &active) {
  const std::string id = row.at("id");
  const auto bytes = ReadBytes(root / row.at("file").get<std::string>());
  const auto container = Must(id, "the container does not parse",
                              [&] { return Json::parse(bytes); });
  // FlushContainer's bytes (dump()) come back unchanged.
  Require(container.is_object() && container.dump() == bytes, id,
          "the container does not re-encode to its own bytes");
  const int slot = row.at("slot");
  Require(slot == 2 && container.value("slot", -1) == slot, id,
          "not a File 3 container");
  Require(container.contains("oot") && container.at("oot").is_object() &&
              container.contains("mm") && container.at("mm").is_object(),
          id, "it does not hold a save page of each game");
  Require((container.value("combo", Json::object()).value("visitedGames", 0) &
           3) == 3,
          id, "it was not played in both games");
  Require(container.value("comboRelease", Json()).is_string() &&
              ComboReleaseMajorMinor(
                  container.at("comboRelease").get<std::string>()) ==
                  ComboReleaseMajorMinor(COMBO_RELEASE_VERSION),
          id,
          "this launcher would back it up instead of loading it: it is from "
          "another release line than COMBO_RELEASE_VERSION " +
              std::string(COMBO_RELEASE_VERSION));

  // The slot holds the corpus seed exactly, in either stored form, and both
  // the shipping set and this build read it back with its catalogs, native
  // seed rules and ER runtime flag.
  const auto shipping = CapabilitySet::AtLeast(Stage::Public);
  const auto stringForm = [&] {
    auto form = container;
    form["combo"]["rando"] = seed.dump();
    return form;
  }();
  for (const auto *caps : {&shipping, &active}) {
    for (const auto *form : {&container, &stringForm})
      Require(Must(id, "the saved seed is refused",
                   [&] { return Profiles::SeedFromContainer(*form, *caps); }) ==
                  seed,
              id, "the saved seed is not its corpus seed");
    Must(id, "the saved catalogs are refused",
         [&] { Profiles::ValidateSavedCatalogs(container, *caps); });
    Must(id, "the saved native seed rules are refused", [&] {
      return Profiles::NativeSeedRulesFromContainer(container, *caps);
    });
    Require(Profiles::ContainerEntranceRuntimeFlags(container, *caps) ==
                kEntranceRuntimeErAny,
            id, "the saved seed does not turn on the ER runtime flag");
  }

  // The launcher's read of Save/file3.combosav and SaveManager::LoadFile's
  // preflight: the OoT page loads as stored, with the ER runtime flag, and
  // the container is cached without a backup, migration or rewrite.
  fs::create_directories(work / "Save");
  fs::current_path(work);
  const auto path = ComboContainerPath(slot);
  WriteBytes(path, bytes);
  g_containerCache.clear();
  g_evictedSlots.clear();
  auto page = Json::parse(Combo_ReadGameSave(ComboRando::GAME_OOT, slot));
  Require(!page.contains(ComboRando::kComboSeedPreflightError) &&
              !page.contains(ComboRando::kKokiriNativePreflightError),
          id, "the launcher read refuses it: " + page.dump().substr(0, 200));
  Must(id, "the native preflight refuses it", [&] {
    ComboRando::CheckComboSeedPreflight(page);
    ComboRando::ReadKokiriSaveRevision(page);
  });
  Require(ComboRando::TakeComboEntranceRuntimeFlags(page) ==
                  kEntranceRuntimeErAny &&
              page == container.at("oot"),
          id, "the OoT page does not load as stored with the ER runtime flag");
  Require(g_evictedSlots.empty() && g_containerCache.contains(slot) &&
              g_containerCache.at(slot) == container &&
              ReadBytes(path) == bytes,
          id, "the launcher changed the container while loading it");
  Require(Json::parse(Combo_ReadGameSave(ComboRando::GAME_MM, slot)) ==
              container.at("mm"),
          id, "the MM page does not load as stored");
  Require(Must(id, "the saved-slot reader refuses it",
               [&] { return Profiles::ReadSavedSeedFile(path, active); }) ==
              seed,
          id, "the saved-slot reader returns another seed");
  // The bind restores the saved layout into both games.
  sOot.clear();
  sMm.clear();
  Require(InstallEntranceSeed(Profiles::SeedFromContainer(container, active)) &&
              sOot.json() == seed.at("entranceLayout") &&
              sMm.json() == seed.at("entranceLayout"),
          id, "the bind does not restore the saved layout");

  // A saved seed changed in a way this build must refuse: the saved-slot
  // reader refuses it, and so does the launcher read, uncached and cached,
  // before the slot is cached and without changing the container.
  using Change = std::function<void(Json &)>;
  const std::vector<std::tuple<std::string, Change, bool>> changes = {
      {"a dev build's layout",
       [](Json &s) { s["entranceLayout"]["devBuild"] = true; },
       !active.devBuild},
      {"a Master Quest dungeon",
       [](Json &s) { s["entranceLayout"]["oot"]["mq"]["Forest"] = true; },
       true},
      {"a private MM profile beside the layout",
       [](Json &s) { s["entranceExpansion"] = Json::object(); }, true},
      {"the file type of seeds without a layout",
       [](Json &s) { s["fileType"] = ComboRando::kComboSeedFileType; }, true},
      {"another seed version", [](Json &s) { s["version"] = 1; }, true},
      {"the Hidden general Wallmaster contract",
       [](Json &s) {
         s["entranceLayout"]["requires"] =
             Json::array({"wallmasters-own-game-v1"});
       },
       true},
      {"a route outside its pool",
       [](Json &s) {
         auto &routes = s["entranceLayout"]["overrides"];
         routes[0]["destination"] = routes.back()["source"];
       },
       true},
  };
  for (const auto &[what, change, refused] : changes) {
    auto changed = container;
    change(changed["combo"]["rando"]);
    if (!refused) {
      Require(Profiles::SeedFromContainer(changed, active) ==
                  changed["combo"]["rando"],
              id, "this build refuses " + what);
      continue;
    }
    Require(
        !Refusal([&] { Profiles::SeedFromContainer(changed, active); }).empty(),
        id, "the saved-slot reader accepts " + what);
    const auto changedBytes = changed.dump();
    for (const bool cached : {false, true}) {
      WriteBytes(ComboContainerPath(0), changedBytes);
      g_containerCache.clear();
      if (cached)
        g_containerCache[0] = changed;
      const auto before = g_containerCache;
      auto refusedPage =
          Json::parse(Combo_ReadGameSave(ComboRando::GAME_OOT, 0));
      Require(refusedPage.contains(ComboRando::kComboSeedPreflightError) &&
                  !refusedPage.contains(ComboRando::kComboEntranceRuntimeFlags),
              id, "the launcher read accepts " + what);
      Require(!Refusal([&] {
                 ComboRando::CheckComboSeedPreflight(refusedPage);
               }).empty(),
              id, "the native preflight accepts " + what);
      Require(g_containerCache == before &&
                  ReadBytes(ComboContainerPath(0)) == changedBytes,
              id, "refusing " + what + " changed the slot");
    }
  }
  fs::remove(ComboContainerPath(0));
  // A build with no capability (0.4.3, 0.4.4) refuses it before decoding.
  Require(Refusal([&] {
            Profiles::SeedFromContainer(container, CapabilitySet{});
          }) == UnsupportedComboSeedFormat().what(),
          id, "a build with no capability does not refuse it before decoding");
}

int main(int argc, char **argv) {
  assert(argc == 3);
  const auto manifestPath = fs::absolute(argv[1]);
  const auto work = fs::absolute(argv[2]);
  const auto root = manifestPath.parent_path();
  const auto manifest = Json::parse(ReadBytes(manifestPath));
  const auto active = ActiveCapabilities();
#ifdef COMBO_ENTRANCE_DEV
  assert(active.devBuild);
#else
  assert(!active.devBuild && active == CapabilitySet::AtLeast(Stage::Public));
#endif
  std::map<std::string, Json> seeds;
  const Json *bound = nullptr;
  for (const auto &row : manifest.at("seeds")) {
    auto seed = CheckSeed(root, row, active, bound);
    bound = &seeds.emplace(row.at("id").get<std::string>(), std::move(seed))
                 .first->second;
  }
  for (const auto &row : manifest.at("saves"))
    CheckSave(root, work, row,
              seeds.at(row.at("corpusSeed").get<std::string>()), active);
  std::cout << "PASS " << seeds.size() << " release corpus seeds and "
            << manifest.at("saves").size()
            << " File 3 saves: exact bytes, seed guard, frozen v1 rules, "
               "install, replay and import in both games, entrance spoiler, "
               "profile view, launcher read, native preflight, layout "
               "restore and saved-v2 refusals ("
            << (active.devBuild ? "dev" : "shipping") << " build)\n";
}
