#pragma once
#include "ComboRandomizerProfiles.h"
#include "rando/KokiriSeedRevision.h"
#include <filesystem>
#include <fstream>
namespace ComboRando::Profiles {
inline Json SeedFromContainer(const Json& container, const Entrances::CapabilitySet& caps) {
    if (!container.is_object())
        throw std::invalid_argument("Saved file container is malformed");
    if (!container.contains("combo"))
        return Json();
    const auto& combo = container["combo"];
    if (!combo.is_object())
        throw std::invalid_argument("Saved file metadata is malformed");
    if (!combo.contains("rando") || combo["rando"].is_null())
        return Json();
    auto seed = combo["rando"];
    if (seed.is_string())
        seed = Json::parse(seed.get<std::string>());
    RequireSupportedSeedCapabilities(seed, caps);
    if (!SeedDocument(seed))
        throw std::invalid_argument("Saved randomizer seed is malformed");
    return seed;
}
// Native reward/flag identities and the authored seed must describe the same
// catalog. Absence is exactly legacy0; no profile/current setting may supply it.
inline void ValidateSavedCatalogs(const Json& container, const Entrances::CapabilitySet& caps) {
    if (container.is_null())
        return;
    if (!container.is_object())
        throw std::invalid_argument("Saved container is malformed");
    const Json::json_pointer path("/oot/sections/randomizer/data");
    const auto* data = container.contains(path) ? &container.at(path) : nullptr;
    const auto native = data ? ReadKokiriCatalogRevisions(*data) : KokiriCatalogRevisions{ 0, 0 };
    const auto seed = SeedFromContainer(container, caps);
    if (seed.is_null())
        return;
    const auto authored = ReadKokiriCatalogRevisions(seed.value("oot", Json::object()));
    if (data && (native.invisibleRupees != authored.invisibleRupees || native.grass != authored.grass))
        throw UnsupportedKokiriSeedRevision("Saved seed and native Kokiri check catalogs are incompatible");
}
// The runtime flags of a container's seed, once ValidateSavedCatalogs has accepted the container.
inline uint32_t ContainerEntranceRuntimeFlags(const Json& container, const Entrances::CapabilitySet& caps) {
    return container.is_object() ? SeedEntranceRuntimeFlags(SeedFromContainer(container, caps)) : 0u;
}
inline Json ReadSavedContainerFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path))
        return Json();
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot read saved settings");
    Json container;
    input >> container;
    return container;
}
inline Json ReadSavedSeedFile(const std::filesystem::path& path, const Entrances::CapabilitySet& caps) {
    const auto container = ReadSavedContainerFile(path);
    ValidateSavedCatalogs(container, caps);
    return container.is_null() ? Json() : SeedFromContainer(container, caps);
}
inline Json NativeSeedRulesFromContainer(const Json& container, const Entrances::CapabilitySet& caps) {
    Json result = Json::object();
    if (container.is_null())
        return result;
    if (!container.is_object())
        throw std::invalid_argument("Saved container is malformed");
    ValidateSavedCatalogs(container, caps);
    const auto* data = container.contains(Json::json_pointer("/oot/sections/randomizer/data"))
                           ? &container.at(Json::json_pointer("/oot/sections/randomizer/data"))
                           : nullptr;
    if (data && data->is_object())
        for (const char* field :
             { "randoSettings", "requiredTrials", "trickOptions", "masterQuestDungeonCount", "masterQuestDungeons",
               "entrances", "kokiriInvisibleRupeeRevision", "kokiriGrassCatalogRevision" })
            if (data->contains(field))
                result["oot"][field] = data->at(field);
    if (data && data->is_object()) {
        const auto catalog = ReadKokiriCatalogRevisions(*data);
        result["oot"][kKokiriSeedRevision] = catalog.invisibleRupees;
        result["oot"][kKokiriGrassCatalogRevision] = catalog.grass;
    }
    for (const char* cycle : { "newCycleSave", "owlSave" }) {
        const Json::json_pointer path(std::string("/mm/") + cycle + "/save/shipSaveInfo/rando/randoSaveOptions");
        if (container.contains(path))
            result["mm"][cycle]["randoSaveOptions"] = container.at(path);
    }
    return result;
}
} // namespace ComboRando::Profiles
