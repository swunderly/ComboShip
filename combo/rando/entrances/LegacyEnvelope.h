// Identity of the four shipped MM-only v1 entrance profiles and their oracle assumptions.
// This historical digest includes CRLF checkout bytes. Never recompute it from a new registry:
// existing saved layouts must retain this exact identity independently of the current source pin.
#pragma once
#include <string_view>

namespace ComboRando::Entrances {
inline constexpr std::string_view kLegacyUpstreamCommit = "41a2b7c0d7dbbb53fed928652f8271bc336c113f";
inline constexpr std::string_view kLegacyRegistrySourceSha256 =
    "d0bd7991f31736ac7d98e49de76734890a9827f02efedff902aaba98db66318b";
} // namespace ComboRando::Entrances
