// Real full-pool MM generation. Public capability stays gated pending runtime proof.
#pragma once
#include "rando/entrances/LegacyEnvelope.h"
#include "rando/entrances/NativeBridge.h"
#include "rando/entrances/MmGrottoGraph.h"
#include "rando/entrances/MmOrdinaryAreas.h"
namespace ComboRando::Entrances {
inline nlohmann::json EncodeMmGrottoExtension(const Overrides& rows, bool noPolarity = false) {
    nlohmann::json value = { { "schemaVersion", 1 },
                             { "profile", "mm-grotto-coupled-v1" },
                             { "upstreamCommit", kLegacyUpstreamCommit },
                             { "registrySha256", kLegacyRegistrySourceSha256 },
                             { "noPolarity", noPolarity },
                             { "overrides", nlohmann::json::array() } };
    for (const auto& [src, dst] : rows) {
        if (IsMmGrottoClockTowerReservation(src, dst))
            continue;
        if (!MmGrottoBound(src) || !MmGrottoBound(dst))
            throw std::invalid_argument("Unsupported generated MM grotto row");
        value["overrides"].push_back({ { "source", src }, { "destination", dst } });
    }
    MmGrottoLayout proof;
    proof.install(value);
    return value;
}
inline void ApplyNativeMmGrottos(const NativeFns& native, const nlohmann::json& extension) {
    MmGrottoLayout proof;
    proof.install(extension);
    if (!native.applySeed || !native.dumpSeed)
        throw std::runtime_error("MM grotto seed exports unavailable");
    const auto* snapshot = native.dumpSeed();
    if (!snapshot)
        throw std::runtime_error("MM grotto seed snapshot unavailable");
    auto current = nlohmann::json::parse(snapshot);
    current["grottoEntranceExpansion"] = extension;
    if (!native.applySeed(current.dump().c_str()))
        throw std::runtime_error("MM grotto seed rejected");
    const auto* dump = native.dumpSeed();
    if (!dump)
        throw std::runtime_error("MM grotto engine roundtrip unavailable");
    const auto loaded = nlohmann::json::parse(dump);
    if (!loaded.contains("grottoEntranceExpansion") || loaded.at("grottoEntranceExpansion") != extension)
        throw std::runtime_error("MM grotto seed failed actual engine roundtrip");
}
class MmGrottoAssumptionScope {
    NativeFns native;

  public:
    MmGrottoAssumptionScope(NativeFns f, const Overrides& rows, const Pools& pools, const Settings& settings)
        : native(f) {
        if (!native.setAssumptions || !native.clearAssumptions)
            throw std::runtime_error("MM grotto assumption exports unavailable");
        if (!native.setAssumptions(EncodeMmGrottoAssumptions(rows, pools, settings).dump().c_str()))
            throw std::runtime_error("MM grotto assumptions rejected");
    }
    ~MmGrottoAssumptionScope() {
        native.clearAssumptions();
    }
    MmGrottoAssumptionScope(const MmGrottoAssumptionScope&) = delete;
};
inline nlohmann::json GenerateNativeMmGrottos(Settings settings, uint32_t seed, const NativeFns& native,
                                              const std::string& ootDump, const std::string& mmDump,
                                              const std::string& forcedOot, const OracleFns& oot, const OracleFns& mm,
                                              bool mmStart, CwGoal goal) {
    if (settings.erGrottos == Mode::None)
        return nlohmann::json();
    if (settings.games != Games::Mm || settings.erGrottos != Mode::OwnGame || settings.erDecoupled ||
        settings.erBoss != Mode::None || settings.erDungeons != Mode::None || settings.erRegions != Mode::None ||
        settings.erOverworld != Mode::None || settings.erIndoors != Mode::None || settings.erMixed != Mode::None ||
        settings.erWallmasters != Mode::None || settings.erOneWays != Mode::None || settings.erWarps != Warps::None ||
        settings.erSpawns != Spawns::None)
        throw std::invalid_argument("MM grotto generator requires the complete coupled default-US own-game pool");
    NativeScope restore(native);
    AssumedWorld world(ootDump, mmDump, forcedOot);
    CwRng rng(CwHashName("entrances:mm-grotto-coupled-v1:" + std::to_string(seed)));
    Bindings bindings;
    bindings.areaExists = [](std::string_view area) {
        return MmOrdinaryNativeAreaExists(area) || MmGrottoNativeAreaExists(area);
    };
    bindings.sourceSupported = [](const Record& record) { return MmGrottoBound(record.name); };
    bindings.destinationSupported = bindings.sourceSupported;
    bindings.chooseIndex = [&](size_t count) {
        CheckGenerationCancellation();
        return rng.below(static_cast<uint32_t>(count));
    };
    bindings.validateAssumed = [&](const Overrides& rows, const Pools& pools) {
        MmGrottoAssumptionScope scope(native, rows, pools, settings);
        return world.validate(oot, mm, mmStart, goal);
    };
    bindings.validateComplete = [&](const Overrides& rows) {
        const auto extension = EncodeMmGrottoExtension(rows, settings.erNoPolarity);
        if (extension.at("overrides").size() != kMmGrottoGraphBindings.size())
            throw std::runtime_error("Incomplete default-US MM grotto pool");
        ApplyNativeMmGrottos(native, extension);
        return world.validate(oot, mm, mmStart, goal);
    };
    const auto rows = Shuffler(settings, std::move(bindings)).run();
    return EncodeMmGrottoExtension(rows, settings.erNoPolarity);
}
} // namespace ComboRando::Entrances
