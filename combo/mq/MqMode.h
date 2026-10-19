#pragma once
// Which Master Quest (MQ) mode a dungeon scene plays in, and where its MQ data comes from. Pure C++17 without
// game headers, so host tests check the whole truth table.
//
// SoH asks one question, "is this scene MQ?" (ResourceManagerHelpers.cpp IsSceneMasterQuest), and its answer
// does two jobs:
// - gameplay: key counts, check identification, map marks and every other caller that follows the seed;
// - assets: whether scene, room, display-list and texture paths read the "mq" files of a real oot-mq.o2r.
// OoTMM plays MQ dungeons from the vanilla ROM: the gameplay answer is the seed's choice (src/common/config.c:48-51
// Config_IsMq) and the vanilla scene files stay, with OoTMM's MQ lists patched in when the commands run
// (src/oot/mq.c:220-437). So with the bundled data (combo/mq, ComboMqData.h) the two answers split: gameplay
// follows the seed and assets stay vanilla.
//
// Only a file whose seed records the bundled source (kMqDataSourceKey) is played that way. A seed or save without
// the marker is Legacy and resolves exactly as SoH always has: MQ only with a real oot-mq.o2r.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ComboMq {

// The per-seed and per-save marker: the key, in the seed document's "oot" section, the SoH spoiler and the save's
// randomizer section, and its one known value. Absent means Legacy.
inline constexpr const char* kMqDataSourceKey = "mqDataSource";
inline constexpr const char* kMqDataSourceBundled = "ootmm-32.3";

// Where a seed's MQ dungeons get their data.
enum class SeedMqSource : uint8_t {
    Legacy = 0,      // no marker: SoH's rule, a real oot-mq.o2r or nothing
    Bundled = 1,     // OoTMM v32.3's data compiled into this build (kMqDataSourceBundled)
    Unsupported = 2, // a marker this build does not know; never played
};

// The marker text to SeedMqSource. nullptr is an absent marker.
inline SeedMqSource SeedMqSourceFromMarker(const char* marker) {
    if (marker == nullptr)
        return SeedMqSource::Legacy;
    return std::strcmp(marker, kMqDataSourceBundled) == 0 ? SeedMqSource::Bundled : SeedMqSource::Unsupported;
}
// The text written for `source`; nullptr means write no marker (Legacy). Unsupported is never written.
constexpr const char* MarkerForSeedMqSource(SeedMqSource source) {
    return source == SeedMqSource::Bundled ? kMqDataSourceBundled : nullptr;
}

// The OoT ROM versions OoTMM's data is verified against: NTSC-U 1.0 (soh/soh/GameVersions.h OOT_NTSC_US_10).
// OoTMM builds from that ROM, and its hardcoded patches name its polygons: Ganon's Castle 2403-2410 and 2473-2480
// (src/oot/mq.c:159-174) and the Fire Temple delta (scripts/make_mq.ts:307-347).
inline constexpr uint32_t kBundledMqRomVersion = 0xEC7011B7;

// Why the bundled data is or is not available on this install.
enum class BundledMqStatus : uint8_t {
    Available,
    NoVanillaRom,       // no vanilla oot.o2r: OoTMM's data patches the vanilla scenes
    UnsupportedVersion, // a vanilla ROM other than NTSC-U 1.0, or one whose version is unknown
    DataInvalid,        // the embedded bins did not decode (ComboMq::BundledError)
};
// `originalVersions` are the game versions of the loaded vanilla (non-MQ) archives. Every one must be allowed.
constexpr BundledMqStatus BundledMqAvailability(bool hasOriginal, const uint32_t* originalVersions, size_t count,
                                                bool dataDecodes) {
    if (!hasOriginal || count == 0)
        return BundledMqStatus::NoVanillaRom;
    for (size_t i = 0; i < count; i++)
        if (originalVersions[i] != kBundledMqRomVersion)
            return BundledMqStatus::UnsupportedVersion;
    return dataDecodes ? BundledMqStatus::Available : BundledMqStatus::DataInvalid;
}

// Why the Master Quest options are greyed out (settings.cpp CreateOptions and the profile editor), where MQ cannot be
// played beside the vanilla game. With the vanilla oot.o2r loaded, no oot-mq.o2r is either, and the bundled data
// plays only over a USA 1.0 ROM; without it, SoH's own reason holds: only the MQ archive is loaded.
constexpr const char* MqOptionsDisabledText(bool hasOriginal) {
    return hasOriginal ? "This option has been disabled because Master Quest dungeons need an OoT USA 1.0 ROM"
                       : "This option has been disabled because only one type of O2R has been loaded";
}

// The marker a newly generated Combo seed records: the bundled data whenever this install has it, so its MQ
// dungeons are OoTMM's, the same on every device. Without it the seed stays Legacy.
constexpr SeedMqSource SourceForNewSeed(bool bundledAvailable) {
    return bundledAvailable ? SeedMqSource::Bundled : SeedMqSource::Legacy;
}

// The stream a bundled seed rolls its MQ dungeons on (settings.cpp Context::FinalizeSettings): FNV-1a of
// "mqDungeons:ootmm-32.3:<combo master seed>", named like the launcher's other per-seed streams. OoTMM resolves
// mqDungeons once from the seed's random (packages/logic/src/world/flags.ts:45-75); a stream of their own makes every
// finalize of the seed (generation, the oracle, a reload, a headless replay) roll the same MQ dungeons, wherever the
// shared rando stream happens to be.
inline constexpr const char* kMqRollStream = "mqDungeons:ootmm-32.3:";
constexpr uint32_t MqRollSeed(uint64_t comboSeed) {
    uint32_t hash = 2166136261u;
    for (const char* c = kMqRollStream; *c != '\0'; c++)
        hash = (hash ^ static_cast<unsigned char>(*c)) * 16777619u;
    char digits[20] = {};
    int count = 0;
    do {
        digits[count++] = static_cast<char>('0' + comboSeed % 10);
        comboSeed /= 10;
    } while (comboSeed != 0);
    while (count > 0)
        hash = (hash ^ static_cast<unsigned char>(digits[--count])) * 16777619u;
    return hash;
}

// Whether this install can play a save or seed with `mqDungeonCount` MQ dungeons and this marker. Legacy is SoH's
// file-select rule (requiresMasterQuest needs a real oot-mq.o2r); a marker this build does not know never plays.
constexpr bool CanPlayMqDungeons(SeedMqSource source, size_t mqDungeonCount, bool hasArchiveMq, bool bundledAvailable) {
    switch (source) {
        case SeedMqSource::Legacy:
            return mqDungeonCount == 0 || hasArchiveMq;
        case SeedMqSource::Bundled:
            return mqDungeonCount == 0 || bundledAvailable;
        default:
            return false;
    }
}

// The debug warp screen's MQ override (CVar BetterDebugWarpScreenMQMode). SoH's enum names read swapped, but
// WARP_MODE_OVERRIDE_MQ_AS_VANILLA makes the scene MQ and WARP_MODE_OVERRIDE_VANILLA_AS_MQ makes it vanilla.
enum class DebugWarpOverride : uint8_t { Off, ForceMq, ForceVanilla };
// gSaveContext.ship.quest.id: QUEST_MASTER, QUEST_RANDOMIZER, or anything else (normal, boss rush).
enum class QuestKind : uint8_t { Other, MasterQuest, Randomizer };
// The loaded seed's DungeonInfo for the scene (Rando::Dungeons::GetDungeonFromScene): none for every scene but the
// 12 MQ-capable dungeons, exactly OoTMM's mqDungeonId (src/oot/mq.c:93-124).
enum class DungeonQuest : uint8_t { NotADungeon, Vanilla, MasterQuest };

struct MqModeInputs {
    DebugWarpOverride debugWarp = DebugWarpOverride::Off;
    bool hasOriginal = false;      // OTRGlobals::HasOriginal: a vanilla oot.o2r is loaded
    bool hasArchiveMq = false;     // OTRGlobals::HasMasterQuest: a real oot-mq.o2r is loaded
    bool bundledAvailable = false; // OTRGlobals::HasBundledMasterQuest: BundledMqStatus::Available
    QuestKind quest = QuestKind::Other;
    DungeonQuest dungeon = DungeonQuest::NotADungeon;
    SeedMqSource seedSource = SeedMqSource::Legacy; // the loaded file's marker
};

enum class MqDataSource : uint8_t {
    Vanilla,     // not MQ
    Archive,     // a real oot-mq.o2r's scenes/mq files
    Bundled,     // the vanilla files with OoTMM's MQ lists (combo/mq)
    Unavailable, // the seed's data source is missing on this install; such a file is refused before it plays
};

struct MqMode {
    bool gameplayMq; // ResourceMgr_IsSceneMasterQuest: the dungeon plays as MQ
    bool assetsMq;   // ResourceMgr_SceneUsesMqAssets: read the scenes/mq files of oot-mq.o2r
    MqDataSource source;
};

// SoH's IsSceneMasterQuest after the debug override, unchanged (ResourceManagerHelpers.cpp, before this resolver).
constexpr bool LegacySceneIsMq(const MqModeInputs& in) {
    if (!in.hasArchiveMq)
        return false;
    if (!in.hasOriginal || in.quest == QuestKind::MasterQuest)
        return true;
    return in.quest == QuestKind::Randomizer && in.dungeon == DungeonQuest::MasterQuest;
}

constexpr MqMode ResolveMqMode(const MqModeInputs& in) {
    // The debug override keeps its meaning: the archive, for both answers.
    if (in.debugWarp == DebugWarpOverride::ForceMq)
        return { true, true, MqDataSource::Archive };
    if (in.debugWarp == DebugWarpOverride::ForceVanilla)
        return { false, false, MqDataSource::Vanilla };
    // A randomizer file whose seed records a source: the seed decides gameplay (Config_IsMq), assets stay vanilla.
    if (in.quest == QuestKind::Randomizer && in.seedSource != SeedMqSource::Legacy) {
        const bool mq = in.dungeon == DungeonQuest::MasterQuest;
        if (!mq)
            return { false, false, MqDataSource::Vanilla };
        const bool usable = in.seedSource == SeedMqSource::Bundled && in.bundledAvailable;
        return { true, false, usable ? MqDataSource::Bundled : MqDataSource::Unavailable };
    }
    const bool legacy = LegacySceneIsMq(in);
    return { legacy, legacy, legacy ? MqDataSource::Archive : MqDataSource::Vanilla };
}

} // namespace ComboMq
