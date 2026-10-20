// C4 release-configuration coverage proof (M1 plan A-10, P1-04). Built twice by
// entrance_release_coverage_tests.py, with and without COMBO_ENTRANCE_DEV.
//
// It pins which capability ids are Public. Then, for this build's capabilities,
// for the vanilla Wallmaster id named explicitly and for the vanilla and Master
// Quest ids together, it enumerates erWallmasters x all 4096 OoT quest masks.
// For every mask a set admits, every source and every destination assignable
// from one must have a logic edge row of the mask's quest, an arrival region,
// a native arrival at both ages, a runtime source classification, a Temple
// binding and a spoiler name, each read from the native tables the app builds
// (release_coverage_native.inc), and layouts routing every destination must
// validate, pass the seed guard and install in both games. A mask with more
// OoT sources than dungeon entrances (MQ Spirit and BotW with vanilla Shadow
// and Ganon) is refused by the generator with the capacity sentence; every
// other mask a set does not admit is refused by Validate, the seed guard, both
// installers, the generator (before any draw or native change) and the menu.
// Dropping any one binding must report a gap (negative control).
#include "rando/GenerationRules.h"
#include "rando/entrances/EntranceProfile.h"
#include "rando/entrances/EntranceSpoiler.h"
#include "rando/entrances/MmGrottoBindings.h"
#include "rando/entrances/MmOrdinaryBindings.h"
#include "rando/entrances/NativeArrival.h"
#include "rando/entrances/OotNativeArrival.h"
#include "rando/entrances/OotWallmasterSource.h"
#include "rando/entrances/UnifiedBridge.h"
#include "rando/entrances/Wallmasters.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// The real native enums and entrance table, as oot_wallmaster_arrivals_tests.py
// compiles them.
// clang-format off
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define RANDO_ENUM_END(name) };
// clang-format on
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerRegion.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
enum NativeEntrance {
#define DEFINE_ENTRANCE(name, ...) name,
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
// One row of the native entrance table: its scene and spawn.
struct EntranceLayer {
  std::string_view scene;
  int spawn;
};
static const EntranceLayer kEntranceTable[] = {
#define DEFINE_ENTRANCE(name, scene, spawn, ...) {#scene, spawn},
#include "tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
// combo_oot_wallmaster_arrivals.inc: row, logical id, native entrance, region.
struct OotArrival {
  std::string_view row;
  uint32_t logical;
  int native;
  RandomizerRegion region;
};
static const OotArrival kOotArrivals[] = {
#include "soh/Enhancements/randomizer/combo_oot_wallmaster_arrivals.inc"
};
#include "release_coverage_native.inc"

using namespace ComboRando::Entrances;
using ComboRando::kComboSeedFileType;
using ComboRando::kEntranceSeedFileType;
using ComboRando::kEntranceSeedVersion;
using ComboRando::RequireSupportedSeedCapabilities;
using ComboRando::UnsupportedComboSeedFormat;
using Json = nlohmann::json;

// The capability ids with a Public value (M1 plan P1-05). Each one needs a
// coverage model below; only the vanilla Wallmaster id has one.
static const std::set<std::string_view> kExpectedPublic = {
    kVanillaWallmasterCapability};

static constexpr unsigned kQuestMasks = 1u << kOotMqDungeons.size();
static OotQuests Quests(unsigned mask) {
  OotQuests quests{};
  for (size_t i = 0; i < quests.size(); ++i)
    quests[i] = (mask >> i) & 1u;
  return quests;
}

// Every native table the proof reads, copied so the negative control can drop
// one entry at a time.
struct Tables {
  std::vector<OotEdge> ootEdges;
  std::vector<OotArrival> ootArrivals;
  std::map<int, std::string_view> ootRegionScenes;
  std::vector<EntranceLayer> ootEntrances;
  std::vector<OotCapture> ootCaptures;
  std::vector<MmSource> mmSources;
  std::vector<MmCapture> mmCaptures;
  std::vector<MmArrival> mmArrivals;
  std::set<uint32_t> mmArrivalBound;
  std::set<std::string_view> templeBound;
  std::map<std::string_view, std::pair<std::string_view, std::string_view>>
      spoiler;
};
static Tables RealTables() {
  Tables t;
  t.ootEdges.assign(std::begin(kOotEdges), std::end(kOotEdges));
  t.ootArrivals.assign(std::begin(kOotArrivals), std::end(kOotArrivals));
  for (const auto &row : kOotRegionScenes)
    t.ootRegionScenes.emplace(row.region, row.scene);
  t.ootEntrances.assign(std::begin(kEntranceTable), std::end(kEntranceTable));
  t.ootCaptures.assign(std::begin(kOotCaptures), std::end(kOotCaptures));
  t.mmSources.assign(std::begin(kMmSources), std::end(kMmSources));
  t.mmCaptures.assign(std::begin(kMmCaptures), std::end(kMmCaptures));
  t.mmArrivals.assign(std::begin(kMmArrivals), std::end(kMmArrivals));
  // The MM arrival preflight's tables (CrossEntrance.inc.h MmArrivalBound).
  for (const auto &row : kMmOrdinaryBindings)
    t.mmArrivalBound.insert(row.logicalId);
  for (const auto &row : kMmGrottoGraphBindings)
    t.mmArrivalBound.insert(row.logicalId);
  // CheckTempleCapability's bound OoT rows (TempleAdapter.h).
  t.templeBound.insert(kOotTempleCandidateRows.begin(),
                       kOotTempleCandidateRows.end());
  t.templeBound.insert(kOotWallmasterSources.begin(),
                       kOotWallmasterSources.end());
  t.templeBound.insert(kOotWallmasterArrivalRows.begin(),
                       kOotWallmasterArrivalRows.end());
  // The spoiler names its rows by the frozen v1 labels (EntranceSpoiler.h).
  for (const auto &row : LayoutV1::kRows)
    t.spoiler[row.name] = {row.from, row.to};
  return t;
}

struct Rows {
  Names ootSources, mmSources, ootDestinations, mmDestinations;
};
// The sources of every pool and the destinations assignable from one.
static Rows PoolRows(const Pools &pools, const Settings &s) {
  Rows rows;
  for (const auto &pool : pools)
    for (const auto src : pool.src) {
      const auto &from = *Find(src);
      (from.game == Game::Oot ? rows.ootSources : rows.mmSources).insert(src);
      for (const auto dst : pool.dst)
        if (Assignable(from, *Find(dst), pool.ownGame, s))
          (Find(dst)->game == Game::Oot ? rows.ootDestinations
                                        : rows.mmDestinations)
              .insert(dst);
    }
  return rows;
}

// Every missing binding of the rows, by row name. Empty when all are bound.
static std::vector<std::string> Gaps(const Tables &t, const Rows &rows,
                                     const OotQuests &quests = {}) {
  std::vector<std::string> gaps;
  const auto gap = [&](std::string_view row, const std::string &what) {
    gaps.push_back(std::string(row) + ": " + what);
  };
  const auto spoiler = [&](std::string_view row) {
    const auto it = t.spoiler.find(row);
    if (it == t.spoiler.end() || it->second.first.empty() ||
        it->second.second.empty() || it->second.first == "NONE" ||
        it->second.second == "NONE")
      gap(row, "no spoiler name");
  };
  const auto regionScene = [&](int region) {
    const auto it = t.ootRegionScenes.find(region);
    return it == t.ootRegionScenes.end() ? std::string_view() : it->second;
  };
  // The native scene of every capture each runtime classifies by actor room.
  std::map<std::string_view, std::set<std::string_view>> ootCaptured,
      mmCaptured;
  for (int room = 0; room < 256; ++room) {
    for (const auto &capture : t.ootCaptures)
      if (const auto *found =
              OotWallmasterSource(capture.scene, static_cast<uint8_t>(room)))
        ootCaptured[found->name].insert(capture.native);
    for (const auto &capture : t.mmCaptures)
      if (const auto *found =
              WallmasterSource(capture.scene, static_cast<uint8_t>(room)))
        mmCaptured[found->name].insert(capture.native);
  }
  for (const auto src : rows.ootSources) {
    const auto *record = Find(src);
    if (!record || record->game != Game::Oot ||
        record->type != Type::Wallmaster)
      gap(src, "not an OoT Wallmaster row");
    // Logic: an audited edge of the source's quest from a native region
    // (combo_entrance_overlay.cpp, OotWallmasterSourceNames.h).
    std::set<std::string_view> anchorScenes;
    for (const auto &edge : t.ootEdges)
      if (edge.source == src && OotWallmasterEdgeInQuests(edge.audit, quests)) {
        if (regionScene(edge.anchor).empty())
          gap(src, "edge " + std::string(edge.audit) + " has no native region");
        anchorScenes.insert(regionScene(edge.anchor));
      }
    if (anchorScenes.empty())
      gap(src, "no logic edge row");
    // Runtime: a capture classified as this source, in the edge's scene
    // (combo_entrance_runtime.cpp).
    const auto &captureScenes = ootCaptured[src];
    if (captureScenes.empty())
      gap(src, "no runtime capture classification");
    for (const auto scene : anchorScenes)
      if (!captureScenes.contains(scene))
        gap(src, "logic edge scene " + std::string(scene) +
                     " is not a capture scene");
    if (!t.templeBound.contains(src))
      gap(src, "no Temple binding");
    spoiler(src);
  }
  for (const auto dst : rows.ootDestinations) {
    const auto *record = Find(dst);
    const OotArrival *arrival = nullptr;
    for (const auto &row : t.ootArrivals)
      if (row.row == dst)
        arrival = &row;
    if (!record || !arrival || arrival->logical != record->id) {
      gap(dst, "no arrival row");
    } else {
      const auto scene = regionScene(arrival->region);
      if (scene.empty())
        gap(dst, "no native arrival region");
      // The runtime transport's arrival (combo_entrance_runtime.cpp).
      const auto native = ResolveOotNativeArrival(record->id);
      if (!native || native->grotto || native->returning ||
          native->entrance > 0xffffu ||
          native->entrance != static_cast<uint32_t>(arrival->native))
        gap(dst, "no native arrival");
      // Child day, child night, adult day and adult night load the arrival
      // region's scene at one spawn.
      for (int layer = 0; layer < 4; ++layer) {
        const size_t index = static_cast<size_t>(arrival->native) + layer;
        if (index >= t.ootEntrances.size() ||
            t.ootEntrances[index].scene != scene ||
            t.ootEntrances[index].spawn !=
                t.ootEntrances[static_cast<size_t>(arrival->native)].spawn)
          gap(dst, "entrance layer " + std::to_string(layer) +
                       " does not arrive at the region's scene");
      }
    }
    if (!t.templeBound.contains(dst))
      gap(dst, "no Temple binding");
    spoiler(dst);
  }
  for (const auto src : rows.mmSources) {
    const auto *record = Find(src);
    if (!record || record->game != Game::Mm || record->type != Type::Wallmaster)
      gap(src, "not an MM Wallmaster row");
    // Logic: the region ComboEntrancesMM.cpp SourceForRegion explores from.
    std::string_view scene;
    for (const auto &row : t.mmSources)
      if (row.source == src)
        scene = row.scene;
    if (scene.empty())
      gap(src, "no logic source region");
    // Runtime: Combo_MMWallmasterTransport classifies a capture in that scene.
    if (scene.empty() || !mmCaptured[src].contains(scene))
      gap(src, "no runtime capture in the logic region's scene");
    // The Temple probe lists MM rows without probing them (TempleCandidate.h).
    if (!record || record->game != Game::Mm)
      gap(src, "no Temple binding");
    spoiler(src);
  }
  for (const auto dst : rows.mmDestinations) {
    const auto *record = Find(dst);
    if (!record) {
      gap(dst, "no registry row");
      continue;
    }
    // Logic: the unified MM graph routes it (MmUnifiedLayout.h) and one native
    // region receives it (Logic.cpp GetRegionIdFromEntrance).
    const auto route = MakeMmUnifiedRoute(*record);
    if (!MmUnifiedDestinationBound(dst) || route.destination.game != Game::Mm ||
        route.toVoid || record->id > 0xffffu)
      gap(dst, "no unified MM route");
    size_t regions = 0;
    for (const auto &row : t.mmArrivals)
      regions += row.entrance == record->id;
    if (regions != 1)
      gap(dst, std::to_string(regions) + " MM arrival regions");
    // Preflight at either age: MM_CanEntranceArrival checks only the age's
    // range and MmArrivalBound, which reads these tables.
    if (!t.mmArrivalBound.contains(record->id))
      gap(dst, "no MM arrival preflight binding");
    // The transport's season fixup stays a native entrance in every cleared
    // state, with the cutscene layer added (CrossEntrance.inc.h).
    for (unsigned flags = 0; flags < 16; ++flags) {
      int cutscene = 0;
      const auto native = FixMmNativeArrival(record->id, flags & 1, flags & 2,
                                             flags & 4, flags & 8, cutscene);
      const unsigned layer = cutscene >= 0xfff0 ? (cutscene & 0xf) + 1 : 0;
      if (native + layer > 0xffffu)
        gap(dst, "no native arrival in cleared state " + std::to_string(flags));
    }
    spoiler(dst);
  }
  return gaps;
}

// Matches every source to a distinct assignable destination, the forced pair
// first; the frozen v1 rules decide what is assignable.
static bool Match(const std::vector<std::string_view> &sources,
                  const std::vector<std::string_view> &destinations,
                  size_t forcedSource, size_t forcedDestination,
                  Overrides &routes) {
  std::vector<int> owner(destinations.size(), -1);
  const auto assignable = [&](size_t s, size_t d) {
    return LayoutV1::RowAssignable(*LayoutV1::FindRow(sources[s]),
                                   *LayoutV1::FindRow(destinations[d]), true);
  };
  if (!assignable(forcedSource, forcedDestination))
    return false;
  owner[forcedDestination] = static_cast<int>(forcedSource);
  std::function<bool(size_t, std::vector<bool> &)> augment =
      [&](size_t s, std::vector<bool> &seen) {
        for (size_t d = 0; d < destinations.size(); ++d) {
          if (seen[d] || d == forcedDestination || !assignable(s, d))
            continue;
          seen[d] = true;
          if (owner[d] < 0 || augment(static_cast<size_t>(owner[d]), seen)) {
            owner[d] = static_cast<int>(s);
            return true;
          }
        }
        return false;
      };
  for (size_t s = 0; s < sources.size(); ++s) {
    std::vector<bool> seen(destinations.size());
    if (s != forcedSource && !augment(s, seen))
      return false;
  }
  for (size_t d = 0; d < destinations.size(); ++d)
    if (owner[d] >= 0)
      routes[sources[static_cast<size_t>(owner[d])]] = destinations[d];
  return true;
}
// Layouts that together route every destination: one per OoT destination, its
// MM part rotated so every MM destination is used too.
static std::vector<Overrides> CoveringLayouts(const Rows &rows) {
  const std::vector<std::string_view> ootSrc(rows.ootSources.begin(),
                                             rows.ootSources.end()),
      ootDst(rows.ootDestinations.begin(), rows.ootDestinations.end()),
      mmSrc(rows.mmSources.begin(), rows.mmSources.end()),
      mmDst(rows.mmDestinations.begin(), rows.mmDestinations.end());
  std::vector<Overrides> layouts;
  for (size_t i = 0; i < std::max(ootDst.size(), mmDst.size()); ++i) {
    Overrides routes;
    bool oot = false, mm = false;
    for (size_t s = 0; s < ootSrc.size() && !oot; ++s)
      oot = Match(ootSrc, ootDst, s, i % ootDst.size(), routes);
    for (size_t s = 0; s < mmSrc.size() && !mm; ++s)
      mm = Match(mmSrc, mmDst, s, i % mmDst.size(), routes);
    assert(oot && mm);
    layouts.push_back(std::move(routes));
  }
  return layouts;
}

static EntranceLayout Layout(const Settings &s, const CapabilitySet &caps,
                             unsigned mask, const Overrides &routes) {
  EntranceLayout layout;
  layout.required = {std::string(mask ? kQuestsWallmasterCapability
                                      : kVanillaWallmasterCapability)};
  layout.devBuild = caps.devBuild;
  layout.settings = s;
  layout.ootMq = Quests(mask);
  layout.overrides = routes;
  return layout;
}
static Json Seed(const EntranceLayout &layout) {
  return Json{{"fileType", kEntranceSeedFileType},
              {"version", kEntranceSeedVersion},
              {"entranceLayout", EncodeEntranceLayout(layout)}};
}

// A native module that answers the quest context and counts every call that
// would change native state.
struct Native {
  static inline OotQuests quests{};
  static inline int changes = 0;
  static inline std::string context;
  static int Change(const char *) {
    ++changes;
    return 1;
  }
  static void Clear() { ++changes; }
  static const char *Dump() { return "{}"; }
  static const char *Context() {
    auto mq = Json::object();
    for (size_t i = 0; i < quests.size(); ++i)
      mq[std::string(kOotMqDungeons[i])] = quests[i];
    const Json value{{"profile", "ootmm-oot-area-context-v1"},
                     {"registry", kRegistrySourceSha256},
                     {"ootMq", mq}};
    context = value.dump();
    return context.c_str();
  }
  static NativeFns Fns() {
    return {Change, Change, Clear, Dump, Change,
            Change, Change, Clear, Dump, Context};
  }
  // The OoT oracle's reset, which finalizes the settings again before the
  // quest context is read, and a resolved child start.
  static inline int resets = 0;
  static void Reset() { ++resets; }
  static uint8_t ChildStart() { return 2; }
};
// More OoT Wallmasters than dungeon entrances (m1-contract.json
// mqSources.capacityShortfalls): MQ Spirit and BotW, vanilla Shadow and Ganon.
static bool Shortfall(const OotQuests &quests) {
  return quests[OotMqIndex("Spirit")] && quests[OotMqIndex("BotW")] &&
         !quests[OotMqIndex("Shadow")] && !quests[OotMqIndex("Ganon")];
}
// The Wallmaster dungeons only: masks that agree on them have the same rows.
static unsigned WallmasterProjection(unsigned mask) {
  unsigned projection = 0;
  for (const auto dungeon :
       {"Forest", "Spirit", "Shadow", "BotW", "GTG", "Ganon"})
    projection = projection * 2 + ((mask >> OotMqIndex(dungeon)) & 1u);
  return projection;
}
// The generator's answer for one mask: its UnifiedEntranceRefused sentence, or
// "" when it passed the quest, coverage and capacity gates and stopped at the
// Temple session these fakes leave out, before any draw or native change.
static std::string Generation(const Settings &s, const CapabilitySet &caps,
                              unsigned mask) {
  Native::quests = Quests(mask);
  Native::changes = Native::resets = 0;
  UnifiedEntranceStats stats;
  UnifiedGeneration g;
  g.settings = s;
  g.active = caps;
  g.seed = mask;
  g.readSetting = [](const char *, int fallback) { return fallback; };
  g.ootDump = g.mmDump = "{}";
  g.oot.Reset = Native::Reset;
  g.oot.GetStartAges = Native::ChildStart;
  g.stats = &stats;
  std::string sentence = "no refusal";
  try {
    GenerateEntranceLayout(g, Native::Fns(), {});
  } catch (const UnifiedEntranceRefused &error) {
    sentence = error.player();
  } catch (const TempleProbeUnsupported &error) {
    assert(std::string(error.what()) ==
           "Temple probe: native entry points are missing");
    sentence = "";
  }
  // The quest context is read after the oracle's first reset.
  assert(stats.randomDraws == 0 && Native::changes == 0 && Native::resets == 1);
  return sentence;
}

// Proves one capability set: returns the number of admitted settings x masks.
static size_t Prove(const char *name, const CapabilitySet &caps,
                    const Tables &tables) {
  size_t proved = 0;
  const auto selected = caps.Selected("");
  // The menu offers the row, with None and Own Game, exactly when Own Game is
  // admitted.
  Settings ownGame;
  ownGame.erWallmasters = Mode::OwnGame;
  const auto catalog = EntranceProfileCatalog(caps);
  if (AdmitsEntranceSettings(ownGame, selected))
    assert(catalog.size() == 1 && catalog[0]["key"] == "erWallmasters" &&
           catalog[0]["choices"].size() == 2);
  else
    assert(catalog.empty());
  const bool questsActive = caps.Active(kQuestsWallmasterCapability);
  for (const Mode mode : {Mode::None, Mode::OwnGame, Mode::Full}) {
    Settings s;
    s.erWallmasters = mode;
    ValidateEntranceSettings(s);
    const bool admitted = AdmitsEntranceSettings(s, selected);
    // Full stays hidden; None is always admitted.
    assert(mode != Mode::Full || !admitted);
    assert(mode != Mode::None || admitted);
    if (!admitted)
      continue;
    // The menu offers exactly the admitted choice, and reading the setting in
    // game keeps it.
    const auto read = [&](const char *key, int fallback) {
      return std::string_view(key) == "gCombo.Rando.Entrances.erWallmasters"
                 ? static_cast<int>(mode)
                 : fallback;
    };
    assert(ReadEntranceSettings(read, caps).erWallmasters == mode);
    if (mode == Mode::None) {
      for (unsigned mask = 0; mask < kQuestMasks; ++mask) {
        Native::quests = Quests(mask);
        Native::changes = 0;
        UnifiedGeneration g;
        g.settings = s;
        g.active = caps;
        assert(GenerateEntranceLayout(g, Native::Fns(), {}).is_null());
        assert(Native::changes == 0);
        ++proved;
      }
      continue;
    }
    // Every admitted combination is the vanilla id's own; a Master Quest
    // context moves it to the quests id once the quest context is read.
    const auto required = UnifiedRequirement(s, "", caps);
    assert(required == kVanillaWallmasterCapability);
    assert(AdmitsEntranceSettings(s, selected.Only(required)));
    std::set<unsigned> covered; // Wallmaster-dungeon projections laid out
    std::vector<Overrides> vanillaLayouts;
    size_t shortfalls = 0;
    for (unsigned mask = 0; mask < kQuestMasks; ++mask) {
      const auto quests = Quests(mask);
      const AreaExists areaExists = [&](std::string_view area) {
        return LayoutV1::AreaExists(area, quests, false);
      };
      const auto pools = ResolvePools(s, areaExists).pools;
      const auto rows = PoolRows(pools, s);
      const bool short_ = rows.ootSources.size() > rows.ootDestinations.size();
      assert(short_ == Shortfall(quests));
      if (mask == 0 || questsActive) {
        if (short_) {
          // OoTMM cannot place these (UnifiedBridge.h
          // RequireWallmasterCapacity): refused with the capacity sentence
          // before any draw or native change, after the oracle's reset; no
          // layout validates.
          assert(Generation(s, caps, mask) == kWallmasterCapacityText);
          for (const auto &routes : vanillaLayouts) {
            bool refused = false;
            try {
              Validate(Layout(s, caps, mask, routes), caps);
            } catch (const EntranceLayoutError &) {
              refused = true;
            }
            assert(refused);
          }
          ++shortfalls;
          ++proved;
          continue;
        }
        // The live generator's pools are the frozen v1 pools Validate uses.
        const auto frozen = LayoutV1::MakePools(s, quests, false);
        assert(pools.size() == frozen.size());
        for (size_t i = 0; i < pools.size(); ++i)
          assert(pools[i].name == frozen[i].name &&
                 pools[i].src == frozen[i].src &&
                 pools[i].dst == frozen[i].dst &&
                 pools[i].ownGame == frozen[i].ownGame &&
                 pools[i].alias == frozen[i].alias);
        assert(rows.ootSources.size() >= 7 && rows.ootSources.size() <= 11 &&
               rows.mmSources.size() == 4 &&
               rows.ootDestinations.size() == 11 &&
               rows.mmDestinations.size() == 7);
        assert(mask != 0 || rows.ootSources.size() == 9);
        const auto gaps = Gaps(tables, rows, quests);
        for (const auto &line : gaps)
          std::cerr << name << " mask " << mask << ": " << line << '\n';
        assert(gaps.empty());
        RequireUnifiedCoverage(pools, s, true);
        for (const auto age :
             {TempleStartingAge::Child, TempleStartingAge::Adult}) {
          TempleFacts facts;
          facts.startingAge = age;
          CheckTempleCapability(s, facts, areaExists);
        }
        assert(Generation(s, caps, mask).empty());
        // Layouts routing every destination validate, pass the seed guard,
        // install in both games and name every route in the spoiler, once per
        // set of Wallmaster dungeon quests.
        if (covered.insert(WallmasterProjection(mask)).second) {
          const auto layouts = CoveringLayouts(rows);
          if (mask == 0)
            vanillaLayouts = layouts;
          Names routed;
          for (const auto &routes : layouts) {
            const auto layout = Layout(s, caps, mask, routes);
            const auto validated = Validate(layout, caps);
            const auto seed = Seed(layout);
            RequireSupportedSeedCapabilities(seed, caps);
            OotUnifiedLayout oot;
            MmUnifiedLayout mm;
            oot.install(seed, caps);
            mm.install(seed, caps);
            assert(oot.routes().size() == rows.ootSources.size() &&
                   mm.routes().size() == 4);
            const auto spoiler = FormatEntranceSpoiler(&validated);
            for (const auto &[src, dst] : routes) {
              const auto *from = LayoutV1::FindRow(src);
              const auto *to = LayoutV1::FindRow(dst);
              assert(spoiler.find(std::string(from->from) + " to " +
                                  std::string(from->to) + " (" +
                                  std::string(src) + ")") != std::string::npos);
              assert(spoiler.find(std::string(to->to) + " from " +
                                  std::string(to->from) + " (" +
                                  std::string(dst) + ")") != std::string::npos);
              routed.insert(dst);
            }
          }
          assert(routed.size() == 18);
          // The frozen vanilla id never takes a Master Quest mask, and the
          // quests id never an all-vanilla one.
          auto other = Layout(s, caps, mask, layouts.front());
          other.required = {std::string(mask ? kVanillaWallmasterCapability
                                             : kQuestsWallmasterCapability)};
          bool refused = false;
          try {
            Validate(other, CapabilitySet::AtLeast(Stage::Hidden));
          } catch (const EntranceLayoutError &) {
            refused = true;
          }
          assert(refused);
        }
        ++proved;
        continue;
      }
      // Any Master Quest dungeon without the quests id: refused before
      // anything changes.
      auto layout = Layout(s, caps, mask, vanillaLayouts.front());
      layout.required = {std::string(kVanillaWallmasterCapability)};
      bool refused = false;
      try {
        Validate(layout, caps);
      } catch (const EntranceLayoutError &error) {
        refused = std::string(error.what()) ==
                  "Wallmaster Shuffle currently supports vanilla OoT dungeons "
                  "only";
      }
      assert(refused);
      for (const auto *id :
           {&kVanillaWallmasterCapability, &kQuestsWallmasterCapability}) {
        layout.required = {std::string(*id)};
        refused = false;
        try {
          RequireSupportedSeedCapabilities(Seed(layout), caps);
        } catch (const UnsupportedComboSeedFormat &) {
          refused = true;
        }
        assert(refused);
        OotUnifiedLayout oot;
        MmUnifiedLayout mm;
        refused = false;
        try {
          oot.install(Seed(layout), caps);
        } catch (const std::exception &) {
          refused = true;
        }
        assert(refused && !oot.active());
        refused = false;
        try {
          mm.install(Seed(layout), caps);
        } catch (const std::exception &) {
          refused = true;
        }
        assert(refused && !mm.active());
      }
      assert(Generation(s, caps, mask) == kWallmasterMasterQuestText);
      ++proved;
    }
    // Exactly the shortfall masks: MQ Spirit and BotW with vanilla Shadow and
    // Ganon, any of the other eight dungeons.
    assert(shortfalls == (questsActive ? 256u : 0u));
    assert(covered.size() == (questsActive ? 64u - 4u : 1u));
  }
  return proved;
}

int main() {
  const auto active = ActiveCapabilities();
  // Exactly the expected rows are Public, with every value.
  for (const auto &row : kEntranceCapabilities) {
    bool any = false, all = true;
    for (const auto &value : row.values) {
      any = any || value.stage == Stage::Public;
      all = all && value.stage == Stage::Public;
    }
    assert(any == kExpectedPublic.contains(row.id) && (!any || all));
  }
  for (const auto id : kExpectedPublic)
    assert(id == kVanillaWallmasterCapability);
#ifdef COMBO_ENTRANCE_DEV
  auto dev = CapabilitySet::AtLeast(Stage::Dev);
  dev.devBuild = true;
  assert(active == dev);
#else
  assert(!active.devBuild && active == CapabilitySet::AtLeast(Stage::Public));
  for (const auto &row : kEntranceCapabilities)
    assert(active.Active(row.id) == kExpectedPublic.contains(row.id));
#endif
  // Apart from erWallmasters, every setting row admits only its default.
  for (const auto &row : kEntranceSettings)
    for (int value = 0; value < static_cast<int>(row.choices.size()); ++value)
      if (std::string_view(row.key) != "erWallmasters" &&
          value != row.defaultValue)
        assert(!row.supports(value, active.Selected("")));

  const auto tables = RealTables();
  // This build's own settings, then the vanilla id by name and the vanilla and
  // Master Quest ids together, as this build would hold them, so the proof
  // runs whatever the ids' stages.
  auto vanilla =
      CapabilitySet::AtLeast(Stage::Hidden).Only(kVanillaWallmasterCapability);
  vanilla.devBuild = active.devBuild;
  auto withQuests =
      vanilla |
      CapabilitySet::AtLeast(Stage::Hidden).Only(kQuestsWallmasterCapability);
  withQuests.devBuild = active.devBuild;
  const size_t build = Prove("build", active, tables);
  const size_t named = Prove("vanilla id", vanilla, tables);
  const size_t both = Prove("vanilla and quests ids", withQuests, tables);
  assert(named == 2 * kQuestMasks && both == 2 * kQuestMasks);
  assert(build ==
         (active.Active(kVanillaWallmasterCapability) ? 2 : 1) * kQuestMasks);

  // With the vanilla id alone, the menu refuses every state in which native
  // can choose a Master Quest dungeon: MQ data alone, or playable MQ data (a
  // real oot-mq.o2r or OoTMM's bundled data over the vanilla oot.o2r: the
  // catalogue's mqPlayable) with any MQ Dungeon Setting but None. Without MQ
  // data, or with None, native keeps every dungeon vanilla (settings.cpp; the
  // harness pins those lines). With the Master Quest id it refuses none of
  // these: no MQ Dungeon Setting alone forces the capacity shortfall.
  const Json draft{{"fileType", kComboSeedFileType},
                   {"version", 1},
                   {"setupCVars", Json::object()},
                   {"mm", {{"settings", Json::object()}}}};
  for (const bool original : {true, false})
    for (const bool masterQuest : {true, false})
      for (const bool bundled : {false, true}) {
        if (bundled && !original)
          continue; // The bundled data plays only over the vanilla oot.o2r.
        const bool playable = (masterQuest || bundled) && original;
        for (int setting = 0; setting < 4; ++setting) {
          const Json catalog{{"hasOriginal", original},
                             {"hasMasterQuest", masterQuest},
                             {"mqPlayable", playable}};
          const auto read = [&](const char *key, int fallback) {
            return std::string_view(key) == "gRandoSettings.MQDungeons"
                       ? setting
                       : fallback;
          };
          const auto why =
              ProfileWallmasterRefusal(draft, catalog, read, vanilla);
          const bool mqPossible =
              (masterQuest && !original) || (playable && setting != 0);
          assert(mqPossible ? why.player == kWallmasterMasterQuestText
                            : why.empty());
          assert(ProfileWallmasterRefusal(draft, catalog, read, withQuests)
                     .empty());
          assert(ProfileWallmasterRefusal(draft, catalog, read) ==
                 ProfileWallmasterRefusal(draft, catalog, read, active));
        }
      }

  // Negative control: dropping any one binding reports a gap for its row.
  Settings s;
  s.erWallmasters = Mode::OwnGame;
  const auto quests = Quests(0);
  const auto rows =
      PoolRows(ResolvePools(s,
                            [&](std::string_view area) {
                              return LayoutV1::AreaExists(area, quests, false);
                            })
                   .pools,
               s);
  const auto firstOot = *rows.ootSources.begin();
  const auto firstMm = *rows.mmSources.begin();
  const auto ootDst = *rows.ootDestinations.begin();
  const auto mmDst = *rows.mmDestinations.begin();
  const auto *ootArrival =
      &*std::find_if(tables.ootArrivals.begin(), tables.ootArrivals.end(),
                     [&](const OotArrival &a) { return a.row == ootDst; });
  const std::vector<std::pair<std::string_view, std::function<void(Tables &)>>>
      drops = {
          {firstOot,
           [&](Tables &t) {
             std::erase_if(t.ootEdges, [&](const OotEdge &e) {
               return e.source == firstOot;
             });
           }},
          {firstOot,
           [&](Tables &t) {
             for (const auto &e : t.ootEdges)
               if (e.source == firstOot)
                 t.ootRegionScenes.erase(e.anchor);
           }},
          {firstOot,
           [&](Tables &t) {
             std::erase_if(t.ootCaptures, [&](const OotCapture &c) {
               for (int room = 0; room < 256; ++room)
                 if (const auto *found = OotWallmasterSource(
                         c.scene, static_cast<uint8_t>(room));
                     found && found->name == firstOot)
                   return true;
               return false;
             });
           }},
          {firstOot, [&](Tables &t) { t.templeBound.erase(firstOot); }},
          {firstOot, [&](Tables &t) { t.spoiler.erase(firstOot); }},
          {ootDst,
           [&](Tables &t) {
             std::erase_if(t.ootArrivals, [&](const OotArrival &a) {
               return a.row == ootDst;
             });
           }},
          {ootDst,
           [&](Tables &t) { t.ootRegionScenes.erase(ootArrival->region); }},
          {ootDst,
           [&](Tables &t) {
             t.ootEntrances[static_cast<size_t>(ootArrival->native) + 2].scene =
                 "SCENE_ADULT_ELSEWHERE";
           }},
          {ootDst, [&](Tables &t) { t.templeBound.erase(ootDst); }},
          {ootDst, [&](Tables &t) { t.spoiler[ootDst].second = ""; }},
          {firstMm,
           [&](Tables &t) {
             std::erase_if(t.mmSources, [&](const MmSource &m) {
               return m.source == firstMm;
             });
           }},
          {firstMm, [&](Tables &t) { t.mmCaptures.clear(); }},
          {firstMm, [&](Tables &t) { t.spoiler.erase(firstMm); }},
          {mmDst,
           [&](Tables &t) {
             std::erase_if(t.mmArrivals, [&](const MmArrival &a) {
               return a.entrance == Find(mmDst)->id;
             });
           }},
          {mmDst, [&](Tables &t) { t.mmArrivalBound.erase(Find(mmDst)->id); }},
          {mmDst, [&](Tables &t) { t.spoiler[mmDst].first = "NONE"; }},
      };
  for (const auto &[row, drop] : drops) {
    auto altered = tables;
    drop(altered);
    const auto gaps = Gaps(altered, rows);
    assert(std::any_of(gaps.begin(), gaps.end(), [&](const std::string &line) {
      return line.rfind(std::string(row) + ": ", 0) == 0;
    }));
  }
  std::cout << "PASS: " << kExpectedPublic.size() << " Public id(s); " << build
            << " build, " << named << " vanilla-id and " << both
            << " vanilla+quests settings x quest masks; 7 to 11 OoT + 4 MM "
               "sources and 11+7 destinations bound, 256 capacity refusals; "
            << drops.size() << " dropped bindings caught\n";
}
