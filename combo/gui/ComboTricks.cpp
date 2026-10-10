#include "ComboTricks.h"

namespace ComboRando {
namespace {
Detail::TrickCatalogueFocus sPendingFocus;
} // namespace

void FocusTrick(const char* name) {
    sPendingFocus.Set(name);
}

bool ApplyTrickFocus(char (&query)[160], int& world, int& status, int& skill) {
    return sPendingFocus.Apply(query, world, status, skill);
}
} // namespace ComboRando
