#pragma once
#include "EntranceSpoiler.h"
#include "rando/ComboSeedFormat.h"
#include <filesystem>
#include <fstream>

namespace ComboRando::Entrances {
// Recorded routes only: never generate a layout while displaying a spoiler. The seed guard runs
// before any file is opened, including in release builds which admit no unified layout yet.
inline std::string EntranceSpoilerForSeed(const nlohmann::json& seed, const CapabilitySet& caps) {
    RequireSupportedSeedDocument(seed, caps);
    if (!seed.contains("entranceLayout"))
        return {};
    const auto layout = Validate(DecodeEntranceLayout(seed.at("entranceLayout")), caps);
    return FormatEntranceSpoiler(&layout);
}
// Keep the consolidated JSON schema and bytes unchanged. A unified seed gets the pinned text
// section in a sibling .entrances.txt file. After the caller has successfully written its JSON,
// a legacy/no-layout seed removes only that derived sidecar left by an earlier layout seed.
inline bool WriteEntranceSpoilerFile(const std::filesystem::path& seedPath, const nlohmann::json& seed,
                                     const CapabilitySet& caps) {
    const auto text = EntranceSpoilerForSeed(seed, caps);
    if (seedPath.empty())
        return true;
    if (seedPath.extension() != ".json")
        throw std::invalid_argument("Entrance spoiler needs a JSON seed path");
    auto path = seedPath;
    path.replace_extension(".entrances.txt");
    if (text.empty()) {
        std::error_code error;
        const auto status = std::filesystem::symlink_status(path, error);
        if (error == std::errc::no_such_file_or_directory)
            return true;
        if (error)
            return false;
        if (!std::filesystem::exists(status))
            return true;
        // Do not remove directories or follow links: only the generated sibling file is ours.
        if (!std::filesystem::is_regular_file(status))
            return false;
        std::filesystem::remove(path, error);
        return !error;
    }
    std::ofstream out(path, std::ios::trunc | std::ios::binary);
    out << text;
    out.close();
    return bool(out);
}
} // namespace ComboRando::Entrances
