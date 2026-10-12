// BuildTempleCandidate against an independent re-implementation of the pinned
// changedWorld (OoTMM 413573d packages/logic/src/solver/entrances.ts:203-218,
// :289-385) over Registry.h.
#include "rando/CrossWorldRando.h"
#include "rando/entrances/OotEdgeBindingNames.h"
#include "rando/entrances/TempleCandidate.h"
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace ComboRando::Entrances;
using Json = nlohmann::json;

// Symbolic world: area -> destination area -> disjuncts. A disjunct names the
// ORIGINAL area pair whose expression it carries, suffixed "&sot" when
// can_reset_time is ANDed on.
using Expr = std::set<std::string>;
using World = std::map<std::string, std::map<std::string, Expr>>;
std::string From(std::string_view row) { return std::string(Find(row)->from); }
std::string To(std::string_view row) { return std::string(Find(row)->to); }
World Original() {
  World w;
  for (const auto &r : kRegistry)
    if (r.from != "NONE" && r.to != "NONE")
      w[std::string(r.from)][std::string(r.to)] = {
          "E(" + std::string(r.from) + "->" + std::string(r.to) + ")"};
  return w;
}

// Line-by-line port of changedWorld/placeSingleExpr/changeWorldAssumePools on
// the symbolic world.
World ReferenceWorld(const Overrides &overrides, const Pools &pools,
                     const Settings &s) {
  const World original = Original();
  World w = original;
  auto placeSingleExpr = [&](std::string_view o, std::string_view n) {
    const auto &replacement = *Find(n);
    Expr expr;
    for (const auto &d :
         original.at(From(o)).at(To(o))) // getExpr reads this.world
      expr.insert(replacement.game == Game::Mm && !replacement.noSongOfTime
                      ? d + "&sot"
                      : d);
    auto &slot = w[From(o)][To(n)];
    slot.insert(expr.begin(), expr.end()); // exprOr with any existing exit
  };
  for (const auto &[o, n] : overrides)
    w[From(o)].erase(To(o));
  for (const auto &pool : pools)
    for (const auto o : pool.src)
      w[From(o)].erase(To(o));
  for (const auto &[o, n] : overrides)
    placeSingleExpr(o, n);
  std::vector<const Names *> assumedSets;
  for (const auto &pool : pools) {
    if (pool.src.size() * pool.dst.size() > 5000 && pool.src.size() > 50)
      assumedSets.push_back(&pool.dst);
    else
      for (const auto o : pool.src)
        for (const auto n : pool.dst)
          if (Assignable(*Find(o), *Find(n), pool.ownGame, s))
            placeSingleExpr(o, n);
  }
  w["ASSUMED"]; // installed for every candidate: `if (assumedSets)` tests an
                // array
  for (const auto *set : assumedSets)
    for (const auto n : *set)
      w["ASSUMED"][To(n)] = {"true"};
  w["OOT SPAWN"]["ASSUMED"] = {"true"};
  return w;
}

// The same edit described by a TempleCandidate (portal links are reference
// edits too).
World Apply(const TempleCandidate &c) {
  const World original = Original();
  World w = original;
  for (const auto o : c.deleted)
    w[From(o)].erase(To(o));
  for (const auto &e : c.portalLinks)
    w[From(e.src)].erase(To(e.src));
  auto add = [&](const TempleEdge &e) {
    for (const auto &d : original.at(From(e.src)).at(To(e.src)))
      w[From(e.src)][To(e.dst)].insert(e.resetTimeGate ? d + "&sot" : d);
  };
  for (const auto &e : c.portalLinks)
    add(e);
  for (const auto &e : c.edges)
    add(e);
  w["ASSUMED"];
  if (c.assumedInstalled)
    w["OOT SPAWN"]["ASSUMED"] = {"true"};
  for (const auto n : c.assumed)
    w["ASSUMED"][To(n)] = {"true"};
  return w;
}

// Pools as the Shuffler builds them for a fresh layout (Shuffler::makePools
// without reservations).
Pools PoolsFor(const Settings &s) {
  Pools pools;
  for (const auto &d : MakePools(s)) {
    Pool p{d.name, {}, {}, d.ownGame, d.alias};
    for (const auto &r : kRegistry) {
      if (s.games == Games::Oot && r.game != Game::Oot)
        continue;
      // Shuffler::forTypes adds each row's reverse with it.
      for (auto *names : {d.src.contains(r.type) ? &p.src : nullptr,
                          d.dst.contains(r.type) ? &p.dst : nullptr})
        if (names) {
          names->insert(r.name);
          if (const auto *reverse = Reverse(r))
            names->insert(reverse->name);
        }
    }
    for (auto reserved :
         {"OOT_MARKET_FROM_MASK_SHOP", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER"})
      p.src.erase(reserved);
    for (auto reserved : {"OOT_SHOP_MASKS", "MM_CLOCK_TOWER_FROM_CLOCK_TOWN"})
      p.dst.erase(reserved);
    pools.push_back(std::move(p));
  }
  return pools;
}
const Overrides kReserved{
    {"OOT_SHOP_MASKS", "MM_CLOCK_TOWN_FROM_CLOCK_TOWER"},
    {"MM_CLOCK_TOWER_FROM_CLOCK_TOWN", "OOT_MARKET_FROM_MASK_SHOP"}};
Settings OneWays() {
  Settings s;
  s.erOneWays = Mode::OwnGame;
  s.erOneWaysMajor = s.erOneWaysOwls = s.erOneWaysIkana =
      s.erOneWaysWaterVoids = true;
  return s;
}
void Same(const Overrides &overrides, const Pools &pools, const Settings &s) {
  assert(Apply(BuildTempleCandidate(overrides, pools, s)) ==
         ReferenceWorld(overrides, pools, s));
}

void TrancheOneMatchesReference() {
  const auto s = OneWays();
  auto pools = PoolsFor(s);
  assert(pools.size() == 1 && pools[0].name == "ONE_WAYS" && pools[0].ownGame &&
         !IsLargeTemplePool(pools[0]));
  Same(kReserved, pools, s);
  auto c = BuildTempleCandidate(kReserved, pools, s);
  // Every OoT row a tranche-1 candidate deletes or adds is a bound native
  // candidate row.
  std::set<std::string_view> bound(kOotTempleCandidateRows.begin(),
                                   kOotTempleCandidateRows.end());
  for (const auto row : c.deleted)
    assert(Find(row)->game == Game::Mm || bound.contains(row));
  for (const auto &e : c.edges) {
    assert(Find(e.src)->game == Find(e.dst)->game); // own game
    if (Find(e.src)->game == Game::Oot)
      assert(bound.contains(e.src) && bound.contains(e.dst) &&
             !e.resetTimeGate);
  }
  // Place one owl, then evaluate the remaining pool, as Shuffler::place does.
  auto overrides = kReserved;
  overrides["OOT_FIELD_OWL"] = "OOT_LAKE_HYLIA_FROM_VALLEY";
  auto rest = pools;
  rest[0].src.erase("OOT_FIELD_OWL");
  rest[0].dst.erase("OOT_LAKE_HYLIA_FROM_VALLEY");
  Same(overrides, rest, s);
  c = BuildTempleCandidate(overrides, rest, s);
  assert(std::count(
             c.edges.begin(), c.edges.end(),
             TempleEdge{"OOT_FIELD_OWL", "OOT_LAKE_HYLIA_FROM_VALLEY"}) == 1);
  assert(std::binary_search(c.deleted.begin(), c.deleted.end(),
                            std::string_view("OOT_FIELD_OWL")));
}

void LargePoolsAreAssumed() {
  Settings s;
  s.erGrottos = Mode::OwnGame;
  s.erOneWays = Mode::OwnGame;
  s.erOneWaysOwls = true;
  auto pools = PoolsFor(s);
  Same(kReserved, pools, s);
  auto c = BuildTempleCandidate(kReserved, pools, s);
  const auto &grottos =
      *std::find_if(pools.begin(), pools.end(),
                    [](const Pool &p) { return p.name == "GROTTOS"; });
  assert(IsLargeTemplePool(grottos) && c.assumed.size() == grottos.dst.size());
  for (const auto &e :
       c.edges) // no assumption edge, filter or gate from the large pool
    assert(!grottos.src.contains(e.src));
  for (const auto src : grottos.src) // its sources are still deleted
    assert(std::binary_search(c.deleted.begin(), c.deleted.end(), src));
}

void DuplicatesGatesAndAssignability() {
  // An override and a small-pool pair with the same rows stay two edges (OR,
  // :213-215).
  Settings s = OneWays();
  s.erOneWays = Mode::Full;
  Pools pools{{"X",
               {"OOT_FIELD_OWL"},
               {"OOT_VILLAGE_OWL", "MM_SWAMP_FROM_IKANA_CANYON"},
               false,
               false}};
  Overrides overrides{{"OOT_FIELD_OWL", "OOT_VILLAGE_OWL"}};
  Same(overrides, pools, s);
  auto c = BuildTempleCandidate(overrides, pools, s);
  assert(std::count(c.edges.begin(), c.edges.end(),
                    TempleEdge{"OOT_FIELD_OWL", "OOT_VILLAGE_OWL"}) == 2);
  // Full (not own game) admits the cross-game pair, gated because Swamp lacks
  // no-sot.
  assert(std::count(
      c.edges.begin(), c.edges.end(),
      TempleEdge{"OOT_FIELD_OWL", "MM_SWAMP_FROM_IKANA_CANYON", true}));
  pools[0].ownGame = true;
  c = BuildTempleCandidate(overrides, pools, s);
  for (const auto &e : c.edges)
    assert(Find(e.dst)->game == Game::Oot);
  // The reset gate is exactly "MM destination without no-sot"; v32.3 has 28
  // no-sot MM rows.
  int noSot = 0;
  for (const auto &r : kRegistry) {
    noSot += r.game == Game::Mm && r.noSongOfTime;
    assert(MakeTempleEdge("OOT_SPAWN_CHILD", r.name).resetTimeGate ==
           (r.game == Game::Mm && !r.noSongOfTime));
  }
  assert(noSot == 28);
}

void PortalsAssumedAndEncoding() {
  const Settings s = OneWays();
  auto empty = BuildTempleCandidate({}, {}, s);
  assert(empty.assumedInstalled && empty.assumed.empty() &&
         empty.edges.empty() && empty.deleted.empty());
  Same({}, {}, s);
  auto c = BuildTempleCandidate(kReserved, {}, s);
  assert(c.portalLinks.size() == 2 && c.deleted.empty() && c.edges.empty());
  // The games=Oot reservation is not a portal: it would need a real (unbound)
  // edit.
  c = BuildTempleCandidate({{"OOT_SHOP_MASKS", "OOT_MARKET_FROM_MASK_SHOP"}},
                           {}, s);
  assert(c.portalLinks.empty() && c.deleted.size() == 1 && c.edges.size() == 1);

  // Encoding and token do not depend on insertion or pool order.
  auto pools = PoolsFor(s);
  pools.push_back(
      {"SMALL", {"MM_VOID_ZORA_CAPE"}, {"MM_VOID_GREAT_BAY"}, true, false});
  auto reversed = pools;
  std::reverse(reversed.begin(), reversed.end());
  Overrides a = kReserved, b;
  a["OOT_VILLAGE_OWL"] = "OOT_FIELD_OWL";
  b["OOT_VILLAGE_OWL"] = "OOT_FIELD_OWL";
  b.insert(kReserved.begin(), kReserved.end());
  const auto first = EncodeTempleQuery(BuildTempleCandidate(a, pools, s));
  assert(first == EncodeTempleQuery(BuildTempleCandidate(b, reversed, s)));
  auto json = Json::parse(first);
  const uint32_t token = json["token"];
  json.erase("token");
  assert(token == TempleTokenHash(json.dump()) &&
         token == ComboRando::CwHashName(json.dump()));
  assert(json["schema"] == "oot-temple-probe-query-v1" &&
         json["registry"] == std::string(kRegistrySourceSha256));
  assert(json["assumedInstalled"] == true && json["assumed"].empty() &&
         json["portalLinks"].size() == 2);
  for (const auto &row : json["deleted"])
    assert(Find(row.get<std::string>())->game == Game::Oot);
  for (const auto &row : json["mm"]["deleted"])
    assert(Find(row.get<std::string>())->game == Game::Mm);
  for (const auto &e : json["edges"])
    assert(Find(e["src"].get<std::string>())->game == Game::Oot);
  assert(!json["mm"]["edges"].empty() &&
         json["edges"].size() + json["mm"]["edges"].size() ==
             BuildTempleCandidate(a, pools, s).edges.size());
  // A different candidate changes the token.
  a["OOT_VILLAGE_OWL"] = "OOT_LAKE_HYLIA_FROM_VALLEY";
  assert(Json::parse(EncodeTempleQuery(
             BuildTempleCandidate(a, pools, s)))["token"] != token);
}

int main() {
  TrancheOneMatchesReference();
  LargePoolsAreAssumed();
  DuplicatesGatesAndAssignability();
  PortalsAssumedAndEncoding();
  std::cout << "Temple candidate matches the pinned changedWorld, ASSUMED, "
               "gates, portals and encoding\n";
}
