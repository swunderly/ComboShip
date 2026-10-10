#pragma once
#include <algorithm>
#include <string>

namespace ComboRando {
namespace Detail {
// Search navigation carries a one-shot focus without changing any settings.
class TrickCatalogueFocus {
  public:
    void Set(const char* name) { pending = name ? name : ""; }
    template <size_t N>
    bool Apply(char (&query)[N], int& world, int& status, int& skill) {
        static_assert(N > 0);
        if (pending.empty()) return false;
        const auto length = std::min(pending.size(), N - 1);
        std::copy_n(pending.data(), length, query);
        query[length] = '\0';
        world = status = skill = 0;
        pending.clear();
        return true;
    }
  private:
    std::string pending;
};
}
void FocusUnifiedTrick(const char* name);
void DrawUnifiedTricks();
}
