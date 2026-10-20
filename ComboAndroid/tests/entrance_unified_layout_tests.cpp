// The unified entrance layout (combo/rando/entrances/EntranceLayout.h): the
// frozen v1 pool rules against the live ResolvePools (Shuffle.h) and the pinned
// M1 contract (tools/ootmm/m1-contract.json), the strict codec, every
// rejection path of Validate, and the seed guard of this build. Built twice: as
// shipped and with COMBO_ENTRANCE_DEV.
#include "rando/ComboSeedFormat.h"
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace ComboRando;
using namespace ComboRando::Entrances;
using Json = nlohmann::json;
using Strings = std::set<std::string>;

namespace {
Strings ToStrings(const Names &names) {
  Strings result;
  for (const auto name : names)
    result.emplace(name);
  return result;
}

Settings FromFixture(const Json &j) {
  const auto mode = [](const Json &value) {
    assert(value == "none" || value == "ownGame");
    return value == "none" ? Mode::None : Mode::OwnGame;
  };
  Settings s;
  s.erWallmasters = mode(j.at("erWallmasters"));
  s.erOneWays = mode(j.at("erOneWays"));
  s.erOneWaysMajor = j.at("erOneWaysMajor").get<bool>();
  s.erOneWaysIkana = j.at("erOneWaysIkana").get<bool>();
  s.erOneWaysOwls = j.at("erOneWaysOwls").get<bool>();
  s.erOneWaysWaterVoids = j.at("erOneWaysWaterVoids").get<bool>();
  s.erOneWaysAnywhere = j.at("erOneWaysAnywhere").get<bool>();
  assert(j.size() == 7);
  return s;
}

// The quests of the twelve MQ dungeons from the names in `mq`.
OotQuests Quests(const Strings &mq) {
  OotQuests quests{};
  for (const auto &dungeon : mq) {
    assert(OotMqIndex(dungeon) < quests.size());
    quests[OotMqIndex(dungeon)] = true;
  }
  return quests;
}

// computeAllEntrances for the live registry, from the fixture's presence table
// (quest-only areas, MM JP layouts off).
AreaExists LiveAreas(const Json &presence, const OotQuests &quests) {
  Strings absent;
  for (const auto &area : presence.at("never"))
    absent.insert(area.get<std::string>());
  for (const auto &area : presence.at("byMmJpLayout").at("jp"))
    absent.insert(area.get<std::string>());
  for (const auto &quest : presence.at("byQuest").items())
    for (const auto &area :
         quest.value()[quests[OotMqIndex(quest.key())] ? "vanilla" : "mq"])
      absent.insert(area.get<std::string>());
  return [absent](std::string_view area) {
    return !absent.contains(std::string(area));
  };
}

void SamePools(const Pools &a, const Pools &b) {
  assert(a.size() == b.size());
  for (size_t i = 0; i < a.size(); ++i)
    assert(a[i].name == b[i].name && a[i].ownGame == b[i].ownGame &&
           a[i].alias == b[i].alias && a[i].src == b[i].src &&
           a[i].dst == b[i].dst);
}

// The one catalog row a combination needs.
std::string Requirement(const Settings &s) {
  if (s.erWallmasters != Mode::None && s.erOneWays != Mode::None)
    return "wallmasters-with-one-ways-v1";
  return s.erWallmasters != Mode::None ? "wallmasters-own-game-v1"
                                       : "one-ways-own-game-v1";
}

CapabilitySet WholeCatalog(bool devBuild = true) {
  auto caps = CapabilitySet::AtLeast(Stage::Hidden);
  caps.devBuild = devBuild;
  return caps;
}

template <class F> std::string LayoutError(F action) {
  try {
    action();
  } catch (const EntranceLayoutError &error) {
    return error.what();
  }
  assert(!"an entrance layout error was expected");
  return {};
}
std::string Rejected(const Json &value,
                     const CapabilitySet &caps = WholeCatalog()) {
  return LayoutError([&] { Validate(DecodeEntranceLayout(value), caps); });
}

// The frozen rows: exactly every live row of the v1 types, their reverses and
// the game links, field for field, and the fixture's rows.
void CheckFrozenRows(const Json &rows) {
  const Types v1{Type::Wallmaster,   Type::OneWay,          Type::OneWayIkana,
                 Type::OneWayOwl,    Type::OneWayWaterVoid, Type::Dungeon,
                 Type::DungeonMinor, Type::DungeonSh};
  Strings expected{"OOT_SHOP_MASKS", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER",
                   "MM_CLOCK_TOWER_FROM_CLOCK_TOWN",
                   "OOT_MARKET_FROM_MASK_SHOP"};
  for (const auto &e : kRegistry)
    if (v1.contains(e.type)) {
      expected.emplace(e.name);
      if (!e.reverse.empty())
        expected.emplace(e.reverse);
    }
  Strings frozen;
  for (const auto &row : LayoutV1::kRows) {
    frozen.emplace(row.name);
    const auto *live = Find(row.name);
    assert(live && live->game == row.game && live->id == row.id &&
           live->type == row.type && live->reverse == row.reverse &&
           live->from == row.from && live->to == row.to &&
           live->fromMap == row.fromMap && live->toMap == row.toMap &&
           live->fromSubmap == row.fromSubmap &&
           live->toSubmap == row.toSubmap &&
           live->noSongOfTime == row.noSongOfTime &&
           live->sourceLine == row.sourceLine);
    assert(Find(row.game, row.id) == live);
    if (rows.contains(std::string(row.name))) {
      const auto &f = rows.at(std::string(row.name));
      assert(f.at("id").get<uint32_t>() == row.id &&
             f.at("from") == std::string(row.from) &&
             f.at("to") == std::string(row.to) &&
             f.at("noSot").get<bool>() == row.noSongOfTime);
    }
  }
  assert(frozen == expected && frozen.size() == 67 && rows.size() == 63);
  for (const auto &row : rows.items())
    assert(frozen.contains(row.key()));
  assert(LayoutV1::kUpstreamCommit == kUpstreamCommit &&
         LayoutV1::kRegistrySha256 == kRegistrySourceSha256);
}

// The quest-only areas are the fixture's, and no v1 row depends on JP layouts.
void CheckAreaTables(const Json &presence) {
  std::set<std::tuple<std::string, bool, std::string>> fixture, frozen;
  for (const auto &quest : presence.at("byQuest").items())
    for (const char *kind : {"vanilla", "mq"})
      for (const auto &area : quest.value().at(kind))
        fixture.emplace(quest.key(), std::string(kind) == "mq",
                        area.get<std::string>());
  for (const auto &quest : LayoutV1::kQuestOnlyAreas) {
    assert(OotMqIndex(quest.dungeon) < kOotMqDungeons.size());
    frozen.emplace(quest.dungeon, quest.mq, quest.area);
  }
  assert(fixture == frozen && presence.at("never").empty());
  for (const auto &area : presence.at("byMmJpLayout").at("jp"))
    for (const auto &row : LayoutV1::kRows)
      assert(row.from != area.get<std::string>() &&
             row.to != area.get<std::string>());
}

// Frozen pools, live ResolvePools and the fixture agree, with candidates.
void CheckCombination(const Json &combination, const Json &presence) {
  const auto s = FromFixture(combination.at("settings"));
  const OotQuests vanilla{};
  const auto frozen = LayoutV1::MakePools(s, vanilla, false);
  const auto live = ResolvePools(s, LiveAreas(presence, vanilla));
  SamePools(frozen, live.pools);
  assert(live.fixed.size() == 2);
  const auto &expected = combination.at("pools");
  assert(frozen.size() == expected.size());
  for (size_t i = 0; i < frozen.size(); ++i) {
    const auto &pool = frozen[i];
    const auto &e = expected[i];
    assert(e.at("name") == pool.name && e.at("ownGame") == pool.ownGame &&
           e.at("alias") == pool.alias &&
           e.at("dst").get<Strings>() == ToStrings(pool.dst));
    std::map<std::string, Strings> candidates;
    for (const auto &group : e.at("candidates"))
      for (const auto &src : group.at("sources"))
        candidates[src.get<std::string>()] =
            group.at("destinations").get<Strings>();
    assert(candidates.size() == pool.src.size());
    for (const auto src : pool.src) {
      Strings frozenCandidates, liveCandidates;
      for (const auto dst : pool.dst) {
        if (LayoutV1::RowAssignable(*LayoutV1::FindRow(src),
                                    *LayoutV1::FindRow(dst), pool.ownGame))
          frozenCandidates.emplace(dst);
        if (Assignable(*Find(src), *Find(dst), pool.ownGame, s))
          liveCandidates.emplace(dst);
      }
      assert(frozenCandidates == candidates.at(std::string(src)) &&
             liveCandidates == frozenCandidates);
    }
  }
}

EntranceLayout SampleLayout(const Json &combination) {
  EntranceLayout layout;
  layout.settings = FromFixture(combination.at("settings"));
  layout.required = {Requirement(layout.settings)};
  Overrides overrides;
  for (const auto &row : combination.at("sampleLayout"))
    overrides[LayoutV1::FindRow(row[0].get<std::string>())->name] =
        LayoutV1::FindRow(row[1].get<std::string>())->name;
  layout.overrides = std::move(overrides);
  return layout;
}

bool SameLayout(const EntranceLayout &a, const EntranceLayout &b) {
  return a.upstreamCommit == b.upstreamCommit &&
         a.registrySha256 == b.registrySha256 && a.required == b.required &&
         a.devBuild == b.devBuild &&
         SameEntranceSettings(a.settings, b.settings) && a.ootMq == b.ootMq &&
         a.overrides == b.overrides;
}

// Every sample layout validates, routes every source once and round-trips byte
// for byte; its routeless settings document validates too.
void CheckSample(const Json &combination) {
  auto layout = SampleLayout(combination);
  if (combination.at("pools").empty()) {
    assert(LayoutError([&] {
             ValidateEntranceLayoutSettings(layout, WholeCatalog());
           }).find("place no entrance") != std::string::npos);
    return;
  }
  const auto routes = Validate(layout, WholeCatalog());
  assert(routes.oot.size() + routes.mm.size() ==
         combination.at("sampleLayout").size());
  for (const auto &route : routes.oot)
    assert(route.source->game == Game::Oot);
  for (const auto &route : routes.mm)
    assert(route.source->game == Game::Mm);
  const auto bytes = EncodeEntranceLayout(layout).dump();
  const auto decoded = DecodeEntranceLayout(Json::parse(bytes));
  assert(SameLayout(decoded, layout) &&
         EncodeEntranceLayout(decoded).dump() == bytes);
  auto document = Json::parse(bytes);
  document.erase("overrides");
  const auto settingsOnly =
      DecodeEntranceLayout(document, LayoutPart::SettingsOnly);
  assert(!settingsOnly.overrides &&
         EncodeEntranceLayout(settingsOnly) == document);
  ValidateEntranceLayoutSettings(settingsOnly, WholeCatalog());
  assert(LayoutError([&] {
           Validate(settingsOnly, WholeCatalog());
         }).find("no routes") != std::string::npos);
}

// The OoT Wallmaster sources of every vanilla/MQ context, frozen and live,
// against the fixture's table; the other six quests change nothing.
void CheckMqContexts(const Json &fixture) {
  const auto &presence = fixture.at("registryAreaPresence");
  const auto &dungeons = fixture.at("mqSources").at("dungeons");
  const auto &table = fixture.at("mqSources").at("table");
  assert(table.size() == 64);
  Settings wallmasters;
  wallmasters.erWallmasters = Mode::OwnGame;
  for (const auto &context : table) {
    for (bool others : {false, true}) {
      auto mq = context.at("mq").get<Strings>();
      if (others)
        for (const char *dungeon : {"DT", "DC", "JJ", "Fire", "Water", "IC"})
          mq.insert(dungeon);
      const auto quests = Quests(mq);
      const auto frozen = LayoutV1::MakePools(wallmasters, quests, false);
      SamePools(frozen,
                ResolvePools(wallmasters, LiveAreas(presence, quests)).pools);
      Strings oot;
      for (const auto src : frozen.at(0).src)
        if (LayoutV1::FindRow(src)->game == Game::Oot)
          oot.emplace(src);
      assert(oot == context.at("ootSources").get<Strings>());
      // The MM tranche reports every OoT Wallmaster area absent.
      const auto tranche = LayoutV1::MakePools(wallmasters, quests, true);
      for (const auto src : tranche.at(0).src)
        assert(LayoutV1::FindRow(src)->game == Game::Mm);
      assert(tranche.at(0).src.size() == 4);
    }
  }
  assert(dungeons.size() == 6);
  // Only the OoT Wallmaster sources depend on the quests (the exporter checks
  // that over every combination), so the frozen pools of the Wallmaster
  // combinations with every non-Anywhere flag are compared with ResolvePools
  // in every context.
  for (const auto &combination : fixture.at("combinations")) {
    const auto s = FromFixture(combination.at("settings"));
    if (s.erWallmasters == Mode::None ||
        !(s.erOneWaysMajor && s.erOneWaysIkana && s.erOneWaysOwls &&
          s.erOneWaysWaterVoids))
      continue;
    for (const auto &context : table) {
      const auto quests = Quests(context.at("mq").get<Strings>());
      SamePools(LayoutV1::MakePools(s, quests, false),
                ResolvePools(s, LiveAreas(presence, quests)).pools);
    }
  }
}

Names Sources(const Settings &s, const Strings &mq) {
  Names result;
  for (const auto &pool : LayoutV1::MakePools(s, Quests(mq), false))
    result.insert(pool.src.begin(), pool.src.end());
  return result;
}

void CheckMqCompleteness() {
  Settings s;
  s.erWallmasters = Mode::OwnGame;
  const auto vanilla = Sources(s, {});
  assert(vanilla.contains("OOT_WALLMASTER_SHADOW") &&
         !Sources(s, {"Shadow"}).contains("OOT_WALLMASTER_SHADOW"));
  assert(!vanilla.contains("OOT_WALLMASTER_BOTW_BASEMENT") &&
         !vanilla.contains("OOT_WALLMASTER_BOTW_PIT"));
  const auto botw = Sources(s, {"BotW"});
  assert(botw.contains("OOT_WALLMASTER_BOTW_BASEMENT") &&
         botw.contains("OOT_WALLMASTER_BOTW_PIT") &&
         botw.contains("OOT_WALLMASTER_BOTW_MAIN"));
  assert(vanilla.contains("OOT_WALLMASTER_GANON_LIGHT") &&
         !Sources(s, {"Ganon"}).contains("OOT_WALLMASTER_GANON_LIGHT") &&
         Sources(s, {"Ganon"}).contains("OOT_WALLMASTER_GANON_SPIRIT"));
  const auto spirit = Sources(s, {"Spirit"});
  assert(vanilla.contains("OOT_WALLMASTER_SPIRIT_CHILD_RUPEES") &&
         !spirit.contains("OOT_WALLMASTER_SPIRIT_CHILD_RUPEES") &&
         spirit.contains("OOT_WALLMASTER_SPIRIT_CHILD_SUN") &&
         spirit.contains("OOT_WALLMASTER_SPIRIT_STATUE") &&
         !vanilla.contains("OOT_WALLMASTER_SPIRIT_STATUE"));
  // MQ Spirit + MQ BotW with vanilla Shadow and Ganon: 12 OoT sources for the
  // 11 OoT destinations. The layout cannot exist; the validator is not a
  // capacity check (generation fails, M1-17).
  size_t oot = 0;
  for (const auto src : Sources(s, {"Spirit", "BotW"}))
    oot += LayoutV1::FindRow(src)->game == Game::Oot;
  assert(oot == 12 && Sources(s, {}).size() == 13);
}

// A valid Wallmasters-alone document: the four MM sources to four MM temples
// and the nine vanilla OoT sources to nine OoT dungeons.
Json WallmasterDocument(const Json &fixture) {
  for (const auto &combination : fixture.at("combinations"))
    if (combination.at("id") == "erWallmasters=ownGame;erOneWays=none")
      return EncodeEntranceLayout(SampleLayout(combination));
  assert(!"fixture combination missing");
  return {};
}

Json::json_pointer P(const std::string &path) {
  return Json::json_pointer(path);
}
Json With(Json value, const Json::json_pointer &path, const Json &replacement) {
  value[path] = replacement;
  return value;
}
Json Without(Json value, const Json::json_pointer &path) {
  value[path.parent_pointer()].erase(path.back());
  return value;
}
Json WithRoute(Json value, const std::string &src, const std::string &dst) {
  auto &rows = value.at("overrides");
  for (auto &row : rows)
    if (row.at("source") == src) {
      row["destination"] = dst;
      return value;
    }
  rows.push_back({{"source", src}, {"destination", dst}});
  std::sort(rows.begin(), rows.end(), [](const Json &a, const Json &b) {
    return a.at("source").get<std::string>() <
           b.at("source").get<std::string>();
  });
  return value;
}
Json WithoutRoute(Json value, const std::string &src) {
  auto &rows = value.at("overrides");
  for (auto it = rows.begin(); it != rows.end(); ++it)
    if (it->at("source") == src) {
      rows.erase(it);
      return value;
    }
  assert(!"route missing");
  return value;
}

void CheckDecodeRejections(const Json &good) {
  Validate(DecodeEntranceLayout(good), WholeCatalog());
  for (const Json &value : {Json(), Json::array(), Json(1), Json("layout")})
    Rejected(value);
  Rejected(With(good, P("/extra"), 1));
  for (const char *key :
       {"schema", "upstreamCommit", "registrySha256", "requires", "devBuild",
        "settings", "oot", "overrides"})
    Rejected(Without(good, P(std::string("/") + key)));
  Rejected(With(good, P("/schema"), "ootmm-entrance-layout-v2"));
  Rejected(With(good, P("/schema"), 1));
  Rejected(With(good, P("/upstreamCommit"), 413573));
  Rejected(With(good, P("/registrySha256"), Json()));
  Rejected(With(good, P("/requires"), "wallmasters-own-game-v1"));
  Rejected(With(good, P("/requires"), Json::array({1})));
  Rejected(With(good, P("/devBuild"), 0));
  Rejected(With(good, P("/settings"), Json::array()));
  Rejected(With(good, P("/settings/erShuffleEverything"), false));
  Rejected(Without(good, P("/settings/erBoss")));
  Rejected(With(good, P("/settings/erSelfLoops"), "false"));
  Rejected(With(good, P("/settings/erWallmasters"), true));
  Rejected(With(good, P("/settings/erWallmasters"), "anywhere"));
  Rejected(With(good, P("/settings/erWallmasters"), 1));
  Rejected(With(good, P("/oot"), Json::array()));
  Rejected(With(good, P("/oot/extra"), Json::object()));
  Rejected(With(good, P("/oot/mq"), Json::array()));
  Rejected(Without(good, P("/oot/mq/Ganon")));
  Rejected(With(good, P("/oot/mq/Ganondorf"), false));
  Rejected(
      With(Without(good, P("/oot/mq/Ganon")), P("/oot/mq/Ganondorf"), false));
  Rejected(With(good, P("/oot/mq/Forest"), 1));
  Rejected(With(good, P("/overrides"), Json::object()));
  Rejected(With(good, P("/overrides/0"), Json::array()));
  Rejected(With(good, P("/overrides/0/extra"), 1));
  Rejected(Without(good, P("/overrides/0/destination")));
  Rejected(With(good, P("/overrides/0/source"), 7));
  Rejected(With(good, P("/overrides/0/destination"), "MM_NOWHERE"));
  // A real registry row outside the v1 rows.
  assert(Find("OOT_BOSS_DEKU_TREE"));
  Rejected(With(good, P("/overrides/0/destination"), "OOT_BOSS_DEKU_TREE"));
  auto unsorted = good;
  std::swap(unsorted.at("overrides")[0], unsorted.at("overrides")[1]);
  Rejected(unsorted);
  auto repeated = good;
  repeated.at("overrides")
      .insert(repeated.at("overrides").begin() + 1,
              repeated.at("overrides")[0]);
  Rejected(repeated);
  // A settings document is exactly the envelope without routes.
  assert(LayoutError([&] {
           DecodeEntranceLayout(good, LayoutPart::SettingsOnly);
         }).find("unknown key overrides") != std::string::npos);
  assert(LayoutError([&] {
           DecodeEntranceLayout(Without(good, P("/overrides")));
         }).find("missing overrides") != std::string::npos);
}

void CheckValidateRejections(const Json &good) {
  const auto requires_ = P("/requires");
  Rejected(With(good, P("/upstreamCommit"),
                "41a2b7c0d7dbbb53fed928652f8271bc336c113f"));
  Rejected(With(good, P("/registrySha256"), std::string(64, '0')));
  Rejected(With(good, requires_, Json::array()));
  Rejected(With(good, requires_, {"wallmasters-own-game-v2"}));
  Rejected(With(good, requires_,
                {"wallmasters-own-game-v1", "wallmasters-own-game-v1"}));
  // More ids than the settings use, in either order.
  Rejected(With(good, requires_,
                {"wallmasters-own-game-v1", "wallmasters-with-one-ways-v1"}));
  Rejected(With(good, requires_,
                {"wallmasters-with-one-ways-v1", "wallmasters-own-game-v1"}));
  Rejected(
      With(good, requires_,
           {"wallmasters-own-game-mm-tranche", "wallmasters-own-game-v1"}));
  // Settings no required row admits, or not admitted in this build.
  Rejected(With(good, requires_, {"one-ways-own-game-v1"}));
  assert(Rejected(good, WholeCatalog().Only("one-ways-own-game-v1"))
             .find("does not have") != std::string::npos);
  assert(Rejected(good, CapabilitySet{}).find("does not have") !=
         std::string::npos);
  Rejected(With(good, P("/settings/erWallmasters"), "full"));
  Rejected(With(good, P("/settings/erBoss"), "ownGame"));
  // A dev-build layout only in a dev build.
  const auto dev = With(good, P("/devBuild"), true);
  Validate(DecodeEntranceLayout(dev), WholeCatalog());
  assert(Rejected(dev, WholeCatalog(false)).find("dev build") !=
         std::string::npos);
  Validate(DecodeEntranceLayout(good), WholeCatalog(false));
  // Not a fixed point of OoTMM's conditions: a one-way flag without one-ways.
  assert(Rejected(With(good, P("/settings/erOneWaysMajor"), true))
             .find("condition") != std::string::npos);
  // Settings that place nothing: one-ways with no flag.
  auto empty = With(Without(good, P("/overrides")),
                    P("/settings/erWallmasters"), "none");
  empty["settings"]["erOneWays"] = "ownGame";
  empty["requires"] = {"one-ways-own-game-v1"};
  empty["overrides"] = Json::array();
  assert(Rejected(empty).find("place no entrance") != std::string::npos);
  // Routes: one missing, a foreign one, a stored game link, a destination
  // outside the pool, a cross-game one, a dungeon exit (polarity), a reused
  // one.
  Rejected(WithoutRoute(good, "MM_WALLMASTER_DAMPE"));
  Rejected(WithoutRoute(good, "OOT_WALLMASTER_GTG"));
  Rejected(WithRoute(good, "OOT_WALLMASTER_BOTW_PIT", "OOT_ICE_CAVERN"));
  Rejected(WithRoute(good, "MM_VOID_GREAT_BAY", "MM_TEMPLE_WOODFALL"));
  assert(Rejected(WithRoute(good, "OOT_SHOP_MASKS",
                            "MM_CLOCK_TOWN_FROM_CLOCK_TOWER"))
             .find("game link") != std::string::npos);
  Rejected(WithRoute(good, "MM_WALLMASTER_DAMPE", "MM_VOID_GREAT_BAY"));
  Rejected(WithRoute(good, "MM_WALLMASTER_DAMPE", "OOT_TEMPLE_FOREST"));
  Rejected(WithRoute(good, "MM_WALLMASTER_DAMPE", "MM_WOODFALL_FROM_TEMPLE"));
  Rejected(WithRoute(good, "OOT_WALLMASTER_GTG",
                     "OOT_GERUDO_FORTRESS_FROM_GERUDO_TRAINING"));
  std::string dampe;
  for (const auto &row : good.at("overrides"))
    if (row.at("source") == "MM_WALLMASTER_DAMPE")
      dampe = row.at("destination").get<std::string>();
  assert(Rejected(WithRoute(good, "MM_WALLMASTER_BTW_EXIT", dampe))
             .find("twice") != std::string::npos);
  // A layout built in code (as generation builds one) may name registry rows
  // outside v1; they are refused, not looked up.
  for (const bool asSource : {false, true}) {
    auto built = DecodeEntranceLayout(good);
    const auto boss = Find("OOT_BOSS_DEKU_TREE")->name;
    if (asSource)
      (*built.overrides)[boss] = "OOT_DEKU_TREE";
    else
      (*built.overrides)["OOT_WALLMASTER_GTG"] = boss;
    assert(LayoutError([&] {
             Validate(built, WholeCatalog());
           }).find(asSource ? "which no pool shuffles" : "cannot route") !=
           std::string::npos);
  }
  // The routes come from the recorded quests: vanilla routes in an MQ Shadow
  // world name a source that does not exist there.
  Rejected(With(good, P("/oot/mq/Shadow"), true));
  auto mqBotw = With(good, P("/oot/mq/BotW"), true);
  Rejected(mqBotw); // Basement and Pit unrouted
  // The two OoT dungeons the sample leaves free.
  std::vector<std::string> unused;
  const auto pools = LayoutV1::MakePools(DecodeEntranceLayout(good).settings,
                                         OotQuests{}, false);
  for (const auto dst : pools.at(0).dst) {
    const auto &row = *LayoutV1::FindRow(dst);
    bool used = row.game != Game::Oot || row.type == Type::DungeonExit;
    for (const auto &route : good.at("overrides"))
      used = used || route.at("destination") == std::string(dst);
    if (!used)
      unused.emplace_back(dst);
  }
  assert(unused.size() == 2);
  assert(Rejected(WithRoute(WithRoute(mqBotw, "OOT_WALLMASTER_BOTW_BASEMENT",
                                      unused[0]),
                            "OOT_WALLMASTER_BOTW_PIT", unused[0]))
             .find("twice") != std::string::npos);
  mqBotw =
      WithRoute(WithRoute(mqBotw, "OOT_WALLMASTER_BOTW_BASEMENT", unused[0]),
                "OOT_WALLMASTER_BOTW_PIT", unused[1]);
  Validate(DecodeEntranceLayout(mqBotw), WholeCatalog());
  // Quests of the six dungeons without Wallmasters change nothing.
  auto mqOthers = good;
  for (const char *dungeon : {"DT", "DC", "JJ", "Fire", "Water", "IC"})
    mqOthers["oot"]["mq"][dungeon] = true;
  Validate(DecodeEntranceLayout(mqOthers), WholeCatalog());
}

// The dev-only MM tranche: only the MM rows, and only by its own id.
void CheckTranche(const Json &good) {
  auto tranche = good;
  tranche["requires"] = {"wallmasters-own-game-mm-tranche"};
  assert(Rejected(tranche).find("which no pool shuffles") != std::string::npos);
  for (const auto &row : good.at("overrides"))
    if (row.at("source").get<std::string>().starts_with("OOT_"))
      tranche = WithoutRoute(tranche, row.at("source").get<std::string>());
  const auto routes = Validate(DecodeEntranceLayout(tranche), WholeCatalog());
  assert(routes.oot.empty() && routes.mm.size() == 4);
  assert(Rejected(tranche, WholeCatalog().WithoutDevOnly())
             .find("does not have") != std::string::npos);
}

// Ownership, polarity and self-loops of the one-way pools.
const Json &Combination(const Json &fixture, const std::string &oneWays) {
  for (const auto &combination : fixture.at("combinations"))
    if (combination.at("id") ==
        "erWallmasters=none;erOneWays=ownGame" + oneWays)
      return combination;
  assert(!"fixture combination missing");
  return fixture;
}
void CheckOneWays(const Json &fixture) {
  // Own game: an OoT one-way never leads into MM, in the permutation or with
  // Anywhere.
  for (const char *flags :
       {";erOneWaysMajor", ";erOneWaysMajor;erOneWaysAnywhere"}) {
    const auto good =
        EncodeEntranceLayout(SampleLayout(Combination(fixture, flags)));
    Validate(DecodeEntranceLayout(good), WholeCatalog());
    Rejected(WithRoute(good, "OOT_LAKE_HYLIA_FROM_VALLEY",
                       "MM_SWAMP_FROM_IKANA_CANYON"));
  }
  // An owl that lands on the map it leaves is a self-loop
  // (entrances.ts:459-482).
  const auto owls = EncodeEntranceLayout(
      SampleLayout(Combination(fixture, ";erOneWaysOwls;erOneWaysAnywhere")));
  Rejected(WithRoute(owls, "OOT_VILLAGE_OWL",
                     "OOT_MOUNTAIN_TRAIL_FROM_DODONGO_CAVERN"));
  Rejected(
      WithRoute(owls, "OOT_FIELD_OWL", "OOT_LAKE_HYLIA_FROM_TEMPLE_WATER"));
  Validate(DecodeEntranceLayout(WithRoute(owls, "OOT_VILLAGE_OWL",
                                          "OOT_KOKIRI_FOREST_FROM_DEKU_TREE")),
           WholeCatalog());
  // Without Anywhere the one-ways are a permutation: a route back to itself
  // is fine, a dungeon is not in the pool.
  const auto major = EncodeEntranceLayout(
      SampleLayout(Combination(fixture, ";erOneWaysMajor")));
  Rejected(WithRoute(major, "OOT_LAKE_HYLIA_FROM_VALLEY", "OOT_DEKU_TREE"));
}

// The song travel map of a seed with "Play warp songs across games" in logic.
Json ForeignSongs() {
  SongTravelMap map;
  map.foreignSongs = map.logicForeignSongs = true;
  return map.json();
}
// Refused by the seed guard with `reason`.
void SeedRefused(const Json &seed, const CapabilitySet &caps,
                 const std::string &reason) {
  std::string message;
  try {
    RequireSupportedSeedDocument(seed, caps);
  } catch (const UnsupportedComboSeedFormat &error) {
    message = error.what();
  }
  assert(message.find(reason) != std::string::npos);
}

// This build. A release build admits only the two Own Game Wallmaster
// contracts, vanilla (P1-05) and Master Quest (U11): it validates their layouts
// and refuses every other layout after decoding it. A dev build adds the MM
// tranche and the staged foreign-song contract (limits design, Stage 1), and
// accepts dev layouts. In both, an id must carry the seed's song facts.
void CheckBuildGuard(const Json &good) {
  const auto caps = ActiveCapabilities();
  Json seed = {{"fileType", kEntranceSeedFileType},
               {"version", kEntranceSeedVersion},
               {"entranceLayout", good}};
  RequireSupportedSeedDocument(seed, WholeCatalog());
  const std::string before = UnsupportedComboSeedFormat().what();
#ifdef COMBO_ENTRANCE_DEV
  assert(caps.devBuild && caps.Active(kMmTrancheCapability) &&
         caps.WithoutDevOnly() == (caps.Only(kVanillaWallmasterCapability) |
                                   caps.Only(kQuestsWallmasterCapability) |
                                   caps.Only(kSongsWallmasterCapability)));
  auto vanilla = good;
  vanilla["requires"] = {std::string(kVanillaWallmasterCapability)};
  vanilla["devBuild"] = true;
  seed["entranceLayout"] = vanilla;
  RequireSupportedSeedDocument(seed, caps);
  // A Master Quest dungeon takes the quests id, which a dev build has.
  auto quests = vanilla;
  quests["requires"] = {std::string(kQuestsWallmasterCapability)};
  quests["oot"]["mq"]["DT"] = true;
  seed["entranceLayout"] = quests;
  RequireSupportedSeedDocument(seed, caps);
  // Foreign-song logic takes the songs id, in either quest context, which a
  // dev build has; neither other id takes it, and the songs id needs it.
  for (const auto *base : {&vanilla, &quests}) {
    auto songs = *base;
    songs["requires"] = {std::string(kSongsWallmasterCapability)};
    seed["entranceLayout"] = songs;
    seed["songTravel"] = ForeignSongs();
    RequireSupportedSeedDocument(seed, caps);
    seed["entranceLayout"] = *base;
    SeedRefused(seed, caps, "which does not admit foreign-song logic");
    seed["entranceLayout"] = songs;
    seed.erase("songTravel");
    SeedRefused(seed, caps, "without foreign-song logic");
  }
  auto tranche = good;
  tranche["requires"] = {std::string(kMmTrancheCapability)};
  tranche["devBuild"] = true;
  for (const auto &row : good.at("overrides"))
    if (row.at("source").get<std::string>().starts_with("OOT_"))
      tranche = WithoutRoute(tranche, row.at("source").get<std::string>());
  seed["entranceLayout"] = tranche;
  RequireSupportedSeedDocument(seed, caps);
  RequireSupportedSettingsCapabilities(seed, caps);
  for (const auto &layout : {good, Json(), Json::object()}) {
    seed["entranceLayout"] = layout;
    std::string message;
    try {
      RequireSupportedSeedCapabilities(seed, caps);
    } catch (const UnsupportedComboSeedFormat &error) {
      message = error.what();
    }
    assert(!message.empty() && message != before);
  }
#else
  assert(!caps.devBuild && caps == CapabilitySet::AtLeast(Stage::Public) &&
         caps == (caps.Only(kVanillaWallmasterCapability) |
                  caps.Only(kQuestsWallmasterCapability)) &&
         caps.Active(kVanillaWallmasterCapability));
  auto layout = DecodeEntranceLayout(good);
  layout.required = {std::string(kVanillaWallmasterCapability)};
  const auto vanilla = EncodeEntranceLayout(layout);
  seed["entranceLayout"] = vanilla;
  RequireSupportedSeedDocument(seed, caps);
  RequireSupportedSeedCapabilities(seed, caps);
  RequireSupportedSettingsCapabilities(seed, caps);
  // Everything else is decoded and refused: a dev seed, the hidden general and
  // One-Ways contracts, the MM tranche, a Master Quest dungeon, a malformed
  // layout and any layout beside a private MM profile.
  const auto refuse = [&](const Json &value, bool legacy,
                          const std::string &reason) {
    seed = {{"fileType", kEntranceSeedFileType},
            {"version", kEntranceSeedVersion},
            {"entranceLayout", value}};
    if (legacy)
      seed["entranceExpansion"] = Json::object();
    for (const auto &check : std::vector<std::function<void()>>{
             [&] { RequireSupportedSeedDocument(seed, caps); },
             [&] { RequireSupportedSeedCapabilities(seed, caps); },
             [&] { RequireSupportedSettingsCapabilities(seed, caps); }}) {
      std::string message;
      try {
        check();
      } catch (const UnsupportedComboSeedFormat &error) {
        message = error.what();
      }
      assert(!message.empty() && message != before &&
             message.find(reason) != std::string::npos);
    }
  };
  std::vector<std::pair<Json, std::string>> refused;
  for (const auto *id :
       {"wallmasters-own-game-v1", "one-ways-own-game-v1",
        "wallmasters-with-one-ways-v1", "wallmasters-own-game-mm-tranche",
        "wallmasters-own-game-songs-v1"}) {
    auto other = vanilla;
    other["requires"] = {id};
    refused.emplace_back(other, "which this build does not have");
  }
  // The staged foreign-song contract: a seed with that logic is refused under
  // the vanilla id, as under the songs id, which this build does not have yet;
  // with No Logic it has no song facts and loads.
  seed = {{"fileType", kEntranceSeedFileType},
          {"version", kEntranceSeedVersion},
          {"entranceLayout", vanilla},
          {"songTravel", ForeignSongs()}};
  SeedRefused(seed, caps, "which does not admit foreign-song logic");
  seed["accessibility"] = {{"version", 1}, {"mode", "noLogic"}};
  RequireSupportedSeedDocument(seed, caps);
  auto songs = layout;
  songs.required = {std::string(kSongsWallmasterCapability)};
  seed["entranceLayout"] = EncodeEntranceLayout(songs);
  seed.erase("accessibility");
  SeedRefused(seed, caps, "which this build does not have");
  seed.erase("songTravel");
  // The Master Quest contract (U11): a seed with a Master Quest dungeon loads;
  // the quests id with all-vanilla dungeons does not, they keep the vanilla id.
  auto quests = layout;
  quests.required = {std::string(kQuestsWallmasterCapability)};
  quests.ootMq[OotMqIndex("DT")] = true;
  seed["entranceLayout"] = EncodeEntranceLayout(quests);
  RequireSupportedSeedDocument(seed, caps);
  RequireSupportedSeedCapabilities(seed, caps);
  RequireSupportedSettingsCapabilities(seed, caps);
  quests.ootMq = {};
  refused.emplace_back(EncodeEntranceLayout(quests),
                       "without a Master Quest dungeon");
  auto dev = vanilla;
  dev["devBuild"] = true;
  refused.emplace_back(dev, "comes from a dev build");
  auto mq = layout;
  mq.ootMq[OotMqIndex("Spirit")] = true;
  refused.emplace_back(EncodeEntranceLayout(mq), "vanilla OoT dungeons only");
  refused.emplace_back(Json(), "");
  refused.emplace_back(Json::object(), "");
  for (const auto &[value, reason] : refused)
    refuse(value, false, reason);
  refused.emplace_back(vanilla, "");
  for (const auto &item : refused)
    refuse(item.first, true, "cannot be combined with entranceExpansion");
#endif
}

void Run(const char *path) {
  std::ifstream input(path, std::ios::binary);
  const auto fixture = Json::parse(input);
  assert(fixture.at("upstreamCommit") ==
         std::string(LayoutV1::kUpstreamCommit));
  CheckFrozenRows(fixture.at("rows"));
  CheckAreaTables(fixture.at("registryAreaPresence"));
  size_t placed = 0;
  for (const auto &combination : fixture.at("combinations")) {
    CheckCombination(combination, fixture.at("registryAreaPresence"));
    CheckSample(combination);
    placed += !combination.at("pools").empty();
  }
  assert(fixture.at("combinations").size() == 66 && placed == 63);
  CheckMqContexts(fixture);
  CheckMqCompleteness();
  const auto good = WallmasterDocument(fixture);
  CheckDecodeRejections(good);
  CheckValidateRejections(good);
  CheckTranche(good);
  CheckOneWays(fixture);
  CheckBuildGuard(good);
  std::cout << "PASS frozen v1 pools = ResolvePools = pinned contract (66 "
               "combinations, 64 MQ contexts), byte-stable codec, every "
               "rejection, this build's guard\n";
}
} // namespace

int main(int argc, char **argv) {
  assert(argc == 2);
  Run(argv[1]);
}
