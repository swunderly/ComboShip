#pragma once

#include <cstdint>

namespace ComboRando {

// OoTMM v32.3 macros_mm.yml: possession and renewable supply are separate.
// Search-local state deliberately stays out of the persistent save structure.
enum MmResource : uint32_t {
    MmRupees = 1u << 0, MmArrows = 1u << 1, MmBombs = 1u << 2,
    MmMagic = 1u << 3, MmSticks = 1u << 4, MmNuts = 1u << 5,
    MmBombchus = 1u << 6, MmHotWaterNorth = 1u << 7, MmChateau = 1u << 8,
};

struct MmResourceAccess {
    uint32_t sources = 0;
    bool active = false;

    bool Add(uint32_t incoming) {
        const auto previous = sources;
        sources |= incoming;
        return sources != previous;
    }
    bool Has(MmResource resource) const { return !active || (sources & resource) != 0; }
    bool CanPay(unsigned price, unsigned capacity) const {
        return price == 0 || (price <= capacity && Has(MmRupees));
    }
    bool CanUse(bool owned, MmResource resource) const { return owned && Has(resource); }
};

inline thread_local MmResourceAccess mmResourceAccess;

class MmResourceScope {
    MmResourceAccess previous;
public:
    MmResourceScope() : previous(mmResourceAccess) { mmResourceAccess = {0, true}; }
    ~MmResourceScope() { mmResourceAccess = previous; }
    MmResourceScope(const MmResourceScope&) = delete;
    MmResourceScope& operator=(const MmResourceScope&) = delete;
};

} // namespace ComboRando
