// combo/mq/MqMode.h: the Master Quest mode resolver's whole truth table, the
// per-seed marker, the save compatibility rule and the ROM version gate.
// The legacy answer is checked against a transcription of SoH's
// IsSceneMasterQuest as it was before the resolver
// (soh/soh/ResourceManagerHelpers.cpp), so a file without the marker resolves
// exactly as before on every install.
#include "GameVersions.h"
#include "mq/MqMode.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace ComboMq;

namespace {

// soh/soh/Enhancements/enhancementTypes.h and soh/include/z64save.h.
enum {
  WARP_MODE_OVERRIDE_OFF,
  WARP_MODE_OVERRIDE_MQ_AS_VANILLA,
  WARP_MODE_OVERRIDE_VANILLA_AS_MQ
};
enum { QUEST_NORMAL, QUEST_MASTER, QUEST_RANDOMIZER, QUEST_BOSSRUSH };

// SoH's IsSceneMasterQuest before the resolver, statement for statement.
// `dungeonMq` is -1 when GetDungeonFromScene returns nullptr.
bool OriginalIsSceneMasterQuest(int mqMode, bool hasMasterQuest,
                                bool hasOriginal, int questId, int dungeonMq) {
  if (mqMode == WARP_MODE_OVERRIDE_MQ_AS_VANILLA) {
    return true;
  }
  if (mqMode == WARP_MODE_OVERRIDE_VANILLA_AS_MQ) {
    return false;
  }
  if (hasMasterQuest) {
    if (!hasOriginal) {
      return true;
    }
    if (questId == QUEST_MASTER) {
      return true;
    }
    if (questId == QUEST_RANDOMIZER) {
      if (dungeonMq != -1 && dungeonMq == 1) {
        return true;
      }
    }
  }
  return false;
}

// The call site's mapping of the raw values (ResourceManagerHelpers.cpp
// MqModeInputsForScene).
DebugWarpOverride WarpOf(int mqMode) {
  return mqMode == WARP_MODE_OVERRIDE_MQ_AS_VANILLA ? DebugWarpOverride::ForceMq
         : mqMode == WARP_MODE_OVERRIDE_VANILLA_AS_MQ
             ? DebugWarpOverride::ForceVanilla
             : DebugWarpOverride::Off;
}
QuestKind QuestOf(int questId) {
  return questId == QUEST_MASTER       ? QuestKind::MasterQuest
         : questId == QUEST_RANDOMIZER ? QuestKind::Randomizer
                                       : QuestKind::Other;
}
DungeonQuest DungeonOf(int dungeonMq) {
  return dungeonMq < 0    ? DungeonQuest::NotADungeon
         : dungeonMq == 1 ? DungeonQuest::MasterQuest
                          : DungeonQuest::Vanilla;
}

bool Same(const MqMode &a, const MqMode &b) {
  return a.gameplayMq == b.gameplayMq && a.assetsMq == b.assetsMq &&
         a.source == b.source;
}

// Every input combination, including a warp CVar outside the enum and boss
// rush. `bundledAvailable` implies `hasOriginal` (BundledMqAvailability).
void CheckExhaustive() {
  const SeedMqSource sources[] = {SeedMqSource::Legacy, SeedMqSource::Bundled,
                                  SeedMqSource::Unsupported};
  int combinations = 0;
  for (int mqMode = 0; mqMode <= 3; mqMode++)
    for (int hasOriginal = 0; hasOriginal <= 1; hasOriginal++)
      for (int hasArchive = 0; hasArchive <= 1; hasArchive++)
        for (int bundled = 0; bundled <= 1; bundled++)
          for (int questId = QUEST_NORMAL; questId <= QUEST_BOSSRUSH; questId++)
            for (int dungeonMq = -1; dungeonMq <= 1; dungeonMq++)
              for (SeedMqSource source : sources) {
                if (bundled && !hasOriginal)
                  continue;
                combinations++;
                MqModeInputs in;
                in.debugWarp = WarpOf(mqMode);
                in.hasOriginal = hasOriginal;
                in.hasArchiveMq = hasArchive;
                in.bundledAvailable = bundled;
                in.quest = QuestOf(questId);
                in.dungeon = DungeonOf(dungeonMq);
                in.seedSource = source;
                const MqMode mode = ResolveMqMode(in);
                const bool original = OriginalIsSceneMasterQuest(
                    mqMode, hasArchive, hasOriginal, questId, dungeonMq);
                const MqMode legacy = {original, original,
                                       original ? MqDataSource::Archive
                                                : MqDataSource::Vanilla};
                const bool rando = questId == QUEST_RANDOMIZER;
                const bool warp = in.debugWarp != DebugWarpOverride::Off;

                // Old seeds and saves (no marker), every non-randomizer file
                // and the debug override: exactly SoH's answer, for gameplay
                // and assets alike, on every install.
                if (source == SeedMqSource::Legacy || !rando || warp)
                  assert(Same(mode, legacy));
                if (!warp)
                  assert(LegacySceneIsMq(in) == original);

                // Without the bundled data nothing changes for a file that
                // has no marker, whatever else is installed.
                if (!bundled && source == SeedMqSource::Legacy)
                  assert(Same(mode, legacy));

                // A vanilla dungeon or any other scene resolves as before for
                // every file this install can play. A file with a vanilla
                // dungeon needs the vanilla ROM (requiresOriginal).
                if (dungeonMq != 1 && hasOriginal &&
                    CanPlayMqDungeons(source, 0, hasArchive, bundled))
                  assert(Same(mode, legacy));

                if (rando && !warp && source != SeedMqSource::Legacy) {
                  // A marked seed never reads the MQ archive's files.
                  assert(!mode.assetsMq);
                  // The gameplay answer is the seed's own DungeonInfo::IsMQ,
                  // as OoTMM's Config_IsMq: key counts (randomizer.h
                  // *_SMALL_KEY_MAX) agree with GetSmallKeyCount.
                  assert(mode.gameplayMq == (dungeonMq == 1));
                  const MqDataSource expected =
                      dungeonMq != 1 ? MqDataSource::Vanilla
                      : source == SeedMqSource::Bundled && bundled
                          ? MqDataSource::Bundled
                          : MqDataSource::Unavailable;
                  assert(mode.source == expected);
                }

                // Only the debug override or the archive ever ask for the MQ
                // files, and the bundled source never claims them.
                if (mode.assetsMq)
                  assert(mode.source == MqDataSource::Archive);
                if (mode.source == MqDataSource::Bundled)
                  assert(mode.gameplayMq && !mode.assetsMq && bundled &&
                         source == SeedMqSource::Bundled && rando);
                // Non-randomizer files never use the marker.
                if (!rando)
                  assert(mode.source != MqDataSource::Bundled &&
                         mode.source != MqDataSource::Unavailable);
              }
  // 4 warp modes * 3 (original, archive, bundled) installs * 2 archive *
  // 4 quests * 3 scenes * 3 markers.
  assert(combinations == 4 * 3 * 2 * 4 * 3 * 3);
}

struct Row {
  DebugWarpOverride warp;
  bool hasOriginal, hasArchive, bundled;
  QuestKind quest;
  DungeonQuest dungeon;
  SeedMqSource source;
  bool gameplay, assets;
  MqDataSource expected;
};

// The truth table, one row per rule. Columns: warp, vanilla ROM, MQ archive,
// bundled data, quest, the seed's dungeon, the file's marker -> gameplay,
// assets, data source.
void CheckTable() {
  using D = DebugWarpOverride;
  using Q = QuestKind;
  using G = DungeonQuest;
  using S = SeedMqSource;
  using M = MqDataSource;
  const Row rows[] = {
      // The debug warp override wins, with today's meaning.
      {D::ForceMq, true, false, true, Q::Randomizer, G::Vanilla, S::Bundled,
       true, true, M::Archive},
      {D::ForceVanilla, true, true, true, Q::Randomizer, G::MasterQuest,
       S::Bundled, false, false, M::Vanilla},
      // Android today: oot.o2r only, no marker. Never MQ.
      {D::Off, true, false, true, Q::Randomizer, G::MasterQuest, S::Legacy,
       false, false, M::Vanilla},
      {D::Off, true, false, false, Q::Randomizer, G::MasterQuest, S::Legacy,
       false, false, M::Vanilla},
      // Desktop with a real oot-mq.o2r, old seed: the archive, as today.
      {D::Off, true, true, true, Q::Randomizer, G::MasterQuest, S::Legacy, true,
       true, M::Archive},
      {D::Off, true, true, true, Q::Randomizer, G::Vanilla, S::Legacy, false,
       false, M::Vanilla},
      // MQ-only install: every scene MQ from the archive, as today.
      {D::Off, false, true, false, Q::Randomizer, G::NotADungeon, S::Legacy,
       true, true, M::Archive},
      {D::Off, false, true, false, Q::Randomizer, G::MasterQuest, S::Bundled,
       true, false, M::Unavailable},
      // A Master Quest file (unreachable in Combo, kept for parity).
      {D::Off, true, true, false, Q::MasterQuest, G::NotADungeon, S::Legacy,
       true, true, M::Archive},
      {D::Off, true, false, true, Q::MasterQuest, G::NotADungeon, S::Bundled,
       false, false, M::Vanilla},
      // New seed with bundled data: the seed decides gameplay, assets stay
      // vanilla, with or without an archive.
      {D::Off, true, false, true, Q::Randomizer, G::MasterQuest, S::Bundled,
       true, false, M::Bundled},
      {D::Off, true, true, true, Q::Randomizer, G::MasterQuest, S::Bundled,
       true, false, M::Bundled},
      {D::Off, true, true, true, Q::Randomizer, G::Vanilla, S::Bundled, false,
       false, M::Vanilla},
      {D::Off, true, false, true, Q::Randomizer, G::NotADungeon, S::Bundled,
       false, false, M::Vanilla},
      // Bundled seed on an install without the data (another ROM version):
      // refused before play; never falls back to the archive.
      {D::Off, true, true, false, Q::Randomizer, G::MasterQuest, S::Bundled,
       true, false, M::Unavailable},
      {D::Off, true, false, false, Q::Randomizer, G::Vanilla, S::Bundled, false,
       false, M::Vanilla},
      // A marker this build does not know.
      {D::Off, true, true, true, Q::Randomizer, G::MasterQuest, S::Unsupported,
       true, false, M::Unavailable},
      // A marker left over in the context never applies outside randomizer.
      {D::Off, true, false, true, Q::Other, G::MasterQuest, S::Bundled, false,
       false, M::Vanilla},
  };
  for (const Row &row : rows) {
    MqModeInputs in;
    in.debugWarp = row.warp;
    in.hasOriginal = row.hasOriginal;
    in.hasArchiveMq = row.hasArchive;
    in.bundledAvailable = row.bundled;
    in.quest = row.quest;
    in.dungeon = row.dungeon;
    in.seedSource = row.source;
    const MqMode mode = ResolveMqMode(in);
    assert(mode.gameplayMq == row.gameplay);
    assert(mode.assetsMq == row.assets);
    assert(mode.source == row.expected);
  }
}

void CheckMarker() {
  assert(SeedMqSourceFromMarker(nullptr) == SeedMqSource::Legacy);
  assert(SeedMqSourceFromMarker("ootmm-32.3") == SeedMqSource::Bundled);
  for (const char *other :
       {"", "ootmm-32.2", "OOTMM-32.3", "ootmm-32.3 ", "archive", "legacy"})
    assert(SeedMqSourceFromMarker(other) == SeedMqSource::Unsupported);
  assert(MarkerForSeedMqSource(SeedMqSource::Legacy) == nullptr);
  assert(MarkerForSeedMqSource(SeedMqSource::Unsupported) == nullptr);
  assert(SeedMqSourceFromMarker(MarkerForSeedMqSource(SeedMqSource::Bundled)) ==
         SeedMqSource::Bundled);
  assert(SeedMqSourceFromMarker(MarkerForSeedMqSource(SeedMqSource::Legacy)) ==
         SeedMqSource::Legacy);
  // New seeds use the bundled data whenever the install has it.
  assert(SourceForNewSeed(true) == SeedMqSource::Bundled);
  assert(SourceForNewSeed(false) == SeedMqSource::Legacy);
}

// FileChoose_IsSaveCompatible's MQ rule and SaveManager::LoadFile's refusal.
void CheckCanPlay() {
  for (size_t count = 0; count <= 12; count++)
    for (int archive = 0; archive <= 1; archive++)
      for (int bundled = 0; bundled <= 1; bundled++) {
        // Legacy is SoH's rule exactly: requiresMasterQuest needs
        // ResourceMgr_GameHasMasterQuest.
        const bool requiresMasterQuest = count > 0;
        assert(CanPlayMqDungeons(SeedMqSource::Legacy, count, archive,
                                 bundled) == (!requiresMasterQuest || archive));
        assert(CanPlayMqDungeons(SeedMqSource::Bundled, count, archive,
                                 bundled) == (!requiresMasterQuest || bundled));
        assert(!CanPlayMqDungeons(SeedMqSource::Unsupported, count, archive,
                                  bundled));
      }
}

void CheckVersionGate() {
  assert(kBundledMqRomVersion == OOT_NTSC_US_10);
  const uint32_t refused[] = {
      OOT_NTSC_US_11,      OOT_NTSC_US_12,  OOT_PAL_10,     OOT_PAL_11,
      OOT_NTSC_JP_GC_CE,   OOT_NTSC_JP_GC,  OOT_NTSC_US_GC, OOT_PAL_GC,
      OOT_PAL_GC_DBG1,     OOT_PAL_GC_DBG2, OOT_IQUE_TW,    OOT_IQUE_CN,
      OOT_NTSC_JP_MQ,      OOT_NTSC_US_MQ,  OOT_PAL_MQ,     OOT_PAL_GC_MQ_DBG,
      UNKNOWN_GAME_VERSION};
  const uint32_t us10[] = {OOT_NTSC_US_10};
  assert(BundledMqAvailability(true, us10, 1, true) ==
         BundledMqStatus::Available);
  assert(BundledMqAvailability(true, us10, 1, false) ==
         BundledMqStatus::DataInvalid);
  assert(BundledMqAvailability(false, us10, 1, true) ==
         BundledMqStatus::NoVanillaRom);
  assert(BundledMqAvailability(true, us10, 0, true) ==
         BundledMqStatus::NoVanillaRom);
  for (uint32_t version : refused) {
    assert(BundledMqAvailability(true, &version, 1, true) ==
           BundledMqStatus::UnsupportedVersion);
    // Every loaded vanilla archive must be the verified version.
    const uint32_t mixed[] = {OOT_NTSC_US_10, version};
    assert(BundledMqAvailability(true, mixed, 2, true) ==
           BundledMqStatus::UnsupportedVersion);
    const uint32_t reversed[] = {version, OOT_NTSC_US_10};
    assert(BundledMqAvailability(true, reversed, 2, true) ==
           BundledMqStatus::UnsupportedVersion);
  }
  const uint32_t twice[] = {OOT_NTSC_US_10, OOT_NTSC_US_10};
  assert(BundledMqAvailability(true, twice, 2, true) ==
         BundledMqStatus::Available);
}

} // namespace

int main() {
  CheckExhaustive();
  CheckTable();
  CheckMarker();
  CheckCanPlay();
  CheckVersionGate();
  std::puts("PASS: MQ mode resolver keeps every unmarked file on SoH's rule, "
            "plays marked seeds from the bundled data, and gates it to "
            "NTSC-U 1.0");
  return 0;
}
