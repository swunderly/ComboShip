/*
Adapted from OoTMM 41a2b7c0d7dbbb53fed928652f8271bc336c113f
packages/generator/src/common/sr.c and src/oot/actors/En/En_G_Switch.c
MIT License

Copyright (c) 2020-2022 OoTMM Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <array>
#include <cstdint>
#include <optional>
namespace Rando::SilverRupees {
constexpr size_t PuzzleCount = 18;
// The GTG slopes puzzle also clears room 2 when solved, as the native silver rupee tracker does.
constexpr size_t GtgSlopesSlot = 11;
struct Puzzle {
    uint8_t scene, flag, count;
};
// Scene IDs are the native scene_table.h indices, never ROM memory addresses.
inline constexpr std::array<Puzzle, PuzzleCount> Vanilla = { { { 1, 0, 0 },
                                                               { 8, 0x1f, 5 },
                                                               { 6, 5, 5 },
                                                               { 6, 0x0a, 5 },
                                                               { 6, 2, 5 },
                                                               { 7, 1, 5 },
                                                               { 7, 0, 0 },
                                                               { 7, 9, 5 },
                                                               { 7, 8, 5 },
                                                               { 9, 8, 5 },
                                                               { 9, 9, 5 },
                                                               { 11, 0x1c, 5 },
                                                               { 11, 0x0c, 5 },
                                                               { 11, 0x1b, 5 },
                                                               { 13, 0x0b, 5 },
                                                               { 13, 0x12, 5 },
                                                               { 13, 9, 5 },
                                                               { 13, 0x0e, 5 } } };
inline constexpr std::array<Puzzle, PuzzleCount> MasterQuest = { { { 1, 0x25, 5 },
                                                                   { 8, 0, 0 },
                                                                   { 6, 0x37, 5 },
                                                                   { 6, 0, 5 },
                                                                   { 6, 0, 0 },
                                                                   { 7, 1, 5 },
                                                                   { 7, 3, 10 },
                                                                   { 7, 0x11, 5 },
                                                                   { 7, 8, 10 },
                                                                   { 9, 0, 0 },
                                                                   { 9, 0, 0 },
                                                                   { 11, 0x1c, 5 },
                                                                   { 11, 0x0c, 6 },
                                                                   { 11, 0x1b, 3 },
                                                                   { 13, 0x0b, 5 },
                                                                   { 13, 2, 5 },
                                                                   { 13, 1, 5 },
                                                                   { 13, 0, 0 } } };
constexpr Puzzle GetPuzzle(size_t slot, bool mq) {
    return (mq ? MasterQuest : Vanilla).at(slot);
}
// Physical state is supplied separately; a received pouch never mutates it.
struct ReceivedState {
    std::array<uint8_t, PuzzleCount> counts{};
};
struct Grant {
    uint8_t count;
    uint16_t rupees;
    bool permanentSwitch;
};
inline Grant Receive(ReceivedState& received, size_t slot, bool mq, bool pouch,
                     std::optional<uint8_t> activeScene = std::nullopt) {
    const auto puzzle = GetPuzzle(slot, mq);
    const unsigned amount = pouch ? puzzle.count : 1;
    const auto total = unsigned(received.counts.at(slot)) + amount;
    const auto count = static_cast<uint8_t>(total > puzzle.count ? puzzle.count : total);
    received.counts.at(slot) = count;
    const bool solved = puzzle.count && count == puzzle.count;
    return { count, static_cast<uint16_t>(5 * amount),
             solved && puzzle.flag < 0x20 && (!activeScene || *activeScene != puzzle.scene) };
}
inline std::optional<uint16_t> PhysicalId(uint8_t scene, int room, unsigned local, bool mq) {
    unsigned base;
    switch (scene) {
        case 1:
            base = 0;
            break;
        case 8:
            base = 5;
            break;
        case 6:
            switch (room) {
                case 0:
                case 2:
                    base = 0xa;
                    break;
                case 8:
                case 0x17:
                    base = 0xf;
                    break;
                case 0xd:
                    base = 0x14;
                    break;
                default:
                    return {};
            }
            break;
        case 7:
            switch (room) {
                case 6:
                    base = 0x19;
                    break;
                case 0x10:
                    base = 0x1e;
                    break;
                case 9:
                    base = 0x28;
                    break;
                case 0xb:
                    base = 0x2d;
                    break;
                default:
                    return {};
            }
            break;
        case 9:
            switch (room) {
                case 3:
                    base = 0x37;
                    break;
                case 5:
                    base = 0x3c;
                    break;
                default:
                    return {};
            }
            break;
        case 11:
            switch (room) {
                case 2:
                    base = 0x41;
                    break;
                case 6:
                    base = 0x46;
                    break;
                case 9:
                    base = 0x4c;
                    break;
                default:
                    return {};
            }
            break;
        case 13:
            switch (room) {
                case 0xc:
                case 0x11:
                    base = 0x51;
                    break;
                case 3:
                case 8:
                    base = 0x56;
                    break;
                case 0xe:
                    base = 0x5b;
                    break;
                case 6:
                    base = 0x60;
                    break;
                default:
                    return {};
            }
            break;
        default:
            return {};
    }
    if (base + local >= 0x80)
        return {};
    // Callers look the id up in SilverRupeeData.inc; layouts without that rupee have no check.
    return static_cast<uint16_t>((base + local) | (mq ? 0x80 : 0));
}
} // namespace Rando::SilverRupees
