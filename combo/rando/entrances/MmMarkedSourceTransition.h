#pragma once
#include <cstdint>
#include <optional>
namespace ComboRando::Entrances {
// A native source hook marks intent; only completed transitions consume it.
// Owner/file/unchanged source checks prevent stale scene/save and return lookups.
class MmMarkedSourceTransition {
    const void* owner = nullptr;
    int file = -1;
    uint16_t source = 0;

  public:
    void clear() {
        owner = nullptr;
        file = -1;
        source = 0;
    }
    void mark(const void* play, int slot, uint16_t nativeSource) {
        clear();
        if (!play || slot < 0 || slot > 2)
            return;
        owner = play;
        file = slot;
        source = nativeSource;
    }
    std::optional<uint16_t> take(const void* play, int slot, uint16_t nativeSource) {
        const auto savedOwner = owner;
        const auto savedFile = file;
        const auto savedSource = source;
        clear(); // pinned boolean marker is consumed before any override lookup
        if (!savedOwner || play != savedOwner || slot != savedFile || nativeSource != savedSource)
            return std::nullopt;
        return savedSource;
    }
};
} // namespace ComboRando::Entrances
