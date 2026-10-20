// Unified OoTMM entrance generator: one Shuffler run over the pinned pools, validated by the candidate
// world assumptions, the opposite-age Temple probe and a complete install, and written as the seed's
// "entranceLayout" (EntranceLayout.h). Adapted from OoTMM v32.3 (413573d)
// packages/logic/src/solver/entrances.ts (WorldShuffler.run :905-934, LogicPassEntrances :1061-1134).
// The dev-only MM tranche retains its absent-OoT-source contract. The separately named vanilla
// capability binds both games, with actual native quest context and no MQ admission; the quests
// capability binds the same rules where a Master Quest dungeon of OoTMM's bundled data is played. The
// launcher and headless tool call it once per generation attempt, after the native pools and Ship's own
// entrance shuffle and before the combined fill.
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "NativeBridge.h"
#include "EntranceProfile.h"
#include "MmUnifiedLayout.h"
#include "OotUnifiedLayout.h"
#include "TempleNative.h"
#include "mq/MqMode.h"
#include "rando/GenerationRules.h"
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ComboRando::Entrances {
// A refusal before any random draw: this build, these settings or the seed's other options cannot
// generate the layout. Not a ShuffleError, so neither the Shuffler nor an outer generation attempt
// retries it (M1 plan, critic correction C1).
class UnifiedEntranceRefused : public std::logic_error {
  public:
    using std::logic_error::logic_error;
    // A settings or dungeon data reason: what() keeps the developer text for logs (EntranceAvailability.h).
    explicit UnifiedEntranceRefused(const EntranceRefusal& why) : std::logic_error(why.log), sentence(why.player) {
    }
    // The generation error the player sees: the reason's plain sentence (owner decision D3), or the developer text
    // of an internal fault, which has none.
    std::string player() const {
        return sentence.empty() ? what() : sentence;
    }

  private:
    std::string sentence;
};
// The refusal of a Master Quest context with more OoT Wallmasters than dungeon entrances (RequireWallmasterCapacity).
// It depends on the Master Quest dungeons the attempt rolled, so its caller can tell the seed's own roll from a
// retry's (WallmasterShortfallStopsGeneration).
class WallmasterCapacityShortfall : public UnifiedEntranceRefused {
  public:
    using UnifiedEntranceRefused::UnifiedEntranceRefused;
};
// OoTMM resolves a seed's Master Quest dungeons once (packages/logic/src/world/flags.ts:45-75) and fails the seed when
// they leave more Wallmasters than entrances. The port's first generation attempt (0) is the seed's own roll: its
// shortfall stops generation, as OoTMM's seed fails. Each later attempt rerolls the dungeons from its own master seed
// (ComboShip.cpp RunComboFill, ComboRandoHeadless.cpp), so its shortfall fails only that attempt, as a ShuffleError
// does, and generation goes on.
constexpr bool WallmasterShortfallStopsGeneration(uint64_t attempt) {
    return attempt == 0;
}

// RSK_MQ_DUNGEON_RANDOM; option 0 (RO_MQ_DUNGEONS_NONE) keeps every dungeon vanilla (settings.cpp:2919-2922).
inline constexpr const char* kMqDungeonsSetting = "gRandoSettings.MQDungeons";

using SettingReader = std::function<int(const char*, int)>;
struct UnifiedEntranceStats {
    size_t randomDraws = 0, assumedChecks = 0, completeChecks = 0;
    bool monotoneShortcutUsed = false;
};

// One generation attempt.
struct UnifiedGeneration {
    Settings settings;              // the er* settings (ReadEntranceSettings, or headless --er)
    std::string capability;         // the selected capability id (headless --er-capability), or empty
    CapabilitySet active;           // this build's ActiveCapabilities()
    uint32_t seed = 0;              // the attempt's masterSeed
    SettingReader readSetting;      // Ship's native settings; read only when a pool is placed
    bool privateMmProfiles = false; // a private MM entrance generator or saved envelope is in use
    std::string ootDump, mmDump, forcedOot;
    OracleFns oot{}, mm{};
    bool mmStart = false;
    CwGoal goal;
    OotAccess access = OotAccess::ALL_REACHABLE;
    uint32_t sharedMask = 0;
    bool songTravelLogic = false, foreignSongsLogic = false; // the installed SongTravelMap's logic flags
    bool wallmasterShortcut = true; // developer differential seam; never serialized as a seed setting
    UnifiedEntranceStats* stats = nullptr;
    // Where the seed's Master Quest dungeons come from (MqMode.h SourceForNewSeed, or a restored seed's marker). Only
    // Bundled admits a Master Quest context (AdmitsMasterQuestContext).
    ComboMq::SeedMqSource mqSource = ComboMq::SeedMqSource::Legacy;
};

// The soh Temple probe exports, through the caller's module lookup (TempleNative.h).
template <class Lookup> TempleProbeFns ResolveUnifiedTempleProbe(Lookup&& lookup) {
    return ResolveTempleProbe(std::forward<Lookup>(lookup));
}

// The one capability id a layout requires: the selected one, or the row that admits the whole
// combination (Capabilities.h). Own Game Wallmasters take the vanilla id here; UnifiedQuestRequirement
// moves a Master Quest context to the quests id once the native quest context is read.
inline std::string UnifiedRequirement(const Settings& s, std::string_view capability,
                                      const CapabilitySet& active = ActiveCapabilities()) {
    if (!capability.empty())
        return std::string(capability);
    if (Active(s.erWallmasters) && Active(s.erOneWays))
        return "wallmasters-with-one-ways-v1";
    if (Active(s.erWallmasters) && active.Active(kVanillaWallmasterCapability))
        return std::string(kVanillaWallmasterCapability);
    if (Active(s.erWallmasters) && active.Active(kQuestsWallmasterCapability))
        return std::string(kQuestsWallmasterCapability);
    return Active(s.erWallmasters) ? "wallmasters-own-game-v1" : "one-ways-own-game-v1";
}
// The OoT Wallmaster ids bind the audited OoT rows; every other id keeps its MM-only contract.
inline bool BindsOotWallmasters(std::string_view required) {
    return required == kVanillaWallmasterCapability || required == kQuestsWallmasterCapability;
}
// Whether the generation may read a Master Quest context: the quests id, or the vanilla id chosen by
// settings (not selected by name) in a build that has the quests id too, and in either case only for
// OoTMM's bundled data. A bundled seed rolls its Master Quest dungeons on a stream of their own from its
// master seed (settings.cpp, MqMode.h MqRollSeed), so every finalize of an attempt rolls the same ones, as
// OoTMM resolves them once per seed. Legacy data (a real oot-mq.o2r) rolls them on the shared stream,
// which the oracle's first reset can leave elsewhere, so it keeps the vanilla dungeons only.
inline bool AdmitsMasterQuestContext(std::string_view required, std::string_view capability,
                                     const CapabilitySet& active, ComboMq::SeedMqSource source) {
    if (source != ComboMq::SeedMqSource::Bundled)
        return false;
    return required == kQuestsWallmasterCapability || (capability.empty() && required == kVanillaWallmasterCapability &&
                                                       active.Active(kQuestsWallmasterCapability));
}
// The id for the native quest context: a Master Quest dungeon moves the vanilla id to the quests id, so an
// all-vanilla seed keeps the frozen vanilla id and the layout 0.4.12 makes. A selected id is kept.
inline std::string UnifiedQuestRequirement(const std::string& required, const OotQuests& quests,
                                           std::string_view capability, const CapabilitySet& active,
                                           ComboMq::SeedMqSource source) {
    if (AnyMasterQuest(quests) && AdmitsMasterQuestContext(required, capability, active, source))
        return std::string(kQuestsWallmasterCapability);
    return required;
}

// The old MM-only contract stays unchanged; the OoT Wallmaster contracts add audited OoT rows.
inline bool UnifiedSourceBound(const Record& r, bool ootBound = false) {
    return (r.game == Game::Mm && MmUnifiedSourceBound(r.name)) ||
           (ootBound && r.game == Game::Oot && OotUnifiedSourceBound(r.name));
}
inline bool UnifiedDestinationBound(const Record& r, bool ootBound = false) {
    return (r.game == Game::Mm && MmUnifiedDestinationBound(r.name)) ||
           (ootBound && r.game == Game::Oot && OotUnifiedDestinationBound(r.name));
}
// Every source of every pool, and every destination assignable from one, must be bound: a pool is
// never shrunk to the rows that are.
inline void RequireUnifiedCoverage(const Pools& pools, const Settings& s, bool ootBound = false) {
    for (const auto& pool : pools)
        for (const auto src : pool.src) {
            const auto& from = *Find(src);
            if (!UnifiedSourceBound(from, ootBound))
                throw UnifiedEntranceRefused(std::string(src) + " has no native entrance binding yet");
            for (const auto dst : pool.dst)
                if (Assignable(from, *Find(dst), pool.ownGame, s) && !UnifiedDestinationBound(*Find(dst), ootBound))
                    throw UnifiedEntranceRefused(std::string(dst) + " has no native entrance binding yet");
        }
}
// Each source of a pool takes a destination of its own: OoTMM's placePoolsRecursive removes the destination it
// routes (packages/logic/src/solver/entrances.ts:529-536), alias pool or not. A game with more sources than the
// destinations they can take can never be placed, and OoTMM then fails the seed: "Too many backtracks" (:556-560) in
// each of LogicPassEntrances.run's 1000 attempts (:1119-1134), on the Master Quest dungeons it resolved once from the
// seed (world/flags.ts:45-75, builder.ts:173-175, pipeline.ts:63-87), which nothing re-rolls (generator.ts:108).
// Refused here, before any random draw. Only Master Quest Spirit Temple and Bottom of the Well with a vanilla Shadow
// Temple and Ganon's Castle do it: 12 OoT Wallmasters for 11 dungeon entrances (tools/ootmm/m1-contract.json
// mqSources.capacityShortfalls). That refusal is a WallmasterCapacityShortfall, with the sentence for settings whose
// every roll is such a context (`forced`, EntranceProfile.h MqWallmasterShortfallForced) or for a random roll that
// landed on one; any other pool's shortfall would not depend on the roll and stops generation.
inline void RequireWallmasterCapacity(const Pools& pools, const Settings& s, bool forced = false) {
    for (const auto& pool : pools)
        for (const Game game : { Game::Oot, Game::Mm }) {
            size_t sources = 0;
            Names destinations;
            for (const auto src : pool.src) {
                const auto& from = *Find(src);
                if (pool.ownGame && from.game != game)
                    continue;
                ++sources;
                for (const auto dst : pool.dst)
                    if (Assignable(from, *Find(dst), pool.ownGame, s))
                        destinations.insert(dst);
            }
            if (sources <= destinations.size())
                continue;
            const auto log = pool.name + " has " + std::to_string(sources) + " sources for " +
                             std::to_string(destinations.size()) + " destinations; OoTMM cannot place them";
            if (game == Game::Oot && pool.name == "WALLMASTERS")
                throw WallmasterCapacityShortfall(forced ? WallmasterCapacityRefusal(log)
                                                         : WallmasterSeedCapacityRefusal(log));
            throw UnifiedEntranceRefused(log);
        }
}
inline void RequireOotWallmasterExports(const NativeFns& native) {
    if (!native.ootAreaContext || !native.applyOotLayout || !native.dumpOotLayout || !native.setOotAssumptions ||
        !native.clearOotAssumptions || !native.applySeed || !native.dumpSeed || !native.setAssumptions ||
        !native.clearAssumptions || !native.setGenerationSettings)
        throw UnifiedEntranceRefused("OoT Wallmaster generation needs all native entrance exports");
}
// The native quest context, read from the dump's finalize and again after the oracle's first reset. A Master Quest
// dungeon is refused unless `admitMq` (AdmitsMasterQuestContext).
inline OotQuests ReadUnifiedOotQuests(const NativeFns& native, bool admitMq = false) {
    RequireOotWallmasterExports(native);
    bool decoded = false;
    try {
        const auto* text = native.ootAreaContext();
        if (!text)
            throw std::invalid_argument("native OoT quest context unavailable");
        auto quests = DecodeOotAreaContext(nlohmann::json::parse(text));
        decoded = true;
        if (!admitMq)
            RequireVanillaOotQuests(quests);
        return quests;
    } catch (const std::exception& error) {
        throw UnifiedEntranceRefused(EntranceRefusal{ decoded ? kWallmasterMasterQuestText : kWallmasterDungeonDataText,
                                                      std::string("OoT Wallmaster generation: ") + error.what() });
    }
}

// The seed facts the Temple pass needs, read after the native pools were prepared and the OoT oracle's
// first reset, which can resolve a Random starting age again.
inline TempleFacts UnifiedTempleFacts(const UnifiedGeneration& g) {
    TempleFacts f;
    const uint8_t ages = g.oot.GetStartAges ? g.oot.GetStartAges() : 0; // adult 1, child 2
    f.startingAge = ages == 1   ? TempleStartingAge::Adult
                    : ages == 2 ? TempleStartingAge::Child
                                : TempleStartingAge::Random;
    f.logicEnabled = g.access != OotAccess::NO_LOGIC;
    f.glitchless = g.readSetting && g.readSetting("gRandoSettings.LogicRules", 0) == 0;
    f.shipwrightEntrances = ShipEntrancesActive(g.readSetting);
    f.songTravelLogic = g.songTravelLogic;
    f.foreignSongsLogic = g.foreignSongsLogic;
    f.dungeonBeaten = true; // OoTMM's default regionState (packages/core/src/settings/data.ts:1390-1401)
    const auto starting = [](const std::string& dump) {
        return nlohmann::json::parse(dump).value("sharedStartingItems", std::vector<std::string>{});
    };
    f.ootStartingItems = starting(g.ootDump);
    f.mmStartingItems = starting(g.mmDump);
    f.foreignStartingItems = NormalizeForeignStartingItems(g.sharedMask, g.ootDump, g.mmDump);
    return f;
}

// Installs a generated layout for the fill, the hints, the playthrough and the spoiler of the attempt
// that made it (C13). The next attempt's clear, or the generation's own NativeScope, removes it.
inline void ApplyUnifiedLayout(const NativeFns& native, const nlohmann::json& layout, const CapabilitySet& caps) {
    ApplyNativeLayout(native, nlohmann::json{ { "entranceLayout", layout } }, caps);
}

// The seed identity (displaySeed) of a layout seed also covers its layout, so two players whose
// worlds differ only in their entrances never share file icons or a co-op sync seed (C5). Any other
// seed hashes the same text as before.
inline std::string SeedIdentityText(std::string text, const nlohmann::json& layout) {
    if (!layout.is_null())
        text += layout.dump();
    return text;
}
// The text a generated seed's displaySeed hashes, in the app (RunComboFill) and the headless tool alike: the input
// seed, both settings dumps and only the rule and song-map parts that shape logic, so per-run enhancement values
// (e.g. Shorter Ballad) never change the icons or the Anchor sync seed of one seed; then a solo run's mode, and the
// layout. rules is SnapshotGameplayRules' block and songs the SongTravelMap's json().
inline std::string GeneratedSeedIdentityText(const std::string& inputSeed, const std::string& ootDump,
                                             const std::string& mmDump, const nlohmann::json& rules,
                                             const nlohmann::json& songs, const std::string& mode,
                                             const nlohmann::json& layout) {
    const nlohmann::json logicRules = { { "logicValues", rules.at("logicValues") },
                                        { "mmTricks", rules.at("mmTricks") },
                                        { "ootTricks", rules.at("ootTricks") } };
    const nlohmann::json logicSongs = { { "logicEnabled", songs.at("logicEnabled") },
                                        { "logicForeignSongs", songs.at("logicForeignSongs") },
                                        { "destinations", songs.at("destinations") } };
    return SeedIdentityText(inputSeed + ootDump + mmDump + logicRules.dump() + logicSongs.dump() + mode, layout);
}

// The layout of one generation attempt, or null when no pool is placed. In order:
// 1. normalize the settings and check them against this build's capabilities;
// 2. no pool to place: no layout, no random draw, no native call (OoTMM keeps worldChanged false and
//    validates nothing, entrances.ts:581-589, :1101-1112);
// 3. before any random draw: the private MM profiles and Ship's own entrance shuffle (owner decision
//    D1), the quest context and the capability it needs, the coverage of every pool row, the capacity
//    of every pool, the OoT oracle's first reset and the same quest context after it, then the Temple
//    gate and its Prepare;
// 4. the Shuffler with games=Both on the stream "entrances:ootmm-entrances-v1:<seed>".
// A ShuffleError (no layout in the pinned 1000 attempts) may be retried by another outer attempt, and
// so may a WallmasterCapacityShortfall after the first (WallmasterShortfallStopsGeneration); every
// other exception stops generation. The final check of OoTMM's validate(), the solver
// (logicPassSolverValidate, :1083), is the combined fill's own: it places the items on the installed
// layout under the same accessibility policy and fails the attempt if that cannot be done.
inline nlohmann::json GenerateEntranceLayout(const UnifiedGeneration& g, const NativeFns& native,
                                             const TempleProbeFns& temple) {
    Settings s = g.settings;
    ValidateEntranceSettings(s);
    if (!g.capability.empty() && !CapabilitySet::Known(g.capability))
        throw UnifiedEntranceRefused("Unknown entrance capability " + g.capability);
    const auto caps = g.active.Selected(g.capability);
    if (!AdmitsEntranceSettings(s, caps))
        throw UnifiedEntranceRefused("Entrance settings " + EntranceSettingsText(s) +
                                     " are not available in this build");
    if (MakePools(s).empty())
        return nlohmann::json();
    const auto settingsRequired = UnifiedRequirement(s, g.capability, g.active);
    const bool ootBound = BindsOotWallmasters(settingsRequired);
    if (!AdmitsEntranceSettings(s, caps.Only(settingsRequired)))
        throw UnifiedEntranceRefused("Entrance settings " + EntranceSettingsText(s) + " are not admitted by " +
                                     settingsRequired);
    // The MM tranche counts the OoT Wallmaster areas as absent (LayoutV1::AreaExists). Quest-only
    // areas are OoT Wallmaster areas and a WALLMASTERS pool keeps its MM sources, so whether a pool
    // is placed does not depend on the quests.
    const bool tranche = settingsRequired == kMmTrancheCapability;
    const auto areas = [tranche](const OotQuests& quests) -> AreaExists {
        return [tranche, quests](std::string_view area) { return LayoutV1::AreaExists(area, quests, tranche); };
    };
    if (ResolvePools(s, areas(OotQuests{})).pools.empty())
        return nlohmann::json();

    if (g.privateMmProfiles)
        throw UnifiedEntranceRefused("Wallmaster and One-Way shuffles cannot be combined with a private MM entrance "
                                     "profile");
    if (!g.readSetting)
        throw UnifiedEntranceRefused("Entrance generation cannot read Ship's settings");
    if (ShipEntrancesActive(g.readSetting))
        throw UnifiedEntranceRefused(ShipEntrancesRefusal());
    if (ootBound && g.readSetting("gEnhancements.RandomizedEnemies", 0))
        throw UnifiedEntranceRefused(EnemyRandomizerRefusal());
    // The layout records the quest of every MQ dungeon, and only the OoT Wallmaster capabilities read
    // the native quest context. Every other capability needs the MQ setting off. MQ dungeons can be
    // played (a real oot-mq.o2r or OoTMM's bundled data, combo/mq), so a profile that selects them,
    // Allsanity's for one, is refused here with One-Ways or MM Wallmasters, before any random draw.
    if (!ootBound && g.readSetting(kMqDungeonsSetting, 0) != 0)
        throw UnifiedEntranceRefused(
            EntranceRefusal{ kWallmasterMasterQuestText, "Entrance layouts need Master Quest dungeons off for now" });

    // The quest context of the dump's finalize, and the id it needs. Only a bundled seed's Master Quest
    // dungeons are admitted (AdmitsMasterQuestContext).
    const bool admitMq = ootBound && AdmitsMasterQuestContext(settingsRequired, g.capability, g.active, g.mqSource);
    const OotQuests quests = ootBound ? ReadUnifiedOotQuests(native, admitMq) : OotQuests{};
    const auto required = UnifiedQuestRequirement(settingsRequired, quests, g.capability, g.active, g.mqSource);
    if (required == kQuestsWallmasterCapability && !AnyMasterQuest(quests))
        throw UnifiedEntranceRefused(std::string(kQuestsWallmasterCapability) + " needs a Master Quest dungeon");
    const auto areaExists = areas(quests);
    const auto pools = ResolvePools(s, areaExists).pools;
    RequireUnifiedCoverage(pools, s, ootBound);
    RequireWallmasterCapacity(pools, s, MqWallmasterShortfallForced(g.readSetting, false));
    Names offered;
    for (const auto& pool : pools)
        for (const auto src : pool.src)
            for (const auto dst : pool.dst)
                if (Assignable(*Find(src), *Find(dst), pool.ownGame, s))
                    offered.insert(dst);

    // The probe needs the OoT oracle prepared, and its first reset (EnsureOracleInit) rebuilds the
    // region graph, which must not happen between Prepare and a query (TempleProbe.cpp P1, P2), and
    // finalizes the OoT settings again, which re-rolls a Random starting age and the Master Quest
    // dungeons. Reset it once first, as the first candidate's all-items check would, and read the facts
    // Prepare checks only after it. The quest context after it must be the one read above, for either
    // id: the oracle, the fill and the seed play that finalize's dungeons, and the layout records the
    // dump's. A bundled seed's roll is the same in both finalizes, so this holds for every bundled seed;
    // a refusal here is an internal fault, without a plain sentence.
    TempleFacts logic;
    logic.logicEnabled = g.access != OotAccess::NO_LOGIC;
    if (NeedsTemplePass(s, logic, areaExists)) {
        if (!g.oot.Reset)
            throw UnifiedEntranceRefused("Entrance validation requires the OoT reachability oracle");
        g.oot.Reset();
    }
    if (ootBound && ReadUnifiedOotQuests(native, admitMq) != quests)
        throw UnifiedEntranceRefused("OoT quest context changed while preparing entrance generation");
    const auto facts = UnifiedTempleFacts(g);
    if (ootBound) {
        auto settingsFacts = static_cast<const EntranceSettingsFacts&>(facts);
        settingsFacts.enemyRandomizer = g.readSetting("gEnhancements.RandomizedEnemies", 0) != 0;
        if (const auto why = ComboRando::EntranceGenerationRefusal(settingsFacts); !why.empty())
            throw UnifiedEntranceRefused(why);
    }
    TempleProbeRun templeRun(temple, s, facts, areaExists);
    // Everything of the layout but its routes.
    EntranceLayout envelope;
    envelope.required = { required };
    envelope.devBuild = g.active.devBuild;
    envelope.settings = s;
    envelope.ootMq = quests;
    NativeScope restore(native);
    // The MM logic's er_enabled follows these settings while the candidates are checked
    // (macros_mm.yml:273, :305), as it follows the installed layout's afterwards.
    GenerationSettingsScope erSettings(native, EncodeEntranceLayout(envelope).dump());
    AssumedWorld world(g.ootDump, g.mmDump, g.forcedOot);
    const auto layoutOf = [&](const Overrides& rows) {
        EntranceLayout layout = envelope;
        Overrides routes;
        for (const auto& [src, dst] : rows)
            if (!IsReservedGameLink(src, dst))
                routes.emplace(src, dst);
        layout.overrides = std::move(routes);
        Validate(layout, g.active);
        return EncodeEntranceLayout(layout);
    };
    CwRng rng(CwHashName("entrances:ootmm-entrances-v1:" + std::to_string(g.seed)));
    Bindings b;
    b.logicEnabled = facts.logicEnabled;
    b.areaExists = areaExists;
    b.sourceSupported = [ootBound](const Record& r) { return UnifiedSourceBound(r, ootBound); };
    b.destinationSupported = [&](const Record& r) {
        return UnifiedDestinationBound(r, ootBound) || !offered.contains(r.name);
    };
    b.chooseIndex = [&](size_t count) {
        CheckGenerationCancellation();
        if (g.stats && count)
            ++g.stats->randomDraws; // CwRng::below consumes exactly one next() for nonzero count.
        return rng.below(static_cast<uint32_t>(count));
    };
    // One candidate world for both games' oracles and the Temple probe (CandidateWorld.h).
    b.validateAssumed = [&](const Overrides& rows, const Pools& remaining) {
        if (g.stats)
            ++g.stats->assumedChecks;
        const auto candidate = BuildCandidateWorld(rows, remaining, s);
        AssumptionScope scope(native, EncodeMmOverlay(candidate), EncodeOotOverlay(candidate));
        return world.validate(g.oot, g.mm, g.mmStart, g.goal, g.access);
    };
    b.validateAgeTemple = templeRun.Validator();
    b.validateComplete = [&](const Overrides& rows) {
        if (g.stats)
            ++g.stats->completeChecks;
        ApplyUnifiedLayout(native, layoutOf(rows), g.active);
        return world.validate(g.oot, g.mm, g.mmStart, g.goal, g.access);
    };
    // Removing a synthetic VOID Wallmaster source cannot remove native reachability. Certify the
    // base once with both inventories, then retain the pinned sampling stream and final validation.
    // Old contracts and every combination with another declared pool keep the original path.
    if (ootBound && g.wallmasterShortcut && facts.logicEnabled && MakePools(s).size() == 1 && pools.size() == 1 &&
        pools.front().name == "WALLMASTERS" && pools.front().ownGame && pools.front().alias) {
        const auto base = ReservedEntrances(s);
        auto basePools = pools;
        for (auto& pool : basePools)
            pool.dst.clear(); // Delete every source, with no fixed or assumed Wallmaster edge.
        if (b.validateAssumed(base, basePools) && b.validateAgeTemple(base, basePools)) {
            b.monotoneWallmasterBaseProven = true;
            if (g.stats)
                g.stats->monotoneShortcutUsed = true;
        }
    }
    return layoutOf(Shuffler(s, std::move(b)).run());
}
} // namespace ComboRando::Entrances
