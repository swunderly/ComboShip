#pragma once
#ifdef COMBO_BUILD
#include "entrance.h"
#include <vector>

namespace Rando::ComboEntrances {
std::vector<Entrance> OverlayExits(RandomizerRegion region);
int SetLayout(const char* seedJson);
const char* DumpLayout();
int SetAssumptions(const char* candidateJson);
void ClearAssumptions();
const char* AreaContext();
bool HasVanillaWallmasterLayout();
} // namespace Rando::ComboEntrances
#endif
