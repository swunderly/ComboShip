#pragma once
namespace Rando {
// Publish families independently after upstream policy and native logic validation.
inline constexpr bool ExpandedSoulsIntegrationReady = true;
inline constexpr bool AnimalSoulsIntegrationReady = true;
inline constexpr bool MiscSoulsIntegrationReady = true;
inline constexpr bool ExpandedSoulAvailable(unsigned index) {
    return index < 4 ? AnimalSoulsIntegrationReady : index < 6 ? MiscSoulsIntegrationReady
        : index < 59 && ExpandedSoulsIntegrationReady;
}
}

