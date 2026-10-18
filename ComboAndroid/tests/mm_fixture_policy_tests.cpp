#include "rando/entrances/MmFixturePolicy.h"
#include <cassert>

int main() {
  namespace F = ComboRando::Entrances::Fixture;
  assert(F::CanUse(2, true, true, true, true));
  for (int file : {-1, 0, 1, 3, 0xFE, 0xFF})
    assert(!F::CanUse(file, true, true, true, true));
  for (int flags = 0; flags < 15; ++flags)
    assert(!F::CanUse(2, flags & 1, flags & 2, flags & 4, flags & 8));
  for (auto invalid :
       {"", "0", "2", "3", "4", "6", "8", "-1", "01", "1x", "1 ", "2147483648"})
    assert(!F::FindWellRoom(invalid));
  for (const auto &expected : F::kWellRooms) {
    const auto *room = F::FindWellRoom(expected.argument);
    assert(room && (room->room == 1 || room->room == 5 || room->room == 7));
    assert(room->x == expected.x && room->z == expected.z);
  }
}
