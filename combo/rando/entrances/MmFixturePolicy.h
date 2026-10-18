#pragma once
#include <array>
#include <string_view>

namespace ComboRando::Entrances::Fixture {
struct WellRoom {
    std::string_view argument;
    int room;
    int x;
    int z;
};
// Authored Wallmaster XZ positions, verified against the two-scene metadata export.
// Y is deliberately not copied from these ceiling actors; native gravity/floor
// collision must establish grounding after the ordinary room respawn.
inline constexpr std::array<WellRoom, 3> kWellRooms = { WellRoom{ "1", 1, 1170, -30 }, WellRoom{ "5", 5, -150, -2370 },
                                                        WellRoom{ "7", 7, 1050, -2430 } };
inline const WellRoom* FindWellRoom(std::string_view argument) {
    for (const auto& room : kWellRooms)
        if (argument == room.argument)
            return &room;
    return nullptr;
}
inline bool CanUse(int fileNum, bool normalGame, bool rando, bool unifiedWallmasters, bool stablePlay) {
    return fileNum == 2 && normalGame && rando && unifiedWallmasters && stablePlay;
}
} // namespace ComboRando::Entrances::Fixture
