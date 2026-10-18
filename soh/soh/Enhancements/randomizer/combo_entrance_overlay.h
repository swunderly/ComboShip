#pragma once
#ifdef COMBO_BUILD
#include "entrance.h"
#include <vector>
#include <string_view>

namespace Rando::ComboEntrances {
std::vector<Entrance> OverlayExits(RandomizerRegion region);
int SetLayout(const char* seedJson);
const char* DumpLayout();
int SetAssumptions(const char* candidateJson);
void ClearAssumptions();
const char* AreaContext();
bool HasVanillaWallmasterLayout();
bool VanillaWallmasterContext();
// Value-only factory for the Temple probe: no installed layout or assumptions are read.
std::vector<Entrance> WallmasterCandidateExits(std::string_view source, std::string_view destination);
} // namespace Rando::ComboEntrances
#endif
