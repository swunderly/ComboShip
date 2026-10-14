// The candidate world (combo/rando/entrances/CandidateWorld.h) against the
// pinned M1 contract (tools/ootmm/m1-contract.json, OoTMM 413573d
// packages/logic/src/solver/entrances.ts changedWorld, :289-385): the per-game
// split of every M1 combination, MQ context and committed sample layout, the
// reset gate and VOID marks, ORed duplicates, the order-independent token, the
// ASSUMED refusal, and the Temple probe's view of the same world.
#include "rando/CrossWorldRando.h"
#include "rando/entrances/TempleCandidate.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ComboRando::Entrances;
using Json = nlohmann::json;
using Strings = std::set<std::string>;
using Targets = std::map<std::string, Strings>; // source -> destinations

namespace {
Json fixture;

const Json &PinnedRow(std::string_view name) {
  return fixture.at("rows").at(std::string(name));
}
bool PinnedVoid(std::string_view name) {
  const auto to = PinnedRow(name).at("to").get<std::string>();
  return to == "OOT VOID" || to == "MM VOID";
}
Game GameOf(std::string_view name) { return Find(name)->game; }
std::string GameName(Game game) { return game == Game::Oot ? "oot" : "mm"; }

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

// computeAllEntrances for one vanilla/MQ choice (MM JP layouts off).
AreaExists Areas(const Strings &mq) {
  const auto &presence = fixture.at("registryAreaPresence");
  Strings absent;
  for (const auto &area : presence.at("never"))
    absent.insert(area.get<std::string>());
  for (const auto &area : presence.at("byMmJpLayout").at("jp"))
    absent.insert(area.get<std::string>());
  for (const auto &quest : presence.at("byQuest").items())
    for (const auto &area :
         quest.value().at(mq.contains(quest.key()) ? "vanilla" : "mq"))
      absent.insert(area.get<std::string>());
  return [absent](std::string_view area) {
    return !absent.contains(std::string(area));
  };
}

const Json &Combination(const std::string &id) {
  for (const auto &c : fixture.at("combinations"))
    if (c.at("id") == id)
      return c;
  throw std::invalid_argument("No fixture combination " + id);
}
const std::string kWallmasters = "erWallmasters=ownGame;erOneWays=none";
const std::string kWidest =
    "erWallmasters=ownGame;erOneWays=ownGame;erOneWaysMajor;erOneWaysIkana;"
    "erOneWaysOwls;erOneWaysWaterVoids";

// The pinned facts of one edge, from the fixture's rows rather than
// Registry.h: the reset gate is exactly "MM destination without no-sot", and
// the VOID mark exactly "destination leads into VOID".
void CheckEdge(const CandidateEdge &e) {
  const auto &dst = PinnedRow(e.dst);
  assert(e.resetTimeGate ==
         (dst.at("game") == "mm" && !dst.at("noSot").get<bool>()));
  assert(e.toVoid == PinnedVoid(e.dst));
}

Targets ByTarget(const std::vector<CandidateEdge> &edges) {
  Targets result;
  for (const auto &e : edges)
    result[std::string(e.src)].emplace(e.dst);
  return result;
}
Strings ToStrings(const std::vector<std::string_view> &names) {
  return Strings(names.begin(), names.end());
}

// The encoded overlay of one game is exactly that game's split.
void CheckOverlay(const CandidateWorld &w, Game game) {
  const auto o = SplitCandidateWorld(w, game);
  const auto json =
      Json::parse(game == Game::Oot ? EncodeOotOverlay(w) : EncodeMmOverlay(w));
  assert(json.size() == 8 &&
         json.at("profile") == "ootmm-entrance-assumptions-v1" &&
         json.at("game") == GameName(game) &&
         json.at("registry") == std::string(kRegistrySourceSha256) &&
         json.at("world").get<uint32_t>() == CandidateWorldToken(w) &&
         json.at("assumed").empty());
  assert(json.at("deleted").size() == o.deleted.size());
  for (size_t i = 0; i < o.deleted.size(); ++i) {
    const auto &row = json.at("deleted")[i];
    assert(row.size() == 2 && row.at("row") == std::string(o.deleted[i]) &&
           GameOf(o.deleted[i]) == game);
    assert(row.at("void").get<bool>() == IsVoidArea(Find(o.deleted[i])->to));
  }
  const auto same = [&](const Json &list,
                        const std::vector<CandidateEdge> &edges) {
    assert(list.size() == edges.size());
    for (size_t i = 0; i < edges.size(); ++i) {
      const auto &e = list[i];
      assert(e.size() == 4 && e.at("src") == std::string(edges[i].src) &&
             e.at("dst") == std::string(edges[i].dst) &&
             e.at("resetTime").get<bool>() == edges[i].resetTimeGate &&
             e.at("void").get<bool>() == edges[i].toVoid &&
             GameOf(edges[i].src) == game);
    }
  };
  same(json.at("fixed"), o.fixed);
  same(json.at("remaining"), o.remaining);
}

// The Temple probe reads the same world: its candidate is the world with the
// fixed and remaining edges merged, its query splits them as the overlays do
// (own-game M1 pools never cross games), and it names the world by the token
// both overlays carry.
void CheckTempleView(const CandidateWorld &w, const Overrides &overrides,
                     const Pools &pools, const Settings &s) {
  const auto c = BuildTempleCandidate(overrides, pools, s);
  auto edges = w.fixed;
  edges.insert(edges.end(), w.remaining.begin(), w.remaining.end());
  std::sort(edges.begin(), edges.end());
  assert(c.deleted == w.deleted && c.edges == edges && c.assumed == w.assumed &&
         c.assumedInstalled == w.assumedInstalled &&
         c.portalLinks == w.portalLinks && c.world == CandidateWorldToken(w));
  const auto query = Json::parse(EncodeTempleQuery(c));
  assert(query.at("world") == Json::parse(EncodeOotOverlay(w)).at("world") &&
         query.at("world") == Json::parse(EncodeMmOverlay(w)).at("world"));
  for (const auto game : {Game::Oot, Game::Mm}) {
    const auto o = SplitCandidateWorld(w, game);
    const auto &part = game == Game::Oot ? query : query.at("mm");
    Strings deleted;
    for (const auto &row : part.at("deleted"))
      deleted.insert(row.get<std::string>());
    assert(deleted == ToStrings(o.deleted));
    auto merged = o.fixed;
    merged.insert(merged.end(), o.remaining.begin(), o.remaining.end());
    std::sort(merged.begin(), merged.end());
    assert(part.at("edges").size() == merged.size());
    for (size_t i = 0; i < merged.size(); ++i)
      assert(part.at("edges")[i].at("src") == std::string(merged[i].src) &&
             part.at("edges")[i].at("dst") == std::string(merged[i].dst));
  }
}

// Every M1 combination in the all-vanilla world: the pools still to place give
// one OR edge per pinned candidate, split by source game; the reset-gated
// destinations and the VOID dead ends are the fixture's. Then the committed
// sample layout: no remaining edge, one fixed edge per route.
void SplitMatchesFixture() {
  size_t checked = 0, committed = 0;
  for (const auto &c : fixture.at("combinations")) {
    const auto s = FromFixture(c.at("settings"));
    const auto resolved = ResolvePools(s, Areas({}));
    const auto w = BuildCandidateWorld(resolved.fixed, resolved.pools, s);
    assert(w.fixed.empty() && w.assumed.empty() && w.assumedInstalled);
    assert(w.portalLinks.size() == 2);
    for (const auto &e : w.portalLinks)
      assert(IsReservedGameLink(e.src, e.dst));

    std::map<Game, Targets> expected;
    std::map<Game, Strings> sources;
    for (const auto &pool : c.at("pools"))
      for (const auto &group : pool.at("candidates"))
        for (const auto &src : group.at("sources")) {
          const auto name = src.get<std::string>();
          const auto game = GameOf(name);
          sources[game].insert(name);
          for (const auto &dst : group.at("destinations"))
            expected[game][name].insert(dst.get<std::string>());
        }
    Strings resetGated, deadEnds;
    for (const auto game : {Game::Oot, Game::Mm}) {
      const auto o = SplitCandidateWorld(w, game);
      assert(o.fixed.empty());
      assert(ToStrings(o.deleted) == sources[game]);
      assert(ByTarget(o.remaining) == expected[game]);
      for (const auto &e : o.remaining) {
        CheckEdge(e);
        if (e.resetTimeGate)
          resetGated.emplace(e.dst);
        if (e.toVoid)
          deadEnds.emplace(e.dst);
      }
      for (const auto row : o.deleted) // M1 sources lead into VOID or not
        assert(IsVoidArea(Find(row)->to) == PinnedVoid(row));
      CheckOverlay(w, game);
    }
    assert(resetGated == c.at("resetGated").get<Strings>());
    assert(deadEnds == c.at("deadEnds").get<Strings>());
    CheckTempleView(w, resolved.fixed, resolved.pools, s);
    ++checked;

    // No pool, no layout (erOneWays=ownGame with no flag or only Anywhere).
    assert(c.at("sampleLayout").empty() == c.at("pools").empty());
    if (c.at("sampleLayout").empty())
      continue;
    auto overrides = resolved.fixed;
    std::map<Game, Targets> routes;
    for (const auto &pair : c.at("sampleLayout")) {
      const auto src = pair[0].get<std::string>(),
                 dst = pair[1].get<std::string>();
      overrides[Find(src)->name] = Find(dst)->name;
      routes[GameOf(src)][src].insert(dst);
    }
    const auto done = BuildCandidateWorld(overrides, {}, s);
    assert(done.remaining.empty() && done.assumed.empty() &&
           done.portalLinks == w.portalLinks && done.deleted == w.deleted);
    assert(CandidateWorldToken(done) != CandidateWorldToken(w));
    for (const auto game : {Game::Oot, Game::Mm}) {
      const auto o = SplitCandidateWorld(done, game);
      assert(o.remaining.empty() && ByTarget(o.fixed) == routes[game]);
      for (const auto &e : o.fixed)
        CheckEdge(e);
      CheckOverlay(done, game);
    }
    CheckTempleView(done, overrides, {}, s);
    ++committed;
  }
  assert(checked == 66 && committed == 63);
}

// The 64 vanilla/MQ choices of the six Wallmaster dungeons: the OoT overlay
// deletes exactly the active Wallmaster sources, each into VOID, and offers
// each the same destinations as in the all-vanilla world.
void MqContexts() {
  for (const auto *id : {&kWallmasters, &kWidest}) {
    const auto &c = Combination(*id);
    const auto s = FromFixture(c.at("settings"));
    Strings vanillaTargets;
    for (const auto &pool : c.at("pools"))
      for (const auto &group : pool.at("candidates"))
        if (Find(group.at("sources")[0].get<std::string>())->type ==
                Type::Wallmaster &&
            GameOf(group.at("sources")[0].get<std::string>()) == Game::Oot)
          vanillaTargets = group.at("destinations").get<Strings>();
    assert(vanillaTargets.size() == (*id == kWallmasters ? 11u : 14u));
    size_t contexts = 0;
    for (const auto &entry : fixture.at("mqSources").at("table")) {
      const auto resolved =
          ResolvePools(s, Areas(entry.at("mq").get<Strings>()));
      const auto w = BuildCandidateWorld(resolved.fixed, resolved.pools, s);
      const auto o = SplitCandidateWorld(w, Game::Oot);
      Strings wallmasters;
      for (const auto row : o.deleted)
        if (Find(row)->type == Type::Wallmaster) {
          assert(IsVoidArea(Find(row)->to));
          wallmasters.emplace(row);
        }
      assert(wallmasters == entry.at("ootSources").get<Strings>());
      const auto targets = ByTarget(o.remaining);
      for (const auto &src : wallmasters)
        assert(targets.at(src) == vanillaTargets);
      ++contexts;
    }
    assert(contexts == 64);
  }
}

// The reset gate is on every MM destination without no-sot, whatever the
// source game, and on nothing else; it stays on a one-way mapped to itself,
// whose source exit is still deleted and placed again (:352-371).
void ResetGateAndVoid() {
  size_t voids = 0;
  for (const auto &r : kRegistry) {
    for (const auto *src : {"OOT_FIELD_OWL", "MM_WALLMASTER_DAMPE"}) {
      const auto e = MakeCandidateEdge(src, r.name);
      assert(e.resetTimeGate == (r.game == Game::Mm && !r.noSongOfTime));
      assert(e.toVoid == IsVoidArea(r.to));
    }
    if (IsVoidArea(r.to)) {
      assert(r.type == Type::Wallmaster || r.type == Type::OneWayWaterVoid);
      ++voids;
    }
  }
  assert(voids == 13 + 4 + 4);
  for (const auto &row : fixture.at("rows").items()) {
    const auto *live = Find(row.key());
    assert(live && live->noSongOfTime == row.value().at("noSot").get<bool>() &&
           live->to == row.value().at("to").get<std::string>());
  }
  bool threw = false;
  try {
    MakeCandidateEdge("OOT_FIELD_OWL", "NOT_A_ROW");
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  assert(threw);

  // Major alone: OoT 1<->1 and MM 1<->1, the identity.
  Settings s;
  s.erOneWays = Mode::OwnGame;
  s.erOneWaysMajor = true;
  const auto resolved = ResolvePools(s, Areas({}));
  assert(resolved.pools.size() == 1 && resolved.pools[0].name == "ONE_WAYS");
  const std::vector<CandidateEdge> identity{
      {"MM_SWAMP_FROM_IKANA_CANYON", "MM_SWAMP_FROM_IKANA_CANYON", true, false},
      {"OOT_LAKE_HYLIA_FROM_VALLEY", "OOT_LAKE_HYLIA_FROM_VALLEY", false,
       false}};
  auto w = BuildCandidateWorld(resolved.fixed, resolved.pools, s);
  assert(w.remaining == identity);
  auto overrides = resolved.fixed;
  for (const auto &e : identity)
    overrides[e.src] = e.dst;
  w = BuildCandidateWorld(overrides, {}, s);
  assert(w.fixed == identity && w.remaining.empty());
  assert(w.deleted ==
         (std::vector<std::string_view>{"MM_SWAMP_FROM_IKANA_CANYON",
                                        "OOT_LAKE_HYLIA_FROM_VALLEY"}));
  const auto mm = Json::parse(EncodeMmOverlay(w));
  assert(mm.at("deleted") ==
         Json::parse(R"([{"row":"MM_SWAMP_FROM_IKANA_CANYON","void":false}])"));
  assert(mm.at("fixed") ==
         Json::parse(R"([{"src":"MM_SWAMP_FROM_IKANA_CANYON",)"
                     R"("dst":"MM_SWAMP_FROM_IKANA_CANYON",)"
                     R"("resetTime":true,"void":false}])"));
  assert(Json::parse(EncodeOotOverlay(w)).at("fixed")[0].at("resetTime") ==
         false);

  // A cross-game pair (Full, not M1) is gated in the source's overlay.
  s.erOneWays = Mode::Full;
  w = BuildCandidateWorld(
      {},
      {{"X", {"OOT_FIELD_OWL"}, {"MM_SWAMP_FROM_IKANA_CANYON"}, false, false}},
      s);
  const auto oot = SplitCandidateWorld(w, Game::Oot);
  assert(oot.remaining.size() == 1 && oot.remaining[0].resetTimeGate &&
         SplitCandidateWorld(w, Game::Mm).remaining.empty());

  // Water voids are VOID dead ends, still gated (no no-sot).
  const auto v = MakeCandidateEdge("MM_WALLMASTER_DAMPE", "MM_VOID_GREAT_BAY");
  assert(v.toVoid && v.resetTimeGate);
}

// Two edges with the same rows stay two edges, never merged: the native side
// ORs them, as placeSingleExpr does (:213-215).
void OrDuplicates() {
  Settings s;
  s.erOneWays = Mode::OwnGame;
  s.erOneWaysIkana = s.erOneWaysWaterVoids = true;
  const Pools pools{{"X",
                     {"MM_IKANA_CASTLE_INTERIOR_KEG"},
                     {"MM_IKANA_CASTLE_INTERIOR_BLOCK", "MM_VOID_ZORA_CAPE"},
                     true,
                     false},
                    {"Y",
                     {"MM_IKANA_CASTLE_INTERIOR_KEG"},
                     {"MM_IKANA_CASTLE_INTERIOR_BLOCK"},
                     true,
                     false}};
  const Overrides overrides{
      {"MM_IKANA_CASTLE_INTERIOR_KEG", "MM_IKANA_CASTLE_INTERIOR_BLOCK"}};
  const auto w = BuildCandidateWorld(overrides, pools, s);
  const CandidateEdge block{"MM_IKANA_CASTLE_INTERIOR_KEG",
                            "MM_IKANA_CASTLE_INTERIOR_BLOCK", true, false};
  assert(w.fixed == std::vector<CandidateEdge>{block});
  assert(std::count(w.remaining.begin(), w.remaining.end(), block) == 2);
  assert(w.remaining.size() == 3 && w.deleted.size() == 1);
  const auto json = Json::parse(EncodeMmOverlay(w));
  assert(json.at("fixed").size() == 1 && json.at("remaining").size() == 3);
  assert(json.at("remaining")[0] == json.at("remaining")[1]);
  assert(json.at("remaining")[2].at("void") == true);
  const auto c = BuildTempleCandidate(overrides, pools, s);
  assert(std::count(c.edges.begin(), c.edges.end(), block) == 3);
  assert(Json::parse(EncodeOotOverlay(w)).at("remaining").empty());
}

// The same world gives the same text and token whatever order the pools came
// in; any other world gives another token. Both overlays and the Temple query
// carry it as `world`; the query's own `token` is the digest of its text.
void OrderIndependentToken() {
  const auto s = FromFixture(Combination(kWidest).at("settings"));
  const auto resolved = ResolvePools(s, Areas({}));
  auto pools = resolved.pools;
  // Place one Wallmaster, as Shuffler::place does.
  const auto wallmaster = std::find_if(pools.begin(), pools.end(), [](auto &p) {
    return p.name == "WALLMASTERS";
  });
  assert(wallmaster != pools.end());
  wallmaster->src.erase("OOT_WALLMASTER_GTG");
  wallmaster->dst.erase("OOT_VILLAGE_OWL");
  auto a = resolved.fixed;
  a["OOT_WALLMASTER_GTG"] = "OOT_VILLAGE_OWL";
  Overrides b{{"OOT_WALLMASTER_GTG", "OOT_VILLAGE_OWL"}};
  b.insert(resolved.fixed.begin(), resolved.fixed.end());
  auto reversed = pools;
  std::reverse(reversed.begin(), reversed.end());
  const auto wa = BuildCandidateWorld(a, pools, s);
  const auto wb = BuildCandidateWorld(b, reversed, s);
  const auto text = EncodeCandidateWorld(wa);
  assert(text == EncodeCandidateWorld(wb));
  assert(EncodeOotOverlay(wa) == EncodeOotOverlay(wb) &&
         EncodeMmOverlay(wa) == EncodeMmOverlay(wb));
  auto json = Json::parse(text);
  const uint32_t token = json.at("token");
  json.erase("token");
  assert(token == CandidateTokenHash(json.dump()) &&
         token == ComboRando::CwHashName(json.dump()) &&
         token == CandidateWorldToken(wa));
  assert(json.at("schema") == "ootmm-candidate-world-v1" &&
         json.at("registry") == std::string(kRegistrySourceSha256) &&
         json.at("fixed").size() == 1 && json.at("portalLinks").size() == 2);
  assert(Json::parse(EncodeOotOverlay(wa)).at("world") == token &&
         Json::parse(EncodeMmOverlay(wa)).at("world") == token);
  auto query =
      Json::parse(EncodeTempleQuery(BuildTempleCandidate(a, pools, s)));
  assert(query ==
         Json::parse(EncodeTempleQuery(BuildTempleCandidate(b, reversed, s))));
  assert(query.at("world") == token);
  const uint32_t queryToken = query.at("token");
  query.erase("token");
  assert(queryToken == TempleTokenHash(query.dump()) && queryToken != token);
  // Another destination, or the same route still open, is another world.
  a["OOT_WALLMASTER_GTG"] = "OOT_FIELD_OWL";
  assert(CandidateWorldToken(BuildCandidateWorld(a, pools, s)) != token);
  assert(Json::parse(EncodeTempleQuery(BuildTempleCandidate(a, pools, s)))
             .at("world") ==
         CandidateWorldToken(BuildCandidateWorld(a, pools, s)));
  assert(CandidateWorldToken(
             BuildCandidateWorld(resolved.fixed, resolved.pools, s)) != token);
}

// A large pool becomes ASSUMED exits (:293-314). No M1 pool is large, so the
// overlays refuse it; the world and the Temple query still describe it.
void AssumedRefused() {
  Settings s;
  s.erGrottos = Mode::OwnGame;
  const auto resolved = ResolvePools(s, Areas({}));
  const auto grottos =
      std::find_if(resolved.pools.begin(), resolved.pools.end(),
                   [](const Pool &p) { return p.name == "GROTTOS"; });
  assert(grottos != resolved.pools.end() && IsLargeCandidatePool(*grottos));
  const auto w = BuildCandidateWorld(resolved.fixed, resolved.pools, s);
  assert(w.assumed.size() == grottos->dst.size() && w.remaining.empty());
  for (const auto game : {Game::Oot, Game::Mm}) {
    bool threw = false;
    try {
      EncodeCandidateOverlay(w, game);
    } catch (const std::invalid_argument &) {
      threw = true;
    }
    assert(threw);
  }
  assert(Json::parse(EncodeCandidateWorld(w)).at("assumed").size() ==
         w.assumed.size());
  CandidateWorld one;
  one.assumed.push_back("OOT_DEKU_TREE");
  for (const auto encode : {&EncodeOotOverlay, &EncodeMmOverlay}) {
    bool threw = false;
    try {
      encode(one);
    } catch (const std::invalid_argument &) {
      threw = true;
    }
    assert(threw);
  }
  // M1 never needs one: no M1 pool is large, even in the MQ context with the
  // most OoT Wallmaster sources (MQ Spirit and BotW, 12).
  for (const auto &c : fixture.at("combinations")) {
    const auto m1 = FromFixture(c.at("settings"));
    for (const auto &pool : ResolvePools(m1, Areas({"BotW", "Spirit"})).pools)
      assert(!IsLargeCandidatePool(pool) && pool.src.size() <= 50);
  }
}
} // namespace

int main(int argc, char **argv) {
  assert(argc == 2);
  std::ifstream in(argv[1], std::ios::binary);
  assert(in);
  fixture = Json::parse(in);
  assert(fixture.at("upstreamCommit") == std::string(kUpstreamCommit));
  try {
    SplitMatchesFixture();
    MqContexts();
    ResetGateAndVoid();
    OrDuplicates();
    OrderIndependentToken();
    AssumedRefused();
  } catch (const std::exception &error) {
    std::cerr << "Unexpected exception: " << error.what() << '\n';
    return 1;
  }
  std::cout << "Candidate world splits per game as the pinned M1 contract: "
               "pools, MQ contexts, layouts, reset gate, VOID, OR, token, "
               "ASSUMED\n";
}
