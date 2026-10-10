#pragma once
#include <algorithm>
#include <string>
#include <vector>
namespace ComboRando {
inline bool IsOcarinaName(const std::string& name) {
    return name == "Progressive Ocarina" || name == "Fairy Ocarina" || name == "Ocarina of Time";
}
inline int OwnedOcarinaTier(const std::vector<std::string>& owned) {
    int progressive = 0, tier = 0;
    for (const auto& name : owned) {
        if (name == "Progressive Ocarina")
            ++progressive;
        else if (name == "Ocarina of Time")
            tier = 2;
        else if (name == "Fairy Ocarina")
            tier = std::max(tier, 1);
    }
    return std::max(tier, std::min(progressive, 2));
}
inline void RaiseOwnedOcarina(std::vector<std::string>& owned, int tier) {
    if (tier > OwnedOcarinaTier(owned))
        owned.push_back(tier >= 2 ? "Ocarina of Time" : "Fairy Ocarina");
}
// Pinned item_add.c: OOT Fairy only crosses when MM Fairy is enabled; MM always crosses.
inline void MirrorSharedOcarina(std::vector<std::string>& oot, std::vector<std::string>& mm, bool fairyMm) {
    const int o = OwnedOcarinaTier(oot), m = OwnedOcarinaTier(mm);
    RaiseOwnedOcarina(oot, m);
    if (o >= 2 || fairyMm)
        RaiseOwnedOcarina(mm, o);
}
} // namespace ComboRando
