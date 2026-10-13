// The real Shuffle.h pools and candidates against the pinned M1 contract
// (tools/ootmm/m1-contract.json), which tools/ootmm/export_m1_contract.py
// derives from OoTMM 413573d packages/logic/src/solver/entrances.ts without
// using Shuffle.h. MakePools gives the pool descriptions; a Shuffler with
// scripted bindings exposes what Shuffler::makePools (forTypes, used rows, game
// links, area filter) and the candidate filter of Shuffler::place (Assignable)
// produce.
#include "rando/entrances/Shuffle.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace ComboRando::Entrances;
using Json = nlohmann::json;
using Strings = std::set<std::string>;
using Candidates = std::map<std::string, Strings>;

// computeAllEntrances for one vanilla/MQ choice: registry areas absent from
// that world.
Strings AbsentAreas(const Json &presence, const Strings &mq) {
  Strings absent;
  for (const auto &area : presence["never"])
    absent.insert(area.get<std::string>());
  // MM JP layouts are off by default.
  for (const auto &area : presence["byMmJpLayout"]["jp"])
    absent.insert(area.get<std::string>());
  for (const auto &quest : presence["byQuest"].items())
    for (const auto &area :
         quest.value()[mq.contains(quest.key()) ? "vanilla" : "mq"])
      absent.insert(area.get<std::string>());
  return absent;
}

Settings FromFixture(const Json &j) {
  const auto mode = [](const std::string &value) {
    assert(value == "none" || value == "ownGame");
    return value == "none" ? Mode::None : Mode::OwnGame;
  };
  Settings s;
  s.erWallmasters = mode(j.at("erWallmasters").get<std::string>());
  s.erOneWays = mode(j.at("erOneWays").get<std::string>());
  s.erOneWaysMajor = j.at("erOneWaysMajor").get<bool>();
  s.erOneWaysIkana = j.at("erOneWaysIkana").get<bool>();
  s.erOneWaysOwls = j.at("erOneWaysOwls").get<bool>();
  s.erOneWaysWaterVoids = j.at("erOneWaysWaterVoids").get<bool>();
  s.erOneWaysAnywhere = j.at("erOneWaysAnywhere").get<bool>();
  assert(j.size() == 7);
  return s;
}

struct Stop {};

Bindings BaseBindings(const Strings &absent) {
  Bindings b;
  b.areaExists = [&absent](std::string_view area) {
    return !absent.contains(std::string(area));
  };
  b.sourceSupported =
      b.destinationSupported = [](const Record &) { return true; };
  b.validateAgeTemple = [](const Overrides &, const Pools &) -> bool {
    throw std::logic_error("Temple probe");
  };
  b.validateComplete = [](const Overrides &) -> bool {
    throw std::logic_error("Complete validation");
  };
  return b;
}

// The one pair a candidate check places: the override that is not a reserved
// game link.
std::pair<std::string_view, std::string_view>
Placed(const Overrides &overrides) {
  std::pair<std::string_view, std::string_view> placed;
  size_t count = 0;
  for (const auto &[src, dst] : overrides)
    if (src != "OOT_SHOP_MASKS" && src != "MM_CLOCK_TOWER_FROM_CLOCK_TOWN") {
      placed = {src, dst};
      ++count;
    }
  assert(count == 1);
  return placed;
}

// The pools Shuffler::makePools builds, read back from the first candidate
// check (pool 0, source 0, candidate 0). When no pool is placed, only the
// absence of RNG draws and candidate checks is asserted. Whether the complete
// validation runs is left open: Shuffle.h calls validateComplete, a known
// divergence from pinned OoTMM, which drops the empty pools
// (entrances.ts:581-589), keeps worldChanged false (entrances.ts:902) and so
// skips validate() (entrances.ts:1108-1111).
Pools RealPools(const Settings &s, const Strings &absent) {
  Pools pools;
  size_t draws = 0;
  auto b = BaseBindings(absent);
  b.chooseIndex = [&draws](size_t) {
    ++draws;
    return size_t{0};
  };
  b.validateAssumed = [&pools](const Overrides &overrides,
                               const Pools &next) -> bool {
    const auto [src, dst] = Placed(overrides);
    pools = next;
    pools.front().src.insert(src);
    pools.front().dst.insert(dst);
    throw Stop{};
  };
  b.validateComplete = [](const Overrides &) { return true; };
  try {
    Shuffler(s, std::move(b)).run();
  } catch (const Stop &) {
    return pools;
  }
  assert(draws == 0); // no candidate was offered
  return {};
}

// Every candidate Shuffler::place checks for every source: one attempt per
// source, walking all of its candidates (each check fails, so nothing is placed
// and nothing recurses).
Candidates RealCandidates(const Settings &s, const Strings &absent,
                          const Pools &pools) {
  struct Step {
    size_t pool, source, candidates;
  };
  std::vector<Step> steps;
  for (size_t i = 0; i < pools.size(); ++i) {
    size_t j = 0;
    for (const auto src : pools[i].src) {
      size_t n = 0;
      for (const auto dst : pools[i].dst)
        n += Assignable(*Find(src), *Find(dst), pools[i].ownGame, s);
      steps.push_back({i, j++, n});
    }
  }
  Candidates seen;
  size_t step = 0, phase = 0, left = 0;
  auto b = BaseBindings(absent);
  b.chooseIndex = [&](size_t count) -> size_t {
    if (step == steps.size())
      throw Stop{};
    const auto &current = steps[step];
    if (phase == 0) {
      assert(count == pools.size());
      phase = 1;
      return current.pool;
    }
    if (phase == 1) {
      assert(count == pools[current.pool].src.size());
      left = current.candidates;
      phase = left ? 2 : 0;
      step += left ? 0 : 1;
      return current.source;
    }
    assert(count == left); // place() offers exactly the Assignable destinations
    if (--left == 0) {
      phase = 0;
      ++step;
    }
    return 0;
  };
  b.validateAssumed = [&seen](const Overrides &overrides, const Pools &) {
    const auto [src, dst] = Placed(overrides);
    const bool fresh = seen[std::string(src)].insert(std::string(dst)).second;
    assert(fresh);
    return false;
  };
  try {
    Shuffler(s, std::move(b)).run();
  } catch (const Stop &) {
  }
  assert(step == steps.size());
  return seen;
}

struct Expected {
  std::string name;
  bool ownGame, alias;
  Strings srcTypes, dstTypes, src, dst;
  Candidates candidates;
};

std::vector<Expected> ExpectedPools(const Json &combination) {
  std::vector<Expected> result;
  for (const auto &p : combination["pools"]) {
    Expected e{p["name"].get<std::string>(), p["ownGame"].get<bool>(),
               p["alias"].get<bool>(),       p["srcTypes"].get<Strings>(),
               p["dstTypes"].get<Strings>(), {},
               p["dst"].get<Strings>(),      {}};
    for (const auto &group : p["candidates"])
      for (const auto &src : group["sources"]) {
        e.src.insert(src.get<std::string>());
        e.candidates[src.get<std::string>()] =
            group["destinations"].get<Strings>();
      }
    result.push_back(std::move(e));
  }
  return result;
}

// The fixture matrix is computed in the all-vanilla world; only the OoT
// Wallmaster sources depend on the vanilla/MQ choice, and they share one
// candidate set (both checked by the exporter).
void UseOotWallmasterSources(std::vector<Expected> &pools,
                             const Strings &sources, const Json &rows) {
  for (auto &p : pools) {
    if (p.name != "WALLMASTERS")
      continue;
    Strings shared;
    for (auto it = p.src.begin(); it != p.src.end();)
      if (rows.at(*it).at("game").get<std::string>() == "oot") {
        shared = p.candidates[*it];
        p.candidates.erase(*it);
        it = p.src.erase(it);
      } else {
        ++it;
      }
    assert(!shared.empty());
    for (const auto &src : sources) {
      p.src.insert(src);
      p.candidates[src] = shared;
    }
  }
}

Strings ToStrings(const Names &names) {
  Strings result;
  for (const auto name : names)
    result.emplace(name);
  return result;
}

void SamePools(const Pools &real, const std::vector<Expected> &expected) {
  assert(real.size() == expected.size());
  for (size_t i = 0; i < real.size(); ++i) {
    assert(real[i].name == expected[i].name &&
           real[i].ownGame == expected[i].ownGame &&
           real[i].alias == expected[i].alias);
    assert(ToStrings(real[i].src) == expected[i].src &&
           ToStrings(real[i].dst) == expected[i].dst);
  }
}

void SameCandidates(const Candidates &real,
                    const std::vector<Expected> &expected) {
  Candidates all;
  for (const auto &p : expected)
    for (const auto &[src, dsts] : p.candidates)
      if (!dsts.empty())
        all[src] = dsts;
  assert(real == all);
}

// Registry.h and the fixture describe the same rows; also learns each Type's
// upstream name.
std::map<Type, std::string> CheckRows(const Json &rows) {
  std::map<Type, std::string> names;
  std::set<std::string> seenNames;
  for (const auto &item : rows.items()) {
    const auto &row = item.value();
    const auto *r = Find(item.key());
    assert(r && (r->game == Game::Oot ? "oot" : "mm") ==
                    row["game"].get<std::string>());
    assert(r->id == row["id"].get<uint32_t>() &&
           r->from == row["from"].get<std::string>() &&
           r->to == row["to"].get<std::string>() &&
           r->fromMap == row["fromMap"].get<std::string>() &&
           r->toMap == row["toMap"].get<std::string>() &&
           r->noSongOfTime == row["noSot"].get<bool>());
    assert(r->reverse == (row["reverse"].is_null()
                              ? std::string()
                              : row["reverse"].get<std::string>()));
    const auto type = row["type"].get<std::string>();
    const auto [it, added] = names.emplace(r->type, type);
    assert(it->second == type && (!added || seenNames.insert(type).second));
  }
  return names;
}

Strings TypeNames(const Types &types,
                  const std::map<Type, std::string> &names) {
  Strings result;
  for (const auto type : types)
    result.insert(names.at(type));
  return result;
}

void Run(const char *path) {
  std::ifstream input(path, std::ios::binary);
  const auto fixture = Json::parse(input);
  assert(fixture["upstreamCommit"].get<std::string>() == kUpstreamCommit);
  const auto &rows = fixture["rows"];
  const auto &presence = fixture["registryAreaPresence"];
  const auto typeNames = CheckRows(rows);
  const auto vanilla = AbsentAreas(presence, {});

  // Every combination in the all-vanilla world: descriptions, pools and every
  // candidate.
  size_t placedCombinations = 0;
  for (const auto &combination : fixture["combinations"]) {
    const auto s = FromFixture(combination["settings"]);
    const auto expected = ExpectedPools(combination);
    const auto descriptions = MakePools(s);
    for (const auto &d : descriptions) {
      const auto it =
          std::find_if(expected.begin(), expected.end(),
                       [&](const auto &e) { return e.name == d.name; });
      // An empty pool (no M1 type selected) is dropped by the Shuffler.
      if (it == expected.end()) {
        assert(d.src.empty());
        continue;
      }
      assert(TypeNames(d.src, typeNames) == it->srcTypes &&
             TypeNames(d.dst, typeNames) == it->dstTypes);
    }
    if (descriptions.empty()) { // no bindings, RNG or layout
      assert(expected.empty() && Shuffler(s, {}).run().empty());
      continue;
    }
    const auto pools = RealPools(s, vanilla);
    SamePools(pools, expected);
    if (pools.empty())
      continue;
    SameCandidates(RealCandidates(s, vanilla, pools), expected);
    ++placedCombinations;
  }
  assert(fixture["combinations"].size() == 66 && placedCombinations == 63);

  // The MQ source table: the Wallmaster pool for every vanilla/MQ choice of the
  // six dungeons, and the shared candidate set for every active OoT source.
  const auto &alone =
      *std::find_if(fixture["combinations"].begin(),
                    fixture["combinations"].end(), [](const Json &c) {
                      return c["id"].get<std::string>() ==
                             "erWallmasters=ownGame;erOneWays=none";
                    });
  const auto s = FromFixture(alone["settings"]);
  size_t contexts = 0, shortfalls = 0;
  for (const auto &row : fixture["mqSources"]["table"]) {
    const auto absent = AbsentAreas(presence, row["mq"].get<Strings>());
    auto expected = ExpectedPools(alone);
    const auto sources = row["ootSources"].get<Strings>();
    UseOotWallmasterSources(expected, sources, rows);
    const auto pools = RealPools(s, absent);
    SamePools(pools, expected);
    SameCandidates(RealCandidates(s, absent, pools), expected);
    ++contexts;
    shortfalls += sources.size() >
                  expected.front().candidates.at(*sources.begin()).size();
  }
  assert(contexts == 64 &&
         shortfalls == fixture["mqSources"]["capacityShortfalls"].size() &&
         shortfalls == 4);
  std::cout << "Shuffle.h pools and candidates match the pinned M1 contract "
               "for 66 combinations and 64 vanilla/MQ contexts.\n";
}

int main(int argc, char **argv) {
  assert(argc == 2);
  try {
    Run(argv[1]);
  } catch (const std::exception &error) { // e.g. Shuffle.h rejects a row
    std::cerr << "Unexpected exception: " << error.what() << '\n';
    return 1;
  }
}
