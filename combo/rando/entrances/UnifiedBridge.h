// Unified OoTMM entrance generator: one Shuffler run over the pinned pools, validated by the candidate
// world assumptions, the opposite-age Temple probe and a complete install, and written as the seed's
// "entranceLayout" (EntranceLayout.h). Adapted from OoTMM v32.3 (413573d)
// packages/logic/src/solver/entrances.ts (WorldShuffler.run :905-934, LogicPassEntrances :1061-1134).
// The dev-only MM tranche retains its absent-OoT-source contract. The separately named vanilla
// capability binds both games, with actual native quest context and no MQ admission. The launcher and
// headless tool call it once per generation attempt, after the native pools and Ship's own entrance
// shuffle and before the combined fill.
// MIT, Copyright (c) 2020-2022 OoTMM Team; see LICENSE.OoTMM.
#pragma once
#include "NativeBridge.h"
#include "MmUnifiedLayout.h"
#include "OotUnifiedLayout.h"
#include "TempleNative.h"
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
};

// Ship's own entrance options: any of them turns on the native RSK_SHUFFLE_ENTRANCES
// (soh/soh/Enhancements/randomizer/settings.cpp:2905-2915). Every "off" value is option index 0.
inline constexpr const char* kShipEntranceSettings[] = {
    "gRandoSettings.ShuffleDungeonsEntrances",
    "gRandoSettings.ShuffleBossEntrances",
    "gRandoSettings.ShuffleOverworldEntrances",
    "gRandoSettings.ShuffleInteriorsEntrances",
    "gRandoSettings.ShuffleThievesHideoutEntrances",
    "gRandoSettings.ShuffleGrottosEntrances",
    "gRandoSettings.ShuffleOwlDrops",
    "gRandoSettings.ShuffleWarpSongs",
    "gRandoSettings.ShuffleOverworldSpawns",
};
// RSK_MQ_DUNGEON_RANDOM; option 0 (RO_MQ_DUNGEONS_NONE) keeps every dungeon vanilla (settings.cpp:2900-2903).
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
};

// The soh Temple probe exports, through the caller's module lookup (TempleNative.h).
template <class Lookup> TempleProbeFns ResolveUnifiedTempleProbe(Lookup&& lookup) {
    return ResolveTempleProbe(std::forward<Lookup>(lookup));
}

// The one capability id a layout requires: the selected one, or the row that admits the whole
// combination (Capabilities.h).
inline std::string UnifiedRequirement(const Settings& s, std::string_view capability,
                                      const CapabilitySet& active = ActiveCapabilities()) {
    if (!capability.empty())
        return std::string(capability);
    if (Active(s.erWallmasters) && Active(s.erOneWays))
        return "wallmasters-with-one-ways-v1";
    if (Active(s.erWallmasters) && active.Active(kVanillaWallmasterCapability))
        return std::string(kVanillaWallmasterCapability);
    return Active(s.erWallmasters) ? "wallmasters-own-game-v1" : "one-ways-own-game-v1";
}

// The old MM-only contract stays unchanged; the explicit vanilla contract adds audited OoT rows.
inline bool UnifiedSourceBound(const Record& r, bool vanillaOot = false) {
    return (r.game == Game::Mm && MmUnifiedSourceBound(r.name)) ||
           (vanillaOot && r.game == Game::Oot && OotUnifiedSourceBound(r.name));
}
inline bool UnifiedDestinationBound(const Record& r, bool vanillaOot = false) {
    return (r.game == Game::Mm && MmUnifiedDestinationBound(r.name)) ||
           (vanillaOot && r.game == Game::Oot && OotUnifiedDestinationBound(r.name));
}
// Every source of every pool, and every destination assignable from one, must be bound: a pool is
// never shrunk to the rows that are.
inline void RequireUnifiedCoverage(const Pools& pools, const Settings& s, bool vanillaOot = false) {
    for (const auto& pool : pools)
        for (const auto src : pool.src) {
            const auto& from = *Find(src);
            if (!UnifiedSourceBound(from, vanillaOot))
                throw UnifiedEntranceRefused(std::string(src) + " has no native entrance binding yet");
            for (const auto dst : pool.dst)
                if (Assignable(from, *Find(dst), pool.ownGame, s) && !UnifiedDestinationBound(*Find(dst), vanillaOot))
                    throw UnifiedEntranceRefused(std::string(dst) + " has no native entrance binding yet");
        }
}
inline OotQuests ReadUnifiedOotQuests(const NativeFns& native) {
    if (!native.ootAreaContext || !native.applyOotLayout || !native.dumpOotLayout || !native.setOotAssumptions ||
        !native.clearOotAssumptions || !native.applySeed || !native.dumpSeed || !native.setAssumptions ||
        !native.clearAssumptions || !native.setGenerationSettings)
        throw UnifiedEntranceRefused("Vanilla Wallmaster generation needs all native entrance exports");
    try {
        const auto* text = native.ootAreaContext();
        if (!text)
            throw std::invalid_argument("native OoT quest context unavailable");
        auto quests = DecodeOotAreaContext(nlohmann::json::parse(text));
        RequireVanillaOotQuests(quests);
        return quests;
    } catch (const std::exception& error) {
        throw UnifiedEntranceRefused(std::string("Vanilla Wallmaster generation: ") + error.what());
    }
}

inline bool ShipEntrancesActive(const SettingReader& read) {
    for (const char* key : kShipEntranceSettings)
        if (read(key, 0) != 0)
            return true;
    return false;
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
    f.glitchless = f.logicEnabled; // RSK_LOGIC_RULES is glitchless or no logic
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

// The layout of one generation attempt, or null when no pool is placed. In order:
// 1. normalize the settings and check them against this build's capabilities;
// 2. no pool to place: no layout, no random draw, no native call (OoTMM keeps worldChanged false and
//    validates nothing, entrances.ts:581-589, :1101-1112);
// 3. before any random draw: the private MM profiles and Ship's own entrance shuffle (owner decision
//    D1), the quest context, the coverage of every pool row, then the Temple gate and its Prepare;
// 4. the Shuffler with games=Both on the stream "entrances:ootmm-entrances-v1:<seed>".
// A ShuffleError (no layout in the pinned 1000 attempts) may be retried by another outer attempt;
// every other exception stops generation. The final check of OoTMM's validate(), the solver
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
    const auto required = UnifiedRequirement(s, g.capability, g.active);
    const bool vanillaOot = required == kVanillaWallmasterCapability;
    if (!AdmitsEntranceSettings(s, caps.Only(required)))
        throw UnifiedEntranceRefused("Entrance settings " + EntranceSettingsText(s) + " are not admitted by " +
                                     required);
    // The MM tranche counts the OoT Wallmaster areas as absent (LayoutV1::AreaExists). Quest-only
    // areas are OoT Wallmaster areas and a WALLMASTERS pool keeps its MM sources, so whether a pool
    // is placed does not depend on the quests.
    const bool tranche = required == kMmTrancheCapability;
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
        throw UnifiedEntranceRefused("Wallmaster and One-Way shuffles need Ship's own entrance shuffle off");
    if (vanillaOot && g.readSetting("gEnhancements.RandomizedEnemies", 0))
        throw UnifiedEntranceRefused("Wallmaster Shuffle needs Enemy Randomizer off");
    // The layout records the quest of every MQ dungeon; until the native quest context exists only
    // the all-vanilla one is known.
    if (!vanillaOot && g.readSetting(kMqDungeonsSetting, 0) != 0)
        throw UnifiedEntranceRefused("Entrance layouts need Master Quest dungeons off for now");
    const OotQuests quests = vanillaOot ? ReadUnifiedOotQuests(native) : OotQuests{};
    const auto areaExists = areas(quests);
    const auto pools = ResolvePools(s, areaExists).pools;
    RequireUnifiedCoverage(pools, s, vanillaOot);
    Names offered;
    for (const auto& pool : pools)
        for (const auto src : pool.src)
            for (const auto dst : pool.dst)
                if (Assignable(*Find(src), *Find(dst), pool.ownGame, s))
                    offered.insert(dst);

    // The probe needs the OoT oracle prepared, and its first reset (EnsureOracleInit) rebuilds the
    // region graph, which must not happen between Prepare and a query (TempleProbe.cpp P1, P2), and
    // finalizes the OoT settings again, which re-rolls a Random starting age. Reset it once first, as
    // the first candidate's all-items check would, and read the facts Prepare checks only after it.
    // Whether the pass runs depends only on the facts' logic flag.
    TempleFacts logic;
    logic.logicEnabled = g.access != OotAccess::NO_LOGIC;
    if (NeedsTemplePass(s, logic, areaExists)) {
        if (!g.oot.Reset)
            throw UnifiedEntranceRefused("Entrance validation requires the OoT reachability oracle");
        g.oot.Reset();
    }
    if (vanillaOot && ReadUnifiedOotQuests(native) != quests)
        throw UnifiedEntranceRefused("OoT quest context changed while preparing entrance generation");
    const auto facts = UnifiedTempleFacts(g);
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
    b.sourceSupported = [vanillaOot](const Record& r) { return UnifiedSourceBound(r, vanillaOot); };
    b.destinationSupported = [&](const Record& r) {
        return UnifiedDestinationBound(r, vanillaOot) || !offered.contains(r.name);
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
    if (vanillaOot && g.wallmasterShortcut && facts.logicEnabled && MakePools(s).size() == 1 && pools.size() == 1 &&
        pools.front().name == "WALLMASTERS" && pools.front().ownGame && pools.front().alias) {
        const auto base = ReservedEntrances(s);
        if (b.validateAssumed(base, {}) && b.validateAgeTemple(base, {})) {
            b.monotoneWallmasterBaseProven = true;
            if (g.stats)
                g.stats->monotoneShortcutUsed = true;
        }
    }
    return layoutOf(Shuffler(s, std::move(b)).run());
}
} // namespace ComboRando::Entrances
