// The entrance capability catalog (Capabilities.h) and the settings readers
// that consult it (OotmmSettings.h). CMake builds this twice: as shipped, and
// with COMBO_ENTRANCE_DEV, the dev-only switch.
#include "rando/entrances/OotmmSettings.h"
#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace ComboRando::Entrances;

namespace {
constexpr int kInjected[] = {-1, 0, 1, 2, 3, 255, 256};
const std::set<std::string_view> kM1Keys = {
    "erWallmasters",    "erOneWays",     "erOneWaysMajor",
    "erOneWaysIkana",   "erOneWaysOwls", "erOneWaysWaterVoids",
    "erOneWaysAnywhere"};
const Stage kStages[] = {Stage::Hidden, Stage::Dev, Stage::Public};

// The stage of every catalog value, as section 6 of the M1 plan stages them.
// Phase 0: everything is Hidden. A unit that opens a value updates this table.
Stage ExpectedStage(std::string_view id, std::string_view key) {
  (void)id;
  (void)key;
  return Stage::Hidden;
}

bool Same(const Settings &a, const Settings &b) {
  for (const auto &row : kEntranceSettings)
    if (row.get(a) != row.get(b))
      return false;
  return a.games == b.games && a.erWarps == b.erWarps &&
         a.erOneWaysSongs == b.erOneWaysSongs &&
         a.erOneWaysStatues == b.erOneWaysStatues &&
         a.erOneWaysWoods == b.erOneWaysWoods;
}

Settings Read(const Settings &in, const CapabilitySet &caps,
              std::string_view selected = {}) {
  return ReadEntranceSettings(
      [&](const char *cvar, int fallback) {
        for (const auto &row : kEntranceSettings)
          if (std::string_view(cvar) == row.cvar)
            return row.get(in);
        assert(false);
        return fallback;
      },
      caps, selected);
}

void CatalogRows() {
  const std::vector<std::string_view> ids = {
      "wallmasters-own-game-mm-tranche", "wallmasters-own-game-v1",
      "one-ways-own-game-v1", "wallmasters-with-one-ways-v1"};
  assert(kCapabilityCount == ids.size());
  std::set<std::string_view> unique;
  for (size_t i = 0; i < kCapabilityCount; ++i) {
    const auto &row = kEntranceCapabilities[i];
    assert(row.id == ids[i] && unique.insert(row.id).second);
    assert(CapabilitySet::Known(row.id));
    assert(row.devOnly == (row.id == "wallmasters-own-game-mm-tranche"));
    std::set<std::string_view> keys;
    for (const auto &value : row.values) {
      // No devOnly row is ever Public, and Full is never admitted.
      assert(!(row.devOnly && value.stage == Stage::Public));
      assert(value.value != "full" && value.value != "false");
      const auto *setting = FindEntranceSetting(value.key);
      assert(setting != nullptr && kM1Keys.count(value.key) == 1);
      assert(keys.insert(value.key).second);
      assert(value.stage == ExpectedStage(row.id, value.key));
    }
    // Companion closure: every companion exists and lists each of this row's
    // values at a stage at least as open.
    for (const auto companion : row.companions) {
      assert(CapabilitySet::Known(companion) && companion != row.id);
      assert(!row.devOnly);
    }
    for (const auto &value : row.values) {
      bool listed = row.companions.empty();
      for (const auto companion : row.companions)
        for (const auto &other : kEntranceCapabilities)
          if (other.id == companion)
            for (const auto &theirs : other.values)
              listed = listed || (theirs.key == value.key &&
                                  theirs.value == value.value &&
                                  theirs.stage >= value.stage);
      assert(listed);
    }
  }
  assert(!CapabilitySet::Known("wallmasters-own-game") &&
         !CapabilitySet::Known(""));
  // The tranche and the release row admit the same value; the companion
  // combines both one-way and Wallmaster values.
  const auto all = CapabilitySet::AtLeast(Stage::Hidden);
  for (const auto id : ids)
    assert(all.Active(id) && !all.Only(id).Empty());
  assert(all.Only("wallmasters-own-game-mm-tranche")
             .Admits("erWallmasters", "ownGame"));
  assert(!all.Only("one-ways-own-game-v1").Admits("erWallmasters", "ownGame"));
  assert(all.Only("unknown-id").Empty());
  assert(!all.WithoutDevOnly().Active("wallmasters-own-game-mm-tranche"));
  assert(all.WithoutDevOnly().Active("wallmasters-own-game-v1"));
  assert(all.Selected("") == all.WithoutDevOnly());
  for (const auto id : ids)
    assert(all.Selected(id) == all.Only(id));
  assert(
      (all.Only("wallmasters-own-game-v1") | all.Only("one-ways-own-game-v1"))
          .Active("one-ways-own-game-v1"));
}

void StageSets() {
  for (const Stage least : kStages) {
    const auto set = CapabilitySet::AtLeast(least);
    assert(!set.devBuild);
    for (size_t i = 0; i < kCapabilityCount; ++i) {
      const auto &row = kEntranceCapabilities[i];
      bool open = false;
      for (const auto &value : row.values) {
        assert(set.Admits(i, value.key, value.value) == (value.stage >= least));
        open = open || value.stage >= least;
      }
      assert(set.Active(row.id) == open);
      // Runtime check of the static_assert: no devOnly row ships.
      if (least == Stage::Public && row.devOnly)
        assert(!set.Active(row.id));
      // Companion closure holds in every staged set: an admitted value of a
      // companion row is admitted by one of the rows it names.
      for (const auto &value : row.values)
        if (!row.companions.empty() && set.Admits(i, value.key, value.value)) {
          bool listed = false;
          for (const auto companion : row.companions)
            for (size_t c = 0; c < kCapabilityCount; ++c)
              listed = listed || (kEntranceCapabilities[c].id == companion &&
                                  set.Admits(c, value.key, value.value));
          assert(listed);
        }
    }
  }
  // Phase 0: nothing is open in either build configuration.
  assert(CapabilitySet::AtLeast(Stage::Public).Empty());
  assert(CapabilitySet::AtLeast(Stage::Dev).Empty());
  assert(!CapabilitySet::AtLeast(Stage::Hidden).Empty());
}

void ActiveSetFollowsTheBuild() {
  const auto active = ActiveCapabilities();
#ifdef COMBO_ENTRANCE_DEV
  auto expected = CapabilitySet::AtLeast(Stage::Dev);
  expected.devBuild = true;
  assert(active.devBuild && active == expected);
#else
  assert(!active.devBuild && active == CapabilitySet::AtLeast(Stage::Public));
#endif
  for (const auto &row : kEntranceCapabilities)
    if (row.devOnly)
      assert(!CapabilitySet::AtLeast(Stage::Public).Active(row.id));
}

// The shipping set strips every non-default value from every source a reader
// sees (ActiveSetFollowsTheBuild shows a shipping build uses that set).
void ShippingStripsEverything() {
  {
    const auto caps = CapabilitySet::AtLeast(Stage::Public);
    for (const auto &row : kEntranceSettings) {
      assert(!row.visible(caps));
      for (const int value : kInjected)
        assert(row.supports(value, caps) == (value == row.defaultValue));
    }
    for (const int injected : kInjected) {
      size_t reads = 0;
      const auto s = ReadEntranceSettings(
          [&](const char *cvar, int fallback) {
            assert(
                std::string_view(cvar).starts_with("gCombo.Rando.Entrances."));
            assert(fallback == 0);
            ++reads;
            return injected;
          },
          caps);
      assert(reads == std::size(kEntranceSettings) && MakePools(s).empty());
      assert(Same(s, Settings{}));
    }
  }
}

// A set admits exactly the catalog values at its stage, and nothing else.
void SetsAdmitExactlyTheCatalog() {
  for (const Stage least : kStages) {
    const auto set = CapabilitySet::AtLeast(least);
    for (const auto &row : kEntranceSettings) {
      bool visible = false;
      for (size_t value = 0; value < row.choices.size(); ++value) {
        bool listed = false;
        for (const auto &capability : kEntranceCapabilities)
          for (const auto &admitted : capability.values)
            listed = listed || (admitted.key == row.key &&
                                admitted.value == row.choices[value].value &&
                                admitted.stage >= least);
        const int v = static_cast<int>(value);
        assert(row.supports(v, set) == (v == row.defaultValue || listed));
        visible = visible || (v != row.defaultValue && listed);
      }
      assert(row.visible(set) == visible);
    }
  }
  const auto all = CapabilitySet::AtLeast(Stage::Hidden);
  std::set<std::string_view> visible;
  for (const auto &row : kEntranceSettings)
    if (row.visible(all))
      visible.insert(row.key);
  assert(visible == kM1Keys);
  assert(FindEntranceSetting("erWallmasters")->supports(1, all));
  assert(!FindEntranceSetting("erWallmasters")->supports(2, all));
  assert(!FindEntranceSetting("erOneWays")->supports(2, all));
  assert(!FindEntranceSetting("erDungeons")->supports(1, all));
}

Settings M1(int wallmasters, int oneWays, int flags) {
  Settings s;
  FindEntranceSetting("erWallmasters")->set(s, wallmasters);
  FindEntranceSetting("erOneWays")->set(s, oneWays);
  const char *keys[] = {"erOneWaysMajor", "erOneWaysIkana", "erOneWaysOwls",
                        "erOneWaysWaterVoids", "erOneWaysAnywhere"};
  for (int i = 0; i < 5; ++i)
    FindEntranceSetting(keys[i])->set(s, (flags >> i) & 1);
  return s;
}

// C23: when a combination lacks its companion row, the later value in array
// order is reset (util.ts:79-130 resets a failed cond the same way).
void CompanionsAndArrayOrder() {
  const auto all = CapabilitySet::AtLeast(Stage::Hidden);
  const auto noCompanion =
      all.Only("wallmasters-own-game-v1") | all.Only("one-ways-own-game-v1");
  const auto both = M1(1, 1, 1);
  auto s = Read(both, all);
  assert(s.erWallmasters == Mode::OwnGame && s.erOneWays == Mode::OwnGame &&
         s.erOneWaysMajor);
  for (const auto &caps : {noCompanion, all.Only("wallmasters-own-game-v1")}) {
    s = Read(both, caps);
    assert(s.erWallmasters == Mode::OwnGame && s.erOneWays == Mode::None &&
           !s.erOneWaysMajor);
  }
  s = Read(both, all, "wallmasters-own-game-mm-tranche");
  assert(s.erWallmasters == Mode::OwnGame && s.erOneWays == Mode::None &&
         !s.erOneWaysMajor);
  s = Read(both, all.Only("one-ways-own-game-v1"));
  assert(s.erWallmasters == Mode::None && s.erOneWays == Mode::OwnGame &&
         s.erOneWaysMajor);
  // The companion alone admits each value and the combination.
  s = Read(both, all.Only("wallmasters-with-one-ways-v1"));
  assert(Same(s, both));
  // Full is never admitted, so OoTMM's conds then drop the one-way flags.
  s = Read(M1(2, 2, 31), all);
  assert(Same(s, Settings{}));
  // Values outside the catalog never pass, even beside admitted ones.
  auto mixed = M1(1, 0, 0);
  mixed.erDungeons = Mode::OwnGame;
  mixed.erMajorDungeons = true;
  s = Read(mixed, all);
  assert(Same(s, M1(1, 0, 0)));
}

// A devOnly row is used only when selected by id, by both readers. In a Phase A
// dev build (only the MM tranche, at Dev), a menu, profile or imported
// erWallmasters=ownGame is stripped, as headless --er refuses it without
// --er-capability: OoTMM's Wallmaster pool always has the OoT sources
// (packages/logic/src/solver/entrances.ts:657-669), so the tranche must not
// shrink it unasked.
void DevOnlyRowsOnlyWhenSelected() {
  const auto all = CapabilitySet::AtLeast(Stage::Hidden);
  auto phaseA = all.Only("wallmasters-own-game-mm-tranche");
  phaseA.devBuild = true;
  const std::string_view tranche = "wallmasters-own-game-mm-tranche";
  const auto wallmasters = M1(1, 0, 0);
  const std::vector<std::string> pair = {"erWallmasters=ownGame"};
  Settings parsed;
  assert(Same(Read(wallmasters, phaseA), Settings{}));
  assert(
      !FindEntranceSetting("erWallmasters")->supports(1, phaseA.Selected("")));
  assert(ReadEntranceArgs(pair, "", phaseA, parsed) ==
         "erWallmasters=ownGame is not available in this build without "
         "--er-capability");
  // Selected by id, both readers take it.
  assert(Same(Read(wallmasters, phaseA, tranche), wallmasters));
  assert(ReadEntranceArgs(pair, tranche, phaseA, parsed).empty() &&
         Same(parsed, wallmasters));
  // A selected row the build does not admit, or an unknown id, admits nothing.
  assert(
      Same(Read(wallmasters, phaseA, "wallmasters-own-game-v1"), Settings{}));
  assert(Same(Read(wallmasters, all, "unknown-id"), Settings{}));
  // Release rows need no selection, and selecting one uses only that row.
  assert(Same(Read(wallmasters, all.Only("wallmasters-own-game-v1")),
              wallmasters));
  assert(Same(Read(M1(1, 1, 1), all, "one-ways-own-game-v1"), M1(0, 1, 1)));
}

// Every M1 combination against several sets and selections: the result is a
// fixed point the selected rows admit, only resets values, keeps an admitted
// input whole, and resets a value only when it cannot join the values kept
// before it (array order).
void ExhaustiveM1Combinations() {
  const auto all = CapabilitySet::AtLeast(Stage::Hidden);
  const struct {
    CapabilitySet active;
    std::string_view selected;
  } sets[] = {
      {CapabilitySet::AtLeast(Stage::Public), ""},
      {ActiveCapabilities(), ""},
      {all, ""},
      {all.Only("wallmasters-own-game-v1") | all.Only("one-ways-own-game-v1"),
       ""},
      {all.Only("wallmasters-own-game-mm-tranche"), ""},
      {all, "wallmasters-own-game-mm-tranche"},
      {all.Only("one-ways-own-game-v1"), ""},
      {all.Only("wallmasters-with-one-ways-v1"), ""},
      {all, "wallmasters-with-one-ways-v1"}};
  size_t cases = 0;
  for (const auto &set : sets)
    for (int w = 0; w < 3; ++w)
      for (int o = 0; o < 3; ++o)
        for (int flags = 0; flags < 32; ++flags) {
          // The rows the reader uses.
          const auto caps = set.active.Selected(set.selected);
          const auto in = M1(w, o, flags);
          const auto out = Read(in, set.active, set.selected);
          assert(Same(Read(out, set.active, set.selected), out));
          assert(AdmitsEntranceSettings(out, caps));
          Settings normalized = in;
          ValidateEntranceSettings(normalized);
          for (const auto &row : kEntranceSettings)
            assert(row.get(out) == row.get(normalized) ||
                   row.get(out) == row.defaultValue);
          bool supported = true;
          for (const auto &row : kEntranceSettings)
            supported = supported && row.supports(row.get(normalized), caps);
          if (supported && AdmitsEntranceSettings(normalized, caps))
            assert(Same(out, normalized));
          // A value the capabilities reset could not join the kept prefix.
          Settings prefix = out;
          for (const auto &row : kEntranceSettings)
            row.set(prefix, row.defaultValue);
          for (const auto &row : kEntranceSettings) {
            if (row.get(out) != row.get(normalized)) {
              Settings tried = prefix;
              row.set(tried, row.get(normalized));
              Settings condChecked = tried;
              ValidateEntranceSettings(condChecked);
              assert(!AdmitsEntranceSettings(tried, caps) ||
                     row.get(condChecked) == row.defaultValue);
            }
            row.set(prefix, row.get(out));
          }
          ++cases;
        }
  assert(cases == std::size(sets) * 3 * 3 * 32);
  // With every value open, ReadEntranceSettings is OoTMM's validation except
  // for Full.
  for (int w = 0; w < 2; ++w)
    for (int o = 0; o < 2; ++o)
      for (int flags = 0; flags < 32; ++flags) {
        Settings normalized = M1(w, o, flags);
        ValidateEntranceSettings(normalized);
        assert(Same(Read(M1(w, o, flags), all), normalized));
      }
}
} // namespace

int main() {
  CatalogRows();
  StageSets();
  ActiveSetFollowsTheBuild();
  ShippingStripsEverything();
  SetsAdmitExactlyTheCatalog();
  CompanionsAndArrayOrder();
  DevOnlyRowsOnlyWhenSelected();
  ExhaustiveM1Combinations();
#ifdef COMBO_ENTRANCE_DEV
  std::cout
      << "Entrance capability catalog passed (COMBO_ENTRANCE_DEV build).\n";
#else
  std::cout << "Entrance capability catalog passed (shipping build).\n";
#endif
}
