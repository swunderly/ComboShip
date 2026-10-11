#pragma once
#include "rando/entrances/LegacyEnvelope.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/MmMixedGraph.h"
#include "rando/entrances/MmOrdinaryAreas.h"
#include "rando/entrances/MmOrdinaryGenerationSettings.h"
namespace ComboRando::Entrances {
inline Settings MmMixedGenerationSettings(Settings ordinary, bool grottos) {
    ordinary.erMixed = Mode::OwnGame;
    ordinary.erMixedIndoors = Active(ordinary.erIndoors);
    ordinary.erMixedOverworld = Active(ordinary.erOverworld);
    ordinary.erMixedDungeons = Active(ordinary.erDungeons);
    if (grottos) {
        ordinary.erGrottos = Mode::OwnGame;
        ordinary.erMixedGrottos = true;
    }
    return ordinary;
}
inline bool MmMixedAreaExists(std::string_view area) {
    return MmOrdinaryNativeAreaExists(area) || MmGrottoNativeAreaExists(area);
}
inline void ValidateMmMixedGenerationCoverage(const Settings& settings) {
    if (settings.games != Games::Mm || settings.erDecoupled || settings.erMixed != Mode::OwnGame ||
        settings.erGrottos != Mode::OwnGame || !settings.erMixedGrottos ||
        (!settings.erMixedIndoors && !settings.erMixedOverworld && !settings.erMixedDungeons) ||
        settings.erBoss != Mode::None || settings.erWallmasters != Mode::None || settings.erOneWays != Mode::None ||
        settings.erWarps != Warps::None || settings.erSpawns != Spawns::None)
        throw std::invalid_argument("MM mixed generator requires coupled MM-only ordinary and grotto pools");
    Types selected;
    for (const auto& pool : MakePools(settings)) {
        selected.insert(pool.src.begin(), pool.src.end());
        selected.insert(pool.dst.begin(), pool.dst.end());
    }
    auto require = [](const Record& r) {
        if (!MmMixedBound(&r) || (r.from != "NONE" && !MmMixedAreaExists(r.from)) ||
            (r.to != "NONE" && !MmMixedAreaExists(r.to)))
            throw std::invalid_argument("Requested MM mixed pool has no complete native binding: " +
                                        std::string(r.name));
    };
    for (const auto& record : kRegistry)
        if (record.game == Game::Mm && selected.contains(record.type)) {
            // Pin computeAllEntrances excludes JP interiors absent from the selected US
            // world. These four pairs are conditional on actual area/context availability;
            // no Default-US record is removed to make a requested pool pass admission.
            const bool conditionalJp =
                record.name == "MM_GROTTO_JP_CLIMB_LEFT" || record.name == "MM_GROTTO_JP_CLIMB_RIGHT" ||
                record.name == "MM_GROTTO_JP_LINE_START" || record.name == "MM_GROTTO_JP_LINE_END";
            if (conditionalJp && !MmMixedAreaExists(record.to))
                continue;
            require(record);
            if (const auto* reverse = Reverse(record))
                require(*reverse);
        }
}
inline nlohmann::json EncodeMmMixedExtension(const Overrides& rows, const Settings& settings) {
    nlohmann::json value = { { "schemaVersion", 1 },
                             { "profile", "mm-mixed-coupled-v1" },
                             { "upstreamCommit", kLegacyUpstreamCommit },
                             { "registrySha256", kLegacyRegistrySourceSha256 },
                             { "noPolarity", settings.erNoPolarity },
                             { "selfLoops", settings.erSelfLoops },
                             { "overworld", Active(settings.erOverworld) },
                             { "overrides", nlohmann::json::array() } };
    for (const auto& [src, dst] : rows) {
        if (IsMmGrottoClockTowerReservation(src, dst))
            continue;
        value["overrides"].push_back({ { "source", src }, { "destination", dst } });
    }
    MmMixedLayout proof;
    proof.install(value);
    return value;
}
inline void ApplyNativeMmMixed(const NativeFns& native, const nlohmann::json& extension) {
    MmMixedLayout proof;
    proof.install(extension);
    if (!native.applySeed || !native.dumpSeed)
        throw std::runtime_error("MM mixed seed exports unavailable");
    const auto* dump = native.dumpSeed();
    if (!dump)
        throw std::runtime_error("MM mixed seed snapshot unavailable");
    auto current = nlohmann::json::parse(dump);
    current.erase("ordinaryEntranceExpansion");
    current.erase("grottoEntranceExpansion");
    current["mixedEntranceExpansion"] = extension;
    ApplyNativeSeed(native, current);
}
class MmMixedAssumptionScope {
    NativeFns native;

  public:
    MmMixedAssumptionScope(NativeFns f, const Overrides& rows, const Pools& pools, const Settings& settings)
        : native(f) {
        if (!native.setAssumptions || !native.clearAssumptions)
            throw std::runtime_error("MM mixed assumption exports unavailable");
        if (!native.setAssumptions(EncodeMmMixedAssumptions(rows, pools, settings).dump().c_str()))
            throw std::runtime_error("MM mixed assumptions rejected");
    }
    ~MmMixedAssumptionScope() {
        native.clearAssumptions();
    }
    MmMixedAssumptionScope(const MmMixedAssumptionScope&) = delete;
};
inline nlohmann::json GenerateNativeMmMixed(Settings settings, uint32_t seed, const NativeFns& native,
                                            const std::string& ootDump, const std::string& mmDump,
                                            const std::string& forcedOot, const OracleFns& oot, const OracleFns& mm,
                                            bool mmStart, CwGoal goal, OotAccess access = OotAccess::ALL_REACHABLE) {
    ValidateMmMixedGenerationCoverage(settings);
    NativeScope restore(native);
    AssumedWorld world(ootDump, mmDump, forcedOot);
    CwRng rng(CwHashName("entrances:mm-mixed-coupled-v1:" + std::to_string(seed)));
    Bindings bindings;
    bindings.logicEnabled = access != OotAccess::NO_LOGIC;
    bindings.areaExists = MmMixedAreaExists;
    bindings.sourceSupported = [](const Record& r) { return MmMixedBound(&r); };
    bindings.destinationSupported = bindings.sourceSupported;
    bindings.chooseIndex = [&](size_t count) {
        CheckGenerationCancellation();
        return rng.below(static_cast<uint32_t>(count));
    };
    bindings.validateAssumed = [&](const Overrides& rows, const Pools& pools) {
        const auto graph = BuildMmMixedAssumptions(rows, pools, settings);
        MmMixedAssumptionScope scope(native, rows, pools, settings);
        return world.validate(oot, mm, mmStart || !graph.syntheticDestinations.empty(), goal, access);
    };
    bindings.validateComplete = [&](const Overrides& rows) {
        ApplyNativeMmMixed(native, EncodeMmMixedExtension(rows, settings));
        return world.validate(oot, mm, mmStart, goal, access);
    };
    const auto rows = Shuffler(settings, std::move(bindings)).run();
    return EncodeMmMixedExtension(rows, settings);
}
} // namespace ComboRando::Entrances
