#pragma once
#ifdef COMBO_BUILD
#include "entrance.h"
#include <vector>
#include <string_view>

namespace Rando::ComboEntrances {
std::vector<Entrance> OverlayExits(RandomizerRegion region);
// Full Wallmasters: the region's audited edges into MM, with their MM destination bits (CrossWallmasters.h).
std::vector<std::pair<Entrance, int>> ForeignOverlayExits(RandomizerRegion region);
// The entryway region of an OoT Wallmaster arrival bit.
RandomizerRegion WallmasterArrivalRegion(int bit);
int SetLayout(const char* seedJson);
const char* DumpLayout();
int SetAssumptions(const char* candidateJson);
void ClearAssumptions();
const char* AreaContext();
bool HasWallmasterLayout();
// The loaded dungeons' quests are ones this build's Wallmaster capabilities admit (Master Quest needs the quests id).
bool WallmasterQuestContextAdmitted();
// Value-only factory for the Temple probe: no installed layout or assumptions are read.
std::vector<Entrance> WallmasterCandidateExits(std::string_view source, std::string_view destination);
} // namespace Rando::ComboEntrances
#endif
